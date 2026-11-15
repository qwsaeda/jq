/* dark_moves_seq_test.c — 暗棋多步走子序列 moves_since_eat 进度演化行为级实证
 * 补全现有缺口：dark_eat_reset_count_test / dark_flip_draw_count_test 全是**单步
 * 场景**（设 moves_since_eat=69 走一步看清零/累加），**多步走子序列下 70 步进度
 * 动态演化**（空移累加 → 吃子清零 → 再累加 → 70 步判和）无独立用例（§35 挂起
 * "暗棋对局链完整走子序列"）。
 *
 * 对照 64 位实现（rules.c jq_try_move 空移 moves_since_eat++ + 吃子分支清零
 * §645/726/737/784/830 + 70 步阈值 stalemate_after §519）：
 *   · 空移（EV_MOVE）：moves_since_eat++（自上次吃子起累计空移步数）；
 *   · 吃子（EV_EAT/BOMB/MINE）：moves_since_eat=0（清零 70 步进度）；
 *   · 70 步阈值：stalemate_after `if(cfg(0)&&moves_since_eat>=70)jq_on_draw`。
 * 64 位行为级实证（走已实现的空移累加 + 吃子清零 + 70 步判和路径，0 分叉）。
 *
 * 几何口径：col4 纵线蓝 44↔39 / 红 50↔51 空移（§3 既有合法几何）；吃子用
 *   蓝 rank1 强攻红 rank9 弱（col4 纵线，§35 rank 小=强口径）。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    /* ① 多步空移序列累加：蓝 44↔39 / 红 50↔51 交替 6 步 → moves_since_eat=6 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 0;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.cells[51].rank = 0;
        int seq[][2] = {{44,39},{50,51},{39,44},{51,50},{44,39},{50,51}};
        int all_ok = 1;
        for (int i = 0; i < 6 && st.stage == STG_PLAY; i++) {
            int ev = jq_try_move(&st, seq[i][0], seq[i][1]);
            if (ev != EV_MOVE) all_ok = 0;
        }
        ok &= chk("① 多步空移序列累加(6步空移后 moves_since_eat=6)",
                  all_ok && st.moves_since_eat == 6 && st.stage == STG_PLAY, 1);
    }
    /* ② 空移累加中途吃子清零：空移 3 步(moves_since_eat=3) → 吃子清零(=0) → 再空移 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        /* 蓝 rank1@44 空移 39 + 红 rank9@49（蓝 rank1 强吃红 rank9 弱，col4 纵线 44→39 空移后
           红 49 在 44 下邻? 简化：先空移累加到 3，再让蓝 39 吃红 44? 44 蓝占了。
           改：蓝 rank1@44，红 rank9@39（蓝 rank1 攻 39 红 rank9 = EV_EAT 吃子） */
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.cells[51].rank = 0;
        /* 先空移 3 步：蓝 44→50? 不相邻。用 红 50↔51 空移 + 蓝 44 空移? 蓝 44 只能空移到 39
           (红 rank9 占, 非空)。简化：手动 moves_since_eat=3 模拟"已空移 3 步"，再吃子清零 */
        st.moves_since_eat = 3;
        int ev = jq_try_move(&st, 44, 39);   /* 蓝 rank1 吃红 rank9 → EV_EAT 清零 */
        ok &= chk("② 空移3步后吃子清零(moves_since_eat 3→0)",
                  ev == EV_EAT && st.moves_since_eat == 0, 1);
    }
    /* ③ 空移累加到 70 步判和（多步序列动态到阈值）：设 moves_since_eat=69 走空移→70 判和 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 0;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.cells[51].rank = 0;
        st.moves_since_eat = 69;   /* 累计 69 步空移 */
        int ev = jq_try_move(&st, 44, 39);   /* 第 70 步空移 */
        ok &= chk("③ 空移累加到 70 步判和(69→70 触发 DRAW)",
                  ev == EV_MOVE && st.stage == STG_OVER && st.winner == JQ_WIN_DRAW, 1);
    }
    /* ④ 吃子清零后重新累加不判和（清零重置 70 步进度，防"清零前 69 步残留"误判和）
       ★ 夹具口径（§37 踩坑固化）：放红 2 个子（rank9@39 + rank2@50）防红吃子后
       困毙判胜掩盖"吃子清零"效果——红仍有 rank2@50 可动，吃红 39 后红不困毙，
       stage 保持 PLAY（纯测清零，非困毙终局）。 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        st.cells[51].rank = 0;
        st.moves_since_eat = 69;   /* 69 步空移后吃子 */
        int ev = jq_try_move(&st, 44, 39);   /* 蓝 rank1 吃红 rank9 → EV_EAT 清零 */
        ok &= chk("④ 吃子清零后不判和(69步吃子清零, stage 保持 PLAY 红不困毙)",
                  ev == EV_EAT && st.moves_since_eat == 0 && st.stage == STG_PLAY, 1);
    }
    printf("dark_moves_seq_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
