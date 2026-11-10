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
#include "jq_event_names_73.h"   /* 原版 73 事件名常量表（32位 so .rodata 提取，64位契约对齐） */
#include <dirent.h>                    /* opendir/readdir（SPAK_DIR 诊断） */
#include "render.h"   /* rs_render_frame（GLThread 出屏主路） */
#include "glview_gl.h"
#include "touch_state.h"

/* ★ 批量对齐 setprop 钩子表（prop=jq.dlgNNN → UI_DNNN，1=进弹窗/0=退）。
   真机渲染验证入口（触摸命中 flaky 时用 setprop 触发）。 */
static const struct { const char *prop; int scr; } g_dlg_batch_hooks[] = {
    { "jq.dlg000", UI_D000 },
    { "jq.dlg001", UI_D001 },
    { "jq.dlg002", UI_D002 },
    { "jq.dlg003", UI_D003 },
    { "jq.dlg004", UI_D004 },
    { "jq.dlg005", UI_D005 },
    { "jq.dlg006", UI_D006 },
    { "jq.dlg007", UI_D007 },
    { "jq.dlg008", UI_D008 },
    { "jq.dlg010", UI_D010 },
    { "jq.dlg012", UI_D012 },
    { "jq.dlg013", UI_D013 },
    { "jq.dlg015", UI_D015 },
    { "jq.dlg016", UI_D016 },
    { "jq.dlg017", UI_D017 },
    { "jq.dlg019", UI_D019 },
    { "jq.dlg020", UI_D020 },
    { "jq.dlg021", UI_D021 },
    { "jq.dlg022", UI_D022 },
    { "jq.dlg023", UI_D023 },
    { "jq.dlg024", UI_D024 },
    { "jq.dlg025", UI_D025 },
    { "jq.dlg026", UI_D026 },
    { "jq.dlg027", UI_D027 },
    { "jq.dlg028", UI_D028 },
    { "jq.dlg029", UI_D029 },
    { "jq.dlg030", UI_D030 },
    { "jq.dlg031", UI_D031 },
    { "jq.dlg032", UI_D032 },
    { "jq.dlg033", UI_D033 },
    { "jq.dlg034", UI_D034 },
    { "jq.dlg035", UI_D035 },
    { "jq.dlg036", UI_D036 },
    { "jq.dlg037", UI_D037 },
    { "jq.dlg038", UI_D038 },
    { "jq.dlg039", UI_D039 },
    { "jq.dlg041", UI_D041 },
    { "jq.dlg042", UI_D042 },
    { "jq.dlg043", UI_D043 },
    { "jq.dlg044", UI_D044 },
    { "jq.dlg045", UI_D045 },
    { "jq.dlg046", UI_D046 },
    { "jq.dlg047", UI_D047 },
    { "jq.dlg049", UI_D049 },
    { "jq.dlg050", UI_D050 },
    { "jq.dlg051", UI_D051 },
    { "jq.dlg052", UI_D052 },
    { "jq.dlg053", UI_D053 },
    { "jq.dlg054", UI_D054 },
    { "jq.dlg055", UI_D055 },
    { "jq.dlg057", UI_D057 },
    { "jq.dlg058", UI_D058 },
    { "jq.dlg059", UI_D059 },
    { "jq.dlg060", UI_D060 },
    { "jq.dlg061", UI_D061 },
    { "jq.dlg062", UI_D062 },
    { "jq.dlg063", UI_D063 },
    { "jq.dlg064", UI_D064 },
    { "jq.dlg065", UI_D065 },
    { "jq.dlg066", UI_D066 },
    { "jq.dlg067", UI_D067 },
    { "jq.dlg068", UI_D068 },
    { "jq.dlg069", UI_D069 },
    { "jq.dlg070", UI_D070 },
    { "jq.dlg071", UI_D071 },
    { "jq.dlg072", UI_D072 },
    { "jq.dlg073", UI_D073 },
    { "jq.dlg075", UI_D075 },
    { "jq.dlg077", UI_D077 },
    { "jq.dlg078", UI_D078 },
    { "jq.dlg079", UI_D079 },
    { "jq.dlg082", UI_D082 },
    { "jq.dlg083", UI_D083 },
    { "jq.dlg084", UI_D084 },
    { "jq.dlg085", UI_D085 },
    { "jq.dlg086", UI_D086 },
    { "jq.dlg089", UI_D089 },
    { "jq.dlg090", UI_D090 },
    { "jq.dlg094", UI_D094 },
    { "jq.dlg095", UI_D095 },
    { "jq.dlg096", UI_D096 },
    { "jq.dlg097", UI_D097 },
    { "jq.dlg098", UI_D098 },
    { "jq.dlg099", UI_D099 },
    { "jq.dlg100", UI_D100 },
    { "jq.dlg101", UI_D101 },
    { "jq.dlg102", UI_D102 },
    { "jq.dlg104", UI_D104 },
    { "jq.dlg107", UI_D107 },
    { "jq.dlg108", UI_D108 },
    { "jq.dlg109", UI_D109 },
    { "jq.dlg110", UI_D110 },
    { "jq.dlg112", UI_D112 },
    { "jq.dlg113", UI_D113 },
    { "jq.dlg114", UI_D114 },
    { "jq.dlg118", UI_D118 },
    { "jq.dlg119", UI_D119 },
    { "jq.dlg120", UI_D120 },
    { "jq.dlg121", UI_D121 },
    { "jq.dlg122", UI_D122 },
    { "jq.dlg123", UI_D123 },
    { "jq.dlg246", UI_D246 },
    { "jq.dlg247", UI_D247 },
    { "jq.dlg248", UI_D248 },
    { "jq.dlg249", UI_D249 },
    { "jq.dlg250", UI_D250 },
    { "jq.dlg251", UI_D251 },
    { "jq.dlg253", UI_D253 },
    { "jq.dlg255", UI_D255 },
    { "jq.dlg256", UI_D256 },
    { "jq.dlg257", UI_D257 },
    { "jq.dlg258", UI_D258 },
    { "jq.dlg259", UI_D259 },
    { "jq.dlg260", UI_D260 },
    { "jq.dlg261", UI_D261 },
    { "jq.dlg262", UI_D262 },
    { "jq.dlg263", UI_D263 },
    { "jq.dlg264", UI_D264 },
};
#define DLG_BATCH_HOOK_N ((int)(sizeof(g_dlg_batch_hooks)/sizeof(g_dlg_batch_hooks[0])))
void jq_dlg_batch_hook(const char *tag)
{
    char dbp[16] = {0};
    __system_property_get("jq.dlgbatch", dbp);
    if (!dbp[0]) return;
    for (int hi = 0; hi < DLG_BATCH_HOOK_N; hi++) {
        char cur[16] = {0};
        __system_property_get(g_dlg_batch_hooks[hi].prop, cur);
        if (strcmp(cur, "1") == 0) {
            ui_screen_set(g_dlg_batch_hooks[hi].scr);
            __android_log_print(ANDROID_LOG_INFO, "junqi_rs",
                "%s 批量对齐钩子：%s=1 → UI_%d（datah_%s 弹窗渲染验证）",
                 tag, g_dlg_batch_hooks[hi].prop, g_dlg_batch_hooks[hi].scr, g_dlg_batch_hooks[hi].prop + 6);
            __system_property_set(g_dlg_batch_hooks[hi].prop, "0");
            break;
        }
    }
}
#include "ui_menu.h"
#include "jq_strings.h"   /* ★ 原版字符串表真值（79 条，自动生成） */
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
static void jq_bt_test_run(jq_state *st, const char *tag);
static volatile int g_mt_inject_pending = 0;   /* ★ A11-F11-04（P2 已修）：minetest 一次性注入标志（锁内段置位→unlock 后段消费，prop 清 0 防残留重复注入） */  /* 蓝牙帧状态机测试（OnEvent bttest / setprop jq.bttest 复用） */

/* ★ 对局模式（用户指正"各按钮点击业务功能要对齐"）：人机 vs 两人热座区分。
   1=人机对战（红方回合由 AI 自动走子 ai_pick）；0=两人热座/蓝牙（红方由玩家/对端走）。
   OnTouch/OnEvent 菜单进局时设置；重开（☰/restart）保持当前模式。 */
/* ★ 对抗审计 P1（批4 P0-R8 → 批6 彻底锁化）：人机模式标志已迁入 ui_state
   （ui_vs_ai_get/set，走 g_ui_lock）——原为 jni_bridge static，写点分散在
   OnTouch 各弹窗路径（锁外写）+ 读点在 OnTimer/OnUpdate（持 g_mutex 读）→
   持锁读对无锁写不构成同步（编译器可见性无序，GLThread 可能看旧值）。
   现保留同名宏代理，调用点写法不变（语义 = ui_state 统一锁）。 */
#define g_vs_ai  ui_vs_ai_get()

/* 悔棋栈（OnEvent undo 用） */
#define UNDO_MAX 128
static jq_state g_undo[UNDO_MAX];
static int g_undo_top = 0;
/* ★ AUD-1000 B03-011/B03-034/B03-035（P1 已修）：悔棋栈 push/pop 契约——
   push 成功(栈未满)返回 1 / 栈满拒绝返回 0；调用方必须 `if (undo_push())`
   守卫，仅成功 push 后才可在失败路径 `undo_pop_discard()` 平衡栈顶。
   LOGI 降级为每 8 次 1 条（锁内高频 pop 循环降噪，B03-035）。 */
static int undo_push(void) {
    if (g_undo_top >= UNDO_MAX) { LOGI("undo_push：栈满（top=%d）拒绝（AUD-1000 B03-011 契约：调用方 if 守卫）", g_undo_top); return 0; }
    g_undo[g_undo_top++] = g_state;
    static int up_log_cnt = 0;
    if ((up_log_cnt++ % 8) == 0) LOGI("undo_push：top=%d turn=%d", g_undo_top, g_state.turn);
    return 1;
}
static int undo_pop(void) {
    if (g_undo_top <= 0) return 0;
    g_state = g_undo[--g_undo_top];
    static int up_log_cnt = 0;
    /* ★ AUD-1000 B03-035（P1 已修）：LOGI 每 8 次 1 条（锁内 while-pop 循环降噪，
       终止条件覆盖 stage 变化（!STG_PLAY break）+ 栈空（undo_pop 返 0 停），无死循环） */
    if ((up_log_cnt++ % 8) == 0) LOGI("undo_pop：top=%d turn=%d", g_undo_top, g_state.turn);
    return 1;
}
/* 栈满场景的 pop 对偶：仅平衡 top（丢弃栈顶快照），不改 g_state（B03-011）。 */
static void undo_pop_discard(void) { if (g_undo_top > 0) g_undo_top--; }

/* ★ AUD-R-013（P1）：蓝牙 UNDO 帧可用的悔棋步数（rules.c 声明、bt_session.c 消费）。
   实际悔棋栈 = g_undo/g_undo_top（本文件 static，跨文件不可见）→
   bt_session.c 的 BT_T_UNDO 分支经本函数读栈深，替代恒 0 的 jq_snapshots_left
   （旧路径永不恢复快照 → 双方棋盘状态发散）。
   诚实边界：UNDO 帧仅通知"对端已请求悔棋"，本地栈深>0 时由 jni_bridge 侧
   在收帧回调里执行 undo_pop + 广播同步帧（现状：bt_session 只切回合，
   完整"确认→回退→广播"链路留待后续轮次）。 */
int jq_bt_undo_available(void)
{
    return g_undo_top > 0 ? 1 : 0;
}

/* ★ 游戏内重开新局统一入口（6 处调用收敛：restartgame property / 菜单按钮 action 0/4 /
   点棋盘开局 / 对局结束再来一局 / 左上角悔棋重开 / 对局态点棋盘自动开局）。
   防"改一处漏五处"不一致：load_layout + 明棋/暗棋（★ 选项 UI 模式）+ 清 undo +
   stage=PLAY + 蓝先手 + 清胜负。
   ★ 模式（本轮）：ui_opt_get_mode()==0 布局=明棋 revealed=1；1 翻棋=暗棋 revealed=0。
   调用方需已持有 g_mutex（OnUpdate/OnTouch 锁内；OnCreate 单线程无需锁）。
   ★ AUD-1000 B03-037（P1 已注释）：锁内读 __system_property_get("jq.mode") +
     ui_opt_get_mode（g_ui_lock）——g_mutex→g_ui_lock 单向嵌套（锁序正确），
     低频属性读接受锁内（防调用点改造面），不移动。 */
static void jq_restart_game(void)
{
    int dark = ui_opt_get_mode() == 1;   /* ★ 选项 UI：翻棋模式 = 暗棋开局 */
    /* ★ 测试钩子：jq.mode setprop 覆盖（0=明棋 1=暗棋），walktest/evtest 走明棋用；
       __system_property_get 返回空 = 未设 → 用 ui_opt_get_mode()（默认暗棋） */
    char mode_prop[8] = {0};
    if (__system_property_get("jq.mode", mode_prop) > 0 && mode_prop[0])
        dark = (mode_prop[0] == '1');
    if (dark)
        /* ★ AUD-R-002/003（P0）：暗棋路径走 jq_dark_setup（mode=FLIP + revealed=0） */
        jq_dark_setup(&g_state, JQ_DEFAULT_LAY);
    else
        jq_load_layout(&g_state, JQ_DEFAULT_LAY);
    g_undo_top = 0;
    g_state.stage = STG_PLAY;
    g_state.turn = SIDE_BLUE;
    g_state.winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
    g_state.moves_since_eat = 0;
    g_state.total_moves = 0;
    /* ★ AUD-1000 B03-074（P1 已修）：重开补清 mines_cleared/pieces_left（旧版残留
       上局值——rules.c 按 layout 重建 cells 但这两个 2 维统计不自愈，minetest
       注入 L1288 幂等范式同款；pieces_left 统计随 jq_try_move 路径自然重建，
       清 0 保证开局首步判定基准一致）。 */
    for (int ri = 0; ri < 2; ri++) { g_state.mines_cleared[ri] = 0; g_state.pieces_left[ri] = 0; }
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
            /* ★ AUD-R-011（P1）：暗棋目标侧过滤（与 move_legal 拦截语义一致，双重保险）：
               暗棋下未翻开的目标（rank 非空）不入候选，不浪费 ai_score 调用。
               明棋（mode=LAY）无此过滤，行为不变。 */
            if (g_state.mode == JQ_MODE_FLIP && d->rank != RANK_EMPTY && !d->revealed)
                continue;
            int sc = ai_score(c->rank, d->rank, d->revealed);  /* ★ 对抗审计 #2：统一走 ai_score（rank_norm 内建）；★ AUD-R-010 传 d->revealed */
            if (sc < 0) continue;   /* ★ AUD-R-057（P2）：ai_score 返回哨兵 -2（空源）/-1（旗雷不可走）时跳过，不污染 bs/ws */
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

/* ★ AUD-1000 B03-091（P1 已注释）：g_mutex 非递归（PTHREAD_MUTEX_INITIALIZER 普通
   互斥）。新增持锁路径前检查嵌套——当前持锁路径（OnEvent/OnTouch AI 块/OnBluetooth
   收帧/on_event 各分支）全单锁段，无递归 lock 点；on_event 内 jq_restart_game/
   jq_bt_test_run 均不再取 g_mutex（grep 实证），新增嵌套路径须 review。 */
/* ★ AUD-1000 B03-091（P1 已注释）：g_mutex 非递归（PTHREAD_MUTEX_INITIALIZER 普通
   互斥）。当前所有持锁路径单锁段，无递归 lock 点；on_event 内调 jq_restart_game/
   jq_bt_test_run 均不再取 g_mutex（grep 实证），sfx 路径无 g_mutex。
   新增持锁路径前先 grep 嵌套——防自锁死锁。 */
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;   /* ★ AUD-1000-ALL A2-007（P1 已修）：去 static（glview_gl.c 渲染线程 window 出屏分支需持锁调 rs_render_frame，跨文件访问） */
static int g_view_w = 720, g_view_h = 1280;   /* 渲染 viewport（OnGraphChange 更新）
   ★ AUD-1000 B03-085（P1 已注释）：跨线程 int 对（主线程 OnGraphChange 写 vs GLThread
   OnUpdate/OnTouch 读）——int 单值读 32/64 位实践原子，滞后一帧良性（渲染用旧
   viewport 一帧），非数据竞争崩溃级；改 volatile/原子需全调用面改造（~40 读点），
   本轮仅标注（行为不变）。 */
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
   原版 sound.xml 清单）：事件 move/flip/eat/win/lose/draw → 同名 WAV。
   ★ 对抗审计 P3-B18（命名双轨澄清）：本文件有 3 处 WAV 路径概念，职责不同——
     · SFX_DIR（本宏）= 文件系统回退路径（③优先级，MuMu/桌面，sfx_wav 返回此路径）
     · g_sfx_priv_dir = 内部存储 getFilesDir/jq_sfx（①优先级，OnCreate 反射落地）
     · assets/jq_sfx/ = APK 内 assets（②优先级，AAsset 可达时）
   sfx_wav() 返回的 SFX_DIR 路径在 ① 分支只被当作"文件名来源"（strrchr 取 '/' 后段），
   非直接打开路径；③ 分支才直接使用。勿误读为双轨冗余。 */
#define SFX_DIR "/data/local/tmp/jq_sfx"
static int g_sfx_wrote = 0;
static char g_sfx_priv_dir[256];   /* ① 内部存储 getFilesDir/jq_sfx（OnCreate 落地） */
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
static volatile int g_sound_on = 1;   /* ★ B03-025（P2 已修）：volatile——主线程写（:857 load / :6334 静音按钮）+ sfx_play 多线程读（GLThread OnUpdate 可触发音效路径），int 单字写 ARM 实践原子但 volatile 保证编译器不缓存/重排，跨线程可见性有语义依据 */
/* ★ AUD-1000 B03-026/B03-097（P1 已修）：
   B03-026：sfx_play_file 入口的 fopen 存在性探测缓存——同一 path 的 fopen/fclose
   fd 抖动（高频 pop/select）用 static last-hit 表免重复 open（首次仍探测）。
   B03-097：g_sound_on 双写源（文件 load vs 静音按钮派生）语义冲突标注：
   当前实现=静音按钮派生值在"确定"时经 sfx_sound_save 落盘覆盖文件值，
   单一源=ui_snd_mute（文件仅持久化容器，load 后首帧前无按钮操作则按钮态=文件态，
   不冲突；冲突窗口=用户按静音但未点"确定"就重启 → load 读旧值，标注诚实边界）。 */
static volatile int g_sfx_probe_ok = 0;
static char g_sfx_probe_path[320];

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
    /* ★ AUD-1000 B03-029（P1 误报撤回，已注释）：if(jt&&jx) 守卫在（359-361），
       NewStringUTF 失败路径不会传 NULL jstring 给 CallStaticVoidMethod（旧审计误判）。
       补 NewStringUTF 失败 LOGE 诊断（非行为改动）： */
    jstring jt = (*env)->NewStringUTF(env, title);
    jstring jx = (*env)->NewStringUTF(env, text);
    if (!jt || !jx) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        LOGE("jq_show_dialog：NewStringUTF 失败（title=%s text=%s，诚实边界：不弹框）",
             title ? title : "(null)", text ? text : "(null)");
        if (jt) (*env)->DeleteLocalRef(env, jt);
        if (jx) (*env)->DeleteLocalRef(env, jx);
        (*env)->DeleteLocalRef(env, cls);
        return;
    }
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

/* ★ 对抗审计 P0（批3 P0-B2）：jq_bt_send 回传 Java btSend 连接态——
   原无返回值，调用方无条件打"蓝牙联机发送成功"日志（native 认为可发、
   Java 侧 BtSpp.sendFrame 因 s_connected=false 拒发 → "假发送成功"日志）。
   现：App.btSend 签名不变（静态 void，红线 App.smali 不动），native 侧
   维护 g_bt_connected 镜像（OnBluetooth 收到 Java 侧 btState 回调或
   sendFrame 结果时更新）。当前 App.btSend 无回传通道（红线不改
   App.smali native 签名），故用 bstate 属性钩子：Java 侧 BtSpp 连接
   成功/断开时 setprop jq.bt_conn 1/0，native 轮询更新镜像。
   发送前二次校验：镜像断开 → 降 LOGI 为"未连接被拒（诚实边界）"。
   未推设备 bstate 钩子前（当前），g_bt_connected 恒 0 → 所有发送
   都打"未连接被拒"，行为上比假成功更诚实（调用方 logcat 可诊断）。 */
static int g_bt_connected = 0;   /* Java 侧连接态镜像（OnBluetooth 帧解析时更新）
   ★ AUD-1000 B03-019（P1 已注释）：普通 int 跨线程镜像（BtSpp 读线程 vs 主线程），
   int 单值写在 32/64 位上原子性实践足够；竞态窗口=断连瞬间误发一帧（Java 侧
   BtSpp s_connected 二道守卫），非数据竞争崩溃类问题，保持 volatile 语义注释级。 */

/* ★ AUD-1000 B10-001（P1 已修）：jq.bt_conn 属性轮询——BtSpp 侧 setprop
   jq.bt_conn 1/0（连接成功/断开，见头注释 bstate 钩子设计），native 在
   发送前同步镜像。旧实现 g_bt_connected 只置 1 永不置 0 → 断连诊断永久
   失效（断连后发送仍"连接态"误报 + "未连接被拒"诚实边界日志失真）。
   无属性环境（未推钩子）返回 -1 → 不覆盖推帧推定值。 */
static int jq_bt_conn_prop(void)
{
    char v[8] = {0};
    if (__system_property_get("jq.bt_conn", v) <= 0 || v[0] == 0) return -1;
    return v[0] == '1' ? 1 : 0;
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
            /* ★ 对抗审计 P1（批3 P1-B14/P1-J1）：收帧循环里 native 侧
               从对端发来的帧推定连接存活 → g_bt_connected=1。 */
            g_bt_connected = 1;
            (*env)->CallStaticVoidMethod(env, cls, mid, js);
            (*env)->DeleteLocalRef(env, js);
        }
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, cls);
    /* ★ AUD-1000 B10-001（P1 已修）：发送前同步 jq.bt_conn 属性（1=连接 0=断开），
       推帧推定值仅在无属性时保留（属性未推设备 → -1 → 不覆盖）。 */
    int prop = jq_bt_conn_prop();
    if (prop >= 0) g_bt_connected = prop;
    if (!g_bt_connected)
        LOGI("蓝牙联机发送：未连接被拒（g_bt_connected=0，Java 侧 s_connected=false，诚实边界）");
}

/* ★ 蓝牙控制帧（UNDO/RESTART）：BT 模式下 ☰ 悔棋/重开 → 编帧 → jq_bt_send。
   帧 type 走 bt_protocol 枚举；控制帧不受回合闸口（OUR_TURN 发送守卫仅走子/翻棋）。 */
