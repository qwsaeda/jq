/*
 * junqi_core.h — 军棋核心公共定义（手写路线 R1/R2/R3）
 *
 * 说明：
 *   - 这是**标准中国军棋（陆战棋）规则**的 arm64 原生实现骨架，
 *     用于二进制翻译路线（R2 硬墙）之外的可行路径。
 *   - **非**原版 32 位 libjunqi.so 的逐函数逆向产物；完整匹配原版
 *     行为需从 .text 逐函数逆向（月级工程）。不冒充"完整版"。
 *
 * 棋盘（逻辑网格，行列坐标）：
 *   - 12 行 × 5 列兵站交叉点，行 0-4 蓝方、行 7-11 红方，行 5-6 铁路中线。
 *   - 每方 25 兵站（5行×5列）+ 2 大本营（底线两侧）+ 行营若干。
 */
#ifndef JUNQI_CORE_H
#define JUNQI_CORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 常量 ---- */
#define JQ_BOARD_ROWS   12
#define JQ_BOARD_COLS   5
#define JQ_CELLS        (JQ_BOARD_ROWS * JQ_BOARD_COLS)   /* 60 */
#define JQ_ARMY_SIZE    25        /* 每方兵站数 */
#define JQ_PIECES       25        /* 每方棋子总数（原版 default.lay 30字节-5行营=25 子，L59） */
#define JQ_SIDE_BLUE    0
#define JQ_SIDE_RED     1
#define JQ_SIDE_UNKNOWN 0xFF  /* 暗棋（ruleflip）翻子前无归属，先手翻子定色（L64） */
#define JQ_WINNER_DRAW  2   /* 和棋（L55）：双方无子可动 / 均无法取胜 */

/* ---- 棋子等级：对齐原版 32 位 .so 字节语义（逆向确认，REVERSE_NOTES.md）----
   原版 default.lay 字节：0=空, 1-9=普通 rank（1=司令最强...9=工兵最弱），
   10=炸弹, 11=地雷, 12=军旗；红方已翻开=字节+0xc。
   arm64 直接用原版字节值作 rank，吃子比较用 src<dst（rank 小=强）。 */
#define JQ_RANK_MINE    11  /* 地雷（原版字节 11） */
#define JQ_RANK_BOMB    10  /* 炸弹（原版字节 10） */
#define JQ_RANK_FLAG    12  /* 军旗（原版字节 12） */
#define JQ_RANK_SOLDIER 9   /* 工兵（原版字节 9，最弱普通子，可挖雷） */
#define JQ_RANK_MIN     1   /* 司令（最强） */
#define JQ_RANK_MAX     9   /* 工兵（最弱） */

/* ---- 棋子类型 ---- */
typedef enum {
    JQ_EMPTY = 0,   /* 空格 */
    JQ_PIECE        /* 有子 */
} jq_cell_state;

/* ---- 一格 ---- */
typedef struct {
    uint8_t  rank;      /* JQ_RANK_* 或 32..40；0=无子 */
    uint8_t  side;      /* JQ_SIDE_BLUE/RED */
    uint8_t  revealed;  /* 暗棋模式是否已翻开 */
    uint8_t  is_flag;   /* 是否军旗 */
    uint8_t  is_mine;   /* 是否地雷 */
    uint8_t  is_bomb;   /* 是否炸弹 */
} jq_cell;

/* ---- 对局阶段 ---- */
typedef enum {
    JQ_STAGE_SETUP = 0,  /* 布阵中 */
    JQ_STAGE_PLAY,       /* 对局中 */
    JQ_STAGE_OVER,       /* 结束 */
    JQ_STAGE_DARK        /* 暗棋（翻翻棋）：随机布子，翻开走 */
} jq_stage;

/* ---- 行走结果 ---- */
typedef enum {
    JQ_MOVE_OK = 0,
    JQ_MOVE_BLOCKED,     /* 被阻挡/路径不通 */
    JQ_MOVE_BAD_SRC,     /* 源格无子或不可动 */
    JQ_MOVE_BAD_DST,     /* 目标非法（己方/行营禁入等） */
    JQ_MOVE_WRONG_TURN,  /* 非当前方 */
    JQ_MOVE_NOT_REVEALED /* 暗棋未翻开 */
} jq_move_result;

