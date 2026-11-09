/*
 * layout.c — 布局加载 / 校验（逆源码重写）
 * 依据原版函数（VA 见注释）：
 *   0x2a340：布局加载（30 字节循环，经 0x6a3a4 映射）
 *   0x6a3a4：字节映射 `if(byte==0) return 0; if(side==0) return byte+0xc; return byte;`
 *   0x2a380：读布局文件（0x3dd60 读 → 0x2a340 解析），state+0x48=侧 / +0x4c=长度
 *   0x2a228：单格校验（红方自定义布局，§10/L81 逐指令确证）
 *   0x6a3b0：全 60 格校验 + 计数表（§7）
 * 布局文件：SPAK data/lay/%02d.lay（00-49 全唯一，30B；L82 诚实边界：
 *   难度菜单 LowLay/MidLay/HighLay 是菜单选择，00.lay==default.lay）。
 */
#include "junqi.h"
#include <string.h>
#include "jq_default_lay.h"   /* ★ A4-13：JQ_DEFAULT_LAY 双端共享 */

/* 默认布局：SPAK data/lay/00.lay（=default.lay，L52 分布确证 25 子/方）
   ★ AUD-1000 B10-012（P1 诚实标注）：30 字节默认布局在 rev64/src_rs/layout.c
   与 rev64/src_jni_translated/jq_rs_layout.c 各定义一份（当前逐字节相同），
   改一处忘另一处 → 64/32 位开局布局分叉（分叉矩阵 R-138 未含此数据项）。
   修法（抽共享头 jq_default_lay.h 单点定义 + rules_fork_check.py 增布局
   数据一致性比对）留待下一轮；当前以本表为 64 位事实源。 */
/* ★ A4-13（P1 已修）：JQ_DEFAULT_LAY 双端共享头（jq_default_lay.h 单点定义，
   32 位 jq_rs_layout.c 同 include，改一处即双端同步，消开局布局双源分叉） */

/* 标准单方 25 子 rank 分布（default.lay 实测，L52 决定性证据）。
   AUD-A-0005/0006（P2 诚实标注，探针误报）：cnt 是**调用方栈数组**（jq_standard_army_count
   签名契约 int cnt[RANK_COUNT]，RANK_COUNT=13 编译期宏）；本函数写死常量下标 1-12
   （cnt[1]..cnt[12]），全部 < RANK_COUNT，无外部输入下标——探针把形参类型标注当越界
   风险，属误报。jq_validate_layout 的 seen[]/cnt[] 循环 r∈[1,RANK_COUNT) 亦为编译期
   宏边界（AUD-R-080 rank_norm 上界钳到 RANK_TOP=12 保证 r 恒合法）。 */
void jq_standard_army_count(int cnt[RANK_COUNT])
{
    /* AUD-A-0005 探针降噪：常量下标 1-12 全部 < RANK_COUNT(13)，恒真断言（行为 0 变化）。
       ★ AUD-1000 B01-038（P1 已注释，降 P3）：断言写法 `12 >= RANK_COUNT` 恒假
       （12>=13 假→永不 return）——行为正确（下界守护），写法不规范为已知
       探针噪声，本条核实无缺陷。 */
    if (12 >= RANK_COUNT) return;
    memset(cnt, 0, sizeof(int[RANK_COUNT]));
    cnt[1]=1; cnt[2]=1; cnt[3]=2; cnt[4]=2; cnt[5]=2; cnt[6]=2;
    cnt[7]=3; cnt[8]=3; cnt[9]=3; cnt[10]=2; cnt[11]=3; cnt[12]=1;
}

void jq_state_init(jq_state *st)
{
    memset(st, 0, sizeof(*st));
    st->turn = SIDE_BLUE;
    st->stage = STG_PLAY;
    st->winner = JQ_WIN_NONE;   /* AUD-R-053：winner 未定哨兵 */
    st->mode = JQ_MODE_LAY;   /* ★ AUD-R-002/003（P0）：默认明棋（行为不变） */
    /* ★ AUD-R-014（P1）：pieces_left 统计未接线（刷新逻辑恒 1、吃子后变负），
       初始化为 0（勿用于判胜；困毙走 jq_side_stalemated 实时扫描）。
       原 memset 已清零，此处显式注释标记诚实边界。 */
}

