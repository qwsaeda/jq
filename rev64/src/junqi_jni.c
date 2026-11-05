/*
 * junqi_jni.c — R1 JNI 入口绑定（手写路线）
 * 14 个 Java_com_cnvcs_App_On* 静态 native，签名与 smali/com/cnvcs/App.smali 完全一致。
 *
 * 约定：
 *   OnCreate 返回 jobject GLView（返回 NULL 时 Java 侧自行 new GLView，见 App.smali）。
 *   OnTouch/OnKey/OnUpdate 返回 jboolean（true=消费）。
 *   OnEvent/OnInput 接收 UTF-8 字符串，派发到游戏逻辑。
 * 线程：GLThread 回调在渲染线程，这里用简单互斥保护全局状态。
 */
#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "junqi_core.h"
#include "spak_load.h"
#include "render_gles2.h"
#include "png_decode.h"
#include "audio_sl.h"
#include "bt_spak.h"
#include "bt_net.h"

#define LOG_TAG "junqi64"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* 全局游戏状态 + 互斥 */
static jq_state g_state;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_assets_dir[512];
static int g_view_w = 0, g_view_h = 0;
static int g_selected = -1;      /* 当前选中格（-1=无） */
static jobject g_activity = NULL;  /* 全局 Activity 引用（胜负 Toast 用，NewGlobalRef） */
static int g_over_notified = 0;    /* 胜负已通知（防重复弹 Toast） */
/* 存档 one.sjq（L74）：进度 + 文件路径（files/one.sjq） */
static jq_save_data g_save;
static char g_save_path[512];
static char g_game_save_path[512];   /* one.sjg 对局进度存档路径 */
static int g_secs = 0;             /* 累计对局秒（OnUpdate 计） */
static int g_frame_cnt = 0;        /* 帧计数（约 60 帧=1 秒） */

/* 棋盘 PNG hash（datah_188 = datah#188，737×751；datax_297 = datax#297，736×736）。
   路径未逆出（运行时拼接 data/{分辨率}/ 前缀），故按 hash 确定性加载。 */
static const uint8_t BOARD_HASH_DATAH[16] = {0x21,0xd9,0x5e,0x03,0xe7,0x69,0x2e,0x98,0xbd,0x16,0x49,0x64,0x72,0x25,0xf8,0x19};

/* 棋子图集 PNG hash（datah#183，212×280，4列×7行，每格53×40）。
   datah#183 图集：红方行0-2=司令军长师长旅长/团长营长连长排长/工兵炸弹地雷军旗
   （rank1-12 行序），蓝方行3-5（同序），行6=状态块（选中/暗棋等）。
   rank→列 (rank-1)%4，子行 (rank-1)/4；红(side=1)=行子行，蓝(side=0)=行3+子行。 */
static const uint8_t PIECE_HASH_DATAH[16] = {0x9e,0x1b,0x7f,0xbc,0x4d,0xfc,0x09,0xc3,0xd2,0x3c,0xdf,0x85,0x47,0xcc,0xa2,0x2b};

/* 从 SPAK 加载棋盘 PNG 并注入渲染（GL 上下文就绪后调用） */
static void load_board_texture(void) {
    uint32_t len = 0;
    const uint8_t *png = spak_get_by_hash(BOARD_HASH_DATAH, &len);
    if (!png || len < 8) { LOGI("棋盘纹理: SPAK 未加载或无资源"); return; }
    unsigned char *rgba = NULL;
    int w = 0, h = 0;
    if (png_decode(png, len, &rgba, &w, &h) == 0) {
        junqi_render_set_board_texture(rgba, w, h);
        LOGI("棋盘纹理: 已注入 %dx%d 真实棋盘 PNG", w, h);
        free(rgba);
    } else {
        LOGI("棋盘纹理: PNG 解码失败 (%u 字节)", len);
    }
}

/* 从 SPAK 加载棋子图集（480×490，12×10 网格）并注入渲染 */
static void load_piece_texture(void) {
    uint32_t len = 0;
    const uint8_t *png = spak_get_by_hash(PIECE_HASH_DATAH, &len);
    if (!png || len < 8) { LOGI("棋子图集: SPAK 未加载或无资源"); return; }
    unsigned char *rgba = NULL;
    int w = 0, h = 0;
    if (png_decode(png, len, &rgba, &w, &h) == 0) {
        junqi_render_set_piece_atlas(rgba, w, h);
        LOGI("棋子图集: 已注入 %dx%d 真实棋子 PNG (4x7 网格)", w, h);
        free(rgba);
    } else {
        LOGI("棋子图集: PNG 解码失败 (%u 字节)", len);
    }
}

