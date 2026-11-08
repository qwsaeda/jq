/* ui_state_stub.c — 桌面 RS_NO_GL 桩（UI 菜单状态机；桌面 headless 无 GL，
   ui_screen_get 恒返 UI_GAME 等价：对局逻辑正常跑，菜单渲染无意义。 */
#include "ui_state.h"
#include <stdio.h>
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
/* ★ P2（datah_061/002）：先后手/明暗棋/亮度 桩（桌面 headless 无持久化，默认值供
   一致性/evverify 链接；真机走 ui_state.c 真实实现）。 */
static int g_first_stub = 0, g_show_stub = 0, g_bright_stub = 1;
int ui_first_get(void) { return g_first_stub; }
void ui_first_set(int v) { g_first_stub = v ? 1 : 0; }
int ui_show_get(void) { return g_show_stub; }
void ui_show_set(int v) { g_show_stub = v ? 1 : 0; }
int ui_bright_get(void) { return g_bright_stub; }
void ui_bright_set(int v) { g_bright_stub = (v >= 0 && v <= 2) ? v : 1; }
void ui_bright_save(void) {}
void ui_bright_load(void) {}
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
static int g_rule_dark_stub = 1; /* ★ AUD-R-002/003：暗棋开关桩（默认开） */
int ui_rule_dark_get(void) { return g_rule_dark_stub; }
void ui_rule_dark_set(int v) { g_rule_dark_stub = v ? 1 : 0; }
int ui_rule_cfg_get(int which)
{
    /* AUD-1000 B07-040：case 3（吃军旗开关）补全 + case 4 走暗棋桩变量
       （旧：case 3 走 default 返 g_rf 碰巧正确；case 4 硬编码 1 不可改） */
    switch (which) {
      case 0: return g_rd;
      case 1: return g_rh;
      case 2: return g_rm;
      case 3: return g_rf;            /* 吃军旗开关（ui_rule_flag_get/set 同域） */
      case 4: return ui_rule_dark_get(); /* 暗棋开关（可 ui_rule_dark_set 改） */
      default: return 1;   /* 与 ui_state.c 真实实现一致（未知开关默认开） */
    }
}
void ui_rules_load(void) {}
void ui_rules_save(void) {}
/* ★ P2（datah_007 证据）：图像设置 4 开关桩（桌面 headless 无持久化，默认值供
   一致性/evverify 链接；真机走 ui_state.c 真实实现）。
   诚实边界：图像/夜间模式渲染参数未实现，开关仅 UI 状态记录。 */
static int g_it = 1, g_ip = 1, g_in = 0, g_io = 1;
int ui_imgset_trans_get(void) { return g_it; }
void ui_imgset_trans_set(int v) { g_it = v ? 1 : 0; }
int ui_imgset_piece_get(void) { return g_ip; }
void ui_imgset_piece_set(int v) { g_ip = v ? 1 : 0; }
int ui_imgset_night_get(void) { return g_in; }
void ui_imgset_night_set(int v) { g_in = v ? 1 : 0; }
int ui_imgset_opp_get(void) { return g_io; }
void ui_imgset_opp_set(int v) { g_io = v ? 1 : 0; }
int ui_imgset_cfg_get(int which)
{
    switch (which) { case 0: return g_it; case 1: return g_ip; case 2: return g_in; default: return g_io; }
}
void ui_imgset_load(void) {}
void ui_imgset_save(void) {}

/* ★ P1（datah_016/017/020）：布局存档 API 桩（桌面 headless：evverify 直接调真实
   save.c/jq_load_layout，不依赖此桩；此桩仅保证一致性构建链接完整）。 */
static char lay_dir_stub[320] = "./layouts";
void ui_laydir_set(const char *d) { if (d) snprintf(lay_dir_stub, sizeof lay_dir_stub, "%s", d); }
const char *ui_laydir_get(void) { return lay_dir_stub; }
int ui_layout_list(void) { return 0; }
int ui_layout_count(void) { return 0; }
const char *ui_layout_name(int i) { (void)i; return 0; }
int ui_layout_selected(void) { return 0; }
void ui_layout_select(int i) { (void)i; }
int ui_layout_selected_valid(void) { return 0; }
int ui_layout_save(const uint8_t l[30], const char *n) { (void)l; (void)n; return -1; }
int ui_layout_load(const char *n, uint8_t o[30]) { (void)n; (void)o; return -1; }
int ui_layout_delete(const char *n) { (void)n; return -1; }
int ui_layout_rename(const char *a, const char *b) { (void)a; (void)b; return -1; }

/* ★ P1（datah_039/107）：复盘回放桩（桌面 headless：evverify 直接调 replay.c 真实
   实现校验回放确定性；此桩仅保证一致性/桌面构建链接完整，默认行为不变）。 */
void ui_replay_init(void) {}
void ui_replay_record(int f, int t, int is_flip) { (void)f; (void)t; (void)is_flip; }
int  ui_replay_count(void) { return 0; }
int  ui_replay_cur(void) { return 0; }
int  ui_replay_rewind(int cur) { (void)cur; return 0; }

/* ★ P2（datah_037/104/061）：ui_opt 难度/模式 桩（桌面 headless 无 GL，默认中级/暗棋
   供一致性/evverify 链接；真机走 ui_state.c 真实实现）。 */
static int g_opt_level_stub = 2;   /* 默认中级区 */
static int g_opt_mode_stub = 1;    /* 默认暗棋（对齐原版） */
void ui_opt_select(int level, int mode)
{ if (level >= 1 && level <= 4) g_opt_level_stub = level; g_opt_mode_stub = (mode == 1) ? 1 : 0; }
int ui_opt_get_level(void) { return g_opt_level_stub; }
int ui_opt_get_mode(void) { return g_opt_mode_stub; }
