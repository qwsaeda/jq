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
 * 铁路（L49/L68 + board.c 实现，AUD-R-001 方向相关修正后拓扑）：
 *   · 纵向铁路 = 仅 col2 一条纵贯线（row0..row11，行营/大本营除外）；
 *   · 横向铁路 = row0（蓝底线）/ row5 / row6（中线）/ row11（红底线）整行。
 *   工兵可拐弯（BFS 仅限 rail 格），其他子直走不能转弯；行营/大本营在铁路外。
 *   ★ AUD-R-022（P2）：原注释写"纵向仅 col2 贯穿；横向仅 row5/6"与 board.c
 *   is_rail 实现（含 row0/11 底线）矛盾，已对齐 board.c + rail_path_straight
 *   的方向相关校验（横向滑行整行须∈{0,5,6,11}；纵向滑行须 col2）。
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
/* 该格能否被"经过"（非子、非行营、非大本营）。
   ★ AUD-R-017（P1）：原 mover 形参为残留未用（编译器 -Wdiscarded-qualifiers 告警），已删。 */
static bool walkable(const jq_state *st, int row, int col)
{
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) return false;
    const jq_cell *c = &st->cells[cell_index(row, col)];
    if (c->rank != RANK_EMPTY) return false;
    if (is_camp(row, col)) return false;   /* 行营非铁路，不通行（L49） */
    if (is_base(row, col)) return false;
    return true;
}

/* 工兵沿铁路任意路径（可拐弯）：BFS，横向仅 row5/6 中线 */
static bool rail_path_engineer(const jq_state *st, int fr, int fc, int tr, int tc)
{
    if (fr == tr && fc == tc) return true;
    if (fr < 0 || fr >= BOARD_ROWS || fc < 0 || fc >= BOARD_COLS) return false;  /* AUD-R-025：起点越界自检 */
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
            if (!walkable(st, nr, nc)) continue;
            visited[nr][nc] = 1;
            qr[tail] = nr; qc[tail] = nc; tail++;
        }
    }
    return false;
}

/* 非工兵沿铁路直走（不拐弯）：同列任意行（纵向贯穿）或同行 row5/6 */
static bool rail_path_straight(const jq_state *st, int fr, int fc, int tr, int tc)
{
    if (fr != tr && fc != tc) return false;   /* 必须同行或同列 */
    if (!is_rail(fr, fc) || !is_rail(tr, tc)) return false;
    /* ★★★ 对抗审计 AUD-R-001（P0）修正：铁路**方向相关**。
       原版军棋铁路（12×5 盘）：
         · 纵向铁路 = **仅 col2 一条纵贯线**（row0..row11，行营/大本营除外）
         · 横向铁路 = row0（蓝底线）/ row5 / row6（中线）/ row11（红底线）整行
       旧实现只校验**端点** is_rail → 底线（row0/11 整行是铁路）与 col0/col4（非纵线）
       组合出**跨底线纵向超长滑行**：(11,0)→(0,0) 一步从红底线滑到蓝底线（非法）。
       旧版立论「底线不设铁路则底角棋子困毙误判」经 20000 局 A/B 实证**证伪**
       （1848 = 1848，零影响）→ 可安全收紧。
       现按方向校验：横向滑行 → 整行须是横向铁路行；纵向滑行 → 须在 col2 纵线上。 */
    if (fr == tr) {
        /* 横向滑行：整行必须是横向铁路行 */
        if (!(fr == 0 || fr == 5 || fr == 6 || fr == 11)) return false;
    } else {
        /* 纵向滑行：整列必须是纵向铁路（唯一纵贯线 col2） */
        if (fc != 2) return false;
        /* 纵线中段：行营（(3,2)/(8,2)）在铁路外（is_rail 判端点已排），
           非工兵直走不能拐弯 → 纵线上遇到行营/大本营阻断滑行 */
        int rsr = rank_norm(st->cells[cell_index(fr, fc)].rank);
        for (int rr = fr + ((tr > fr) ? 1 : -1);
             rr != tr; rr += (tr > fr) ? 1 : -1) {
            if (is_camp(rr, 2) || is_base(rr, 2)) {
                if (rsr != 9) return false;  /* 工兵(rank 9) BFS 拐弯绕行 */
                break;
            }
        }
    }
    int sr = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
    int sc = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
    int r = fr + sr, c = fc + sc;
    while (r != tr || c != tc) {
        if (!walkable(st, r, c)) return false;
        r += sr; c += sc;
    }
    return true;
}

