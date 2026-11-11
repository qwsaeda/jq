# 阶段 R2 · 游戏状态与内存模型（手写路线）

## 目标
- 定义 arm64 库的游戏状态模型（棋盘/子力/阶段/胜负），供规则（R3）和渲染（R4）复用。
- 12×5 棋盘、每方 25 子（标准军棋）、行营/大本营、明/暗棋阶段。

## 状态模型规范（junqi_core.h）
```c
typedef enum { JQ_SIDE_BLUE=0, JQ_SIDE_RED=1 } jq_side;
typedef enum { JQ_STAGE_PLAY, JQ_STAGE_DARK, JQ_STAGE_OVER } jq_stage;

typedef struct {
    uint8_t rank;      /* JQ_RANK_*：1炸弹/2军旗/31地雷/32-40兵种 */
    uint8_t side;      /* JQ_SIDE_BLUE/RED */
    uint8_t revealed;  /* 暗棋未翻开=0 */
    uint8_t is_flag, is_mine, is_bomb;
} jq_cell;

typedef struct {
    jq_cell board[JQ_CELLS];   /* 60 格（12行×5列） */
    uint8_t turn;              /* 当前方 */
    uint8_t winner;            /* 0xFF=未定 */
    jq_stage stage;
    int pieces_left[2];        /* 每方剩余子数 */
    int flags_captured[2];
} jq_state;
```

### 常量
- `JQ_BOARD_ROWS=12`、`JQ_BOARD_COLS=5`、`JQ_CELLS=60`、`JQ_PIECES=25`、`JQ_ARMY_SIZE=25`。
- `JQ_RANK_MINE=31`（非零，避免与 JQ_EMPTY=0 冲突）；`JQ_RANK_BOMB=1`；`JQ_RANK_FLAG=2`；
  `JQ_RANK_SOLDIER=32`；`JQ_RANK_MIN=32`；`JQ_RANK_MAX=40`。

### 格子工具
- `jq_cell_index(row,col)=row*5+col`；`jq_cell_row/col`。
- `jq_is_camp`：行营（蓝行2/3/4 特定格，红镜像）。
- `jq_is_base`：大本营（军旗所在底线格，蓝行0/红行11 col1/col3）。
- `jq_is_rail`：铁路（横向 row5/6 + 纵向贯穿）。

## 接口规范
```c
void jq_state_init(jq_state *st, jq_stage stage);
void jq_state_reset(jq_state *st);
void jq_place(jq_state *st, int idx, uint8_t side, uint8_t rank); /* 设子+标记 */
```

## 经验（→ LESSONS）
- L21：default.lay 30 字节/方（6行×5列），值**降序**编码。
- **地雷 rank 非零**（31）：JQ_RANK_MINE 与 JQ_EMPTY 不能同为 0，否则地雷被跳过/误判空。

## 复用指引
- 规则层（R3）读 `jq_state.board`，不直接改（经 `jq_try_move`）。
- 渲染层（R4）读 `board[]` 画 60 格。
- JNI OnTouch 经 `jq_cell_index` 把格坐标转状态索引。

## 已验证
- `jq_load_layout`：蓝红各 25 子（炸弹4/地雷6/军旗2），镜像对称，军旗被地雷护卫。
- 棋盘显示 12×5 完整输出（L=蓝/R=红 + rank）。

## 诚实边界
- 棋盘为 12×5（每方 6行×5列=30格，含行营空位），**非**原版精确棋盘尺寸。
- 原版棋盘素材（737×751 等 PNG）可用于渲染，但格子坐标映射需按素材校准。
