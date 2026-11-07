#ifndef RS_BOARD_GEOM_H
#define RS_BOARD_GEOM_H
/*
 * board_geom.h — 渲染层棋盘几何（720×1280 背景图坐标换算），与规则层分离
 * （AUD-R-077/118/120 修复：junqi.h 职责单一化——本头文件此前混放于规则头
 *   junqi.h，使规则层编译单元隐式依赖渲染资源规格。纯搬移，行为 0 变化。）
 * 依赖规则层常量（BOARD_ROWS/BOARD_COLS 等），故顶部 include junqi.h。
 */
#include "junqi.h"

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

/* 设备格(row,col)（row0=底部，与 touch/walktest 约定一致）→ 图像中心像素
   ★ AUD-R-078（P2）：越界前置守卫——row/col 超域（image_row 负值会走错 bg_row_cy
   分支返回"看似合法"的错误坐标）→ 置 -1 返回，调用方据此跳过渲染。 */
static inline void bg_cell_img_center(int row, int col, float *ix, float *iy)
{
    int image_row = BOARD_ROWS - 1 - row;
    if (ix) *ix = -1.0f;
    if (iy) *iy = -1.0f;
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) return;
    if (ix) *ix = BG_GRID_COL0_CX + col * BG_GRID_COL_W;
    if (iy) *iy = bg_row_cy(image_row);
}

#endif /* RS_BOARD_GEOM_H */