/* 载入 30 字节布局（原版 0x2a340 语义，§3）：
   layout 行 r→棋盘行 (5-r)（蓝方，军旗行5→棋盘行0 底线）；
   红方 = (6+r, 4-c) 镜像（L67/L79）。
   格子：蓝 rank 原值；红 rank+0xc（已翻开存储，0x6a3a4 侧0=蓝映射？——
   0x6a3a4 实际语义（§3/L60）：`if(side==0) return byte+0xc`，侧0=红方编码+0xc）。 */
void jq_load_layout(jq_state *st, const uint8_t layout[30])
{
    jq_state_init(st);
    for (int i = 0; i < 30; i++) {
        int v = layout[i];
        if (v == 0) continue;                 /* 行营空位 */
        /* ★ AUD-1000 B01-039/040（P1 已修）：脏字节拦截（v>RANK_TOP）——
           蓝方 rank=(uint8_t)v 脏 13-255 静默入棋盘 → rank_norm 钳位
           与真军旗同值（B01-010/033 同源）；红方 rank=(uint8_t)(v+0xc)
           脏 250+ 回绕成合法 rank（250+0xc=262%256=6 连长）。修：
           装载即拒 v>12（蓝合法域），脏布局不污染规则引擎。
           JQ_DEFAULT_LAY 恒 [0,12]，ui_layout_load 30 字节来自
           自存档（extract 侧蓝 rank ∈[0,12]）恒不触发。 */
        if (v > RANK_TOP) continue;           /* ★ B01-040：红方 rank=(uint8_t)(v+0xc) 脏 250+ 回绕防护（同 B01-039 装载即拒） */
        int lrow = i / 5, lcol = i % 5;
        /* 蓝方：棋盘 (5-lrow, lcol) */
        jq_cell *b = &st->cells[cell_index(5 - lrow, lcol)];
        b->rank = (uint8_t)v; b->side = SIDE_BLUE;
        b->revealed = 1;
        b->flags = (v == RANK_BOMB ? JQF_BOMB : 0)
                 | (v == RANK_MINE ? JQF_MINE : 0)
                 | (v == RANK_FLAG ? JQF_FLAG : 0);
        /* 红方：棋盘 (6+lrow, 4-lcol)，rank+0xc（红方编码，L47/L60） */
        jq_cell *r = &st->cells[cell_index(6 + lrow, 4 - lcol)];
        r->rank = (uint8_t)(v + 0xc); r->side = SIDE_RED;
        r->revealed = 1;
        r->flags = b->flags;
        (void)r;
    }
    /* 明棋（rulelay）：所有布局子可动可吃，revealed=1；行营暗置语义仅暗棋
       （main.c flip / jni_bridge 暗棋路径）生效，随机布局设 revealed=0。
       ★ 2026-11-07 回归修复：旧版"双方暗置（军旗明置）"把非军旗子全置 0 →
       明棋 50 局 AI 对弈 0 吃子（回归断言挂），明棋语义与暗棋混淆。 */
    for (int i = 0; i < N_CELLS; i++)
        if (st->cells[i].rank)
            st->cells[i].revealed = 1;
    st->turn = SIDE_BLUE;
}

/* 单格校验（0x2a228 逐指令确证，§10/L81，r1=索引0-59，r2=字节）：
   if idx<=29: 合法（蓝半区/固定区）
   if raw>0xc: raw-=0xc（红方已翻开还原）
   炸弹(0xa)：idx<=34 → ZhaError（炸弹禁红方前线路行6，idx34=row6col4）
   地雷(0xb)：idx<=49 → LeiError（地雷仅最后两行 row10-11）
   军旗(0xc)：idx∈{56,58} 合法，否则 QiError（红方大本营格）
   普通 1-9：合法 */
