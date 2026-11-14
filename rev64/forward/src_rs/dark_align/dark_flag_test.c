#include <stdio.h>
#include <string.h>
#include "junqi.h"
int main(void){
    /* 暗棋扛旗终局：蓝工兵挖满 3 雷 → 扛红军旗 → 蓝胜 */
    jq_state a; jq_state_init(&a); a.mode=JQ_MODE_FLIP;
    memset(&a.cells,0,sizeof(a.cells)); a.stage=STG_PLAY; a.winner=JQ_WIN_NONE; a.turn=SIDE_BLUE;
    /* 蓝工兵 cell50，红军旗 cell55（大本营），蓝已挖 3 雷 */
    a.cells[50].rank=RANK_WORKER; a.cells[50].side=SIDE_BLUE; a.cells[50].revealed=1;
    a.cells[55].rank=RANK_FLAG; a.cells[55].side=SIDE_RED; a.cells[55].revealed=1;
    a.mines_cleared[SIDE_BLUE]=3;
    /* cell50→cell55 扛旗（假设相邻/铁路放行） */
    printf("can_move(50→55 扛旗)=%d（工兵+挖3雷）\n", jq_can_move(&a,50,55));
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
    return 0;
}
