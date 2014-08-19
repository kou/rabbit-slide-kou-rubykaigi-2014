#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <glib.h>

static gchar *port = "22929";
static gint n_workers = 8;

static GOptionEntry entries[] =
{
  {"port", 0, 0, G_OPTION_ARG_STRING, &port, "Port to connect", "PORT"},
  {"concurrency", 0, 0, G_OPTION_ARG_INT, &n_workers,
   "The number of workers", "N"},
  {NULL}
};

static void
worker(gpointer data, gpointer user_data)
{
  int *client_socket_fd = data;

  while (TRUE) {
#define BUFFER_SIZE 4096
    char buffer[BUFFER_SIZE];
    size_t read_size;

    read_size = read(*client_socket_fd, buffer, BUFFER_SIZE);
    if (read_size == -1) {
      perror("failed to read()");
      goto exit;
    }
    if (write(*client_socket_fd, buffer, read_size) == -1) {
      perror("failed to write()");
      goto exit;
    }
#undef BUFFER_SIZE
  }

exit:
  close(*client_socket_fd);
  g_free(client_socket_fd);
}

int
main(int argc, char **argv)
{
  GThreadPool *pool;
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
    gboolean exclusive = TRUE;
    GError *error = NULL;

    pool = g_thread_pool_new(worker, NULL, n_workers, exclusive, &error);
    if (error) {
      g_print("failed to create thread pool: %s\n",
              error->message);
      g_error_free(error);
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
      int *client_socket_fd;

      client_socket_fd = g_new(int, 1);
      *client_socket_fd = accept(accept_socket_fd, NULL, 0);
      if (*client_socket_fd > 0) {
        GError *error = NULL;
        g_thread_pool_push(pool, client_socket_fd, &error);
        if (error) {
          g_print("failed to push thread pool: %s\n", error->message);
          g_error_free(error);
          return EXIT_FAILURE;
        }
      } else {
        g_free(client_socket_fd);
        perror("failed to accept()");
        return EXIT_FAILURE;
      }
    }
  }

  {
    gboolean immediate = FALSE;
    gboolean wait = TRUE;
    g_thread_pool_free(pool, immediate, wait);
  }

  freeaddrinfo(addresses);

  return EXIT_SUCCESS;
}
