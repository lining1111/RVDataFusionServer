#ifndef _SQLITE_API_H_
#define _SQLITE_API_H_

#include "sqlite3.h"

#ifdef __cplusplus

extern "C"
{
#endif

typedef int (*SQLITE3_CALLBACK)(void *,int ,char **,char **);

extern int callback_db(void *ptr,int count);

extern int open_database(char *addr,sqlite3 **db);

extern int exec_sql(sqlite3 *db,char *sql_string,SQLITE3_CALLBACK sql_callback,void *param);

extern int close_database(sqlite3 *db);

/*执行sql语句*/
extern int db_exec_sql(char *addr,char *sql_string,SQLITE3_CALLBACK sql_callback,void *param);
extern int db_file_exec_sql(char *sql_file,char *sql_string,SQLITE3_CALLBACK sql_callback,void *param);

extern int exec_get_table(sqlite3 *db,char *sql_string,char ***data,int *row,int *col);

/*需要在外面释放data：sqlite3_free_table( dbResult );*/
extern int db_exec_sql_table(char *addr,char *sql_string,char ***data,int *row,int *col);
int db_file_exec_sql_table(char *sql_file,char *sql_string,char ***data,int *row,int *col);
void sqlite3_free_table_safe(char **result);

#ifdef __cplusplus
}
#endif

#endif //_SQLITE_API_H_
