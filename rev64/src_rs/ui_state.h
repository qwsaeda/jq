#ifndef RS_UI_STATE_H
#define RS_UI_STATE_H
#include "ui_menu.h"
#include "replay.h"
int  ui_screen_get(void);
void ui_screen_set(int s);
void ui_on_menu_action(int action);   /* 菜单按钮点击：0人机→选项弹窗 4重开 其它占位 */
int  ui_game_active(void);            /* 对局态（触摸走子放行） */
/* ★ 难度/暗棋模式（玩法内部参数，非弹窗 UI）：level 1-4（新手/中级/高级/好友）
   mode 0=布局明棋 1=翻棋暗棋。★ 对抗审计（datah_041 证据）：原版人机弹难度选择弹窗
   （新手/中级/高级/好友区 × 布局/翻棋 8 按钮），选完开局——非直开局。默认中级/暗棋。 */
void ui_opt_select(int level, int mode);  /* level 1-4（新手/中级/高级/好友）mode 0=布局明棋 1=翻棋暗棋 */
int  ui_opt_get_level(void);              /* 返回 1-4 */
int  ui_opt_get_mode(void);               /* 返回 0=明棋 1=暗棋 */
/* ★ 对抗审计（datah_067）：蓝牙联机模式标志——☰ 菜单联机版（datah_067：提和/认输/聊天）
   与本地版（datah_006：电脑变着/软件信息/退出）分流 */
void ui_bt_set_active(int on);
int  ui_bt_active(void);
/* ★ 对抗审计（datah_001）：声音设置（静音模式/按键声音/音量 1-3）——功能设置弹窗读/写 */
int  ui_snd_mute_get(void);   /* 0=声音开 1=静音 */
void ui_snd_mute_set(int m);
int  ui_btn_sound_get(void);  /* 0=关 1=开（按键声） */
void ui_btn_sound_set(int v);
int  ui_volume_get(void);     /* 1=低 2=中 3=高 */
void ui_volume_set(int v);
/* ★ P2（datah_061）：新局完整版（完整版补充）先后手/明暗棋 独立循环位
   （比 ui_opt 多 2 个循环状态：先后手=蓝先手/红先手，明暗棋=明/暗 2 位）。
   ui_first 0=蓝先手 1=红先手（默认 0 蓝先手）；ui_show 0=明 1=暗（默认 1 暗棋，
   对齐原版暗棋默认）。★ 诚实边界：先后手=蓝/红 切换简化（仅影响开局侧）。 */
int  ui_first_get(void);      /* 0=蓝先手 1=红先手（默认 0） */
void ui_first_set(int v);
int  ui_show_get(void);       /* 0=明 1=暗（默认 1，对齐原版暗棋默认） */
void ui_show_set(int v);
/* ★ P2（datah_002）：亮度调节档位（0=低 1=中 2=高，默认 1）。
   ★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换，不画真滑条；
   确定=持久化（单字段 jq_bright.txt，复用 g_set_dir 范式）。 */
int  ui_bright_get(void);     /* 0=低 1=中 2=高（默认 1） */
void ui_bright_set(int v);
void ui_bright_save(void);    /* 写 jq_bright.txt（确定后调） */
void ui_bright_load(void);    /* 读 jq_bright.txt（取消/OnCreate 调，缺文件默认中） */
void ui_settings_load(void);  /* 读文件（OnCreate 调，需 g_files_dir 已设） */
void ui_settings_save(void);  /* 写文件（设置确定后调） */
void ui_settings_set_dir(const char *dir);  /* 设存储目录（jni_bridge g_files_dir 同步） */

/* ★ P1（datah_012 证据）：游戏设置 4 规则开关（和棋/大本营/吃地雷/吃军旗），
   默认全开（对齐原版默认，行为不变——一致性回归 9/9 不破）。
   g_rule_* 供 rules.c 判定点守卫；jq_rules.txt 持久化（OnCreate 读 / 确定写 / 取消恢复）。 */
