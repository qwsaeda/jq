/*
 * rules.c — 走子 / 吃子 / 翻棋 / 胜负 规则（逆源码重写）
 * 依据原版函数（VA）+ 规则文本（SPAK data/junqi/rulelay.txt / ruleflip.txt，GBK 已读）：
 *   0x39e14 目标类型判定：
 *     r2>0xc 非法; 炸弹(0xa): to_row!=0 合法; 地雷(0xb): from_row>3 合法(仅工兵吃);
 *     军旗(0xc): from_row∈{1,3,5} 特判(0x39e7a); 普通 1-9 合法。
 *     （§5/L47/L48/L50/L59：军旗翻棋 from==to 永假→军旗不可翻；地雷可翻；炸弹可翻。）
 *   0x39f20 移动执行器：from==to→翻棋；坐标 0x6ef54 比较（[0]=col [1]=row）；
 *     格子 +0x80+[from*5+to_offset]，rank 在格 +4（L65/L79）。
 *   0x3a15c 走子交换：from rank ← to rank 互换（含吃子落点）。
 *   0x30200 胜负分发：r3=0/1/2（蓝胜/红胜/和）→ Junqi.Win/Draw 事件。
 *   0x30260 吃旗胜 / 0x30280 和棋 / 0x302b4 困毙（status=3，L53/L54）。
 * 权威规则（rulelay 原文）：
 *   - 吃子：司令>军长>师长>旅长>团长>营长>连长>排长>工兵（rank 小=强，L52）；
 *     炸弹与任何棋子相遇双方都消失（包括军旗）；地雷<工兵、>所有其他棋；
 *     任何棋子都能扛军旗。
 *   - 行营=安全岛（敌方不能吃行营中棋子）；进大本营不可再动；军旗必须在大本营。
 *   - 胜负：棋子被吃光 / 军旗被扛 / 无棋可走 → 判负；
 *     连续 70 步无吃子 → 和；总步数 1000 → 和。
 * 铁路（L49/L68）：纵向仅 col2 贯穿；横向仅 row5/6（中线）；工兵可拐弯（BFS），
 *   其他子直走不能转弯；行营/大本营在铁路外。
 */
#include "junqi.h"
#include "ui_state.h"   /* ★ P1（datah_012）：游戏设置 4 规则开关 ui_rule_cfg_get()
                          （和棋/大本营/吃地雷/吃军旗，默认全开对齐原版默认，行为不变） */
#include <string.h>
#include <stdlib.h>

const char *JQ_EVENT_NAMES[6] = {
    "Junqi.Move", "Junqi.Eat", "Junqi.Bomb", "Junqi.Flag", "Junqi.Dead", "Junqi.Move"
};

/* ---------- 走子路径 ---------- */
/* 该格能否被"经过"（非子、非行营、非大本营） */
static bool walkable(const jq_state *st, int row, int col, uint8_t mover)
{
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) return false;
    const jq_cell *c = &st->cells[cell_index(row, col)];
    if (c->rank != RANK_EMPTY) return false;
    if (is_camp(row, col)) return false;   /* 行营非铁路，不通行（L49） */
    if (is_base(row, col)) return false;
    (void)mover;
    return true;
}

/* 工兵沿铁路任意路径（可拐弯）：BFS，横向仅 row5/6 中线 */
static bool rail_path_engineer(const jq_state *st, int fr, int fc, int tr, int tc)
{
    if (fr == tr && fc == tc) return true;
    static const int dr[4] = {1,-1,0,0}, dc[4] = {0,0,1,-1};
    int visited[BOARD_ROWS][BOARD_COLS];
    memset(visited, 0, sizeof(visited));
    int qr[64], qc[64], head = 0, tail = 0;
    qr[tail] = fr; qc[tail] = fc; tail++;
    visited[fr][fc] = 1;
    while (head < tail) {
        int r = qr[head], c = qc[head]; head++;
        for (int k = 0; k < 4; k++) {
            int nr = r + dr[k], nc = c + dc[k];
            if (nr < 0 || nr >= BOARD_ROWS || nc < 0 || nc >= BOARD_COLS) continue;
            if (visited[nr][nc]) continue;
            if (nc != c && r != 5 && r != 6) continue;   /* 横向仅中线 */
            if (!is_rail(nr, nc)) continue;
            if (nr == tr && nc == tc) return true;
            if (!walkable(st, nr, nc, 0)) continue;
            visited[nr][nc] = 1;
            qr[tail] = nr; qc[tail] = nc; tail++;
        }
    }
    return false;
}

