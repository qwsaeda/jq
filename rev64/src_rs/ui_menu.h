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
    UI_DIFFICULTY, /* ★ 对抗审计（datah_041 证据）：人机对战难度选择弹窗——
                       新手/中级/高级/好友区 × 布局/翻棋 8 按钮，选完开局 */
    UI_WAYSEL,     /* ★ 对抗审计（datah_072 证据）：联网/联机方式选择弹窗——
                       蓝牙联机/Wi-Fi 联机/个人资料/返回 4 按钮 */
    UI_SETTINGS,   /* ★ 对抗审计（datah_001 证据）：声音设置弹窗——
                       静音模式/按键声音/音量 + 确定/取消 */
} ui_screen;

/* ★ 声音设置弹窗（对抗审计 datah_001 证据：静音模式/按键声音/音量/确定/取消）。
   几何 720×1280 基准（datah_001 450×390 居中 ×1.2）。action：
   0=静音模式切换 1=按键声音切换 2=音量循环 3=确定 4=取消 */
#define UI_SETTINGS_COUNT 5
#define UI_SETTINGS_POPUP_X0 170.0f
#define UI_SETTINGS_POPUP_Y0 400.0f
#define UI_SETTINGS_POPUP_W  380.0f
#define UI_SETTINGS_POPUP_H  470.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=静音模式 1=按键声音 2=音量 3=确定 4=取消 */
} ui_set_btn;

const ui_set_btn *ui_set_buttons(void);   /* 5 按钮（静态表） */
int ui_set_hit(const ui_set_btn *btns, int n, float x, float y);  /* 命中 0-4，-1 无 */
int ui_set_popup_inside(float x, float y); /* 点在弹窗面板内 */

/* ★ 联网/联机方式选择弹窗（对抗审计 datah_072 证据：蓝牙联机/Wi-Fi 联机/个人资料/返回 4 按钮）。
   几何 720×1280 基准（datah_072 250×325 居中 ×1.2）。action：
   0=蓝牙联机 1=Wi-Fi 联机 2=个人资料 3=返回 */
#define UI_WAYSEL_COUNT 4
#define UI_WAYSEL_POPUP_X0 210.0f
#define UI_WAYSEL_POPUP_Y0 440.0f
#define UI_WAYSEL_POPUP_W  300.0f
#define UI_WAYSEL_POPUP_H  390.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=蓝牙联机 1=Wi-Fi 联机 2=个人资料 3=返回 */
} ui_way_btn;

const ui_way_btn *ui_way_buttons(void);   /* 4 按钮（静态表） */
int ui_way_hit(const ui_way_btn *btns, int n, float x, float y);  /* 命中 0-3，-1 无 */
int ui_way_popup_inside(float x, float y); /* 点在弹窗面板内 */

/* ★ 人机难度选择弹窗（对抗审计 datah_041 证据：新手区/中级区/高级区/好友区 × 布局/翻棋 8 按钮）。
   几何 720×1280 基准（datah_041 450×335 居中）。action 编码 level/mode：
   action=0..7 → level=action/2+1（1新手/2中级/3高级/4好友），mode=action%2（0布局/1翻棋） */
#define UI_DIFF_COUNT 8
#define UI_DIFF_POPUP_X0 180.0f
#define UI_DIFF_POPUP_Y0 420.0f
#define UI_DIFF_POPUP_W  360.0f
#define UI_DIFF_POPUP_H  400.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=新手布局 1=新手翻棋 2=中级布局 3=中级翻棋
                               4=高级布局 5=高级翻棋 6=好友布局 7=好友翻棋 */
} ui_diff_btn;

const ui_diff_btn *ui_diff_buttons(void);   /* 8 按钮（静态表） */
int ui_diff_hit(const ui_diff_btn *btns, int n, float x, float y);  /* 命中 0-7，-1 无 */
int ui_diff_popup_inside(float x, float y); /* 点在弹窗面板内 */

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
/* ★ 对抗审计（datah_067 证据）：蓝牙联机 ☰ 菜单（New/Regret/Draw/Resign/Chat/Config/Stat/Rule）。
   action 编码：0=重新开始 1=悔棋 2=提和 3=认输 4=聊天 5=功能设置 6=战况统计 7=游戏规则 */
const ui_imen_btn *ui_imen_bt_buttons(void);
/* ☰ / ↩ 图标命中区域（棋盘中部，720×1280 基准；原版 ~(270,900)/(620,900) 屏幕） */
void ui_imen_icon_rect(int which, float *x0, float *y0, float *x1, float *y1);

#endif
