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

//#define INFINITY (-log(0)) /*定义正无限大的浮点数*/
#define MY_INFINITY (10000000.0) /*定义正无限大的浮点数*/


unsigned int g_frameId;
unsigned int g_money;

#define RUN_TIME (3*60*50)  /*比赛用时 帧数*/
static unsigned int algo1_get_remain_frame(void){
    return RUN_TIME - g_frameId;
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
    int carry_type; /*携带物品类型*/
    int bind_robot_id; /*绑定特定的机器人去完成任务*/
    double robot_init_x;    /*机器人初始坐标*/
    double robot_init_y;
    double total_distance;
    double done_distance;

    double complete_rate;   /*任务完成度 0~1*/
};

static struct task_edge g_task_edge[MAX_ROBOT_NUM] = {0};

#define ALGO1_RBT_STATE_AVAILABLE   (0)
#define ALGO1_RBT_STATE_BUSY        (1) /*在路上奔波*/
#define ALGO1_RBT_STATE_SELLING     (3) /*正在售卖*/
#define ALGO1_RBT_STATE_BUYING     (4) /*正在购买*/

struct algo1_rbt_motion_command{
    bool valid_flag; /*valid_flag为true的时候才会设置机器人的运动*/
    double c_flag;  /*c_flag >= 0, 代表设置逆时针转动的角速度*/
    double angleSpeed;
    double lineSpeed;
};
struct algo1_robot_state{
    struct task_edge task;
    struct algo1_rbt_motion_command motion_scmd;
    int state;  /*表示机器人当前的任务状态, 0表示可用，1表示正在运输货物*/
    
};

static struct algo1_robot_state g_algo1_rbt_state[MAX_ROBOT_NUM];
static struct algo1_robot_state * algo1_rbt_get_state(int rbtId){
    if (rbtId >= map_get_rbt_num()){
        LOG_RED("(rbtId >= map_get_rbt_num())");
        return NULL;
    }

    return &g_algo1_rbt_state[rbtId];
}

static int algo1_rbt_get_available_num(void){
    int num = 0;
    for (int i = 0; i < map_get_rbt_num(); ++i){
        if (ALGO1_RBT_STATE_AVAILABLE == g_algo1_rbt_state[i].state){
            ++num;
        }
    }

    return num;
}

static int algo1_rbt_get_available(void){
    for (int i = 0; i < map_get_rbt_num(); ++i){
        if (ALGO1_RBT_STATE_AVAILABLE == g_algo1_rbt_state[i].state){
            return i;
        }
    }

    LOG_RED("error ");  /*一定是能找到的*/
    return 0xffffff;
}


static void algo1_rbt_add_task(int rbtId,
       const struct working_table* src_wt, const struct working_table *dest_wt){

    struct algo1_robot_state * state =  algo1_rbt_get_state(rbtId);
    const struct robot  * rbt = map_get_rbt(rbtId);

    LOG_GREEN("frame[%u] add task: rbt: %d, wt%d[type%d]->wt%d[type%d]\n",
        g_frameId, 
        rbtId, src_wt->id, src_wt->type, dest_wt->id, dest_wt->type);
    state->task.bind_robot_id = rbtId;
    state->task.carry_type = src_wt->type;
    state->task.complete_rate = 0;
    state->task.dest_wt_id = dest_wt->id;
    state->task.dest_x = dest_wt->pos_x;
    state->task.dest_y = dest_wt->pos_y;
    state->task.done_distance = 0;
    state->task.priority = 1;
    state->task.robot_init_x = rbt->pos_x;
    state->task.robot_init_y = rbt->pos_y;
    state->task.start_wt_id = src_wt->id;
    state->task.start_x = src_wt->pos_x;
    state->task.start_y = src_wt->pos_y;
    state->task.total_distance = 0;

    state->state = ALGO1_RBT_STATE_BUSY;
}



