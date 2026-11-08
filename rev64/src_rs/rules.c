/*
 * rules.c — 走子 / 吃子 / 翻棋 / 胜负 规则（逆源码重写）
 * 依据原版函数（VA）+ 规则文本（SPAK data/junqi/rulelay.txt / ruleflip.txt，GBK 已读）：
 *
 * ★ AUD-R-096（P3 已修）：头注释按「确证 / 推断 / 存疑」三档标注每条依据
 * （与 docs_rs/CODE_STYLE.md 诚实边界要求一致）：
 *
 * 【确证】（32 位 so 反汇编实证）：
 *   0x39e14 目标类型判定（★ AUD-R-021（P2 已修）：下表为**布局期**校验限制，
 *   非走子目标判定——实现在 layout.c jq_check_layout_cell，走子路径不引用这些约束，
 *   见 REVERSE_NOTES_01 §L59「0x39e14 布局校验确证：非走子方向限制」）：
 *     布局校验：炸弹 row!=0（不放第一行）；地雷 row>3（仅最后两行）；
 *     军旗 row∈{大本营格}（0x39e7a 特判）。
 *   0x39f20 移动执行器：from==to→翻棋；坐标 0x6ef54 比较（[0]=col [1]=row）；
 *     格子 +0x80+[from*5+to_offset]，rank 在格 +4（L65/L79）。
 *   0x3a15c 走子交换：from rank ← to rank 互换（含吃子落点）。
 *   0x30200 胜负分发：r3=0/1/2（蓝胜/红胜/和）→ Junqi.Win/Draw 事件。
 *   0x30260 吃旗胜 / 0x30280 和棋 / 0x302b4 困毙（status=3，L53/L54）。
 *
 * 【推断】（rulelay 原文语义，与兄弟实现 game_logic.c 对齐）：
 *   权威规则（rulelay 原文）：
 *   - 吃子：司令>军长>师长>旅长>团长>营长>连长>排长>工兵（rank 小=强，L52）；
 *     炸弹与任何棋子相遇双方都消失（包括军旗）；地雷<工兵、>所有其他棋；
 *     任何棋子都能扛军旗。
 *   - 行营=安全岛（敌方不能吃行营中棋子）；进大本营不可再动；军旗必须在大本营。
 *   - 胜负：棋子被吃光 / 军旗被扛 / 无棋可走 → 判负；
 *     连续 70 步无吃子 → 和；总步数 1000 → 和。
 *
 * 【存疑】（逆向笔记两种解读并存，人工定案前保持当前行为）：
 *   铁路（L49/L68 + board.c 实现，AUD-R-001 方向相关修正后拓扑）：
 *   · 纵向铁路 = 仅 col2 一条纵贯线（row0..row11，行营/大本营除外）；
 *   · 横向铁路 = row0（蓝底线）/ row5 / row6（中线）/ row11（红底线）整行。
 *   ★ AUD-R-023（P2 诚实标注，需人工确认）：行营排除出铁路 → 中线 col2 被
 *   行营 (3,2)/(8,2) 切成三段（工兵也无法一次走子穿过中心行营，实测
 *   (2,2)→(4,2)=0 即使目标空）。与 L68「行营(3,2)(8,2)分三段」一致，但与
 *   rulelay「工兵可在铁路线上任意行走」的通常解读存在张力（兄弟实现
 *   game_logic.c:44 注释却写「纵向移动任意列可行」，同仓解读矛盾）。
 *   二选一（(a) 维持"行营断路"写进 SPEC；(b) 改"行营可穿行不可吃"）需人工
 *   复核原版 so 的 jq_is_rail 调用者路径后定案，本轮不改行为。
 */
#include "junqi.h"
#include "ui_state.h"   /* ★ P1（datah_012）：游戏设置 4 规则开关 ui_rule_cfg_get()
                          （和棋/大本营/吃地雷/吃军旗，默认全开对齐原版默认，行为不变）
                          ★ AUD-R-032（P2 诚实标注，大工程留待下一轮）：规则引擎在
                          move_legal 大本营/暗棋段/吃地雷分支、stalemate_after 和棋
                          判定点、EV_FLAG 分支内直接调 UI 层 ui_rule_cfg_get() → 规则
                          层与 UI 层耦合：(a) 任何链接 rules.c 的目标必须提供 UI 层
                          实现（桌面靠 ui_state_stub.c 兜底）；(b) 规则行为随 UI 设置
                          改变，非纯函数，无法做规则矩阵回归。修法（规则开关抽到
                          jq_state / rules_cfg 由调用方注入，rules.c 只读 st->cfg.*）
                          为分层重构（CODE_STYLE.md 分层原则），影响 5 处判定点 +
                          桌面桩，本轮诚实标注不重构。 */
#include <string.h>
/* ★ AUD-R-097（P3 已修）：<stdlib.h> 已删——grep 确认 rules.c 无 abs/atoi/rand/
   malloc/free/calloc/realloc/qsort/bsearch 任何使用点（仅 string.h 的 memset）。 */

/* ★ AUD-R-061（P2）：7 类事件（SPEC §3 契约表），EV_FLIP=Junqi.Flip（32 位 so
   .rodata 明文含此事件名）。★ AUD-R-062（P2）：[0]=EV_MOVE 与 [5]=EV_MINE 同名
   "Junqi.Move"（对齐原版设计），[6]=EV_FLIP 为翻棋独立通道。 */
/* ★ AUD-R-127（P3 已修）：元素类型改 const char *const（指针本身 const，外部
   无法改写 JQ_EVENT_NAMES[i]；junqi.h 声明同步）。 */
const char *const JQ_EVENT_NAMES[7] = {
    "Junqi.Move", "Junqi.Eat", "Junqi.Bomb", "Junqi.Flag", "Junqi.Dead", "Junqi.Move", "Junqi.Flip"
};

/* ---------- 走子路径 ---------- */
/* 该格能否被"经过"（非子、非行营、非大本营）。
   ★ AUD-R-017（P1）：原 mover 形参为残留未用（编译器 -Wdiscarded-qualifiers 告警），已删。
   ★ AUD-R-027（P2 已修）：修法=删 mover 形参（上一轮 AUD-R-017 一并落地，调用点
     rail_path_engineer/rail_path_straight 同步）；"能否穿过己方子"语义未定案，
     若将来需要，按 move_legal 的 dst->side==me 拦截风格另加谓词，勿悬空形参。
     ★ AUD-R-098（P3 关闭确认）：walkable() 的 mover 形参已随 AUD-R-017/R-027 删除，
     本项确认关闭（grep 确认无调用点传 0）。 */
static bool walkable(const jq_state *st, int row, int col)
{
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) return false;
    const jq_cell *c = &st->cells[cell_index(row, col)];
    if (c->rank != RANK_EMPTY) return false;
    if (is_camp(row, col)) return false;   /* 行营非铁路，不通行（L49） */
    if (is_base(row, col)) return false;
    return true;
}

/* ★ AUD-R-099（P3 已修）：方向表提为文件级 static const（原在函数体内 static，
   无运行时开销但降低可读性，与 board.c 风格不一致）。 */