/* 非工兵沿铁路直走（不拐弯）：同列任意行（纵向贯穿）或同行 row5/6 */
static bool rail_path_straight(const jq_state *st, int fr, int fc, int tr, int tc)
{
    if (fr != tr && fc != tc) return false;
    /* ★ 对抗审计 #5：放行双方底线（row0/11）横滑（board.c:40 is_rail 已标底线铁路），
       旧版 `fr != 5 && fr != 6` 误拒底线横滑 → 与 is_rail 矛盾。row5/6 中线仍放行。 */
    if (fr == tr && fr != 5 && fr != 6 && fr != 0 && fr != 11) return false;
    if (!is_rail(fr, fc) || !is_rail(tr, tc)) return false;
    int sr = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
    int sc = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
    int r = fr + sr, c = fc + sc;
    while (r != tr || c != tc) {
        if (!walkable(st, r, c, 0)) return false;
        r += sr; c += sc;
    }
    return true;
}

/* ---------- 走子合法性（0x39e14 目标类型 + 路径 + 暗棋/行营/大本营约束） ---------- */
bool jq_can_move(const jq_state *st, int from, int to)
{
    if (st->stage != STG_PLAY || from == to)
        return false;
    if (from < 0 || from >= N_CELLS || to < 0 || to >= N_CELLS) return false;
    jq_cell *src = &st->cells[from];
    jq_cell *dst = &st->cells[to];
    if (src->rank == RANK_EMPTY) return false;
    int sr = rank_norm(src->rank);
    int dr_ = rank_norm(dst->rank);
    /* ★ 回合校验（用户指正"各按钮点击业务功能要对齐"）：只能走当前 turn 方棋子。
       缺失此校验 → 两人热座可连走一方/人机可走 AI 回合/蓝牙可抢走，破坏回合交替。
       暗棋 side=0xFF 未定子翻棋后才定 side（ruleflip），此处 src->side 已定。 */
    if (src->side != st->turn) return false;
    /* 军旗/地雷不能动 */
    if (sr == RANK_FLAG || sr == RANK_MINE) return false;
    /* 源格在大本营：进入大本营的棋子不能再动（rulelay 原文）。
       ★ P1（datah_012）：大本营开关（ui_rule_cfg_get(1)，默认 1 对齐原版默认）：
       =0 时进大本营的棋子可再动。 */
    if (ui_rule_cfg_get(1) && is_base(cell_row(from), cell_col(from))) return false;
    /* 暗棋：未翻开不可动（side 未定/已定但 revealed=0） */
    if (!src->revealed) return false;
    if (src->side == 0xFF) return false;
    uint8_t me = src->side;
    /* 目标被己方子占据 → 不可 */
    if (dst->rank != RANK_EMPTY && dst->side == me) return false;
    /* 行营安全岛：敌方棋子不能吃行营中的棋子（rulelay 原文）；
       空行营可走（一步相邻，行营非铁路不可穿行滑行） */
    if (is_camp(cell_row(to), cell_col(to)) && dst->rank != RANK_EMPTY &&
        dst->side != me)
        return false;
    int fr = cell_row(from), fc = cell_col(from), tr = cell_row(to), tc = cell_col(to);
    /* 一步相邻（公路线）：行营落点仅允许一步相邻（非铁路） */
    int adjacent = (fr == tr && (fc - tc == 1 || tc - fc == 1)) ||
                   (fc == tc && (fr - tr == 1 || tr - fr == 1));
    if (is_camp(tr, tc) && !adjacent) return false;
    /* 目标在大本营：仅能吃军旗/地雷（翻棋模式）或其他子（明棋"任何棋子扛旗"） */
    if (is_base(cell_row(to), cell_col(to))) {
        if (dst->rank == RANK_EMPTY) {
            /* 大本营空格：只有对方军旗/地雷可走（布局），否则不可进 */
            return false;
        }
    }
    /* 一步相邻（公路线） */
    if (adjacent)
        return true;
    /* 铁路滑行（行营/大本营已在前面排除） */
    if (sr == 9)      /* 工兵（rank 9）可拐弯 */
        return rail_path_engineer(st, fr, fc, tr, tc);
    return rail_path_straight(st, fr, fc, tr, tc);
}

