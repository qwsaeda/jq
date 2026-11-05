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
#include <stdio.h>
#include <string.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "junqi_rs", __VA_ARGS__)

static int g_ui = UI_MENU;
static pthread_mutex_t g_ui_lock = PTHREAD_MUTEX_INITIALIZER;
/* ★ 选项 UI：人机难度弹窗选中值（level 1-4 / mode 0=布局明棋 1=翻棋暗棋） */
static int g_opt_level = 2;   /* 默认中级区（datah_041 中间档） */
static int g_opt_mode = 1;    /* ★ 原版开局对齐：人机/两人均全背面暗棋（orig_human/orig_two3 截图自证）→ 默认暗棋（mode=1） */

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

/* ★ 对抗审计（datah_067）：蓝牙联机模式标志 */
static int g_bt_active = 0;
void ui_bt_set_active(int on) { pthread_mutex_lock(&g_ui_lock); g_bt_active = on; pthread_mutex_unlock(&g_ui_lock); }
int ui_bt_active(void) { return g_bt_active; }

/* ★ 对抗审计（datah_001）：声音设置（静音/按键声音/音量 1-3），存文件持久化 */
static int g_snd_mute = 0;    /* 0=声音开 1=静音 */
static int g_btn_sound = 1;   /* 0=关 1=开 */
static int g_volume = 2;      /* 1=低 2=中 3=高 */
static char g_set_dir[320] = {0};
int ui_snd_mute_get(void) { return g_snd_mute; }
void ui_snd_mute_set(int m) { g_snd_mute = m ? 1 : 0; }
int ui_btn_sound_get(void) { return g_btn_sound; }
void ui_btn_sound_set(int v) { g_btn_sound = v ? 1 : 0; }
int ui_volume_get(void) { return g_volume; }
void ui_volume_set(int v) { g_volume = (v >= 1 && v <= 3) ? v : 2; }
void ui_settings_set_dir(const char *dir)
{
    pthread_mutex_lock(&g_ui_lock);
    if (dir) { snprintf(g_set_dir, sizeof g_set_dir, "%s", dir); }
    pthread_mutex_unlock(&g_ui_lock);
}
void ui_settings_load(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_settings.txt", g_set_dir);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("声音设置：无文件（%s）→ 默认 开/开/中", pth); return; }
    int m = -1, b = -1, v = -1;
    if (fscanf(f, "%d %d %d", &m, &b, &v) == 3) {
        if (m == 0 || m == 1) g_snd_mute = m;
        if (b == 0 || b == 1) g_btn_sound = b;
        if (v >= 1 && v <= 3) g_volume = v;
    }
    fclose(f);
    LOGI("声音设置：读取 %s → 静音=%d 按键=%d 音量=%d", pth, g_snd_mute, g_btn_sound, g_volume);
}
void ui_settings_save(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_settings.txt", g_set_dir);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("声音设置：写失败 %s", pth); return; }
    fprintf(f, "%d %d %d\n", g_snd_mute, g_btn_sound, g_volume);
    fclose(f);
    LOGI("声音设置：保存 %s → 静音=%d 按键=%d 音量=%d", pth, g_snd_mute, g_btn_sound, g_volume);
}

/* 菜单按钮点击处理：0=人机 → UI_DIFFICULTY（★ 对抗审计 datah_041：原版弹难度选择弹窗，
   选难度+布局/翻棋后开局）；1=两人 → UI_GAME（直开局，原版两人无难度弹窗）；
   4=四国 → 不切 UI_GAME（OnTouch 显示四国边界对话框，勿静默开 2 人局误导）。
   2/3=联网/联机 → 方式选择（datah_072，OnTouch 分流）。 */
void ui_on_menu_action(int action)
{
    ui_menu_action_log(action);
    pthread_mutex_lock(&g_ui_lock);
    if (action == 0) {
        g_ui = UI_DIFFICULTY;
        LOGI("ui_state：菜单 action=%d → UI_DIFFICULTY（人机难度选择弹窗 datah_041）", action);
    } else if (action == 1) {
        g_ui = UI_GAME;
        LOGI("ui_state：菜单 action=%d → UI_GAME（两人直开局）", action);
    }
    pthread_mutex_unlock(&g_ui_lock);
}
