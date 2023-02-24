//
// Created by lining on 2023/2/24.
//

#include "fileFun.h"

std::string getFileMD5(std::string file) {
    std::ifstream inStream(file, std::ios::binary);
    if (inStream.is_open()) {
        MD5_CTX ctx;
        MD5_Init(&ctx);
        char *buf = new char[1024];
        while (!inStream.eof()) {
            inStream.read(buf, 1024);
            std::streamsize num = inStream.gcount();
            MD5_Update(&ctx, buf, num);
        }
        memset(buf, 0, 1024);
        MD5_Final((unsigned char *) buf, &ctx);
        std::string ret;
        char hex[3];
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            memset(hex, 0, 3);
            snprintf(hex, 3, "%02x", buf[i]);
            ret.append(hex);
        }

        delete[] buf;
        inStream.close();

        return ret;
    } else {
        LOG(ERROR) << "file not open";
        return "";
    }
}

static ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = (const char *) vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;        /* and call write() again */
            else
                return (-1);            /* error */
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}

int downloadTotal = 0;
int downloaded = 0;
static int countDownloadPrint = 0;

static size_t download_file_callback(void *buffer, size_t size, size_t nmemb, void *userp) {
    static int count = 0;
    if (count++ % 20 == 0) {
        printf(".");
        fflush(stdout);
    }
    countDownloadPrint++;
    downloaded += (nmemb * size);
    std::string printPrefix;
    for (int i = 0; i < countDownloadPrint; i++) {
        printPrefix.append("#");
    }
    LOG(INFO) << printPrefix << "..." << "download:" << downloaded << ",total:" << downloadTotal;
    return writen(*(int *) userp, buffer, nmemb * size);
}


/**
 * 下载并保存到本地（程序运行当前路径下
 * @param url
 * @param timeout
 * @param fileName
 * @param fileSize
 * @param fileMD5
 * @return 0：成功
 * -1：下载文件不存在
 * -2：下载超时
 * -3：本地剩余空间不足
 */
int downloadFile(std::string url, int timeout, std::string fileName, int fileSize, std::string fileMD5) {
    LOG(INFO) << "downloadFile," << "url:" << url << ",timeout:" << timeout
              << "file name:" << fileName << ",size:" << fileSize << ",md5:" << fileMD5;

    int ret = 0;

    CURL *curl;
    CURLcode res;
    std::vector<char> fileBuf;
    struct curl_slist *headerlist = nullptr;

    int fd = open(fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd <= 0) {
        LOG(ERROR) << "create file fail:" << fileName;
        return -3;
    }
    std::string ipurl;
    std::string host, port, ipaddr;
    searchDNS(url, ipurl, 0, host, port, ipaddr);
    host = "Host:" + host;
    port = "Port:" + port;
    LOG(INFO) << "download " << host << "," << port;
    headerlist = curl_slist_append(headerlist, "Expect:");
    //headerlist = curl_slist_append(headerlist, "Accept: application/json");
    headerlist = curl_slist_append(headerlist, host.c_str());
    headerlist = curl_slist_append(headerlist, port.c_str());

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);            //接收数据时超时设置，如果10秒内数据未接收完，直接退出
        //curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);                       //查找次数，防止查找太深
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);      //连接超时，这个数值如果设置太短可能导致数据请求不到就断开了
        curl_easy_setopt(curl, CURLOPT_URL, (char *) ipurl.c_str());//请求的URL
        downloadTotal = fileSize;
        downloaded = 0;
        countDownloadPrint = 0;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_file_callback);     //数据回来后的回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fd);              //回调函数里面用到的参数
        //Todo:302重定向问题
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);    // 设置重定向的最大次数
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);    // 设置301、302跳转跟随location
//      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);                         //可视化调试

        res = curl_easy_perform(curl);
        long http_response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response_code);
        LOG(INFO) << "res:" << res << ",resp code:" << http_response_code;

        if (res == CURLE_OK && http_response_code == 200) {
            //请求成功
            LOG(INFO) << "download file:" << fileName << ",from url:" << url << "success";
            ret = 0;
            std::string md5 = getFileMD5(fileName);
            if (md5 != fileMD5) {
                LOG(ERROR) << "md5 check fail, cal:" << md5 << "," << fileMD5;
                ret = -4;
            }
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            LOG(INFO) << "download file:" << url << "," << fileName << ",fail,err:" << curl_easy_strerror(res);
            ret = -2;
        } else if (res == CURLE_WRITE_ERROR) {
            LOG(INFO) << "download file:" << url << "," << fileName << ",fail,err:" << curl_easy_strerror(res);
            ret = -3;
        } else {
            LOG(INFO) << "download file:" << url << "," << fileName << ",fail,err:" << curl_easy_strerror(res);
            ret = -1;
        }
        curl_easy_cleanup(curl);
    }
    curl_slist_free_all(headerlist);
    close(fd);
    return ret;

}

int extract_file(const char *file_path) {
    int ret = 0;
    char cmd[256] = {0};
    memset(cmd, 0, 256);
    sprintf(cmd, "rm -rf %s", UPDATEUNZIPFILE);
    LOG(INFO) << "Extract file start,cmd=" << cmd;
    ret = os::execute_command(cmd);
    if (ret < 0) {
        LOG(ERROR) << "exec cmd err" << cmd;
        return -1;
    }
    memset(cmd, 0, 256);
    sprintf(cmd, "mkdir -p %s", UPDATEUNZIPFILE);
    LOG(INFO) << "Extract file start,cmd=" << cmd;
    ret = os::execute_command(cmd);
    if (ret != 0) {
        LOG(ERROR) << "exec cmd err" << cmd;
        return -1;
    }
    memset(cmd, 0, 256);
    sprintf(cmd, "tar -zxf %s -C %s", file_path, UPDATEUNZIPFILE);
    LOG(INFO) << "Extract file start,cmd=" << cmd;
    ret = os::execute_command(cmd);
    if (ret < 0) {
        LOG(ERROR) << "exec cmd err" << cmd;
        return -1;
    }

    if (remove(file_path) != 0) {
        LOG(ERROR) << "remove err:" << file_path;
        return -1;
    }

    LOG(INFO) << "Extract file succeed";
    return 0;
}

int start_upgrade() {
    int ret = 0;
    LOG(INFO) << "开始升级";
    char cmd[256] = {0};
    memset(cmd, 0, 256);
    sprintf(cmd, "chmod +x %s/%s", UPDATEUNZIPFILE, INSTALLSH);
    LOG(INFO) << "Extract file start,cmd=" << cmd;
    ret = os::execute_command(cmd);
    if (ret < 0) {
        LOG(ERROR) << "exec cmd err" << cmd;
        return -1;
    }
    memset(cmd, 0, 256);
    sprintf(cmd, "sh %s/%s %s", UPDATEUNZIPFILE, INSTALLSH, HOME_PATH);
    DLOG(INFO) << "Extract file start,cmd=" << cmd;
    ret = os::execute_command(cmd);
    if (ret < 0) {
        LOG(ERROR) << "exec cmd err" << cmd;
        return -1;
    }
    LOG(INFO) << "upgrade succeed";
    return 0;
}