/*rbtId会在 (centerx, centery)发生碰撞*/
static void algo1_rbt_crash_set_motion_cmd(int rbtId,
                        double center_x, double center_y){
    struct algo1_robot_state * state = algo1_rbt_get_state(rbtId);
//    return ;
    LOG_BLUE("setting rbt %d around crash point (%f, %f)\n",
                rbtId, center_x, center_y);
    
    state->motion_scmd.valid_flag = true;

    state->motion_scmd.lineSpeed = 6;
   // state->motion_scmd.c_flag = -(state->motion_scmd.c_flag);
    state->motion_scmd.c_flag = -1;
    state->motion_scmd.angleSpeed = PI;
    
    return ;
}


/*todo:目前只根据云速直线运动近似出坐标点，后面再适配曲线运动计算坐标点*/
static void algo1_rbt_get_pos_on_frame(int rbtId, int frame,
            double * x, double *y){
    const struct robot * pRbt = map_get_rbt(rbtId);
    struct algo1_robot_state * stat = algo1_rbt_get_state(rbtId);

    if (0 == frame){
        *x = pRbt->pos_x;
        *y = pRbt->pos_y;
        return ;
    }

    double speed_x = pRbt->line_speed_x;
    double speed_y = pRbt->line_speed_y;
    double angle_speed = pRbt->angle_speed;

    double delta_t = frame * FRAME_INTVAL;

    if (stat->motion_scmd.valid_flag){
        double speed_now = util_distance(speed_x, speed_y, 0, 0);
        double speed_setting = stat->motion_scmd.lineSpeed;

        double rate;
        if (0 == speed_now){
            rate = 0.1;
        } else {
            rate = speed_setting / speed_now;
        }

        speed_x = speed_x * rate;
        speed_y = speed_y * rate;
    }

    *x = pRbt->pos_x + speed_x * delta_t;
    *y = pRbt->pos_y + speed_y * delta_t;
}


/*检测未来10帧内，两个机器人是否会发生碰撞*/
static bool algo1_rbt_check_crash2(int rbt1, int rbt2, 
                      double * crash_x, double *crash_y){
    for (int frame = 1; frame <= 25; ++frame){
        double x1, y1, x2, y2;
        algo1_rbt_get_pos_on_frame(rbt1, frame, &x1, &y1);
        algo1_rbt_get_pos_on_frame(rbt2, frame, &x2, &y2);

        double distance = util_distance(x1, y1, x2, y2);
        if (distance < map_get_rbt_radius(rbt1)
                + map_get_rbt_radius(rbt2))
        {
            *crash_x = (x1 + x2) / 2;
            *crash_y = (y1 + y2) / 2;
            return true;
        }
    }
    
    return false;
}

/*避免碰撞算法
    1. 检测当前运动状态和要下达的指令，检测是否会发生碰撞
    2. 会发生碰撞---下达以碰撞点为中心做圆周运动的指令
       不会碰撞 ----- 原来的指令*/