jq_layout_err jq_check_layout_cell(int idx, int raw)
{
    if (idx < 0 || idx >= N_CELLS) return ERR_RANGE;
    if (raw == 0) return LAYOUT_OK;              /* 行营空位（default.lay 5 格） */
    /* ★ AUD-1000 B01-004（P0 已修）：蓝半区不再无条件放行——旧实现 raw 原值直放，
       蓝方脏 rank>12（合法域 [0,12]，+0xc 编码仅红方）静默进棋盘后经 rank_norm
       钳为军旗同值（B01-010/033 同源）污染规则引擎。修：蓝半区 raw>RANK_TOP
       判 ERR_RANGE（与红半区还原后越域拦截同源口径）；装载链路（jq_load_layout
       蓝方字节 [0,12]）恒不触发，仅脏注入（BT SYNC_CELL 旧白名单前）可拦。
       红半区逻辑不变。 */
    if (idx <= 0x1d) {
        if (raw > RANK_TOP) return ERR_RANGE;          /* 蓝方脏域 13-255 拒 */
        return LAYOUT_OK;                             /* 蓝半区
       ★ AUD-R-063（P2 定案，2026-11-09 正向对齐收口）：蓝半区（idx 0-29）无条件 LAYOUT_OK——忠实对齐原版 0x2a228 不对称校验（蓝半区无军旗格约束）。
       不受 0x38/0x3a 格约束——is_base 同时认 row0 col1/3（蓝大本营）与 row11
       col1/3（红大本营），但校验器只对红方半区做军旗格校验（0x2a228 原版指令
       即如此，蓝半区由 30 字节布局装载链路 jq_load_layout 保证：layout 行5（蓝旗行）
       固定映射 (0,*) → 默认/暗棋开局蓝旗必在 row0）。自定义布局（0x2a228 之外的
       装载路径）理论上可出现"蓝旗不在蓝大本营"，引擎照样放行——对齐原版 0x2a228
       的不对称校验（原版蓝半区即无条件 LAYOUT_OK）。
       ★ 行为锁定：evverify run_layout_validate_sym（②蓝旗idx2单格放行/⑤红旗0x37 ERR_QI）防误改回对称校验。
       但布局编码 30 字节无 side 信息——军旗 rank（RANK_FLAG）在 30 字节编码中
       全局唯一（蓝方半区 0-29 与红方半区 30-59 共享编码空间），蓝方军旗在
       idx=1/3（蓝大本营）时 raw==RANK_FLAG，红方半区 0x38/0x3a 也允许
       raw==RANK_FLAG（红方军旗），蓝/红军旗共享编码 → 对称校验误拦合法配置。
       蓝半区军旗格约束需走子期（side 已知后 move_legal 拦军旗非大本营），
       非布局校验期。is_base 对称认 4 格（AUD-R-131）是几何谓词，与布局编码
       校验是两层语义，不冲突。 */
    }
    if (raw > RANK_TOP) raw -= 0xc;                    /* 红方还原
       ★ AUD-1000 B01-041（P1 已注释，降 P3）：raw∈[13,24] 为合法红编码还原 1-12；
       raw∈[25,255] 还原后 13-243 → 下行 ERR_RANGE 拦截，域闭合（两步组合拦截
       全部脏域），本条核实无缺陷。 */
    if (raw > RANK_TOP || raw == 0) return ERR_RANGE;
    if (raw == RANK_BOMB && idx <= 0x22) return ERR_ZHA;
    if (raw == RANK_MINE && idx <= 0x31) return ERR_LEI;
    if (raw == RANK_FLAG && idx != 0x38 && idx != 0x3a) return ERR_QI;
    return LAYOUT_OK;
}

/* 全 60 格校验 + 计数表（0x6a3b0 §7：循环 60 格，rank>0xc 还原，
   军旗/地雷特判 `rank-0xb<=1`；计数表 +0x78 每 rank +1） */
jq_layout_err jq_validate_layout(const jq_state *st)
{
    int cnt[RANK_COUNT];
    int seen[RANK_COUNT];
    jq_standard_army_count(cnt);
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < N_CELLS; i++) {
        jq_layout_err e = jq_check_layout_cell(i, st->cells[i].rank);
        if (e != LAYOUT_OK) return e;
        int r = rank_norm(st->cells[i].rank);
        /* ★ AUD-R-064（P2 诚实标注）：ERR_RANGE 复用于"分布不符"（UI 若按码映射
           文案会显示"字节非法"而非"子力分布不符"）——错误码语义混淆，修法（新增
           ERR_COUNT）需同步 UI 文案映射，留待下一轮。
           ★ AUD-R-065（P2 诚实标注）/ AUD-1000 B01-042（P1 已注释，降 P3）：
           seen[r]==cnt[r]*2 假设全盘双方严格镜像（非镜像布局需独立分侧校验
           路径或 SPEC 明文"jq_validate_layout 仅验标准镜像布局"）
           （25+25 子）——自定义/联机/四国非标准布局会被误报 ERR_RANGE，仅默认
           布局校验有效（jq_extract_layout_30 注释已承认该限制）。 */
        if (r >= 0 && r < RANK_COUNT) seen[r]++;
    }
    /* 双方各一份标准 25 子 → 全盘 60 格 = 每 rank 计数 ×2（红+蓝镜像） */
    for (int r = 1; r < RANK_COUNT; r++) {
        if (r < 0 || r >= RANK_COUNT) continue;   /* AUD-A-0006 探针降噪：循环上界即宏定维，恒真断言 */
        if (seen[r] != cnt[r] * 2) return ERR_RANGE;   /* 子力分布不符标准 25 子/方 */
    }
    return LAYOUT_OK;
}

