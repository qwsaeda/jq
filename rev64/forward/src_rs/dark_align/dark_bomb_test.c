#include <stdio.h>
#include <string.h>
#include "junqi.h"
/* 炸弹同归全场景（rulelay"炸弹与任何棋子相遇双方都消失，包括军旗"） */
static void put(jq_state*s,int i,int r,int side){s->cells[i].rank=r;s->cells[i].side=side;s->cells[i].revealed=1;}
static void setup2(jq_state*s){
    jq_state_init(s); s->mode=JQ_MODE_LAY; memset(&s->cells,0,sizeof(s->cells));
    s->stage=STG_PLAY; s->winner=JQ_WIN_NONE; s->turn=SIDE_BLUE;
    s->cells[28].rank=RANK_BOMB; s->cells[28].side=SIDE_BLUE; s->cells[28].revealed=1;
    s->cells[33].rank=RANK_BOMB; s->cells[33].side=SIDE_RED; s->cells[33].revealed=1;
}
static void setup(jq_state*s,int rA,int rB){
    jq_state_init(s); s->mode=JQ_MODE_LAY; memset(&s->cells,0,sizeof(s->cells));
    s->stage=STG_PLAY; s->winner=JQ_WIN_NONE; s->turn=SIDE_BLUE;
    put(s,44,rA,SIDE_BLUE); put(s,39,rB,SIDE_RED);
}
int main(void){
    int ok=1;
    /* ① 蓝炸弹(10) 撞 红军长(2) → 同归两格空，EV_BOMB */
    jq_state a; setup(&a,RANK_BOMB,2); int ev=jq_try_move(&a,44,39);
    int c1=(ev==EV_BOMB && a.cells[44].rank==0 && a.cells[39].rank==0);
    printf("① 炸弹撞普通子 ev=%d(应BOMB=2) 双格空=%d %s\n",ev,a.cells[44].rank==0&&a.cells[39].rank==0,c1?"PASS":"FAIL"); ok&=c1;
    /* ② 蓝炸弹 撞 红军旗(12) → 同归 + 扛旗方(蓝)胜（EV_BOMB 撞旗判胜，AUD-R-020） */
    jq_state b; setup(&b,RANK_BOMB,RANK_FLAG); int ev2=jq_try_move(&b,44,39);
    int c2=(ev2==EV_BOMB && b.stage==STG_OVER && b.winner==JQ_WIN_BLUE);
    printf("② 炸弹撞军旗 ev=%d(应BOMB) stage=%d(应OVER) winner=%d(应BLUE=0) %s\n",ev2,b.stage,b.winner,c2?"PASS":"FAIL"); ok&=c2;
    /* ③ 红炸弹 撞 蓝军旗 → 红胜 */
    jq_state c; jq_state_init(&c); c.mode=JQ_MODE_LAY; memset(&c.cells,0,sizeof(c.cells));
    c.stage=STG_PLAY; c.winner=JQ_WIN_NONE; c.turn=SIDE_RED;
    c.cells[44].rank=RANK_BOMB; c.cells[44].side=SIDE_RED; c.cells[44].revealed=1;
    c.cells[39].rank=RANK_FLAG; c.cells[39].side=SIDE_BLUE; c.cells[39].revealed=1;
    int ev3=jq_try_move(&c,44,39);
    int c3=(ev3==EV_BOMB && c.stage==STG_OVER && c.winner==JQ_WIN_RED);
    printf("③ 红炸弹撞蓝军旗 ev=%d winner=%d(应RED=1) %s\n",ev3,c.winner,c3?"PASS":"FAIL"); ok&=c3;
    /* ④ 炸弹撞炸弹（双炸弹无旗）→ 同归双格空 + 双方无普通子 → Draw（和棋）。
       原版 0x302f0：sp[16](蓝普通子)==0 && sp[17](红普通子)==0 → winner=3 Draw。
       普通子 = rank 1-10 含炸弹（RANK_BOMB=10），双炸弹消失后双方无普通子 → Draw。
       ★ 2026-11-14 定案：旧用例④期望 stage=PLAY 无旗不判胜是错误期望——
       双炸弹无旗后双方确实无普通子（炸弹 rank 10 是普通子已消失），原版判 Draw。 */
    jq_state d; setup2(&d); int ev4=jq_try_move(&d,28,33);
    int c4=(ev4==EV_BOMB && d.cells[28].rank==0 && d.cells[33].rank==0
            && d.stage==STG_OVER && d.winner==JQ_WIN_DRAW);
    printf("④ 双炸弹无旗 ev=%d(应BOMB) 双格空 stage=%d(应OVER) winner=%d(应DRAW=2) %s\n",
           ev4, d.stage, d.winner, c4?"PASS":"FAIL"); ok&=c4;
    printf("炸弹同归全场景: %s\n", ok?"PASS":"FAIL");
    return ok?0:1;
}
