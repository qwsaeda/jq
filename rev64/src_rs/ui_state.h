#ifndef RS_UI_STATE_H
#define RS_UI_STATE_H
#include "ui_menu.h"
int  ui_screen_get(void);
void ui_screen_set(int s);
void ui_on_menu_action(int action);   /* 菜单按钮点击：0人机→选项弹窗 4重开 其它占位 */
int  ui_game_active(void);            /* 对局态（触摸走子放行） */
/* ★ 难度/暗棋模式（玩法内部参数，非弹窗 UI）：level 1-4（新手/中级/高级/好友）
   mode 0=布局明棋 1=翻棋暗棋。原版人机直开局无难度弹窗（datah_041 判误），
   默认中级区/明棋（原版暗棋差异记录 EXPERIENCE_LOG）。 */
void ui_opt_select(int level, int mode);  /* level 1-4（新手/中级/高级/好友）mode 0=布局明棋 1=翻棋暗棋 */
int  ui_opt_get_level(void);              /* 返回 1-4 */
int  ui_opt_get_mode(void);               /* 返回 0=明棋 1=暗棋 */
#endif
