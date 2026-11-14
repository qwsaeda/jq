#include <stdio.h>
#include <string.h>
#include "junqi.h"
/* 吃子 rank 全序实证（规则原文：司令1>军长2>师长3>旅长4>团长5>营长6>连长7>排长8>工兵9）
   rank 小=强：任意 rA<rB → rA 吃 rB（EV_EAT）；rA>rB → rA 死（EV_DEAD）。
   全序 8 对相邻 + 跨级 + 工兵最弱验证。 */
static void put(jq_state*s,int i,int r,int side){s->cells[i].rank=r;s->cells[i].side=side;s->cells[i].revealed=1;}
static void setup(jq_state*s,int rA,int rB){
    jq_state_init(s); s->mode=JQ_MODE_LAY; memset(&s->cells,0,sizeof(s->cells));
    s->stage=STG_PLAY; s->winner=JQ_WIN_NONE; s->turn=SIDE_BLUE;
    put(s,44,rA,SIDE_BLUE); put(s,39,rB,SIDE_RED);  /* cell44(蓝) → cell39(红) 相邻 */
}
int main(void){
    int allpass=1;
    /* 相邻级：大吃小（rankA<rankB → A 吃 B）+ 小撞大（rankA>rankB → A 死）*/
    for (int r=1;r<=8;r++){
        jq_state a; setup(&a,r,r+1);  int ev=jq_try_move(&a,44,39);
        int ok=(ev==EV_EAT && a.cells[39].rank==r && a.cells[44].rank==0);
        jq_state b; setup(&b,r+1,r);  int ev2=jq_try_move(&b,44,39);
        int ok2=(ev2==EV_DEAD && b.cells[44].rank==0 && b.cells[39].rank==r);
        if(!ok||!ok2){ printf("  rank%d vs rank%d: 大吃小 ev=%d(应EAT) 小撞大 ev=%d(应DEAD) → %s\n",r,r+1,ev,ev2,(ok&&ok2)?"":"FAIL"); allpass=0; }
    }
    printf("① 相邻级 8 对全序（大吃小 EAT + 小撞大 DEAD）: %s\n", allpass?"PASS":"FAIL");
    /* 跨级：司令(1) 吃 工兵(9) 最大跨 */
    jq_state c; setup(&c,1,9); int evc=jq_try_move(&c,44,39);
    printf("② 司令(1)吃工兵(9) 跨级 ev=%d(应EAT) %s\n", evc, evc==EV_EAT?"PASS":"FAIL");
    /* 工兵(9) 最弱：撞 司令(1) 死 */
    jq_state d; setup(&d,9,1); int evd=jq_try_move(&d,44,39);
    printf("③ 工兵(9)撞司令(1) ev=%d(应DEAD) %s\n", evd, evd==EV_DEAD?"PASS":"FAIL");
    return allpass?0:1;
}
