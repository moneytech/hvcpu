/*
Author: nilput (check license file)
License: MIT
*/
#include <stdio.h>
#include <stdlib.h>
#ifdef DEBUG_LOG

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
void _log_msg(char *msg)
{

    struct timespec systime;
    struct tm btime;
#define BUFFSZ 128
    char buff[BUFFSZ];
    clock_gettime(CLOCK_REALTIME, &systime);
    localtime_r(&systime.tv_sec, &btime);
    strftime(buff, BUFFSZ, "#(%F %R:%M:%S): ", &btime);
#undef BUFFSZ

    int fd;
    int count = 0;

    fd = open("log.txt", O_RDWR | O_APPEND | O_CREAT, 0755);
    while (fd < 0 && count++ < 3){
        usleep(1e3 * 100);
        fd = open(LOGFILE, O_RDWR | O_APPEND | O_CREAT, 0755);
    }
    if (fd >= 0){
        write(fd, buff, strlen(buff));
        write(fd, msg, strlen(msg));
        close(fd);
    }
}

#endif //ifdef DEBUG_LOG

void panic_exit(char *msg)
{
    fprintf(stderr, msg);
    exit(1);
}
