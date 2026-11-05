/*
 * game_logic.c — 军棋核心规则（R3，手写路线）
 * 标准中国军棋规则：
 *   - 走子：一步相邻；铁路直线滑行（工兵可拐弯）。
 *   - 吃子：大压小；同级/炸弹同归于尽；工兵挖雷；炸弹炸雷；吃旗即胜。
 *   - 行营：己方可进出，敌方不可入。大本营：军旗不可动。
 *   - 暗棋：随机布子，未翻开不可动，翻开后按明棋规则。
 */
#include "junqi_core.h"
#include <string.h>
#include <stdlib.h>

/* ---- 工具 ---- */
static int side_of(int row) {
    return (row <= 4) ? JQ_SIDE_BLUE : JQ_SIDE_RED;
}

/* 该格是否可被"行走"经过（非子、非敌方行营） */
static bool walkable(const jq_state *st, int row, int col, uint8_t mover_side) {
    int idx = jq_cell_index(row, col);
    jq_cell *c = &st->board[idx];
    if (c->rank != JQ_EMPTY) return false;      /* 有子阻路 */
    if (jq_is_camp(row, col)) return false;      /* 行营非铁路，不通行（L49） */
    if (jq_is_base(row, col)) return false;      /* 大本营不通行 */
    return true;
}

/* BFS：工兵沿铁路任意路径（可拐弯），不穿子/行营 */
static bool rail_path_engineer(const jq_state *st, int fr, int fc, int tr, int tc, uint8_t side) {
    if (fr == tr && fc == tc) return true;
    static const int dr[4] = {1,-1,0,0};
    static const int dc[4] = {0,0,1,-1};
    int visited[JQ_BOARD_ROWS][JQ_BOARD_COLS];
    memset(visited, 0, sizeof(visited));
    int qr[64], qc[64], head = 0, tail = 0;
    qr[tail] = fr; qc[tail] = fc; tail++;
    visited[fr][fc] = 1;
    while (head < tail) {
        int r = qr[head], c = qc[head]; head++;
        for (int k = 0; k < 4; k++) {
            int nr = r + dr[k], nc = c + dc[k];
            if (nr < 0 || nr >= JQ_BOARD_ROWS || nc < 0 || nc >= JQ_BOARD_COLS) continue;
            if (visited[nr][nc]) continue;
            /* 横向移动（nc != c）仅限铁路中线 row5/6；纵向移动任意列可行（L68 原版"十"字铁路） */
            if (nc != c && r != 5 && r != 6) continue;
            if (!jq_is_rail(nr, nc)) continue;                 /* 只沿铁路 */
            if (nr == tr && nc == tc) return true;             /* 到达 */
            if (!walkable(st, nr, nc, side)) continue;         /* 有子/行营/大本营阻路 */
            visited[nr][nc] = 1;
            qr[tail] = nr; qc[tail] = nc; tail++;
        }
    }
    return false;
}

/* 普通子沿铁路直线滑行（不拐弯），途中无子/行营 */
static bool rail_path_straight(const jq_state *st, int fr, int fc, int tr, int tc, uint8_t side) {
    if (fr != tr && fc != tc) return false;   /* 须同行或同列 */
    /* 纵向移动（同列）：纵向铁路贯穿，任意行可行。
       横向移动（同行）：仅限中线两行铁路（row5/6，原版"十"字铁路 L68）。 */
    if (fr == tr && fr != 5 && fr != 6) return false;
    if (!jq_is_rail(fr, fc) || !jq_is_rail(tr, tc)) return false;
    int step_r = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
    int step_c = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
    int r = fr + step_r, c = fc + step_c;
    while (r != tr || c != tc) {
        if (!walkable(st, r, c, side)) return false;
        r += step_r; c += step_c;
    }
    return true;
}