static void algo1_rbt_check_crash(void){
    /*c_flag >= 0, 代表设置逆时针转动的角速度*/

    /*记录每个机器人的最快发生的碰撞点
    ，flag为true代表会发生碰撞*/
    double crash_point_x[MAX_ROBOT_NUM];
    double crash_point_y[MAX_ROBOT_NUM];
    bool crash_flag[MAX_ROBOT_NUM];

    int crash_rbt[MAX_ROBOT_NUM];   /*记录rbt会和哪个rbt相撞*/

    for (int rbtId = 0; rbtId < map_get_rbt_num(); ++rbtId){
        crash_flag[rbtId] = false;
        crash_rbt[rbtId] = -1;
    }

    for (int rbtId = 0; rbtId < map_get_rbt_num(); ++rbtId){
        /*记录当前的rbt发生的碰撞点*/
        double _x;
        double _y;
        double distance = MY_INFINITY;
        for (int rbtId1 = 0; rbtId1 < map_get_rbt_num(); ++rbtId1){
            if (rbtId == rbtId1){
                continue;
            }
            
            if (algo1_rbt_check_crash2(rbtId, rbtId1, 
                        &_x, &_y)){
                crash_flag[rbtId] = true;
                double rbt_x = map_get_rbt(rbtId)->pos_x;
                double rbt_y = map_get_rbt(rbtId)->pos_y;
                double _distance = util_distance(rbt_x, rbt_y, _x, _y);
                if (_distance < distance){
                    distance = _distance;
                    crash_point_x[rbtId] = _x;
                    crash_point_y[rbtId] = _y;
                    crash_rbt[rbtId] = rbtId1;
                }
            }
        }
    }

    /*根据碰撞点重新设置指令--下达让*/
    for (int rbtId = 0; rbtId < map_get_rbt_num(); ++rbtId){
        if (true != crash_flag[rbtId]){
            continue;
        }

        algo1_rbt_crash_set_motion_cmd(rbtId, 
                        crash_point_x[rbtId],
                        crash_point_y[rbtId]);
    }

}

static void algo1_rbt_go_point(int rbtId, double x, double y){
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

    double angleSpeed = angle/0.015;

    double linespeed = 0.1;

    /*距离越近速度越慢,每一帧的间隔是15ms即0.015s*/
    double dist = util_distance(x, y, rbt_x, rbt_y);

    linespeed = (dist > 2.0)? 
        MAX_ROBOT_FORWARD_SPEED : MAX_ROBOT_FORWARD_SPEED/2;

    if (4 == map_get_mapId()){
        linespeed = (angle > PI/2) ? (MAX_ROBOT_FORWARD_SPEED/3) : linespeed;
    } else {
        linespeed = (angle > PI/2) ? (0) : linespeed;
    }

    struct algo1_robot_state * rbt_stat = 
            algo1_rbt_get_state(rbtId);
    rbt_stat->motion_scmd.valid_flag = true;
    rbt_stat->motion_scmd.c_flag = flag;
    rbt_stat->motion_scmd.angleSpeed = angleSpeed;
    rbt_stat->motion_scmd.lineSpeed = linespeed;
}
#define LEVEL_UNKOWN    (-1)
#define LEVEL_0     (0)
#define LEVEL_1     (1)
#define LEVEL_2     (2)
#define LEVEL_3     (3)

#define LEVEL_NUM   (4) /*一共有四层级别*/
static int algo1_get_level(const struct working_table *p){
    int level = LEVEL_UNKOWN;
    if (NULL == p){
        LOG_RED("ERROR: (NULL == p)\n");
        return level;
    }
    switch (p->type)
    {
        case 1:
        case 2:
        case 3:{
            level = LEVEL_0;
            break;
        }
        case 4:
        case 5:
        case 6:{
            level = LEVEL_1;
            break;
        }
        case 7:{
            level = LEVEL_2;
            break;
        }
        case 8:{
            level = LEVEL_3;
            break;
        }
        default:{
            level = LEVEL_UNKOWN;
           // LOG_RED("ERROR: level unkown\n");
            break;
        }
    }

    return level;
}

/*todo: 需要考虑一个工作台有多个product的情况，
所以用工作台表示产品类型并不合理...........*/
/*todo: 还需要考虑不同的产品价格差价是不一样的，
并不是统一层的产品都是等价的产品。.........*/
struct product_pool {
    /*NULL,代表没有产品，!NULL代表有产品*/
    const struct working_table * wt[MAX_WORKING_TABLE_NUM];
};

/*只有三个层才有产品池*/
static struct product_pool g_algo1_product_pool[LEVEL_NUM-1];


static void algo1_pool_clear_product(void){
    memset(g_algo1_product_pool, 0, sizeof(g_algo1_product_pool));
}


static struct product_pool * algo1_pool_get_pool(int level){
    if (level >= (LEVEL_NUM-1)){
        LOG_RED("(level >= (LEVEL_NUM-1))\n");
        return NULL;
    }