/* ---------- 走子执行（0x39f20 + 0x3a15c 语义） ---------- */
/* 执行走子。成功返回事件类型；失败返回 -1（不切 turn、不计数，L54）。 */
int jq_try_move(jq_state *st, int from, int to)
{
    if (!jq_can_move(st, from, to)) return -1;
    jq_cell *src = &st->cells[from], *dst = &st->cells[to];
    int sr = rank_norm(src->rank);
    int dr_ = rank_norm(dst->rank);
    uint8_t me = src->side;

    jq_event ev;
    if (dst->rank == RANK_EMPTY) {
        /* 空目标：互换 rank（0x3a15c：from←to, to←from，to=0） */
        *dst = *src;
        memset(src, 0, sizeof(*src));
        ev = EV_MOVE;
        st->moves_since_eat++;
    } else if (sr == RANK_BOMB || dr_ == RANK_BOMB) {
        /* 炸弹与任何棋子相遇双方消失（rulelay："包括军旗"） */
        memset(src, 0, sizeof(*src));
        memset(dst, 0, sizeof(*dst));
        st->pieces_left[me]--;
        if (dst->side != me) st->pieces_left[dst->side]--;
        ev = EV_BOMB;
        st->moves_since_eat = 0;
    } else if (dr_ == RANK_FLAG) {
        /* 吃旗胜：任何棋子都能扛军旗（0x30260 → Junqi.Flag）。
           ★ P1（datah_012）：吃军旗开关（ui_rule_cfg_get(3)，默认 1 对齐原版默认）：
           =0 时扛旗不判胜（棋子入位但局终不置，继续对局）。 */
        *dst = *src;
        memset(src, 0, sizeof(*src));
        ev = EV_FLAG;
        if (ui_rule_cfg_get(3))
            jq_on_flag_capture(st, me);
        st->turn = 1 - me;
        st->total_moves++;
        st->moves_since_eat = 0;
        return ev;
    } else if (dr_ == RANK_MINE) {
        if (sr == 9 && ui_rule_cfg_get(2)) {
            /* 工兵挖雷（rank 小=强，9 工兵 > 11 地雷）：工兵留、雷消失。
               ★ P1（datah_012）：吃地雷开关（ui_rule_cfg_get(2)，默认 1 对齐原版默认）：
               =0 时工兵也不能吃地雷（地雷无敌，工兵撞雷=送死）。 */
            /* 工兵挖雷（rank 小=强，9 工兵 > 11 地雷）：工兵留、雷消失 */
            *dst = *src;
            memset(src, 0, sizeof(*src));
            st->pieces_left[1 - me]--;
            if (src->side != 0xFF) st->mines_cleared[me]++;
            ev = EV_MINE;
        } else {
            /* 非工兵撞雷：地雷>其他所有棋，攻击子被炸 */
            memset(src, 0, sizeof(*src));
            st->pieces_left[me]--;
            ev = EV_DEAD;
        }
        st->moves_since_eat = 0;
    } else {
        /* 普通吃子比较：rank 小=强（L52 确证） */
        if (sr < dr_) {              /* 大压小：src 吃 dst */
            *dst = *src;
            memset(src, 0, sizeof(*src));
            st->pieces_left[1 - me]--;
            ev = EV_EAT;
        } else if (sr > dr_) {      /* src 被吃 */
            memset(src, 0, sizeof(*src));
            st->pieces_left[me]--;
            ev = EV_DEAD;
        } else {                    /* 同级同归于尽（军棋标准：同级互杀） */
            memset(src, 0, sizeof(*src));
            memset(dst, 0, sizeof(*dst));
            st->pieces_left[me]--;
            st->pieces_left[1 - me]--;
            ev = EV_BOMB;
        }
        st->moves_since_eat = 0;
    }
    st->turn = 1 - me;
    st->total_moves++;
    /* 和棋条件（rulelay：70 步无吃子 / 总 1000 步，0x30280）。
       ★ P1（datah_012）：和棋开关（ui_rule_cfg_get(0)，默认 1 对齐原版默认）：
       =0 时关闭 70/1000 步判和。 */
    if (ui_rule_cfg_get(0) && st->stage == STG_PLAY &&
        (st->moves_since_eat >= 70 || st->total_moves >= 1000))
        jq_on_draw(st);
    /* 困毙判定（0x302b4：无棋可走 → 该方负） */
    if (st->stage == STG_PLAY && jq_side_stalemated(st, st->turn))
        jq_on_stalemate(st);
    return ev;
}

/* ---------- 翻棋（0x39e14 同格判定，§15/L48/L50） ---------- */
bool jq_can_flip(const jq_state *st, int idx)
{
    if (idx < 0 || idx >= N_CELLS || st->stage != STG_PLAY) return false;
    const jq_cell *c = &st->cells[idx];
    int r = rank_norm(c->rank);
    if (c->rank == RANK_EMPTY || c->revealed) return false;
    if (r == RANK_FLAG) return false;   /* 军旗不可翻（§15：from==to 永假） */
    if (c->side == 0xFF) return true;   /* 暗棋未定子可翻（先手定色，ruleflip） */
    /* ★ 回合校验：己方暗子（side==turn）可翻；敌方暗子（side!=turn）不可翻。
       缺失此校验 → 任意回合可翻任意方暗子，破坏暗棋回合交替。 */
    return c->side == st->turn;         /* 己方暗子可翻 */
}