/* ---- R3 主入口：from→to 是否可走 ---- */
bool jq_can_move(const jq_state *st, int from, int to) {
    if (!st || from < 0 || from >= JQ_CELLS || to < 0 || to >= JQ_CELLS) return false;
    if (from == to) return false;
    jq_cell *src = &st->board[from];
    jq_cell *dst = &st->board[to];
    if (src->rank == JQ_EMPTY) return false;
    if (!src->revealed) return false;                    /* 暗棋未翻开 */
    if (src->is_flag || src->is_mine) return false;      /* 军旗/地雷不能动 */
    if (src->side != st->turn) return false;             /* 非当前方 */

    int fr = jq_cell_row(from), fc = jq_cell_col(from);
    int tr = jq_cell_row(to),   tc = jq_cell_col(to);
    uint8_t side = src->side;

    if (jq_is_base(fr, fc)) return false;                /* 大本营内子不能动 */

    /* 目标：不能是己方子 */
    if (dst->rank != JQ_EMPTY && dst->side == side) return false;
    /* 不能进入敌方行营 */
    if (jq_is_camp(tr, tc) && side_of(tr) != side) return false;
    /* 大本营：可吃军旗（扛旗胜）或地雷（原版 ruleflip"大本营的地雷、军旗可以吃"，L57）；
       其他子不可进大本营（原版 default.lay 大本营仅放军旗/地雷） */
    if (jq_is_base(tr, tc) && !dst->is_flag && !dst->is_mine) return false;

    /* 暗棋模式（ruleflip）：地雷/军旗仅工兵能吃；扛旗需已挖3雷（L63）。
       只能吃已翻子（side 已定）；UNKNOWN 未翻子不可吃（先手定色后翻子才归属，L70）。 */
    if (st->stage == JQ_STAGE_DARK && dst->rank != JQ_EMPTY) {
        if (dst->side == JQ_SIDE_UNKNOWN) return false;
        if (dst->is_mine && src->rank != JQ_RANK_SOLDIER) return false;
        if (dst->is_flag && (src->rank != JQ_RANK_SOLDIER || st->mines_cleared[side] < 3)) return false;
    }

    /* 相邻一步 */
    if ((fr == tr && (fc - tc == 1 || tc - fc == 1)) ||
        (fc == tc && (fr - tr == 1 || tr - fr == 1))) {
        return true;
    }
    /* 铁路滑动 */
    if (src->rank == JQ_RANK_SOLDIER) {   /* 工兵可拐弯 */
        return rail_path_engineer(st, fr, fc, tr, tc, side);
    }
    return rail_path_straight(st, fr, fc, tr, tc, side);
}

/* ---- 悔棋：移动前快照（循环栈，深度 JQ_UNDO_DEPTH，L40）---- */
static void snapshot_undo(jq_state *st) {
    int slot = st->undo_head;
    memcpy(st->undo_board[slot], st->board, sizeof(st->board));
    st->undo_turn[slot] = st->turn;
    st->undo_stage[slot] = st->stage;
    st->undo_winner[slot] = st->winner;
    memcpy(st->undo_flags[slot], st->flags_captured, sizeof(st->flags_captured));
    memcpy(st->undo_pieces[slot], st->pieces_left, sizeof(st->pieces_left));
    memcpy(st->undo_mines[slot], st->mines_cleared, sizeof(st->mines_cleared));
    st->undo_moves_since_eat[slot] = st->moves_since_eat;
    st->undo_head = (slot + 1) % JQ_UNDO_DEPTH;
    if (st->undo_count < JQ_UNDO_DEPTH) st->undo_count++;
}

/* 悔棋：恢复最近快照（若存在）。返回 1 成功。 */
bool jq_undo(jq_state *st) {
    if (!st || st->undo_count <= 0) return false;
    int slot = (st->undo_head - 1 + JQ_UNDO_DEPTH) % JQ_UNDO_DEPTH;
    st->undo_head = slot;
    st->undo_count--;
    memcpy(st->board, st->undo_board[slot], sizeof(st->board));
    st->turn = st->undo_turn[slot];
    st->stage = st->undo_stage[slot];
    st->winner = st->undo_winner[slot];
    memcpy(st->flags_captured, st->undo_flags[slot], sizeof(st->flags_captured));
    memcpy(st->pieces_left, st->undo_pieces[slot], sizeof(st->pieces_left));
    memcpy(st->mines_cleared, st->undo_mines[slot], sizeof(st->mines_cleared));
    st->moves_since_eat = st->undo_moves_since_eat[slot];
    if (st->moves > 0) st->moves--;
    return true;
}

/* ---- 困毙判定（标准军棋：对方无子可动 → 当前方胜）----
   遍历 side 所有子：非空、非地雷、非军旗、已revealed（暗棋未翻的不算），
   且存在至少一个合法移动目标 → 未困毙。 */
static int side_has_movable(const jq_state *st, uint8_t side) {
    int has_face_down = 0;
    /* 暗棋模式（ruleflip）：只要有未翻（side=UNKNOWN）非军旗/地雷暗子，当前方可翻 → 未困毙（L64/L70）。
       （军旗/地雷暗棋不可翻——jq_can_flip 排除 is_flag；地雷暗棋不可翻，翻后不可动。） */
    if (st->stage == JQ_STAGE_DARK) {
        for (int i = 0; i < JQ_CELLS; i++) {
            const jq_cell *c = &st->board[i];
            if (c->rank != JQ_EMPTY && c->side == JQ_SIDE_UNKNOWN &&
                !c->revealed && !c->is_flag && !c->is_mine) return 1;
        }
    }
    for (int i = 0; i < JQ_CELLS; i++) {
        const jq_cell *c = &st->board[i];
        if (c->rank == JQ_EMPTY || c->side != side) continue;
        if (!c->revealed) {
            /* 暗棋可翻：军旗/地雷暗棋不可动（翻了也不能走），不计 */
            if (!c->is_flag && !c->is_mine) has_face_down = 1;
            continue;
        }
        if (c->is_flag || c->is_mine) continue;
        for (int t = 0; t < JQ_CELLS; t++) {
            if (t == i) continue;
            if (jq_can_move(st, i, t)) return 1;
        }
    }
    /* 无已翻子可动，但还有非军旗/地雷暗棋可翻 → 未困毙 */
    return has_face_down;
}