/* ---------- 走子合法性（0x39e14 目标类型 + 路径 + 暗棋/行营/大本营约束） ---------- */
/* ★ 对抗审计 AUD-R-005（P1）修正：回合校验原写死在 jq_can_move 里 →
   jq_side_stalemated(st, side≠turn) 时所有 can_move 恒 false → 困毙**误报**。
   现把回合校验参数化（move_legal 的 turn 参数），困毙判定可传任意 side。 */
static bool move_legal(const jq_state *st, int from, int to, int turn)
{
    if (st->stage != STG_PLAY || from == to)
        return false;
    if (from < 0 || from >= N_CELLS || to < 0 || to >= N_CELLS) return false;
    /* ★ AUD-R-017（P1）：const 限定对齐 const jq_state*（原非 const 指针触发
       -Wdiscarded-qualifiers；残留 dr_ 未用变量已删——走子合法性不需要目标 rank） */
    const jq_cell *src = &st->cells[from];
    const jq_cell *dst = &st->cells[to];
    if (src->rank == RANK_EMPTY) return false;
    int sr = rank_norm(src->rank);
    /* ★ 回合校验（用户指正"各按钮点击业务功能要对齐"）：只能走当前 turn 方棋子。
       缺失此校验 → 两人热座可连走一方/人机可走 AI 回合/蓝牙可抢走，破坏回合交替。
       暗棋 side=0xFF 未定子翻棋后才定 side（ruleflip），此处 src->side 已定。 */
    if (turn >= 0 && src->side != (uint8_t)turn) return false;
    /* 军旗/地雷不能动 */
    if (sr == RANK_FLAG || sr == RANK_MINE) return false;
    /* 源格在大本营：进入大本营的棋子不能再动（rulelay 原文）。
       ★ P1（datah_012）：大本营开关（ui_rule_cfg_get(1)，默认 1 对齐原版默认）：
       =0 时进大本营的棋子可再动。 */
    if (ui_rule_cfg_get(1) && is_base(cell_row(from), cell_col(from))) return false;
    /* 暗棋：未翻开不可动（side 未定/已定但 revealed=0）
       ★ AUD-R-006（P1）：未定色暗子（side=0xFF）不可被直接攻击（ruleflip 原文：
       只能翻、不能吃；兄弟实现 game_logic.c 同判定）。明棋/布局模式下 0xFF 不可达，
       行为不变；明棋已翻子 revealed=1 恒放行。 */
    if (!src->revealed || src->side == SIDE_UNKNOWN) return false;
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
        /* ★ 对抗审计 AUD-R-007（P1）修正：ruleflip（暗棋）原文 —
           「行营、大本营是个安全岛，进入以后，敌方棋子不能吃其中的棋子；
             大本营的地雷、军旗可以吃」。
           旧版只把**行营**当安全岛，大本营内的普通子可被直接吃掉 → 违反暗棋规则。
           明棋（rulelay）原文只把行营当安全岛 → 本条仅在暗棋模式生效。
           ★ 根因修正（2026-11-07）：mode 判定信号 = src->revealed（非 dst、非
           ui_opt_get_mode 运行时全局态）：明棋布局全子 revealed=1（含走到大本营的
           子），暗棋布局未翻子 revealed=0。dst 已进大本营时其 revealed 位不可靠
           （大本营是落点，非棋子自身状态）→ 误拦吃旗（evverify flag-on FAIL 根因：
           蓝 rank2 走进大本营后 revealed=0 → d_rank=军旗 被误判"非雷非旗"拦拒）。
           明棋：src->revealed 恒 1 → 检查恒放行，行为与 HEAD 完全一致。 */
        {
            int d_rank = rank_norm(dst->rank);
            if (src->revealed == 0 && d_rank != RANK_MINE && d_rank != RANK_FLAG)
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

/* 公开 API：当前回合的走子合法性（保持既有语义） */
bool jq_can_move(const jq_state *st, int from, int to)
{
    return move_legal(st, from, to, st->turn);
}

/* ★ 内部用：忽略回合限制的走子合法性（困毙判定用，turn 传 -1） */
static bool can_move_ignore_turn(const jq_state *st, int from, int to)
{
    return move_legal(st, from, to, -1);
}

/* ★ AUD-R-018（P1）：「杀光对方所有可动子」判据（rulelay L9/L16 三条胜负之一）。
   明棋（全员 revealed=1）下与困毙判定精确等价（可动子存在 ⇔ 不困毙），
   无需独立分支；暗棋（存在未翻暗子）下困毙逻辑先扫未翻子 → 不判负，
   "杀光可动子"的即时判负未独立实现。★ 诚实边界：当前靠困毙近似覆盖，
   完整实现依赖 AUD-R-002/003（暗棋规则族 + 先手翻子定色）落地后补。 */

/* ---------- 走子执行（0x39f20 + 0x3a15c 语义） ---------- */
/* ★ AUD-R-008（P1）：走子/翻棋共用的终局收口（顺序对齐原 jq_try_move：先和棋后困毙）。
   stage 已 OVER（吃旗/炸旗终局）时双守卫自动放行，不动终局 winner。 */
static void stalemate_after(jq_state *st)
{
    if (st->stage != STG_PLAY) return;
    if (ui_rule_cfg_get(0) &&
        (st->moves_since_eat >= 70 || st->total_moves >= 1000))
        jq_on_draw(st);
    /* 困毙判定（0x302b4：无棋可走 → 该方负） */
    if (st->stage == STG_PLAY && jq_side_stalemated(st, st->turn))
        jq_on_stalemate(st);
}

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
        /* 炸弹与任何棋子相遇双方消失（rulelay L12 原文：「炸弹与任何棋子相遇时，
           双方都消失，包括军旗」）。
           ★ 对抗审计 AUD-R-009/AUD-R-020（P1）修正：旧版撞旗后只是清空两格，
           **不产生任何终局信号** → 军旗没了但局继续（旗被毁 = 对方失去军旗 = 判负，
           rulelay L16「军旗被扛…判负」）。现补判胜：扛旗方（用炸弹的一方）获胜。
           诚实边界：原版在「炸弹撞军旗」时是否判扛旗方胜，尚未从 32 位 so 反汇编确证
           （当前依据为 rulelay 规则文本 L12+L16 推断）；若原版另有判定，需按原版调整。 */
        int dst_side = dst->side;
        int dst_rank = dr_;
        memset(src, 0, sizeof(*src));
        memset(dst, 0, sizeof(*dst));
        st->pieces_left[me]--;
        /* ★ 对抗审计修正（AUD-R-004 P0）：dst->side 可能是暗棋未定态 0xFF
           （junqi.h SIDE_UNKNOWN）→ pieces_left[255] 越界写 1020 字节外，
           破坏 jq_state 相邻成员。加范围守卫。 */
        if (dst_side != me && SIDE_VALID(dst_side)) st->pieces_left[dst_side]--;
        ev = EV_BOMB;
        st->moves_since_eat = 0;
        /* 炸弹炸毁军旗 → 被炸方失去军旗 → 扛旗方（己方 me）胜 */
        if (dst_rank == RANK_FLAG && SIDE_VALID(dst_side)) {
            st->winner = (int)me;         /* winner: 0=蓝胜 1=红胜；me=用炸弹的一方=胜方 */
            st->stage = STG_OVER;
        }
    } else if (dr_ == RANK_FLAG) {
        /* 吃旗胜：任何棋子都能扛军旗（0x30260 → Junqi.Flag）。
           ★ P1（datah_012）：吃军旗开关（ui_rule_cfg_get(3)，默认 1 对齐原版默认）：
           =0 时扛旗不判胜（棋子入位但局终不置，继续对局）。
           ★ AUD-R-016（P1）：开关关闭时不切 turn/不计数（与 EV_MOVE 空移语义一致的
           "走子后收口"路径），但事件码保留 EV_FLAG（棋子确实落在军旗格，音效分类
           语义不变；evverify flag-off 夹具 53→58 期望 stage=PLAY winner=-1）。
           诚实边界：开关关闭时旗被吃而局不终，后续胜负只能靠困毙/吃光可动子近似，
           与 rulelay L16 语义存在差距（原文该开关语义源自原版 4 开关 UI，具体关闭后
           行为未从 32 位 so 确证；此处保持"走子语义但终局不置"最小改动）。 */
        *dst = *src;
        memset(src, 0, sizeof(*src));
        ev = EV_FLAG;
        if (ui_rule_cfg_get(3)) {
            jq_on_flag_capture(st, me);
            st->turn = 1 - me;
            st->total_moves++;
            st->moves_since_eat = 0;
            return ev;   /* 终局：跳过后续收口（stage=OVER 守卫也会放行） */
        }
        st->turn = 1 - me;
        st->total_moves++;
        st->moves_since_eat = 0;
        /* 开关关闭：走子正常收口（困毙判定可在此触发，与 EV_MOVE 一致）。
           AUD-R-008 后行为：flag-off 后 stalemate_after 判定 RED 困毙（RED 只剩 58 军旗
           不可动），winner=BLUE stage=OVER。evverify 夹具期望相应更新。 */
        stalemate_after(st);
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
            /* ★ AUD-R-039（P2）：src 已 memset 后 side 恒 0，原 `!= 0xFF` 死判断已删
               （挖雷计数无条件递增，符合 ruleflip）。
               ★ AUD-R-015（P1）：mines_cleared 仅记录——"工兵挖 3 雷才能扛旗"
               胜负判定未接线（待暗棋规则族实现，见 AUD-R-002 修法；
               保留数据供后续暗棋族使用，勿删除该字段）。 */
            st->mines_cleared[me]++;
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
    /* 走子后收口（和棋+困毙；AUD-R-008 对称，stalemate_after 与 jq_flip 共用） */
    stalemate_after(st);
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
    if (c->side == SIDE_UNKNOWN) c->side = st->turn;
    c->revealed = 1;
    st->turn = 1 - st->turn;
    st->total_moves++;
    st->moves_since_eat++;
    /* ★ AUD-R-008（P1）：翻棋路径与走子路径对称收口（和棋+困毙）。
       原版 game_logic.c:309 的 jq_flip 末尾 check_stalemate(st) 同款语义。 */
    stalemate_after(st);
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
        /* ★ AUD-R-005：必须用忽略回合的版本（否则 side≠turn 时恒 false → 困毙误报） */
        for (int t = 0; t < N_CELLS; t++)
            if (t != i && can_move_ignore_turn(st, i, t)) return false;
    }
    return true;
}

