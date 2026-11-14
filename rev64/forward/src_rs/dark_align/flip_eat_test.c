/* flip_eat_test.c — 翻出子能否吃到未翻暗子格（0xFF）
 * 翻出师长 @(5,2)，相邻 (5,1) 是未翻暗子（rank3, 0xFF）。
 * 64 位 B01-053 应拦（can_move=0）；原版/32 位翻译体应允许（can_move=1）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static void put(jq_state *st, int row, int col, int rank, int side, int rev)
{
    jq_cell *c = &st->cells[cell_index(row, col)];
    c->rank = (uint8_t)rank; c->side = (uint8_t)side;
    c->revealed = (uint8_t)(rev ? 1 : 0); c->flags = 0;
}

int main(void)
{
    jq_state st;
    memset(&st, 0, sizeof(st));
    jq_state_init(&st);
    st.mode = JQ_MODE_FLIP;
    put(&st, 5, 2, 4, SIDE_BLUE, 1);       /* 已翻师长 */
    put(&st, 5, 1, 3, SIDE_UNKNOWN, 0);    /* 未翻暗子 rank3 */
    st.turn = SIDE_BLUE;
    int f = cell_index(5, 2), t = cell_index(5, 1);
    printf("暗棋翻出师长(4) -> 未翻暗子(3,0xFF) 格: can_move=%d\n",
           jq_can_move(&st, f, t));
    printf("  (1=允许吃未翻暗子=原版行为; 0=拦=64位B01-053)\n");
    return 0;
}
