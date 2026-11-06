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
      UI_LOGIN,      /* ★ P1（datah_038 证据）：登录弹窗——
                  用户名/密码占位标签（EditLabel，不画真输入框）+ 登录/注册/游客/返回 4 按钮
                  （原版 datah_038 432×285 面板居中，局部坐标 1:1 平移偏移 +144,+497.5）。
                  诚实边界：原版登录=账号系统（登录/注册 需服务器协议，src_rs 未逆向，
                  Junqi.* 登录/注册 20+ 事件未实现）；游客=本地单机（无账号，唯一真生效路径）；
                  返回=回主菜单。入口：datah_072 方式弹窗"个人资料"→ UI_LOGIN +
                  setprop jq.login 测试钩子 */
UI_REPLAY,     /* ★ P1（datah_039/107 证据）：复盘回放态——
                          棋盘（当前回放位置）+ datah_107 底部 5 按钮控制条
                          （菜单/回退/前进/跳转/返回，460×60 @10,705）+ 步数标签"第 cur/n 步"。
                          入口：datah_039 BackDlg 复盘小浮标（原版命中 flaky）→ setprop jq.replay 测试钩子。
                          诚实边界：跳转=回开头(cur=0)/跳结尾(cur=n)循环（无步进输入框）；
                          菜单=退复盘回对局（无复盘内子菜单）；复盘态不画真实棋子图 */
  UI_RULES,      /* ★ P2（datah_037 证据）：规则文本弹窗——
                      规则-翻棋/规则-布局 2 模式切换按钮 + 返回（原版 datah_037 260×270 面板居中，
                      局部坐标 1:1 平移偏移 +230,+505）。点"规则-翻棋"→ 显示翻棋模式规则文本
                      （暗棋：棋子背面，按 rank 大小吃子，工兵挖雷，炸弹同归于尽，夺旗胜）；
                      点"规则-布局"→ 显示布局模式规则文本（明棋：自由布阵，军旗在大本营，
                      地雷后两行，炸弹前线）。入口：☰ 菜单"游戏规则"（本地 !bt_mode act=3 /
                      联机 bt_mode act=7）改切 UI_RULES 弹窗 + setprop jq.rules 测试钩子。
                      ★ 诚实边界：规则文本=src_rs 现有简化版固定文本（2 套：翻棋/布局） */
  UI_NEWGAME104, /* ★ P2（datah_104 证据）：新局完整版弹窗（我方/电脑水平/玩法 3 选择 +
                      确定/取消，原版 datah_104 450×390 面板居中，局部坐标 1:1 平移偏移 +135,+445）。
                      比 UI_NEWGAME（datah_062 简化版）多"我方/电脑水平"2 项：
                      我方=蓝/红 切换，电脑水平=难度 1-4 循环，玩法=布局/翻棋 切换；
                      3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。
                      确定 → 按选择开局（ui_opt_select + jq_restart_game 随模式）；取消 → 回主菜单。
                      入口：setprop jq.newgame104 测试钩子（不破坏 062 简化版 UI_NEWGAME）。
                      ★ 诚实边界：我方=蓝/红 切换简化（本机蓝先手固定，我方选择仅影响开局侧）；
                      文字显示当前值（循环切换简化版） */
  UI_JOIN,       /* ★ P2（datah_034 证据）：联机"加入游戏"弹窗（桌号输入 + 确定/取消，
                      原版 datah_034 420×235 面板居中，局部坐标 1:1 平移偏移 +150,+522.5）。
                      桌号=占位标签（不画真输入框）。入口：datah_072 方式弹窗"Wi-Fi 联机"
                      （wact=1）改切 UI_JOIN 弹窗 + setprop jq.join 测试钩子。
                      ★ 诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP；
                      确定 → 诚实边界对话框（"加入游戏需 Wi-Fi/局域网协议，src_rs 仅蓝牙联机已实现"）
                      + 回主菜单；取消 → 回主菜单 */
  UI_CREATE,     /* ★ P2（datah_035 证据）：联机"创建游戏"弹窗（密码输入 + 确定/取消，
                      原版 datah_035 420×235 面板居中，局部坐标 1:1 平移偏移 +150,+522.5）。
                      密码=占位标签（不画真输入框）。入口：setprop jq.create 测试钩子。
                      ★ 诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP；
                      确定 → 诚实边界对话框（"创建房间需 Wi-Fi/局域网协议，src_rs 仅蓝牙联机已实现"）
                      + 回主菜单；取消 → 回主菜单 */
   UI_NG061,      /* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗（先后手/电脑水平/玩法/明暗棋
                       4 选择 + 确定/取消，原版 datah_061 450×465 面板居中，局部坐标 1:1 平移偏移
                       +135,+407.5）。比 UI_NEWGAME104（datah_104 简化版）多"先后手/明暗棋"2 项：
                       先后手=蓝/红 循环，电脑水平=难度 1-4 循环，玩法=布局/翻棋 切换，
                       明暗棋=明/暗 切换；4 项各循环切换当前值（对齐 datah_104 循环切换范式）。
                       确定 → 按选择 ui_opt_select + jq_restart_game 随模式开局；取消 → 回主菜单。
                       入口：setprop jq.newgame061 测试钩子（现有 UI_DIFFICULTY datah_041 简化版不破坏）。
                       ★ 诚实边界：4 项各循环切换当前值简化版（先后手/明暗棋 各占独立循环位） */
   UI_GAMEINFO,   /* ★ P2（datah_050 证据）：对局信息弹窗（Info 文本框 + 复盘/删除/返回 3 按钮，
                       原版 datah_050 420×380 面板居中，局部坐标 1:1 平移偏移 +150,+450）。
                       Info 文本=☰ 菜单"战况统计"复用（当前回合/总步数/蓝红余子/winner）；
                       复盘 → 进 UI_REPLAY 复盘态（复用 P1 复盘）；删除 → 清复盘记录
                       （诚实边界：原版删除对局存档未逆向）；返回 → 回对局。
                       入口：setprop jq.gameinfo 测试钩子 */
   UI_BRIGHT,     /* ★ P2（datah_002 证据）：亮度调节弹窗（滑条简化=亮度 低/中/高 循环切换
                       + 确定/取消，原版 datah_002 460×240 面板居中，局部坐标 1:1 平移偏移
                       +130,+520）。★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换
                       （对齐 datah_001 音量循环范式），不画真滑条。
                       确定 → 保存 g_bright/ui_volume；取消 → 恢复原值。
                       入口：setprop jq.bright 测试钩子 */
   UI_CHATLOG,    /* ★ P2（datah_029 证据）：聊天记录弹窗（Msg 文本框 + 清除/关闭 2 按钮，
                       原版 datah_029 420×530 面板居中，局部坐标 1:1 平移偏移 +150,+375）。
                       ★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是
                       占位帧（BT_T_CHAT，对端无文本渲染）；Msg 文本框显示诚实边界占位文本 +
                       清除 → 清本地占位记录 + 关闭 → 回对局。
                       入口：setprop jq.chatlog 测试钩子 */
   UI_PROFILE,    /* ★ P2（datah_031 证据）：个人资料弹窗（Info 文本框 + 修改/帮助/返回 3 按钮，
                       原版 datah_031 420×520 面板居中，局部坐标 1:1 平移偏移 +150,+380）。
                       Info 文本=诚实边界占位（账号系统未逆向，离线单机无账号）；
                       修改 → 诚实边界对话框（"修改资料需账号系统，未实现"）；
                       帮助 → 显示帮助文本；返回 → 回主菜单。
                       入口：setprop jq.profile 测试钩子（现有 datah_072"个人资料"→UI_LOGIN
                       不破坏，031 作为登录后资料查看补充，独立态） */
    UI_DRAWDETAIL, /* ★ P2（datah_042 证据）：提和详情弹窗（联机版 ☰ 菜单 7 图标：
                        提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，
                        原版 datah_042 450×285 面板居中，局部坐标 1:1 平移偏移 +135,+497.5；
                        比 datah_067 8 图标少 1 项"游戏规则"，多 1 项"玩家资料"）。
                        提和 → jq_bt_ctrl_send(BT_T_DRAW) + 诚实边界对话框（本地提和判定未
                        实现，原版需对端应答）；认输 → g_state.winner=对方 stage=OVER +
                        jq_bt_ctrl_send(BT_T_RESIGN)；聊天 → jq_bt_ctrl_send(BT_T_CHAT) +
                        边界对话框（联机聊天文本未逆向）；系统设置 → UI_GAMESET（datah_012）；
                        玩家资料 → UI_PROFILE（datah_031）；战况统计/软件信息 → 复用 ☰ 菜单
                        逻辑（遍历 cells 蓝红余子+总步数+回合 / jq_show_dialog 版本）。
                        入口：联机版 ☰ 菜单（datah_067）"提和"(act=2) 改切 UI_DRAWDETAIL
                        （原直接发帧升级为完整弹窗入口，发帧逻辑保留在弹窗内）+
                        setprop jq.drawdetail 测试钩子（OnCreate + OnUpdate 双路）。
                        ★ 诚实边界：本地提和判定未实现（src_rs 离线单机无对端），
                        联机提和/认输走 BT 帧 + 边界对话框，不包装成"已提和判定" */

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

