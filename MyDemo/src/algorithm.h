#ifndef __MYDEMO_ALGORITHM_H__
#define __MYDEMO_ALGORITHM_H__

int algo_get_frameId(void);
int algo_get_money(void);

void algo_init(void);
int algo_digest_one_frame(unsigned int frameid, unsigned int money);
int algo_run(void);
int algo_send_control_frame(void);

#endif