/* 移动/翻棋后调用：若轮到的一方无子可动 → 该方负（原版 rulelay "无棋可走被判负"，L57）。
   **注意**：原版和棋不是"双方无子可动"，而是"连续70步无吃子" / "总步数1000"（见 jq_try_move）。 */
static void check_stalemate(jq_state *st) {
    if (st->stage == JQ_STAGE_OVER) return;
    if (!side_has_movable(st, st->turn)) {
        st->winner = 1 - st->turn;
        st->stage = JQ_STAGE_OVER;
    }
}

/* 原版和棋条件（rulelay 权威文本）：连续70步无吃子 → 和；双方总步数达1000 → 和（L57） */
static void check_draw(jq_state *st) {
    if (st->stage == JQ_STAGE_OVER) return;
    if (st->moves_since_eat >= 70 || st->moves >= 1000) {
        st->winner = JQ_WINNER_DRAW;
        st->stage = JQ_STAGE_OVER;
    }
}

/* ---- 执行移动（含吃子判定）----
   eat_type 输出走子事件类型（对齐原版 Junqi.Move/Eat/Bomb/Flag/Dead 音效）：
   0=move(空移) 1=eat(普通吃) 2=bomb(炸弹同归) 3=flag(扛旗) 4=dead(被吃死亡) 5=mine(挖雷)。 */
jq_move_result jq_try_move(jq_state *st, int from, int to, uint8_t *eat_type) {
    if (eat_type) *eat_type = 0;
    if (!jq_can_move(st, from, to)) return JQ_MOVE_BLOCKED;
    snapshot_undo(st);
    jq_cell *src = &st->board[from];
    jq_cell *dst = &st->board[to];
    uint8_t side = src->side;

    if (dst->rank == JQ_EMPTY) {
        /* 空目标：直接移（走子，连续无吃子步数++） */
        *dst = *src;
        memset(src, 0, sizeof(*src));
        st->moves_since_eat++;
        if (eat_type) *eat_type = 0;   /* move */
    } else if (src->is_bomb) {
        /* 炸弹撞任何子（含军旗/地雷/炸弹）：双方消失（原版 rulelay"包括军旗"，L57） */
        memset(src, 0, sizeof(*src)); memset(dst, 0, sizeof(*dst));
        st->pieces_left[side]--;
        st->pieces_left[1 - side]--;
        st->moves_since_eat = 0;
        if (eat_type) *eat_type = 2;   /* bomb */
    } else if (dst->is_flag) {
        /* 吃军旗 → 胜（原版 rulelay"任何棋子都能扛军旗"，L57） */
        *dst = *src; memset(src, 0, sizeof(*src));
        st->winner = side;
        st->stage = JQ_STAGE_OVER;
        st->flags_captured[1 - side]++;
        if (eat_type) *eat_type = 3;   /* flag */
        return JQ_MOVE_OK;
    } else if (dst->is_mine) {
        if (src->rank == JQ_RANK_SOLDIER) {   /* 工兵挖雷 */
            *dst = *src; memset(src, 0, sizeof(*src));
            st->pieces_left[1 - side]--;
            if (st->stage == JQ_STAGE_DARK) st->mines_cleared[side]++;  /* ruleflip 挖雷计数（L63） */
            if (eat_type) *eat_type = 5;   /* mine（工兵挖雷占位） */
        } else {
            /* 其他子被雷炸 */
            memset(src, 0, sizeof(*src));
            st->pieces_left[side]--;
            if (eat_type) *eat_type = 4;   /* dead（被雷炸） */
        }
        st->moves_since_eat = 0;
    } else if (dst->rank == JQ_RANK_BOMB) {
        /* 撞敌方炸弹：同归于尽 */
        memset(src, 0, sizeof(*src)); memset(dst, 0, sizeof(*dst));
        st->pieces_left[side]--;
        st->pieces_left[1 - side]--;
        st->moves_since_eat = 0;
        if (eat_type) *eat_type = 2;   /* bomb */
    } else {
        /* 大小比较：对齐原版字节语义，rank 小=强（1 司令最强）。
           src->rank < dst->rank → src 赢（吃 dst）。 */
        if (src->rank < dst->rank) {
            *dst = *src; memset(src, 0, sizeof(*src));
            st->pieces_left[1 - side]--;
            if (eat_type) *eat_type = 1;   /* eat */
        } else if (src->rank > dst->rank) {
            memset(src, 0, sizeof(*src));
            st->pieces_left[side]--;
            if (eat_type) *eat_type = 4;   /* dead（被吃） */
        } else {
            memset(src, 0, sizeof(*src)); memset(dst, 0, sizeof(*dst));
            st->pieces_left[side]--;
            st->pieces_left[1 - side]--;
            if (eat_type) *eat_type = 2;   /* bomb（同rank同归） */
        }
        st->moves_since_eat = 0;
    }
    st->turn = 1 - side;
    st->moves++;
    check_draw(st);          /* 连续70步无吃子 / 总步数1000 → 和（L57） */
    check_stalemate(st);     /* 无棋可走 → 该方负（L57） */
    return JQ_MOVE_OK;
}