    return &g_algo1_product_pool[level];
}


/*计算产品池中有没有物品，返回值为物品的个数*/
static int algo1_pool_get_product_num(int level){
    struct product_pool * pool = algo1_pool_get_pool(level);

    int num = 0;

    for (int i = 0; i < MAX_WORKING_TABLE_NUM; ++i){
        if (NULL != pool->wt[i]){
            num += 1;
        }
    }

    return num;
}


static const struct working_table * algo1_pool_get_product(int level, int pdtId){
    struct product_pool * pool = algo1_pool_get_pool(level);

    int id = 0;

    for (int i = 0; i < MAX_WORKING_TABLE_NUM; ++i){
        if (NULL != pool->wt[i]){
            if (id == pdtId){
                return pool->wt[i];
            }
            id += 1;
        }
    }

    LOG_RED("not found\n"); /*一定是能找到的，如果执行到这里，则说明错误*/
    return NULL;
}




static int algo1_pool_push_product(int wtId){

    /*TODO：需要判断更复杂的情况，如任务线上的产品已经购买过了，
    此时产生的新产品应该是可以加入到产品池的*/
    /*检查是否在其他机器人的任务线上,如果是，则直接返回即可*/
    /*检查是否有机器人的目标工作台是生产工作台*/
    for (int rbtId = 0; rbtId < map_get_rbt_num();++rbtId){
        struct algo1_robot_state * state = algo1_rbt_get_state(rbtId);
        if (ALGO1_RBT_STATE_AVAILABLE == state->state){
            continue;
        }

        bool flag;
        if (4 != map_get_mapId()){
            flag = (!map_rbt_has_product(rbtId));
        } else {
            flag = true;
        }
        
        if (state->task.start_wt_id == wtId && flag){
                /*之前分数
                {"status":"Successful","score":818099}
{"status":"Successful","score":673736}
{"status":"Successful","score":501270}
{"status":"Successful","score":660838}
TOTAL:818099+673736+501270+660838=2653943
*/
                /*{"status":"Successful","score":761764}
            {"status":"Successful","score":678227}
            {"status":"Successful","score":536501}
            {"status":"Successful","score":553722}
            TOTAL:761764+678227+536501+553722=2530214
                添加判断后的分数*/
            return 0;
        }
#if 1
        int dest_level = algo1_get_level(map_get_wt(state->task.dest_wt_id));
        if (state->task.dest_wt_id == wtId &&
            (LEVEL_1 == dest_level || LEVEL_2 == dest_level)){
            return 0;
            /*添加之后的分数：
            {"status":"Successful","score":836085}
{"status":"Successful","score":691370}
{"status":"Successful","score":524600}
{"status":"Successful","score":666254}
TOTAL:836085+691370+524600+666254=2718309
*/
        }
#endif
    }
    
    if (!map_wt_has_product(wtId)){
        return 0;
    }
    const struct working_table * const pWt = map_get_wt(wtId);
    int level = algo1_get_level(pWt);

    struct product_pool * pool = algo1_pool_get_pool(level);

    /*检查当前工作台有没有加入到产品池中*/
    for (int i = 0; i < MAX_WORKING_TABLE_NUM; ++i){
        if (pWt == pool->wt[i]){
            return 0;   /*已经在产品池了，直接返回*/
        }
    }

    /*把工作台加入到产品池中*/
    for (int i = 0; i < MAX_WORKING_TABLE_NUM; ++i){
        if (NULL == pool->wt[i]){
            pool->wt[i] = pWt;
            return 0;
        }
    }

    LOG_RED("error: cant't put working table to product pool\n");
    return ALGO1_RET_ERR;
}


static void algo1_pool_update(void){
    /*清空*/
    algo1_pool_clear_product();

    /*塞产品到产品池*/
    for (int wtId = 0; wtId < map_get_wt_num(); ++wtId){
        algo1_pool_push_product(wtId);
    }
}

