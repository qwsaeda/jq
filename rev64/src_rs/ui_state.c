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
#include <stdatomic.h>
#include <sys/stat.h>
#include <sys/types.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "junqi_rs", __VA_ARGS__)

static int g_ui = UI_MENU;
static pthread_mutex_t g_ui_lock = PTHREAD_MUTEX_INITIALIZER;

/* 对局全局状态（桌面构建在 consistency_desktop.c 定义；NDK so 在 jni_bridge.c 定义） */
extern jq_state g_state;
/* ★ 选项 UI：人机难度弹窗选中值（level 1-4 / mode 0=布局明棋 1=翻棋暗棋） */
static int g_opt_level = 2;   /* 默认中级区（datah_041 中间档） */
static int g_opt_mode = 1;    /* ★ 原版开局对齐：人机/两人均全背面暗棋（orig_human/orig_two3 截图自证）→ 默认暗棋（mode=1） */

int ui_screen_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_ui; pthread_mutex_unlock(&g_ui_lock); return v; }
/* ★ AUD-1000 B02-001/002（P0 已修）：getter 与 setter 同锁（旧实现无锁读 g_ui，
   与 ui_screen_set 持锁写构成数据竞争，GLThread/主线程并发可见中间态）。 */
void ui_screen_set(int s) { pthread_mutex_lock(&g_ui_lock); g_ui = s; pthread_mutex_unlock(&g_ui_lock); }
int ui_game_active(void) { pthread_mutex_lock(&g_ui_lock); int v = (g_ui == UI_GAME); pthread_mutex_unlock(&g_ui_lock); return v; }

/* ★ 选项 UI：选难度/模式（弹窗按钮命中后调，开局前） */
void ui_opt_select(int level, int mode)
{
    pthread_mutex_lock(&g_ui_lock);
    if (level >= 1 && level <= 4) g_opt_level = level;
    g_opt_mode = (mode == 1) ? 1 : 0;
    pthread_mutex_unlock(&g_ui_lock);
}
int ui_opt_get_level(void) { pthread_mutex_lock(&g_ui_lock); int v = g_opt_level; pthread_mutex_unlock(&g_ui_lock); return v; }
int ui_opt_get_mode(void) { pthread_mutex_lock(&g_ui_lock); int v = g_opt_mode; pthread_mutex_unlock(&g_ui_lock); return v; }
/* ★ AUD-1000 B03-023（P1 已修）：明暗棋切换单入口（锁内 read-modify-write，
   旧调用点 "ui_opt_get_mode→取反→ui_opt_select" 三步锁外裸改并发丢更新）。 */
int ui_opt_toggle_mode(void)
{
    pthread_mutex_lock(&g_ui_lock);
    g_opt_mode = !g_opt_mode;
    int v = g_opt_mode;
    pthread_mutex_unlock(&g_ui_lock);
    return v;
}

/* ★ 对抗审计（datah_067）：蓝牙联机模式标志 */
static int g_bt_active = 0;
void ui_bt_set_active(int on) { pthread_mutex_lock(&g_ui_lock); g_bt_active = on; pthread_mutex_unlock(&g_ui_lock); }
int ui_bt_active(void) { /* ★ AUD-1000 B02-004（P1 已修，前批）：getter 走 g_ui_lock
    （与 ui_bt_set_active 锁内写同锁，数据竞争闭合） */
    pthread_mutex_lock(&g_ui_lock); int v = g_bt_active; pthread_mutex_unlock(&g_ui_lock); return v; }

/* ★ 对抗审计 P1（批4 P0-R8 / 批6 彻底锁化）：人机模式标志（g_vs_ai）移入 ui_state。
   原为 jni_bridge.c 的 static int g_vs_ai：写点分散在 OnTouch 各弹窗路径（**锁外**
   写），读点在 OnTimer/OnUpdate（**持 g_mutex 内**读）→ 持锁读对无锁写不构成
   同步，编译器可见性无序 → GLThread 可能长期看旧值（该进 AI 时不进 / 该停时不停）。
   先以 volatile 临时兜底（批4），现彻底迁入 ui_state 走 g_ui_lock（与 g_bt_active
   同款），写读双方同一把锁 → 数据竞争根除。 */