/* ★ P1（datah_107 证据）：复盘回放控制条 5 按钮（菜单/回退/前进/跳转/返回）。
   几何 720×1280 基准：面板 460×60 @ 绝对 10,705（不居中，局部 1:1 平移）；
   5 按钮各 84×60（步长 94，原版 datah_107 精确 720 边界：
   10,104 / 104,188 / 188,282 / 292,376 / 386,470；y 705-765）。
   action：0=菜单（简化=退复盘回对局）1=回退（cur-- 重放）
   2=前进（cur++ 走 1 步）3=跳转（简化=回开头 cur=0 / 跳结尾 cur=n 循环）
   4=返回（退复盘回对局/主菜单）。
   ★ 诚实边界：跳转无步进输入框（二选一循环）；菜单无复盘内子菜单。 */
#define UI_REPLAY_CTRL_COUNT 5
#define UI_REPLAY_CTRL_POPUP_X0 10.0f
#define UI_REPLAY_CTRL_POPUP_Y0 705.0f
#define UI_REPLAY_CTRL_POPUP_W  460.0f
#define UI_REPLAY_CTRL_POPUP_H  60.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=菜单 1=回退 2=前进 3=跳转 4=返回 */
} ui_replay_btn;

const ui_replay_btn *ui_replay_ctrl_buttons(void);  /* 5 按钮（静态表） */
int ui_replay_ctrl_hit(const ui_replay_btn *btns, int n, float x, float y);  /* 命中 0-4，-1 无 */
int ui_replay_ctrl_popup_inside(float x, float y); /* 点在控制条面板内 */

