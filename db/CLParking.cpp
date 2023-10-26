//
// Created by lining on 2023/10/26.
//

#include "CLParking.h"
#include <glog/logging.h>
#include "sqliteApi.h"

namespace CLParking {

#define HOME_PATH "/home/nvidianx"

    DBInfo dbInfo = {
            .mtx = new std::mutex(),
            .path=HOME_PATH"/bin/CLParking.db",
            .version="V_1_0_0"};

    int dbGetUname(std::string &uname) {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int rtn = 0;
        char *sqlstr = new char[1024];
        char **sqldata;
        int nrow = 0;
        int ncol = 0;

        memset(sqlstr, 0, 1024);
        sprintf(sqlstr, "select UName from CL_ParkingArea where ID=(select max(ID) from CL_ParkingArea)");
        rtn = dbFileExecSqlTable(dbInfo.path, sqlstr, &sqldata, &nrow, &ncol);
        if (rtn < 0) {
            LOG(ERROR) << "db fail,sql:" << sqlstr;
            delete[] sqlstr;
            return -1;
        }
        if (nrow == 1) {
            uname = sqldata[1] ? sqldata[1] : "";
        } else {
            LOG(ERROR) << "db select count err,sql:" << sqlstr;
            delete[] sqlstr;
            dbFreeTable(sqldata);
            return 1;
        }
        delete[] sqlstr;
        dbFreeTable(sqldata);

        return 0;
    }

}