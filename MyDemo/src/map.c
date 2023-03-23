#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "map.h"
#include "public.h"
#include "config.h"
#include "logging.h"

/* 根据任务说明书，地图数据是一个50*50m的地图
 * 用100*100 个字符表示，
 * 为了应对后续地图规模变化，这里不固定死地图大小
 */
static double g_map_width = 0;
static double g_map_height = 0;

static struct working_table g_wt[MAX_WORKING_TABLE_NUM];
static struct robot g_rbt[MAX_ROBOT_NUM];

static unsigned int g_wt_num = 0;
static unsigned int g_rbt_num = 0;


static unsigned int g_wt_id = 0;
static unsigned int g_rbt_id = 0;


static void dig_line(char * line, double height){
    /*当地图上标记某个位置为机器人或者工作台时，则他们的坐标是该区域的中心坐标。*/
    double x = LENGTH_PER_CHAR/2;
    while(*line != '\0'){
        switch(*line){
            case 'A':{
                g_rbt[g_rbt_num].pos_x = x;
                g_rbt[g_rbt_num].pos_y = height;
                g_rbt[g_rbt_num].id = g_rbt_id++;
                ++g_rbt_num;
                break;
            }
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':{
                g_wt[g_wt_num].pos_x = x;
                g_wt[g_wt_num].pos_y = height;
                char num[3];
                num[0] = *line; num[1] = '\0';
                g_wt[g_wt_num].type = atoi(num);
                g_wt[g_wt_num].id = g_wt_id++;
                ++g_wt_num;
                break;
            }
            case '.':
            case '\r':
            case '\n':{
                break;
            }
            default:{
                LOG_RED("digest err char [%d]\n", *line);
                break;
            }
        }
        
        ++line;
        x += LENGTH_PER_CHAR;
    }
}

int map_init(void){
    g_map_width = 0;
    g_map_height = 0;

    g_wt_id = 0;
    g_rbt_id = 0;
    /*一个字符0.5m*/
    char line[(int)(MAX_MAP_WIDTH/LENGTH_PER_CHAR + 10)];
    while (fgets(line, sizeof(line), stdin)) {
        /* 初始化 地图的宽度 */
        if (0 == g_map_width){
            int len = strlen(line);
            if (line[len - 1] == '\r' || line[len - 1] == '\n'){
                len -= 1;
            }
            if (line[len - 1] == '\r' || line[len - 1] == '\n'){
                len -= 1;
            }

            g_map_width = len * LENGTH_PER_CHAR;
        }

        if (line[0] == 'O' && line[1] == 'K') {
            for (int i = 0; i < g_wt_num; ++i){
                g_wt[i].pos_y = g_map_height - g_wt[i].pos_y;
            }

            for (int i  = 0; i < g_rbt_num; ++i){
                g_rbt[i].pos_y = g_map_height - g_rbt[i].pos_y;
            }
            return 0;
        }
        
        dig_line(line, g_map_height + LENGTH_PER_CHAR/2);
        g_map_height += LENGTH_PER_CHAR;

        //do something
    }

    return MAP_RET_ERR;
}

void map_print_des(void){
    LOG_YELLOW("map width: %f, map height: %f\n",g_map_width, g_map_height);
    LOG_YELLOW("working table list[num=%d]:\n", g_wt_num);
    for (int i = 0; i < g_wt_num; ++i){
        LOG("id[%d], type[%d], pos(%f, %f)\n",
            g_wt[i].id, g_wt[i].type, g_wt[i].pos_x, g_wt[i].pos_y);
    }

    LOG_YELLOW("robot list[num=%d]:\n", g_rbt_num);
    for (int i = 0; i < g_rbt_num; ++i){
        LOG("id[%d], pos(%f, %f)\n",
            g_rbt[i].id, g_rbt[i].pos_x, g_rbt[i].pos_y);
    }
}


int map_get_rbt_num(void){
    return g_rbt_num;
}

int map_get_wt_num(void){
    return g_wt_num;
}

int map_set_wt_state(int wt_id, int wt_type, 
    double posX, double posY, int remainPdtFrame, 
    unsigned int rawMaterialState, unsigned int productState){
    if (wt_id >= map_get_wt_num()){
        LOG_RED("(wt_id >= g_wt_num)");
        return MAP_RET_ERR;
    }
    
    struct working_table * p = &g_wt[wt_id];

    if (p->type != wt_type || p->pos_x != posX || p->pos_y != posY){
        LOG_RED ("(p->type != wt_type || p->pos_x != posX || p->pos_y != posY)");
        return MAP_RET_ERR;
    }

    p->remain_product_frame = remainPdtFrame;
    p->raw_material_state = rawMaterialState;
    p->product_state = productState;
}

int map_set_rbt_state(int rbtId,
                            int inWhichWt,
                            int carryItmType,
                            double timeValueFactor,
                            double collisionValueFactor,
                            double angleSpeed,
                            double lineSpeedX, double lineSpeedY,
                            double direction, 
                            double posX, double posY){
    if (rbtId >= map_get_rbt_num()){
        LOG_RED("(rbtId >= map_get_rbt_num())");
        return MAP_RET_ERR;
    }

    struct robot * p = &g_rbt[rbtId];

    if (inWhichWt >= map_get_wt_num()){
        LOG_RED("(inWhichWt >= map_get_wt_num())");
    }
    
    p->in_which_working_table = inWhichWt;
    p->carry_item_type = carryItmType;
    p->time_value_factor = timeValueFactor;
    p->collision_value_factor    = collisionValueFactor;
    p->angle_speed = angleSpeed;
    p->line_speed_x = lineSpeedX;
    p->line_speed_y = lineSpeedY;
    p->direction = direction;
    p->pos_x = posX;
    p->pos_y = posY;
}

