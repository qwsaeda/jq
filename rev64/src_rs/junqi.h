#ifndef RS_JUNQI_H
#define RS_JUNQI_H
#define RS_NARROW_BOARD 1   /* 12 行 × 5 列（原版 L79 棋维度确证；棋盘渲染 737×751 为贴图坐标系） */
/*
 * junqi.h — 逆源码重写（R-route）核心定义
 * 依据：原版 libjunqi.so（1435156B，ELF32/ARM，军棋.apk lib/armeabi）逆向 +
 *       REVERSE_NOTES_01（§3/§5/§7/§10/§14/§15/§17、L47/L49/L52/L68）。
 * 所有函数注释中的 VA = 原版 .so 虚拟地址（.text VA==Off）。
 */
#include <stdint.h>
#include <stdbool.h>

/* ---- 棋子 rank 字节语义（原版 0x6a3a4 / 0x39e14 确证，L47）----
   0=空, 1-9=普通 rank（1=司令最强 … 9=工兵最弱，default.lay 分布确证 L52），
   10=炸弹, 11=地雷, 12=军旗。红方已翻开存储 rank+0xc。 */
#define RANK_EMPTY   0
#define RANK_BOMB    10   /* 0x0a */
#define RANK_MINE    11   /* 0x0b */
#define RANK_FLAG    12   /* 0x0c */
#define RANK_TOP    12    /* 普通 rank 上限 9，特殊子 10-12 */
#define RANK_COUNT  13    /* 计数表大小（0-12） */

/* ★ AUD-R-002/003（P0）：对局模式（明棋布局 vs 暗棋翻棋） */
#define JQ_MODE_LAY  0
#define JQ_MODE_FLIP 1

#define SIDE_BLUE 0
#define SIDE_RED  1
/* ★ 对抗审计修正（AUD-R-004 P0）：暗棋「未定色」态的 side 值。
   junqi.h 原注释写「暗棋未定=0xFF」，但**无具名常量** → 代码里散落字面量 0xFF，
   且 pieces_left[2] 用 side 直接作下标 → side=0xFF 时越界写 1020 字节外
   （破坏 jq_state 相邻成员）。现具名化并提供范围守卫宏。 */
#define SIDE_UNKNOWN 0xFF
#define SIDE_VALID(s) ((s) == SIDE_BLUE || (s) == SIDE_RED)

/* ---- 棋盘几何（原版 L68 十字铁路 / L79 12×5）----
   12 行 × 5 列；蓝方 row0-5（row0 底线含大本营 col1/3），红方 row6-11。
   铁路：纵向仅 col2 贯穿；横向仅 row5/6（中线）贯穿。行营/大本营在铁路外。 */
#define BOARD_ROWS 12
#define BOARD_COLS 5
#define N_CELLS    60

/* ---- datax_177 棋盘格（★ 层 2 视觉对齐：原版 720×1280 完整棋盘背景）----
   棋盘格只占图像 y≈205-1195（非全高 12 等分）：col0 中心 x=78.5，列宽 140.5；
   row0-5 中心 y=222+r*71（行高 71），row6-11 中心 y=822+(r-6)*71
   （row5→6 铁路中线间隙 246，非等距）。图像 row0=顶部；设备格 row0=底部
   （真机 walktest 锚点实证：row0 蓝/人在底部），故 image_row = 11 - 设备row。 */
#define BG_IMG_W        720.0f
#define BG_IMG_H        1280.0f
#define BG_GRID_COL0_CX 78.5f
#define BG_GRID_COL_W   140.5f
#define BG_GRID_ROW0_CY 222.0f
#define BG_GRID_ROW_H   71.0f
#define BG_GRID_ROW6_CY 822.0f

/* datax_177 图像内某行（image_row 0-11，0=顶）的中心 y 像素 */
static inline float bg_row_cy(int image_row)
{
    return image_row < 6
        ? BG_GRID_ROW0_CY + image_row * BG_GRID_ROW_H
        : BG_GRID_ROW6_CY + (image_row - 6) * BG_GRID_ROW_H;
}

