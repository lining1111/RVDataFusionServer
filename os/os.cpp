//
// Created by lining on 9/30/22.
//

#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include "os.h"

using namespace std;

namespace os {

    int runCmd(const string cmd, string &out) {
        FILE *fp = popen(cmd.c_str(), "r");
        if (!fp) {
            return errno == 0 ? -1 : errno;
        }

//        char buf[1024 * 1024];
        char *buf = (char *) malloc(1024 * 1024);
        while (!feof(fp)) {
            memset(buf, 0, 1024 * 1024);
            size_t len = fread(buf, 1, 1024 * 1024, fp);
            if (len > 0) {
                out.append(buf, len);
            }
        }
        free(buf);
        return pclose(fp);
    }

    int GetVectorFromFile(vector<uint8_t> &array, string filePath) {
        ifstream fout;
        fout.open(filePath.c_str(), ios::in | ios::binary);
        if (fout.is_open()) {
            char val;
            while (fout.get(val)) {
                array.push_back(val);
            }
            fout.seekg(0, ios::beg);
            fout.close();
        } else {
            return -1;
        }

        return 0;
    }

    int GetFileFromVector(vector<uint8_t> &array, string filePath) {
        fstream fin;
        fin.open(filePath.c_str(), ios::out | ios::binary | ios::trunc);
        if (fin.is_open()) {
            for (auto iter:array) {
                fin.put(iter);
            }
            fin.flush();
            fin.close();
        } else {
            return -1;
        }

        return 0;
    }

    vector<string> split(const string &in, const string &delim) {
        stringstream tran(in.c_str());
        string tmp;
        vector<string> out;
        out.clear();
        while (std::getline(tran, tmp, *(delim.c_str()))) {
            out.push_back(tmp);
        }
        return out;
    }

    void PrintHex(uint8_t *data, uint32_t len) {
        int count = 0;
        for (int i = 0; i < len; i++) {
            printf("%02x ", data[i]);
            count++;
            if (count == 16) {
                printf("\n");
                count = 0;
            }
        }
        printf("\n");
    }


    int GbkToUtf8TRANSLATE(char *in, unsigned int *lenOfIn, char *out, unsigned int *lenOfOut) {
        //
        /* 目的编码, TRANSLIT：遇到无法转换的字符就找相近字符替换
         *           IGNORE ：遇到无法转换字符跳过*/
        int bRes = -1;
        const char *const encTo = "UTF-8//TRANSLIT"; // TRANSLIT //IGNORE
        const char *const encFrom = "GBK"; //UNICODE GBK
        iconv_t cd = iconv_open(encTo, encFrom);
        if (cd == (iconv_t) -1) {
            perror("iconv_open");
            iconv_close(cd);
            return -1;
        }
        /* 由于iconv()函数会修改指针，所以要保存源指针 */

        /* 需要转换的字符串 */
        char bufIn[1024];
        size_t lenOfBufIn;
        char *pBufIn = bufIn;
        memset(bufIn, 0, sizeof(bufIn));
        snprintf(bufIn, sizeof(bufIn) - 1, "%s", in);
        lenOfBufIn = *lenOfIn;

        /* 存放转换后的字符串 */
        char bufOut[1024];
        size_t lenOfBufOut = sizeof(bufOut);
        char *pBufOut = bufOut;
        memset(bufOut, 0, sizeof(bufOut));

        /* 进行转换
         * *@param cd iconv_open()产生的句柄
         * *@param bufIn 需要转换的字符串
         * *@param lenOfBufIn存放还有多少字符没有转换
         * *@param bufOut 存放转换后的字符串
         *	*@param outlen 存放转换后,tempoutbuf剩余的空间
         *  */
        size_t ret = iconv(cd, &pBufIn, &lenOfBufIn, &pBufOut, &lenOfBufOut);
        if (ret == -1) {
            perror("iconv");
            iconv_close(cd);
            return -1;
        }
        memcpy(out, bufOut, strlen(bufOut));
        *lenOfOut = strlen(bufOut);
        /* 关闭句柄 */
        //
        iconv_close(cd);
        return bRes;

    }

