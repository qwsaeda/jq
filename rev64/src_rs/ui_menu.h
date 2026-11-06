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
     UI_LOAD_LAY,   /* ★ P1（datah_017 证据）：载入布局弹窗——
                         布局名文本列表（6 条截断 + 选中高亮）+ 取消（原版 datah_017 400×535 面板居中）。
                         点列表项 → 载入该布局到棋盘 + 清 undo + 回对局；取消/面板外 → 回对局。
                         诚实边界：ListBox 滚动简化为文本列表 6 条截断（触摸 flaky 不画滚动条） */
     UI_SAVE_LAY,   /* ★ P1（datah_020 证据）：保存布局弹窗——
                         布局名列表 + 保存/改名/删除/关闭 4 按钮（原版 datah_020 400×535 面板居中）。
                         保存 = 当前棋盘 30 字节 .lay 写 layouts/；改名 = 旧名+数字后缀（简化）；
                         删除 = 删选中项 .lay；关闭/面板外 → 回对局 */
     UI_LAY_MGR,    /* ★ P1（datah_016 证据）：布局管理弹窗——
                         30 格棋子预览（简化小方块占位）+ 载入/删除/取消 3 按钮（原版 datah_016 400×475 面板居中）。
                         选中布局显示预览 → 载入（应用到棋盘）/ 删除（删文件）/ 取消。
                         诚实边界：30 格 PiecePic 真棋子图未画，小方块占位（有子=蓝/红） */
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

/* ★ P1（datah_017 证据）： 载入布局弹窗——布局名文本列表（6 条，原版 ListBox 20,75,380,440
   400 系 → 720 系行高 60px：447-807 每行一个可点区）+ 取消（原版 Cancel 120,455,280,515 →
   280,828-440,888）。面板 400×535 居中（偏移+160,+372.5，720 基准 160,373-560,908）。
   action：0-5=列表项（ui_layout_list 第 0-5 条）6=取消。
   ★ 诚实边界： ListBox 滚动未实现（触摸 flaky），文本列表 6 条截断 + 选中高亮。 */
#define UI_LOADLAYOUT_COUNT 7
#define UI_LOADLAYOUT_POPUP_X0 160.0f
#define UI_LOADLAYOUT_POPUP_Y0 372.5f
#define UI_LOADLAYOUT_POPUP_W  400.0f
#define UI_LOADLAYOUT_POPUP_H  535.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0-5=列表项 6=取消 */
} ui_loadlayout_btn;

const ui_loadlayout_btn *ui_loadlayout_buttons(void);   /* 7 控件（静态表） */
int ui_loadlayout_hit(const ui_loadlayout_btn *btns, int n, float x, float y);  /* 命中 0-6，-1 无 */
int ui_loadlayout_popup_inside(float x, float y);      /* 点在弹窗面板内 */

/* ★ P1（datah_020 证据）： 保存布局弹窗——布局名列表（同 017 ListBox 区）+ 保存/改名/删除/关闭
   4 按钮（原版 Ok 15,455,100,515 → 175,828-260,888；Rename 110,455,195,515 → 270,828-355,888；
   Delete 205,455,290,515 → 365,828-450,888；Cancel 300,455,385,515 → 460,828-545,888）。
   action：0=保存（当前棋盘 30 字节 .lay 写 layouts/）1=改名（旧名+数字后缀）2=删除（删选中项）
   3=关闭（回对局）。 ★ 诚实边界： 改名简化为自动编号（无文本输入框）。 */
#define UI_SAVELAYOUT_COUNT 4
#define UI_SAVELAYOUT_POPUP_X0 160.0f
#define UI_SAVELAYOUT_POPUP_Y0 372.5f
#define UI_SAVELAYOUT_POPUP_W  400.0f
#define UI_SAVELAYOUT_POPUP_H  535.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=保存 1=改名 2=删除 3=关闭 */
} ui_savelayout_btn;

const ui_savelayout_btn *ui_savelayout_buttons(void);   /* 4 按钮（静态表） */
int ui_savelayout_hit(const ui_savelayout_btn *btns, int n, float x, float y);  /* 命中 0-3，-1 无 */
int ui_savelayout_popup_inside(float x, float y);      /* 点在弹窗面板内 */

/* ★ P1（datah_016 证据）： 布局管理弹窗——30 格棋子预览（简化小方块 6×5，原版 Piece0-29
   20,75,380,360 区占位）+ 载入/删除/取消 3 按钮（原版 Ok 25,390,129,450 → 185,793-289,853；
   Delete 148,390,252,450 → 308,793-412,853；Cancel 271,390,375,450 → 431,793-535,853）。
   action：0=载入（应用到棋盘 + 清 undo）1=删除（删选中项文件）2=取消（回对局）。
   面板 400×475 居中（偏移+160,+402.5，720 基准 160,402-560,877）。
   ★ 诚实边界： 30 格 PiecePic 真棋子图未画（小方块占位，有子=蓝/红方块）；
   原版 Ok2/Cancel2 重叠备用显示按钮未实现。 */
#define UI_LAYMGR_COUNT 3
#define UI_LAYMGR_POPUP_X0 160.0f
#define UI_LAYMGR_POPUP_Y0 402.5f
#define UI_LAYMGR_POPUP_W  400.0f
#define UI_LAYMGR_POPUP_H  475.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=载入 1=删除 2=取消 */
} ui_laymgr_btn;

const ui_laymgr_btn *ui_laymgr_buttons(void);   /* 3 按钮（静态表） */
int ui_laymgr_hit(const ui_laymgr_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_laymgr_popup_inside(float x, float y);   /* 点在弹窗面板内 */
#endif

