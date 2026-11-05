/*
 * ui_state.c — 菜单/对局 状态机（正向开发 UI 层）
 * 原版主菜单（人机/两人/联网/联机/四国）→ 点"人机/两人"→ 直开局（暗棋原版）；
 * 点"悔棋重开"直接进对局。
 * ★ 原版 UI 对齐（用户指正"没有高手新手"）：人机/两人直开局，无难度弹窗
 *   （datah_041 判误，非人机难度弹窗）。☰ 游戏内菜单（datah_006 8 项）才是选项 UI。
 * 两人/联网/四国 = 占位（日志记录，未实现联机 UI/四国规则，诚实边界）。
 * ★ 难度/暗棋模式为玩法内部参数（ui_opt_get_level/mode 供 AI/开局读），
 *   非弹窗 UI；默认中级区/明棋（原版暗棋差异记录 EXPERIENCE_LOG）。
 */
#include "ui_state.h"
#include "ui_menu.h"
#include <android/log.h>
#include <pthread.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "junqi_rs", __VA_ARGS__)

static int g_ui = UI_MENU;
static pthread_mutex_t g_ui_lock = PTHREAD_MUTEX_INITIALIZER;
/* ★ 选项 UI：人机难度弹窗选中值（level 1-4 / mode 0=布局明棋 1=翻棋暗棋） */
static int g_opt_level = 2;   /* 默认中级区（datah_041 中间档） */
static int g_opt_mode = 0;    /* 默认布局（明棋） */

int ui_screen_get(void) { return g_ui; }
void ui_screen_set(int s) { pthread_mutex_lock(&g_ui_lock); g_ui = s; pthread_mutex_unlock(&g_ui_lock); }
int ui_game_active(void) { return g_ui == UI_GAME; }

/* ★ 选项 UI：选难度/模式（弹窗按钮命中后调，开局前） */
void ui_opt_select(int level, int mode)
{
    pthread_mutex_lock(&g_ui_lock);
    if (level >= 1 && level <= 4) g_opt_level = level;
    g_opt_mode = (mode == 1) ? 1 : 0;
    pthread_mutex_unlock(&g_ui_lock);
}
int ui_opt_get_level(void) { return g_opt_level; }
int ui_opt_get_mode(void) { return g_opt_mode; }

/* 菜单按钮点击处理：0=人机/1=两人 → UI_GAME（★ 原版 UI 对齐：直开局，无难度弹窗）；
   4=重开 → UI_GAME；2/3=占位。
   ★ datah_041（难度弹窗）判误：原版人机/两人直开局（orig_human/orig_two3 截图自证），
   无"高手/新手"难度选择（用户指正）。UI_OPT_PVP 已从人机路径移除。 */
void ui_on_menu_action(int action)
{
    ui_menu_action_log(action);
    pthread_mutex_lock(&g_ui_lock);
    if (action == 0 || action == 1 || action == 4) {
        g_ui = UI_GAME;
        LOGI("ui_state：菜单 action=%d → UI_GAME（直开局）", action);
    }
    pthread_mutex_unlock(&g_ui_lock);
}
