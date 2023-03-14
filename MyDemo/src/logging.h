#ifndef __MY_LOGGING_H__
#define __MY_LOGGING_H__

#include <stdio.h>

#define LOG_ON 1

int log_init(char * logfile);
void log_deinit(void);
FILE * get_logfile_des(void);


#if LOG_ON

/* yello print */
#define LOG_INFO(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "\033[33;1m[%s:%d][INFO]" fmt "\033[00m", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)

/* red print */
#define LOG_ERR(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "\033[31;1m[%s:%d][ERR]" fmt "\033[00m", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)

/* normal print */
#define LOG(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "[%s:%d][INFO]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)
#else

#define LOG_INFO(fmt, ...)
#define LOG_ERR(fmt, ...)
#define LOG(fmt, ...)

#endif



#endif
