/*
 * 这个文件包括sqlite操作相关api函数
 *
 * 创建日期:20170803
 * 修改内容:
 * 
 */

#include <stdio.h>
#include "sqlite_api.h"
#include "logger.h"
#include "sqlite3.h"
#include <unistd.h>

int open_database(char *addr,sqlite3 **db)
{
    int result;
    
    result = sqlite3_open(addr,db);                        
    if (result != SQLITE_OK)
    {
        ERR("fuc open_database opendb=%s error.",addr);
        return -1;
    }
    
    return 0;
}

int exec_sql(sqlite3 *db,char *sql_string,SQLITE3_CALLBACK sql_callback,void *param)
{
    int result;
    char *errMsg;
    
    result = sqlite3_exec(db,sql_string,sql_callback,param,&errMsg);
    if (result != SQLITE_OK)
    {
        ERR("fuc exec_sql sql_string=%s ;errMsg = %s.error.",sql_string,errMsg);
        sqlite3_free(errMsg);
        return -2;
    }
    sqlite3_free(errMsg);
    
    return 0;
}

int exec_get_table(sqlite3 *db,char *sql_string,char ***data,int *row,int *col)
{
    int ret;
    char *errMsg;
    ret = sqlite3_get_table(db, sql_string, data, row, col, &errMsg);
    if (ret != SQLITE_OK) 
    {
        ERR("fuc exec_get_table sql_string=%s error=%s.",sql_string,errMsg);
        sqlite3_free(errMsg);
        return -1;
    }
    sqlite3_free(errMsg);
    
    return ret;
}

int close_database(sqlite3 *db)
{
    if (db == NULL)
    {
        return -1;
    }
    sqlite3_close(db);
    return 0;
}

int callback_db(void *ptr,int count)
{  
//    DBG("database is locak now,can not write/read.count= %d.",count);  //每次执行一次回调函数打印一次该信息
    usleep(100*1000);   //如果获取不到锁，等待100毫秒
    return 1;    //回调函数返回值为1，则将不断尝试操作数据库。
}


int db_exec_sql(char *addr,char *sql_string,SQLITE3_CALLBACK sql_callback,void *param)
{
    int ret = 0;
    sqlite3 *db;
    
    ret = open_database(addr,&db);
    if (ret)
    {
        return -1;
    }

    sqlite3_busy_handler(db,callback_db,(void *)db);

    ret = exec_sql(db,sql_string,sql_callback,param);

    close_database(db);

    return ret;
}

int db_file_exec_sql(char *sql_file,char *sql_string,SQLITE3_CALLBACK sql_callback,void *param)
{
    int ret = 0;
    sqlite3 *db;
    
	ret = sqlite3_open(sql_file,&db);                        
    if (ret != SQLITE_OK)
    {
        ERR("fuc open_database opendb=%s error.",sql_file);
        return -1;
    }

    sqlite3_busy_handler(db,callback_db,(void *)db);

    ret = exec_sql(db,sql_string,sql_callback,param);

    close_database(db);

    return ret;
}


/*需要在外面释放data：sqlite3_free_table( dbResult );*/
int db_exec_sql_table(char *addr,char *sql_string,char ***data,int *row,int *col)
{
    int ret = 0;
    sqlite3 *db;

    ret = open_database(addr,&db);
    if (ret)
    {
        return -1;
    }

    sqlite3_busy_handler(db,callback_db,(void *)db);

    ret = exec_get_table(db,sql_string,data,row,col);

    close_database(db);

    return ret;
}

/*需要在外面释放data：sqlite3_free_table( dbResult );*/
int db_file_exec_sql_table(char *sql_file,char *sql_string,char ***data,int *row,int *col)
{
    int ret = 0;
    sqlite3 *db;

	ret = sqlite3_open(sql_file,&db);                        
    if (ret != SQLITE_OK)
    {
        ERR("fuc open_database opendb=%s error.",sql_file);
        return -1;
    }

    sqlite3_busy_handler(db,callback_db,(void *)db);

    ret = exec_get_table(db,sql_string,data,row,col);

    close_database(db);

    return ret;
}

void sqlite3_free_table_safe(char **result)
{
    if(result != NULL)
    {
	    sqlite3_free_table(result);
    }
	else
	{}

	return;
}


