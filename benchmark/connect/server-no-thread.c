#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <glib.h>

static gchar *port = "2929";

static GOptionEntry entries[] =
{
  {"port", 0, 0, G_OPTION_ARG_STRING, &port, "Port to connect", "PORT"},
  {NULL}
};

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

    while (TRUE) {
      int client_socket_fd;
      client_socket_fd = accept(accept_socket_fd, NULL, 0);
      if (client_socket_fd == -1) {
        perror("failed to accept()");
        return EXIT_FAILURE;
      }
      close(client_socket_fd);
    }
  }

  freeaddrinfo(addresses);

  return EXIT_SUCCESS;
}
