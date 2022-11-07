#ifndef _MYSYSTEMCMD_HPP_
#define _MYSYSTEMCMD_HPP_

/*
* 代替system()函数的接口；   
* 返回值：
* 0 ：成功；
* 非零：失败；
*/
int MySystemCmd (const char *format, ...);

#endif