int  ui_rule_draw_get(void);   /* 和棋开关：1=开（70 步无吃子/总 1000 步判和） */
void ui_rule_draw_set(int v);
int  ui_rule_hq_get(void);     /* 大本营开关：1=开（进大本营不可再动） */
void ui_rule_hq_set(int v);
int  ui_rule_mine_get(void);   /* 吃地雷开关：1=开（工兵可挖地雷） */
void ui_rule_mine_set(int v);
int  ui_rule_flag_get(void);   /* 吃军旗开关：1=开（扛旗判胜） */
void ui_rule_flag_set(int v);
void ui_rules_load(void);      /* 读 jq_rules.txt（OnCreate 调，缺文件 → 默认全开） */
void ui_rules_save(void);     /* 写 jq_rules.txt（游戏设置确定后调） */
/* 规则引擎读入口（rules.c 判定点守卫用，返回 4 开关值） */
int  ui_rule_cfg_get(int which);  /* which 0=和棋 1=大本营 2=吃地雷 3=吃军旗，返回 0/1 */

/* ★ P1（datah_016/017/020 证据）：布局存档 API——.lay 30 字节格式（对齐原版
   OnEvent layout 读 30 字节范式 / data/lay/%02d.lay），layouts/ 目录文件 IO。
   目录：ui_laydir_set（jni_bridge OnCreate 设 /data/local/tmp/layouts；桌面 evverify
   设 tmp 目录；未设时桌面缺省 ./layouts、真机缺省 /data/local/tmp/layouts）。 */
void ui_laydir_set(const char *dir);
const char *ui_laydir_get(void);
int  ui_layout_list(void);                 /* 刷新目录扫描（readdir *.lay 排序）→ 返回条数 */
int  ui_layout_count(void);                /* 当前列表条数（≤64） */
const char *ui_layout_name(int i);         /* 第 i 条布局名（无扩展名，越界/空=0） */
int  ui_layout_selected(void);            /* 017/020/016 共享选中项（默认 0） */
void ui_layout_select(int i);              /* 设选中项（越界 clamp） */
int  ui_layout_selected_valid(void);      /* 选中项在列表内 = 1 */
int  ui_layout_save(const uint8_t lay[30], const char *name); /* 写 <dir>/<name>.lay（30 字节） */
int  ui_layout_load(const char *name, uint8_t out[30]);        /* 读 <dir>/<name>.lay，返回 0 成功/负失败 */
int  ui_layout_delete(const char *name);   /* 删 <dir>/<name>.lay，返回 0 成功/负失败 */
int  ui_layout_rename(const char *old, const char *new);       /* 重命名（改名=旧名+数字后缀，简化） */
/* ★ 真机目录缺省：jni_bridge OnCreate 调 ui_laydir_set("/data/local/tmp/layouts")。
   桌面 evverify：ui_laydir_set(tmp 目录)。未 set 时桌面缺省 ./layouts（mkdir）。 */

/* ★ P1（datah_039/107 证据）：复盘回放——走子记录 + 逐 cursor 重放。
   记录在 ui_state 维护（对局生命周期），jni_bridge 每次 jq_try_move/jq_flip
   成功后调 ui_replay_record；开局/重开/布局载入调 ui_replay_init。
   回放 = 初始布局 + 前 cur 步 jq_try_move/jq_flip 重走（确定性，桌面可验证）。
   诚实边界：跳转简化为回开头(cur=0)/跳结尾(cur=n)循环；菜单简化为退复盘。 */
void ui_replay_init(void);                /* 开局/重开/布局载入清记录（需 g_mutex 内，读 g_state） */
void ui_replay_record(int from, int to, int is_flip);  /* 走子/翻棋成功后追加 */
int  ui_replay_count(void);                /* 已记录步数 */
int  ui_replay_cur(void);                 /* 回放游标 */
int  ui_replay_rewind(int cur);           /* 重放前 cur 步到 g_state（0/负=越界不重建；越界返 -1 不改 state） */
#endif
