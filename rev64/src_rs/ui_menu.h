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
    UI_NPLAYER,    /* ★ P0（datah_089 证据）：四国人数选择弹窗——
                       2/3/4 人 3 按钮（原版 datah_089 230×230 面板）。
                       诚实边界：选人数 → 边界对话框（4 方规则未实现）→ 回主菜单 */
    UI_NEWGAME,    /* ★ P0（datah_062 证据）：两人新局简化弹窗——
                       玩法(Lay)/明暗棋(Show) + 确定/取消 4 控件（原版 datah_062 450×315 面板）。
                       两人热座（AI 不介入）；取消 → 回主菜单 */
    UI_GAMESET,    /* ★ P1（datah_012 证据）：游戏设置弹窗——
                        4 规则开关（和棋/大本营/吃地雷/吃军旗）+ 确定/取消（原版 datah_012 450×465 面板）。
                        4 开关各循环翻转 开/关；确定 → jq_rules.txt 持久化 + 应用 g_rule_*；取消 → 恢复原值 */
} ui_screen;

/* ★ 声音设置弹窗（对抗审计 datah_001 证据：静音模式/按键声音/音量/确定/取消）。
   几何 720×1280 基准（datah_001 450×390 居中 ×1.2）。action：
   0=静音模式切换 1=按键声音切换 2=音量循环 3=确定 4=取消 */
#define UI_SETTINGS_COUNT 5
#define UI_SETTINGS_POPUP_X0 135.0f
#define UI_SETTINGS_POPUP_Y0 445.0f
#define UI_SETTINGS_POPUP_W  450.0f
#define UI_SETTINGS_POPUP_H  390.0f
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
#define UI_WAYSEL_POPUP_X0 235.0f
#define UI_WAYSEL_POPUP_Y0 478.0f
#define UI_WAYSEL_POPUP_W  250.0f
#define UI_WAYSEL_POPUP_H  325.0f
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
#define UI_DIFF_POPUP_X0 135.0f
#define UI_DIFF_POPUP_Y0 472.0f
#define UI_DIFF_POPUP_W  450.0f
#define UI_DIFF_POPUP_H  335.0f
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
#define UI_IMENU_POPUP_X0 135.0f
#define UI_IMENU_POPUP_Y0 498.0f
#define UI_IMENU_POPUP_W  450.0f
#define UI_IMENU_POPUP_H  285.0f
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

/* ★ P0（datah_089 证据）：四国人数选择弹窗——2/3/4 人 3 按钮。
   几何 720×1280 基准（datah_089 230×230 面板居中，偏移=(720-230)/2=245,(1280-230)/2=525；
   局部 1:1 平移：按钮 15,15,215,75 / 15,85,215,145 / 15,155,215,215 →
   720 系 260,540-460,600 / 260,610-460,670 / 260,680-460,740）。
   action：0=2 人 1=3 人 2=4 人（默认 4）。★ 诚实边界：四国 4 方规则未实现（src_rs=10×9 两人军棋）。 */
#define UI_NPLAYER_COUNT 3
#define UI_NPLAYER_POPUP_X0 245.0f
#define UI_NPLAYER_POPUP_Y0 525.0f
#define UI_NPLAYER_POPUP_W  230.0f
#define UI_NPLAYER_POPUP_H  230.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=2 人 1=3 人 2=4 人 */
} ui_nplayer_btn;

const ui_nplayer_btn *ui_nplayer_buttons(void);   /* 3 按钮（静态表） */
int ui_nplayer_hit(const ui_nplayer_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_nplayer_popup_inside(float x, float y);   /* 点在弹窗面板内 */

/* ★ P0（datah_062 证据）：两人新局简化弹窗——玩法(Lay)/明暗棋(Show)/确定/取消 4 控件。
   几何 720×1280 基准（datah_062 450×315 面板居中，偏移=(720-450)/2=135,(1280-315)/2=482.5；
   局部 1:1 平移：玩法 220,75,420,135 / 明暗棋 220,150,420,210 /
   确定 30,235,210,295 / 取消 240,235,420,295 →
   720 系 355,557-555,617 / 355,632-555,692 / 165,717-345,777 / 375,717-555,777）。
   action：0=玩法切换（明棋/翻棋暗棋循环）1=明暗棋切换（同玩法切换）2=确定（随模式开局）3=取消（回主菜单）。
   ★ 诚实边界：两人热座（AI 不介入），非四国/非 AI。 */
#define UI_NEWGAME_COUNT 4
#define UI_NEWGAME_POPUP_X0 135.0f
#define UI_NEWGAME_POPUP_Y0 482.5f
#define UI_NEWGAME_POPUP_W  450.0f
#define UI_NEWGAME_POPUP_H  315.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=玩法 1=明暗棋 2=确定 3=取消 */
} ui_newgame_btn;

const ui_newgame_btn *ui_newgame_buttons(void);   /* 4 控件（静态表） */
int ui_newgame_hit(const ui_newgame_btn *btns, int n, float x, float y);  /* 命中 0-3，-1 无 */
int ui_newgame_popup_inside(float x, float y);   /* 点在弹窗面板内 */

/* ★ P1（datah_012 证据）：游戏设置弹窗——4 规则开关（和棋/大本营/吃地雷/吃军旗）+ 确定/取消。
   几何 720×1280 基准（datah_012 450×465 面板居中，偏移=(720-450)/2=135,(1280-465)/2=407.5；
   局部 1:1 平移：和棋 210,75,420,135 → 345,483-555,543；大本营 210,150,420,210 →
   345,558-555,618；吃地雷 210,225,420,285 → 345,633-555,693；吃军旗 210,300,420,360 →
   345,708-555,768；确定 30,385,210,445 → 165,793-345,853；取消 240,385,420,445 → 375,793-555,853）。
   action：0=和棋开关 1=大本营开关 2=吃地雷开关 3=吃军旗开关 4=确定（持久化+应用到规则引擎）
   5=取消（恢复原值）。4 开关各循环翻转 开/关（对齐 datah_001 音量循环范式）。
   ★ 诚实边界：开关"开"状态的原版精确可选值 .rodata 未读全（和棋=70步/1000步判和、
   大本营=进大本营不可再动、地雷=工兵可挖雷、扛旗=扛旗胜为 src_rs 现状语义），默认全开。 */
#define UI_GAMESET_COUNT 6
#define UI_GAMESET_POPUP_X0 135.0f
#define UI_GAMESET_POPUP_Y0 407.5f
#define UI_GAMESET_POPUP_W  450.0f
#define UI_GAMESET_POPUP_H  465.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=和棋 1=大本营 2=吃地雷 3=吃军旗 4=确定 5=取消 */
} ui_gameset_btn;

const ui_gameset_btn *ui_gameset_buttons(void);   /* 6 控件（静态表） */
int ui_gameset_hit(const ui_gameset_btn *btns, int n, float x, float y);  /* 命中 0-5，-1 无 */
int ui_gameset_popup_inside(float x, float y);   /* 点在弹窗面板内 */

#endif