/* 设备格(row,col)（row0=底部，与 touch/walktest 约定一致）→ 图像中心像素 */
static inline void bg_cell_img_center(int row, int col, float *ix, float *iy)
{
    int image_row = BOARD_ROWS - 1 - row;
    if (ix) *ix = BG_GRID_COL0_CX + col * BG_GRID_COL_W;
    if (iy) *iy = bg_row_cy(image_row);
}

typedef struct {
    uint8_t rank;    /* 0-12（红方翻开存储 +0xc，用 rank_norm 还原） */
    uint8_t side;    /* SIDE_BLUE/SIDE_RED；暗棋未定=0xFF */
    uint8_t revealed;/* 翻棋状态：0=背面（暗棋未翻）1=正面 */
    uint8_t flags;   /* 位：1=is_bomb 2=is_mine 4=is_flag
                       ★ AUD-R-046（P2 诚实标注）：flags 完全可由 rank_norm(rank)
                       推导，却独立存储：layout.c 装载时写一次，走子靠整格拷贝
                       被动跟随，全仓无规则读取点（规则引擎一律用 rank 判）——纯
                       冗余。jq_flip 只改 side/revealed 不改 flags（AUD-R-043）即
                       静默不一致的入口。修法（删字段或用 rank 派生 + 写点同步维护
                       断言）留待下一轮。 */
} jq_cell;

#define JQF_BOMB 1
#define JQF_MINE 2
#define JQF_FLAG 4

/* ---- 对局阶段 ---- */
typedef enum {
    STG_SETUP = 0,  /* 布阵（rulelay 编辑） */
    STG_PLAY,       /* 对局 */
    STG_OVER,
} jq_stage;

/* ★ AUD-R-053（P2）：winner 取值具名化（原靠"0x30200 r3=0/1/2"注释，int8_t 允许 -1）。
   原版 0x30200 分发 r3=0/1/2（蓝胜/红胜/和）；-1 为本重写"未定"哨兵（原版用 status 判定）。 */
#define JQ_WIN_NONE  -1
#define JQ_WIN_BLUE   SIDE_BLUE   /* 0：蓝胜 */
#define JQ_WIN_RED    SIDE_RED    /* 1：红胜 */
#define JQ_WIN_DRAW   2          /* 和 */

/* ---- 走子事件（原版 Junqi.* 音效分类，rodata 0x120f14-0x120f50）---- */
typedef enum {
    EV_MOVE = 0,    /* Junqi.Move 空移 */
    EV_EAT,         /* Junqi.Eat   普通吃子（大压小） */
    EV_BOMB,        /* Junqi.Bomb  炸弹同归于尽 */
    EV_FLAG,        /* Junqi.Flag  吃旗胜 */
    EV_DEAD,        /* Junqi.Dead  被吃方死亡 */
    EV_MINE,        /* Junqi.Move  工兵挖雷（音效分类，arm64 对齐 L71）
                     ★ AUD-R-062（P2）：EV_MINE(5) 与 EV_MOVE(0) 共用 "Junqi.Move"
                     事件名（对齐原版 32 位 .rodata 设计：挖雷不结束对局，音效按名
                     分流）；调用方按**事件码**分流统计（g_event_cnt[0]/[5] 分开计），
                     按**事件名**分流音效。 */
    EV_FLIP,        /* Junqi.Flip  翻棋（★ AUD-R-061（P2）：补 7 类事件对齐 SPEC §3；
                     值=6 置于 EV_MINE 后，0-5 顺序不变，既有事件码消费方不受影响。
                     翻棋不经 jq_try_move 事件码通道（jq_flip 返回 bool 不变），
                     计数由 jni_bridge 调用点 ui_replay_record + g_event_cnt[EV_FLIP]++
                     承担；32 位 so .rodata 明文含 Junqi.Flip 事件名，原版确有此通道。 */
} jq_event;

