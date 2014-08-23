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

static gchar *port = "22929";

static GOptionEntry entries[] =
{
  {"port", 0, 0, G_OPTION_ARG_STRING, &port, "Port to connect", "PORT"},
  {NULL}
};

typedef struct {
  int epoll_fd;
} Context;

#define MESSAGE_SIZE 4096
typedef struct {
  int socket_fd;
  gchar message[MESSAGE_SIZE];
  size_t message_size;
  Context *context;
} Session;

static gboolean
start_session(Context *context, int accept_fd)
{
  Session *new_session;
  int client_socket_fd;

  client_socket_fd = accept(accept_fd, NULL, 0);
  if (client_socket_fd == -1) {
    perror("failed to accept()");
    return FALSE;
  }

  new_session = g_new(Session, 1);
  new_session->socket_fd = client_socket_fd;
  new_session->message_size = 0;
  new_session->context = context;

  {
    int epoll_fd = context->epoll_fd;
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    event.data.ptr = new_session;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket_fd, &event) == -1) {
      perror("failed to epoll_ctl(EPOLL_CTL_ADD, client_socket_fd)");
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
stop_session(Session *session)
{
  epoll_ctl(session->context->epoll_fd, EPOLL_CTL_DEL, session->socket_fd, NULL);
  close(session->socket_fd);
  g_free(session);
  return TRUE;
}

static gboolean
receive_message(Session *session)
{
  session->message_size =
    read(session->socket_fd, session->message, MESSAGE_SIZE);
  if (session->message_size == -1) {
    perror("failed to read()");
    return FALSE;
  }

  if (session->message_size == 0) {
    return stop_session(session);
  }

  {
    int epoll_fd = session->context->epoll_fd;
    struct epoll_event event;
    event.events = EPOLLOUT;
    event.data.ptr = session;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, session->socket_fd, &event) == -1) {
      perror("failed to epoll_ctl(EPOLL_CTL_MOD, client_socket_fd)");
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
send_message(Session *session)
{
  size_t written_size;
  written_size = write(session->socket_fd,
                       session->message, session->message_size);
  if (written_size == -1) {
    perror("failed to write()");
    return FALSE;
  }
  session->message_size = 0;

  {
    int epoll_fd = session->context->epoll_fd;
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    event.data.ptr = session;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, session->socket_fd, &event) == -1) {
      perror("failed to epoll_ctl(EPOLL_CTL_MOD, client_socket_fd)");
      return FALSE;
    }
  }

  return TRUE;
}

int
main(int argc, char **argv)
{
  struct addrinfo *addresses;

  {
    GOptionContext *context;
    GError *error = NULL;

    context = g_option_context_new("server side implementation by thread");
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
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    error = getaddrinfo(NULL, port, &hints, &addresses);
    if (error != 0) {
      g_print("failed to getaddrinfo(): %s\n", g_strerror(error));
      return EXIT_FAILURE;
    }
  }

  {
    struct addrinfo *address = addresses;
    int accept_socket_fd;

    accept_socket_fd = socket(address->ai_family,
                              address->ai_socktype,
                              address->ai_protocol);
    if (accept_socket_fd == -1) {
      perror("failed to socket()");
      return EXIT_FAILURE;
    }

    {
      gboolean reuse_address = TRUE;
      if (setsockopt(accept_socket_fd, SOL_SOCKET, SO_REUSEADDR,
                     &reuse_address, sizeof(reuse_address)) == -1) {
        perror("failed to setsockopt(SO_REUSEADDR): %s");
        return EXIT_FAILURE;
      }
    }

    if (bind(accept_socket_fd, address->ai_addr, address->ai_addrlen) == -1) {
      perror("failed to bind()");
      return EXIT_FAILURE;
    }

    {
      const int backlog = 1024;
      if (listen(accept_socket_fd, backlog) == -1) {
        perror("failed to listen()");
        return EXIT_FAILURE;
      }
    }

    {
      Context context;
      Session accept_session;
      int not_used_size = 1;
#define MAX_EVENTS 10
      struct epoll_event events[MAX_EVENTS];

      context.epoll_fd = epoll_create(not_used_size);
      if (context.epoll_fd == -1) {
        perror("failed to epoll_create()");
        return EXIT_FAILURE;
      }

      {
        struct epoll_event event;
        int epoll_fd = context.epoll_fd;
        event.events = EPOLLIN;
        event.data.ptr = &accept_session;
        accept_session.socket_fd = accept_socket_fd;
        accept_session.message_size = 0;
        accept_session.context = &context;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_socket_fd, &event) == -1) {
          perror("failed to epoll_ctl(EPOLL_CTL_ADD, accept_fd)");
          return EXIT_FAILURE;
        }

        while (TRUE) {
          int i, n_events;

          n_events = epoll_wait(context.epoll_fd, events, MAX_EVENTS, -1);

          for (i = 0; i < n_events; i++) {
            struct epoll_event *event = events + i;
            Session *session;

            session = event->data.ptr;
            if (session->socket_fd == accept_socket_fd) {
              if (!start_session(&context, session->socket_fd)) {
                return EXIT_FAILURE;
              }
              continue;
            }

            if (event->events & (EPOLLIN | EPOLLPRI)) {
              if (!receive_message(session)) {
                return EXIT_FAILURE;
              }
            } else if (event->events & EPOLLOUT) {
              if (!send_message(session)) {
                return EXIT_FAILURE;
              }
            } else if (event->events & (EPOLLHUP | EPOLLERR)) {
              if (!stop_session(session)) {
                return EXIT_FAILURE;
              }
            }
          }
        }
      }
    }
  }

  freeaddrinfo(addresses);

  return EXIT_SUCCESS;
}
