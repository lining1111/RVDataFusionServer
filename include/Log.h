//
// Created by lining on 2/15/22.
// 基于zlog的日志系统
//

#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <zlog.h>

using namespace std;

namespace log {

    extern zlog_category_t *m_zlog_category;

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
    zlog_fatal(m_zlog_category,format,##__VA_ARGS__)

#define Error(format, ...) \
    zlog_error(m_zlog_category,format,##__VA_ARGS__)

#define Warn(format, ...)\
    zlog_warn(m_zlog_category,format,##__VA_ARGS__)

#define Notice(format, ...) \
    zlog_notice(m_zlog_category,format,##__VA_ARGS__)
#define Info(format, ...); \
    zlog_info(m_zlog_category,format,##__VA_ARGS__)
#define Debug(format, ...) \
    zlog_debug(m_zlog_category,format,##__VA_ARGS__)
};


#endif //_LOG_H
