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

/* 默认布局：SPAK data/lay/00.lay（=default.lay，L52 分布确证 25 子/方） */
const uint8_t JQ_DEFAULT_LAY[30] = {
    3,7,6,5,8, 10,0,9,0,1,
    2,10,0,9,5, 4,0,6,0,7,
    8,3,4,9,11, 11,12,11,8,7
};

/* 标准单方 25 子 rank 分布（default.lay 实测，L52 决定性证据） */
void jq_standard_army_count(int cnt[RANK_COUNT])
{
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
    if (idx <= 0x1d) return LAYOUT_OK;                 /* 蓝半区
       ★ AUD-R-063（P2 诚实标注）：蓝半区（idx 0-29 = row0-5）直接放行，蓝方军旗
       不受 0x38/0x3a 格约束——is_base 同时认 row0 col1/3（蓝大本营）与 row11
       col1/3（红大本营），但校验器只对红方半区做军旗格校验（0x2a228 原版指令
       即如此，蓝半区由 30 字节布局装载链路 jq_load_layout 保证：layout 行5（蓝旗行）
       固定映射 (0,*) → 默认/暗棋开局蓝旗必在 row0）。自定义布局（0x2a228 之外的
       装载路径）理论上可出现"蓝旗不在蓝大本营"，引擎照样放行——对齐原版 0x2a228
       的不对称校验（原版蓝半区即无条件 LAYOUT_OK），补齐蓝半区对称校验留待下一轮。 */
    if (raw > RANK_TOP) raw -= 0xc;                    /* 红方还原 */
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
           ★ AUD-R-065（P2 诚实标注）：seen[r]==cnt[r]*2 假设全盘双方严格镜像
           （25+25 子）——自定义/联机/四国非标准布局会被误报 ERR_RANGE，仅默认
           布局校验有效（jq_extract_layout_30 注释已承认该限制）。 */
        if (r >= 0 && r < RANK_COUNT) seen[r]++;
    }
    /* 双方各一份标准 25 子 → 全盘 60 格 = 每 rank 计数 ×2（红+蓝镜像） */
    for (int r = 1; r < RANK_COUNT; r++)
        if (seen[r] != cnt[r] * 2) return ERR_RANGE;   /* 子力分布不符标准 25 子/方 */
    return LAYOUT_OK;
}

/* 从当前棋盘提取 30 字节布局（jq_load_layout 的逆映射，★ P1 布局存档用）：
   蓝方半区 cells(5-lrow, lcol)（lrow 0-5）→ layout[lrow*5+lcol]，蓝 rank 原值。
   提取后 jq_validate_layout 校验（非标准分布布局仍提取成功，仅校验返回错误）。 */
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
     兄弟实现对齐需 game_logic.c 同步改（留待翻译体下一轮，R-138 分叉边界）。 */
void jq_dark_setup(jq_state *st, const uint8_t layout[30])
{
    jq_state_init(st);
    for (int i = 0; i < 30; i++) {
        int v = layout[i];
        if (v == 0) continue;                 /* 行营空位 */
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
   勿用于判胜/UI 展示（需真实统计时改为累加器 + 走子后调用）。 */
void jq_refresh_movable_counts(jq_state *st)
{
    for (int s = 0; s < 2; s++) {
        int n = 0;
        for (int i = 0; i < N_CELLS; i++) {
            const jq_cell *c = &st->cells[i];
            if (c->rank == RANK_EMPTY || c->side != s) continue;
            int r = rank_norm(c->rank);
            if (r == RANK_FLAG || r == RANK_MINE) continue;
            if (!c->revealed) continue;
            for (int t = 0; t < N_CELLS; t++)
                if (t != i && jq_can_move(st, i, t)) { n = 1; break; }
            if (!n) n++;
        }
        st->pieces_left[s] = n;
    }
}
