/*
 * main.c — 桌面 headless 可执行（验收：能编译 + 走一局 + 走子/吃子/悔棋对齐）
 * 用法：
 *   ./jq_run              默认 default.lay，蓝方输入 / 红方 AI 自动，走一局到终局
 *   ./jq_run <0-49>       用预设布局 N（dist/spak_res 解包的 data_lay 文件）
 *   ./jq_run flip         暗棋翻棋（ruleflip：50 子随机、先手翻子定色）
 * 输入（仅蓝方=玩家）：
 *   "r c r c" 走子/吃子（如 "4 2 3 2"）
 *   "F r c" 翻棋；"u" 悔棋（撤销上一步）；"r" 重开；"q" 认输；"h" 帮助
 * 暗棋模式双方均由 AI 自动（演示翻子定色）。
 */
#include "junqi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- 悔棋历史（栈）---- */
#define UNDO_MAX 16
static jq_state g_undo[UNDO_MAX];
static int g_undo_top = 0;
static void undo_push(const jq_state *st) {
    if (g_undo_top < UNDO_MAX) g_undo[g_undo_top++] = *st;
}
static int undo_pop(jq_state *st) {
    if (g_undo_top <= 0) return 0;
    *st = g_undo[--g_undo_top];
    return 1;
}

/* ---------- 简单 AI（当前方自动走子）：吃子→前进→兜底→翻棋 ---------- */
static int ai_pick(const jq_state *st, int *from, int *to, int *is_flip)
{
    uint8_t me = st->turn;
    *is_flip = 0;
    /* 1. 吃子（已翻子，目标为对方/未翻子） */
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || !c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        for (int t = 0; t < N_CELLS; t++) {
            if (t == i) continue;
            const jq_cell *d = &st->cells[t];
            if (d->rank == RANK_EMPTY || d->side == me) continue;
            if (!jq_can_move(st, i, t)) continue;
            int dr_ = rank_norm(d->rank);
            if (r < dr_ || r == dr_ || r == RANK_BOMB || dr_ == RANK_BOMB ||
                (r == 9 && dr_ == RANK_MINE)) { *from = i; *to = t; return 1; }
        }
    }
    /* 2. 向对方方向前进 */
    int dr = (me == SIDE_RED) ? -1 : 1;
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || !c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        int nr = cell_row(i) + dr, nc = cell_col(i);
        if (nr < 0 || nr >= BOARD_ROWS) continue;
        int t2 = cell_index(nr, nc);
        if (jq_can_move(st, i, t2)) { *from = i; *to = t2; return 1; }
    }
    /* 3. 兜底：任一合法移动 */
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || !c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        for (int t2 = 0; t2 < N_CELLS; t2++)
            if (t2 != i && jq_can_move(st, i, t2)) { *from = i; *to = t2; return 1; }
    }
    /* 4. 翻棋 */
    for (int i = 0; i < N_CELLS; i++)
        if (jq_can_flip(st, i)) { *from = i; *to = i; *is_flip = 1; return 1; }
    return 0;
}

/* ---------- 棋盘打印（rank 名 + 暗棋 x + 行营/大本营标记） ---------- */
static const char *RN[13] = {".","司","军","师","旅","团","营","连","排","工","炸","雷","旗"};
static void board_print(const jq_state *st)
{
    (void)st;
    printf("      col: 0  1  2  3  4\n");
    for (int r = 0; r < BOARD_ROWS; r++) {
        char line[64]; int n = 0;
        n += sprintf(line + n, "row%2d: ", r);
        for (int c = 0; c < BOARD_COLS; c++) {
            const jq_cell *x = &st->cells[cell_index(r, c)];
            int rv = rank_norm(x->rank);
            char ch = '.';
            if (x->rank != RANK_EMPTY)
                ch = x->revealed ? RN[rv][0] : 'x';
            char tag = ' ';
            if (is_camp(r, c)) tag = '@';
            else if (is_base(r, c)) tag = '#';
            n += sprintf(line + n, "%c%c  ", ch, tag);
        }
        printf("%s\n", line);
    }
}