const struct working_table * map_get_wt(int wtId){
    if (wtId >= g_wt_num){
        LOG_RED("(i >= g_rbt_num)");
        return NULL;
    }
    return &g_wt[wtId];
}

const struct robot * map_get_rbt(int i){
    if (i >= g_rbt_num){
        LOG_RED("(i >= g_rbt_num)");
        return NULL;
    }

    return &g_rbt[i];
}

/*检查两个工作台之间可不可以运输货物。*/
bool map_check_vality_between_node(const struct working_table *start,
                            const struct working_table *dest){

    if (NULL == start || NULL == dest){
        LOG_RED("ERROR: (NULL == start || NULL == dest)\n");
        return false;
    }

    /*判断工作台是否有货物*/
    if(PRODUCT_NONE == start->product_state){
        return false;
    }
    
    /*工作台4只能接受工作台1,2的货物*/
    unsigned int state = dest->raw_material_state;

    if (4 == dest->type){
        if (1 == start->type && !(state & PRODUCT_1_MASK)){
            return true;
        } else if (2 == start->type && !(state & PRODUCT_2_MASK)){
            return true;
        } else {
            return false;
        }
    /*工作台5只能接受工作台1,3的货物*/
    } else if (5 == dest->type){
        if (1 == start->type && !(state & PRODUCT_1_MASK)){
            return true;
        } else if (3 == start->type && !(state & PRODUCT_3_MASK)){
            return true;
        } else {
            return false;
        }
    } else if (6 == dest->type){
        if (2 == start->type && !(state & PRODUCT_2_MASK)){
            return true;
        } else if (3 == start->type && !(state & PRODUCT_3_MASK)){
            return true;
        } else {
            return false;
        }
    } else if (7 == dest->type){
        if (4 == start->type && !(state & PRODUCT_4_MASK)){
            return true;
        } else if (5 == start->type && !(state & PRODUCT_5_MASK)){
            return true;
        } else if (6 == start->type && !(state & PRODUCT_6_MASK)){
            return true;
        } else {
            return false;
        }
    } else if (8 == dest->type) {
        if (7 == start->type && !(state & PRODUCT_7_MASK)){
            return true;
        } else {
            return false;
        }
    } else if (9 == dest->type) {
        if (1 == start->type && !(state & PRODUCT_1_MASK)){
            return true;
        } else if (2 == start->type && !(state & PRODUCT_2_MASK)){
            return true;
        } else if (3 == start->type && !(state & PRODUCT_3_MASK)){
            return true;
        } else if (4 == start->type && !(state & PRODUCT_4_MASK)){
            return true;
        } else if (5 == start->type && !(state & PRODUCT_5_MASK)){
            return true;
        } else if (6 == start->type && !(state & PRODUCT_6_MASK)){
            return true;
        } else if (7 == start->type && !(state & PRODUCT_7_MASK)){
            return true;
        }
    }

    return false;
}


bool map_rbt_has_product(int rbtId){
    if (rbtId >= g_rbt_num){
        LOG_RED("(rbtId >= g_rbt_num)");
        return false;
    }

    return !!(g_rbt[rbtId].carry_item_type);
}

bool map_wt_has_product(int wtId){
    if (wtId >= g_wt_num){
        LOG_RED("(wtId >= g_wt_num)");
        return false;
    }

    return !!(g_wt[wtId].product_state);
}

int map_get_rbt_in_which_wt(int rbtId){
    if (rbtId >= g_rbt_num){
        LOG_RED("(rbtId >= g_rbt_num)");
        return ROBOT_IN_NONE_WORKING_TABLE;
    }

    return g_rbt[rbtId].in_which_working_table;
}

double map_get_rbt_radius(int rbtId){
    if (rbtId >= g_rbt_num){
        LOG_RED("(rbtId >= g_rbt_num)");
        return ROBOT_IN_NONE_WORKING_TABLE;
    }

    if (PRODUCT_TYPE_NONE == g_rbt[rbtId].carry_item_type){
        return ROBOT_CARRY_NONE_RADIUS;
    } else {
        return ROBOT_CARRY_RADIUS;
    }
}

bool map_unkown_prodcut(int product_type){
    switch (product_type){
        case PRODUCT_TYPE_1:
        case PRODUCT_TYPE_2:
        case PRODUCT_TYPE_3:
        case PRODUCT_TYPE_4:
        case PRODUCT_TYPE_5:
        case PRODUCT_TYPE_6:
        case PRODUCT_TYPE_7:{
            return false;
            }
        default:{
            return true;
        }
    }
}

void map_set_raw_material_state(
        unsigned int * state, int product_type){
    if (map_unkown_prodcut(product_type)){
        LOG_RED("(map_unkown_prodcut(product_type))");
        return ;
    }

    *state |= ((unsigned int)1 << product_type);
}

/*有对应产品，返回 true,否则返回false*/
bool map_has_raw_material(unsigned int state, int pdt){
    return !!(state & ((unsigned int)1 << pdt));
}