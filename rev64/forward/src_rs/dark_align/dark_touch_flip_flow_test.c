/* dark_touch_flip_flow_test.c — 暗棋翻棋触摸态机全流程对齐（AUD-R-044 修复固
 * 化：翻棋成功清选中态）行为级实证（2026-11-16 用户实锤三症状②③根因）
 *
 * 用户实锤症状（2026-11-16）：
 *   ② "我翻棋，对方翻棋，然后我就翻不了棋了"
 *   ③ "选中棋子也没有效果"
 * 根因（本轮实锤）：
 *   旧实现 jni_bridge.c OnTouch 翻棋成功后**保持选中态**（jq_glview_set_selected
 *   (flip_done ? from : -1) + 态机停留 SELECTED(A)），暗棋翻棋后轮对方，对方翻完
 *   我点暗子 B 被态机解释为**走子 A→B**（touch_state.c:78 to!=from → JQ_EV_MOVE
 *   走子意图）而非翻棋/重新选中 → jq_can_move 拒（目标暗子格）→ 翻不了棋；点 B
 *   无 SELECT 事件 → 无高亮 → 选中无效果。
 * 32 位原版实锤对照（§124y 手点全链）：蓝翻 cell6（两次点）→ AI 翻 cell3 →
 *   **蓝再翻 cell10 成功**——原版翻棋后清选中（点暗子 B = 全新选中）。
 * 修复（jni_bridge.c 落地）：翻棋成功 jq_touch_state_reset(&g_ts) + 高亮清 -1。
 *
 * 本用例桌面态机级固化（0 设备依赖）：
 *   · 修复语义：翻棋成功→reset→下一次点暗子 = JQ_EV_SELECT（全新选中）→再点
 *     同格 = 翻棋成功，蓝→红→蓝回合交替可持续；
 *   · 旧缺陷回归守卫：不 reset 时 SELECTED(A) 下点暗子 B = JQ_EV_MOVE(A→B)
 *     走子意图（文档化根因，防复发）；
 *   · 引擎层配合：三次翻棋（蓝/红/蓝）全成功 + winner 不误判。
 */
#include "junqi.h"
#include "touch_state.h"
#include "board_geom.h"
#include <stdio.h>
#include <string.h>

static int g_fail = 0;
static int chk(const char *name, int cond)
{
    printf("%s %s\n", cond ? "PASS" : "FAIL", name);
    if (!cond) g_fail++;
    return cond;
}

/* 设备格(row,col) → 触摸坐标（view 720×1280，bs=1 bby0=0，与 touch.c 命中互逆） */
static void cell_tap(int row, int col, float *px, float *py)
{
    float ix, iy;
    bg_cell_img_center(row, col, &ix, &iy);
    *px = ix; *py = iy;
}

extern void jq_dark_setup(jq_state *st, const uint8_t layout[30]);

