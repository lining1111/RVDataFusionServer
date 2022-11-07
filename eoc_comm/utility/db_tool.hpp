/*
 * db_tool.hpp
 *
 *  Created on: 2020年4月23日
 *      Author: zpc
 */

#ifndef SOURCE_SRC_UTILITY_DB_TOOL_HPP_
#define SOURCE_SRC_UTILITY_DB_TOOL_HPP_

#include <string>

#include "sqlite_api.h"

using namespace std;


typedef struct
{
	char table_name[64];			//数据库表名
	char column_name[64];			//列名
	char column_description[64];	//列描述
}DBT_Table;
/*
 * 根据路径创建各级目录，最大路径长度512-1，递归创建知道最后一个'/'
 * 如"/home/nvidia/run/aa"将递归创建home、nvidia、run
 * 如"/home/nvidia/run/aa/"将递归创建home、nvidia、run、aa
 *
 * 参数：
 * path：将要递归创建的文件路径或文件夹路径
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int dbt_mkdir_from_path(const char* path);
/*
 * 校验数据库表中的一列或者添加一列
 * 如果数据库表中存在此列名但是列的index与设置不符，会认为失败。
 * 如果不存在此表则也会失败。
 *
 * db_path：数据库的路径
 * table_name：表名
 * coulumn_index：要校验或添加的列的index
 * column_name：要校验或添加的列的名字
 * column_description：列的描述，如果不存在此列，则会按照描述添加一个列
 *
 * 0：成功
 * -1：失败
 * */
int dbt_check_or_add_column(const char *db_path, const char *table_name, int coulumn_index, const char *column_name, const char *column_description);
/*
 * 检查或添加一个表，默认添加一个column[id]
 * 如果存在此名称的表则不会创建，并且不会检查表是否存在id列。
 * 如果不存在则会创建一个表并且默认创建一个id列
 *
 * db_path：数据库的路径
 * table_name：表名
 *
 * 0：成功
 * -1：失败
 * */
int dbt_check_or_add_table(const char *db_path, const char* table_name);
int dbt_delete_table(const char *db_path, const char* table_name);
/*
 * 校验一个数据库表，如果可能会根据描述创建一个表
 *
 * 参数：
 * db_path：数据库路径
 * table：数据库描述
 *
 * 返回值;
 * 0：成功
 * -1：失败
 * */
int dbt_check(const char* db_path, const DBT_Table table[], int coulumn_size);
/*
 * 时间戳转数据库格式（TIMESTAMP）的时间字符串
 * */
void dbt_timet2timestamp(time_t t, char *timestr);
/*
 * 时间戳转数据库格式（TIMESTAMP）的时间字符串
 * */
string dbt_timet2timestamp(time_t t);
/*
 * 时间戳转时间字符串(20200609172443)
 * */
string dbt_timet2str(time_t t);

void dbt_test();

#endif /* SOURCE_SRC_UTILITY_DB_TOOL_HPP_ */
