#ifndef __MYDEMO_COMMAND_H__
#define __MYDEMO_COMMAND_H__

int command_rbt_forward(int rbt_id, double speed);
int command_rbt_back(int rbt_id, double speed);
int command_rbt_rotate_clockwise(int rbt_id, double speed);
int command_rbt_rotate_anticlockwise(int rbt_id, double speed);

int command_rbt_buy(int rbt_id);
int command_rbt_sell(int rbt_id);
int command_rbt_destroy(int rbt_id);


#define command_ok()    printf("OK\n")
#define command_send()  fflush(stdout)

#endif
