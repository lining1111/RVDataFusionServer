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

namespace log {

    extern zlog_category_t *m_zlog_category;
    extern bool isInit;//是否已初始化

    /**
     * 初始化
     * @return
     */
    int init();

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
        struct timeval tv; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%d:%d [Fatal]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)

#define Error(format, ...) \
    do{                    \
        if(isInit){\
        zlog_error(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%d:%d [Error]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)

#define Warn(format, ...)\
    do{                    \
        if(isInit){\
        zlog_warn(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%d:%d [Warn]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)

#define Notice(format, ...) \
    do{                    \
        if(isInit){\
        zlog_notice(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%d:%d [Notice]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)
#define Info(format, ...) \
    do{                    \
        if(isInit){\
        zlog_info(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%d:%d [Info]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)
#define Debug(format, ...) \
    do{                    \
        if(isInit){\
        zlog_debug(m_zlog_category,format,##__VA_ARGS__); \
        }else{             \
        struct timeval tv; \
        gettimeofday(&tv,nullptr);\
        char time_str[32]; \
        strftime(time_str,sizeof(time_str),"%Y-%m-%d %H:%M:%S",localtime(&tv.tv_sec)); \
        printf("%s:%d:%d [Debug]:",time_str,tv.tv_usec/1000,(tv.tv_usec-(tv.tv_usec/1000)*1000));  \
        printf(format,##__VA_ARGS__);                     \
        printf("\n");                   \
        }                       \
    }while(0)
};


#endif //_LOG_H
