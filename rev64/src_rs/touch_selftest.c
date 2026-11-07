/*
 * touch_selftest.c — 触摸命中 + 两步走子态机自测（桌面，无 JNI）
 * 验证（touch.c 命中几何 + touch_state.c 态机，对齐原版 0xbdd2c/0x7bf80 行为）：
 *   1. 格中心坐标 → 命中正确格（jq_touch_hit_cell）
 *   2. 越界坐标 → 空白（cell<0）
 *   3. 两步态机：IDLE→点A(SEL)→点B(MOVE A→B)→IDLE
 *   4. 点 A 自身 → MOVE(to==from，翻棋意图)
 *   5. 选中态点空白 → 取消(UNSELECT)
 *   6. targets：选中格的可走目标数（≥1 或含自身翻棋）
 * 编译: make touchtest → dist/jq_touchtest
 */
#include <stdio.h>
#include "junqi.h"
#include "touch.h"
#include "touch_state.h"

static int rc = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); rc = 1; } \
    else printf("  ok: %s\n", msg); \
} while (0)

int main(void) {
    const int W = 720, H = 1280;  /* 渲染默认视口 */
    jq_state st;
    jq_state_init(&st);
    jq_load_layout(&st, JQ_DEFAULT_LAY);
    jq_touch_state ts;
    jq_touch_state_init(&ts);

    printf("== 触摸命中 + 两步态机自测 ==\n");

    /* 1. 找一颗有子的格，算其屏幕中心坐标，命中应返回该格 */
    int A = -1;
    for (int i = 0; i < N_CELLS; i++)
        if (st.cells[i].rank) { A = i; break; }
    CHECK(A >= 0, "默认布局有棋子格");
    int ac = cell_col(A), ar = cell_row(A);
    /* ★ 层 2：命中几何与 touch.c 同源（datax_177 BG_GRID，格中心=bg_cell_img_center，
       视图坐标 = 图像像素 / BG_IMG * view，顶左原点 y 下） */
    float cx, cy; bg_cell_img_center(ar, ac, &cx, &cy);
    cx = cx / BG_IMG_W * W; cy = cy / BG_IMG_H * H;
    int hit = jq_touch_hit_cell(cx, cy, W, H, NULL, NULL);
    CHECK(hit == A, "格中心命中 = 原格");

    /* 2. 越界（负坐标 / 超视口）→ 空白 */
    CHECK(jq_touch_hit_cell(-5, -5, W, H, NULL, NULL) < 0, "负坐标命中空白");
    CHECK(jq_touch_hit_cell(W + 50, H + 50, W, H, NULL, NULL) < 0, "超视口命中空白");

    /* 3. 两步态机：IDLE → 点A(SEL) → 点B(MOVE) → IDLE */
    int from = -1, to = -1;
    int ev1 = jq_touch_state_process(&ts, cx, cy, W, H, &from, &to);
    CHECK(ev1 == JQ_EV_SELECT && from == A, "点A → SELECT 事件 + from=A");
    int B = -1;
    for (int i = 0; i < N_CELLS; i++)
        if (i != A && st.cells[i].rank) { B = i; break; }
    float bx, by; bg_cell_img_center(cell_row(B), cell_col(B), &bx, &by);
    bx = bx / BG_IMG_W * W; by = by / BG_IMG_H * H;
    int f2 = -1, t2 = -1;
    int ev2 = jq_touch_state_process(&ts, bx, by, W, H, &f2, &t2);
    CHECK(ev2 == JQ_EV_MOVE && f2 == A && t2 == B, "点B → MOVE(A→B)");
    CHECK(ts.mode == JQ_TS_IDLE, "走子后回 IDLE");

    /* 4. 点 A 自身 → 翻棋意图（MOVE to==from） */
    int f3 = -1, t3 = -1;
    int ev3 = jq_touch_state_process(&ts, cx, cy, W, H, &f3, &t3);  /* IDLE→SEL */
    CHECK(ev3 == JQ_EV_SELECT, "IDLE 点A → SELECT");
    int f4 = -1, t4 = -1;
    int ev4 = jq_touch_state_process(&ts, cx, cy, W, H, &f4, &t4);  /* 再点A */
    CHECK(ev4 == JQ_EV_MOVE && f4 == A && t4 == A, "再点A自身 → MOVE(A→A) 翻棋意图");

    /* 5. 翻棋后保持选中（点 A 再触发翻棋意图）；点空白 → 取消 */
    int f5 = -1, t5 = -1;
    int ev5 = jq_touch_state_process(&ts, cx, cy, W, H, &f5, &t5);   /* 保持选中，再点A = MOVE(A→A) */
    CHECK(ev5 == JQ_EV_MOVE && f5 == A && t5 == A, "翻棋后点A → MOVE(A→A)（保持选中）");
    CHECK(ts.mode == JQ_TS_SELECTED, "翻棋后保持选中");
    int ev6 = jq_touch_state_process(&ts, -1, -1, W, H, &f5, &t5);  /* 空白 */
    CHECK(ev6 == JQ_EV_UNSELECT && ts.mode == JQ_TS_IDLE, "选中态点空白 → UNSELECT + IDLE");

    /* 6. targets：选中格 A 的合法目标（走子 + 翻棋自身）
       ★ A 可能是地雷/军旗（default.lay (0,0)=地雷 rank=11，不可动）→
       改选第一个**可动**子（非旗非雷）做 targets 断言。 */
    jq_touch_state ts2;
    jq_touch_state_init(&ts2);
    int A2 = -1;
    for (int i = 0; i < N_CELLS; i++) {
        int r = rank_norm(st.cells[i].rank);
        if (st.cells[i].rank == RANK_EMPTY || r == RANK_FLAG || r == RANK_MINE) continue;
        int n2 = 0;
        for (int t = 0; t < N_CELLS; t++)
            if (t != i && jq_can_move(&st, i, t)) { n2++; break; }
        if (n2 > 0) { A2 = i; break; }
    }
    CHECK(A2 >= 0, "布局存在可动子");
    float c2x, c2y; bg_cell_img_center(cell_row(A2), cell_col(A2), &c2x, &c2y);
    c2x = c2x / BG_IMG_W * W; c2y = c2y / BG_IMG_H * H;
    int f6 = -1, t6 = -1;
    jq_touch_state_process(&ts2, c2x, c2y, W, H, &f6, &t6);  /* SEL A2 */
    int tg[64];
    int nt = jq_touch_state_targets(&st, &ts2, tg, 64);
    CHECK(nt >= 1, "选中格有合法目标（走子或翻棋）");

    printf(rc ? "== 有失败 ==\n" : "== 全部通过 ==\n");
    return rc;
}
