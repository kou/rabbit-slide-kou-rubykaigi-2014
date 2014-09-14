#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <groonga.h>

static grn_obj *
get(grn_ctx *ctx, const gchar *name)
{
  return grn_ctx_get(ctx, name, strlen(name));
}

int
main(int argc, char **argv)
{
  grn_init();

  {
    grn_ctx ctx;
    grn_obj *database;
    grn_obj *table;

    grn_ctx_init(&ctx, 0);
    database = grn_db_open(&ctx, argv[1]);
    table = get(&ctx, "Entries");

    {
      GTimer *timer;

      timer = g_timer_new();
      {
        grn_obj *expr;
        grn_obj *variable;
        const gchar *filter = "description @ \"Ruby\"";
        grn_obj *result;

        GRN_EXPR_CREATE_FOR_QUERY(&ctx, table, expr, variable);
        grn_expr_parse(&ctx, expr,
                       filter, strlen(filter), NULL,
                       GRN_OP_MATCH, GRN_OP_AND,
                       GRN_EXPR_SYNTAX_SCRIPT);
        result = grn_table_select(&ctx, table, expr, NULL, GRN_OP_OR);
        grn_obj_unlink(&ctx, expr);
        grn_obj_unlink(&ctx, result);
      }
      g_print("%f\n", g_timer_elapsed(timer, NULL));
      g_timer_destroy(timer);
    }

    grn_obj_unlink(&ctx, table);
    grn_obj_close(&ctx, database);
    grn_ctx_fin(&ctx);
  }

  grn_fin();

  return EXIT_SUCCESS;
}
