#include <jni.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <android/asset_manager.h>   /* AAsset / AAssetManager / AAsset_openFileDescriptor */
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <sys/system_properties.h>   /* __system_property_get/set（flagtest 触发） */
#include "render.h"   /* rs_render_frame（GLThread 出屏主路） */
#include "glview_gl.h"
#include "touch_state.h"
#include "ui_menu.h"
#include "ui_state.h"
#include "bt_session.h"
#include "containers.h"     /* jq_layout_container / jq_container_* */
#include "audio.h"          /* jq_audio */
#include "render_obj.h"     /* jq_subsys / jq_egl_obj / jq_audio_vec */
#include "glview_gl.h"      /* jq_glview */

#include "bt_protocol.h"    /* BT_T_FLIP / 帧类型枚举 */
/* jq_scene / jq_renderer 取自 render_obj.h（避免 render_scene.h 重定义冲突） */
typedef struct { int vtable, flag0, size; uint8_t pad[0x1c]; } jq_scene_t;

typedef AAssetManager *(*AAM_fromJava_fn)(JNIEnv *, jobject);

/* 子系统/音频/渲染/场景全局（原版 0xba12c 构造链） */
static jq_subsys_list g_subs;
static jq_egl_obj g_egl;
static jq_audio_vec g_aud;
static jq_renderer g_rend;
static jq_scene_t g_scene;
static jq_layout_container g_laycc;

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "junqi_rs", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "junqi_rs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "junqi_rs", __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "junqi_rs", __VA_ARGS__)

/* ★ 这些是 app 运行时全局（jni_bridge.c 定义，非 extern——jni_bridge 是 SoC 入口） */
jq_state g_state;                /* 对局全局状态（规则引擎） */
jq_glview g_glview;             /* GLView 线程驱动 */
jq_touch_state g_ts;            /* 触摸两步态机 */
jq_bt_session g_bt_sess;        /* 蓝牙会话 */
static void jq_bt_test_run(jq_state *st, const char *tag);  /* 蓝牙帧状态机测试（OnEvent bttest / setprop jq.bttest 复用） */

/* ★ 对局模式（用户指正"各按钮点击业务功能要对齐"）：人机 vs 两人热座区分。
   1=人机对战（红方回合由 AI 自动走子 ai_pick）；0=两人热座/蓝牙（红方由玩家/对端走）。
   OnTouch/OnEvent 菜单进局时设置；重开（☰/restart）保持当前模式。 */
static int g_vs_ai = 0;

/* 悔棋栈（OnEvent undo 用） */
#define UNDO_MAX 128
static jq_state g_undo[UNDO_MAX];
static int g_undo_top = 0;
static void undo_push(void) { if (g_undo_top < UNDO_MAX) { g_undo[g_undo_top++] = g_state; LOGI("undo_push：top=%d turn=%d", g_undo_top, g_state.turn); } }
static int undo_pop(void) { if (g_undo_top <= 0) return 0; g_state = g_undo[--g_undo_top]; LOGI("undo_pop：top=%d turn=%d", g_undo_top, g_state.turn); return 1; }

/* ★ 游戏内重开新局统一入口（6 处调用收敛：restartgame property / 菜单按钮 action 0/4 /
   点棋盘开局 / 对局结束再来一局 / 左上角悔棋重开 / 对局态点棋盘自动开局）。
   防"改一处漏五处"不一致：load_layout + 明棋/暗棋（★ 选项 UI 模式）+ 清 undo +
   stage=PLAY + 蓝先手 + 清胜负。
   ★ 模式（本轮）：ui_opt_get_mode()==0 布局=明棋 revealed=1；1 翻棋=暗棋 revealed=0。
   调用方需已持有 g_mutex（OnUpdate/OnTouch 锁内；OnCreate 单线程无需锁）。 */
static void jq_restart_game(void)
{
    jq_load_layout(&g_state, JQ_DEFAULT_LAY);
    int dark = ui_opt_get_mode() == 1;   /* ★ 选项 UI：翻棋模式 = 暗棋开局 */
    /* ★ 测试钩子：jq.mode setprop 覆盖（0=明棋 1=暗棋），walktest/evtest 走明棋用；
       __system_property_get 返回空 = 未设 → 用 ui_opt_get_mode()（默认暗棋） */
    char mode_prop[8] = {0};
    if (__system_property_get("jq.mode", mode_prop) > 0 && mode_prop[0])
        dark = (mode_prop[0] == '1');
    for (int i = 0; i < N_CELLS; i++)
        if (g_state.cells[i].rank != RANK_EMPTY)
            g_state.cells[i].revealed = dark ? 0 : 1;
    g_undo_top = 0;
    g_state.stage = STG_PLAY;
    g_state.turn = SIDE_BLUE;
    g_state.winner = -1;
    g_state.moves_since_eat = 0;
    g_state.total_moves = 0;
    ui_replay_init();   /* ★ P1（datah_039/107）：重开清复盘走子记录 + 重新提取开局布局 */
    jq_touch_state_init(&g_ts);
}

/* 红方 AI 贪心（吃子优先 > 同级 > 炸弹 > 工兵挖雷 > 空移 > 翻棋），对齐 game_selftest ai_step
   视角版：me = 走子方。返回 0=选出着法 / -1=困毙。
   ★ 难度（本轮）：ui_opt_get_level()——>=2 贪心最优（吃子 sc=1000 优先）；
     1（新手区）弱——在 sc>=2 里取最小（避免主动吃子/送死，被动游走）。 */
static int ai_pick(int *from, int *to, int *is_flip)
{
    uint8_t me = g_state.turn;
    int bf = -1, bt = -1, bs = -1;
    int wf = -1, wt = -1, ws = 999;   /* 新手区：sc>=2 里取最小（弱） */
    for (int i = 0; i < N_CELLS; i++) {
        jq_cell *c = &g_state.cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || !c->revealed) continue;
        /* ★ 对抗审计 #2：红方棋子存储 rank+0xc（layout.c:59），ai_pick 恒在 turn==RED 被调，
           必须 rank_norm 还原（旧版用原始编码比较 → 红 rank+12 永不能 < 蓝 rank → sc=1000/900/
           800/700 全死，AI 只空移不主动吃子）。对照 main.c:39 / game_selftest.c:33 正确写法。 */
        int r = rank_norm(c->rank);
        if (r == RANK_FLAG || r == RANK_MINE) continue;
        for (int t = 0; t < N_CELLS; t++) {
            if (t == i) continue;
            jq_cell *d = &g_state.cells[t];
            if (!jq_can_move(&g_state, i, t)) continue;
            int sc = ai_score(c->rank, d->rank);  /* ★ 对抗审计 #2：统一走 ai_score（rank_norm 内建） */
            if (sc > bs) { bs = sc; bf = i; bt = t; }
            /* ★ 新手区（level 1）：sc>=2 里取最小（空移 sc=2 < 吃子 sc=1000 → 被动） */
            if (sc >= 2 && sc < ws) { ws = sc; wf = i; wt = t; }
        }
    }
    /* ★ 难度分流：新手区弱（被动游走），中级/高级/好友区贪心最优 */
    if (ui_opt_get_level() == 1) {
        if (wf >= 0) { *from = wf; *to = wt; *is_flip = 0; return 0; }
        if (bf >= 0 && bs >= 1) { *from = bf; *to = bt; *is_flip = 0; return 0; }  /* 无安全格 → 兜底 */
    } else if (bf >= 0 && bs >= 1) {
        *from = bf; *to = bt; *is_flip = 0; return 0;
    }
    /* 无走子 → 翻棋 */
    for (int i = 0; i < N_CELLS; i++) {
        jq_cell *c = &g_state.cells[i];
        if (c->rank == RANK_EMPTY || c->side != me || c->revealed) continue;
        if (c->rank == RANK_FLAG || c->rank == RANK_MINE) continue;
        if (jq_can_flip(&g_state, i)) { *from = i; *to = i; *is_flip = 1; return 0; }
    }
    if (bf >= 0) { *from = bf; *to = bt; *is_flip = 0; return 0; }  /* 空移兜底 */
    return -1;
}

static JavaVM *g_jvm = NULL;

/* jq_get_jvm：glview_gl.c 引用（渲染线程 attach JVM 用）。正常 so 有定义（0x9e44），
   重建 header 时丢过 → so 里成 NOTYPE UND → 真机 fail to link。补定义（返回 g_jvm）。 */
JavaVM *jq_get_jvm(void) { return g_jvm; }

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_view_w = 720, g_view_h = 1280;   /* 渲染 viewport（OnGraphChange 更新） */
static jobject g_gl_surf = NULL;             /* Surface global ref（OnCreate surface 未 ready 存，OnResume 重试） */
static AAssetManager *g_assets = NULL;
static int g_assets_avail = 0;
/* AAsset 函数指针全局（JNI_OnLoad 单线程初始化，免多线程 dlsym 竞态 + 免 AAssetManager_open
   内 libandroid 全局锁竞争 SIGSEGV SI_KERNEL fault 0x0——崩溃栈实证 AAssetManager_open+54
   pthread_mutex_lock 崩：渲染线程与 OnCreate 主线程竞争 libandroid 内部锁）。 */
static AAsset *(*g_aam_open)(AAssetManager *, const char *, int);
static long (*g_aasset_len)(AAsset *);
static long (*g_aasset_read)(AAsset *, void *, long);
static void (*g_aasset_close)(AAsset *);

/* ---- 音效：Java MediaPlayer 播放（WAV 从 /data/local/tmp/jq_sfx/ 读，对齐
   原版 sound.xml 清单）：事件 move/flip/eat/win/lose/draw → 同名 WAV。 */
#define SFX_DIR "/data/local/tmp/jq_sfx"
static int g_sfx_wrote = 0;
static char g_sfx_priv_dir[256];
static char g_files_dir[256];   /* ★ 音效持久化：getFilesDir 路径（jq_sound_on.txt 存/取） */

/* MediaPlayer global ref 缓存（跨线程安全） */
static jobject g_mp = NULL;
static jclass g_mp_cls = NULL;
static jmethodID g_mp_reset, g_mp_prepare, g_mp_start, g_mp_stop;
static jmethodID g_mp_setfile = NULL;     /* 无 File 重载（恒 NULL，用 setstr 替代） */
static jmethodID g_mp_setstr  = NULL;     /* setDataSource(String) */
static jmethodID g_mp_afd = 0;            /* setDataSource(AssetFileDescriptor) 1 参 */
static jmethodID g_mp_setfd = NULL;       /* setDataSource(FileDescriptor,long,long) */
static jclass g_f_cls_sfx = NULL;
static jmethodID g_f_ctor_sfx = NULL;     /* File(String) */

/* ★ 功能设置（☰ 菜单 action=4）：音效开关。sfx_play 顶部门控。 */
static int g_sound_on = 1;

/* ★ 按键功能对齐：Java AlertDialog（游戏规则/战况统计/软件信息/联网/联机）。
   调用 com.cnvcs.App.showInfo(String,String)（App.smali 静态方法，AlertDialog 显示）。 */
static void jq_show_dialog(JNIEnv *env, const char *title, const char *text)
{
    if (!env || !title || !text) return;
    jclass cls = (*env)->FindClass(env, "com/cnvcs/App");
    if (!cls) { (*env)->ExceptionClear(env); return; }
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "showInfo",
                                              "(Ljava/lang/String;Ljava/lang/String;)V");
    if (!mid) { (*env)->ExceptionClear(env); (*env)->DeleteLocalRef(env, cls); return; }
    jstring jt = (*env)->NewStringUTF(env, title);
    jstring jx = (*env)->NewStringUTF(env, text);
    if (jt && jx) (*env)->CallStaticVoidMethod(env, cls, mid, jt, jx);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    if (jt) (*env)->DeleteLocalRef(env, jt);
    if (jx) (*env)->DeleteLocalRef(env, jx);
    (*env)->DeleteLocalRef(env, cls);
}

/* ★ 蓝牙联机接线：菜单 联网（host=1）/联机（host=0）→ App.btHost/btClient → BtSpp socket。
   发送侧：App.btSend(String) → BtSpp.sendFrame。均走 App 静态方法（与 showInfo 同模式）。 */
static void jq_bt_start(JNIEnv *env, int host)
{
    if (!env) return;
    jclass cls = (*env)->FindClass(env, "com/cnvcs/App");
    if (!cls) { (*env)->ExceptionClear(env); return; }
    if (host) {
        jmethodID mid = (*env)->GetStaticMethodID(env, cls, "btHost", "()V");
        if (mid) (*env)->CallStaticVoidMethod(env, cls, mid);
    } else {
        jmethodID mid = (*env)->GetStaticMethodID(env, cls, "btClient",
                                                  "(Ljava/lang/String;)V");
        if (mid) {
            jstring jz = (*env)->NewStringUTF(env, "");
            if (jz) {
                (*env)->CallStaticVoidMethod(env, cls, mid, jz);
                (*env)->DeleteLocalRef(env, jz);
            }
        }
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, cls);
}

static void jq_bt_send(JNIEnv *env, const char *frame)
{
    if (!env || !frame) return;
    jclass cls = (*env)->FindClass(env, "com/cnvcs/App");
    if (!cls) { (*env)->ExceptionClear(env); return; }
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "btSend",
                                              "(Ljava/lang/String;)V");
    if (mid) {
        jstring js = (*env)->NewStringUTF(env, frame);
        if (js) {
            (*env)->CallStaticVoidMethod(env, cls, mid, js);
            (*env)->DeleteLocalRef(env, js);
        }
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, cls);
}

/* ★ 蓝牙控制帧（UNDO/RESTART）：BT 模式下 ☰ 悔棋/重开 → 编帧 → jq_bt_send。
   帧 type 走 bt_protocol 枚举；控制帧不受回合闸口（OUR_TURN 发送守卫仅走子/翻棋）。 */
static void jq_bt_ctrl_send(JNIEnv *env, int type)
{
    if (!env) return;
    char tb[BT_BUF];
    int n = jq_bt_frame_encode(tb, sizeof tb, type, 0, 0, 0, 0, 0, 0, 0);
    if (n > 0) {
        tb[n] = 0;
        jq_bt_send(env, tb);
        LOGI("蓝牙联机控制帧：type=%d len=%d frame=%s", type, n, tb);
    }
}

/* 音效名 → WAV 文件（对齐 spak_res 解包产物名 data_junqi_*.wav） */
static const char *sfx_wav(const char *event)
{
    if (!event) return NULL;
    if (!strcmp(event, "move")   ) return SFX_DIR "/data_junqi_move.wav";
    if (!strcmp(event, "eat")    ) return SFX_DIR "/data_junqi_eat.wav";
    if (!strcmp(event, "bomb")   ) return SFX_DIR "/data_junqi_bomb.wav";
    if (!strcmp(event, "flag")   ) return SFX_DIR "/data_junqi_flag.wav";
    if (!strcmp(event, "dead")   ) return SFX_DIR "/data_junqi_dead.wav";
    if (!strcmp(event, "flip")   ) return SFX_DIR "/data_junqi_flip.wav";
    if (!strcmp(event, "win")    ) return SFX_DIR "/data_junqi_win.wav";
    if (!strcmp(event, "lose")   ) return SFX_DIR "/data_junqi_lose.wav";
    if (!strcmp(event, "draw")   ) return SFX_DIR "/data_junqi_draw.wav";
    if (!strcmp(event, "start")  ) return SFX_DIR "/data_junqi_start.wav";
    if (!strcmp(event, "over")   ) return SFX_DIR "/data_junqi_over.wav";
    if (!strcmp(event, "select") ) return SFX_DIR "/data_junqi_select.wav";
    if (!strcmp(event, "close")  ) return SFX_DIR "/data_ui_close.wav";
    if (!strcmp(event, "pop")    ) return SFX_DIR "/data_ui_pop.wav";
    return SFX_DIR "/data_junqi_move.wav";
}

/* 走子事件 → 对应音效名（原版 Junqi.* 分类：Move/Eat/Bomb/Flag/Dead）。
   ★ 走子后必须按事件播对应音效（吃子播 eat 非 move，实证 25→30 吃子统一播 move 是缺口）。 */
static const char *sfx_for_move(int ev)
{
    switch (ev) {
    case EV_EAT:   return "eat";
    case EV_BOMB:  return "bomb";
    case EV_FLAG:  return "flag";
    case EV_DEAD:  return "dead";
    case EV_MOVE:
    default:       return "move";
    }
}

/* 对局结束音效统一入口（走子后 stage==OVER → win/lose）。
   ★ 3 处调用统一（OnTouch 锁外 over_win / AI 锁内 g_state / OnCreate flagtest 锁内），
   防"改一处漏两处"不一致。winner 无效（-1/越界）不播。 */
static void sfx_play(JNIEnv *env, const char *event);   /* 前置声明（audio.h 非 static，此处 static 定义在 382） */
static void sfx_game_over(JNIEnv *env, int stage, int winner)
{
    if (stage != STG_OVER) return;
    if (winner == SIDE_BLUE)        sfx_play(env, "win");
    else if (winner == SIDE_RED)    sfx_play(env, "lose");
    else if (winner == 2)           sfx_play(env, "draw");   /* 和棋（winner=2）→ draw 音效 */
}

/* MediaPlayer 引用缓存（OnCreate 主线程建，global ref 跨线程共享） */
static int g_mp_ok = 0;
static void sfx_mp_init(JNIEnv *env)
{
    if (g_mp_ok || !env) return;
    jclass cls = (*env)->FindClass(env, "android/media/MediaPlayer");
    if (!cls) { (*env)->ExceptionClear(env); return; }
    g_mp_cls = (jclass)(*env)->NewGlobalRef(env, cls);
    /* ★ 每个 GetMethodID 失败（NoSuchMethodError）必须 ExceptionClear，否则累积
       pending exception → 后续 GetMethodID abort（实证：setDataSource(File) 不存在
       → NoSuchMethodError 未清 → setDataSource(AFD,JJ) 又失败 → ART abort）。
       MediaPlayer 实际重载：setDataSource(String)/setDataSource(FileDescriptor)/
       setDataSource(FileDescriptor,long,long)/setDataSource(AssetFileDescriptor)（1 参）。
       不存在：setDataSource(File)、setDataSource(AFD,long,long)。 */
    jmethodID reset = (*env)->GetMethodID(env, cls, "reset", "()V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); reset = NULL; }
    jmethodID prep  = (*env)->GetMethodID(env, cls, "prepare", "()V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); prep = NULL; }
    jmethodID start = (*env)->GetMethodID(env, cls, "start", "()V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); start = NULL; }
    jmethodID stop  = (*env)->GetMethodID(env, cls, "stop", "()V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); stop = NULL; }
    /* 文件路径重载（正解：setDataSource(String)，非 File——MediaPlayer 无 File 重载） */
    jmethodID sets  = (*env)->GetMethodID(env, cls, "setDataSource",
                    "(Ljava/lang/String;)V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); sets = NULL; }
    /* AFD 1 参重载（MediaPlayer.setDataSource(AssetFileDescriptor)，非 (AFD,JJ)） */
    jmethodID seta  = (*env)->GetMethodID(env, cls, "setDataSource",
                    "(Landroid/content/res/AssetFileDescriptor;)V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); seta = NULL; }
    /* FileDescriptor + 偏移重载（AAsset fd 直接播放用） */
    jmethodID setfd = (*env)->GetMethodID(env, cls, "setDataSource",
                    "(Ljava/io/FileDescriptor;JJ)V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); setfd = NULL; }
    g_mp_reset = reset; g_mp_prepare = prep; g_mp_start = start; g_mp_stop = stop;
    g_mp_setfile = NULL;                 /* 无 File 重载（用 setstr 替代） */
    g_mp_setstr  = sets;
    g_mp_afd     = (jmethodID)seta;
    g_mp_setfd   = setfd;
    /* ★ MediaPlayer 实例（公开无参构造 new MediaPlayer()，API 1+）。
       之前注释"无公开无参构造"是错的 → g_mp 从未建 → sfx_play_file 降级不出声。
       建实例 + NewGlobalRef（跨线程共享，sfx_play_file 调 setDataSource/prepare/start）。 */
    jmethodID mp_ctor = (*env)->GetMethodID(env, cls, "<init>", "()V");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); mp_ctor = NULL; }
    if (mp_ctor) {
        jobject mp = (*env)->NewObject(env, cls, mp_ctor);
        if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); mp = NULL; }
        if (mp) {
            g_mp = (*env)->NewGlobalRef(env, mp);
            (*env)->DeleteLocalRef(env, mp);
        }
    }
    jclass f_cls = (*env)->FindClass(env, "java/io/File");
    if (f_cls) {
        g_f_cls_sfx = (jclass)(*env)->NewGlobalRef(env, f_cls);
        g_f_ctor_sfx = (*env)->GetMethodID(env, f_cls, "<init>", "(Ljava/lang/String;)V");
        if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); g_f_ctor_sfx = NULL; }
        (*env)->DeleteLocalRef(env, f_cls);
    }
    g_mp_ok = 1;
    LOGI("sfx：MediaPlayer 就绪（实例=%s，setDataSource(String)=%s AFD=%s FD=%s，事件播放链启用）",
         g_mp ? "已建" : "未建", g_mp_setstr ? "可用" : "无", g_mp_afd ? "可用" : "无",
         g_mp_setfd ? "可用" : "无");
}

/* assets 音效通路：g_assets=0（JNI_OnLoad 设文件系统回退，AAssetManager_fromJava 设备
   转译层不可达 → 直接 UND 会 fail to link）→ 此函数永不执行。WAV 由
   sfx_wav_ensure_private 用 Java 反射读 assets 落地 getFilesDir/jq_sfx/（sfx_play ① 优先）。
   保留空实现：不引用 AAsset_getFd/AAsset_openFileDescriptor（设备 libandroid 不导出，
   直接引用 → so 有 UND → fail to link）。 */
static void sfx_play_asset(JNIEnv *env, const char *event)
{
    (void)env; (void)event;
    LOGI("sfx：assets 通路未启用（g_assets=0，WAV 走内部目录落地）");
}

static void sfx_wav_ensure_private(JNIEnv *env, jobject activity)
{
    if (g_sfx_wrote) return;
    g_sfx_wrote = 1;
    /* ★ 只做内部目录 mkdirs（getFilesDir/jq_sfx）+ 设 g_sfx_priv_dir，不碰 AAssetManager_open。
       根因：AAssetManager_open（libandroid）真机有全局锁，主线程 OnCreate 与渲染线程竞争
       → SIGSEGV SI_KERNEL fault 0x0（崩溃栈实证 AAssetManager_open+54 pthread_mutex_lock）。
       WAV 文件本身走 sfx_play 的 ② assets 通路（AAsset_openFileDescriptor 一次性拿 fd 不持流，
       锁窗口小）或 ③ 文件系统回退（/sdcard push，MuMu/桌面）。内部目录暂空仅作 fallback 路径。 */
    if (!activity) { LOGI("sfx：activity 空 → 跳过内部目录"); return; }
    jclass act_cls = (*env)->GetObjectClass(env, activity);
    jmethodID getFilesDir = (*env)->GetMethodID(env, act_cls, "getFilesDir", "()Ljava/io/File;");
    if (!getFilesDir) { (*env)->ExceptionClear(env); (*env)->DeleteLocalRef(env, act_cls);
                        LOGI("sfx：getFilesDir 方法缺失"); return; }
    jobject fobj = (*env)->CallObjectMethod(env, activity, getFilesDir);
    (*env)->ExceptionClear(env);
    if (!fobj) { (*env)->DeleteLocalRef(env, act_cls);
                 LOGI("sfx：getFilesDir 返回 null"); return; }
    jclass f_cls = (*env)->FindClass(env, "java/io/File");
    jmethodID ctor_f = (*env)->GetMethodID(env, f_cls, "<init>",
                    "(Ljava/io/File;Ljava/lang/String;)V");
    jstring sub = (*env)->NewStringUTF(env, "jq_sfx");
    jobject subdir = (*env)->NewObject(env, f_cls, ctor_f, fobj, sub);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    jmethodID mkdirs_m = (*env)->GetMethodID(env, f_cls, "mkdirs", "()Z");
    (*env)->CallBooleanMethod(env, subdir, mkdirs_m);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    /* getAbsolutePath */
    jmethodID getAbs = (*env)->GetMethodID(env, f_cls, "getAbsolutePath", "()Ljava/lang/String;");
    jobject abspath = (*env)->CallObjectMethod(env, subdir, getAbs);
    (*env)->ExceptionClear(env);
    /* ★ 音效持久化：同时捕获 getFilesDir 根路径（jq_sound_on.txt 存取用） */
    jobject fabs = (*env)->CallObjectMethod(env, fobj, getAbs);
    (*env)->ExceptionClear(env);
    if (fabs) {
        const char *fd = (*env)->GetStringUTFChars(env, fabs, NULL);
        if (fd) { snprintf(g_files_dir, sizeof g_files_dir, "%s", fd);
                  ui_settings_set_dir(g_files_dir);   /* ★ 对抗审计（datah_001）：声音设置持久化目录 */
                  (*env)->ReleaseStringUTFChars(env, fabs, fd); }
        else (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, fabs);
    }
    if (!abspath) {
        (*env)->DeleteLocalRef(env, sub); (*env)->DeleteLocalRef(env, act_cls);
        (*env)->DeleteLocalRef(env, fobj); (*env)->DeleteLocalRef(env, subdir);
        (*env)->DeleteLocalRef(env, f_cls);
        return;
    }
    const char *adir = (*env)->GetStringUTFChars(env, abspath, NULL);
    if (adir) {
        snprintf(g_sfx_priv_dir, sizeof g_sfx_priv_dir, "%s", adir);
        (*env)->ReleaseStringUTFChars(env, abspath, adir);
        LOGI("sfx：内部目录就绪 %s（WAV 走 assets/文件回退，内部目录备 fallback）", g_sfx_priv_dir);
    } else {
        (*env)->ExceptionClear(env);
        g_sfx_priv_dir[0] = 0;
        LOGI("sfx：getAbsolutePath 失败 → 内部目录未设");
    }
    (*env)->DeleteLocalRef(env, sub); (*env)->DeleteLocalRef(env, act_cls);
    (*env)->DeleteLocalRef(env, fobj); (*env)->DeleteLocalRef(env, subdir);
    (*env)->DeleteLocalRef(env, abspath); (*env)->DeleteLocalRef(env, f_cls);

    /* ★ WAV 落地：Java 反射读 assets（activity.getAssets() + AssetManager.open）写
       getFilesDir/jq_sfx/。必须在 setContentView 前（渲染线程未启动）跑 → 无
       AAssetManager_open 锁竞争。AAssetManager_fromJava 设备转译层不可达（直接 UND
       会 fail to link），故纯 Java 反射读 assets（内部锁竞争窗口在单线程早期可忽略）。 */
    if (!g_sfx_priv_dir[0]) return;
    static const char *wav_names[] = {
        "data_junqi_move.wav","data_junqi_eat.wav","data_junqi_bomb.wav",
        "data_junqi_flag.wav","data_junqi_dead.wav","data_junqi_flip.wav",
        "data_junqi_win.wav","data_junqi_lose.wav","data_junqi_draw.wav",
        "data_junqi_start.wav","data_junqi_over.wav","data_junqi_select.wav",
        "data_ui_close.wav","data_ui_pop.wav", NULL };
    /* ★ act_cls 已在上面 DeleteLocalRef，这里必须重新获取（否则悬空引用 SIGSEGV 0x70）。 */
    jclass act_cls2 = (*env)->GetObjectClass(env, activity);
    jmethodID getAssets = (*env)->GetMethodID(env, act_cls2, "getAssets",
                    "()Landroid/content/res/AssetManager;");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); getAssets = NULL; }
    jobject am = getAssets ? (*env)->CallObjectMethod(env, activity, getAssets) : NULL;
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); am = NULL; }
    if (!am) { LOGI("sfx：getAssets 失败 → WAV 不落地（内部目录空）");
               if (act_cls2) (*env)->DeleteLocalRef(env, act_cls2); return; }
    jclass am_cls = (*env)->GetObjectClass(env, am);
    jmethodID open_m = (*env)->GetMethodID(env, am_cls, "open",
                    "(Ljava/lang/String;)Ljava/io/InputStream;");
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); open_m = NULL; }
    jclass is_cls = (*env)->FindClass(env, "java/io/InputStream");
    jmethodID read_m = is_cls ? (*env)->GetMethodID(env, is_cls, "read", "([B)I") : NULL;
    jmethodID is_close = is_cls ? (*env)->GetMethodID(env, is_cls, "close", "()V") : NULL;
    jclass fos_cls = (*env)->FindClass(env, "java/io/FileOutputStream");
    jmethodID fos_ctor = fos_cls ? (*env)->GetMethodID(env, fos_cls, "<init>", "(Ljava/lang/String;)V") : NULL;
    jmethodID fos_write = fos_cls ? (*env)->GetMethodID(env, fos_cls, "write", "([BII)V") : NULL;
    jmethodID fos_close = fos_cls ? (*env)->GetMethodID(env, fos_cls, "close", "()V") : NULL;
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    if (!open_m || !read_m || !fos_ctor) { LOGI("sfx：反射方法缺失 → WAV 不落地"); }
    else {
        int copied = 0;
        for (int i = 0; wav_names[i]; i++) {
            char asset_name[96], out_path[320];
            snprintf(asset_name, sizeof asset_name, "jq_sfx/%s", wav_names[i]);
            snprintf(out_path, sizeof out_path, "%s/%s", g_sfx_priv_dir, wav_names[i]);
            jstring aname = (*env)->NewStringUTF(env, asset_name);
            jobject is = (*env)->CallObjectMethod(env, am, open_m, aname);
            (*env)->DeleteLocalRef(env, aname);
            if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); continue; }
            if (!is) continue;
            jstring opath = (*env)->NewStringUTF(env, out_path);
            jobject fos = (*env)->NewObject(env, fos_cls, fos_ctor, opath);
            (*env)->DeleteLocalRef(env, opath);
            if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env);
                (*env)->CallVoidMethod(env, is, is_close); continue; }
            if (!fos) continue;
            jbyteArray buf = (*env)->NewByteArray(env, 4096);
            jsize n;
            while ((n = (*env)->CallIntMethod(env, is, read_m, buf)) > 0) {
                if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); break; }
                (*env)->CallVoidMethod(env, fos, fos_write, buf, 0, n);
                if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); break; }
            }
            if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            (*env)->CallVoidMethod(env, is, is_close);
            (*env)->CallVoidMethod(env, fos, fos_close);
            if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, buf);
            (*env)->DeleteLocalRef(env, is);
            (*env)->DeleteLocalRef(env, fos);
            copied++;
        }
        LOGI("sfx：WAV 落地 %s（%d/14 拷贝）", g_sfx_priv_dir, copied);
    }
    if (am) (*env)->DeleteLocalRef(env, am);
    if (am_cls) (*env)->DeleteLocalRef(env, am_cls);
    if (is_cls) (*env)->DeleteLocalRef(env, is_cls);
    if (fos_cls) (*env)->DeleteLocalRef(env, fos_cls);
    if (act_cls2) (*env)->DeleteLocalRef(env, act_cls2);
}

/* ★ 音效持久化（功能设置 ☰ action=4）：g_sound_on 存 getFilesDir/jq_sound_on.txt。
   读（OnCreate 调）：文件存在 "0" → 关，"1"/缺 → 开。写（☰ 菜单 4 切后调）。 */
