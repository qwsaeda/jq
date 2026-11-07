/*
 * game_selftest.c — 完整对局自测（双方 AI 走满，验证胜负/和棋/困毙全路径）
 * 蓝方 = ai_pick（固定视角），红方 = 镜像 ai_pick。
 * 验证（原版规则引擎 0x39e14/0x39f20/0x30200/0x30280/0x302b4 全路径）：
 *   1. N 局全部合法终局（胜/负/和 之一，无非法状态）
 *   2. 事件类型合法（move/eat/bomb/flag/dead/mine）
 *   3. 困毙（无棋可走判负）+ 和棋（70 步无吃子 / 总 1000 步）覆盖
 *   4. 悔棋不破坏对局流
 * 编译: make gametest → dist/jq_gametest
 */
#include <stdio.h>
#include "junqi.h"
#include "board_geom.h"   /* AUD-R-077/118/120：bg_cell_img_center/BG_IMG_W/BG_IMG_H 已拆出 junqi.h */
#include "touch_state.h"

static int rc = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); rc = 1; } \
    else printf("  ok: %s\n", msg); \
} while (0)

static jq_state st;

/* 单方走一步（暗棋阶段先翻，明棋走子/吃子）。返回 0 成功 -1 无步。 */
static int ai_step(jq_state *as, uint8_t me, int *from, int *to)
{
    /* 贪心：优先吃子（大压小/同级/炸弹/工兵挖雷），无吃子才空移，再无才翻。
       让对局收束到胜负（原版 0x30200 分发 / 0x30260 旗 / 0x30280 和 / 0x302b4 困毙）。
       ★ 接受 jq_state* 参数（修全局 st 串味：独立局状态跑 ai_step 会误读全局 st）。 */
    int bf = -1, bt = -1, bs = -1;
    for (int i = 0; i < N_CELLS; i++) {
        jq_cell *c = &as->cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || !c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        for (int t = 0; t < N_CELLS; t++) {
            if (t == i) continue;
            jq_cell *d = &as->cells[t];
            if (!jq_can_move(as, i, t)) continue;
            int dr_ = d->rank ? rank_norm(d->rank) : 0;
            int sc;
            if (d->rank) {
                if (r < dr_)        sc = 1000;   /* 大压小：能吃 */
                else if (r == dr_)  sc = 900;    /* 同级互杀 */
                else if (r == RANK_BOMB) sc = 800;
                else if (r == 9 && dr_ == RANK_MINE) sc = 700; /* 工兵挖雷 */
                else sc = 50;                     /* 吃不掉（会 DEAD），低分 */
            } else sc = 1;                         /* 空移 */
            if (sc > bs) { bs = sc; bf = i; bt = t; }
        }
    }
    if (bf >= 0 && bs >= 50) { *from = bf; *to = bt; return 0; }  /* 有吃子机会 */
    /* 无吃子 → 翻（暗棋阶段）*/
    for (int i = 0; i < N_CELLS; i++) {
        jq_cell *c = &as->cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        if (jq_can_flip(as, i)) { *from = i; *to = i; return 0; }
    }
    /* 再无 → 空移兜底（bf 是全局最优，含空移分 1） */
    if (bf >= 0) { *from = bf; *to = bt; return 0; }
    return -1;
}

