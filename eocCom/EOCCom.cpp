//
// Created by lining on 2023/2/23.
//

#include "EOCCom.h"
#include "db/DBTable.h"
#include <glog/logging.h>
#include "fileFun.h"

EOCCom::EOCCom(string ip, int port, string caPath) : TlsClient(ip, port, caPath) {

}

int EOCCom::Open() {
    int ret = 0;
    ret = TlsClient::Open();
    if (ret != 0) {
        LOG(ERROR) << "eoc server connect fail";
    }
    return ret;
}

int EOCCom::Run() {
    int ret = 0;
    ret = TlsClient::Run();
    businessStart = true;
    future_getPkgs = std::async(std::launch::async, getPkgs, this);

    future_proPkgs = std::async(std::launch::async, proPkgs, this);

    future_interval = std::async(std::launch::async, intervalPro, this);
    return ret;
}

int EOCCom::Close() {
    int ret = 0;
    ret = TlsClient::Close();
    if (businessStart) {
        try {
            future_getPkgs.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }

        try {
            future_proPkgs.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }

        try {
            future_interval.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }
    }
    businessStart = false;
    return ret;
}

int EOCCom::getPkgs(void *p) {
    if (p == nullptr) {
        return -1;
    }
    EOCCom *local = (EOCCom *) p;
    LOG(INFO) << __FUNCTION__ << "run";
    char *buf = new char[4096];
    int index = 0;
    memset(buf, 0, 4096);
    while (local->isLive.load()) {
        usleep(1000);
        while (local->rb->GetReadLen() > 0) {
            char value = 0x00;
            if (local->rb->Read(&value, 1) > 0) {
                if (value == '*') {
                    //得到分包尾部
                    std::string pkg = std::string(buf);
                    local->pkgs.push(pkg);
                    index = 0;
                    memset(buf, 0, 4096);
                } else {
                    buf[index] = value;
                    index++;
                }
            }
        }
    }
    delete[] buf;
    LOG(INFO) << __FUNCTION__ << "exit";
    return 0;
}

#define COMVersion "V1.0.0"

//100
void processS100(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }

}

void processR100(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
    LOG(INFO) << "recv:" << cmd;
    local->heartFlag = 0;
}

