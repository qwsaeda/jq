/*
 * ★ AUD-R-138（P3）：本文件（32 位翻译体）与 rev64/forward/src_rs/rules.c（64 位重写）分叉自
 * commit 401a735（§124y-19 会话 B 规则引擎移植进 32 位 so 走子桥）。两者实现层级不同
 * （汇编级翻译 vs C 级重写），不做强求 1:1 代码同步。
 * 同步机制 = 行为一致性闸门：
 *   · 32 位走子桥基准 = rev64/tools/arm32_walkregress.sh（4 路径回归，F_3ab80 判据）
 *   · 64 位基准 = evverify 167/167 + regress.py 50/50 + consistency 9/9
 *   · 分叉检测 = rev64/tools/rules_fork_check.py（列出 src_rs 已修 AUD-R 项在翻译体侧的实证状态）
 * 新 AUD-R 修复需同时验证两条路线基准（32 位 arm32_walkregress + 64 位 evverify），
 * 单条路线 PASS 不代表另一条路线对齐。翻译体侧规则修复须走 32 位反汇编路线
 * （§124y 实证），不搬移 src_rs 的 C 修复（层级不同，搬了也是语义近似）。
 *
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
#include "jq_rs_rules.h"
#define JQ_RS_DEF __attribute__((visibility("default")))

#include <string.h>
#include <stdlib.h>

/* §124y-161 AUD-R 分叉标记：事件名表（会话 B 移植）。
   · AUD-R-061 EV_FLIP=6 事件契约 → 32 位侧本表 6 项无 EV_FLIP 通道（翻子事件走子桥
     仅 logcat）→ 分叉（64 位独有设计）
   · AUD-R-062 EV_MINE/EV_MOVE 同名"Junqi.Move" → 32 位侧表 [5]="Junqi.Move"
     与 64 位同名设计对齐（无按码分流统计）→ 对齐
   32 位 .rodata 明文 Junqi.Flip 事件名存在，但走子桥未实装该事件通道，
   细分规则 32 位侧未逐条实证，诚实边界留待后续专项。 */
const char *JQ_EVENT_NAMES[6] = {
    "Junqi.Move", "Junqi.Eat", "Junqi.Bomb", "Junqi.Flag", "Junqi.Dead", "Junqi.Move"
};

/* ---------- 走子路径 ---------- */
/* §124y-166 AUD-R 分叉标记：走子路径族（会话 B 移植，对应 32 位 0x39e14 铁路判定）。
   与 64 位 src_rs/rules.c 的 AUD-R 分叉项对照（诚实标注，非实证对齐）：
   · R-025 rail_path_engineer 起点越界 → 32 位侧调用方 jq_can_move 已前置
     from∈[0,N_CELLS) 检查（越界 from 到不了 rail_path_engineer）→ 行为等价（守卫前置）
   · R-026 BFS 队列 64 槽无上界断言 → 32 位侧同 64 位（棋盘 60 格固定，64 槽天然够），
     未加断言 → 分叉（64 位独有设计：显式断言）
   · R-027 walkable mover 参数未使用 → 32 位侧同样 (void)mover 未用 → 对齐（同源设计）
   · R-023 头注释"行营/大本营在铁路外"争议 → 32 位侧头注释同 64 位（"行营/
     大本营在铁路外"），与标准军棋中心行营在纵路的争议同源，32 位侧未专项实证
     → 分叉（同源争议，未实证）
   · R-098 walkable mover 全调用点传 0 → 32 位侧同（BFS 无 mover 维度）→ 对齐
   · R-099 方向表函数体内 static → 32 位侧同（dr/dc 数组在函数体内）→ 对齐
   · R-100 visited 240B 栈可压缩 → 32 位侧同布局（int visited[12][5]），未压缩 → 对齐（同源）
   · R-101 fr==tr&&fc==tc 提前返 true → 32 位侧同（rail 两函数首行同判）→ 对齐
   · R-102 jq_can_move stage 检查顺序 → 32 位侧 stage 与 from==to 同行合并判断 → 分叉（行序差异，行为等价）
   走子桥基准 = arm32_walkregress 4/4 PASS；细分 32 位侧未逐条实证，诚实边界。 */
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