int main(void)
{
    int ok = 1;
    const int VW = 720, VH = 1280;

    /* ---- 夹具：暗棋全 0xFF（先手翻子定色），rank2 全阵（避开行营冲突无——
       jq_dark_setup 布局 30 格全非 0 = 60 格全填） ---- */
    jq_state st; memset(&st, 0, sizeof st);
    uint8_t layout[30];
    for (int i = 0; i < 30; i++) layout[i] = 2;   /* rank2 军长 */
    jq_dark_setup(&st, layout);
    st.stage = STG_PLAY;
    st.winner = JQ_WIN_NONE;

    /* 选三个可翻暗子格（不同行不同列，触摸坐标互不干扰） */
    int c1 = -1, c2 = -1, c3 = -1, n = 0;
    for (int i = 0; i < N_CELLS; i++) {
        if (jq_can_flip(&st, i)) {
            if (n == 0) c1 = i; else if (n == 1) c2 = i; else if (n == 2) c3 = i;
            n++;
        }
    }
    ok &= chk("① 夹具：暗棋全 0xFF 可翻格 ≥3", n >= 3 && c1 >= 0 && c2 >= 0 && c3 >= 0);

    jq_touch_state ts; jq_touch_state_init(&ts);
    float px, py;

    /* ---- 蓝首翻（两次点：选中→确认翻） ---- */
    {
        int r1 = c1 / 5, col1 = c1 % 5;
        cell_tap(r1, col1, &px, &py);
        /* 命中自检：触摸坐标必须命中同一格（几何互逆守卫） */
        int hit = jq_touch_hit_cell(px, py, VW, VH, NULL, NULL);
        ok &= chk("② 触摸几何互逆：cell→tap→hit 同格", hit == c1);
        int ev1 = jq_touch_state_process(&ts, px, py, VW, VH, NULL, NULL);
        ok &= chk("③ 蓝点暗子A第1次 = JQ_EV_SELECT（全新选中）", ev1 == JQ_EV_SELECT
                  && jq_touch_state_selected(&ts) == c1);
        int ev2 = jq_touch_state_process(&ts, px, py, VW, VH, NULL, NULL);
        ok &= chk("④ 蓝点暗子A第2次 = JQ_EV_MOVE(to==from)（翻棋意图）",
                  ev2 == JQ_EV_MOVE);
        /* OnTouch 消费翻棋（引擎）+ AUD-R-044 修复：翻棋成功 reset */
        int flipped = jq_flip(&st, c1);
        jq_touch_state_reset(&ts);   /* ★ 修复语义（jni_bridge.c 同步落地） */
        ok &= chk("⑤ 蓝翻A成功 turn→RED（先手定色蓝）",
                  flipped && st.turn == SIDE_RED
                  && st.cells[c1].side == SIDE_BLUE && st.cells[c1].revealed == 1);
    }

    /* ---- 对方（AI）翻（引擎直推，不走态机——与真机 AI 应着一致） ---- */
    {
        int flipped = jq_flip(&st, c2);
        ok &= chk("⑥ 对方翻B成功 turn→BLUE（AI 翻子定红）",
                  flipped && st.turn == SIDE_BLUE
                  && st.cells[c2].side == SIDE_RED && st.cells[c2].revealed == 1);
    }

    /* ---- 蓝再翻（★ 修复核心：点暗子 C 必须是全新选中，非走子 A→B） ---- */
    {
        int r3 = c3 / 5, col3 = c3 % 5;
        cell_tap(r3, col3, &px, &py);
        int ev3 = jq_touch_state_process(&ts, px, py, VW, VH, NULL, NULL);
        ok &= chk("⑦ ★修复核心：翻棋成功reset后点暗子C = JQ_EV_SELECT（全新选中，非走子A→B）",
                  ev3 == JQ_EV_SELECT && jq_touch_state_selected(&ts) == c3);
        int ev4 = jq_touch_state_process(&ts, px, py, VW, VH, NULL, NULL);
        int ffrom = -1, fto = -1;
        cell_tap(r3, col3, &px, &py);
        ev4 = jq_touch_state_process(&ts, px, py, VW, VH, &ffrom, &fto);
        ok &= chk("⑧ 蓝点C第2次 = JQ_EV_MOVE(to==from)（翻棋意图）",
                  ev4 == JQ_EV_MOVE && ffrom == c3 && fto == c3);
        int flipped = jq_flip(&st, c3);
        jq_touch_state_reset(&ts);
        ok &= chk("⑨ 蓝再翻C成功 turn→RED（蓝→红→蓝交替持续，winner 不误判）",
                  flipped && st.turn == SIDE_RED && st.winner == JQ_WIN_NONE);
    }

    /* ---- 旧缺陷回归守卫（文档化根因，防复发）：不 reset 时 SELECTED(A) 下点
       暗子 B = JQ_EV_MOVE(A→B) 走子意图——正是用户实锤"翻不了/选中无效果"的
       根因路径。断言旧行为可复现（若未来态机改为自动清选中，此处语义随之更新
       需同步改 OnTouch 调用点）。 ---- */
    {
        jq_touch_state ts2; jq_touch_state_init(&ts2);
        int r1 = c1 / 5, col1 = c1 % 5;
        cell_tap(r1, col1, &px, &py);
        jq_touch_state_process(&ts2, px, py, VW, VH, NULL, NULL);   /* SELECT A */
        jq_touch_state_process(&ts2, px, py, VW, VH, NULL, NULL);   /* MOVE A==A */
        /* 模拟旧实现：翻棋成功不 reset（态机停留 SELECTED(A)） */
        int r3 = c3 / 5, col3 = c3 % 5;
        cell_tap(r3, col3, &px, &py);
        int from = -1, to = -1;
        int ev = jq_touch_state_process(&ts2, px, py, VW, VH, &from, &to);
        ok &= chk("⑩ 旧缺陷守卫：不reset时SELECTED(A)点暗子C=JQ_EV_MOVE(A→C)走子意图（根因路径文档化）",
                  ev == JQ_EV_MOVE && from == c1 && to == c3);
        /* 引擎层旁证：走子 A→C 被拒（C 是暗子格）→ 用户"翻不了棋"实锤 */
        int cm = jq_can_move(&st, c1, c3);
        ok &= chk("⑪ 引擎旁证：走子A→C被拒（已翻A→暗子C非合法走子）=用户'翻不了'根因",
                  cm == 0);
    }

    /* ---- 引擎层全流程：三翻之后蓝仍可翻（对局可持续，非立即终局） ---- */
    {
        int remain = 0, canf = 0;
        for (int i = 0; i < N_CELLS; i++) {
            if (st.cells[i].rank != RANK_EMPTY && !st.cells[i].revealed) {
                remain++;
                if (jq_can_flip(&st, i)) canf++;
            }
        }
        ok &= chk("⑫ 三翻后剩余暗子仍可翻（对局可持续，非困毙/终局）",
                  remain >= 57 && canf == remain && st.stage == STG_PLAY
                  && st.winner == JQ_WIN_NONE);
    }

    printf("dark_touch_flip_flow_test: %s\n", (ok && !g_fail) ? "PASS" : "FAIL");
    return (ok && !g_fail) ? 0 : 1;
}
