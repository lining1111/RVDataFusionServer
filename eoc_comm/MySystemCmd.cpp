#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "MySystemCmd.hpp"
#include "logger.h"

#define _CMD_LEN    (256)
 
static int _system (char *cmd);

int MySystemCmd (const char *format, ...)
{
    char cmdBuff[_CMD_LEN];
    va_list vaList;
    
    va_start (vaList, format);
    vsnprintf ((char *)cmdBuff, sizeof(cmdBuff), format, vaList);
    va_end (vaList);
    
    return _system ((char *)cmdBuff);
}


static void _close_all_fds (void)
{
    int i;
    for (i = 0; i < sysconf(_SC_OPEN_MAX); i++)
    {
      if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO)
        close(i);
    }
}
/*
 *脚本执行成功放回0，失败返回-1
 *
 */
static int _system (char *command)
{
    pid_t pid, ppid;
    int status = -2;
    char *argv[4];
    extern char **environ;

    if (NULL == command) {
    	ERR("%s NULL == command", __FUNCTION__);
        return -1;
    }

    pid = vfork();        /* vfork() also works */
    if (pid < 0) {
    	ERR("%s pid < 0", __FUNCTION__);
        return -1;
    }
    if (0 == pid) {             /* child process */
        //_close_all_fds();       
        argv[0] = (char*)"sh";
        argv[1] = (char*)"-c";
        argv[2] = command;
        argv[3] = NULL;

        execve ("/bin/sh", argv, environ);    /* execve() also an implementation of exec()  该函数执行成功会默认不会返回，子进程退出状态为0.失败出错时返回*/
        perror("execve");
        _exit(-1);
    }

    // else
    /* wait for child process to start */
    do
    {
        ppid = waitpid(pid, &status, WNOHANG);
        if(0 == ppid){
                //DBG("_system Nochild exited.");
                //sleep(1);
				usleep(20*1000);
        }
    } while (0 == ppid);

    if(ppid == pid){
        //printf("get child [%d] ok.\n",ppid);
    }else{
        //ERR("get child failed.\n");
    	ERR("%s get child failed", __FUNCTION__);
    }

    if (WIFEXITED(status)) {
        status = WEXITSTATUS(status);
    }

    return status;
}


