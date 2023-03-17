#include <time.h>
#include <math.h>

#include "util.h"

#include "config.h"
#include "logging.h"

#if TIMER_ON
static clock_t g_start_time;
static clock_t g_stop_time;

void my_timer_start(void){
    g_start_time = clock();
}

void my_timer_stop(void){
    g_stop_time = clock();
}

/*ms级别*/
double my_timer_duration(void){
    return (g_stop_time - g_start_time) * 1.0 * 1000 / (CLOCKS_PER_SEC);
}

#endif



double util_distance(double pos1_x, double pos1_y, double pos2_x, double pos2_y){
    return sqrt
            ( (pos1_x - pos2_x)*(pos1_x - pos2_x) + 
              (pos1_y - pos2_y)*(pos1_y - pos2_y) );
}

/*计算(x,y)和(m,n)连线与direction之间的夹角(-PI, PI)之间的*/
double util_angle_on_direction(double x, double y, double m, double n, double direction){
    /*direction 的向量通过点(x,y)*/
    double a, b;    /*定义第三点以组成两个向量*/
    double tanTheta = tan(direction);

    if (-PI/2 == direction){
        LOG_RED("warning:(-PI/2 == direction)");
        a = x;
        b = y - 1;
    } else if (PI/2 == direction){
        LOG_RED("warning:(PI/2 == direction)");
        a = x;
        b = y + 1;
    } else if (PI == direction || -PI == direction){
        a = x - 1;
        b = y;
    } else if (0 == direction){
        a = x + 1;
        b = y;
    } else {
        if (tanTheta < 0){
            if (direction < 0){
                a = x + 1;
                b = y + tanTheta;
            } else {
                a = x -1;
                b = y - tanTheta;
            }
        } else if (tanTheta > 0){
            if (direction < 0){
                a = x - 1;
                b = y - tanTheta;
            } else {
                a = x + 1;
                b = y + tanTheta;
            }
        } else {
            LOG_RED("tanTheta == 0\n");
        }
    }

    double ab_x = a - x;
    double ab_y = b - y;

    double cd_x = m - x;
    double cd_y = n - y;

    

    double cosTheta = (ab_x * cd_x + ab_y * cd_y) / 
               (util_distance(ab_x, ab_y, 0, 0)*util_distance(cd_x, cd_y, 0, 0));

    return acos(cosTheta);
    
    /*todo到网上搜搜PI有没有C语言标准宏定义*/
}

/*todo: 把坐标的定义写到这里来*/