/* ---- 布局编辑（布阵阶段）错误码：对齐原版 0x2a228/0x2a5f0 校验（L80）。
   原版事件 Junqi.ZhaError/QiError/LeiError（0x2a24a/262/276，REVERSE_NOTES §11）。---- */
typedef enum {
    JQ_LAYOUT_OK = 0,
    JQ_LAYOUT_ZHA_ERROR,   /* 炸弹放在第一行（前线路）——原版 Junqi.ZhaError */
    JQ_LAYOUT_QI_ERROR,    /* 军旗不在大本营——原版 Junqi.QiError */
    JQ_LAYOUT_LEI_ERROR,   /* 地雷不在最后两行——原版 Junqi.LeiError */
    JQ_LAYOUT_COUNT_ERROR, /* 子数/阵营/军力分布错误 */
    JQ_LAYOUT_BAD_POS      /* 目标不在己方（蓝方）区域 */
} jq_layout_error;

/* ---- 核心状态 ---- */
#define JQ_UNDO_DEPTH 8   /* 悔棋历史深度（连续悔棋） */
typedef struct {
    jq_cell  board[JQ_CELLS];   /* 60 格 */
    uint8_t  turn;              /* 当前方 */
    jq_stage stage;
    uint8_t  winner;            /* 胜方，0xFF=未定 */
    uint8_t  flags_captured[2]; /* 各被吃掉的军旗数 */
    uint8_t  mines_cleared[2];  /* 各挖掉的对方地雷数（ruleflip：工兵挖3雷才能扛旗，L63） */
    uint8_t  pieces_left[2];    /* 各方剩余可动子数 */
    uint32_t moves;             /* 已走步数 */
    uint32_t moves_since_eat;   /* 连续无吃子步数（原版和棋条件，L57） */
    /* 悔棋历史（循环栈，最近 N 步棋盘快照，供连续 undo） */
    jq_cell  undo_board[JQ_UNDO_DEPTH][JQ_CELLS];
    uint8_t  undo_turn[JQ_UNDO_DEPTH], undo_stage[JQ_UNDO_DEPTH], undo_winner[JQ_UNDO_DEPTH];
    uint8_t  undo_flags[JQ_UNDO_DEPTH][2];
    uint8_t  undo_pieces[JQ_UNDO_DEPTH][2];
    uint8_t  undo_mines[JQ_UNDO_DEPTH][2];
    uint32_t undo_moves_since_eat[JQ_UNDO_DEPTH];
    int      undo_head;          /* 栈顶索引（写入位置） */
    int      undo_count;         /* 历史条数 */
} jq_state;

/* ---- API（供 JNI 层调用）---- */

/* R2: 状态管理 */
void jq_state_init(jq_state *st, jq_stage stage);
void jq_state_reset(jq_state *st);
int  jq_cell_index(int row, int col);
int  jq_cell_row(int idx);
int  jq_cell_col(int idx);
void jq_place(jq_state *st, int idx, uint8_t side, uint8_t rank);

/* R3: 规则 */
bool jq_is_rail(int row, int col);
bool jq_is_camp(int row, int col);          /* 行营 */
bool jq_is_base(int row, int col);          /* 大本营 */
bool jq_can_move(const jq_state *st, int from, int to);
jq_move_result jq_try_move(jq_state *st, int from, int to, uint8_t *eat_type);
bool jq_can_flip(const jq_state *st, int idx);
bool jq_flip(jq_state *st, int idx);
bool jq_is_game_over(const jq_state *st);
uint8_t jq_winner(const jq_state *st);
/* 悔棋：恢复上一快照（走子/翻棋前）。返回 1 成功。 */
bool jq_undo(jq_state *st);
/* 简单 AI 走一步。返回：0=无,1=移动,2=翻棋,3=吃子（音效区分）。 */
int jq_ai_move(jq_state *st);
/* AI 难度（原版 Low/Mid/High/Rank 菜单，L73 8 标签确证；arm64 三档决策强度）。
   0=LOW（随机失误） 1=MID（贪心，默认） 2=HIGH（价值+安全吃子）。 */