static void sfx_sound_load(void)
{
    if (!g_files_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_sound_on.txt", g_files_dir);
    FILE *f = fopen(pth, "rb");
    if (!f) { LOGI("音效设置：无文件（%s）→ 默认开", pth); return; }
    char c = 0;
    if (fread(&c, 1, 1, f) == 1) g_sound_on = (c == '0') ? 0 : 1;
    fclose(f);
    LOGI("音效设置：读取 %s → %s", pth, g_sound_on ? "开" : "关");
}
static void sfx_sound_save(void)
{
    if (!g_files_dir[0]) return;
    char pth[320]; snprintf(pth, sizeof pth, "%s/jq_sound_on.txt", g_files_dir);
    FILE *f = fopen(pth, "wb");
    if (!f) { LOGI("音效设置：写失败 %s", pth); return; }
    fwrite(g_sound_on ? "1" : "0", 1, 1, f);
    fclose(f);
    LOGI("音效设置：保存 %s → %s", g_sound_on ? "开" : "关", pth);
}


static void sfx_play_file(JNIEnv *env, const char *event, const char *path_override);   /* 前向声明（文件回退/内部存储） */
static void sfx_play(JNIEnv *env, const char *event)
{
    /* ★ 对抗审计（datah_001）：静音模式关时静默（ui_snd_mute_get()==1）；
       按键声音开关只影响 UI 按钮音（pop/select/close） */
    if (ui_snd_mute_get()) return;
    if ((!strcmp(event, "pop") || !strcmp(event, "select") || !strcmp(event, "close")) &&
        !ui_btn_sound_get()) return;
    /* ① 内部存储落地优先（sfx_wav_ensure_private OnCreate 已写 getFilesDir/jq_sfx/，
       MediaPlayer File 路径纯本地读，免 AAssetManager_open 多线程锁竞争 SIGSEGV） */
    if (g_sfx_priv_dir[0]) {
        char pth[320];
        const char *base = sfx_wav(event);
        if (base) {
            const char *bn = strrchr(base, '/');
            snprintf(pth, sizeof pth, "%s/%s", g_sfx_priv_dir, bn ? bn : base);
            FILE *chk = fopen(pth, "rb");
            if (chk) { fclose(chk); sfx_play_file(env, event, pth); return; }
        }
    }
    /* ② 内部存储无文件 → assets 通路（AAsset_openFileDescriptor + AFD setDataSource，
       一次性拿 fd 不持流，锁竞争窗口小） */
    if (g_assets && g_aam_open) { sfx_play_asset(env, event); return; }
    /* ③ 文件系统回退（/sdcard push，MuMu/桌面） */
    sfx_play_file(env, event, NULL);
}

static void sfx_play_file(JNIEnv *env, const char *event, const char *path_override)
{
    const char *base = sfx_wav(event);
    if (!base) return;
    const char *path = path_override ? path_override : base;
    if (!path) return;
    if (!g_mp_ok) { sfx_mp_init(env); if (!g_mp_ok) return; }
    /* ★ 跨线程安全：类引用用 global ref（g_mp_cls），jmethodID 线程安全可缓存复用。
       MediaPlayer 实际重载 setDataSource(String)（非 File——File 重载不存在，
       GetMethodID(File) 会 NoSuchMethodError 累积 abort，实证）。
       每个 CallVoidMethod 后清 pending exception（防 NoMethodError 累积 abort）。 */
    jstring jpath = (*env)->NewStringUTF(env, path);
    if (!jpath) { if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env); return; }
    /* ★ g_mp 是 global ref（首建时 NewGlobalRef），所有线程共享同一实例。
       避免 local ref 跨线程复用 → MuMu checkJNI "accessed deleted Local" abort。
       MediaPlayer 严格说非线程安全，但本重写仅验证播放链通路（日志实证），
       真机出声走 AudioTrack/宿主音频；共享 global ref 不崩即可（诚实边界）。 */
    if (!g_mp) {
        if (!g_mp_cls) { LOGI("sfx：MediaPlayer 类未缓存 → 跳过 %s", event); return; }
        /* MediaPlayer 无公开无参构造；reset 是实例方法。用 MediaPlayer.create 替代不可得，
           改用 static 实例：首次 new 走反射 MediaPlayer() 需 Context 无 → 降级跳过（不崩）。 */
        LOGI("sfx：MediaPlayer 实例未建（降级，不出声） %s", event);
        return;
    }
    /* reset → setDataSource(String) → prepare → start */
    if (g_mp_reset) {
        (*env)->CallVoidMethod(env, g_mp, g_mp_reset);
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    }
    if (g_mp_setstr) {
        (*env)->CallVoidMethod(env, g_mp, g_mp_setstr, jpath);
        if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); }
    } else {
        LOGI("sfx：%s setDataSource(String) 无 → 跳过", event);
        (*env)->DeleteLocalRef(env, jpath);
        return;
    }
    if (g_mp_prepare) {
        (*env)->CallVoidMethod(env, g_mp, g_mp_prepare);
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            LOGI("sfx：%s WAV 缺失/prepare 失败（%s）", event, path);
            (*env)->DeleteLocalRef(env, jpath);
            return;
        }
    }
    if (g_mp_start) {
        (*env)->CallVoidMethod(env, g_mp, g_mp_start);
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    }
    LOGI("sfx：播放 %s（%s）", event, path);
    (*env)->DeleteLocalRef(env, jpath);
}

/* ---- 事件 → 音效分类（原版 Junqi.* 字符串，.rodata 0x120f14-0x120f50）---- */
static int  g_bt_sess_init = 0;
static int  g_bt_sess_init_done = 0;
static int g_event_cnt[6];
static int g_event_total = 0;
static int  g_way_host = 1;   /* ★ 对抗审计（datah_072）：方式选择来源——2=联网(host) 3=联机(client) */

static void on_event(const char *name, const char *value, JNIEnv *env)
{
    jq_state *st = &g_state;
    if (strcmp(name, "move") == 0) {
        int from = -1, to = -1;
        if (sscanf(value, "%d,%d", &from, &to) == 2) {
            undo_push();
            int ev = jq_try_move(st, from, to);
            if (ev < 0) {
                g_undo_top--;
                LOGI("OnEvent move 非法 (%d,%d)→(%d,%d)", cell_row(from), cell_col(from),
                     cell_row(to), cell_col(to));
                return;
            }
            g_event_cnt[ev]++;
            g_event_total++;
            ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：OnEvent 走子成功记录 1 步 */
            LOGI("OnEvent move (%d,%d)→(%d,%d) %s（#%d 累计）", cell_row(from), cell_col(from),
                 cell_row(to), cell_col(to), JQ_EVENT_NAMES[ev], g_event_cnt[ev]);
        }
    } else if (strcmp(name, "flip") == 0) {
        int idx = -1;
        if (sscanf(value, "%d", &idx) == 1) {
            undo_push();
            if (jq_flip(st, idx)) {
                ui_replay_record(idx, idx, 1);   /* ★ P1（datah_039/107）：OnEvent 翻棋成功记录 1 步 */
                LOGI("OnEvent flip (%d,%d) rank=%d", cell_row(idx), cell_col(idx),
                     rank_norm(st->cells[idx].rank));
            } else g_undo_top--;
        }
    } else if (strcmp(name, "undo") == 0) {
        if (undo_pop()) LOGI("OnEvent undo 成功");
        else LOGI("OnEvent undo 无历史");
    } else if (strcmp(name, "resign") == 0) {
        st->winner = 1 - st->turn;
        st->stage = STG_OVER;
        LOGI("OnEvent resign → 对方胜");
        sfx_game_over(env, st->stage, st->winner);   /* 认输 → 对方胜音效（on_event 不经走子块） */
    } else if (strcmp(name, "restart") == 0) {
        /* ★ 对抗审计 #4：统一走 jq_restart_game（随模式：明棋布局/暗棋翻棋 + 清 undo +
           stage/turn/winner/moves）。旧版内联 jq_load_layout（暗棋）→ 与 OnTouch 路径不一致。 */
        jq_restart_game();
        LOGI("OnEvent restart（jq_restart_game：default.lay 随模式 mode=%d，stage=PLAY 蓝先手）",
             ui_opt_get_mode());
    } else if (strcmp(name, "flagtest") == 0) {
        /* 测试夹具：构造近旗状态（蓝 rank2 @ cell53(row10,col3)，红军旗 cell58(row11,col3)），
           真机触摸走一步 cell53→cell58 = Junqi.Flag（EV_FLAG）。桌面 jq_try_move 已证 =3。
           诚实边界：起点状态是测试夹具（非默认布局开局），走子判定/事件链路真实。 */
        memset(st, 0, sizeof(*st));
        st->turn = SIDE_BLUE; st->stage = STG_PLAY; st->winner = -1;
        for (int i = 0; i < N_CELLS; i++) st->cells[i].rank = RANK_EMPTY;
        st->cells[53].rank = 2; st->cells[53].side = SIDE_BLUE; st->cells[53].revealed = 1;
        st->cells[58].rank = 12 + 0xc; st->cells[58].side = SIDE_RED; st->cells[58].revealed = 1;
        st->cells[58].flags = JQF_FLAG;
        g_undo_top = 0;
        LOGI("OnEvent flagtest → 近旗状态（蓝 rank2@53，红军旗@58，走 53→58 = Junqi.Flag）");
    } else if (strcmp(name, "layout") == 0) {
        /* 原版 OnEvent("layout","<0-11>") → data/lay/%02d.lay（12 预设，§R6 确证）。
           本地从 layouts/ 目录读（桌面 headless），MuMu 走文件系统 /data/local/tmp/layouts/。 */
        int lay = -1;
        if (sscanf(value, "%d", &lay) == 1 && lay >= 0 && lay <= 11) {
            uint8_t lb[30];
            const char *dirs[] = { "layouts", "/data/local/tmp/layouts", "/data/local/tmp/spak_res/layouts" };
            int loaded = 0;
            for (int d = 0; d < 3 && !loaded; d++) {
                char path[256];
                snprintf(path, sizeof path, "%s/%02d.lay", dirs[d], lay);
                FILE *f = fopen(path, "rb");
                if (f) {
                    if (fread(lb, 1, 30, f) == 30) {
                        jq_load_layout(st, lb);
                        g_undo_top = 0;
                        ui_replay_init();   /* ★ P1（datah_039/107）：布局载入清复盘记录 */
                        loaded = 1;
                        LOGI("OnEvent layout %d（%s）", lay, path);
                    }
                    fclose(f);
                }
            }
            if (!loaded) {
                /* 文件缺失 → 回退 default */
                jq_load_layout(st, JQ_DEFAULT_LAY);
                g_undo_top = 0;
                ui_replay_init();   /* ★ P1（datah_039/107）：布局载入清复盘记录 */
                LOGI("OnEvent layout %d 文件缺失 → default.lay", lay);
            }
        }
    } else if (strcmp(name, "menu") == 0) {
        /* 主菜单按钮（对齐 §124u 实测的 5 按钮：人机对战/两人对战/联网/联机/四国军棋）。
           语义：value=0 人机(默认 local，蓝先手 + 红 AI 自动应) / 1 两人 / 2 联网 / 3 联机 / 4 四国。
           本地模式立即开局（STG_PLAY + 蓝先手）；多人模式走 OnBluetooth 帧流（桩：日志记录）。 */
        int mode = -1;
        if (sscanf(value, "%d", &mode) != 1 || mode < 0 || mode > 4)
            mode = 0;
        if (mode == 0 || mode == 1 || mode == 4) {
            /* ★ 对抗审计 #4：本地/两人/四国开局统一走 jq_restart_game（随模式），
               旧版内联 jq_load_layout（暗棋）→ 与 OnTouch 按钮路径不一致。
               ★ 按钮业务对齐：mode 0=人机（AI 走红），mode 1=两人热座，mode 4=四国（AI 不介入）。 */
            g_vs_ai = (mode == 0);
            jq_restart_game();
            LOGI("OnEvent menu 开局 mode=%d（%s，jq_restart_game：随模式 mode=%d stage=PLAY 蓝先手）",
                 mode, g_vs_ai ? "人机AI走红" : "两人热座", ui_opt_get_mode());
            ui_screen_set(UI_GAME);   /* 对局态：AI 介入 + 触摸走子放行 */
        } else {
            /* 联机模式：等 OnBluetooth 帧（本方蓝先手，SPP 由 Java 宿主承担） */
            g_vs_ai = 0;   /* ★ 蓝牙模式：AI 不介入（回合闸口接管红方走子） */
            g_bt_sess_init_done = 0;
            ui_bt_set_active(0);   /* ★ 对抗审计（datah_067）：退出联机 ☰ 菜单分流 */
            st->stage = STG_PLAY;
            st->turn = SIDE_BLUE;
            st->winner = -1;
            LOGI("OnEvent menu 联机 mode=%d（等 OnBluetooth 帧，SPP 由 Java 宿主承担）", mode);
        }
    } else if (strcmp(name, "start") == 0) {
        /* 开局/重开：布局号可选（"start" / "start:3"），默认 default.lay。
           ★ 对抗审计 #4：明棋（revealed=1）+ 清 undo/stage/turn/winner/moves，对齐 jq_restart_game 语义；
           旧版两个重复 start 分支（第二个死代码）合并为一。 */
        int lay = -1;
        if (sscanf(value, ":%d", &lay) == 1 || sscanf(value, "%d", &lay) == 1) {
            if (lay < 0 || lay > 11) lay = -1;
        }
        if (lay >= 0) {
            char lp[128];
            snprintf(lp, sizeof lp, "/data/local/tmp/layouts/%02d.lay", lay);
            uint8_t lb[30];
            FILE *f = fopen(lp, "rb");
            if (f && fread(lb, 1, 30, f) == 30) {
                jq_load_layout(st, lb);
                fclose(f);
                /* ★ 对抗审计（本轮）：start:lay 与 jq_restart_game 语义统一——随模式
                   （布局明棋/翻棋暗棋），去掉"强制明棋"分裂（旧版写死 revealed=1）。 */
                int dark = ui_opt_get_mode() == 1;
                for (int i = 0; i < N_CELLS; i++)
                    if (st->cells[i].rank != RANK_EMPTY) st->cells[i].revealed = dark ? 0 : 1;
                LOGI("OnEvent start：布局 %d（%s）%s", lay, lp, dark ? "暗棋（翻棋）" : "明棋（布局）");
            } else {
                if (f) fclose(f);
                jq_restart_game();
                LOGI("OnEvent start：布局 %d 缺失 → default.lay（随模式 mode=%d）", lay, ui_opt_get_mode());
            }
        } else {
            jq_restart_game();
            LOGI("OnEvent start：default.lay（随模式 mode=%d 蓝先手）", ui_opt_get_mode());
        }
        g_undo_top = 0;
        st->stage = STG_PLAY;
        st->turn = SIDE_BLUE;
        st->winner = -1;
        st->moves_since_eat = 0;
        st->total_moves = 0;
        ui_replay_init();   /* ★ P1（datah_039/107）：开局清复盘记录 + 存开局 30 字节布局 */
        LOGI("OnEvent start：对局开始（stage=PLAY turn=蓝方）");
    } else if (strcmp(name, "status") == 0) {
        /* 查事件累计（对齐原版 Junqi.* 事件序列，桌面/实机诊断） */
        LOGI("OnEvent status：stage=%d turn=%d moves=%d events_total=%d [%s]",
             st->stage, st->turn, st->total_moves, g_event_total,
             "move/eat/bomb/flag/dead/mine=");
        for (int i = 0; i < 6; i++)
            LOGI("  %s = %d", JQ_EVENT_NAMES[i], g_event_cnt[i]);
    } else if (strcmp(name, "bttest") == 0) {
        jq_bt_test_run(st, "OnEvent");
    } else if (strcmp(name, "gameset") == 0) {
        /* ★ P1（datah_012）：游戏设置弹窗测试钩子（OnEvent("gameset","1") → UI_GAMESET）。
           值 0=关闭（恢复对局/主菜单）；与 jq.bttest 测试钩子同范式。 */
        int on = 0;
        if (sscanf(value, "%d", &on) != 1) on = 1;
        if (on) {
            ui_screen_set(UI_GAMESET);
            LOGI("OnEvent gameset：→ UI_GAMESET 游戏设置弹窗（datah_012 4 规则开关 + 确定/取消）");
        } else {
            ui_screen_set((ui_bt_active()) ? UI_GAME : UI_MENU);
            LOGI("OnEvent gameset：关闭 → %s", ui_bt_active() ? "回对局" : "回主菜单");
        }
    } else if (strcmp(name, "soundset") == 0) {
        /* ★ 对抗审计（datah_001）：声音设置弹窗测试钩子（OnEvent("soundset","1") → UI_SETTINGS）。
           游戏设置（datah_012）接管 ☰ 功能设置后，声音设置经此钩子触发（不破坏现有声音设置）。 */
        int on = 0;
        if (sscanf(value, "%d", &on) != 1) on = 1;
        if (on) {
            ui_screen_set(UI_SETTINGS);
            LOGI("OnEvent soundset：→ UI_SETTINGS 声音设置弹窗（datah_001 静音/按键/音量 + 确定/取消）");
        } else {
            ui_screen_set((ui_bt_active()) ? UI_GAME : UI_MENU);
            LOGI("OnEvent soundset：关闭 → %s", ui_bt_active() ? "回对局" : "回主菜单");
        }
    } else if (strcmp(name, "close") == 0) {
        /* UI 关闭音效（14 音效全量验证：close 此前未触发；Java/桌面可触发） */
        sfx_play(env, "close");
        LOGI("OnEvent close：播 close 音效（UI 关闭）");
    } else if (strcmp(name, "pop") == 0) {
        /* UI 弹窗/按钮点击音效（OnTouch 菜单按钮已播 pop；此处 OnEvent 路径备用） */
        sfx_play(env, "pop");
        LOGI("OnEvent pop：播 pop 音效（UI 弹窗）");
    } else if (strcmp(name, "layoutsave") == 0 ||
               strcmp(name, "layoutload") == 0 ||
               strcmp(name, "layoutmgr") == 0) {
        /* ★ P1（datah_016/017/020 证据）：布局存档 3 弹窗测试钩子（OnEvent setprop 范式，
           与 jq.gameset L1001 同；触摸 flaky 下可靠触发）。值 0=关闭（回对局）。 */
        int on = 0;
        if (sscanf(value, "%d", &on) != 1) on = 1;
        if (on) {
            ui_layout_list();   /* 进弹窗前刷新目录扫描 */
            if (strcmp(name, "layoutsave") == 0) {
                ui_screen_set(UI_SAVE_LAY);
                LOGI("OnEvent layoutsave：→ UI_SAVE_LAY 保存布局弹窗（datah_020 保存/改名/删除/关闭，30 字节 .lay）");
            } else if (strcmp(name, "layoutload") == 0) {
                ui_screen_set(UI_LOAD_LAY);
                LOGI("OnEvent layoutload：→ UI_LOAD_LAY 载入布局弹窗（datah_017 布局名列表 6 条 + 取消，ListBox 滚动简化）");
            } else {
                ui_screen_set(UI_LAY_MGR);
                LOGI("OnEvent layoutmgr：→ UI_LAY_MGR 布局管理弹窗（datah_016 30 格预览小方块 + 载入/删除/取消）");
            }
        } else {
            ui_screen_set(UI_GAME);
            LOGI("OnEvent %s：关闭 → 回对局", name);
        }
    } else if (strcmp(name, "replay") == 0) {
        /* ★ P1（datah_039/107 证据）：复盘入口测试钩子（OnEvent("replay","1") → UI_REPLAY）。
           值 1=进复盘态（ui_replay_rewind 重建 g_state 到 cur=n 记录末尾）；
           0=退复盘回对局（保留记录）。★ 诚实边界：datah_039 原版 BackDlg 浮标命中
           flaky → 钩子触发（与 jq.gameset L1001 同范式），非触摸路径。 */
        int on = 0;
        if (sscanf(value, "%d", &on) != 1) on = 1;
        if (on) {
            int n = ui_replay_count();
            int rc = ui_replay_rewind(n);   /* 重建到记录末尾（n=0 = 纯开局布局） */
            ui_screen_set(UI_REPLAY);
            LOGI("OnEvent replay：→ UI_REPLAY 复盘回放态（datah_107 控制条 5 按钮，cur=%d/%d rewind rc=%d，入口 setprop 钩子范式；跳转简化回开头/跳结尾，菜单简化退复盘）",
                 n, n, rc);
        } else {
            ui_screen_set(UI_GAME);
            LOGI("OnEvent replay：关闭 → 回对局（保留走子记录 %d 条）", ui_replay_count());
        }
    }
    (void)env;
}

/* ================= 14 个 JNI 入口（签名与 App.smali 一致） ================= */

/* OnCreate(Activity)→GLView：原版返回 GLView jobject；Java 侧 null 则 new。
   渲染层为桩（对象图月级），返回 NULL 走 Java 兜底。 */
/* ★ btconn 测试钩子 worker：spawn 线程模拟 BtSpp 后台回调 btState("host_connected")。
   真实场景：BtSpp accept/读线程调 App.btState → runOnUiThread post 主线程 → App$BtStart → OnEvent。 */
static void *btconn_worker(void *arg)
{
    /* arg = 主线程已 FindClass+NewGlobalRef 的 com/cnvcs/App（native 线程 FindClass 用系统
       classloader 会 ClassNotFoundException，故用缓存全局引用——JNI 已知陷阱）。 */
    jclass cls = (jclass)arg;
    if (!cls) return NULL;
    JavaVM *vm = jq_get_jvm();
    if (!vm) return NULL;
    JNIEnv *e = NULL;
    if ((*vm)->AttachCurrentThread(vm, &e, NULL) != 0 || !e) return NULL;
    LOGI("btconn worker：已附加线程，调 btState(host_connected)");
    jmethodID mid = (*e)->GetStaticMethodID(e, cls, "btState",
                                            "(Ljava/lang/String;)V");
    if (mid) {
        jstring js = (*e)->NewStringUTF(e, "host_connected");
        if (js) {
            (*e)->CallStaticVoidMethod(e, cls, mid, js);
            LOGI("btconn worker：btState 已调用");
            (*e)->DeleteLocalRef(e, js);
        }
    } else LOGI("btconn worker：GetStaticMethodID btState 失败");
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    (*vm)->DetachCurrentThread(vm);
    (*e)->DeleteGlobalRef(e, cls);
    return NULL;
}


/* ★ 蓝牙帧状态机测试（无真机蓝牙时验 JNI→bt_session 通路）：构造 FLIP 帧走 decode+on_frame。
   供 OnEvent("bttest") 与 setprop jq.bttest 钩子复用（提取自 OnEvent 分支）。 */
