#ifndef __MYDEMO_ALGORITHM1_H__
#define __MYDEMO_ALGORITHM1_H__
void algo1_init(void);
int algo1_digest_one_frame(unsigned int frameid, unsigned int money);

int algo1_run(int frameId, int money);
int algo1_send_control_frame(int frameID);

#endif
