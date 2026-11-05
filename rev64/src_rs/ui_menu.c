/*
 * ui_menu.c — 主菜单 + 触摸高亮（正向开发 UI 层）
 * 原版主菜单文案对齐 string.xml/uitag.xml（GBK 解包：人机对战/两人对战/联网/联机/四国军棋）。
 * 本重写用 5 按钮（屏幕像素几何，720×1280 基准）：
 *   人机对战（开局 STG_PLAY 蓝先手）/ 两人对战（占位）/ 联网对战（占位）/
 *   四国军棋（占位）/ 悔棋重开（restart）。
 * 触摸命中：OnTouch 菜单态查 ui_menu_hit → 进对局；对局态走 jq_touch_state（已有两步走子）。
 * 诚实边界：两人/联网/四国 = 占位（日志记录，不实现联机 UI）；人机 = 直接开局。
 */
#include "ui_menu.h"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "junqi_rs", __VA_ARGS__)

/* 5 按钮：横排 2+2+1，屏幕 720×1280（y 从上到下）。
   ★ 命中判定用"相对实际触摸高度"而非归一化到 720×1280 基准（MuMu 触摸坐标系
     ≈ 渲染 viewport 的 2/3，直接归一化会错位）。故命中 = 触摸坐标相对 g_view_h
     归一化后，与按钮相对 g_view_h 归一化同区间（按钮几何本身按 720×1280 画，
     触摸按实际 view 高折算）。 */
/* 按钮几何 = 实际触摸有效坐标（MuMu 触摸高 ≈ 480；渲染按 view 720×1024 缩放同一套相对位置） */
/* 5 按钮几何（720×1280 基准，y 从上到下）；渲染 rs_draw_menu 按实际 view 缩放画到屏幕，
   触摸命中用归一化 [0,1]² 相对区间（x/view_w、y/view_h）与 g_btns 归一化同区间。 */
/* 按钮几何 720×1280 基准（PBuffer 全高），y 全在 0-1280 */
/* 720x1280 坐标，sx=sy=1（PBuffer 720x1280） */
/* ★ 按钮几何 = 实际屏幕像素坐标系（view_w×view_h，MuMu 720×1024）；
   渲染 rs_draw_menu 用 sx=sy=1 直接画，触摸命中归一化（x/view_w, y/view_h）
   vs 按钮归一化（x0/view_w...）同一坐标系。屏幕/触摸完全对齐。 */
/* ★ 按钮几何 = MuMu 实测触摸坐标系（native 收到的 x/y；实测 y_input=y_actual+160）。
   渲染 PBuffer 同坐标系 + 触摸命中同坐标系 → 屏幕/触摸对齐。y 300-600 区。 */
/* ★ 原版 UI 对齐（本轮，用户指正"它没有高手新手"+截图对比自证）：
   原版主菜单 = datax_294 卡通士兵背景 + 5 按钮在屏幕下部（实测 orig_menu.png
   900×1600 蓝色像素检测：人机(252,1084)-(645,1179)、两人(252,1227)-(645,1322)、
   联网(215,1371)-(429,1479)、联机(469,1371)-(683,1479)、四国(727,1371)-(858,1479)）。
   转 src_rs 720×1280 基准（src_rs view=900×1468+132 状态栏偏移，
   sx=view_w/720=1.25, sy=view_h/1280=1468/1280=1.1469）：
     基准 = (屏幕坐标 - 132偏移)/sx,sy
     人机 (252,952)/(1.25,1.1469)=(201.6,830.1) ... (645,1047)/(1.25,1.1469)=(516,912.9)
     两人 (201.6,954.8)-(516,1037.6)
     联网 (172,1080.3)-(343.2,1174.5)
     联机 (375.2,1080.3)-(546.4,1174.5)
     四国 (581.6,1080.3)-(686.4,1174.5)
   ★ datah_023（y540-750）判误：非主菜单按钮实测位置（原版主菜单按钮全在下部 y1084+）。
   ★ datah_041（新手/中级/高级/好友区）判误：非人机难度弹窗——原版人机/两人直开局
     （orig_human.png/orig_two3.png 暗棋直开局无弹窗），datah_041 疑联网房间选择。
   渲染 rs_draw_menu 用 sx=view_w/720 sy=view_h/1280 缩放；OnTouch 命中同缩放（一致）。 */