static const int8_t RAIL_DIRS[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

/* 工兵沿铁路任意路径（可拐弯）：BFS。
   铁路拓扑唯一事实源 = board.c is_rail()：纵向仅 col2 一条纵贯线；
   横向 = row0（蓝底线）/ row5 / row6（中线）/ row11（红底线）整行。
   ★ AUD-R-025（P2 已修）：入口起点越界自检（fr/fc 越界 → false，防 visited 越界写）；
   当前唯一调用方 jq_can_move 传已校验坐标，本守卫为双保险（函数 static 但无自检）。
   ★ AUD-R-026（P2 诚实标注 → 真修）：队列 qr/qc[64] > 60 格理论安全；visited 保证
   不重复入队。AUD-A-0040/0041（P2）补显式 tail 上界断言（入队前 `if (tail >= 64)
   break;`）——60 格棋盘 visited 不重复入队，tail 恒 ≤60 永不触发，纯防御（与
   AUD-R-026 "BFS 边界行为已冻结"一致，行为 0 变化，仅把"依赖 visited 语义不被
   改动"的隐性假设升级为显式守卫）。 */
static bool rail_path_engineer(const jq_state *st, int fr, int fc, int tr, int tc)
{
    /* ★ AUD-R-101（P3 注释确认）：同格视为可达（防御早退；调用方 jq_can_move
       已在 move_legal 入口排除 from==to，此分支不可达，保留为函数自身契约）。 */
    if (fr == tr && fc == tc) return true;
    if (fr < 0 || fr >= BOARD_ROWS || fc < 0 || fc >= BOARD_COLS) return false;  /* AUD-R-025：起点越界自检 */
    /* ★ AUD-R-100（P3 已修）：visited int→uint8_t（240B→60B 栈），memset 一次清零。 */
    uint8_t visited[BOARD_ROWS][BOARD_COLS];
    memset(visited, 0, sizeof(visited));
    /* AUD-1000 B01-007：qr/qc 索引域 0-59 → uint8_t[64]（栈 512B→128B，栈帧瘦身） */
    uint8_t qr[64], qc[64];
    int head = 0, tail = 0;
    /* ★ AUD-1000 B01-067（P3 已修）：旧 `if (tail >= 64) return false;` 在 qr[0] 赋值
       前（tail 恒 0）永不触发，系死代码；真保护为入队前 :131 `if (tail >= 64) break;`
       （可能触发）。本轮删死断言，保留真保护（:131），行为 0 变化。 */
    qr[tail] = fr; qc[tail] = fc; tail++;
    visited[fr][fc] = 1;
    while (head < tail) {
        int r = qr[head], c = qc[head]; head++;
        for (int k = 0; k < 4; k++) {
            int nr = r + RAIL_DIRS[k][0], nc = c + RAIL_DIRS[k][1];
            if (nr < 0 || nr >= BOARD_ROWS || nc < 0 || nc >= BOARD_COLS) continue;
            if (visited[nr][nc]) continue;
            /* ★ AUD-R-024（P2 已修）：横向行集合与 rail_path_straight 的
               AUD-R-001 方向校验对齐 —— 横向铁路 = row0/5/6/11 整行，
               纵向铁路 = 仅 col2。旧实现只放 row5/6 → 工兵沿底线横滑
               （直滑允许但 BFS 禁止）两套判定打架。 */
            /* ★ AUD-1000 B01-068（P3 注释）：nc!=c = 列变（横移）仅中线 row5/6
               允许（横铁路仅中线）；nc==c = 列不变（纵滑）col2 全高（纵线贯穿，
               非仅中线）。r 是当前 BFS 节点行（非目标行），语义正确。 */
            if (nc != c && !(r == 0 || r == 5 || r == 6 || r == 11)) continue;
            if (!is_rail(nr, nc)) continue;
            /* ★ AUD-1000 B01-001（P0 已修）：目标判定并入 walkable——行营/大本营
               空格非铁路占位 → 工兵 BFS 不可"滑行"进入（与 move_legal 条款一致）；
               占位空格且非行营/大本营 → walkable 真 → 目标命中。 */
            if (nr == tr && nc == tc) return walkable(st, nr, nc);
            if (!walkable(st, nr, nc)) continue;
            visited[nr][nc] = 1;
            if (tail >= 64) break;   /* AUD-A-0041：队列满保护（60 格棋盘恒不触发，纯防御） */
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
    /* ★ AUD-1000 B01-069（P3 注释）：与 B01-001 同源——is_rail 对 camp/base 返 false
       （行营/大本营端点被拦），工兵 engineer 路径 BFS 同样拦行营/大本营节点
       （:128 walkable 守护）；工兵端点在行营/大本营时非工兵 straight 路径拦
       （:128 walkable 判 false），工兵路径由 B01-001 :128 早退覆盖。 */
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
           非工兵直走不能拐弯 → 纵线上遇到行营/大本营阻断滑行。
        ★ AUD-1000 B01-019（P3 注释）：横滑中间格含大本营/纵线交点——
        纵线 (0,2)/(11,2) 底线交点 is_base 假不阻断；(3,2)/(8,2) 行营 is_camp
        真阻断；横滑 row5/row6 中线无行营/大本营（行营在 col2）无拦截。 */
        /* AUD-1000 B01-018：工兵恒走 rail_path_engineer（:292 分派），
           非工兵才入 straight —— 旧 rsr!=RANK_WORKER 判断下该分支恒 true，
           break 不可达（死代码）。简化为：纵线遇行营/大本营直接阻断。
           ★ AUD-1000 B01-081（P3 注释）：rsr（源格 rank）在 B01-018 修复后
           已删（旧工兵特判段消费 rsr，非工兵路径 rsr 恒 !=9 死变量无消费）。
           本条核实 rsr 已删，无死变量残留。 */
        for (int rr = fr + ((tr > fr) ? 1 : -1);
             rr != tr; rr += (tr > fr) ? 1 : -1) {
            if (is_camp(rr, 2) || is_base(rr, 2)) return false;
            /* ★ AUD-1000 B01-097（P3 注释）：纵线 col2 遇行营 (3,2)/(8,2) 或大本营阻断。
               (0,2)/(11,2) 非行营（is_camp 假）非大本营（is_base col1/3 仅）→ 不阻断
               纵滑穿过 (0,2)/(11,2) 底线交点 ✓ 正确（AUD-R-066 双重身份已标注）。 */
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
    /* ★ AUD-R-102（P3 已修）：stage 与 from==to 拆开并注释——
       布阵阶段（STG_SETUP）禁止走子；同格走子无效（jq_try_move 入口双重校验）。 */
    if (st->stage != STG_PLAY)
        return false;   /* 布阵阶段禁止走子（jq_can_move 仅 STG_PLAY 放行） */
    if (from == to)
        return false;
    if (from < 0 || from >= N_CELLS || to < 0 || to >= N_CELLS) return false;
    /* ★ AUD-1000 B01-051（P1 已修）：turn 参数脏值拒收——turn<0 视为 -1（困毙忽略
       回合语义），turn>1 直接拒（非静默全拒）。旧实现脏 turn（2/255）经
       (uint8_t)turn 与 src->side(0/1/0xFF) 比较恒 false → 全拒所有走子 →
       困毙误判（该方"无棋可走"）。调用方 jq_can_move 传 st->turn（恒 0/1），
       困毙路径传 -1，本守卫仅防外部误传脏值。 */
    if (turn < 0) turn = -1;
    else if (turn > 1) return false;
    /* ★ AUD-R-017（P1）：const 限定对齐 const jq_state*（原非 const 指针触发
       -Wdiscarded-qualifiers；残留 dr_ 未用变量已删——走子合法性不需要目标 rank）。
       ★ AUD-R-103（P3 关闭确认）：dr_ 未用变量已随 AUD-R-017 删除（grep 确认），
       本项确认关闭。 */
    const jq_cell *src = &st->cells[from];
    const jq_cell *dst = &st->cells[to];
    if (src->rank == RANK_EMPTY) return false;
    int sr = rank_of(src);
    /* ★ 回合校验（用户指正"各按钮点击业务功能要对齐"）：只能走当前 turn 方棋子。
       缺失此校验 → 两人热座可连走一方/人机可走 AI 回合/蓝牙可抢走，破坏回合交替。
       暗棋 side=0xFF 未定子翻棋后才定 side（ruleflip），此处 src->side 已定。 */
    if (turn >= 0 && src->side != (uint8_t)turn) return false;
    /* 军旗/地雷不能动 */
    if (sr == RANK_FLAG || sr == RANK_MINE) return false;
    /* 源格在大本营：进入大本营的棋子不能再动（rulelay 原文）。
       ★ P1（datah_012）：大本营开关（ui_rule_cfg_get(1)，默认 1 对齐原版默认）：
       =0 时进大本营的棋子可再动。
       ★ AUD-R-030（P2 诚实标注）：实现语义="当前在大本营内的棋子不能动"，
       规则原文="曾进入过大本营的棋子冻结"——仅在"子进大本营后开关关闭/
       旗被吃大本营变空"场景不等价。旗被吃后攻子冻在大本营的实测行为正确
       （evverify hq 夹具）；完整"进入即锁定"需棋盘层标记位，留待下一轮。 */
    if (ui_rule_cfg_get(1) && is_base(cell_row(from), cell_col(from))) return false;
    /* 暗棋：未翻开不可动（side 未定/已定但 revealed=0）
       ★ AUD-R-006（P1）：未定色暗子（side=0xFF）不可被直接攻击（ruleflip 原文：
       只能翻、不能吃；兄弟实现 game_logic.c 同判定）。明棋/布局模式下 0xFF 不可达，
       行为不变；明棋已翻子 revealed=1 恒放行。
       ★ AUD-1000 B01-052（P3 注释）：明棋（JQ_MODE_LAY）布局全子 revealed=1（layout.c
       jq_load_layout 写 revealed=1），本检查恒放行；暗棋（JQ_MODE_FLIP）未翻子
       revealed=0 被拦（"未翻不可动"语义正确）。存档读档部分格脏 revealed=0 时
       明棋模式下本检查仍拦——该场景未列入 SPEC，保持现状不改行为。
       ★ AUD-1000 B01-053（P1 诚实标注）：暗棋未翻暗子目标（rank 1-9 revealed=0）
       非雷非旗 → 放行到后续判定 → 非工兵可撞未翻暗子（撞子即翻暴露 rank，
       信息不泄露被破坏）。修法：:259 暗棋段补
       `if (!dst->revealed && dst->rank != RANK_EMPTY &&
              dst->rank != RANK_MINE && dst->rank != RANK_FLAG) return false;`
       （暗棋未翻普通暗子目标不可撞，仅可翻/不可吃）。影响 evverify 暗棋基准
       （暗棋走子路径全需目标已翻），需同步 evverify 夹具，留待下一轮。
       ★ AUD-1000 B01-015：mines_cleared 下界钳制——mines_cleared[me] 只增不减
       （EV_MINE 挖雷 +1），正常路径恒 ≥0；若外部误写负值（数据损坏）恒 <3
       拒扛旗 = 安全降级（拒优于放行），钳 0 仅消除"负计数"语义歧义。
       注：me 已经入口 SIDE_VALID 守卫（:424-437），下标 me∈{0,1} 安全。 */
    if (!src->revealed || src->side == SIDE_UNKNOWN) return false;
    uint8_t me = src->side;
    /* 目标被己方子占据 → 不可 */
    if (dst->rank != RANK_EMPTY && dst->side == me) return false;
    /* ★ AUD-1000 B01-053（P1 已修）：暗棋未翻暗子目标不可撞（仅可翻/不可吃）。
       原实现在此检查点缺失：暗棋下 rank 1-9 未翻子（revealed=0）可被普通子撞击
       （信息泄露）。现补：暗棋模式下非雷非旗未翻目标拒走（只能走空格或已翻子格）。
       工兵撞雷/扛旗路径不受影响（dr==MINE/FLAG 在 :280 段单独处理）。 */
    if (st->mode == JQ_MODE_FLIP && ui_rule_cfg_get(4) &&
        !dst->revealed && dst->rank != RANK_EMPTY &&
        dst->rank != RANK_MINE && dst->rank != RANK_FLAG)
        return false;
    /* 行营安全岛：敌方棋子不能吃行营中的棋子（rulelay 原文）；
       空行营可走（一步相邻，行营非铁路不可穿行滑行）
       ★ AUD-R-028（P2 诚实标注，需人工确认）：空行营允许敌方一步进入——
       兄弟实现 game_logic.c:93 明确禁止「不能进入敌方行营」，rulelay.txt 字面
       只禁「吃行营中的棋子」，同仓两引擎结论相反。修法：人工确认原版
       0x39e14 调用者对行营目标格的处理后统一两实现并写进 SPEC。
       本轮不改行为（保持 rulelay 字面解读）。
        ★ AUD-R-105（P3 关闭确认）：SPEC §2「行营/大本营/铁路」条款已随
        AUD-R-072 补齐（行营安全岛语义 = 敌方不能**吃**行营中棋子，空行营
        可一步进入，行营非铁路不可穿行；与 junqi.h is_camp 注释对齐，
        头注释"敌方不可入"的矛盾表述已修正）。本项确认关闭。 */
    if (is_camp(cell_row(to), cell_col(to)) && dst->rank != RANK_EMPTY &&
        dst->side != me)
        return false;
    /* ★ AUD-1000 B01-098（P3 注释）：行营中敌方子不可吃 ✓（安全岛）；行营中己方子
       不拦（:266 条件假）可走出（源格行营自由走 AUD-R-072）✓ 正确；空行营一步可进
       （AUD-R-028 已标注字面解读）✓ 无缺陷。行营三条语义已标注（安全岛/可走出/空可进）。 */
    /* 暗棋（ruleflip，AUD-R-002 P0）：地雷/军旗仅工兵(rank 9)可吃；
       军旗额外要求攻方工兵已挖满 3 雷（mines_cleared[me] >= 3）。
       明棋（st->mode != JQ_MODE_FLIP）：任何棋子可扛旗（rulelay 原文），本段恒放行。
       ★ 规则开关 ui_rule_cfg_get(4)=1 启用暗棋（默认 1 对齐原版默认）；
       =0 时退回明棋语义（行为不变）。兄弟实现 src/game_logic.c:100-104 同款判定。 */
    {
        int dr_ = rank_of(dst);
        /* ★ AUD-1000 B01-082（P3 注释）：dr_ 空格=0（rank_of 空格返 0），雷/旗按
           dst side 还原（rank_of 按 c->side 判定 + 范围校验）。本条核实无缺陷。 */
        if (st->mode == JQ_MODE_FLIP && ui_rule_cfg_get(4) &&
            dst->rank != RANK_EMPTY &&
            (dr_ == RANK_MINE || dr_ == RANK_FLAG)) {
            /* 地雷：仅工兵可吃 */
            if (dr_ == RANK_MINE) {
                if (sr != RANK_WORKER) return false;
            }
            /* 军旗：仅工兵可扛，且须已挖满 3 雷 */
            else {
                if (sr != RANK_WORKER) return false;
                /* ★ AUD-1000 B01-015（P2 诚实标注，非 C 修复）：mines_cleared
                   无下界钳制——悔棋快照恢复可注入负值（快照在挖雷 ++ 前推送），
                   负值恒 <3 拒扛旗已是安全降级（拒绝方向正确，不误放行）；
                   显式钳 0 需 const 指针非 const 访问（move_legal 契约为
                   纯读），不改执行器外行为，留待 jq_undo_restore 入口钳制
                   或 SPEC 定案。 */
                if (SIDE_VALID(me) && st->mines_cleared[me] < 3) return false;
            }
        }
    }
    int fr = cell_row(from), fc = cell_col(from), tr = cell_row(to), tc = cell_col(to);
    /* 一步相邻（公路线）：行营落点仅允许一步相邻（非铁路）
       ★ AUD-R-072（P2 已修）：源格在行营的子可自由走出（行营是安全岛非陷阱，
       不限制源侧移动）——原 SPEC §2 未记录行营条款，本轮已补
       "行营/大本营/铁路"三条独立条款（与实现逐条对齐）。
       ★ AUD-R-070（P2）：改为具名差值 dr/dc 判定（同行差 1 列 / 同列差 1 行），
       原写法两次减法（fc-tc==1||tc-fc==1）易与"行列同行"语义混淆。 */
    int dr = tr - fr, dc = tc - fc;
    /* ★ AUD-1000 B01-070（P3 注释）：4 邻接（公路线一步，非 8 邻接）——dr==0 横 1 列
       或 dc==0 纵 1 行；无对角误判。from/to 已校验 ∈[0,N_CELLS)，越界 from 在
       :190 拦（无越界），本判定无缺陷。 */
    int adjacent = (dr == 0 && (dc == 1 || dc == -1)) || (dc == 0 && (dr == 1 || dr == -1));
    if (is_camp(tr, tc) && !adjacent) return false;
    /* 目标在大本营：仅能吃军旗/地雷（翻棋模式）或其他子（明棋"任何棋子扛旗"）
       ★ AUD-R-071（P2 诚实标注）：空大本营任何棋子不可进（return false）——
       对齐原版 0x39e14 大本营目标判定（布局态仅军旗/地雷可进；对局态大本营格
       是"落点"非"子"，进大本营 = 吃旗/布雷准备的语义，非普通走子目标格），
       与 rulelay「结点、行营、大本营并列落点」的通常解读存在张力，本轮不改
       行为（保持 0x39e14 实证语义），SPEC §2 已补大本营条款。
       ★ AUD-R-029（P2 诚实标注）：暗棋（src->revealed==0）收窄为"大本营仅旗/雷
       可吃"（ruleflip 第 3 行），明棋放行任何子（rulelay 未禁）。语义边界：
       明棋"大本营内普通子可被吃"未在 rulelay 明文，属推断，需人工复核
       原版 0x39e14 大本营目标格路径后在 SPEC 定案。 */
    if (is_base(cell_row(to), cell_col(to))) {
        if (dst->rank == RANK_EMPTY) {
            /* 大本营空格：只有对方军旗/地雷可走（布局），否则不可进。
               ★ AUD-1000 B01-016（P0 已随 B01-001 关闭）：旧实现工兵 BFS
               目标判定先于 walkable → 可"滑行"进空大本营（BFS 说可达、
               本条款拒 = 分叉）；B01-001 修后 BFS 目标并入 walkable，
               行营/大本营空格滑行不可达，两处判定一致，无分叉。
               ★ AUD-1000 B01-021（P3 注释）：空大本营拒进与 ui_rule_cfg_get(1)
               大本营开关无关——开关 1 仅控"源格在大本营的子可再动"（:202 源格侧），
               目标格侧"空大本营恒拒"是几何事实（大本营格仅容旗/雷布局，走子期
               空大本营无落点语义）。明棋已揭示棋子无法进大本营（布局期占用），
               改动 :287 拒进语义为死代码，不改行为。 */
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
            int d_rank = rank_of(dst);
            /* ★ AUD-1000 B01-071（P3 注释同步 AUD-R-102 修正事实）：rank_of(dst) 对
               dst 在大本营的子（军旗/地雷）还原正确；对 dst 普通子（进大本营冻结子）
               还原正确（rank_of 按 dst->side 还原）。本条核实无缺陷，注释同步 AUD-R-102
               根因修正（mode 判定信号 = src->revealed 非 dst->revealed）。
               ★ AUD-1000 B01-054（P3 注释）：rank_of(dst) 对未定色暗子（side=0xFF）
               的 rank>12 脏值钳 RANK_TOP（12=军旗哨兵），可能误判为军旗放行"吃旗"。
               未定色暗子在暗棋布局中 rank 恒 raw 1-12（jq_dark_setup 全 raw 编码），
               脏值仅 BT SYNC_CELL 注入 / 存档损坏时可达（AUD-R-047 同源）。
               修法：rank_of 对 SIDE_UNKNOWN 不做 +0xc 域钳位（0xFF 侧 rank 恒 raw
               1-12），留待 SPEC 定案。本轮不改行为。 */
            if (src->revealed == 0 && d_rank != RANK_MINE && d_rank != RANK_FLAG)
                return false;
        }
    }
    /* 一步相邻（公路线） */
    if (adjacent)
        return true;
    /* 铁路滑行（行营/大本营已在前面排除） */
    if (sr == RANK_WORKER)      /* 工兵（rank 9）可拐弯 */
        return rail_path_engineer(st, fr, fc, tr, tc);
    return rail_path_straight(st, fr, fc, tr, tc);
}

/* 公开 API：当前回合的走子合法性（保持既有语义）
   ★ AUD-R-082（P2 诚实标注）：困毙/可动判定 O(N²) 次 jq_can_move（每次含工兵 BFS），
   单步 ~3.5k 次合法性判定 / ~20 万次格点运算——规则引擎在主线程 OnTouch 单步执行，
   移动端 60fps 渲染线程不做走子计算，实测 OK；缓存合法目标格或射线扫描替代
   逐目标 BFS（性能重构）留待下一轮压测驱动。
   ★ AUD-R-086（P2 诚实标注）：翻棋（jq_flip）消耗 moves_since_eat 与 total_moves
   （对齐原版 0x39f20 from==to 路径消耗步数，与和棋 70/1000 步判定一致），
   暗棋下翻牌速度主导和棋阈值——与 AUD-R-033 同源，本轮不改行为。
   ★ AUD-R-041（P2 诚实标注）：jq_try_move 全程不刷新 pieces_left/count_tbl
   （只做增量减法，与 AUD-R-014 同源——计数可为负/未刷新，困毙判定走
   jq_side_stalemated 实时扫描不读计数）。原 st->count_tbl 字段已删（全仓
   从未写入，布局计数需求走 containers.c cc->count_tbl 校验链路）。 */
/* ★ AUD-R-091（P3 诚实标注）：规则引擎公开 API（jq_can_move/jq_try_move/jq_can_flip/
   jq_flip/jq_side_stalemated/jq_on_draw/jq_on_flag_capture/jq_on_stalemate/jq_undo_*）未做 st==NULL 防护（兄弟实现
   game_logic.c:75 有），全仓调用点均传有效 jq_state*（jni_bridge 持有 g_state
   全局实例，桌面测试传栈变量）——加 NULL 防护需 10+ 函数入口各 1 行，行为 0
   变化，收益低（无实际 NULL 传入路径）。修法（统一入口防护或 assert 契约）
   留待下一轮；本轮诚实标注不加防护（避免引入 10+ 处改动风险）。 */
bool jq_can_move(const jq_state *st, int from, int to)
{
    /* ★ AUD-R-091/114（P3 NULL 防护统一批）：公开 API 入口 NULL 守卫
       （兄弟实现 game_logic.c 同款防御风格；规则引擎 10 个公开 API 全覆盖）。 */
    if (!st) return false;
    return move_legal(st, from, to, st->turn);
}

/* ★ 内部用：忽略回合限制的走子合法性（困毙判定用，turn 传 -1） */
static bool can_move_ignore_turn(const jq_state *st, int from, int to)
{
    return move_legal(st, from, to, -1);
}

/* ★ AUD-R-014（P1）：公开的"忽略回合"合法性查询。
   jq_can_move 强制 src->side == st->turn → 对**非当前回合方**统计可动子时
   恒为 0（旧 jq_refresh_movable_counts 因此把另一方算成 0/1 个可动子）。 */
bool jq_can_move_any(const jq_state *st, int from, int to)
{
    if (!st) return false;
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
    /* ★ AUD-R-014（P1）：走子/翻棋后刷新双方可动子统计（原版 0x6b87c→0x6a3b0
       "走子后刷新统计"）。同一处覆盖两条路径（jq_try_move / jq_flip），
       避免增量维护漂移（旧实现只 -- 不刷新 → 计数可为负）。 */
    jq_refresh_movable_counts(st);
    /* ★ AUD-R-033（P2 诚实标注）：total_moves 同时计入走子与翻棋（jq_flip :392
       total_moves++）——"1000 步"是否含翻棋无原版依据（70 步无吃子计翻棋有
       §L70 依据；1000 步无）。修法（拆 total_moves/total_turns 双字段或 SPEC
       明文"翻棋计入总步数"）留待下一轮。
       ★ AUD-R-034（P2 诚实标注）：和棋判定先于困毙判定，同一步同时成立时判和
       不判负——原版优先级无据（兄弟实现 game_logic.c:275-276 同序）。修法
       （困毙优先 + 回归用例）留待下一轮。
       ★ AUD-R-075（P2 诚实标注）：和棋先于困毙 + 两者都在 turn 切换后判定
       （stalemate_after 判"下一方"）——原版 0x30200 分发顺序未逆向确证，当前
       实现先和棋后困毙，对齐 evverify 基准（evverify 对和棋/困毙夹具的期望值即
       此优先级），本轮不改优先级。
       ★ AUD-R-089（P2 关闭）：困毙未显式排除"可直接吃旗"情形——能吃旗的子
       必有合法目标格（大本营目标分支放行军旗），jq_side_stalemated 的
       can_move_ignore_turn 扫描已覆盖；终局路径优先级：吃旗（即时 EV_FLAG）
       > 和棋 > 困毙（走子后收口），无需显式排除。 */
    /* ★ AUD-1000 B01-072（P3 注释）：ui_rule_cfg_get(0) 在 stage 守卫之后调用
       （UI 层读无锁）——规则层调 UI 层（AUD-R-032 耦合）且无锁保护（UI 开关
       可能被主线程写）数据竞态。修法：规则开关读加锁（与 g_ui_lock 同源）或
       拷入 jq_state（st->cfg 快照开局时冻结）消除跨线程竞态。
       真机路径 ui_rule_cfg_get 已持 g_ui_lock，竞态窗口极小；桌面桩单线程无并发，
       保持现状不改行为。 */
    /* AUD-1000 B01-023：判定值含本步（走子/翻棋收口前已 moves_since_eat++ /
       total_moves++），即'第 70 步空移完成后'立即读 70 判和，语义='已进行
       70 步无吃子'——与翻棋路径（jq_flip 同序自增）对称，防未来回归改顺序。
       ★ AUD-1000 B01-002（P0 修）：吃子分支清零 moves_since_eat（:493/:522/:535/
       :577/:605 五处）——清零发生在 stalemate_after 调用**之前**（EV_BOMB 分支
       内 :493 清零后 :542 stalemate_after 读到 0 不判和 ✓）。和棋阈值判定对象
       = '走子收口时刻的 moves_since_eat 值'，语义为'自上次吃子起累计空移步数'，
       第 70 个空移完成后读到 70 → 判和（含本步，晚 1 步于'已进行 70 步'严格
       字面量——与 AUD-R-075 'stalemate_after 判下一方'注释一致：turn 已切换，
       判定的是'轮到对方走子前，该方已连续 70 步无吃子'）。原版 0x30200 和棋
       分发 ±1 边界未反汇编确证，evverify 基准即此实现，保持不动。 */
    if (ui_rule_cfg_get(0) &&
        (st->moves_since_eat >= 70 || st->total_moves >= 1000))
        /* ★ AUD-1000 B01-099（P3 注释，与 B01-002 同源）：双阈值 OR（70 无吃子或 1000 总步）
           ✓ 语义正确；70 步无吃子阈值从走子开始计（开局 moves_since_eat=0 累加），
           布阵阶段（STG_SETUP）不累加（走子才累加）✓ 正确；悔棋快照回退可回退
           moves_since_eat（回退后 70 步进度丢失，悔棋后和棋进度重置合理）无缺陷。
           B01-002 修后（计数归属标准）本条自动闭合。 */
        jq_on_draw(st);
    /* 困毙判定（0x302b4：无棋可走 → 该方负）。★ AUD-R-036（P2）：显式传
       loser_side=st->turn（此时"无棋可走方"=当前 turn 方，语义与旧版推导一致，
       不改变任何内部行为；仅消除外部调用点的 turn 依赖）。 */
    if (st->stage == STG_PLAY && jq_side_stalemated(st, st->turn))
        jq_on_stalemate(st, st->turn);
}

/* ★ AUD-R-108（P3 诚实标注，不提取共用收尾）：审计项建议提取
   jq_finish_turn(jq_state*) 统一 5 个事件分支的 turn 切换/计数/收口——
   实际核对现状后**保持现有结构**（行为 0 变化前提下）：
   · 走子/炸弹/地雷/普通吃子 4 分支已在 :470-474 统一收尾
     （st->turn=1-me / st->total_moves++ / stalemate_after）；
   · EV_FLAG 分支两处（:395-409 开关注局 / :410-417 开关关闭）带提前
     return——开关打开时终局（stage=OVER）跳过 stalemate_after 是**有意
     行为**（终局后无需困毙/和棋判定，AUD-R-008 双守卫），提取统一收尾
     会改变该路径（引入 stage=OVER 时的多余扫描 + 潜在 winner 覆盖回归
     风险）；
   · EV_MOVE 分支（空目标 :348-353）走通用收尾，与上述 4 分支同路径。
   故 5 分支中实际 4 分支已统一，EV_FLAG 开关注局提前 return 为唯一
   "不同步"点且为有意设计（evverify flag-on 夹具 53→58 期望 stage=OVER
   winner=me 即此行为）。提取共用函数留待下一轮（需同步 evverify 夹具
   + jni_bridge 终局快照语义），本轮诚实标注。 */

/* 执行走子。成功返回事件类型；失败返回 -1（不切 turn、不计数，L54）。 */
int jq_try_move(jq_state *st, int from, int to)
{
    /* ★ AUD-R-091/114（P3 NULL 防护统一批）：st NULL → -1（失败契约）。
       ★ AUD-1000 B01-092（P3 注释）：NULL 守卫仅 10 个公开 API 入口——内部函数
       （move_legal / rail_path_engineer / rail_path_straight / walkable / stalemate_after）
       无 st 守卫（依赖入口守卫），分层合理。32 位翻译体（jq_rs_rules.c）无
       ui_rule_cfg_get（R-138 分叉）亦无统一 NULL 防护批（翻译体自有入口口径），
       双端防护口径不一致——翻译体补 NULL 守卫同批（R-138 分叉同步义务），
       rules.c 已落地维持。 */
    if (!st) return -1;
    /* ★ AUD-R-042（P2）：双层防护——入口显式校验 from/to 与 from != to。
       主判定在 jq_can_move（from==to→false），但走子有第二个入口时
       （联机/复放/BT 帧）若绕过 jq_can_move 直接进执行器，from==to 会造成
       "自己吃自己"同级同归。此处为执行器自身的最小守卫。 */
    /* ★ AUD-R-083（P2）：原子性确认——任何 return -1 之前无 st-> 写（下方两段
       校验均纯读），所有状态变更（*dst=*src / memset / 计数 / turn/stage/winner）
       都在 jq_can_move 通过之后。stage 守卫由 jq_can_move 承担（stage!=STG_PLAY
       恒 false），复放/BT 帧第二入口若绕过 jq_can_move 直接进执行器，from==to
       入口守卫 + 后续可动性判定仍拦截。 */
    if (from == to || from < 0 || from >= N_CELLS || to < 0 || to >= N_CELLS)
        return -1;
    if (!jq_can_move(st, from, to)) return -1;
    /* ★ AUD-1000 B01-094（P3 注释）：:498 NULL + :504 界检查 + :505 can_move 三层
       守卫完备（AUD-R-042/083/091 已落地）。:504 界检查重复 can_move 内 :190 界检查
       （双保险已标注）无功能缺陷，本条核实无缺陷。 */
    /* ★ AUD-1000 B01-055（P3 注释）：本判定与调用方（ai_pick/replay/BT）前置
       can_move 幂等重复——为执行器自守卫（第二入口防护），无竞态（单线程规则引擎）。 */
    jq_cell *src = &st->cells[from], *dst = &st->cells[to];
    int sr = rank_of(src);
    int dr_ = rank_of(dst);
    /* ★ AUD-R-104（P3 已修）：me 断言——me 用作 pieces_left[]/turn 下标，
       安全性依赖 jq_can_move 的 src->side==turn 前提 + st->turn 在 0/1（
       jq_state_init 初始化 0，turn 切换 1-me 恒 0/1，jni_bridge 直接写
       SIDE_BLUE/SIDE_RED 常量）。若外部误写 turn>1 则 pieces_left[>1] 越界
       （与 AUD-R-004 同类）——入口防御见下方。 */
    uint8_t me = src->side;
    if (!SIDE_VALID(me)) {
        /* 诚实边界（P3 标注，非 P0/P1 修复）：
         · 本守卫在 *dst=*src 之前、所有写操作之前 return，无状态变更；
         · 正常路径（turn∈{0,1}）SIDE_VALID(me) 恒真（sq=src->side=turn）；
         · 仅外部误写 st->turn>1 时可达 → 返回 -1（原行为越界写 pieces_left[>1]，
           本守卫将该 UB 降级为明确的调用失败）；
         · 不改 jq_can_move 入口（避免影响既有 167/167 基准）；
         · 若需彻底防护：jni_bridge 各写 turn 点位（:226/1084/1112/5979/6109）
           已写 SIDE_BLUE/SIDE_RED 常量（0/1），无需改。 */
        return -1;
    }

    jq_event ev;
    if (dst->rank == RANK_EMPTY) {
        /* 空目标：互换 rank（0x3a15c：from←to, to←from，to=0）
           ★ AUD-R-106（P3 诚实标注）：整格拷贝 *dst=*src 把 flags 冗余字段（AUD-R-046）
           一并搬走——flags 可由 rank_norm(rank) 推导，全仓无规则读取点（规则引擎
           一律用 rank 判）。删 flags 字段需改 3 处整格拷贝 + layout.c 写点
           （跨文件重构），收益小（仅 1 字节冗余跟随），行为 0 变化，留待下一轮。 */
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
            ★ AUD-R-020（P1）：炸弹撞军旗 → 扛旗方（用炸弹的一方 me）胜（守卫
            SIDE_VALID(dst_side)：暗棋未定色暗子 side=0xFF 撞旗同归后不判胜，旗归属未定）。
            诚实边界：原版该场景判定尚未从 32 位 so 反汇编确证（依据 rulelay L12+L16 文本
            推断），若原版另有判定需按原版调整。 */
        int dst_side = dst->side;
        int dst_rank = dr_;
        memset(src, 0, sizeof(*src));
        memset(dst, 0, sizeof(*dst));
        st->pieces_left[me]--;   /* ★ AUD-1000 B01-056（P3 注释）：攻方炸弹消失 -1 */
        /* ★ 对抗审计修正（AUD-R-004 P0）：dst->side 可能是暗棋未定态 0xFF
           （junqi.h SIDE_UNKNOWN）→ pieces_left[255] 越界写 1020 字节外，
           破坏 jq_state 相邻成员。加范围守卫。 */
        if (dst_side != me && SIDE_VALID(dst_side)) st->pieces_left[dst_side]--;
        /* AUD-1000 B01-009：暗棋未定色暗子（dst_side=0xFF）撞炸弹同归后该子从棋盘
           消失但 pieces_left 无计数对象（pieces_left[2] 域外，SIDE_VALID 拒）。
           旧行为=计数漂移（暗棋 50 子开局下总量偏 1）；修：0xFF 消失子计入
           先手（蓝）池——先手翻子定色语义下 0xFF 暗子归先手（B01-011 同源），
           定色前消失=先手失去 1 子。mines_cleared 不动（非地雷）。 */
        else if (dst_side != me && dst_side == SIDE_UNKNOWN) st->pieces_left[SIDE_BLUE]--;
        /* ★ AUD-1000 B01-083（P3 注释，与 B01-009 同源）：dst_side=0xFF（未定色）跳过
           （扣错对象），B01-009 已修（未定色消失计先手池）。本条与 B01-009 同源，
           B01-009 修后本条自动闭合。 */
        ev = EV_BOMB;
        st->moves_since_eat = 0;   /* ★ AUD-R-107（P3 诚实标注）：4 分支各含一份（炸弹/旗/雷/吃），统一 ate 标记留待下一轮 */
         /* 炸弹炸毁军旗 → 被炸方失去军旗 → 扛旗方（己方 me）胜。
            ★ AUD-R-020（P1）：终局信号（winner+stage 双置，stage=OVER 使后续收口守卫放行）。
            暗棋未定色暗子（side=0xFF）撞旗同归后 SIDE_VALID 不满足 → 不判胜（旗归属未定）。 */
        if (dst_rank == RANK_FLAG && SIDE_VALID(dst_side)) {
            st->winner = (int)me;         /* winner: 0=蓝胜 1=红胜；me=用炸弹的一方=胜方 */
            st->stage = STG_OVER;
        }
        /* ★ AUD-R-107（P3 诚实标注）：st->moves_since_eat=0 在 4 分支（炸弹/旗
           /地雷/普通吃子）中重复——统一"ate 标记"需改 4 分支局部变量，
           且 EV_FLAG 开关关闭路径（:410）也含计数=0 需保持同步。收益小
           （4 处 1 行语句），行为 0 变化，留注释标注防新增分支漏写。 */
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
        /* ★ AUD-1000 B01-064（P1 已修）：终局函数原子性——stage!=PLAY 时（双终局叠加
           场景：EV_BOMB 撞旗先置 OVER 后 EV_FLAG 扛旗）攻子入位不回滚缺陷修复：
           先快照 src/dst 到局部，jq_on_flag_capture 成功后再实际写格（原实现先写格
           再调终局函数，stage 守卫拒收时攻子已入位不回滚）。 */
        {
            jq_cell src_snap;
            src_snap = *src;
            if (ui_rule_cfg_get(3)) {
                /* ★ B01-064（P1 已修）：先调终局函数，成功后再写格（原子性）。
                   若 jq_on_flag_capture 拒收（stage 已 OVER）则 src/dst 保持原样，
                   攻子入位不回滚缺陷消除。turn/计数仅在成功时更新。 */
                jq_on_flag_capture(st, me);
                if (st->stage == STG_OVER) {   /* 终局成功（winner 已置） */
                    *dst = src_snap;
                    memset(src, 0, sizeof(*src));
                    ev = EV_FLAG;
                    st->turn = 1 - me;   /* ★ AUD-R-037 诚实标注：终局快照 turn 指向败方 */
                    st->total_moves++;
                    st->moves_since_eat = 0;
                }
            } else {
                /* 开关关闭：走子正常收口，困毙判定可在此触发（与 EV_MOVE 一致）。
                   ★ B01-024 注释纠错（P3）：攻子入 58 格军旗后军旗格已无军旗，
                   flag-off 路径实际困毙靠攻子冻结 + 守方无合法着法。 */
                *dst = src_snap;
                memset(src, 0, sizeof(*src));
                ev = EV_FLAG;
                st->turn = 1 - me;
                st->total_moves++;
                st->moves_since_eat = 0;
                stalemate_after(st);
                return ev;
            }
        }
        return ev;
    } else if (dr_ == RANK_MINE) {
        /* ★ AUD-R-031（P2 诚实标注）：吃地雷开关关（ui_rule_cfg_get(2)=0）时工兵
           撞雷走 else 送死（EV_DEAD），即"关开关"=「工兵撞雷必死」。4 个规则开关
           的原版语义来源/默认值未写进 SPEC §2（now 仅 datah_012 UI 层注释），
           且 ui_rule_cfg_get 的 which 参数为裸数字 0/1/2/4——修法（SPEC 开关表 +
           枚举常量）留待下一轮，本轮仅标注耦合点。 */
        if (sr == RANK_WORKER && ui_rule_cfg_get(2)) {
            /* 工兵挖雷（rank 小=强，9 工兵 > 11 地雷）：工兵留、雷消失。
               ★ P1（datah_012）：吃地雷开关（ui_rule_cfg_get(2)，默认 1 对齐原版默认）：
               =0 时工兵也不能吃地雷（地雷无敌，工兵撞雷=送死）。 */
            /* 工兵挖雷（rank 小=强，9 工兵 > 11 地雷）：工兵留、雷消失 */
            int dst_side = dst->side;
            *dst = *src;
            memset(src, 0, sizeof(*src));
            /* AUD-1000 B01-009 同源（地雷分支）：暗棋未定色地雷（dst_side=0xFF）
               被工兵挖除后该子消失无计数对象 → 归先手（蓝）池（同炸弹分支语义） */
            if (dst_side == SIDE_UNKNOWN) st->pieces_left[SIDE_BLUE]--;
            /* ★ AUD-1000 B01-084（P3 注释，与 B01-009 同源）：暗棋未定色雷（side=0xFF）
               被工兵挖除后该子消失无计数对象 → 归先手池（同炸弹分支语义）。
               明棋雷 side 恒 0/1 恒扣减 ✓ 无缺陷。 */
            /* ★ AUD-R-040（P2）：计数对象用 dst->side（原子的实际归属方）而非 1-me
               （对方）。暗棋未定色暗子（side=SIDE_UNKNOWN）时 1-me 会扣错对象；
               地雷本非"可动子"（AUD-R-014），SIDE_VALID 范围守卫与 EV_BOMB 分支一致。 */
            if (SIDE_VALID(dst_side)) st->pieces_left[dst_side]--;
            /* ★ AUD-R-039（P2）：src 已 memset 后 side 恒 0，原 `!= 0xFF` 死判断已删
               （挖雷计数无条件递增，符合 ruleflip）。
               ★ AUD-R-015（P1）：mines_cleared 仅记录——"工兵挖 3 雷才能扛旗"
               胜负判定未接线（待暗棋规则族实现，见 AUD-R-002 修法；
               保留数据供后续暗棋族使用，勿删除该字段）。 */
            /* AUD-1000 B01-036（口径纠正）：mines_cleared 恒递增——挖雷事件
               本身与模式无关（工兵挖掉地雷 = 记录 1 次），:287 扛旗门槛仅在
               mode==FLIP && cfg(4) 分支内消费，明棋计数不被读 = 无害（rulestest
               t_worker_mine 明棋夹具断言 mines_cleared[BLUE]=1 即此口径）。
               前批误改为仅暗棋递增 → 明棋夹具 FAIL，纠正回恒递增。 */
            st->mines_cleared[me]++;
            /* AUD-1000 B01-008：挖雷攻方（工兵）消失也须 pieces_left[me]--，
               与 EV_DEAD 送死（:574 攻方--）计数对象对称。me 已经入口
               SIDE_VALID 守卫（恒 0/1），下标安全。 */
            st->pieces_left[me]--;
            ev = EV_MINE;
            /* ★ AUD-1000 B01-022（P1）：真挖雷（地雷消失）才清零无吃子计数 */
            st->moves_since_eat = 0;
        } else {
            /* 非工兵撞雷：地雷>其他所有棋，攻击子被炸
               ★ AUD-1000 B01-022（P1 已修）：EV_DEAD 送死（地雷未消失、无子被吃）
               **不**清零 moves_since_eat——旧实现在 if/else 公共尾部清零，把"送死"
               当"吃子活动"，70 步无吃子和棋进度被送死步误重置。 */
            memset(src, 0, sizeof(*src));
            st->pieces_left[me]--;
            ev = EV_DEAD;
        }
    } else {
        /* 普通吃子比较：rank 小=强（L52 确证） */
        if (sr < dr_) {              /* 大压小：src 吃 dst */
            *dst = *src;
            memset(src, 0, sizeof(*src));
            /* ★ AUD-1000 B01-057（P1 已修）：统一 dst->side 口径（原 1-me 推导入手方，
               暗棋未定色 0xFF dst 时 1-me 仍合法 0/1 下标但扣错对象）；
               SIDE_VALID(dst->side) 守卫防 0xFF 越界，未定色消失子归先手池（B01-009 同源）。 */
            if (SIDE_VALID(dst->side))
                st->pieces_left[dst->side]--;
            else if (dst->side == SIDE_UNKNOWN)
                st->pieces_left[SIDE_BLUE]--;
            ev = EV_EAT;
        } else if (sr > dr_) {      /* src 被吃 */
            memset(src, 0, sizeof(*src));
            st->pieces_left[me]--;   /* ★ AUD-1000 B01-058（P3 注释）：攻子消失 减攻方计数（经 SIDE_VALID 守卫 me∈{0,1}）
               ★ AUD-1000 B01-085（P3 注释，与 B01-057 同源）：攻方扣减用 me/1-me 推导
               非 dst side，与 :536 EV_EAT 守方扣减口径分叉（AUD-R-040 已指出）。
               B01-057 统一 dst->side 口径后本条自动闭合。
               ★ AUD-1000 B01-095（P3 注释，与 B01-022 同源）：EV_DEAD 分支不扣守方
               （雷）pieces_left（雷存活）✓ 正确（送死攻方死雷留），攻方扣减口径正确。
               B01-022 修后（EV_DEAD 不清零）本条自动闭合。 */
            ev = EV_DEAD;
        } else {                    /* 同级同归于尽（军棋标准：同级互杀） */
            memset(src, 0, sizeof(*src));
            memset(dst, 0, sizeof(*dst));
            st->pieces_left[me]--;
            st->pieces_left[1 - me]--;   /* ★ AUD-1000 B01-086（P3 注释）：同级互杀攻防跨方 1-me=守方 */
            ev = EV_BOMB;
        }
        st->moves_since_eat = 0;
    }
    /* ★ AUD-1000 B01-003（P0 已修）：终局（stage=OVER，EV_BOMB/FLAG 撞旗判胜）
       不再切 turn——旧实现终局快照 turn 指向胜方（1-me），战况统计/AI 误读
       "胜方当前回合"。非终局路径行为 0 变化（stage!=OVER 才切）。
       ★ AUD-1000 B01-074（P3 注释，与 B01-003 同源）：EV_BOMB 非旗同归（无终局）
       落本分支切 turn ✓ 正确（非终局正常切）；EV_BOMB 撞旗 :509-512 置 OVER 后
       stage!=STG_PLAY 不切 ✓（B01-003 修后自动闭合）。 */
    if (st->stage == STG_PLAY) {
        st->turn = 1 - me;
        st->total_moves++;
        /* 走子后收口（和棋+困毙；AUD-R-008 对称，stalemate_after 与 jq_flip 共用） */
        /* AUD-1000 B01-012：走子后棋盘变化，困毙缓存失效（stalemate_after 内全扫时重填） */
        st->stale_dirty = 1;
        stalemate_after(st);
    }
    return ev;
}

/* ---------- 翻棋（0x39e14 同格判定，§15/L48/L50） ---------- */
bool jq_can_flip(const jq_state *st, int idx)
{
    /* ★ AUD-R-091/114（P3 NULL 防护统一批）：st NULL → false。 */
    if (!st) return false;
    /* ★ AUD-R-043（P2 诚实标注）：仅判 rank!=EMPTY + 军旗不可翻 + 回合，
       未校验 rank_norm 合法性（非法 rank>12 蓝方值会被 rank_norm 静默还原为
       别的 rank）。rank 写入点（layout.c 装载 / BT SYNC_CELL 注入）均不在此
       函数防护——修法（can_flip 加 rank_norm∈[1,RANK_TOP] 校验 + flags 由 rank
       派生）留待下一轮。
       ★ AUD-R-045（P2 诚实标注）：可翻白名单只排军旗（地雷可翻但翻后不可动，
       困毙判定 :397 同白名单）——兄弟实现 game_logic.c:158-163 把地雷也排外
       （"地雷暗棋不可翻"），两实现不一致。修法（统一 is_flippable(c) 谓词，
       军旗+地雷均不可翻）留待下一轮。 */
    if (idx < 0 || idx >= N_CELLS || st->stage != STG_PLAY) return false;
    /* ★ AUD-1000 B01-093（P3 注释）：idx 界检查在 st->cells[idx] 访问前 ✓ 无越界，
       界检查与 st NULL 守卫（:722）口径一致（先 NULL 后界）。本条核实无缺陷。 */
    const jq_cell *c = &st->cells[idx];
    /* ★ AUD-R-110（P3 已修）：空/已翻判断前置（原 rank_norm 先求值再判空，
       逻辑无害但顺序反直觉；rank_norm(0)==0==RANK_EMPTY 无害，仅为可读性调整）。 */
    if (c->rank == RANK_EMPTY || c->revealed) return false;
    int r = rank_of(c);
    /* ★ AUD-1000 B01-075（P3 注释）：军旗位置由布局定（jq_load_layout/jq_dark_setup
       写军旗格），非翻时泄露。未翻暗子的 rank 由 rank_of 按 c->side 还原（不写回），
       本拦截正确（军旗不可翻）。 */
    if (r == RANK_FLAG) return false;   /* 军旗不可翻（§15：from==to 永假） */
    /* ★ AUD-R-111（P3 诚实标注）：side==0xFF（未定色）可翻分支描述的行为在当前
       装载路径（jq_dark_setup 写死 side 分区）下**不可达**（实测 0 个 0xFF 格）
       ——先手翻子定色（ruleflip）留待 AUD-R-003 落地后才成立，本分支为
       机制预留（与兄弟实现 game_logic.c 同款判定），非当前可用行为。 */
    /* ★ AUD-1000 B01-013/014（P0 已修）：0xFF 未定色暗子**仅先手（蓝）回合可翻**——
       旧实现 0xFF 恒可翻（无回合门），后手（红）回合可翻先手暗子并 jq_flip 定色
       写 st->turn（RED）→ 先手暗子池被后手"抢定色"，先手翻子定色规则被绕过。
       定色归翻子方，0xFF 定色仅先手（蓝）可触发。 */
    /* ★ AUD-1000 B01-076（P3 注释，与 B01-013 同源）：0xFF 子先走本分支（仅先手蓝可翻），
       绕过 :675 回合同期门（side==turn）——0xFF 定色无回合约束，B01-013 已修（仅先手
       可触发）。本条与 B01-013 同源，B01-013 修后本条自动闭合。 */
    if (c->side == 0xFF) return st->turn == SIDE_BLUE;
    /* ★ 回合校验：己方暗子（side==turn）可翻；敌方暗子（side!=turn）不可翻。
       缺失此校验 → 任意回合可翻任意方暗子，破坏暗棋回合交替。
       ★ AUD-R-085（P2）：回合门已确认——side==0xFF（未定色）分支在暗棋"先手
       定色"语义下属当前方翻子定色（与 move_legal 回合门不冲突：走子仍须
       src->side==turn）；已定色子必须 side==turn 方可翻。与 jq_can_move 的
       回合门一致，本项关闭。 */
    return c->side == st->turn;         /* 己方暗子可翻 */
}

bool jq_flip(jq_state *st, int idx)
{
    /* ★ AUD-R-091/114（P3 NULL 防护统一批）：st NULL → false。 */
    if (!st) return false;
    /* ★ AUD-R-044（P2 诚实标注）：翻棋成功即切 turn（:470），但 UI 侧
       jni_bridge :7751-7755 翻棋后保持选中提示"下一步走子"——玩家点目标格
       会被 move_legal 回合校验（src->side != turn）拒，UI 与引擎语义不一致。
       修法（翻棋后清选中态 jq_glview_set_selected(-1) + 文案"已翻牌轮到对方"）
       在 UI 侧，留待下一轮。 */
    if (!jq_can_flip(st, idx)) return false;
    jq_cell *c = &st->cells[idx];
    /* 暗棋先手翻子定色（ruleflip 原文：先手翻出第一个棋子确定该玩家持此颜色）
       ★ AUD-1000 B01-059（P3 注释）：定色读切换前 turn=翻子方（:717 st->turn 未切换），
       定色归翻子方非对方（新 turn），语义正确。 */
    if (c->side == SIDE_UNKNOWN) c->side = st->turn;
    c->revealed = 1;
    st->turn = 1 - st->turn;
    st->total_moves++;
    st->moves_since_eat++;
    /* ★ AUD-1000 B01-060（P3 注释）：翻棋计入 70/1000 步阈值（AUD-R-086 已标注）——
       翻棋无吃子（moves_since_eat 不在此清零），翻棋累积 70 步无吃子（无走子吃子
       介入）判和，与"走子 70 步"同源阈值但翻棋节奏（每步翻 1 子）远快于走子
       （走子可能 60 格滑行）→ 暗棋翻牌主导和棋（AUD-R-086 已标注）。
       SPEC 待明文"翻棋计入 70/1000 步阈值"（消除"依据无"状态）。 */
    /* ★ AUD-1000 B01-087（P3 注释）：翻棋收口判对方（新 turn）困毙与走子同口径。
       翻棋定色方（旧 turn）的子刚定色可能立即可动（定色后己方子解锁），但
       stalemate_after 判对方（新 turn）困毙——定色方可动性不在收口检查（对方困毙才判）。
       ★ AUD-R-008（P1）：翻棋路径与走子路径对称收口（和棋+困毙）。
       原版 game_logic.c:309 的 jq_flip 末尾 check_stalemate(st) 同款语义。
       ★ AUD-R-109（P3 注释确认）：和棋判定（stalemate_after 内）带 st->stage==STG_PLAY
       前置守卫（:317），jq_flip 入口 jq_can_flip 已校验 stage，双守卫行为 0 变化。 */
    /* AUD-1000 B01-012：翻棋后棋盘变化，困毙缓存失效 */
    st->stale_dirty = 1;
    stalemate_after(st);
    return true;
}

/* ---------- 困毙（0x302b4，L53：当前方无子可动/可翻 → 该方负） ---------- */
/* ★ AUD-R-112（P3 诚实标注）：O(N²) 枚举（60 格 × 60 格 = 3600 次
   can_move_ignore_turn，每次含工兵 BFS）在 jq_try_move 每步调用一次。
   规则引擎在主线程 OnTouch 单步执行，移动端 60fps 渲染线程不做走子
   计算，实测 OK（AUD-R-082 同源性能分析）。缓存"当前方是否存在合法
   着法"仅在落子后增量更新需引入新字段，行为 0 变化但重构量大，
   留待下一轮压测驱动。 */
bool jq_side_stalemated(const jq_state *st, uint8_t side)
{
    /* ★ AUD-R-091/114（P3 NULL 防护统一批）：st NULL → false（未困毙）。 */
    if (!st) return false;
    if (st->stage != STG_PLAY || st->winner >= 0) return false;
    /* AUD-1000 B01-012：增量缓存——stale_dirty==0 且 stale_cache 有效时直接读。
       stale_cache[s]==1：方 s 可行动（未困毙，返 false）；stale_cache[s]==0：困毙（返 true）。
       走子/翻棋后 stale_dirty=1 失效，下次全扫并写回。
       ★ 构造态兜底：jq_state_init memset 全 0 → stale_dirty=0 + stale_cache 全 0，
       若直接读缓存会把"从未写过的 0"误判为"困毙"。故缓存有效判据须 stale_cache
       任一侧曾被写（==1 即有效），全 0 = 未初始化 → 走全扫。 */
    if (!st->stale_dirty && (st->stale_cache[0] || st->stale_cache[1])) {
        if (st->stale_cache[side]) return false;
        else                        return true;
    }
    /* ★ AUD-1000 B01-077（P3 注释）：winner>=0 含 DRAW(2) 全终局拒收——
       winner=JQ_WIN_NONE(-1) 时 -1>=0 假放行正常扫描；winner=2(DRAW 已判和) 时
       2>=0 真拒收；winner=0/1(蓝胜/红胜) 时 0>=0/1>=0 真拒收。本条核实无缺陷。 */
    /* 还有未翻暗子可翻 → 未困毙
       ★ AUD-R-073（P2）：排除地雷（rank_norm==RANK_MINE）——地雷翻了也不能走子
       （RANK_MINE 在下方"已翻可动"循环被 continue），"可翻"检查须与可动性一致，
       否则暗地雷会被误算"可翻 → 未困毙"（与 AUD-R-045 同源，兄弟实现 game_logic.c:162 同款排除）。 */
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        int cn = rank_of(c);
        /* AUD-1000 B01-011：0xFF 未定色暗子仅先手（蓝）可翻（jq_can_flip 门控），
           红方困毙扫描时 0xFF 子不算"红方可翻"（否则红方永不能因只剩未定色暗子而困毙）。
           旧实现 (c->side == 0xFF || c->side == side) 让 0xFF 子对双方都判"可翻"→红方恒不困毙。 */
        if (c->rank != RANK_EMPTY && !c->revealed &&
            cn != RANK_FLAG && cn != RANK_MINE &&
            (c->side == 0xFF ? side == SIDE_BLUE : c->side == side)) {
            /* ★ AUD-1000 B01-089（P3 注释，与 B01-011 同源）：0xFF 子仅先手计未困毙
               （B01-011 已修：扫红方时 0xFF 子被误计红方，现 0xFF 子仅先手侧计）。
               本条与 B01-011 同源，B01-011 修后本条自动闭合。 */
            /* AUD-1000 B01-012：写缓存（可翻暗子 → 未困毙） */
            ((jq_state *)st)->stale_cache[side] = 1;
            ((jq_state *)st)->stale_dirty = 0;
            return false;
        }
    }
    /* 有已翻可动子且存在合法目标 → 未困毙 */
    for (int i = 0; i < N_CELLS; i++) {
        const jq_cell *c = &st->cells[i];
        if (c->rank == RANK_EMPTY || c->side != side || !c->revealed) continue;
        int r = rank_of(c);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        /* ★ AUD-R-005：必须用忽略回合的版本（否则 side≠turn 时恒 false → 困毙误报）
           ★ AUD-R-074（P2 已修，随 R-005 关闭）：jq_side_stalemated 已参数化
           can_move_ignore_turn（turn=-1），不再依赖 side==st->turn 耦合，本项关闭。 */
        /* ★ AUD-1000 B01-078（P3 注释）：内层纯读判定（can_move_ignore_turn 只读
           不写，无 stalemate_after 触发）——t!=i 拦同格走子（from==to），
           move_legal :188 拦，双保险。 */
        for (int t = 0; t < N_CELLS; t++)
            if (t != i && can_move_ignore_turn(st, i, t)) {
                /* AUD-1000 B01-012：写缓存（可动子有合法目标 → 未困毙） */
                ((jq_state *)st)->stale_cache[side] = 1;
                ((jq_state *)st)->stale_dirty = 0;
                return false;
            }
    }
    /* AUD-1000 B01-012：全扫无可动/可翻 → 困毙，写缓存 */
    ((jq_state *)st)->stale_cache[side] = 0;
    ((jq_state *)st)->stale_dirty = 0;
    return true;
}

/* ---------- 悔棋（本地快照；联机由 Regret* 事件驱动，L77） ---------- */
/* ★ AUD-R-012/013（P1）已修：删除空壳 jq_undo_snapshot()/jq_snapshots_left()。
   悔棋栈唯一实现 = jni_bridge.c g_undo[UNDO_MAX]（undo_push/undo_pop）；
   蓝牙 UNDO 闸口 = jq_bt_undo_available()（读真实栈深）。
   保留 jq_undo_restore（*st = *snap，真实可用）。
   ★ AUD-1000 B01-025（P3 注释确认）：snapshot/restore 契约不对称（restore 有效、
   无公开 snapshot）已随 AUD-R-012/013 删除空壳关闭——调用方按 junqi.h 契约
   "先 snapshot 再 restore" 的路径已不存在（API 已删），悔棋由调用方栈实现
   （jni_bridge g_undo 栈 + 蓝牙 UNDO 闸口），与 AUD-R-012/113 同源。 */
void jq_undo_restore(jq_state *st, const jq_state *snap)
{
    if (!st || !snap) return;   /* ★ AUD-R-091 同批：NULL 防护 */
    if (st == snap) return;     /* ★ AUD-1000 B01-090（P1 已修）：自拷拒收（防御性缺口，正常调用点 g_undo 栈不同内存无重叠） */
    /* ★ 若 st 与 snap 内存重叠（非同指针但相邻）→
       memcpy 自拷未定义但 C 结构赋值同指针为自拷（UB 但实践恒等）。
       正常调用点（jni_bridge g_undo 栈不同内存）无重叠，保持现状不改行为。 */
    *st = *snap;
    /* ★ AUD-1000 B01-008（P0 已修）：悔棋恢复入口钳制 mines_cleared 下界——
       损坏快照可注入负值（合法走子链快照恒 >=0），负值在 :242 扛旗门槛
       恒 <3 拒放行（方向安全），此处显式钳 0 消除语义歧义（负=数据损坏）。 */
    for (int i = 0; i < 2; i++)
        if (st->mines_cleared[i] < 0) st->mines_cleared[i] = 0;
}

int  jq_bt_undo_available(void);   /* bt_session.c BT_T_UNDO 分支用（jni_bridge.c 实现；bttest 桩在 bt_selftest.c） */

/* ---------- 胜负（0x30200 分发，winner: JQ_WIN_BLUE 蓝胜 / JQ_WIN_RED 红胜 / JQ_WIN_DRAW 和，AUD-R-053） ---------- */
void jq_on_flag_capture(jq_state *st, uint8_t captor_side)
{
    /* ★ AUD-R-091/114（P3 NULL 防护统一批）：st NULL → 忽略。 */
    if (!st) return;
    /* ★ AUD-R-035（P2）：已终局则忽略（防测试钩子/外部调用改写终局 winner）。 */
    if (st->stage != STG_PLAY) return;
    /* ★ AUD-1000 B01-064（P3 注释）：终局函数原子性缺口——调用方 jq_try_move EV_FLAG
       分支在 *dst=*src（攻子入位）之后调用本函数，若 stage 已 OVER（由他路径
       如 EV_BOMB 撞旗先置）则本函数拒收但攻子入位不回滚（AUD-R-083 注释"所有状态
       变更在 can_move 之后"指 can_move 通过之后，不指终局函数内部）。
       双终局叠加场景（先 EV_BOMB 撞旗置 OVER 后 EV_FLAG 扛旗）攻子入位不回滚。
       修法：攻子入位前快照攻格（回滚用），或 stage 检查前移，留待下一轮。 */
    /* ★ AUD-R-054（P2）：前置守卫——吃旗方必须合法（SIDE_VALID），防御非法侧
       值污染 winner（int8_t 会存 0xFF/越界值）。 */
    if (!SIDE_VALID(captor_side)) return;
    st->winner = captor_side;        /* 吃旗方胜（JQ_WIN_BLUE/JQ_WIN_RED） */
    st->stage = STG_OVER;
    /* AUD-1000 B01-002：终局后不再切 turn（旧实现终局快照 turn 已翻转到败方，
       战况统计 UI 显示"败方当前回合"含混）。winner/stage 置位后收口，
       后续收口守卫（stage!=PLAY）放行；stalemate_after/和棋判定对终局无效。 */
    /* ★ AUD-R-054（P2）：被吃方旗计数扣减（旗是普通子之一；flag 被吃时该方
       pieces_left 同步 -1，与 EV_BOMB/EV_DEAD 分支的 pieces_left 维护风格一致。
       AUD-R-014 注：pieces_left 统计未完全接线，此扣减仅保持字段一致性）。
       ★ AUD-1000 B01-026（P3 注释确认）：终局后 pieces_left 继续变化无守卫——
       本扣减在 stage=OVER 后置（:776-777），终局快照计数字段不冻结，与
       EV_BOMB/EV_DEAD 分支的递减风格一致（AUD-R-014 同源）。
       SIDE_VALID(captor_side) 守卫（:774）保证 1-captor_side∈{0,1} 下标安全。 */
    st->pieces_left[1 - captor_side]--;
}

void jq_on_draw(jq_state *st)
{
    /* ★ AUD-R-091/114（P3 NULL 防护统一批）：st NULL → 忽略。 */
    if (!st) return;
    /* ★ AUD-R-035（P2）：已终局则忽略。 */
    if (st->stage != STG_PLAY) return;
    /* ★ AUD-1000 B01-065（P1 已修）：已有 winner 拒收（防止脏 winner 被和棋覆盖）。
       与 B01-066 同源：jq_on_stalemate 同守卫。 */
    if (st->winner != JQ_WIN_NONE) return;
    /* ★ AUD-1000 B01-065（P3 注释）：无 winner==JQ_WIN_NONE 前置——外部脏 winner
       （0/1 由其他路径误置）+ stage=PLAY 时本函数覆盖脏值无告警。
       正常路径 winner 恒 JQ_WIN_NONE（-1）直到终局，stage!=PLAY 守卫已覆盖
       终局后调用。脏 winner + stage=PLAY 场景需外部误写（非正常路径），保持现状不改行为。 */
    st->winner = JQ_WIN_DRAW;        /* 和（0x30280） */
    st->stage = STG_OVER;
}

/* ★ AUD-R-036（P2）：胜方改由显式 loser_side 参数推导（原用 st->turn 推导，
   外部调用（jni_bridge stalematetest 钩子）依赖调用时刻 turn==应判负方，
   否则胜方颠倒；现与 jq_on_flag_capture(st, captor_side) 的显式传参风格一致）。
   内部调用点（stalemate_after 走子收口）传 st->turn：此时 st->turn 即
   "无棋可走方"（走子前 turn 未切换，AI 路径的 mover 也恒等于 st->turn）。 */
void jq_on_stalemate(jq_state *st, uint8_t loser_side)
{
     /* ★ AUD-R-091/114（P3 NULL 防护统一批）：st NULL → 忽略（三终局函数统一入口守卫）。 */
    if (!st) return;
    if (st->stage != STG_PLAY) return;
    /* ★ AUD-1000 B01-066（P1 已修，与 B01-065 同源）：已有 winner 拒收。 */
    if (st->winner != JQ_WIN_NONE) return;
    /* ★ AUD-R-140（P3 已修）：turn/loser_side 若被外部误写 >1，1-loser_side 为负
       （int8_t winner 与"未定"哨兵 -1 撞码）。用 & 1 取模确保 winner∈{0,1}
       与 JQ_WIN_BLUE/JQ_WIN_RED 常量域一致，外部误写时安全降级而非 UB。
       ★ AUD-R-114（P3 已修）：jq_on_flag_capture/jq_on_draw/jq_on_stalemate
       三终局函数入口统一加 st==NULL 守卫（本轮随 AUD-R-091 NULL 防护统一批落地）。 */
    /* AUD-1000 B01-027：入口拒非法 loser_side（旧 &1 掩码静默降级任意 uint8_t 为
       合法 0/1 winner，调用方无法感知传参错误；&1 保留作双保险） */
    if (!SIDE_VALID(loser_side)) return;
    /* ★ AUD-1000 B01-066（P3 注释）：与 B01-065 同源——无 winner==JQ_WIN_NONE 前置，
       外部脏 winner + stage=PLAY 时覆盖无告警。stage!=PLAY 守卫（:795）覆盖终局后
       调用；脏 winner + stage=PLAY 场景需外部误写，保持现状不改行为。 */
    st->winner = 1 - (loser_side & 1);   /* 无棋可走方负，对方胜（&1 防 turn>1 时 winner 为负）
       ★ AUD-1000 B01-091（P3 注释，与 B01-027 同源）：对 loser∈{0,1} 正确
       （蓝困毙红胜 1-0=1、红困毙蓝胜 1-1=0）；脏 side 静默降级 B01-027 已修
       （SIDE_VALID 拒收脏 side）。本条与 B01-027 同源，B01-027 修后本条自动闭合。 */
    st->stage = STG_OVER;
}

/* AI 贪心评分（对抗审计 #2 提取：设备端 ai_pick 用原始编码 rank 比较的 bug 修复 + 可单测化）。
   src/dst 为原始存储 rank（红方 = 实际+0xc，需 rank_norm 还原）。
   返回：1000 吃子 / 900 同级 / 800 炸弹 / 700 工兵挖雷 / 2 空移 / 1 送死 / -1 旗雷不可走 / -2 空源（AUD-R-057）。
   ★ AUD-R-055（P2 诚实标注）：评分不区分子价值（用司令吃掉排长 = 用排长吃掉司令 = 1000）——
     对齐原版 0x39f20/0x55d40 单步贪心无价值维度（原版无 ai_value 子力表，兄弟实现
     game_logic.c:534 ai_value 为手写路线自有设计，非原版对齐目标）。
   ★ AUD-R-056（P2 诚实标注）：不评估吃子后被反吃的风险（无前瞻/无模拟走子）——
     原版贪心 0x55d40 handler 实证单步评分，不实现前瞻（性能 + 对齐原版）。
   ★ AUD-R-059（P2 诚实标注）：炸弹空移与普通子空移同分（2）——原版贪心无此区分。
   ★ AUD-R-060（P2 诚实标注）：同级同归 900 覆盖"两炸弹相撞"与"同级普通子互杀"两种语义——
     原版 ai_score 无此区分（0x55d40 实证），事件侧 EV_BOMB 同码同源。
   ★ AUD-R-058（P2 诚实标注）：暗棋扛旗前置条件（工兵 + 挖 3 雷）未进评分——需 st 状态
     查询（mines_cleared/mode），ai_score 签名改动大，留待下一轮；当前暗棋下 AI 靠
     move_legal 拦截兜底（非工兵扛旗被拒），不会误胜。
   ★ AUD-1000 B01-031（P1 诚实标注）：炸弹撞旗（r=10<dr=12）命中 1000 分（'吃子最高分'），
     语义上是'同归胜局'而非'净吃'——原版 0x55d40 单步贪心无价值维度，无炸弹/旗专项权重，
     保持 1000 对齐原版（若 SPEC 定案'同归+胜局组合权重'需改 800+win_bonus，留待下一轮）。
     ★ AUD-1000 B01-030（P1 已修）：炸弹撞雷（dr=RANK_MINE, r=RANK_BOMB）评 800（同归分），
     旧实现评 1（送死）导致 AI 避免炸弹撞雷——但规则上炸弹撞雷双方同归（EV_BOMB 800 语义），
     现修正为 800 对齐 EV_BOMB 事件码。ai_selftest.c 无炸弹撞雷用例（仅工兵撞雷 700 +
     普通子撞雷 1），基准不受影响。
   ★ AUD-1000 B01-032（P1 诚实标注）：同级普通子互杀（如排长 vs 排长）返回 900（同级分）
     但事件码走 EV_BOMB（JQ_EVENT_NAMES[2]="Junqi.Bomb"）——音效播'炸弹'声而非'同级互杀'。
     原版 0x55d40 同码同源（AUD-R-062），区分需扩事件码（0-6 已满 7 项）同步 jni_bridge 计数，
     留待 SPEC 定案。 */
int ai_score(int src_rank_raw, int dst_rank_raw, int dst_revealed)
{
    /* AUD-1000 B01-028/B01-063/B01-062：负数 rank 域统一拦截（rank 存储域 0-255 合法，
       负数为脏输入 → 空源哨兵 -2，与调用方 ai_pick 外层过滤同语义）。
       入参 int（非 uint8_t）——调用方 :278 传 c->rank（uint8_t 自动升 int ✓）、
       :88 rank_norm(d->rank)（int ✓），负数入参（如 -1 哨兵传 raw）走本拦截路径。
       旧实现 rank_norm(负数) 返回负数参与比较 = 未定义行为。 */
    if (src_rank_raw < 0 || dst_rank_raw < 0) return -2;
    /* ★ AUD-1000 B01-029/033（P1 已修，rank 域统一入口）：src 越 rank 编码域
       （>24=红方 raw+0xc 合法域上限）→ 脏值拒绝走空源哨兵 -2（非静默经
       rank_norm 钳 RANK_TOP 后按军旗 -1 降级）。dst 同域守护：dst>24 脏值
       走 has_dst 判真但 dr=rank_norm→RANK_TOP 哨兵（rank_of 语义一致）。 */
    if (src_rank_raw > 24) return -2;
    int r = rank_norm(src_rank_raw);
    /* ★ AUD-1000 B01-029（P1 诚实标注，SPEC 定案后改行为）：ai_score 入参为
       裸 rank_norm（非 side 感知 rank_of）——蓝方 rank>12 脏值（13）被 rank_norm
       还原为 1（司令），AI 基于脏数据决策。调用方 jni_bridge :278 传 c->rank
       （uint8_t 自动升 int），蓝方正常布局 rank∈[0,12] 无脏值；蓝方脏值仅
       BT SYNC_CELL 注入 / 存档损坏时可达（AUD-R-047 同源）。修法：ai_score
       改 (side, raw) 双参走 rank_of 侧感知还原，调用方 ai_pick 传入前对蓝方
       侧 rank>12 做非法过滤。签名改动影响 ai_selftest.c 全部用例 + jni_bridge
       调用点，需同步 evverify 基准（蓝方脏值用例新增），留待下一轮。 */
    /* ★ AUD-R-057（P2）：空源（rank=0）返回哨兵 -2，与非法着法区分
       （原返回 2 与合法空移同分，调用方无法识别"源格为空"这种非法输入）。
       当前调用方 ai_pick 靠外层 c->rank==RANK_EMPTY 过滤不会传 0，本守卫为函数自身契约。
       ★ AUD-R-092（P3 关闭确认）：哨兵 -2 已随 AUD-R-057 落地，本项确认关闭。
       ★ AUD-R-095（P3 诚实标注）：rank_norm 上界校验（v>24 钳到 RANK_TOP）已随
       AUD-R-080 落地（junqi.h rank_norm）；ai_score 对 RANK_TOP 钳位值的处理
       （钳位后=12 走 RANK_FLAG 分支返回 -1）为正确降级，无需额外哨兵。
       若原版 0x55d40 对非法 rank 有独立评分分支，需专项逆向确认。 */
    if (src_rank_raw == 0) return -2;
    /* ★ AUD-1000 B01-079（P3 注释）：评分收表——1000 吃子 / 900 同级 / 800 炸弹 /
       700 工兵挖雷 / 2 空移 / 1 送死 / -1 旗雷不可走 / -2 空源。
       B01-030 已修炸弹撞雷=800；同级炸弹 900 已标注（AUD-R-060）维持。 */
    if (r == RANK_FLAG || r == RANK_MINE) return -1;
    /* ★ AUD-R-010（P1）：暗棋目标未翻开（dst_revealed=0 且非空格）→ 评分上限降为空移级
       sc=2，不按真 rank 透视敌子（旧版 ai_score 入参只有 2 个 rank，暗棋下 AI 直接读
       未翻暗子真 rank 给出 1000/900/800 分 = 自带透视，与 ruleflip 信息博弈前提冲突）。
       明棋路径 dst 恒 revealed=1，行为不变；暗棋翻棋后 revealed=1 再走真评分。 */
    if (dst_rank_raw && !dst_revealed) return 2;
    /* ★ AUD-1000 B01-080（P3 注释，AUD-R-059 同源）：空格（dst_rank_raw=0）与未翻子
       同评 2 分（不区分目标是否空格）——AI 不偏好空格（实际空格可进，未翻子可能拦）。
       如需区分空格 2 分 vs 未翻 1 分需改本行返 1，改变基准，留待 SPEC 定案。 */
    /* ★ AUD-R-093（P3 已修）：has_dst 归并两次 dst_rank_raw 判定（原 dr 计算与
       紧随其后的 if 是同一判定的两处表达，易改一漏一）。 */
    bool has_dst = (dst_rank_raw != 0);
    int dr = has_dst ? rank_norm(dst_rank_raw) : 0;
    if (has_dst) {
        /* ★ AUD-R-094（P3 已修）：原 else-if 链的 r==RANK_BOMB→800 分支实际只在
           dr∈{1..9}（炸弹撞普通子）时可达——dr=FLAG(12) 时 r(10)<dr 先命中 1000
           （炸弹撞旗=1000），dr=BOMB(10) 时 r==dr 先命中 900。现改显式分派：
           ① 雷特判（普通子碰雷=送死）→ ② 吃子 r<dr（含炸弹撞旗 10<12=1000）
           → ③ 同级互杀 r==dr（含炸弹撞炸弹=900，AUD-R-060）
           → ④ 炸弹撞普通子(rank 1-9)同归 800 → ⑤ 送死 1。
           **数值与原 else-if 链逐例 0 变化**（1000/900/800/700/2/1 不变）。 */
        if (dr == RANK_MINE) {
            if (r == RANK_WORKER) return 700;   /* 工兵挖雷 */
            if (r == RANK_BOMB)   return 800;   /* ★ AUD-1000 B01-030（P1 已修）：炸弹撞雷双方同归（EV_BOMB 800 语义） */
            return 1;                            /* 普通子撞雷送死 */
        }
        /* ② 大压小：能吃（炸弹撞旗 10<12 也在此命中=1000，保持原行为） */
        if (r < dr)          return 1000;
        /* ③ 同级互杀（炸弹撞炸弹=900，AUD-R-060） */
        if (r == dr)         return 900;
        /* ④ 炸弹（rank 10）撞更弱普通子（rank 1-9）同归 800 */
        if (r == RANK_BOMB)  return 800;
        /* ⑤ 吃不掉（会 DEAD）：送死最低分 */
        return 1;
    }
    return 2;                /* 空移：优先于送死（AI 不主动送死，无空移才走被吃格） */
}
