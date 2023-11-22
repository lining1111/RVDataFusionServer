//
// Created by lining on 2023/10/26.
//

#ifndef TABLEINFO_H
#define TABLEINFO_H

#include <mutex>
#include <string>

typedef struct {
    std::mutex *mtx;
    std::string path;
    std::string version;
} DBInfo;

typedef struct {
    std::string tableName;            //数据库表名
    std::string columnName;            //列名
    std::string columnDescription;    //列描述
} DBTableInfo;

/****表字段检查配置******/
typedef struct {
    std::string name;
    std::string type;
} DBTableColInfo;

/**
 * 校验一个数据库表，如果可能会根据描述创建一个表
 * @param dbFile
 * @param table
 * @param column_size
 * @return 0 成功  -1 失败
 */
int checkTable(std::string dbFile, const DBTableInfo *table, int column_size);

#endif //TABLEINFO_H