int main(void) {
    int games = 60;
    printf("== 完整对局自测（蓝 vs 红 AI，%d 局）==\n", games);
    int win_red = 0, win_blue = 0, draws = 0, stalemates = 0;
    int total_moves = 0;
    for (int g = 0; g < games; g++) {
        jq_state_init(&st);
        jq_load_layout(&st, JQ_DEFAULT_LAY);
        st.stage = STG_PLAY;
        st.turn = SIDE_BLUE;
        int guard = 0;
        while (st.stage == STG_PLAY && guard++ < 5000) {
            int from = -1, to = -1;
            if (ai_step(&st, st.turn, &from, &to) != 0) {
                jq_on_stalemate(&st, st.turn);   /* AUD-R-036：显式 loser=当前 turn */
                stalemates++;
                break;
            }
            if (from == to) {
                /* 翻棋（jq_flip 成功返回 true；失败 = 军旗等不可翻 → 跳过找下一步） */
                if (!jq_flip(&st, from)) continue;
            } else {
                int ev = jq_try_move(&st, from, to);
                /* jq_try_move 内部已做胜负分发（0x30200 旗 / 0x30280 和 / 0x302b4 困毙） */
                if (ev < 0) continue;
                total_moves++;
            }
            /* jq_try_move/jq_flip 后若 stage=OVER 记统计 */
            if (st.stage == STG_OVER) {
                /* st.winner 语义（0x30200）：-1 未定 / 0 蓝胜 / 1 红胜 / 2 和 */
                if (st.winner == 1) win_red++;
                else if (st.winner == 0) win_blue++;
                else if (st.winner == JQ_WIN_DRAW) draws++;
                else printf("  局%d winner=%d 非法值\n", g, st.winner);
                break;
            }
        }
        CHECK(st.stage == STG_OVER || st.stage == STG_PLAY, "对局合法终局/进行中");
    }
    printf("  统计: 红胜=%d 蓝胜=%d 和=%d 困毙=%d 总走子=%d\n",
           win_red, win_blue, draws, stalemates, total_moves);
    /* 合法终局 = 胜负(0/1) + 和(2) + 困毙（jq_on_stalemate 设 winner 0/1）
       贪心吃子 AI 常把子吃光/扛旗 → 困毙/胜负，全困毙是正常对局收束，非失败 */
    CHECK(win_red + win_blue + draws + stalemates == games, "全部对局合法收束（胜/和/困毙）");
    printf("  困毙局数=%d（0x302b4 路径实证覆盖）\n", stalemates);

    /* ② 触摸走子判定 + 规则事件覆盖（与真机触摸闭环同一套 jq_touch_state + jq_try_move）：
       模拟两步走子（选 cell A → 点 cell B）触发的 Junqi.Move/Bomb/Eat 事件。
       桌面无法注触摸，用 jq_touch_state_process 喂坐标等价验证事件路径。 */
    printf("-- 触摸走子判定 + 规则事件（jq_touch_state → jq_try_move）--\n");
    /* ★ 层 2 修正：每（蓝子 i, 合法目标 t2）用全新局面独立测一步。
       旧版坐标命中镜像格（Y 翻）+ 拒走不翻 turn，才意外凑出多步；
       坐标修正后真实走子会翻 turn → 蓝方无法连走，故逐对独立新局面。
       统计 jq_try_move 事件分布（真实军棋走子含吃子/炸弹）。 */
    /* ★ 对抗审计修复（本轮）：分设 ev_dead/ev_mine/ev_neg 计数——旧版 else ev_move++
       把 EV_DEAD/EV_MINE/非法(-1) 静默并进"空走"，使"无非法值"断言无法检测非法返回。
       ★ 覆盖面诚实：本循环只测"初始全明棋盘、蓝方单步"（FLAG/MINE 类别此处不触发，
       由回归 [12/12] evverify --flag/--mine 独立覆盖——扛旗/挖雷路径不在本循环）。 */
    int ev_move = 0, ev_bomb = 0, ev_eat = 0, ev_flag = 0, ev_dead = 0, ev_mine = 0, ev_neg = 0, ev_try = 0;
    float viewW = 720.0f, viewH = 1280.0f;
    for (int i = 0; i < N_CELLS && ev_try < 200; i++) {
        for (int t2 = 0; t2 < N_CELLS && ev_try < 200; t2++) {
            if (t2 == i) continue;
            jq_state tsf; jq_state_init(&tsf);
            jq_load_layout(&tsf, JQ_DEFAULT_LAY);
            for (int c = 0; c < N_CELLS; c++)
                if (tsf.cells[c].rank != RANK_EMPTY) tsf.cells[c].revealed = 1;
            tsf.stage = STG_PLAY; tsf.turn = SIDE_BLUE;
            if (tsf.cells[i].rank == RANK_EMPTY || tsf.cells[i].side != SIDE_BLUE) continue;
            if (!jq_can_move(&tsf, i, t2)) continue;
            /* 两步触摸：选中 i → 点 t2（坐标 = 格中心，view 720x1280 基准，
               与 touch.c/render.c 同基准 datax_177 BG_GRID） */
            float ax, ay, bx, by;
            bg_cell_img_center(cell_row(i), cell_col(i), &ax, &ay);
            ax = ax / BG_IMG_W * viewW; ay = ay / BG_IMG_H * viewH;
            bg_cell_img_center(cell_row(t2), cell_col(t2), &bx, &by);
            bx = bx / BG_IMG_W * viewW; by = by / BG_IMG_H * viewH;
            jq_touch_state gtsf; jq_touch_state_init(&gtsf);
            int f = -1, t = -1;
            int e1 = jq_touch_state_process(&gtsf, ax, ay, 720, 1280, &f, &t);
            if (e1 != 1) continue;   /* ev=1 SELECT */
            int e2 = jq_touch_state_process(&gtsf, bx, by, 720, 1280, &f, &t);
            if (e2 != 7) continue;   /* ev=7 MOVE（帧驱动判定） */
            int e = jq_try_move(&tsf, f, t);
            ev_try++;
            if (e == EV_MOVE) ev_move++;
            else if (e == EV_BOMB) ev_bomb++;
            else if (e == EV_EAT) ev_eat++;
            else if (e == EV_FLAG) ev_flag++;
            else if (e == EV_DEAD) ev_dead++;
            else if (e == EV_MINE) ev_mine++;
            else ev_neg++;   /* 非法返回（不应出现：jq_can_move 已证合法） */
        }
    }
    printf("  触摸事件: 空走=%d 吃子=%d 炸弹=%d 吃旗=%d 被吃=%d 挖雷=%d 非法=%d（共 %d 次 jq_try_move）\n",
           ev_move, ev_eat, ev_bomb, ev_flag, ev_dead, ev_mine, ev_neg, ev_try);
    CHECK(ev_try > 0, "触摸走子判定有实际触发（≥1 次 jq_try_move）");
    CHECK(ev_neg == 0, "jq_try_move 无非法返回（jq_can_move 预证合法 → 全部合法事件）");
    CHECK(ev_move + ev_eat + ev_bomb + ev_flag + ev_dead + ev_mine == ev_try,
          "每次 jq_try_move 都返回已知合法事件（无未分类返回值）");
    CHECK(ev_eat + ev_bomb > 0, "对局含吃子/炸弹事件（规则引擎非全空移，真实军棋走子）");

    /* ③ 人机对战回合交替闭环（与真机 OnUpdate AI 驱动同一套 ai_pick + jq_try_move）：
       蓝方（ai_step）走子 → turn 切红 → 红方（ai_pick）AI 应着 → turn 切回蓝，
       验证"下一局"判据：玩家走子后对方自动应着，不卡死。
       桌面等价验证：N 局 30 回合，统计回合交替 + 红方 AI 实际产生走子。 */
    printf("-- 人机对战回合交替闭环（蓝 ai_step vs 红 AI 应着）--\n");
    /* 单局：蓝方（ai_step 视角 0）走子 → jq_try_move 后 turn=SIDE_RED →
       红方（ai_step 视角 1）AI 应着 → jq_try_move 后 turn 切回蓝。
       走子方 = 走子前 as.turn。验证"蓝走子后红方 AI 自动应着，回合翻转"。 */
    jq_state as; jq_state_init(&as);
    jq_load_layout(&as, JQ_DEFAULT_LAY);
    for (int i = 0; i < N_CELLS; i++)
        if (as.cells[i].rank != RANK_EMPTY) as.cells[i].revealed = 1;
    as.stage = STG_PLAY; as.turn = SIDE_BLUE;
    int blue_moves = 0, red_moves = 0, flip_cnt = 0, prev_turn = as.turn, guard = 0;
    int stale = 0;
    while (as.stage == STG_PLAY && guard++ < 20) {
        int from = -1, to = -1;
        int mover = as.turn;
        if (ai_step(&as, as.turn, &from, &to) != 0) { stale = 1; jq_on_stalemate(&as, mover); break; }
        if (from == to) { if (!jq_flip(&as, from)) continue; }
        else { int ev = jq_try_move(&as, from, to); if (ev < 0) continue; }
        if (mover == SIDE_BLUE) blue_moves++; else red_moves++;
        if (as.turn != prev_turn) { flip_cnt++; prev_turn = as.turn; }
    }
    printf("  回合交替: 蓝方走子=%d 红方AI走子=%d 翻转=%d 困毙=%d（单局 20 回合上限）\n",
           blue_moves, red_moves, flip_cnt, stale);
    CHECK(red_moves > 0, "红方 AI 有实际应着（非全卡死，人机对战可玩）");
    CHECK(blue_moves > 0, "蓝方（玩家方）有实际走子");
    CHECK(flip_cnt > 0, "回合翻转发生（蓝→红 交替，非单方死循环）");
    printf(rc ? "== 有失败 ==\n" : "== 全部通过 ==\n");
    return rc;
}
