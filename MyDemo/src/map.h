#ifndef __MYDEMO_MAP_H__
#define __MYDEMO_MAP_H__
#include <stdbool.h>

#define PRODUCT_NONE    0   /*产品格无产品*/
#define PRODUCT_OK      1   /*产品格有产品*/

#define PRODUCT_1_MASK  0x02 /*0000 0010b*/
#define PRODUCT_2_MASK  0x04 /*0000 0100b*/
#define PRODUCT_3_MASK  0x08 /*0000 1000b*/
#define PRODUCT_4_MASK  0x10 /*0001 0000b*/
#define PRODUCT_5_MASK  0x20 /*0010 0000b*/
#define PRODUCT_6_MASK  0x40 /*0100 0000b*/
#define PRODUCT_7_MASK  0x80 /*1000 0000b*/


#define ROBOT_IN_NONE_WORKING_TABLE     (-1)


#define FRAME_INTVAL        (0.02)  /*帧间隔为 20ms */

#define ROBOT_CARRY_RADIUS  (0.53)  /*机器人携带物品半径*/
#define ROBOT_CARRY_NONE_RADIUS (0.45)  /*机器人未携带物品半径*/

struct working_table{
    double pos_x;
    double pos_y;
    int type; /*type取值为1-9，代表1-9号工作台*/
    int id; /*编号*/
    unsigned int raw_material_state;    /*原材料格状态，二进制位表表示*/
    int product_state;  /*产品格状态， 0表示无，1表示有*/
    int remain_product_frame; /*产品剩余生产时间
                                -1:表示没有生产。
                                0:表示生产因输出格满而阻塞。
                                >=0:表示剩余生产帧数。*/
};

struct robot{
    double pos_x;
    double pos_y;
    int id; /*机器人编号*/

    int in_which_working_table; /*当前所处的工作台id*/
    int carry_item_type;    /*当前搬运的货品类型,范围[0,7],0表示未携带物品。
                                1-7表示对应物品*/
    double time_value_factor;   /*时间价值系数*/
    double collision_value_factor;  /*碰撞价值系数*/
    double angle_speed;         /*角速度*/
    double line_speed_x; double line_speed_y;   /*线速度*/
    double direction;   /*朝向*/
};

int map_init(void);
void map_print_des(void);
int map_get_rbt_num(void);
int map_get_wt_num(void);

int map_set_rbt_state(int rbtId,
                            int inWhichWt,
                            int carryItmType,
                            double timeValueFactor,
                            double collisionValueFactor,
                            double angleSpeed,
                            double lineSpeedX, double lineSpeedY,
                            double direction, 
                            double posX, double posY);

int map_set_wt_state(int wt_id, int wt_type, 
    double posX, double posY, int remainPdtFrame, 
    unsigned int rawMaterialState, unsigned int productState);
const struct working_table * map_get_wt(int wtId);
const struct robot * map_get_rbt(int i);
bool map_check_vality_between_node(const struct working_table *start,
                            const struct working_table *dest);
bool map_rbt_has_product(int rbtId);
bool map_wt_has_product(int wtId);
int map_get_rbt_in_which_wt(int rbtId);
double map_get_rbt_radius(int rbtId);
bool map_unkown_prodcut(int product_type);
void map_set_raw_material_state(unsigned int * state, int product_type);
bool map_has_raw_material(unsigned int state, int pdt);
#endif