/* ---- 暗棋 ---- */
bool jq_can_flip(const jq_state *st, int idx) {
    if (!st || idx < 0 || idx >= JQ_CELLS) return false;
    jq_cell *c = &st->board[idx];
    /* 军旗明置不可翻（需走子吃旗胜，L45）；地雷/炸弹可翻 */
    return (c->rank != JQ_EMPTY && !c->revealed && !c->is_flag);
}

bool jq_flip(jq_state *st, int idx) {
    if (!jq_can_flip(st, idx)) return false;
    jq_cell *c = &st->board[idx];
    /* 布局模式：只能翻当前方（turn）的子（L50）。
       暗棋模式（ruleflip）：side 未定（UNKNOWN）可翻，先手翻子定色（L64） */
    if (st->stage != JQ_STAGE_DARK && c->side != st->turn) return false;
    snapshot_undo(st);
    if (c->side == JQ_SIDE_UNKNOWN) {
        c->side = st->turn;              /* ruleflip 先手翻子定色（L64） */
        st->pieces_left[st->turn]++;
    }
    c->revealed = 1;
    if (c->is_flag && c->side != st->turn) {
        /* 翻开对方军旗即胜（简化暗棋规则） */
        st->winner = st->turn;
        st->stage = JQ_STAGE_OVER;
    }
    st->turn = 1 - st->turn;
    st->moves++;
    st->moves_since_eat++;
    check_draw(st);        /* 70步无吃子 / 1000步 → 和（L57） */
    check_stalemate(st);   /* 无棋可走 → 该方负（L57） */
    return true;
}

/* 每方 25 子配置（对齐原版字节语义：1=司令...9=工兵, 10=炸弹, 11=地雷, 12=军旗） */
static const uint8_t ARMY_RANKS[JQ_PIECES] = {
    /* 司令1 军长2 师长3×2 旅长4×2 团长5×2 营长6×2 */
    1, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    /* 连长7×3 排长8×3 工兵9×3 炸弹2 地雷3 军旗1 */
    7, 7, 7, 8, 8, 8, 9, 9, 9,
    JQ_RANK_BOMB, JQ_RANK_BOMB,
    JQ_RANK_MINE, JQ_RANK_MINE, JQ_RANK_MINE,
    JQ_RANK_FLAG
};

/* 简单 xorshift PRNG */
static uint32_t rng_state;
static uint32_t rng_next(void) {
    uint32_t x = rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    rng_state = x;
    return x;
}

void jq_dark_setup(jq_state *st, uint32_t seed) {
    rng_state = seed ? seed : 0x9E3779B9u;
    jq_state_init(st, JQ_STAGE_DARK);
    /* 50 子（25×2）随机放 60 格（简化暗棋：全部格可放） */
    int cells[JQ_CELLS];
    for (int i = 0; i < JQ_CELLS; i++) cells[i] = i;
    /* Fisher-Yates 洗 60 格 */
    for (int i = JQ_CELLS - 1; i > 0; i--) {
        int j = rng_next() % (i + 1);
        int t = cells[i]; cells[i] = cells[j]; cells[j] = t;
    }
    /* 前 50 格放子（暗棋：side 未定，先手翻子定色 L64） */
    for (int i = 0; i < 50; i++) {
        uint8_t rank = ARMY_RANKS[i % JQ_PIECES];
        int idx = cells[i];
        jq_cell *c = &st->board[idx];
        c->rank = rank; c->side = JQ_SIDE_UNKNOWN; c->revealed = 0;
        c->is_flag = (rank == JQ_RANK_FLAG);
        c->is_mine = (rank == JQ_RANK_MINE);
        c->is_bomb = (rank == JQ_RANK_BOMB);
    }
}

/* default.lay 布局值 0-12 → 军棋子力（对齐原版字节语义，REVERSE_NOTES.md §5）。
   原版 `0x6a3a4` 映射：`红方=字节+0xc / 蓝方=字节`；`0x6f7b0` 布局字节直接入 state。
   即：**布局字节值 = rank 本身**（1-9 普通、10 炸弹、11 地雷、12 军旗）。
   值 1=司令最强...9=工兵最弱（标准军棋 1 号最强编码惯例）。
   arm64 直接返回字节值（不再做 40..32 降序转换）。 */
