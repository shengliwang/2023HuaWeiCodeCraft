#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "algorithm1.h"
#include "public.h"
#include "logging.h"
#include "map.h"
#include "util.h"
#include "config.h"
#include "command.h"

/* 此算法，没有考虑其他因素，只是单纯的买东西，买东西，不考虑其他任何因素
* 不考虑碰撞，不考虑持有时间。不可抢占，
* 把运输抽象成任务，按照优先级分配给机器人，机器人一下只能执行一个任务。
在执行一个任务的时候，不能中断去执行其他任务。当前任务必须执行完，才能重新计算。
*/

#if ALGO1_EN
/*处理判题器发来的一帧数据*/
int algo1_digest_one_frame(unsigned int frameid, unsigned int money){
    char line[1024];

    int wt_num = 0;
    if (scanf("%d", &wt_num) < 0){
        LOG_RED("error in scanf\n");
    }

    if (wt_num != map_get_wt_num()){
        LOG_RED("wt_num != map_get_wt_num()");
    }

  //  LOG("read working table from frame %d:\n", frameid);
    /*读取每一帧工作台的信息*/
    for (int i = 0; i < wt_num; ++i){
        int wtType;
        double posX, posY;
        int remainPdtFrame;     /*剩余生产帧数*/
        unsigned int rawMaterialState;
        unsigned int productState;

        scanf("%d %lf %lf %d %d %d", &wtType, &posX, &posY, &remainPdtFrame,
                                    &rawMaterialState, &productState);
        map_set_wt_state(i, wtType, posX, posY, 
                            remainPdtFrame, rawMaterialState, productState);
#if 0
        LOG_INFO("[%d]%d %lf %lf %d 0x%x %d\n", i, wtType, posX, posY, remainPdtFrame, 
                                    &rawMaterialState, productState);
#endif
    }
  //  LOG("read robot info from frame %d:\n", frameid);

    /*读取每一帧的机器人信息*/
    for (int i = 0; i < map_get_rbt_num(); ++i){
        int inWhichWt;  /*所处工作台*/
        int carryItmType;/*携带物品类型*/
        double timeValueFactor; /*时间价值系数*/
        double collisionValueFactor;    /*碰撞价值系数*/
        double angleSpeed;          /*角速度*/
        double lineSpeedX, lineSpeedY;  /*线速度*/
        double direction;   /*朝向*/
        double posX, posY;  /*坐标*/
        
        scanf("%d %d %lf %lf %lf %lf %lf %lf %lf %lf",
                        &inWhichWt, &carryItmType,
                        &timeValueFactor, &collisionValueFactor, 
                        &angleSpeed, &lineSpeedX, &lineSpeedY,&direction,
                        &posX, &posY);
        map_set_rbt_state(i, inWhichWt, carryItmType,
                        timeValueFactor, collisionValueFactor,
                        angleSpeed, lineSpeedX, lineSpeedY,direction,
                        posX, posY);
#if 0
        LOG_INFO("%d %d %lf %lf %lf %lf %lf %lf %lf %lf\n", 
                    inWhichWt, carryItmType, 
                    timeValueFactor, collisionValueFactor,
                    angleSpeed, lineSpeedX, lineSpeedY, direction, 
                    posX, posY);
#endif
    }

    /*剩余数据*/
    while (fgets(line, sizeof line, stdin)) {
        if (line[0] == 'O' && line[1] == 'K') {
            return 0;
        }
        //LOG_INFO("warning: recv[%s]", line);
    }
    
    LOG_RED("ret err!\n");
    return ALGO1_RET_ERR;
}

#define LEVEL_1     (0)
#define LEVEL_2     (1)
#define LEVEL_3     (2)
#define LEVEL_4     (3)

static bool algo1_wt_is_level(const struct working_table * p, int level){
    switch(level){
        case LEVEL_1:{
            if (1 == p->type || 2 == p->type || 3 == p->type){
                return true;
            } else {
                return false;
            }
        }
        case LEVEL_2:{
            if (4 == p->type || 5 == p->type || 6 == p->type){
                return true;
            } else {
                return false;
            }
        }
        case LEVEL_3:{
            if (7 == p->type){
                return true;
            } else {
                return false;
            }
        }
        case LEVEL_4:{
            if (8 == p->type){
                return true;
            } else {
                return false;
            }
        }
        default:{
            return false;
        }
    }
}

int algo1_get_level_wt_node(struct working_table *p, int level){
    int num = map_get_wt_num();
    const struct working_table * wt = map_get_wt();

    int copy_num = 0;

    for (int i = 0; i < num; ++i){
        if (algo1_wt_is_level(wt, level)){
            *p = *wt;
            ++p;
            ++copy_num;
        }
        ++wt;
    }

    return copy_num;
}

