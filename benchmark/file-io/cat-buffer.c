#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

static gsize buffer_size = 4096;

static GOptionEntry entries[] =
{
  {"buffer-size", 0, 0, G_OPTION_ARG_INT, &buffer_size,
   "The size of buffer", "SIZE"},
  {NULL}
};

int
main(int argc, char **argv)
{
  {
    GOptionContext *context;
    GError *error = NULL;

    context = g_option_context_new("cat with buffer");
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
        gchar *buffer;
        buffer = g_malloc(buffer_size);
        while (TRUE) {
          ssize_t read_size;
          read_size = read(fd, buffer, buffer_size);
          if (read_size == 0) {
            break;
          } else if (read_size == -1) {
            perror("read");
            break;
          } else {
            write(STDOUT_FILENO, buffer, read_size);
          }
        }
        g_free(buffer);
      }

      close(fd);
    }

    fprintf(stderr, "%fs\n", g_timer_elapsed(timer, NULL));
    g_timer_destroy(timer);
  }

  return EXIT_SUCCESS;
}