/* §124y-166 AUD-R 分叉标记：走子合法性别（会话 B 移植，对应 32 位 0x39e14 语义）。
   · R-007 暗棋大本营非安全岛 → 32 位侧走子桥布局写死 side 分区（明棋藏脸），
     暗棋模式维度未实装 → 分叉（64 位独有设计）
   · R-028 敌方一步进空行营 → 32 位侧同 64 位旧语义（行营落点允许一步相邻），
     game_logic.c 兄弟实现结论相反 → 分叉（32 位侧未同步兄弟结论，未实证）
   · R-029 大本营目标判定未分雷/旗 → 32 位侧空大本营一律拒，
     未区分大本营内雷/旗可吃 → 分叉（64 位独有设计）
   · R-030 进大本营不可再动仅源格 → 32 位侧同（仅源格判），未拦再进 → 分叉（同源简化）
   · R-031 吃雷开关语义未记录 → 32 位侧走子桥无规则开关维度（原版 32 位 so 开关
     在 .bss 事件表，未提取）→ 分叉（64 位独有设计）
   · R-032 4 个规则开关直接调 UI 层函数（规则/UI 耦合）→ 32 位侧走子桥无规则开关
     维度（jq_can_move 无开关入参，纯 rank 规则判定）→ N/A（层级不适用，32 位侧
     压根无开关→UI 调用链）
   · R-070 相邻判定两次减法 → 32 位侧同写法 → 对齐（同源）
   · R-071 空大本营全拒 → 32 位侧同→ 对齐（同源简化，未同步 64 位修法）
   · R-072 源在行营可自由走出未记录 → 32 位侧同（无源格行营拦截）→ 对齐（同源）
   · R-103 dr_ 未使用 → 32 位侧同（dr_ 归一化未消费）→ 对齐（同源）
   · R-105 一步进敌方空行营术语对齐 → 32 位侧同 64 位语义 → 对齐（同源）
   32 位侧 jq_can_move 无 const 限定丢弃告警（32 位翻译体 st 非 const 入参），R-017 不适用。 */
/* ---------- 走子合法性（0x39e14 目标类型 + 路径 + 暗棋/行营/大本营约束） ---------- */
/* §124y-161 AUD-R 分叉标记：走子合法性（会话 B 移植）。
   · AUD-R-001 铁路方向（跨底线纵滑 1061 条非法）→ 32 位侧走子桥基准
     arm32_walkregress 4/4 PASS 覆盖了主链走子，但**未逐条验证铁路方向**
     → 分叉（64 位独有设计，32 位侧未专项实证）
   · AUD-R-005 困毙参数化 → 32 位侧走子桥困毙走实时扫描
     （jq_side_stalemated，仍用带回合门的 jq_can_move(st,i,t) 枚举，
     依赖调用侧==turn 前提；64 位已修 can_move_ignore_turn 参数化（turn=-1 任意侧
     可查），32 位未移植）→ 分叉（★ 对抗审计 P1 批2 B-2 纠正：原"等价"判读只在
     侧==turn 调用子集成立，API 契约不等价——32 位 side≠turn 时恒返回 true 困毙误报）
   · AUD-R-024 工兵 BFS 横向行集合 → 32 位侧走子桥横向仅 row5/6，
     64 位已与 R-001 方向校验对齐为 row0/5/6/11 整行 → 分叉（64 位独有设计）
   · AUD-R-006 暗棋信息不泄露 → 32 位侧走子桥布局写死 side 分区（"明棋藏脸"），
     暗棋信息遮蔽未实装 → 分叉（64 位独有设计）
     ★ 对抗审计 P0（批2 F-3 漏标）：32 位 jq_can_move 只拦 dst->side==me
     （己方），**不拦 dst->side==0xFF**（未定色暗子目标）——而 32 位 jq_can_flip
     存在 0xFF 未定子分支、jq_flip 有 0xFF→turn 定色，即 0xFF 格
     可达；此时任意回合方可按真 rank 吃掉未定色暗子，与 64 位已修 AUD-R-006
     （dst->side==0xFF → false）语义分叉 → 分叉（64 位已修，32 位未同步）
   走子桥基准 = arm32_walkregress 4/4 PASS，覆盖走子/翻棋/终局主链；
   细分规则 32 位侧未逐条实证，诚实边界留待后续专项。 */