/*算法1的算法执行*/
/*赛区初赛规则：
1: None
2: None 
3: None
4: 1+2;
5: 1+3;
6: 2+3;
7: 4+5+6;
8: 7
9: 1,2,3,4,5,6,7
*/
struct task_edge{
    int start_wt_id;
    double start_x;
    double start_y;

    int dest_wt_id;
    double dest_x;
    double dest_y;

    int priority;
    int bind_robot_id; /*绑定特定的机器人去完成任务*/
    double robot_init_x;    /*机器人初始坐标*/
    double robot_init_y;
    double total_distance;
    double done_distance;

    double complete_rate;   /*任务完成度 0~1*/
};

static struct task_edge g_task_edge[MAX_ROBOT_NUM] = {0};

void algo1_init(void){
    memset(g_task_edge, 0, sizeof(g_task_edge));

    for (int i = 0; i < MAX_ROBOT_NUM; ++i){
        g_task_edge[i].complete_rate = 1;
    }
}

/*在所有已完成的任务中选取优先级最低的任务,
返回NULL,代表在当前帧的情况下没有可用的任务*/
static struct task_edge * algo1_get_min_priority_done_task(void){
    int rbt_num = map_get_rbt_num();

    int minIdx = -1;
    int minValue = 0;

    int i;

    /*寻找第一个已经完成或者在当前帧刚加进来的任务*/
    for (i = 0; i < rbt_num; ++i){
        if (1 == g_task_edge[i].complete_rate ||
            0 == g_task_edge[i].complete_rate){

            minIdx = i;
            minValue = g_task_edge[i].priority;
            break;
        }
    }

    /*所有任务均未完成，由于任务不可抢占，返回NULL*/
    if (-1 == minIdx){
        return NULL;
    }
    
    for (i = minIdx + 1; i < rbt_num; ++i){
        if (1 == g_task_edge[i].complete_rate
                && minValue > g_task_edge[i].priority){
            minIdx = i;
            minValue = g_task_edge[i].priority;
        }
    }

    return &g_task_edge[minIdx];
}

int algo1_get_available_rbt(struct task_edge * task){
    /*算法代码没写错的情况下，肯定能找到一个空闲机器人来干活的*/

    /*找到第一个空闲机器人*/
    /*todo: 后续再研究多个机器人的情况下，根据距离分配机器人干活*/
    bool availableFlag;
    for (int i = 0; i < map_get_rbt_num(); ++i){
        availableFlag = 1;
        for (int taskId = 0; taskId < MAX_ROBOT_NUM; ++taskId){
            if (g_task_edge[taskId].priority > 0
                && g_task_edge[taskId].bind_robot_id == i){
                availableFlag = 0;
                break;
            }
        }
        if(availableFlag){
            return i;
        }
    }

    LOG_RED("not find available robot!\n");
    return 0xffffff; /*会导致段错误，便于调试*/
//
}


int algo1_run(int frameId){
    static struct working_table levelWtNode[4][MAX_WORKING_TABLE_NUM];
    int levelWtNodeNum[4];

    for (int level = LEVEL_1; level <= LEVEL_4; ++level){
        levelWtNodeNum[level] = 
                algo1_get_level_wt_node(levelWtNode[level], level);
      //  LOG_INFO("fram%d, level %d, node num %d\n",
        //            frameId, level, levelWtNodeNum[level]);
    }

    int taskAvailableFlag = 1;
    int priority = 1;
    /*循环三次 计算*/
    for (int level = LEVEL_1; level < LEVEL_4; ++ level){
        struct working_table * p1 = levelWtNode[level];
        struct working_table * p2 = levelWtNode[level+1];
        int p1_num = levelWtNodeNum[level];
        int p2_num = levelWtNodeNum[level+1];
        
        /*从p1节点运输货物到p2的节点上*/
        for (int i = 0; i < p1_num; ++i){
            for (int j = 0; j < p2_num; ++j){
                if (map_check_vality_between_node(p1+i, p2+j)){
                    struct task_edge * targetTask = algo1_get_min_priority_done_task();
                    if (NULL == targetTask){
                        LOG_RED("frame%d, NULL ==  targetTask\n", frameId);
                        taskAvailableFlag = 0;
                        break;
                    }
                    
                    targetTask->start_wt_id = (p1+i)->id;
                    targetTask->start_x = (p1+i)->pos_x;
                    targetTask->start_y = (p1+i)->pos_y;

                    targetTask->dest_wt_id = (p2+j)->id;
                    targetTask->dest_x = (p2+j)->pos_x;
                    targetTask->dest_y = (p2+j)->pos_y;
                    targetTask->complete_rate = 0;
                    targetTask->priority = priority;
                    targetTask->bind_robot_id = -1;
                    break;  /*当前的节点已经确定目的地了，后面不在分配
                            todo： 需要考虑后续更高的优先级。。。*/
                }
            }
            if(0 == taskAvailableFlag){
                break;
            }
        }

        if (0 == taskAvailableFlag){
            break;
        }
        ++priority;
    }

    /*给未分配机器人的任务分配机器人*/
    for (int i = 0; i < MAX_ROBOT_NUM; ++i){
        struct task_edge *p = &g_task_edge[i];
        
        if (p->priority > 0 && 
                p->bind_robot_id == -1){

            p->bind_robot_id = algo1_get_available_rbt(p);
            p->robot_init_x = map_get_rbt(i)->pos_x;
            p->robot_init_y = map_get_rbt(i)->pos_y;

            double x1 = p->robot_init_x;
            double y1 = p->robot_init_y;

            double x2 = p->start_x;
            double y2 = p->start_y;
            
            double x3 = p->dest_x;
            double y3 = p->dest_y;
            p->total_distance =
                    util_distance(x1, y1, x2, y2) +
                    util_distance(x2, y2, x3, y3);

            if (0 == p->total_distance){
                LOG_RED("error: p->total_distance = %f\n", p->total_distance);
            }
            p->done_distance = 0;
       }
    }

    /*对于优先级不等于0的任务，计算完成度所需要的信息*/
    for(int i = 0; i < MAX_ROBOT_NUM; ++i){
        struct task_edge *p = &g_task_edge[i];
        const struct robot * rbt = map_get_rbt(p->bind_robot_id);

        if (p->priority > 0){
            if (map_rbt_has_product(p->bind_robot_id)){
                p->done_distance = 
                    util_distance(p->robot_init_x, p->robot_init_y,
                                p->start_x, p->start_y) + 
                    util_distance(p->start_x, p->start_y,
                                rbt->pos_x, rbt->pos_y);
            } else {
                p->done_distance = 
                    util_distance(rbt->pos_x, rbt->pos_y,
                                p->robot_init_x, p->robot_init_y);
            }

            if (0 == p->total_distance){
                LOG_RED("error: p->total_distance = %f\n", p->total_distance);
                p->total_distance = 100;
            }
            p->complete_rate = p->done_distance / p->total_distance;
        }
    }
}