void jq_ai_set_level(int level);
int  jq_ai_get_level(void);

/* 暗棋：随机布子（简单伪随机，种子可注入） */
void jq_dark_setup(jq_state *st, uint32_t seed);

/* 原版布局：用 default.lay 的 30 字节布阵初始化（蓝方布局，红方镜像）。
   layout[30] 值 0-12：0=空，1..12=棋子等级索引（映射到标准军棋子力）。 */
void jq_load_layout(jq_state *st, const uint8_t layout[30]);

/* ---- 布局编辑（布阵阶段 JQ_STAGE_SETUP，rulelay 全条款校验，L80 对齐原版 0x2a228）----
   蓝方（玩家）在己方区域（行0-5）排列 25 子，红方（AI）用 default.lay 镜像作预览。
   rulelay 权威规则（data_junqi_rulelay.txt，L57/L72）：
     - 炸弹不能放在第一行（蓝方前线路=棋盘行5）
     - 地雷只能放在最后两行（棋盘行0-1）
     - 军旗只能放在大本营（棋盘行0 列1/3）
   原版 0x2a228 红方校验（对称）：炸弹禁行6、地雷仅行10-11、军旗格56/58。 */

/* 进入布阵阶段：用 default.lay 初始化双方，蓝方棋子全部翻开（可编辑），红方预览。 */
void jq_setup_begin(jq_state *st, const uint8_t layout[30]);
/* 布阵放置/交换：from（蓝方子）→ to（蓝方区域空格或蓝方子，交换）。
   每步按 rulelay 逐位校验（原版 0x2a228 逐格语义）。返回错误码，OK=成功。 */
jq_layout_error jq_setup_place(jq_state *st, int from, int to);
/* 校验整个蓝方布局（rulelay 全条款 + 25 子标准军力分布）。返回 OK 或首个错误。 */
jq_layout_error jq_setup_validate(const jq_state *st);
/* 确认布阵：validate → 进入对局（JQ_STAGE_PLAY），双方暗置（军旗明置）。 */
jq_layout_error jq_setup_confirm(jq_state *st);
/* 是否为布阵阶段。 */
bool jq_is_setup(const jq_state *st);

/* ---- 存档 one.sjq（L74 原版 0x37664 读 / 0x379ec 写确证）----
   原版字段偏移：0x38=time、0x3c=difficulty、0x40=level、0x44=mark、0x48=resultEx[4]；
   resultEx 序列化用 "%d;%d;%d;%d"（rodata 0x121774）。
   诚实边界：**字段语义对齐原版**（字段名/顺序/resultEx 格式确证）；键值文本格式
   （"key=value\n" 分隔）为 arm64 重建，非原版逐字节确证（原版写逻辑在 C++ 层，0x379ec 长链）。 */
typedef struct {
    uint32_t time;        /* 累计对局时间（秒） */
    int      difficulty;  /* AI 难度 0-2（L82） */
    int      level;       /* 玩家等级 */
    int      mark;        /* 分数/标记 */
    int      resultEx[4]; /* 扩展结果（如 胜/负/和/连，"%d;%d;%d;%d"，0x121774） */
} jq_save_data;

/* one.sjq 文本读写（path=完整文件路径）。返回 1 成功。 */
int jq_save_write(const char *path, const jq_save_data *sd);
int jq_save_read(const char *path, jq_save_data *sd);

/* ---- 对局进度存档 one.sjg（L89 原版 0x390ac 写确证）----
   原版 one.sjg 是二进制对局进度存档（0x39090 stm 写字段 / 0x390ac fopen one.sjg）。
   arm64 用文本格式（诚实边界：格式为 arm64 重建，保存完整棋盘状态供"退出重进续局"）。
   保存：board 60 格（rank,side,revealed）+ turn/stage/winner + flags/mines/pieces + moves。 */
int jq_save_game_write(const char *path, const jq_state *st);
int jq_save_game_read(const char *path, jq_state *st);


#ifdef __cplusplus
}
#endif
#endif /* JUNQI_CORE_H */
