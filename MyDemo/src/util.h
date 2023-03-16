#ifndef __MYDEMO_UTIL_H__
#define __MYDEMO_UTIL_H__


#include "config.h"



#if TIMER_ON
void my_timer_start(void);
void my_timer_stop(void);
double my_timer_duration(void);

#define    util_time_start()    my_timer_start()
#define    util_time_stop()     my_timer_stop()
#define    util_time_duration() my_timer_duration()
#else

#define    util_time_start()
#define    util_time_stop()
#define    util_time_duration()     (0.00000)

#endif


double util_distance(double pos1_x, double pos1_y,
                         double pos2_x, double pos2_y);
double util_angle_on_direction(double x, double y,
                            double m, double n, double direction);
#endif