    int Utf8ToGbkTRANSLATE(char *in, unsigned int *lenOfIn, char *out, unsigned int *lenOfOut) {
        /*
         *  目的编码, TRANSLIT：遇到无法转换的字符就找相近字符替换
         *      	IGNORE  ：遇到无法转换字符跳过
         */
        char *encTo = "GBK//TRANSLIT";
        /* 源编码 */
        char *encFrom = "UTF-8";

        /*
         * 获得转换句柄
         * *@param encTo 目标编码方式
         * *@param encFrom 源编码方式
         */
        iconv_t cd = iconv_open(encTo, encFrom);
        if (cd == (iconv_t) -1) {
            perror("iconv_open");
            iconv_close(cd);
            return -1;
        }

        /* 需要转换的字符串 */
        char bufIn[1024];
        size_t lenOfBufIn;
        char *pBufIn = bufIn;
        memset(bufIn, 0, sizeof(bufIn));
        snprintf(bufIn, sizeof(bufIn) - 1, "%s", in);
        lenOfBufIn = *lenOfIn;

        /* 存放转换后的字符串 */
        char bufOut[1024];
        size_t lenOfBufOut = sizeof(bufOut);
        char *pBufOut = bufOut;
        memset(bufOut, 0, sizeof(bufOut));
        /* 由于iconv()函数会修改指针，所以要保存源指针 */

        /* 进行转换
         * *@param cd iconv_open()产生的句柄
         * *@param bufIn 需要转换的字符串
         * *@param lenOfBufIn存放还有多少字符没有转换
         * *@param bufOut 存放转换后的字符串
         *	*@param outlen 存放转换后,tempoutbuf剩余的空间
         *  */
        size_t ret = iconv(cd, &pBufIn, &lenOfBufIn, &pBufOut, &lenOfBufOut);
        if (ret == -1) {
            perror("iconv");
            iconv_close(cd);
            return -1;
        }

        memcpy(out, bufOut, strlen(bufOut));
        *lenOfOut = strlen(bufOut);
        /* 关闭句柄 */
        iconv_close(cd);
        return 0;
    }

    void Trim(string &str, char trim) {
        std::string::iterator end_pos = std::remove(str.begin(), str.end(), trim);
        str.erase(end_pos, str.end());
    }

    bool startsWith(const std::string str, const std::string prefix) {
        return (str.rfind(prefix, 0) == 0);
    }

    bool endsWith(const std::string str, const std::string suffix) {
        if (suffix.length() > str.length()) {
            return false;
        }

        return (str.rfind(suffix) == (str.length() - suffix.length()));
    }

