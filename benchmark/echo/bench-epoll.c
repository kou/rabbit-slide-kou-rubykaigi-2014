#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <glib.h>

static gsize message_size = 1024;

static gchar *host = "127.0.0.1";
static gchar *port = "22929";
static gint n_concurrent_connections = 10000;

#define CHUNK_SIZE 8192

static GOptionEntry entries[] =
{
  {"host", 0, 0, G_OPTION_ARG_STRING, &host, "Host to connect", "HOST"},
  {"port", 0, 0, G_OPTION_ARG_STRING, &port, "Port to connect", "PORT"},
  {"message-size", 0, 0, G_OPTION_ARG_INT, &message_size,
   "The size of a message", "SIZE"},
  {"n-concurrent-connections", 0, 0, G_OPTION_ARG_INT, &n_concurrent_connections,
   "The number of concurrent connections", "N"},
  {NULL}
};

typedef struct {
  int epoll_fd;
  struct addrinfo *addresses;
  GString *message;
  gint n_sent_messages;
  gint n_finished_messages;
  GTimer *timer;
} Context;

typedef struct {
  int socket_fd;
  gboolean connected;
  gsize sent_message_size;
  gsize received_message_size;
  Context *context;
} Session;

static void
report_statistic(Context *context)
{
  gdouble elapsed_time;
  gdouble sent_throughput;
  gdouble finished_throughput;

  elapsed_time = g_timer_elapsed(context->timer, NULL);
  sent_throughput = context->n_sent_messages / elapsed_time;
  finished_throughput = context->n_finished_messages / elapsed_time;
  g_print("%10.3f sent-messages/s\t"
          "%10.3f finished-messages/s\n",
          sent_throughput,
          finished_throughput);

  context->n_sent_messages = 0;
  context->n_finished_messages = 0;
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
      session->connected = FALSE;
      session->sent_message_size = 0;
      session->received_message_size = 0;
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

  {
    struct epoll_event event;
    Session *session;

    session = g_new(Session, 1);
    session->socket_fd = socket_fd;
    session->connected = TRUE;
    session->sent_message_size = 0;
    session->received_message_size = 0;
    session->context = context;

    event.events = EPOLLIN | EPOLLPRI | EPOLLOUT;
    event.data.ptr = session;
    if (epoll_ctl(context->epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
      perror("failed to epoll_ctl(EPOLL_CTL_ADD, socket_fd)");
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
connect_again(Session *session)
{
  Context *context = session->context;
  struct addrinfo *address = context->addresses;
  int socket_fd = session->socket_fd;

  if (connect(socket_fd,
              address->ai_addr,
              address->ai_addrlen) == -1) {
    perror("failed to connect() again");
    close(socket_fd);
    g_free(session);
    return FALSE;
  }

  {
    struct epoll_event event;

    session->connected = TRUE;
    event.events = EPOLLIN | EPOLLPRI | EPOLLOUT;
    event.data.ptr = session;
    if (epoll_ctl(context->epoll_fd, EPOLL_CTL_MOD, socket_fd, &event) == -1) {
      perror("failed to epoll_ctl(EPOLL_CTL_MOD, connected socket_fd)");
      close(socket_fd);
      g_free(session);
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
send_message(Session *session)
{
  Context *context = session->context;

  {
    size_t written_size;
    written_size = write(session->socket_fd,
                         context->message->str + session->sent_message_size,
                         context->message->len - session->sent_message_size);
    if (written_size == -1) {
      perror("failed to write()");
      return FALSE;
    }
    session->sent_message_size += written_size;
  }

  if (session->sent_message_size == context->message->len) {
    context->n_sent_messages++;
    session->sent_message_size = 0;
    {
      int socket_fd = session->socket_fd;
      struct epoll_event event;

      event.events = EPOLLIN | EPOLLPRI;
      event.data.ptr = session;
      if (epoll_ctl(context->epoll_fd, EPOLL_CTL_MOD, socket_fd, &event) == -1) {
        perror("failed to epoll_ctl(EPOLL_CTL_MOD, -EPOLLOUT)");
        return FALSE;
      }
    }
  }

  return TRUE;
}

static gboolean
receive_message(Session *session)
{
  Context *context = session->context;

  {
    size_t read_size;
    char buffer[CHUNK_SIZE];
    read_size = read(session->socket_fd, buffer, sizeof(buffer));
    if (read_size == -1) {
      perror("failed to read()");
      return FALSE;
    }
    session->received_message_size += read_size;
  }

  if (session->received_message_size == context->message->len) {
    context->n_finished_messages++;
    session->received_message_size = 0;
    {
      int socket_fd = session->socket_fd;
      struct epoll_event event;

      event.events = EPOLLIN | EPOLLPRI | EPOLLOUT;
      event.data.ptr = session;
      if (epoll_ctl(context->epoll_fd, EPOLL_CTL_MOD, socket_fd, &event) == -1) {
        perror("failed to epoll_ctl(EPOLL_CTL_MOD, +EPOLLOUT)");
        return FALSE;
      }
    }
  }

  return TRUE;
}

int
main(int argc, char **argv)
{
  Context context;

  {
    GOptionContext *context;
    GError *error = NULL;

    context = g_option_context_new("benchmark implementation by epoll");
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
    gsize i;
    context.message = g_string_sized_new(message_size);
    for (i = 0; i < message_size; i++) {
      g_string_append_c(context.message, 'X');
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

    context.n_finished_messages = 0;
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
          if (event->events & EPOLLOUT && !session->connected) {
            if (connect_again(session)) {
              continue;
            } else {
              return EXIT_FAILURE;
            }
          }

          if (event->events & (EPOLLIN | EPOLLPRI)) {
            if (!receive_message(session)) {
              return EXIT_FAILURE;
            }
          }
          if (event->events & EPOLLOUT) {
            if (!send_message(session)) {
              return EXIT_FAILURE;
            }
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