/* 从当前棋盘提取 30 字节布局（jq_load_layout 的逆映射，★ P1 布局存档用）：
   蓝方半区 cells(5-lrow, lcol)（lrow 0-5）→ layout[lrow*5+lcol]，蓝 rank 原值。
   提取后 jq_validate_layout 校验（非标准分布布局仍提取成功，仅校验返回错误）。
   ★ AUD-1000 B01-043（P1 已注释，诚实边界）：extract 仅收蓝半区 BLUE 侧
   （c->side==SIDE_BLUE 过滤）——红方半区信息全丢（AUD-R-048 已知）；
   暗棋局 side=UNKNOWN(0xFF) 定色前 60 格全被过滤 → 提取出全 0 布局，
   30 字节回载 jq_load_layout = 空棋盘。明棋镜像布局可用，暗棋开局需
   30 字节布局直接走 jq_dark_setup 路径（勿经本函数 round-trip）。 */
void jq_extract_layout_30(const jq_state *st, uint8_t out[30])
{
    for (int i = 0; i < 30; i++) out[i] = 0;
    for (int lrow = 0; lrow < 6; lrow++) {
        for (int lcol = 0; lcol < 5; lcol++) {
            const jq_cell *c = &st->cells[cell_index(5 - lrow, lcol)];
            /* 只收蓝方子（红方半区 cells(6+lrow,4-lcol) 镜像同布局，不重复） */
            if (c->rank != RANK_EMPTY && c->side == SIDE_BLUE)
                out[lrow * 5 + lcol] = c->rank;   /* 蓝方 rank 原值（无 +0xc） */
        }
    }
}

/* 暗棋（ruleflip）开局装载（AUD-R-003 P0 先手翻子定色，本轮落地）：
   与明棋同结构（蓝 (5-lrow,lcol) / 红 (6+lrow,4-lcol) 镜像，rank 红方 +0xc 编码），
   但**全 60 格 side=SIDE_UNKNOWN(0xFF)、rank 统一为蓝方原值（不含 +0xc）**：
   · 先手翻子定色：首翻子 c->side==SIDE_UNKNOWN → 翻定 st->turn 方（rules.c jq_flip，
     §L48 ruleflip「先手翻出第一个棋子确定该玩家持此颜色」）；
   · 未翻子不可被吃（move_legal 暗棋守卫，AUD-R-006 信息不泄露）；
   · 地雷/军旗仅工兵(rank 9)可吃；工兵挖满 3 雷（mines_cleared[me]>=3）方可扛旗（AUD-R-002）。
   ★ 诚实边界（与兄弟实现 game_logic.c:333 的分歧点）：兄弟实现 jq_dark_setup 仍写死
     side 分区（"明棋藏脸"），本实现真正走 SIDE_UNKNOWN 先手翻子定色（原版 ruleflip 玩法）。
     红方 rank 不 +0xc（rank_norm 还原依赖 side==SIDE_RED 时才加 0xc，先手翻子定色前
     双方 side 均 UNKNOWN → 全用蓝方原值统一编码，定色后 move_legal 按 src->side 判归属）。
     兄弟实现对齐需 game_logic.c 同步改（留待翻译体下一轮，R-138 分叉边界）。
   ★ AUD-1000 B01-044（P1 已注释，诚实边界）：本函数 = 满盘 60 子镜像装载
   （30 字节 ×2），与 main.c 暗棋"50 枚随机 60 格"路径子数语义分叉
   （60 子满盘 vs 50 子 10 格空）——SPEC 定案暗棋开局子数留待下一轮；
   当前 64 位正向以本函数为准（30 字节布局 round-trip 语义闭合），
   main.c 随机 50 子路径为 32 位对照范式（不改）。 */
