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
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
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

/* ★ P1（datah_012 证据）：游戏设置 4 规则开关（和棋/大本营/吃地雷/吃军旗），
   默认全开（对齐原版默认，行为不变——一致性回归 9/9 不破）。
   持久化 jq_rules.txt（与 ui_settings 同目录范式，4 字段）。 */
static int g_rule_draw = 1;    /* 和棋：1=开（70 步无吃子/总 1000 步判和） */
static int g_rule_hq = 1;      /* 大本营：1=开（进大本营不可再动） */
static int g_rule_mine = 1;    /* 吃地雷：1=开（工兵可挖地雷） */
static int g_rule_flag = 1;    /* 吃军旗：1=开（扛旗判胜） */
int ui_rule_draw_get(void) { return g_rule_draw; }
void ui_rule_draw_set(int v) { g_rule_draw = v ? 1 : 0; }
int ui_rule_hq_get(void) { return g_rule_hq; }
void ui_rule_hq_set(int v) { g_rule_hq = v ? 1 : 0; }
int ui_rule_mine_get(void) { return g_rule_mine; }
void ui_rule_mine_set(int v) { g_rule_mine = v ? 1 : 0; }
int ui_rule_flag_get(void) { return g_rule_flag; }
void ui_rule_flag_set(int v) { g_rule_flag = v ? 1 : 0; }
int ui_rule_cfg_get(int which)
{
    switch (which) {
    case 0: return g_rule_draw;
    case 1: return g_rule_hq;
    case 2: return g_rule_mine;
    case 3: return g_rule_flag;
    }
    return 1;
}
void ui_rules_load(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_rules.txt", g_set_dir);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("游戏设置：无文件（%s）→ 默认全开", pth); return; }
    int d = -1, h = -1, m = -1, fl = -1;
    if (fscanf(f, "%d %d %d %d", &d, &h, &m, &fl) == 4) {
        g_rule_draw = (d == 0 || d == 1) ? d : 1;
        g_rule_hq = (h == 0 || h == 1) ? h : 1;
        g_rule_mine = (m == 0 || m == 1) ? m : 1;
        g_rule_flag = (fl == 0 || fl == 1) ? fl : 1;
    }
    fclose(f);
    LOGI("游戏设置：读取 %s → 和棋=%d 大本营=%d 吃地雷=%d 吃军旗=%d", pth,
         g_rule_draw, g_rule_hq, g_rule_mine, g_rule_flag);
}
void ui_rules_save(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_rules.txt", g_set_dir);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("游戏设置：写失败 %s", pth); return; }
    fprintf(f, "%d %d %d %d\n", g_rule_draw, g_rule_hq, g_rule_mine, g_rule_flag);
    fclose(f);
    LOGI("游戏设置：保存 %s → 和棋=%d 大本营=%d 吃地雷=%d 吃军旗=%d", pth,
         g_rule_draw, g_rule_hq, g_rule_mine, g_rule_flag);
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

/* ★ P1（datah_016/017/020 证据）：布局存档 API——.lay 30 字节格式（对齐原版
   OnEvent layout 读 30 字节范式 / SPAK data/lay/%02d.lay），layouts/ 目录文件 IO。
   简化：目录扫描 readdir（*.lay 排序）+ 存/取/删/改名；无 ListBox 滚动（UI 层简化，
   文本列表 6 条截断 + 小方块占位预览，诚实边界见 AUDIT_LAYOUT_ALIGN §十六）。 */
#define UI_LAY_MAX  64      /* 布局名列表上限 */
#define UI_LAY_NAME 32      /* 单条布局名（无扩展名） */
static char g_lay_dir[320] = {0};
static char g_lay_names[UI_LAY_MAX][UI_LAY_NAME];
static int  g_lay_count = 0;
static int  g_lay_sel = 0;

void ui_laydir_set(const char *dir)
{
    pthread_mutex_lock(&g_ui_lock);
    if (dir) snprintf(g_lay_dir, sizeof g_lay_dir, "%s", dir);
    pthread_mutex_unlock(&g_ui_lock);
}
const char *ui_laydir_get(void)
{
    pthread_mutex_lock(&g_ui_lock);
    if (!g_lay_dir[0]) {
        /* 未 set（桌面）→ 缺省 ./layouts（mkdir 兜底；真机由 jni_bridge OnCreate set /data/local/tmp/layouts） */
        snprintf(g_lay_dir, sizeof g_lay_dir, "./layouts");
        mkdir(g_lay_dir, 0777);   /* 桌面 evverify 用；真机目录 jni_bridge 已 mkdir */
    }
    pthread_mutex_unlock(&g_ui_lock);
    return g_lay_dir;
}

static void lay_path(const char *name, char *out, int out_sz)
{
    snprintf(out, out_sz, "%s/%s.lay", ui_laydir_get(), name);
}

int ui_layout_list(void)
{
    const char *dir = ui_laydir_get();
    DIR *d = opendir(dir);
    pthread_mutex_lock(&g_ui_lock);
    g_lay_count = 0;
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL && g_lay_count < UI_LAY_MAX) {
            size_t n = strlen(e->d_name);
            if (n < 4 || n > 36) continue;              /* *.lay 长度守护（≤31 名 + .lay） */
            if (strcmp(e->d_name + n - 4, ".lay") != 0) continue;
            snprintf(g_lay_names[g_lay_count], UI_LAY_NAME, "%s", e->d_name);
            g_lay_names[g_lay_count][n - 4] = 0;         /* 去 .lay 扩展名 */
            g_lay_count++;
        }
        closedir(d);
    }
    if (g_lay_count >= 2) {                               /* 排序（qsort 交换整条名） */
        for (int i = 0; i < g_lay_count; i++)
            for (int j = i + 1; j < g_lay_count; j++)
                if (strcmp(g_lay_names[i], g_lay_names[j]) > 0) {
                    char tmp[UI_LAY_NAME];
                    memcpy(tmp, g_lay_names[i], UI_LAY_NAME);
                    memcpy(g_lay_names[i], g_lay_names[j], UI_LAY_NAME);
                    memcpy(g_lay_names[j], tmp, UI_LAY_NAME);
                }
    }
    if (g_lay_sel >= g_lay_count) g_lay_sel = 0;           /* clamp 选中项 */
    int n = g_lay_count;
    pthread_mutex_unlock(&g_ui_lock);
    LOGI("布局目录扫描：%s → %d 条%s", dir, n,
         (dir && dir[0] == '.') ? "（桌面）" : "");
    for (int i = 0; i < n && i < 8; i++) LOGI("  布局[%d] %s", i, ui_layout_name(i));
    return n;
}
int ui_layout_count(void) { return g_lay_count; }
const char *ui_layout_name(int i)
{
    if (i < 0 || i >= g_lay_count) return 0;
    return g_lay_names[i];
}
int ui_layout_selected(void) { return g_lay_sel; }
void ui_layout_select(int i)
{
    pthread_mutex_lock(&g_ui_lock);
    g_lay_sel = (i >= 0 && i < g_lay_count) ? i : 0;
    pthread_mutex_unlock(&g_ui_lock);
}
int ui_layout_selected_valid(void) { return g_lay_sel >= 0 && g_lay_sel < g_lay_count; }