bool JQ_RS_DEF jq_can_move(const jq_state *st, int from, int to)
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
    /* 源格在大本营：进入大本营的棋子不能再动（rulelay 原文） */
    if (is_base(cell_row(from), cell_col(from))) return false;
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
/* §124y-161 AUD-R 分叉标记：走子执行器（会话 B 移植，对应 src_rs 0x39f20/0x3a15c 语义）。
   与 64 位 src_rs/rules.c jq_try_move 的 AUD-R 分叉项对照：
   · 事件码 0-5 同名契约（Junqi.Move/Eat/Bomb/Flag/Dead/Move）→ 对齐
   · AUD-R-061 EV_FLIP=6 事件契约 → 32 位侧走子桥未实现翻子事件通道（走子桥
     jq_walk_bridge_flip 仅 logcat，无事件码）→ 分叉（64 位独有设计）
   · AUD-R-020 炸弹撞旗判胜 → 32 位侧 jq_try_move 的 EV_BOMB 分支只清两格 + 减计数，
     **不检查 dst 是否军旗、不置 winner/stage**——炸弹撞旗后旗被毁但局继续
     （与 AUD-R-020 描述的旧 bug 同）；64 位 rules.c EV_BOMB 内已加
     `if (dr_ == RANK_FLAG) { winner=me; stage=OVER; }`（扛旗方胜）→ 分叉
     （64 位独有设计；★ 对抗审计 P1 批2 B-4 纠正：原标记引"走子桥认输逻辑
     winner=1-turn"与 AUD-R-020 无关且掩盖"32 位 EV_BOMB 无炸弹撞旗判胜分支"实况）
   · AUD-R-004 pieces_left 越界写守卫 → 32 位侧走子桥维护 pieces_left（EV_BOMB/EV_EAT/
     EV_DEAD 分支），但`if (dst->side != me) st->pieces_left[dst->side]--;`
     **无 SIDE_VALID 守卫**——dst->side=0xFF（未定色暗子）时 pieces_left[255]--
     越界写 1020 字节，AUD-R-004 P0 越界写 bug 在 32 位同样存在且未修 → 分叉
     （64 位已加 SIDE_VALID(dst_side) 守卫，32 位未同步；32 位布局写死分区时
     dst->side 恒 0/1 不触发，暗棋路径可达）
   · AUD-R-073 困毙翻棋排除 RANK_MINE → 32 位侧 jq_side_stalemated 未排除地雷
     （翻出地雷视为仍可行动）→ 分叉（64 位独有设计）
   走子桥基准 = arm32_walkregress 4/4 PASS（F_3ab80 ok=1），覆盖走子/翻棋/
   终局主链；细分规则（吃旗/炸弹撞旗/和棋 70 步/1000 步上限）32 位侧未
   逐条实证，诚实边界留待后续专项。 */
