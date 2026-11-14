/* flip_rail_test.c — 暗棋翻出的子能否沿铁路滑行（开阔棋盘）
 * 翻出师长 @(5,2)（中线横铁×纵铁交点），周围全空，枚举可达格。
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
    int from = cell_index(5, 2);
    put(&st, 5, 2, 4, SIDE_BLUE, 1);   /* 已翻师长 @(5,2) */
    st.turn = SIDE_BLUE;
    int n = 0;
    for (int t = 0; t < N_CELLS; t++) {
        if (t != from && jq_can_move_any(&st, from, t)) {
            printf("  -> %d (r%d c%d) rank=%d\n", t, cell_row(t), cell_col(t), st.cells[t].rank);
            n++;
        }
    }
    printf("暗棋翻出师长 @(5,2) 开阔棋盘 可达 = %d 格\n", n);
    return 0;
}