/* ---- 布局错误（原版 0x2a228 事件串 ZhaError/QiError/LeiError，§11/§10）---- */
typedef enum {
    LAYOUT_OK = 0,
    ERR_ZHA,   /* 炸弹在禁放前线路 */
    ERR_LEI,   /* 地雷不在最后两行 */
    ERR_QI,    /* 军旗不在大本营格 */
    ERR_RANGE, /* 字节非法 */
} jq_layout_err;

/* ---- 对局状态（原版 0x39f20 移动执行器 state 结构偏移对齐，L65）----
   原版格子 8 字节：[0]=col [1]=row [4]=rank；索引 = row*5+col（0x6ef54 确证 L79）。
   本重写用 4 字节 jq_cell 紧凑布局（等价语义，L79 记录的布局差异）。
   原版偏移：+0xa2/from 打包坐标、+0xa4/to、+0x48/侧、+0x4c/布局长度、
   +0xb0/+0x6c/+0x50 各计数、+0x78 布局计数表[rank]（0x6a3b0 确证 §7）。 */
typedef struct {
    jq_cell   cells[N_CELLS];        /* 60 格，cells[row*5+col] */
    uint8_t   turn;                  /* 当前方（原版 +0xa8 相关） */
    jq_stage  stage;
    int8_t    winner;                /* JQ_WIN_NONE 未定 / JQ_WIN_BLUE / JQ_WIN_RED / JQ_WIN_DRAW（0x30200 r3 语义，AUD-R-053） */
    int       moves_since_eat;       /* 连续无吃子步数（和棋 70 步条件） */
    int       total_moves;           /* 总步数（和棋 1000 条件） */
    int       mines_cleared[2];      /* 暗棋（ruleflip）工兵挖雷计数
                                        ★ AUD-R-015（P1）：仅记录，胜负判定未用（扛旗条件待暗棋族实现） */
    int       pieces_left[2];        /* 各方剩余可动子数（0x6a3b0 统计）
                                        ★ AUD-R-014（P1）：统计未接线——刷新逻辑恒 1、
                                        吃子后持续增减（可为负），勿用于判胜；
                                        困毙判定走 jq_side_stalemated 实时扫描，不读本字段 */
    /* ★ AUD-R-041（P2 诚实标注）：原 count_tbl[RANK_COUNT] 布局计数表字段已删——
       全仓从未写入（layout.c jq_validate_layout 用局部 seen[] 数组，从不回填
       st->count_tbl），与 undo_depth 同类"零使用者"死字段；布局计数需求走
       containers.c 的 cc->count_tbl（jq_container_check 校验链路，有写入/读取点）。 */
    uint8_t   mode;                  /* ★ AUD-R-002/003（P0）：JQ_MODE_LAY=明棋 / JQ_MODE_FLIP=暗棋
                                        暗棋语义：未翻子 revealed=0、地雷/军旗仅工兵可吃、
                                        工兵挖满 3 雷方可扛旗（ruleflip 原文）。
                                        jq_state_init 默认 JQ_MODE_LAY（明棋），
                                        jni_bridge 暗棋路径 set 为 JQ_MODE_FLIP。 */
} jq_state;

/* 红方已翻开的 rank 存储值：rank+0xc（0x6a3a4 确证，L60） */
static inline int rank_encode(uint8_t side, int rank) {
    return (side == SIDE_RED && rank != 0) ? rank + 0xc : rank;
}
static inline int rank_norm(int v) {
    return v > RANK_TOP ? v - 0xc : v;   /* 红方已翻开还原（0x6a3b0 §7）
     ★ AUD-R-047（P2 诚实标注）：还原只看数值大小不看 side——蓝方若出现 >12 的
     rank（BT SYNC_CELL 注入 / 复盘数据损坏）会被静默还原为别的 rank（13→1）。
     修法（rank_of(const jq_cell*) 结合 side 判定 + 范围校验，废弃裸 rank_norm
     11 处调用点；或约定 rank 字节永不>12）留待下一轮。 */
}

