#ifndef RS_UI_MENU_H
#define RS_UI_MENU_H
#include "junqi.h"

/* 菜单/界面状态（正向开发 UI 层，原版主菜单文案对齐 string.xml/uitag.xml） */
typedef enum {
    UI_MENU = 0,   /* 主菜单（5 按钮：人机/两人/联网/联机/四国） */
    UI_GAME,       /* 对局（棋盘 + 触摸走子 + 可走格高亮） */
    UI_INGAME_MENU, /* ★ 原版 UI 对齐：☰ 游戏内菜单（datah_006 8 项）——
                       对局态点棋盘中部左 ☰ 打开，8 项（重新开始/悔棋/电脑变着/
                       游戏规则/功能设置/战况统计/软件信息/退出程序） */
} ui_screen;

/* ☰ 游戏内菜单按钮（原版实测 720×1280 基准，非 datah_006 定义——实测图标中心
   4 列 x=123/284/445/606（列距 161）、2 行 y=514/702（行距 188）、图标 88×80、
   文字行中心 594/780；datah_006 的 450×285 非实际面板尺寸） */
#define UI_IMENU_COUNT 8
#define UI_IMENU_POPUP_X0 55.0f
#define UI_IMENU_POPUP_Y0 460.0f
#define UI_IMENU_POPUP_W  615.0f
#define UI_IMENU_POPUP_H  360.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准绝对） */
    int action;             /* 0=重新开始 1=悔棋 2=电脑变着 3=游戏规则
                               4=功能设置 5=战况统计 6=软件信息 7=退出程序 */
} ui_imen_btn;

/* 菜单按钮（屏幕像素几何，720×1280 基准；触摸命中 + 渲染共用） */
#define UI_BTN_COUNT 5
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（屏幕像素） */
    const char *label;      /* 文案 */
    int action;            /* 0=人机 1=两人 2=联网 3=联机 4=四国 */
} ui_btn;

const ui_btn *ui_menu_buttons(void);   /* 5 按钮几何（静态表） */
int ui_menu_hit(const ui_btn *btns, int n, float x, float y);  /* 命中返回按钮 index，-1 无 */
void ui_menu_action_log(int action);  /* 按钮动作日志（占位语义） */

/* ★ 原版 UI 对齐：☰ 游戏内菜单（datah_006）8 按钮几何 + 命中 */
const ui_imen_btn *ui_imen_buttons(void);   /* 8 按钮（静态表，action 编码） */
int ui_imen_hit(const ui_imen_btn *btns, int n, float x, float y);  /* 命中 0-7，-1 无 */
int ui_imen_popup_inside(float x, float y); /* 点在菜单面板内 */
/* ☰ / ↩ 图标命中区域（棋盘中部，720×1280 基准；原版 ~(270,900)/(620,900) 屏幕） */
void ui_imen_icon_rect(int which, float *x0, float *y0, float *x1, float *y1);

#endif
