#ifndef __MYDEMO_CONFIG_H__
#define __MYDEMO_CONFIG_H__


#define PI (3.1415926)

#define LENGTH_PER_CHAR     (0.5)     /*地图数据，一个字符表示0.5m*/
#define MAX_MAP_WIDTH       (100)     /*地图最大宽度，100m*/
#define MAX_WORKING_TABLE_NUM   (50)    /*地图中最大工作台数目， 50个*/
#define MAX_ROBOT_NUM       (4)        /*地图中最多的机器人数目*/

#define MAX_ROBOT_FORWARD_SPEED (6.0) /*机器人最大的前进速度 6m/s*/
#define MAX_ROBOT_BACK_SPEED    (2.0) /*机器人最大的后退速度 2m/s*/

#define MAX_ROBOT_ROTATE_SPEED  (PI)    /*机器人最大旋转速度 PI/s*/

/*note: 地图相关的规则应配置如下函数*/
//map_check_vality_between_node---判断两个工作台之间是否能传输货物



/*以下为程序调试功能*/
#define LOG_ON      (1)     /*log 功能是否开启*/
#define TIMER_ON    (1)     /*计时功能是否开启*/




#define   ALGO1_EN  (1)     /*启用算法1*/



#endif
