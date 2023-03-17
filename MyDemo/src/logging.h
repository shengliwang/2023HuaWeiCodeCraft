#ifndef __MY_LOGGING_H__
#define __MY_LOGGING_H__

#include <stdio.h>
#include "config.h"


#if LOG_ON
int log_init(char * logfile);
void log_deinit(void);
FILE * get_logfile_des(void);

/* yello print */
#define LOG_YELLOW(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "\033[33;1m[%s:%d]" fmt "\033[00m", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)

/* red print */
#define LOG_RED(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "\033[31;1m[%s:%d]" fmt "\033[00m", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)


/* green print */
#define LOG_GREEN(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "\033[32;1m[%s:%d]" fmt "\033[00m", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)

/* blue print */
#define LOG_BLUE(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "\033[34;1m[%s:%d]" fmt "\033[00m", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)
/* normal print */
#define LOG(fmt, ...) \
    do {    \
            fprintf(get_logfile_des(),\
                "[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            fflush(get_logfile_des()); \
    }while(0)
#else

#define log_init(file) (0)
#define log_deinit() (0)
#define get_logfile_des() (0)

#define LOG_YELLOW(fmt, ...)
#define LOG_RED(fmt, ...)
#define LOG_GREEN(fmt, ...)
#define LOG_BLUE(fmt, ...)
#define LOG(fmt, ...)

#endif



#endif