static uint8_t layout_rank(uint8_t v) {
    return (v < 13) ? v : 0;
}

void jq_load_layout(jq_state *st, const uint8_t layout[30]) {
    jq_state_init(st, JQ_STAGE_PLAY);
    /* default.lay 30 字节 = 每方 6行×5列 = 30 格（行主序），0=空（行营）。
       蓝方占行0-5，红方占行6-11。军旗在 layout 行5（底线）：
         蓝方：layout 行 r → 棋盘行 (5-r)，军旗→行0（蓝自己底线）。
         红方：layout 行 r → 棋盘行 (6+r)，军旗→行11（红自己底线），列镜像。
       值→rank 由 layout_rank（降序）映射，25 子/方自动落位。 */
    for (int i = 0; i < 30; i++) {
        uint8_t v = layout[i];
        if (v == 0) continue;               /* 空位（行营） */
        uint8_t rank = layout_rank(v);
        if (!rank) continue;
        int lrow = i / 5, lcol = i % 5;
        /* 蓝方：翻转行，军旗(lrow5)→棋盘行0 */
        jq_place(st, jq_cell_index(5 - lrow, lcol), JQ_SIDE_BLUE, rank);
        /* 红方：镜像（行+6, 列翻转），军旗(lrow5)→棋盘行11 */
        jq_place(st, jq_cell_index(6 + lrow, 4 - lcol), JQ_SIDE_RED, rank);
    }
    /* 暗棋开局：所有子未 revealed（背面朝上，需翻开），**军旗明置**（L45）。
       jq_place 设 revealed=1，这里统一置暗（军旗除外——军旗可见，不可翻，需吃旗胜）。 */
    for (int i = 0; i < JQ_CELLS; i++)
        if (st->board[i].rank != JQ_EMPTY && !st->board[i].is_flag)
            st->board[i].revealed = 0;
    st->turn = JQ_SIDE_BLUE;
}

/* ================= 布局编辑（布阵阶段 JQ_STAGE_SETUP，L80 对齐原版 0x2a228） =================
   rulelay 权威规则（data_junqi_rulelay.txt，L57/L72）：
     - 炸弹不能放在第一行（蓝方前线路 = 棋盘行5）
     - 地雷只能放在最后两行（棋盘行0-1）
     - 军旗只能放在大本营（棋盘行0 列1/3）
   原版 0x2a228 红方校验（对称，60 格索引）：炸弹禁行6、地雷仅行10-11、军旗格56/58。
   蓝方（玩家）区域 = 棋盘行0-5（30 格）；红方（AI）用 default.lay 镜像作不可编辑预览。 */

/* 标准 default.lay（30 字节，蓝方视角：行0=前线路，行5=底线/军旗）。
   与 SPAK data/lay/default.lay 字节一致（L59）。无 SPAK 时兜底。 */
static const uint8_t DEFAULT_LAY[30] = {
    3,7,6,5,8, 10,0,9,0,1, 2,10,0,9,5, 4,0,6,0,7, 8,3,4,9,11, 11,12,11,8,7
};

/* 标准军力分布（25 子/方，rank 1-12 计数），供 jq_setup_validate 全量校验。 */
static void standard_army_count(int cnt[13]) {
    for (int i = 0; i < 13; i++) cnt[i] = 0;
    for (int i = 0; i < JQ_PIECES; i++) cnt[ARMY_RANKS[i]]++;
}

/* 逐格校验：rank 在 (row,col) 是否合法（原版 0x2a228 逐格语义，蓝方对称）。 */
static jq_layout_error check_place_rank(uint8_t rank, int row, int col) {
    if (rank == JQ_EMPTY) return JQ_LAYOUT_OK;
    if (rank == JQ_RANK_BOMB && row == 5) return JQ_LAYOUT_ZHA_ERROR; /* 炸弹禁第一行（前线路） */
    if (rank == JQ_RANK_MINE && row > 1)  return JQ_LAYOUT_LEI_ERROR; /* 地雷仅最后两行 */
    if (rank == JQ_RANK_FLAG && !(row == 0 && (col == 1 || col == 3)))
        return JQ_LAYOUT_QI_ERROR;                                    /* 军旗仅大本营 */
    return JQ_LAYOUT_OK;
}

void jq_setup_begin(jq_state *st, const uint8_t layout[30]) {
    jq_load_layout(st, layout ? layout : DEFAULT_LAY);  /* 双方 default.lay 镜像 */
    st->stage = JQ_STAGE_SETUP;
    st->turn = JQ_SIDE_BLUE;
    st->winner = 0xFF;
    st->moves = 0;
    st->moves_since_eat = 0;
    /* 布阵阶段：蓝方（玩家）棋子全部翻开可编辑；红方（AI）暗置作不可编辑预览。 */
    for (int i = 0; i < JQ_CELLS; i++) {
        jq_cell *cell = &st->board[i];
        if (cell->rank == JQ_EMPTY) continue;
        cell->revealed = (cell->side == JQ_SIDE_BLUE) ? 1 : 0;
    }
}

