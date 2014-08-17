#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <glib.h>

static gchar *host = "127.0.0.1";
static gchar *port = "12929";
static gint n_requests = 30000;
static gint n_workers  =  1000;

static GOptionEntry entries[] =
{
  {"host", 0, 0, G_OPTION_ARG_STRING, &host, "Host to connect", "HOST"},
  {"port", 0, 0, G_OPTION_ARG_STRING, &port, "Port to connect", "PORT"},
  {"n-requests", 0, 0, G_OPTION_ARG_INT, &n_requests,
   "The number of requests", "N"},
  {"concurrency", 0, 0, G_OPTION_ARG_INT, &n_workers,
   "The number of workers", "N"},
  {NULL}
};

typedef struct {
  struct addrinfo *addresses;
} WorkerData;

static void
worker(gpointer data, gpointer user_data)
{
  WorkerData *worker_data = user_data;
  struct addrinfo *address;
  int socket_fd = 0;

  address = worker_data->addresses;
  socket_fd = socket(address->ai_family,
                     address->ai_socktype,
                     address->ai_protocol);
  if (socket_fd == -1) {
    perror("failed to socket()");
    goto exit;
  }

  if (connect(socket_fd,
              address->ai_addr,
              address->ai_addrlen) == -1) {
    perror("failed to connect()");
    goto exit;
  }

  if (write(socket_fd, "x", 1) == -1) {
    perror("failed to write()");
    goto exit;
  }

  {
    char buffer[1];
    if (read(socket_fd, buffer, 1) == -1) {
      perror("failed to read()");
      goto exit;
    }
  }

exit:
  if (socket_fd > 0) {
    close(socket_fd);
  }
}

int
main(int argc, char **argv)
{
  GThreadPool *pool;
  WorkerData data;

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

    error = getaddrinfo(host, port, &hints, &(data.addresses));
    if (error != 0) {
      g_print("failed to getaddrinfo(): %s\n", g_strerror(error));
      return EXIT_FAILURE;
    }
  }

  {
    gboolean exclusive = TRUE;
    GError *error = NULL;

    pool = g_thread_pool_new(worker, &data, n_workers, exclusive, &error);
    if (error) {
      g_print("failed to create thread pool: %s\n",
              error->message);
      g_error_free(error);
      return EXIT_FAILURE;
    }
  }

  {
    GTimer *timer;

    timer = g_timer_new();
    {
      gint i;
      for (i = 0; i < n_requests; i++) {
        GError *error = NULL;
        g_thread_pool_push(pool, &i, &error);
        if (error) {
          g_print("failed to push data to thread pool: %s\n",
                  error->message);
          g_error_free(error);
        }
      }
    }

    {
      gboolean immediate = FALSE;
      gboolean wait = TRUE;
      g_thread_pool_free(pool, immediate, wait);
    }
    g_timer_stop(timer);

    g_print("Total:   %0.3fs\n",
            g_timer_elapsed(timer, NULL));
    g_print("Average: %0.3fms\n",
            (g_timer_elapsed(timer, NULL) / n_requests) * 1000);
  }

  freeaddrinfo(data.addresses);

  return EXIT_SUCCESS;
}
