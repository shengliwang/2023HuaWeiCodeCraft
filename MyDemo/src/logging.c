#include <stdio.h>

#include "logging.h"
#include "public.h"
#include "config.h"

#if LOG_ON

static FILE * g_logFileDes = NULL;


int log_init(char * logfile){
    g_logFileDes = fopen(logfile, "w");
    if (NULL == g_logFileDes){
        return LOG_RET_ERR;
    }
    return 0;
}
void log_deinit(void){
    if (NULL != g_logFileDes){
        fclose(g_logFileDes);
    }
}

FILE * get_logfile_des(void){
    return g_logFileDes;
}

#endif