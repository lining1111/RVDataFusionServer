/*
 * logger.c
 *
 *  Created on: 2019年10月9日
 *      Author: Pengchao
 */
#include "logger.h"

int logger_facility_priority = LOG_DEBUG|LOG_LOCAL1;
int logger_level = DBG_LEVEL;

/*
 * 设置日志的存储文件，具体对应的文件由syslog的配置文件决定，默认日志级别为DBG_LEVEL，默认的日志文件为LOG_LOCAL1对应的文件
 * 此接口需要在应用最开始调用以设置日志文件
 *
 * 参数：
 *  level：日志的打印级别，DBG_LEVEL、INFO_LEVEL、WARNING_LEVEL、ERR_LEVEL
 *  place：LOG_LOCAL1~LOG_LOCAL7，syslog的配置文件决定了LOG_LOCAL1~LOG_LOCAL7对应的具体文件
 * */
void logger_level_place_set(int level, int place)
{
    logger_level = level;
    logger_facility_priority = LOG_DEBUG|place;
}



