//
// Created by lining on 2023/2/24.
//

#ifndef FILEFUN_H
#define FILEFUN_H

#include <glog/logging.h>
#include <fstream>
#include <iomanip>
#include <openssl/md5.h>
#include <curl/curl.h>
#include <fcntl.h>
#include "dns/DNSServer.h"
#include "os/os.h"

#define HOME_PATH "/home/nvidianx"
#define UPDATEFILE "zkj_update.zip"
#define UPDATEUNZIPFILE HOME_PATH"/zkj_update_tmp"  // 升级包解压路径
#define INSTALLSH "update.sh"  // 升级脚本


std::string getFileMD5(std::string file);

extern int downloadTotal;
extern int downloaded;
int downloadFile(std::string url, int timeout, std::string fileName, int fileSize, std::string fileMD5);
int extractFile(std::string filePath);

extern std::string exePath;
int startUpgrade();

#endif //FILEFUN_H