static void algo1_go_point(int rbtId, double x, double y){
    const struct robot * rbt = map_get_rbt(rbtId);

    double rbt_x = rbt->pos_x;
    double rbt_y = rbt->pos_y;

    double rbt_direction = rbt->direction;

    /*生成两个向量A, B, A为表示robot朝向的向量，
        B为 robot 指向终点的向量 */
    double Ax, Ay, Bx, By;

    util_gen_vector_from_direction(&Ax, &Ay, rbt_direction);
    util_gen_vector_from_point(&Bx, &By,
                    rbt_x, rbt_y,
                    x, y);

    /*计算两个向量之间的夹角*/
    double angle = util_vector_angle(Ax, Ay, Bx, By);

    double flag = util_c_dirction(Ax, Ay, Bx, By);

    //LOG_GREEN("rbt%d: A:(%f,%f), B:(%f,%f), angle:%f, c_direction:%f\n",
    //rbtId, Ax, Ay, Bx, By, angle, flag);

    double angleSpeed = angle/0.015;

    if (flag >=0 ){
        command_rbt_rotate_anticlockwise(rbtId, angleSpeed);
    } else {
        command_rbt_rotate_clockwise(rbtId, angleSpeed);
    }

    double linespeed = 0.1;

    /*距离越近速度越慢,每一帧的间隔是15ms即0.015s*/
    double dist = util_distance(x, y, rbt_x, rbt_y);

    linespeed = (dist > 2.0)? 
        MAX_ROBOT_FORWARD_SPEED : MAX_ROBOT_FORWARD_SPEED/2;
   // LOG_GREEN("rbt%d, distance to dest %f, set speed to %.3f\n",rbtId, dist, linespeed);
    
    command_rbt_forward(rbtId, linespeed);
}

/*控制机器人运动*/
int algo1_send_control_frame(int frameID){

    /*发送帧序号*/
    printf("%d\n", frameID);

    /*处理任务边*/
    for (int i = 0; i < MAX_ROBOT_NUM; ++i){
        struct task_edge * task = &g_task_edge[i];
        
        if (task->priority <= 0){
            LOG_RED("continue\n");
            continue;
        }

        int rbtId = task->bind_robot_id;
        int inWorkTable = map_get_rbt_in_which_wt(rbtId);


        if (map_rbt_has_product(rbtId)){
            if (inWorkTable != task->dest_wt_id){
                algo1_go_point(rbtId, task->dest_x, task->dest_y);
            } else {
                command_rbt_sell(rbtId);
                task->priority = 0;
               // task->bind_robot_id = -1;
            }
        }else{
            if (inWorkTable != task->start_wt_id){
                algo1_go_point(rbtId, task->start_x, task->start_y);
            } else {
                command_rbt_buy(rbtId);
            }
        }
    }

    command_ok();
    command_send();
}


#endif