/* §124y-166 AUD-R 分叉标记：走子执行器族（会话 B 移植，对应 32 位 0x39f20/0x3a15c/0x30200）。
   · R-014 pieces_left 全线不可信 → 32 位侧走子桥仍维护 pieces_left，
     但 32 位侧消费点仅走子桥 logcat（无 AI/同步读），不可信影响面小 → 分叉（消费链不同）
   · R-016 EV_FLAG 关闭扛旗开关后事件语义 → 32 位侧无规则开关维度（见 R-031）→ 分叉（64 位独有设计）
   · R-018 杀光判据缺失 → 32 位侧困毙走实时扫描，语义等价兜底 → 行为等价
   · R-033 和棋 total_moves 含翻棋 → 32 位侧 jq_flip 同样累加
     total_moves/moves_since_eat（与 64 位 rules.c 同源口径），jq_flip 内和棋判定段
     有和棋判定 → 对齐（同源，两侧翻棋均计入总步数）
   · R-034 和棋先于困毙优先级 → 32 位侧同序（和棋先、困毙后），
     双成立时困毙覆盖和棋（后执行生效）→ 对齐（同源顺序）
   · R-035 on_draw/on_stalemate 不查 stage → 32 位侧 jq_on_draw/jq_on_stalemate
     直接写 winner=2/1-turn，无 stage 守卫；64 位三终局函数均有
     `if (st->stage != STG_PLAY) return;` 守卫 → 分叉（32 位缺 stage 守卫，
     64 位独有设计；★ 对抗审计 P0 批2 F-5 方向纠正：原标记"分叉（64 位独有设计）"
     方向写反）
   · R-036 on_stalemate 胜方由 turn 推导 → 32 位侧同→ 对齐（同源）
   · R-037 吃旗分支切 turn/累加 total_moves → 32 位侧同（吃旗后切 turn+累加）→ 对齐（同源）
   · R-038 EV_FLAG 提前 return 跳和棋困毙 → 32 位侧同（提前 return）→ 对齐（同源简化）
   · R-039 EV_MINE 死判断 src->side≠0xFF → 32 位侧同（在 memset src 后判 src->side）→ 对齐（同源）
   · R-040 EV_MINE pieces_left[1-me] 代替 dst->side → 32 位位侧同→ 对齐（同源）
   · R-041 try_move 不刷新 pieces_left/count_tbl → 32 位侧同（增量减，无全量刷新）→ 对齐（同源）
   · R-042 to!=from 双层防护 → 32 位侧依赖 jq_can_move 前置→ 行为等价
   · R-104 me∈{0,1} 未断言 → 32 位侧同（无断言）→ 对齐（同源）
   · R-106 空目标整格拷贝 *dst=*src → 32 位侧同→ 对齐（同源）
   · R-107 计数复位散落各分支 → 32 位侧同（各分支内各自复位）→ 对齐（同源）
   · R-108 EV_FLAG 与收尾重复切 turn → 32 位侧吃旗分支自切 turn+return，
     不重复执行通用收尾 → 对齐（同源结构）
   · R-083 事件返回与局面非原子 → 32 位侧走子桥先 can_move 后执行（失败不切 turn，
     jq_can_move 前置），主链原子；细分子分支未逐条实证 → 分叉（未实证）
   · R-084 联机同步帧 side/rank 未过滤 → 32 位侧走子桥无联机同步帧维度 → 分叉（64 位独有设计）
   · R-087 count_tbl 240B 表语义未验证 → 32 位侧走子桥无 count_tbl 消费（
     原版 .bss 0x155000+0x78 计数表，§124y-32 实锤 0x15bfec 惰性单例未构造，
     走子桥不读该表）→ N/A（层级不适用，32 位侧 count_tbl 在 .bss 事件表侧未提取）
   走子桥基准 = arm32_walkregress 4/4 PASS；细分子分支 32 位侧未逐条实证，诚实边界。 */