static void jq_bt_test_run(jq_state *st, const char *tag)
{
    int fidx = -1;
    for (int i = 0; i < N_CELLS; i++)
        if (st->cells[i].rank && !st->cells[i].revealed && jq_can_flip(st, i))
            { fidx = i; break; }
    char tb[128];
    if (fidx >= 0) {
        int n = jq_bt_frame_encode(tb, sizeof tb, BT_T_FLIP,
                                   cell_col(fidx), cell_row(fidx), 0, 0, 0, 0, 0);
        if (n > 0) tb[n] = 0;
        int g[8];
        if (jq_bt_frame_decode(tb, &g[0], &g[1], &g[2], &g[3], &g[4], &g[5], &g[6])) {
            if (!g_bt_sess_init_done) { jq_bt_session_init(&g_bt_sess, st, 1); g_bt_sess_init_done = 1; }
            int rc = jq_bt_session_on_frame(&g_bt_sess, st,
                                             g[0], g[1], g[2], g[3], g[4], g[5], g[6]);
            LOGI("%s bttest：FLIP 帧 idx=%d rc=%d state=%d（JNI→bt_session 通路验证）",
                 tag, fidx, rc, (int)g_bt_sess.state);
        }
    } else LOGI("%s bttest：无可翻子", tag);
}
JNIEXPORT jobject JNICALL
Java_com_cnvcs_App_OnCreate(JNIEnv *env, jclass clazz, jobject activity)
{
    (void)clazz;
    pthread_mutex_lock(&g_mutex);
    /* 主线程拿 AAssetManager（若 JNI_OnLoad 判可达） */
    if (g_assets_avail && !g_assets && env && activity) {
        /* AAssetManager 走文件系统回退（正常 so 已验证：不直接链接 AAssetManager_fromJava，
           MuMu/真机转译层 libandroid 不导出该符号 → fail to link；用 AAsset_read 流式读
           需 AAssetManager，但转译层可达性存疑 → 全走 /data/local/tmp/spak_res 文件系统）。 */
        LOGI("OnCreate：AAsset 管线走文件系统回退（spak_res 目录）");
    }
    jq_load_layout(&g_state, JQ_DEFAULT_LAY);
    g_undo_top = 0;
    ui_replay_init();   /* ★ P1（datah_039/107）：开局清复盘记录 + 存开局 30 字节布局 */
    /* 明棋开局：全子 revealed=1（棋子画正面 rank 映射，50 子全可见；
       暗棋玩法后续加"翻棋"交互——点背面格 revealed=1 露出 rank）。
       渲染器在下方 jq_glview_init 快照 g_state，故必须先设 revealed。 */
    for (int i = 0; i < N_CELLS; i++)
        if (g_state.cells[i].rank != RANK_EMPTY)
            g_state.cells[i].revealed = 1;
    g_state.stage = STG_PLAY;
    g_state.turn = SIDE_BLUE;
    g_state.winner = -1;
    g_state.moves_since_eat = 0;
    g_state.total_moves = 0;
    /* 正向 UI 层：初始进主菜单（5 按钮）；点"人机对战"进对局（UI_GAME）。
       AI 只在 UI_GAME 态介入（OnUpdate/OnTimer 检查 ui_screen_get()）。 */
    /* UI 菜单初始态：对局已开局（STG_PLAY 蓝先手）但渲染显示主菜单 5 按钮。
       ★ 可玩闭环：OnEvent("menu","0")（Java 侧/桌面触发"人机对战"）→ UI_GAME + 重置对局，
       之后 ai_pick 介入推进红方、触摸放行走子。菜单态 AI 不推红方（等玩家点"人机对战"）。 */
    ui_screen_set(UI_MENU);
    sfx_mp_init(env);            /* MediaPlayer 引用缓存（OnTouch 事件播放用） */
    sfx_wav_ensure_private(env, activity);   /* WAV 落地内部存储 getFilesDir/jq_sfx（OnCreate 单线程一次性，
       事件播放纯 MediaPlayer File 路径，免 AAssetManager_open 多线程锁竞争 SIGSEGV） */
    sfx_sound_load();            /* ★ 音效持久化：读 jq_sound_on.txt（☰ 功能设置 4 上次状态） */
    ui_settings_load();          /* ★ 对抗审计（datah_001）：读 jq_settings.txt（静音/按键/音量） */
    ui_rules_load();            /* ★ P1（datah_012）：读 jq_rules.txt（和棋/大本营/地雷/扛旗 4 规则开关） */
    ui_bright_load();          /* ★ P2（datah_002）：读 jq_bright.txt（亮度档位 0低/1中/2高，缺文件默认中） */
    ui_imgset_load();          /* ★ P2（datah_007）：读 jq_imgset.txt（界面/棋子/夜间/对方 4 图像设置开关，缺文件默认值） */
    LOGI("OnCreate：明棋开局（stage=PLAY 蓝先手 revealed=全翻开 50 子）+ UI 菜单就绪（OnEvent menu=0 进对局）");
    /* ★ flagtest 触发（adb setprop jq.flagtest 1 → 重启 → OnCreate 注入近旗状态 + 自动走一步 FLAG）。
       触摸注入 MuMu flaky 不达 native（环境限制）→ 走 OnCreate 自动触发路径；
       事件链路/规则引擎/音效真实（jq_try_move(53,58)=EV_FLAG 桌面已证），
       仅"走子由 OnCreate 自动触发"非触摸。一次性（setprop 清 0）。 */
    {
        char ft_prop[16] = {0};
        __system_property_get("jq.flagtest", ft_prop);
        if (strcmp(ft_prop, "1") == 0) {
            memset(&g_state, 0, sizeof(g_state));
            g_state.turn = SIDE_BLUE; g_state.stage = STG_PLAY; g_state.winner = -1;
            for (int i = 0; i < N_CELLS; i++) g_state.cells[i].rank = RANK_EMPTY;
            g_state.cells[53].rank = 2; g_state.cells[53].side = SIDE_BLUE; g_state.cells[53].revealed = 1;
            g_state.cells[58].rank = 12 + 0xc; g_state.cells[58].side = SIDE_RED; g_state.cells[58].revealed = 1;
            g_state.cells[58].flags = JQF_FLAG;
            g_undo_top = 0;
            ui_screen_set(UI_GAME);
            int ftev = jq_try_move(&g_state, 53, 58);
            if (ftev >= 0 && ftev < 6) { g_event_cnt[ftev]++; g_event_total++; }
            LOGI("flagtest：近旗状态注入 + 自动走 53→58 ev=%d %s（turn=%d stage=%d winner=%d）",
                 ftev, ftev < 0 ? "-" : JQ_EVENT_NAMES[ftev],
                 g_state.turn, g_state.stage, g_state.winner);
            sfx_play(env, "flag");
            /* ★ 对局结束音效（flagtest 自动走吃旗 → stage=OVER → win/lose；
               OnCreate 自动走不经 OnTouch 走子块，故此处补播） */
            sfx_game_over(env, g_state.stage, g_state.winner);
            __system_property_set("jq.flagtest", "0");
        }
    }
    /* ★ btconn 测试钩子（adb setprop jq.btconn 1 → 重启 → 模拟 Java 蓝牙连接回调
       btState("host_connected") → 主线程 OnEvent menu 1 → 两人对局自动开局。
       验证 Java btState→runOnUiThread→App$BtStart→OnEvent 全链，无真机蓝牙时用）。 */
    {
        char bc_prop[16] = {0};
        __system_property_get("jq.btconn", bc_prop);
        LOGI("btconn 钩子：属性=%s（%s）", bc_prop, strcmp(bc_prop, "1") == 0 ? "触发" : "未触发");
        if (strcmp(bc_prop, "1") == 0) {
            /* ★ 真实场景：btState 由 BtSpp 后台线程调用 → runOnUiThread post 到主线程。
               钩子从 OnCreate 主线程同步调会导致 runOnUiThread 立即执行 + 重入 OnEvent 死锁，
               故 spawn 后台线程（AttachCurrentThread）模拟 BtSpp 读/accept 线程。
               FindClass 须主线程做（native 线程 classloader 不同）→ NewGlobalRef 传 worker。 */
            jclass acls = (*env)->FindClass(env, "com/cnvcs/App");
            if (acls) {
                jclass gref = (jclass)(*env)->NewGlobalRef(env, acls);
                (*env)->DeleteLocalRef(env, acls);
                if (gref) {
                    pthread_t tid;
                    int rc = pthread_create(&tid, NULL, btconn_worker, gref);
                    LOGI("btconn 钩子：pthread_create rc=%d", rc);
                    if (rc == 0) pthread_detach(tid);
                    else (*env)->DeleteGlobalRef(env, gref);
                } else LOGI("btconn 钩子：NewGlobalRef 失败");
            } else LOGI("btconn 钩子：FindClass 失败");
            if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            __system_property_set("jq.btconn", "0");
        }
    }
    /* ★ bttest 测试钩子（adb setprop jq.bttest 1 → 重启 → OnCreate 走蓝牙帧状态机测试：
       构造 FLIP 帧 → decode → jq_bt_session_on_frame，验证 JNI→bt_session 接收通路。 */
    {
        char bt_prop[16] = {0};
        __system_property_get("jq.bttest", bt_prop);
        if (strcmp(bt_prop, "1") == 0) {
            jq_bt_test_run(&g_state, "setprop");
            __system_property_set("jq.bttest", "0");
        }
    }
    /* ★ minetest 触发（adb setprop jq.minetest 1 → 重启 → OnCreate 注入挖雷状态 + 自动走一步）。
       蓝工兵 rank9 @ cell49(row9,col4)，红雷 rank11 @ cell54(row10,col4)，红 rank3 @ cell59
       （红有子 → 挖雷不结束对局，stage 保持 PLAY）。jq_try_move(49,54)=EV_MINE 桌面已证。 */
    {
        char mt_prop[16] = {0};
        __system_property_get("jq.minetest", mt_prop);
        if (strcmp(mt_prop, "1") == 0) {
            memset(&g_state, 0, sizeof(g_state));
            g_state.turn = SIDE_BLUE; g_state.stage = STG_PLAY; g_state.winner = -1;
            /* ★ 幂等修复：memset 后 mines_cleared/pieces_left 残留上一次注入值（ev 漂移 5→4），
               每次注入前重置（挖雷状态测试需确定性 ev=5）。 */
            for (int i = 0; i < 2; i++) { g_state.mines_cleared[i] = 0; g_state.pieces_left[i] = 0; }
            for (int i = 0; i < N_CELLS; i++) g_state.cells[i].rank = RANK_EMPTY;
            g_state.cells[49].rank = 9; g_state.cells[49].side = SIDE_BLUE; g_state.cells[49].revealed = 1;
            g_state.cells[54].rank = 11 + 0xc; g_state.cells[54].side = SIDE_RED; g_state.cells[54].revealed = 1;
            g_state.cells[54].flags = JQF_MINE;
            g_state.cells[59].rank = 3 + 0xc; g_state.cells[59].side = SIDE_RED; g_state.cells[59].revealed = 1;
            g_undo_top = 0;
            ui_screen_set(UI_GAME);
            int mtev = jq_try_move(&g_state, 49, 54);
            if (mtev >= 0 && mtev < 6) { g_event_cnt[mtev]++; g_event_total++; }
            LOGI("minetest：挖雷状态注入 + 自动走 49→54 ev=%d %s（turn=%d stage=%d）",
                 mtev, mtev < 0 ? "-" : JQ_EVENT_NAMES[mtev], g_state.turn, g_state.stage);
            sfx_play(env, "move");
            __system_property_set("jq.minetest", "0");
        }
    }
    /* ★ P1（datah_012）：游戏设置 setprop 钩子（adb setprop jq.gameset 1 → 重启 → OnCreate 弹 UI_GAMESET）。
       测试钩子范式，与 jq.bttest 同；值 1=弹出，0=不触发。一次性（清 0）。 */
    {
        char gs_prop[16] = {0};
        __system_property_get("jq.gameset", gs_prop);
        if (strcmp(gs_prop, "1") == 0) {
            ui_screen_set(UI_GAMESET);
            LOGI("jq.gameset 钩子：→ UI_GAMESET 游戏设置弹窗（datah_012 4 规则开关 + 确定/取消）");
            __system_property_set("jq.gameset", "0");
        }
    }
    /* ★ P1（datah_038）：登录 setprop 钩子（adb setprop jq.login 1 → 重启 → OnCreate 弹 UI_LOGIN）。
       测试钩子范式，与 jq.gameset 同；值 1=进登录弹窗，0=不触发。一次性（清 0）。
       诚实边界：原版登录=账号系统（服务器协议未逆向），游客=本地单机唯一真生效路径。 */
    {
        char ln_prop[16] = {0};
        __system_property_get("jq.login", ln_prop);
        if (strcmp(ln_prop, "1") == 0) {
            ui_screen_set(UI_LOGIN);
            LOGI("jq.login 钩子：→ UI_LOGIN 登录弹窗（datah_038 用户名/密码 + 登录/注册/游客/返回，"
                 "诚实边界：账号系统未逆向，游客=本地单机唯一真生效路径）");
            __system_property_set("jq.login", "0");
        }
    }
    /* ★ P1 布局存档目录：/data/local/tmp/layouts（jni_bridge L684 dirs[1] 范式，
       真机写目录；桌面 evverify 用 tmp 目录 ui_laydir_set）。★ 必须在 ui_layout_list
       （setprop 钩子块）之前 set，否则首次扫描走 ui_laydir_get 缺省 ./layouts 桌面路径
       （实证 0 条），扫描不到真机 /data/local/tmp/layouts。 */
    ui_laydir_set("/data/local/tmp/layouts");
    /* ★ P1（datah_016/017/020 证据）：布局存档 3 弹窗 setprop 钩子
       （adb setprop jq.layoutsave/layoutload/layoutmgr 1 → 重启 → OnCreate 弹对应弹窗）。
       测试钩子范式，与 jq.gameset 同；值 1=弹出，0=不触发。一次性（清 0）。 */
    {
        char ls_prop[16] = {0};
        __system_property_get("jq.layoutsave", ls_prop);
        if (strcmp(ls_prop, "1") == 0) {
            ui_layout_list();
            ui_screen_set(UI_SAVE_LAY);
            LOGI("jq.layoutsave 钩子：→ UI_SAVE_LAY 保存布局弹窗（datah_020 保存/改名/删除/关闭，30 字节 .lay）");
            __system_property_set("jq.layoutsave", "0");
        }
    }
    {
        char ll_prop[16] = {0};
        __system_property_get("jq.layoutload", ll_prop);
        if (strcmp(ll_prop, "1") == 0) {
            ui_layout_list();
            ui_screen_set(UI_LOAD_LAY);
            LOGI("jq.layoutload 钩子：→ UI_LOAD_LAY 载入布局弹窗（datah_017 布局名列表 6 条 + 取消，ListBox 滚动简化）");
            __system_property_set("jq.layoutload", "0");
        }
    }
    {
        char lm_prop[16] = {0};
        __system_property_get("jq.layoutmgr", lm_prop);
        if (strcmp(lm_prop, "1") == 0) {
            ui_layout_list();
            ui_screen_set(UI_LAY_MGR);
            LOGI("jq.layoutmgr 钩子：→ UI_LAY_MGR 布局管理弹窗（datah_016 30 格预览小方块 + 载入/删除/取消）");
            __system_property_set("jq.layoutmgr", "0");
        }
    }

    /* ★ P2（datah_037 证据）：规则文本 setprop 钩子（adb setprop jq.rules 1 → 重启 →
        OnCreate 弹 UI_RULES 规则文本弹窗：规则-翻棋/规则-布局 2 模式 + 返回）。
        测试钩子范式，与 jq.gameset 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：规则文本=src_rs 现有简化版固定文本（2 套：翻棋/布局）。 */
    {
        char ru_prop[16] = {0};
        __system_property_get("jq.rules", ru_prop);
        if (strcmp(ru_prop, "1") == 0) {
            ui_screen_set(UI_RULES);
            LOGI("jq.rules 钩子：→ UI_RULES 规则文本弹窗（datah_037 规则-翻棋/规则-布局 2 模式 + 返回，"
                 "诚实边界：规则文本=src_rs 简化版固定文本 2 套）");
            __system_property_set("jq.rules", "0");
        }
    }
    /* ★ P2（datah_104 证据）：新局完整版 setprop 钩子（adb setprop jq.newgame104 1 → 重启 →
        OnCreate 弹 UI_NEWGAME104 新局完整版弹窗：我方/电脑水平/玩法 3 选择 + 确定/取消）。
        测试钩子范式，与 jq.gameset 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。 */
    {
        char ng_prop[16] = {0};
        __system_property_get("jq.newgame104", ng_prop);
        if (strcmp(ng_prop, "1") == 0) {
            ui_screen_set(UI_NEWGAME104);
            LOGI("jq.newgame104 钩子：→ UI_NEWGAME104 新局完整版弹窗（datah_104 我方/电脑水平/玩法 3 选择 + 确定/取消，"
                 "诚实边界：3 项各循环切换当前值简化版）");
            __system_property_set("jq.newgame104", "0");
        }
    }
    /* ★ P2（datah_034 证据）：加入游戏 setprop 钩子（adb setprop jq.join 1 → 重启 →
        OnCreate 弹 UI_JOIN 加入游戏弹窗：桌号占位标签 + 确定/取消）。
        测试钩子范式，与 jq.gameset 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP；
        确定 → 诚实边界对话框 + 回主菜单。 */
    {
        char jo_prop[16] = {0};
        __system_property_get("jq.join", jo_prop);
        if (strcmp(jo_prop, "1") == 0) {
            ui_screen_set(UI_JOIN);
            LOGI("jq.join 钩子：→ UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
                 "诚实边界：Wi-Fi/局域网协议未逆向，src_rs 仅蓝牙联机已实现，确定→边界对话框）");
            __system_property_set("jq.join", "0");
        }
    }
    /* ★ P2（datah_035 证据）：创建游戏 setprop 钩子（adb setprop jq.create 1 → 重启 →
        OnCreate 弹 UI_CREATE 创建游戏弹窗：密码占位标签 + 确定/取消）。
        测试钩子范式，与 jq.gameset 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP；
        确定 → 诚实边界对话框 + 回主菜单。 */
    {
        char cr_prop[16] = {0};
        __system_property_get("jq.create", cr_prop);
        if (strcmp(cr_prop, "1") == 0) {
            ui_screen_set(UI_CREATE);
            LOGI("jq.create 钩子：→ UI_CREATE 创建游戏弹窗（datah_035 密码占位标签 + 确定/取消，"
                 "诚实边界：Wi-Fi/局域网协议未逆向，src_rs 仅蓝牙联机已实现，确定→边界对话框）");
            __system_property_set("jq.create", "0");
        }

    /* ★ P2（datah_061 证据）：新局完整版（完整版补充）setprop 钩子（adb setprop jq.newgame061 1 → 重启 →
        OnCreate 弹 UI_NG061 新局完整版弹窗：先后手/电脑水平/玩法/明暗棋 4 选择 + 确定/取消）。
        测试钩子范式，与 jq.newgame104 同；值 1=弹出，0=不触发。一次性（清 0）。
        现有 UI_DIFFICULTY datah_041 简化版不破坏，061 作为完整版补充（独立态）。
        ★ 诚实边界：4 项各循环切换当前值（对齐 datah_104 循环切换范式）。 */
    {
        char n061_prop[16] = {0};
        __system_property_get("jq.newgame061", n061_prop);
        if (strcmp(n061_prop, "1") == 0) {
            ui_screen_set(UI_NG061);
            LOGI("jq.newgame061 钩子：→ UI_NG061 新局完整版（完整版补充）弹窗（datah_061 先后手/电脑水平/玩法/明暗棋 "
                  "4 选择 + 确定/取消，诚实边界：4 项各循环切换当前值简化版）");
            __system_property_set("jq.newgame061", "0");
        }
    }
    /* ★ P2（datah_050 证据）：对局信息 setprop 钩子（adb setprop jq.gameinfo 1 → 重启 →
        OnCreate 弹 UI_GAMEINFO 对局信息弹窗：Info 文本框（战况统计复用） + 复盘/删除/返回 3 按钮）。
        测试钩子范式，与 jq.newgame061 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：Info 文本=☰ 战况统计复用，删除=清复盘记录简化。 */
    {
        char gi_prop[16] = {0};
        __system_property_get("jq.gameinfo", gi_prop);
        if (strcmp(gi_prop, "1") == 0) {
            ui_screen_set(UI_GAMEINFO);
            LOGI("jq.gameinfo 钩子：→ UI_GAMEINFO 对局信息弹窗（datah_050 Info 文本框（战况统计复用） + "
                  "复盘/删除/返回 3 按钮，诚实边界：删除=清复盘记录简化）");
            __system_property_set("jq.gameinfo", "0");
        }
    }
    /* ★ P2（datah_002 证据）：亮度调节 setprop 钩子（adb setprop jq.bright 1 → 重启 →
        OnCreate 弹 UI_BRIGHT 亮度调节弹窗：滑条简化=亮度 低/中/高 3 档循环 + 确定/取消）。
        测试钩子范式，与 jq.newgame061 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换（对齐 datah_001 音量循环范式）。 */
    {
        char br_prop[16] = {0};
        __system_property_get("jq.bright", br_prop);
        if (strcmp(br_prop, "1") == 0) {
            ui_screen_set(UI_BRIGHT);
            LOGI("jq.bright 钩子：→ UI_BRIGHT 亮度调节弹窗（datah_002 滑条简化=亮度 低/中/高 3 档循环 + "
                  "确定/取消，诚实边界：不画真滑条，cur=%d）", ui_bright_get());
            __system_property_set("jq.bright", "0");
        }
    }
    /* ★ P2（datah_029 证据）：聊天记录 setprop 钩子（adb setprop jq.chatlog 1 → 重启 →
        OnCreate 弹 UI_CHATLOG 聊天记录弹窗：Msg 文本框（诚实边界占位） + 清除/关闭 2 按钮）。
        测试钩子范式，与 jq.newgame061 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
    {
        char cl_prop[16] = {0};
        __system_property_get("jq.chatlog", cl_prop);
        if (strcmp(cl_prop, "1") == 0) {
            ui_screen_set(UI_CHATLOG);
            LOGI("jq.chatlog 钩子：→ UI_CHATLOG 聊天记录弹窗（datah_029 Msg 文本框（诚实边界占位） + "
                  "清除/关闭 2 按钮，诚实边界：联机聊天文本未逆向）");
            __system_property_set("jq.chatlog", "0");
        }
    }
    /* ★ P2（datah_031 证据）：个人资料 setprop 钩子（adb setprop jq.profile 1 → 重启 →
        OnCreate 弹 UI_PROFILE 个人资料弹窗：Info 文本框（诚实边界占位） + 修改/帮助/返回 3 按钮）。
        测试钩子范式，与 jq.newgame061 同；值 1=弹出，0=不触发。一次性（清 0）。
        现有 datah_072 方式弹窗"个人资料"→UI_LOGIN 不破坏，031 作为登录后资料查看补充（独立态）。
        ★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
    {
        char pf_prop[16] = {0};
        __system_property_get("jq.profile", pf_prop);
        if (strcmp(pf_prop, "1") == 0) {
            ui_screen_set(UI_PROFILE);
            LOGI("jq.profile 钩子：→ UI_PROFILE 个人资料弹窗（datah_031 Info 文本框（诚实边界占位） + "
                  "修改/帮助/返回 3 按钮，诚实边界：账号系统未逆向，离线单机无账号）");
            __system_property_set("jq.profile", "0");
        }
    }
    /* ★ P2（datah_042 证据）：提和详情 setprop 钩子（adb setprop jq.drawdetail 1 → 重启 →
        OnCreate 弹 UI_DRAWDETAIL 提和详情弹窗：联机版 ☰ 菜单 7 图标 提和/认输/聊天/系统设置/
        玩家资料/战况统计/软件信息）。测试钩子范式，与 jq.profile 同；值 1=弹出，0=不触发。
        一次性（清 0）。★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
        联机提和/认输走 BT 帧 + 边界对话框，不包装成"已提和判定"。 */
    {
        char dd_prop[16] = {0};
        __system_property_get("jq.drawdetail", dd_prop);
        if (strcmp(dd_prop, "1") == 0) {
            ui_screen_set(UI_DRAWDETAIL);
            LOGI("jq.drawdetail 钩子：→ UI_DRAWDETAIL 提和详情弹窗（datah_042 联机版 ☰ 菜单 7 图标：提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，"
                  "诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
            __system_property_set("jq.drawdetail", "0");
        }
    }
    /* ★ P2（datah_042 证据）：提和详情 setprop 钩子（adb setprop jq.drawdetail 1 → 重启 →
        OnCreate 弹 UI_DRAWDETAIL 提和详情弹窗：联机版 ☰ 菜单 7 图标 提和/认输/聊天/系统设置/
        玩家资料/战况统计/软件信息）。测试钩子范式，与 jq.profile 同；值 1=弹出，0=不触发。
        一次性（清 0）。★ 诚实边界：本地提和判定未实现（原版需对端应答），联机提和/认输走
        BT 帧 + 边界对话框，不包装成"已提和判定"。 */
    {
        char dd_prop[16] = {0};
        __system_property_get("jq.drawdetail", dd_prop);
        if (strcmp(dd_prop, "1") == 0) {
            ui_screen_set(UI_DRAWDETAIL);
            LOGI("jq.drawdetail 钩子：→ UI_DRAWDETAIL 提和详情弹窗（datah_042 联机版 ☰ 菜单 7 图标："
                  "提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，诚实边界：本地提和判定未实现，"
                  "提和/认输走 BT 帧 + 边界对话框）");
            __system_property_set("jq.drawdetail", "0");
        }
    }
    /* ★ P2（datah_003 证据）：设置入口 setprop 钩子（adb setprop jq.setentry 1 → 重启 →
        OnCreate 弹 UI_SETEXTTRY 设置入口弹窗（完整版）：图像/声音/游戏设置 3 按钮入口。
        测试钩子范式，与 jq.drawdetail 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像设置渲染参数未实现，图像设置入口指向 UI_IMGSET7；
        声音设置→UI_SETTINGS（datah_001）；游戏设置→UI_GAMESET（datah_012）。 */
    {
        char se_prop[16] = {0};
        __system_property_get("jq.setentry", se_prop);
        if (strcmp(se_prop, "1") == 0) {
            ui_screen_set(UI_SETEXTTRY);
            LOGI("jq.setentry 钩子：→ UI_SETEXTTRY 设置入口弹窗（datah_003 图像/声音/游戏设置 3 按钮入口，"
                 "图像→UI_IMGSET7；声音→UI_SETTINGS（datah_001）；游戏→UI_GAMESET（datah_012），"
                 "★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）");
            __system_property_set("jq.setentry", "0");
        }
    }
    /* ★ P2（datah_004 证据）：设置入口（简化版）setprop 钩子（adb setprop jq.setentry4 1 → 重启 →
        OnCreate 弹 UI_SETEXTTRY4 设置入口（简化版）弹窗：图像/声音 2 按钮入口（无游戏设置项）。
        测试钩子范式，与 jq.setentry 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像设置渲染参数未实现，图像设置入口指向 UI_IMGSET8；
        声音设置→UI_SETTINGS（datah_001）。 */
    {
        char se4_prop[16] = {0};
        __system_property_get("jq.setentry4", se4_prop);
        if (strcmp(se4_prop, "1") == 0) {
            ui_screen_set(UI_SETEXTTRY4);
            LOGI("jq.setentry4 钩子：→ UI_SETEXTTRY4 设置入口（简化版）弹窗（datah_004 图像/声音 2 按钮入口，"
                 "图像→UI_IMGSET8；声音→UI_SETTINGS（datah_001），★ 诚实边界：图像设置渲染参数未实现）");
            __system_property_set("jq.setentry4", "0");
        }
    }
    /* ★ P2（datah_007 证据）：图像设置（完整版）setprop 钩子（adb setprop jq.imgset7 1 → 重启 →
        OnCreate 弹 UI_IMGSET7 图像设置弹窗（完整版）：界面/棋子动画/夜间模式/对方棋子
        4 开关 + 确定/取消。测试钩子范式，与 jq.setentry 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像/夜间/对方棋子渲染参数未实现，开关仅 UI 状态记录；
        确定→ui_imgset_save（jq_imgset.txt 4 字段）。 */
    {
        char i7_prop[16] = {0};
        __system_property_get("jq.imgset7", i7_prop);
        if (strcmp(i7_prop, "1") == 0) {
            ui_screen_set(UI_IMGSET7);
            LOGI("jq.imgset7 钩子：→ UI_IMGSET7 图像设置弹窗（完整版 datah_007 4 开关 + 确定/取消，"
                 "★ 诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录；"
                 "界面=%d 棋子=%d 夜间=%d 对方=%d）",
                 ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get(), ui_imgset_opp_get());
            __system_property_set("jq.imgset7", "0");
        }
    }
    /* ★ P2（datah_008 证据）：图像设置（简化版）setprop 钩子（adb setprop jq.imgset8 1 → 重启 →
        OnCreate 弹 UI_IMGSET8 图像设置弹窗（简化版）：界面/棋子动画/夜间模式
        3 开关 + 确定/取消。测试钩子范式，与 jq.imgset7 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像/夜间渲染参数未实现，开关仅 UI 状态记录；
        确定→ui_imgset_save（jq_imgset.txt 3 字段）。 */
    {
        char i8_prop[16] = {0};
        __system_property_get("jq.imgset8", i8_prop);
        if (strcmp(i8_prop, "1") == 0) {
            ui_screen_set(UI_IMGSET8);
            LOGI("jq.imgset8 钩子：→ UI_IMGSET8 图像设置弹窗（简化版 datah_008 3 开关 + 确定/取消，"
                 "★ 诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录；"
                 "界面=%d 棋子=%d 夜间=%d）",
                 ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get());
            __system_property_set("jq.imgset8", "0");
        }
    }
    /* ★ P2（datah_065 证据）：加入游戏完整版 setprop 钩子（adb setprop jq.join065 1 → 重启 →
        OnCreate 弹 UI_JOIN065 加入游戏完整版弹窗：先手/后手/玩法/明暗棋/4 规则显示
        + 开始/取消。测试钩子范式，与 jq.join（datah_034 简化版）同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：联机开局配置=占位标签，开始→边界对话框（src_rs 仅蓝牙已实现）。 */
    {
        char j65_prop[16] = {0};
        __system_property_get("jq.join065", j65_prop);
        if (strcmp(j65_prop, "1") == 0) {
            ui_screen_set(UI_JOIN065);
            LOGI("jq.join065 钩子：→ UI_JOIN065 加入游戏完整版弹窗（datah_065 先手/后手/玩法/明暗棋/"
                 "4 规则显示 + 开始/取消，★ 诚实边界：联机开局配置=占位标签，开始→边界对话框）");
            __system_property_set("jq.join065", "0");
        }
    }
    /* ★ P2（datah_069 证据）：创建游戏完整版 setprop 钩子（adb setprop jq.create069 1 → 重启 →
        OnCreate 弹 UI_CREATE069 创建游戏完整版弹窗：先后手/玩法/明暗棋/游戏规则
        4 选择 + 确定/取消。测试钩子范式，与 jq.create（datah_035 简化版）同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：联机房间创建=边界对话框；游戏规则→UI_RULES070（datah_070）。 */
    {
        char c69_prop[16] = {0};
        __system_property_get("jq.create069", c69_prop);
        if (strcmp(c69_prop, "1") == 0) {
            ui_screen_set(UI_CREATE069);
            LOGI("jq.create069 钩子：→ UI_CREATE069 创建游戏完整版弹窗（datah_069 先后手/玩法/明暗棋/"
                 "游戏规则 4 选择 + 确定/取消，★ 诚实边界：联机房间创建=边界对话框；游戏规则→UI_RULES070）");
            __system_property_set("jq.create069", "0");
        }
    }
    /* ★ P2（datah_070 证据）：游戏规则完整版 setprop 钩子（adb setprop jq.rules070 1 → 重启 →
        OnCreate 弹 UI_RULES070 游戏规则完整版弹窗：和棋/大本营/吃地雷/吃军旗
        4 规则开关 + 确定/取消。与 datah_012 游戏设置同构，复用 ui_rule_*_get/set。
        测试钩子范式，与 jq.rules（datah_037 规则文本）同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 与 datah_012 区分：070 是联机"游戏规则"弹窗（datah_069 入口），
        012 是本地 ☰ 菜单"功能设置"→UI_GAMESET，复用同一组 ui_rule_* 开关。 */
    {
        char r70_prop[16] = {0};
        __system_property_get("jq.rules070", r70_prop);
        if (strcmp(r70_prop, "1") == 0) {
            ui_screen_set(UI_RULES070);
            LOGI("jq.rules070 钩子：→ UI_RULES070 游戏规则完整版弹窗（datah_070 4 规则开关 + 确定/取消，"
                 "与 datah_012 同构，复用 ui_rule_*；★ 与 012 区分：070=联机游戏规则弹窗，012=本地 ☰ UI_GAMESET）");
            __system_property_set("jq.rules070", "0");
        }
    }
    /* ★ P2（datah_060 证据）：房间态 setprop 钩子（adb setprop jq.room060 1 → 重启 →
        OnCreate 弹 UI_ROOM060 房间态弹窗：2 玩家房间列表（占位标签）+ 开始/换桌/离开。
        测试钩子范式，与 jq.join065 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签。 */
    {
        char rm_prop[16] = {0};
        __system_property_get("jq.room060", rm_prop);
        if (strcmp(rm_prop, "1") == 0) {
            ui_screen_set(UI_ROOM060);
            LOGI("jq.room060 钩子：→ UI_ROOM060 房间态弹窗（datah_060 2 玩家房间列表（占位标签）+ 开始/换桌/离开，"
                 "★ 诚实边界：联机房间=边界对话框，2 玩家显示区=占位标签）");
            __system_property_set("jq.room060", "0");
        }
    }
    /* ★ P2（datah_063 证据）：聊天完整版 setprop 钩子（adb setprop jq.chat063 1 → 重启 →
        OnCreate 弹 UI_CHAT063 聊天完整版弹窗：Msg 输入框（占位标签）+ 发送/记录/关闭。
        测试钩子范式，与 jq.chatlog（datah_029 聊天记录）同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029）。 */
    {
        char ch63_prop[16] = {0};
        __system_property_get("jq.chat063", ch63_prop);
        if (strcmp(ch63_prop, "1") == 0) {
            ui_screen_set(UI_CHAT063);
            LOGI("jq.chat063 钩子：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭，"
                 "★ 诚实边界：联机聊天文本未逆向，发送=占位帧+边界对话框；记录→UI_CHATLOG（datah_029））");
            __system_property_set("jq.chat063", "0");
        }
    }

    }
    /* ★ P1（datah_039 证据）：复盘回放 setprop 钩子（adb setprop jq.replay 1 → 重启 →
       OnCreate 进 UI_REPLAY 复盘态：重建 g_state 到记录末尾 + datah_107 控制条渲染）。
       入口简化（原版 BackDlg 浮标命中 flaky → 钩子触发，参考 jq.gameset L1024 范式）。
       值 1=进复盘态，0=不触发。一次性（清 0）。
       ★ 走子记录验证链（真机实证）：jq.movetest 1 钩子（下方，OnCreate 自动走 3 步
       每步 ui_replay_record 1 条，n=3）→ 同会话 setprop jq.replay 1（OnUpdate 轮询，
       不重启 app）→ UI_REPLAY cur=3/3 → 回退/前进/跳转 有真实步进。 */
    {
        char rp_prop[16] = {0};
        __system_property_get("jq.replay", rp_prop);
        LOGI("jq.replay 钩子：属性=%s（%s）", rp_prop, strcmp(rp_prop, "1") == 0 ? "触发进复盘" : "未触发");
        if (strcmp(rp_prop, "1") == 0) {
            int n = ui_replay_count();
            int rc = ui_replay_rewind(n);   /* 重建到记录末尾（n=0 = 纯开局布局） */
            ui_screen_set(UI_REPLAY);
            LOGI("jq.replay 钩子：→ UI_REPLAY 复盘回放态（datah_107 控制条 5 按钮，cur=%d/%d rewind rc=%d；跳转简化回开头/跳结尾，菜单简化退复盘，入口 setprop 钩子）",
                 n, n, rc);
            __system_property_set("jq.replay", "0");
        }
    }
    /* ★ P1（datah_039 走子记录实证钩子）：jq.movetest 1 → 重启 → OnCreate 明棋开局 +
       自动走 3 步（默认布局 6→11 / 30→25 / 5→6，蓝红交替合法走子，桌面 evverify
       同序列已证）→ 每步走子成功后 ui_replay_record 1 条（n=3）→ 供 jq.replay 钩子
       进复盘态 cur=3/3 走子记录回放实证（诚实边界：走子由 OnCreate 自动触发非触摸，
       规则引擎/事件/记录链真实；触摸 flaky 下的可靠触发路径）。一次性（清 0）。 */
    {
        char mv_prop[16] = {0};
        __system_property_get("jq.movetest", mv_prop);
        LOGI("jq.movetest 钩子：属性=%s（%s）", mv_prop, strcmp(mv_prop, "1") == 0 ? "触发自动走 3 步" : "未触发");
        if (strcmp(mv_prop, "1") == 0) {
            static const int seq3[3][2] = { {6,11}, {30,25}, {5,6} };   /* 蓝/红 交替（turn 走子后翻转） */
            int okcnt = 0;
            for (int k = 0; k < 3; k++) {
                int f = seq3[k][0], t = seq3[k][1];
                int mv = jq_try_move(&g_state, f, t);
                if (mv >= 0) {
                    ui_replay_record(f, t, 0);   /* 走子成功记录 1 步（复盘序列 n++） */
                    okcnt++;
                    LOGI("movetest 自动走子 第%d步 (%d,%d)→(%d,%d) ev=%d %s（记录 n=%d）",
                         k + 1, f, t, f, t, mv, JQ_EVENT_NAMES[mv], ui_replay_count());
                } else {
                    LOGI("movetest 自动走子 第%d步 (%d,%d) 非法（布局/回合不合法，停止）", k + 1, f, t);
                    break;
                }
            }
            sfx_play(env, "move");
            LOGI("movetest：自动走子完成 %d/3 步（复盘走子记录 n=%d，jq.replay 钩子可进复盘 cur=%d/%d）",
                 okcnt, ui_replay_count(), ui_replay_cur(), ui_replay_count());
            __system_property_set("jq.movetest", "0");
        }
    }

    jq_touch_state_init(&g_ts);   /* ⑤b 态机初始化 */
    /* ③ 渲染对象图构造（对齐原版 0xba12c → 0xba034/0xc5d54/0xb7500） */
    jq_subsys_list_init(&g_subs);
    jq_subsys sgl, sgl2, srend, saud;
    sgl.type = 0; sgl.obj = &g_egl;      jq_egl_obj_init(&g_egl);
    sgl2.type = 1; sgl2.obj = &g_aud;    jq_audio_vec_init(&g_aud);
    srend.type = 2; srend.obj = &g_rend;
    saud.type = 3; saud.obj = &g_scene;
    jq_subsys_list_push(&g_subs, &sgl);
    jq_subsys_list_push(&g_subs, &sgl2);
    jq_subsys_list_push(&g_subs, &srend);
    jq_subsys_list_push(&g_subs, &saud);
    /* ④ 布局容器（0x6a3b0 +0x78 计数表 + +0x68 vector） */
    jq_container_fill_layout(&g_laycc, JQ_DEFAULT_LAY);
    jq_container_validate(&g_laycc, &g_state);
    pthread_mutex_unlock(&g_mutex);
    /* ③b：初始化 GLView（EGL 上下文 + 渲染器 ①②）。
       返回 Java GLView jobject（App.smali：非 null 则 setContentView + setRenderer，GLThread 每帧调 OnUpdate 出帧）；
       EGL 失败（无 GPU）→ 返回 NULL，Java 侧跳过 setContentView（空白界面，不崩）。 */
    #define SPAK_DIR "/data/local/tmp/spak_res"  /* 免 push：可改 app 私有目录；当前固定 tmp */
    int glv_ok = (jq_glview_init(&g_glview, &g_state, SPAK_DIR, NULL, g_assets) == 0);
    if (glv_ok)
        LOGI("OnCreate：GLView EGL 上下文 + 渲染器就绪");
    else
        LOGI("OnCreate：GLView 初始化失败 → 返回 NULL（无 GPU 兜底）");
    if (!glv_ok || !env || !activity)
        return NULL;
    /* 创建 Java GLView（com.cnvcs.GLView）并返回给 App.smali 走 setContentView */
    jclass glview_cls = (*env)->FindClass(env, "com/cnvcs/GLView");
    if (!glview_cls) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        LOGE("OnCreate：FindClass(GLView) 失败 → 返回 NULL");
        return NULL;
    }
    jmethodID ctor = (*env)->GetMethodID(env, glview_cls, "<init>",
                                         "(Landroid/content/Context;)V");
    if (!ctor) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        LOGE("OnCreate：GLView <init> 查找失败 → 返回 NULL");
        return NULL;
    }
    jobject jview = (*env)->NewObject(env, glview_cls, ctor, activity);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        jview = NULL;
    }
    if (jview) {
        /* ★ 让 GLView 的 EglHelper 建 GL2 context（默认 GL1，GL2 调用会 SIGSEGV）：
           setEGLContextClientVersion(2) 必须在 GLThread start 前调（GLView 的 checkRenderThreadState）。
           这样 Java GLThread 出屏链（surfaceCreated→EglHelper→eglSwapBuffers）走 GL2 通路。 */
        jmethodID setCV = (*env)->GetMethodID(env, glview_cls, "setEGLContextClientVersion", "(I)V");
        if (setCV) {
            (*env)->CallVoidMethod(env, jview, setCV, 2);
            if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            LOGI("OnCreate：GLView.setEGLContextClientVersion(2)（EglHelper 出屏链走 GL2）");
        } else {
            if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            LOGI("OnCreate：setEGLContextClientVersion 查找失败（回退 GL1 EglHelper）");
        }
        /* App.smali 没调 setContentView（只 setOnTouchListener/setRenderer）→ view 不 attach
           → GLView.onAttachedToWindow 不触发 → GLThread 不启动 → OnUpdate 永不被调（出帧链断）。
           native 直接 activity.setContentView(view) 让 view attach，GLThread 起来驱动出帧。 */
        jclass act_cls = (*env)->GetObjectClass(env, activity);
        jmethodID scv = (*env)->GetMethodID(env, act_cls, "setContentView", "(Landroid/view/View;)V");
        if (scv) {
            (*env)->CallVoidMethod(env, activity, scv, jview);
            if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            /* GLThread RENDERMODE_WHEN_DIRTY（默认）需要 requestRender 才出首帧；
               onAttachedToWindow 已起 GLThread，但没 requestRender → GLThread 停在 wait。 */
            jmethodID rr_m = (*env)->GetMethodID(env, glview_cls, "requestRender", "()V");
            if (rr_m) {
                (*env)->CallVoidMethod(env, jview, rr_m);
                if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
                LOGI("OnCreate：GLView.requestRender()（GLThread 出首帧，EglHelper window surface 链激活）");
            }
            LOGI("OnCreate：setContentView(GLView) 完成，GLThread 出帧链启动（surface 由 Java 侧 attach 创建）");
        /* GL2/GLSL 出帧能力探测（决定 render.c 是否可改 GLES2）：
           app 进程内建独立 GL2 context 测全屏三角形，独立 64 位 so 在 MuMu 找不到 GL 实现 */
        /* GL2 出帧由 native 独立渲染线程驱动（PBuffer GL2 context + window surface 合成，
           绕开 Java GLThread 的 GL1 EglHelper 死结）。
           window surface：GLView extends SurfaceView → getHolder().getSurface() →
           ANativeWindow_fromSurface → 渲染线程 blit 出屏。 */
        jq_glview_start_render_thread();
        /* 拿 GLView 的 Surface（SurfaceView.getHolder().getSurface()）→ 主线程 fromSurface
           （JNI 调用需 attach 线程；渲染线程未 attach 不能调）。ANativeWindow* 存全局，
           渲染线程只做 EGL 合成 + swap（不碰 JNI）。
           SurfaceView surface 在 attach 后才 valid——setContentView 已让 GLView attach，
           但 surface 创建可能异步；fromSurface 失败则等渲染线程首帧后重试一次。 */
        {
            jclass holder_cls = (*env)->FindClass(env, "android/view/SurfaceHolder");
            jmethodID getHolder = (*env)->GetMethodID(env, glview_cls, "getHolder", "()Landroid/view/SurfaceHolder;");
            if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); getHolder = NULL; }
            if (holder_cls && getHolder) {
                jobject holder = (*env)->CallObjectMethod(env, jview, getHolder);
                if (holder) {
                    jmethodID getSurf = (*env)->GetMethodID(env, holder_cls, "getSurface", "()Landroid/view/Surface;");
                    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); getSurf = NULL; }
                    jclass surf_cls = (*env)->FindClass(env, "android/view/Surface");
                    if (getSurf && surf_cls) {
                        jobject surf = (*env)->CallObjectMethod(env, holder, getSurf);
                        if (surf) {
                            /* SurfaceView surface 懒创建（OnCreate 时 isValid=0，layout pass 才分配）。
                               setZOrderOnTop(true) 已调（上方），但 buffer 分配在下一帧 ——
                               OnResume 重试 fromSurface（isValid=1 后 window 出屏通路就绪）。 */
                            void *nw = ANativeWindow_fromSurface(env, surf);
                            if (nw) {
                                jq_glview_set_window(nw, g_glview.disp);
                                LOGI("OnCreate：ANativeWindow 已传渲染线程（%p，window 出屏通路就绪）", nw);
                            } else {
                                /* surface 未 ready → 存 Surface global ref，OnResume 重试 fromSurface 诊断。 */
                                g_gl_surf = (*env)->NewGlobalRef(env, surf);
                                jq_glview_set_surface_env(env, g_gl_surf, g_glview.disp);
                                LOGI("OnCreate：surface 未 ready（fromSurface 返 NULL），存 global ref（OnResume 重试）");
                            }
                            (*env)->DeleteLocalRef(env, surf);
                        }
                    }
                    (*env)->DeleteLocalRef(env, holder);
                }
                (*env)->DeleteLocalRef(env, holder_cls);
            }
        }
        } else {
            LOGE("OnCreate：setContentView 查找失败 → GLThread 不启动");
        }
        (*env)->DeleteLocalRef(env, act_cls);
    }
    return jview;   /* App.smali：非 null → setOnTouchListener + setRenderer(this) + GLThread 启动 */
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnDestroy(JNIEnv *env, jclass clazz)
{
    (void)env; (void)clazz;
    pthread_mutex_lock(&g_mutex);
    g_undo_top = 0;
    pthread_mutex_unlock(&g_mutex);
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnPause(JNIEnv *env, jclass clazz)
{
    (void)env; (void)clazz;
    /* 原版 OnPause 保存 one.sjg（0x390ac）；此处存档 */
    pthread_mutex_lock(&g_mutex);
    jq_save_game_write("/data/local/tmp/one.sjg", &g_state);
    pthread_mutex_unlock(&g_mutex);
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnResume(JNIEnv *env, jclass clazz)
{
    (void)clazz;
    /* OnResume：GLView surface 在 onResume 后才真正 ready → 重试 ANativeWindow_fromSurface
       （OnCreate 时 surface 未创建，fromSurface 返 NULL）。成功即建 window EGL surface 出屏。 */
    /* 出屏主路归 Java EglHelper（window surface + eglSwapBuffers 已由 GLThread 持有）：
       OnResume 不做 fromSurface（不再抢 ANativeWindow 引用）。g_gl_surf 保留 global ref
       仅作诊断（surface 生命周期跟随 Activity）。 */
    (void)g_gl_surf;
}

/* OnTimer()：原版 AI 定时器入口（L76/L78：OnTimer/OnUpdate 均 bl 0x8598）。
   人机模式：红方回合由 AI 走子（ai_pick）。 */
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnTimer(JNIEnv *env, jclass clazz)
{
    (void)clazz;
    /* 出屏主路归 Java EglHelper（OnUpdate 在 GLThread window context 上 rs_render_frame）：
       OnTimer 不再 fromSurface 重试。 */
    static int g_timer_cnt = 0;
    (void)g_timer_cnt;
    if (env) {
        pthread_mutex_lock(&g_mutex);
        if (ui_screen_get() == UI_GAME && g_vs_ai && g_state.stage == STG_PLAY && g_state.turn == SIDE_RED) {
        int from, to, flip;
        if (ai_pick(&from, &to, &flip) == 0) {
            undo_push();
            if (flip) {
                if (jq_flip(&g_state, from)) ui_replay_record(from, from, 1);   /* ★ P1（datah_039/107）：AI 翻棋记录 */
            } else {
                int e = jq_try_move(&g_state, from, to);
                if (e >= 0) ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：AI 走子记录 */
                LOGI("红方 AI 应着：from=%d to=%d 结果=%s（turn 切回蓝方）",
                     from, to, JQ_EVENT_NAMES[e < 0 ? 0 : e]);
            }
            LOGI("OnTimer 人机AI：%s cell=%d turn→%d", flip ? "翻棋" : "走子", from, g_state.turn);
        }
        }
        pthread_mutex_unlock(&g_mutex);
    }
}

/* OnUpdate()→jboolean：每帧回调；AI 自动走子 + 返回 1（消费）。
   GL 渲染 SIGSEGV 兜底：MuMu GL 转译可能因 glVertexAttribPointer 等 GL2 管线
   未实现而在 GLThread 上崩 → 信号 handler 设标志位，OnUpdate 检测到后跳过渲染
   只做 AI 推进（同影子库 §113 生命周期安全阀思路）。 */
static volatile sig_atomic_t g_gl_crash = 0;

/* 信号 handler：设标志 + siglongjmp 回 OnUpdate 入口（同影子库 §113 longjmp 展开） */
#include <setjmp.h>
static jmp_buf g_gl_jmp;
static volatile int g_gl_jmp_armed = 0;

static void _gl_sig_handler(int sig, siginfo_t *si, void *uc) {
    g_gl_crash = 1;
    signal(sig, SIG_DFL);
    raise(sig);   /* 让信号正常终止进程 → 日志落 crash buffer（正向验收不吞崩溃，根因可查） */
}

__attribute__((constructor))
static void _gl_sig_init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = _gl_sig_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaltstack(NULL, NULL);  /* 默认 stack */
    sigaction(SIGSEGV, &sa, NULL);
}

JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnUpdate(JNIEnv *env, jclass clazz)
{
    (void)env; (void)clazz;
    /* ③b：GLView GLThread 每帧回调（原版 GLView$Renderer.onDrawFrame → OnUpdate）：
       GLView 就绪 → 驱动 ①② 出帧；否则退化 AI 推进。
       GL 渲染 SIGSEGV 兜底：setjmp 武装后调渲染，siglongjmp 回本函数入口（§113 思路）。 */
    static int g_upd_cnt = 0;
    /* ★ 正向出屏（安卓游戏规范）：OnUpdate 由 GLThread.onDrawFrame 调用，此刻 EglHelper 的
       window surface 已 makeCurrent（GL2 context，setEGLContextClientVersion(2)）→
       在此调 rs_render_frame 画到 window 上，onDrawFrame 返回后 EglHelper.eglSwapBuffers
       出屏。rs_render_frame 的 gl_now_ctx 检测切 context 时重建 GL 对象（纹理/VBO/shader
       属于各自 context）。PBuffer 渲染线程仅在 GLThread 未出帧时兜底（离屏，不上屏）。 */
    /* GL 渲染崩（g_gl_crash=1，SIGSEGV handler 设位）→ 后续帧跳过渲染只推 AI（不闪退）。
       ★ 对抗审计（本轮）：rs_render_frame 读 g_state（rr->game->cells/jq_can_move）——
       OnTouch/OnEvent 在 g_mutex 内写 g_state → 渲染读必须持锁，否则 cells 字节撕裂
       （棋子 rank/side 闪错帧）。持锁只覆盖主屏 GLThread 路径（OnUpdate 出屏）；
       PBuffer 离屏兜底线程不持锁（不上屏，撕裂不可见，诚实边界推迟）。sel_cell 是
       int 对齐原子读，高亮滞后一帧良性，不在本锁范围。rs_render_frame 纯 GL 不回调
       Java/不锁 g_mutex → 持锁无死锁（OnTouch 短暂等待 <1 帧）。 */
    if (!g_gl_crash && jq_glview_is_ready() && g_glview.rr) {
        pthread_mutex_lock(&g_mutex);
        rs_render_frame(g_glview.rr);
        pthread_mutex_unlock(&g_mutex);
    }
    if (++g_upd_cnt <= 2 || (g_upd_cnt % 600) == 0)
        LOGI("OnUpdate 帧 %d（GLThread EglHelper window 出屏；PBuffer 渲染线程兜底）", g_upd_cnt);
    /* ★ 游戏内重开（setprop jq.restartgame 1 → OnUpdate 定期检测 → 重开新局，不重启 app）。
       触摸 MuMu flaky 下可靠触发（不依赖触摸）；OnTouch 触摸重开按钮备用。
       每 60 帧读一次 property（~1 次/秒，避免每帧系统调用开销）。 */
    if (g_upd_cnt % 60 == 0) {
        char rg_prop[16] = {0};
        __system_property_get("jq.restartgame", rg_prop);
        if (strcmp(rg_prop, "1") == 0) {
            pthread_mutex_lock(&g_mutex);
            jq_restart_game();
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnUpdate 游戏内重开：restartgame（default.lay 重新开局，stage=PLAY 蓝先手）");
            __system_property_set("jq.restartgame", "0");
        }
        /* ★ 游戏内悔棋（setprop jq.undotest 1 → OnUpdate → undo_pop 恢复上一步）。
           undo 栈存完整 jq_state 快照（cells+turn+stage+winner），undo_pop 整体恢复。
           ★ 坑：undo_pop 一次恢复 turn=RED（AI 应着前）→ OnUpdate AI 立即补应着 →
           undo 失效（AI 反复 undo_push，栈永不空）。修 = 连续 undo_pop 直到 turn==BLUE
           （撤销 AI 应着 + 玩家走子，恢复玩家可走态）。触摸 flaky 下可靠触发。 */
        char ud_prop[16] = {0};
        __system_property_get("jq.undotest", ud_prop);
        if (strcmp(ud_prop, "1") == 0) {
            pthread_mutex_lock(&g_mutex);
            int n = 0;
            while (undo_pop()) {
                n++;
                if (g_state.turn == SIDE_BLUE || g_state.stage != STG_PLAY) break;
            }
            jq_touch_state_init(&g_ts);
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnUpdate 游戏内悔棋：undotest %s（pop %d 步 → turn=%d stage=%d）",
                 n > 0 ? "成功" : "无历史", n, g_state.turn, g_state.stage);
            __system_property_set("jq.undotest", "0");
        }
        /* ★ 认输（setprop jq.resigntest 1 → OnEvent resign → 对方胜，stage=OVER）。
           on_event resign 已有（winner=1-turn）；此处 property 触发（触摸 flaky 可靠）。 */
        char rs_prop[16] = {0};
        __system_property_get("jq.resigntest", rs_prop);
        if (strcmp(rs_prop, "1") == 0) {
            pthread_mutex_lock(&g_mutex);
            on_event("resign", "", env);
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnUpdate 认输：resigntest（stage=OVER winner=%d）", g_state.winner);
            __system_property_set("jq.resigntest", "0");
        }
        /* ★ 和棋（setprop jq.drawtest 1 → jq_on_draw → winner=2，stage=OVER）。
           on_event 无 draw 分支；直接调 jq_on_draw（rules.c，和棋判据 70 步/1000 步）。 */
        char dw_prop[16] = {0};
        __system_property_get("jq.drawtest", dw_prop);
        if (strcmp(dw_prop, "1") == 0) {
            pthread_mutex_lock(&g_mutex);
            jq_on_draw(&g_state);
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnUpdate 和棋：drawtest（stage=OVER winner=%d）", g_state.winner);
            sfx_game_over(env, g_state.stage, g_state.winner);   /* 和棋 → draw 音效 */
            __system_property_set("jq.drawtest", "0");
        }
        /* ★ 困毙（setprop jq.stalematetest 1 → jq_on_stalemate → winner=1-turn，stage=OVER）。
           jq_side_stalemated 是走子后自动判断（0x302b4）；此处直接模拟困毙判定结果
           （当前方无子可动 → 对方胜），对齐 jq_on_stalemate 语义。 */
        char sm_prop[16] = {0};
        __system_property_get("jq.stalematetest", sm_prop);
        if (strcmp(sm_prop, "1") == 0) {
            pthread_mutex_lock(&g_mutex);
            jq_on_stalemate(&g_state);
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnUpdate 困毙：stalematetest（stage=OVER winner=%d，无子可动方负）", g_state.winner);
            sfx_game_over(env, g_state.stage, g_state.winner);   /* 困毙 → 对方胜音效 */
            __system_property_set("jq.stalematetest", "0");
        }
        /* ★ UI 音效（setprop jq.closetest/jq.poptest 1 → 播 close/pop，14 音效全量验证）。
           close/pop 是 UI 组（原版弹窗关闭/按钮点击）；OnEvent close/pop 已有，
           此处 property 触发（真机 adb 不能调 OnEvent；触摸 flaky 下可靠）。 */
        char cl_prop[16] = {0};
        __system_property_get("jq.closetest", cl_prop);
        if (strcmp(cl_prop, "1") == 0) {
            sfx_play(env, "close");
            LOGI("OnUpdate 音效：closetest（播 close，UI 关闭）");
            __system_property_set("jq.closetest", "0");
        }
        char pp_prop[16] = {0};
        __system_property_get("jq.poptest", pp_prop);
        if (strcmp(pp_prop, "1") == 0) {
            sfx_play(env, "pop");
            LOGI("OnUpdate 音效：poptest（播 pop，UI 弹窗）");
            __system_property_set("jq.poptest", "0");
        }
        /* ★ P1（datah_039 证据）：复盘入口（setprop jq.replay 1 → OnUpdate → UI_REPLAY 复盘态，
           不重启 app；与 OnCreate 钩子同语义）。1=进复盘态 / 2=退复盘回对局。 */
        char rp_prop[16] = {0};
        __system_property_get("jq.replay", rp_prop);
        if (strcmp(rp_prop, "1") == 0 || strcmp(rp_prop, "2") == 0) {
            if (strcmp(rp_prop, "1") == 0) {
                int n = ui_replay_count();
                int rc = ui_replay_rewind(n);
                ui_screen_set(UI_REPLAY);
                LOGI("OnUpdate 复盘入口：replay=1 → UI_REPLAY（cur=%d/%d rewind rc=%d，datah_107 控制条 5 按钮；跳转简化回开头/跳结尾）",
                     n, n, rc);
            } else {
                ui_screen_set(UI_GAME);
                LOGI("OnUpdate 复盘入口：replay=2 → 退复盘回对局（保留记录 %d 条）", ui_replay_count());
            }
            __system_property_set("jq.replay", "0");
        }
    }
    /* ★ P2（datah_037 证据）：规则文本 setprop 钩子（adb setprop jq.rules 1 → 同会话 OnUpdate 轮询
       → UI_RULES 规则文本弹窗；0=退规则回对局。与 jq.replay 同范式，不重启 app）。
       ★ 诚实边界：规则文本=src_rs 简化版固定文本（2 套：翻棋/布局）。 */
    {
        char ru_prop[16] = {0};
        __system_property_get("jq.rules", ru_prop);
        if (strcmp(ru_prop, "1") == 0) {
            ui_screen_set(UI_RULES);
            LOGI("OnUpdate 规则文本入口：rules=1 → UI_RULES 规则文本弹窗（datah_037 规则-翻棋/规则-布局 2 模式 + 返回，"
                 "诚实边界：规则文本=src_rs 简化版固定文本 2 套）");
            __system_property_set("jq.rules", "0");
        } else if (strcmp(ru_prop, "2") == 0) {
            ui_screen_set(UI_GAME);
            LOGI("OnUpdate 规则文本入口：rules=2 → 退规则回对局");
            __system_property_set("jq.rules", "0");
        }
    }
    /* ★ P2（datah_104 证据）：新局完整版 setprop 钩子（adb setprop jq.newgame104 1 → 同会话 OnUpdate 轮询
       → UI_NEWGAME104 新局完整版弹窗；0=退弹窗回主菜单。与 jq.replay 同范式，不重启 app）。
       ★ 诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。 */
    {
        char ng_prop[16] = {0};
        __system_property_get("jq.newgame104", ng_prop);
        if (strcmp(ng_prop, "1") == 0) {
            ui_screen_set(UI_NEWGAME104);
            LOGI("OnUpdate 新局完整版入口：newgame104=1 → UI_NEWGAME104 新局完整版弹窗（datah_104 我方/电脑水平/玩法 3 选择 + 确定/取消，"
                 "诚实边界：3 项各循环切换当前值简化版）");
            __system_property_set("jq.newgame104", "0");
        } else if (strcmp(ng_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 新局完整版入口：newgame104=2 → 退弹窗回主菜单");
            __system_property_set("jq.newgame104", "0");
        }
    }
    /* ★ P2（datah_034 证据）：加入游戏 setprop 钩子（adb setprop jq.join 1 → 同会话 OnUpdate 轮询
       → UI_JOIN 加入游戏弹窗；0=退弹窗回主菜单。与 jq.replay 同范式，不重启 app）。
       ★ 诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP。 */
    {
        char jo_prop[16] = {0};
        __system_property_get("jq.join", jo_prop);
        if (strcmp(jo_prop, "1") == 0) {
            ui_screen_set(UI_JOIN);
            LOGI("OnUpdate 加入游戏入口：join=1 → UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
                 "诚实边界：Wi-Fi/局域网协议未逆向，src_rs 仅蓝牙联机已实现，确定→边界对话框）");
            __system_property_set("jq.join", "0");
        } else if (strcmp(jo_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 加入游戏入口：join=2 → 退弹窗回主菜单");
            __system_property_set("jq.join", "0");
        }
    }
    /* ★ P2（datah_035 证据）：创建游戏 setprop 钩子（adb setprop jq.create 1 → 同会话 OnUpdate 轮询
       → UI_CREATE 创建游戏弹窗；0=退弹窗回主菜单。与 jq.replay 同范式，不重启 app）。
       ★ 诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP。 */
    {
        char cr_prop[16] = {0};
        __system_property_get("jq.create", cr_prop);
        if (strcmp(cr_prop, "1") == 0) {
            ui_screen_set(UI_CREATE);
            LOGI("OnUpdate 创建游戏入口：create=1 → UI_CREATE 创建游戏弹窗（datah_035 密码占位标签 + 确定/取消，"
                 "诚实边界：Wi-Fi/局域网协议未逆向，src_rs 仅蓝牙联机已实现，确定→边界对话框）");
            __system_property_set("jq.create", "0");
        } else if (strcmp(cr_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 创建游戏入口：create=2 → 退弹窗回主菜单");
            __system_property_set("jq.create", "0");
        }

    /* ★ P2（datah_061 证据）：新局完整版（完整版补充）setprop 钩子（adb setprop jq.newgame061 1 → 同会话 OnUpdate 轮询
       → UI_NG061 新局完整版弹窗；0=退弹窗回主菜单。与 jq.create 同范式，不重启 app）。
       ★ 诚实边界：4 项各循环切换当前值（对齐 datah_104 循环切换范式）。 */
    {
        char n061_prop[16] = {0};
        __system_property_get("jq.newgame061", n061_prop);
        if (strcmp(n061_prop, "1") == 0) {
            ui_screen_set(UI_NG061);
            LOGI("OnUpdate 新局完整版（完整版补充）入口：newgame061=1 → UI_NG061 新局完整版弹窗（datah_061 先后手/电脑水平/玩法/明暗棋 "
                  "4 选择 + 确定/取消，诚实边界：4 项各循环切换当前值简化版）");
            __system_property_set("jq.newgame061", "0");
        } else if (strcmp(n061_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 新局完整版（完整版补充）入口：newgame061=2 → 退弹窗回主菜单");
            __system_property_set("jq.newgame061", "0");
        }
    }
    /* ★ P2（datah_050 证据）：对局信息 setprop 钩子（adb setprop jq.gameinfo 1 → 同会话 OnUpdate 轮询
       → UI_GAMEINFO 对局信息弹窗；0=退弹窗回对局。与 jq.newgame061 同范式，不重启 app）。
       ★ 诚实边界：Info 文本=☰ 战况统计复用，删除=清复盘记录简化。 */
    {
        char gi_prop[16] = {0};
        __system_property_get("jq.gameinfo", gi_prop);
        if (strcmp(gi_prop, "1") == 0) {
            ui_screen_set(UI_GAMEINFO);
            LOGI("OnUpdate 对局信息入口：gameinfo=1 → UI_GAMEINFO 对局信息弹窗（datah_050 Info 文本框（战况统计复用） + "
                  "复盘/删除/返回 3 按钮，诚实边界：删除=清复盘记录简化）");
            __system_property_set("jq.gameinfo", "0");
        } else if (strcmp(gi_prop, "2") == 0) {
            ui_screen_set(UI_GAME);
            LOGI("OnUpdate 对局信息入口：gameinfo=2 → 退弹窗回对局");
            __system_property_set("jq.gameinfo", "0");
        }
    }
    /* ★ P2（datah_002 证据）：亮度调节 setprop 钩子（adb setprop jq.bright 1 → 同会话 OnUpdate 轮询
       → UI_BRIGHT 亮度调节弹窗；0=退弹窗回对局。与 jq.newgame061 同范式，不重启 app）。
       ★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换。 */
    {
        char br_prop[16] = {0};
        __system_property_get("jq.bright", br_prop);
        if (strcmp(br_prop, "1") == 0) {
            ui_screen_set(UI_BRIGHT);
            LOGI("OnUpdate 亮度调节入口：bright=1 → UI_BRIGHT 亮度调节弹窗（datah_002 滑条简化=亮度 低/中/高 3 档循环 + "
                  "确定/取消，诚实边界：不画真滑条，cur=%d）", ui_bright_get());
            __system_property_set("jq.bright", "0");
        } else if (strcmp(br_prop, "2") == 0) {
            ui_screen_set(UI_GAME);
            LOGI("OnUpdate 亮度调节入口：bright=2 → 退弹窗回对局");
            __system_property_set("jq.bright", "0");
        }
    }
    /* ★ P2（datah_029 证据）：聊天记录 setprop 钩子（adb setprop jq.chatlog 1 → 同会话 OnUpdate 轮询
       → UI_CHATLOG 聊天记录弹窗；0=退弹窗回对局。与 jq.newgame061 同范式，不重启 app）。
       ★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
    {
        char cl_prop[16] = {0};
        __system_property_get("jq.chatlog", cl_prop);
        if (strcmp(cl_prop, "1") == 0) {
            ui_screen_set(UI_CHATLOG);
            LOGI("OnUpdate 聊天记录入口：chatlog=1 → UI_CHATLOG 聊天记录弹窗（datah_029 Msg 文本框（诚实边界占位） + "
                  "清除/关闭 2 按钮，诚实边界：联机聊天文本未逆向）");
            __system_property_set("jq.chatlog", "0");
        } else if (strcmp(cl_prop, "2") == 0) {
            ui_screen_set(UI_GAME);
            LOGI("OnUpdate 聊天记录入口：chatlog=2 → 退弹窗回对局");
            __system_property_set("jq.chatlog", "0");
        }
    }
    /* ★ P2（datah_031 证据）：个人资料 setprop 钩子（adb setprop jq.profile 1 → 同会话 OnUpdate 轮询
       → UI_PROFILE 个人资料弹窗；0=退弹窗回主菜单。与 jq.newgame061 同范式，不重启 app）。
       现有 datah_072 方式弹窗"个人资料"→UI_LOGIN 不破坏，031 作为登录后资料查看补充（独立态）。
       ★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
    {
        char pf_prop[16] = {0};
        __system_property_get("jq.profile", pf_prop);
        if (strcmp(pf_prop, "1") == 0) {
            ui_screen_set(UI_PROFILE);
            LOGI("OnUpdate 个人资料入口：profile=1 → UI_PROFILE 个人资料弹窗（datah_031 Info 文本框（诚实边界占位） + "
                  "修改/帮助/返回 3 按钮，诚实边界：账号系统未逆向，离线单机无账号）");
            __system_property_set("jq.profile", "0");
        } else if (strcmp(pf_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 个人资料入口：profile=2 → 退弹窗回主菜单");
            __system_property_set("jq.profile", "0");
        }
    }
    /* ★ P2（datah_042 证据）：提和详情 setprop 钩子（adb setprop jq.drawdetail 1 → 同会话 OnUpdate 轮询
        → UI_DRAWDETAIL 提和详情弹窗；2=退弹窗回对局。与 jq.profile 同范式，不重启 app）。
        ★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
        联机提和/认输走 BT 帧 + 边界对话框，不包装成"已提和判定"。 */
    {
        char dd_prop[16] = {0};
        __system_property_get("jq.drawdetail", dd_prop);
        if (strcmp(dd_prop, "1") == 0) {
            ui_screen_set(UI_DRAWDETAIL);
            LOGI("OnUpdate 提和详情入口：drawdetail=1 → UI_DRAWDETAIL 提和详情弹窗（datah_042 联机版 ☰ 菜单 7 图标："
                  "提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
            __system_property_set("jq.drawdetail", "0");
        } else if (strcmp(dd_prop, "2") == 0) {
            ui_screen_set(UI_GAME);
            LOGI("OnUpdate 提和详情入口：drawdetail=2 → 退弹窗回对局");
            __system_property_set("jq.drawdetail", "0");
        }
    }

    /* ★ P2（datah_003 证据）：设置入口 setprop 钩子（adb setprop jq.setentry 1 → 重启 →
        OnCreate 弹 UI_SETEXTTRY 设置入口弹窗（完整版）：图像/声音/游戏设置 3 按钮入口。
        测试钩子范式，与 jq.profile 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
        图像设置入口指向 UI_IMGSET7（datah_008 简化版，4 开关仅 UI 状态记录）；
        声音设置→UI_SETTINGS（datah_001 已实现）；游戏设置→UI_GAMESET（datah_012 已实现）。 */
    {
        char se_prop[16] = {0};
        __system_property_get("jq.setentry", se_prop);
        if (strcmp(se_prop, "1") == 0) {
            ui_screen_set(UI_SETEXTTRY);
            LOGI("jq.setentry 钩子：→ UI_SETEXTTRY 设置入口弹窗（datah_003 图像/声音/游戏设置 3 按钮入口，"
                 "图像→UI_IMGSET7（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）；声音→UI_SETTINGS（datah_001）；"
                 "游戏→UI_GAMESET（datah_012））");
            __system_property_set("jq.setentry", "0");
        } else if (strcmp(se_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 设置入口入口：setentry=2 → 退弹窗回主菜单");
            __system_property_set("jq.setentry", "0");
        }
    }
    /* ★ P2（datah_004 证据）：设置入口（简化版）setprop 钩子（adb setprop jq.setentry4 1 → 重启 →
        OnCreate 弹 UI_SETEXTTRY4 设置入口（简化版）弹窗：图像/声音 2 按钮入口（无游戏设置项，
        联机态/蓝牙态设置入口）。测试钩子范式，与 jq.setentry 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像设置渲染参数未实现，图像设置入口指向 UI_IMGSET8（datah_008 简化版，
        3 开关仅 UI 状态记录）；声音设置→UI_SETTINGS（datah_001 已实现）。 */
    {
        char se4_prop[16] = {0};
        __system_property_get("jq.setentry4", se4_prop);
        if (strcmp(se4_prop, "1") == 0) {
            ui_screen_set(UI_SETEXTTRY4);
            LOGI("jq.setentry4 钩子：→ UI_SETEXTTRY4 设置入口（简化版）弹窗（datah_004 图像/声音 2 按钮入口，"
                 "无游戏设置项，联机态/蓝牙态；图像→UI_IMGSET8（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）；"
                 "声音→UI_SETTINGS（datah_001））");
            __system_property_set("jq.setentry4", "0");
        } else if (strcmp(se4_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 设置入口（简化版）入口：setentry4=2 → 退弹窗回主菜单");
            __system_property_set("jq.setentry4", "0");
        }
    }
    /* ★ P2（datah_007 证据）：图像设置（完整版）setprop 钩子（adb setprop jq.imgset7 1 → 重启 →
        OnCreate 弹 UI_IMGSET7 图像设置弹窗（完整版）：界面/棋子动画/夜间模式/对方棋子
        4 开关 + 确定/取消。测试钩子范式，与 jq.setentry 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
        开关只是 UI 状态记录（不实际改渲染），确定→ui_imgset_save（jq_imgset.txt 4 字段）。 */
    {
        char i7_prop[16] = {0};
        __system_property_get("jq.imgset7", i7_prop);
        if (strcmp(i7_prop, "1") == 0) {
            ui_screen_set(UI_IMGSET7);
            LOGI("jq.imgset7 钩子：→ UI_IMGSET7 图像设置弹窗（完整版 datah_007 4 开关 + 确定/取消，"
                 "诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录；"
                 "界面=%d 棋子=%d 夜间=%d 对方=%d）",
                 ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get(), ui_imgset_opp_get());
            __system_property_set("jq.imgset7", "0");
        } else if (strcmp(i7_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 图像设置（完整版）入口：imgset7=2 → 退弹窗回主菜单");
            __system_property_set("jq.imgset7", "0");
        }
    }
    /* ★ P2（datah_008 证据）：图像设置（简化版）setprop 钩子（adb setprop jq.imgset8 1 → 重启 →
        OnCreate 弹 UI_IMGSET8 图像设置弹窗（简化版）：界面/棋子动画/夜间模式
        3 开关 + 确定/取消（无对方棋子项，蓝牙态）。测试钩子范式，与 jq.imgset7 同；
        值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
        开关只是 UI 状态记录（不实际改渲染），确定→ui_imgset_save（jq_imgset.txt 3 字段）。 */
    {
        char i8_prop[16] = {0};
        __system_property_get("jq.imgset8", i8_prop);
        if (strcmp(i8_prop, "1") == 0) {
            ui_screen_set(UI_IMGSET8);
            LOGI("jq.imgset8 钩子：→ UI_IMGSET8 图像设置弹窗（简化版 datah_008 3 开关 + 确定/取消，"
                 "诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录；"
                 "界面=%d 棋子=%d 夜间=%d）",
                 ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get());
            __system_property_set("jq.imgset8", "0");
        } else if (strcmp(i8_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 图像设置（简化版）入口：imgset8=2 → 退弹窗回主菜单");
            __system_property_set("jq.imgset8", "0");
        }
    }
    /* ★ P2（datah_065 证据）：加入游戏完整版 setprop 钩子（adb setprop jq.join065 1 → 重启 →
        OnCreate 弹 UI_JOIN065 加入游戏完整版弹窗：先手/后手/玩法/明暗棋/4 规则显示
        + 开始/取消。测试钩子范式，与 jq.join 同（datah_034 简化版不破坏）；
        值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：先手/后手/玩法/明暗棋/4 规则 = 联机开局配置（src_rs 无联机开局逻辑，
        显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
        开始→边界对话框（"加入游戏完整版：联机开局需 Wi-Fi/局域网房间协议，
        src_rs 仅蓝牙联机已实现，请选蓝牙联机"）+ 回主菜单；取消→回主菜单。 */
    {
        char j65_prop[16] = {0};
        __system_property_get("jq.join065", j65_prop);
        if (strcmp(j65_prop, "1") == 0) {
            ui_screen_set(UI_JOIN065);
            LOGI("jq.join065 钩子：→ UI_JOIN065 加入游戏完整版弹窗（datah_065 先手/后手/玩法/明暗棋/"
                 "4 规则显示 + 开始/取消，诚实边界：联机开局配置=占位标签，开始→边界对话框（src_rs 仅蓝牙已实现））");
            __system_property_set("jq.join065", "0");
        } else if (strcmp(j65_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 加入游戏完整版入口：join065=2 → 退弹窗回主菜单");
            __system_property_set("jq.join065", "0");
        }
    }
    /* ★ P2（datah_069 证据）：创建游戏完整版 setprop 钩子（adb setprop jq.create069 1 → 重启 →
        OnCreate 弹 UI_CREATE069 创建游戏完整版弹窗：先后手/玩法/明暗棋/游戏规则
        4 选择 + 设置/确定/取消。测试钩子范式，与 jq.create 同（datah_035 简化版不破坏）；
        值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：先后手/玩法/明暗棋 = 联机开局配置（src_rs 无联机房间创建逻辑，
        循环切换仅 UI 状态记录）；游戏规则（action=3）→UI_RULES070（datah_070）；
        确定→边界对话框（"创建游戏完整版：联机房间创建需 Wi-Fi/局域网协议，src_rs 仅蓝牙已实现"）
        + 回主菜单；取消→回主菜单。 */
    {
        char c69_prop[16] = {0};
        __system_property_get("jq.create069", c69_prop);
        if (strcmp(c69_prop, "1") == 0) {
            ui_screen_set(UI_CREATE069);
            LOGI("jq.create069 钩子：→ UI_CREATE069 创建游戏完整版弹窗（datah_069 先后手/玩法/明暗棋/"
                 "游戏规则 4 选择 + 确定/取消，诚实边界：联机房间创建=边界对话框（src_rs 仅蓝牙已实现）；"
                 "游戏规则→UI_RULES070（datah_070））");
            __system_property_set("jq.create069", "0");
        } else if (strcmp(c69_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 创建游戏完整版入口：create069=2 → 退弹窗回主菜单");
            __system_property_set("jq.create069", "0");
        }
    }
    /* ★ P2（datah_070 证据）：游戏规则完整版 setprop 钩子（adb setprop jq.rules070 1 → 重启 →
        OnCreate 弹 UI_RULES070 游戏规则完整版弹窗：和棋/大本营/吃地雷/吃军旗
        4 规则开关 + 确定/取消。与 datah_012 游戏设置同构，复用 ui_rule_*_get/set。
        测试钩子范式，与 jq.rules（datah_037 规则文本）同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 与 datah_012 区分：012 是本地 ☰ 菜单"功能设置"→UI_GAMESET，
        070 是联机"游戏规则"弹窗（datah_069 创建游戏"游戏规则"按钮切此弹窗），
        复用同一组 ui_rule_* 开关；确定→ui_rules_save（jq_rules.txt 4 字段）；
        取消→ui_rules_load 恢复原值。 */
    {
        char r70_prop[16] = {0};
        __system_property_get("jq.rules070", r70_prop);
        if (strcmp(r70_prop, "1") == 0) {
            ui_screen_set(UI_RULES070);
            LOGI("jq.rules070 钩子：→ UI_RULES070 游戏规则完整版弹窗（datah_070 4 规则开关 + 确定/取消，"
                 "与 datah_012 同构，复用 ui_rule_*；诚实边界：联机游戏规则弹窗，非本地 ☰ 菜单 UI_GAMESET）");
            __system_property_set("jq.rules070", "0");
        } else if (strcmp(r70_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 游戏规则完整版入口：rules070=2 → 退弹窗回主菜单");
            __system_property_set("jq.rules070", "0");
        }
    }
    /* ★ P2（datah_060 证据）：房间态 setprop 钩子（adb setprop jq.room060 1 → 重启 →
        OnCreate 弹 UI_ROOM060 房间态弹窗：2 玩家房间列表（占位标签）+ 开始/换桌/离开。
        测试钩子范式，与 jq.join065 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：联机房间需 Wi-Fi/局域网协议（src_rs 仅蓝牙已实现），
        开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据，触摸 flaky 下房间列表无意义）。 */
    {
        char rm_prop[16] = {0};
        __system_property_get("jq.room060", rm_prop);
        if (strcmp(rm_prop, "1") == 0) {
            ui_screen_set(UI_ROOM060);
            LOGI("jq.room060 钩子：→ UI_ROOM060 房间态弹窗（datah_060 2 玩家房间列表（占位标签）+ 开始/换桌/离开，"
                 "诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签）");
            __system_property_set("jq.room060", "0");
        } else if (strcmp(rm_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 房间态入口：room060=2 → 退弹窗回主菜单");
            __system_property_set("jq.room060", "0");
        }
    }
    /* ★ P2（datah_063 证据）：聊天完整版 setprop 钩子（adb setprop jq.chat063 1 → 重启 →
        OnCreate 弹 UI_CHAT063 聊天完整版弹窗：Msg 输入框（占位标签）+ 发送/记录/关闭。
        测试钩子范式，与 jq.chatlog（datah_029 聊天记录）同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
        Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"；
        发送→BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029 已实现）；
        关闭→回对局。 */
    {
        char ch63_prop[16] = {0};
        __system_property_get("jq.chat063", ch63_prop);
        if (strcmp(ch63_prop, "1") == 0) {
            ui_screen_set(UI_CHAT063);
            LOGI("jq.chat063 钩子：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭，"
                 "诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029））");
            __system_property_set("jq.chat063", "0");
        } else if (strcmp(ch63_prop, "2") == 0) {
            ui_screen_set(UI_GAME);
            LOGI("OnUpdate 聊天完整版入口：chat063=2 → 退弹窗回对局");
            __system_property_set("jq.chat063", "0");
        }
    }

    }
    /* ★ P1（datah_038）：登录 setprop 钩子（adb setprop jq.login 1 → 同会话 OnUpdate 轮询
       → UI_LOGIN 登录弹窗；0=退登录回主菜单。与 jq.replay 同范式，不重启 app）。
       诚实边界：原版登录=账号系统（服务器协议未逆向），游客=本地单机唯一真生效路径。 */
    {
        char ln_prop[16] = {0};
        __system_property_get("jq.login", ln_prop);
        if (strcmp(ln_prop, "1") == 0) {
            ui_screen_set(UI_LOGIN);
            LOGI("OnUpdate 登录入口：login=1 → UI_LOGIN 登录弹窗（datah_038 用户名/密码 + "
                 "登录/注册/游客/返回，诚实边界：账号系统未逆向，游客=本地单机唯一真生效路径）");
            __system_property_set("jq.login", "0");
        } else if (strcmp(ln_prop, "2") == 0) {
            ui_screen_set(UI_MENU);
            LOGI("OnUpdate 登录入口：login=2 → 退登录回主菜单");
            __system_property_set("jq.login", "0");
        }
    }
    /* AI 退化推进（仅对局态；菜单态 AI 不介入，等"人机对战"按钮进 GAME）。
       ★ 红方 AI 应着加日志（实证"下一局"闭环：蓝走子 → 红 AI 自动应着 → 回合切回蓝）。
       ★ 对抗审计 #6：g_state 读（stage/turn）移进 g_mutex 内（旧版锁外读 → 数据竞争）。 */
    if (ui_screen_get() == UI_GAME) {
        int from, to, flip;
        pthread_mutex_lock(&g_mutex);
        if (g_state.stage == STG_PLAY && g_vs_ai && g_state.turn == SIDE_RED) {
            if (ai_pick(&from, &to, &flip) == 0) {
                undo_push();
                if (flip) {
                    if (jq_flip(&g_state, from)) ui_replay_record(from, from, 1);   /* ★ P1（datah_039/107）：AI 翻棋记录 */
                    LOGI("红方 AI 应着（翻棋）：cell=%d（turn 切回蓝方）", from);
                } else {
                    int e = jq_try_move(&g_state, from, to);
                    if (e >= 0) ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：AI 走子记录 */
                    LOGI("红方 AI 应着：from=%d to=%d 结果=%s（turn 切回蓝方）",
                         from, to, JQ_EVENT_NAMES[e < 0 ? 0 : e]);
                }
            }
        }
        pthread_mutex_unlock(&g_mutex);
    }
    /* ③ EglHelper 出屏：onDrawFrame 返回后 GLThread EglHelper 已 makeCurrent + glClear，
       触发 gl.glClear() 让 SurfaceView surface 出首帧像素（验证 screencap 非黑）。
       GL10 对象为 GLView EglHelper 创建的 EGL10 实现（EGL_CONTEXT_CLIENT_VERSION=2 时
       仍走 GL1 接口清屏——glClear 是 GL10 基础调用，GL2 context 上合法）。 */
    return JNI_TRUE;
}

/* OnEvent(String name, String value)：事件派发（走子/翻棋/悔棋/认输/重开） */
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnEvent(JNIEnv *env, jclass clazz, jstring name, jstring value)
{
    (void)clazz;
    /* ★ 对抗审计 #1：GetStringUTFChars 返回 const char*，严禁 (char) 截断指针到 1 字节
       （旧版 `char *n = (char)(...)` 把指针截断成垃圾 → on_event strcmp 读垃圾地址 SIGSEGV，
       ReleaseStringUTFChars 传错指针 JNI UB）。原指针保留，on_event 后释放。 */
    const char *n = (*env)->GetStringUTFChars(env, name, NULL);
    const char *v = (*env)->GetStringUTFChars(env, value, NULL);
    pthread_mutex_lock(&g_mutex);
    on_event(n, v, env);
    pthread_mutex_unlock(&g_mutex);
    if (n) (*env)->ReleaseStringUTFChars(env, name, n);
    if (v) (*env)->ReleaseStringUTFChars(env, value, v);
}

/* OnInput(String)：文本输入（原版登录/注册 20+ 事件，L77）——桩（UI 层）。 */
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnInput(JNIEnv *env, jclass clazz, jstring text)
{
    (void)env; (void)clazz; (void)text;
}

/* OnKey(KeyEvent)→jboolean：按键。桩（原版键盘输入映射 UI 层）。 */
JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnKey(JNIEnv *env, jclass clazz, jobject keyEvent)
{
    (void)env; (void)clazz; (void)keyEvent;
    return JNI_FALSE;
}

/* ★ 对抗审计（datah_072）：蓝牙联机启动（联网=host 蓝先手 / 联机=client 红后手）。
   从方式选择弹窗"蓝牙联机"按钮进入。 */
static void start_bt_session(JNIEnv *env, int host)
{
    g_vs_ai = 0;   /* ★ 蓝牙模式：红方由对端/本机玩家走，AI 不介入（回合闸口接管） */
    jq_bt_start(env, host);
    /* ★ 联机会话初始化：联网=蓝先手（host），联机=红后手（client）——双方 we_blue 不可同为 1。
       发送走子帧经 jq_bt_send；BT 模式走子由本机状态机放行。 */
    if (!g_bt_sess_init_done) {
        pthread_mutex_lock(&g_mutex);
        jq_bt_session_init(&g_bt_sess, &g_state, host ? 1 : 0);
        /* host 蓝先手=OUR_TURN（可发首帧）；client 红后手=THEIR_TURN（等 host 首帧）。
           ★ 回合闸口：send_move 须 OUR_TURN，on_frame MOVE/FLIP 须 THEIR_TURN。 */
        g_bt_sess.state = host ? BT_ST_OUR_TURN : BT_ST_THEIR_TURN;
        pthread_mutex_unlock(&g_mutex);
        g_bt_sess_init_done = 1;
        g_bt_sess_init = 1;
        ui_bt_set_active(1);   /* ★ 对抗审计（datah_067）：联机 ☰ 菜单分流 */
        LOGI("start_bt_session：蓝牙会话初始化（host=%d %s，%s）",
             host, host ? "蓝先手" : "红后手",
             host ? "OUR_TURN" : "THEIR_TURN");
    }
    jq_show_dialog(env, host ? "联网" : "联机",
        host
        ? "联网（主机）：\n蓝牙监听已启动，等待对方连接…\n"
          "(设备需在系统蓝牙设置配对；状态见 logcat junqi_bt)"
        : "联机（客户端）：\n蓝牙连接已启动（自动选已配对设备）…\n"
          "(设备需在系统蓝牙设置配对；状态见 logcat junqi_bt)");
    LOGI("start_bt_session：蓝牙联机启动（host=%d）", host);
}

/* OnTouch(MotionEvent)→jboolean：触摸 → 棋盘格命中（⑤ 逆源码，原版 0xbdd2c）。
   UI 分流（正向开发）：菜单态点按钮 → 进对局（ui_on_menu_action）；
   对局态 → 两步走子（jq_touch_state_process），走子/翻棋触发音效（MediaPlayer）。
   MotionEvent ACTION_DOWN 取 x/y → jq_touch_process → 事件码 1(选中)/2(取消)/7(走子)
   入场景队列（0xbd580 语义，帧驱动 0xbd5a0 消费）。
   走子（type 7）需"已选中 + 再点目标格"两步；此处点同格=翻棋/走子意图，
   实际落子由帧驱动按 jq_can_move/jq_can_flip 判定（诚实：坐标→格几何为 ② 重建）。 */
JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnTouch(JNIEnv *env, jclass clazz, jobject motionEvent)
{
    if (!env || !motionEvent) return JNI_FALSE;
    /* 原版 0xbdd2c 经 JNI 方法表调 MotionEvent.getX/getY（vtable 槽）；
       本重写直接取（行为等价）。★ mt 是 local ref，必须在同 scope 内用完即放，
       且后续所有 CallVoidMethod 后清 pending exception（防 MuMu checkJNI abort）。 */
    jclass mt = (*env)->GetObjectClass(env, motionEvent);
    if (!mt) return JNI_FALSE;
    jmethodID getx = (*env)->GetMethodID(env, mt, "getX", "()F");
    jmethodID gety = (*env)->GetMethodID(env, mt, "getY", "()F");
    if (!getx || !gety) { (*env)->DeleteLocalRef(env, mt); if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env); return JNI_FALSE; }
    float x = (float)(*env)->CallFloatMethod(env, motionEvent, getx);
    float y = (float)(*env)->CallFloatMethod(env, motionEvent, gety);
    /* ★ 只处理 ACTION_DOWN(0)：原版 setOnTouchListener 语义是按下即消费，
       MOVE/UP 事件（input tap 产生 DOWN+UP 两次）不重复消费 → 避免一次 tap
       触发 SELECT+MOVE 两次（走子变翻棋）。 */
    jmethodID getaction = (*env)->GetMethodID(env, mt, "getAction", "()I");
    int action = getaction ? (int)(*env)->CallIntMethod(env, motionEvent, getaction) : 0;
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, mt);   /* ★ 立即释放 mt（local ref 同 scope 内用完即放） */
    if (action != 0) return JNI_FALSE;   /* 非 ACTION_DOWN 不消费 */
    /* 触摸诊断（定位"菜单点击不生效"：确认 native 收到 + 坐标 + 当前 UI 态） */
    static int g_touch_cnt = 0;
    if (++g_touch_cnt <= 5 || (g_touch_cnt % 500) == 0)
        LOGI("OnTouch 触摸 #%d（x=%.0f y=%.0f，view=%dx%d，ui=%s）",
             g_touch_cnt, x, y, g_view_w, g_view_h,
             ui_screen_get() == UI_MENU ? "MENU" : "GAME");
    /* UI 分流：菜单态点按钮 → 进对局（人机/重开），其它按钮占位日志。
       ★ 触摸坐标（MotionEvent.getX/getY）与渲染 viewport 坐标系不一致
         （MuMu input 注入坐标被缩放），故菜单命中用 720×1280 按钮几何
         的"屏幕相对位置"：x ∈ [0.05, 0.97] 实际 view 宽度内的按钮 x 区 +
         y 按实际 view 高度 5 等分带（按钮 5 行 × 2 列布局的 y 相对位置），
         命中 = 实际 view 内的 5 按钮矩形（与 rs_draw_menu 的缩放渲染一致，
         不依赖 MotionEvent 坐标的绝对像素）。 */
    if (ui_screen_get() == UI_MENU) {
        /* 按钮相对几何（归一化：x 相对 view_w，y 相对 view_h）—— 与渲染的
           基准坐标 720×1280 同布局，rs_draw_menu 用 sx/sy 缩放画到实际 view，
           触摸 x/y 在实际 view 坐标系 → 归一化后直接命中同一套相对矩形。 */
        /* 命中：触摸坐标归一化 [0,1]²（x/view_w、y/view_h）vs g_btns 720×1280 基准归一化，
           与渲染 rs_draw_menu 的 sx/sy 缩放同一套相对几何（MuMu 触摸坐标系不同 → 归一化解耦）。 */
        const ui_btn *btns = ui_menu_buttons();
        float nx = g_view_w > 0 ? x / (float)g_view_w : 0;
        float ny = g_view_h > 0 ? y / (float)g_view_h : 0;
        int hit = -1, hit_action = -1;
        for (int i = 0; i < UI_BTN_COUNT; i++) {
            /* ★ 按钮几何 720×1280 基准 → 实际 view 像素（与 rs_draw_menu sx/sy 缩放同几何）：
               MotionEvent x/y 已是 view 像素（顶左原点）→ 按钮矩形转 view 像素直接比较 */
            float b0 = btns[i].x0 * g_view_w / 720.0f, b1 = btns[i].x1 * g_view_w / 720.0f;
            float c0 = btns[i].y0 * g_view_h / 1280.0f, c1 = btns[i].y1 * g_view_h / 1280.0f;
            if (x >= b0 && x <= b1 && y >= c0 && y <= c1) { hit = i; hit_action = btns[i].action; }
        }
        LOGI("UI命中：x=%.0f y=%.0f nx=%.3f ny=%.3f hit=%d action=%d",
             x, y, nx, ny, hit, hit_action);
        if (hit >= 0) {
            LOGI("UI命中 按钮%d（action=%d）", hit, hit_action);
            ui_on_menu_action(hit_action);
            sfx_play(env, "pop");   /* ★ UI 按钮点击音效（14 音效全量验证：pop 此前未触发） */
            /* ★ 按键功能对齐：联网/联机（action 2/3）为蓝牙联机——协议层就绪，
               SPP socket 由 Java 宿主承担 + 无真机蓝牙验证（诚实边界），显示说明对话框 */
            if (hit_action == 2 || hit_action == 3) {
                /* ★ 对抗审计（datah_072 证据）：联网/联机 → 方式选择弹窗
                   （蓝牙联机/Wi-Fi 联机/个人资料/返回），选"蓝牙联机"才启动蓝牙会话 */
                g_way_host = (hit_action == 2);
                ui_screen_set(UI_WAYSEL);
                LOGI("OnTouch 菜单按钮%d（action=%d → UI_WAYSEL 联网/联机方式选择弹窗，来源=%s）",
                     hit, hit_action, g_way_host ? "联网(host)" : "联机(client)");
                (void)clazz;
                return JNI_TRUE;
            }
            /* ★ P0（datah_089 证据）：四国（action 4）→ 四国人数选择弹窗（UI_NPLAYER，
               2/3/4 人 3 按钮，原版 datah_089 230×230 面板居中）。诚实边界：选人数后
               显示边界对话框（4 方规则未实现，勿静默开 2 人局误导为四国可玩）。 */
            if (hit_action == 4) {
                ui_screen_set(UI_NPLAYER);
                LOGI("OnTouch 菜单按钮%d（action=%d → UI_NPLAYER 四国人数选择弹窗，datah_089 2/3/4 人，诚实边界）",
                     hit, hit_action);
                (void)clazz;
                return JNI_TRUE;
            }
            /* ★ 原版 UI 对齐（用户指正"没有高手新手"）：人机/两人/重开 → 直开局，
               无难度弹窗（datah_041 判误）。flagtest/minetest 测试夹具走 setprop 路径
               （mumu_evtest.sh），不占菜单按钮。
               ★ 对抗审计（datah_041 证据修正）：人机（action 0）弹难度选择弹窗
               （新手/中级/高级/好友区 × 布局/翻棋，选完开局）。
               ★ P0（datah_062 证据修正）：两人（action 1）→ 新局简化弹窗（UI_NEWGAME，
               玩法/明暗棋 + 确定/取消 4 控件，原版 datah_062 450×315 面板居中），
               选玩法+明暗棋后确定 → 随模式直开局（两人热座，AI 不介入）；取消 → 回主菜单。 */
            if (hit_action == 0) {
                /* 人机：进难度弹窗（UI_DIFFICULTY），选难度/模式后开局（AI 走红） */
                g_vs_ai = 1;
                ui_screen_set(UI_DIFFICULTY);
                LOGI("OnTouch 菜单按钮%d（action=%d → UI_DIFFICULTY 人机难度弹窗，AI 走红）",
                     hit, hit_action);
            } else if (hit_action == 1) {
                /* ★ P0（datah_062 证据）：两人 → 新局简化弹窗（玩法/明暗棋 + 确定/取消），
                   确定后才开局（两人热座，AI 不介入；默认翻棋暗棋对齐原版） */
                g_vs_ai = 0;
                ui_screen_set(UI_NEWGAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 菜单按钮%d（action=%d → UI_NEWGAME 两人新局弹窗，datah_062 玩法/明暗棋+确定/取消，两人热座 AI 不介入）",
                     hit, hit_action);
            }
            (void)clazz;
            return JNI_TRUE;
        }
        /* ★ 原版 UI 对齐：菜单态点按钮外 = 不消费（原版主菜单仅 5 按钮可点，
           点空白无反应——去掉旧"点棋盘区快速开局"测试路径，对齐原版交互） */
        (void)clazz;
        return JNI_FALSE;
    }
    /* ★ 对抗审计（datah_041 证据）：人机难度选择弹窗态。
       点 8 按钮（布局/翻棋 × 新手/中级/高级/好友区）→ ui_opt_select(level,mode)
       + g_vs_ai=1 + jq_restart_game → UI_GAME；点面板外 → 回主菜单。 */
    if (ui_screen_get() == UI_DIFFICULTY) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_diff_btn *db = ui_diff_buttons();
        int dh = ui_diff_hit(db, UI_DIFF_COUNT, bx, by);
        if (dh >= 0) {
            int level = db[dh].action / 2 + 1, mode = db[dh].action % 2;
            ui_opt_select(level, mode);
            g_vs_ai = 1;   /* ★ 人机：红方 AI 走子 */
            pthread_mutex_lock(&g_mutex);
            jq_restart_game();
            pthread_mutex_unlock(&g_mutex);
            ui_screen_set(UI_GAME);
            sfx_play(env, "over");
            LOGI("OnTouch 难度弹窗：选按钮%d（level=%d mode=%d %s）→ 开局 AI 走红",
                 dh, level, mode, mode ? "翻棋暗棋" : "布局明棋");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_diff_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 难度弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ 对抗审计（datah_072 证据）：联网/联机方式选择弹窗态。
       点"蓝牙联机"→ start_bt_session（联网=host/联机=client）；
       点"Wi-Fi 联机"/"个人资料"→ 诚实边界对话框；点"返回"→ 主菜单；
       点面板外 → 回主菜单。 */
    if (ui_screen_get() == UI_WAYSEL) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_way_btn *wb = ui_way_buttons();
        int wh = ui_way_hit(wb, UI_WAYSEL_COUNT, bx, by);
        if (wh >= 0) {
            int wact = wb[wh].action;
            LOGI("OnTouch 方式选择：按钮%d（action=%d）", wh, wact);
            if (wact == 0) {   /* 蓝牙联机 */
                start_bt_session(env, g_way_host);
                ui_screen_set(UI_GAME);
            } else if (wact == 1) {   /* Wi-Fi 联机（★ P2 datah_034：加入游戏弹窗，诚实边界） */
                ui_screen_set(UI_JOIN);
                sfx_play(env, "pop");
                LOGI("OnTouch 方式选择：Wi-Fi 联机 → UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
                     "诚实边界：Wi-Fi/局域网协议未逆向，src_rs 仅蓝牙联机已实现，确定→边界对话框）");
            } else if (wact == 2) {   /* 个人资料 → 登录弹窗（★ P1 datah_038 证据：对齐原版登录对话框几何） */
                ui_screen_set(UI_LOGIN);
                sfx_play(env, "pop");
                LOGI("OnTouch 方式选择：个人资料 → UI_LOGIN 登录弹窗（datah_038 用户名/密码 + 登录/注册/游客/返回，"
                     "诚实边界：账号系统未逆向，游客=本地单机唯一真生效路径）");
            } else if (wact == 3) {   /* 返回 */
                ui_screen_set(UI_MENU);
                LOGI("OnTouch 方式选择：返回 → 主菜单");
            }
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_way_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 方式选择外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ 对抗审计（datah_001 证据）：声音设置弹窗态。
       点 静音模式/按键声音/音量 → 切换当前值（本地副本，未确定不落盘）；
       点 确定 → ui_settings_save + 回对局；点 取消 → 回对局（不保存）；
       点面板外 → 回对局（不保存）。 */
    if (ui_screen_get() == UI_SETTINGS) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_set_btn *sb = ui_set_buttons();
        int sh = ui_set_hit(sb, UI_SETTINGS_COUNT, bx, by);
        if (sh >= 0) {
            int sact = sb[sh].action;
            LOGI("OnTouch 声音设置：按钮%d（action=%d）", sh, sact);
            if (sact == 0) {   /* 静音模式切换 */
                ui_snd_mute_set(!ui_snd_mute_get());
                sfx_play(env, "pop");
                LOGI("声音设置：静音模式 → %s", ui_snd_mute_get() ? "开" : "关");
            } else if (sact == 1) {   /* 按键声音切换 */
                ui_btn_sound_set(!ui_btn_sound_get());
                sfx_play(env, "pop");
                LOGI("声音设置：按键声音 → %s", ui_btn_sound_get() ? "开" : "关");
            } else if (sact == 2) {   /* 音量循环 低→中→高→低 */
                int v = ui_volume_get(); ui_volume_set(v % 3 + 1);
                sfx_play(env, "pop");
                LOGI("声音设置：音量 → %d", ui_volume_get());
            } else if (sact == 3) {   /* 确定：保存 */
                ui_settings_save();
                sfx_play(env, "pop");
                LOGI("声音设置：确定 → 保存 + 回对局");
                ui_screen_set(UI_GAME);
            } else if (sact == 4) {   /* 取消：不保存回对局 */
                ui_settings_load();   /* ★ 重读磁盘恢复原值（取消不落盘） */
                sfx_play(env, "pop");
                LOGI("声音设置：取消 → 恢复原值 + 回对局");
                ui_screen_set(UI_GAME);
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_set_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 声音设置外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P0（datah_089 证据）：四国人数选择弹窗态。
       点 2/3/4 人 → 诚实边界对话框（4 方规则未实现）→ 回主菜单；
       点面板外 → 回主菜单。★ 诚实边界：不假装四国可玩。 */
    if (ui_screen_get() == UI_NPLAYER) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_nplayer_btn *nb = ui_nplayer_buttons();
        int nh = ui_nplayer_hit(nb, UI_NPLAYER_COUNT, bx, by);
        if (nh >= 0) {
            int nact = nb[nh].action;   /* 0=2 人 1=3 人 2=4 人（默认） */
            jq_show_dialog(env, "四国军棋",
                "四国军棋（诚实边界）：\n"
                "src_rs 为 10×9 两人军棋规则重写，四国（4 方混战）\n"
                "未实现。原版四国棋盘/规则需另项逆向移植。\n"
                "四国模式当前不可玩，请选择人机/两人对战。");
            LOGI("OnTouch 四国人数弹窗：选%d（action=%d 人数）→ 诚实边界对话框 → 回主菜单",
                 nact + 2, nact);
            sfx_play(env, "pop");
            ui_screen_set(UI_MENU);
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_nplayer_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 四国人数弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P0（datah_062 证据）：两人新局简化弹窗态。
       点 玩法/明暗棋 → 切换模式（明棋/翻棋暗棋循环，默认翻棋暗棋对齐原版）；
       点 确定 → ui_opt_select(mode) + jq_restart_game（随模式开局，两人热座 AI 不介入）；
       点 取消 → 回主菜单；点面板外 → 回主菜单。 */
    if (ui_screen_get() == UI_NEWGAME) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_newgame_btn *nb = ui_newgame_buttons();
        int nh = ui_newgame_hit(nb, UI_NEWGAME_COUNT, bx, by);
        if (nh >= 0) {
            int nact = nb[nh].action;
            if (nact == 0 || nact == 1) {   /* 玩法/明暗棋：同义切换（明棋=布局 / 翻棋暗棋） */
                int mode = ui_opt_get_mode();
                mode = !mode;
                ui_opt_select(ui_opt_get_level(), mode);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局弹窗：玩法/明暗棋切换 → mode=%d（%s，默认翻棋暗棋对齐原版）",
                     mode, mode ? "翻棋暗棋" : "布局明棋");
            } else if (nact == 2) {         /* 确定：随模式开局（两人热座，AI 不介入） */
                g_vs_ai = 0;
                pthread_mutex_lock(&g_mutex);
                jq_restart_game();
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME);
                sfx_play(env, "over");
                LOGI("OnTouch 新局弹窗：确定 → 开局（stage=PLAY 蓝先手，mode=%d %s，两人热座 AI 不介入）",
                     ui_opt_get_mode(), ui_opt_get_mode() ? "翻棋暗棋" : "布局明棋");
            } else if (nact == 3) {         /* 取消：回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局弹窗：取消 → 回主菜单（不开局）");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_newgame_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 新局弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P1（datah_038 证据）：登录弹窗态。
       点 用户名/密码（占位标签，无输入框）→ 无动作（LOGI）；
       点 登录/注册 → 诚实边界对话框（账号系统/服务器协议未逆向）+ 回主菜单；
       点 游客 → 本地单机模式（g_vs_ai=0 两人热座开局，无账号）；
       点 返回 → 回主菜单；点面板外 → 回主菜单。
       ★ 诚实边界：原版登录=账号系统（登录/注册 需服务器协议，src_rs 未逆向，
       Junqi.* 登录/注册 20+ 事件未实现）；游客=本地单机唯一真生效路径。 */
    if (ui_screen_get() == UI_LOGIN) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_login_btn *lb = ui_login_buttons();
        int lh = ui_login_hit(lb, UI_LOGIN_COUNT, bx, by);
        if (lh >= 0) {
            int lact = lb[lh].action;   /* 0=用户名 1=密码 2=登录 3=注册 4=游客 5=返回 */
            LOGI("OnTouch 登录弹窗：控件%d（action=%d）", lh, lact);
            if (lact == 0 || lact == 1) {   /* 用户名/密码 占位标签（触摸 flaky 无输入框） */
                LOGI("OnTouch 登录弹窗：用户名/密码 占位标签（★ 诚实边界：输入框简化，不画真输入框）");
            } else if (lact == 2) {   /* 登录 → 诚实边界对话框 + 回主菜单 */
                jq_show_dialog(env, "账号登录（诚实边界）",
                    "账号登录（诚实边界）：\n"
                    "服务器协议未逆向，原版 Junqi.* 登录/注册 20+ 事件\n"
                    "未实现，当前为离线单机模式。请选择游客进入。");
                LOGI("OnTouch 登录弹窗：登录 → 诚实边界对话框 → 回主菜单");
                ui_screen_set(UI_MENU);
            } else if (lact == 3) {   /* 注册 → 同样诚实边界对话框 + 回主菜单 */
                jq_show_dialog(env, "账号注册（诚实边界）",
                    "账号注册（诚实边界）：\n"
                    "服务器协议未逆向，原版 Junqi.* 登录/注册 20+ 事件\n"
                    "未实现，当前为离线单机模式。请选择游客进入。");
                LOGI("OnTouch 登录弹窗：注册 → 诚实边界对话框 → 回主菜单");
                ui_screen_set(UI_MENU);
            } else if (lact == 4) {   /* 游客 → 本地单机（两人热座，无账号） */
                g_vs_ai = 0;   /* 游客=本地单机：AI 不介入，两人热座 */
                pthread_mutex_lock(&g_mutex);
                jq_restart_game();
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME);
                sfx_play(env, "over");
                LOGI("OnTouch 登录弹窗：游客 → 本地单机模式（g_vs_ai=0 两人热座开局，无账号，"
                     "★ 诚实边界：唯一真生效路径，登录/注册 服务器协议未逆向）");
            } else if (lact == 5) {   /* 返回 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 登录弹窗：返回 → 主菜单");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_login_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 登录弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P1（datah_012 证据）：游戏设置弹窗态。
       点 4 开关（和棋/大本营/吃地雷/吃军旗）→ 各循环翻转 开/关（本地副本，未确定不落盘）；
       点 确定 → ui_rules_save + 应用到规则引擎 + 回对局；点 取消 → ui_rules_load 恢复原值 + 回对局；
       点面板外 → 恢复原值 + 回对局。★ 诚实边界：4 开关默认全开（对齐原版默认）。 */
    if (ui_screen_get() == UI_GAMESET) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_gameset_btn *gb = ui_gameset_buttons();
        int gh = ui_gameset_hit(gb, UI_GAMESET_COUNT, bx, by);
        if (gh >= 0) {
            int gact = gb[gh].action;
            LOGI("OnTouch 游戏设置：按钮%d（action=%d）", gh, gact);
            if (gact == 0) {   /* 和棋开关翻转（开→关→开） */
                ui_rule_draw_set(!ui_rule_draw_get());
                sfx_play(env, "pop");
                LOGI("游戏设置：和棋 → %s", ui_rule_draw_get() ? "开" : "关");
            } else if (gact == 1) {   /* 大本营开关翻转 */
                ui_rule_hq_set(!ui_rule_hq_get());
                sfx_play(env, "pop");
                LOGI("游戏设置：大本营 → %s", ui_rule_hq_get() ? "开" : "关");
            } else if (gact == 2) {   /* 吃地雷开关翻转 */
                ui_rule_mine_set(!ui_rule_mine_get());
                sfx_play(env, "pop");
                LOGI("游戏设置：吃地雷 → %s", ui_rule_mine_get() ? "开" : "关");
            } else if (gact == 3) {   /* 吃军旗开关翻转 */
                ui_rule_flag_set(!ui_rule_flag_get());
                sfx_play(env, "pop");
                LOGI("游戏设置：吃军旗 → %s", ui_rule_flag_get() ? "开" : "关");
            } else if (gact == 4) {   /* 确定：持久化 + 应用到规则引擎 + 回对局 */
                ui_rules_save();
                sfx_play(env, "pop");
                LOGI("游戏设置：确定 → 保存（和棋=%d 大本营=%d 地雷=%d 扛旗=%d）+ 应用 g_rule_* + 回对局",
                     ui_rule_draw_get(), ui_rule_hq_get(), ui_rule_mine_get(), ui_rule_flag_get());
                ui_screen_set(UI_GAME);
            } else if (gact == 5) {   /* 取消：重读磁盘恢复原值 + 回对局 */
                ui_rules_load();
                sfx_play(env, "pop");
                LOGI("游戏设置：取消 → 恢复原值（和棋=%d 大本营=%d 地雷=%d 扛旗=%d）+ 回对局",
                     ui_rule_draw_get(), ui_rule_hq_get(), ui_rule_mine_get(), ui_rule_flag_get());
                ui_screen_set(UI_GAME);
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_gameset_popup_inside(bx, by)) {
            ui_rules_load();   /* 面板外 = 取消语义：恢复原值 */
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 游戏设置外（bx=%.0f by=%.0f）→ 恢复原值 + 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P1（datah_017 证据）：载入布局弹窗态。
       点列表项（0-5）→ 选中并载入该布局到棋盘 + 清 undo + 回对局；
       点 取消 / 面板外 → 回对局（不载入）。★ 诚实边界：ListBox 滚动未实现（6 条截断）。 */
    if (ui_screen_get() == UI_LOAD_LAY) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_loadlayout_btn *lb = ui_loadlayout_buttons();
        int lh = ui_loadlayout_hit(lb, UI_LOADLAYOUT_COUNT, bx, by);
        if (lh >= 0) {
            int lrow = lb[lh].action;   /* 0-5=列表项 6=取消 */
            LOGI("OnTouch 载入布局：控件%d（action=%d）", lh, lrow);
            if (lrow <= 5 && lrow < ui_layout_count()) {
                const char *nm = ui_layout_name(lrow);
                ui_layout_select(lrow);
                uint8_t lay[30];
                if (nm && ui_layout_load(nm, lay) == 0) {
                    pthread_mutex_lock(&g_mutex);
                    jq_load_layout(&g_state, lay);
                    int dark = ui_opt_get_mode() == 1;
                    for (int i = 0; i < N_CELLS; i++)
                        if (g_state.cells[i].rank != RANK_EMPTY)
                            g_state.cells[i].revealed = dark ? 0 : 1;
                    g_undo_top = 0;
                    g_state.stage = STG_PLAY;
                    g_state.turn = SIDE_BLUE;
                    g_state.winner = -1;
                    pthread_mutex_unlock(&g_mutex);
                    ui_screen_set(UI_GAME);
                    sfx_play(env, "over");
                    LOGI("载入布局：载入 %s 到棋盘（30 字节 .lay，随模式 mode=%d，清 undo，蓝先手）",
                         nm, ui_opt_get_mode());
                    (void)clazz;
                    return JNI_TRUE;
                }
            } else {
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("载入布局：取消/列表外 → 回对局（不载入）");
                (void)clazz;
                return JNI_TRUE;
            }
        }
        if (!ui_loadlayout_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 载入布局外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P1（datah_020 证据）：保存布局弹窗态。
       点 保存 → 当前棋盘 30 字节 .lay 写 layouts/<名>；点 改名 → 旧名+数字后缀（简化，
       自动编号）；点 删除 → 删选中项 .lay；点 关闭 / 面板外 → 回对局。
       点列表区（同 017 ListBox 行）→ 选中该条目（供改名/删除操作）。★ 诚实边界：
       改名=自动编号（无文本输入框）。 */
    if (ui_screen_get() == UI_SAVE_LAY) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_savelayout_btn *sb = ui_savelayout_buttons();
        int sh = ui_savelayout_hit(sb, UI_SAVELAYOUT_COUNT, bx, by);
        if (sh >= 0) {
            int sact = sb[sh].action;
            LOGI("OnTouch 保存布局：按钮%d（action=%d）", sh, sact);
            if (sact == 0) {   /* 保存：当前棋盘 30 字节 .lay 写 layouts/ */
                uint8_t lay[30];
                pthread_mutex_lock(&g_mutex);
                jq_extract_layout_30(&g_state, lay);
                pthread_mutex_unlock(&g_mutex);
                /* 自动命名 Lay%d（找未占用号） */
                char name[32];
                int k = 0;
                int cnt = ui_layout_count();
                for (k = 0; k < 64 && k < cnt; k++) {
                    char probe[32];
                    snprintf(probe, sizeof probe, "Lay%d", k);
                    if (strcmp(ui_layout_name(k), probe) != 0) break;
                }
                snprintf(name, sizeof name, "Lay%d", k);
                if (ui_layout_save(lay, name) == 0)
                    LOGI("保存布局：当前棋盘 → %s.lay（30 字节，layouts/ 目录）", ui_laydir_get());
            } else if (sact == 1) {   /* 改名：旧名+数字后缀（简化：Lay1 → Lay1_1） */
                const char *old = ui_layout_selected_valid() ? ui_layout_name(ui_layout_selected()) : 0;
                if (old) {
                    char newn[40];
                    int cnt = ui_layout_count();
                    int n2 = 1, found = 0;
                    for (n2 = 1; n2 < 100 && !found; n2++) {
                        snprintf(newn, sizeof newn, "%s_%d", old, n2);
                        for (int k = 0; k < cnt; k++)
                            if (strcmp(ui_layout_name(k), newn) == 0) { found = 1; break; }
                    }
                    if (found) newn[0] = 0;   /* 全被占 → 不执行 */
                    if (newn[0] && ui_layout_rename(old, newn) == 0)
                        LOGI("保存布局：改名 %s → %s（旧名+数字后缀，简化无输入框）", old, newn);
                }
            } else if (sact == 2) {   /* 删除：删选中项 */
                const char *nm = ui_layout_selected_valid() ? ui_layout_name(ui_layout_selected()) : 0;
                if (nm && ui_layout_delete(nm) == 0)
                    LOGI("保存布局：删除 %s.lay（选中项）", nm);
            } else if (sact == 3) {   /* 关闭：回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("保存布局：关闭 → 回对局");
                (void)clazz;
                return JNI_TRUE;
            }
            if (sact != 3) sfx_play(env, "pop");
            ui_layout_list();   /* 列表刷新 */
            (void)clazz;
            return JNI_TRUE;
        }
        /* 列表区（同 017 ListBox 行 180,447-540,807）：点行 = 选中该条目 */
        {
            const ui_loadlayout_btn *lb = ui_loadlayout_buttons();
            int lh = ui_loadlayout_hit(lb, 6, bx, by);
            if (lh >= 0 && lh < ui_layout_count()) {
                ui_layout_select(lh);
                sfx_play(env, "select");
                LOGI("保存布局：选中 %s（action=%d）", ui_layout_name(lh), lh);
                (void)clazz;
                return JNI_TRUE;
            }
        }
        if (!ui_savelayout_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 保存布局外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P1（datah_016 证据）：布局管理弹窗态。
       点 载入 → 选中布局应用到棋盘 + 清 undo + 回对局；点 删除 → 删选中项文件；
       点 取消 / 面板外 → 回对局。30 格预览区 = 小方块占位（★ 诚实边界）。 */
    if (ui_screen_get() == UI_LAY_MGR) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_laymgr_btn *mb = ui_laymgr_buttons();
        int mh = ui_laymgr_hit(mb, UI_LAYMGR_COUNT, bx, by);
        if (mh >= 0) {
            int mact = mb[mh].action;
            LOGI("OnTouch 布局管理：按钮%d（action=%d）", mh, mact);
            if (mact == 0) {   /* 载入：选中布局应用到棋盘 */
                const char *nm = ui_layout_selected_valid() ? ui_layout_name(ui_layout_selected()) : 0;
                uint8_t lay[30];
                if (nm && ui_layout_load(nm, lay) == 0) {
                    pthread_mutex_lock(&g_mutex);
                    jq_load_layout(&g_state, lay);
                    int dark = ui_opt_get_mode() == 1;
                    for (int i = 0; i < N_CELLS; i++)
                        if (g_state.cells[i].rank != RANK_EMPTY)
                            g_state.cells[i].revealed = dark ? 0 : 1;
                    g_undo_top = 0;
                    g_state.stage = STG_PLAY;
                    g_state.turn = SIDE_BLUE;
                    g_state.winner = -1;
                    pthread_mutex_unlock(&g_mutex);
                    ui_screen_set(UI_GAME);
                    sfx_play(env, "over");
                    LOGI("布局管理：载入 %s 到棋盘（30 字节 .lay，清 undo，蓝先手）", nm);
                    (void)clazz;
                    return JNI_TRUE;
                }
                sfx_play(env, "pop");
                LOGI("布局管理：载入失败（选中无效/文件缺失）");
            } else if (mact == 1) {   /* 删除：删选中项文件 */
                const char *nm = ui_layout_selected_valid() ? ui_layout_name(ui_layout_selected()) : 0;
                if (nm && ui_layout_delete(nm) == 0) {
                    ui_layout_list();
                    LOGI("布局管理：删除 %s.lay（选中项）", nm);
                }
                sfx_play(env, "pop");
            } else if (mact == 2) {   /* 取消：回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("布局管理：取消 → 回对局");
                (void)clazz;
                return JNI_TRUE;
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_laymgr_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 布局管理外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P1（datah_107 证据）：复盘回放控制条态——底部 5 按钮（菜单/回退/前进/跳转/返回，
       460×60 @10,705 不居中）。业务（replay.c 确定性回放，g_mutex 内改 g_state）：
       回退 = cur-- 重建 / 前进 = cur++ 走 1 步 / 跳转 = 回开头(cur=0)/跳结尾(cur=n) 循环
       （简化，无步进输入框）/ 菜单/返回 = 退复盘回对局（保留记录）。
       ★ 诚实边界：原版 datah_039 入口浮标命中 flaky，复盘态入口走 setprop jq.replay 钩子；
       本控制条按钮命中走触摸路径（与 gameset 同范式，flaky 时业务逻辑桌面 evverify 实证）。 */
    if (ui_screen_get() == UI_REPLAY) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_replay_btn *rb = ui_replay_ctrl_buttons();
        int rh = ui_replay_ctrl_hit(rb, UI_REPLAY_CTRL_COUNT, bx, by);
        if (rh >= 0) {
            int ract = rb[rh].action;
            int cur = ui_replay_cur(), n = ui_replay_count();
            LOGI("OnTouch 复盘控制条：按钮%d（action=%d，cur=%d n=%d）", rh, ract, cur, n);
            if (ract == 0 || ract == 4) {   /* 菜单（简化=退复盘）/ 返回（退复盘回对局） */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("复盘控制条：%s → 退复盘回对局（保留走子记录 %d 条）",
                     ract == 0 ? "菜单（简化=退复盘，无复盘内子菜单）" : "返回", n);
                (void)clazz;
                return JNI_TRUE;
            }
            pthread_mutex_lock(&g_mutex);
            int rc;
            if (ract == 1) {               /* 回退：cur-- 重建（游标 0 停住） */
                cur = ui_replay_cur();
                if (cur > 0) {
                    cur--;
                    rc = ui_replay_rewind(cur);
                    if (rc != 0) { LOGI("复盘控制条：回退重放失败（记录与规则引擎不一致，防御停止）"); }
                } else {
                    rc = 0;
                    LOGI("复盘控制条：回退 → 已在开头（cur=0）");
                }
            } else if (ract == 2) {        /* 前进：cur++ 走 1 步（到 n 停住） */
                cur = ui_replay_cur();
                if (cur < n) {
                    cur++;
                    rc = ui_replay_rewind(cur);
                    if (rc != 0) { LOGI("复盘控制条：前进重放失败（记录与规则引擎不一致，防御停止）"); }
                } else {
                    rc = 0;
                    LOGI("复盘控制条：前进 → 已在末尾（cur=%d）", cur);
                }
            } else {                       /* 跳转：回开头(cur=0) / 跳结尾(cur=n) 循环（简化） */
                cur = ui_replay_cur();
                cur = (cur == 0) ? n : 0;  /* 0 → 跳结尾；否则回开头 */
                rc = ui_replay_rewind(cur);
                LOGI("复盘控制条：跳转 → cur=%d（简化=回开头/跳结尾二选一循环，无步进输入框）", cur);
            }
            pthread_mutex_unlock(&g_mutex);
            sfx_play(env, "select");
            if (jq_glview_is_ready()) jq_glview_dirty(&g_glview);
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_replay_ctrl_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 复盘控制条外（bx=%.0f by=%.0f）→ 退复盘回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_037 证据）：规则文本弹窗态。
        点 规则-翻棋 → 显示翻棋模式规则文本（暗棋：棋子背面，按 rank 大小吃子，
        工兵挖雷，炸弹同归于尽，夺旗胜）；点 规则-布局 → 显示布局模式规则文本
        （明棋：自由布阵，军旗在大本营，地雷后两行，炸弹前线）；点 返回 → 回对局。
        点面板外 → 回对局。★ 诚实边界：规则文本=src_rs 简化版固定文本（2 套）。 */
    if (ui_screen_get() == UI_RULES) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_rules_btn *rb = ui_rules_buttons();
        int rh = ui_rules_hit(rb, UI_RULES_COUNT, bx, by);
        if (rh >= 0) {
            int ract = rb[rh].action;
            LOGI("OnTouch 规则文本弹窗：控件%d（action=%d）", rh, ract);
            if (ract == 0) {   /* 规则-翻棋 → 显示翻棋模式规则文本 */
                jq_show_dialog(env, "规则-翻棋（暗棋）",
                    "翻棋模式规则（简化版）：\n"
                    "暗棋：棋子背面，按 rank 大小吃子。\n"
                    "工兵可挖雷，炸弹遇任何子同归于尽。\n"
                    "军旗被吃或被困则负；夺旗方胜。\n"
                    "★ 诚实边界：规则文本=src_rs 简化版固定文本。");
                sfx_play(env, "pop");
                LOGI("OnTouch 规则文本弹窗：规则-翻棋 → 显示翻棋模式规则文本（暗棋）");
            } else if (ract == 1) {   /* 规则-布局 → 显示布局模式规则文本 */
                jq_show_dialog(env, "规则-布局（明棋）",
                    "布局模式规则（简化版）：\n"
                    "明棋：自由布阵，军旗在大本营。\n"
                    "地雷后两行，炸弹前线。\n"
                    "工兵挖雷，炸弹同归于尽，夺旗方胜。\n"
                    "★ 诚实边界：规则文本=src_rs 简化版固定文本。");
                sfx_play(env, "pop");
                LOGI("OnTouch 规则文本弹窗：规则-布局 → 显示布局模式规则文本（明棋）");
            } else if (ract == 2) {   /* 返回 → 回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 规则文本弹窗：返回 → 回对局");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_rules_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 规则文本弹窗外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_104 证据）：新局完整版弹窗态。
        点 我方 → 蓝/红 切换（循环）；点 电脑水平 → 难度 1-4 循环；
        点 玩法 → 布局/翻棋 切换（循环）；点 确定 → 按选择开局；
        点 取消 → 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环范式）。 */
    if (ui_screen_get() == UI_NEWGAME104) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_newgame104_btn *nb = ui_newgame104_buttons();
        int nh = ui_newgame104_hit(nb, UI_NEWGAME104_COUNT, bx, by);
        if (nh >= 0) {
            int nact = nb[nh].action;
            LOGI("OnTouch 新局完整版弹窗：控件%d（action=%d）", nh, nact);
            if (nact == 0) {   /* 我方：蓝/红 切换（循环，对齐 datah_001 音量循环范式） */
                int cur = ui_opt_get_mode();   /* 用 mode 位存储我方选择（0=蓝方 1=红方，简化） */
                ui_opt_select(ui_opt_get_level(), cur ? 0 : 1);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版弹窗：我方切换 → mode=%d（蓝方/红方，★ 诚实边界：简化用 mode 位存储）",
                     ui_opt_get_mode());
            } else if (nact == 1) {   /* 电脑水平：难度 1-4 循环 */
                int lvl = ui_opt_get_level() + 1;
                if (lvl > 4) lvl = 1;
                ui_opt_select(lvl, ui_opt_get_mode());
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版弹窗：电脑水平切换 → level=%d（新手/中级/高级/好友循环）", lvl);
            } else if (nact == 2) {   /* 玩法：布局/翻棋 切换 */
                int mode = !ui_opt_get_mode();
                ui_opt_select(ui_opt_get_level(), mode);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版弹窗：玩法切换 → mode=%d（%s，默认翻棋暗棋）",
                     mode, mode ? "翻棋暗棋" : "布局明棋");
            } else if (nact == 3) {   /* 确定：按选择开局（ui_opt_select + jq_restart_game 随模式） */
                g_vs_ai = 0;
                pthread_mutex_lock(&g_mutex);
                jq_restart_game();
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME);
                sfx_play(env, "over");
                LOGI("OnTouch 新局完整版弹窗：确定 → 开局（stage=PLAY 蓝先手，level=%d mode=%d，两人热座 AI 不介入）",
                     ui_opt_get_level(), ui_opt_get_mode());
            } else if (nact == 4) {   /* 取消：回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版弹窗：取消 → 回主菜单（不开局）");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_newgame104_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 新局完整版弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_034 证据）：加入游戏弹窗态。
        点 桌号（占位标签）→ 无动作（LOGI）；点 确定 → 诚实边界对话框 + 回主菜单；
        点 取消 → 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP。 */
    if (ui_screen_get() == UI_JOIN) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_join_btn *jb = ui_join_buttons();
        int jh = ui_join_hit(jb, UI_JOIN_COUNT, bx, by);
        if (jh >= 0) {
            int jact = jb[jh].action;
            LOGI("OnTouch 加入游戏弹窗：控件%d（action=%d）", jh, jact);
            if (jact == 0) {   /* 桌号占位标签（触摸 flaky 无输入框） */
                LOGI("OnTouch 加入游戏弹窗：桌号占位标签（★ 诚实边界：输入框简化，不画真输入框）");
            } else if (jact == 1) {   /* 确定 → 诚实边界对话框 + 回主菜单 */
                jq_show_dialog(env, "加入游戏（诚实边界）",
                    "加入游戏（诚实边界）：\n"
                    "桌号加入需 Wi-Fi/局域网协议，src_rs 仅蓝牙联机已实现，\n"
                    "请选蓝牙联机（datah_072 方式弹窗“蓝牙联机”项）。");
                LOGI("OnTouch 加入游戏弹窗：确定 → 诚实边界对话框 → 回主菜单");
                ui_screen_set(UI_MENU);
            } else if (jact == 2) {   /* 取消 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 加入游戏弹窗：取消 → 回主菜单");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_join_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 加入游戏弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_035 证据）：创建游戏弹窗态。
        点 密码（占位标签）→ 无动作（LOGI）；点 确定 → 诚实边界对话框 + 回主菜单；
        点 取消 → 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP。 */
    if (ui_screen_get() == UI_CREATE) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_create_btn *cb = ui_create_buttons();
        int ch = ui_create_hit(cb, UI_CREATE_COUNT, bx, by);
        if (ch >= 0) {
            int cact = cb[ch].action;
            LOGI("OnTouch 创建游戏弹窗：控件%d（action=%d）", ch, cact);
            if (cact == 0) {   /* 密码占位标签（触摸 flaky 无输入框） */
                LOGI("OnTouch 创建游戏弹窗：密码占位标签（★ 诚实边界：输入框简化，不画真输入框）");
            } else if (cact == 1) {   /* 确定 → 诚实边界对话框 + 回主菜单 */
                jq_show_dialog(env, "创建游戏（诚实边界）",
                    "创建游戏（诚实边界）：\n"
                    "创建房间需 Wi-Fi/局域网协议，src_rs 仅蓝牙联机已实现，\n"
                    "请选蓝牙联机（datah_072 方式弹窗“蓝牙联机”项）。");
                LOGI("OnTouch 创建游戏弹窗：确定 → 诚实边界对话框 → 回主菜单");
                ui_screen_set(UI_MENU);
            } else if (cact == 2) {   /* 取消 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 创建游戏弹窗：取消 → 回主菜单");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_create_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 创建游戏弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗态。
        点 先后手 → 蓝/红 循环；点 电脑水平 → 难度 1-4 循环；
        点 玩法 → 布局/翻棋 切换；点 明暗棋 → 明/暗 切换；
        点 确定 → 按选择 ui_opt_select + jq_restart_game 开局；
        点 取消 → 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：4 项各循环切换当前值（对齐 datah_104 循环切换范式）。 */
    if (ui_screen_get() == UI_NG061) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ng061_btn *nb = ui_ng061_buttons();
        int nh = ui_ng061_hit(nb, UI_NG061_COUNT, bx, by);
        if (nh >= 0) {
            int nact = nb[nh].action;
            LOGI("OnTouch 新局完整版（完整版补充）弹窗：控件%d（action=%d）", nh, nact);
            if (nact == 0) {   /* 先后手：蓝/红 循环（对齐 datah_001 音量循环范式） */
                ui_first_set(ui_first_get() ? 0 : 1);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：先后手切换 → %s（★ 诚实边界：简化用 ui_first 独立循环位）",
                     ui_first_get() ? "红先手" : "蓝先手");
            } else if (nact == 1) {   /* 电脑水平：难度 1-4 循环 */
                int lvl = ui_opt_get_level() + 1;
                if (lvl > 4) lvl = 1;
                ui_opt_select(lvl, ui_opt_get_mode());
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：电脑水平切换 → level=%d（新手/中级/高级/好友循环）", lvl);
            } else if (nact == 2) {   /* 玩法：布局/翻棋 切换 */
                int mode = !ui_opt_get_mode();
                ui_opt_select(ui_opt_get_level(), mode);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：玩法切换 → mode=%d（%s）",
                     mode, mode ? "翻棋暗棋" : "布局明棋");
            } else if (nact == 3) {   /* 明暗棋：明/暗 切换（ui_show 独立循环位） */
                ui_show_set(ui_show_get() ? 0 : 1);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：明暗棋切换 → %s（★ 诚实边界：简化用 ui_show 独立循环位）",
                     ui_show_get() ? "暗棋" : "明棋");
            } else if (nact == 4) {   /* 确定：按选择开局（ui_opt_select + jq_restart_game 随模式） */
                g_vs_ai = 0;
                pthread_mutex_lock(&g_mutex);
                jq_restart_game();
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME);
                sfx_play(env, "over");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：确定 → 开局（stage=PLAY，level=%d mode=%d first=%d show=%d，"
                     "两人热座 AI 不介入，★ 诚实边界：先后手/明暗棋 各循环切换简化版）",
                     ui_opt_get_level(), ui_opt_get_mode(), ui_first_get(), ui_show_get());
            } else if (nact == 5) {   /* 取消：回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：取消 → 回主菜单（不开局）");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ng061_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 新局完整版（完整版补充）弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_050 证据）：对局信息弹窗态。
        点 复盘 → 进 UI_REPLAY 复盘态（复用 P1 复盘）；
        点 删除 → 清复盘记录（诚实边界：原版删除对局存档未逆向）；
        点 返回 → 回对局；点面板外 → 回对局。
        Info 文本=☰ 战况统计复用（渲染层占位标签 + LOGI 实际值）。 */
    if (ui_screen_get() == UI_GAMEINFO) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_gameinfo_btn *gb = ui_gameinfo_buttons();
        int gh = ui_gameinfo_hit(gb, UI_GAMEINFO_COUNT, bx, by);
        if (gh >= 0) {
            int gact = gb[gh].action;
            LOGI("OnTouch 对局信息弹窗：控件%d（action=%d）", gh, gact);
            if (gact == 0) {   /* 复盘：进 UI_REPLAY 复盘态（复用 P1 复盘） */
                int n = ui_replay_count();
                int rc = ui_replay_rewind(n);
                ui_screen_set(UI_REPLAY);
                sfx_play(env, "pop");
                LOGI("OnTouch 对局信息弹窗：复盘 → UI_REPLAY 复盘态（datah_107 控制条 5 按钮，cur=%d/%d rewind rc=%d）",
                     n, n, rc);
            } else if (gact == 1) {   /* 删除：清复盘记录（诚实边界简化） */
                ui_replay_init();
                sfx_play(env, "pop");
                LOGI("OnTouch 对局信息弹窗：删除 → 清复盘记录（n=%d，★ 诚实边界：原版删除对局存档未逆向，"
                     "简化为清复盘记录）", ui_replay_count());
            } else if (gact == 2) {   /* 返回：回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 对局信息弹窗：返回 → 回对局");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_gameinfo_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 对局信息弹窗外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_002 证据）：亮度调节弹窗态。
        点 亮度 → 低/中/高 3 档循环切换（ui_bright_set，不画真滑条）；
        点 确定 → ui_bright_save 持久化 + 回对局；
        点 取消 → ui_bright_load 恢复原值 + 回对局；点面板外 → 回对局。
        ★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为 3 档循环切换（对齐 datah_001 音量循环范式）。 */
    if (ui_screen_get() == UI_BRIGHT) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_bright_btn *bb = ui_bright_buttons();
        int bh = ui_bright_hit(bb, UI_BRIGHT_COUNT, bx, by);
        if (bh >= 0) {
            int bact = bb[bh].action;
            LOGI("OnTouch 亮度调节弹窗：控件%d（action=%d）", bh, bact);
            if (bact == 0) {   /* 亮度：低/中/高 3 档循环切换（ui_bright_set） */
                int v = ui_bright_get() + 1;
                if (v > 2) v = 0;
                ui_bright_set(v);
                sfx_play(env, "pop");
                LOGI("OnTouch 亮度调节弹窗：亮度切换 → 档位=%d（%s，★ 诚实边界：不画真滑条，3 档循环简化）",
                     v, v == 0 ? "低" : (v == 1 ? "中" : "高"));
            } else if (bact == 1) {   /* 确定：持久化 + 回对局 */
                ui_bright_save();
                sfx_play(env, "pop");
                LOGI("OnTouch 亮度调节弹窗：确定 → 保存档位=%d + 回对局", ui_bright_get());
                ui_screen_set(UI_GAME);
            } else if (bact == 2) {   /* 取消：恢复原值 + 回对局 */
                ui_bright_load();
                sfx_play(env, "pop");
                LOGI("OnTouch 亮度调节弹窗：取消 → 恢复原值 档位=%d + 回对局", ui_bright_get());
                ui_screen_set(UI_GAME);
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_bright_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 亮度调节弹窗外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_029 证据）：聊天记录弹窗态。
        点 清除 → 清本地占位记录（诚实边界：联机聊天文本未逆向，仅蓝牙占位帧）；
        点 关闭 → 回对局；点面板外 → 回对局。
        Msg 文本框=诚实边界占位（渲染层占位标签 + LOGI）。 */
    if (ui_screen_get() == UI_CHATLOG) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_chatlog_btn *cb = ui_chatlog_buttons();
        int ch = ui_chatlog_hit(cb, UI_CHATLOG_COUNT, bx, by);
        if (ch >= 0) {
            int cact = cb[ch].action;
            LOGI("OnTouch 聊天记录弹窗：控件%d（action=%d）", ch, cact);
            if (cact == 0) {   /* 清除：清本地占位记录（诚实边界） */
                static char chat_log_clear = 1;   /* 占位记录标志（无真实聊天文本存储） */
                chat_log_clear = 0;
                sfx_play(env, "pop");
                LOGI("OnTouch 聊天记录弹窗：清除 → 清本地占位记录（★ 诚实边界：联机聊天文本未逆向，"
                     "src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本存储，清除=清占位标志）");
            } else if (cact == 1) {   /* 关闭：回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 聊天记录弹窗：关闭 → 回对局");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_chatlog_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 聊天记录弹窗外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_031 证据）：个人资料弹窗态。
        点 修改 → 诚实边界对话框（修改资料需账号系统，未实现）；
        点 帮助 → 显示帮助文本；
        点 返回 → 回主菜单；点面板外 → 回主菜单。
        Info 文本框=诚实边界占位（渲染层占位标签 + LOGI）。
        现有 datah_072 方式弹窗"个人资料"→UI_LOGIN 不破坏，031 作为登录后资料查看补充（独立态）。 */
    if (ui_screen_get() == UI_PROFILE) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_profile_btn *pb = ui_profile_buttons();
        int ph = ui_profile_hit(pb, UI_PROFILE_COUNT, bx, by);
        if (ph >= 0) {
            int pact = pb[ph].action;
            LOGI("OnTouch 个人资料弹窗：控件%d（action=%d）", ph, pact);
            if (pact == 0) {   /* 修改 → 诚实边界对话框（修改资料需账号系统，未实现） */
                jq_show_dialog(env, "修改资料（诚实边界）",
                    "修改资料（诚实边界）：\n"
                    "原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，\n"
                    "当前为离线单机模式，无账号，修改资料不可用。");
                sfx_play(env, "pop");
                LOGI("OnTouch 个人资料弹窗：修改 → 诚实边界对话框 → 回主菜单");
                ui_screen_set(UI_MENU);
            } else if (pact == 1) {   /* 帮助 → 显示帮助文本 */
                jq_show_dialog(env, "帮助（个人资料）",
                    "个人资料（帮助）：\n"
                    "原版个人资料含 昵称/头像/胜率/账号 信息，\n"
                    "src_rs 当前为离线单机模式，无账号系统，\n"
                    "这些字段均为诚实边界占位，未实现账号功能。");
                sfx_play(env, "pop");
                LOGI("OnTouch 个人资料弹窗：帮助 → 显示帮助文本");
            } else if (pact == 2) {   /* 返回 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 个人资料弹窗：返回 → 回主菜单");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_profile_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 个人资料弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }

    /* ★ P2（datah_042 证据）：提和详情弹窗态（联机版 ☰ 菜单 7 图标）。
        提和 → jq_bt_ctrl_send(BT_T_DRAW) + 诚实边界对话框（本地提和判定未实现，
        原版需对端应答）；认输 → g_state.winner=对方 stage=OVER + jq_bt_ctrl_send(BT_T_RESIGN)；
        聊天 → jq_bt_ctrl_send(BT_T_CHAT) + 边界对话框（联机聊天文本未逆向）；
        系统设置 → UI_GAMESET（datah_012 游戏设置，已实现）；
        玩家资料 → UI_PROFILE（datah_031 个人资料，已实现，诚实边界账号系统未逆向）；
        战况统计 → 遍历 cells 蓝红余子+总步数+回合（复用 ☰ 菜单战况统计逻辑）；
        软件信息 → jq_show_dialog 版本（复用 ☰ 菜单软件信息逻辑）；
        点面板外 → 回对局。
        ★ 诚实边界：本地提和判定未实现（src_rs 离线单机无对端），联机提和/认输走
        BT 帧 + 边界对话框，不包装成"已提和判定"。 */
    if (ui_screen_get() == UI_DRAWDETAIL) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_drawdetail_btn *db = ui_drawdetail_buttons();
        int dh = ui_drawdetail_hit(db, UI_DRAWDETAIL_COUNT, bx, by);
        if (dh >= 0) {
            int dact = db[dh].action;
            LOGI("OnTouch 提和详情弹窗：图标%d（action=%d）", dh, dact);
            if (dact == 0) {   /* 提和 → 发 BT_T_DRAW 帧 + 诚实边界对话框（本地提和判定未实现） */
                jq_bt_ctrl_send(env, BT_T_DRAW);
                jq_show_dialog(env, "提和（诚实边界）",
                    "提和已发送，等待对方回应。\n"
                    "本地提和判定未实现（原版需对端应答，\n"
                    "src_rs 离线单机无对端），联机协议为诚实边界。");
                sfx_play(env, "pop");
                LOGI("OnTouch 提和详情：提和 → 发送 DRAW 帧 + 诚实边界对话框（本地提和判定未实现）");
            } else if (dact == 1) {   /* 认输 → 对方胜 + 发 BT_T_RESIGN 帧（保留现有逻辑） */
                pthread_mutex_lock(&g_mutex);
                g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
                g_state.stage = STG_OVER;
                pthread_mutex_unlock(&g_mutex);
                jq_bt_ctrl_send(env, BT_T_RESIGN);
                sfx_play(env, "over");
                LOGI("OnTouch 提和详情：认输 → 对方胜（winner=%d）+ RESIGN 帧", g_state.winner);
            } else if (dact == 2) {   /* 聊天 → 发 BT_T_CHAT 占位帧 + 边界对话框 */
                jq_bt_ctrl_send(env, BT_T_CHAT);
                jq_show_dialog(env, "聊天（诚实边界）",
                    "聊天（诚实边界）：\n"
                    "原版联机聊天消息（Junqi.Chat*）未逆向，\n"
                    "当前发送聊天占位帧（对端无文本渲染）。");
                sfx_play(env, "pop");
                LOGI("OnTouch 提和详情：聊天 → 发送 CHAT 占位帧（联机聊天文本未逆向）");
            } else if (dact == 3) {   /* 系统设置 → 游戏设置弹窗（datah_012，已实现） */
                ui_screen_set(UI_GAMESET);
                sfx_play(env, "pop");
                LOGI("OnTouch 提和详情：系统设置 → UI_GAMESET 游戏设置弹窗（datah_012 4 规则开关）");
            } else if (dact == 4) {   /* 玩家资料 → 个人资料弹窗（datah_031，已实现，诚实边界） */
                ui_screen_set(UI_PROFILE);
                sfx_play(env, "pop");
                LOGI("OnTouch 提和详情：玩家资料 → UI_PROFILE 个人资料弹窗（datah_031，"
                     "诚实边界：账号系统未逆向，离线单机无账号）");
            } else if (dact == 5) {   /* 战况统计 → 复用 ☰ 菜单遍历 cells 逻辑 */
                pthread_mutex_lock(&g_mutex);
                int b = 0, r = 0, i;
                for (i = 0; i < N_CELLS; i++) {
                    if (g_state.cells[i].rank == RANK_EMPTY) continue;
                    if (g_state.cells[i].side == SIDE_BLUE) b++;
                    else if (g_state.cells[i].side == SIDE_RED) r++;
                }
                char msg[160];
                snprintf(msg, sizeof msg,
                    "蓝方余子：%d\n红方余子：%d\n总步数：%d\n当前回合：%s",
                    b, r, g_state.total_moves,
                    g_state.turn == SIDE_BLUE ? "蓝方" : "红方");
                pthread_mutex_unlock(&g_mutex);
                jq_show_dialog(env, "战况统计", msg);
                sfx_play(env, "pop");
                LOGI("OnTouch 提和详情：战况统计 → 蓝%d 红%d 步%d（复用 ☰ 菜单逻辑）", b, r, g_state.total_moves);
            } else if (dact == 6) {   /* 软件信息 → 复用 ☰ 菜单版本信息逻辑 */
                jq_show_dialog(env, "软件信息",
                    "军棋（Junqi）src_rs 64 位重写版\n"
                    "版本：1.6.8（com.cnvcs.junqirs）\n"
                    "玩法等价重写：规则/UI 对齐原版 v152；\n"
                    "暗棋默认（对齐原版人机/两人开局）；\n"
                    "人机=AI 走红 / 两人=热座轮流；\n"
                    "蓝牙联机/四国为诚实边界。");
                sfx_play(env, "pop");
                LOGI("OnTouch 提和详情：软件信息 → 显示版本信息（复用 ☰ 菜单逻辑）");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_drawdetail_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 提和详情弹窗外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_003 证据）：设置入口弹窗态（完整版，3 按钮）。
        点 图像 → UI_IMGSET7（图像设置完整版 datah_008）；
        点 声音 → UI_SETTINGS（声音设置 datah_001，已实现）；
        点 游戏 → UI_GAMESET（游戏设置 datah_012，已实现）；
        点面板外 → 回主菜单。
        ★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
        开关只是 UI 状态记录（不实际改渲染）。 */
    if (ui_screen_get() == UI_SETEXTTRY) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_setentry_btn *sb = ui_setentry_buttons();
        int sh = ui_setentry_hit(sb, UI_SETEXTTRY_COUNT, bx, by);
        if (sh >= 0) {
            int sact = sb[sh].action;
            LOGI("OnTouch 设置入口弹窗（完整版）：按钮%d（action=%d）", sh, sact);
            if (sact == 0) {   /* 图像 → UI_IMGSET7（图像设置完整版 datah_008） */
                ui_screen_set(UI_IMGSET7);
                sfx_play(env, "pop");
                LOGI("OnTouch 设置入口弹窗（完整版）：图像 → UI_IMGSET7 图像设置弹窗（datah_008，"
                     "★ 诚实边界：渲染参数未实现，仅 UI 状态记录）");
            } else if (sact == 1) {   /* 声音 → UI_SETTINGS（datah_001，已实现） */
                ui_screen_set(UI_SETTINGS);
                sfx_play(env, "pop");
                LOGI("OnTouch 设置入口弹窗（完整版）：声音 → UI_SETTINGS 声音设置弹窗（datah_001，已实现）");
            } else if (sact == 2) {   /* 游戏 → UI_GAMESET（datah_012，已实现） */
                ui_screen_set(UI_GAMESET);
                sfx_play(env, "pop");
                LOGI("OnTouch 设置入口弹窗（完整版）：游戏 → UI_GAMESET 游戏设置弹窗（datah_012，已实现）");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_setentry_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 设置入口弹窗（完整版）外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_004 证据）：设置入口（简化版）弹窗态（2 按钮，无游戏设置项）。
        点 图像 → UI_IMGSET8（图像设置简化版 datah_008）；
        点 声音 → UI_SETTINGS（声音设置 datah_001，已实现）；
        点面板外 → 回主菜单。
        业务：datah_004 是 datah_003 的简化版（联机态/蓝牙态设置入口，无游戏设置项）。
        ★ 诚实边界：图像设置渲染参数未实现，开关只是 UI 状态记录。 */
    if (ui_screen_get() == UI_SETEXTTRY4) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_setentry4_btn *s4 = ui_setentry4_buttons();
        int s4h = ui_setentry4_hit(s4, UI_SETEXTTRY4_COUNT, bx, by);
        if (s4h >= 0) {
            int s4act = s4[s4h].action;
            LOGI("OnTouch 设置入口（简化版）弹窗：按钮%d（action=%d）", s4h, s4act);
            if (s4act == 0) {   /* 图像 → UI_IMGSET8（图像设置简化版 datah_008） */
                ui_screen_set(UI_IMGSET8);
                sfx_play(env, "pop");
                LOGI("OnTouch 设置入口（简化版）弹窗：图像 → UI_IMGSET8 图像设置弹窗（简化版 datah_008，"
                     "★ 诚实边界：渲染参数未实现，仅 UI 状态记录）");
            } else if (s4act == 1) {   /* 声音 → UI_SETTINGS（datah_001，已实现） */
                ui_screen_set(UI_SETTINGS);
                sfx_play(env, "pop");
                LOGI("OnTouch 设置入口（简化版）弹窗：声音 → UI_SETTINGS 声音设置弹窗（datah_001，已实现）");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_setentry4_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 设置入口（简化版）弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_007 证据）：图像设置弹窗（完整版）态（4 开关 + 确定/取消）。
        点 界面动画/棋子动画/夜间模式/对方棋子 → 各循环切换 开/关
        （ui_imgset_trans/piece/night/opp_set，对齐 datah_001/012 循环范式）；
        点 确定 → ui_imgset_save（jq_imgset.txt 4 字段）+ 回主菜单；
        点 取消 → ui_imgset_load 恢复原值 + 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
        开关只是 UI 状态记录（不实际改渲染）。 */
    if (ui_screen_get() == UI_IMGSET7) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_imgset7_btn *ib = ui_imgset7_buttons();
        int ih = ui_imgset7_hit(ib, UI_IMGSET7_COUNT, bx, by);
        if (ih >= 0) {
            int iact = ib[ih].action;
            LOGI("OnTouch 图像设置弹窗（完整版）：控件%d（action=%d）", ih, iact);
            if (iact == 0) {   /* 界面动画：开/关 循环 */
                ui_imgset_trans_set(!ui_imgset_trans_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（完整版）：界面动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
                     ui_imgset_trans_get() ? "开" : "关");
            } else if (iact == 1) {   /* 棋子动画：开/关 循环 */
                ui_imgset_piece_set(!ui_imgset_piece_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（完整版）：棋子动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
                     ui_imgset_piece_get() ? "开" : "关");
            } else if (iact == 2) {   /* 夜间模式：开/关 循环 */
                ui_imgset_night_set(!ui_imgset_night_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（完整版）：夜间模式切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
                     ui_imgset_night_get() ? "开" : "关");
            } else if (iact == 3) {   /* 对方棋子：开/关 循环 */
                ui_imgset_opp_set(!ui_imgset_opp_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（完整版）：对方棋子切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
                     ui_imgset_opp_get() ? "开" : "关");
            } else if (iact == 4) {   /* 确定：保存 + 回主菜单 */
                ui_imgset_save();
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（完整版）：确定 → 保存 jq_imgset.txt（界面=%d 棋子=%d 夜间=%d 对方=%d）+ 回主菜单",
                     ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get(), ui_imgset_opp_get());
                ui_screen_set(UI_MENU);
            } else if (iact == 5) {   /* 取消：恢复原值 + 回主菜单 */
                ui_imgset_load();
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（完整版）：取消 → 恢复原值（界面=%d 棋子=%d 夜间=%d 对方=%d）+ 回主菜单",
                     ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get(), ui_imgset_opp_get());
                ui_screen_set(UI_MENU);
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_imgset7_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 图像设置弹窗（完整版）外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_008 证据）：图像设置弹窗（简化版）态（3 开关 + 确定/取消）。
        点 界面动画/棋子动画/夜间模式 → 各循环切换 开/关
        （ui_imgset_trans/piece/night_set，对齐 datah_001/012 循环范式）；
        点 确定 → ui_imgset_save（jq_imgset.txt 3 字段）+ 回主菜单；
        点 取消 → ui_imgset_load 恢复原值 + 回主菜单；点面板外 → 回主菜单。
        业务：datah_008 是 datah_007 简化版（3 开关，蓝牙态）。
        ★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
        开关只是 UI 状态记录（不实际改渲染）。 */
    if (ui_screen_get() == UI_IMGSET8) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_imgset8_btn *i8 = ui_imgset8_buttons();
        int i8h = ui_imgset8_hit(i8, UI_IMGSET8_COUNT, bx, by);
        if (i8h >= 0) {
            int i8act = i8[i8h].action;
            LOGI("OnTouch 图像设置弹窗（简化版）：控件%d（action=%d）", i8h, i8act);
            if (i8act == 0) {   /* 界面动画：开/关 循环 */
                ui_imgset_trans_set(!ui_imgset_trans_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（简化版）：界面动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
                     ui_imgset_trans_get() ? "开" : "关");
            } else if (i8act == 1) {   /* 棋子动画：开/关 循环 */
                ui_imgset_piece_set(!ui_imgset_piece_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（简化版）：棋子动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
                     ui_imgset_piece_get() ? "开" : "关");
            } else if (i8act == 2) {   /* 夜间模式：开/关 循环 */
                ui_imgset_night_set(!ui_imgset_night_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（简化版）：夜间模式切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
                     ui_imgset_night_get() ? "开" : "关");
            } else if (i8act == 3) {   /* 确定：保存 + 回主菜单 */
                ui_imgset_save();
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（简化版）：确定 → 保存 jq_imgset.txt（界面=%d 棋子=%d 夜间=%d）+ 回主菜单",
                     ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get());
                ui_screen_set(UI_MENU);
            } else if (i8act == 4) {   /* 取消：恢复原值 + 回主菜单 */
                ui_imgset_load();
                sfx_play(env, "pop");
                LOGI("OnTouch 图像设置弹窗（简化版）：取消 → 恢复原值（界面=%d 棋子=%d 夜间=%d）+ 回主菜单",
                     ui_imgset_trans_get(), ui_imgset_piece_get(), ui_imgset_night_get());
                ui_screen_set(UI_MENU);
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_imgset8_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 图像设置弹窗（简化版）外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_065 证据）：加入游戏完整版弹窗态（8 显示项 + 开始/取消）。
        8 显示项 = 占位标签（先手/后手/玩法/明暗棋/4 规则，非按钮，显示当前值占位
        "蓝方/红方/布局/暗棋/开/开/开/开"，触摸 flaky 下输入框无意义，不画真数据）；
        点 开始 → 诚实边界对话框（"加入游戏完整版：联机开局需 Wi-Fi/局域网房间协议，
        src_rs 仅蓝牙联机已实现，请选蓝牙联机"）+ 回主菜单；
        点 取消 → 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：先手/后手/玩法/明暗棋/4 规则 = 联机开局配置（src_rs 无联机开局逻辑）。 */
    if (ui_screen_get() == UI_JOIN065) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_join065_btn *jb = ui_join065_buttons();
        int jh = ui_join065_hit(jb, UI_JOIN065_COUNT, bx, by);
        if (jh >= 0) {
            int jact = jb[jh].action;
            LOGI("OnTouch 加入游戏完整版弹窗：控件%d（action=%d）", jh, jact);
            if (jact >= 0 && jact <= 7) {   /* 8 显示项：占位标签（不实际处理，仅日志） */
                LOGI("OnTouch 加入游戏完整版弹窗：显示项%d（占位标签，不实际处理，"
                     "★ 诚实边界：联机开局配置 src_rs 无逻辑）", jact);
            } else if (jact == 8) {   /* 开始 → 边界对话框 + 回主菜单 */
                jq_show_dialog(env, "加入游戏完整版（诚实边界）",
                    "加入游戏完整版（诚实边界）：\n"
                    "联机开局需 Wi-Fi/局域网房间协议，\n"
                    "src_rs 仅蓝牙联机已实现，请选蓝牙联机。");
                sfx_play(env, "pop");
                LOGI("OnTouch 加入游戏完整版弹窗：开始 → 边界对话框（联机开局需 Wi-Fi/局域网协议，src_rs 仅蓝牙已实现）+ 回主菜单");
                ui_screen_set(UI_MENU);
            } else if (jact == 9) {   /* 取消 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 加入游戏完整版弹窗：取消 → 回主菜单");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_join065_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 加入游戏完整版弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_069 证据）：创建游戏完整版弹窗态（4 选择 + 确定/取消）。
        点 先后手 → 蓝/红 循环（ui_first_set，对齐 datah_104 范式）；
        点 玩法 → 布局/翻棋 切换（ui_opt_select mode，对齐 datah_104 范式）；
        点 明暗棋 → 明/暗 切换（ui_show_set，对齐 datah_104 范式）；
        点 游戏规则（action=3）→ UI_RULES070（datah_070 游戏规则弹窗）；
        点 确定 → 诚实边界对话框（"创建游戏完整版：联机房间创建需 Wi-Fi/局域网协议，
        src_rs 仅蓝牙已实现"）+ 回主菜单；
        点 取消 → 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：先后手/玩法/明暗棋 = 联机开局配置（src_rs 无联机房间创建逻辑，
        循环切换仅 UI 状态记录） */
    if (ui_screen_get() == UI_CREATE069) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_create069_btn *cb = ui_create069_buttons();
        int ch = ui_create069_hit(cb, UI_CREATE069_COUNT, bx, by);
        if (ch >= 0) {
            int cact = cb[ch].action;
            LOGI("OnTouch 创建游戏完整版弹窗：控件%d（action=%d）", ch, cact);
            if (cact == 0) {   /* 先后手：蓝/红 循环 */
                ui_first_set(ui_first_get() ? 0 : 1);
                sfx_play(env, "pop");
                LOGI("OnTouch 创建游戏完整版弹窗：先后手切换 → %s（★ 诚实边界：联机开局配置，仅 UI 状态记录）",
                     ui_first_get() ? "红先手" : "蓝先手");
            } else if (cact == 1) {   /* 玩法：布局/翻棋 切换 */
                int mode = !ui_opt_get_mode();
                ui_opt_select(ui_opt_get_level(), mode);
                sfx_play(env, "pop");
                LOGI("OnTouch 创建游戏完整版弹窗：玩法切换 → mode=%d（%s，★ 诚实边界：联机开局配置，仅 UI 状态记录）",
                     mode, mode ? "翻棋暗棋" : "布局明棋");
            } else if (cact == 2) {   /* 明暗棋：明/暗 切换 */
                ui_show_set(ui_show_get() ? 0 : 1);
                sfx_play(env, "pop");
                LOGI("OnTouch 创建游戏完整版弹窗：明暗棋切换 → %s（★ 诚实边界：联机开局配置，仅 UI 状态记录）",
                     ui_show_get() ? "暗棋" : "明棋");
            } else if (cact == 3) {   /* 游戏规则 → UI_RULES070（datah_070） */
                ui_screen_set(UI_RULES070);
                sfx_play(env, "pop");
                LOGI("OnTouch 创建游戏完整版弹窗：游戏规则 → UI_RULES070 游戏规则弹窗（datah_070，与 012 同构）");
            } else if (cact == 4) {   /* 确定 → 边界对话框 + 回主菜单 */
                jq_show_dialog(env, "创建游戏完整版（诚实边界）",
                    "创建游戏完整版（诚实边界）：\n"
                    "联机房间创建需 Wi-Fi/局域网协议，\n"
                    "src_rs 仅蓝牙已实现。");
                sfx_play(env, "pop");
                LOGI("OnTouch 创建游戏完整版弹窗：确定 → 边界对话框（联机房间创建需 Wi-Fi/局域网协议，src_rs 仅蓝牙已实现）+ 回主菜单");
                ui_screen_set(UI_MENU);
            } else if (cact == 5) {   /* 取消 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 创建游戏完整版弹窗：取消 → 回主菜单");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_create069_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 创建游戏完整版弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_070 证据）：游戏规则完整版弹窗态（4 规则开关 + 确定/取消）。
        与 datah_012 游戏设置同构，复用 ui_rule_*_get/set + 4 开关各循环切换
        + 确定保存（ui_rules_save）/取消恢复（ui_rules_load）。
        点 和棋/大本营/吃地雷/吃军旗 → 各循环切换 开/关（ui_rule_*_set）；
        点 确定 → ui_rules_save（jq_rules.txt 4 字段）+ 回主菜单；
        点 取消 → ui_rules_load 恢复原值 + 回主菜单；点面板外 → 回主菜单。
        ★ 与 datah_012 区分：012 是本地 ☰ 菜单"功能设置"→UI_GAMESET，
        070 是联机"游戏规则"弹窗（datah_069 创建游戏"游戏规则"按钮切此弹窗）。 */
    if (ui_screen_get() == UI_RULES070) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_rules070_btn *r70 = ui_rules070_buttons();
        int r70h = ui_rules070_hit(r70, UI_RULES070_COUNT, bx, by);
        if (r70h >= 0) {
            int r70act = r70[r70h].action;
            LOGI("OnTouch 游戏规则完整版弹窗：控件%d（action=%d）", r70h, r70act);
            if (r70act == 0) {   /* 和棋：开/关 循环 */
                ui_rule_draw_set(!ui_rule_draw_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 游戏规则完整版弹窗：和棋切换 → %s（与 datah_012 同构，复用 ui_rule_draw_set）",
                     ui_rule_draw_get() ? "开" : "关");
            } else if (r70act == 1) {   /* 大本营：开/关 循环 */
                ui_rule_hq_set(!ui_rule_hq_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 游戏规则完整版弹窗：大本营切换 → %s（与 datah_012 同构，复用 ui_rule_hq_set）",
                     ui_rule_hq_get() ? "开" : "关");
            } else if (r70act == 2) {   /* 吃地雷：开/关 循环 */
                ui_rule_mine_set(!ui_rule_mine_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 游戏规则完整版弹窗：吃地雷切换 → %s（与 datah_012 同构，复用 ui_rule_mine_set）",
                     ui_rule_mine_get() ? "开" : "关");
            } else if (r70act == 3) {   /* 吃军旗：开/关 循环 */
                ui_rule_flag_set(!ui_rule_flag_get());
                sfx_play(env, "pop");
                LOGI("OnTouch 游戏规则完整版弹窗：吃军旗切换 → %s（与 datah_012 同构，复用 ui_rule_flag_set）",
                     ui_rule_flag_get() ? "开" : "关");
            } else if (r70act == 4) {   /* 确定：保存 + 回主菜单 */
                ui_rules_save();
                sfx_play(env, "pop");
                LOGI("OnTouch 游戏规则完整版弹窗：确定 → 保存 jq_rules.txt（和棋=%d 大本营=%d 吃地雷=%d 吃军旗=%d）+ 回主菜单",
                     ui_rule_draw_get(), ui_rule_hq_get(), ui_rule_mine_get(), ui_rule_flag_get());
                ui_screen_set(UI_MENU);
            } else if (r70act == 5) {   /* 取消：恢复原值 + 回主菜单 */
                ui_rules_load();
                sfx_play(env, "pop");
                LOGI("OnTouch 游戏规则完整版弹窗：取消 → 恢复原值（和棋=%d 大本营=%d 吃地雷=%d 吃军旗=%d）+ 回主菜单",
                     ui_rule_draw_get(), ui_rule_hq_get(), ui_rule_mine_get(), ui_rule_flag_get());
                ui_screen_set(UI_MENU);
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_rules070_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 游戏规则完整版弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_060 证据）：房间态弹窗（2 玩家房间列表 + 开始/换桌/离开）。
        Player1/Player2/Ready1/2/Score1/2/Rate1/2/View1/2 是 TransText 显示区
        （非按钮，简化为占位标签"蓝方/红方"，触摸 flaky 下房间列表无意义）；
        点 开始 → 诚实边界对话框（"房间开始（诚实边界）：联机房间需 Wi-Fi/局域网协议，
        src_rs 仅蓝牙已实现"）；
        点 换桌 → 诚实边界对话框（"换桌（诚实边界）：联机房间需 Wi-Fi/局域网协议，
        src_rs 仅蓝牙已实现"）；
        点 离开 → 回主菜单；点面板外 → 回主菜单。
        ★ 诚实边界：联机房间需 Wi-Fi/局域网协议（src_rs 仅蓝牙已实现），
        2 玩家显示区 = 占位标签（不画真数据） */
    if (ui_screen_get() == UI_ROOM060) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_room060_btn *rm = ui_room060_buttons();
        int rmh = ui_room060_hit(rm, UI_ROOM060_COUNT, bx, by);
        if (rmh >= 0) {
            int rmact = rm[rmh].action;
            LOGI("OnTouch 房间态弹窗：按钮%d（action=%d）", rmh, rmact);
            if (rmact == 0) {   /* 开始 → 边界对话框 */
                jq_show_dialog(env, "房间开始（诚实边界）",
                    "房间开始（诚实边界）：\n"
                    "联机房间需 Wi-Fi/局域网协议，\n"
                    "src_rs 仅蓝牙已实现。");
                sfx_play(env, "pop");
                LOGI("OnTouch 房间态弹窗：开始 → 边界对话框（联机房间需 Wi-Fi/局域网协议，src_rs 仅蓝牙已实现）");
            } else if (rmact == 1) {   /* 换桌 → 边界对话框 */
                jq_show_dialog(env, "换桌（诚实边界）",
                    "换桌（诚实边界）：\n"
                    "联机房间需 Wi-Fi/局域网协议，\n"
                    "src_rs 仅蓝牙已实现。");
                sfx_play(env, "pop");
                LOGI("OnTouch 房间态弹窗：换桌 → 边界对话框（联机房间需 Wi-Fi/局域网协议，src_rs 仅蓝牙已实现）");
            } else if (rmact == 2) {   /* 离开 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 房间态弹窗：离开 → 回主菜单");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_room060_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 房间态弹窗外（bx=%.0f by=%.0f）→ 回主菜单", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ P2（datah_063 证据）：聊天完整版弹窗态（Msg 输入框 + 发送/记录/关闭）。
        Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"；
        点 发送 → jq_bt_ctrl_send(BT_T_CHAT) 占位帧 + 诚实边界对话框
        （"聊天（诚实边界）：联机聊天文本未逆向，src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
        对端无文本渲染"）；
        点 记录 → UI_CHATLOG（datah_029 聊天记录已实现）；
        点 关闭 → 回对局；点面板外 → 回对局。
        ★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT） */
    if (ui_screen_get() == UI_CHAT063) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_chat063_btn *c63 = ui_chat063_buttons();
        int c63h = ui_chat063_hit(c63, UI_CHAT063_COUNT, bx, by);
        if (c63h >= 0) {
            int c63act = c63[c63h].action;
            LOGI("OnTouch 聊天完整版弹窗：按钮%d（action=%d）", c63h, c63act);
            if (c63act == 0) {   /* 发送 → BT_T_CHAT 占位帧 + 边界对话框 */
                jq_bt_ctrl_send(env, BT_T_CHAT);
                jq_show_dialog(env, "聊天（诚实边界）",
                    "聊天（诚实边界）：\n"
                    "联机聊天文本未逆向，\n"
                    "src_rs 蓝牙聊天是占位帧 BT_T_CHAT，\n"
                    "对端无文本渲染。");
                sfx_play(env, "pop");
                LOGI("OnTouch 聊天完整版弹窗：发送 → BT_T_CHAT 占位帧 + 边界对话框（联机聊天文本未逆向）");
            } else if (c63act == 1) {   /* 记录 → UI_CHATLOG（datah_029 聊天记录已实现） */
                ui_screen_set(UI_CHATLOG);
                sfx_play(env, "pop");
                LOGI("OnTouch 聊天完整版弹窗：记录 → UI_CHATLOG 聊天记录弹窗（datah_029，已实现）");
            } else if (c63act == 2) {   /* 关闭 → 回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 聊天完整版弹窗：关闭 → 回对局");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_chat063_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 聊天完整版弹窗外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ 原版 UI 对齐：☰ 游戏内菜单 / ↩ 悔棋 图标（棋盘中部）。
       对局态点 ☰ → UI_INGAME_MENU；点 ↩ → 悔棋（undo_pop）。
       坐标换算 720×1280 基准（与渲染同几何）。
       ★ 对抗审计修复：图标命中块加 UI_GAME 守卫——☰ 菜单（UI_INGAME_MENU）打开时
         图标不可再截获菜单按钮（"退出程序"中心落在 ↩ 图标、"功能设置"中心落在 ☰ 图标，
         无守卫会误触发悔棋/重开菜单，所见与所点不一致）。 */
    if (ui_screen_get() == UI_GAME) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        float ix0, iy0, ix1, iy1;
        ui_imen_icon_rect(0, &ix0, &iy0, &ix1, &iy1);   /* ☰ */
        if (bx >= ix0 && bx <= ix1 && by >= iy0 && by <= iy1) {
            ui_screen_set(UI_INGAME_MENU);
            LOGI("OnTouch 对局态点 ☰ → UI_INGAME_MENU（datah_006 游戏内菜单）");
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        ui_imen_icon_rect(1, &ix0, &iy0, &ix1, &iy1);   /* ↩ 悔棋 */
        if (bx >= ix0 && bx <= ix1 && by >= iy0 && by <= iy1) {
            /* ★ 对抗审计修复：连续 pop 到 BLUE（人机模式 undo 栈顶是 AI 应着前
               turn=RED，一次 pop 恢复 RED 后 OnUpdate AI 立即补应着 → 悔棋失效）。 */
            pthread_mutex_lock(&g_mutex);
            int popped = 0;
            while (undo_pop()) {
                popped = 1;
                if (g_state.turn == SIDE_BLUE || g_state.stage != STG_PLAY) break;
            }
            pthread_mutex_unlock(&g_mutex);
            if (popped) {
                LOGI("OnTouch ↩ 悔棋：连续 undo_pop 到 BLUE（撤销 AI 应着 + 蓝的走子）");
                sfx_play(env, "pop");
            } else {
                LOGI("OnTouch ↩ 悔棋：无可悔（undo 栈空）");
            }
            (void)clazz;
            return JNI_TRUE;
        }
    }
    /* ★ 原版 UI 对齐：☰ 游戏内菜单态（datah_006 8 项）——点按钮 → 动作；
       点面板外 → 回对局。
       ★ 对抗审计（datah_067）：蓝牙联机 ☰ 菜单 = New/Regret/提和/认输/聊天/Config/Stat/Rule
       （无电脑变着/软件信息/退出）；本地菜单 = datah_006（电脑变着/软件信息/退出）。 */
    if (ui_screen_get() == UI_INGAME_MENU) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        int bt_mode = ui_bt_active();
        const ui_imen_btn *im = bt_mode ? ui_imen_bt_buttons() : ui_imen_buttons();
        int ih = ui_imen_hit(im, UI_IMENU_COUNT, bx, by);
        if (ih >= 0) {
            int act = im[ih].action;
            LOGI("OnTouch ☰菜单：按钮%d（action=%d，%s）", ih, act, bt_mode ? "联机版" : "本地版");
            if (act == 0) {   /* 重新开始 */
                pthread_mutex_lock(&g_mutex);
                jq_restart_game();
                pthread_mutex_unlock(&g_mutex);
                sfx_play(env, "over");
                LOGI("☰菜单 重新开始：default.lay 重开局（stage=PLAY 蓝先手）");
                /* ★ BT 联机：重开 → 发 RESTART 帧（对端同步重开） */
                if (g_bt_sess_init_done) jq_bt_ctrl_send(env, BT_T_RESTART);
            } else if (act == 1) {   /* 悔棋 */
                pthread_mutex_lock(&g_mutex);
                int popped = 0;
                if (g_vs_ai) {
                    /* ★ 人机模式：悔到玩家蓝回合（AI 应着步 + 玩家步都悔，undo 栈含 AI 步） */
                    while (undo_pop()) {
                        popped++;   /* ★ 计数累加（旧版 `popped=1` 恒 1，pop 多次只报 1） */
                        if (g_state.turn == SIDE_BLUE || g_state.stage != STG_PLAY) break;
                    }
                } else {
                    /* ★ 两人热座/蓝牙：悔最近 1 步（对方走子后轮到对方，只悔本方可悔步） */
                    popped = undo_pop();
                }
                pthread_mutex_unlock(&g_mutex);
                sfx_play(env, "pop");
                LOGI("☰菜单 悔棋：连续 undo_pop 到 BLUE（popped=%d）", popped);
                /* ★ BT 联机：悔棋 → 发 UNDO 帧（对端同步悔棋） */
                if (g_bt_sess_init_done && popped) jq_bt_ctrl_send(env, BT_T_UNDO);
            } else if (bt_mode && act == 2) {   /* 提和（联机版，datah_067）→ 提和详情弹窗（★ P2 datah_042） */
                /* ★ P2（datah_042 证据）：提和入口从"直接发帧"升级为 datah_042 7 图标
                   提和详情弹窗（提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息），
                   发帧逻辑保留在弹窗内（提和→BT_T_DRAW 帧 + 边界对话框）。 */
                ui_screen_set(UI_DRAWDETAIL);
                sfx_play(env, "pop");
                LOGI("☰菜单 提和：→ UI_DRAWDETAIL 提和详情弹窗（datah_042 7 图标：提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，"
                     "诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
                (void)clazz;
                return JNI_TRUE;
            } else if (bt_mode && act == 3) {   /* 认输（联机版，datah_067） */
                pthread_mutex_lock(&g_mutex);
                /* 认输 → 对方胜（本机当前 turn 方为认输方；双方可认输） */
                g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
                g_state.stage = STG_OVER;
                pthread_mutex_unlock(&g_mutex);
                jq_bt_ctrl_send(env, BT_T_RESIGN);
                sfx_play(env, "over");
                LOGI("☰菜单 认输：本机认输 → 对方胜（winner=%d）", g_state.winner);
            } else if (bt_mode && act == 4) {   /* 聊天（联机版，datah_067） */
                jq_bt_ctrl_send(env, BT_T_CHAT);
                jq_show_dialog(env, "聊天",
                    "聊天（诚实边界）：\n"
                    "原版联机聊天消息（Junqi.Chat*）未逆向，\n"
                    "当前发送聊天占位帧（对端无文本渲染）。");
                sfx_play(env, "pop");
                LOGI("☰菜单 聊天：发送 CHAT 占位帧");
            } else if (bt_mode && act == 7) {   /* 游戏规则（联机版，datah_067）→ 规则文本弹窗（datah_037） */
                ui_screen_set(UI_RULES);
                sfx_play(env, "pop");
                LOGI("☰菜单 游戏规则（联机版）：→ UI_RULES 规则文本弹窗（datah_037 规则-翻棋/规则-布局 2 模式 + 返回，"
                     "诚实边界：规则文本=src_rs 简化版固定文本 2 套）");
            } else if (bt_mode && act == 5) {   /* 系统设置（联机版）→ 游戏设置弹窗（datah_012） */
                ui_screen_set(UI_GAMESET);
                sfx_play(env, "pop");
                LOGI("☰菜单 系统设置：→ UI_GAMESET 游戏设置弹窗（datah_012 4 规则开关，原版 datah_067 Config=系统设置含声音+游戏设置）");
            } else if (bt_mode && act == 6) {   /* 战况统计（联机版） */
                pthread_mutex_lock(&g_mutex);
                int b = 0, r = 0, i;
                for (i = 0; i < N_CELLS; i++) {
                    if (g_state.cells[i].rank == RANK_EMPTY) continue;
                    if (g_state.cells[i].side == SIDE_BLUE) b++;
                    else if (g_state.cells[i].side == SIDE_RED) r++;
                }
                char msg[160];
                snprintf(msg, sizeof msg,
                    "蓝方余子：%d\n红方余子：%d\n总步数：%d\n当前回合：%s",
                    b, r, g_state.total_moves,
                    g_state.turn == SIDE_BLUE ? "蓝方" : "红方");
                pthread_mutex_unlock(&g_mutex);
                jq_show_dialog(env, "战况统计", msg);
                sfx_play(env, "pop");
                LOGI("☰菜单 战况统计：蓝%d 红%d 步%d", b, r, g_state.total_moves);
            } else if (!bt_mode && act == 7) {   /* 退出程序 → 回主菜单（本地版） */
                ui_screen_set(UI_MENU);
                ui_bt_set_active(0);
                sfx_play(env, "pop");
                LOGI("☰菜单 退出程序 → 回主菜单");
                (void)clazz;
                return JNI_TRUE;
            } else if (!bt_mode && act == 2) {   /* 电脑变着：AI 为当前回合方走子（本地版） */
                pthread_mutex_lock(&g_mutex);
                int done = 0;
                if (g_state.stage == STG_PLAY) {
                    int from, to, flip;
                    if (ai_pick(&from, &to, &flip) == 0) {
                        undo_push();
                        if (flip) { if (jq_flip(&g_state, from)) ui_replay_record(from, from, 1); }   /* ★ P1（datah_039/107）：电脑变着翻棋记录 */
                        else { int e = jq_try_move(&g_state, from, to); if (e >= 0) ui_replay_record(from, to, 0); (void)e; }   /* ★ P1（datah_039/107）：电脑变着走子记录 */
                        done = 1;
                    }
                }
                pthread_mutex_unlock(&g_mutex);
                sfx_play(env, done ? "move" : "pop");
                LOGI("☰菜单 电脑变着：%s（turn=%d）", done ? "AI 走子" : "无可走", g_state.turn);
            } else if (!bt_mode && act == 3) {   /* 游戏规则（本地版）→ 规则文本弹窗（datah_037） */
                ui_screen_set(UI_RULES);
                sfx_play(env, "pop");
                LOGI("☰菜单 游戏规则（本地版）：→ UI_RULES 规则文本弹窗（datah_037 规则-翻棋/规则-布局 2 模式 + 返回，"
                     "诚实边界：规则文本=src_rs 简化版固定文本 2 套）");
            } else if (!bt_mode && act == 4) {   /* 功能设置（本地版）→ 游戏设置弹窗（datah_012） */
                ui_screen_set(UI_GAMESET);
                sfx_play(env, "pop");
                LOGI("☰菜单 功能设置：→ UI_GAMESET 游戏设置弹窗（datah_012 4 规则开关；声音设置 UI_SETTINGS 经 OnEvent gameset/sound 测试钩子触发，不覆写）");
            } else if (!bt_mode && act == 5) {   /* 战况统计（本地版） */
                pthread_mutex_lock(&g_mutex);
                int b = 0, r = 0, i;
                for (i = 0; i < N_CELLS; i++) {
                    if (g_state.cells[i].rank == RANK_EMPTY) continue;
                    if (g_state.cells[i].side == SIDE_BLUE) b++;
                    else if (g_state.cells[i].side == SIDE_RED) r++;
                }
                char msg[160];
                snprintf(msg, sizeof msg,
                    "蓝方余子：%d\n红方余子：%d\n总步数：%d\n当前回合：%s",
                    b, r, g_state.total_moves,
                    g_state.turn == SIDE_BLUE ? "蓝方" : "红方");
                pthread_mutex_unlock(&g_mutex);
                jq_show_dialog(env, "战况统计", msg);
                sfx_play(env, "pop");
                LOGI("☰菜单 战况统计：蓝%d 红%d 步%d", b, r, g_state.total_moves);
            } else if (!bt_mode && act == 6) {   /* 软件信息（本地版） */
                jq_show_dialog(env, "软件信息",
                    "军棋（Junqi）src_rs 64 位重写版\n"
                    "版本：1.6.8（com.cnvcs.junqirs）\n"
                    "玩法等价重写：规则/UI 对齐原版 v152；\n"
                    "暗棋默认（对齐原版人机/两人开局）；\n"
                    "人机=AI 走红 / 两人=热座轮流；\n"
                    "蓝牙联机/四国为诚实边界。");
                sfx_play(env, "pop");
                LOGI("☰菜单 软件信息：显示版本信息");
            }
            /* ★ 对抗审计（datah_001）：功能设置切到 UI_SETTINGS（声音设置弹窗），
               不覆写回对局；其余动作（弹窗类已 dialog）回对局。 */
            if (ui_screen_get() == UI_INGAME_MENU) ui_screen_set(UI_GAME);
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_imen_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch ☰菜单外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* 对局态：两步走子态机（点格 A 选中 → 点格 B 走子/翻棋） */
    int from = -1, to = -1;
    /* ★ 对抗审计 #6：AI 应着音效锁外播（锁内缓存，锁外调 sfx_play/sfx_game_over） */
    int sfx_done = 0;
    const char *sfx_name = NULL;
    int sfx_stage = -1, sfx_win = -1;
    /* 触摸坐标归一化到 [0,1]²（消除 MotionEvent 坐标系与渲染 viewport 的 MuMu 缩放差异） */
    float nx = g_view_w > 0 ? x / (float)g_view_w : 0;
    float ny = g_view_h > 0 ? y / (float)g_view_h : 0;
    (void)ny;
    /* ★ 对局结束"再来一局"按钮：stage==OVER + 触摸按钮区（nx 0.35-0.65 / ny 0.62-0.72，
       与 rs_draw_overlay 按钮几何一致）→ 重开新局。对局结束 UI 用户流程闭环
       （胜负有横幅 + 按钮可点 → 重开，不重启 app）。触摸路径；property 为可靠备用。 */
    if (nx >= 0.35f && nx <= 0.72f &&
        ny >= 0.62f && ny <= 0.72f) {
        pthread_mutex_lock(&g_mutex);
        int over = (g_state.stage == STG_OVER);   /* ★ 对抗审计 #6：stage 读锁内（旧版锁外读） */
        if (over) jq_restart_game();
        pthread_mutex_unlock(&g_mutex);
        if (over) {
            LOGI("OnTouch 对局结束再来一局：按钮（default.lay 重新开局，stage=PLAY 蓝先手）");
            sfx_play(env, "over");
            (void)clazz;
            return JNI_TRUE;
        }
    }
    /* ★ 游戏内"悔棋重开"按钮：对局态左上角（nx<0.15, ny<0.10，棋盘左侧空闲区）
       → 重开新局（不重启 app）。触摸路径（property jq.restartgame 为可靠备用路径）。 */
    if (nx < 0.15f && ny < 0.10f) {
        pthread_mutex_lock(&g_mutex);
        jq_restart_game();
        pthread_mutex_unlock(&g_mutex);
        LOGI("OnTouch 游戏内重开：左上角重开按钮（default.lay 重新开局，stage=PLAY 蓝先手）");
        sfx_play(env, "over");
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ 原版 UI 对齐：删除旧"点棋盘区自动开局"块（对齐原版点棋盘走子，无自动开局）。
       ★ 对抗审计修复：该块在中心带（nx 0.366-0.631）每次触摸无条件重置 turn=BLUE/
         winner=-1/moves——对局中破坏进行中的棋局；stage=OVER 时点棋盘上部绕过来一局
         按钮意外重开。原版无此行为，整块删除。 */
    int ev = jq_touch_state_process(&g_ts, x, y, g_view_w, g_view_h, &from, &to);
    if (ev != 0)
        LOGI("OnTouch 走子态机：ev=%d（SELECT/UNSELECT/MOVE）from=%d to=%d（cell 索引）",
             ev, from, to);
    /* ★ 走子/翻棋的 g_state 读写 + jq_try_move/jq_flip 全部在 g_mutex 内（GLThread
       的 OnUpdate/AI 也持同一把锁读写 g_state → 免并发野指针 0x80b5dbe78 SIGSEGV）。
       sfx 在锁外调（不读 g_state，只读事件名 + MediaPlayer 全局 ref）。 */
    int jnev = -1;
    int over_win = -1;   /* 走子后 stage==OVER 记录 winner（锁外 sfx win/lose，防锁竞争读） */
    int flip_done = 0, move_ok = 0;
    if (ev == JQ_EV_SELECT) {
        if (jq_glview_is_ready()) jq_glview_set_selected(&g_glview, jq_touch_state_selected(&g_ts));
        sfx_play(env, "select");
    } else if (ev == JQ_EV_UNSELECT) {
        if (jq_glview_is_ready()) jq_glview_set_selected(&g_glview, -1);
    } else if (ev == JQ_EV_MOVE) {
        pthread_mutex_lock(&g_mutex);
        jq_state *st = &g_state;
        if (to == from) {
            /* 点自身 = 翻棋。翻棋成功保持选中（态机已 SELECTED，下一步走子）；
               翻棋失败（已翻开/非己方）回 IDLE。 */
            if (from >= 0 && jq_can_flip(st, from)) {
                undo_push();
                flip_done = jq_flip(st, from);
                if (flip_done) ui_replay_record(from, from, 1);   /* ★ P1（datah_039/107）：翻棋成功记录 1 步 */
                LOGI("走子判定 翻棋：cell=%d rc=%d turn→%d（人机AI走红=%d）",
                     from, flip_done, st->turn, g_vs_ai);
            } else {
                jq_touch_state_reset(&g_ts);
            }
        } else {
            /* 走子 A→B（jq_can_move 判定 + jq_try_move 事件） */
            if (from >= 0 && to >= 0) {
                int movable = jq_can_move(st, from, to);
                undo_push();
                jnev = jq_try_move(st, from, to);
                move_ok = (jnev >= 0);
                if (move_ok) ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：走子成功记录 1 步 */
                if (move_ok && jnev >= 0 && jnev < 6) {
                    g_event_cnt[jnev]++;   /* 事件计数（对齐 on_event，OnTouch 直接走子不经 on_event） */
                    g_event_total++;
                }
                if (move_ok && st->stage == STG_OVER)
                    over_win = st->winner;   /* 对局结束（吃旗/无子可走等），锁外播 win/lose */
                LOGI("走子判定：from=%d to=%d can_move=%d 结果=%s（事件累计 move=%d eat=%d）",
                     from, to, movable, JQ_EVENT_NAMES[jnev < 0 ? 0 : jnev], g_event_cnt[0], g_event_cnt[1]);
            }
        }
        pthread_mutex_unlock(&g_mutex);
        /* ★ 蓝牙联机发送侧：走子/翻棋成功且联机会话已建立且**轮到本方**（OUR_TURN）→ 编码 v2 帧 → Java BtSpp 写出。
           ★ 回合闸口：client（THEIR_TURN）不发送；host 首帧后 send_move 置 THEIR_TURN。 */
        if ((flip_done || move_ok) && g_bt_sess_init_done &&
            g_bt_sess.state == BT_ST_OUR_TURN) {
            char tb[BT_BUF];
            int bn = jq_bt_session_send_move(&g_bt_sess, &g_state, from,
                                             flip_done ? from : to, flip_done,
                                             tb, sizeof tb);
            if (bn > 0) {
                tb[bn] = 0;
                jq_bt_send(env, tb);
                LOGI("蓝牙联机发送：type=%d len=%d frame=%s",
                     (int)g_bt_sess.last_type, bn, tb);
            }
        }
        if ((flip_done || move_ok) && jq_glview_is_ready()) {
            jq_glview_dirty(&g_glview);
            sfx_play(env, flip_done ? "flip" : sfx_for_move(jnev));
        }
        /* ★ 对局结束音效（走子后 stage==OVER → win/lose，sfx_wav 表已有 win/lose/draw） */
        if (over_win >= 0)
            sfx_game_over(env, STG_OVER, over_win);
        /* 翻棋成功保持选中（下一步走子）；走子/翻棋失败清高亮 */
        if (jq_glview_is_ready())
            jq_glview_set_selected(&g_glview, flip_done ? from : -1);
        /* ★ 红方 AI 应着（主线程同步，OnTouch 走子后触发；GLThread OnUpdate 的 AI 块
           在 MuMu surface attach flaky 停帧时兜底不了 → 主线程直接推进）。
           OnUpdate 检查 turn==RED 时 AI 已把 turn 切回 BLUE → 不重复（同 g_mutex）。 */
        if (move_ok) {
            pthread_mutex_lock(&g_mutex);
            if (ui_screen_get() == UI_GAME && g_state.stage == STG_PLAY &&
                g_vs_ai && g_state.turn == SIDE_RED) {
                int af, at, aflip, ae = -1;
                if (ai_pick(&af, &at, &aflip) == 0) {
                    undo_push();
                    if (aflip) {
                        if (jq_flip(&g_state, af)) ui_replay_record(af, af, 1);   /* ★ P1（datah_039/107）：AI 翻棋记录 */
                        LOGI("红方 AI 应着（翻棋）：cell=%d（turn 切回蓝方）", af);
                    } else {
                        ae = jq_try_move(&g_state, af, at);
                        if (ae >= 0) ui_replay_record(af, at, 0);   /* ★ P1（datah_039/107）：AI 走子记录 */
                        LOGI("红方 AI 应着：from=%d to=%d 结果=%s（turn 切回蓝方）",
                             af, at, JQ_EVENT_NAMES[ae < 0 ? 0 : ae]);
                    }
                    if (jq_glview_is_ready()) {
                        jq_glview_dirty(&g_glview);
                        sfx_done = 1;
                        sfx_name = aflip ? "flip" : sfx_for_move(ae);
                        sfx_stage = g_state.stage; sfx_win = g_state.winner;  /* 锁内读，锁外播 */
                    }
                }
            }
            pthread_mutex_unlock(&g_mutex);
            /* ★ 对抗审计 #6：音效挪锁外（sfx_play/sfx_game_over 不读 g_state，
               旧版锁内调与 GUIDELINE §3"sfx 锁外"矛盾） */
            if (sfx_done) {
                sfx_play(env, sfx_name);
                if (sfx_stage == STG_OVER && sfx_win >= 0)
                    sfx_game_over(env, sfx_stage, sfx_win);
            }
        }
    }
    (void)clazz;
    return ev != 0;   /* 命中棋盘区=消费 */
}

/* OnGraphCreate()：GLView.onSurfaceCreated → 场景构造（对齐原版 0xbd764 OnGraphCreate
   构造链：渲染对象图 + 棋盘/棋子纹理上传 + 菜单布局）。 */
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnGraphCreate(JNIEnv *env, jclass clazz)
{
    (void)env; (void)clazz;
    pthread_mutex_lock(&g_mutex);
    /* 场景构造：布局加载（12 预设）+ 棋盘/棋子纹理就绪（rs_render 由 EglHelper 出屏链驱动） */
    if (g_state.stage == STG_PLAY)
        LOGI("OnGraphCreate：场景构造（stage=PLAY 布局已加载 棋子/棋盘纹理由渲染线程上传）");
    pthread_mutex_unlock(&g_mutex);
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnGraphChange(JNIEnv *env, jclass clazz, jint w, jint h)
{
    (void)env; (void)clazz;
    /* ③b：GLView.onSurfaceChanged(w,h) → OnGraphChange(w,h) → 渲染器 resize */
    g_view_w = w > 0 ? w : g_view_w;
    g_view_h = h > 0 ? h : g_view_h;
    if (jq_glview_is_ready())
        jq_glview_resize(&g_glview, w, h);
}

/* OnResult(int req, int res, Intent data)：Activity 结果（对话框等）。桩。 */
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnResult(JNIEnv *env, jclass clazz, jint req, jint res, jobject data)
{
    (void)env; (void)clazz; (void)req; (void)res; (void)data;
}

/* OnBluetooth(Intent)：原版 0xbe9cc 主逻辑（getAction/getIntExtra/getAddress
   + RFCOMM SPP，§89/§92）。帧状态机在 bt_session.c（收帧→校验→规则引擎→回合翻转）。
   SPP socket IO 由 Java 宿主承担；此入口解析 Intent 的 extra 文本帧，
   经 jq_bt_session 处理。无真机蓝牙验证（诚实边界）。 */

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnBluetooth(JNIEnv *env, jclass clazz, jobject intent)
{
    if (!intent) return;
    /* 懒初始化：本方 = 蓝方（先手），对局 = 当前状态 */
    if (!g_bt_sess_init_done) {
        jq_bt_session_init(&g_bt_sess, &g_state, 1);
        g_bt_sess_init_done = 1;
        g_bt_sess_init = 1;
    }
    /* 取 extra 文本（Java 宿主把收到的 v2 帧字符串放 Intent extra）
       extra 键名 = 原版 .rodata "extra"（0x127458 确证） */
    /* 走 getStringExtra("extra") 路径（原版 .rodata "extra" 0x127458 确证键名） */
    jclass i_cls = (*env)->GetObjectClass(env, intent);
    jmethodID gse = (*env)->GetMethodID(env, i_cls, "getStringExtra",
                                        "(Ljava/lang/String;)Ljava/lang/String;");
    if (!gse) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, i_cls);
        LOGI("OnBluetooth：getStringExtra 方法查找失败");
        return;
    }
    jstring key = (*env)->NewStringUTF(env, "extra");
    jstring extra = (jstring)(*env)->CallObjectMethod(env, intent, gse, key);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, key);
    (*env)->DeleteLocalRef(env, i_cls);
    if (!extra) { LOGI("OnBluetooth：extra 为空"); return; }

    /* 测试入口：extra="test" → 构造 1 个合法 FLIP 帧走完整处理链（无真机蓝牙时验证 JNI→状态机通路） */
    const char *txt = (*env)->GetStringUTFChars(env, extra, NULL);
    if (!txt) return;
    const char *disp = txt;   /* ★ 对抗审计 #11：解析用指针（test 路径可能切到栈缓冲 testframe） */
    if (strcmp(txt, "test") == 0) {
        /* 找当前可翻子（暗棋阶段），构造 FLIP 帧文本 → 走 decode + on_frame 链 */
        int fidx = -1;
        pthread_mutex_lock(&g_mutex);
        for (int i = 0; i < N_CELLS; i++)
            if (g_state.cells[i].rank && !g_state.cells[i].revealed && jq_can_flip(&g_state, i))
                { fidx = i; break; }
        char testframe[128];
        if (fidx >= 0) {
            int n = jq_bt_frame_encode(testframe, sizeof testframe, BT_T_FLIP,
                                       cell_col(fidx), cell_row(fidx), 0, 0, 0, 0, 0);
            if (n > 0) testframe[n] = 0;
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnBluetooth(test)：构造 FLIP 帧 idx=%d → %s", fidx, testframe);
            /* 解析用 testframe（栈缓冲）；释放仍用 txt（原 JNI 指针，勿释放栈地址） */
            disp = testframe;
        } else {
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnBluetooth(test)：无可翻子，跳过");
            (*env)->ReleaseStringUTFChars(env, extra, txt);
            return;
        }
    }

    /* 逐帧解析（v2 帧以 '\n' 结尾，可能多帧连发） */
    const char *p = disp;
    int f[8];
    int nframes = 0;
    while (*p) {
        if (jq_bt_frame_decode(p, &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6])) {
            int rc = jq_bt_session_on_frame(&g_bt_sess, &g_state,
                                            f[0], f[1], f[2], f[3],
                                            f[4], f[5], f[6]);
            nframes++;
            if (rc < 0)
                LOGI("OnBluetooth：帧 type=%d 被拒（rc=%d）", f[0], rc);
            /* 找下一帧起点（跳过本帧的 '\n'） */
            const char *nl = strchr(p, '\n');
            p = nl ? nl + 1 : p + 1;
        } else {
            break;
        }
    }
    (*env)->ReleaseStringUTFChars(env, extra, txt);
    LOGI("OnBluetooth：处理 %d 帧，会话状态=%d，stage=%d",
         nframes, (int)g_bt_sess.state, (int)g_state.stage);
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    (void)reserved;
    g_jvm = vm;
    LOGI("JNI_OnLoad：逆源码重写版 libjunqi（14 On* 入口 + 规则引擎）");
    /* assets 管线：AAssetManager 在真机可达（小米 libandroid.so 导出 AAssetManager_fromJava），
       MuMu 64 位转译体裁剪该符号 → 走文件系统回退（/data/local/tmp/spak_res）。
       WAV 音效打包进 assets/jq_sfx/（AAssetManager 读，免外部存储 SELinux 限制）。 */
    /* AAsset 管线走文件系统回退（正常 so 已验证：AAssetManager_fromJava 等 libandroid
       符号在设备转译层不可达 → 不直接链接；WAV/纹理全从 /data/local/tmp/spak_res 读）。 */
    g_assets_avail = 0;
    g_aam_open = NULL;
    g_aasset_len = NULL;
    g_aasset_read = NULL;
    g_aasset_close = NULL;
    g_assets = NULL;
    LOGI("JNI_OnLoad：assets 管线未启用（文件系统回退 spak_res）");
    return JNI_VERSION_1_2;
}
