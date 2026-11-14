#include <stdio.h>
#include <string.h>
#include "junqi.h"
/* 铁路滑行阻断（rulelay"铁路上没有障碍时"）：
   col2 纵线被中心行营(3,2)/(8,2)阻断 + 中间有子阻断 + 工兵可拐弯/非工兵直走 */
int main(void){
    /* ① 非工兵纵线直走被中心行营阻断：cell(4,2)→cell(7,2) 经 row5/6（行营? row5col2 非行营） */
    jq_state a; jq_state_init(&a); a.mode=JQ_MODE_LAY;
    memset(&a.cells,0,sizeof(a.cells)); a.stage=STG_PLAY; a.winner=JQ_WIN_NONE; a.turn=SIDE_BLUE;
    a.cells[cell_index(4,2)].rank=5; a.cells[cell_index(4,2)].side=SIDE_BLUE; a.cells[cell_index(4,2)].revealed=1;
    printf("① 非工兵 (4,2)→(7,2) 纵线直走 can=%d（row5/6 无行营，期望 1）\n",
        jq_can_move(&a,cell_index(4,2),cell_index(7,2)));
    /* ② 中间有子阻断：(4,2) 上有子挡 (5,2) → (4,2)→(7,2) 不可 */
    jq_state b; jq_state_init(&b); b.mode=JQ_MODE_LAY;
    memset(&b.cells,0,sizeof(b.cells)); b.stage=STG_PLAY; b.winner=JQ_WIN_NONE; b.turn=SIDE_BLUE;
    b.cells[cell_index(4,2)].rank=5; b.cells[cell_index(4,2)].side=SIDE_BLUE; b.cells[cell_index(4,2)].revealed=1;
    b.cells[cell_index(5,2)].rank=3; b.cells[cell_index(5,2)].side=SIDE_RED; b.cells[cell_index(5,2)].revealed=1;
    printf("② 中间有子 (5,2) 挡 → (4,2)→(7,2) can=%d（期望 0 阻断）\n",
        jq_can_move(&b,cell_index(4,2),cell_index(7,2)));
    /* ③ 工兵可拐弯：(4,2)→(5,4)（纵→横拐） */
    jq_state c; jq_state_init(&c); c.mode=JQ_MODE_LAY;
    memset(&c.cells,0,sizeof(c.cells)); c.stage=STG_PLAY; c.winner=JQ_WIN_NONE; c.turn=SIDE_BLUE;
    c.cells[cell_index(4,2)].rank=RANK_WORKER; c.cells[cell_index(4,2)].side=SIDE_BLUE; c.cells[cell_index(4,2)].revealed=1;
    printf("③ 工兵拐弯 (4,2)→(5,4) can=%d（期望 1）\n",
        jq_can_move(&c,cell_index(4,2),cell_index(5,4)));
    /* ④ 非工兵不可拐弯：(4,2)→(5,4) */
    jq_state d; jq_state_init(&d); d.mode=JQ_MODE_LAY;
    memset(&d.cells,0,sizeof(d.cells)); d.stage=STG_PLAY; d.winner=JQ_WIN_NONE; d.turn=SIDE_BLUE;
    d.cells[cell_index(4,2)].rank=5; d.cells[cell_index(4,2)].side=SIDE_BLUE; d.cells[cell_index(4,2)].revealed=1;
    printf("④ 非工兵拐弯 (4,2)→(5,4) can=%d（期望 0 直走不拐弯）\n",
        jq_can_move(&d,cell_index(4,2),cell_index(5,4)));
    return 0;
}
