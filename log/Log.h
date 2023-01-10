//
// Created by lining on 2/15/22.
// 基于zlog的日志系统
//

#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <time.h>
#include <sys/time.h>
#include <zlog.h>

using namespace std;

namespace z_log {

    extern zlog_category_t *m_zlog_category;
    extern bool isInit;//是否已初始化

    /**
     * 初始化
     * @return
     */
    int init(string programName, int keepSeconds = -1);

    /**
     * 退出
     * @return
     */
    int finish();


#define Fatal(format, ...) \
    do{                    \
        if(isInit){\
        zlog_fatal(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv = {0,0}; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%lu:%lu [Fatal]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)

#define Error(format, ...) \
    do{                    \
        if(isInit){\
        zlog_error(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv = {0,0}; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%lu:%lu [Error]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)

#define Warn(format, ...)\
    do{                    \
        if(isInit){\
        zlog_warn(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv = {0,0}; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%lu:%lu [Warn]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)

#define Notice(format, ...) \
    do{                    \
        if(isInit){\
        zlog_notice(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv = {0,0}; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%lu:%lu [Notice]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)
#define Info(format, ...) \
    do{                    \
        if(isInit){\
        zlog_info(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv = {0,0}; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%lu:%lu [Info]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)
#define Debug(format, ...) \
    do{                    \
        if(isInit){\
        zlog_debug(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv = {0,0}; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%lu:%lu [Debug]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)
};



//函数版的demo
//void LogInfo(const char *szlog, ...) {
//    char z_log[1024 * 1024] = {0};
//    memset(z_log, 0, sizeof(z_log) / sizeof(z_log[0]));
//    va_list args;
//    va_start(args, szlog);
//    vsprintf(z_log, szlog, args);
//    va_end(args);
//    zlog_info(m_zlog_category, z_log);
//    return;
//}

#endif //_LOG_H
