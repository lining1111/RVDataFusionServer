/*
 * db_tool.cpp
 *
 *  Created on: 2020年4月23日
 *      Author: zpc
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
//#include <uuid/uuid.h>

#include "sqlite_api.h"
#include "logger.h"
#include "db_tool.hpp"

#define K_SIZE    1024

/*
 * 简单string类
 * */
template<class T>
class Estring {
public:
    T * data;
    size_t size;
    Estring(size_t s) {
        size = s;
        data = NULL;
        if (size > 0) {
            data = (T*) calloc(size, sizeof(T));
        }
        if (data == NULL) {
            ERR("%s:malloc(size=%d * sizeof(T)=%d) = %p, ERR", __FUNCTION__, (int )size, (int )sizeof(T), data);
        }
    }
    ;
    ~Estring() {
        if (data) {
            free(data);
        }
    }
    ;
};

/*
 * 根据路径创建各级目录，最大路径长度512-1，递归创建知道最后一个'/'
 * 如"/home/nvidia/run/aa"将递归创建home、nvidia、run
 * 如"/home/nvidia/run/aa/"将递归创建home、nvidia、run、aa
 * 如"home/nvidia/run/aa/"将在当前目录递归创建home、nvidia、run、aa
 * 如"./home/nvidia/run/aa/"将在当前目录递归创建home、nvidia、run、aa
 *
 * 参数：
 * path：将要递归创建的文件路径或文件夹路径
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int dbt_mkdir_from_path(const char* path)
{
    char path_tmp[512] = { 0 };
    int len = strlen(path);
    if(len >= 512){
    	ERR("%s:path=%s is too long >= 512", __FUNCTION__, path);
    	return -1;
    }
    sprintf(path_tmp, "%s", path);

    for (int i = 1; i < len; i++) {
        if (path_tmp[i] == '/') {
            path_tmp[i] = 0;
            if (access(path_tmp, R_OK | W_OK | F_OK) != 0) {
                if (mkdir(path_tmp, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
                    ERR("%s mkdir [%s] Err", __FUNCTION__, path_tmp);
                    return -1;
                }
            }
            path_tmp[i] = '/';
        }
    }

    return 0;
}
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
int dbt_check_or_add_column(const char *db_path, const char *table_name, int coulumn_index, const char *column_name, const char *column_description) {
    Estring<char> estr(K_SIZE);
    if (estr.data == NULL) {
        ERR("%s:Estring malloc ERR", __FUNCTION__);
        return -1;
    }

    int ret = 0;
    int row = 0;
    int col = 0;
    char **sqdata;
    int index_real = -1;

    //获取表中所有column的名称
    snprintf(estr.data, estr.size, "pragma table_info('%s');", table_name);
    ret = db_exec_sql_table((char *) db_path, estr.data, &sqdata, &row, &col);
    if (ret == -1) {
        ERR("%s:db_exec_sql_table sqlstr = %s. Err", __FUNCTION__, estr.data);
        sqlite3_free_table(sqdata);
        return -1;
    }
    bool need_alter = true;
    if (ret == 0) {
        for (int temp = 0; temp < row; temp++) {
            int n_index = col * (temp + 1);
//            DBG("%s:%s table %s column[%d] name = %s", __FUNCTION__, db_path, table_name, temp, sqdata[n_index + 1]);
            if (strcmp(column_name, sqdata[n_index + 1]) == 0) {
                need_alter = false;
                index_real = temp;
                break;
            }
        }

        if (need_alter) {
            //添加一列
            sqlite3_free_table(sqdata);
            snprintf(estr.data, estr.size, "alter table %s add %s %s;", table_name, column_name, column_description);
            DBG("%s:%s", __FUNCTION__, estr.data);
            ret = db_exec_sql_table((char *) db_path, estr.data, &sqdata, &row, &col);
            if (ret == -1) {
                ERR("%s:db_exec_sql_table sqlstr = %s Err", __FUNCTION__, estr.data);
                sqlite3_free_table(sqdata);
                return -1;
            }
        }
    }

    sqlite3_free_table(sqdata);
    if (need_alter) {
        return 0;
    }
    if (index_real == coulumn_index) {
        return 0;
    }
    ERR("%s:column[%s] index is %d,but index_real = %d,Err", __FUNCTION__, column_name, coulumn_index, index_real);
    return -1;
}
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
int dbt_check_or_add_table(const char *db_path, const char* table_name) {
    Estring<char> estr(K_SIZE);
    if (estr.data == NULL) {
        ERR("%s:Estring malloc ERR", __FUNCTION__);
        return -1;
    }

    int ret = 0;

    snprintf(estr.data, estr.size, "create table IF NOT EXISTS %s(id INTEGER PRIMARY KEY NOT NULL)", table_name);
    ret = db_file_exec_sql((char*) db_path, estr.data, NULL, NULL);
    if (ret < 0) {
        DBG("db_file_exec_sql err.sqlstr:%s\n", estr.data);
        return -1;
    }

    return 0;
}
int dbt_delete_table(const char *db_path, const char* table_name) {
    DBG("%s:db_path=%s, table_name=%s", __FUNCTION__, db_path, table_name);
    Estring<char> estr(K_SIZE);
    if (estr.data == NULL) {
        ERR("%s:Estring malloc ERR", __FUNCTION__);
        return -1;
    }

    int ret = 0;

    snprintf(estr.data, estr.size, "drop table %s", table_name);
    ret = db_file_exec_sql((char*) db_path, estr.data, NULL, NULL);
    if (ret < 0) {
        ERR("db_file_exec_sql err.sqlstr:%s\n", estr.data);
        return -1;
    }

    return 0;
}
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
int dbt_check(const char* db_path, const DBT_Table table[], int coulumn_size) {
    int ret = 0;

    ret = dbt_mkdir_from_path(db_path);
	if(ret != 0){
		ERR("%s:dbt_mkdir_from_path(%s) Err", __FUNCTION__, db_path);
		return -1;
	}

    if(coulumn_size <= 0){
    	ERR("%s:coulumn_size = %d, return", __FUNCTION__, coulumn_size);
    	return -1;
    }

	for(int i=0;i<coulumn_size;i++){
		bool check_succeed = true;
		ret = dbt_check_or_add_table(db_path, table[0].table_name);
		if(ret != 0){
			ERR("%s:check_or_add_table Err", __FUNCTION__);
			return -1;
		}
		for(int c_index=0;c_index<coulumn_size;c_index++){
			ret = dbt_check_or_add_column(db_path, table[0].table_name, c_index+1, table[c_index].column_name,
					table[c_index].column_description);
			if(ret != 0){
				check_succeed = false;
				ERR("%s:dbt_check_or_add_column Err, delete table and retry", __FUNCTION__);
				dbt_delete_table(db_path, table[0].table_name);
				break;
			}
		}
		if(check_succeed){
			INFO("%s:check %s,coulumn_size = %d succeed", __FUNCTION__, table[0].table_name, coulumn_size);
			return 0;
		}
	}
	ERR("%s:check %s,coulumn_size = %d failed", __FUNCTION__, table[0].table_name, coulumn_size);
	return -1;
}
/*
 * 时间戳转数据库格式（TIMESTAMP）的时间字符串(2020-06-09 17:24:43)
 * */