//101
void processS101(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

void processR101(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
    LOG(INFO) << "recv:" << cmd;
    //登录分析
    R101 r101;
    if (r101.JsonUnmarshal(in)) {
        switch (r101.Data.State) {
            case 1: {
                LOG(INFO) << "登录成功,信息：" << r101.Data.Message;
                local->isLogIn = true;
            }
                break;
            default: {
                LOG(INFO) << "登录失败，信息：" << r101.Data.Message;
            }
                break;
        }
    } else {
        LOG(ERROR) << cmd << " json unmarshal fail";
    }
}

//102
void processS102(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

void processR102(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
    //处理配置下发
    R102 r102;
    if (r102.JsonUnmarshal(in)) {
        int result = 0;
        //核心板基础配置
        DBBaseSet dbBaseSet;
        dbBaseSet.deleteFromDB();
        EOCCom::convertBaseSetS2DB(r102.Data.BaseSetting, dbBaseSet, r102.Data.Index);
        if (dbBaseSet.insertToDB() == 0) {
            LOG(INFO) << "核心板基础配置,写入成功";
        } else {
            LOG(INFO) << "核心板基础配置,写入失败";
            result = -1;
        }
        //所属路口信息
        DBIntersection dbIntersection;
        dbIntersection.deleteFromDB();
        EOCCom::convertIntersectionS2DB(r102.Data.IntersectionInfo, dbIntersection);
        if (dbIntersection.insertToDB() == 0) {
            LOG(INFO) << "所属路口信息,写入成功";
        } else {
            LOG(INFO) << "所属路口信息,写入失败";
            result = -1;
        }
        //融合参数
        DBFusionParaSet dbFusionParaSet;
        dbFusionParaSet.deleteFromDB();
        EOCCom::convertFusion_Para_SetS2DB(r102.Data.FusionParaSetting, dbFusionParaSet);
        if (dbFusionParaSet.insertToDB() == 0) {
            LOG(INFO) << "融合参数,写入成功";
        } else {
            LOG(INFO) << "融合参数,写入失败";
            result = -1;
        }
        //关联设备
        DBAssociatedEquip dbAssociatedEquip1;
        dbAssociatedEquip1.deleteAllFromDB();
        for (auto iter: r102.Data.AssociatedEquips) {
            DBAssociatedEquip dbAssociatedEquip;
            EOCCom::convertAssociated_EquipS2DB(iter, dbAssociatedEquip);
            if (dbAssociatedEquip.insertToDB() == 0) {
                LOG(INFO) << "关联设备,写入成功" << iter.EquipCode;
            } else {
                LOG(INFO) << "关联设备,写入失败" << iter.EquipCode;
                result = -1;
            }
        }
        //数据版本
        std::string version = r102.Data.DataVersion;
        if (!version.empty()) {
            DBDataVersion dbDataVersion;
            dbDataVersion.deleteFromDB();
            dbDataVersion.version = version;
            dbDataVersion.id = 0;

            time_t now;
            struct tm timenow;
            time(&now);
            localtime_r(&now, &timenow);
            char time_p[50] = {0};
            sprintf(time_p, "%u-%02u-%02u %02u:%02u:%02u",
                    1900 + timenow.tm_year, 1 + timenow.tm_mon, timenow.tm_mday,
                    timenow.tm_hour, timenow.tm_min, timenow.tm_sec);

            dbDataVersion.time = std::string(time_p);

            if (dbDataVersion.insertToDB() == 0) {
                LOG(INFO) << "write config version success:" << dbDataVersion.version << " ,"
                          << dbDataVersion.time;
            } else {
                LOG(INFO) << "write config version fail:" << dbDataVersion.version << " ,"
                          << dbDataVersion.time;
                result = -1;
            }
        } else {
            LOG(ERROR) << "config dataVersion empty";
            result = -1;
        }
        //配置处理完成后,发送S102信息 添加重启任务
        if (result != 0) {
            S102 s102;
            s102.get(COMVersion, r102.head.Guid, 0, "fail");

            //组json
            Json::FastWriter fastWriter;
            Json::Value out;
            s102.JsonMarshal(out);
            std::string body;
            body = fastWriter.write(out);
            body.push_back('*');
            int len = local->Write(body.data(), body.size());

            if (len < 0) {
                LOG(ERROR) << "s102_send err, return:" << len;
            } else {
                LOG(INFO) << "s102_send ok";
            }
            LOG(INFO) << "配置写入失败";
        } else {
            S102 s102;
            s102.get(COMVersion, r102.head.Guid, 1, "ok");

            //组json
            Json::FastWriter fastWriter;
            Json::Value out;
            s102.JsonMarshal(out);
            std::string body;
            body = fastWriter.write(out);
            body.push_back('*');
            int len = local->Write(body.data(), body.size());

            if (len < 0) {
                LOG(ERROR) << "s102_send err, return:" << len;
            } else {
                LOG(INFO) << "s102_send ok";
            }
            LOG(INFO) << "配置写入成功，5s添加后重启任务";
            sleep(5);
            auto tasks = &local->eocCloudData.task;
            tasks->push_back(EOCCom::SYS_REBOOT);
        }

    } else {
        LOG(ERROR) << cmd << " json unmarshal fail";
    }
}

//103
void processS103(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

void processR103(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

//104
void processS104(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

void processR104(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
    //接收到下发后，按r102处理
    LOG(INFO) << "接受到R104配置下发，按R102处理";
    processR102(local, content, cmd);

}

//105
void processS105(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

void processR105(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
    R105 r105;
    if (r105.JsonUnmarshal(in)) {
        int state = r105.Data.State;
        switch (state) {
            case 0: {
                LOG(INFO) << "外网状态发送失败，信息：" << r105.Data.Message;
            }
                break;
            case 1: {
                LOG(INFO) << "外网状态发送成功，信息：" << r105.Data.Message;
            }
                break;
            default: {
                LOG(INFO) << "外网状态发送:" << r105.Data.State << "，信息：" << r105.Data.Message;
            }
                break;
        }
    } else {
        LOG(ERROR) << cmd << " json unmarshal fail";
    }
}

//106
void processS106(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

static void ThreadDownload(EOCCom *local, std::string url) {
    if (local == nullptr) {
        return;
    }
    LOG(INFO) << "download thread,url:" << url << " start";
    std::string updateFileName;
    auto msgDownload = &local->eocCloudData.downloads_msg;
    bool isUrlExist = false;
    for (int i = 0; i < msgDownload->size(); i++) {
        auto iter = &msgDownload->at(i);
        LOG(INFO) << "index:" << i << ",download url:" << iter->download_url;
        //如果有链接和要下载的链接相同
        if (iter->download_url == url) {
            isUrlExist = true;
            //找到下载链接信息
            if (iter->upgrade_dev.empty()) {
                //主控板升级下载文件
                updateFileName = UPDATEFILE;
            } else {
                if (iter->upgrade_dev.at(0).dev_type == EOCCom::EOC_UPGRADE_PARKINGAREA) {
                    //主控板升级下载文件
                    updateFileName = UPDATEFILE;
                } else {
                    //相机或矩阵控制器升级下载文件
                    updateFileName = iter->download_file_md5 + "_" + iter->download_file_name;
                }
            }
            //正式下载文件
            LOG(INFO) << "正式下载文件";
            int result = downloadFile(iter->download_url, 8 * 60 * 1000,
                                      updateFileName, iter->download_file_size, iter->download_file_md5);

            switch (result) {
                case 0: {
                    LOG(INFO) << "下载完成:" << iter->download_url;
                    iter->download_status = EOCCom::DOWNLOAD_FINISHED;
                }
                    break;
                case -1: {
                    LOG(ERROR) << "下载文件不存在:" << iter->download_url;
                    iter->download_status = EOCCom::DOWNLOAD_FILE_NOT_EXIST;
                }
                    break;
                case -2: {
                    LOG(ERROR) << "下载超时:" << iter->download_url;
                    iter->download_status = EOCCom::DOWNLOAD_TIMEOUT;
                }
                    break;
                case -3: {
                    LOG(ERROR) << "下载本地剩余空间不足:" << iter->download_url;
                    iter->download_status = EOCCom::DOWNLOAD_SPACE_NOT_ENOUGH;
                }
                    break;
                case -4: {
                    LOG(ERROR) << "下载失败MD5校验失败:" << iter->download_url;
                    iter->download_status = EOCCom::DOWNLOAD_MD5_FAILD;
                }
                    break;
            }
        }
    }
    if (!isUrlExist) {
        LOG(ERROR) << "search download url err";
    }

    LOG(INFO) << "download thread,url:" << url << " exit";
}

void processR106(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
    //处理软件下载文件指令，将会把文件下载到指定目录下
    LOG(INFO) << "处理软件下载文件指令，将会把文件下载到指定目录下";
    R106 r106;
    if (r106.JsonUnmarshal(in)) {
        LOG(INFO) << "eoc download file:" << r106.Data.DownloadPath <<
                  ",file:" << r106.Data.FileName << ",size:" << r106.Data.FileSize << ",md5:" << r106.Data.FileMD5;
        LOG(INFO) << "file local store:" << UPDATEFILE;
        //发送S106信息
        S106 s106;
        s106.get(COMVersion, r106.head.Guid, 1, 1, 0, "Receive controller download msg succeed");
        //组json
        Json::FastWriter fastWriter;
        Json::Value out;
        s106.JsonMarshal(out);
        std::string body;
        body = fastWriter.write(out);
        body.push_back('*');
        int len = local->Write(body.data(), body.size());

        if (len < 0) {
            LOG(ERROR) << "s106_send err, return:" << len;
        } else {
            LOG(INFO) << "s106_send ok";
        }
        //判断文件是否存在，已存在直接返回下载成功
        if (access(UPDATEFILE, R_OK | R_OK) == 0) {
            LOG(INFO) << "文件已存在:" << UPDATEFILE;
            //判断md5
            std::string md5 = getFileMD5(UPDATEFILE);
            LOG(INFO) << "MD5 cal:" << md5 << "," << r106.Data.FileMD5;
            if (md5 == r106.Data.FileMD5) {
                LOG(INFO) << "下载文件MD5校验通过";
                //发送s106
                S106 s106_md5success;
                s106_md5success.get(COMVersion, r106.head.Guid, 2, 1, 0, "下载完成 MD5 OK");
                //组json
                Json::FastWriter fastWriter;
                Json::Value out;
                s106_md5success.JsonMarshal(out);
                std::string body;
                body = fastWriter.write(out);
                body.push_back('*');
                int len = local->Write(body.data(), body.size());

                if (len < 0) {
                    LOG(ERROR) << "s106_md5success_send err, return:" << len;
                } else {
                    LOG(INFO) << "s106_md5success_send ok";
                }
                return;
            }
        } else {
            //文件不存在或者md5校验失败，添加下载任务
            //下载任务信息
            EOCCom::EocDownloadsMsg rcv_download_msg;
            EOCCom::EocUpgradeDev rcv_dev_msg;
            rcv_download_msg.download_file_name = r106.Data.FileName;
            rcv_download_msg.download_file_size = atoi(r106.Data.FileSize.c_str());
            rcv_download_msg.download_url = r106.Data.DownloadPath;
            rcv_download_msg.download_file_md5 = r106.Data.FileMD5;
            rcv_dev_msg.dev_guid.clear();
            rcv_dev_msg.dev_ip.clear();
            rcv_dev_msg.dev_type = EOCCom::EOC_UPGRADE_PARKINGAREA;
            rcv_dev_msg.comm_guid = r106.head.Guid;
            bool isDownloadExist = false;
            auto msgDownload = &local->eocCloudData.downloads_msg;
            for (int i = 0; i < msgDownload->size(); i++) {
                auto iter = msgDownload->at(i);
                if (iter.download_url == rcv_download_msg.download_url) {
                    //下载链接已存在
                    isDownloadExist = true;
                    LOG(INFO) << "download thread is running,add ip:" << rcv_dev_msg.dev_ip.c_str();
                    if (rcv_dev_msg.dev_type == EOCCom::EOC_UPGRADE_PARKINGAREA) {
                        //
                        LOG(INFO) << "雷视机升级下载已添加";
                        return;
                    }

                    bool isFirmwareDownloadExist = false;
                    for (int j = 0; j < iter.upgrade_dev.size(); j++) {
                        auto iter1 = iter.upgrade_dev.at(j);
                        if ((iter1.dev_ip == rcv_dev_msg.dev_ip) && (iter1.comm_guid == rcv_dev_msg.comm_guid)) {
                            LOG(INFO) << "固件下载已添加";
                            isFirmwareDownloadExist = true;
                        }
                    }
                    if (!isFirmwareDownloadExist) {
                        //如果是固件下载但是任务不存在
                        LOG(INFO) << "固件下载添加";
                        iter.upgrade_dev.push_back(rcv_dev_msg);
                    }
                    break;
                }
            }
            if (!isDownloadExist) {
                //下载链接不存在，添加下载
                rcv_download_msg.download_status = EOCCom::DOWNLOAD_IDLE;
                rcv_download_msg.upgrade_dev.clear();
                rcv_download_msg.upgrade_dev.push_back(rcv_dev_msg);
                local->eocCloudData.downloads_msg.push_back(rcv_download_msg);
                //创建一个下载的子线程
                LOG(INFO) << "添加下载子线程 detach";
                std::thread downloadThread = std::thread(ThreadDownload, local, rcv_download_msg.download_url);
                local->eocCloudData.downloads_msg.back().download_thread_id = downloadThread.get_id();
                downloadThread.detach();
            }

        }

    } else {
        LOG(ERROR) << cmd << " json unmarshal fail";
    }
}

//107
void processS107(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}

void processR107(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
    R107 r107;
    if (r107.JsonUnmarshal(in)) {
        //回送接收到升级命令
        S107 s107;
        s107.get(COMVersion, r107.head.Guid, 1, 1, 0, "接收到升级命令");
        //组json
        Json::FastWriter fastWriter;
        Json::Value out;
        s107.JsonMarshal(out);
        std::string body;
        body = fastWriter.write(out);
        body.push_back('*');
        int len = local->Write(body.data(), body.size());

        if (len < 0) {
            LOG(ERROR) << "s107_send err, return:" << len;
        } else {
            LOG(INFO) << "s107_send ok";
        }

        //1 判断文件是否存在
        if (access(UPDATEFILE, R_OK | R_OK) != 0) {
            LOG(ERROR) << "文件不存在," << UPDATEFILE;
            S107 s107_1;
            s107_1.get(COMVersion, r107.head.Guid, 2, 0, 0, "文件不存在");
            //组json
            Json::FastWriter fastWriter;
            Json::Value out;
            s107_1.JsonMarshal(out);
            std::string body;
            body = fastWriter.write(out);
            body.push_back('*');
            int len = local->Write(body.data(), body.size());

            if (len < 0) {
                LOG(ERROR) << "s107_send err, return:" << len;
            } else {
                LOG(INFO) << "s107_send ok";
            }
            return;
        }
        //2 MD5校验
        std::string md5 = getFileMD5(UPDATEFILE);
        if (md5 != r107.Data.FileMD5) {
            LOG(ERROR) << "md5 check fail,cal:" << md5 << "," << r107.Data.FileMD5;
            S107 s107_2;
            s107_2.get(COMVersion, r107.head.Guid, 2, 0, 0, "MD5校验失败");
            //组json
            Json::FastWriter fastWriter;
            Json::Value out;
            s107_2.JsonMarshal(out);
            std::string body;
            body = fastWriter.write(out);
            body.push_back('*');
            int len = local->Write(body.data(), body.size());

            if (len < 0) {
                LOG(ERROR) << "s107_send err, return:" << len;
            } else {
                LOG(INFO) << "s107_send ok";
            }
            return;
        }
        //3 版本校验... ...

        //校验完成开始升级
        LOG(INFO) << "校验完成开始升级";
        //4 执行升级文件
        if (extractFile(UPDATEFILE) != 0) {
            LOG(ERROR) << "解压缩失败:" << UPDATEFILE;
            S107 s107_3;
            s107_3.get(COMVersion, r107.head.Guid, 2, 0, 0, "解压缩失败");
            //组json
            Json::FastWriter fastWriter;
            Json::Value out;
            s107_3.JsonMarshal(out);
            std::string body;
            body = fastWriter.write(out);
            body.push_back('*');
            int len = local->Write(body.data(), body.size());

            if (len < 0) {
                LOG(ERROR) << "s107_send err, return:" << len;
            } else {
                LOG(INFO) << "s107_send ok";
            }
            return;
        }

        //发送升级完成
        {
            S107 s107_4;
            s107_4.get(COMVersion, r107.head.Guid, 2, 1, 0, "升级完成");
            //组json
            Json::FastWriter fastWriter;
            Json::Value out;
            s107_4.JsonMarshal(out);
            std::string body;
            body = fastWriter.write(out);
            body.push_back('*');
            int len = local->Write(body.data(), body.size());

            if (len < 0) {
                LOG(ERROR) << "s107_send err, return:" << len;
            } else {
                LOG(INFO) << "s107_send ok";
            }
        }
        //开始升级
        startUpgrade();

        EOCCom::EocUpgradeDev eocUpgradeDev;
        eocUpgradeDev.sw_version = r107.Data.FileVersion;
        eocUpgradeDev.hw_version = r107.Data.HardwareVersion;
        eocUpgradeDev.upgrade_mode = r107.Data.UpgradeMode;


    } else {
        LOG(ERROR) << cmd << " json unmarshal fail";
    }

}

//108
void processS108(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }
}


void processR108(void *p, string content, string cmd) {
    if (p == nullptr) {
        return;
    }
    EOCCom *local = (EOCCom *) p;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return;
    }

}

typedef void (*Process)(void *p, string content, string cmd);

static std::map<std::string, Process> business = {
        make_pair("MCCS100", processS100),
        make_pair("MCCR100", processR100),

        make_pair("MCCS101", processS101),
        make_pair("MCCR101", processR101),

        make_pair("MCCS102", processS102),
        make_pair("MCCR102", processR102),

        make_pair("MCCS103", processS103),
        make_pair("MCCR103", processR103),

        make_pair("MCCS104", processS104),
        make_pair("MCCR104", processR104),

        make_pair("MCCS105", processS105),
        make_pair("MCCR105", processR105),

        make_pair("MCCS106", processS106),
        make_pair("MCCR106", processR106),

        make_pair("MCCS107", processS107),
        make_pair("MCCR107", processR107),

        make_pair("MCCS108", processS108),
        make_pair("MCCR108", processR108),
};

std::string parseCode(std::string content) {
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "json parse fail:" << content << ",err:" << reader.getFormattedErrorMessages();
        return "";
    }
    std::string code = in["Code"].asString();
    return code;
}


int EOCCom::proPkgs(void *p) {
    if (p == nullptr) {
        return -1;
    }
    EOCCom *local = (EOCCom *) p;
    LOG(INFO) << __FUNCTION__ << "run";
    while (local->isLive.load()) {
        usleep(1000);
        std::string pkg;
        if (local->pkgs.pop(pkg)) {
            //接收到一包数据
            LOG(INFO) << "eoc recv json:" << pkg;


            std::string code = parseCode(pkg);
            if (code.empty()) {
                LOG(ERROR) << "recv code empty";
            } else {
                //按照code分别处理
                auto iter = business.find(code);
                if (iter != business.end()) {
                    //find
                    LOG(INFO) << "code:" << code << ",content:" << pkg;
                    //开启处理线程
//                thread process = thread(iter->second, client, content, cmd);
//                process.join();
                    iter->second(local, pkg, code);
                } else {
                    LOG(ERROR) << "code:" << code << " not find,content:" << pkg;
                }
            }

        }
    }
    LOG(INFO) << __FUNCTION__ << "exit";
    return 0;
}

int EOCCom::intervalPro(void *p) {
    if (p == nullptr) {
        return -1;
    }
    EOCCom *local = (EOCCom *) p;
    LOG(INFO) << __FUNCTION__ << "run";
    while (local->isLive.load()) {
        sleep(1);
        time_t now = time(NULL);
        if (!local->isLogIn) {
            //未登录的话，一直申请登录
            if (now - local->last_login_t > 10) {
                local->last_login_t = now;

                S101 s101;
                int ret = s101.get(COMVersion);
                if (ret != 0) {
                    LOG(ERROR) << "s101 get fail";
                    continue;
                }

                //组json
                Json::FastWriter fastWriter;
                Json::Value out;
                s101.JsonMarshal(out);
                std::string body;
                body = fastWriter.write(out);
                body.push_back('*');
                int len = local->Write(body.data(), body.size());

                if (len < 0) {
                    LOG(ERROR) << "s101_login_send err, return:" << len;
                } else {
                    LOG(INFO) << "s101_login_send ok";
                }
                local->last_send_heart_t = now - 20;
                local->heartFlag++;
                if (local->heartFlag > 10) {
                    local->isLive.store(false);
                }
            }
        } else {
            //登录后，每隔30s上传1次心跳，如果上次获取配置的时间大于180s且获取配置失败的话，主动发送获取配置命令，每300s上传一次外围状态
            //同时处理一些升级下载任务

            //1.获取配置
            std::string version;
            DBDataVersion dbDataVersion;
            dbDataVersion.selectFromDB();
            version = dbDataVersion.version;
            if (version.empty()) {
                if (now - local->last_get_config_t > 180) {
                    local->last_get_config_t = now;
                    //主动发起一次获取配置的请求
                    S104 s104;
                    int ret = s104.get(COMVersion);
                    if (ret != 0) {
                        LOG(ERROR) << "s104 get fail";
                        continue;
                    }

                    //组json
                    Json::FastWriter fastWriter;
                    Json::Value out;
                    s104.JsonMarshal(out);
                    std::string body;
                    body = fastWriter.write(out);
                    body.push_back('*');
                    int len = local->Write(body.data(), body.size());

                    if (len < 0) {
                        LOG(ERROR) << "s104 send err, return:" << len;
                    } else {
                        LOG(INFO) << "s104 send ok";
                    }
                }
            }

            //2.心跳
            if (now - local->last_send_heart_t > 30) {
                local->last_send_heart_t = now;
                S100 s100;
                int ret = s100.get(COMVersion);
                if (ret != 0) {
                    LOG(ERROR) << "s100 get fail";
                    continue;
                }

                //组json
                Json::FastWriter fastWriter;
                Json::Value out;
                s100.JsonMarshal(out);
                std::string body;
                body = fastWriter.write(out);
                body.push_back('*');
                int len = local->Write(body.data(), body.size());

                if (len < 0) {
                    LOG(ERROR) << "s100 send err, return:" << len;
                } else {
                    LOG(INFO) << "s100 send ok";
                }
            }

            //3.外网状态上传
            if (now - local->last_send_net_state_t > 300) {
                local->last_send_net_state_t = now;

                S105 s105;
                int ret = s105.get(COMVersion, local->sendNetTotal, local->sendNetSuccess);
                if (ret != 0) {
                    LOG(ERROR) << "s105 get fail";
                    continue;
                }

                //组json
                Json::FastWriter fastWriter;
                Json::Value out;
                s105.JsonMarshal(out);
                std::string body;
                body = fastWriter.write(out);
                body.push_back('*');
                int len = local->Write(body.data(), body.size());
                local->sendNetTotal++;

                if (len < 0) {
                    LOG(ERROR) << "s105 send err, return:" << len;
                } else {
                    LOG(INFO) << "s105 send ok";
                    if (local->sendNetTotal == 0) {
                        local->sendNetSuccess = 0;
                    } else {
                        local->sendNetSuccess++;
                    }
                }
            }
            //4.升级、下载
            //4.1处理下载消息
            auto msgDownload = &local->eocCloudData.downloads_msg;
            for (auto iter = msgDownload->begin(); iter != msgDownload->end();) {
                //如果不是下载暂停则发送信息给eoc
                if (iter->download_status != EOCCom::DOWNLOAD_IDLE) {
                    //发送信息给eoc
                    //遍历设备类型数组
                    for (auto &iter1: iter->upgrade_dev) {
                        if (iter1.dev_type == EOCCom::EOC_UPGRADE_PARKINGAREA) {
                            //如果是则
                            S106 s106;
                            //软件升级
                            switch (iter->download_status) {
                                case EOCCom::DOWNLOAD_FINISHED: {
                                    //下载完成
                                    s106.get(COMVersion, iter1.comm_guid, 2, 1, 0, "下载完成 MD5 OK");
                                }
                                    break;
                                case EOCCom::DOWNLOAD_FILE_NOT_EXIST: {
                                    //文件不存在
                                    s106.get(COMVersion, iter1.comm_guid, 2, 0, 0, "下载文件不存在");
                                }
                                    break;
                                case EOCCom::DOWNLOAD_TIMEOUT: {
                                    //下载超时
                                    s106.get(COMVersion, iter1.comm_guid, 2, 0, 0, "下载超时");
                                }
                                    break;
                                case EOCCom::DOWNLOAD_SPACE_NOT_ENOUGH: {
                                    //下载空间不足
                                    s106.get(COMVersion, iter1.comm_guid, 2, 0, 0, "本地剩余空间不足");
                                }
                                    break;
                                case EOCCom::DOWNLOAD_MD5_FAILD: {
                                    //下载MD5文件失败
                                    s106.get(COMVersion, iter1.comm_guid, 2, 0, 0, "MD5校验失败");
                                }
                                    break;
                            }
                            //发送
                            //组json
                            Json::FastWriter fastWriter;
                            Json::Value out;
                            s106.JsonMarshal(out);
                            std::string body;
                            body = fastWriter.write(out);
                            body.push_back('*');
                            int len = local->Write(body.data(), body.size());

                            if (len < 0) {
                                LOG(ERROR) << "s106 send err, return:" << len;
                                //发送失败立即退出循环
                                break;
                            } else {
                                LOG(INFO) << "s106 send ok";
                            }
                        }
                    }

                    iter = msgDownload->erase(iter); //删除元素，返回值指向已删除元素的下一个位置
                } else {
                    ++iter;
                }
            }
            //4.2升级
            auto msgUpgrade = &local->eocCloudData.upgrade_msg;

            //5.任务处理
            auto tasks = &local->eocCloudData.task;
            if (!tasks->empty()) {
                //把任务拷贝出来，删除原数组上位置内容，查看
                EOCCom::EOCCLOUD_TASK task = tasks->at(0);
                tasks->erase(tasks->begin());
                switch (task) {
                    case EOCCom::SYS_REBOOT: {
                        //判断是否有其他任务以及未传送完的信息
                        if (!tasks->empty() || !msgDownload->empty() || !msgDownload->empty()) {
                            //在后续添加重启任务
                            LOG(INFO) << "有未完成的任务，添加稍后重启任务";
                            tasks->push_back(EOCCom::SYS_REBOOT);
                        } else {
                            LOG(INFO) << "5秒后重启";
                            sleep(5);
                            exit(0);
                        }
                    }
                        break;
                    default: {
                        LOG(INFO) << "不支持的任务类型,task:" << task;
                    }
                        break;
                }
            }
        }
    }
    LOG(INFO) << __FUNCTION__ << "exit";
    return 0;
}

void EOCCom::convertBaseSetS2DB(class BaseSettingEntity in, DBBaseSet &out, int index) {
    out.Index = index;
    out.City = in.City;
    out.IsUploadToPlatform = in.IsUploadToPlatform;
    out.Is4Gmodel = in.Is4Gmodel;
    out.IsIllegalCapture = in.IsIllegalCapture;
    out.IsPrintIntersectionName = in.IsPrintIntersectionName;
    out.FilesServicePath = in.FilesServicePath;
    out.FilesServicePort = in.FilesServicePort;
    out.MainDNS = in.MainDNS;
    out.AlternateDNS = in.AlternateDNS;
    out.PlatformTcpPath = in.PlatformTcpPath;
    out.PlatformTcpPort = in.PlatformTcpPort;
    out.PlatformHttpPath = in.PlatformHttpPath;
    out.PlatformHttpPort = in.PlatformHttpPort;
    out.SignalMachinePath = in.SignalMachinePath;
    out.SignalMachinePort = in.SignalMachinePort;
    out.IsUseSignalMachine = in.IsUseSignalMachine;
    out.NtpServerPath = in.NtpServerPath;
    out.FusionMainboardIp = in.FusionMainboardIp;
    out.FusionMainboardPort = in.FusionMainboardPort;
    out.IllegalPlatformAddress = in.IllegalPlatformAddress;
    out.Remarks = in.Remarks;
}

void EOCCom::convertIntersectionS2DB(class IntersectionEntity in, DBIntersection &out) {
    out.Guid = in.Guid;
    out.Name = in.Name;
    out.Type = in.Type;
    out.PlatId = in.PlatId;
    out.XLength = in.XLength;
    out.YLength = in.YLength;
    out.LaneNumber = in.LaneNumber;
    out.Latitude = in.Latitude;
    out.Longitude = in.Longitude;

    out.FlagEast = in.IntersectionBaseSetting.FlagEast;
    out.FlagSouth = in.IntersectionBaseSetting.FlagSouth;
    out.FlagWest = in.IntersectionBaseSetting.FlagWest;
    out.FlagNorth = in.IntersectionBaseSetting.FlagNorth;
    out.DeltaXEast = in.IntersectionBaseSetting.DeltaXEast;
    out.DeltaYEast = in.IntersectionBaseSetting.DeltaYEast;
    out.DeltaXSouth = in.IntersectionBaseSetting.DeltaXSouth;
    out.DeltaYSouth = in.IntersectionBaseSetting.DeltaYSouth;
    out.DeltaXWest = in.IntersectionBaseSetting.DeltaXWest;
    out.DeltaYWest = in.IntersectionBaseSetting.DeltaYWest;
    out.DeltaXNorth = in.IntersectionBaseSetting.DeltaXNorth;
    out.DeltaYNorth = in.IntersectionBaseSetting.DeltaYNorth;
    out.WidthX = in.IntersectionBaseSetting.WidthX;
    out.WidthY = in.IntersectionBaseSetting.WidthY;
}

void EOCCom::convertFusion_Para_SetS2DB(class FusionParaEntity in, DBFusionParaSet &out) {
    out.IntersectionAreaPoint1X = in.IntersectionAreaPoint1X;
    out.IntersectionAreaPoint1Y = in.IntersectionAreaPoint1Y;

    out.IntersectionAreaPoint2X = in.IntersectionAreaPoint2X;
    out.IntersectionAreaPoint2Y = in.IntersectionAreaPoint2Y;

    out.IntersectionAreaPoint3X = in.IntersectionAreaPoint3X;
    out.IntersectionAreaPoint3Y = in.IntersectionAreaPoint3Y;

    out.IntersectionAreaPoint4X = in.IntersectionAreaPoint4X;
    out.IntersectionAreaPoint4Y = in.IntersectionAreaPoint4Y;
}

void EOCCom::convertAssociated_EquipS2DB(class AssociatedEquip in, DBAssociatedEquip &out) {
    out.EquipType = in.EquipType;
    out.EquipCode = in.EquipCode;
}

