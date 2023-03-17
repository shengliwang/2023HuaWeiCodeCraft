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

int util_gen_vector_from_direction(
                        double *x, double *y,
                        double direction);

void util_gen_vector_from_point(double * x, double * y,
                            double Ax, double Ay,
                          double Bx, double By);

double util_c_dirction(double Ax, double Ay, 
                       double Bx, double By);

double util_vector_angle(double Ax, double Ay,
                            double Bx, double By);
#endif
