#ifndef RS_UI_STATE_H
#define RS_UI_STATE_H
#include "ui_menu.h"
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
void ui_settings_load(void);  /* 读文件（OnCreate 调，需 g_files_dir 已设） */
void ui_settings_save(void);  /* 写文件（设置确定后调） */
void ui_settings_set_dir(const char *dir);  /* 设存储目录（jni_bridge g_files_dir 同步） */
#endif
