#include <stdio.h>


#include "command.h"
#include "map.h"
#include "public.h"
#include "logging.h"

/* speed 范围：[-2,6], 单位m/s,正数表示前进，负数表示后退*/
int command_rbt_forward(int rbt_id, double speed){
    if (rbt_id >= map_get_rbt_num()){
        LOG_RED("error:%d >= %d\n", rbt_id, map_get_rbt_num());
        return COMMAND_RET_ERR;
    }
    printf("forward %d %f\n", rbt_id, speed);
    return 0;
}
int command_rbt_back(int rbt_id, double speed){
    if (rbt_id >= map_get_rbt_num()){
        LOG_RED("error:%d >= %d\n", rbt_id, map_get_rbt_num());
        return COMMAND_RET_ERR;
    }
    printf("forward %d %f\n", rbt_id, -speed);
    return 0;
}


/* speed范围：[-π,π]之间，设置旋转速度，单位为弧度/秒。
    负数表示顺时针旋转，正数表示逆时针旋转。*/
int command_rbt_rotate_clockwise(int rbt_id, double speed){
    if (rbt_id >= map_get_rbt_num()){
        LOG_RED("error:%d >= %d\n", rbt_id, map_get_rbt_num());
        return COMMAND_RET_ERR;
    }
    printf("rotate %d %f\n", rbt_id, -speed);
    return 0;
}

int command_rbt_rotate_anticlockwise(int rbt_id, double speed){
    if (rbt_id >= map_get_rbt_num()){
        LOG_RED("error:%d >= %d\n", rbt_id, map_get_rbt_num());
        return COMMAND_RET_ERR;
    }
    printf("rotate %d %f\n", rbt_id, speed);
    return 0;
}

int command_rbt_buy(int rbt_id){
    if (rbt_id >= map_get_rbt_num()){
        LOG_RED("error:%d >= %d\n", rbt_id, map_get_rbt_num());
        return COMMAND_RET_ERR;
    }

    printf("buy %d\n", rbt_id);
    return 0;
}

int command_rbt_sell(int rbt_id){
    if (rbt_id >= map_get_rbt_num()){
        LOG_RED("error:%d >= %d\n", rbt_id, map_get_rbt_num());
        return COMMAND_RET_ERR;
    }

    printf("sell %d\n", rbt_id);
    return 0;
}


int command_rbt_destroy(int rbt_id){
    if (rbt_id >= map_get_rbt_num()){
        LOG_RED("error:%d >= %d\n", rbt_id, map_get_rbt_num());
        return COMMAND_RET_ERR;
    }

    printf("destroy %d\n", rbt_id);
    return 0;
}