bool jq_flip(jq_state *st, int idx)
{
    if (!jq_can_flip(st, idx)) return false;
    jq_cell *c = &st->cells[idx];
    /* 暗棋先手翻子定色（ruleflip 原文：先手翻出第一个棋子确定该玩家持此颜色） */
    if (c->side == 0xFF) c->side = st->turn;
    c->revealed = 1;
    st->turn = 1 - st->turn;
    st->total_moves++;
    st->moves_since_eat++;
    /* ★ P1（datah_012）：和棋开关守卫（同 jq_try_move，默认 1 对齐原版默认） */
    if (ui_rule_cfg_get(0) &&
        (st->moves_since_eat >= 70 || st->total_moves >= 1000))
        jq_on_draw(st);
    return true;
}

/* ---------- 困毙（0x302b4，L53：当前方无子可动/可翻 → 该方负） ---------- */
bool jq_side_stalemated(const jq_state *st, uint8_t side)
{
    if (st->stage != STG_PLAY || st->winner >= 0) return false;
    /* 还有未翻暗子可翻 → 未困毙 */
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        if (c->rank != RANK_EMPTY && !c->revealed &&
            rank_norm(c->rank) != RANK_FLAG &&
            (c->side == 0xFF || c->side == side))
            return false;
    }
    /* 有已翻可动子且存在合法目标 → 未困毙 */
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        if (c->rank == RANK_EMPTY || c->side != side || !c->revealed) continue;
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        for (int t = 0; t < N_CELLS; t++)
            if (t != i && jq_can_move(st, i, t)) return false;
    }
    return true;
}

/* ---------- 悔棋（本地快照；联机由 Regret* 事件驱动，L77） ---------- */
void jq_undo_snapshot(const jq_state *st)
{
    /* 快照由调用方维护（main.c / jni_bridge.c 有独立栈）；
       原版 so 无内建悔棋栈，联机 RegretReq/Agree 事件驱动。 */
    (void)st;
}

void jq_undo_restore(jq_state *st, const jq_state *snap)
{
    *st = *snap;
}

int jq_snapshots_left(const jq_state *st)
{
    (void)st;
    return 0;   /* 快照栈在调用方 */
}

/* ---------- 胜负（0x30200 分发，winner: 0=蓝胜 1=红胜 2=和） ---------- */
void jq_on_flag_capture(jq_state *st, uint8_t captor_side)
{
    st->winner = captor_side;        /* 吃旗方胜 */
    st->stage = STG_OVER;
}

void jq_on_draw(jq_state *st)
{
    st->winner = 2;
    st->stage = STG_OVER;
}

void jq_on_stalemate(jq_state *st)
{
    st->winner = 1 - st->turn;      /* 无棋可走方负，对方胜 */
    st->stage = STG_OVER;
}

/* AI 贪心评分（对抗审计 #2 提取：设备端 ai_pick 用原始编码 rank 比较的 bug 修复 + 可单测化）。
   src/dst 为原始存储 rank（红方 = 实际+0xc，需 rank_norm 还原）。
   返回：1000 吃子 / 900 同级 / 800 炸弹 / 700 工兵挖雷 / 2 空移 / 1 送死 / -1 旗雷不可走。 */
int ai_score(int src_rank_raw, int dst_rank_raw)
{
    int r = rank_norm(src_rank_raw);
    if (r == RANK_FLAG || r == RANK_MINE) return -1;
    int dr = dst_rank_raw ? rank_norm(dst_rank_raw) : 0;
    if (dst_rank_raw) {
        /* ★ 地雷特判必须在 r<dr 之前：普通子（rank<11）碰雷会先命中 r<dr → 误判"能吃"1000，
           但实际只有工兵(9)能挖雷，普通子碰雷=被雷炸死=送死（对齐原版 0x39e14 地雷 `from_row>3` 工兵）。 */
        if (dr == RANK_MINE) return (r == 9) ? 700 : 1;
        if (r < dr)          return 1000;  /* 大压小：能吃 */
        else if (r == dr)   return 900;   /* 同级互杀 */
        else if (r == RANK_BOMB) return 800;
        else return 1;       /* 吃不掉（会 DEAD）：最低分，避免主动送死 */
    }
    return 2;                /* 空移：优先于送死（AI 不主动送死，无空移才走被吃格） */
}