static void algo1_pool_remove_product(int level, int pdtId){
    struct product_pool * pool = algo1_pool_get_pool(level);

    int id = 0;

    for (int i = 0; i < MAX_WORKING_TABLE_NUM; ++i){
        if (NULL != pool->wt[i]){
            if (pdtId == id){
                pool->wt[i] = NULL;
                return ;
            }
            id += 1;
        }
    }

    LOG_RED("not find\n");
}




/*只表示4,5,6,7四种平台即可*/
struct algo1_dest_table_state{
    unsigned int algo1RawMaterialState;/*根据机器人送东西的状态确定*/
};
static struct algo1_dest_table_state
    g_dest_wt_state[MAX_WORKING_TABLE_NUM];

static struct algo1_dest_table_state *
    algo1_dest_wt_get_state(int wtId){
    if (wtId >= map_get_wt_num()){
        LOG_RED("(wtId >= map_get_wt_num())");
        return NULL;
    }

    const struct working_table * wt = map_get_wt(wtId);

    return &g_dest_wt_state[wtId];
}

/*根据机器人的任务信息和工作台状态信息，更新目标工作台原材料格状态*/
static void algo1_dest_wt_update_state(void){
    for (int wtId = 0; wtId < map_get_wt_num(); ++wtId){
        struct algo1_dest_table_state * dest_state =
                algo1_dest_wt_get_state(wtId);
        const struct working_table * wt = map_get_wt(wtId);
        dest_state->algo1RawMaterialState = wt->raw_material_state;
    }

    for (int rbtId = 0; rbtId < map_get_rbt_num(); ++rbtId){
        struct algo1_robot_state * rbt_state = algo1_rbt_get_state(rbtId);

        if (ALGO1_RBT_STATE_AVAILABLE != rbt_state->state){
            int dest_id = rbt_state->task.dest_wt_id;
            int carray_type = rbt_state->task.carry_type;
        
            struct algo1_dest_table_state * dest_state =
                algo1_dest_wt_get_state(dest_id);

            if (map_unkown_prodcut(carray_type)){
                LOG_RED("(map_unkown_prodcut(type))\n");
                continue;
            }

            unsigned int * rawStat = &(dest_state->algo1RawMaterialState);
            map_set_raw_material_state(rawStat, carray_type);
        }
    }
}

static bool algo1_dest_src_match(int src_type, int dest_type){
    bool flag = false;
    switch(dest_type){
        case PRODUCT_TYPE_4:{
            if (PRODUCT_TYPE_1 == src_type || PRODUCT_TYPE_2 == src_type){
                flag = true;
            }
            break;
        }
        case PRODUCT_TYPE_5:{
            if (PRODUCT_TYPE_1 == src_type || PRODUCT_TYPE_3 == src_type){
                flag = true;
            }
            break;
        }
        case PRODUCT_TYPE_6:{
            if (PRODUCT_TYPE_2 == src_type || PRODUCT_TYPE_3 == src_type){
                flag = true;
            }
            break;
        }
        case PRODUCT_TYPE_7:{
            if (PRODUCT_TYPE_4 == src_type || PRODUCT_TYPE_5 == src_type ||
                PRODUCT_TYPE_6 == src_type){
                flag = true;
            }
            break;
        }
        case 8:{
            if (PRODUCT_TYPE_7 == src_type){
                flag = true;
            }
            break;
        }
        case 9:{
            flag = true;
            break;
        }
        default:{
            flag = false;
            LOG_RED("ERROR: dest_type wrong\n");
            break;
        }
    }

    return flag;
}

