/* 针对性测试：先手翻子定色下 蓝首翻→红方不困毙→红翻→蓝方不困毙 */
#include <stdio.h>
#include "junqi.h"
extern const uint8_t JQ_DEFAULT_LAY[30];

int main(void) {
    jq_state st;
    jq_dark_setup(&st, JQ_DEFAULT_LAY);
    st.stage = STG_PLAY;
    /* 找第一个未翻 0xFF 子给蓝翻 */
    int b = -1;
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank != RANK_EMPTY && !st.cells[i].revealed && st.cells[i].side == SIDE_UNKNOWN) { b = i; break; }
    printf("蓝首翻 idx=%d can_flip=%d\n", b, jq_can_flip(&st, b));
    int f1 = jq_flip(&st, b);
    printf("蓝翻 f1=%d side=%d turn=%d 红方困毙=%d（期望 0=未困毙）\n",
           f1, st.cells[b].side, st.turn, jq_side_stalemated(&st, SIDE_RED));
    /* 红翻一个 0xFF 子（跳过军旗——军旗不可翻是正确规则） */
    int r = -1;
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank != RANK_EMPTY && !st.cells[i].revealed && st.cells[i].side == SIDE_UNKNOWN &&
            st.cells[i].rank != RANK_FLAG) { r = i; break; }
    printf("红翻 idx=%d can_flip=%d\n", r, jq_can_flip(&st, r));
    int f2 = jq_flip(&st, r);
    printf("红翻 f2=%d side=%d turn=%d 蓝方困毙=%d（期望 0=未困毙）\n",
           f2, st.cells[r].side, st.turn, jq_side_stalemated(&st, SIDE_BLUE));
    int ok = (f1 == 1 && st.cells[b].side == SIDE_BLUE &&
              jq_side_stalemated(&st, SIDE_RED) == 0 &&
              f2 == 1 && st.cells[r].side == SIDE_RED &&
              jq_side_stalemated(&st, SIDE_BLUE) == 0);
    printf("dark_flip_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