/* ---- 格子/铁路/行营/大本营（L68 十字铁路、L49 修复版拓扑）---- */
int  cell_index(int row, int col);
int  cell_row(int idx);
int  cell_col(int idx);
bool is_camp(int row, int col);       /* 行营（安全岛，敌方不可入，非铁路） */
bool is_base(int row, int col);       /* 大本营（军旗所在，进后不可动 L17） */
bool is_rail(int row, int col);       /* 铁路（col2 纵线 + row5/6 横线） */
int  coord_cmp(const int8_t *a, const int8_t *b); /* 0x6ef54：[0]=col [1]=row，0=相同 */

/* ---- 布局（0x2a340 加载 / 0x6a3a4 映射 / 0x6a3b0 校验，§3/§7）---- */
void jq_state_init(jq_state *st);
/* 0x2a340 载入 30 字节布局（6 行 × 5 列，蓝方视角 row0=前线路 … row5=底线）。
   原版 0x2a340：循环 30 次经 0x6a3a4 映射（蓝原值/红+0xc）。
   红方 = 蓝方镜像（row+6, col 翻转）。 */
void jq_load_layout(jq_state *st, const uint8_t layout[30]);
/* ★ AUD-R-003（P0）：暗棋（ruleflip）开局装载——
   50 枚子全暗置（revealed=0，side 按 5-lrow/6+lrow 分区写死 BLUE/RED，
   明棋布局同结构但全 revealed=1），st->mode = JQ_MODE_FLIP。
   暗棋规则族（AUD-R-002 P0）：地雷/军旗仅工兵可吃；工兵挖 3 雷才可扛旗。
   兄弟实现 src/game_logic.c:333-353 jq_dark_setup 同款。
   ★ 诚实边界：先手翻子定色（side=UNKNOWN 机制）留待后续轮次。 */
void jq_dark_setup(jq_state *st, const uint8_t layout[30]);
/* 0x2a228 单格校验：idx 0-59，raw 字节（红方翻开 +0xc 自动归一）。
   炸弹：idx<=34 报错；地雷：idx<=49 报错；军旗：仅 idx∈{56,58}。
   ★ AUD-R-021（P2 已修）：上述为**布局期**校验限制（非走子目标判定，
   走子路径不引用；rules.c 头注释已同步改写，依据 REVERSE_NOTES_01 §L59）。 */
jq_layout_err jq_check_layout_cell(int idx, int raw);
/* 0x6a3b0 全 60 格校验 + 计数表（军旗/地雷特判 rank-0xb<=1） */
jq_layout_err jq_validate_layout(const jq_state *st);
void jq_refresh_movable_counts(jq_state *st);

/* ---- 走子（0x39e14 目标类型判定 + 0x39f20 执行器 + 0x3a15c 交换，§5/§6）---- */
bool jq_can_move(const jq_state *st, int from, int to);
/* 执行走子。返回事件类型；失败返回 -1（不切 turn，L54）。 */
int jq_try_move(jq_state *st, int from, int to);
bool jq_can_flip(const jq_state *st, int idx);
bool jq_flip(jq_state *st, int idx);
/* 困毙判定（0x302b4 status=3）：当前方无子可动/可翻 → 对方胜 */
bool jq_side_stalemated(const jq_state *st, uint8_t side);

/* ---- 悔棋（联机 Regret* 事件驱动，L77；本地快照恢复）----
   ★ AUD-R-012（P1）：jq_undo_snapshot/jq_snapshots_left 引擎内未实现（空壳），
   实际悔棋由 jni_bridge.c g_undo 栈承担；bt_session UNDO 帧走 jq_bt_undo_available()。 */