/* 把 JNI 字符串拷为 UTF-8（调用后需 free） */
static char *jstr_to_c(JNIEnv *env, jstring js) {
    if (!js) return NULL;
    const char *chars = (*env)->GetStringUTFChars(env, js, NULL);
    if (!chars) return NULL;
    size_t n = strlen(chars);
    char *out = (char *)malloc(n + 1);
    if (out) { memcpy(out, chars, n); out[n] = '\0'; }
    (*env)->ReleaseStringUTFChars(env, js, chars);
    return out;
}

/* ---- JNI 入口 ---- */

/* ---- 胜负通知（native→Java Toast，原版机制，L31）----
   检测 stage==OVER 且未通知过 → Toast.makeText(activity,msg,LENGTH_SHORT).show()。
   Toast 内部经主线程 Handler 派发，任意线程调用安全。 */
static void notify_game_over(JNIEnv *env) {
    if (!env) return;
    if (g_state.stage != JQ_STAGE_OVER) { g_over_notified = 0; return; }
    if (g_over_notified) return;
    g_over_notified = 1;
    /* 对局结果记入存档（L74）：resultEx[0]=玩家胜 [1]=AI胜 [2]=和 [3]=总盘。
       诚实边界：resultEx 语义为 arm64 重建（原版仅确证字段名/格式）。 */
    if (g_state.winner == JQ_WINNER_DRAW)      g_save.resultEx[2]++;
    else if (g_state.winner == JQ_SIDE_BLUE)   g_save.resultEx[0]++;
    else if (g_state.winner == JQ_SIDE_RED)    g_save.resultEx[1]++;
    g_save.resultEx[3]++;
    /* 对局结束：清除对局进度存档 one.sjg（防异常退出后残留过期进度，L90） */
    if (g_game_save_path[0]) remove(g_game_save_path);
    if (!g_activity) { LOGI("胜负通知: 无 activity 引用"); return; }
    const char *msg = (g_state.winner == JQ_WINNER_DRAW) ? "和棋！" :
                      (g_state.winner == JQ_SIDE_BLUE) ? "蓝方胜！" : "红方胜！";
    jclass toast = (*env)->FindClass(env, "android/widget/Toast");
    if (!toast) { LOGI("胜负通知: Toast 类缺失"); return; }
    jmethodID makeText = (*env)->GetStaticMethodID(
        env, toast, "makeText",
        "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
    jmethodID show = (*env)->GetMethodID(env, toast, "show", "()V");
    if (!makeText || !show) { LOGI("胜负通知: makeText/show 缺失"); return; }
    jstring jmsg = (*env)->NewStringUTF(env, msg);
    jobject t = (*env)->CallStaticObjectMethod(env, toast, makeText,
                                               g_activity, jmsg, (jint)0 /* LENGTH_SHORT */);
    if (t) (*env)->CallVoidMethod(env, t, show);
    if (jmsg) (*env)->DeleteLocalRef(env, jmsg);
    LOGI("胜负通知: %s", msg);
}

/* ---- 蓝牙联机：本地走子/翻棋 → 发送 v2 帧（仅连接时） ---- */
static void bt_send_local_move(int from, int to, uint8_t mover) {
    if (!jq_bt_connected()) return;
    jq_bt_send_frame(JQ_BT_T_MOVE, from, to, g_state.board[to].rank,
                     mover, g_state.turn, (int)g_state.moves);
    LOGI("BT send move %d->%d (mover=%d turn=%d)", from, to, mover, g_state.turn);
}
static void bt_send_local_flip(int idx, uint8_t mover) {
    if (!jq_bt_connected()) return;
    jq_bt_send_frame(JQ_BT_T_FLIP, idx, -1, g_state.board[idx].rank,
                     mover, g_state.turn, (int)g_state.moves);
    LOGI("BT send flip %d (mover=%d turn=%d)", idx, mover, g_state.turn);
}

/* ---- 蓝牙帧状态机（reader 线程回调）：解析 v2 帧 → 应用到 g_state ----
   g_mutex 保护 g_state；env 为 reader 线程已 attach 的 JNIEnv（Toast 任意线程安全）。 */
static void bt_on_frame(void *user, JNIEnv *env, int type,
                        int a, int b, int c, int d, int e, int f) {
    (void)user; (void)e; (void)f;
    pthread_mutex_lock(&g_mutex);
    int was_over = (g_state.stage == JQ_STAGE_OVER);
    switch (type) {
    case JQ_BT_T_MOVE:
        if (g_state.stage == JQ_STAGE_OVER) break;
        if (a >= 0 && a < JQ_CELLS && b >= 0 && b < JQ_CELLS) {
            uint8_t et;
            uint8_t side = (uint8_t)d;
            if (g_state.turn == side) {
                jq_try_move(&g_state, a, b, &et);
            } else {
                /* 帧与本地 turn 不一致（重连/失步）：强制按发送方 side 应用 */
                uint8_t old = g_state.turn;
                g_state.turn = side;
                if (jq_try_move(&g_state, a, b, &et) != JQ_MOVE_OK) g_state.turn = old;
            }
            g_selected = -1; junqi_render_set_selected(-1);
            LOGI("BT recv move %d->%d turn=%d stage=%d", a, b, g_state.turn, g_state.stage);
        }
        break;
    case JQ_BT_T_FLIP:
        if (g_state.stage == JQ_STAGE_OVER) break;
        if (a >= 0 && a < JQ_CELLS) {
            uint8_t side = (uint8_t)d;
            if (g_state.stage != JQ_STAGE_DARK && g_state.turn != side) {
                uint8_t old = g_state.turn;
                g_state.turn = side;
                if (!jq_flip(&g_state, a)) g_state.turn = old;
            } else {
                jq_flip(&g_state, a);
            }
            g_selected = -1; junqi_render_set_selected(-1);
            LOGI("BT recv flip %d turn=%d", a, g_state.turn);
        }
        break;
    case JQ_BT_T_UNDO:
        if (jq_undo(&g_state)) {
            g_selected = -1; junqi_render_set_selected(-1);
            junqi_audio_play("move");
        }
        LOGI("BT recv undo");
        break;
    case JQ_BT_T_SYNC_REQ:
        /* 对端请求全盘同步 → 回发本端完整状态（stage/turn + 60 格） */
        LOGI("BT recv sync_req → 回发全盘同步");
        jq_bt_send_sync(env, &g_state);
        break;
    case JQ_BT_T_SYNC:
        /* 全盘同步：c=stage, d=turn（后随 C 行逐格应用） */
        if (c >= 0 && c <= JQ_STAGE_DARK) g_state.stage = (jq_stage)c;
        if (d == JQ_SIDE_BLUE || d == JQ_SIDE_RED) g_state.turn = (uint8_t)d;
        g_selected = -1; junqi_render_set_selected(-1);
        junqi_render_set_setup(g_state.stage == JQ_STAGE_SETUP);
        LOGI("BT recv sync stage=%d turn=%d", g_state.stage, g_state.turn);
        break;
    case JQ_BT_T_SYNC_CELL:
        if (a >= 0 && a < JQ_CELLS) {
            jq_cell *cell = &g_state.board[a];
            cell->rank = (uint8_t)b;
            cell->side = (uint8_t)c;
            cell->revealed = (uint8_t)d;
            cell->is_flag = (b == JQ_RANK_FLAG);
            cell->is_mine = (b == JQ_RANK_MINE);
            cell->is_bomb = (b == JQ_RANK_BOMB);
        }
        break;
    case JQ_BT_T_RESTART:
        {
            uint32_t lay_len = 0;
            const uint8_t *lay = spak_get("data/lay/default.lay", &lay_len);
            if (lay && lay_len >= 30) jq_setup_begin(&g_state, lay);
            else jq_dark_setup(&g_state, (uint32_t)(uintptr_t)&g_state);
            g_selected = -1; junqi_render_set_selected(-1);
            g_over_notified = 0;
            junqi_audio_play("start");
        }
        LOGI("BT recv restart");
        break;
    case JQ_BT_T_RESIGN:
        if (a == JQ_SIDE_BLUE || a == JQ_SIDE_RED) {
            g_state.winner = (uint8_t)a;
            g_state.stage = JQ_STAGE_OVER;
        }
        LOGI("BT recv resign winner=%d", a);
        break;
    default:
        LOGI("BT recv 未知帧 type=%d", type);
        break;
    }
    if (g_state.stage == JQ_STAGE_OVER && !was_over) {
        junqi_audio_play(g_state.winner == JQ_WINNER_DRAW ? "draw" :
                         g_state.winner == JQ_SIDE_BLUE ? "win" : "lose");
    }
    pthread_mutex_unlock(&g_mutex);
    notify_game_over(env);   /* 胜负 Toast（reader 线程，任意线程安全） */
}

JNIEXPORT jobject JNICALL
Java_com_cnvcs_App_OnCreate(JNIEnv *env, jclass clazz, jobject activity) {
    (void)clazz;
    pthread_mutex_lock(&g_mutex);
    /* 保存 Activity 全局引用（胜负 Toast 通知用，L31） */
    if (activity && !g_activity) {
        g_activity = (*env)->NewGlobalRef(env, activity);
        LOGI("OnCreate: 已保存 Activity 全局引用");
    }
    /* 蓝牙联机：初始化（默认适配器）+ 注册帧状态机回调（bt_on_frame） */
    jq_bt_init(env, activity);
    jq_bt_set_frame_cb(bt_on_frame, NULL);
    /* 从 Activity 的 AssetManager 加载 SPAK 资源（APK assets 无文件路径）。
       先用 assetmanager 变体；失败则回退外部注入的 g_assets_dir。 */
    if (activity) {
        jclass actcls = (*env)->GetObjectClass(env, activity);
        jmethodID getAssets = (*env)->GetMethodID(env, actcls, "getAssets",
                                                  "()Landroid/content/res/AssetManager;");
        if (getAssets) {
            jobject am = (*env)->CallObjectMethod(env, activity, getAssets);
            if (am) {
                spak_load_assetmanager((void *)am);
                LOGI("OnCreate: SPAK 已从 AssetManager 加载");
            }
        }
        /* 存档路径 = getFilesDir()/one.sjq（L74）。 */
        jmethodID getFiles = (*env)->GetMethodID(env, actcls, "getFilesDir",
                                                 "()Ljava/io/File;");
        if (getFiles) {
            jobject fdir = (*env)->CallObjectMethod(env, activity, getFiles);
            if (fdir) {
                jclass fcls = (*env)->GetObjectClass(env, fdir);
                jmethodID getPath = (*env)->GetMethodID(env, fcls, "getAbsolutePath",
                                                        "()Ljava/lang/String;");
                if (getPath) {
                    jstring jp = (*env)->CallObjectMethod(env, fdir, getPath);
                    const char *p = (*env)->GetStringUTFChars(env, jp, NULL);
                    if (p) {
                        snprintf(g_save_path, sizeof(g_save_path), "%s/one.sjq", p);
                        snprintf(g_game_save_path, sizeof(g_game_save_path), "%s/one.sjg", p);
                        (*env)->ReleaseStringUTFChars(env, jp, p);
                        LOGI("OnCreate: 存档路径 %s / 对局 %s", g_save_path, g_game_save_path);
                    }
                }
            }
        }
    } else if (g_assets_dir[0]) {
        spak_load(g_assets_dir);
        LOGI("OnCreate: SPAK 已从 %s 加载", g_assets_dir);
    }
    /* 优先尝试续局（one.sjg 对局进度存档，L89）：若存在未结束对局则恢复。
       否则用原版 default.lay 布局进入布阵阶段（L80）；再无则回退暗棋。 */
    int resumed = 0;
    if (g_game_save_path[0]) {
        jq_state saved;
        if (jq_save_game_read(g_game_save_path, &saved)) {
            if (saved.stage == JQ_STAGE_PLAY || saved.stage == JQ_STAGE_DARK) {
                g_state = saved;
                resumed = 1;
                LOGI("OnCreate: 续局 one.sjg（stage=%d turn=%d moves=%u）",
                     g_state.stage, g_state.turn, g_state.moves);
            }
        }
    }
    if (!resumed) {
        uint32_t lay_len = 0;
        const uint8_t *lay = spak_get("data/lay/default.lay", &lay_len);
        if (lay && lay_len >= 30) {
            jq_setup_begin(&g_state, lay);
            LOGI("OnCreate: 使用原版 default.lay 布局（布阵阶段）");
        } else {
            jq_dark_setup(&g_state, (uint32_t)(uintptr_t)&g_state);
            LOGI("OnCreate: 回退暗棋随机布局");
        }
    }
    pthread_mutex_unlock(&g_mutex);
    junqi_render_set_setup(g_state.stage == JQ_STAGE_SETUP);
    junqi_audio_init();   /* OpenSL ES（OnCreate 后初始化） */
    LOGI("OnCreate: state initialized (%d cells)", JQ_CELLS);
    return NULL;   /* Java 侧 new GLView */
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnDestroy(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    pthread_mutex_lock(&g_mutex);
    jq_state_reset(&g_state);
    pthread_mutex_unlock(&g_mutex);
    junqi_audio_shutdown();
    jq_bt_shutdown(env);   /* 蓝牙联机：关 socket、停线程、清全局引用 */
    if (g_activity) { (*env)->DeleteGlobalRef(env, g_activity); g_activity = NULL; }
    g_over_notified = 0;
    LOGI("OnDestroy");
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnPause(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    pthread_mutex_lock(&g_mutex);
    /* 对局进度存档 one.sjg（L89）：对局中/暗棋中保存，供退出重进续局 */
    if (g_game_save_path[0] &&
        (g_state.stage == JQ_STAGE_PLAY || g_state.stage == JQ_STAGE_DARK)) {
        int ok = jq_save_game_write(g_game_save_path, &g_state);
        LOGI("OnPause: 对局进度 %s (%d)", g_game_save_path, ok);
    } else if (g_game_save_path[0]) {
        remove(g_game_save_path);   /* 非对局中（布阵/结束）不保存进度 */
    }
    pthread_mutex_unlock(&g_mutex);
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnResume(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz; LOGI("OnResume");
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnTimer(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz; LOGI("OnTimer");
}

JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnUpdate(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    pthread_mutex_lock(&g_mutex);
    /* 对局秒计（约 60 帧/秒，L74 time 字段） */
    if (++g_frame_cnt >= 60) { g_secs++; g_frame_cnt = 0; }
    /* 同步布阵模式到渲染（确认按钮/棋盘缩放），避免阶段切换后 UI 不同步 */
    junqi_render_set_setup(g_state.stage == JQ_STAGE_SETUP);
    /* 单机对弈：若轮到红方（AI），自动走一步；蓝方是玩家（触屏）。
       蓝牙联机（双方真人）时关闭 AI，走子由对端帧驱动。 */
    if (g_state.stage != JQ_STAGE_OVER && g_state.turn == JQ_SIDE_RED &&
        !jq_bt_connected()) {
        int act = jq_ai_move(&g_state);
        if (act) {
            LOGI("OnUpdate: AI (red) action=%d, turn now %d", act, g_state.turn);
            if (g_state.stage == JQ_STAGE_OVER)
                junqi_audio_play(g_state.winner == JQ_WINNER_DRAW ? "draw" :
                                 g_state.winner == JQ_SIDE_BLUE ? "win" : "lose");
            else if (act == 2) junqi_audio_play("flip");
            else if (act == 3) junqi_audio_play("eat");
            else junqi_audio_play("move");
        }
    }
    junqi_render_frame(&g_state);
    pthread_mutex_unlock(&g_mutex);
    notify_game_over(env);   /* 胜负 Toast（L31） */
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnGraphCreate(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    pthread_mutex_lock(&g_mutex);
    junqi_render_init(g_view_w, g_view_h);
    load_board_texture();
    load_piece_texture();
    pthread_mutex_unlock(&g_mutex);
    LOGI("OnGraphCreate");
}
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnGraphChange(JNIEnv *env, jclass clazz, jint w, jint h) {
    (void)env; (void)clazz;
    pthread_mutex_lock(&g_mutex);
    g_view_w = w; g_view_h = h;
    junqi_render_init(w, h);
    pthread_mutex_unlock(&g_mutex);
    LOGI("OnGraphChange %dx%d", w, h);
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnResult(JNIEnv *env, jclass clazz, jint req, jint res, jobject data) {
    (void)env; (void)clazz; (void)req; (void)res; (void)data;
    LOGI("OnResult req=%d res=%d", req, res);
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnBluetooth(JNIEnv *env, jclass clazz, jobject intent) {
    (void)clazz;
    /* 蓝牙联机（方案A：native JNI 直调 Java 蓝牙 API）：
       解析 Intent action（getAction）+ EXTRA_DEVICE → 状态机
       （FOUND→连接 / SCAN_MODE_CHANGED→监听 / DISCOVERY→日志）。 */
    jq_bt_handle_intent(env, intent);
}

/* OnEvent(name, value)：事件派发（走子/翻棋/悔棋/联机等） */
JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnEvent(JNIEnv *env, jclass clazz, jstring name, jstring value) {
    (void)clazz;
    char *n = jstr_to_c(env, name);
    char *v = jstr_to_c(env, value);
    pthread_mutex_lock(&g_mutex);
    LOGI("OnEvent %s=%s", n ? n : "(null)", v ? v : "(null)");
    /* 简单命令映射：move:<from>,<to> / flip:<idx> / undo / restart */
    if (n && v) {
        if (strcmp(n, "move") == 0) {
            int from = -1, to = -1;
            if (sscanf(v, "%d,%d", &from, &to) == 2) {
                uint8_t et;
                uint8_t mover = g_state.turn;   /* 移动前捕获（jq_try_move 后翻转） */
                if (jq_try_move(&g_state, from, to, &et) == JQ_MOVE_OK)
                    bt_send_local_move(from, to, mover);
                /* 音效：0=move 1=eat 2=bomb 3=flag 4=dead 5=mine（L71 原版音效分类） */
                const char *sfx[6] = {"move","eat","bomb","flag","dead","eat"};
                junqi_audio_play(sfx[et]);
            }
        } else if (strcmp(n, "flip") == 0) {
            int idx = -1;
            if (sscanf(v, "%d", &idx) == 1) {
                uint8_t mover = g_state.turn;
                if (jq_flip(&g_state, idx))
                    bt_send_local_flip(idx, mover);
            }
        } else if (strcmp(n, "undo") == 0) {
            if (jq_undo(&g_state)) {
                junqi_audio_play("move");
                g_selected = -1; junqi_render_set_selected(-1);   /* 悔棋后重置选中（L41） */
                if (jq_bt_connected())
                    jq_bt_send_frame(JQ_BT_T_UNDO, -1, -1, -1, -1, -1, -1);
            }
        } else if (strcmp(n, "restart") == 0) {
            uint32_t lay_len = 0;
            const uint8_t *lay = spak_get("data/lay/default.lay", &lay_len);
            if (lay && lay_len >= 30) jq_setup_begin(&g_state, lay);
            else jq_dark_setup(&g_state, (uint32_t)(uintptr_t)&g_state);
            g_selected = -1; junqi_render_set_selected(-1);
            junqi_render_set_setup(g_state.stage == JQ_STAGE_SETUP);
            g_over_notified = 0;   /* 重开后允许再次胜负通知 */
            junqi_audio_play("start");
            if (jq_bt_connected())
                jq_bt_send_frame(JQ_BT_T_RESTART, -1, -1, -1, -1, -1, -1);
        } else if (strcmp(n, "resign") == 0) {
            if (g_state.stage != JQ_STAGE_OVER) {
                g_state.winner = 1 - g_state.turn;   /* 认输：对方胜 */
                g_state.stage = JQ_STAGE_OVER;
                if (jq_bt_connected())
                    jq_bt_send_frame(JQ_BT_T_RESIGN, g_state.winner, -1, -1, -1, -1, -1);
            }
        } else if (strcmp(n, "bt_enable") == 0) {
            jq_bt_request_enable(env);
        } else if (strcmp(n, "bt_discoverable") == 0) {
            jq_bt_request_discoverable(env, v ? atoi(v) : 120);
        } else if (strcmp(n, "bt_host") == 0) {
            jq_bt_start_host(env);
        } else if (strcmp(n, "bt_discover") == 0) {
            jq_bt_start_discovery(env);
        } else if (strcmp(n, "bt_connect") == 0) {
            jq_bt_connect_addr(env, v ? v : "");
        } else if (strcmp(n, "bt_disconnect") == 0) {
            jq_bt_shutdown(env);
        } else if (strcmp(n, "setup_begin") == 0) {
            uint32_t lay_len = 0;
            const uint8_t *lay = spak_get("data/lay/default.lay", &lay_len);
            jq_setup_begin(&g_state, (lay && lay_len >= 30) ? lay : NULL);
            g_selected = -1; junqi_render_set_selected(-1);
            junqi_render_set_setup(1);
            g_over_notified = 0;
        } else if (strcmp(n, "layout") == 0) {
            /* 布局选择：layout:<0-49> → data/lay/%02d.lay（原版 50 预设 00-49.lay，default.lay==00.lay）。
               §95 确证 SPAK 含 00-49.lay 全唯一（30B=30 格，每字节 rank，值 0-12），arm64 扩展支持全部。 */
            int lidx = -1;
            if (sscanf(v, "%d", &lidx) == 1 && lidx >= 0 && lidx <= 49) {
                char lpath[32];
                snprintf(lpath, sizeof(lpath), "data/lay/%02d.lay", lidx);
                uint32_t lay_len = 0;
                const uint8_t *lay = spak_get(lpath, &lay_len);
                if (lay && lay_len >= 30) {
                    jq_setup_begin(&g_state, lay);
                    g_selected = -1; junqi_render_set_selected(-1);
                    junqi_render_set_setup(g_state.stage == JQ_STAGE_SETUP);
                    g_over_notified = 0;
                    LOGI("OnEvent layout: 已载入 %s (%uB)", lpath, lay_len);
                } else {
                    LOGI("OnEvent layout: %s 未找到", lpath);
                }
            }
        } else if (strcmp(n, "setup_place") == 0) {
            int from = -1, to = -1;
            if (sscanf(v, "%d,%d", &from, &to) == 2) {
                jq_layout_error le = jq_setup_place(&g_state, from, to);
                LOGI("OnEvent setup_place %d->%d = %d", from, to, le);
            }
        } else if (strcmp(n, "setup_confirm") == 0) {
            jq_layout_error le = jq_setup_confirm(&g_state);
            LOGI("OnEvent setup_confirm = %d", le);
            if (le == JQ_LAYOUT_OK) {
                g_selected = -1; junqi_render_set_selected(-1);
                junqi_render_set_setup(0);
            }
        } else if (strcmp(n, "ai_level") == 0) {
            int lv = atoi(v ? v : "1");
            jq_ai_set_level(lv);
            g_save.difficulty = jq_ai_get_level();
            LOGI("OnEvent ai_level = %d (get=%d)", lv, jq_ai_get_level());
        } else if (strcmp(n, "save") == 0) {
            g_save.time = (uint32_t)g_secs;
            g_save.difficulty = jq_ai_get_level();
            int ok = jq_save_write(g_save_path, &g_save);
            LOGI("OnEvent save -> %d (%s)", ok, g_save_path);
        } else if (strcmp(n, "load") == 0) {
            int ok = jq_save_read(g_save_path, &g_save);
            if (ok) jq_ai_set_level(g_save.difficulty);
            LOGI("OnEvent load -> %d time=%u diff=%d level=%d mark=%d rx=%d;%d;%d;%d",
                 ok, g_save.time, g_save.difficulty, g_save.level, g_save.mark,
                 g_save.resultEx[0], g_save.resultEx[1],
                 g_save.resultEx[2], g_save.resultEx[3]);
        }
    }
    pthread_mutex_unlock(&g_mutex);
    free(n); free(v);
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnInput(JNIEnv *env, jclass clazz, jstring text) {
    (void)clazz;
    char *t = jstr_to_c(env, text);
    pthread_mutex_lock(&g_mutex);
    LOGI("OnInput %s", t ? t : "(null)");
    pthread_mutex_unlock(&g_mutex);
    free(t);
}

JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnKey(JNIEnv *env, jclass clazz, jobject keyEvent) {
    (void)env; (void)clazz; (void)keyEvent;
    return JNI_FALSE;   /* 不消费，交 Java 处理 */
}

/* ---- 屏幕坐标 → 棋盘格（与渲染棋盘区域一致，L35）----
   棋盘居中 + 格方形（5:12），棋盘外返回 -1。 */
static int touch_to_cell(float x, float y) {
    if (g_view_w <= 0 || g_view_h <= 0) return -1;
    return junqi_render_touch_cell(x, y, g_view_w, g_view_h);
}

/* ---- 处理一次点击：选中 → 走子/翻棋 状态机 ---- */
static void handle_tap(jq_state *st, int idx) {
    if (idx < 0 || idx >= JQ_CELLS) { g_selected = -1; junqi_render_set_selected(-1); return; }
    /* 终局后禁输入（L41） */
    if (st->stage == JQ_STAGE_OVER) {
        g_selected = -1; junqi_render_set_selected(-1);
        return;
    }
    /* 布阵阶段：点蓝方子=选中，点目标格=放置/交换（rulelay 逐格校验，L80） */
    if (st->stage == JQ_STAGE_SETUP) {
        jq_cell *c = &st->board[idx];
        if (g_selected >= 0 && g_selected != idx) {
            jq_layout_error le = jq_setup_place(st, g_selected, idx);
            if (le == JQ_LAYOUT_OK) {
                LOGI("setup: place %d->%d ok", g_selected, idx);
                junqi_audio_play("move");
                g_selected = -1; junqi_render_set_selected(-1);
            } else {
                /* 逐格校验失败：保留选中，玩家可换目标 */
                LOGI("setup: place %d->%d FAIL code=%d", g_selected, idx, le);
                junqi_audio_play("select");
            }
            return;
        }
        if (c->rank != JQ_EMPTY && c->side == JQ_SIDE_BLUE) {
            g_selected = idx; junqi_render_set_selected(idx);
            LOGI("setup: select %d (rank=%d)", idx, c->rank);
            junqi_audio_play("select");
        } else {
            g_selected = -1; junqi_render_set_selected(-1);  /* 点空格/红方预览：取消 */
        }
        return;
    }
    jq_cell *c = &st->board[idx];

    /* 已选中一格：点另一格 → 尝试走子/吃子 */
    if (g_selected >= 0 && g_selected != idx) {
        uint8_t et;
        uint8_t mover = st->turn;   /* 移动前捕获（jq_try_move 后翻转） */
        jq_move_result r = jq_try_move(st, g_selected, idx, &et);
        if (r == JQ_MOVE_OK) {
            LOGI("touch: move %d→%d ok, turn now %d", g_selected, idx, st->turn);
            bt_send_local_move(g_selected, idx, mover);   /* 蓝牙联机：同步走子帧 */
            /* 音效：0=move 1=eat 2=bomb 3=flag 4=dead 5=mine（L71 原版音效分类）；
               胜负→win/lose/draw 优先。 */
            if (st->stage == JQ_STAGE_OVER) {
                junqi_audio_play(st->winner == JQ_WINNER_DRAW ? "draw" :
                                 st->winner == JQ_SIDE_BLUE ? "win" : "lose");
            } else {
                const char *sfx[6] = {"move","eat","bomb","flag","dead","eat"};
                junqi_audio_play(sfx[et]);
            }
            g_selected = -1; junqi_render_set_selected(-1);
            return;
        }
        /* 走子失败：若点的恰好是己方子则改选它 */
        if (c->rank != JQ_EMPTY && c->side == st->turn) {
            g_selected = idx; junqi_render_set_selected(idx);
            LOGI("touch: reselect %d", idx);
            return;
        }
        g_selected = -1; junqi_render_set_selected(-1);   /* 非法目标：取消选中 */
        return;
    }

    /* 未选中：选己方子 / 翻暗棋 */
    if (c->rank == JQ_EMPTY) { g_selected = -1; junqi_render_set_selected(-1); return; }
    if (!c->revealed) {
        /* 暗棋（未翻开）：翻开 */
        if (jq_flip(st, idx)) {
            LOGI("touch: flip %d (rank=%d side=%d)", idx, c->rank, c->side);
            bt_send_local_flip(idx, 1 - st->turn);   /* 蓝牙联机：同步翻棋帧 */
            junqi_audio_play("flip");
            if (st->stage == JQ_STAGE_OVER)
                junqi_audio_play(st->winner == JQ_WINNER_DRAW ? "draw" :
                                 st->winner == JQ_SIDE_BLUE ? "win" : "lose");
        }
        g_selected = -1; junqi_render_set_selected(-1);
        return;
    }
    if (c->side == st->turn) {
        g_selected = idx; junqi_render_set_selected(idx);
        LOGI("touch: select %d (rank=%d)", idx, c->rank);
        junqi_audio_play("select");
    } else {
        g_selected = -1; junqi_render_set_selected(-1);   /* 点对方子：无效 */
    }
}

JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnTouch(JNIEnv *env, jclass clazz, jobject motionEvent) {
    (void)clazz;
    if (!motionEvent) return JNI_FALSE;
    jclass cls = (*env)->GetObjectClass(env, motionEvent);
    jmethodID getAction = (*env)->GetMethodID(env, cls, "getAction", "()I");
    int action = getAction ? (*env)->CallIntMethod(env, motionEvent, getAction) : -1;
    /* ACTION_DOWN=0 / ACTION_UP=1（Android MotionEvent 常量），在 DOWN 时处理 */
    if (action == 0) {
        /* 读取坐标（getX/getY 在 API 8+，返回 float） */
        jmethodID getX = (*env)->GetMethodID(env, cls, "getX", "()F");
        jmethodID getY = (*env)->GetMethodID(env, cls, "getY", "()F");
        float x = getX ? (*env)->CallFloatMethod(env, motionEvent, getX) : 0;
        float y = getY ? (*env)->CallFloatMethod(env, motionEvent, getY) : 0;
        pthread_mutex_lock(&g_mutex);
        /* 布阵阶段：优先命中确认按钮（rulelay 全量校验 + 进入对局，L80） */
        if (g_state.stage == JQ_STAGE_SETUP &&
            junqi_render_setup_confirm_hit(x, y, g_view_w, g_view_h)) {
            jq_layout_error le = jq_setup_confirm(&g_state);
            if (le == JQ_LAYOUT_OK) {
                g_selected = -1; junqi_render_set_selected(-1);
                junqi_audio_play("start");
                LOGI("setup: confirm OK -> PLAY");
            } else {
                junqi_audio_play("select");
                LOGI("setup: confirm FAIL code=%d", le);
            }
            pthread_mutex_unlock(&g_mutex);
            return JNI_TRUE;
        }
        int idx = touch_to_cell(x, y);
        LOGI("OnTouch DOWN (%.0f,%.0f) -> cell %d", x, y, idx);
        handle_tap(&g_state, idx);
        pthread_mutex_unlock(&g_mutex);
        notify_game_over(env);   /* 胜负 Toast（L31） */
    }
    return JNI_TRUE;
}

/* JNI_OnLoad：返回 JNI 版本 */
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)reserved;
    LOGI("JNI_OnLoad junqi64");
    jq_bt_set_vm(vm);   /* 蓝牙联机：保存 JavaVM（pthread attach 用） */
    return JNI_VERSION_1_6;
}
