#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <glib.h>

static gchar *host = "127.0.0.1";
static gchar *port = "2929";
static gint n_concurrent_connections = 10000;

static GOptionEntry entries[] =
{
  {"host", 0, 0, G_OPTION_ARG_STRING, &host, "Host to connect", "HOST"},
  {"port", 0, 0, G_OPTION_ARG_STRING, &port, "Port to connect", "PORT"},
  {"n-concurrent-connections", 0, 0, G_OPTION_ARG_INT, &n_concurrent_connections,
   "The number of concurrent connections", "N"},
  {NULL}
};

typedef struct {
  int epoll_fd;
  struct addrinfo *addresses;
  gint n_connections;
  GTimer *timer;
} Context;

typedef struct {
  int socket_fd;
  Context *context;
} Session;

static void
report_statistic(Context *context)
{
  gdouble elapsed_time;
  gdouble throughput;

  elapsed_time = g_timer_elapsed(context->timer, NULL);
  throughput = context->n_connections / elapsed_time;
  g_print("%10.3f messages/s\n", throughput);

  context->n_connections = 0;
  g_timer_start(context->timer);
}

static gboolean
try_connect(Context *context)
{
  struct addrinfo *address = context->addresses;
  int socket_fd = 0;

  socket_fd = socket(address->ai_family,
                     address->ai_socktype,
                     address->ai_protocol);
  if (socket_fd == -1) {
    perror("failed to socket()");
    return FALSE;
  }

  if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) == -1) {
    perror("failed to fcntl(F_SETFL, O_NONBLOCK)");
    close(socket_fd);
    return FALSE;
  }

  if (connect(socket_fd,
              address->ai_addr,
              address->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      struct epoll_event event;
      Session *session;

      session = g_new(Session, 1);
      session->socket_fd = socket_fd;
      session->context = context;

      event.events = EPOLLOUT;
      event.data.ptr = session;
      if (epoll_ctl(context->epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        perror("failed to epoll_ctl(EPOLL_CTL_ADD, socket_fd)");
        close(socket_fd);
        return FALSE;
      }

      return TRUE;
    } else {
      perror("failed to connect()");
      close(socket_fd);
      return FALSE;
    }
  }

  close(socket_fd);
  context->n_connections++;
  g_print("not block\n");
  return FALSE;
}

static gboolean
connect_again(Session *session)
{
  Context *context = session->context;
  struct addrinfo *address = context->addresses;
  int socket_fd = session->socket_fd;
  gboolean succeeded = TRUE;

  if (epoll_ctl(context->epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL) == -1) {
    perror("failed to epoll_ctl(EPOLL_CTL_DEL, socket_fd)");
    succeeded = FALSE;
    goto exit;
  }

  if (connect(socket_fd,
              address->ai_addr,
              address->ai_addrlen) == -1) {
    perror("failed to connect() again");
    succeeded = FALSE;
    goto exit;
  }

  context->n_connections++;

exit:
  close(socket_fd);
  g_free(session);

  return succeeded;
}

int
main(int argc, char **argv)
{
  Context context;

  {
    GOptionContext *context;
    GError *error = NULL;

    context = g_option_context_new("client side implementation by thread");
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
      g_print("failed to parse options: %s\n", error->message);
      g_error_free(error);
      g_option_context_free(context);
      return EXIT_FAILURE;
    }
    g_option_context_free(context);
  }

  {
    struct addrinfo hints;
    int error;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    error = getaddrinfo(host, port, &hints, &(context.addresses));
    if (error != 0) {
      g_print("failed to getaddrinfo(): %s\n", g_strerror(error));
      return EXIT_FAILURE;
    }
  }

  {
    gint i;
    int not_used_size = 1;
#define MAX_EVENTS 10
    struct epoll_event events[MAX_EVENTS];
    int timer_fd;

    context.epoll_fd = epoll_create(not_used_size);
    if (context.epoll_fd == -1) {
      perror("failed to epoll_create()");
      return EXIT_FAILURE;
    }

    context.n_connections = 0;
    context.timer = g_timer_new();

    {
      struct itimerspec timer_spec;
      struct epoll_event event;

      timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
      if (timer_fd == -1) {
        perror("failed to timerfd_create()");
        return EXIT_FAILURE;
      }

      timer_spec.it_interval.tv_sec = 1;
      timer_spec.it_interval.tv_nsec = 0;
      timer_spec.it_value.tv_sec = 1;
      timer_spec.it_value.tv_nsec = 0;
      if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        perror("failed to timerfd_settime()");
        return EXIT_FAILURE;
      }

      event.events = EPOLLIN | EPOLLPRI;
      event.data.fd = timer_fd;
      if (epoll_ctl(context.epoll_fd, EPOLL_CTL_ADD, timer_fd, &event) == -1) {
        perror("failed to epoll_ctl(timer_fd)");
        return EXIT_FAILURE;
      }
    }

    for (i = 0; i < n_concurrent_connections; i++) {
      if (!try_connect(&context)) {
        return EXIT_FAILURE;
      }
    }

    while (TRUE) {
      int i, n_events;

      n_events = epoll_wait(context.epoll_fd, events, MAX_EVENTS, -1);
      if (n_events == -1) {
        perror("failed to epoll_wait()");
        return EXIT_FAILURE;
      }

      for (i = 0; i < n_events; i++) {
        struct epoll_event *event = &(events[i]);

        if (event->data.fd == timer_fd) {
          guint64 n_expirations;
          if (read(event->data.fd, &n_expirations, sizeof(guint64)) == -1) {
            perror("failed to read(timer_fd)");
            return EXIT_FAILURE;
          }
          report_statistic(&context);
          continue;
        }

        {
          Session *session = event->data.ptr;
          if (connect_again(session)) {
            if (!try_connect(&context)) {
              return EXIT_FAILURE;
            }
          } else {
            return EXIT_FAILURE;
          }
        }
      }
    }

    close(timer_fd);
#undef MAX_EVENTS
  }

  freeaddrinfo(context.addresses);

  return EXIT_SUCCESS;
}
