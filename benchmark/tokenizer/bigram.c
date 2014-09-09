#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

int
main(int argc, char **argv)
{
  gchar *text;
  gsize length;

  {
    GError *error = NULL;
    if (!g_file_get_contents(argv[1], &text, &length, &error)) {
      g_print("failed to read content: %s\n", error->message);
      return EXIT_FAILURE;
    }
  }

  {
    GTimer *timer;

    timer = g_timer_new();
    {
      GPtrArray *tokens;
      const gchar *previous, *current, *end;
      g_utf8_validate(text, length, &end);

      tokens = g_ptr_array_new_with_free_func(g_free);
      previous = text;
      current = g_utf8_next_char(text);
      while (current < end) {
        gchar *next = g_utf8_next_char(current);
        gchar *token;
        token = g_strndup(previous, next - previous);
        g_ptr_array_add(tokens, token);
        previous = current;
        current = next;
      }
      g_ptr_array_free(tokens, TRUE);
    }
    g_print("%0.6fs\n", g_timer_elapsed(timer, NULL));
    g_timer_destroy(timer);
  }

  g_free(text);

  return EXIT_SUCCESS;
}