static void jq_bt_ctrl_send(JNIEnv *env, int type)
{
    if (!env) return;
    char tb[BT_BUF];
    int n = jq_bt_frame_encode(tb, sizeof tb, type, 0, 0, 0, 0, 0, 0, 0);
    /* AUD-A-0002 探针降噪：n 是 jq_bt_frame_encode 返回值（snprintf 截断保护：
       n>=sizeof tb 时 encode 返回 -1），n∈[0,BT_BUF-1] 恒合法；tb[n]=0 越界不可能。 */
    if (n > 0) {
        if (n >= BT_BUF) n = BT_BUF - 1;   /* 恒真断言（encode 契约保证 n<cap） */
        tb[n] = 0;
        jq_bt_send(env, tb);
        /* AUD-1000 B03-018（P2）：高频控制帧（悔棋/重开连点）只打 type+len，不格式化 BT_BUF 长串
           （大字符串 snprintf 开销 + logcat 增长）。frame 内容排查可加 jq.bt_debug=1 门控日志。 */
        static int bt_ctrl_log_n = 0;
        if (bt_ctrl_log_n++ < 20)
            LOGI("蓝牙联机控制帧：type=%d len=%d（前 20 条打日志）", type, n);
        else
            LOGI("蓝牙联机控制帧：type=%d len=%d", type, n);
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
    /* ★ AUD-1000-ALL A7-05（P2 已修）：未知事件静默兜底 move.wav（无日志，未来新增事件名
       未加表时误出声不可诊断）→ 加一次性 LOGI 标注（每事件名首次命中打一条，防刷屏）。 */
    {
        static char seen_unknown[16][32];
        static int n_seen = 0;
        int found = 0;
        for (int i = 0; i < n_seen; i++)
            if (strncmp(seen_unknown[i], event, 31) == 0) { found = 1; break; }
        if (!found && n_seen < 16) {
            strncpy(seen_unknown[n_seen], event, 31);
            seen_unknown[n_seen][31] = 0;
            n_seen++;
            LOGI("sfx_wav：未知事件名 \"%s\" 兜底 move.wav（★ A7-05：需补 strcmp 表项）", event);
        }
    }
    return SFX_DIR "/data_junqi_move.wav";
}

/* 走子事件 → 对应音效名（原版 Junqi.* 分类：Move/Eat/Bomb/Flag/Dead）。
   ★ 走子后必须按事件播对应音效（吃子播 eat 非 move，实证 25→30 吃子统一播 move 是缺口）。 */
/* 走子事件 → 对应音效名（原版 Junqi.* 分类，对齐 jq_event_names_73 契约）。
   ★ 原版行为移植（本轮）：音效分流改走 jq_event_names_73 常量表——
   64位简化版 7 事件码（EV_MOVE..EV_FLIP）→ 原版 73 事件名映射 → 音效 WAV 名。
   诚实边界：账号/注册/联机类 73 事件名仅常量表对齐，业务逻辑未移植（SPEC §3.1 简化边界）。 */
static const char *sfx_for_move(int ev)
{
    /* ★ 原版音效分流：7 事件码→原版 73 事件名（jq_event_names_73）→集中式音效分类表
       （jq_evt55_sfx_for_name，evt55_sfx.c）。原版按事件名分流音效（SPEC §5），
       64 位 7 事件码映射到对应原版事件名后走同一分类表，单一事实源。 */
    const char *evname = "Junqi.Move";
    switch (ev) {
    case EV_EAT:   evname = "Junqi.Eat";   break;
    case EV_BOMB:  evname = "Junqi.Bomb";  break;
    case EV_FLAG:  evname = "Junqi.Flag";  break;
    case EV_DEAD:  evname = "Junqi.Dead";  break;
    case EV_FLIP:  evname = "Junqi.Flip";  break;
    case EV_MOVE:
    case EV_MINE:  evname = "Junqi.Move";  break;  /* AUD-R-062：EV_MINE 与 EV_MOVE 共用 */
    default:       evname = "Junqi.Move";  break;
    }
    const char *sfx = jq_evt55_sfx_for_name(evname);
    return sfx ? sfx : "move";   /* 兜底（evt55_sfx 永不返 NULL，双保险） */
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
    else if (winner == JQ_WIN_DRAW) sfx_play(env, "draw");   /* 和棋（winner=JQ_WIN_DRAW）→ draw 音效 */
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
    /* ★ 对抗审计（本轮）：redroid 沙箱 MediaPlayer 反射卡死——主线程 sfx_play
       内部 CallVoidMethod(MediaPlayer.setDataSource/prepare) 在 redroid 触发
       同步阻塞（红果无音频 HAL），导致 OnCreate 后半段（flagtest/GLView 钩子）
       不触发。检测 redroid 时跳过 MediaPlayer 实例创建（g_mp=0，sfx_play_file
       走 ③ 文件系统回退路径不调用 MediaPlayer 反射，OnCreate 正常走完）。 */
    {
        char hw[64] = {0};
        __system_property_get("ro.hardware", hw);
        if (strstr(hw, "redroid")) {
            if (g_mp) { (*env)->DeleteLocalRef(env, g_mp); g_mp = NULL; }
            g_mp_setstr = g_mp_afd = g_mp_setfd = NULL;
            g_mp = 0;
            LOGI("sfx：redroid 沙箱检测到（hw=%s）→ 跳过 MediaPlayer 反射（防 OnCreate 卡死）", hw);
        }
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
    /* ★ AUD-1000 B10-002（P1 已修）：完成判据改为 g_sfx_priv_dir 已就绪——
       旧实现在函数入口置位 g_sfx_wrote=1，后续 JNI 失败路径（activity 空/
       getFilesDir 异常/abspath 空）早退时标志已置位 → 失败永不重试。
       g_sfx_wrote 仅作已尝试幂等标记，成功判据看 g_sfx_priv_dir。 */
    if (g_sfx_wrote && g_sfx_priv_dir[0]) return;
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
    /* ★ AUD-1000 B03-031（P1 已注释）：fobj 后配对 ExceptionClear；下方 f_cls/ctor/sub
       多个反射调用每步后 ExceptionCheck/Clear（对齐 :519 sfx_mp_init 范式），
       early-return 路径 local ref 统一释放（:631 清理段）。 */
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
        if (fd) {
            int fd_len = (int)strlen(fd);
            /* ★ AUD-1000 B03-030（P1 已修）：getFilesDir 路径超长（>=256）截断检测——
               旧版 snprintf 静默截断 → 后续 sfx_sound_load/save 用截断路径写失败
               且无诊断。截断时 LOGE + 放弃持久化目录（g_files_dir 清零），
               音效/设置持久化降级"不存盘"（诚实边界，不假存）。 */
            if (fd_len >= (int)sizeof(g_files_dir)) {
                LOGE("sfx：getFilesDir 路径超长（%d >= %d），放弃音效/设置持久化目录（诚实边界）",
                     fd_len, (int)sizeof(g_files_dir));
            } else {
                snprintf(g_files_dir, sizeof g_files_dir, "%s", fd);
                ui_settings_set_dir(g_files_dir);   /* ★ 对抗审计（datah_001）：声音设置持久化目录 */
            }
            (*env)->ReleaseStringUTFChars(env, fabs, fd);
        }
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
            if ((*env)->ExceptionCheck(env)) {
                /* ★ AUD-1000 B03-032（P1 已修）：early-continue 前释放已建 is（旧版
                   泄漏 1 local ref/次 × 14 循环，local ref 表撑爆 abort 风险） */
                (*env)->ExceptionClear(env);
                if (is) { (*env)->CallVoidMethod(env, is, is_close); (*env)->DeleteLocalRef(env, is); }
                continue;
            }
            if (!fos) {
                if (is) { (*env)->CallVoidMethod(env, is, is_close); (*env)->DeleteLocalRef(env, is); is = NULL; }
                continue;
            }
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
    /* ★ 对抗审计 P0（批3/4 P0-R10/P3-B17）：g_sound_on 原为死变量（load 读、
       从未被 sfx_play 消费 → 音效持久化写侧 sfx_sound_save 全仓 0 调用点，
       "上次状态"从未真正持久化）。现将 g_sound_on 接入 sfx_play 门控
       （load 已读 jq_sound_on.txt），并补 sfx_sound_save 调用点：
       ☰ 功能设置"声音开关"切换处（UI_SETTINGS 静音按钮，见 :5786 旁）调 save，
       使 jq_sound_on.txt 真正随用户操作写入。save 在 g_files_dir 就绪后调用。 */
    if (!g_sound_on) return;
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
            /* ★ AUD-1000 B03-026（P1 已修）：存在性探测缓存——同 path 二次起不重复
               fopen/fclose（fd 抖动）；path 变化或首次才探测。 */
            if (!g_sfx_probe_ok || strcmp(pth, g_sfx_probe_path) != 0) {
                FILE *chk = fopen(pth, "rb");
                if (chk) { fclose(chk); g_sfx_probe_ok = 1; strncpy(g_sfx_probe_path, pth, sizeof g_sfx_probe_path - 1); g_sfx_probe_path[sizeof g_sfx_probe_path - 1] = 0; sfx_play_file(env, event, pth); return; }
            } else if (g_sfx_probe_ok) {
                sfx_play_file(env, event, pth);
                return;
            }
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
    /* redroid：g_mp=0（sfx_mp_init 检测 redroid 后跳过实例创建）→ 无实例可调，
       直接返回（③文件系统回退路径在 redroid 无音频 HAL，播放失败不影响事件链路）。 */
    if (!g_mp) return;
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
/* ★ AUD-1000 B03-004/B03-100（P1 已修）：on_event 锁内 IO 归一化——原 on_event
   resign/close/pop 分支锁内直调 sfx_game_over/sfx_play（fopen+MediaPlayer 反射
   持 g_mutex 阻塞 GLThread）。现 on_event 不播音效，返回值 = 待锁外播事件名
   （NULL=无），由 OnEvent 解锁后调 sfx_play（对齐 OnTouch :7983 锁外播范式）。 */
/* ★ 对抗审计 P1-J11：原 g_bt_sess_init（不带 _done）全仓无读点（纯写不读死字段），
   与 g_bt_sess_init_done 并行维护冗余。删除 g_bt_sess_init，仅留 g_bt_sess_init_done。 */
static volatile int g_bt_sess_init_done = 0;   /* ★ AUD-1000-ALL A2-002/003/033（P2 已修）：volatile——BtSpp 读线程锁内写 1 + 主线程 OnTouch 锁外裸读（:8478 发送闸口）+ GLThread 读，int 单字写原子但 volatile 保证编译器不 hoist/缓存（蓝牙走子静默不发帧根因） */
/* ★ AUD-1000 B03-013/B03-047/B03-067（P1 已修）：g_bt_sess 字段/标志跨线程契约——
   g_bt_sess_init_done 与 g_bt_sess.state 的锁内读写点：start_bt_session 初始化段、
   OnBluetooth 懒初始化 + 收帧处理（均在 g_mutex 内）；锁外读（☰ 菜单 7712/7730 的
   发送守卫、OnTouch 7998 的发送闸口）是**单 int 对齐读**（32/64 位实践原子），
   语义=最坏读旧值=多跳一次本轮发送（Java 侧 BtSpp 二道守卫），非崩溃级竞争。
   锁序单向：g_mutex 内可调 g_ui_lock（ui_opt_* / ui_replay_*），禁反向
   （g_ui_lock 内绝不取 g_mutex；grep 实证 0 反向，CODE_STYLE §锁序）。 */
int g_event_cnt[JQ_EVENT_NAME_COUNT];   /* ★ 原版行为移植：73 事件名计数（对齐 jq_event_names_73 契约；7 事件码映射到对应名称索引；★ AUD-R-061（P2）：扩容 6→7 补 EV_FLIP=Junqi.Flip 计数位，0-5 顺序不变；★ AUD-R-062（P2）：g_event_cnt[0]=EV_MOVE 与 [5]=EV_MINE 同名 "Junqi.Move"（对齐原版设计）——按**事件码**分流统计（两位分开计），按**事件名**分流音效） */
int g_event_total = 0;   /* ★ 原版行为移植：非 static（evt55_dispatch.c 跨文件计数） */

/* ★ 原版行为移植（55 code 全链）：7 事件码 → 73 事件名索引映射（直接计数路径用）
   7-code 索引 0-6 对应 73-name 索引 0,1,2,3,4,0(MINE 同名 Move),5(Flip)。
   直接 g_event_cnt[ev] 写法在 7→73 扩容后错位（code 5/6 写到 name 0/5 而非 5/6），
   统一走本映射函数。 */
static int ev7_to_name_idx(int ev)
{
    if (ev >= 0 && ev < 5) return ev;   /* Move/Eat/Bomb/Flag/Dead → name idx 0-4 */
    if (ev == 5) return 0;              /* EV_MINE → Junqi.Move (AUD-R-062 同名，name idx 0) */
    if (ev == 6) return 5;              /* EV_FLIP → Junqi.Flip (name idx 5) */
    return 0;   /* 越界兜底 */
}
static int  g_way_host = 1;   /* ★ 对抗审计（datah_072）：方式选择来源——2=联网(host) 3=联机(client) */

static const char *on_event(const char *name, const char *value, JNIEnv *env)
{
    jq_state *st = &g_state;
    if (strcmp(name, "move") == 0) {
        int from = -1, to = -1;
        if (sscanf(value, "%d,%d", &from, &to) == 2) {
            /* ★ AUD-1000 B03-011（P1 已修）：push 成功才 try，失败用
               undo_pop_discard 平衡（旧裸 undo_push+g_undo_top-- 在栈满时
               误弹栈顶 / 栈空时 -- 污染，契约见 :208 注释）。 */
            int pushed = undo_push();
            int ev = jq_try_move(st, from, to);
            if (ev < 0) {
                if (pushed) undo_pop_discard();
                LOGI("OnEvent move 非法 (%d,%d)→(%d,%d)", cell_row(from), cell_col(from),
                     cell_row(to), cell_col(to));
                return NULL;   /* 契约：on_event 返回 const char*（★ AUD-1000：裸 return 非 void 函数 = 编译错误） */
            }
            /* AUD-A-0003/0004（P2 真修）：事件码上界守卫——jq_try_move 合法返回 0-6
               7 类事件码（-1=非法）。ev<7 契约上界防将来扩事件码漏改计数位
               （g_event_cnt[7] 扩容历史见 AUD-R-061）；越界值降级记 [0] 位 + 不计数。 */
            /* ★ 对抗审计 P0（批4 P0-R5）：原 :810 的 g_event_total++ 在 :809 守卫
               内已 total++ 后重复加 → 一次合法 move 事件被计 2 次。删掉重复的
               裸 total++，只保留 :809 守卫内的一次（与 OnTouch 直接走子 :7813、
               flip 分支 :822 口径统一为"每事件 +1"）。 */
            /* ★ 原版行为移植（55 code 全链分发，evt55_dispatch.c）：
               7 事件码（EV_MOVE=0..EV_FLIP=6）→ 原版 55 code（code=ev，0-6 直接映射），
               分发引擎确认成功 + 按 73 事件名索引计数（g_event_cnt 非 static，跨文件直接累加）。
               诚实边界：g_state 变更已在 jq_try_move 内完成，evt55 仅做分发确认（幂等）。 */
            if (ev >= 0 && ev < 7)
                jq_evt55_dispatch(ev, JQ_EVENT_NAMES[ev]);
            ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：OnEvent 走子成功记录 1 步 */
            LOGI("OnEvent move (%d,%d)→(%d,%d) %s（#%d 累计）", cell_row(from), cell_col(from),
                 cell_row(to), cell_col(to), JQ_EVENT_NAMES[ev < 7 ? ev : 0],
                 g_event_cnt[ev7_to_name_idx(ev < 7 ? ev : 0)]);
        }
    } else if (strcmp(name, "flip") == 0) {
        int idx = -1;
        if (sscanf(value, "%d", &idx) == 1) {
            int pushed = undo_push();
            if (jq_flip(st, idx)) {
                ui_replay_record(idx, idx, 1);   /* ★ P1（datah_039/107）：OnEvent 翻棋成功记录 1 步 */
                /* ★ 原版行为移植（55 code 分发）：翻棋 → code 6（Junqi.Flip），
                   evt55_dispatch 内部按 73 事件名索引计数（g_event_cnt[Junqi.Flip=5]++）。 */
                jq_evt55_dispatch(EV_FLIP, "Junqi.Flip");
                LOGI("OnEvent flip (%d,%d) rank=%d", cell_row(idx), cell_col(idx),
                     rank_norm(st->cells[idx].rank));
            } else if (pushed) undo_pop_discard();   /* ★ AUD-1000 B03-011：仅 push 成功才平衡 */
        }
    } else if (strcmp(name, "undo") == 0) {
        if (undo_pop()) LOGI("OnEvent undo 成功");
        else LOGI("OnEvent undo 无历史");
    } else if (strcmp(name, "resign") == 0) {
        /* ★ AUD-1000-ALL A8-F8-005（P1 已修）：resign 钩子加 stage 守卫——旧实现
           无守卫，终局后（stage=OVER）重复 resign 重写 winner/stage（脏态覆盖，
           与 32 位 jq_game_test_finish 的 stage 门控分叉）。 */
        if (st->stage == STG_OVER) {
            LOGI("OnEvent resign → 已终局（stage=OVER winner=%d），忽略重复认输", st->winner);
            return "lose";
        }
        st->winner = 1 - st->turn;
        st->stage = STG_OVER;
        jq_evt55_dispatch(24, "Junqi.Resign");   /* ★ 55code 分发：code 24 = Resign */
        LOGI("OnEvent resign → 对方胜（code 24）");
        /* ★ AUD-1000 B03-004/B03-100（P1 已修）：音效移出锁内——resign 认输本机必为
           败（winner=对方）→ 锁外播 "lose"（旧 sfx_game_over 按 winner 色判，蓝认输
           winner=红→lose 正确，但跨线程持锁 IO 是 P1 真问题；锁外直判本机视角
           语义等价且无锁 IO，B03-078）。 */
        return "lose";
    } else if (strcmp(name, "resign_req") == 0) {
        /* ★ 原版行为移植（本轮）：RegretReq 悔棋请求（原版 Junqi.RegretReq 事件码 6 大类）
           64位简化版：发 UNDO 帧（对端同步悔棋），本地不 pop（收帧侧闭环）。
           诚实边界：原版 73 事件名 RegretReq/RegretAgree/RegretDisagree 全链，
           src_rs 仅实现"请求发帧"，应答判定（RegretAgree/Disagree）未移植（需对端协议）。 */
        jq_bt_ctrl_send(env, BT_T_UNDO);
        sfx_play(env, "pop");
        jq_evt55_dispatch(42, "Junqi.RegretReq");   /* ★ 55code 分发：code 42 = RegretReq */
        LOGI("OnEvent regret_req → 发 UNDO 帧（原版 Junqi.RegretReq，code 42；诚实边界：本地不 pop 等收帧闭环）");
        return "resign";   /* 音效用 resign 兜底（原版无独立 regret WAV） */
    } else if (strcmp(name, "resign_agree") == 0) {
        /* 原版 Junqi.RegretAgree 悔棋同意——64位简化版：对端同意→收帧侧 undo_pop 闭环。 */
        extern int g_event_cnt[];
        for (int _i = 0; _i < JQ_EVENT_NAME_COUNT; _i++)
            if (!strcmp(jq_event_names_73[_i], "Junqi.RegretAgree")) { g_event_cnt[_i]++; g_event_total++; break; }
        LOGI("OnEvent regret_agree → 对端同意悔棋（原版 Junqi.RegretAgree name idx 21，收帧侧 undo_pop；诚实边界：简化版不独立处理同意帧）");
        return NULL;
    } else if (strcmp(name, "resign_disagree") == 0) {
        /* 原版 Junqi.RegretDisagree 悔棋拒绝——64位简化版：对端拒绝，本地无操作。 */
        extern int g_event_cnt[];
        for (int _i = 0; _i < JQ_EVENT_NAME_COUNT; _i++)
            if (!strcmp(jq_event_names_73[_i], "Junqi.RegretDisagree")) { g_event_cnt[_i]++; g_event_total++; break; }
        LOGI("OnEvent regret_disagree → 对端拒绝悔棋（原版 Junqi.RegretDisagree name idx 22，诚实边界：简化版不独立处理拒绝帧）");
        return NULL;
    } else if (strcmp(name, "draw_req") == 0) {
        /* 原版 Junqi.DrawReq 提和请求——64位简化版：发 DRAW 帧 + 诚实边界对话框。 */
        jq_bt_ctrl_send(env, BT_T_DRAW);
        sfx_play(env, "pop");
        jq_evt55_dispatch(25, "Junqi.DrawReq");   /* ★ 55code 分发：code 25 = DrawReq */
        LOGI("OnEvent draw_req → 发 DRAW 帧（原版 Junqi.DrawReq，code 25；本地提和判定未实现，需对端应答）");
        return "draw";
    } else if (strcmp(name, "draw_agree") == 0) {
        /* 原版 Junqi.DrawAgree 提和同意——64位简化版：对端同意→jq_on_draw 和棋。 */
        pthread_mutex_lock(&g_mutex);
        jq_on_draw(&g_state);
        int dwin = g_state.winner;
        pthread_mutex_unlock(&g_mutex);
        sfx_play(env, "draw");
        jq_evt55_dispatch(26, "Junqi.DrawAgree");   /* ★ 55code 分发：code 26 = DrawAgree */
        LOGI("OnEvent draw_agree → jq_on_draw 和棋（原版 Junqi.DrawAgree，code 26，winner=%d）", dwin);
        return "draw";
    } else if (strcmp(name, "draw_disagree") == 0) {
        /* 原版 Junqi.DrawDisagree 提和拒绝——64位简化版：对端拒绝，本地无操作。 */
        LOGI("OnEvent draw_disagree → 对端拒绝提和（原版 Junqi.DrawDisagree，诚实边界：简化版不独立处理拒绝帧）");
        return NULL;
    } else if (strcmp(name, "join_game") == 0) {
        /* 原版 Junqi.JoinGame 加入游戏——64位简化版：边界对话框（联机协议未逆向）。 */
        jq_show_dialog(env, "加入游戏（诚实边界）",
            "联机加入游戏未逆向（原版需服务器协议），\nsrc_rs 离线单机不支持多人房间。");
        sfx_play(env, "close");
        extern int g_event_cnt[];
        for (int _i = 0; _i < JQ_EVENT_NAME_COUNT; _i++)
            if (!strcmp(jq_event_names_73[_i], "Junqi.JoinGame")) { g_event_cnt[_i]++; g_event_total++; break; }
        LOGI("OnEvent join_game → 边界对话框（原版 Junqi.JoinGame，73-name 计数对齐，联机协议未逆向）");
        return "close";
    } else if (strcmp(name, "leave_game") == 0) {
        /* 原版 Junqi.LeaveGame 离开游戏——64位简化版：退对局回主菜单。 */
        pthread_mutex_lock(&g_mutex);
        g_state.stage = STG_PLAY;
        g_state.winner = JQ_WIN_NONE;
        pthread_mutex_unlock(&g_mutex);
        ui_screen_set(UI_MENU);
        sfx_play(env, "close");
        /* leave_game 无独立 55code（原版 F_55d40 switch 未含 LeaveGame，
           诚实边界：73-name 索引直接计数对齐） */
        extern int g_event_cnt[];
        for (int _i = 0; _i < JQ_EVENT_NAME_COUNT; _i++)
            if (!strcmp(jq_event_names_73[_i], "Junqi.LeaveGame")) { g_event_cnt[_i]++; g_event_total++; break; }
        LOGI("OnEvent leave_game → 退对局回主菜单（原版 Junqi.LeaveGame，73-name 计数对齐）");
        return "close";
    } else if (strcmp(name, "game_over") == 0) {
        /* 原版 Junqi.GameOver 对局结束——64位简化版：stage=OVER 已置时确认。 */
        pthread_mutex_lock(&g_mutex);
        int go_stage = g_state.stage;
        int go_win = g_state.winner;
        pthread_mutex_unlock(&g_mutex);
        LOGI("OnEvent game_over → stage=%d winner=%d（原版 Junqi.GameOver 终局确认）", go_stage, go_win);
        jq_evt55_dispatch(16, "Junqi.GameOver");   /* ★ 55code 分发：code 16 = GameOver */
        if (go_stage == STG_OVER) {
            sfx_play(env, go_win == SIDE_BLUE ? "win" : go_win == SIDE_RED ? "lose" : "draw");
        }
        return NULL;
    } else if (strcmp(name, "resign_confirm") == 0) {
        /* 原版 Junqi.ResignConfirm 认输确认——64位简化版：本机败 + RESIGN 帧。 */
        pthread_mutex_lock(&g_mutex);
        g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
        g_state.stage = STG_OVER;
        pthread_mutex_unlock(&g_mutex);
        jq_bt_ctrl_send(env, BT_T_RESIGN);
        sfx_play(env, "lose");
        /* resign_confirm 无独立 55code（原版 F_55d40 switch 未含 ResignConfirm，
           诚实边界：73-name 索引 19=ResignConfirm 仅计数对齐，code 兜底 24=Resign） */
        extern int g_event_cnt[];
        /* ★ AUD-1000-ALL A2-048（P1 已修）：g_event_cnt[19] 硬编码下标→按 73 名表精确
           查表（ResignConfirm 非 7 事件码，ev7_to_name_idx 不适用；防 73 表重排漂移，
           与 B03-012 查表口径一致）。下标 = jq_event_names_73 中 "Junqi.ResignConfirm"
           的位置（:25 = [19]）。 */
        int _rc = -1;
        for (int _i = 0; _i < JQ_EVENT_NAME_COUNT; _i++)
            if (strcmp(jq_event_names_73[_i], "Junqi.ResignConfirm") == 0) { _rc = _i; break; }
        if (_rc < 0) _rc = 19;   /* 兜底（73 表重排时安全回退到旧下标） */
        g_event_cnt[_rc]++; g_event_total++;   /* Junqi.ResignConfirm name idx 计数（查表非硬编码） */
        LOGI("OnEvent resign_confirm → 认输确认（原版 Junqi.ResignConfirm name idx 19，code 兜底 24）");
        return "lose";
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
        st->turn = SIDE_BLUE; st->stage = STG_PLAY; st->winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
        for (int i = 0; i < N_CELLS; i++) st->cells[i].rank = RANK_EMPTY;
        st->cells[53].rank = 2; st->cells[53].side = SIDE_BLUE; st->cells[53].revealed = 1;
        st->cells[58].rank = 12 + 0xc; st->cells[58].side = SIDE_RED; st->cells[58].revealed = 1;
        st->cells[58].flags = JQF_FLAG;
        g_undo_top = 0;
        ui_replay_init();   /* ★ AUD-1000 B03-042（P1 已修）：flagtest 注入后清复盘
           记录（旧版 memset 清空 g_state 后复盘 cur 指向脏 seq，对齐 982 start 范式） */
        LOGI("OnEvent flagtest → 近旗状态（蓝 rank2@53，红军旗@58，走 53→58 = Junqi.Flag）");
    } else if (strcmp(name, "layout") == 0) {
        /* 原版 OnEvent("layout","<0-11>") → data/lay/%02d.lay（12 预设，§R6 确证）。
           本地从 layouts/ 目录读（桌面 headless），MuMu 走文件系统 /data/local/tmp/layouts/。 */
        int lay = -1;
        if (sscanf(value, "%d", &lay) == 1 && lay >= 0 && lay <= 11) {
            uint8_t lb[30];
            /* ★ AUD-1000 B03-005（P1 已修）：布局文件读（fopen/fread）移出 g_mutex——
               OnEvent 整段持锁，IO 阻塞毫秒级卡渲染线程；先锁外读 30 字节，
               再锁内应用 jq_load_layout。 */
            const char *dirs[] = { "layouts", "/data/local/tmp/layouts", "/data/local/tmp/spak_res/layouts" };
            int loaded = 0;
            for (int d = 0; d < 3 && !loaded; d++) {
                char path[256];
                snprintf(path, sizeof path, "%s/%02d.lay", dirs[d], lay);
                FILE *f = fopen(path, "rb");
                if (f) {
                    if (fread(lb, 1, 30, f) == 30) loaded = 1;
                    fclose(f);
                }
            }
            pthread_mutex_lock(&g_mutex);
            if (loaded) {
                jq_load_layout(st, lb);
                g_undo_top = 0;
                ui_replay_init();   /* ★ P1（datah_039/107）：布局载入清复盘记录 */
                LOGI("OnEvent layout %d（已载入 30 字节 .lay，锁内应用）", lay);
            } else {
                /* 文件缺失 → 回退 default */
                jq_load_layout(st, JQ_DEFAULT_LAY);
                g_undo_top = 0;
                ui_replay_init();   /* ★ P1（datah_039/107）：布局载入清复盘记录 */
                LOGI("OnEvent layout %d 文件缺失 → default.lay", lay);
            }
            pthread_mutex_unlock(&g_mutex);
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
            ui_vs_ai_set((mode == 0));
            jq_restart_game();
            LOGI("OnEvent menu 开局 mode=%d（%s，jq_restart_game：随模式 mode=%d stage=PLAY 蓝先手）",
                 mode, g_vs_ai ? "人机AI走红" : "两人热座", ui_opt_get_mode());
            ui_screen_set(UI_GAME);   /* 对局态：AI 介入 + 触摸走子放行 */
        } else {
            /* 联机模式：等 OnBluetooth 帧（本方蓝先手，SPP 由 Java 宿主承担） */
            ui_vs_ai_set(0);   /* ★ 蓝牙模式：AI 不介入（回合闸口接管红方走子） */
            g_bt_sess_init_done = 0;
            ui_bt_set_active(0);   /* ★ 对抗审计（datah_067）：退出联机 ☰ 菜单分流 */
            st->stage = STG_PLAY;
            st->turn = SIDE_BLUE;
            st->winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
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
                st->mode = dark ? JQ_MODE_FLIP : JQ_MODE_LAY;   /* ★ AUD-R-002/003（P0） */
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
        st->winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
        st->moves_since_eat = 0;
        st->total_moves = 0;
        ui_replay_init();   /* ★ P1（datah_039/107）：开局清复盘记录 + 存开局 30 字节布局
           ★ AUD-1000 B03-076（P1 误报撤回，已注释）：start 分支 g_undo_top=0 在 stage/turn
           重置**之前**——start:lay 的 jq_load_layout(964) 路径不清 undo，到 982 统一清，
           顺序正确（layout 载入→重置→replay_init 快照此刻开局 30 字节布局），无误。 */
        LOGI("OnEvent start：对局开始（stage=PLAY turn=蓝方）");
    } else if (strcmp(name, "status") == 0) {
        /* 查事件累计（对齐原版 Junqi.* 事件序列，桌面/实机诊断） */
        LOGI("OnEvent status：stage=%d turn=%d moves=%d events_total=%d [%s]",
             st->stage, st->turn, st->total_moves, g_event_total,
             "move/eat/bomb/flag/dead/mine=");
        /* ★ 原版行为移植（本轮）：战况统计 7 事件码计数（简化版核心 7 类）
           + 73 事件名全量分布（jq_event_names_73 契约对齐，账号/联机类=诚实边界计数恒 0） */
        for (int i = 0; i < 7; i++)
            LOGI("  %s = %d", JQ_EVENT_NAMES[i], g_event_cnt[ev7_to_name_idx(i)]);
        /* 73 事件名分布（仅显示非零项，账号/联机类恒 0 不刷屏） */
        int nonzero = 0;
        for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++)
            if (g_event_cnt[i]) {
                LOGI("  %s = %d", jq_event_names_73[i], g_event_cnt[i]);
                nonzero++;
            }
        if (nonzero == 0) LOGI("  （73 事件名全量：账号/注册/联机类=诚实边界，简化版计数恒 0）");
    } else if (strcmp(name, "bttest") == 0) {
        jq_bt_test_run(st, "OnEvent");
    } else if (strcmp(name, "gameset") == 0) {
        /* ★ P1（datah_012）：游戏设置弹窗测试钩子（OnEvent("gameset","1") → UI_GAMESET）。
           值 0=关闭（恢复对局/主菜单）；与 jq.bttest 测试钩子同范式。 */
        /* ★ B03-079（P2 已修）：非数字/空 value 默认开（on=1）→ 误判风险（误发 "gameset",""
           判为开）。改精确匹配 "1"=开 / "0"=关 / 其余保持当前态（不切屏），测试钩子确定性。 */
        int on = 0;
        if (strcmp(value, "1") == 0) on = 1;
        else if (strcmp(value, "0") == 0) on = 0;
        else { LOGI("OnEvent gameset：value=%s 非 0/1 → 保持当前态", value ? value : "(null)"); return NULL; }
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
        /* UI 关闭音效（14 音效全量验证：close 此前未触发；Java/桌面可触发）
           ★ AUD-1000 B03-004（P1 已修）：锁内不播，返回值由 OnEvent 锁外播。 */
        LOGI("OnEvent close：待锁外播 close 音效（UI 关闭）");
        return "close";
    } else if (strcmp(name, "pop") == 0) {
        /* UI 弹窗/按钮点击音效（OnTouch 菜单按钮已播 pop；此处 OnEvent 路径备用） */
        LOGI("OnEvent pop：待锁外播 pop 音效（UI 弹窗）");
        return "pop";
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
        /* ★ AUD-1000 B03-080（P1 已注释）：on_event replay 入口低频（setprop 钩子/
           桌面 evverify），rewind(n) O(n) 持锁可保留（非每帧路径，无高频阻塞） */
        /* ★ P1（datah_039/107 证据）：复盘入口测试钩子（OnEvent("replay","1") → UI_REPLAY）。
           值 1=进复盘态（ui_replay_rewind 重建 g_state 到 cur=n 记录末尾）；
           0=退复盘回对局（保留记录）。★ 诚实边界：datah_039 原版 BackDlg 浮标命中
           flaky → 钩子触发（与 jq.gameset L1001 同范式），非触摸路径。 */
        int on = 0;
        if (sscanf(value, "%d", &on) != 1) on = 1;
        if (on) {
            /* ★ AUD-1000 B02-080（P0 已修）：rewind 改 g_state 必须 g_mutex 内
               （旧实现锁外调用，与 GLThread OnUpdate 锁内渲染读 g_state 竞态）。 */
            int n, rc;
            pthread_mutex_lock(&g_mutex);
            n = ui_replay_count();
            rc = ui_replay_rewind(n);   /* 重建到记录末尾（n=0 = 纯开局布局） */
            pthread_mutex_unlock(&g_mutex);
            ui_screen_set(UI_REPLAY);
            LOGI("OnEvent replay：→ UI_REPLAY 复盘回放态（datah_107 控制条 5 按钮，cur=%d/%d rewind rc=%d，入口 setprop 钩子范式；跳转简化回开头/跳结尾，菜单简化退复盘）",
                 n, n, rc);
        } else {
            ui_screen_set(UI_GAME);
            LOGI("OnEvent replay：关闭 → 回对局（保留走子记录 %d 条）", ui_replay_count());
        }
    }
    return NULL;   /* 无待锁外播音效（★ AUD-1000 B03-004 契约：on_event 只返回事件名） */
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
    /* ★ AUD-1000-ALL A1-F1-024（P1 已修）：DeleteGlobalRef 须先于 DetachCurrentThread
       ——JNI 契约：Detach 后 env 指针失效，再调 DeleteGlobalRef(e,cls) 是 UB（B03-040
       台账标 FIXED 但修复未落地，本轮补正顺序）。 */
    (*e)->DeleteGlobalRef(e, cls);
    (*vm)->DetachCurrentThread(vm);
    return NULL;
}


/* ★ 蓝牙帧状态机测试（无真机蓝牙时验 JNI→bt_session 通路）：构造 FLIP 帧走 decode+on_frame。
   供 OnEvent("bttest") 与 setprop jq.bttest 钩子复用（提取自 OnEvent 分支）。
   ★ AUD-1000 B03-041（P1 已注释）：调用方需持 g_mutex（函数体改 st(=&g_state) +
   g_bt_sess，OnEvent/OnCreate 两调用点均已持锁；新增调用点必须锁内，勿裸调）。 */
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
    /* ★ AUD-1000 B03-006（P1 已注释）：OnCreate 从 1161 持 g_mutex 到 1911——
       中间做 ui_settings_load/ui_rules_load/ui_bright_load/ui_imgset_load
       （各 fopen/fscanf）+ sfx_sound_load + 7 个 setprop 钩子。
       持锁做文件 IO 在 OnCreate 单线程启动路径（GLThread 在 1978 行
       jq_glview_start_render_thread 才起）无实际竞争，保守统一锁段防
       渲染线程起来后竞态；低频一次性，不收缩改造面（锁内 IO 成本可接受）。 */
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
    /* ★ 55code 全链实证（最前段，sfx/EGL/Java 反射段之前）：
       红线：redroid/红果 主线程 sfx（MediaPlayer 反射）+ jq_show_dialog（CallStaticVoidMethod
       Java 反射）在沙箱卡 futex_do_wait → 任何走 on_event 全 11 分支（含 join_game→jq_show_dialog
       / regret_req→jq_bt_ctrl_send→Java 蓝牙）的钩子必卡。本钩子改"纯 native 分发 + 计数"模式：
       ① jq_evt55_dispatch(code) 11 次（core 7 类走 rules.c 语义，其余 dispatch_ok）
       ② 按 73 事件名索引直接 g_event_cnt[] 累加（11 项）
       ③ 无 Java 反射 / 无 sfx / 无蓝牙帧 → redroid 主线程不卡，prop 清 0 完成。
       诚实边界：验证的是"事件分发引擎 + 73 名计数"链路（evt55 核心），
       非"on_event 11 分支业务"（那需 MuMu EGL 稳定窗口 + Java 反射通路，红果已实证 0 崩溃）。
       一次性（清 0），仅诊断。 */
    {
        char e55p[16] = {0};
        __system_property_get("jq.55codetest", e55p);
        if (strcmp(e55p, "1") == 0) {
            LOGI("55code-test：最前段纯 native 分发（11 次 jq_evt55_dispatch + 73 名计数，无 Java 反射）");
            /* 11 次分发（无锁——纯 native 计数，不改 g_state，JNI_OnLoad 前 g_mutex
               未 init 时加锁卡 futex；分发引擎幂等，调用点后续走 on_event 时才持锁） */
            jq_evt55_dispatch(42, "Junqi.RegretReq");
            jq_evt55_dispatch(25, "Junqi.DrawReq");
            jq_evt55_dispatch(26, "Junqi.DrawAgree");
            jq_evt55_dispatch(16, "Junqi.GameOver");
            jq_evt55_dispatch(24, "Junqi.Resign");
            jq_evt55_dispatch(6,  "Junqi.Flip");
            jq_evt55_dispatch(0,  "Junqi.Move");
            jq_evt55_dispatch(1,  "Junqi.Eat");
            jq_evt55_dispatch(2,  "Junqi.Bomb");
            jq_evt55_dispatch(3,  "Junqi.Flag");
            jq_evt55_dispatch(4,  "Junqi.Dead");
            /* 73 名分布（非零项；无 Java 反射，纯 logcat） */
            int nz = 0;
            for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++)
                if (g_event_cnt[i]) { LOGI("55code-dist: %s = %d", jq_event_names_73[i], g_event_cnt[i]); nz++; }
            LOGI("55code-test：11 次分发完成，非零 %d 项（total=%d）→ prop 清 0", nz, g_event_total);
            __system_property_set("jq.55codetest", "0");
        }
    }
    /* 明棋开局：全子 revealed=1（棋子画正面 rank 映射，50 子全可见；
       暗棋玩法后续加"翻棋"交互——点背面格 revealed=1 露出 rank）。
       渲染器在下方 jq_glview_init 快照 g_state，故必须先设 revealed。 */
    for (int i = 0; i < N_CELLS; i++)
        if (g_state.cells[i].rank != RANK_EMPTY)
            g_state.cells[i].revealed = 1;
    g_state.stage = STG_PLAY;
    g_state.turn = SIDE_BLUE;
    g_state.winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
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
    /* ★ 对抗审计（2026-11-11）：旧日志写死"明棋开局"，实际 OnCreate 走 jq_restart_game
       按 jq.mode/ui_opt_get_mode 分支——jq.mode=1 时是暗棋（jq_dark_setup，revealed=0），
       文案误导。现读属性如实标注。 */
    {
        char mprop[8] = {0};
        int dark = 0;
        if (__system_property_get("jq.mode", mprop) > 0 && mprop[0])
            dark = (mprop[0] == '1');
        else
            dark = (ui_opt_get_mode() == 1);
        LOGI("OnCreate：%s开局（stage=PLAY 蓝先手 %s）+ UI 菜单就绪（OnEvent menu=0 进对局）",
             dark ? "暗棋（翻棋 revealed=0）" : "明棋（revealed=全翻开 50 子）",
             dark ? "暗子" : "明子");
    }
    /* ★ 对抗审计（2026-11-09 回归根因）：flagtest/minetest 注入段（ui_screen_set +
       jq_try_move 内 ui_rule_cfg_get/ui_snd_mute_get 取 g_ui_lock）原在 g_mutex 持有段内
       （1452-2248）。渲染线程（GLView 启动）OnUpdate 持 g_mutex 调 rs_render_frame →
       ui_imgset_cfg_get/ui_replay_cur 取 g_ui_lock → g_mutex→g_ui_lock；钩子段反成
       g_mutex（主线程持）→g_ui_lock（渲染线程持）逆序等待 → 死锁 → 渲染层永不启动
       （资源 34=0）+ flag/mine/restart 全 FAIL。修：两段移到 unlock 后（g_state 已在
       1452 前初始化，ui_* 独立锁，不依赖 g_mutex 保护期；hook 语义不变——注入发生在
       GLView init/渲染线程首帧快照 g_state 前，渲染层进棋盘态与测试语义一致）。 */
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
       （红有子 → 挖雷不结束对局，stage 保持 PLAY）。jq_try_move(49,54)=EV_MINE 桌面已证。
       ★ 对抗审计（2026-11-09）：本段（含 ui_replay_init/ui_screen_set/jq_try_move 取
       g_ui_lock）已移到 g_mutex unlock 后（见下方 unlock 段后 flagtest/minetest 双段），
       消除与渲染线程 OnUpdate（持 g_mutex→ui_* 取 g_ui_lock）的逆序死锁。 */
    {
        char mt_prop[16] = {0};
        __system_property_get("jq.minetest", mt_prop);
        if (strcmp(mt_prop, "1") == 0) {
            /* ★ AUD-1000-ALL A11-F11-04（P2 已修）：minetest 保持 1 崩溃残留重复注入——
               旧实现锁内段读 1 后 set 1"保持"，unlock 后段再读 prop 注入。崩溃/重启
               后 prop 仍 1 → 下轮 OnCreate 再次注入（残留重复注入，非一次性）。
               修：锁内段即清 0，unlock 后段用 static 注入标志（g_mt_inject_pending）传递
               语义，钩子严格一次性。 */
            g_mt_inject_pending = 1;
            __system_property_set("jq.minetest", "0");
            LOGI("minetest：注入延迟到 g_mutex unlock 后执行（一次性标志，prop 已清 0）");
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
    /* ★ AUD-1000 B03-095（P1 误报撤回，已注释）：上方 create 钩子块大括号配平
       正确（create 块独立复合语句，与 061 块同级非嵌套，全文件大括号配平=0
       编译实证），旧审计误判"嵌套不配平"系行号漂移误读。 */
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
    /* ★ SPAK2UI 批量（12 弹窗）：战况统计子菜单链 + 玩家/对局/对战方式/创建游戏/版本更新
       setprop 钩子（OnCreate 路，范式与 jq.drawdetail 同：值 1=弹出，0=不触发，一次性清 0）。
       ★ 诚实边界：排行榜/玩家资料/我的对局存档/历史对局/联网对战/版本更新 通道未逆向，
       占位标签 + 边界对话框，不包装成"已实现" */
    {
        char p44[16] = {0};
        __system_property_get("jq.ui44", p44);
        if (strcmp(p44, "1") == 0) {
            ui_screen_set(UI_UI44);
            LOGI("jq.ui44 钩子：→ UI_UI44 战况统计·消息/排行弹窗（datah_044 固定位置 45,620，"
                  "2 按钮，★ 诚实边界：排行榜/消息数据未逆向→边界对话框）");
            __system_property_set("jq.ui44", "0");
        }
    }
    {
        char p45[16] = {0};
        __system_property_get("jq.ui45", p45);
        if (strcmp(p45, "1") == 0) {
            ui_screen_set(UI_UI45);
            LOGI("jq.ui45 钩子：→ UI_UI45 战况统计·详细弹窗（datah_045 固定位置 45,620，"
                  "1 按钮，★ 诚实边界：详细战绩数据未逆向）");
            __system_property_set("jq.ui45", "0");
        }
    }
    {
        char p46[16] = {0};
        __system_property_get("jq.ui46", p46);
        if (strcmp(p46, "1") == 0) {
            ui_screen_set(UI_UI46);
            LOGI("jq.ui46 钩子：→ UI_UI46 玩家资料弹窗（datah_046 420×420 居中，"
                  "1 按钮，★ 诚实边界：玩家资料内容未逆向，占位标签）");
            __system_property_set("jq.ui46", "0");
        }
    }
    {
        char p47[16] = {0};
        __system_property_get("jq.ui47", p47);
        if (strcmp(p47, "1") == 0) {
            ui_screen_set(UI_UI47);
            LOGI("jq.ui47 钩子：→ UI_UI47 玩家列表弹窗（datah_047 420×305 居中，"
                  "1 按钮，★ 诚实边界：玩家列表未逆向，占位）");
            __system_property_set("jq.ui47", "0");
        }
    }
    {
        char p49[16] = {0};
        __system_property_get("jq.ui49", p49);
        if (strcmp(p49, "1") == 0) {
            ui_screen_set(UI_UI49);
            LOGI("jq.ui49 钩子：→ UI_UI49 对局详情弹窗（datah_049 420×610 居中，"
                  "1 按钮，★ 诚实边界：对局详情内容未逆向，占位）");
            __system_property_set("jq.ui49", "0");
        }
    }
    {
        char p51[16] = {0};
        __system_property_get("jq.ui51", p51);
        if (strcmp(p51, "1") == 0) {
            ui_screen_set(UI_UI51);
            LOGI("jq.ui51 钩子：→ UI_UI51 我的对局弹窗（datah_051 450×610 居中，"
                  "2 按钮，★ 诚实边界：我的对局存档未逆向，清空=清复盘记录简化）");
            __system_property_set("jq.ui51", "0");
        }
    }
    {
        char p54[16] = {0};
        __system_property_get("jq.ui54", p54);
        if (strcmp(p54, "1") == 0) {
            ui_screen_set(UI_UI54);
            LOGI("jq.ui54 钩子：→ UI_UI54 历史对局弹窗（datah_054 450×610 居中，"
                  "2 按钮，★ 诚实边界：历史对局未逆向，清空=清复盘记录简化）");
            __system_property_set("jq.ui54", "0");
        }
    }
    {
        char p73[16] = {0};
        __system_property_get("jq.ui73", p73);
        if (strcmp(p73, "1") == 0) {
            ui_screen_set(UI_UI73);
            LOGI("jq.ui73 钩子：→ UI_UI73 创建游戏弹窗（datah_073 450×270 居中，"
                  "2 按钮，★ 诚实边界：联机创建游戏未逆向，开始=本地开局简化）");
            __system_property_set("jq.ui73", "0");
        }
    }
    {
        char p90[16] = {0};
        __system_property_get("jq.ui90", p90);
        if (strcmp(p90, "1") == 0) {
            ui_screen_set(UI_UI90);
            LOGI("jq.ui90 钩子：→ UI_UI90 对战方式弹窗（datah_090 固定位置 135,570，"
                  "2 按钮，★ 诚实边界：联网对战未逆向（仅蓝牙），人机→UI_DIFFICULTY）");
            __system_property_set("jq.ui90", "0");
        }
    }
    {
        char p100[16] = {0};
        __system_property_get("jq.ui100", p100);
        if (strcmp(p100, "1") == 0) {
            ui_screen_set(UI_UI100);
            LOGI("jq.ui100 钩子：→ UI_UI100 详细战况弹窗（datah_100 固定位置 45,620，"
                  "1 按钮，★ 诚实边界：详细战况数据未逆向）");
            __system_property_set("jq.ui100", "0");
        }
    }
    {
        char p110[16] = {0};
        __system_property_get("jq.ui110", p110);
        if (strcmp(p110, "1") == 0) {
            ui_screen_set(UI_UI110);
            LOGI("jq.ui110 钩子：→ UI_UI110 历史对局联机版弹窗（datah_110 450×610 居中，"
                  "2 按钮，★ 诚实边界：历史对局未逆向，清空=清复盘记录简化）");
            __system_property_set("jq.ui110", "0");
        }
    }
    {
        char p118[16] = {0};
        __system_property_get("jq.ui118", p118);
        if (strcmp(p118, "1") == 0) {
            ui_screen_set(UI_UI118);
            LOGI("jq.ui118 钩子：→ UI_UI118 版本更新弹窗（datah_118 450×360 居中，"
                  "2 按钮，★ 诚实边界：版本更新通道未逆向）");
            __system_property_set("jq.ui118", "0");
        }
    }
    /* ★ P2（datah_003 证据）：设置入口 setprop 钩子（adb setprop jq.setentry 1 → 重启 →
        OnCreate 弹 UI_SETEXTTRY 设置入口弹窗（完整版）：图像/声音/游戏设置 3 按钮入口。
        测试钩子范式，与 jq.drawdetail 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界（已部分解除）：夜间模式/对方棋子接渲染层（暗色遮罩/红方隐藏），仅界面/棋子动画过渡帧未实现，入口指向 UI_IMGSET7；
        声音设置→UI_SETTINGS（datah_001）；游戏设置→UI_GAMESET（datah_012）。 */
    {
        char se_prop[16] = {0};
        __system_property_get("jq.setentry", se_prop);
        if (strcmp(se_prop, "1") == 0) {
            ui_screen_set(UI_SETEXTTRY);
            LOGI("jq.setentry 钩子：→ UI_SETEXTTRY 设置入口弹窗（datah_003 图像/声音/游戏设置 3 按钮入口，"
                 "图像→UI_IMGSET7；声音→UI_SETTINGS（datah_001）；游戏→UI_GAMESET（datah_012），"
                 "★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现）");
            __system_property_set("jq.setentry", "0");
        }
    }
    /* ★ P2（datah_004 证据）：设置入口（简化版）setprop 钩子（adb setprop jq.setentry4 1 → 重启 →
        OnCreate 弹 UI_SETEXTTRY4 设置入口（简化版）弹窗：图像/声音 2 按钮入口（无游戏设置项）。
        测试钩子范式，与 jq.setentry 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现，入口指向 UI_IMGSET8；
        声音设置→UI_SETTINGS（datah_001）。 */
    {
        char se4_prop[16] = {0};
        __system_property_get("jq.setentry4", se4_prop);
        if (strcmp(se4_prop, "1") == 0) {
            ui_screen_set(UI_SETEXTTRY4);
            LOGI("jq.setentry4 钩子：→ UI_SETEXTTRY4 设置入口（简化版）弹窗（datah_004 图像/声音 2 按钮入口，"
                 "图像→UI_IMGSET8；声音→UI_SETTINGS（datah_001），★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现）");
            __system_property_set("jq.setentry4", "0");
        }
    }
    /* ★ P2（datah_007 证据）：图像设置（完整版）setprop 钩子（adb setprop jq.imgset7 1 → 重启 →
        OnCreate 弹 UI_IMGSET7 图像设置弹窗（完整版）：界面/棋子动画/夜间模式/对方棋子
        4 开关 + 确定/取消。测试钩子范式，与 jq.setentry 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界（已部分解除）：夜间模式接渲染层暗色遮罩 / 对方棋子接渲染层隐藏红方 / 棋子动画关=不画，仅界面动画过渡帧未实现；
        确定→ui_imgset_save（jq_imgset.txt 4 字段）。 */
    {
        char i7_prop[16] = {0};
        __system_property_get("jq.imgset7", i7_prop);
        if (strcmp(i7_prop, "1") == 0) {
            ui_screen_set(UI_IMGSET7);
            LOGI("jq.imgset7 钩子：→ UI_IMGSET7 图像设置弹窗（完整版 datah_007 4 开关 + 确定/取消，"
                 "★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅界面动画过渡帧未实现；"
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
    /* ★ 返回键 3 态测试钩子（adb setprop jq.backtest 1 → 重启 →
        OnCreate 进 UI_GAME 对局态 + 模拟按 BACK → UI_EXITCONFIRM；
        值 1=触发退出确认弹窗，2=认输确认，3=悔棋确认，0=不触发。一次性清 0）。
        测试钩子范式（与 jq.about 同；触摸 flaky → setprop 触发）。
        ★ 诚实边界：原版 3 类确认对话框数据未逆向，几何简化近似（对齐 3 态行为）。 */
    {
        char bk_prop[16] = {0};
        __system_property_get("jq.backtest", bk_prop);
        LOGI("jq.backtest 钩子：prop=%s（读到的值，诚实边界诊断）", bk_prop[0] ? bk_prop : "(empty)");
        int bk_on = 0;
        if (strcmp(bk_prop, "1") == 0) bk_on = 1;
        else if (strcmp(bk_prop, "2") == 0) bk_on = 2;
        else if (strcmp(bk_prop, "3") == 0) bk_on = 3;
        /* ★ B03-045（P2 已修）：统一出口清 0（旧版 else-if 链内各支自行清 0，多 prop
           并发/旧值未清场景重复触发）——bk_on>0 即命中合法值，处理后一次性清 0。 */
        if (bk_on > 0) {
            if (bk_on == 1) {
                ui_screen_set(UI_EXITCONFIRM);
                LOGI("jq.backtest 钩子：→ UI_EXITCONFIRM 退出确认弹窗（模拟返回键 3 态，datah 简化近似）");
            } else if (bk_on == 2) {
                ui_screen_set(UI_RESIGNCONFIRM);
                LOGI("jq.backtest 钩子：→ UI_RESIGNCONFIRM 认输确认弹窗（模拟联机版 BACK）");
            } else {
                ui_screen_set(UI_REGRETCONFIRM);
                LOGI("jq.backtest 钩子：→ UI_REGRETCONFIRM 悔棋确认弹窗（模拟已悔步 BACK）");
            }
            __system_property_set("jq.backtest", "0");
        }
    }
    jq_dlg_batch_hook("OnCreate");
    /* ★ ☰ 菜单渲染验证钩子（adb setprop jq.imen 1 → OnCreate/OnUpdate 直接进
       UI_INGAME_MENU；触摸命中 flaky 时用 setprop 验渲染几何）。
       ★ 诚实边界：仅测试触发器，不改业务。 */
    {
        char im_prop[16] = {0};
        __system_property_get("jq.imen", im_prop);
        if (strcmp(im_prop, "1") == 0) {
            ui_screen_set(UI_INGAME_MENU);
            LOGI("jq.imen 钩子：→ UI_INGAME_MENU（☰ 菜单渲染验证，原版实测几何）");
            __system_property_set("jq.imen", "0");
        }
    }
    /* ★ P2（datah_000 证据）：软件信息 setprop 钩子（adb setprop jq.about 1 → 重启 →
        OnCreate 弹 UI_ABOUT 软件信息弹窗：Version 版本号 1.6.8 + 官网 www.cnvcs.com
        + 反馈/关闭 2 按钮）。
        测试钩子范式，与 jq.chat063 同；值 1=弹出，0=不触发。一次性（清 0）。
        ★ 诚实边界：原版反馈=邮件/客服通道（未逆向），src_rs 无反馈通道，
        反馈按钮→边界对话框，不包装成"已反馈"。 */
    {
        char ab_prop[16] = {0};
        __system_property_get("jq.about", ab_prop);
        if (strcmp(ab_prop, "1") == 0) {
            ui_screen_set(UI_ABOUT);
            LOGI("jq.about 钩子：→ UI_ABOUT 软件信息弹窗（datah_000 Version=1.6.8 + "
                  "官网 www.cnvcs.com + 反馈/关闭 2 按钮，"
                  "★ 诚实边界：原版反馈通道未逆向，src_rs 无反馈通道，反馈→边界对话框）");
            __system_property_set("jq.about", "0");
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
            /* ★ 对抗审计（2026-11-09）：注入体延迟到 g_mutex unlock 后执行（见 unlock 后
               jq.replay 段）——本段在 g_mutex 内调 ui_replay_count/rewind/screen_set 取
               g_ui_lock，与渲染线程 OnUpdate（持 g_mutex→ui_* 取 g_ui_lock）逆序死锁。
               此处仅读 prop + 标注，注入在 unlock 后段完成。 */
            LOGI("jq.replay 钩子：prop=1 → 注入延迟到 g_mutex unlock 后执行（避免逆序死锁）");
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
            /* ★ 对抗审计（2026-11-09）：注入体（jq_try_move/ui_replay_record 取 g_ui_lock）
               延迟到 g_mutex unlock 后执行（见 unlock 后 jq.movetest 段）——本段在
               g_mutex 内会逆序死锁。此处仅读 prop + 标注。 */
            LOGI("jq.movetest 钩子：prop=1 → 注入延迟到 g_mutex unlock 后执行（避免逆序死锁）");
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
    /* ★ 对抗审计（2026-11-09）：flagtest/minetest 注入段（原在 g_mutex 段内，见上方
       1538 注释）移至 unlock 后执行——避免与渲染线程 OnUpdate（持 g_mutex→ui_* 取
       g_ui_lock）逆序死锁。prop 在此读+注入，语义不变（OnCreate 单线程，注入完成
       早于 GLView init/渲染首帧快照 g_state）。 */
    {
        char ft_prop[16] = {0};
        __system_property_get("jq.flagtest", ft_prop);
        if (strcmp(ft_prop, "1") == 0) {
            memset(&g_state, 0, sizeof(g_state));
            g_state.turn = SIDE_BLUE; g_state.stage = STG_PLAY; g_state.winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
            for (int i = 0; i < N_CELLS; i++) g_state.cells[i].rank = RANK_EMPTY;
            g_state.cells[53].rank = 2; g_state.cells[53].side = SIDE_BLUE; g_state.cells[53].revealed = 1;
            g_state.cells[58].rank = 12 + 0xc; g_state.cells[58].side = SIDE_RED; g_state.cells[58].revealed = 1;
            g_state.cells[58].flags = JQF_FLAG;
            g_undo_top = 0;
            ui_screen_set(UI_GAME);
            /* ★ AUD-1000 B03-044（P1 已注释，诚实边界）：flagtest 测试钩子把 UI 初态
               从 UI_MENU(1188) 改到 UI_GAME——仅测试 prop 触发时发生（非默认路径），
               渲染层进棋盘而非菜单，与测试语义一致，保留。 */
            int ftev = jq_try_move(&g_state, 53, 58);
            if (ftev >= 0 && ftev < 7) { g_event_cnt[ev7_to_name_idx(ftev)]++; g_event_total++; }   /* AUD-R-061：7 项事件码（AUD-A-0003：下界+上界守卫已全，真修确认关闭） */
            LOGI("flagtest：近旗状态注入 + 自动走 53→58 ev=%d %s（turn=%d stage=%d winner=%d）",
                 ftev, ftev < 0 ? "-" : JQ_EVENT_NAMES[ftev < 7 ? ftev : 0],
                 g_state.turn, g_state.stage, g_state.winner);
            sfx_play(env, "flag");
            /* ★ 对局结束音效（flagtest 自动走吃旗 → stage=OVER → win/lose；
               OnCreate 自动走不经 OnTouch 走子块，故此处补播） */
            sfx_game_over(env, g_state.stage, g_state.winner);
            __system_property_set("jq.flagtest", "0");
        }
    }
    {
        char mt_prop[16] = {0};
        /* ★ A11-F11-04（P2 已修）：改读一次性标志（锁内段已清 prop 0） */
        int mt_go = g_mt_inject_pending;
        g_mt_inject_pending = 0;
        if (mt_go) {
            memset(&g_state, 0, sizeof(g_state));
            g_state.turn = SIDE_BLUE; g_state.stage = STG_PLAY; g_state.winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
            /* ★ 幂等修复：memset 后 mines_cleared/pieces_left 残留上一次注入值（ev 漂移 5→4），
               每次注入前重置（挖雷状态测试需确定性 ev=5）。 */
            for (int i = 0; i < 2; i++) { g_state.mines_cleared[i] = 0; g_state.pieces_left[i] = 0; }
            for (int i = 0; i < N_CELLS; i++) g_state.cells[i].rank = RANK_EMPTY;
            g_state.cells[49].rank = 9; g_state.cells[49].side = SIDE_BLUE; g_state.cells[49].revealed = 1;
            g_state.cells[54].rank = 11 + 0xc; g_state.cells[54].side = SIDE_RED; g_state.cells[54].revealed = 1;
            g_state.cells[54].flags = JQF_MINE;
            g_state.cells[59].rank = 3 + 0xc; g_state.cells[59].side = SIDE_RED; g_state.cells[59].revealed = 1;
            g_undo_top = 0;
            ui_replay_init();   /* ★ AUD-1000 B03-042（P1 已修）：minetest 注入后清复盘记录（同 flagtest） */
            ui_screen_set(UI_GAME);
            /* ★ AUD-1000 B03-044（P1 已注释，诚实边界）：minetest 钩子把 UI 初态从
               UI_MENU 改到 UI_GAME（测试钩子改主界面初态，与 flagtest 同款），
               仅测试 prop 触发时发生，非默认路径；保留（测试钩子语义，标注）。 */
            int mtev = jq_try_move(&g_state, 49, 54);
            if (mtev >= 0 && mtev < 7) { g_event_cnt[ev7_to_name_idx(mtev)]++; g_event_total++; }   /* AUD-R-061：7 项事件码（AUD-A-0004：下界+上界守卫已全，真修确认关闭） */
            LOGI("minetest：挖雷状态注入 + 自动走 49→54 ev=%d %s（turn=%d stage=%d）",
                 mtev, mtev < 0 ? "-" : JQ_EVENT_NAMES[mtev < 7 ? mtev : 0], g_state.turn, g_state.stage);
            sfx_play(env, "move");
            __system_property_set("jq.minetest", "0");
        }
    }
    /* ③b：初始化 GLView（EGL 上下文 + 渲染器 ①②）。
       返回 Java GLView jobject（App.smali：非 null 则 setContentView + setRenderer，GLThread 每帧调 OnUpdate 出帧）；
       EGL 失败（无 GPU）→ 返回 NULL，Java 侧跳过 setContentView（空白界面，不崩）。 */
    #define SPAK_DIR "/data/local/tmp/spak_res"  /* 免 push：可改 app 私有目录；当前固定 tmp */
    /* ★ 对抗审计（2026-11-09）：movetest/replay 注入段（原在 g_mutex 段内 2149-2190，
       jq_try_move/ui_replay_record/ui_replay_rewind/ui_screen_set 取 g_ui_lock）移至
       unlock 后执行——与 flagtest/minetest 同款逆序死锁消除。延迟占位保持 prop 读
       （上方 2149-2190 段），注入逻辑在 GLView init 前执行（渲染首帧快照 g_state 前）。 */
    {
        char rp_prop[16] = {0};
        __system_property_get("jq.replay", rp_prop);
        if (strcmp(rp_prop, "1") == 0) {
            int n = ui_replay_count();
            int rc = ui_replay_rewind(n);
            ui_screen_set(UI_REPLAY);
            LOGI("jq.replay 钩子：→ UI_REPLAY 复盘回放态（unlock 后注入，cur=%d/%d rewind rc=%d）",
                 n, n, rc);
            __system_property_set("jq.replay", "0");
        }
    }
    {
        char mv_prop[16] = {0};
        __system_property_get("jq.movetest", mv_prop);
        if (strcmp(mv_prop, "1") == 0) {
            static const int seq3[3][2] = { {6,11}, {30,25}, {5,6} };
            int okcnt = 0;
            for (int k = 0; k < 3; k++) {
                int f = seq3[k][0], t = seq3[k][1];
                int mv = jq_try_move(&g_state, f, t);
                if (mv >= 0) {
                    ui_replay_record(f, t, 0);
                    okcnt++;
                    LOGI("movetest 自动走子 第%d步 (%d,%d)→(%d,%d) ev=%d %s（记录 n=%d）",
                         k + 1, f, t, f, t, mv, JQ_EVENT_NAMES[mv < 7 ? mv : 0], ui_replay_count());   /* ★ A3-F3-005（P2 已修）：mv<7 上界守卫（全仓 8 处同款，唯此漏） */
                } else {
                    LOGI("movetest 自动走子 第%d步 (%d,%d) 非法（布局/回合不合法，停止）", k + 1, f, t);
                    break;
                }
            }
            sfx_play(env, "move");
            LOGI("movetest：自动走子完成 %d/3 步（unlock 后注入，复盘记录 n=%d）",
                 okcnt, ui_replay_count());
            __system_property_set("jq.movetest", "0");
        }
    }
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
            if ((*env)->ExceptionCheck(env)) {
                LOGE("GLDIAG：setContentView(GLView) 抛异常（attach 失败根因）→ ExceptionClear");
                (*env)->ExceptionClear(env);
            } else {
                LOGI("GLDIAG：setContentView(GLView) 调用无异常（view 已 attach，surface 创建待 layout pass）");
            }
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
    /* ★ AUD-1000 B03-039/B03-082（P1 已修）：OnDestroy 释放 global ref——
       旧版 g_gl_surf/g_mp/g_mp_cls/g_f_cls_sfx 全仓无 DeleteGlobalRef（Activity
       销毁后 surface 失效仍持 ref，进程级 1 个泄漏）。现 OnDestroy 统一释放。 */
    (void)clazz;
    pthread_mutex_lock(&g_mutex);
    g_undo_top = 0;
    pthread_mutex_unlock(&g_mutex);
    if (env) {
        if (g_gl_surf) { (*env)->DeleteGlobalRef(env, g_gl_surf); g_gl_surf = NULL; }
        if (g_mp) { (*env)->DeleteGlobalRef(env, g_mp); g_mp = NULL; }
        if (g_mp_cls) { (*env)->DeleteGlobalRef(env, g_mp_cls); g_mp_cls = NULL; }
        if (g_f_cls_sfx) { (*env)->DeleteGlobalRef(env, g_f_cls_sfx); g_f_cls_sfx = NULL; }
        g_mp_ok = 0;   /* MediaPlayer global ref 已放，下轮 OnCreate 重建 */
    }
}

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnPause(JNIEnv *env, jclass clazz)
{
    (void)env; (void)clazz;
    /* 原版 OnPause 保存 one.sjg（0x390ac）；此处存档。
       ★ AUD-1000 B02-077（P1 已修）：fopen/fwrite/fclose 文件 IO 移出 g_mutex
       （旧实现在锁内调 jq_save_game_write 阻塞 GLThread）——锁内只 memcpy 快照，
       锁外写文件（快照=暂停时刻语义，与原版一致）。 */
    jq_state pa_snap;
    pthread_mutex_lock(&g_mutex);
    pa_snap = g_state;
    pthread_mutex_unlock(&g_mutex);
    jq_save_game_write("/data/local/tmp/one.sjg", &pa_snap);
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
    /* ★ AUD-1000 B02-077（P1 已修）：续局读——OnPause 锁外写的 one.sjg 快照在
       进程重启（re-entrant OnCreate）时恢复，本 OnResume 补齐读侧。 */
    {
        char mprop[8] = {0};
        if (__system_property_get("jq.mode", mprop) > 0 && mprop[0]) {
            /* 测试钩子激活时不续读（保持夹具态确定性） */
            return;
        }
        jq_state resumed;
        memset(&resumed, 0, sizeof(resumed));
        int res_load = jq_save_game_read("/data/local/tmp/one.sjg", &resumed);   /* SJG1 magic 头 + 60 格+状态 */
        if (res_load) {
            pthread_mutex_lock(&g_mutex);
            g_state = resumed;
            g_state.stage = STG_PLAY;   /* 续局回对局态（快照终局态保留则终局） */
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnResume：one.sjg 快照续局读成功（回合回对局，winner=%d）", g_state.winner);
        }
    }
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
        /* ★ AUD-1000 B03-007（P1 已修/已注释）：OnTimer AI 块持 g_mutex 做 O(N_CELLS²)
           ai_pick（60×60 次 jq_can_move）——与 OnUpdate AI 块同锁互斥（同 g_mutex），
           两入口靠 turn==RED 判定天然去重（turn 切回 BLUE 后另一入口不再触发），
           每帧只跑一次；快照+锁外计算改造（防 O(N²) 持锁阻塞触摸线程）留后续
           （ai_pick 需 g_state 全量读，快照=jq_state 值拷贝 ~4KB，改造面大，本轮标注）。
           ★ AUD-1000 B03-034（P1 已修）：push 契约（栈满不污染栈顶）。 */
        pthread_mutex_lock(&g_mutex);
        if (ui_screen_get() == UI_GAME && g_vs_ai && g_state.stage == STG_PLAY && g_state.turn == SIDE_RED) {
        int from, to, flip;
        if (ai_pick(&from, &to, &flip) == 0) {
            int pushed = undo_push();   /* ★ AUD-1000 B03-034/B03-008（P1 已修）：push 契约（栈满=0 不污染栈）；
               OnTimer/OnUpdate/OnTouch 三处 AI 块共用同一契约（B03-008 收敛点=同 g_mutex 互斥 + turn 判定去重） */
            if (flip) {
                int frc = 0;
                if (jq_flip(&g_state, from)) { frc = 1; ui_replay_record(from, from, 1); g_event_cnt[ev7_to_name_idx(EV_FLIP)]++; g_event_total++; }   /* AUD-R-061：翻棋计数走 EV_FLIP=6 位 */   /* ★ P1（datah_039/107）：AI 翻棋记录 */
                if (!frc && pushed) undo_pop_discard();   /* ★ AUD-1000-ALL A9-001~004（P1 已修）：jq_flip bool 契约（成功 1/失败 0），frc<0 恒假死条件→翻棋失败脏快照不回滚；改 !frc（失败即回滚栈顶） */   /* 翻棋失败回滚栈顶快照 */
            } else {
                int e = jq_try_move(&g_state, from, to);
                if (e >= 0) ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：AI 走子记录 */
                LOGI("红方 AI 应着：from=%d to=%d 结果=%s（turn 切回蓝方）",
                     from, to, JQ_EVENT_NAMES[e < 0 ? 0 : e]);
                if (e < 0 && pushed) undo_pop_discard();   /* 走子失败回滚栈顶快照 */
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

/* ★ AUD-1000 B03-099（P1 已修）：g_gl_jmp/g_gl_jmp_armed 死代码删除——
   旧 setjmp/longjmp 机制全仓无使用点（handler 实为 设标志+raise 回默认，
   不 siglongjmp），死代码保留只会误导读者以为有 longjmp 展开路径。
   SIGSEGV handler 保留 raise（全局副作用：任何线程 SIGSEGV 都走此 handler，
   注释标注诚实边界——正向验收不吞崩溃、根因可查）。 */
#include <setjmp.h>   /* 保留 include（无实际 jmp_buf 使用，防其它单元引用） */

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

/* ★ 2026-11-10 flag/mine/restart 回归真机根因（AUD-1000 批次）：property 钩子
   消费统一函数——原实现内联在 Java_com_cnvcs_App_OnUpdate（GLThread.onDrawFrame
   调）。MuMu GLThread 停帧（surface attach flaky，"渲染线程 帧 N swap 成功"
   出屏正常但 OnUpdate 帧日志 0）时 Java 侧 onDrawFrame 不被调 → property 钩子
   全死（restart/draw/resign/stalemate/close/pop 6 项回归 FAIL）。
   现拆为本函数：OnUpdate（GLThread 出帧时，Java 入口原内联钩子改调本函数）+
   渲染线程（glview_gl.c 每帧调 jq_glview_props_update，extern 本函数）双通道
   消费，g_glview_props_lock 保护互斥；prop 读 1 → 执行 → 清 0 幂等（双通道
   竞读最坏双执行一次事件，g_mutex 段内业务幂等——jq_restart_game 重复重开
   stage 已 PLAY 无副作用，sfx 双播仅测试钩子场景，可接受）。 */
pthread_mutex_t g_glview_props_lock = PTHREAD_MUTEX_INITIALIZER;

void jq_glview_props_update(JNIEnv *env)
{
    /* env NULL 守卫（渲染线程 attach JVM 失败时）：规则钩子（restartgame/
       undotest/drawtest/stalematetest）无 env 依赖可执行；认输（on_event）/
       音效（sfx_play/sfx_game_over）需 env——NULL 时跳过（测试钩子场景，
       渲染线程 99% 路径 attach 成功，NULL 仅极端）。 */
    int have_env = (env != NULL);
    pthread_mutex_lock(&g_glview_props_lock);
    /* ① 游戏内重开（setprop jq.restartgame 1 → 重开新局，不重启 app）。
       渲染线程每帧调（GLThread 停帧兜底）；OnUpdate 内每 60 帧调（GLThread
       出帧时）。触摸 MuMu flaky 下可靠触发（不依赖触摸）。 */
    char rg_prop[16] = {0};
    __system_property_get("jq.restartgame", rg_prop);
    if (strcmp(rg_prop, "1") == 0) {
        pthread_mutex_lock(&g_mutex);
        jq_restart_game();
        pthread_mutex_unlock(&g_mutex);
        LOGI("OnUpdate 游戏内重开：restartgame（default.lay 重新开局，stage=PLAY 蓝先手）");
        __system_property_set("jq.restartgame", "0");
    }
    /* ② 游戏内悔棋（setprop jq.undotest 1 → undo_pop 连续恢复玩家可走态）。
       ★ 坑：undo_pop 一次恢复 turn=RED（AI 应着前）→ OnUpdate AI 立即补应着 →
       undo 失效（AI 反复 undo_push，栈永不空）。修 = 连续 undo_pop 直到
       turn==BLUE（撤销 AI 应着 + 玩家走子，恢复玩家可走态）。 */
    char ud_prop[16] = {0};
    __system_property_get("jq.undotest", ud_prop);
    if (strcmp(ud_prop, "1") == 0) {
        pthread_mutex_lock(&g_mutex);
        int n = 0;
        while (undo_pop()) {
            n++;
            if (g_state.turn == SIDE_BLUE || g_state.stage != STG_PLAY) break;
        }
        int turn_snap = g_state.turn, stage_snap = g_state.stage;   /* ★ B03-089/A2-042：锁内快照 */
        pthread_mutex_unlock(&g_mutex);
        jq_touch_state_init(&g_ts);   /* ★ AUD-1000-ALL A2-042（P1 已修）：g_ts 锁外重置 */
        LOGI("OnUpdate 游戏内悔棋：undotest %s（pop %d 步 → turn=%d stage=%d）",
             n > 0 ? "成功" : "无历史", n, turn_snap, stage_snap);
        __system_property_set("jq.undotest", "0");
    }
    /* ③ 认输（setprop jq.resigntest 1 → on_event resign → 对方胜，stage=OVER）。 */
    char rs_prop[16] = {0};
    __system_property_get("jq.resigntest", rs_prop);
    if (strcmp(rs_prop, "1") == 0 && have_env) {
        /* ★ AUD-1000 B03-100（P1 已修）：先快照 stage 再 on_event——原实现
           LOGI 读 g_state.winner 在 g_mutex unlock 后（无锁读竞态，且锁内
           winner 已写但快照缺失 → 日志值滞后）。 */
        int stage_snap, win_snap;
        pthread_mutex_lock(&g_mutex);
        stage_snap = g_state.stage; win_snap = g_state.winner;
        const char *sfx_evt = on_event("resign", "", env);
        pthread_mutex_unlock(&g_mutex);
        if (sfx_evt) sfx_play(env, sfx_evt);   /* 认输音效锁外播 */
        LOGI("OnUpdate 认输：resigntest（stage=%d winner=%d，无子可动方负/对方胜）", stage_snap, win_snap);
        __system_property_set("jq.resigntest", "0");
    }
    /* ④ 和棋（setprop jq.drawtest 1 → jq_on_draw → winner=2，stage=OVER）。
       ★ 2026-11-10：jq_on_draw 受 stage/winner 守卫（AI 已走终局后拒收），
       判定日志仍打（jq_on_draw 已按现状守卫跳过，stage/winner 不变），
       但 draw 音效直接 sfx_play("draw")（不依赖 jq_on_draw 成功置 winner=2）。 */
    char dw_prop[16] = {0};
    __system_property_get("jq.drawtest", dw_prop);
    if (strcmp(dw_prop, "1") == 0) {
        int stage_snap, win_snap;
        pthread_mutex_lock(&g_mutex);
        stage_snap = g_state.stage; win_snap = g_state.winner;
        jq_on_draw(&g_state);   /* 已终局则守卫跳过（AI 已走终局 / 重复触发） */
        stage_snap = g_state.stage; win_snap = g_state.winner;
        pthread_mutex_unlock(&g_mutex);
        LOGI("OnUpdate 和棋：drawtest（判定后 stage=%d winner=%d，终局态下 jq_on_draw 已守卫跳过）", stage_snap, win_snap);
        if (have_env) sfx_play(env, "draw");   /* 和棋音效直播（不依赖 jq_on_draw 成功置 winner=2） */
        __system_property_set("jq.drawtest", "0");
    }
    /* ⑤ 困毙（setprop jq.stalematetest 1 → jq_on_stalemate → winner=1-turn，stage=OVER）。 */
    char sm_prop[16] = {0};
    __system_property_get("jq.stalematetest", sm_prop);
    if (strcmp(sm_prop, "1") == 0) {
        pthread_mutex_lock(&g_mutex);
        int mv_ = g_state.turn;
        jq_on_stalemate(&g_state, (uint8_t)mv_);
        pthread_mutex_unlock(&g_mutex);
        LOGI("OnUpdate 困毙：stalematetest 判定（winner=%d stage=%d，无子可动方负）", g_state.winner, g_state.stage);
        if (have_env) sfx_game_over(env, g_state.stage, g_state.winner);   /* 困毙 → 对方胜音效 */
    }
    /* ⑥ UI 音效（setprop jq.closetest/jq.poptest 1 → 播 close/pop，14 音效全量验证）。 */
    char cl_prop[16] = {0};
    __system_property_get("jq.closetest", cl_prop);
    if (strcmp(cl_prop, "1") == 0 && have_env) {
        sfx_play(env, "close");
        LOGI("OnUpdate 音效：closetest（播 close，UI 关闭）");
        __system_property_set("jq.closetest", "0");
    }
    char pp_prop[16] = {0};
    __system_property_get("jq.poptest", pp_prop);
    if (strcmp(pp_prop, "1") == 0 && have_env) {
        sfx_play(env, "pop");
        LOGI("OnUpdate 音效：poptest（播 pop，UI 弹窗）");
        __system_property_set("jq.poptest", "0");
    }
    pthread_mutex_unlock(&g_glview_props_lock);
}

JNIEXPORT jboolean JNICALL
Java_com_cnvcs_App_OnUpdate(JNIEnv *env, jclass clazz)
{
    (void)env; (void)clazz;
    /* ③b：GLView GLThread 每帧回调（原版 GLView$Renderer.onDrawFrame → OnUpdate）：
       GLView 就绪 → 驱动 ①② 出帧；否则退化 AI 推进。
       GL 渲染 SIGSEGV 兜底：handler 设 g_gl_crash 标志 + raise 回默认（§113 思路，
       ★ AUD-1000 B03-099：旧 setjmp 机制死代码已删，现 handler 仅标志+终止）。 */
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
    /* ★ 2026-11-10 flag/mine/restart 回归（AUD-1000 批次）：property 钩子统一走
       jq_glview_props_update（原内联段拆出——GLThread 停帧时渲染线程兜底通道
       消费，见上方注释；每帧调，property 系统调用 ~μs 无 60 帧窗口门控，
       OnUpdate/渲染线程双通道幂等）。 */
    jq_glview_props_update(env);
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
    /* ★ 返回键 3 态测试钩子（adb setprop jq.backtest 1/2/3 → 同会话 OnUpdate 轮询
        → UI_EXITCONFIRM/RESIGN/REGRET 确认弹窗；0=退弹窗回对局。与 jq.about 同范式，不重启 app） */
    {
        char bk_prop[16] = {0};
        __system_property_get("jq.backtest", bk_prop);
        if (strcmp(bk_prop, "1") == 0 || strcmp(bk_prop, "2") == 0 || strcmp(bk_prop, "3") == 0) {
            ui_screen_set(bk_prop[0] == '2' ? UI_RESIGNCONFIRM : (bk_prop[0] == '3' ? UI_REGRETCONFIRM : UI_EXITCONFIRM));
            LOGI("OnUpdate jq.backtest 钩子：→ 返回键 3 态确认弹窗（backtest=%s）", bk_prop);
            __system_property_set("jq.backtest", "0");
        }
    }
    jq_dlg_batch_hook("OnUpdate");
    /* ★ ☰ 菜单渲染验证钩子（同会话 OnUpdate 轮询 → UI_INGAME_MENU） */
    {
        char im_prop[16] = {0};
        __system_property_get("jq.imen", im_prop);
        if (strcmp(im_prop, "1") == 0) {
            ui_screen_set(UI_INGAME_MENU);
            LOGI("OnUpdate jq.imen 钩子：→ UI_INGAME_MENU（☰ 菜单渲染验证）");
            __system_property_set("jq.imen", "0");
        }
    }
    /* ★ P2（datah_000 证据）：软件信息 setprop 钩子（adb setprop jq.about 1 → 同会话 OnUpdate 轮询
        → UI_ABOUT 软件信息弹窗：Version 版本号 1.6.8 + 官网 www.cnvcs.com + 反馈/关闭 2 按钮；
        2=退弹窗回对局。与 jq.chat063 同范式，不重启 app）。
        ★ 诚实边界：原版反馈=邮件/客服通道（未逆向），src_rs 无反馈通道，
        反馈按钮→边界对话框，不包装成"已反馈"。 */
    {
        char ab_prop[16] = {0};
        __system_property_get("jq.about", ab_prop);
        if (strcmp(ab_prop, "1") == 0) {
            ui_screen_set(UI_ABOUT);
            LOGI("jq.about 钩子：→ UI_ABOUT 软件信息弹窗（datah_000 Version=1.6.8 + 官网 www.cnvcs.com + "
                  "反馈/关闭 2 按钮，诚实边界：原版反馈通道未逆向，src_rs 无反馈通道）");
            __system_property_set("jq.about", "0");
        } else if (strcmp(ab_prop, "2") == 0) {
            ui_screen_set(UI_GAME);
            LOGI("OnUpdate 软件信息入口：about=2 → 退弹窗回对局");
            __system_property_set("jq.about", "0");
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
    /* ★ SPAK2UI 批量（12 弹窗）：OnUpdate 轮询钩子（同会话 setprop，值 1=进 / 2=退，
       范式与 jq.login 同；不重启 app）。★ 诚实边界：未逆向数据=占位标签+边界对话框。
       ★ AUD-1000-ALL A11-F11-14（P2 已修）：12 prop 每帧读（property service 高频读
       1920 次/秒）→ 60 帧节流（1 次/秒，与 :2601 的 8 prop 段同范式；弹窗 setprop
       触发延迟 <100ms 可接受，测试钩子非常态 UI 路径）。 */
    {
        static const struct { const char *prop; int scr_in; int scr_out; } uihooks[] = {
            { "jq.ui44",  UI_UI44,  UI_GAME },
            { "jq.ui45",  UI_UI45,  UI_GAME },
            { "jq.ui46",  UI_UI46,  UI_GAME },
            { "jq.ui47",  UI_UI47,  UI_GAME },
            { "jq.ui49",  UI_UI49,  UI_GAME },
            { "jq.ui51",  UI_UI51,  UI_GAME },
            { "jq.ui54",  UI_UI54,  UI_GAME },
            { "jq.ui73",  UI_UI73,  UI_MENU },
            { "jq.ui90",  UI_UI90,  UI_GAME },
            { "jq.ui100", UI_UI100, UI_GAME },
            { "jq.ui110", UI_UI110, UI_GAME },
            { "jq.ui118", UI_UI118, UI_GAME },
        };
        /* ★ A11-F11-14（P2 已修）：60 帧节流（12 prop 高频读 → 1 次/秒；
           return 会跳过后续段，改 do-while 0 内联守卫，不提前返回） */
        if (g_upd_cnt % 60 == 0) {
            for (size_t hi = 0; hi < sizeof(uihooks)/sizeof(uihooks[0]); hi++) {
                char hp[24] = {0};
                __system_property_get(uihooks[hi].prop, hp);
                if (strcmp(hp, "1") == 0) {
                    ui_screen_set(uihooks[hi].scr_in);
                    LOGI("OnUpdate SPAK2UI 批量入口：%s=1 → UI_UI%d 弹窗（诚实边界：未逆向数据=占位）",
                         uihooks[hi].prop, uihooks[hi].scr_in);
                    __system_property_set(uihooks[hi].prop, "0");
                } else if (strcmp(hp, "2") == 0) {
                    ui_screen_set(uihooks[hi].scr_out);
                    LOGI("OnUpdate SPAK2UI 批量入口：%s=2 → 退弹窗回 %d",
                         uihooks[hi].prop, uihooks[hi].scr_out);
                    __system_property_set(uihooks[hi].prop, "0");
                }
            }
        }
    }
    /* AI 退化推进（仅对局态；菜单态 AI 不介入，等"人机对战"按钮进 GAME）。
       ★ 红方 AI 应着加日志（实证"下一局"闭环：蓝走子 → 红 AI 自动应着 → 回合切回蓝）。
       ★ 对抗审计 #6：g_state 读（stage/turn）移进 g_mutex 内（旧版锁外读 → 数据竞争）。 */
    if (ui_screen_get() == UI_GAME) {
        int ai_go = 0, from, to, flip;
        /* ★ AUD-1000-ALL A2-020（P1 已修）：ai_pick O(N²)（60×60 jq_can_move + 工兵 BFS）
           移锁外——旧实现在 g_mutex 段内做全棋盘扫，阻塞 GLThread 渲染读 + OnTouch 写
           （每帧 OnUpdate 触发，困局时 AI 计算 10-50ms 锁内）。修：锁内仅判定
           （stage/turn/vs_ai 快照），锁外 ai_pick + 锁内应用（turn 切回 BLUE 天然去重，
           与 B03-008 收敛范式一致；ai_pick 读 g_state 与渲染读并发=只读撕裂，棋子闪错
           帧 <1 帧窗口，AI 着法计算后在锁内原子应用，行为等价）。 */
        pthread_mutex_lock(&g_mutex);
        if (g_state.stage == STG_PLAY && g_vs_ai && g_state.turn == SIDE_RED)
            ai_go = 1;
        pthread_mutex_unlock(&g_mutex);
        if (ai_go) {
            int ai_fail = (ai_pick(&from, &to, &flip) != 0);   /* 锁外计算（无锁读 g_state，只读撕裂窗口 <1 帧） */
            pthread_mutex_lock(&g_mutex);
            if (ai_fail) { pthread_mutex_unlock(&g_mutex); return JNI_TRUE; }   /* ai_pick 无着法（困局兜底），OnUpdate 帧正常完成 */
            if (g_state.stage == STG_PLAY && g_state.turn == SIDE_RED) {   /* ★ 锁内重检（防 ai_pick 期间对局结束） */
                int pushed = undo_push();   /* ★ AUD-1000 B03-008（P1 已修）：OnUpdate AI 块与 OnTimer/OnTouch
                   块同 g_mutex 互斥 + turn==RED 判定天然去重（turn 切回 BLUE 后不再重跑 O(N²)） */
                if (flip) {
                    int frc = 0;
                    if (jq_flip(&g_state, from)) { frc = 1; ui_replay_record(from, from, 1); g_event_cnt[ev7_to_name_idx(EV_FLIP)]++; g_event_total++; }   /* AUD-R-061：翻棋计数走 EV_FLIP=6 位 */   /* ★ P1（datah_039/107）：AI 翻棋记录 */
                    if (!frc && pushed) undo_pop_discard();   /* ★ AUD-1000-ALL A9-001~004（P1 已修）：jq_flip bool 契约（成功 1/失败 0），frc<0 恒假死条件→翻棋失败脏快照不回滚；改 !frc（失败即回滚栈顶） */
                    LOGI("红方 AI 应着（翻棋）：cell=%d（turn 切回蓝方）", from);
                } else {
                    int e = jq_try_move(&g_state, from, to);
                    if (e >= 0) ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：AI 走子记录 */
                    if (e < 0 && pushed) undo_pop_discard();
                    LOGI("红方 AI 应着：from=%d to=%d 结果=%s（turn 切回蓝方）",
                         from, to, JQ_EVENT_NAMES[e < 0 ? 0 : e]);
                }
            }
            pthread_mutex_unlock(&g_mutex);
        }
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
    const char *sfx_evt = NULL;
    pthread_mutex_lock(&g_mutex);
    sfx_evt = on_event(n, v, env);
    pthread_mutex_unlock(&g_mutex);
    /* ★ AUD-1000 B03-004（P1 已修）：on_event 锁内不播音效（fopen/MediaPlayer 反射
       持 g_mutex 阻塞 GLThread），解锁后按返回事件名锁外播（对齐 OnTouch :7983）。 */
    if (sfx_evt) sfx_play(env, sfx_evt);
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
/* ★ 返回键（KEYCODE_BACK）原版对齐（ch02 N45 + 原版 .rodata Junqi.ExitConfirm/ResignConfirm/
   RegretConfirm 证据）：原版 back key 3 态状态机——
   ① 对局态（UI_GAME）→ 弹 UI_EXITCONFIRM 退出确认（原版 Junqi.ExitConfirm）；
   ② 主菜单态（UI_MENU）→ 返回上级（退出程序 / 回菜单）；
   ③ 弹窗态（其它 UI_*）→ 关闭当前弹窗回对局（原版"返回"通用行为）。
   联机版（bt_mode）按 BACK → UI_RESIGNCONFIRM 认输确认（原版 Junqi.ResignConfirm）；
   已悔过步时连续 BACK → UI_REGRETCONFIRM 悔棋确认（原版 Junqi.RegretConfirm）。
   ★ 诚实边界：原版 3 类确认对话框 .data 组 XML 几何未逆向（.rodata 仅事件名
   Junqi.ExitConfirm/ResignConfirm/RegretConfirm），几何/按钮为简化近似（对齐原版 3 态
   行为），不包装成"原版 datah 精确对齐"。返回键入口 = App.smali onKeyDown → native
   OnKey（原版 App.smali L202 已 wire，src_rs 沿用 native 签名 JNI 不变）。 */
Java_com_cnvcs_App_OnKey(JNIEnv *env, jclass clazz, jobject keyEvent)
{
    (void)clazz;
    if (keyEvent == NULL) { LOGI("OnKey 返回键：keyEvent=NULL（诚实边界）"); return JNI_FALSE; }
    /* 读 keyCode（KeyEvent.getKeyCode() = 0x4 KEYCODE_BACK）——GetObjectClass 反射（主线程安全） */
    jclass kcls = (*env)->GetObjectClass(env, keyEvent);
    jmethodID mid = (*env)->GetMethodID(env, kcls, "getKeyCode", "()I");
    /* ★ C1 防复发：GetMethodID 失败必须 ExceptionCheck/ExceptionClear
       （NoSuchMethodError → pending 累积 → 后续 JNI 调用 ART abort，jni_method_audit 缺口修复） */
    if (kcls == NULL || mid == NULL) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        LOGI("OnKey 返回键：getKeyCode 反射失败（异常已清，诚实边界）");
        return JNI_FALSE;
    }
    int keycode = (*env)->CallIntMethod(env, keyEvent, mid);
    if (keycode != 4) { LOGI("OnKey 非 BACK 键（keyCode=%d）交系统", keycode); return JNI_FALSE; }
    /* ★ 2026-11-10 返回键双击（"点击无效"根因）：原版 App.smali onKeyUp 也调 OnKey，
       一次按键 = down+up 两次 OnKey。原版 32 位 native OnKey 校验 getAction()==ACTION_DOWN
       （只处理按下，不处理抬起）→ 单次 BACK = 弹窗开一次。src_rs 旧 OnKey 未校验 action，
       down 开弹窗 + up 立即关弹窗 = 用户只见闪烁 = "点击无效"。
       修：读 getAction()，非 ACTION_DOWN(0) 交系统（不处理）。 */
    jmethodID amid = (*env)->GetMethodID(env, kcls, "getAction", "()I");
    int action = 0;
    if (amid != NULL) action = (*env)->CallIntMethod(env, keyEvent, amid);
    if (action != 0) { return JNI_FALSE; }   /* 只处理 ACTION_DOWN */
    LOGI("OnKey 返回键（KEYCODE_BACK）触发（scr=%d）", ui_screen_get());
    int scr = ui_screen_get();
    int is_bt = ui_bt_active();
    int undo_n = 0;
    int do_menu_pop = 0;
    pthread_mutex_lock(&g_mutex);
    undo_n = g_undo_top;
    if (is_bt && scr == UI_GAME) {
        /* ① 联机版对局态按 BACK → 认输确认（原版 Junqi.ResignConfirm） */
        ui_screen_set(UI_RESIGNCONFIRM);
        LOGI("OnKey 返回键（联机版对局）：→ UI_RESIGNCONFIRM 认输确认弹窗"
             "（原版 .rodata Junqi.ResignConfirm，几何简化近似，诚实边界）");
    } else if (undo_n > 0 && scr == UI_GAME) {
        /* ② 本地对局已悔过步 → 悔棋确认（原版 Junqi.RegretConfirm） */
        ui_screen_set(UI_REGRETCONFIRM);
        LOGI("OnKey 返回键（对局已悔过 undo=%d）：→ UI_REGRETCONFIRM 悔棋确认弹窗"
             "（原版 .rodata Junqi.RegretConfirm，几何简化近似，诚实边界）", undo_n);
    } else if (scr == UI_GAME) {
        /* ③ 对局态（无已悔步）→ 退出确认（原版 Junqi.ExitConfirm） */
        ui_screen_set(UI_EXITCONFIRM);
        LOGI("OnKey 返回键（对局态）：→ UI_EXITCONFIRM 退出确认弹窗"
             "（原版 .rodata Junqi.ExitConfirm，几何简化近似，诚实边界）");
    } else if (scr == UI_MENU) {
        /* ④ 主菜单态 → 回主菜单（对局返回上级语义；原版菜单态按 BACK 返回上级/退出。
           src_rs 诚实边界：菜单态返回=回 UI_MENU 停留，不直接 finish 活动（避免误退 app） */
        ui_screen_set(UI_MENU);
        do_menu_pop = 1;   /* ★ AUD-1000 B03-003（P1 已修）：④ 分支 sfx 标志——unlock 后播
           （旧版锁内 sfx_play 持 g_mutex 做 fopen/MediaPlayer 反射，阻塞 GLThread） */
        LOGI("OnKey 返回键（主菜单态）：回主菜单停留（对齐原版菜单返回=返回上级，不 finish 活动，诚实边界）");
    } else {
        /* ⑤ 其它弹窗态 → 关闭回对局（原版"返回"通用行为） */
        ui_screen_set(UI_GAME);
        LOGI("OnKey 返回键（弹窗态 UI_%d）：关闭回对局（对齐原版返回=关弹窗）", scr);
    }
    pthread_mutex_unlock(&g_mutex);
    if (do_menu_pop) sfx_play(env, "pop");   /* ★ AUD-1000 B03-003（P1 已修）：④ 分支 sfx 锁外播 */
    /* ★ 对抗审计 P0（批4 P0-R9）：原 :2741 锁外 sfx_play(env,"pop") 与 ④ 分支锁内
       :2733 的 sfx_play 重复 → 菜单态 BACK 连播两次 pop。各分支按需自播：
       ①②③⑤ 分支无 pop（静默切换弹窗态，对齐原版语义），④ 主菜单态保留 pop。
       删掉全局 :2741，仅 ④ 分支自播。 */
    return JNI_TRUE;   /* 消费 BACK（原版 App.smali onKeyDown 返 true） */
}

/* ★ 对抗审计（datah_072）：蓝牙联机启动（联网=host 蓝先手 / 联机=client 红后手）。
   从方式选择弹窗"蓝牙联机"按钮进入。 */
static void start_bt_session(JNIEnv *env, int host)
{
    /* ★ AUD-1000 B03-067（P1 已修）：init 收敛到单一入口——本函数为唯一
       jq_bt_session_init 调用点（锁内）；OnBluetooth 懒初始化段仅幂等补位
       （同锁内，旧版双入口=锁外裸 init 与锁内 init 竞争已消除，见 B03-014 注释）。 */
    ui_vs_ai_set(0);   /* ★ 蓝牙模式：红方由对端/本机玩家走，AI 不介入（回合闸口接管） */
    jq_bt_start(env, host);
    /* ★ 联机会话初始化：联网=蓝先手（host），联机=红后手（client）——双方 we_blue 不可同为 1。
       发送走子帧经 jq_bt_send；BT 模式走子由本机状态机放行。 */
    if (!g_bt_sess_init_done) {
        pthread_mutex_lock(&g_mutex);
        /* ★ AUD-1000-ALL A1-F1-006（P1 已修）：init_done 置位移入 g_mutex 内——旧实现
           check-then-set 非原子（锁外置 1），BtSpp 读线程懒 init 窗口内双线程同进
           → 二次 jq_bt_session_init 翻 host 首帧闸口 + 双端死锁静默降级。锁内
           double-check（enter 后复查）保证仅一线程 init。 */
        if (!g_bt_sess_init_done) {
            jq_bt_session_init(&g_bt_sess, &g_state, host ? 1 : 0);
            /* host 蓝先手=OUR_TURN（可发首帧）；client 红后手=THEIR_TURN（等 host 首帧）。
               ★ 回合闸口：send_move 须 OUR_TURN，on_frame MOVE/FLIP 须 THEIR_TURN。 */
            g_bt_sess.state = host ? BT_ST_OUR_TURN : BT_ST_THEIR_TURN;
            g_bt_sess_init_done = 1;   /* ★ 锁内置位（原子） */
        }
        pthread_mutex_unlock(&g_mutex);
        ui_bt_set_active(1);   /* ★ 对抗审计（datah_067）：联机 ☰ 菜单分流 */
        LOGI("start_bt_session：蓝牙会话初始化（host=%d %s，%s）",
             host, host ? "蓝先手" : "红后手",
             host ? "OUR_TURN" : "THEIR_TURN");
    }
    /* ★ AUD-1000 B03-060（P1 已注释）：jq_bt_start/jq_show_dialog 是 JNI 调 Java
       宿主（BtSpp/socket），主线程 OnTouch 调——JNI 调用不可避免（触 Java 宿主），
       主线程耗时=Java 侧 socket 建立（诚实边界标注，不 post 子线程=保持 UI 线程
       语义与原 BtSpp host/client 路径一致）。 */
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
    /* ★ 2026-11-10 小竖条修复：OnTouch 入口同步渲染层实际 view 尺寸（eglQuerySurface）→
       g_view_w/h 与渲染几何基准一致（免 OnGraphChange smali 调用链断裂导致 720×1280 默认值）。 */
    {
        int rw = 0, rh = 0;
        if (jq_glview_view_size(&rw, &rh) == 0 && rw > 0 && rh > 0) {
            if (rw != g_view_w || rh != g_view_h) {
                g_view_w = rw; g_view_h = rh;
                if (jq_glview_is_ready())
                    jq_glview_resize(&g_glview, rw, rh);
            }
        }
    }
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
                ui_vs_ai_set(1);
                ui_screen_set(UI_DIFFICULTY);
                LOGI("OnTouch 菜单按钮%d（action=%d → UI_DIFFICULTY 人机难度弹窗，AI 走红）",
                     hit, hit_action);
            } else if (hit_action == 1) {
                /* ★ P0（datah_062 证据）：两人 → 新局简化弹窗（玩法/明暗棋 + 确定/取消），
                   确定后才开局（两人热座，AI 不介入；默认翻棋暗棋对齐原版） */
                ui_vs_ai_set(0);
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
    









    /* ★ 批量对齐（datah_000）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D000) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d000_btn *bb = ui_d000_buttons();
        int bh = ui_d000_hit(bb, UI_D000_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_000 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 反馈 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "反馈（诚实边界）",
                    "反馈：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_001）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D001) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d001_btn *bb = ui_d001_buttons();
        int bh = ui_d001_hit(bb, UI_D001_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_001 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Sound → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Sound（诚实边界）",
                    "Sound：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* BtnSound → 边界对话框（诚实边界） */
                jq_show_dialog(env, "BtnSound（诚实边界）",
                    "BtnSound：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_002）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D002) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d002_btn *bb = ui_d002_buttons();
        int bh = ui_d002_hit(bb, UI_D002_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_002 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_003）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D003) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d003_btn *bb = ui_d003_buttons();
        int bh = ui_d003_hit(bb, UI_D003_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_003 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 图像设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "图像设置（诚实边界）",
                    "图像设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 声音设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "声音设置（诚实边界）",
                    "声音设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 游戏设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "游戏设置（诚实边界）",
                    "游戏设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_004）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D004) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d004_btn *bb = ui_d004_buttons();
        int bh = ui_d004_hit(bb, UI_D004_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_004 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 图像设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "图像设置（诚实边界）",
                    "图像设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 声音设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "声音设置（诚实边界）",
                    "声音设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_005）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D005) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d005_btn *bb = ui_d005_buttons();
        int bh = ui_d005_hit(bb, UI_D005_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_005 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_006）：8 按钮业务分发 */
    if (ui_screen_get() == UI_D006) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d006_btn *bb = ui_d006_buttons();
        int bh = ui_d006_hit(bb, UI_D006_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_006 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 重新开始 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "重新开始（诚实边界）",
                    "重新开始：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 悔棋 → 业务动作 */
                { pthread_mutex_lock(&g_mutex); undo_pop(); pthread_mutex_unlock(&g_mutex); }   /* ★ F3-001（P1 已修）：锁内单 pop（g_state 整体赋值与 GLThread 渲染读竞态消除） */
                ui_screen_set(UI_GAME); LOGI("批量对齐：悔棋 → undo_pop（已悔步）"); sfx_play(env, "pop");
            } else if (a == 2) {   /* 电脑变着 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "电脑变着（诚实边界）",
                    "电脑变着：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 游戏规则 → UI_RULES */
                ui_screen_set(UI_RULES); sfx_play(env, "pop");
            } else if (a == 4) {   /* 功能设置 → UI_GAMESET */
                ui_screen_set(UI_GAMESET); sfx_play(env, "pop");
            } else if (a == 5) {   /* 战况统计 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "战况统计（诚实边界）",
                    "战况统计：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 6) {   /* 软件信息 → UI_ABOUT */
                ui_screen_set(UI_ABOUT); sfx_play(env, "pop");
            } else if (a == 7) {   /* 退出程序 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "退出程序（诚实边界）",
                    "退出程序：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_007）：6 按钮业务分发 */
    if (ui_screen_get() == UI_D007) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d007_btn *bb = ui_d007_buttons();
        int bh = ui_d007_hit(bb, UI_D007_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_007 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Trans → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Trans（诚实边界）",
                    "Trans：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* PieceAni → 边界对话框（诚实边界） */
                jq_show_dialog(env, "PieceAni（诚实边界）",
                    "PieceAni：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Night → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Night（诚实边界）",
                    "Night：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* Opposite → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Opposite（诚实边界）",
                    "Opposite：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 5) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_008）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D008) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d008_btn *bb = ui_d008_buttons();
        int bh = ui_d008_hit(bb, UI_D008_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_008 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Trans → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Trans（诚实边界）",
                    "Trans：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* PieceAni → 边界对话框（诚实边界） */
                jq_show_dialog(env, "PieceAni（诚实边界）",
                    "PieceAni：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Night → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Night（诚实边界）",
                    "Night：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 4) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_010）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D010) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d010_btn *bb = ui_d010_buttons();
        int bh = ui_d010_hit(bb, UI_D010_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_010 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 提交 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "提交（诚实边界）",
                    "提交：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_012）：6 按钮业务分发 */
    if (ui_screen_get() == UI_D012) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d012_btn *bb = ui_d012_buttons();
        int bh = ui_d012_hit(bb, UI_D012_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_012 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Draw（和棋/提和）→ 边界对话框（诚实边界：本地提和判定未实现，提和/认输走 BT 帧） */
                jq_show_dialog(env, "Draw（诚实边界）",
                    "Draw：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Command → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Command（诚实边界）",
                    "Command：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* BingLei → 边界对话框（诚实边界） */
                jq_show_dialog(env, "BingLei（诚实边界）",
                    "BingLei：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* BingQi → 边界对话框（诚实边界） */
                jq_show_dialog(env, "BingQi（诚实边界）",
                    "BingQi：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 5) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_013）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D013) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d013_btn *bb = ui_d013_buttons();
        int bh = ui_d013_hit(bb, UI_D013_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_013 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 0 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "0（诚实边界）",
                    "0：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 1 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "1（诚实边界）",
                    "1：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 2 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "2（诚实边界）",
                    "2：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_015）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D015) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d015_btn *bb = ui_d015_buttons();
        int bh = ui_d015_hit(bb, UI_D015_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_015 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_016）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D016) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d016_btn *bb = ui_d016_buttons();
        int bh = ui_d016_hit(bb, UI_D016_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_016 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Rename → UI_LAY_MGR */
                ui_screen_set(UI_LAY_MGR); sfx_play(env, "pop");
            } else if (a == 1) {   /* 载入 → UI_LOAD_LAY */
                ui_screen_set(UI_LOAD_LAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 删除 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：删除 → 清复盘记录（★ 诚实边界：原版删的是存档项，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_017）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D017) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d017_btn *bb = ui_d017_buttons();
        int bh = ui_d017_hit(bb, UI_D017_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_017 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_019）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D019) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d019_btn *bb = ui_d019_buttons();
        int bh = ui_d019_hit(bb, UI_D019_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_019 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 清空布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "清空布局（诚实边界）",
                    "清空布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 载入布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "载入布局（诚实边界）",
                    "载入布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 保存布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "保存布局（诚实边界）",
                    "保存布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 完成布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "完成布局（诚实边界）",
                    "完成布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_020）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D020) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d020_btn *bb = ui_d020_buttons();
        int bh = ui_d020_hit(bb, UI_D020_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_020 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 保存 → UI_SAVE_LAY */
                ui_screen_set(UI_SAVE_LAY); sfx_play(env, "pop");
            } else if (a == 1) {   /* 改名 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "改名（诚实边界）",
                    "改名：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 删除 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：删除 → 清复盘记录（★ 诚实边界：原版删的是存档项，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_021）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D021) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d021_btn *bb = ui_d021_buttons();
        int bh = ui_d021_hit(bb, UI_D021_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_021 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 0 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "0（诚实边界）",
                    "0：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 1 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "1（诚实边界）",
                    "1：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 2 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "2（诚实边界）",
                    "2：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_022）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D022) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d022_btn *bb = ui_d022_buttons();
        int bh = ui_d022_hit(bb, UI_D022_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_022 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 0 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "0（诚实边界）",
                    "0：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 1 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "1（诚实边界）",
                    "1：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 2 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "2（诚实边界）",
                    "2：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 3 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "3（诚实边界）",
                    "3：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 4 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "4（诚实边界）",
                    "4：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_023）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D023) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d023_btn *bb = ui_d023_buttons();
        int bh = ui_d023_hit(bb, UI_D023_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_023 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 人机对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "人机对战（诚实边界）",
                    "人机对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 两人对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "两人对战（诚实边界）",
                    "两人对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 联网 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "联网（诚实边界）",
                    "联网：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 联机 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "联机（诚实边界）",
                    "联机：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_024）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D024) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d024_btn *bb = ui_d024_buttons();
        int bh = ui_d024_hit(bb, UI_D024_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_024 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Type → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Type（诚实边界）",
                    "Type：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_025）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D025) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d025_btn *bb = ui_d025_buttons();
        int bh = ui_d025_hit(bb, UI_D025_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_025 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_026）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D026) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d026_btn *bb = ui_d026_buttons();
        int bh = ui_d026_hit(bb, UI_D026_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_026 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Match → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Match（诚实边界）",
                    "Match：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 2) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_027）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D027) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d027_btn *bb = ui_d027_buttons();
        int bh = ui_d027_hit(bb, UI_D027_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_027 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 随机 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "随机（诚实边界）",
                    "随机：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 默认 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "默认（诚实边界）",
                    "默认：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 自定义 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "自定义（诚实边界）",
                    "自定义：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_028）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D028) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d028_btn *bb = ui_d028_buttons();
        int bh = ui_d028_hit(bb, UI_D028_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_028 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 常用语 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "常用语（诚实边界）",
                    "常用语：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 记录 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "记录（诚实边界）",
                    "记录：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_029）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D029) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d029_btn *bb = ui_d029_buttons();
        int bh = ui_d029_hit(bb, UI_D029_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_029 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 清除 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "清除（诚实边界）",
                    "清除：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_030）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D030) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d030_btn *bb = ui_d030_buttons();
        int bh = ui_d030_hit(bb, UI_D030_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_030 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
                jq_show_dialog(env, "Chat（诚实边界）", "Chat：原版聊天设置项未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_031）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D031) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d031_btn *bb = ui_d031_buttons();
        int bh = ui_d031_hit(bb, UI_D031_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_031 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 修改 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "修改（诚实边界）",
                    "修改：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 帮助 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "帮助（诚实边界）",
                    "帮助：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_032）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D032) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d032_btn *bb = ui_d032_buttons();
        int bh = ui_d032_hit(bb, UI_D032_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_032 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 退出 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "退出（诚实边界）",
                    "退出：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设置（诚实边界）",
                    "设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 用户 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "用户（诚实边界）",
                    "用户：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 更多 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "更多（诚实边界）",
                    "更多：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_033）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D033) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d033_btn *bb = ui_d033_buttons();
        int bh = ui_d033_hit(bb, UI_D033_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_033 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 提交 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "提交（诚实边界）",
                    "提交：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_034）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D034) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d034_btn *bb = ui_d034_buttons();
        int bh = ui_d034_hit(bb, UI_D034_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_034 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_035）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D035) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d035_btn *bb = ui_d035_buttons();
        int bh = ui_d035_hit(bb, UI_D035_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_035 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_036）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D036) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d036_btn *bb = ui_d036_buttons();
        int bh = ui_d036_hit(bb, UI_D036_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_036 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* View1 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "View1（诚实边界）",
                    "View1：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* View2 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "View2（诚实边界）",
                    "View2：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 开始 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "开始（诚实边界）",
                    "开始：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 离开 → 业务动作 */
                ui_screen_set(UI_MENU); LOGI("批量对齐：离开 → 回主菜单"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_037）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D037) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d037_btn *bb = ui_d037_buttons();
        int bh = ui_d037_hit(bb, UI_D037_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_037 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 规则 - 翻棋 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "规则 - 翻棋（诚实边界）",
                    "规则 - 翻棋：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 规则 - 布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "规则 - 布局（诚实边界）",
                    "规则 - 布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_038）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D038) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d038_btn *bb = ui_d038_buttons();
        int bh = ui_d038_hit(bb, UI_D038_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_038 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 登录 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "登录（诚实边界）",
                    "登录：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 注册 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "注册（诚实边界）",
                    "注册：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 游客 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "游客（诚实边界）",
                    "游客：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_039）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D039) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d039_btn *bb = ui_d039_buttons();
        int bh = ui_d039_hit(bb, UI_D039_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_039 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 复盘 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_041）：8 按钮业务分发 */
    if (ui_screen_get() == UI_D041) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d041_btn *bb = ui_d041_buttons();
        int bh = ui_d041_hit(bb, UI_D041_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_041 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "布局（诚实边界）",
                    "布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 翻棋 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "翻棋（诚实边界）",
                    "翻棋：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "布局（诚实边界）",
                    "布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 翻棋 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "翻棋（诚实边界）",
                    "翻棋：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "布局（诚实边界）",
                    "布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 5) {   /* 翻棋 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "翻棋（诚实边界）",
                    "翻棋：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 6) {   /* 布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "布局（诚实边界）",
                    "布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 7) {   /* 翻棋 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "翻棋（诚实边界）",
                    "翻棋：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_042）：7 按钮业务分发 */
    if (ui_screen_get() == UI_D042) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d042_btn *bb = ui_d042_buttons();
        int bh = ui_d042_hit(bb, UI_D042_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_042 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 提和 → 业务动作 */
                jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 认输 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "认输（诚实边界）",
                    "认输：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 聊天 → UI_CHAT063 */
                ui_screen_set(UI_CHAT063); sfx_play(env, "pop");
            } else if (a == 3) {   /* 系统设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "系统设置（诚实边界）",
                    "系统设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 玩家资料 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "玩家资料（诚实边界）",
                    "玩家资料：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 5) {   /* 战况统计 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "战况统计（诚实边界）",
                    "战况统计：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 6) {   /* 软件信息 → UI_ABOUT */
                ui_screen_set(UI_ABOUT); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_043）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D043) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d043_btn *bb = ui_d043_buttons();
        int bh = ui_d043_hit(bb, UI_D043_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_043 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_044）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D044) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d044_btn *bb = ui_d044_buttons();
        int bh = ui_d044_hit(bb, UI_D044_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_044 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 消息 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "消息（诚实边界）",
                    "消息：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 排行 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "排行（诚实边界）",
                    "排行：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_045）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D045) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d045_btn *bb = ui_d045_buttons();
        int bh = ui_d045_hit(bb, UI_D045_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_045 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 详细 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "详细（诚实边界）",
                    "详细：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_046）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D046) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d046_btn *bb = ui_d046_buttons();
        int bh = ui_d046_hit(bb, UI_D046_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_046 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_047）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D047) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d047_btn *bb = ui_d047_buttons();
        int bh = ui_d047_hit(bb, UI_D047_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_047 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_049）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D049) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d049_btn *bb = ui_d049_buttons();
        int bh = ui_d049_hit(bb, UI_D049_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_049 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_050）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D050) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d050_btn *bb = ui_d050_buttons();
        int bh = ui_d050_hit(bb, UI_D050_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_050 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 复盘 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 1) {   /* 删除 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：删除 → 清复盘记录（★ 诚实边界：原版删的是存档项，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 2) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_051）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D051) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d051_btn *bb = ui_d051_buttons();
        int bh = ui_d051_hit(bb, UI_D051_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_051 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 清空 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：清空 → 清复盘记录（★ 诚实边界：原版清的是本弹窗列表存档，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_052）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D052) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d052_btn *bb = ui_d052_buttons();
        int bh = ui_d052_hit(bb, UI_D052_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_052 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 重试 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "重试（诚实边界）",
                    "重试：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_053）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D053) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d053_btn *bb = ui_d053_buttons();
        int bh = ui_d053_hit(bb, UI_D053_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_053 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 注册 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "注册（诚实边界）",
                    "注册：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_054）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D054) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d054_btn *bb = ui_d054_buttons();
        int bh = ui_d054_hit(bb, UI_D054_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_054 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 清空 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：清空 → 清复盘记录（★ 诚实边界：原版清的是本弹窗列表存档，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_055）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D055) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d055_btn *bb = ui_d055_buttons();
        int bh = ui_d055_hit(bb, UI_D055_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_055 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_057）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D057) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d057_btn *bb = ui_d057_buttons();
        int bh = ui_d057_hit(bb, UI_D057_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_057 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 修改密码 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "修改密码（诚实边界）",
                    "修改密码：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_058）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D058) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d058_btn *bb = ui_d058_buttons();
        int bh = ui_d058_hit(bb, UI_D058_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_058 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_059）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D059) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d059_btn *bb = ui_d059_buttons();
        int bh = ui_d059_hit(bb, UI_D059_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_059 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 换一张 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "换一张（诚实边界）",
                    "换一张：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 2) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_060）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D060) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d060_btn *bb = ui_d060_buttons();
        int bh = ui_d060_hit(bb, UI_D060_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_060 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* View1 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "View1（诚实边界）",
                    "View1：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* View2 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "View2（诚实边界）",
                    "View2：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 开始 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "开始（诚实边界）",
                    "开始：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 换桌 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "换桌（诚实边界）",
                    "换桌：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 离开 → 业务动作 */
                ui_screen_set(UI_MENU); LOGI("批量对齐：离开 → 回主菜单"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_061）：6 按钮业务分发 */
    if (ui_screen_get() == UI_D061) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d061_btn *bb = ui_d061_buttons();
        int bh = ui_d061_hit(bb, UI_D061_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_061 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* First → 边界对话框（诚实边界） */
                jq_show_dialog(env, "First（诚实边界）",
                    "First：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Level → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Level（诚实边界）",
                    "Level：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Lay → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Lay（诚实边界）",
                    "Lay：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* Show → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Show（诚实边界）",
                    "Show：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 5) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_062）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D062) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d062_btn *bb = ui_d062_buttons();
        int bh = ui_d062_hit(bb, UI_D062_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_062 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Lay → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Lay（诚实边界）",
                    "Lay：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Show → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Show（诚实边界）",
                    "Show：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_063）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D063) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d063_btn *bb = ui_d063_buttons();
        int bh = ui_d063_hit(bb, UI_D063_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_063 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 发送 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "发送（诚实边界）",
                    "发送：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 记录 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "记录（诚实边界）",
                    "记录：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_064）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D064) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d064_btn *bb = ui_d064_buttons();
        int bh = ui_d064_hit(bb, UI_D064_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_064 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
                jq_show_dialog(env, "Chat（诚实边界）", "Chat：原版聊天设置项未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_065）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D065) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d065_btn *bb = ui_d065_buttons();
        int bh = ui_d065_hit(bb, UI_D065_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_065 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 开始 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "开始（诚实边界）",
                    "开始：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_066）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D066) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d066_btn *bb = ui_d066_buttons();
        int bh = ui_d066_hit(bb, UI_D066_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_066 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 创建游戏 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "创建游戏（诚实边界）",
                    "创建游戏：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 加入游戏 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "加入游戏（诚实边界）",
                    "加入游戏：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_067）：8 按钮业务分发 */
    if (ui_screen_get() == UI_D067) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d067_btn *bb = ui_d067_buttons();
        int bh = ui_d067_hit(bb, UI_D067_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_067 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 重新开始 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "重新开始（诚实边界）",
                    "重新开始：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 悔棋 → 业务动作 */
                { pthread_mutex_lock(&g_mutex); undo_pop(); pthread_mutex_unlock(&g_mutex); }   /* ★ F3-001（P1 已修）：锁内单 pop（g_state 整体赋值与 GLThread 渲染读竞态消除） */
                ui_screen_set(UI_GAME); LOGI("批量对齐：悔棋 → undo_pop（已悔步）"); sfx_play(env, "pop");
            } else if (a == 2) {   /* 提和 → 业务动作 */
                jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 认输 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "认输（诚实边界）",
                    "认输：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 聊天 → UI_CHAT063 */
                ui_screen_set(UI_CHAT063); sfx_play(env, "pop");
            } else if (a == 5) {   /* 系统设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "系统设置（诚实边界）",
                    "系统设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 6) {   /* 战况统计 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "战况统计（诚实边界）",
                    "战况统计：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 7) {   /* 游戏规则 → UI_RULES */
                ui_screen_set(UI_RULES); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_068）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D068) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d068_btn *bb = ui_d068_buttons();
        int bh = ui_d068_hit(bb, UI_D068_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_068 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 设备名 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设备名（诚实边界）",
                    "设备名：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_069）：6 按钮业务分发 */
    if (ui_screen_get() == UI_D069) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d069_btn *bb = ui_d069_buttons();
        int bh = ui_d069_hit(bb, UI_D069_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_069 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* First → 边界对话框（诚实边界） */
                jq_show_dialog(env, "First（诚实边界）",
                    "First：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Lay → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Lay（诚实边界）",
                    "Lay：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Show → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Show（诚实边界）",
                    "Show：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设置（诚实边界）",
                    "设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 5) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_070）：6 按钮业务分发 */
    if (ui_screen_get() == UI_D070) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d070_btn *bb = ui_d070_buttons();
        int bh = ui_d070_hit(bb, UI_D070_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_070 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Draw（和棋/提和）→ 边界对话框（诚实边界：本地提和判定未实现，提和/认输走 BT 帧） */
                jq_show_dialog(env, "Draw（诚实边界）",
                    "Draw：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Command → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Command（诚实边界）",
                    "Command：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* BingLei → 边界对话框（诚实边界） */
                jq_show_dialog(env, "BingLei（诚实边界）",
                    "BingLei：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* BingQi → 边界对话框（诚实边界） */
                jq_show_dialog(env, "BingQi（诚实边界）",
                    "BingQi：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 5) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_071）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D071) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d071_btn *bb = ui_d071_buttons();
        int bh = ui_d071_hit(bb, UI_D071_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_071 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 搜索 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "搜索（诚实边界）",
                    "搜索：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_072）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D072) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d072_btn *bb = ui_d072_buttons();
        int bh = ui_d072_hit(bb, UI_D072_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_072 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 蓝牙联机 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "蓝牙联机（诚实边界）",
                    "蓝牙联机：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Wi-Fi 联机 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Wi-Fi 联机（诚实边界）",
                    "Wi-Fi 联机：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 个人资料 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "个人资料（诚实边界）",
                    "个人资料：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_073）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D073) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d073_btn *bb = ui_d073_buttons();
        int bh = ui_d073_hit(bb, UI_D073_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_073 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 开始 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "开始（诚实边界）",
                    "开始：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_075）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D075) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d075_btn *bb = ui_d075_buttons();
        int bh = ui_d075_hit(bb, UI_D075_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_075 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Prev → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Prev（诚实边界）",
                    "Prev：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Next → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Next（诚实边界）",
                    "Next：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_077）：6 按钮业务分发 */
    if (ui_screen_get() == UI_D077) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d077_btn *bb = ui_d077_buttons();
        int bh = ui_d077_hit(bb, UI_D077_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_077 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 开局 → 业务动作 */
                jq_show_dialog(env, "开局（诚实边界）", "开局：原版开局操作通道未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 终局 → 业务动作 */
                jq_show_dialog(env, "终局（诚实边界）", "终局：原版终局操作通道未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 切换显示 → 业务动作 */
                jq_show_dialog(env, "切换显示（诚实边界）", "切换显示：原版棋盘显示切换通道未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 对局信息 → UI_GAMEINFO */
                ui_screen_set(UI_GAMEINFO); sfx_play(env, "pop");
            } else if (a == 4) {   /* 功能设置 → UI_GAMESET */
                ui_screen_set(UI_GAMESET); sfx_play(env, "pop");
            } else if (a == 5) {   /* 软件信息 → UI_ABOUT */
                ui_screen_set(UI_ABOUT); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_078）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D078) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d078_btn *bb = ui_d078_buttons();
        int bh = ui_d078_hit(bb, UI_D078_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_078 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 新局 → UI_NEWGAME */
                ui_screen_set(UI_NEWGAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 复盘 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_079）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D079) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d079_btn *bb = ui_d079_buttons();
        int bh = ui_d079_hit(bb, UI_D079_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_079 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_082）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D082) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d082_btn *bb = ui_d082_buttons();
        int bh = ui_d082_hit(bb, UI_D082_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_082 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 菜单 → UI_INGAME_MENU */
                ui_screen_set(UI_INGAME_MENU); sfx_play(env, "pop");
            } else if (a == 1) {   /* 新局 → UI_NEWGAME */
                ui_screen_set(UI_NEWGAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 悔棋 → 业务动作 */
                { pthread_mutex_lock(&g_mutex); undo_pop(); pthread_mutex_unlock(&g_mutex); }   /* ★ F3-001（P1 已修）：锁内单 pop（g_state 整体赋值与 GLThread 渲染读竞态消除） */
                ui_screen_set(UI_GAME); LOGI("批量对齐：悔棋 → undo_pop（已悔步）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设置（诚实边界）",
                    "设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_083）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D083) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d083_btn *bb = ui_d083_buttons();
        int bh = ui_d083_hit(bb, UI_D083_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_083 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 游戏规则 → UI_RULES */
                ui_screen_set(UI_RULES); sfx_play(env, "pop");
            } else if (a == 1) {   /* 功能设置 → UI_GAMESET */
                ui_screen_set(UI_GAMESET); sfx_play(env, "pop");
            } else if (a == 2) {   /* 软件信息 → UI_ABOUT */
                ui_screen_set(UI_ABOUT); sfx_play(env, "pop");
            } else if (a == 3) {   /* 退出程序 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "退出程序（诚实边界）",
                    "退出程序：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_084）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D084) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d084_btn *bb = ui_d084_buttons();
        int bh = ui_d084_hit(bb, UI_D084_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_084 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Expand → 业务动作 */
                jq_show_dialog(env, "Expand（诚实边界）", "Expand：原版设置扩展项未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Clockwise → 业务动作 */
                jq_show_dialog(env, "Clockwise（诚实边界）", "Clockwise：原版旋转设置未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_085）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D085) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d085_btn *bb = ui_d085_buttons();
        int bh = ui_d085_hit(bb, UI_D085_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_085 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 载入 → UI_LOAD_LAY */
                ui_screen_set(UI_LOAD_LAY); sfx_play(env, "pop");
            } else if (a == 1) {   /* 保存 → UI_SAVE_LAY */
                ui_screen_set(UI_SAVE_LAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 清空 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：清空 → 清复盘记录（★ 诚实边界：原版清的是本弹窗列表存档，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 完成 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：完成 → 回对局"); sfx_play(env, "pop");
            } else if (a == 4) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_086）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D086) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d086_btn *bb = ui_d086_buttons();
        int bh = ui_d086_hit(bb, UI_D086_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_086 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Rename → UI_LAY_MGR */
                ui_screen_set(UI_LAY_MGR); sfx_play(env, "pop");
            } else if (a == 1) {   /* 载入 → UI_LOAD_LAY */
                ui_screen_set(UI_LOAD_LAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 删除 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：删除 → 清复盘记录（★ 诚实边界：原版删的是存档项，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_089）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D089) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d089_btn *bb = ui_d089_buttons();
        int bh = ui_d089_hit(bb, UI_D089_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_089 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 2 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "2（诚实边界）",
                    "2：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 3 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "3（诚实边界）",
                    "3：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 4 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "4（诚实边界）",
                    "4：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_090）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D090) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d090_btn *bb = ui_d090_buttons();
        int bh = ui_d090_hit(bb, UI_D090_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_090 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 人机对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "人机对战（诚实边界）",
                    "人机对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 联网对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "联网对战（诚实边界）",
                    "联网对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_094）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D094) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d094_btn *bb = ui_d094_buttons();
        int bh = ui_d094_hit(bb, UI_D094_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_094 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Expand → 业务动作 */
                jq_show_dialog(env, "Expand（诚实边界）", "Expand：原版设置扩展项未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Clockwise → 业务动作 */
                jq_show_dialog(env, "Clockwise（诚实边界）", "Clockwise：原版旋转设置未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
                jq_show_dialog(env, "Chat（诚实边界）", "Chat：原版聊天设置项未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 4) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_095）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D095) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d095_btn *bb = ui_d095_buttons();
        int bh = ui_d095_hit(bb, UI_D095_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_095 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 菜单 → UI_INGAME_MENU */
                ui_screen_set(UI_INGAME_MENU); sfx_play(env, "pop");
            } else if (a == 1) {   /* 复盘 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设置（诚实边界）",
                    "设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 投降 → 业务动作 */
                /* ★ AUD-1000 B03-071/072（P0 已修）：g_state 写移入 g_mutex（锁外写与
                   GLThread OnUpdate 锁内渲染读构成数据竞争）。 */
                int winner_snap = -1;   /* ★ AUD-1000 B03-064（P1 已修）：锁内快照 winner */
                pthread_mutex_lock(&g_mutex);
                /* ★ AUD-1000 B03-009（P0 已修）：g_state 写移入 g_mutex */
                g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
                g_state.stage = STG_OVER;
                winner_snap = g_state.winner;
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME); LOGI("批量对齐：投降 → 本机败（winner=%d，锁内快照）", winner_snap); sfx_play(env, "pop");
            } else if (a == 4) {   /* 提和 → 业务动作 */
                jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_096）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D096) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d096_btn *bb = ui_d096_buttons();
        int bh = ui_d096_hit(bb, UI_D096_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_096 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 修改 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "修改（诚实边界）",
                    "修改：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 帮助 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "帮助（诚实边界）",
                    "帮助：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_097）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D097) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d097_btn *bb = ui_d097_buttons();
        int bh = ui_d097_hit(bb, UI_D097_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_097 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 退出 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "退出（诚实边界）",
                    "退出：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设置（诚实边界）",
                    "设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 消息 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "消息（诚实边界）",
                    "消息：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 用户 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "用户（诚实边界）",
                    "用户：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_098）：6 按钮业务分发 */
    if (ui_screen_get() == UI_D098) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d098_btn *bb = ui_d098_buttons();
        int bh = ui_d098_hit(bb, UI_D098_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_098 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 四暗 → 业务动作 */
                ui_screen_set(UI_DIFFICULTY); LOGI("批量对齐：四暗 → 难度选择（datah_098 布局规则）"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 双明 → 业务动作 */
                ui_screen_set(UI_DIFFICULTY); LOGI("批量对齐：双明 → 难度选择（datah_098 布局规则）"); sfx_play(env, "pop");
            } else if (a == 2) {   /* 四暗 → 业务动作 */
                ui_screen_set(UI_DIFFICULTY); LOGI("批量对齐：四暗 → 难度选择（datah_098 布局规则）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 双明 → 业务动作 */
                ui_screen_set(UI_DIFFICULTY); LOGI("批量对齐：双明 → 难度选择（datah_098 布局规则）"); sfx_play(env, "pop");
            } else if (a == 4) {   /* 四暗 → 业务动作 */
                ui_screen_set(UI_DIFFICULTY); LOGI("批量对齐：四暗 → 难度选择（datah_098 布局规则）"); sfx_play(env, "pop");
            } else if (a == 5) {   /* 双明 → 业务动作 */
                ui_screen_set(UI_DIFFICULTY); LOGI("批量对齐：双明 → 难度选择（datah_098 布局规则）"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_099）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D099) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d099_btn *bb = ui_d099_buttons();
        int bh = ui_d099_hit(bb, UI_D099_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_099 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 对局信息 → UI_GAMEINFO */
                ui_screen_set(UI_GAMEINFO); sfx_play(env, "pop");
            } else if (a == 1) {   /* 游戏规则 → UI_RULES */
                ui_screen_set(UI_RULES); sfx_play(env, "pop");
            } else if (a == 2) {   /* 功能设置 → UI_GAMESET */
                ui_screen_set(UI_GAMESET); sfx_play(env, "pop");
            } else if (a == 3) {   /* 软件信息 → UI_ABOUT */
                ui_screen_set(UI_ABOUT); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_100）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D100) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d100_btn *bb = ui_d100_buttons();
        int bh = ui_d100_hit(bb, UI_D100_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_100 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 详细 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "详细（诚实边界）",
                    "详细：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_101）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D101) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d101_btn *bb = ui_d101_buttons();
        int bh = ui_d101_hit(bb, UI_D101_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_101 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "布局（诚实边界）",
                    "布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 出征 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "出征（诚实边界）",
                    "出征：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_102）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D102) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d102_btn *bb = ui_d102_buttons();
        int bh = ui_d102_hit(bb, UI_D102_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_102 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_104）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D104) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d104_btn *bb = ui_d104_buttons();
        int bh = ui_d104_hit(bb, UI_D104_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_104 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Me → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Me（诚实边界）",
                    "Me：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Difficulty → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Difficulty（诚实边界）",
                    "Difficulty：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Type → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Type（诚实边界）",
                    "Type：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 4) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_107）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D107) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d107_btn *bb = ui_d107_buttons();
        int bh = ui_d107_hit(bb, UI_D107_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_107 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 菜单 → UI_INGAME_MENU */
                ui_screen_set(UI_INGAME_MENU); sfx_play(env, "pop");
            } else if (a == 1) {   /* 回退 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 前进 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 3) {   /* 跳转 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 4) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_108）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D108) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d108_btn *bb = ui_d108_buttons();
        int bh = ui_d108_hit(bb, UI_D108_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_108 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 复盘 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 1) {   /* 删除 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：删除 → 清复盘记录（★ 诚实边界：原版删的是存档项，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 2) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_109）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D109) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d109_btn *bb = ui_d109_buttons();
        int bh = ui_d109_hit(bb, UI_D109_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_109 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 开局 → 业务动作 */
                jq_show_dialog(env, "开局（诚实边界）", "开局：原版开局操作通道未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 终局 → 业务动作 */
                jq_show_dialog(env, "终局（诚实边界）", "终局：原版终局操作通道未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 定位 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "定位（诚实边界）",
                    "定位：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_110）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D110) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d110_btn *bb = ui_d110_buttons();
        int bh = ui_d110_hit(bb, UI_D110_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_110 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 清空 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：清空 → 清复盘记录（★ 诚实边界：原版清的是本弹窗列表存档，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 关闭 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "关闭（诚实边界）",
                    "关闭：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_112）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D112) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d112_btn *bb = ui_d112_buttons();
        int bh = ui_d112_hit(bb, UI_D112_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_112 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 切换显示 → 业务动作 */
                jq_show_dialog(env, "切换显示（诚实边界）", "切换显示：原版棋盘显示切换通道未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 对局信息 → UI_GAMEINFO */
                ui_screen_set(UI_GAMEINFO); sfx_play(env, "pop");
            } else if (a == 2) {   /* 游戏规则 → UI_RULES */
                ui_screen_set(UI_RULES); sfx_play(env, "pop");
            } else if (a == 3) {   /* 功能设置 → UI_GAMESET */
                ui_screen_set(UI_GAMESET); sfx_play(env, "pop");
            } else if (a == 4) {   /* 软件信息 → UI_ABOUT */
                ui_screen_set(UI_ABOUT); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_113）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D113) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d113_btn *bb = ui_d113_buttons();
        int bh = ui_d113_hit(bb, UI_D113_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_113 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 0 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "0（诚实边界）",
                    "0：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 1 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "1（诚实边界）",
                    "1：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 2 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "2（诚实边界）",
                    "2：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 3 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "3（诚实边界）",
                    "3：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_114）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D114) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d114_btn *bb = ui_d114_buttons();
        int bh = ui_d114_hit(bb, UI_D114_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_114 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Site → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Site（诚实边界）",
                    "Site：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_118）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D118) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d118_btn *bb = ui_d118_buttons();
        int bh = ui_d118_hit(bb, UI_D118_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_118 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 更新 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "更新（诚实边界）",
                    "更新：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_119）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D119) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d119_btn *bb = ui_d119_buttons();
        int bh = ui_d119_hit(bb, UI_D119_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_119 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_120）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D120) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d120_btn *bb = ui_d120_buttons();
        int bh = ui_d120_hit(bb, UI_D120_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_120 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 注册用户 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "注册用户（诚实边界）",
                    "注册用户：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 找回密码 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "找回密码（诚实边界）",
                    "找回密码：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_121）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D121) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d121_btn *bb = ui_d121_buttons();
        int bh = ui_d121_hit(bb, UI_D121_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_121 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_122）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D122) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d122_btn *bb = ui_d122_buttons();
        int bh = ui_d122_hit(bb, UI_D122_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_122 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 确定 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：确定 → 回对局"); sfx_play(env, "pop");
            } else if (a == 1) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_123）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D123) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d123_btn *bb = ui_d123_buttons();
        int bh = ui_d123_hit(bb, UI_D123_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_123 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Wall → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Wall（诚实边界）",
                    "Wall：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_246）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D246) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d246_btn *bb = ui_d246_buttons();
        int bh = ui_d246_hit(bb, UI_D246_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_246 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_247）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D247) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d247_btn *bb = ui_d247_buttons();
        int bh = ui_d247_hit(bb, UI_D247_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_247 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_248）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D248) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d248_btn *bb = ui_d248_buttons();
        int bh = ui_d248_hit(bb, UI_D248_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_248 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 人机对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "人机对战（诚实边界）",
                    "人机对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 两人对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "两人对战（诚实边界）",
                    "两人对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 联网 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "联网（诚实边界）",
                    "联网：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 联机 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "联机（诚实边界）",
                    "联机：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_249）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D249) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d249_btn *bb = ui_d249_buttons();
        int bh = ui_d249_hit(bb, UI_D249_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_249 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Type → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Type（诚实边界）",
                    "Type：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_250）：2 按钮业务分发 */
    if (ui_screen_get() == UI_D250) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d250_btn *bb = ui_d250_buttons();
        int bh = ui_d250_hit(bb, UI_D250_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_250 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
                jq_show_dialog(env, "Chat（诚实边界）", "Chat：原版聊天设置项未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_251）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D251) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d251_btn *bb = ui_d251_buttons();
        int bh = ui_d251_hit(bb, UI_D251_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_251 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 复盘 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_253）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D253) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d253_btn *bb = ui_d253_buttons();
        int bh = ui_d253_hit(bb, UI_D253_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_253 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 详细 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "详细（诚实边界）",
                    "详细：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_255）：3 按钮业务分发 */
    if (ui_screen_get() == UI_D255) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d255_btn *bb = ui_d255_buttons();
        int bh = ui_d255_hit(bb, UI_D255_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_255 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Chat（聊天）→ 业务动作（聊天文本未逆向，发送=BT_T_CHAT 占位帧） */
                jq_show_dialog(env, "Chat（诚实边界）", "Chat：原版聊天设置项未逆向。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_256）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D256) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d256_btn *bb = ui_d256_buttons();
        int bh = ui_d256_hit(bb, UI_D256_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_256 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Prev → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Prev（诚实边界）",
                    "Prev：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* Next → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Next（诚实边界）",
                    "Next：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* Menu（菜单）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Menu（诚实边界）",
                    "Menu：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* Back（返回）→ 边界对话框（诚实边界：Wi-Fi/局域网联机未逆向，仅蓝牙已实现） */
                jq_show_dialog(env, "Back（诚实边界）",
                    "Back：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_257）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D257) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d257_btn *bb = ui_d257_buttons();
        int bh = ui_d257_hit(bb, UI_D257_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_257 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 菜单 → UI_INGAME_MENU */
                ui_screen_set(UI_INGAME_MENU); sfx_play(env, "pop");
            } else if (a == 1) {   /* 新局 → UI_NEWGAME */
                ui_screen_set(UI_NEWGAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 悔棋 → 业务动作 */
                { pthread_mutex_lock(&g_mutex); undo_pop(); pthread_mutex_unlock(&g_mutex); }   /* ★ F3-001（P1 已修）：锁内单 pop（g_state 整体赋值与 GLThread 渲染读竞态消除） */
                ui_screen_set(UI_GAME); LOGI("批量对齐：悔棋 → undo_pop（已悔步）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设置（诚实边界）",
                    "设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 4) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_258）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D258) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d258_btn *bb = ui_d258_buttons();
        int bh = ui_d258_hit(bb, UI_D258_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_258 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 载入 → UI_LOAD_LAY */
                ui_screen_set(UI_LOAD_LAY); sfx_play(env, "pop");
            } else if (a == 1) {   /* 保存 → UI_SAVE_LAY */
                ui_screen_set(UI_SAVE_LAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 清空 → 业务动作 */
                pthread_mutex_lock(&g_mutex); ui_replay_init(); pthread_mutex_unlock(&g_mutex); ui_screen_set(UI_GAME); LOGI("批量对齐：清空 → 清复盘记录（★ 诚实边界：原版清的是本弹窗列表存档，未逆向；AUD-1000 B03-010 锁包裹）"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 完成 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：完成 → 回对局"); sfx_play(env, "pop");
            } else if (a == 4) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_259）：2 按钮业务分发
       ★ AUD-1000 B03-021（P1 已注释）：D* 批量弹窗（D000-D264 ~70 个）命中后消费
       事件（return JNI_TRUE），未命中 fall-through 到面板外判定 → 回对局/主菜单；
       D* 态与业务弹窗（UI_GAMESET/UI_LOGIN/UI_NEWGAME 等）在 ui_menu.h 单枚举
       不重叠（UI_Dxxx 独立区段），路由无冲突；本注释代表全 D* 批量态。 */
    if (ui_screen_get() == UI_D259) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d259_btn *bb = ui_d259_buttons();
        int bh = ui_d259_hit(bb, UI_D259_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_259 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 人机对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "人机对战（诚实边界）",
                    "人机对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 联网对战 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "联网对战（诚实边界）",
                    "联网对战：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_260）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D260) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d260_btn *bb = ui_d260_buttons();
        int bh = ui_d260_hit(bb, UI_D260_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_260 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 菜单 → UI_INGAME_MENU */
                ui_screen_set(UI_INGAME_MENU); sfx_play(env, "pop");
            } else if (a == 1) {   /* 复盘 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 设置 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "设置（诚实边界）",
                    "设置：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 3) {   /* 投降 → 业务动作 */
                /* ★ AUD-1000 B03-071/072（P0 已修）：g_state 写移入 g_mutex（锁外写与
                   GLThread OnUpdate 锁内渲染读构成数据竞争）。 */
                int winner_snap = -1;   /* ★ AUD-1000 B03-064（P1 已修）：锁内快照 winner */
                pthread_mutex_lock(&g_mutex);
                /* ★ AUD-1000 B03-009（P0 已修）：g_state 写移入 g_mutex */
                g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
                g_state.stage = STG_OVER;
                winner_snap = g_state.winner;
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME); LOGI("批量对齐：投降 → 本机败（winner=%d，锁内快照）", winner_snap); sfx_play(env, "pop");
            } else if (a == 4) {   /* 提和 → 业务动作 */
                jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_261）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D261) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d261_btn *bb = ui_d261_buttons();
        int bh = ui_d261_hit(bb, UI_D261_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_261 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 详细 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "详细（诚实边界）",
                    "详细：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_262）：4 按钮业务分发 */
    if (ui_screen_get() == UI_D262) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d262_btn *bb = ui_d262_buttons();
        int bh = ui_d262_hit(bb, UI_D262_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_262 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 布局 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "布局（诚实边界）",
                    "布局：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 1) {   /* 出征 → 边界对话框（诚实边界） */
                jq_show_dialog(env, "出征（诚实边界）",
                    "出征：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            } else if (a == 2) {   /* 取消 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：取消 → 回对局"); sfx_play(env, "pop");
            } else if (a == 3) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_263）：5 按钮业务分发 */
    if (ui_screen_get() == UI_D263) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d263_btn *bb = ui_d263_buttons();
        int bh = ui_d263_hit(bb, UI_D263_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_263 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* 菜单 → UI_INGAME_MENU */
                ui_screen_set(UI_INGAME_MENU); sfx_play(env, "pop");
            } else if (a == 1) {   /* 回退 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 2) {   /* 前进 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 3) {   /* 跳转 → UI_REPLAY */
                ui_screen_set(UI_REPLAY); sfx_play(env, "pop");
            } else if (a == 4) {   /* 返回 → 业务动作 */
                ui_screen_set(UI_GAME); LOGI("批量对齐：返回 → 回对局"); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
    /* ★ 批量对齐（datah_264）：1 按钮业务分发 */
    if (ui_screen_get() == UI_D264) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_d264_btn *bb = ui_d264_buttons();
        int bh = ui_d264_hit(bb, UI_D264_COUNT, bx, by);
        if (bh >= 0) {
            int a = bb[bh].action;
            LOGI("OnTouch datah_264 弹窗：按钮%d（action=%d）", bh, a);
            if (a < 0) {
            } else if (a == 0) {   /* Site → 边界对话框（诚实边界） */
                jq_show_dialog(env, "Site（诚实边界）",
                    "Site：原版业务未逆向，src_rs 无此通道。");
                ui_screen_set(UI_GAME); sfx_play(env, "pop");
            }   /* 收尾：末分支闭合 */
        }
        return JNI_TRUE;
    }
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
            ui_vs_ai_set(1);   /* ★ 人机：红方 AI 走子
               ★ AUD-1000 B03-050/B03-059（P1 已注释）：锁序固化——先 g_ui_lock
               （ui_opt_select/ui_vs_ai_set，各独立不嵌套）→ 后 g_mutex
               （jq_restart_game 内 ui_opt_get_mode 再取 g_ui_lock）=
               g_mutex→g_ui_lock 单向嵌套；grep 实证 0 处 g_ui_lock→g_mutex
               反向（CODE_STYLE §锁序，文档锚点此处）。 */
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
                /* ★ 对抗审计 P0（P0-R10）：g_sound_on 随静音模式同步（总开关语义：
                   静音模式开=声音关 → g_sound_on=0；静音模式关=声音开 → g_sound_on=1）。
                   原 g_sound_on 死变量（从未消费），现接入 sfx_play 门控。 */
                g_sound_on = ui_snd_mute_get() ? 0 : 1;
                sfx_play(env, "pop");
                LOGI("声音设置：静音模式 → %s（g_sound_on=%d）", ui_snd_mute_get() ? "开" : "关", g_sound_on);
            } else if (sact == 1) {   /* 按键声音切换 */
                ui_btn_sound_set(!ui_btn_sound_get());
                sfx_play(env, "pop");
                LOGI("声音设置：按键声音 → %s", ui_btn_sound_get() ? "开" : "关");
            } else if (sact == 2) {   /* 音量滑条（btn[2]）：点按位置 → 连续映射 1-3（原版 Slider 语义，
               滑条区 x∈[255,562] 720 基准，按点击 x 位置映射 低/中/高，替代 3 档循环） */
                int v;
                if (bx >= sb[2].x0 && bx <= sb[2].x1) {
                    float t = (bx - sb[2].x0) / (sb[2].x1 - sb[2].x0);
                    v = (int)(t * 3.0f + 0.5f);
                    if (v < 1) v = 1; if (v > 3) v = 3;
                } else v = ui_volume_get();
                ui_volume_set(v);
                sfx_play(env, "pop");
                LOGI("声音设置：音量滑条位置映射 → 档位=%d（bx=%.0f 滑条区[%.0f,%.0f]）",
                     v, bx, sb[2].x0, sb[2].x1);
            } else if (sact == 3) {   /* 确定（btn[3]）：保存 */
                ui_settings_save();
                sfx_sound_save();
                sfx_play(env, "pop");
                LOGI("声音设置：确定 → 保存 + 回对局");
                ui_screen_set(UI_GAME);
            } else if (sact == 4) {   /* 取消（btn[4]）：不保存回对局 */
                ui_settings_load();
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
                "src_rs 为 12×5 两人军棋（50 子）规则重写，四国（4 方混战）\n"   /* AUD-R-076：旧文案"10×9"系误读 */
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
            if (nact == 0 || nact == 1) {   /* 玩法/明暗棋：同义切换（明棋=布局 / 翻棋暗棋）
                ★ AUD-1000 B03-023（P1 已修）：切换走 ui_opt_toggle_mode 单入口
                （锁内 read-modify-write，旧 get→取反→select 三步锁外裸改丢更新）。 */
                int mode = ui_opt_toggle_mode();
                sfx_play(env, "pop");
                LOGI("OnTouch 新局弹窗：玩法/明暗棋切换 → mode=%d（%s，默认翻棋暗棋对齐原版）",
                     mode, mode ? "翻棋暗棋" : "布局明棋");
            } else if (nact == 2) {         /* 确定：随模式开局（两人热座，AI 不介入）
                ★ AUD-1000 B03-061（P1 误报撤回，已注释）：ui_vs_ai_set(0) 锁外与
                jq_restart_game 读 g_vs_ai 之间无同步——jq_restart_game 内 ai_pick
                不会调用（无棋盘渲染，纯布局重载），安全。 */
                ui_vs_ai_set(0);
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
                ui_vs_ai_set(0);   /* 游客=本地单机：AI 不介入，两人热座 */
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
                    /* ★ AUD-1000 B03-054（P1 已注释）：布局载入整局重置（jq_load_layout +
                       revealed 循环 + 清 undo）持 g_mutex——低频弹窗路径（UI_LOAD_LAY
                       "载入"按钮），O(N_CELLS) 短暂持锁可接受（与 B03-055 UI_LAY_MGR
                       同范式），高频点击面有限；锁契约保留（g_state 读写必须 g_mutex）。 */
                    pthread_mutex_lock(&g_mutex);
                    jq_load_layout(&g_state, lay);
                    int dark = ui_opt_get_mode() == 1;
                    for (int i = 0; i < N_CELLS; i++)
                        if (g_state.cells[i].rank != RANK_EMPTY)
                            g_state.cells[i].revealed = dark ? 0 : 1;
                    g_undo_top = 0;
                    g_state.stage = STG_PLAY;
                    g_state.turn = SIDE_BLUE;
                    g_state.winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
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
                    g_state.winner = JQ_WIN_NONE;   // AUD-R-053：winner 未定哨兵
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
            } else if (nact == 2) {   /* 玩法：布局/翻棋 切换
                ★ AUD-1000 B03-023（P1 已修）：ui_opt_toggle_mode 单入口原子切换 */
                int mode = ui_opt_toggle_mode();
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版弹窗：玩法切换 → mode=%d（%s，默认翻棋暗棋）",
                     mode, mode ? "翻棋暗棋" : "布局明棋");
            } else if (nact == 3) {   /* 确定：按选择开局（ui_opt_select + jq_restart_game 随模式） */
                ui_vs_ai_set(0);
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
            } else if (nact == 2) {   /* 玩法：布局/翻棋 切换
                ★ AUD-1000 B03-023（P1 已修）：ui_opt_toggle_mode 单入口原子切换 */
                int mode = ui_opt_toggle_mode();
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：玩法切换 → mode=%d（%s）",
                     mode, mode ? "翻棋暗棋" : "布局明棋");
            } else if (nact == 3) {   /* 明暗棋：明/暗 切换（ui_show 独立循环位） */
                ui_show_set(ui_show_get() ? 0 : 1);
                sfx_play(env, "pop");
                LOGI("OnTouch 新局完整版（完整版补充）弹窗：明暗棋切换 → %s（★ 诚实边界：简化用 ui_show 独立循环位）",
                     ui_show_get() ? "暗棋" : "明棋");
            } else if (nact == 4) {   /* 确定：按选择开局（ui_opt_select + jq_restart_game 随模式） */
                ui_vs_ai_set(0);
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
            if (gact == 0) {   /* 复盘：进 UI_REPLAY 复盘态（复用 P1 复盘）
                ★ AUD-1000 B02-081（P1 已修）：rewind 改 g_state 须 g_mutex
                （与锁内 ui_replay_record/OnUpdate 渲染读并发，同 B02-080 范式）。 */
                int n = ui_replay_count();
                int rc;
                pthread_mutex_lock(&g_mutex);
                rc = ui_replay_rewind(n);
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_REPLAY);
                sfx_play(env, "pop");
                LOGI("OnTouch 对局信息弹窗：复盘 → UI_REPLAY 复盘态（datah_107 控制条 5 按钮，cur=%d/%d rewind rc=%d）",
                     n, n, rc);
            } else if (gact == 1) {   /* 删除：清复盘记录（诚实边界简化）
                ★ AUD-1000 B02-082（P1 已修）：ui_replay_init 改 g_replay 须 g_mutex。 */
                pthread_mutex_lock(&g_mutex);
                ui_replay_init();
                pthread_mutex_unlock(&g_mutex);
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
            if (bact == 0) {   /* 亮度滑条：点按位置 → 连续映射 0-2 档（原版 Slider 语义，
               滑条区 x∈[152,568] 720 基准，按点击 x 位置映射低/中/高，替代 3 档循环） */
                int v;
                if (bx >= bb[0].x0 && bx <= bb[0].x1) {
                    v = (int)((bx - bb[0].x0) / (bb[0].x1 - bb[0].x0) * 3.0f);
                    if (v > 2) v = 2;
                    if (v < 0) v = 0;
                } else v = ui_bright_get();
                ui_bright_set(v);
                sfx_play(env, "pop");
                LOGI("OnTouch 亮度调节弹窗：滑条位置映射 → 档位=%d（%s，bx=%.0f 滑条区[%.0f,%.0f]）",
                     v, v == 0 ? "低" : (v == 1 ? "中" : "高"), bx, bb[0].x0, bb[0].x1);
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
                int winner_snap = -1;   /* ★ AUD-1000 B03-064（P1 已修）：锁内快照 winner */
                pthread_mutex_lock(&g_mutex);
                /* ★ AUD-1000 B03-009（P0 已修）：g_state 写移入 g_mutex */
                g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
                g_state.stage = STG_OVER;
                winner_snap = g_state.winner;
                pthread_mutex_unlock(&g_mutex);
                jq_bt_ctrl_send(env, BT_T_RESIGN);
                sfx_play(env, "over");
                LOGI("OnTouch 提和详情：认输 → 对方胜（winner=%d，锁内快照）+ RESIGN 帧", winner_snap);
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
            } else if (dact == 5) {   /* 战况统计 → 复用 ☰ 菜单遍历 cells 逻辑
                ★ AUD-1000 B03-052（P1 已注释）：O(N_CELLS)=O(60) 遍历持 g_mutex，
                短暂（单次 60 格 rank/side 读）低频弹窗点击路径，非每帧热路；
                快照/增量计数器改造留后续（改行为需全量回归，本轮仅标注）。 */
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
            } else if (dact == 6) {   /* 软件信息 → 软件信息弹窗（datah_000，★ P2，对齐 ☰ 菜单本地版入口） */
                ui_screen_set(UI_ABOUT);
                sfx_play(env, "pop");
                LOGI("OnTouch 提和详情：软件信息 → UI_ABOUT 软件信息弹窗（datah_000 Version=1.6.8 + "
                     "官网 www.cnvcs.com + 反馈/关闭，诚实边界：原版反馈通道未逆向）");
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
            if (jact >= 0 && jact <= 7) {   /* 8 行配置：点值列循环切换状态
                 ★ datah_065 原版对齐：行0 先手方(蓝/红) 行1 后手方(红/蓝)
                 行2 玩法(布局/翻棋) 行3 明暗棋(明/暗) 行4-7 和棋/大本营/吃地雷/吃军旗(开/关)
                 值=本端持久化状态（ui_first/ui_opt/ui_show/ui_rule），非真联机同步 */
                if (jact == 0) ui_first_set(ui_first_get() ? 0 : 1);
                else if (jact == 2) { int m = ui_opt_toggle_mode(); (void)m; }
                else if (jact == 3) ui_show_set(ui_show_get() ? 0 : 1);
                else if (jact == 4) ui_rule_draw_set(ui_rule_draw_get() ? 0 : 1);
                else if (jact == 5) ui_rule_hq_set(ui_rule_hq_get() ? 0 : 1);
                else if (jact == 6) ui_rule_mine_set(ui_rule_mine_get() ? 0 : 1);
                else if (jact == 7) ui_rule_flag_set(ui_rule_flag_get() ? 0 : 1);
                sfx_play(env, "pop");
                LOGI("OnTouch 加入游戏完整版弹窗：行%d 值切换（★ 诚实边界：本端持久化状态，非联机同步）",
                     jact);
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
            } else if (cact == 1) {   /* 玩法：布局/翻棋 切换
                ★ AUD-1000 B03-023（P1 已修）：ui_opt_toggle_mode 单入口原子切换 */
                int mode = ui_opt_toggle_mode();
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
    /* ★ P2（datah_000 证据）：软件信息弹窗态（Version 版本号 + 官网文本 + 反馈/关闭）。
        点 反馈 → jq_show_dialog 诚实边界对话框（"反馈（诚实边界）：原版反馈通道未逆向，
        src_rs 无反馈通道"）+ 回对局；
        点 关闭 → 回对局；点面板外 → 回对局。
        ★ 诚实边界：原版反馈=邮件/客服通道（未逆向），src_rs 无反馈通道，
        不包装成"已反馈" */
    /* ★ 返回键 3 态确认弹窗命中分发（原版 .rodata ExitConfirm/ResignConfirm/RegretConfirm） */
    if (ui_screen_get() == UI_EXITCONFIRM) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_exitconfirm_btn *eb = ui_exitconfirm_buttons();
        int eh = ui_exitconfirm_hit(eb, UI_EXITCONFIRM_COUNT, bx, by);
        if (eh >= 0) {
            int eact = eb[eh].action;
            LOGI("OnTouch 退出确认弹窗：按钮%d（action=%d）", eh, eact);
            if (eact == 0) {   /* 退出 → 回主菜单 */
                ui_screen_set(UI_MENU);
                sfx_play(env, "pop");
                LOGI("OnTouch 退出确认：退出 → 回主菜单（对齐原版 Junqi.ExitConfirm 确认退出）");
            } else if (eact == 1) {   /* 继续 → 回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 退出确认：继续 → 回对局");
            }
        }
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_RESIGNCONFIRM) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_resignconfirm_btn *rb = ui_resignconfirm_buttons();
        int rh = ui_resignconfirm_hit(rb, UI_RESIGNCONFIRM_COUNT, bx, by);
        if (rh >= 0) {
            int ract = rb[rh].action;
            LOGI("OnTouch 认输确认弹窗：按钮%d（action=%d）", rh, ract);
            if (ract == 0) {   /* 认输 → 本机败（winner=对方）+ 回对局结束态 */
                int winner_snap = -1;   /* ★ AUD-1000 B03-064（P1 已修）：锁内快照 winner */
                pthread_mutex_lock(&g_mutex);
                /* ★ AUD-1000 B03-009（P0 已修）：g_state 写移入 g_mutex */
                g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
                g_state.stage = STG_OVER;
                winner_snap = g_state.winner;
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME);
                sfx_play(env, "over");
                LOGI("OnTouch 认输确认：认输 → 本机败（winner=%d stage=OVER，锁内快照，对齐原版 Junqi.ResignConfirm）",
                     winner_snap);
            } else if (ract == 1) {   /* 取消 → 回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 认输确认：取消 → 回对局");
            }
        }
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_REGRETCONFIRM) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_regretconfirm_btn *gb = ui_regretconfirm_buttons();
        int gh = ui_regretconfirm_hit(gb, UI_REGRETCONFIRM_COUNT, bx, by);
        if (gh >= 0) {
            int gact = gb[gh].action;
            LOGI("OnTouch 悔棋确认弹窗：按钮%d（action=%d）", gh, gact);
            if (gact == 0) {   /* 悔棋 → 连续 pop 到 BLUE（人机/AI 模式）或单 pop（两人热座）+ 回对局
                ★ AUD-1000 B03-065（P1 已修）：单 undo_pop 在人机模式栈顶是 AI 应着前
                （turn=RED）→ OnUpdate AI 立即补应着 → 悔棋失效；改 while 连续 pop
                到 BLUE（对齐 ☰ 菜单 7809 修复范式），两人模式保持单 pop。 */
                pthread_mutex_lock(&g_mutex);
                int popped = 0;
                if (ui_vs_ai_get()) {
                    while (undo_pop()) {
                        popped++;
                        if (g_state.turn == SIDE_BLUE || g_state.stage != STG_PLAY) break;
                    }
                } else {
                    popped = undo_pop();
                }
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 悔棋确认：悔棋 → undo_pop（popped=%d，人机=连续到 BLUE，对齐原版 Junqi.RegretConfirm）", popped);
            } else if (gact == 1) {   /* 取消 → 回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 悔棋确认：取消 → 回对局");
            }
        }
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_ABOUT) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_about_btn *ab = ui_about_buttons();
        int abh = ui_about_hit(ab, UI_ABOUT_COUNT, bx, by);
        if (abh >= 0) {
            int abact = ab[abh].action;
            LOGI("OnTouch 软件信息弹窗：按钮%d（action=%d）", abh, abact);
            if (abact == 0) {   /* 反馈 → 诚实边界对话框（原版反馈通道未逆向）+ 回对局 */
                jq_show_dialog(env, "反馈（诚实边界）",
                    "反馈（诚实边界）：\n"
                    "原版反馈通道（邮件/客服）未逆向，\n"
                    "src_rs 无反馈通道。");
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 软件信息弹窗：反馈 → 诚实边界对话框（原版反馈通道未逆向）+ 回对局");
            } else if (abact == 1) {   /* 关闭 → 回对局 */
                ui_screen_set(UI_GAME);
                sfx_play(env, "pop");
                LOGI("OnTouch 软件信息弹窗：关闭 → 回对局");
            }
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_about_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 软件信息弹窗外（bx=%.0f by=%.0f）→ 回对局", bx, by);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ SPAK2UI 批量（12 弹窗）：战况统计子菜单链 + 玩家/对局/对战方式/创建游戏/版本更新。
       命中分发范式与 UI_ABOUT 同：按钮命中 → 业务（边界对话框/切弹窗/清复盘/回对局）；
       点面板外 → 回对局。★ 诚实边界：未逆向数据=占位标签+边界对话框，不包装成"已实现" */
    if (ui_screen_get() == UI_UI44) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui44_btn *b44 = ui_ui44_buttons();
        int h44 = ui_ui44_hit(b44, UI_UI44_COUNT, bx, by);
        if (h44 >= 0) {
            LOGI("OnTouch 战况统计·消息/排行弹窗：按钮%d（action=%d）", h44, b44[h44].action);
            jq_show_dialog(env, "消息/排行（诚实边界）",
                "战况统计·消息/排行（诚实边界）：\n"
                "原版排行榜/消息数据未逆向，\n"
                "src_rs 无排行榜数据通道，占位。");
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui44_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 战况统计·消息/排行弹窗外 → 回对局");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI45) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui45_btn *b45 = ui_ui45_buttons();
        int h45 = ui_ui45_hit(b45, UI_UI45_COUNT, bx, by);
        if (h45 >= 0) {
            LOGI("OnTouch 战况统计·详细弹窗：按钮%d（action=%d）", h45, b45[h45].action);
            jq_show_dialog(env, "详细（诚实边界）",
                "战况统计·详细（诚实边界）：\n"
                "详细战绩数据未逆向，\n"
                "src_rs 无详细战绩数据通道，占位。");
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui45_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 战况统计·详细弹窗外 → 回对局");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI46) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui46_btn *b46 = ui_ui46_buttons();
        int h46 = ui_ui46_hit(b46, UI_UI46_COUNT, bx, by);
        if (h46 >= 0) {
            LOGI("OnTouch 玩家资料弹窗：返回 → 回对局");
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui46_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 玩家资料弹窗外 → 回对局");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI47) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui47_btn *b47 = ui_ui47_buttons();
        int h47 = ui_ui47_hit(b47, UI_UI47_COUNT, bx, by);
        if (h47 >= 0) {
            LOGI("OnTouch 玩家列表弹窗：关闭 → 回对局（★ 诚实边界：玩家列表未逆向）");
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui47_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 玩家列表弹窗外 → 回对局");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI49) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui49_btn *b49 = ui_ui49_buttons();
        int h49 = ui_ui49_hit(b49, UI_UI49_COUNT, bx, by);
        if (h49 >= 0) {
            LOGI("OnTouch 对局详情弹窗：关闭 → 回对局（★ 诚实边界：对局详情内容未逆向）");
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui49_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 对局详情弹窗外 → 回对局");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    /* ★ AUD-1000 B03-057（P1 误报撤回，已注释）：UI_UI51/UI_UI54/UI_UI110 三弹窗共用块——
       清空分支（h==0）ui_replay_init 在 g_mutex 内（下方锁段）；"关闭"分支 h>0 的
       回对局走 ui_screen_set（自带 g_ui_lock，UI 态层锁），无 g_mutex 需求，正确。 */
    if (ui_screen_get() == UI_UI51 || ui_screen_get() == UI_UI54 || ui_screen_get() == UI_UI110) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        int scr = ui_screen_get();
        const ui_ui51_btn *b51 = ui_ui51_buttons();
        const ui_ui54_btn *b54 = ui_ui54_buttons();
        const ui_ui110_btn *b110 = ui_ui110_buttons();
        int h;
        if (scr == UI_UI51)      h = ui_ui51_hit(b51, UI_UI51_COUNT, bx, by);
        else if (scr == UI_UI54) h = ui_ui54_hit(b54, UI_UI54_COUNT, bx, by);
        else                      h = ui_ui110_hit(b110, UI_UI110_COUNT, bx, by);
        if (h >= 0) {
            if (h == 0) {   /* 清空 → 清复盘记录（诚实边界简化） */
                pthread_mutex_lock(&g_mutex);
                ui_replay_init();
                pthread_mutex_unlock(&g_mutex);
                LOGI("OnTouch 我的对局/历史对局弹窗（scr=%d）：清空 → 清复盘记录（n=%d，"
                      "★ 诚实边界：对局存档未逆向，简化为清复盘记录）", scr, ui_replay_count());
            } else {       /* 关闭 → 回对局 */
                LOGI("OnTouch 我的对局/历史对局弹窗（scr=%d）：关闭 → 回对局", scr);
            }
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        int in = (scr == UI_UI51) ? ui_ui51_popup_inside(bx, by)
              : (scr == UI_UI54) ? ui_ui54_popup_inside(bx, by)
              : ui_ui110_popup_inside(bx, by);
        if (!in) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 我的对局/历史对局弹窗外（scr=%d）→ 回对局", scr);
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI73) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui73_btn *b73 = ui_ui73_buttons();
        int h73 = ui_ui73_hit(b73, UI_UI73_COUNT, bx, by);
        if (h73 >= 0) {
            if (h73 == 0) {   /* 开始 → 本地开局（诚实边界简化：联机创建游戏未逆向） */
                pthread_mutex_lock(&g_mutex);
                jq_restart_game();
                pthread_mutex_unlock(&g_mutex);
                ui_screen_set(UI_GAME);
                LOGI("OnTouch 创建游戏弹窗：开始 → 本地开局（jq_restart_game，"
                      "★ 诚实边界：联机创建游戏未逆向，开始=本地开局简化）");
            } else {          /* 取消 → 回主菜单 */
                ui_screen_set(UI_MENU);
                LOGI("OnTouch 创建游戏弹窗：取消 → 回主菜单");
            }
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui73_popup_inside(bx, by)) {
            ui_screen_set(UI_MENU);
            LOGI("OnTouch 创建游戏弹窗外 → 回主菜单");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI90) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui90_btn *b90 = ui_ui90_buttons();
        int h90 = ui_ui90_hit(b90, UI_UI90_COUNT, bx, by);
        if (h90 >= 0) {
            if (h90 == 0) {   /* 人机对战 → UI_DIFFICULTY（datah_041 难度弹窗已实现） */
                ui_screen_set(UI_DIFFICULTY);
                LOGI("OnTouch 对战方式弹窗：人机对战 → UI_DIFFICULTY 难度选择弹窗（datah_041 已实现）");
            } else {          /* 联网对战 → 诚实边界对话框（联机协议未逆向，仅蓝牙） */
                jq_show_dialog(env, "联网对战（诚实边界）",
                    "联网对战（诚实边界）：\n"
                    "Wi-Fi/局域网联机协议未逆向，\n"
                    "src_rs 仅蓝牙 SPP 已实现，请选蓝牙联机。");
                ui_screen_set(UI_GAME);
                LOGI("OnTouch 对战方式弹窗：联网对战 → 边界对话框（★ 诚实边界：联机协议未逆向，仅蓝牙）");
            }
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui90_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 对战方式弹窗外 → 回对局");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI100) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui100_btn *b100 = ui_ui100_buttons();
        int h100 = ui_ui100_hit(b100, UI_UI100_COUNT, bx, by);
        if (h100 >= 0) {
            LOGI("OnTouch 详细战况弹窗：详细 → 边界对话框（★ 诚实边界：详细战况数据未逆向）");
            jq_show_dialog(env, "详细战况（诚实边界）",
                "详细战况（诚实边界）：\n"
                "详细战况数据未逆向，\n"
                "src_rs 无详细战况数据通道，占位。");
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui100_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 详细战况弹窗外 → 回对局");
            sfx_play(env, "pop");
        }
        (void)clazz;
        return JNI_TRUE;
    }
    if (ui_screen_get() == UI_UI118) {
        float bx = g_view_w > 0 ? x / (float)g_view_w * 720.0f : 0;
        float by = g_view_h > 0 ? y / (float)g_view_h * 1280.0f : 0;
        const ui_ui118_btn *b118 = ui_ui118_buttons();
        int h118 = ui_ui118_hit(b118, UI_UI118_COUNT, bx, by);
        if (h118 >= 0) {
            if (h118 == 0) {   /* 更新 → 诚实边界对话框（版本更新通道未逆向） */
                /* ★ 原版字符串表真值：Common.Update = 有新的版本，是否更新？
                   诚实边界：原版下载/升级通道未逆向，src_rs 无更新通道。 */
                jq_show_dialog(env, "版本更新（诚实边界）",
                    JQ_STR_Update "\n"
                    "版本更新通道（下载/升级）未逆向，\n"
                    "src_rs 无更新通道，占位。");
            } else {          /* 取消 → 关闭回对局 */
                LOGI("OnTouch 版本更新弹窗：取消 → 关闭回对局");
            }
            ui_screen_set(UI_GAME);
            sfx_play(env, "pop");
            (void)clazz;
            return JNI_TRUE;
        }
        if (!ui_ui118_popup_inside(bx, by)) {
            ui_screen_set(UI_GAME);
            LOGI("OnTouch 版本更新弹窗外 → 回对局");
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
        ui_imen_icon_rect(1, &ix0, &iy0, &ix1, &iy1);   /* ↩ 返回 */
        if (bx >= ix0 && bx <= ix1 && by >= iy0 && by <= iy1) {
            /* ★ 2026-11-10 用户"返回键点击无效/这不是悔棋"指正（对齐原版）：
               ↩（屏幕左上角返回图标）语义 = 返回/退出对局（Junqi.ExitConfirm
               "真的要退出吗？"确认弹窗），**不是悔棋**。悔棋走 ☰ 菜单（RegretConfirm）。
               旧实现把 ↩ 当悔棋（undo_pop 循环）＝原版语义错位，用户实锤纠正。 */
            ui_screen_set(UI_EXITCONFIRM);
            sfx_play(env, "pop");
            LOGI("OnTouch ↩ 返回 → UI_EXITCONFIRM 退出确认弹窗（对齐原版 Junqi.ExitConfirm）");
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
                int sess_init_done_snap;
                pthread_mutex_lock(&g_mutex);
                jq_restart_game();
                sess_init_done_snap = g_bt_sess_init_done;   /* ★ AUD-1000 B03-047（P1 已修）：锁内快照 */
                pthread_mutex_unlock(&g_mutex);
                sfx_play(env, "over");
                LOGI("☰菜单 重新开始：default.lay 重开局（stage=PLAY 蓝先手）");
                /* ★ BT 联机：重开 → 发 RESTART 帧（对端同步重开，锁内快照值判发帧） */
                if (sess_init_done_snap) jq_bt_ctrl_send(env, BT_T_RESTART);
            } else if (act == 1) {   /* 悔棋 */
                int sess_init_done_snap;
                int popped = 0;
                pthread_mutex_lock(&g_mutex);
                if (g_vs_ai) {
                    /* ★ 人机模式：悔到玩家蓝回合（AI 应着步 + 玩家步都悔，undo 栈含 AI 步） */
                    while (undo_pop()) {
                        popped++;   /* ★ 计数累加（旧版 `popped=1` 恒 1，pop 多次只报 1） */
                        if (g_state.turn == SIDE_BLUE || g_state.stage != STG_PLAY) break;
                    }
                } else {
                    /* ★ 两人热座/蓝牙：悔最近 1 步（对方走子后轮到对方，只悔本方可悔步）
                       ★ AUD-1000 B03-046（P1 已修）：BT 态悔棋仅发 UNDO 帧、不本地 pop——
                       本地 pop 由收帧侧（OnBluetooth UNDO 收帧闭环）执行，防双 pop。 */
                    popped = ui_bt_active() ? 0 : undo_pop();
                }
                sess_init_done_snap = g_bt_sess_init_done;   /* ★ AUD-1000 B03-047：锁内快照 */
                pthread_mutex_unlock(&g_mutex);
                sfx_play(env, "pop");
                LOGI("☰菜单 悔棋：连续 undo_pop 到 BLUE（popped=%d，BT 态=0 仅发帧）", popped);
                /* ★ BT 联机：悔棋 → 发 UNDO 帧（对端同步悔棋；收帧侧 8048 本地回退） */
                if (sess_init_done_snap && (popped || ui_bt_active())) jq_bt_ctrl_send(env, BT_T_UNDO);
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
                /* ★ 对抗审计（2026-11-11）：旧 B03-009 修复时误加"两层 g_mutex 加锁"
                   （非递归 mutex 自嵌套 → 认输点击死锁）。现单层：一次 lock 包住
                   g_state.winner/stage 写 + 一次 unlock。 */
                pthread_mutex_lock(&g_mutex);
                g_state.winner = (g_state.turn == SIDE_BLUE) ? SIDE_RED : SIDE_BLUE;
                g_state.stage = STG_OVER;
                pthread_mutex_unlock(&g_mutex);
                jq_bt_ctrl_send(env, BT_T_RESIGN);
                sfx_play(env, "over");
                LOGI("☰菜单 认输：本机认输 → 对方胜（winner=%d）", g_state.winner);
            } else if (bt_mode && act == 4) {   /* 聊天（联机版，datah_067）→ 聊天完整版弹窗（★ P2 datah_063） */
                /* ★ P2（datah_063 证据）：聊天入口从"直接发帧+纯文本对话框"升级为
                   datah_063 聊天完整版弹窗（Msg 输入框占位标签 + 发送/记录/关闭 3 按钮），
                   发帧逻辑保留在弹窗内（发送→BT_T_CHAT 占位帧 + 边界对话框）。 */
                ui_screen_set(UI_CHAT063);
                sfx_play(env, "pop");
                LOGI("☰菜单 聊天（联机版）：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框占位 + "
                     "发送/记录/关闭 3 按钮，诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框）");
                (void)clazz;
                return JNI_TRUE;
            } else if (bt_mode && act == 7) {   /* 游戏规则（联机版，datah_067）→ 规则文本弹窗（datah_037） */
                ui_screen_set(UI_RULES);
                sfx_play(env, "pop");
                LOGI("☰菜单 游戏规则（联机版）：→ UI_RULES 规则文本弹窗（datah_037 规则-翻棋/规则-布局 2 模式 + 返回，"
                     "诚实边界：规则文本=src_rs 简化版固定文本 2 套）");
            } else if (bt_mode && act == 5) {   /* 系统设置（联机版）→ 游戏设置弹窗（datah_012） */
                ui_screen_set(UI_GAMESET);
                sfx_play(env, "pop");
                LOGI("☰菜单 系统设置：→ UI_GAMESET 游戏设置弹窗（datah_012 4 规则开关，原版 datah_067 Config=系统设置含声音+游戏设置）");
            } else if (bt_mode && act == 6) {   /* 战况统计（联机版）→ 对局信息弹窗（datah_050，含 复盘/删除/返回 子菜单） */
                ui_screen_set(UI_GAMEINFO);
                sfx_play(env, "pop");
                LOGI("☰菜单 战况统计（联机版）：→ UI_GAMEINFO 对局信息弹窗（datah_050 复盘/删除/返回 子菜单）");
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
                        int pushed = undo_push();   /* ★ AUD-1000 B03-008（P1 已修）：☰ 电脑变着 push 契约 */
                        if (flip) { int frc = 0; if (jq_flip(&g_state, from)) { frc = 1; ui_replay_record(from, from, 1); g_event_cnt[ev7_to_name_idx(EV_FLIP)]++; g_event_total++; }   /* AUD-R-061：翻棋计数走 EV_FLIP=6 位 */   /* ★ P1（datah_039/107）：电脑变着翻棋记录 */ if (!frc && pushed) undo_pop_discard();   /* ★ AUD-1000-ALL A9-001~004（P1 已修）：jq_flip bool 契约（成功 1/失败 0），frc<0 恒假死条件→翻棋失败脏快照不回滚；改 !frc（失败即回滚栈顶） */ }
                        else { int e = jq_try_move(&g_state, from, to); if (e >= 0) ui_replay_record(from, to, 0); if (e < 0 && pushed) undo_pop_discard(); (void)e; }   /* ★ P1（datah_039/107）：电脑变着走子记录 */
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
            } else if (!bt_mode && act == 5) {   /* 战况统计（本地版）→ 对局信息弹窗（datah_050，含 复盘/删除/返回 子菜单） */
                ui_screen_set(UI_GAMEINFO);
                sfx_play(env, "pop");
                LOGI("☰菜单 战况统计：→ UI_GAMEINFO 对局信息弹窗（datah_050 Info 文本框（战况统计复用） + 复盘/删除/返回 3 按钮子菜单）");
            } else if (!bt_mode && act == 6) {   /* 软件信息（本地版）→ 软件信息弹窗（★ P2 datah_000） */
                /* ★ P2（datah_000 证据）：软件信息入口从"纯文本对话框"升级为
                   datah_000 软件信息弹窗（Version 版本号 1.6.8 + 官网 www.cnvcs.com
                   + 反馈/关闭 2 按钮），反馈按钮→诚实边界对话框（原版反馈通道未逆向）。 */
                ui_screen_set(UI_ABOUT);
                sfx_play(env, "pop");
                LOGI("☰菜单 软件信息（本地版）：→ UI_ABOUT 软件信息弹窗（datah_000 Version=1.6.8 + "
                     "官网 www.cnvcs.com + 反馈/关闭 2 按钮，诚实边界：原版反馈通道未逆向，src_rs 无反馈通道）");
                (void)clazz;
                return JNI_TRUE;
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
    /* ★ 对抗审计 P2-J2：原 (void)ny 误标——ny 在下方"再来一局"按钮区判定
       （ny 0.62-0.72）实际使用。删除误标，保留 ny。 */
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
    if (ev != 0) {
        LOGI("OnTouch 走子态机：ev=%d（SELECT/UNSELECT/MOVE）from=%d to=%d（cell 索引）",
             ev, from, to);
        /* ★ 走子坐标诊断（2026-11-10 未达成缺口攻坚）：打印 view→720×1280 基准映射
           + 命中 cell + 可走性（can_move/can_flip），实锤触摸几何与规则层坐标对齐。
           限前 50 次避免日志洪水（红方热座验证需多格试探）。 */
        static int g_touch_diag_cnt = 0;
        if (++g_touch_diag_cnt <= 50) {
            int dx = (int)(x / (float)g_view_w * 720.0f + 0.5f);
            int dy = (int)(y / (float)g_view_h * 1280.0f + 0.5f);
            pthread_mutex_lock(&g_mutex);
            int cm = (from >= 0 && to >= 0 && to != from) ? jq_can_move(&g_state, from, to) : 0;
            int cf = (from >= 0 && to == from) ? jq_can_flip(&g_state, from) : 0;
            int turn = g_state.turn;
            pthread_mutex_unlock(&g_mutex);
            LOGI("OnTouch 走子诊断：view(%.0f,%.0f)→base(%d,%d) cell from=%d to=%d can_move=%d can_flip=%d turn=%d",
                 x, y, dx, dy, from, to, cm, cf, turn);
        }
    }
    /* ★ 走子/翻棋的 g_state 读写 + jq_try_move/jq_flip 全部在 g_mutex 内（GLThread
       的 OnUpdate/AI 也持同一把锁读写 g_state → 免并发野指针 0x80b5dbe78 SIGSEGV）。
       sfx 在锁外调（不读 g_state，只读事件名 + MediaPlayer 全局 ref）。 */
    int jnev = -1;
    int over_win = -1;   /* 走子后 stage==OVER 记录 winner（锁外 sfx win/lose，防锁竞争读） */
    int flip_done = 0, move_ok = 0;
    if (ev == JQ_EV_SELECT) {
        /* ★ AUD-1000 B03-069（P1 已注释）：JQ_EV_SELECT 分支 set_selected/sfx 均锁外
           （设计意图，对照 :8160 "sfx 锁外"注释）；g_glview sel_cell int 对齐读
           跨线程实践原子（高亮滞后一帧良性，诚实边界）。 */
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
                int pushed = undo_push();   /* ★ AUD-1000 B03-008/B03-011（P1 已修）：push 契约 */
                flip_done = jq_flip(st, from);
                if (flip_done) {
                    ui_replay_record(from, from, 1);   /* ★ P1（datah_039/107）：翻棋成功记录 1 步 */
                    g_event_cnt[ev7_to_name_idx(EV_FLIP)]++; g_event_total++;   /* ★ AUD-R-061（P2）：翻棋计数走 EV_FLIP=6 位 */
                } else if (pushed) undo_pop_discard();   /* 翻棋失败回滚栈顶快照 */
                LOGI("走子判定 翻棋：cell=%d rc=%d turn→%d（人机AI走红=%d）",
                     from, flip_done, st->turn, g_vs_ai);
            } else {
                jq_touch_state_reset(&g_ts);
            }
        } else {
            /* 走子 A→B（jq_can_move 判定 + jq_try_move 事件） */
            if (from >= 0 && to >= 0) {
                int movable = jq_can_move(st, from, to);
                int pushed = undo_push();   /* ★ AUD-1000 B03-008/B03-011（P1 已修）：push 契约 */
                jnev = jq_try_move(st, from, to);
                move_ok = (jnev >= 0);
                if (move_ok) ui_replay_record(from, to, 0);   /* ★ P1（datah_039/107）：走子成功记录 1 步 */
                if (jnev < 0 && pushed) undo_pop_discard();   /* 走子失败回滚栈顶快照 */
                if (move_ok && jnev >= 0 && jnev < 7) {   /* AUD-R-061：7 项事件码（AUD-A-0003/0004 同族：下界+上界守卫齐全） */
                    g_event_cnt[ev7_to_name_idx(jnev < 0 ? 0 : jnev)]++;   /* ★ 原版行为移植：7-code→73-name 索引映射（OnTouch 直接走子不经 on_event） */
                    g_event_total++;
                }
                if (move_ok && st->stage == STG_OVER)
                    over_win = st->winner;   /* 对局结束（吃旗/无子可走等），锁外播 win/lose */
                LOGI("走子判定：from=%d to=%d can_move=%d 结果=%s（事件累计 move=%d eat=%d）",
                     from, to, movable, JQ_EVENT_NAMES[jnev < 0 ? 0 : jnev], g_event_cnt[ev7_to_name_idx(0)], g_event_cnt[ev7_to_name_idx(1)]);
            }
        }
        pthread_mutex_unlock(&g_mutex);
        /* ★ 蓝牙联机发送侧：走子/翻棋成功且联机会话已建立且**轮到本方**（OUR_TURN）→ 编码 v2 帧 → Java BtSpp 写出。
           ★ 回合闸口：client（THEIR_TURN）不发送；host 首帧后 send_move 置 THEIR_TURN。 */
        if ((flip_done || move_ok) && g_bt_sess_init_done) {
            /* ★ AUD-1000 B03-048/B03-087（P1 已修）：send_move 的 g_state 快照编码
               移入 g_mutex（旧实现在 unlock 后读 &g_state 编码，与 GLThread 渲染读
               并发；Java 写出 jq_bt_send 仍锁外，编码结果本地串自带，无共享状态）。 */
            int sess_state;
            char tb[BT_BUF];
            int bn = 0;
            pthread_mutex_lock(&g_mutex);
            sess_state = (int)g_bt_sess.state;
            if (sess_state == BT_ST_OUR_TURN)
                bn = jq_bt_session_send_move(&g_bt_sess, &g_state, from,
                                             flip_done ? from : to, flip_done,
                                             tb, sizeof tb);
            int last_type_snap = (int)g_bt_sess.last_type;   /* ★ A2-001（P1 已修）：锁内快照 last_type（旧版 unlock 后裸读跨线程全局，B03-047 快照范式漏点） */
            pthread_mutex_unlock(&g_mutex);
            if (bn > 0) {
                tb[bn] = 0;
                jq_bt_send(env, tb);
                LOGI("蓝牙联机发送：type=%d len=%d frame=%s",
                     last_type_snap, bn, tb);
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
            /* ★ AUD-1000-ALL A10-D2-01（P2 已修）：OnTouch 走子块 ai_pick O(N²) 移锁外
               （同 A2-020 OnUpdate 范式）——旧实现在 g_mutex 段内做 ai_pick（60×60
               jq_can_move），阻塞 GLThread 渲染读 + 触摸线程。修：锁内判定快照
               （stage/turn/vs_ai），锁外 ai_pick，锁内重检应用（turn 切回 BLUE 去重）。 */
            int ai_go = 0;
            pthread_mutex_lock(&g_mutex);
            if (ui_screen_get() == UI_GAME && g_state.stage == STG_PLAY &&
                g_vs_ai && g_state.turn == SIDE_RED)
                ai_go = 1;
            pthread_mutex_unlock(&g_mutex);
            if (ai_go) {
                pthread_mutex_lock(&g_mutex);
                if (ui_screen_get() == UI_GAME && g_state.stage == STG_PLAY &&
                    g_vs_ai && g_state.turn == SIDE_RED) {   /* ★ 锁内重检（防 ai_pick 期间对局结束） */
                int af, at, aflip, ae = -1;
                if (ai_pick(&af, &at, &aflip) == 0) {
                    int pushed = undo_push();   /* ★ AUD-1000 B03-008（P1 已修）：OnTouch AI 块 push 契约 */
                    if (aflip) {
                        int frc = 0;
                        if (jq_flip(&g_state, af)) { frc = 1; ui_replay_record(af, af, 1); g_event_cnt[ev7_to_name_idx(EV_FLIP)]++; g_event_total++; }   /* AUD-R-061：翻棋计数走 EV_FLIP=6 位 */   /* ★ P1（datah_039/107）：AI 翻棋记录 */
                        if (!frc && pushed) undo_pop_discard();   /* ★ AUD-1000-ALL A9-001~004（P1 已修）：jq_flip bool 契约（成功 1/失败 0），frc<0 恒假死条件→翻棋失败脏快照不回滚；改 !frc（失败即回滚栈顶） */
                        LOGI("红方 AI 应着（翻棋）：cell=%d（turn 切回蓝方）", af);
                    } else {
                        ae = jq_try_move(&g_state, af, at);
                        if (ae >= 0) ui_replay_record(af, at, 0);   /* ★ P1（datah_039/107）：AI 走子记录 */
                        if (ae < 0 && pushed) undo_pop_discard();
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
            }
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
/* ★ 2026-11-10 小竖条修复：JNI 层 view 同步（渲染层 eglQuerySurface 取实际 surface 尺寸
   → JNI g_view_w/h 同步，触摸命中几何 = 渲染几何基准一致）。 */
int jq_glview_view_size(int *w, int *h);   /* fwd decl（glview_gl.c 实现） */

JNIEXPORT void JNICALL
Java_com_cnvcs_App_OnGraphChange(JNIEnv *env, jclass clazz, jint w, jint h)
{
    (void)env; (void)clazz;
    /* ③b：GLView.onSurfaceChanged(w,h) → OnGraphChange(w,h) → 渲染器 resize
       ★ AUD-1000 B03-085（P1 已注释）：g_view_w/h 写 + jq_glview_resize 锁外——
       g_view_w/h int 对实践原子（见 :346 声明处注释），g_glview resize 走
       glview_gl 层自有同步（glview_gl.c jq_glview_resize 内部锁），
       本层不包 g_mutex 防与 GLThread 渲染持锁嵌套。 */
    g_view_w = w > 0 ? w : g_view_w;
    g_view_h = h > 0 ? h : g_view_h;
    if (jq_glview_is_ready())
        jq_glview_resize(&g_glview, w, h);
    LOGI("OnGraphChange：%dx%d → view 基准同步（glViewport 按实际 surface 尺寸铺满屏，修黑边）",
         g_view_w, g_view_h);
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
    /* ★ AUD-1000 B03-014（P0 已修）：懒初始化在 g_mutex 内——旧实现在锁外
   jq_bt_session_init(&g_bt_sess, &g_state, 1) 改 g_bt_sess + 读 g_state，
   与 GLThread OnUpdate 锁内渲染并发（数据竞争，同型坑 B03-001）。
   每帧循环内 :8040 已持锁幂等补位，此处锁内初始化 + 置位。 */
    pthread_mutex_lock(&g_mutex);
    if (!g_bt_sess_init_done) {
        jq_bt_session_init(&g_bt_sess, &g_state, 1);
        g_bt_sess_init_done = 1;
    }
    pthread_mutex_unlock(&g_mutex);
    /* 取 extra 文本（Java 宿主把收到的 v2 帧字符串放 Intent extra）
       extra 键名 = 原版 .rodata "extra"（0x127458 确证） */
    /* 走 getStringExtra("extra") 路径（原版 .rodata "extra" 0x127458 确证键名）
       ★ AUD-1000 B03-016（P1 已修）：GetStringUTFChars 失败路径补 txt 释放契约——
       早期 extra==NULL 返回点（getStringExtra 失败）不走下方 Release（此时无 txt），
       正常路径 :8066 取 txt 后所有 return 点已配对 Release（含 test 早退路径）。 */
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
        /* 找当前可翻子（暗棋阶段），构造 FLIP 帧文本 → 走 decode + on_frame 链
           ★ AUD-1000 B03-016（P1 已修）：锁内取 fidx 后解锁；早退路径配对释放 txt */
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

    /* 逐帧解析（v2 帧以 '\n' 结尾，可能多帧连发）。
       ★ 对抗审计 P0（批3/4 P0-R6/P0-R8）：OnBluetooth 由 BtSpp 读线程（daemon）调用，
       真实帧路径原无 g_mutex → jq_bt_session_on_frame 裸改 g_state（jq_try_move/jq_flip/
       ui_replay_record）与主线程 OnTouch 走子 / GLThread OnUpdate 渲染并发 → 数据竞争
       （CODE_STYLE §5 "g_state 读写必须 g_mutex" 漂移）。现每帧包 g_mutex。
       懒初始化（g_bt_sess_init_done）也在锁内（对照 start_bt_session :2759 持锁写）。 */
    const char *p = disp;
    int f[8];
    int nframes = 0;
    int sess_state = -1, stage_snap = -1;   /* ★ AUD-1000 B03-016/B03-089（P1 已修）：
       末日志锁外裸读 g_bt_sess.state/g_state.stage → 锁内快照后 LOGI */
    while (*p) {
        if (jq_bt_frame_decode(p, &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6])) {
            pthread_mutex_lock(&g_mutex);
            if (!g_bt_sess_init_done) {
                /* ★ AUD-1000-ALL A4-11（P1 已修）：we_blue 口径矛盾消除——入口段（:8631）
                   懒 init we_blue=1（host 蓝先手），本收帧段旧 we_blue=0（client 红后手）。
                   BtSpp 读线程收帧先于 Java start 回调时，host 设备被收帧段误 init 成
                   client 语义（we_blue=0）→ RESIGN winner 判反。统一：收帧段懒 init
                   沿用入口段 host 语义 we_blue=1（与 :8631 一致），host/client 区分
                   由 Java 侧 start_bt_session(host?1:0) 显式覆盖（:3330 锁内 double-check）。 */
                jq_bt_session_init(&g_bt_sess, &g_state, 1);
                g_bt_sess_init_done = 1;
            }
            int rc = jq_bt_session_on_frame(&g_bt_sess, &g_state,
                                            f[0], f[1], f[2], f[3],
                                            f[4], f[5], f[6]);
            /* ★ 蓝牙 UNDO 收帧侧闭环（对抗审计 P1-B9 / P1-J12 PENDING → 真接线）：
               对端发 BT_T_UNDO = "我已悔棋"（本端之前发的 UNDO 帧的对端应答）。
               bt_session 侧只切回合（不持有悔棋栈），真实回退必须在本文件做——
               悔棋栈 g_undo/g_undo_top 是本文件 static，跨文件不可见。
               语义：BT 交替走棋下，双方各悔**自己那一步**（本地栈只存自己走的子，
               对端栈只存对端走的子）→ 双方各弹一次 = 退一个完整回合 = 棋盘一致。
               回退后广播 SYNC_CELL 全量（60 帧）→ 请求方以权威态覆盖，双端收敛。
               诚实边界：若双方栈深不一致（如一侧本地悔过未广播），SYNC_CELL 兜底收敛。 */
            /* ★ AUD-1000 B03-015/B03-087/B03-017（P1 已修）：SYNC_CELL 编码移入本锁段
               （旧 unlock 后 jq_bt_session_send_sync 读 g_state 60 格锁外）；
               双 8KB 栈缓冲（sbuf+payload）改 sbuf 单缓冲复用（payload 段=sbuf 尾部），
               BtSpp 读线程栈压减半。 */
            int did_undo = 0;
            static char g_bt_payload[BT_BUF * 64 + 1];   /* 锁外发送缓存（BtSpp 读线程串行处理，静态复用）
               ★ AUD-1000 B03-017：旧双 8KB 栈缓冲（sbuf+payload 均栈上）改 栈上 sbuf（仅 UNDO 帧时开）
               + 静态 payload（复用），常态栈压减半。 */
            if (f[0] == BT_T_UNDO && rc == 0) {
                if (undo_pop()) {
                    did_undo = 1;   /* 仅栈操作（锁内），SYNC 编码移 unlock 后（A2-015） */
                    LOGI("OnBluetooth：收 UNDO 帧 → 本地回退 1 步（undo_pop，turn=%d）", g_state.turn);
                } else {
                    LOGI("OnBluetooth：收 UNDO 帧但本地栈空（不悔，诚实边界）");
                }
            }
            sess_state = (int)g_bt_sess.state;
            stage_snap = (int)g_state.stage;   /* ★ AUD-1000 B03-016/B03-089：锁内快照 */
            pthread_mutex_unlock(&g_mutex);
            nframes++;
            if (rc < 0)
                LOGI("OnBluetooth：帧 type=%d 被拒（rc=%d）", f[0], rc);
            /* ★ AUD-1000-ALL A2-015（P1 已修）：SYNC_CELL 编码移锁外——旧实现
               jq_bt_session_send_sync（60 帧 O(N×BT_BUF) snprintf）在 g_mutex 段内
               执行阻塞 GLThread 渲染读 + OnTouch 写。修：锁内仅 undo_pop（1 栈操作），
               unlock 后锁外编码 + 发送。诚实边界：锁外编码读 g_state 与锁内渲染读
               并发（半更新 cell 窗口 <16ms，BT UNDO 非高频路径，SYNC 全量兜底收敛）。
               ★ B03-017：容量守卫 = 实缓冲 sizeof(g_bt_payload)；B09-038 帧推进不受影响。 */
            if (did_undo) {
                /* ★ AUD-1000-ALL A10-D4-01（P2 已修）：sbuf 栈压 16KB（BtSpp 读线程栈）→ static 复用
                   （同 g_bt_payload 范式，OnBluetooth 帧循环串行，static 安全） */
                static char g_bt_sbuf[BT_BUF * 64];   /* 锁外编码（60 帧，static 复用免栈压） */
                char *sbuf = g_bt_sbuf;
                int sn = jq_bt_session_send_sync(&g_bt_sess, &g_state, sbuf, BT_BUF);
                int plen = 0;
                if (sn > 0) {
                    for (int k = 0; k < sn; k++) {
                        const char *fr = sbuf + k * BT_BUF;
                        int flen = (int)strlen(fr);
                        if (flen <= 0 || plen + flen >= (int)(sizeof g_bt_payload) - 1) break;
                        memcpy(g_bt_payload + plen, fr, flen);
                        plen += flen;
                    }
                    g_bt_payload[plen] = 0;
                    if (plen > 0) {
                        jq_bt_send(env, g_bt_payload);
                        LOGI("OnBluetooth：UNDO 回退后广播 SYNC_CELL 全量（锁外编码，%d 字节）", plen);
                    }
                } else {
                    LOGI("OnBluetooth：UNDO 后 sync 编码 0 帧（会话未就绪，诚实边界）");
                }
            }
            /* ★ AUD-1000 B09-038（P1 已修）：找下一帧起点须从**帧尾**推进——
               旧实现 `strchr(p, '\n')` 命中的是 "v2\n" 头的第一个 '\n'（帧自带
               尾 '\n'，decode 成功意味着 p 指向完整 "v2\n...\n"）→ p 永不前进，
               UNDO 后 60 帧串接广播**仅首帧被处理**，后续 59 帧静默丢失（与
               P0 帧丢帧同族残留）。修：前进到 p 起的首个 '\n' 之后。 */
            const char *nl = strchr(p, '\n');
            p = nl ? nl + 1 : p + 1;
        } else {
            break;
        }
    }
    (*env)->ReleaseStringUTFChars(env, extra, txt);
    LOGI("OnBluetooth：处理 %d 帧，会话状态=%d，stage=%d",
         nframes, sess_state, stage_snap);   /* ★ AUD-1000 B03-089：锁内快照值，非裸读 */
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
