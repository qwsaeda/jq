#include <stdio.h>
#include <string.h>
#include "junqi.h"
/* 吃子方向实证（rank 小=强，司令 rank1 最强）：
   大压小=吃 / 小撞大=送死 / 同级=同归。规则原文"司令>军长>...>工兵"。 */
static void put(jq_state*s,int i,int r,int side){s->cells[i].rank=r;s->cells[i].side=side;s->cells[i].revealed=1;}
int main(void){
    /* ① 司令(1) 吃 军长(2)：大压小 → EV_EAT，司令留 */
    jq_state a; jq_state_init(&a); a.mode=JQ_MODE_LAY; memset(&a.cells,0,sizeof(a.cells));
    a.stage=STG_PLAY; a.winner=JQ_WIN_NONE; a.turn=SIDE_BLUE;
    put(&a,44,1,SIDE_BLUE); put(&a,39,2,SIDE_RED);   /* cell44 蓝司令 → cell39 红军长（相邻） */
    int ev=jq_try_move(&a,44,39);
    printf("① 司令吃军长 ev=%d(1=EAT) 蓝格=%d 红格=%d（期望 EAT+蓝1留+红格空）\n",
        ev, a.cells[39].rank, a.cells[44].rank);
    /* ② 军长(2) 撞 司令(1)：小撞大 → EV_DEAD，军长死 */
    jq_state b; jq_state_init(&b); b.mode=JQ_MODE_LAY; memset(&b.cells,0,sizeof(b.cells));
    b.stage=STG_PLAY; b.winner=JQ_WIN_NONE; b.turn=SIDE_BLUE;
    put(&b,44,2,SIDE_BLUE); put(&b,39,1,SIDE_RED);
    int ev2=jq_try_move(&b,44,39);
    printf("② 军长撞司令 ev=%d(4=DEAD) 蓝格=%d 红格=%d（期望 DEAD+蓝格空+红1留）\n",
        ev2, b.cells[44].rank, b.cells[39].rank);
    /* ③ 同级 团长(5) vs 团长(5) → 同归 EV_EAT（§17 三层口径：AI 900 / 执行器 EV_EAT） */
    jq_state c; jq_state_init(&c); c.mode=JQ_MODE_LAY; memset(&c.cells,0,sizeof(c.cells));
    c.stage=STG_PLAY; c.winner=JQ_WIN_NONE; c.turn=SIDE_BLUE;
    put(&c,44,5,SIDE_BLUE); put(&c,39,5,SIDE_RED);
    int ev3=jq_try_move(&c,44,39);
    printf("③ 同级同归 ev=%d(1=EAT) 蓝格=%d 红格=%d（期望 EAT+两格空）\n",
        ev3, c.cells[44].rank, c.cells[39].rank);
    return 0;
}
