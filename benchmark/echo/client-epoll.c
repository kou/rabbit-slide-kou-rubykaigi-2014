#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <glib.h>

static gchar message[] =
  "{\n"
  "  \"name\": \"Alice\",\n"
  "  \"nick\": \"alice\",\n"
  "  \"age\": 14\n"
  "}\n";

static gchar *host = "127.0.0.1";
static gchar *port = "22929";
static gint n_requests                 = 1000;
static gint max_concurrent_connections =  500;
static gint n_messages                 =  100;

static GOptionEntry entries[] =
{
  {"host", 0, 0, G_OPTION_ARG_STRING, &host, "Host to connect", "HOST"},
  {"port", 0, 0, G_OPTION_ARG_STRING, &port, "Port to connect", "PORT"},
  {"n-requests", 0, 0, G_OPTION_ARG_INT, &n_requests,
   "The number of requests", "N"},
  {"concurrency", 0, 0, G_OPTION_ARG_INT, &max_concurrent_connections,
   "The max concurrent connections", "MAX"},
  {"n-messages", 0, 0, G_OPTION_ARG_INT, &n_messages,
   "The number of messages in a session", "N"},
  {NULL}
};

typedef struct {
  int epoll_fd;
  gint n_rest_requests;
  gint n_running_sessions;
  struct addrinfo *addresses;
} Context;

typedef struct {
  int socket_fd;
  gint n_rest_messages;
  Context *context;
} Session;

static gboolean
start_session(Context *context)
{
  struct addrinfo *address = context->addresses;
  int socket_fd = 0;

  context->n_rest_requests--;

  socket_fd = socket(address->ai_family,
                     address->ai_socktype,
                     address->ai_protocol);
  if (socket_fd == -1) {
    perror("failed to socket()");
    return FALSE;
  }

  if (connect(socket_fd,
              address->ai_addr,
              address->ai_addrlen) == -1) {
    perror("failed to connect()");
    close(socket_fd);
    return FALSE;
  }

  {
    struct epoll_event event;
    Session *session;

    session = g_new(Session, 1);
    session->socket_fd = socket_fd;
    session->n_rest_messages = n_messages;
    session->context = context;
    context->n_running_sessions++;

    event.events = EPOLLOUT;
    event.data.ptr = session;
    if (epoll_ctl(context->epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
      perror("failed to epoll_ctl()");
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
send_message(Session *session)
{
  {
    size_t written_size;
    written_size = write(session->socket_fd, message, sizeof(message) - 1);
    if (written_size == -1) {
      perror("failed to write()");
      return FALSE;
    }
  }

  {
    Context *context = session->context;
    int socket_fd = session->socket_fd;
    struct epoll_event event;

    event.events = EPOLLIN;
    event.data.ptr = session;
    if (epoll_ctl(context->epoll_fd, EPOLL_CTL_MOD, socket_fd, &event) == -1) {
      perror("failed to epoll_ctl(EPOLL_CTL_MOD(EPOLLIN))");
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
receive_message(Session *session)
{
  {
    size_t read_size;
    char buffer[4096];
    read_size = read(session->socket_fd, buffer, sizeof(buffer));
    if (read_size == -1) {
      perror("failed to read()");
      return FALSE;
    }
  }

  session->n_rest_messages--;
  if (session->n_rest_messages > 0) {
    Context *context = session->context;
    int socket_fd = session->socket_fd;
    struct epoll_event event;

    event.events = EPOLLOUT;
    event.data.ptr = session;
    if (epoll_ctl(context->epoll_fd, EPOLL_CTL_MOD, socket_fd, &event) == -1) {
      perror("failed to epoll_ctl(EPOLL_CTL_MOD(EPOLLOUT))");
      return FALSE;
    }
    return TRUE;
  } else {
    Context *context = session->context;
    close(session->socket_fd);
    epoll_ctl(context->epoll_fd, EPOLL_CTL_DEL, session->socket_fd, NULL);
    g_free(session);
    context->n_running_sessions--;
    if (context->n_rest_requests > 0) {
      return start_session(context);
    } else {
      return TRUE;
    }
  }
}

int
main(int argc, char **argv)
{
  Context context;
  context.n_running_sessions = 0;

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
    context.n_rest_requests = n_requests;
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
    GTimer *timer;

    timer = g_timer_new();
    {
      gint i;
      int not_used_size = 1;
#define MAX_EVENTS 10
      struct epoll_event events[MAX_EVENTS];

      context.epoll_fd = epoll_create(not_used_size);
      if (context.epoll_fd == -1) {
        perror("failed to epoll_create()");
        return EXIT_FAILURE;
      }

      for (i = 0; i < max_concurrent_connections; i++) {
        if (!start_session(&context)) {
          return EXIT_FAILURE;
        }
      }

      while (context.n_running_sessions > 0) {
        int i, n_events;

        n_events = epoll_wait(context.epoll_fd, events, MAX_EVENTS, -1);
        if (n_events == -1) {
          perror("failed to epoll_wait()");
          return EXIT_FAILURE;
        }

        for (i = 0; i < n_events; i++) {
          struct epoll_event *event = &(events[i]);
          Session *session = event->data.ptr;
          if (event->events & EPOLLOUT) {
            if (!send_message(session)) {
              return EXIT_FAILURE;
            }
          } else {
            if (!receive_message(session)) {
              return EXIT_FAILURE;
            }
          }
        }
      }
#undef MAX_EVENTS
    }

    g_timer_stop(timer);

    g_print("Total:   %0.3fs\n",
            g_timer_elapsed(timer, NULL));
    g_print("Average: %0.3fms\n",
            (g_timer_elapsed(timer, NULL) / n_requests) * 1000);
  }

  freeaddrinfo(context.addresses);

  return EXIT_SUCCESS;
}