int JQ_RS_DEF jq_try_move(jq_state *st, int from, int to)
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
        /* AUD-1000 B05-090（32 位侧 64 位同步修复）：dst->side=0xFF（暗棋未定色暗子）
           时 pieces_left[255] 越界写 1020B（SIDE_VALID 守卫同 64 位 rules.c :478）。 */
        if (dst->side != me && SIDE_VALID(dst->side)) st->pieces_left[dst->side]--;
        ev = EV_BOMB;
        st->moves_since_eat = 0;
    } else if (dr_ == RANK_FLAG) {
        /* 吃旗胜：任何棋子都能扛军旗（0x30260 → Junqi.Flag）
           AUD-1000 B05-093（32 位侧 64 位同步修复）：炸弹撞旗 → 同归双方消失，
           扛旗方（me）胜；守卫 SIDE_VALID(dst->side) 同 64 位 rules.c。 */
        if (sr == RANK_BOMB) {
            int dst_side = dst->side;
            memset(src, 0, sizeof(*src));
            memset(dst, 0, sizeof(*dst));
            st->pieces_left[me]--;
            if (dst_side != me && SIDE_VALID(dst_side)) st->pieces_left[dst_side]--;
            ev = EV_BOMB;
            st->moves_since_eat = 0;
            /* 炸弹炸旗 → 被炸方失去军旗 → 扛旗方胜
               AUD-1000 B01-003 同源（32 位侧同步）：终局（stage=OVER）后不再切
               turn，旧实现终局快照 turn 指向胜方（战况统计误读）。 */
            if (SIDE_VALID(dst_side)) jq_on_flag_capture(st, me);
            if (st->stage == STG_PLAY) st->turn = 1 - me;
            st->total_moves++;
            return ev;
        }
        *dst = *src;
        memset(src, 0, sizeof(*src));
        ev = EV_FLAG;
        jq_on_flag_capture(st, me);
        /* AUD-1000 B01-003 同源：终局后不再切 turn */
        if (st->stage == STG_PLAY) st->turn = 1 - me;
        st->total_moves++;
        st->moves_since_eat = 0;
        return ev;
    } else if (dr_ == RANK_MINE) {
        if (sr == 9) {
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
    /* 和棋条件（rulelay：70 步无吃子 / 总 1000 步，0x30280） */
    if (st->stage == STG_PLAY &&
        (st->moves_since_eat >= 70 || st->total_moves >= 1000))
        jq_on_draw(st);
    /* 困毙判定（0x302b4：无棋可走 → 该方负） */
    if (st->stage == STG_PLAY && jq_side_stalemated(st, st->turn))
        jq_on_stalemate(st);
    return ev;
}

/* §124y-166 AUD-R 分叉标记：翻棋族（会话 B 移植，对应 32 位 0x39e14 同格判定）。
   · R-043 flip 不校验 rank/flags → 32 位侧 jq_flip 仅查空/已翻/军旗，
     无 rank 合法性/flags 维护 → 分叉（64 位独有设计）
   · R-044 翻棋后 turn 切换 → 32 位侧 jq_flip 切 turn，与 64 位同源 → 对齐（同源）
   · R-045 可翻白名单与困毙白名单不统一 → 32 位侧同（可翻=未翻且非旗，困毙排除地雷）→ 分叉（同源）
   · R-046 flags/rank 双表示 → 32 位侧同布局 → 对齐（同源）
   · R-053 winner 取值注释约定 → 32 位侧同（int8_t -1/0/1/2）→ 对齐（同源）
   · R-085 can_flip 可翻任意未定色暗子不限回合 → 32 位侧已加回合校验（jq_can_flip 回合门，★ 对抗审计 P3 批2 D-6 行号纠正），与 64 位同步修复 → 对齐（已同步）
   · R-086 翻棋消耗 moves_since_eat/total_moves → 32 位侧 jq_flip 同样
     累加（与 64 位同源口径）→ 对齐（同源；有和棋判定但无 stage 前置
     守卫——见 R-109 标记）
   · R-109 flip 和棋缺 stage 守卫 → 32 位侧 jq_flip 有和棋判定（无 stage
     前置守卫、无困毙收口）；★ 对抗审计 P1 批2 B-3 纠正：原"无和棋判定"措辞失实
     → 分叉（AUD-R-008 stalemate_after 收口未移植 + R-035 stage 守卫缺失，走子桥简化）
   · R-110 flip rank_norm 先求值 → 32 位侧同→ 对齐（同源）
   · R-111 0xFF 可翻注释与实际可达性不符 → 32 位侧同→ 对齐（同源）
   · R-140 on_stalemate 后 winner 依赖 turn∈{0,1} 无保护 → 32 位侧同→ 对齐（同源）
   走子桥基准 = arm32_walkregress 4/4 PASS（flip×2 主链）；细分 32 位侧未逐条实证，诚实边界。 */
/* ---------- 翻棋（0x39e14 同格判定，§15/L48/L50） ---------- */
bool JQ_RS_DEF jq_can_flip(const jq_state *st, int idx)
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

/* §124y-161 AUD-R 分叉标记：翻棋（会话 B 移植）。
   · AUD-R-008 翻棋收口困毙（stalemate_after）→ 32 位侧 jq_flip 末尾无
     stalemate_after 收口（走子桥简化，仅 logcat）→ 分叉（64 位独有设计）
   · AUD-R-003 先手翻子定色 → 32 位侧走子桥布局写死 side 分区（"明棋藏脸"），
     本 jq_flip 含 c->side==0xFF→turn 定色逻辑（语义与 64 位一致），但未走
     SIDE_UNKNOWN 全流程实证 → 分叉（64 位独有设计，32 位走子桥简化布局）
   走子桥基准 = arm32_walkregress 4/4 PASS（flip×2），覆盖翻棋主链；
   细分规则 32 位侧未逐条实证，诚实边界留待后续专项。 */
bool JQ_RS_DEF jq_flip(jq_state *st, int idx)
{
    if (!jq_can_flip(st, idx)) return false;
    jq_cell *c = &st->cells[idx];
    /* 暗棋先手翻子定色（ruleflip 原文：先手翻出第一个棋子确定该玩家持此颜色） */
    if (c->side == 0xFF) c->side = st->turn;
    c->revealed = 1;
    st->turn = 1 - st->turn;
    st->total_moves++;
    st->moves_since_eat++;
    if (st->moves_since_eat >= 70 || st->total_moves >= 1000)
        jq_on_draw(st);
    return true;
}

/* §124y-166 AUD-R 分叉标记：困毙族（会话 B 移植，对应 32 位 0x302b4 困毙判定）。
   · R-074 side_stalemated 隐含依赖 side==turn → 32 位侧调用点传 st->turn，
     与 64 位耦合点同源 → 对齐（同源）
   · R-075 和棋困毙判定在 turn 切换后 → 32 位侧同（切 turn 后依次和棋判定 / 困毙判定）→ 对齐（同源）
   · R-089 困毙未排除可吃旗情形 → 32 位侧同（jq_can_move 覆盖吃旗目标）→ 行为等价
   · R-090 stage 与明暗棋正交 → 32 位侧无明暗棋模式维度（走子桥写死分区）→ 分叉（64 位独有设计）
   · R-112 side_stalemated O(N²) 每步调用 → 32 位侧同（双循环），
     移动端性能风险同源 → 对齐（同源，32 位走子桥回归仅 4 路径未压测）
   · R-091 API 无 st==NULL 防护 → 32 位侧走子桥全部 API 无 NULL 防护（jq_can_move / jq_try_move / jq_side_stalemated / jq_undo_* / jq_on_*），
     与兄弟实现防御风格不一致同源 → 对齐（同源）
   32 位走子桥困毙主链实证 = arm32_walkregress 路径 3（终局判定）；
   细分 32 位侧未逐条实证，诚实边界。 */
/* ---------- 困毙（0x302b4，L53：当前方无子可动/可翻 → 该方负） ---------- */
bool jq_side_stalemated(const jq_state *st, uint8_t side)
{
    if (st->stage != STG_PLAY || st->winner >= 0) return false;

    /* ★ §124y-169（行为实证专项）R-073 分叉**实证**（rule_probe_compare.py 双端同源探针
       C4/C4b 用例 diff）：32 位"可翻"检查**未排除 RANK_MINE**（地雷翻了不可走子
       （下方已翻可动循环 RANK_MINE continue），与 64 位 AUD-R-073 排除地雷的口径分叉。
       ★ 实证对照用例 C4b：唯一暗子=军旗 → 两端均困毙=1（军旗排除两端一致）→
       分歧精确到 32 位**仅地雷**算"可行动"（军旗排除一致，地雷 32 位未排）。
       ★ 语义解读（R-138 诚实标注，非 C 移植——两侧地雷模型假设不同）：
       32 位侧 jq_can_flip **允许翻出地雷**（C3 实证：暗地雷可翻=1），且翻出后
       地雷不可动（本函数已翻可动循环 RANK_MINE continue）→ 32 位地雷语义 =
       "地雷=一次性暗雷，翻出即死"（翻了也算有行动 = 未困毙）；64 位语义 =
       "地雷翻了也不能走子"（排除出可行动，翻出无意义 = 仍困毙）。两侧对
       "地雷翻出后是否仍算可行动"判读**相反** → 该分叉是**模型假设差异**
       （32 位 1.43MB 原版是否也如此需 §124y 反汇编实证原版 0x302b4 困毙段，
       32 位翻译体此处忠实会话 B 移植未对齐 64 位 AUD-R-073 口径）。
       影响面：仅暗棋困毙判定（明棋地雷恒 revealed=1 不触发；32 位走子桥基准
       arm32_walkregress 4/4 PASS 不消费此差异）。 */
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

/* §124y-166 AUD-R 分叉标记：悔棋族（会话 B 移植，对应 32 位原版无内建悔棋栈）。
   · R-012 undo_snapshot 空实现 → 32 位侧同（jq_undo_snapshot 空实现，快照栈在调用方）→ 对齐（同源）
   · R-013 snapshots_left 恒 0 → 32 位侧同，蓝牙 UNDO 帧在 32 位侧
     由 jq_bt_ctrl_send UNDO 走调用方栈，不走本 API → 行为等价（职责分离）
   · R-049 undo_depth 无使用者且被 restore 覆盖 → 32 位侧同（jq_state 有 undo_depth 字段，
     走子桥不读）→ 对齐（同源）
   · R-050 undo_restore 整体覆盖 → 32 位侧同（*st=*snap）→ 对齐（同源）
   · R-088 无部分回滚能力 → 32 位侧同（整盘快照）→ 对齐（同源）
   · R-113 悔棋三 API 空壳却公开 → 32 位侧同（快照栈在调用方 main.c/jni_bridge，
     本文件三 API 保留为接口占位）→ 对齐（同源设计）
   32 位走子桥未实证悔棋主链（arm32_walkregress 4 路径无 UNDO）；
   蓝牙 UNDO 帧语义 64 位侧 bt_semtest 78 条已验，32 位侧诚实边界。 */
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

/* §124y-166 AUD-R 分叉标记：胜负分发族（会话 B 移植，对应 32 位 0x30200 分发）。
   · R-054 on_flag_capture 未校验吃旗方合法/未清旗计数 → 32 位侧同（直接写 winner）→ 对齐（同源）
   · R-114 on_draw/on_stalemate/flag_capture 无 NULL 防护 → 32 位侧同（jq_on_flag_capture / jq_on_draw / jq_on_stalemate
     全部无判空）；64 位已有 NULL 防护统一批（rules.c 三终局函数 `if (!st) return;`）
     → 分叉（64 位已修，32 位未同步；★ 对抗审计 P0 批2 S-3"同源"措辞纠正）
   · R-115 flag_capture 参数 uint8_t 与 winner int8_t 隐式窄化 → 32 位侧同签名→ 对齐（同源）
   · R-002 暗棋 ruleflip 三规则（地雷/军旗仅工兵吃、挖 3 雷扛旗）→ 32 位侧走子桥
     布局写死 side 分区（明棋藏脸），ruleflip 模式维度未实装（mines_cleared 写不读）→ 分叉（64 位独有设计）
   · R-015 mines_cleared 只写不读 → 32 位侧同（写入，走子桥无读取点）→ 对齐（同源缺口）
   · R-121 state 缺扛旗计数（战况统计无法区分扛旗胜/困毙胜）→ 32 位侧同（jq_state 无该字段）→ 对齐（同源）
   胜负分发主链实证 = arm32_walkregress 路径 3（终局）；
   暗棋 ruleflip 胜负细分 32 位侧未实证，诚实边界留待 32 位反汇编专项。 */
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
/* §124y-166 AUD-R 分叉标记：AI 评分族（对抗审计 #2 从设备端 ai_pick 提取，非 32 位 so 反汇编产物——
   32 位走子桥 AI 在 0x3ab80 事件分发侧，本 ai_score 为 64 位侧单测化的贪心评分）。
   · R-009 炸弹 vs 军旗判 1000 → 32 位侧 ai_score `r < dr` 对炸弹(10) vs 军旗(12)
     同样判 1000（炸弹撞旗 1000 两侧行为一致保留）；地雷特判前置修的是
     "撞雷误判吃子"（非 R-009 描述的炸弹撞旗）。64 位 rules.c 注释"保持原行为"
     → 对齐（同源保留，炸弹撞旗=1000 两侧一致，均非"已修"）
   · R-010 无 revealed 维度暗棋作弊 → 32 位侧 ai_score 无 revealed 入参（纯 rank 贪心），
     走子桥 AI 仅明棋主链实证 → 分叉（64 位独有设计，暗棋 AI 32 位未实证）
   · R-055 未归一化吃子全 1000 → 32 位侧同（大压小 1000）→ 对齐（同源）
   · R-056 不评估被反吃风险 → 32 位侧同（无 lookhead）→ 对齐（同源简化）
   · R-057 空源 rank=0 返 2 → 32 位侧同（空移 2），但 64 位有 `if (src_rank_raw==0)
     return -2;` 空源哨兵（AUD-R-057/092 修复）32 位无 → 分叉（64 位独有 -2 哨兵，
     32 位非法输入静默得 2 分）
   · R-058 暗棋扛旗条件未实现 → 32 位侧同（无 revealed/mines_cleared 维度）→ 分叉（64 位独有设计）
   · R-059 炸弹空移与普通空移同分 2 → 32 位侧同（不区分炸弹）→ 对齐（同源）
   · R-060 同级同归 900 混淆两语义 → 32 位侧同（同级 900 不区分炸弹/普通）→ 对齐（同源）
   · R-092 ai_score 空源输入返回合法分缺哨兵 → 32 位侧同（rank=0 → 2，无 -2 哨兵）
     → 分叉（64 位独有 -2 哨兵，32 位非法输入静默得 2 分，同 R-057）
   · R-093 dr 计算与 dst_rank_raw 判定重复 → 32 位侧同→ 对齐（同源）
   · R-094 炸弹 800 分支依赖顺序 else-if 链 → 32 位位侧同→ 对齐（同源）
   · R-095 红方编码无上界校验 → 32 位侧 rank_norm 同实现 → 对齐（同源）
   AI 评分 32 位走子桥未实证（ai_pick 非 F_3ab80 主链，arm32_walkregress 不覆盖）；
   诚实边界：ai_score 为 64 位侧单测化提取，32 位侧 AI 实际走 0x3ab80 事件分发。 */
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