void jq_undo_snapshot(const jq_state *st);          /* 占位（未接线，走 jni_bridge g_undo） */
/* ★ AUD-R-050（P2 诚实标注）：整体状态恢复语义（*st = *snap 全字段覆盖，含
   cells/turn/stage/winner/计数字段）——对"悔一步"合适；调用方 bt_session.c:11
   的 g_undo_snap 是**从未写入**的全局变量（UNDO 帧闸口未打开），一旦打开会
   把整盘清零（AUD-R-013 同源）。修法（补写入点或删除 UNDO 分支）留待下一轮。
   ★ AUD-R-049（P2 已修）：原 st->undo_depth 字段已删（全仓无使用者，且快照
   恢复会回退调用方自行维护的栈深，与栈内容不一致）。 */
void jq_undo_restore(jq_state *st, const jq_state *snap);
int  jq_snapshots_left(const jq_state *st);         /* 恒 0（快照栈在调用方） */

/* ---------- 胜负（0x30200 分发 / 0x30260 旗 / 0x30280 和 / 0x302b4 困毙，§12/L53/L54）---- */
void jq_on_flag_capture(jq_state *st, uint8_t captor_side);
void jq_on_draw(jq_state *st);
/* ★ AUD-R-036（P2）：显式传"被判负方"（原用调用时刻 st->turn 推导，外部调用
   语义脆弱）；内部收口（stalemate_after）传 st->turn，AI/main 路径传该步 mover。 */
void jq_on_stalemate(jq_state *st, uint8_t loser_side);
int ai_score(int src_rank_raw, int dst_rank_raw, int dst_revealed);  /* AI 贪心评分（对抗审计 #2，可单测；★ AUD-R-010 暗棋目标未翻=不透视真 rank） */

/* ---- 默认布局（SPAK data/lay/default.lay，L52 分布确证）---- */
extern const uint8_t JQ_DEFAULT_LAY[30];
/* 标准单方 25 子 rank 分布（1×1,2×1,3×2,4×2,5×2,6×2,7×3,8×3,9×3,10×2,11×3,12×1） */
void jq_standard_army_count(int cnt[RANK_COUNT]);
/* 从当前棋盘提取 30 字节布局（jq_load_layout 逆映射：蓝方半区 cells(5-lrow,lcol)→layout[lrow*5+lcol]，
   蓝 rank 原值（无 +0xc 编码）。★ P1 布局存档（datah_016/017/020）保存当前棋盘用。
   ★ AUD-R-048（P2 诚实标注）：只导出蓝方半区 30 字节，红方半区信息丢失——
   仅在"双方布局严格镜像"时成立；红方独有布局（联机同步/单机改布局/四国）
   无法用 30 字节还原真实局面。全量状态存档走 jq_save_game_*（整结构序列化），
   本函数仅用于"导出可分享布局"场景，UI 需标注。 */
void jq_extract_layout_30(const jq_state *st, uint8_t out[30]);

/* ---- 事件名（原版 .rodata 0x120f14-0x120f50 明文字符串）---- */
extern const char *JQ_EVENT_NAMES[7];   /* ★ AUD-R-061（P2）：扩容 6→7（补 EV_FLIP=Junqi.Flip，值 0-5 顺序不变） */

/* ---- 存档（save.c，L74/L83/L89）---- */
typedef struct {
    uint32_t time;
    int      difficulty, level, mark;
    int      resultEx[4];
} jq_save_data;
int jq_save_write(const char *path, const jq_save_data *sd);
int jq_save_read(const char *path, jq_save_data *sd);
int jq_save_game_write(const char *path, const jq_state *st);
int jq_save_game_read(const char *path, jq_state *st);

#endif /* RS_JUNQI_H */

/* ---- 渲染对象图 + 容器（③④ 逆源码，独立头文件，不在此 include 防循环）---- */