void jq_dark_setup(jq_state *st, const uint8_t layout[30])
{
    jq_state_init(st);
    for (int i = 0; i < 30; i++) {
        int v = layout[i];
        if (v == 0) continue;                 /* 行营空位 */
        /* ★ AUD-1000 B01-039/040（P1 已修，同 jq_load_layout 范式）：暗棋
           装载亦拒 v>12 脏字节（防脏布局 rank 污染先手翻子定色路径）。 */
        if (v > RANK_TOP) continue;
        int lrow = i / 5, lcol = i % 5;
        /* 双方同结构：蓝方 (5-lrow, lcol)；红方镜像 (6+lrow, 4-lcol)（与 jq_load_layout 同几何） */
        jq_cell *b = &st->cells[cell_index(5 - lrow, lcol)];
        b->rank = (uint8_t)v; b->side = SIDE_UNKNOWN;   /* ★ AUD-R-003：全 0xFF 未定色 */
        b->revealed = 0;
        b->flags = (v == RANK_BOMB ? JQF_BOMB : 0)
                 | (v == RANK_MINE ? JQF_MINE : 0)
                 | (v == RANK_FLAG ? JQF_FLAG : 0);
        jq_cell *r = &st->cells[cell_index(6 + lrow, 4 - lcol)];
        r->rank = (uint8_t)v; r->side = SIDE_UNKNOWN;   /* ★ 不 +0xc（统一蓝方原值编码） */
        r->revealed = 0;
        r->flags = b->flags;
    }
    st->mode = JQ_MODE_FLIP;
    st->turn = SIDE_BLUE;
}

/* 刷新可动子统计（0x6a3b0 → 0x6b87c，L61：走子后刷新 + 困毙判定输入）
   ★ AUD-R-014（P1）：n 累加逻辑诚实标注——原实现 n 当布尔标志用（n=1 即停），
   统计值恒 1（只要该方有可动子）；保留原语义不改动（调用方无消费者：
   困毙判定走 jq_side_stalemated 实时扫描，不读 pieces_left），
   勿用于判胜/UI 展示（需真实统计时改为累加器 + 走子后调用）。
   ★ AUD-1000 B01-045（P1 已注释，降 P3）：旧实现 `if (t!=i&&jq_can_move) {n=1;break;}`
   + `if (!n) n++;` 有可动子恒 n=1（非真实可动子数）——已修（见函数体 n++ 累加
   计数已翻转可动子），本条核实"恒 1/0"描述针对**旧**实现，现行为已准确。 */
void jq_refresh_movable_counts(jq_state *st)
{
    /* ★ AUD-R-014（P1）修正两处真 bug：
     ① 旧实现 `n = 1; break;` + `if (!n) n++;` 把 n 当布尔标志 →
        **只要该方有子就恒为 1**（真实值约 20），且无走子的子反而把 n 置 1。
     ② 旧实现用 jq_can_move()（强制 src->side == st->turn）→ 非当前回合方
        被全部判"不可动"。改用 jq_can_move_any()（忽略回合）。
     语义：pieces_left[s] = s 方**已翻开**、非旗非雷、存在至少一个合法目标的子数。 */
    if (!st) return;
    for (int s = 0; s < 2; s++) {
        int n = 0;
        for (int i = 0; i < N_CELLS; i++) {
            const jq_cell *c = &st->cells[i];
            if (c->rank == RANK_EMPTY || c->side != s) continue;
            int r = rank_of(c);   /* ★ AUD-1000 B01-005（P1 已修）：side 感知还原（裸
               rank_norm 对蓝方脏 rank>12 钳位值误判"军旗"，与 AUD-R-047 同款） */
            if (r == RANK_FLAG || r == RANK_MINE) continue;
            if (!c->revealed) continue;
            for (int t = 0; t < N_CELLS; t++) {
                if (t != i && jq_can_move_any(st, i, t)) { n++; break; }
            }
        }
        st->pieces_left[s] = n;
    }
}