/* ★ P1（datah_039 证据）：复盘入口小浮标（BackDlg 单按钮，绝对坐标 27,650，120×90
   不居中；Rep 局部 15,15,90,60 → 720 基准 42,665-132,725）。
   业务：对局结束/进行中点"复盘"→ 进 UI_REPLAY。★ 诚实边界：原版浮标命中 flaky，
   真机入口用 setprop jq.replay 测试钩子触发（几何保留供 layout_align_check Δ 校验）。 */
#define UI_REPLAY_ENTRY_X0 42.0f
#define UI_REPLAY_ENTRY_Y0 665.0f
#define UI_REPLAY_ENTRY_X1 132.0f
#define UI_REPLAY_ENTRY_Y1 725.0f

/* ★ P1（datah_038 证据）：登录弹窗——用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮。
   几何 720×1280 基准（datah_038 432×285 面板居中，偏移=(720-432)/2=144,(1280-285)/2=497.5；
   局部 1:1 平移：用户名 EditLabel 130,80,375,125 → 274,578-519,623；
   密码 EditLabel 130,140,375,185 → 274,638-519,683；
   登录 Login 18,205,108,265 → 162,703-252,763；
   注册 Register 120,205,210,265 → 264,703-354,763；
   游客 Guest 222,205,312,265 → 366,703-456,763；
   返回 Cancel 324,205,414,265 → 468,703-558,763）。
   action：0=用户名标签 1=密码标签 2=登录（诚实边界对话框）3=注册（诚实边界对话框）
   4=游客（本地单机开局）5=返回（回主菜单）。
   ★ 诚实边界：原版登录=账号系统（登录/注册 需服务器协议，src_rs 未逆向，
   Junqi.* 登录/注册 20+ 事件未实现）；用户名/密码 输入框简化为占位标签（触摸 flaky
   下输入框无意义）；游客=本地单机模式（无账号，唯一真生效路径）。 */
