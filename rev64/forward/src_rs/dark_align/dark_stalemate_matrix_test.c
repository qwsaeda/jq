/* dark_stalemate_matrix_test.c — 困毙（stalemate）扫描白名单差异 + 0xFF 归属口径行为级实证
 * 补全现有缺口：dark_win_test / dark_flip_test / draw_exist_test 覆盖部分困毙
 * （单侧困毙→对方胜），但**jq_side_stalemated 困毙扫描白名单差异（可翻扫描
 * 排雷不排旗 vs 可动扫描排雷+旗）+ 0xFF 未定色子归属翻子方口径**未独立用例
 * （§B01-011 修复"0xFF 双方可翻"精确化 = "0xFF 子对翻子方计可翻"）。
 *
 * 对照 64 位实现（rules.c jq_side_stalemated §302f0 反汇编精确对齐 + §B01-011
 * 0xFF 归属修复）：
 *   · 可翻扫描（0x30320）：未翻 + **仅排地雷**（军旗计入可翻）+ (0xFF||side)；
 *   · 可动扫描（0x30348）：rank_norm==FLAG || ==MINE → continue（军旗+地雷均不
 *     计可动）；
 *   · 两 API 白名单差异 = 原版设计（翻=动作合法, 困毙可行动=翻完须能走）；
 *   · 0xFF 未定色子：对**翻子方**计可翻（翻归翻子方定色），非"任意方 0xFF 子
 *     所有方可翻"——红方 0xFF 子只有红方翻，蓝方 0xFF 子只有蓝方翻。
 * 64 位行为级实证（走已实现的 jq_side_stalemated 扫描路径，0 分叉）。
 *
 * 几何口径：cell50(row10col0) 红方格；cell 索引 row*5+col。
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
    /* ① 仅剩未翻地雷 → 困毙（可翻扫描仅排地雷，翻雷不算可行动） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[50].rank = RANK_MINE; st.cells[50].side = SIDE_RED; st.cells[50].revealed = 0;
        ok &= chk("① 仅剩未翻地雷→困毙(可翻扫描排雷)",
                  jq_side_stalemated(&st, SIDE_RED), 1);
    }
    /* ② 仅剩未翻军旗 → 不困毙（可翻扫描不排旗，军旗可翻） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[50].rank = RANK_FLAG; st.cells[50].side = SIDE_RED; st.cells[50].revealed = 0;
        ok &= chk("② 仅剩未翻军旗→不困毙(可翻扫描不排旗,军旗可翻)",
                  jq_side_stalemated(&st, SIDE_RED), 0);
    }
    /* ③ 0xFF 归属翻子方：红方 0xFF 子红方可翻(不困毙), 蓝方无己方子→蓝困毙 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[50].rank = 5; st.cells[50].side = SIDE_UNKNOWN; st.cells[50].revealed = 0;
        ok &= chk("③ 红方0xFF子: 红不困毙(翻子方可翻)",
                  jq_side_stalemated(&st, SIDE_RED), 0);
        ok &= chk("③ 红方0xFF子: 蓝困毙(蓝无己方子,红0xFF蓝翻不了)",
                  jq_side_stalemated(&st, SIDE_BLUE), 1);
    }
    /* ④ 蓝方 0xFF 子对称：蓝不困毙, 红困毙 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[9].rank = 3; st.cells[9].side = SIDE_UNKNOWN; st.cells[9].revealed = 0;
        ok &= chk("④ 蓝方0xFF子: 蓝不困毙(翻子方可翻)",
                  jq_side_stalemated(&st, SIDE_BLUE), 0);
        ok &= chk("④ 蓝方0xFF子: 红困毙(红无己方子,蓝0xFF红翻不了)",
                  jq_side_stalemated(&st, SIDE_RED), 1);
    }
    /* ⑤ 无子可走/可翻 → 困毙（对照，双方无子） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        ok &= chk("⑤ 无子: 红困毙", jq_side_stalemated(&st, SIDE_RED), 1);
    }
    /* ⑥ 有已翻普通子可动 → 不困毙（对照 ①⑤） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_FLIP;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_RED;
        st.cells[50].rank = 3; st.cells[50].side = SIDE_RED; st.cells[50].revealed = 1;
        st.cells[55].rank = 0;   /* 55 相邻空格（可走） */
        ok &= chk("⑥ 有已翻普通子可动→不困毙", jq_side_stalemated(&st, SIDE_RED), 0);
    }
    printf("dark_stalemate_matrix_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
