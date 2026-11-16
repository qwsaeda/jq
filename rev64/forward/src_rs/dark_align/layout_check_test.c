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
    
    /* ① 默认布局 60 格全合规（违例=0） */
    chk("① 默认布局 60 格校验违例=0", bad == 0);
    /* ② 炸弹放红前场 row6(idx30) → ERR_ZHA(1) */
    chk("② 炸弹 idx30(row6) → ERR_ZHA(1)", jq_check_layout_cell(30, RANK_BOMB) == ERR_ZHA);
    /* ③ 地雷放 row9(idx45) → ERR_LEI(2)（雷仅 row10-11） */
    chk("③ 地雷 idx45(row9) → ERR_LEI(2)", jq_check_layout_cell(45, RANK_MINE) == ERR_LEI);
    /* ④ 军旗放 idx57（非 56/58）→ ERR_QI(3) */
    chk("④ 军旗 idx57 → ERR_QI(3)", jq_check_layout_cell(57, RANK_FLAG) == ERR_QI);
    /* ⑤ 军旗放 idx58（大本营）→ LAYOUT_OK(0) */
    chk("⑤ 军旗 idx58 → LAYOUT_OK(0)", jq_check_layout_cell(58, RANK_FLAG) == LAYOUT_OK);
    return g_ok ? 0 : 1;

}
