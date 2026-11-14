#include <stdio.h>
#include <string.h>
#include "junqi.h"
/* 布局限制实证（0x2a228）：默认布局全 60 格合规 + 违例格被拒 */
int main(void){
    /* ① 默认布局：全 60 格 jq_check_layout_cell 应全 LAYOUT_OK（蓝半区无条件/红半区雷row10-11/旗56,58） */
    jq_state st; jq_load_layout(&st, JQ_DEFAULT_LAY);
    int bad=0;
    for(int i=0;i<N_CELLS;i++){
        /* 用装载后 cell 的 raw 编码（蓝方原值/红方+0xc）复算校验 */
        int raw = st.cells[i].rank;   /* 装载已按 side 写 rank（红+0xc），直接校验 */
        jq_layout_err e = jq_check_layout_cell(i, raw);
        if (e != LAYOUT_OK && st.cells[i].rank != RANK_EMPTY){
            printf("  违例 cell%d raw=%d → %d\n", i, raw, e); bad++;
        }
    }
    printf("① 默认布局 60 格校验：违例=%d（期望 0，全合规）\n", bad);
    /* ② 炸弹放红前场 row6（idx30）→ ERR_ZHA */
    printf("② 炸弹 idx30(row6) = %d（期望 ERR_ZHA=%d）\n",
        jq_check_layout_cell(30, RANK_BOMB), ERR_ZHA);
    /* ③ 地雷放 row9（idx45）→ ERR_LEI（雷仅 row10-11=idx50-59） */
    printf("③ 地雷 idx45(row9) = %d（期望 ERR_LEI=%d）\n",
        jq_check_layout_cell(45, RANK_MINE), ERR_LEI);
    /* ④ 军旗放 idx57（非 56/58）→ ERR_QI */
    printf("④ 军旗 idx57 = %d（期望 ERR_QI=%d）\n",
        jq_check_layout_cell(57, RANK_FLAG), ERR_QI);
    /* ⑤ 军旗放 idx58（大本营）→ LAYOUT_OK */
    printf("⑤ 军旗 idx58 = %d（期望 LAYOUT_OK=%d）\n",
        jq_check_layout_cell(58, RANK_FLAG), LAYOUT_OK);
    return 0;
}
