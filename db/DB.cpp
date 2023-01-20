//
// Created by lining on 8/11/22.
//

#include "db/DB.h"
#include <sqlite3.h>
#include <iostream>

string eocConfigDB = "eocConfig.db";

static int CallbackGetEocConfigCloud(void *data, int argc, char **argv, char **azColName) {
    string colName;
    if (data != nullptr) {
        auto config = (EocConfigCloud *) data;
        for (int i = 0; i < argc; i++) {
            colName = string(azColName[i]);
            if (colName.compare("plateformTcpPath") == 0) {
                config->ip = string(argv[i]);
                cout << "get cloud ip from db:" + config->ip << endl;
            }
            if (colName.compare("plateformTcpPort") == 0) {
                config->port = atoi(argv[i]);
                cout << "get cloud port from db:" + to_string(config->port) << endl;
            }
        }
    }

    return 0;
}

void getCloudConfigFromDb(EocConfigCloud *config) {

    sqlite3 *db;
    char *errmsg = nullptr;

    string dbName;
#ifdef aarch64
    dbName = "/home/nvidianx/bin/" + eocConfigDB;
#else
    dbName = eocConfigDB;
#endif


    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        printf("sqlite open fail\n");
        sqlite3_close(db);
    }

    //base
    char *sqlGetEocConfigCloud = "select * from baseSettingEntity";
    rc = sqlite3_exec(db, sqlGetEocConfigCloud, CallbackGetEocConfigCloud, config, &errmsg);
    if (rc != SQLITE_OK) {
        printf("sqlite err:%s\n", errmsg);
        sqlite3_free(errmsg);
    }

}
