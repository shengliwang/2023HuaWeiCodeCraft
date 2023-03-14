#ifndef __MY_LOGGING_H__
#define __MY_LOGGING_H__

#include <stdio.h>

#define LOG_ON 1

int log_init(char * logfile);
void log_deinit(void);
FILE * get_logfile_des(void);


#if LOG_ON

#define LOG(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)

#else

#define LOG(fmt, ...)

#endif



#endif
