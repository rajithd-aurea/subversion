/* svn_sqlite.h
 *
 * ====================================================================
 * Copyright (c) 2008 CollabNet.  All rights reserved.
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


#ifndef SVN_SQLITE_H
#define SVN_SQLITE_H

#include <apr_pools.h>

#include "svn_types.h"
#include "svn_error.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct svn_sqlite__db_t svn_sqlite__db_t;
typedef struct svn_sqlite__stmt_t svn_sqlite__stmt_t;

typedef enum svn_sqlite__mode_e {
    svn_sqlite__mode_readonly,   /* open the database read-only */
    svn_sqlite__mode_readwrite,  /* open the database read-write */
    svn_sqlite__mode_rwcreate    /* open/create the database read-write */
} svn_sqlite__mode_t;


/* Steps the given statement; raises an SVN error (and finalizes the
   statement) if it doesn't return SQLITE_DONE. */
svn_error_t *
svn_sqlite__step_done(svn_sqlite__stmt_t *stmt);

/* Steps the given statement; raises an SVN error (and finalizes the
   statement) if it doesn't return SQLITE_ROW. */
svn_error_t *
svn_sqlite__step_row(svn_sqlite__stmt_t *stmt);

/* Steps the given statement; raises an SVN error (and finalizes the
   statement) if it doesn't return SQLITE_DONE or SQLITE_ROW.  Sets
   *GOT_ROW to true iff it got SQLITE_ROW.
*/
svn_error_t *
svn_sqlite__step(svn_boolean_t *got_row, svn_sqlite__stmt_t *stmt);

/* Execute SQL on the sqlite database DB, and raise an SVN error if the
   result is not okay.  */
svn_error_t *
svn_sqlite__exec(svn_sqlite__db_t *db, const char *sql);

/* Open a connection in *DB to the database at PATH. Validate the schema,
   creating/upgrading to LATEST_SCHEMA if needed using the instructions
   in UPGRADE_SQL. The resulting DB is allocated in RESULT_POOL, and any
   temporary allocations are made in SCRATCH_POOL.
   
   STATEMENTS is an array of strings which may eventually be executed, the
   last element of which should be NULL.  These strings are not duplicated
   internally, and should have a lifetime at least as long as RESULT_POOL.
   STATEMENTS itself may be NULL, in which case it has no impact.
   See svn_sqlite__get_statement() for how these strings are used. */
svn_error_t *
svn_sqlite__open(svn_sqlite__db_t **db, const char *repos_path,
                 svn_sqlite__mode_t mode, const char **statements,
                 int latest_schema, const char * const *upgrade_sql,
                 apr_pool_t *result_pool, apr_pool_t *scratch_pool);

/* Returns the statement in *STMT which has been prepared from the
   STATEMENTS[STMT_IDX] string.  This statement is allocated in the same
   pool as the DB, and will be cleaned up with DB is closed. */
svn_error_t *
svn_sqlite__get_statement(svn_sqlite__stmt_t **stmt, svn_sqlite__db_t *db,
                          int stmt_idx);

/* Prepares TEXT as a statement in DB, returning a statement in *STMT,
   allocated in RESULT_POOL. */
svn_error_t *
svn_sqlite__prepare(svn_sqlite__stmt_t **stmt, svn_sqlite__db_t *db,
                    const char *text, apr_pool_t *result_pool);

/* Error-handling wrapper around sqlite3_bind_int64. */
svn_error_t *
svn_sqlite__bind_int64(svn_sqlite__stmt_t *stmt, int slot,
                       apr_int64_t val);

/* Error-handling wrapper around sqlite3_bind_text. VAL cannot contain
   zero bytes; we always pass SQLITE_TRANSIENT. */
svn_error_t *
svn_sqlite__bind_text(svn_sqlite__stmt_t *stmt, int slot,
                      const char *val);

/* Wrapper around sqlite3_column_text. */
const char *
svn_sqlite__column_text(svn_sqlite__stmt_t *stmt, int column);

/* Wrapper around sqlite3_column_int64. */
svn_revnum_t
svn_sqlite__column_revnum(svn_sqlite__stmt_t *stmt, int column);

/* Wrapper around sqlite3_column_int64. */
svn_boolean_t
svn_sqlite__column_boolean(svn_sqlite__stmt_t *stmt, int column);

/* Wrapper around sqlite3_column_int. */
int
svn_sqlite__column_int(svn_sqlite__stmt_t *stmt, int column);

/* Error-handling wrapper around sqlite3_finalize. */
svn_error_t *
svn_sqlite__finalize(svn_sqlite__stmt_t *stmt);

/* Error-handling wrapper around sqlite3_reset. */
svn_error_t *
svn_sqlite__reset(svn_sqlite__stmt_t *stmt);

/* Close DB, returning any ERR which may've necessitated an early connection
   closure, or -- if none -- the error from the closure itself. */
svn_error_t *
svn_sqlite__close(svn_sqlite__db_t *db, svn_error_t *err);

/* Wrapper around sqlite transaction handling. */
svn_error_t *
svn_sqlite__transaction_begin(svn_sqlite__db_t *db);

/* Wrapper around sqlite transaction handling. */
svn_error_t *
svn_sqlite__transaction_commit(svn_sqlite__db_t *db);

/* Wrapper around sqlite transaction handling. */
svn_error_t *
svn_sqlite__transaction_rollback(svn_sqlite__db_t *db);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_SQLITE_H */
