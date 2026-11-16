/* flip_rail_test.c — 暗棋翻出的子能否沿铁路滑行（开阔棋盘）
 * 翻出师长 @(5,2)（中线横铁×纵铁交点），周围全空，枚举可达格。
 */
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
    
    /* 暗棋翻出师长 @(5,2)（横贯×纵贯交叉格）开阔棋盘可达格：
       结构断言（不冻结可疑数字）——横向 row5 全宽端点 (5,0)/(5,4) 可达 +
       纵向 col2 同段 (4,2)/(6,2)/(7,2) 可达 + row8 行营隔断 (8,2) 不可达
       （§92 铁路拓扑 + §95/§96 工兵拐弯口径）。 */
    {
        int r50 = 0, r54 = 0, r42 = 0, r62 = 0, r72 = 0, r82 = 0;
        for (int t = 0; t < N_CELLS; t++) {
            if (t == from || !jq_can_move_any(&st, from, t)) continue;
            if (t == cell_index(5, 0)) r50 = 1;
            if (t == cell_index(5, 4)) r54 = 1;
            if (t == cell_index(4, 2)) r42 = 1;
            if (t == cell_index(6, 2)) r62 = 1;
            if (t == cell_index(7, 2)) r72 = 1;
            if (t == cell_index(8, 2)) r82 = 1;
        }
        chk("① 横向 row5 全宽端点 (5,0)/(5,4) 可达", r50 && r54);
        chk("② 纵向 col2 同段 (4,2)/(6,2)/(7,2) 可达", r42 && r62 && r72);
        chk("③ row8 行营隔断：(8,2) 不可达", !r82);
        chk("④ 可达格数 >= 7（横4+纵3 最小集）", n >= 7);
    }
    return g_ok ? 0 : 1;

}