jq_layout_error jq_setup_place(jq_state *st, int from, int to) {
    if (!st || st->stage != JQ_STAGE_SETUP) return JQ_LAYOUT_BAD_POS;
    if (from < 0 || from >= JQ_CELLS || to < 0 || to >= JQ_CELLS) return JQ_LAYOUT_BAD_POS;
    if (from == to) return JQ_LAYOUT_OK;
    jq_cell *src = &st->board[from];
    /* 源必须是蓝方子（玩家可编辑） */
    if (src->rank == JQ_EMPTY || src->side != JQ_SIDE_BLUE) return JQ_LAYOUT_BAD_POS;
    /* 目标必须在蓝方区域（行0-5）；红方预览不可动 */
    if (jq_cell_row(to) > 5) return JQ_LAYOUT_BAD_POS;
    jq_cell *dst = &st->board[to];
    if (dst->rank != JQ_EMPTY && dst->side != JQ_SIDE_BLUE) return JQ_LAYOUT_BAD_POS;
    /* 逐格校验：源子→目标格 与（交换时）目标子→源格（原版 0x2a228 逐格语义） */
    jq_layout_error e = check_place_rank(src->rank, jq_cell_row(to), jq_cell_col(to));
    if (e != JQ_LAYOUT_OK) return e;
    if (dst->rank != JQ_EMPTY) {
        e = check_place_rank(dst->rank, jq_cell_row(from), jq_cell_col(from));
        if (e != JQ_LAYOUT_OK) return e;
    }
    /* 交换/移动（整格交换，保留 is_flag/is_mine/is_bomb） */
    jq_cell tmp = *src; *src = *dst; *dst = tmp;
    return JQ_LAYOUT_OK;
}

jq_layout_error jq_setup_validate(const jq_state *st) {
    if (!st) return JQ_LAYOUT_COUNT_ERROR;
    int cnt[13]; standard_army_count(cnt);
    int placed[13] = {0};
    int total = 0;
    for (int r = 0; r <= 5; r++) {                 /* 蓝方区域行0-5 */
        for (int c = 0; c < JQ_BOARD_COLS; c++) {
            int idx = jq_cell_index(r, c);
            const jq_cell *cell = &st->board[idx];
            if (cell->rank == JQ_EMPTY) continue;
            if (cell->side != JQ_SIDE_BLUE) return JQ_LAYOUT_COUNT_ERROR; /* 红方子混入 */
            jq_layout_error e = check_place_rank(cell->rank, r, c);
            if (e != JQ_LAYOUT_OK) return e;
            if (cell->rank < 1 || cell->rank > 12) return JQ_LAYOUT_COUNT_ERROR;
            placed[cell->rank]++;
            total++;
        }
    }
    if (total != JQ_PIECES) return JQ_LAYOUT_COUNT_ERROR;
    for (int k = 1; k <= 12; k++)
        if (placed[k] != cnt[k]) return JQ_LAYOUT_COUNT_ERROR;
    return JQ_LAYOUT_OK;
}

jq_layout_error jq_setup_confirm(jq_state *st) {
    if (!st) return JQ_LAYOUT_COUNT_ERROR;
    jq_layout_error e = jq_setup_validate(st);
    if (e != JQ_LAYOUT_OK) return e;
    st->stage = JQ_STAGE_PLAY;
    st->turn = JQ_SIDE_BLUE;
    st->winner = 0xFF;
    st->moves = 0;
    st->moves_since_eat = 0;
    /* 进入对局：双方暗置（军旗明置，L45），先手蓝方 */
    for (int i = 0; i < JQ_CELLS; i++) {
        jq_cell *cell = &st->board[i];
        if (cell->rank != JQ_EMPTY && !cell->is_flag)
            cell->revealed = 0;
    }
    return JQ_LAYOUT_OK;
}

bool jq_is_setup(const jq_state *st) {
    return st && st->stage == JQ_STAGE_SETUP;
}

/* ---- 胜负 ---- */
bool jq_is_game_over(const jq_state *st) {
    return st->stage == JQ_STAGE_OVER;
}
uint8_t jq_winner(const jq_state *st) {
    return st->winner;
}

/* ---- AI 难度（原版 Low/Mid/High/Rank 菜单，L73 8 标签确证）----
   原版难度决策在场景 vtable（运行时填充 0x229c0 stub，静态无法确证）；
   arm64 实现三档**决策强度**（诚实边界：非原版逐指令，功能对齐档数=3）。
   0=LOW（随机失误） 1=MID（贪心，默认，L43） 2=HIGH（价值+安全吃子）。 */
static int g_ai_level = 1;
static uint32_t ai_rng = 0x9E3779B9u;