#define UI_LOGIN_COUNT 6
#define UI_LOGIN_POPUP_X0 144.0f
#define UI_LOGIN_POPUP_Y0 497.5f
#define UI_LOGIN_POPUP_W  432.0f
#define UI_LOGIN_POPUP_H  285.0f
typedef struct {
    float x0, y0, x1, y1;   /* 控件矩形（720×1280 基准） */
    int action;             /* 0=用户名 1=密码 2=登录 3=注册 4=游客 5=返回 */
} ui_login_btn;

const ui_login_btn *ui_login_buttons(void);   /* 6 控件（静态表） */
int ui_login_hit(const ui_login_btn *btns, int n, float x, float y);  /* 命中 0-5，-1 无 */
int ui_login_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_037 证据）：规则文本弹窗——规则-翻棋/规则-布局 2 模式按钮 + 返回。
   几何 720×1280 基准（datah_037 260×270 面板居中，偏移=(720-260)/2=230,(1280-270)/2=505；
   局部 1:1 平移：翻棋 25,25,235,85 → 255,530-465,590；布局 25,105,235,165 →
   255,610-465,670；返回 25,185,235,245 → 255,690-465,750）。
   action：0=规则-翻棋（显示翻棋模式规则文本，暗棋）1=规则-布局（显示布局模式规则文本，明棋）
   2=返回（回对局/主菜单）。★ 诚实边界：规则文本=src_rs 现有简化版固定文本（2 套）。 */
#define UI_RULES_COUNT 3
#define UI_RULES_POPUP_X0 230.0f
#define UI_RULES_POPUP_Y0 505.0f
#define UI_RULES_POPUP_W  260.0f
#define UI_RULES_POPUP_H  270.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=规则-翻棋 1=规则-布局 2=返回 */
} ui_rules_btn;

const ui_rules_btn *ui_rules_buttons(void);   /* 3 按钮（静态表） */
int ui_rules_hit(const ui_rules_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_rules_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_104 证据）：新局完整版弹窗——我方/电脑水平/玩法 3 选择 + 确定/取消。
   几何 720×1280 基准（datah_104 450×390 面板居中，偏移=(720-450)/2=135,(1280-390)/2=445；
   局部 1:1 平移：我方 220,75,420,135 → 355,520-555,580；电脑水平 220,150,420,210 →
   355,595-555,655；玩法 220,225,420,285 → 355,670-555,730；
   确定 30,310,210,370 → 165,755-345,815；取消 240,310,420,370 → 375,755-555,815）。
   action：0=我方（蓝/红 切换）1=电脑水平（难度 1-4 循环）2=玩法（布局/翻棋 切换）
   3=确定（按选择开局）4=取消（回主菜单）。
   ★ 诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。 */
#define UI_NEWGAME104_COUNT 5
#define UI_NEWGAME104_POPUP_X0 135.0f
#define UI_NEWGAME104_POPUP_Y0 445.0f
#define UI_NEWGAME104_POPUP_W  450.0f
#define UI_NEWGAME104_POPUP_H  390.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=我方 1=电脑水平 2=玩法 3=确定 4=取消 */
} ui_newgame104_btn;

const ui_newgame104_btn *ui_newgame104_buttons(void);   /* 5 控件（静态表） */
int ui_newgame104_hit(const ui_newgame104_btn *btns, int n, float x, float y);  /* 命中 0-4，-1 无 */
int ui_newgame104_popup_inside(float x, float y);   /* 点在弹窗面板内 */

/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗——桌号输入（占位标签）+ 确定/取消。
   几何 720×1280 基准（datah_034 420×235 面板居中，偏移=(720-420)/2=150,(1280-235)/2=522.5；
   局部 1:1 平移：桌号 120,80,360,125 → 270,603-510,648；
   确定 35,150,195,210 → 185,673-345,733；取消 225,150,385,210 → 375,673-535,733）。
   action：0=桌号（占位标签）1=确定（诚实边界对话框）2=取消（回主菜单）。
   ★ 诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP；
   确定 → 诚实边界对话框 + 回主菜单；取消 → 回主菜单。 */
