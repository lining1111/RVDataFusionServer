/*
 * logger.h
 *
 *  Created on: 2019年10月9日
 *      Author: Pengchao
 */

#ifndef STANDARDIZED_LOGGER_H_
#define STANDARDIZED_LOGGER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <syslog.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/*
 * 控制日志由syslog记录还是printf打印
 * */
//#define LOG_USING_PRINTF

/*
 * 日志打印级别，只有大于等于当前设置的级别才会打印（如设置级别为INFO_LEVEL，则DBG_LEVEL的日志将不会打印）
 * */
#define DBG_LEVEL       0       //对应DBG
#define INFO_LEVEL      1       //对应INFO
#define WARNING_LEVEL   2       //对应WARNING
#define ERR_LEVEL       3       //对应ERR
#define CRIT_LEVEL      4       //对应CRIT

#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x
/*
 * 由logger_level_place_set()接口设置的变量，禁止直接修改
 * */
extern int logger_facility_priority;
extern int logger_level;

/*
 * 设置日志的存储文件，具体对应的文件由syslog的配置文件决定，默认日志级别为DBG_LEVEL，默认的日志文件为LOG_LOCAL1对应的文件
 * 此接口需要在应用最开始调用以设置日志文件
 *
 * 参数：
 *  level：日志的打印级别，DBG_LEVEL、INFO_LEVEL、WARNING_LEVEL、ERR_LEVEL
 *  place：LOG_LOCAL1~LOG_LOCAL7，syslog的配置文件决定了LOG_LOCAL1~LOG_LOCAL7对应的具体文件
 * */
void logger_level_place_set(int level, int place);

/*
 * 按级别打印日志文件
 * 
 */
#ifdef LOG_USING_PRINTF

#define TIME_STR_LEN    50
#define GET_CURRENT_TIME(p) {time_t now; struct tm *timenow; time(&now); timenow=localtime(&now); sprintf(p, "%u-%02u-%02u %02u:%02u:%02u", 1900+timenow->tm_year, 1+timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);}

#define DBG(fmt, args...)     do{if(DBG_LEVEL    >=logger_level){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]D:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define INFO(fmt, args...)    do{if(INFO_LEVEL   >=logger_level){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]I:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define WARNING(fmt, args...) do{if(WARNING_LEVEL>=logger_level){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]W:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define ERR(fmt, args...)     do{if(ERR_LEVEL    >=logger_level){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]E:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define CRIT(fmt, args...)    do{if(CRIT_LEVEL   >=logger_level){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]C:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)

//只用于调试
#define PTV(val) cout << #val << ":" << val <<endl

#else

#define DBG(fmt, args...)     do{if(DBG_LEVEL    >=logger_level)syslog(logger_facility_priority, "[%s-%u]D:" fmt "\n", filename(__FILE__), __LINE__, ##args);}while(0)
#define INFO(fmt, args...)    do{if(INFO_LEVEL   >=logger_level)syslog(logger_facility_priority, "[%s-%u]I:" fmt "\n", filename(__FILE__), __LINE__, ##args);}while(0)
#define WARNING(fmt, args...) do{if(WARNING_LEVEL>=logger_level)syslog(logger_facility_priority, "[%s-%u]W:" fmt "\n", filename(__FILE__), __LINE__, ##args);}while(0)
#define ERR(fmt, args...)     do{if(ERR_LEVEL    >=logger_level)syslog(logger_facility_priority, "[%s-%u]E:" fmt "\n", filename(__FILE__), __LINE__, ##args);}while(0)
#define CRIT(fmt, args...)    do{if(CRIT_LEVEL   >=logger_level)syslog(logger_facility_priority, "[%s-%u]C:" fmt "\n", filename(__FILE__), __LINE__, ##args);}while(0)

//只用于调试
#define PTV(val)

#endif

#ifdef __cplusplus
}
#endif

#endif /* STANDARDIZED_LOGGER_H_ */