void jq_ai_set_level(int level) {
    g_ai_level = (level < 0) ? 0 : (level > 2 ? 2 : level);
    ai_rng = 0x9E3779B9u ^ ((uint32_t)g_ai_level * 0x9E3779B9u);  /* 定长种子，测试可复现 */
}
int jq_ai_get_level(void) { return g_ai_level; }

static uint32_t ai_rand(void) {
    uint32_t x = ai_rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    ai_rng = x;
    return x;
}

/* 子价值（HIGH 评分）：军旗=胜，rank 1(司令)最强→90 ... 9(工兵)最弱→10。 */
static int ai_value(uint8_t rank) {
    if (rank == JQ_RANK_FLAG) return 10000;
    if (rank == JQ_RANK_BOMB) return 90;
    if (rank == JQ_RANK_MINE) return 80;
    if (rank >= 1 && rank <= 9) return (10 - rank) * 10;
    return 0;
}

/* HIGH：吃子安全性——模拟 i→t 吃子后，是否有敌方能反吃（攻击子会被换掉/吃掉）。
   用 jq_try_move+jq_undo 干净模拟（undo 恢复 board/turn/stage/moves_since_eat/moves，L81 已验证）。 */
static int ai_capture_risky(jq_state *st, int i, int t) {
    uint8_t atk = st->board[i].rank;
    uint8_t me = st->turn;
    jq_try_move(st, i, t, NULL);
    int risky = 0;
    for (int e = 0; e < JQ_CELLS && !risky; e++) {
        jq_cell *ec = &st->board[e];
        if (ec->rank == JQ_EMPTY || ec->side == me) continue;
        if (!ec->revealed || ec->is_flag || ec->is_mine) continue;
        if (!jq_can_move(st, e, t)) continue;
        /* E 反吃：E 是炸弹（同归）或 E.rank <= 攻击子（胜/同归）→ 攻击子会失 */
        if (ec->rank == JQ_RANK_BOMB || ec->rank <= atk) risky = 1;
    }
    jq_undo(st);
    return risky;
}

/* 阶段1：吃子。level=2 → 价值+安全最优；否则首个可吃（L43 贪心）。返回 3=吃子/0=无。 */
static int ai_capture(jq_state *st, int level) {
    uint8_t me = st->turn;
    int best_i = -1, best_t = -1, best_score = -1;
    for (int i = 0; i < JQ_CELLS; i++) {
        jq_cell *c = &st->board[i];
        if (c->rank == JQ_EMPTY || c->side != me) continue;
        if (!c->revealed || c->is_flag || c->is_mine) continue;
        for (int t = 0; t < JQ_CELLS; t++) {
            if (t == i) continue;
            jq_cell *d = &st->board[t];
            if (d->rank == JQ_EMPTY || d->side == me) continue;
            if (!jq_can_move(st, i, t)) continue;
            if (level != 2) {
                jq_try_move(st, i, t, NULL);
                return 3;
            }
            /* HIGH：评分 = 目标价值 - (吃子后攻击子会失 ? 攻击子价值 : 0) */
            int score = ai_value(d->rank) - (ai_capture_risky(st, i, t) ? ai_value(c->rank) : 0);
            if (score > best_score) { best_score = score; best_i = i; best_t = t; }
        }
    }
    if (best_i >= 0) { jq_try_move(st, best_i, best_t, NULL); return 3; }
    return 0;
}

/* 阶段2：前进（向对方方向移动一步）。返回 1=移动/0=无。 */
static int ai_advance(jq_state *st) {
    uint8_t me = st->turn;
    int dr = (me == JQ_SIDE_RED) ? -1 : 1;
    for (int i = 0; i < JQ_CELLS; i++) {
        jq_cell *c = &st->board[i];
        if (c->rank == JQ_EMPTY || c->side != me) continue;
        if (!c->revealed || c->is_flag || c->is_mine) continue;
        int r = jq_cell_row(i), col = jq_cell_col(i);
        int nr = r + dr;
        if (nr < 0 || nr >= JQ_BOARD_ROWS) continue;
        int to = jq_cell_index(nr, col);
        if (jq_can_move(st, i, to)) { jq_try_move(st, i, to, NULL); return 1; }
    }
    return 0;
}

/* 阶段2b：任一合法走子兜底（AI 前方被围，找任意可动格，L70）。返回 1=移动/0=无。 */
static int ai_fallback(jq_state *st) {
    uint8_t me = st->turn;
    for (int i = 0; i < JQ_CELLS; i++) {
        jq_cell *c = &st->board[i];
        if (c->rank == JQ_EMPTY || c->side != me) continue;
        if (!c->revealed || c->is_flag || c->is_mine) continue;
        for (int t = 0; t < JQ_CELLS; t++) {
            if (t == i) continue;
            if (jq_can_move(st, i, t)) { jq_try_move(st, i, t, NULL); return 1; }
        }
    }
    return 0;
}