#define UI_JOIN_COUNT 3
#define UI_JOIN_POPUP_X0 150.0f
#define UI_JOIN_POPUP_Y0 522.5f
#define UI_JOIN_POPUP_W  420.0f
#define UI_JOIN_POPUP_H  235.0f
typedef struct {
    float x0, y0, x1, y1;   /* 控件矩形（720×1280 基准） */
    int action;             /* 0=桌号 1=确定 2=取消 */
} ui_join_btn;

const ui_join_btn *ui_join_buttons(void);   /* 3 控件（静态表） */
int ui_join_hit(const ui_join_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_join_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗——密码输入（占位标签）+ 确定/取消。
   几何 720×1280 基准（datah_035 420×235 面板居中，偏移同 034=+150,+522.5；
   局部 1:1 平移：密码 100,80,320,125 → 250,603-470,648；
   确定 35,150,195,210 → 185,673-345,733；取消 225,150,385,210 → 375,673-535,733）。
   action：0=密码（占位标签）1=确定（诚实边界对话框）2=取消（回主菜单）。
   ★ 诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP；
   确定 → 诚实边界对话框 + 回主菜单；取消 → 回主菜单。 */
#define UI_CREATE_COUNT 3
#define UI_CREATE_POPUP_X0 150.0f
#define UI_CREATE_POPUP_Y0 522.5f
#define UI_CREATE_POPUP_W  420.0f
#define UI_CREATE_POPUP_H  235.0f
typedef struct {
    float x0, y0, x1, y1;   /* 控件矩形（720×1280 基准） */
    int action;             /* 0=密码 1=确定 2=取消 */
} ui_create_btn;

const ui_create_btn *ui_create_buttons(void);   /* 3 控件（静态表） */
int ui_create_hit(const ui_create_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_create_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗——先后手/电脑水平/玩法/明暗棋
   4 选择 + 确定/取消（比 datah_104 简化版多"先后手/明暗棋"2 项）。
   几何 720×1280 基准（datah_061 450×465 面板居中，偏移=(720-450)/2=135,(1280-465)/2=407.5；
   局部 1:1 平移：先后手 220,75,420,135 → 355,482.5-555,542.5 → 取整 355,483-555,543；
   电脑水平 220,150,420,210 → 355,557.5-555,617.5 → 355,558-555,618；
   玩法 220,225,420,285 → 355,632.5-555,692.5 → 355,633-555,693；
   明暗棋 220,300,420,360 → 355,707.5-555,767.5 → 355,708-555,768；
   确定 30,385,210,445 → 165,792.5-345,852.5 → 165,793-345,853；
   取消 240,385,420,445 → 375,792.5-555,852.5 → 375,793-555,853）。
   action：0=先后手（蓝/红 循环）1=电脑水平（难度 1-4 循环）2=玩法（布局/翻棋 切换）
   3=明暗棋（明/暗 切换，同玩法位）4=确定（按选择 ui_opt_select + jq_restart_game 开局）
   5=取消（回主菜单）。
   入口：setprop jq.newgame061 测试钩子（原版入口=主菜单"人机"完整设置；现有
   UI_DIFFICULTY datah_041 简化版不破坏，061 作为完整版补充，独立态）。
   ★ 诚实边界：4 项各循环切换当前值（对齐 datah_104 循环切换范式）；
   文字显示当前值（先后手=蓝/红；电脑水平=1-4；玩法=布局/翻棋；明暗棋=明/暗）。 */
#define UI_NG061_COUNT 6
#define UI_NG061_POPUP_X0 135.0f
#define UI_NG061_POPUP_Y0 407.5f
#define UI_NG061_POPUP_W  450.0f
#define UI_NG061_POPUP_H  465.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=先后手 1=电脑水平 2=玩法 3=明暗棋 4=确定 5=取消 */
} ui_ng061_btn;

const ui_ng061_btn *ui_ng061_buttons(void);   /* 6 控件（静态表） */
int ui_ng061_hit(const ui_ng061_btn *btns, int n, float x, float y);  /* 命中 0-5，-1 无 */
int ui_ng061_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_050 证据）：对局信息弹窗——Info 文本框（复用 ☰ 菜单"战况统计"文本）
   + 复盘/删除/返回 3 按钮。
   几何 720×1280 基准（datah_050 420×380 面板居中，偏移=(720-420)/2=150,(1280-380)/2=450；
   局部 1:1 平移：复盘 25,295,135,355 → 175,745-285,805；
   删除 155,295,265,355 → 305,745-415,805；
   返回 285,295,395,355 → 435,745-545,805）。
   action：0=复盘（进 UI_REPLAY 复盘态，复用 P1 复盘）1=删除（清复盘记录，
   诚实边界：原版删除对局存档未逆向）2=返回（回对局）。
   入口：setprop jq.gameinfo 测试钩子。★ 诚实边界：Info 文本=☰ 战况统计复用，
   删除=清复盘记录简化。 */
#define UI_GAMEINFO_COUNT 3
#define UI_GAMEINFO_POPUP_X0 150.0f
#define UI_GAMEINFO_POPUP_Y0 450.0f
#define UI_GAMEINFO_POPUP_W  420.0f
#define UI_GAMEINFO_POPUP_H  380.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=复盘 1=删除 2=返回 */
} ui_gameinfo_btn;

