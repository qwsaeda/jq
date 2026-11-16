/* flip_eat_test.c — 翻出子能否吃到未翻暗子格（0xFF）
 * 翻出师长 @(5,2)，相邻 (5,1) 是未翻暗子（rank3, 0xFF）。
 * 64 位 B01-053 应拦（can_move=0）；原版/32 位翻译体应允许（can_move=1）。
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
    put(&st, 5, 2, 4, SIDE_BLUE, 1);       /* 已翻师长 */
    put(&st, 5, 1, 3, SIDE_UNKNOWN, 0);    /* 未翻暗子 rank3 */
    st.turn = SIDE_BLUE;
    int f = cell_index(5, 2), t = cell_index(5, 1);
    printf("暗棋翻出师长(4) -> 未翻暗子(3,0xFF) 格: can_move=%d\n",
           jq_can_move(&st, f, t));
    printf("  (1=允许吃未翻暗子=原版行为; 0=拦=64位B01-053)\n");
    
    /* ★ 暗棋：翻出的子能否吃到未翻暗子格（0xFF）——Round 1 对抗审计 R-011 口径项：
       本用例原注释「0=拦=64位B01-053」与实现不符（实测 can_move=1 允许），
       且 §101 双端探针 G1/G2 实测 32 位翻译体同为 1（32↔64 一致，非分叉）。
       口径：R-011「暗棋下引擎允许未定色暗子被吃」= 暗棋信息泄露缺口，**待原版
       .text 定案**（正向暂按现状断言 + 诚实标注，禁止把"允许"包装成"已对齐"）。 */
    chk("① 翻出师长可吃未翻暗子格（现状=允许，R-011 待原版定案，非已对齐结论）",
        jq_can_move(&st, f, t) == 1);
    return g_ok ? 0 : 1;

}
