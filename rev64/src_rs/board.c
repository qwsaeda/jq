/*
 * board.c — 棋盘几何 / 状态初始化（逆源码重写）
 * 依据（§Lxx 格式，REVERSE_NOTES_01_basics.md）：
 *   §L51/§L79（0x6ef54 坐标比较 [0]=col [1]=row）+
 *   §L68（十字铁路：col2 纵线贯穿 + row5/6 横线中线；行营/大本营在铁路外）+
 *   §L49（铁路修复：行营非铁路不可穿行、工兵可拐弯、其他子直走）+
 *   §L59（0x39e14 布局校验：炸弹/地雷/军旗位置限制，AUD-R-021 确认布局期非走子）。
 * ★ AUD-R-133（P3 已修）：头注释统一为"§Lxx（REVERSE_NOTES_01_basics.md）"格式
 *   （原"L51/L79"裸 L 编号检索易漏，与 rules.c "§L49/§L68" 风格不一致）。
 * 棋盘 12×5=60 格；蓝方 row0-5（row0 底线含大本营 col1/3），红方 row6-11。
 */
#include "junqi.h"

/* 行营坐标集合（★ AUD-R-130（P3 已修）：改坐标集合注释，原 layout idx 映射注释
   信息密度高易错且与实现 if 顺序不同；现直接列 10 格坐标）：
   蓝半区 (row2-4)：(2,1)(2,3)(3,2)(4,1)(4,3)
   红半区 (row7-9)：(7,1)(7,3)(8,2)(9,1)(9,3)
   行营是安全岛（敌方不能**吃**行营中棋子，空行营可入），非铁路不可穿行。
   与 default.lay 的一致性校验：layout idx 6,8,12,16,18 → 蓝棋盘坐标
   (4,1)(4,3)(3,2)(2,1)(2,3)，红镜像 (7,3)(7,1)(8,2)(9,3)(9,1)。 */
bool is_camp(int row, int col)
{
    if (row == 2 && (col == 1 || col == 3)) return true;
    if (row == 3 && col == 2) return true;
    if (row == 4 && (col == 1 || col == 3)) return true;
    if (row == 7 && (col == 1 || col == 3)) return true;
    if (row == 8 && col == 2) return true;
    if (row == 9 && (col == 1 || col == 3)) return true;
    return false;
}

/* 大本营：底线两侧（蓝 row0 col1/3，红 row11 col1/3）。
   原版 0x2a228 军旗格 56/58 = row11col1/col3（红大本营）。
   ★ AUD-R-131（P3 已修）：is_base 对称认 4 格——蓝方 (0,1)/(0,3) 与红方
   (11,1)/(11,3) 均被实现认作大本营（原版 0x2a228 仅确证红方 2 格；
   蓝方 2 格为镜像推断，与 jq_check_layout_cell 的不对称（蓝半区 idx≤29
   直接放行，仅红半区校验军旗/地雷位置）一致）。 */
bool is_base(int row, int col)
{
    return ((row == 0 || row == 11) && (col == 1 || col == 3));
}

/* 铁路（L68 十字，对齐原版军棋规则）：
   纵向：col2 纵线贯穿全高；双方底线 row0（蓝方）/row11（红方）横向贯通。
   横向：row5/6（中线）贯通全宽。
   行营/大本营在铁路外（安全格不可穿行）。
   ★ AUD-R-022（P2）：原 board.c is_rail 含 row0/row11 底线，rules.c 头注释写
   "横向仅 row5/6"（缺底线），两处矛盾。现以 board.c 实现为唯一事实源：
   横向铁路 = row0/row5/row6/row11，纵向铁路 = 仅 col2。
   ★ AUD-R-132（P3 已修）：原注释「双方底线为铁路是原版军棋标准（否则底角棋子
   无纵向通道，困毙误判）」经 3000 局走子集合 A/B 实测证伪——底角纵向通道
   不存在时困毙判定不变（1848=1848），且引入 1061 条非法走子（AUD-R-001 P0
   修正已将 rail_path_straight 方向相关校验收紧：底线横向滑行仅限 row∈{0,11}
   整行，纵向滑行仅限 col2）。勿再以「无纵向通道」论据加固。 */
bool is_rail(int row, int col)
{
    if (is_camp(row, col) || is_base(row, col)) return false;
    /* ★ AUD-R-066（P2）：底线与纵线交点 (0,2)/(11,2) 双重身份说明——该格同时
       满足 col2 纵线与 row0/row11 底线，is_rail 只回答"是否铁路格"（布尔），
       路径校验按**方向**分派无歧义：rail_path_straight 横向滑行整行须 ∈{0,5,6,11}
       （(0,2) 按横线端点用），纵向滑行须 col2（(0,2) 按纵线端点用）；两端点
       在同一方向线内即合法。无"铁路线归属"抽象（AUD-R-067 同源，不引入
       rail_line_id 查表，对齐原版 0x39e14 常量 if 链设计）。 */
    if (col == 2) return true;             /* 纵线 */
    if (row == 5 || row == 6) return true; /* 中线 */
    if (row == 0 || row == 11) return true;/* 双方底线 */
    return false;
}

/* ★ AUD-R-024（P2 诚实标注）：「中线行集合 {5,6}」「纵线列 2」在 board.c is_rail /
   rules.c rail_path_engineer(:68) / rail_path_straight(:95,:98) 三处硬编码同义常量。
   修法（提取 is_mid_line(row)/is_main_rail_col(col) 具名谓词）留待下一轮；当前以
   is_rail 为铁路唯一事实源，方向相关校验（横整行∈{0,5,6,11}/纵 col2）已对齐
   （AUD-R-001 P0 + AUD-R-022），行为一致，仅代码组织待改进。 */

/* ★ AUD-R-067（P2 诚实标注）：行营（6 格）/大本营（4 格）/铁路（3 类）全部硬编码
   为坐标 if 链，无可查询地形表——对齐原版 0x39e14 常量 if 链设计（原版亦无
   抽象地形表，常量直接来自 32 位 so 反汇编）。支持 17×12 四国棋盘拓扑需重开
   board.c 几何，属超范围改动，本轮诚实标注不抽象。 */
int cell_index(int row, int col) { return row * BOARD_COLS + col; }
/* ★ AUD-R-068（P2 诚实标注）：cell_index/cell_row/cell_col 无边界检查——row/col 越界
   （或负 idx 传入 cell_row 走 C 向零截断得"看似合法"的 0/-1）属调用方契约：全仓
   调用点均先经 0..N_CELLS-1 校验（rules.c 走子入口 / 棋盘循环）或来自 N_CELLS
   循环本身，负 idx 无传入路径。性能敏感路径（BFS/遍历）不加运行时 check，
   若引入新调用点须先自检。 */
int cell_row(int idx) { return idx / BOARD_COLS; }
int cell_col(int idx) { return idx % BOARD_COLS; }
/* ★ AUD-R-069（P2 诚实标注）：coord_cmp 全仓无调用点（grep 确认）——保留函数体
   对齐原版 0x6ef54 坐标比较语义（[0]=col [1]=row，返回 0=相同/1=不同），调用方
   在翻译体路径（src_jni_translated）；若将来需要坐标等值判定，建议新写
   coord_eq(row_a,col_a,row_b,col_b)，勿复用裸指针签名。 */

int coord_cmp(const int8_t *a, const int8_t *b)
{
    /* 0x6ef54：[0]=col [1]=row，0=相同 1=不同（坐标对逐分量） */
    if (a[0] != b[0] || a[1] != b[1]) return 1;
    return 0;
}