const ui_gameinfo_btn *ui_gameinfo_buttons(void);   /* 3 按钮（静态表） */
int ui_gameinfo_hit(const ui_gameinfo_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_gameinfo_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_002 证据）：亮度调节弹窗——滑条（简化=亮度 低/中/高 循环切换区）
   + 确定/取消。
   几何 720×1280 基准（datah_002 460×240 面板居中，偏移=(720-460)/2=130,(1280-240)/2=520；
   局部 1:1 平移：亮度滑条 Bright 22,81,438,129 → 152,601-568,649；
   确定 30,155,215,215 → 160,675-345,735；取消 245,155,430,215 → 375,675-560,735）。
   action：0=亮度（低/中/高 3 档循环切换，不画真滑条）1=确定（保存 g_bright 或
   ui_volume 持久化）2=取消（恢复原值）。
   入口：setprop jq.bright 测试钩子。★ 诚实边界：原版滑条拖拽=触摸 flaky，
   简化为"低/中/高"3 档循环切换（对齐 datah_001 音量循环范式），不画真滑条。 */
#define UI_BRIGHT_COUNT 3
#define UI_BRIGHT_POPUP_X0 130.0f
#define UI_BRIGHT_POPUP_Y0 520.0f
#define UI_BRIGHT_POPUP_W  460.0f
#define UI_BRIGHT_POPUP_H  240.0f
typedef struct {
    float x0, y0, x1, y1;   /* 控件矩形（720×1280 基准） */
    int action;             /* 0=亮度 1=确定 2=取消 */
} ui_bright_btn;

const ui_bright_btn *ui_bright_buttons(void);   /* 3 控件（静态表） */
int ui_bright_hit(const ui_bright_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_bright_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_029 证据）：聊天记录弹窗（联机聊天历史）——Msg 文本框 + 清除/关闭 2 按钮。
   几何 720×1280 基准（datah_029 420×530 面板居中，偏移=(720-420)/2=150,(1280-530)/2=375；
   局部 1:1 平移：清除 37,450,193,510 → 187,825-343,885；关闭 227,450,383,510 → 377,825-533,885）。
   action：0=清除（清本地占位记录）1=关闭（回对局）。
   入口：setprop jq.chatlog 测试钩子。★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，
   src_rs 蓝牙聊天是占位帧（BT_T_CHAT，对端无文本渲染）；Msg 文本框显示诚实边界占位文本。 */
#define UI_CHATLOG_COUNT 2
#define UI_CHATLOG_POPUP_X0 150.0f
#define UI_CHATLOG_POPUP_Y0 375.0f
#define UI_CHATLOG_POPUP_W  420.0f
#define UI_CHATLOG_POPUP_H  530.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=清除 1=关闭 */
} ui_chatlog_btn;

const ui_chatlog_btn *ui_chatlog_buttons(void);   /* 2 按钮（静态表） */
int ui_chatlog_hit(const ui_chatlog_btn *btns, int n, float x, float y);  /* 命中 0-1，-1 无 */
int ui_chatlog_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_031 证据）：个人资料弹窗（账号信息）——Info 文本框 + 修改/帮助/返回 3 按钮。
   几何 720×1280 基准（datah_031 420×520 面板居中，偏移=(720-420)/2=150,(1280-520)/2=380；
   局部 1:1 平移：修改 25,435,135,495 → 175,815-285,875；
   帮助 155,435,265,495 → 305,815-415,875；
   返回 285,435,395,495 → 435,815-545,875）。
   action：0=修改（诚实边界对话框：修改资料需账号系统未实现）1=帮助（显示帮助文本）
   2=返回（回主菜单）。
   入口：setprop jq.profile 测试钩子（现有 datah_072 方式弹窗"个人资料"(wact=2)
   已改切 UI_LOGIN（P1 登录弹窗），031 作为登录后资料查看补充，独立态，不破坏 UI_LOGIN）。
   ★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框显示诚实
   边界占位文本；修改=边界对话框。 */
#define UI_PROFILE_COUNT 3
#define UI_PROFILE_POPUP_X0 150.0f
#define UI_PROFILE_POPUP_Y0 380.0f
#define UI_PROFILE_POPUP_W  420.0f
#define UI_PROFILE_POPUP_H  520.0f
typedef struct {
    float x0, y0, x1, y1;   /* 按钮矩形（720×1280 基准） */
    int action;             /* 0=修改 1=帮助 2=返回 */
} ui_profile_btn;

const ui_profile_btn *ui_profile_buttons(void);   /* 3 按钮（静态表） */
int ui_profile_hit(const ui_profile_btn *btns, int n, float x, float y);  /* 命中 0-2，-1 无 */
int ui_profile_popup_inside(float x, float y);  /* 点在弹窗面板内 */

/* ★ P2（datah_042 证据）：提和详情弹窗（联机版 ☰ 菜单 7 图标：提和/认输/聊天/系统设置/
    玩家资料/战况统计/软件信息）。比 datah_067 8 图标少 1 项"游戏规则"，多 1 项"玩家资料"。
    几何 720×1280 基准（datah_042 450×285 面板居中，偏移=(720-450)/2=135,(1280-285)/2=497.5；
    局部 1:1 平移 7 图标 105×135：
    提和 15,25 → 150,522.5-255,657.5 → 150,523-255,658；
    认输 120,25 → 255,523-360,658；聊天 225,25 → 360,523-465,658；
    系统设置 330,25 → 465,523-570,658；
    玩家资料 15,150 → 150,647.5-255,782.5 → 150,648-255,783；
    战况统计 120,150 → 255,648-360,783；软件信息 225,150 → 360,648-465,783）。
    action：0=提和（BT_T_DRAW 帧 + 诚实边界对话框）1=认输（winner=对方 stage=OVER +
    BT_T_RESIGN 帧）2=聊天（BT_T_CHAT 帧 + 边界对话框）3=系统设置（UI_GAMESET）
    4=玩家资料（UI_PROFILE）5=战况统计（复用 ☰ 菜单遍历 cells 逻辑）
    6=软件信息（jq_show_dialog 版本，复用 ☰ 菜单逻辑）。
    入口：联机版 ☰ 菜单（datah_067）"提和"(act=2) 改切 UI_DRAWDETAIL + setprop jq.drawdetail。
    ★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
    联机提和/认输走 BT 帧 + 边界对话框，不包装成"已提和判定"。 */
#define UI_DRAWDETAIL_COUNT 7
#define UI_DRAWDETAIL_POPUP_X0 135.0f
#define UI_DRAWDETAIL_POPUP_Y0 497.5f
#define UI_DRAWDETAIL_POPUP_W  450.0f
#define UI_DRAWDETAIL_POPUP_H  285.0f
typedef struct {
    float x0, y0, x1, y1;   /* 图标矩形（720×1280 基准） */
    int action;             /* 0=提和 1=认输 2=聊天 3=系统设置 4=玩家资料 5=战况统计 6=软件信息 */
} ui_drawdetail_btn;

const ui_drawdetail_btn *ui_drawdetail_buttons(void);   /* 7 图标（静态表） */
int ui_drawdetail_hit(const ui_drawdetail_btn *btns, int n, float x, float y);  /* 命中 0-6，-1 无 */
int ui_drawdetail_popup_inside(float x, float y);  /* 点在弹窗面板内 */
#endif

