/* dark_flip_draw_count_test.c — 暗棋（ruleflip）和棋计数行为级实证
 * 补全现有缺口：rule_text_conformance 70/1000 步和棋用例全走明棋 LAY，
 * 暗棋 FLIP 下"翻棋不消耗和棋进度"（§910-922 原版 0x302f0 实证修复）
 * 无独立验证用例。
 *
 * 对照原版 0x302f0 实证（rules.c §910-922 修复口径）：
 *   70 步扫描（0x3036a-0x3039e）只读 6B 走子记录（state+0x6f4 区），
 *   翻棋执行区（0x39f20）**不写**该记录 → "连续 70 步无吃子"的"步"= 走子，
 *   翻牌非走子。64 位 jq_flip 不累加 moves_since_eat/total_moves，行为级对齐。
 *
 * 实证两场景：
 *   ① 暗棋连续翻棋 N 次 → moves_since_eat/total_moves 恒 0（翻棋不计数）→ 不判和；
 *   ② 暗棋走子累加 moves_since_eat → 70 步无吃子判和（jq_on_draw winner=DRAW）。
 * 64 位行为级实证（走已实现的 jq_flip 不计数 + jq_try_move 走子计数路径，0 分叉）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 暗棋连续翻棋不消耗和棋进度：翻多次后 moves_since_eat/total_moves 恒 0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        /* 放 25 个 0xFF 未翻子（蓝翻后归蓝，§30/§31 翻归翻子方口径） */
        int i;
        for (i = 0; i < 25; i++) {
            st.cells[i].rank = (i % 5) + 1; st.cells[i].side = SIDE_UNKNOWN;
            st.cells[i].revealed = 0;   /* 0xFF 未翻，蓝可翻 */
        }
        int flips = 0;
        for (i = 0; i < N_CELLS && flips < 25; i++) {
            if (st.stage != STG_PLAY) break;
            if (jq_can_flip(&st, i)) { jq_flip(&st, i); flips++; }
        }
        ok &= chk("① 暗棋翻棋不消耗和棋进度(moves_since_eat/total_moves 恒0 不判和)",
                  flips > 0 && st.moves_since_eat == 0 && st.total_moves == 0
                  && st.stage == STG_PLAY && st.winner == JQ_WIN_NONE);
        (void)flips;
    }
    /* ② 暗棋走子累加 moves_since_eat → 70 步无吃子判和 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 5; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 0;   /* 44→39 空移合法（col4 纵线相邻） */
        st.moves_since_eat = 69;   /* 累计 69 步无吃子，走第 70 步 */
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("② 暗棋走子累加 moves_since_eat→70 步判和(winner=DRAW)",
                  ev == EV_MOVE && st.moves_since_eat == 70
                  && st.stage == STG_OVER && st.winner == JQ_WIN_DRAW);
    }
    printf("dark_flip_draw_count_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