    const char encodeCharacterTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const signed char decodeCharacterTable[256] = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
            -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    void
    base64_encode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length) {
        char buff1[3];
        char buff2[4];
        unsigned char i = 0, j;
        unsigned int input_cnt = 0;
        unsigned int output_cnt = 0;

        while (input_cnt < input_length) {
            buff1[i++] = input[input_cnt++];
            if (i == 3) {
                output[output_cnt++] = encodeCharacterTable[(buff1[0] & 0xfc) >> 2];
                output[output_cnt++] = encodeCharacterTable[((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4)];
                output[output_cnt++] = encodeCharacterTable[((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6)];
                output[output_cnt++] = encodeCharacterTable[buff1[2] & 0x3f];
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 3; j++) {
                buff1[j] = '\0';
            }
            buff2[0] = (buff1[0] & 0xfc) >> 2;
            buff2[1] = ((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4);
            buff2[2] = ((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6);
            buff2[3] = buff1[2] & 0x3f;
            for (j = 0; j < (i + 1); j++) {
                output[output_cnt++] = encodeCharacterTable[buff2[j]];
            }
            while (i++ < 3) {
                output[output_cnt++] = '=';
            }
        }
        output[output_cnt] = 0;
        *output_length = output_cnt;
    }

    void
    base64_decode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length) {
        char buff1[4];
        char buff2[4];
        unsigned char i = 0, j;
        unsigned int input_cnt = 0;
        unsigned int output_cnt = 0;

        while (input_cnt < input_length) {
            buff2[i] = input[input_cnt++];
            if (buff2[i] == '=') {
                break;
            }
            if (++i == 4) {
                for (i = 0; i != 4; i++) {
                    buff2[i] = decodeCharacterTable[buff2[i]];
                }
                output[output_cnt++] = (char) ((buff2[0] << 2) + ((buff2[1] & 0x30) >> 4));
                output[output_cnt++] = (char) (((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2));
                output[output_cnt++] = (char) (((buff2[2] & 0x3) << 6) + buff2[3]);
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 4; j++) {
                buff2[j] = '\0';
            }
            for (j = 0; j < 4; j++) {
                buff2[j] = decodeCharacterTable[buff2[j]];
            }
            buff1[0] = (buff2[0] << 2) + ((buff2[1] & 0x30) >> 4);
            buff1[1] = ((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2);
            buff1[2] = ((buff2[2] & 0x3) << 6) + buff2[3];
            for (j = 0; j < (i - 1); j++) {
                output[output_cnt++] = (char) buff1[j];
            }
        }
//        output[output_cnt] = 0;
        *output_length = output_cnt;
    }


    string getIpStr(unsigned int ip) {
        union IPNode {
            unsigned int addr;
            struct {
                unsigned char s1;
                unsigned char s2;
                unsigned char s3;
                unsigned char s4;
            };
        };
        union IPNode x;

        x.addr = ip;
        char buffer[64];
        bzero(buffer, 64);
        sprintf(buffer, "%d.%d.%d.%d", x.s1, x.s2, x.s3, x.s4);

        return string(buffer);
    }

    bool isIPv4(string IP) {
        int dotcnt = 0;
        //数一共有几个.
        for (int i = 0; i < IP.length(); i++) {
            if (IP[i] == '.')
                dotcnt++;
        }
        //ipv4地址一定有3个点
        if (dotcnt != 3)
            return false;
        string temp = "";
        for (int i = 0; i < IP.length(); i++) {
            if (IP[i] != '.')
                temp += IP[i];
            //被.分割的每部分一定是数字0-255的数字
            if (IP[i] == '.' || i == IP.length() - 1) {
                if (temp.length() == 0 || temp.length() > 3)
                    return false;
                for (int j = 0; j < temp.length(); j++) {
                    if (!isdigit(temp[j]))
                        return false;
                }
                int tempInt = stoi(temp);
                if (tempInt > 255 || tempInt < 0)
                    return false;
                string convertString = to_string(tempInt);
                if (convertString != temp)
                    return false;
                temp = "";
            }
        }
        if (IP[IP.length() - 1] == '.')
            return false;
        return true;
    }

    bool isIPv6(string IP) {
        int dotcnt = 0;
        for (int i = 0; i < IP.length(); i++) {
            if (IP[i] == ':')
                dotcnt++;
        }
        if (dotcnt != 7) return false;
        string temp = "";
        for (int i = 0; i < IP.length(); i++) {
            if (IP[i] != ':')
                temp += IP[i];
            if (IP[i] == ':' || i == IP.length() - 1) {
                if (temp.length() == 0 || temp.length() > 4)
                    return false;
                for (int j = 0; j < temp.length(); j++) {
                    if (!(isdigit(temp[j]) || (temp[j] >= 'a' && temp[j] <= 'f') || (temp[j] >= 'A' && temp[j] <= 'F')))
                        return false;
                }
                temp = "";
            }
        }
        if (IP[IP.length() - 1] == ':')
            return false;
        return true;
    }


    string validIPAddress(string IP) {
        //以.和：来区分ipv4和ipv6
        for (int i = 0; i < IP.length(); i++) {
            if (IP[i] == '.')
                return isIPv4(IP) ? "IPv4" : "Neither";
            else if (IP[i] == ':')
                return isIPv6(IP) ? "IPv6" : "Neither";
        }
        return "Neither";
    }

    void GetDirFiles(string path, vector<string> &array) {
        DIR *dir;
        struct dirent *ptr;
        char base[1000];
        if ((dir = opendir(path.c_str())) == nullptr) {
            printf("can not open dir %s\n", path.c_str());
            return;
        } else {
            while ((ptr = readdir(dir)) != nullptr) {
                if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) {
//            printf("it is dir\n");
                } else if (ptr->d_type == DT_REG) {
                    string name = ptr->d_name;
                    array.push_back(name);
                }
            }
            closedir(dir);
        }
    }


}