/* ---------- 悔棋（本地快照；联机由 Regret* 事件驱动，L77） ---------- */
/* ★ AUD-R-012（P1）：引擎内未实现快照栈（诚实标注）。
   实际悔棋由 jni_bridge.c 的 g_undo[UNDO_MAX] 环形栈承担（undo_push/undo_pop，
   走子/翻棋前整体拷贝 jq_state），原版 so 无内建悔棋栈。
   本函数为头文件契约占位（"走子前调用"），当前空实现；
   调用方应直接维护自身快照，勿依赖此 API。 */
void jq_undo_snapshot(const jq_state *st)
{
    (void)st;
}

void jq_undo_restore(jq_state *st, const jq_state *snap)
{
    *st = *snap;
}

/* ★ AUD-R-013（P1）：引擎内无快照栈 → 恒 0（快照栈在调用方）。
   蓝牙 UNDO 帧消费点改走 jni_bridge 暴露的 jq_bt_undo_available()。 */
int jq_snapshots_left(const jq_state *st)
{
    (void)st;
    return 0;   /* 快照栈在调用方（jni_bridge g_undo） */
}

int  jq_bt_undo_available(void);   /* bt_session.c BT_T_UNDO 分支用（jni_bridge.c 实现；bttest 桩在 bt_selftest.c） */

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