/*在level级别中，求出src能送出的dest的个数*/
static int algo1_dest_wt_get_available_num
( int level, const struct working_table * src){
    int num = 0;
    /*由于9号工作台可以接受任何物品，所以，最后找不到目标的时候
    可以把9号工作台用起来*/
    /*1. 8号工作台随意返回*/
    /*2. 4,5,6,7要判断*/
    for (int wtId = 0; wtId < map_get_wt_num(); ++wtId){
        const struct working_table * wt = map_get_wt(wtId);
        if (level != algo1_get_level(wt)){
            continue;
        }

        if (!algo1_dest_src_match(src->type, wt->type)){
            continue;
        }
        
        struct algo1_dest_table_state * state = 
            algo1_dest_wt_get_state(wtId);
        int carry_type = src->type;

        unsigned int rawMaterialStat = state->algo1RawMaterialState;

        if (map_has_raw_material(rawMaterialStat, carry_type)){
            continue;
        }

        num += 1;
    }

    for (int wtId = 0; wtId < map_get_wt_num(); ++wtId){
        if (9 == map_get_wt(wtId)->type){
            num += 1;
        }
    }

    return num;
}


static const struct working_table * algo1_dest_wt_get_available
( int level, int dest_id, const struct working_table * src){

    int id = -1;
    /*由于9号工作台可以接受任何物品，所以，最后找不到目标的时候
    可以把9号工作台用起来*/
    /*1. 8号工作台随意返回*/
    /*2. 4,5,6,7要判断*/
    for (int wtId = 0; wtId < map_get_wt_num(); ++wtId){
        const struct working_table * wt = map_get_wt(wtId);
        if (level != algo1_get_level(wt)){
            continue;
        }

        if (!algo1_dest_src_match(src->type, wt->type)){
            continue;
        }
        
        struct algo1_dest_table_state * state = 
            algo1_dest_wt_get_state(wtId);
        int carry_type = src->type;

        unsigned int rawMaterialStat = state->algo1RawMaterialState;

        if (map_has_raw_material(rawMaterialStat, carry_type)){
            continue;
        }

        id += 1;

        if (id == dest_id){
            return wt;
        }
    }

    for (int wtId = 0; wtId < map_get_wt_num(); ++wtId){
        const struct working_table * wt = map_get_wt(wtId);
        if (9 == wt->type){
            id += 1;
            if (id == dest_id){
                return wt;
            }
        }
    }

    /*下面肯定执行不到，用于调试*/
    LOG_BLUE("warning: not find available dest working talbe!\n");
    return NULL;
}


double algo1_dest_pool_calc_distacne(int rbtId, 
        const struct working_table * src,
        const struct working_table * dest){
    if (NULL == src || NULL == dest){
        return MY_INFINITY;
    }

    double rbt_x = map_get_rbt(rbtId)->pos_x;
    double rbt_y = map_get_rbt(rbtId)->pos_y;

    double src_x = src->pos_x;
    double src_y = src->pos_y;

    double dest_x = dest->pos_x;
    double dest_y = dest->pos_y;

    return (util_distance(rbt_x, rbt_y, src_x, src_y) +
            util_distance(src_x, src_y, dest_x, dest_y));
}

/*计算上下层级中，目标工作台，与源工作台中总距离与rbt最近的一条路径*/
int algo1_dest_pool_get_min_distance(
     const struct working_table ** src_wt, 
     const struct working_table ** dest_wt,
         int rbtId, int src_level, int dest_level){

    int src_pdt_num = algo1_pool_get_product_num(src_level);
    if (0 == src_pdt_num){
        *src_wt = NULL;
        *dest_wt = NULL;
        return 0;
    }
    
    const struct working_table * src_wt_tmp;
    const struct working_table * dest_wt_tmp;

    const struct working_table * src_wt_tmp_min = NULL;
    const struct working_table * dest_wt_tmp_min = NULL;

    double distance = MY_INFINITY;

    for (int pdtId = 0; pdtId < src_pdt_num; ++pdtId){
        src_wt_tmp = algo1_pool_get_product(src_level, pdtId);
        int dest_wt_num =
            algo1_dest_wt_get_available_num(dest_level, src_wt_tmp);

        for (int dest_wt_id = 0; dest_wt_id < dest_wt_num; ++dest_wt_id){
            dest_wt_tmp = algo1_dest_wt_get_available(
                        dest_level, dest_wt_id, src_wt_tmp);
            double distance_tmp = algo1_dest_pool_calc_distacne(
                                rbtId, src_wt_tmp, dest_wt_tmp);
            if (distance_tmp < distance ){
                src_wt_tmp_min = src_wt_tmp;
                dest_wt_tmp_min = dest_wt_tmp;
                distance = distance_tmp;
            }
        }
    }

    *src_wt = src_wt_tmp_min;
    *dest_wt = dest_wt_tmp_min;

    return 0;
}

