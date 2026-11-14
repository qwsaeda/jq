#include <stdio.h>
#include <string.h>
#include "junqi.h"
extern const uint8_t JQ_DEFAULT_LAY[30];
static void put(jq_state *st, int row, int col, int rank, int side, int rev){
    jq_cell *c=&st->cells[cell_index(row,col)];
    c->rank=(uint8_t)rank; c->side=(uint8_t)side;
    c->revealed=(uint8_t)(rev?1:0); c->flags=0;
}
int main(void){
    jq_state st; memset(&st,0,sizeof(st)); jq_state_init(&st);
    st.mode=JQ_MODE_FLIP; st.stage=STG_PLAY; st.winner=JQ_WIN_NONE; st.turn=SIDE_BLUE;
    /* 暗棋布局：蓝翻出师长 @(9,4)，红半区仍有未翻暗子（0xFF） */
    put(&st,9,4,4,SIDE_BLUE,1);          /* 蓝已翻师长 */
    put(&st,10,0,7,SIDE_UNKNOWN,0);      /* 未翻暗子 @50 */
    put(&st,10,1,3,SIDE_UNKNOWN,0);      /* 未翻暗子 @51 */
    /* 蓝师长走空格 9→8（上移），收口后不应 Draw（有 0xFF 潜在普通子） */
    int f=cell_index(9,4), t=cell_index(8,4);
    int cm=jq_can_move(&st,f,t), ev=jq_try_move(&st,f,t);
    printf("暗棋师长 49→44: can=%d ev=%d stage=%d winner=%d (期望 stage=PLAY winner=NONE 不Draw)\n",
           cm, ev, st.stage, st.winner);
    int ok = (st.stage==STG_PLAY && st.winner==JQ_WIN_NONE);
    printf("flip_draw_guard: %s\n", ok?"PASS":"FAIL");
    return ok?0:1;
}