/* 阶段3：翻棋（选最靠近敌方前沿的暗棋）。
   布局模式：翻己方未翻子；暗棋模式（ruleflip）：翻 side=UNKNOWN 子（先手定色后，L70）。 */
static int ai_flip(jq_state *st) {
    uint8_t me = st->turn;
    int best = -1, best_dist = -1;
    for (int i = 0; i < JQ_CELLS; i++) {
        jq_cell *c = &st->board[i];
        if (c->rank == JQ_EMPTY || c->is_flag || c->is_mine) continue;
        if (c->revealed) continue;
        if (st->stage != JQ_STAGE_DARK) {
            if (c->side != me) continue;
        } else {
            if (c->side != JQ_SIDE_UNKNOWN) continue;
        }
        int r = jq_cell_row(i);
        int dist = (me == JQ_SIDE_RED) ? r : (JQ_BOARD_ROWS - 1 - r);
        if (best < 0 || dist < best_dist) { best = i; best_dist = dist; }
    }
    if (best >= 0 && jq_flip(st, best)) return 2;
    return 0;
}

/* LOW：随机走法（收集全部合法动作，等概率选一）。返回 1=移动/2=翻棋/3=吃子/0=无。 */
static int ai_random(jq_state *st) {
    uint8_t me = st->turn;
    /* 先收集移动（含吃子），再收集可翻 */
    int moves[60 * 2][2]; int nm = 0;
    int flips[60]; int nf = 0;
    for (int i = 0; i < JQ_CELLS; i++) {
        jq_cell *c = &st->board[i];
        if (c->rank == JQ_EMPTY || c->side != me) continue;
        if (c->revealed) {
            if (c->is_flag || c->is_mine) continue;
            for (int t = 0; t < JQ_CELLS; t++) {
                if (t == i) continue;
                if (jq_can_move(st, i, t) && nm < 120) {
                    moves[nm][0] = i; moves[nm][1] = t; nm++;
                }
            }
        } else if (!c->is_flag && !c->is_mine) {
            if ((st->stage != JQ_STAGE_DARK && c->side == me) ||
                (st->stage == JQ_STAGE_DARK && c->side == JQ_SIDE_UNKNOWN)) {
                if (nf < 60) flips[nf++] = i;
            }
        }
    }
    if (nm == 0 && nf == 0) return 0;
    uint32_t pick = ai_rand();
    if (nm > 0 && (nf == 0 || (pick & 1))) {
        uint32_t k = ai_rand() % nm;
        int i = moves[k][0], t = moves[k][1];
        jq_cell *d = &st->board[t];
        int is_capture = (d->rank != JQ_EMPTY && d->side != me);
        jq_try_move(st, i, t, NULL);
        return is_capture ? 3 : 1;
    }
    uint32_t k = ai_rand() % nf;
    jq_flip(st, flips[k]);
    return 2;
}

/* ---- 简单 AI（当前方自动走一步，分档，L73）----
   返回：0=无动作, 1=移动, 2=翻棋, 3=吃子（音效区分）。 */
int jq_ai_move(jq_state *st) {
    if (!st || st->stage == JQ_STAGE_OVER) return 0;
    uint8_t me = st->turn;

    /* AI 自身困毙：当前方（me）无子可走且无可翻 → 判负（L70）。
       （check_stalemate 在走子/翻棋后查的是对方，此处先查 me。） */
    if (!side_has_movable(st, me)) {
        st->winner = 1 - me;
        st->stage = JQ_STAGE_OVER;
        return 0;
    }

    int act;
    if (g_ai_level == 0) {
        /* LOW：50% 随机失误，否则贪心（无价值偏好） */
        if ((ai_rand() & 1) && (act = ai_random(st)) > 0) return act;
        if ((act = ai_capture(st, 1)) > 0) return act;
        if ((act = ai_advance(st)) > 0) return act;
        if ((act = ai_fallback(st)) > 0) return act;
        if ((act = ai_flip(st)) > 0) return act;
    } else if (g_ai_level == 2) {
        /* HIGH：价值+安全最优吃子，再前进/兜底/翻棋 */
        if ((act = ai_capture(st, 2)) > 0) return act;
        if ((act = ai_advance(st)) > 0) return act;
        if ((act = ai_fallback(st)) > 0) return act;
        if ((act = ai_flip(st)) > 0) return act;
    } else {
        /* MID：贪心（L43），默认 */
        if ((act = ai_capture(st, 1)) > 0) return act;
        if ((act = ai_advance(st)) > 0) return act;
        if ((act = ai_fallback(st)) > 0) return act;
        if ((act = ai_flip(st)) > 0) return act;
    }

    /* 无可走子/可翻子 → 困毙判定（无子可动 → 该方负，L57）；若仍非终局则返回 0 */
    check_stalemate(st);
    return 0;
}
