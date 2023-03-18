#include <stdio.h>
#include <stdbool.h>

#include "logging.h"
#include "map.h"
#include "command.h"
#include "config.h"
#include "algorithm.h"
#include "algorithm1.h"
#include "util.h"

bool readUntilOK() {
    char line[1024];
    while (fgets(line, sizeof line, stdin)) {
        if (line[0] == 'O' && line[1] == 'K') {
            return true;
        }
        //do something
    }
    return false;
}

int main() {
    int ret;
    if (0 != (ret = log_init("./myDemoLog.txt"))){
        return ret;
    }
    LOG("log init OK!\n");
    if (0!=map_init()){
        LOG_RED("map_init failed");
    }

    algo1_init1();
    map_print_des();
    command_ok();
    command_send();

    

    unsigned int frameID;
    unsigned int money;
    while (scanf("%d %d", &frameID, &money) != EOF) {
        util_time_start();
        algo1_digest_one_frame(frameID, money);
        algo1_run1(frameID);
        algo1_send_control_frame1(frameID);
        util_time_stop();
        //LOG_INFO("algo1_run using %lf ms\n", util_time_duration());

    }
    return 0;
}
