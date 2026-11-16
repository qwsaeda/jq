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
int main(void){
    /* 工兵挖雷：蓝工兵 cell44(r4c4) → cell39(r3c4) 相邻，雷在 cell39 */
    jq_state a; jq_state_init(&a); a.mode=JQ_MODE_FLIP;
    memset(&a.cells,0,sizeof(a.cells)); a.stage=STG_PLAY; a.winner=JQ_WIN_NONE; a.turn=SIDE_BLUE;
    a.cells[44].rank=RANK_WORKER; a.cells[44].side=SIDE_BLUE; a.cells[44].revealed=1;
    a.cells[39].rank=RANK_MINE; a.cells[39].side=SIDE_RED; a.cells[39].revealed=1;
    int can_mine = jq_can_move(&a,44,39);   /* ★ 取值须在 try_move 之前 */
    printf("can_move(44→39)=%d\n", can_mine);
    int ev=jq_try_move(&a,44,39);
    printf("工兵挖雷 ev=%d（EV_MINE=5）mines_cleared[蓝]=%d cell44=%d cell39=%d\n",
        ev, a.mines_cleared[SIDE_BLUE], a.cells[44].rank, a.cells[39].rank);
    /* 对照：非工兵撞雷送死（EV_DEAD=4） */
    jq_state b; jq_state_init(&b); b.mode=JQ_MODE_FLIP;
    memset(&b.cells,0,sizeof(b.cells)); b.stage=STG_PLAY; b.winner=JQ_WIN_NONE; b.turn=SIDE_BLUE;
    b.cells[44].rank=5; b.cells[44].side=SIDE_BLUE; b.cells[44].revealed=1;
    b.cells[39].rank=RANK_MINE; b.cells[39].side=SIDE_RED; b.cells[39].revealed=1;
    int ev2=jq_try_move(&b,44,39);
    printf("非工兵撞雷 ev=%d（EV_DEAD=4）cell44=%d cell39=%d（攻方死、雷留）\n",
        ev2, b.cells[44].rank, b.cells[39].rank);
    
    /* ① 工兵挖雷：can_move=1 + EV_MINE(5) + mines_cleared[蓝]=1 + 雷格清空 */
    chk("① 工兵挖雷：can_move=1（try_move 前取值）", can_mine == 1);
    /* 挖雷语义：EV_MINE(5) + mines_cleared[蓝]=1 + 工兵进雷格(39 rank=9=工兵)
       + 原格(44)清空（工兵挖雷后占雷格，非空非雷） */
    chk("① 工兵挖雷：EV_MINE(5)+mines_cleared[蓝]=1+工兵进雷格+原格空",
        ev == 5 && a.mines_cleared[SIDE_BLUE] == 1
        && a.cells[39].rank == RANK_WORKER && a.cells[44].rank == RANK_EMPTY);
    /* ② 对照：非工兵撞雷 = 小撞大送死 → 规则层拒（can_move=0，ev=-1 非法，
          非 EV_DEAD）——§796-818 既定口径「明棋小撞大送死 jq_can_move 规则层拒
          can=0，EV_DEAD 仅暗棋场景触发」。本文件原注释「（EV_DEAD=4）」为旧口径
          残留（Round 1 D 域口径漂移项），按既定口径断言。 */
    chk("② 对照：非工兵撞雷被规则层拒（小撞大送死 can_move=0 + ev=-1）",
        jq_can_move(&b,44,39) == 0 && ev2 < 0
        && b.cells[44].rank == 5 && b.cells[39].rank == RANK_MINE);
    return g_ok ? 0 : 1;

}
