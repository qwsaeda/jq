#include <stdio.h>
#include <string.h>
#include "junqi.h"
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
    return 0;
}