void algo1_init(){
    for (int i = 0; i < map_get_rbt_num(); ++i){
        g_algo1_rbt_state[i].state = ALGO1_RBT_STATE_AVAILABLE;
        memset(&g_algo1_rbt_state[i].task, 0, sizeof(struct task_edge));
    }

    algo1_pool_clear_product();

    for(int wtId = 0; wtId < map_get_wt_num(); ++wtId){
        struct algo1_dest_table_state * wt_stat = 
                        algo1_dest_wt_get_state(wtId);
        wt_stat->algo1RawMaterialState = map_get_wt(wtId)->raw_material_state;
    }
}



/*处理判题器发来的一帧数据*/
int algo1_digest_one_frame(unsigned int frameid, unsigned int money){
    g_frameId = frameid;
    g_money = money;

    char line[1024];

    int wt_num = 0;
    if (scanf("%d", &wt_num) < 0){
        LOG_RED("error in scanf\n");
    }

    if (wt_num != map_get_wt_num()){
        LOG_RED("wt_num != map_get_wt_num()");
    }

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
    }

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
    }

    /*剩余数据*/
    while (fgets(line, sizeof line, stdin)) {
        if (line[0] == 'O' && line[1] == 'K') {
            return 0;
        }
    }
    
    LOG_RED("ret err!\n");
    return ALGO1_RET_ERR;
}

int algo1_run(int frameId, int money){
    int availableRbtNum = algo1_rbt_get_available_num();

    /*没有空闲机器人直接返回*/
    if (0 == availableRbtNum){
        return 0;
    }

    /*生成产品池*/
    algo1_pool_update();

    /*更新目标收购平台的状态，根据机器人的信息*/
    algo1_dest_wt_update_state();
    
    /*从上到下逐层处理*/
    for (int level = LEVEL_3; level >= LEVEL_1; --level){
        /*处理此层*/
        int dest_level = level;
        int src_level = level - 1;
        const struct working_table * dest_wt;
        const struct working_table * src_wt;

        int src_pdt_num = algo1_pool_get_product_num(src_level);
        if (0 == src_pdt_num){
            continue;
        }

        /*改进： 根据机器人去选工作台，所有的距离都算出来，选择最短的...*/
        for (int rbtId = 0; rbtId < map_get_rbt_num(); ++rbtId){
            struct algo1_robot_state * state = 
                algo1_rbt_get_state(rbtId);
            if (ALGO1_RBT_STATE_AVAILABLE != state->state){
                continue;
            }

            algo1_dest_pool_get_min_distance(
                    &src_wt, &dest_wt,
                    rbtId, src_level, dest_level);
            
            if (NULL == src_wt || NULL == dest_wt){
                continue;
            }

            algo1_rbt_add_task(rbtId, src_wt, dest_wt);
            algo1_pool_update();    /*更新产品池*/
            algo1_dest_wt_update_state();   /*更新目标工作台*/
        }

        if (0 == algo1_rbt_get_available_num()){
            return 0;
        }

        //not use
        /*todo: 由于9号工作台也可以收购物品，所以也处理一下*/
    }
}



