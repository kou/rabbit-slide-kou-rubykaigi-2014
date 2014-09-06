#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

static gsize chunk_size = 4096;

static GOptionEntry entries[] =
{
  {"chunk-size", 0, 0, G_OPTION_ARG_INT, &chunk_size,
   "The size of chunk", "SIZE"},
  {NULL}
};

int
main(int argc, char **argv)
{
  {
    GOptionContext *context;
    GError *error = NULL;

    context = g_option_context_new("cat without buffer");
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
    GTimer *timer;
    int i;

    timer = g_timer_new();
    for (i = 1; i < argc; i++) {
      const gchar *file_name = argv[i];
      int fd;

      fd = open(file_name, O_RDONLY);
      if (!fd) {
        continue;
      }

      {
        while (TRUE) {
          gchar *chunk;
          ssize_t read_size;
          chunk = g_malloc(chunk_size);
          read_size = read(fd, chunk, chunk_size);
          if (read_size == 0) {
            g_free(chunk);
            break;
          } else if (read_size == -1) {
            perror("read");
            g_free(chunk);
            break;
          } else {
            write(STDOUT_FILENO, chunk, read_size);
          }
          g_free(chunk);
        }
      }

      close(fd);
    }

    fprintf(stderr, "%f\n", g_timer_elapsed(timer, NULL));
    g_timer_destroy(timer);
  }

  return EXIT_SUCCESS;
}
