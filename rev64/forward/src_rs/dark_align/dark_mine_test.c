#include <stdio.h>
#include <string.h>
#include "junqi.h"
int main(void){
    /* 工兵挖雷：蓝工兵 cell44(r4c4) → cell39(r3c4) 相邻，雷在 cell39 */
    jq_state a; jq_state_init(&a); a.mode=JQ_MODE_FLIP;
    memset(&a.cells,0,sizeof(a.cells)); a.stage=STG_PLAY; a.winner=JQ_WIN_NONE; a.turn=SIDE_BLUE;
    a.cells[44].rank=RANK_WORKER; a.cells[44].side=SIDE_BLUE; a.cells[44].revealed=1;
    a.cells[39].rank=RANK_MINE; a.cells[39].side=SIDE_RED; a.cells[39].revealed=1;
    printf("can_move(44→39)=%d\n", jq_can_move(&a,44,39));
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
    return 0;
}
