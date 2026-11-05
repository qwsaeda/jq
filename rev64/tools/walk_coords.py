#!/usr/bin/env python3
"""walk_coords.py — 军棋走子坐标计算（cell 索引 → 真机 tap 坐标批量）

坐标公式（★ 层 2 视觉对齐：datax_177 棋盘格 BG_GRID，与 touch.c/render.c 同几何）：
  view = 900×1600（★ 全面屏：状态栏已隐藏，无 132 偏移；旧布局 1468+132 已弃）
  设备格 row0=底部（蓝/人），image_row = 11 - 设备 row
  x = (78.5 + col×140.5) / 720 × 900      （col0 中心 78.5，列宽 140.5）
  y = (222 + image_row×71) / 1280 × 1600  （image_row<6 中心 222+image_row×71）
      (822 + (image_row-6)×71) / 1280 × 1600  （image_row≥6，row5→6 间隙 246 非等距）
  cell_index = row×5 + col（60 格，row0 在底）

用法：
  python3 walk_coords.py <cell>...        # 输出每格 tap 坐标
  python3 walk_coords.py --seq 25,30 28,33  # 批量走子序列（from,to 成对）
  python3 walk_coords.py --all            # 全部 60 格

验证锚点（datax_177 BG_GRID，与 jq_touch_hit_cell 逆映射一致）：
  cell0(row0,col0) → tap(98,1471)   ✓ 底部蓝（row0）
  cell25(row5,col0) → tap(98,1028)  ✓ 蓝方铁路底线
  cell30(row6,col0) → tap(98,721)   ✓ 红方铁路底线（row6）
  cell55(row11,col0) → tap(98,278)  ✓ 顶部红底线（row11）
"""
import sys

VIEW_W, VIEW_H = 900, 1600
SCREEN_Y_OFF = 0             # 全面屏：screen_y = native_y（无状态栏偏移）
COLS, ROWS = 5, 12
BG_IMG_W, BG_IMG_H = 720.0, 1280.0
COL0_CX, COL_W = 78.5, 140.5
ROW0_CY, ROW_H, ROW6_CY = 222.0, 71.0, 822.0


def cell_coord(cell: int):
    """cell 索引 → (tap_x, tap_y) 屏幕坐标（顶左原点）"""
    row, col = divmod(cell, COLS)
    image_row = ROWS - 1 - row   # 设备 row0=底 → 图像 row0=顶
    ix = COL0_CX + col * COL_W
    iy = (ROW0_CY + image_row * ROW_H) if image_row < 6 else (ROW6_CY + (image_row - 6) * ROW_H)
    x = ix / BG_IMG_W * VIEW_W
    native_y = iy / BG_IMG_H * VIEW_H
    return round(x), round(native_y + SCREEN_Y_OFF)


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 0
    if args[0] == '--all':
        for c in range(60):
            x, y = cell_coord(c)
            print(f"cell{c:2d}(row{c//5},col{c%5}) → tap({x},{y})")
        return 0
    if args[0] == '--seq':
        pairs = [(int(a.split(',')[0]), int(a.split(',')[1])) for a in args[1:]]
        for f, t in pairs:
            fx, fy = cell_coord(f)
            tx, ty = cell_coord(t)
            print(f"走子 cell{f}→cell{t}: tap({fx},{fy}) 选 → tap({tx},{ty}) 走")
        return 0
    for a in args:
        c = int(a)
        x, y = cell_coord(c)
        print(f"cell{c}(row{c//5},col{c%5}) → tap({x},{y})")
    return 0


if __name__ == '__main__':
    sys.exit(main())
