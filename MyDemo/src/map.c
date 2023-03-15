#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

struct working_table{
    double pos_x;
    double pos_y;
    int type; /*type取值为1-9，代表1-9号工作台*/
    int id; /*编号*/
};

struct robot{
    double pos_x;
    double pos_y;
    int id; /*机器人编号*/
};

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
                LOG_ERR("digest err char [%d]\n", *line);
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
    LOG_INFO("map width: %f, map height: %f\n",g_map_width, g_map_height);
    LOG_INFO("working table list[num=%d]:\n", g_wt_num);
    for (int i = 0; i < g_wt_num; ++i){
        LOG("id[%d], type[%d], pos(%f, %f)\n",
            g_wt[i].id, g_wt[i].type, g_wt[i].pos_x, g_wt[i].pos_y);
    }

    LOG_INFO("robot list[num=%d]:\n", g_rbt_num);
    for (int i = 0; i < g_rbt_num; ++i){
        LOG("id[%d], pos(%f, %f)\n",
            g_rbt[i].id, g_rbt[i].pos_x, g_rbt[i].pos_y);
    }
}


int map_get_rbt_num(void){
    return g_rbt_num;
}