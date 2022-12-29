//
// Created by lining on 9/30/22.
//

#ifndef _OS_H
#define _OS_H

#include <string>
#include <vector>
#include "iconv.h"

namespace os {
    using namespace std;

    int runCmd(const string cmd, string &out);

    int GetVectorFromFile(vector<uint8_t> &array, string filePath);

    int GetFileFromVector(vector<uint8_t> &array, string filePath);

    vector<string> split(const string &in, const string &delim);

    /**
     * 打印hex输出
     * @param data
     * @param len
     */
    void PrintHex(uint8_t *data, uint32_t len);

    int GbkToUtf8TRANSLATE(char *in, unsigned int *lenOfIn, char *out, unsigned int *lenOfOut);

    int Utf8ToGbkTRANSLATE(char *in, unsigned int *lenOfIn, char *out, unsigned int *lenOfOut);

    void Trim(string &str, char trim);

    bool startsWith(const std::string str, const std::string prefix);

    bool endsWith(const std::string str, const std::string suffix);

    void
    base64_encode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length);

    void
    base64_decode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length);

    string getIpStr(unsigned int ip);

    bool isIPv4(string IP);

    bool isIPv6(string IP);

    string validIPAddress(string IP);

    void GetDirFiles(string path, vector<string> &array);
};


#endif //_OS_H
