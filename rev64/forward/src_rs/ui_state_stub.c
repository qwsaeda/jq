/* ui_state_stub.c — 桌面 RS_NO_GL 桩（UI 菜单状态机；桌面 headless 无 GL，
   ui_screen_get 默认 UI_GAME 等价：对局逻辑正常跑，菜单渲染无意义。
   ★ AUD-1000 B08-033（P1 已修）：ui_screen 可切（ui_screen_set 真更新 g_ui，
   非恒 UI_GAME）——菜单/弹窗态用例显式 ui_screen_set(UI_MENU/UI_LOGIN/…)
   后与真机 ui_state.c 同行为；恒 UI_GAME 仅对局态用例默认态。 */
/* ★ AUD-1000 B02-003/004/006/010/011（P1 已注释，桩无需加锁）：桌面桩
   （desk/evverify/一致性构建链 ui_state_stub.c，单线程 headless 无并发）的
   getter/setter 无锁为**已知桩简化**——真机 so 链路（Makefile so 目标链
   ui_state.c）对应读写全部走 g_ui_lock（前批 31 处加锁，ui_state.c 内 grep
   确认）。桩与真机行为一致性由一致性桌面回归（make consistency 双 so 对比）
   覆盖，桩本身不引入数据竞争（无并发线程），非缺陷。 */
#include "ui_state.h"
#include <stdio.h>
/* ★ AUD-1000 B08-033（P1 已修）：ui_screen 改可切（g_ui 初值 UI_GAME 保持对局态
   用例行为不变；菜单态用例显式 ui_screen_set(UI_MENU) 后命中逻辑与真机
   ui_state.c 同行为——旧桩恒 UI_GAME 使 OnTouch menu 态分支桌面永不进，
   桌面"主菜单点人机→难度弹窗"链断言与真机分叉）。 */
static int g_ui = UI_GAME;
static int g_bt_act = 0;
int ui_screen_get(void) { return g_ui; }
void ui_screen_set(int s) { g_ui = s; }
int ui_game_active(void) { return g_ui == UI_GAME; }
void ui_on_menu_action(int action) { (void)action; }
void ui_bt_set_active(int on) { g_bt_act = on; }
/* ★ AUD-1000 批次：ui_vs_ai 桌面桩（g_vs_ai 锁化迁入 ui_state 后桩同步，
   桌面单线程无并发，裸写即可） */
static int g_vs_ai_stub = 0;
void ui_vs_ai_set(int on) { g_vs_ai_stub = on ? 1 : 0; }
int  ui_vs_ai_get(void) { return g_vs_ai_stub; }

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
       （旧：case 3 走 default 返 g_rf 碰巧正确；case 4 硬编码 1 不可改）。
       ★ AUD-1000 B01-020（P3 注释）：旧硬编码 1 已随 B07-040 修为走
       ui_rule_dark_get()（可 ui_rule_dark_set 改），桌面桩可关闭暗棋规则族。
       evverify 暗棋用例可验证"暗棋开关关→明棋语义"路径。
       ★ AUD-1000 B02-060（P1 已注释）：which=3 语义 = 吃军旗开关（桩变量 g_rf
       即 ui_rule_flag_get/set 同域），与 ui_state.c 真实 which=3 返回
       g_rule_flag 对齐——变量名 g_rf 历史命名（flag），非"大本营"。 */
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
   save.c/jq_load_layout，不依赖此桩；此桩仅保证一致性构建链接完整）。
   ★ AUD-1000 B02-062（P1 诚实标注）：桩布局 API 恒返 0/-1/NULL——桌面 jni_bridge
   构建 UI_LOAD_LAY 弹窗永远走取消分支；桌面 evverify 链接真实 save.c/ui_state.c
   侧路径不经过此桩（Makefile 分目标：desk 构建链桩，真机 so 链 ui_state.c），
   桩几何/行为与真机分叉为已知诚实边界，不包装成"已实现"。 */
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