int ui_layout_save(const uint8_t lay[30], const char *name)
{
    if (!lay || !name || !name[0]) return -1;
    char pth[352];
    lay_path(name, pth, sizeof pth);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("布局保存：写失败 %s", pth); return -1; }
    int ok = (fwrite(lay, 1, 30, f) == 30);
    fclose(f);
    if (ok) {
        /* 保存后刷新列表 + 指向新条目 */
        ui_layout_list();
        for (int i = 0; i < g_lay_count; i++)
            if (strcmp(g_lay_names[i], name) == 0) { g_lay_sel = i; break; }
    }
    LOGI("布局保存：%s（30 字节 .lay）→ %s", ok ? "成功" : "失败", pth);
    return ok ? 0 : -1;
}

int ui_layout_load(const char *name, uint8_t out[30])
{
    if (!name || !name[0] || !out) return -1;
    char pth[352];
    lay_path(name, pth, sizeof pth);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("布局载入：无文件 %s", pth); return -1; }
    int ok = (fread(out, 1, 30, f) == 30);
    fclose(f);
    if (!ok) { LOGI("布局载入：读不足 30 字节 %s", pth); return -1; }
    LOGI("布局载入：%s（30 字节 .lay）", pth);
    return 0;
}

int ui_layout_delete(const char *name)
{
    if (!name || !name[0]) return -1;
    char pth[352];
    lay_path(name, pth, sizeof pth);
    int rc = remove(pth);
    if (rc == 0) {
        ui_layout_list();   /* 删后刷新（选中项 clamp 回 0） */
        LOGI("布局删除：成功 %s", pth);
    } else {
        LOGI("布局删除：失败 %s", pth);
    }
    return rc;
}

int ui_layout_rename(const char *old, const char *new)
{
    if (!old || !old[0] || !new || !new[0]) return -1;
    char p1[352], p2[352];
    lay_path(old, p1, sizeof p1);
    lay_path(new, p2, sizeof p2);
    int rc = rename(p1, p2);
    if (rc == 0) {
        ui_layout_list();
        for (int i = 0; i < g_lay_count; i++)
            if (strcmp(g_lay_names[i], new) == 0) { g_lay_sel = i; break; }
        LOGI("布局改名：%s → %s", old, new);
    } else LOGI("布局改名：失败 %s → %s", old, new);
    return rc;
}
