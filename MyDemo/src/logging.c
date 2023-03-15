#include <stdio.h>

#include "logging.h"
#include "public.h"

static FILE * g_logFileDes = NULL;


int log_init(char * logfile){
#if LOG_ON
    g_logFileDes = fopen(logfile, "w");
    if (NULL == g_logFileDes){
        return LOG_RET_ERR;
    }
#endif
    return 0;
}
void log_deinit(void){
    if (NULL != g_logFileDes){
        fclose(g_logFileDes);
    }
}
FILE * get_logfile_des(void){
    return g_logFileDes;
  // return stderr;
}