static const ui_btn g_btns[UI_BTN_COUNT] = {
    /* x0,    y0,    x1,    y1,    label,        action（原版实测 720×1280 基准） */
    { 201.6f, 830.1f, 516.0f, 912.9f, "人机对战", 0 },
    { 201.6f, 954.8f, 516.0f, 1037.6f, "两人对战", 1 },
    { 172.0f, 1080.3f, 343.2f, 1174.5f, "联网", 2 },
    { 375.2f, 1080.3f, 546.4f, 1174.5f, "联机", 3 },
    { 581.6f, 1080.3f, 686.4f, 1174.5f, "四国军棋", 4 },
};

const ui_btn *ui_menu_buttons(void) { return g_btns; }

int ui_menu_hit(const ui_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

void ui_menu_action_log(int action)
{
    switch (action) {
    case 0: LOGI("UI菜单：人机对战 → 直开局（★ 原版 UI 对齐：无难度弹窗）"); break;
    case 1: LOGI("UI菜单：两人对战 → 直开局（★ 原版 UI 对齐：无难度弹窗）"); break;
    case 2: LOGI("UI菜单：联网对战（占位，SPP socket 由 Java 宿主承担）"); break;
    case 3: LOGI("UI菜单：四国军棋（占位，未实现四国规则）"); break;
    case 4: LOGI("UI菜单：悔棋重开 → restart（default.lay）"); break;
    default: break;
    }
}

/* ★ 原版 UI 对齐：☰ 游戏内菜单（原版实测 orig_ingame_menu.png 900×1600，
   非 datah_006 定义——实测图标中心 4 列 x=123/284/445/606（列距 161）、
   2 行 y=514/702（行距 188）、图标 88×80、文字行中心 594/780）。
   ★ datah_041 判误（非人机难度弹窗，原版人机直开局）——datah_006 才是游戏内"选项 UI"。
   ★ datah_006 的 450×285 面板 + 8 按钮 105×135（列距 105）非实际渲染尺寸：
     实测面板更宽（图标 4 列分散列距 161），datah_006 定义仅窗口类声明。
   ☰/↩ 图标在棋盘中部（原版 orig_human.png ~(270,900)/(620,900) 屏幕 900×1600）：
     转 720×1280 基准：x 270/900*720=216, y 900/1600*1280=720 → ☰ 中心 (216,720)；
     ↩ x 620/900*720=496 → 中心 (496,720)。图标范围 70×70。 */
static const ui_imen_btn g_imen_btns[UI_IMENU_COUNT] = {
    /* x0,    y0,    x1,    y1,    action；中心=实测图标中心，覆盖图标+文字 */
    {  48.0f, 474.0f, 198.0f, 607.0f, 0 },  /* 重新开始（中心 123,540） */
    { 209.0f, 474.0f, 359.0f, 607.0f, 1 },  /* 悔棋（中心 284,540） */
    { 370.0f, 474.0f, 520.0f, 607.0f, 2 },  /* 电脑变着（中心 445,540） */
    { 531.0f, 474.0f, 681.0f, 607.0f, 3 },  /* 游戏规则（中心 606,540） */
    {  48.0f, 662.0f, 198.0f, 795.0f, 4 },  /* 功能设置（中心 123,728） */
    { 209.0f, 662.0f, 359.0f, 795.0f, 5 },  /* 战况统计（中心 284,728） */
    { 370.0f, 662.0f, 520.0f, 795.0f, 6 },  /* 软件信息（中心 445,728） */
    { 531.0f, 662.0f, 681.0f, 795.0f, 7 },  /* 退出程序（中心 606,728） */
};

const ui_imen_btn *ui_imen_buttons(void) { return g_imen_btns; }

int ui_imen_hit(const ui_imen_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_imen_popup_inside(float x, float y)
{
    return x >= UI_IMENU_POPUP_X0 && x <= UI_IMENU_POPUP_X0 + UI_IMENU_POPUP_W &&
           y >= UI_IMENU_POPUP_Y0 && y <= UI_IMENU_POPUP_Y0 + UI_IMENU_POPUP_H;
}

void ui_imen_icon_rect(int which, float *x0, float *y0, float *x1, float *y1)
{
    /* which 0=☰（游戏内菜单）1=↩（悔棋）；中心 720×1280 基准 */
    float cx = (which == 0) ? 216.0f : 496.0f;
    float cy = 720.0f, r = 35.0f;
    *x0 = cx - r; *y0 = cy - r; *x1 = cx + r; *y1 = cy + r;
}
