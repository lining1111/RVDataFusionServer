#ifndef _LOGGER_H_
#define _LOGGER_H_
#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef PARALLEL_GUN_LOG
#define LOG_LEVEL   LOG_LOCAL7
#elif PARALLEL_YC
#define LOG_LEVEL   LOG_LOCAL3
#elif ZW
#define LOG_LEVEL   LOG_LOCAL5
#else
#define LOG_LEVEL   LOG_LOCAL7
#endif

/*调试模式*/
#define DEBUG
#define filename(x) (strrchr(x,'/')?strrchr(x,'/')+1:x)
#define TIME_STR_LEN	50
#define GET_CURRENT_TIME(p) {time_t now; struct tm *timenow; time(&now); timenow=localtime(&now); sprintf(p, "%u-%02u-%02u %02u:%02u:%02u", 1900+timenow->tm_year, 1+timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);}

#define PTV(val)

#ifdef DEBUG
#define DBG(fmt, args...) {char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u](DBG):" fmt "\n",_p,filename(__FILE__), __LINE__,   ##args);}
#define INFO(fmt, args...) {char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u](INFO):" fmt "\n",_p,filename(__FILE__), __LINE__,  ##args);}
#define WARNING(fmt, args...) {char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u](WARNING):" fmt "\n",_p,filename(__FILE__), __LINE__,   ##args);}
#define ERR(fmt, args...) {char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u](ERR):" fmt " -%m\n",_p,filename(__FILE__), __LINE__,  ##args);}
#define DBG_F(facility,fmt, args...) {char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u](DBG):" fmt "\n",_p,filename(__FILE__), __LINE__,   ##args);}
#define ERR_F(facility,fmt, args...) {char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u](ERR):" fmt " -%m\n",_p,filename(__FILE__), __LINE__,  ##args);}

#else

#define DBG_F(facility,fmt,args...) syslog(LOG_DEBUG|facility,"[%s-%u](DBG):" fmt "\n", filename(__FILE__), __LINE__, ##args)
#define ERR_F(facility,fmt,args...) syslog(LOG_DEBUG|facility,"[%s-%u](ERR):" fmt "\n", filename(__FILE__), __LINE__, ##args)
#define DBG(fmt, args...) syslog(LOG_DEBUG|LOG_LEVEL,"[%s-%u](DBG):" fmt "\n", filename(__FILE__), __LINE__, ##args)
#define INFO(fmt, args...) syslog(LOG_DEBUG|LOG_LEVEL,"[%s-%u](INFO):" fmt "\n", filename(__FILE__), __LINE__, ##args)
#define WARNING(fmt, args...) syslog(LOG_DEBUG|LOG_LEVEL,"[%s-%u](WARNING):" fmt "\n", filename(__FILE__), __LINE__, ##args)
#define ERR(fmt, args...) syslog(LOG_DEBUG|LOG_LEVEL,"[%s-%u](ERR):" fmt "-%m\n", filename(__FILE__), __LINE__, ##args)


#endif

#ifdef __cplusplus
}
#endif

#endif