void dbt_timet2timestamp(time_t t, char *timestr) {
    time_t now = t;
    struct tm time_tm;

    localtime_r(&now, &time_tm);
    sprintf(timestr, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + time_tm.tm_year, 1 + time_tm.tm_mon, time_tm.tm_mday, time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec);
}
/*
 * 时间戳转数据库格式（TIMESTAMP）的时间字符串(2020-06-09 17:24:43)
 * */
string dbt_timet2timestamp(time_t t) {
    time_t now = t;
    struct tm time_tm;
    char timestr[24] = { 0 };

    localtime_r(&now, &time_tm);
    sprintf(timestr, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + time_tm.tm_year, 1 + time_tm.tm_mon, time_tm.tm_mday, time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec);

    return string(timestr);
}
/*
 * 时间戳转时间字符串(20200609172443)
 * */
string dbt_timet2str(time_t t) {
    time_t now = t;
    struct tm time_tm;
    char timestr[24] = { 0 };

    localtime_r(&now, &time_tm);
    sprintf(timestr, "%04d%02d%02d%02d%02d%02d", 1900 + time_tm.tm_year, 1 + time_tm.tm_mon, time_tm.tm_mday, time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec);

    return string(timestr);
}
static DBT_Table table_test[] = {
	{"table_test", "ParkingRecordGuid", "TEXT"},
	{"table_test", "InfoID", "TEXT"},
	{"table_test", "Info", "TEXT"},
	{"table_test", "Confidence", "FLOAT"},
	{"table_test", "RecordDateTime", "TIMESTAMP"},

	{"table_test", "sendFlag", "INTERGE"},
	{"table_test", "sendTime", "TIMESTAMP"}};
static DBT_Table table_test1[] = {
	{"table_test", "ParkingRecordGuid", "TEXT"},
	{"table_test", "InfoID", "TEXT"},
	{"table_test", "Info", "TEXT"},
	{"table_test", "Confidence", "FLOAT"},
	{"table_test", "RecordDateTime", "TIMESTAMP"},

	{"table_test", "sendTime", "TIMESTAMP"},
	{"table_test", "sendFlag", "INTERGE"}};
void dbt_test()
{
//	dbt_mkdir_from_path("tttt1/t1/t");
//	dbt_mkdir_from_path("tttt11/t11/t/");
//	dbt_mkdir_from_path("./tttt2/t2/t");
//	dbt_mkdir_from_path("./tttt22/t22/t/");
	dbt_check("/home/nvidia/dbt_test.db", table_test, sizeof(table_test)/sizeof(DBT_Table));
	dbt_check("/home/nvidia/dbt_test.db", table_test, sizeof(table_test)/sizeof(DBT_Table));
	dbt_check("/home/nvidia/dbt_test.db", table_test, sizeof(table_test)/sizeof(DBT_Table));

	dbt_check("/home/nvidia/dbt_test.db", table_test1, sizeof(table_test1)/sizeof(DBT_Table));
	dbt_check("/home/nvidia/dbt_test.db", table_test1, sizeof(table_test1)/sizeof(DBT_Table));
	dbt_check("/home/nvidia/dbt_test.db", table_test1, sizeof(table_test1)/sizeof(DBT_Table));
}




