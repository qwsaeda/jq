/* ui_state_stub.c — 桌面 RS_NO_GL 桩（UI 菜单状态机；桌面 headless 无 GL，
   ui_screen_get 恒返 UI_GAME 等价：对局逻辑正常跑，菜单渲染无意义。 */
#include "ui_state.h"
static int g_ui = UI_GAME;
int ui_screen_get(void) { return g_ui; }
void ui_screen_set(int s) { g_ui = s; }
int ui_game_active(void) { return g_ui == UI_GAME; }
void ui_on_menu_action(int action) { (void)action; }
