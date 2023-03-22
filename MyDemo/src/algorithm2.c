#include "algorithm2.h"
#include "config.h"
#include "logging.h"
#include "algorithm.h"
#include "command.h"



#if ALGO2_EN

void algo2_init(void){
    LOG_RED("init\n");
}

int algo2_run(void){
    LOG_RED("run\n");

}
int algo2_send_control_frame(void){
    LOG_RED("sending\n");

    int frameID = algo_get_frameId();
    /*发送帧序号*/
    printf("%d\n", frameID);


    command_ok();
    command_send();

    return 0;
}

#endif
