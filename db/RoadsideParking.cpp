//
// Created by lining on 2023/10/26.
//

#include "RoadsideParking.h"
#include <glog/logging.h>
#include "sqliteApi.h"

namespace RoadsideParking {

#define HOME_PATH "/home/nvidianx"

    DBInfo dbInfo = {
            .mtx = new std::mutex(),
//            .path=HOME_PATH"/bin/RoadsideParking.db",
            .path="./RoadsideParking.db",
            .version="V1_0"};

    int
    dbGetCloudInfo(std::string &server_path, int &server_port, std::string &file_server_path, int &file_server_port) {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int rtn = 0;
        char *sqlstr = new char[1024];
        char **sqldata;
        int nrow = 0;
        int ncol = 0;

        memset(sqlstr, 0, 1024);
        sprintf(sqlstr, "select CloudServerPath,TransferServicePath,CloudServerPort,FileServicePort from TB_ParkingLot "
                        "where ID=(select MIN(ID) from TB_ParkingLot)");
        rtn = dbFileExecSqlTable(dbInfo.path, sqlstr, &sqldata, &nrow, &ncol);
        if (rtn < 0) {
            LOG(ERROR) << "db fail sql:" << sqlstr;
            delete[] sqlstr;
            return -1;
        }
        if (nrow == 1) {
            server_path = sqldata[ncol + 0] ? sqldata[ncol + 0] : "";
            file_server_path = sqldata[ncol + 1] ? sqldata[ncol + 1] : "";
            server_port = atoi(sqldata[ncol + 2] ? sqldata[ncol + 2] : "0");
            file_server_port = atoi(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0");
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