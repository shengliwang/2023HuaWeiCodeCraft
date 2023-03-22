#include "logging.h"
#include "map.h"
#include "algorithm.h"
#include "algorithm1.h"
#include "algorithm2.h"
#include "config.h"
#include "public.h"

static unsigned int g_frameId;
static unsigned int g_money;

int algo_get_frameId(void){
    return g_frameId;
}

int algo_get_money(void){
    return g_money;
}

void algo_init(void){
    #if ALGO1_EN
    algo1_init();
    #elif ALGO2_EN
    algo2_init();
    #endif
}


/*处理判题器发来的一帧数据*/
int algo_digest_one_frame(unsigned int frameid, unsigned int money){
    g_frameId = frameid;
    g_money = money;

    char line[1024];

    int wt_num = 0;
    if (scanf("%d", &wt_num) < 0){
        LOG_RED("error in scanf\n");
    }

    if (wt_num != map_get_wt_num()){
        LOG_RED("wt_num != map_get_wt_num()");
    }

    /*读取每一帧工作台的信息*/
    for (int i = 0; i < wt_num; ++i){
        int wtType;
        double posX, posY;
        int remainPdtFrame;     /*剩余生产帧数*/
        unsigned int rawMaterialState;
        unsigned int productState;

        scanf("%d %lf %lf %d %d %d", &wtType, &posX, &posY, &remainPdtFrame,
                                    &rawMaterialState, &productState);
        map_set_wt_state(i, wtType, posX, posY, 
                            remainPdtFrame, rawMaterialState, productState);
    }

    /*读取每一帧的机器人信息*/
    for (int i = 0; i < map_get_rbt_num(); ++i){
        int inWhichWt;  /*所处工作台*/
        int carryItmType;/*携带物品类型*/
        double timeValueFactor; /*时间价值系数*/
        double collisionValueFactor;    /*碰撞价值系数*/
        double angleSpeed;          /*角速度*/
        double lineSpeedX, lineSpeedY;  /*线速度*/
        double direction;   /*朝向*/
        double posX, posY;  /*坐标*/
        
        scanf("%d %d %lf %lf %lf %lf %lf %lf %lf %lf",
                        &inWhichWt, &carryItmType,
                        &timeValueFactor, &collisionValueFactor, 
                        &angleSpeed, &lineSpeedX, &lineSpeedY,&direction,
                        &posX, &posY);
        map_set_rbt_state(i, inWhichWt, carryItmType,
                        timeValueFactor, collisionValueFactor,
                        angleSpeed, lineSpeedX, lineSpeedY,direction,
                        posX, posY);
    }

    /*剩余数据*/
    while (fgets(line, sizeof line, stdin)) {
        if (line[0] == 'O' && line[1] == 'K') {
            return 0;
        }
    }
    
    LOG_RED("ret err!\n");
    return ALGO_RET_ERR;
}

int algo_run(void){
    int ret = 0;
    #if ALGO1_EN
    ret = algo1_run();
    #elif ALGO2_EN
    ret = algo2_run();
    #endif

    return ret;
}

int algo_send_control_frame(void){
    int ret = 0;
    #if ALGO1_EN
    ret = algo1_send_control_frame();
    #elif ALGO2_EN
    ret = algo2_send_control_frame();
    #endif

    return ret;
}

