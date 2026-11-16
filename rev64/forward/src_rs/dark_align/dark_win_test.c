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
    /* 暗棋：红方无普通可动子（仅剩旗/雷），蓝方有普通子 → 红困毙 → 蓝胜 */
    jq_state a; jq_state_init(&a); a.mode=JQ_MODE_FLIP;
    memset(&a.cells,0,sizeof(a.cells)); a.stage=STG_PLAY; a.winner=JQ_WIN_NONE; a.turn=SIDE_BLUE;
    /* 蓝方：cell50 普通 rank5 已翻（可动） */
    a.cells[50].rank=5; a.cells[50].side=SIDE_BLUE; a.cells[50].revealed=1;
    /* 红方：仅 cell55 军旗 + cell56 地雷（均已翻，无普通子） */
    a.cells[55].rank=RANK_FLAG; a.cells[55].side=SIDE_RED; a.cells[55].revealed=1;
    a.cells[56].rank=RANK_MINE; a.cells[56].side=SIDE_RED; a.cells[56].revealed=1;
    a.cells[50].rank=5; /* 蓝在 cell50 */
    /* 蓝走子（cell50→cell51 空）后 → 红方轮 → 红无普通可动子 → 困毙 → 蓝胜 */
    int ev=jq_try_move(&a,50,51);
    printf("蓝 50→51 ev=%d stage=%d winner=%d（期望 OVER + BLUE=0）\n", ev, a.stage, a.winner);
    
    /* 红方仅剩旗+雷（无普通可动子）→ 蓝走一步后红困毙 → stage=OVER(2)+winner=BLUE(0) */
    chk("① 蓝走子后红方困毙：stage=OVER(2)+winner=BLUE(0)",
        a.stage == 2 && a.winner == 0);
    chk("② 蓝走子事件为合法（ev>=0）", ev >= 0);
    return g_ok ? 0 : 1;

}
