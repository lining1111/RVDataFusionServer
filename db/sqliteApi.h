//
// Created by lining on 2023/2/23.
//

#ifndef SQLITEAPI_H
#define SQLITEAPI_H

#include <sqlite3.h>
#include <string>

/**
 * sqlite3的操作集合
 */

typedef int (*SQLITE3_CALLBACK)(void *, int, char **, char **);

int closeDatabase(sqlite3 *db);

int execSql(sqlite3 *db, char *sql_string, SQLITE3_CALLBACK sql_callback, void *param);

int execGetTable(sqlite3 *db, char *sql_string, char ***data, int *row, int *col);

/**
 * 无需结果的sql
 * @param dbFile
 * @param sql_string
 * @param sql_callback
 * @param param
 * @return
 */
int dbFileExecSql(std::string dbFile, char *sql_string, SQLITE3_CALLBACK sql_callback, void *param);

/**
 * 需要结果的sql 需要在外面释放data：dbFreeTable( dbResult )
 * @param dbFile
 * @param sql_string
 * @param data
 * @param row
 * @param col
 * @return
 */
int dbFileExecSqlTable(std::string dbFile, char *sql_string, char ***data, int *row, int *col);

void dbFreeTable(char **result);

/**
 * 检查或添加一个表，默认添加一个column[id]
 * 如果存在此名称的表则不会创建，并且不会检查表是否存在id列。
 * 如果不存在则会创建一个表并且默认创建一个id列
 * @param dbFile
 * @param tableName
 * @return 0:成功 -1:失败
 */
int dbCheckORAddTable(std::string dbFile, std::string tableName);

/**
 * 校验数据库表中的一列或者添加一列
 * 如果数据库表中存在此列名但是列的index与设置不符，会认为失败。
 * 如果不存在此表则也会失败。
 * @param dbFile
 * @param tableName
 * @param column_index
 * @param columnName
 * @param columnDescription
 * @return
 */
int dbCheckOrAddColumn(std::string dbFile, std::string tableName,
                       int column_index, std::string columnName, std::string columnDescription);

/**
 * 删除一个表
 * @param dbFile
 * @param table_name
 * @return
 */
int dbDeleteTable(std::string dbFile, const char *table_name);

/**
 * 时间戳转数据库格式（TIMESTAMP）的时间字符串(2020-06-09 17:24:43)
 * @param t
 * @param timestr
 */
void db_timet2timestamp(time_t t, char *timestr);

/**
 * 时间戳转数据库格式（TIMESTAMP）的时间字符串(2020-06-09 17:24:43)
 * @param t
 * @return
 */
std::string db_timet2timestamp(time_t t);

#endif //SQLITEAPI_H
