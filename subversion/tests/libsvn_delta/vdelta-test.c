/* vdelta-test.c -- test driver for text deltas
 *
 * ====================================================================
 * Copyright (c) 2000-2002 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */

#define APR_WANT_STDIO
#include <apr_want.h>

#include <apr_general.h>
#include <apr_lib.h>

#include "svn_delta.h"
#include "svn_error.h"
#include "svn_pools.h"

#include "../../libsvn_delta/delta.h"
#include "delta-window-test.h"

static apr_off_t
print_delta_window (const svn_txdelta_window_t *window,
                    const char *tag, int quiet, FILE *stream)
{
  if (quiet)
    return delta_window_size_estimate (window);
  else
    return delta_window_print (window, tag, stream);
}


static void
do_one_diff (FILE *source_file, FILE *target_file,
             int *count, apr_off_t *len,
             int quiet, apr_pool_t *pool,
             const char *tag, FILE* stream)
{
  svn_txdelta_stream_t *delta_stream = NULL;
  svn_txdelta_window_t *delta_window = NULL;
  apr_pool_t *fpool = svn_pool_create (pool);
  apr_pool_t *wpool = svn_pool_create (pool);

  *count = 0;
  *len = 0;
  svn_txdelta (&delta_stream,
               svn_stream_from_stdio (source_file, fpool),
               svn_stream_from_stdio (target_file, fpool),
               fpool);
  do {
    svn_txdelta_next_window (&delta_window, delta_stream, wpool);
    if (delta_window != NULL)
      {
        *len += print_delta_window (delta_window, tag, quiet, stream);
        svn_pool_clear (wpool);
        ++*count;
      }
  } while (delta_window != NULL);
  fprintf (stream, "%s: (LENGTH %" APR_OFF_T_FMT " +%d)\n", tag, *len, *count);

  svn_pool_destroy (fpool);
  svn_pool_destroy (wpool);
}


int
main (int argc, char **argv)
{
  FILE *source_file_A = NULL;
  FILE *target_file_A = NULL;
  int count_A = 0;
  apr_off_t len_A = 0;

  FILE *source_file_B = NULL;
  FILE *target_file_B = NULL;
  int count_B = 0;
  apr_off_t len_B = 0;

  apr_pool_t *pool;
  int quiet = 0;

  if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'q')
    {
      quiet = 1;
      --argc; ++argv;
    }

  if (argc == 2)
    {
      target_file_A = fopen (argv[1], "rb");
    }
  else if (argc == 3)
    {
      source_file_A = fopen (argv[1], "rb");
      target_file_A = fopen (argv[2], "rb");
    }
  else if (argc == 4)
    {
      source_file_A = fopen (argv[1], "rb");
      target_file_A = fopen (argv[2], "rb");
      source_file_B = fopen (argv[2], "rb");
      target_file_B = fopen (argv[3], "rb");
    }
  else
    {
      fprintf (stderr,
               "Usage: vdelta-test [-q] <target>\n"
               "   or: vdelta-test [-q] <source> <target>\n"
               "   or: vdelta-test [-q] <source> <intermediate> <target>\n");
      exit (1);
    }

  apr_initialize();
  pool = svn_pool_create (NULL);

  do_one_diff (source_file_A, target_file_A,
               &count_A, &len_A, quiet, pool, "A ", stdout);

  if (source_file_B)
    {
      apr_pool_t *fpool = svn_pool_create (pool);
      apr_pool_t *wpool = svn_pool_create (pool);
      svn_txdelta_stream_t *stream_A = NULL;
      svn_txdelta_stream_t *stream_B = NULL;
      svn_txdelta_window_t *window_A = NULL;
      svn_txdelta_window_t *window_B = NULL;
      svn_txdelta_window_t *window_AB = NULL;
      int count_AB = 0;
      apr_off_t len_AB = 0;

      putc('\n', stdout);
      do_one_diff (source_file_B, target_file_B,
                   &count_B, &len_B, quiet, pool, "B ", stdout);

      putc('\n', stdout);
      rewind (source_file_A);
      rewind (target_file_A);
      rewind (source_file_B);
      rewind (target_file_B);

      svn_txdelta (&stream_A,
                   svn_stream_from_stdio (source_file_A, fpool),
                   svn_stream_from_stdio (target_file_A, fpool),
                   fpool);
      svn_txdelta (&stream_B,
                   svn_stream_from_stdio (source_file_B, fpool),
                   svn_stream_from_stdio (target_file_B, fpool),
                   fpool);

      for (count_AB = 0; count_AB < count_B; ++count_AB)
        {
          svn_txdelta__compose_ctx_t context = { 0 };
          svn_txdelta_next_window (&window_A, stream_A, wpool);
          svn_txdelta_next_window (&window_B, stream_B, wpool);

          /* Note: It's not possible that window_B is null, we already
             counted the number of windows in the second delta. */
          window_AB =
            svn_txdelta__compose_windows (window_A, window_B, &context, wpool);
          if (!window_AB && context.use_second)
            {
              window_AB = window_B;
              window_AB->sview_offset = context.sview_offset;
              window_AB->sview_len = context.sview_len;
            }
          len_AB += print_delta_window (window_AB, "AB", quiet, stdout);
          svn_pool_clear (wpool);
        }

      fprintf (stdout, "AB: (LENGTH %" APR_OFF_T_FMT " +%d)\n",
               len_AB, count_AB);
    }

  if (source_file_A) fclose (source_file_A);
  if (target_file_A) fclose (target_file_A);
  if (source_file_B) fclose (source_file_B);
  if (target_file_B) fclose (source_file_B);

  svn_pool_destroy (pool);
  apr_terminate();
  exit (0);
}



/*
 * local variables:
 * eval: (load-file "../../../tools/dev/svn-dev.el")
 * end:
 */
