/* ui_state_stub.c — 桌面 RS_NO_GL 桩（UI 菜单状态机；桌面 headless 无 GL，
   ui_screen_get 恒返 UI_GAME 等价：对局逻辑正常跑，菜单渲染无意义。 */
#include "ui_state.h"
static int g_ui = UI_GAME;
static int g_bt_act = 0;
int ui_screen_get(void) { return g_ui; }
void ui_screen_set(int s) { g_ui = s; }
int ui_game_active(void) { return g_ui == UI_GAME; }
void ui_on_menu_action(int action) { (void)action; }
void ui_bt_set_active(int on) { g_bt_act = on; }
int ui_bt_active(void) { return g_bt_act; }
static int g_mute = 0, g_btn = 1, g_vol = 2;
int ui_snd_mute_get(void) { return g_mute; }
void ui_snd_mute_set(int m) { g_mute = m ? 1 : 0; }
int ui_btn_sound_get(void) { return g_btn; }
void ui_btn_sound_set(int v) { g_btn = v ? 1 : 0; }
int ui_volume_get(void) { return g_vol; }
void ui_volume_set(int v) { g_vol = (v >= 1 && v <= 3) ? v : 2; }
void ui_settings_load(void) {}
void ui_settings_save(void) {}
void ui_settings_set_dir(const char *dir) { (void)dir; }
/* ★ P1（datah_012）：游戏设置 4 规则开关桩（默认全开，对齐原版默认） */
static int g_rd = 1, g_rh = 1, g_rm = 1, g_rf = 1;
int ui_rule_draw_get(void) { return g_rd; }
void ui_rule_draw_set(int v) { g_rd = v ? 1 : 0; }
int ui_rule_hq_get(void) { return g_rh; }
void ui_rule_hq_set(int v) { g_rh = v ? 1 : 0; }
int ui_rule_mine_get(void) { return g_rm; }
void ui_rule_mine_set(int v) { g_rm = v ? 1 : 0; }
int ui_rule_flag_get(void) { return g_rf; }
void ui_rule_flag_set(int v) { g_rf = v ? 1 : 0; }
int ui_rule_cfg_get(int which)
{
    switch (which) { case 0: return g_rd; case 1: return g_rh; case 2: return g_rm; default: return g_rf; }
}
void ui_rules_load(void) {}
void ui_rules_save(void) {}