static int g_vs_ai = 0;
void ui_vs_ai_set(int on) { pthread_mutex_lock(&g_ui_lock); g_vs_ai = on ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int  ui_vs_ai_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_vs_ai; pthread_mutex_unlock(&g_ui_lock); return v; }

/* ★ 对抗审计（datah_001）：声音设置（静音/按键声音/音量 1-3），存文件持久化 */
static int g_snd_mute = 0;    /* 0=声音开 1=静音 */
static int g_btn_sound = 1;   /* 0=关 1=开 */
static int g_volume = 2;      /* 1=低 2=中 3=高 */
static char g_set_dir[320] = {0};
int ui_snd_mute_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_snd_mute; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_snd_mute_set(int m) { pthread_mutex_lock(&g_ui_lock); g_snd_mute = m ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_btn_sound_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_btn_sound; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_btn_sound_set(int v) { pthread_mutex_lock(&g_ui_lock); g_btn_sound = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_volume_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_volume; pthread_mutex_unlock(&g_ui_lock); return v; }
/* ★ AUD-1000 B02-006（P1 已修）：g_volume setter 补锁（旧实现裸写，与锁外 getter/
   ui_settings_save 锁外读构成数据竞争；同批 B02-006 清单 7 处漏锁点）。 */
void ui_volume_set(int v) { pthread_mutex_lock(&g_ui_lock); g_volume = (v >= 1 && v <= 3) ? v : 2; pthread_mutex_unlock(&g_ui_lock); }
/* ★ P2（datah_061）：新局完整版（完整版补充）先后手/明暗棋 独立循环位。
   ui_first 0=蓝先手 1=红先手（默认 0）；ui_show 0=明 1=暗（默认 1，对齐原版暗棋默认）。
   ★ 诚实边界：先后手=蓝/红 切换简化（仅影响开局侧）。 */
static int g_first = 0;   /* 先后手：0=蓝先手 1=红先手（默认 0） */
static int g_show = 0;    /* 明暗棋：0=明 1=暗（默认 0 明棋，对齐 OnCreate 明棋开局，诚实边界） */
int ui_first_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_first; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_first_set(int v) { pthread_mutex_lock(&g_ui_lock); g_first = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_show_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_show; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_show_set(int v) { pthread_mutex_lock(&g_ui_lock); g_show = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
/* ★ P2（datah_002）：亮度调节档位（0=低 1=中 2=高，默认 1）。
   ★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换，不画真滑条；
   确定=持久化 jq_bright.txt（复用 g_set_dir 范式，单字段）。 */
static int g_bright = 1;  /* 0=低 1=中 2=高（默认 1） */
int ui_bright_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_bright; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_bright_set(int v) { pthread_mutex_lock(&g_ui_lock); g_bright = (v >= 0 && v <= 2) ? v : 1; pthread_mutex_unlock(&g_ui_lock); }
void ui_bright_save(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_bright.txt", g_set_dir);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("亮度调节：写失败 %s", pth); return; }
    fprintf(f, "%d\n", g_bright);
    fclose(f);
    LOGI("亮度调节：保存 %s → 档位=%d（0低/1中/2高）", pth, g_bright);
}
void ui_bright_load(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_bright.txt", g_set_dir);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("亮度调节：无文件（%s）→ 默认中", pth); return; }
    int v = -1;
    if (fscanf(f, "%d", &v) == 1) g_bright = (v >= 0 && v <= 2) ? v : 1;
    fclose(f);
    LOGI("亮度调节：读取 %s → 档位=%d（0低/1中/2高）", pth, g_bright);
}
void ui_settings_set_dir(const char *dir)
{
    pthread_mutex_lock(&g_ui_lock);
    if (dir) { snprintf(g_set_dir, sizeof g_set_dir, "%s", dir); }
    pthread_mutex_unlock(&g_ui_lock);
}
void ui_settings_load(void)
{
    /* ★ AUD-1000 B02-007（P1 已修）：整段走 g_ui_lock（旧实现锁外裸读写
       g_snd_mute/g_btn_sound/g_volume，与 ui_settings_save/OnTouch 锁内写构成竞争）。 */
    if (!g_set_dir[0]) return;
    char pth[320];
    pthread_mutex_lock(&g_ui_lock);
    if (!g_set_dir[0]) { pthread_mutex_unlock(&g_ui_lock); return; }
    snprintf(pth, sizeof pth, "%s/jq_settings.txt", g_set_dir);
    pthread_mutex_unlock(&g_ui_lock);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("声音设置：无文件（%s）→ 默认 开/开/中", pth); return; }
    int m = -1, b = -1, v = -1;
    int om = -1, ob = -1, ov = -1;
    pthread_mutex_lock(&g_ui_lock);
    om = g_snd_mute; ob = g_btn_sound; ov = g_volume;
    if (fscanf(f, "%d %d %d", &m, &b, &v) == 3) {
        if (m == 0 || m == 1) g_snd_mute = m;
        if (b == 0 || b == 1) g_btn_sound = b;
        if (v >= 1 && v <= 3) g_volume = v;
        m = g_snd_mute; b = g_btn_sound; v = g_volume;
    }
    pthread_mutex_unlock(&g_ui_lock);
    fclose(f);
    LOGI("声音设置：读取 %s → 静音=%d 按键=%d 音量=%d", pth,
         m < 0 ? om : m, b < 0 ? ob : b, v < 0 ? ov : v);
}
void ui_settings_save(void)
{
    /* ★ AUD-1000 B02-007（P1 已修）：快照值锁内读，fprintf 锁外（避免持锁做文件 IO）。 */
    if (!g_set_dir[0]) return;
    char pth[320];
    int sm, sb, sv;
    pthread_mutex_lock(&g_ui_lock);
    if (!g_set_dir[0]) { pthread_mutex_unlock(&g_ui_lock); return; }
    snprintf(pth, sizeof pth, "%s/jq_settings.txt", g_set_dir);
    sm = g_snd_mute; sb = g_btn_sound; sv = g_volume;
    pthread_mutex_unlock(&g_ui_lock);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("声音设置：写失败 %s", pth); return; }
    fprintf(f, "%d %d %d\n", sm, sb, sv);
    fclose(f);
    LOGI("声音设置：保存 %s → 静音=%d 按键=%d 音量=%d", pth, sm, sb, sv);
}

/* ★ P1（datah_012 证据）：游戏设置 4 规则开关（和棋/大本营/吃地雷/吃军旗），
   默认全开（对齐原版默认，行为不变——一致性回归 9/9 不破）。
   持久化 jq_rules.txt（与 ui_settings 同目录范式，4 字段）。 */
static int g_rule_draw = 1;    /* 和棋：1=开（70 步无吃子/总 1000 步判和） */
static int g_rule_hq = 1;      /* 大本营：1=开（进大本营不可再动） */
static int g_rule_mine = 1;    /* 吃地雷：1=开（工兵可挖地雷） */
static int g_rule_flag = 1;    /* 吃军旗：1=开（扛旗判胜） */
static int g_rule_dark = 1; /* ★ AUD-R-002/003（P0）：暗棋开关：1=开（ruleflip 规则族生效） */
int ui_rule_draw_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_rule_draw;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_rule_draw_set(int v) { pthread_mutex_lock(&g_ui_lock); g_rule_draw = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_rule_hq_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_rule_hq;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_rule_hq_set(int v) { pthread_mutex_lock(&g_ui_lock); g_rule_hq = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_rule_mine_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_rule_mine;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_rule_mine_set(int v) { pthread_mutex_lock(&g_ui_lock); g_rule_mine = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_rule_flag_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_rule_flag;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_rule_flag_set(int v) { pthread_mutex_lock(&g_ui_lock); g_rule_flag = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_rule_dark_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_rule_dark;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_rule_dark_set(int v) { pthread_mutex_lock(&g_ui_lock); g_rule_dark = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_rule_cfg_get(int which)
{
    /* ★ AUD-1000 B02-010/011（P1 已修）：整函数走 g_ui_lock（B02-011 which=4 暗棋
       分支锁外读 g_rule_dark 随整函数锁化闭合；旧实现 5 开关锁外裸读，
       与 ui_rule_*_set 锁内写构成数据竞争；getter 逐个有锁但 cfg 聚合读漏锁）。 */
    pthread_mutex_lock(&g_ui_lock);
    switch (which) {
    case 0: return g_rule_draw;
    case 1: return g_rule_hq;
    case 2: return g_rule_mine;
    case 3: return g_rule_flag;
    case 4: return g_rule_dark;   /* ★ AUD-R-002/003（P0）：暗棋开关 */
    default: { int v = 1; pthread_mutex_unlock(&g_ui_lock); return v; }
    }
    pthread_mutex_unlock(&g_ui_lock);
}
void ui_rules_load(void)
{
    if (!g_set_dir[0]) return;
    char pth[320];
    /* ★ AUD-1000 B02-010（P1 已修）：写 g_rule_* 段走 g_ui_lock（旧实现锁外裸写 4 开关，
       与 rules.c 锁内读 / ui_rule_*_get 构成竞争）。 */
    pthread_mutex_lock(&g_ui_lock);
    snprintf(pth, sizeof pth, "%s/jq_rules.txt", g_set_dir);
    pthread_mutex_unlock(&g_ui_lock);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("游戏设置：无文件（%s）→ 默认全开", pth); return; }
    int d = -1, h = -1, m = -1, fl = -1;
    pthread_mutex_lock(&g_ui_lock);
    if (fscanf(f, "%d %d %d %d", &d, &h, &m, &fl) == 4) {
        g_rule_draw = (d == 0 || d == 1) ? d : 1;
        g_rule_hq = (h == 0 || h == 1) ? h : 1;
        g_rule_mine = (m == 0 || m == 1) ? m : 1;
        g_rule_flag = (fl == 0 || fl == 1) ? fl : 1;
    }
    int od = g_rule_draw, oh = g_rule_hq, om = g_rule_mine, of = g_rule_flag;
    pthread_mutex_unlock(&g_ui_lock);
    fclose(f);
    LOGI("游戏设置：读取 %s → 和棋=%d 大本营=%d 吃地雷=%d 吃军旗=%d", pth, od, oh, om, of);
}
void ui_rules_save(void)
{
    if (!g_set_dir[0]) return;
    char pth[320];
    int od, oh, om, of;
    /* ★ AUD-1000 B02-010（P1 已修）：快照值锁内读，fprintf 锁外（避免持锁做文件 IO）。 */
    pthread_mutex_lock(&g_ui_lock);
    snprintf(pth, sizeof pth, "%s/jq_rules.txt", g_set_dir);
    od = g_rule_draw; oh = g_rule_hq; om = g_rule_mine; of = g_rule_flag;
    pthread_mutex_unlock(&g_ui_lock);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("游戏设置：写失败 %s", pth); return; }
    fprintf(f, "%d %d %d %d\n", od, oh, om, of);
    fclose(f);
    LOGI("游戏设置：保存 %s → 和棋=%d 大本营=%d 吃地雷=%d 吃军旗=%d", pth, od, oh, om, of);
}

/* ★ P2（datah_007 证据）：图像设置 4 开关（界面动画/棋子动画/夜间模式/对方棋子）。
   ★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
   开关只是 UI 状态记录（不实际改渲染）。持久化 jq_imgset.txt（4 字段，
   与 ui_rules 同目录范式，默认全开）。 */
static int g_img_trans = 1;    /* 界面动画：1=开 */
static int g_img_piece = 1;    /* 棋子动画：1=开 */
static int g_img_night = 0;    /* 夜间模式：0=关（默认） */
static int g_img_opp = 1;      /* 对方棋子：1=开 */
int ui_imgset_trans_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_img_trans;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_imgset_trans_set(int v) { pthread_mutex_lock(&g_ui_lock); g_img_trans = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_imgset_piece_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_img_piece;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_imgset_piece_set(int v) { pthread_mutex_lock(&g_ui_lock); g_img_piece = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_imgset_night_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_img_night;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_imgset_night_set(int v) { pthread_mutex_lock(&g_ui_lock); g_img_night = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_imgset_opp_get(void) { pthread_mutex_lock(&g_ui_lock); int v = g_img_opp;; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_imgset_opp_set(int v) { pthread_mutex_lock(&g_ui_lock); g_img_opp = v ? 1 : 0; pthread_mutex_unlock(&g_ui_lock); }
int ui_imgset_cfg_get(int which)
{
    /* ★ AUD-1000 B02-010（P1 已修，同 ui_rule_cfg_get 范式）：聚合读走 g_ui_lock */
    pthread_mutex_lock(&g_ui_lock);
    switch (which) {
    case 0: return g_img_trans;
    case 1: return g_img_piece;
    case 2: return g_img_night;
    case 3: return g_img_opp;
    default: { int v = 1; pthread_mutex_unlock(&g_ui_lock); return v; }
    }
    pthread_mutex_unlock(&g_ui_lock);
}
void ui_imgset_load(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_imgset.txt", g_set_dir);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("图像设置：无文件（%s）→ 默认 界面开/棋子开/夜间关/对方开", pth); return; }
    int t = -1, p = -1, n = -1, o = -1;
    if (fscanf(f, "%d %d %d %d", &t, &p, &n, &o) == 4) {
        g_img_trans = (t == 0 || t == 1) ? t : 1;
        g_img_piece = (p == 0 || p == 1) ? p : 1;
        g_img_night = (n == 0 || n == 1) ? n : 0;
        g_img_opp = (o == 0 || o == 1) ? o : 1;
    }
    fclose(f);
    LOGI("图像设置：读取 %s → 界面=%d 棋子=%d 夜间=%d 对方=%d", pth,
         g_img_trans, g_img_piece, g_img_night, g_img_opp);
}
void ui_imgset_save(void)
{
    if (!g_set_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_imgset.txt", g_set_dir);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("图像设置：写失败 %s", pth); return; }
    fprintf(f, "%d %d %d %d\n", g_img_trans, g_img_piece, g_img_night, g_img_opp);
    fclose(f);
    LOGI("图像设置：保存 %s → 界面=%d 棋子=%d 夜间=%d 对方=%d", pth,
         g_img_trans, g_img_piece, g_img_night, g_img_opp);
}

/* 菜单按钮点击处理：0=人机 → UI_DIFFICULTY（★ 对抗审计 datah_041：原版弹难度选择弹窗，
   选难度+布局/翻棋后开局）；1=两人 → UI_GAME（直开局，原版两人无难度弹窗）；
   4=四国 → 不切 UI_GAME（OnTouch 显示四国边界对话框，勿静默开 2 人局误导）。
   2/3=联网/联机 → 方式选择（datah_072，OnTouch 分流）。 */
void ui_on_menu_action(int action)
{
    ui_menu_action_log(action);
    int logged = 0;
    pthread_mutex_lock(&g_ui_lock);
    if (action == 0) {
        g_ui = UI_DIFFICULTY;
        logged = 1;
    } else if (action == 1) {
        g_ui = UI_GAME;
        logged = 2;
    }
    pthread_mutex_unlock(&g_ui_lock);
    /* ★ AUD-1000 B02-092（P1 已修）：LOGI 移到锁外（旧实现持 g_ui_lock 内调
       __android_log_print，日志系统锁与 g_ui_lock 嵌套，logcat 慢会放大锁持有时间）。 */
    if (logged == 1) LOGI("ui_state：菜单 action=%d → UI_DIFFICULTY（人机难度选择弹窗 datah_041）", action);
    else if (logged == 2) LOGI("ui_state：菜单 action=%d → UI_GAME（两人直开局）", action);
}

/* ★ P1（datah_039/107 证据）：复盘回放——走子记录（jq_replay）+ 逐 cursor 重放。
   记录存 ui_state（对局生命周期）；jni_bridge 每次走子/翻棋成功后 ui_replay_record，
   开局/重开/布局载入 ui_replay_init。回放 = 初始布局 + 前 cur 步重走（确定性）。
   诚实边界：跳转简化为回开头/跳结尾；菜单简化为退复盘（不建复盘内子菜单）。
   调用方需已持有 g_mutex（jni_bridge OnTouch/OnEvent 锁内；读/写 g_state）。 */
/* ★ AUD-1000 B02-013（P0 已修）：g_replay 无锁，render.c GLThread 锁外调
   ui_replay_count/cur 与 g_mutex 内 ui_replay_record 写构成数据竞争。
   修：g_replay 访问走 g_ui_lock（与 ui_state 其他字段同款）；g_state 读
   （jq_replay_init 拷开局布局）仍由调用方 g_mutex 契约保证（头文件注释
   "调用方需已持有 g_mutex"不变——锁序 g_mutex → g_ui_lock 单向，
   持 g_mutex 调本 API 时再取 g_ui_lock 无逆序）。 */
static jq_replay g_replay;
/* ★ AUD-1000 B02-014（P1 已修）：g_replay_inited 改 _Atomic（旧实现 int，
   replay_ensure_inited 锁内 RMW 与 ui_replay_init 写点跨函数可见性竞争）。 */
static _Atomic int g_replay_inited = 0;

static void replay_ensure_inited(void)
{
    /* ★ AUD-1000 B02-015（P1 已注释）：jq_replay_init 读 g_state（开局 cells 快照）
       无锁——契约依赖调用方持 g_mutex（jni_bridge 走子成功后调用点均锁内；
       桌面 evverify 单线程）。锁序 g_mutex → g_ui_lock 单向，无逆序死锁。 */
    pthread_mutex_lock(&g_ui_lock);
    if (!g_replay_inited) { g_replay_inited = 1; jq_replay_init(&g_replay, &g_state); }
    pthread_mutex_unlock(&g_ui_lock);
}

void ui_replay_init(void)
{
    /* ★ AUD-1000 B02-015（P1 已注释）：g_state 读契约 = 调用方持 g_mutex（头注释
       "调用方需已持有 g_mutex"不变）；g_replay 写走 g_ui_lock。 */
    pthread_mutex_lock(&g_ui_lock);
    jq_replay_init(&g_replay, &g_state);
    g_replay_inited = 1;
    pthread_mutex_unlock(&g_ui_lock);
    LOGI("复盘回放：初始化（清记录 + 开局 30 字节布局提取，n=0 cur=0）");
}
void ui_replay_record(int from, int to, int is_flip)
{
    replay_ensure_inited();
    pthread_mutex_lock(&g_ui_lock);
    jq_replay_record(&g_replay, from, to, is_flip);
    pthread_mutex_unlock(&g_ui_lock);
    LOGI("复盘记录：+%s from=%d to=%d（n=%d cur=%d）",
         is_flip ? "翻棋" : "走子", from, to, jq_replay_count(&g_replay), jq_replay_cur_get(&g_replay));
}
int  ui_replay_count(void) { replay_ensure_inited(); pthread_mutex_lock(&g_ui_lock); int v = jq_replay_count(&g_replay); pthread_mutex_unlock(&g_ui_lock); return v; }
int  ui_replay_cur(void)   { replay_ensure_inited(); pthread_mutex_lock(&g_ui_lock); int v = jq_replay_cur_get(&g_replay); pthread_mutex_unlock(&g_ui_lock); return v; }
int  ui_replay_rewind(int cur)
{
    /* ★ AUD-1000 B02-026（P0 已修）：rewind 改 g_state + g_replay——g_replay 走
       g_ui_lock；g_state 写须调用方持 g_mutex（头契约不变）。jni_bridge
       OnUpdate UI_REPLAY 路径 :1851 等已持 g_mutex（UI_REPLAY 屏分支锁内），
       UI_GAMEINFO 复盘按钮无锁调用点由 B02-080 单独核查。 */
    replay_ensure_inited();
    int rc;
    pthread_mutex_lock(&g_ui_lock);
    rc = jq_replay_rewind_to(&g_replay, &g_state, cur);
    pthread_mutex_unlock(&g_ui_lock);
    LOGI("复盘回放：重放到 cur=%d/%d %s（g_state 已重建为初始布局+前 cur 步）",
         cur, jq_replay_count(&g_replay), rc == 0 ? "成功" : "越界/不一致");
    return rc;
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
    /* ★ AUD-1000 B02-093（P1 已修）：mkdir 移到锁外（旧实现持 g_ui_lock 内调
       mkdir 系统调用，可能阻塞放大锁持有时间）。 */
    pthread_mutex_lock(&g_ui_lock);
    int need_mkdir = (!g_lay_dir[0]);
    pthread_mutex_unlock(&g_ui_lock);
    if (need_mkdir) mkdir("./layouts", 0777);   /* 桌面 evverify 用；真机目录 jni_bridge 已 mkdir */
    pthread_mutex_lock(&g_ui_lock);
    if (!g_lay_dir[0]) {
        /* 未 set（桌面）→ 缺省 ./layouts（mkdir 兜底；真机由 jni_bridge OnCreate set /data/local/tmp/layouts） */
        snprintf(g_lay_dir, sizeof g_lay_dir, "./layouts");
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
    /* ★ AUD-1000 B02-094（P1 已修）：目录扫描锁外读进局部数组（旧实现持
       g_ui_lock 内做 opendir/readdir/closedir 文件系统 IO，目录锁慢会卡
       g_lay_count/name 读者；先读目录到局部再锁内更新）。 */
    DIR *d = opendir(dir);
    char names_local[UI_LAY_MAX][UI_LAY_NAME];
    int n_local = 0;
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL && n_local < UI_LAY_MAX) {
            size_t n = strlen(e->d_name);
            if (n < 4 || n > 35) continue;  /* B02-023/024：31 名+.lay=35 字节守护（n>35 时 names_local[n-4] 越界写 UI_LAY_NAME=32） */
            if (strcmp(e->d_name + n - 4, ".lay") != 0) continue;
            snprintf(names_local[n_local], UI_LAY_NAME, "%s", e->d_name);
            names_local[n_local][n - 4] = 0;                   /* 去 .lay 扩展名 */
            n_local++;
        }
        closedir(d);
    }
    pthread_mutex_lock(&g_ui_lock);
    g_lay_count = 0;
    for (int i = 0; i < n_local; i++)
        memcpy(g_lay_names[g_lay_count++], names_local[i], UI_LAY_NAME);
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
    for (int i = 0; i < n && i < 8; i++) {
        /* ★ AUD-1000 B02-095（P1 已修）：日志循环移入锁内快照（旧实现解锁后调
           ui_layout_name 锁外读 g_lay_names，与并发 ui_layout_list 锁内写竞争）。 */
        char nm[UI_LAY_NAME];
        pthread_mutex_lock(&g_ui_lock);
        if (i >= 0 && i < g_lay_count)
            memcpy(nm, g_lay_names[i], UI_LAY_NAME);
        else nm[0] = 0;
        pthread_mutex_unlock(&g_ui_lock);
        LOGI("  布局[%d] %s", i, nm);
    }
    return n;
}
int ui_layout_count(void) { /* ★ AUD-1000 B02-016（P1 已修，前批）：getter 走 g_ui_lock
    （与 ui_layout_list 锁内写同锁） */
    pthread_mutex_lock(&g_ui_lock); int v = g_lay_count; pthread_mutex_unlock(&g_ui_lock); return v; }
const char *ui_layout_name(int i)
{
    /* ★ AUD-1000 B02-017（P1 已修）：整个 read-check-return 走 g_ui_lock
       （旧实现锁外读 g_lay_names+g_lay_count，与 ui_layout_list 锁内写竞争；
       返回值指向静态数组，锁只保护索引校验段，数组本体跨调用共享语义不变）。 */
    pthread_mutex_lock(&g_ui_lock);
    if (i < 0 || i >= g_lay_count) { pthread_mutex_unlock(&g_ui_lock); return 0; }
    pthread_mutex_unlock(&g_ui_lock);
    return g_lay_names[i];
}
int ui_layout_selected(void) { /* ★ AUD-1000 B02-018（P1 已修，前批）：getter 走 g_ui_lock
    （与 ui_layout_select 锁内写同锁） */
    pthread_mutex_lock(&g_ui_lock); int v = g_lay_sel; pthread_mutex_unlock(&g_ui_lock); return v; }
void ui_layout_select(int i)
{
    pthread_mutex_lock(&g_ui_lock);
    g_lay_sel = (i >= 0 && i < g_lay_count) ? i : 0;
    pthread_mutex_unlock(&g_ui_lock);
}
int ui_layout_selected_valid(void) { pthread_mutex_lock(&g_ui_lock); int v = (g_lay_sel >= 0 && g_lay_sel < g_lay_count); pthread_mutex_unlock(&g_ui_lock); return v; }

/* ★ AUD-1000 B02-031/032/033/034（P0 已修）：布局名白名单谓词——旧实现
   name 直接拼路径，含 '/'/'..' 可路径注入（../ 越目录写/读/删 layouts 外
   文件）。白名单：[A-Za-z0-9_-] 1-31 字符（.lay 扩展名由 lay_path 追加，
   名体本身不带扩展；31 = UI_LAY_NAME 32 - 1 终止符）。 */
static int lay_name_valid(const char *name)
{
    if (!name || !name[0]) return 0;
    int n = (int)strlen(name);
    if (n > 31) return 0;
    for (int i = 0; i < n; i++) {
        char c = name[i];
        if (!(((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '-')))
            return 0;
    }
    return 1;
}

int ui_layout_save(const uint8_t lay[30], const char *name)
{
    if (!lay || !name || !name[0]) return -1;
    if (!lay_name_valid(name)) { LOGI("布局保存：名称非法 %s（白名单 [A-Za-z0-9_-]）", name); return -1; }
    char pth[352];
    lay_path(name, pth, sizeof pth);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("布局保存：写失败 %s", pth); return -1; }
    int ok = (fwrite(lay, 1, 30, f) == 30);
    fclose(f);
    if (ok) {
        /* 保存后刷新列表 + 指向新条目 */
        ui_layout_list();
        /* ★ AUD-1000 B02-020/100（P1 已修）：g_lay_sel 更新走 g_ui_lock（旧实现锁外
           循环读 g_lay_names + 写 g_lay_sel，与 ui_layout_list 锁内写竞争）。 */
        pthread_mutex_lock(&g_ui_lock);
        for (int i = 0; i < g_lay_count; i++)
            if (strcmp(g_lay_names[i], name) == 0) { g_lay_sel = i; break; }
        pthread_mutex_unlock(&g_ui_lock);
    }
    LOGI("布局保存：%s（30 字节 .lay）→ %s", ok ? "成功" : "失败", pth);
    return ok ? 0 : -1;
}

int ui_layout_load(const char *name, uint8_t out[30])
{
    if (!name || !name[0] || !out) return -1;
    if (!lay_name_valid(name)) { LOGI("布局载入：名称非法 %s（白名单 [A-Za-z0-9_-]）", name); return -1; }
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
    if (!lay_name_valid(name)) { LOGI("布局删除：名称非法 %s（白名单 [A-Za-z0-9_-]）", name); return -1; }
    char pth[352];
    lay_path(name, pth, sizeof pth);
    int rc = remove(pth);
    if (rc == 0) {
        ui_layout_list();   /* 删后刷新（选中项 clamp 回 0）
           ★ AUD-1000 B02-021（P1 已修）：remove 与 ui_layout_list 的目录扫描
           各自锁外/锁内分段，删除后刷新覆盖旧条目，无需同锁包裹（列表刷新
           本身走 B02-094 局部快照锁内更新，无竞态残留）。 */
        LOGI("布局删除：成功 %s", pth);
    } else {
        LOGI("布局删除：失败 %s", pth);
    }
    return rc;
}

int ui_layout_rename(const char *old, const char *new)
{
    if (!old || !old[0] || !new || !new[0]) return -1;
    if (!lay_name_valid(old) || !lay_name_valid(new)) { LOGI("布局改名：名称非法（白名单 [A-Za-z0-9_-]）"); return -1; }
    char p1[352], p2[352];
    lay_path(old, p1, sizeof p1);
    lay_path(new, p2, sizeof p2);
    int rc = rename(p1, p2);
    if (rc == 0) {
        ui_layout_list();
        /* ★ AUD-1000 B02-021（P1 已修）：g_lay_sel 更新走 g_ui_lock（同 ui_layout_save 范式） */
        pthread_mutex_lock(&g_ui_lock);
        for (int i = 0; i < g_lay_count; i++)
            if (strcmp(g_lay_names[i], new) == 0) { g_lay_sel = i; break; }
        pthread_mutex_unlock(&g_ui_lock);
        LOGI("布局改名：%s → %s", old, new);
    } else LOGI("布局改名：失败 %s → %s", old, new);
    return rc;
}
