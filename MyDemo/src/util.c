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

/*根据方向direction生成向量,方向范围[-Pi, Pi]*/
int gen_vector_from_direction(
                        double *x, double *y,
                        double direction){
    *x = 1;
    *y = tan(dirction);
    
    if (isnormal(*y)){
        return 0;
    } else {
        LOG_RED("isnormal(*y) not ok!\n");
        return UTIL_RET_ERR;
    }
}

/*根据两个点生成向量A->B,其中A(Ax, Ay), B(Bx, By)*/
void gen_vector_from_point(double * x, double * y
                            double Ax, double Ay,
                          double Bx, double By){
    *x = Bx - Ax;
    *y = By - Ay;
}

/*判断两个向量叉乘的结果向量的方向
参考：https://zhuanlan.zhihu.com/p/148780358
向量A, B表示：(Ax, Ay), 
            (Bx, By)
   右手螺旋定则：右手四指方向从a转向b，大拇指即得到c方向
   
   用于判断两个向量中，A向量向B向量靠近，是逆时针还是顺时针
   转动走过的弧度少。
   
   flag = Ax * By - Ay * Bx
   flag大于0，A逆时针转动到B
   flag<0, A顺时针转动到B
   flag==0,代表 A，B两个向量平行(可能方向是反的)，此时怎么转动都行

*/
double util_c_dirction(double Ax, double Ay, 
                       double Bx, double By){
    return (Ax*By - Ay*Bx);
}

/*todo: 把坐标的定义写到这里来*/
