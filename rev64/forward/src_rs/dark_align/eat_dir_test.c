#include <stdio.h>
#include <string.h>
#include "junqi.h"

/* ★ Round 1 对抗审计 P0（§104）：本用例原为「零断言观测脚本」（只打印观察值、
   main 尾部无条件 return 0 → 无论行为如何都判 PASS = 假绿三型之判据恒真）。
   现补真断言 + 条件返回（退出码 + 行首 FAIL 双保险），期望值取自本文件注释与
   2026-11-16 实跑核对（口径见各断言行尾注释）。 */
static int g_ok = 1;
static void chk(const char *name, int cond)
{
    printf("%s %s\n", cond ? "PASS" : "FAIL", name);
    if (!cond) g_ok = 0;
}
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
    
    /* ① 大压小：司令吃军长 = EV_EAT(1) + 蓝格留 rank1 + 红格清空 */
    /* 吃子语义（★ 修正本轮初版期望）：攻方进被吃格(39 rank=1 司令)，原格(44)空 */
    chk("① 司令吃军长：EV_EAT(1)+攻方进被吃格+原格空",
        ev == 1 && a.cells[39].rank == 1 && a.cells[44].rank == RANK_EMPTY);
    /* ② 小撞大送死：EV_DEAD(4) + 蓝格清空 + 红格留 rank1 */
    chk("② 军长撞司令：EV_DEAD(4)+蓝格空+红格留",
        ev2 == 4 && b.cells[44].rank == RANK_EMPTY && b.cells[39].rank == 1);
    /* ③ 同级同归：EV_EAT(1) + 两格皆空 */
    chk("③ 同级同归：EV_EAT(1)+两格空",
        ev3 == 1 && c.cells[44].rank == RANK_EMPTY && c.cells[39].rank == RANK_EMPTY);
    return g_ok ? 0 : 1;

}
