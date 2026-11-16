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
    /* 暗棋扛旗终局：蓝工兵挖满 3 雷 → 扛红军旗 → 蓝胜 */
    jq_state a; jq_state_init(&a); a.mode=JQ_MODE_FLIP;
    memset(&a.cells,0,sizeof(a.cells)); a.stage=STG_PLAY; a.winner=JQ_WIN_NONE; a.turn=SIDE_BLUE;
    /* 蓝工兵 cell50，红军旗 cell55（大本营），蓝已挖 3 雷 */
    a.cells[50].rank=RANK_WORKER; a.cells[50].side=SIDE_BLUE; a.cells[50].revealed=1;
    a.cells[55].rank=RANK_FLAG; a.cells[55].side=SIDE_RED; a.cells[55].revealed=1;
    a.mines_cleared[SIDE_BLUE]=3;
    /* cell50→cell55 扛旗（假设相邻/铁路放行） */
    int can_flag = jq_can_move(&a,50,55);   /* ★ 取值须在 try_move 之前（吃子后攻方进被吃格） */
    printf("can_move(50→55 扛旗)=%d（工兵+挖3雷）\n", can_flag);
    int ev=jq_try_move(&a,50,55);
    printf("try ev=%d stage=%d winner=%d（期望 EV_FLAG + OVER + BLUE=0）\n", ev,a.stage,a.winner);
    /* 对照：未挖满 3 雷不能扛旗 */
    jq_state b; jq_state_init(&b); b.mode=JQ_MODE_FLIP;
    memset(&b.cells,0,sizeof(b.cells)); b.stage=STG_PLAY; b.winner=JQ_WIN_NONE; b.turn=SIDE_BLUE;
    b.cells[50].rank=RANK_WORKER; b.cells[50].side=SIDE_BLUE; b.cells[50].revealed=1;
    b.cells[55].rank=RANK_FLAG; b.cells[55].side=SIDE_RED; b.cells[55].revealed=1;
    b.mines_cleared[SIDE_BLUE]=2;
    printf("对照：挖 2 雷 can_move(50→55)=%d（期望 0 不能扛旗）\n", jq_can_move(&b,50,55));
    /* 对照：非工兵不能扛旗（暗棋） */
    jq_state c; jq_state_init(&c); c.mode=JQ_MODE_FLIP;
    memset(&c.cells,0,sizeof(c.cells)); c.stage=STG_PLAY; c.winner=JQ_WIN_NONE; c.turn=SIDE_BLUE;
    c.cells[50].rank=5; c.cells[50].side=SIDE_BLUE; c.cells[50].revealed=1;  /* 普通子 rank5 */
    c.cells[55].rank=RANK_FLAG; c.cells[55].side=SIDE_RED; c.cells[55].revealed=1;
    c.mines_cleared[SIDE_BLUE]=3;
    printf("对照：非工兵 can_move(50→55)=%d（期望 0，暗棋仅工兵扛旗）\n", jq_can_move(&c,50,55));
    
    /* ① 工兵挖满 3 雷 → 扛旗：can_move=1 + EV_FLAG(3) + stage=OVER(2) + winner=BLUE(0) */
    chk("① 工兵挖满3雷扛旗：can_move=1（try_move 前取值）", can_flag == 1);
    chk("① 工兵挖满3雷扛旗：EV_FLAG(3)+stage=OVER(2)+winner=BLUE(0)",
        ev == 3 && a.stage == 2 && a.winner == 0);
    /* 扛旗后工兵进旗格(55)+原格(50)清空 */
    chk("① 扛旗后：工兵进旗格(55 rank=9)+原格(50)空",
        a.cells[55].rank == RANK_WORKER && a.cells[50].rank == RANK_EMPTY);
    /* ② 未挖满 3 雷不能扛旗（can_move=0） */
    chk("② 对照：挖2雷不能扛旗 can_move=0", jq_can_move(&b,50,55) == 0);
    /* ③ 非工兵不能扛旗（暗棋仅工兵扛旗，can_move=0） */
    chk("③ 对照：非工兵不能扛旗 can_move=0", jq_can_move(&c,50,55) == 0);
    return g_ok ? 0 : 1;

}
