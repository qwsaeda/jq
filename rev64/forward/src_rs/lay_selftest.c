/*
 * lay_selftest.c — 12 预设布局加载 + 校验自测
 * 验证 dist/layouts/00-11.lay（SPAK data 组 idx 47-58 提取，00.lay=default 已证）
 *   全部可 jq_load_layout + jq_validate_layout 通过。
 * 编译: make laytest  →  dist/jq_laytest
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

#ifndef LAY_DIR
#define LAY_DIR "."
#endif

int main(void) {
    int ok = 0;
    printf("== 12 预设布局加载 + 校验自测（LAY_DIR=%s）==\n", LAY_DIR);
    for (int i = 0; i < 12; i++) {
        char path[512];
        snprintf(path, sizeof path, LAY_DIR "/%02d.lay", i);
        FILE *f = fopen(path, "rb");
        if (!f) { printf("  layout %02d: 打不开 %s\n", i, path); continue; }
        uint8_t lb[30];
        int rd = fread(lb, 1, 30, f);
        fclose(f);
        if (rd != 30) { printf("  layout %02d: 读取 %d 字节（应 30）\n", i, rd); continue; }
        jq_state st;
        jq_state_init(&st);
        jq_load_layout(&st, lb);
        jq_layout_err e = jq_validate_layout(&st);
        int pcs = 0;
        for (int c = 0; c < N_CELLS; c++) if (st.cells[c].rank) pcs++;
        const char *state = (e == LAYOUT_OK) ? "OK" : "ERR";
        printf("  layout %02d: validate=%s 棋子=%d\n", i, state, pcs);
        if (e == LAYOUT_OK && pcs > 0) ok++;
    }
    printf(ok == 12 ? "== 12/12 布局合法 ==\n" : "== 有布局校验失败 ==\n");
    return ok != 12;
}
