/* dark_fullgame_statemachine_test.c — 暗棋翻子定色→全定色→走子阶段 状态机转换行为级实证
 * 补全现有缺口：§57 dark_flip_color_test 实证"全定色中间态（未翻子=0）"，§59
 * dark_colorwalk_test 实证"全定色后走子比较"，但**"翻子定色阶段→全定色→走子阶段"
 * 的状态机转换触发点**（全定色后 can_flip 全 0 无子可翻 + can_move 走子仍放行，
 * 同一 STG_PLAY 内行为交替，非独立 stage 切换）未独立断言。
 *
 * 对照 64 位实现（rules.c 暗棋 stage 模型 = SETUP/PLAY/OVER 三段，翻子/定色/走子
 * 全程 STG_PLAY，无"定色阶段→走子阶段"独立 stage 转换）：
 *   · 定色阶段：0xFF 未翻子存在时 jq_can_flip=1（可翻，定色进行中）；
 *   · 全定色触发点：0xFF 全翻后 jq_can_flip 全 0（无子可翻，定色阶段结束）；
 *   · 走子阶段：全定色后 jq_can_move 走子仍放行（走子比较按定色 side，§59 既有），
 *     定色/走子在**同一 STG_PLAY** 内行为交替（非 stage 切换）；
 *   · 定色/走子并行：有 0xFF 未翻子时定色与走子**并行**（can_flip=1 + can_move=1），
 *     非"定色完才能走子"的串行门控。
 * 64 位行为级实证（走已实现的 jq_can_flip/jq_can_move/jq_try_move + STG_PLAY 全程
 * stage 路径，0 分叉）。
 *
 * 几何口径：col4 纵线 44(蓝)↔39(红) 相邻；cell 9 0xFF 未翻子（对照）；红 rank2@50
 *   防困毙；cell 索引 row*5+col。
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
    /* ① 全定色后 can_flip 全 0（无子可翻，定色阶段结束）+ stage 仍 STG_PLAY */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int any_flip = 0;
        for (int i = 0; i < N_CELLS; i++) if (jq_can_flip(&st, i)) any_flip = 1;
        ok &= chk("① 全定色后 can_flip 全0(无子可翻定色结束)+stage仍PLAY",
                  any_flip == 0 && st.stage == STG_PLAY);
    }
    /* ② 全定色后走子放行（can_move=1，走子阶段行为开始）*/
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int can_walk = jq_can_move(&st, 44, 39);
        ok &= chk("② 全定色后走子放行(can_move=1走子阶段开始)",
                  can_walk == 1 && st.stage == STG_PLAY);
    }
    /* ③ 走子后 stage 仍 STG_PLAY（无独立"定色→走子"stage 切换，同一 PLAY 内交替）*/
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE; st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;  st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;  st.cells[50].revealed = 1;
        int ev = jq_try_move(&st, 44, 39);
        ok &= chk("③ 走子后 stage仍PLAY(无独立stage切换同一PLAY内交替)",
                  st.stage == STG_PLAY && ev >= 0);
    }
    /* ④ 定色/走子并行（有 0xFF 未翻子时 can_flip=1 + can_move=1 并行，非串行门控）*/
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[9].rank = 3;  st.cells[9].side = SIDE_UNKNOWN; st.cells[9].revealed = 0;
        st.cells[44].rank = 1; st.cells[44].side = SIDE_BLUE;   st.cells[44].revealed = 1;
        st.cells[39].rank = 9; st.cells[39].side = SIDE_RED;    st.cells[39].revealed = 1;
        st.cells[50].rank = 2; st.cells[50].side = SIDE_RED;    st.cells[50].revealed = 1;
        int can_flip_mixed = jq_can_flip(&st, 9);
        int can_walk_mixed = jq_can_move(&st, 44, 39);
        ok &= chk("④ 定色/走子并行(有0xFF未翻子can_flip=1+can_move=1非串行门控)",
                  can_flip_mixed == 1 && can_walk_mixed == 1);
    }
    printf("dark_fullgame_statemachine_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
