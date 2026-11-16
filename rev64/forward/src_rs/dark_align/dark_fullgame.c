#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
extern const uint8_t JQ_DEFAULT_LAY[30];

/* 简单 AI：优先走子（吃子/滑行），否则翻子。统计可达格分布。 */
int main(void){
    jq_state st;
    jq_dark_setup(&st, JQ_DEFAULT_LAY);
    st.stage = STG_PLAY;
    int min_move_range = 999, max_move_range = 0, move_ok=0;
    int flips=0, moves=0, turns=0;
    while (st.stage == STG_PLAY && turns < 400) {
        turns++;
        int acted = 0;
        /* 先尝试走子（已翻的本方子） */
        for (int i = 0; i < N_CELLS && !acted; i++) {
            jq_cell *c=&st.cells[i];
            if (c->rank==RANK_EMPTY || !c->revealed || c->side!=st.turn) continue;
            /* 统计该子可达格数 */
            int rng=0;
            for (int j=0;j<N_CELLS;j++) if (i!=j && jq_can_move(&st,i,j)) rng++;
            if (rng>0){ if(rng<min_move_range)min_move_range=rng; if(rng>max_move_range)max_move_range=rng; }
            for (int j = 0; j < N_CELLS && !acted; j++) {
                if (i != j && jq_can_move(&st, i, j)) {
                    int ev = jq_try_move(&st, i, j);
                    if (ev >= 0) { acted=1; moves++; move_ok++; }
                }
            }
        }
        /* 否则翻子 */
        if (!acted) {
            for (int i = 0; i < N_CELLS && !acted; i++) {
                if (jq_can_flip(&st, i)) {
                    int ev = jq_flip(&st, i);
                    if (ev >= 0) { acted=1; flips++; }
                }
            }
        }
        if (!acted) break;   /* 无行动 */
    }
    printf("暗棋对局: turns=%d moves=%d flips=%d stage=%d winner=%d\n",
           turns, moves, flips, st.stage, st.winner);
    printf("走子可达格范围: min=%d max=%d\n", min_move_range, max_move_range);
    /* 检查每个已翻子的可达格（非0才算正常棋子可动） */
    printf("dark_fullgame: %s\n",
           (st.stage==STG_OVER || turns<400) ? "终局/正常结束" : "超时");
    
    /* 对局必须正常推进并终局（不超时）+ 双方均有走子/翻棋动作（覆盖退化） */
    chk("① 暗棋对局在 400 回合内结束（不超时）", turns < 400);
    chk("② 对局含走子与翻棋动作（不退化）", moves > 0 && flips > 0);
    /* ③ 终局状态合法（OVER 或 步数上限） */
    chk("③ 终局状态合法（stage=OVER 或 回合达上限）",
        st.stage == 2 || turns >= 400);
    return g_ok ? 0 : 1;

}