int algo1_send_control_frame(int frameID){

    /*发送帧序号*/
    printf("%d\n", frameID);

    for (int rbtId = 0; rbtId < map_get_rbt_num(); ++rbtId){
        struct algo1_robot_state * rbt_stat = 
            algo1_rbt_get_state(rbtId);
        rbt_stat->motion_scmd.valid_flag = false;
        if (ALGO1_RBT_STATE_AVAILABLE == rbt_stat->state){
            LOG_RED("continue\n");
            continue;
        }

        int inWorkTable = map_get_rbt_in_which_wt(rbtId);

        
        /*状态机大法控制机器人的状态，机器人状态分为4种*/
        switch(rbt_stat->state){
        case ALGO1_RBT_STATE_BUSY:{
            if (map_rbt_has_product(rbtId)){
                if (inWorkTable != rbt_stat->task.dest_wt_id){
                    algo1_rbt_go_point(rbtId, rbt_stat->task.dest_x,
                                      rbt_stat->task.dest_y);
                } else {
                    command_rbt_sell(rbtId);
                    rbt_stat->state = ALGO1_RBT_STATE_SELLING;
                }
            }else {
                if (inWorkTable != rbt_stat->task.start_wt_id){
                    algo1_rbt_go_point(rbtId, rbt_stat->task.start_x,
                                      rbt_stat->task.start_y);
                } else {
                    if (algo1_get_remain_frame() >= 50){
                        command_rbt_buy(rbtId);
                    }
                    rbt_stat->state = ALGO1_RBT_STATE_BUYING;
                }
            }
            break;
        }
        case ALGO1_RBT_STATE_SELLING:{
            if (!map_rbt_has_product(rbtId)){
//                LOG_RED("robot %d not have product!\n", rbtId);
                rbt_stat->state = ALGO1_RBT_STATE_AVAILABLE;
                break;
            }
            
            if (inWorkTable != rbt_stat->task.dest_wt_id){
                algo1_rbt_go_point(rbtId, rbt_stat->task.dest_x,
                                      rbt_stat->task.dest_y);
            } else {
                command_rbt_sell(rbtId);
            }
            break;
        }
        case ALGO1_RBT_STATE_BUYING:{
            if (map_rbt_has_product(rbtId)){
           //     LOG_RED("robot %d alreay have product\n", rbtId);
                rbt_stat->state = ALGO1_RBT_STATE_BUSY;
                break ;
            }

            if (inWorkTable != rbt_stat->task.start_wt_id){
                algo1_rbt_go_point(rbtId, rbt_stat->task.start_x,
                                  rbt_stat->task.start_y);
            } else {
                if (algo1_get_remain_frame() >= 50){
                    command_rbt_buy(rbtId);
                }
            }
            break;
        }
        default:{
            /*永远不可能到这个分支*/
            LOG_RED("robot state error[%d]\n", rbt_stat->state);
            break;
        }
        }
    }

    /*碰撞检测与避免*/
    if (1 == map_get_mapId()){
        algo1_rbt_check_crash();
    } else {
        // do nothing;
    }

    /*发送运动命令*/
    for (int rbtId = 0; rbtId < map_get_rbt_num(); ++rbtId){
        struct algo1_robot_state * rbt_stat = 
                    algo1_rbt_get_state(rbtId);

        if (true != rbt_stat->motion_scmd.valid_flag){
            continue;
        }
    
        double flag = rbt_stat->motion_scmd.c_flag;
        double angleSpeed = rbt_stat->motion_scmd.angleSpeed;
        double linespeed = rbt_stat->motion_scmd.lineSpeed;

        if (flag >= 0 ){
            command_rbt_rotate_anticlockwise(rbtId, angleSpeed);
        } else {
            command_rbt_rotate_clockwise(rbtId, angleSpeed);
        }
        
        command_rbt_forward(rbtId, linespeed);
    }
    
    command_ok();
    command_send();

    return 0;
}

/*todo: 试试生产者消费者模型的算法*/

#endif
