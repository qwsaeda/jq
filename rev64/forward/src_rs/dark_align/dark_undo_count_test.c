/* dark_undo_count_test.c — 悔棋（jq_undo_restore）快照回退计数行为级实证
 * 补全现有缺口：现有用例未覆盖"悔棋回退 moves_since_eat/total_moves +
 * 撤销和棋判定 + mines_cleared 下界钳制"路径。
 *
 * 对照 64 位实现（rules.c jq_undo_restore §1032，*st=*snap 全字段覆盖 +
 * B01-008 P0 mines_cleared 钳 0）+ §523-524 既有口径（"悔棋快照回退可回退
 * moves_since_eat，回退后 70 步进度丢失，悔棋后和棋进度重置合理"）：
 *   ① 计数回退：走子累加 moves_since_eat/total_moves 后回退 → 全部回滚到快照；
 *   ② mines_cleared 下界钳制：损坏快照注入 -1 → 回退后钳 0（B01-008 P0 已修）；
 *   ③ 和棋进度重置：走子 69→70 触发 jq_on_draw（stage=OVER winner=DRAW）→
 *      回退到 69 快照 → 撤销和棋（stage=PLAY winner=NONE moves_since_eat=69）。
 * 64 位行为级实证（走已实现的 jq_undo_restore 路径，0 分叉）。
 *
 * 几何口径：col4 纵线 44(蓝 rank1)→39(空) 空移（§3 既有合法几何）。
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
    /* ① 计数回退：开局快照 → 走子累加（moves_since_eat/total_moves++）→ 回退 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        jq_state snap = st;                     /* 开局快照 */
        int ev = jq_try_move(&st, 44, 39);      /* 空移累加计数 */
        int m1 = st.moves_since_eat, t1 = st.total_moves;
        jq_undo_restore(&st, &snap);            /* 回退到快照 */
        ok &= chk("① 计数回退(走子累加后回退→全回滚到快照)",
                  ev == EV_MOVE && m1 == 1 && t1 == 1 &&
                  st.moves_since_eat == 0 && st.total_moves == 0
                  && st.cells[44].rank == 1 && st.cells[39].rank == RANK_EMPTY
                  && st.turn == SIDE_BLUE);
    }
    /* ② mines_cleared 下界钳制：损坏快照注入 -1 → 回退后钳 0（§B01-008 P0） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        jq_state snap = st;
        snap.mines_cleared[SIDE_BLUE] = -1;     /* 损坏快照注入负值 */
        jq_undo_restore(&st, &snap);
        ok &= chk("② mines_cleared 下界钳制(损坏快照-1回退后钳0)",
                  st.mines_cleared[SIDE_BLUE] == 0);
    }
    /* ③ 和棋进度重置：走子 69→70 触发和棋 → 回退到 69 快照 → 撤销和棋 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.moves_since_eat = 69;
        jq_state snap = st;                     /* 69 步快照 */
        int ev = jq_try_move(&st, 44, 39);      /* 69→70 触发 jq_on_draw */
        int drawn = (st.stage == STG_OVER && st.winner == JQ_WIN_DRAW);
        jq_undo_restore(&st, &snap);            /* 回退撤销和棋 */
        ok &= chk("③ 和棋进度重置(70步判和后回退→撤销和棋回PLAY)",
                  ev == EV_MOVE && drawn &&
                  st.stage == STG_PLAY && st.winner == JQ_WIN_NONE
                  && st.moves_since_eat == 69);
    }
    printf("dark_undo_count_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