int main(int argc, char **argv)
{
    jq_state st;
    uint8_t lay[30];
    int flip_mode = 0;

    if (argc > 1 && strcmp(argv[1], "flip") == 0) {
        /* 暗棋：每方 25 子随机放 60 格，side 未定（先手翻子定色） */
        jq_state_init(&st);
        st.stage = STG_PLAY;
        static const int dist[25] = {
            1,2,3,3,4,4,5,5,6,6, 7,7,7,8,8,8, 9,9,9,10,10,11,11,11,12
        };
        int cells[N_CELLS];
        for (int i = 0; i < N_CELLS; i++) cells[i] = i;
        unsigned seed = 20261103u;
        for (int i = N_CELLS - 1; i > 0; i--) {
            seed = seed * 1103515245u + 12345u;
            int j = (int)((seed >> 16) % (i + 1));
            int t = cells[i]; cells[i] = cells[j]; cells[j] = t;
        }
        for (int i = 0; i < 50; i++) {
            int idx = cells[i];
            st.cells[idx].rank = (uint8_t)dist[i % 25];
            st.cells[idx].side = 0xFF;
            st.cells[idx].revealed = 0;
            if (dist[i % 25] == RANK_BOMB) st.cells[idx].flags = JQF_BOMB;
            if (dist[i % 25] == RANK_MINE) st.cells[idx].flags = JQF_MINE;
            if (dist[i % 25] == RANK_FLAG) st.cells[idx].flags = JQF_FLAG;
        }
        flip_mode = 1;
        printf("[暗棋 ruleflip：50 子随机 60 格，side 未定，先手翻子定色]\n");
    } else {
        if (argc > 1) {
            int idx = atoi(argv[1]);
            if (idx >= 0 && idx <= 49) {
                /* 预设布局 00-49：SPAK data/lay/NN.lay 已解包到 spak_res（按 MD5 命名） */
                char path[512];
                /* 预设 NN.lay 对应 manifest 中 path=data/lay/NN.lay 的解包文件；
                   简单起见：NN=0 用 default，否则报错提示需要 spak 工具 */
                if (idx == 0) memcpy(lay, JQ_DEFAULT_LAY, 30);
                else {
                    /* 用 spak_res 解包产物（data_lay_NN.lay 不存在时回退 default） */
                    snprintf(path, sizeof path,
                             "/disk/workspaces/jq/rev64/dist/spak_res/lay_%02d.bin", idx);
                    FILE *f = fopen(path, "rb");
                    if (!f) {
                        printf("[预设 %02d 未解包，回退 default.lay]\n", idx);
                        memcpy(lay, JQ_DEFAULT_LAY, 30);
                    } else {
                        if (fread(lay, 1, 30, f) != 30) {
                            printf("预设 %02d 不足 30B，回退 default\n", idx);
                            memcpy(lay, JQ_DEFAULT_LAY, 30);
                        }
                        fclose(f);
                    }
                }
            } else {
                memcpy(lay, JQ_DEFAULT_LAY, 30);
            }
        } else {
            memcpy(lay, JQ_DEFAULT_LAY, 30);
        }
        jq_load_layout(&st, lay);
        printf("[明棋 rulelay：default.lay，双方暗置（军旗明置），蓝先手]\n");
    }

    char cmd[256];
    int moves = 0;
    while (st.stage != STG_OVER) {
        board_print(&st);
        printf("move=%d turn=%s since_eat=%d/70 total=%d/1000 winner=%d\n",
               moves, st.turn == SIDE_BLUE ? "蓝" : "红",
               st.moves_since_eat, st.total_moves, st.winner);

        int from = -1, to = -1, do_flip = 0;
        if (st.turn == SIDE_BLUE && !flip_mode) {
            printf("蓝方> ");
            if (!fgets(cmd, sizeof cmd, stdin)) break;
            cmd[strcspn(cmd, "\n")] = 0;
            if (!*cmd) { printf("(空输入，AI 代走)\n"); }
            else if (*cmd == 'h') {
                printf("r c r c 走子 | F r c 翻棋 | u 悔棋 | r 重开 | q 认输 | a AI代走\n");
                continue;
            } else if (*cmd == 'q') {
                st.winner = SIDE_RED; st.stage = STG_OVER;
                printf("蓝方认输，红胜。\n");
                break;
            } else if (*cmd == 'r') {
                if (flip_mode) { printf("（暗棋重开需重跑，退出）\n"); break; }
                uint8_t l2[30];
                memcpy(l2, JQ_DEFAULT_LAY, 30);
                g_undo_top = 0;
                jq_load_layout(&st, l2);
                moves = 0;
                continue;
            } else if (*cmd == 'u') {
                if (undo_pop(&st))
                    printf("悔棋成功，撤销上一步（turn 回到 %s）。\n",
                           st.turn == SIDE_BLUE ? "蓝" : "红");
                else printf("无可悔棋步。\n");
                continue;
            } else if (*cmd == 'a') {
                /* 蓝方也由 AI 代走 */
                if (!ai_pick(&st, &from, &to, &do_flip)) {
                    printf("[蓝方无棋可走 → 困毙]\n");
                    jq_on_stalemate(&st);
                    break;
                }
            } else if (sscanf(cmd, "F %d %d", &from, &to) == 2) {
                do_flip = 1;
                from = cell_index(from, to);
            } else {
                int fr, fc, tr, tc;
                if (sscanf(cmd, "%d %d %d %d", &fr, &fc, &tr, &tc) == 4) {
                    from = cell_index(fr, fc);
                    to = cell_index(tr, tc);
                } else {
                    printf("无法解析（r c r c / F r c / u / r / q / a），重输。\n");
                    continue;
                }
            }
        } else {
            if (!ai_pick(&st, &from, &to, &do_flip)) {
                printf("[当前方无棋可走/可翻 → 困毙]\n");
                jq_on_stalemate(&st);
                break;
            }
        }

        undo_push(&st);
        if (do_flip) {
            if (jq_flip(&st, from))
                printf("  翻棋 (%d,%d) rank=%d side=%d\n",
                       cell_row(from), cell_col(from),
                       rank_norm(st.cells[from].rank), st.cells[from].side);
            else
                printf("  翻棋失败 (%d,%d)\n", cell_row(from), cell_col(from));
        } else {
            int ev = jq_try_move(&st, from, to);
            if (ev < 0) {
                printf("  非法走子 (%d,%d)→(%d,%d)\n",
                       cell_row(from), cell_col(from), cell_row(to), cell_col(to));
                g_undo_top--;   /* 撤销本步快照 */
                continue;
            }
            printf("  [%s] (%d,%d)→(%d,%d)  %s\n",
                   st.turn == SIDE_BLUE ? "蓝" : "红",
                   cell_row(from), cell_col(from), cell_row(to), cell_col(to),
                   JQ_EVENT_NAMES[ev]);
        }
        moves++;
    }

    printf("\n=== 终局 === winner=%d(%s) total_moves=%d since_eat=%d\n",
           st.winner,
           st.winner == 0 ? "蓝胜" : st.winner == 1 ? "红胜" : st.winner == 2 ? "和" : "未定",
           st.total_moves, st.moves_since_eat);
    return 0;
}
