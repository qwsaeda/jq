#ifndef RS_RENDER_H
#define RS_RENDER_H
/*
 * render.h — 最小出帧渲染器（①+②：GL 状态机 + 纹理管线）
 * 依据原版：
 *   - GL 导入 35 函数（.dynsym：glClear/glBegin/glVertex/glDrawArrays/glTexImage2D...）
 * ★ AUD-A2 F-gl: 排障时可临时开 JQ_GL_ERR_CHECK 宏在 glDrawArrays 后 glGetError（默认关，性能敏感路径不加运行时检查）
 *   - 渲染入口 0xbd764（OnGraphCreate 桩 bl）/ 0xbd5a0（OnUpdate）/ 0xbdd2c（OnTouch）
 *   - 棋盘纹理 SPAK datah#188（737×751，十字棋盘 PNG，L44/L75）
 *   - 棋子图集 SPAK datah#183（212×280，4列×7行 53×40/格，rank→列/行，L75 已确证）
 * 诚实边界：EGL/渲染对象图为重写（非原版 C++ 场景对象图，③④ 未逆）；
 *   纹理/几何对齐原版 SPAK 资源，出帧后视觉验收需真机 GPU。
 */
#include "junqi.h"
#include "ui_state.h"
#include <stdint.h>
#include <stddef.h>
struct AAssetManager;
typedef struct AAssetManager AAssetManager;

#ifndef RS_NO_GL
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
typedef GLuint rs_glid_t;
typedef EGLDisplay rs_egl_disp_t;
typedef EGLSurface rs_egl_surf_t;
typedef EGLContext rs_egl_ctx_t;
#else
typedef uint32_t rs_glid_t;
typedef int rs_egl_disp_t;
typedef int rs_egl_surf_t;
typedef int rs_egl_ctx_t;
#endif

#define N_ROWS 12
#define N_COLS 5

typedef struct {
    const jq_state *game;   /* 指向 JNI 侧全局对局状态（非快照）：走子/翻棋后渲染自动更新 */
    rs_egl_disp_t egl_disp;
    rs_egl_surf_t egl_surf;
    rs_egl_ctx_t  egl_ctx;
    int view_w, view_h;            /* 渲染尺寸（OnGraphChange 更新） */
    /* ★ 双 slot GL 对象（slot0=PBuffer context，slot1=GLThread/EglHelper window context；
       GL 对象 context 私有，各自建一套，避免跨 context 重建崩溃） */
    rs_glid_t tex_board[2];     /* 棋盘 737×751（GL 纹理，每 context 一套） */
    rs_glid_t tex_piece[2];     /* 棋子图集 212×280（GL 纹理，每 context 一套） */
    rs_glid_t tex_text[2];      /* 菜单按钮文字图集 136×214（原版字体 data_029 光栅化） */
    rs_glid_t tex_bg[2];        /* 对局棋盘背景 datax_177 720×1280（原版完整棋盘背景） */
    rs_glid_t tex_menu_bg[2];   /* ★ 原版 UI 对齐：主菜单背景 datax_294 800×1205（卡通士兵） */        rs_glid_t tex_dlg_text[2];   /* ★ 批量对齐：共用按钮文字图集（dlg_batch_text.png） */
    void *dlg_text_px; size_t dlg_text_px_len; int dlg_text_w, dlg_text_h;
    rs_glid_t tex_imen_text[2]; /* ★ ☰ 菜单文字图集 160×320（8 项竖排，data_029 字体） */
    rs_glid_t tex_imen_icons[2];/* ★ ☰ 菜单图标图集 440×200（8 图标 110×100 4×2，原版裁剪） */
    rs_glid_t tex_imen_orig[2]; /* ★ 原版 datax 图标真值图集 384×384（16 图标 4×4，每格 96） */
    rs_glid_t tex_hamburger[2]; /* ★ ☰/↩ 图标图集 128×64（orig_human 裁剪） */
    rs_glid_t tex_btn_big[2];   /* ★ 主菜单大按钮图集 802×118（人机+两人，orig_menu 裁剪） */
    rs_glid_t tex_btn_small[2]; /* ★ 主菜单小按钮图集 666×116（联网+联机+四国，orig_menu 裁剪） */
    rs_glid_t tex_popup[2];     /* ★ 选项 UI：popup.png 24×24 9-patch(8,8,16,16) 弹窗背景 */
    rs_glid_t tex_opt_text[2];  /* ★ 选项 UI：弹窗文字图集 109×258（6 标签竖排） */
    rs_glid_t tex_waysel_text[2]; /* ★ 对抗审计（datah_072）：方式选择文字图集 220×184（4 标签竖排） */
    rs_glid_t tex_btimen_text[2]; /* ★ 对抗审计（datah_067）：联机 ☰ 菜单文字图集 160×320（8 项竖排） */
    rs_glid_t tex_settings_text[2]; /* ★ 对抗审计（datah_001）：声音设置文字图集 200×460（10 标签竖排） */
    rs_glid_t tex_nplayer_text[2];  /* ★ P0（datah_089）：四国人数选择文字图集 220×138（3 标签竖排） */
    rs_glid_t tex_newgame_text[2];  /* ★ P0（datah_062）：两人新局文字图集 220×276（6 标签竖排） */
    rs_glid_t tex_gameset_text[2];  /* ★ P1（datah_012）：游戏设置文字图集 220×368（8 标签竖排） */
    rs_glid_t tex_layres_text[2];   /* ★ P1（datah_016/017/020）：布局存档文字图集 220×414（9 标签竖排：
                                          载入布局/保存布局/布局管理/取消/保存/改名/删除/关闭/载入） */
    rs_glid_t tex_replay_text[2];   /* ★ P1（datah_039/107）：复盘文字图集 220×368（8 标签竖排：
                                          复盘/菜单/回退/前进/跳转/返回/第N步/结束） */
    rs_glid_t tex_login_text[2];  /* ★ P1（datah_038）：登录文字图集 220×368（8 标签竖排：
                                           登录/用户名/密码/注册/游客/返回/账户/边界） */
    rs_glid_t tex_rules_text[2];  /* ★ P2（datah_037）：规则文本文字图集 220×368（8 标签竖排：
                                           规则/翻棋/布局/工兵/地雷/炸弹/夺旗/边界） */
    rs_glid_t tex_ng104_text[2];  /* ★ P2（datah_104）：新局完整版文字图集 220×460（10 标签竖排：
                                           新局/我方/电脑/玩法/蓝方/红方/新手/中级/高级/确定） */
    rs_glid_t tex_join_text[2];   /* ★ P2（datah_034/035）：加入/创建游戏文字图集 220×460
                                           （10 标签竖排：加入/创建/桌号/密码/确定/取消/
                                           蓝牙/边界/联机/Wi-Fi） */
    rs_glid_t tex_ng061_text[2];  /* ★ P2（datah_061）：新局完整版（完整版补充）文字图集 220×460
                                           （10 标签竖排：新局/先后手/电脑/玩法/蓝/红/新手/
                                           中级/高级/确定） */
    rs_glid_t tex_gameinfo_text[2]; /* ★ P2（datah_050）：对局信息文字图集 220×368（8 标签竖排：
                                           信息/复盘/删除/返回/蓝方/红方/步数/回合） */
    rs_glid_t tex_bright_text[2];  /* ★ P2（datah_002）：亮度调节文字图集 220×460（10 标签竖排：
                                           亮度/滑条/低/中/高/确定/取消/调节/档位/保存） */
    rs_glid_t tex_chatlog_text[2]; /* ★ P2（datah_029）：聊天记录文字图集 220×368（8 标签竖排：
                                           记录/聊天/清除/关闭/占位/帧/边界/未逆向） */
    rs_glid_t tex_profile_text[2]; /* ★ P2（datah_031）：个人资料文字图集 220×460（10 标签竖排：
                                           资料/修改/帮助/返回/账号/昵称/头像/胜率/边界/未逆向） */
    rs_glid_t tex_drawdetail_text[2]; /* ★ P2（datah_042）：提和详情文字图集 220×322（7 标签竖排：
                                           提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息） */
    rs_glid_t tex_setentry_text[2]; /* ★ P2（datah_003/004）：设置入口文字图集 220×322
                                           （7 标签竖排：设置入口/图像/声音/游戏/简化/取消/返回） */
    rs_glid_t tex_imgset_text[2];   /* ★ P2（datah_007/008）：图像设置文字图集 220×460
                                           （10 标签竖排：图像/界面/棋子/夜间/对方/开/关/
                                           确定/取消/设置） */
    rs_glid_t tex_join065_text[2];  /* ★ P2（datah_065）：加入游戏完整版文字图集 220×460
                                           （10 标签竖排：加入/先手/后手/玩法/明暗/和棋/
                                           大本营/吃雷/吃旗/开始） */
    rs_glid_t tex_create069_text[2];/* ★ P2（datah_069）：创建游戏完整版文字图集 220×460
                                           （10 标签竖排：创建/先后/玩法/明暗/规则/
                                           蓝方/红方/布局/翻棋/确定） */
    rs_glid_t tex_rules070_text[2]; /* ★ P2（datah_070）：游戏规则完整版文字图集 220×460
                                           （10 标签竖排：规则/和棋/大本营/吃雷/吃旗/
                                           开/关/确定/取消/设置） */
    rs_glid_t tex_room060_text[2];  /* ★ P2（datah_060）：房间态文字图集 220×368
                                           （8 标签竖排：房间/玩家/蓝方/红方/开始/换桌/
                                           离开/边界） */
    rs_glid_t tex_chat063_text[2];  /* ★ P2（datah_063）：聊天完整版文字图集 220×368
                                           （8 标签竖排：聊天/输入/发送/记录/关闭/
                                           占位/边界/未逆向） */
    rs_glid_t tex_about_text[2];   /* ★ P2（datah_000）：软件信息文字图集 220×322
                                           （7 标签竖排：软件信息/版本/官网/反馈/关闭/
                                           1.6.8/cnvcs.com） */
    void *board_px; size_t board_px_len;   /* 解码后 RGBA（跨 context 共享，只解码一次） */
    void *piece_px;  size_t piece_px_len;
    void *text_px;   size_t text_px_len;   /* 菜单文字图集 RGBA（5 label 竖排） */
    int text_w, text_h;                    /* 文字图集尺寸 */
    void *bg_px;     size_t bg_px_len;     /* 对局棋盘背景 datax_177 RGBA（720×1280） */
    int bg_w, bg_h;                        /* 背景尺寸 */
    void *menu_bg_px; size_t menu_bg_px_len;   /* ★ 主菜单背景 datax_294 RGBA（800×1205） */
    int menu_bg_w, menu_bg_h;                  /* 主菜单背景尺寸 */
    void *popup_px;  size_t popup_px_len;  /* ★ 弹窗背景 popup.png RGBA（24×24 9-patch） */
    int popup_w, popup_h;                  /* 弹窗背景尺寸 */
    /* ★ 原版 UI 位图真值（ch02 N45 门牌号→房间真值）：面板 9patch + 彩色按钮 + 图标 */
    void *panel_px;  size_t panel_px_len;  int panel_w, panel_h;         /* 面板 9patch（datax_246 圆形） */
    void *panel_btn_px; size_t panel_btn_px_len; int panel_btn_w, panel_btn_h; /* 彩色按钮（datax_194） */
    void *panel_icon_px; size_t panel_icon_px_len; int panel_icon_w, panel_icon_h; /* 图标（datax_138） */
    unsigned int tex_panel[2]; unsigned int tex_panel_btn[2]; unsigned int tex_panel_icon[2];
    void *opt_text_px; size_t opt_text_px_len;  /* ★ 弹窗文字图集 RGBA（6 label 竖排） */
    int opt_text_w, opt_text_h;                 /* 弹窗文字图集尺寸 */
    void *waysel_text_px; size_t waysel_text_px_len; /* ★ 方式选择文字图集 RGBA（4 标签竖排 220×184） */
    int waysel_text_w, waysel_text_h;           /* 方式选择文字图集尺寸 */
    void *btimen_text_px; size_t btimen_text_px_len; /* ★ 联机 ☰ 菜单文字图集 RGBA（8 项竖排 160×320） */
    int btimen_text_w, btimen_text_h;           /* 联机 ☰ 菜单文字图集尺寸 */
    void *settings_text_px; size_t settings_text_px_len; /* ★ 声音设置文字图集 RGBA（10 标签竖排 200×460） */
    int settings_text_w, settings_text_h;       /* 声音设置文字图集尺寸 */
    void *nplayer_text_px; size_t nplayer_text_px_len; /* ★ P0（datah_089）：四国人数选择文字图集 RGBA（3 标签竖排 220×138） */
    int nplayer_text_w, nplayer_text_h;          /* 四国人数选择文字图集尺寸 */
    void *newgame_text_px; size_t newgame_text_px_len; /* ★ P0（datah_062）：两人新局文字图集 RGBA（6 标签竖排 220×276） */
    int newgame_text_w, newgame_text_h;          /* 两人新局文字图集尺寸 */
    void *gameset_text_px; size_t gameset_text_px_len; /* ★ P1（datah_012）：游戏设置文字图集 RGBA（8 标签竖排 220×368） */
    int gameset_text_w, gameset_text_h;          /* 游戏设置文字图集尺寸 */
    void *layres_text_px; size_t layres_text_px_len; /* ★ P1（datah_016/017/020）：布局存档文字图集 RGBA（9 标签竖排 220×414） */
    int layres_text_w, layres_text_h;          /* 布局存档文字图集尺寸 */
    void *replay_text_px; size_t replay_text_px_len; /* ★ P1（datah_039/107）：复盘文字图集 RGBA（8 标签竖排 220×368） */
    int replay_text_w, replay_text_h;          /* 复盘文字图集尺寸 */
    void *login_text_px; size_t login_text_px_len; /* ★ P1（datah_038）：登录文字图集 RGBA（8 标签竖排 220×368） */
    int login_text_w, login_text_h;           /* 登录文字图集尺寸 */
    void *rules_text_px; size_t rules_text_px_len; /* ★ P2（datah_037）：规则文本文字图集 RGBA（8 标签竖排 220×368） */
    int rules_text_w, rules_text_h;           /* 规则文本文字图集尺寸 */
    void *ng104_text_px; size_t ng104_text_px_len; /* ★ P2（datah_104）：新局完整版文字图集 RGBA（10 标签竖排 220×460） */
    int ng104_text_w, ng104_text_h;           /* 新局完整版文字图集尺寸 */
    void *join_text_px; size_t join_text_px_len; /* ★ P2（datah_034/035）：加入/创建游戏文字图集 RGBA（10 标签竖排 220×460） */
    int join_text_w, join_text_h;             /* 加入/创建游戏文字图集尺寸 */
    void *ng061_text_px; size_t ng061_text_px_len; /* ★ P2（datah_061）：新局完整版（完整版补充）文字图集 RGBA（10 标签竖排 220×460） */
    int ng061_text_w, ng061_text_h;          /* 新局完整版文字图集尺寸 */
    void *gameinfo_text_px; size_t gameinfo_text_px_len; /* ★ P2（datah_050）：对局信息文字图集 RGBA（8 标签竖排 220×368） */
    int gameinfo_text_w, gameinfo_text_h;    /* 对局信息文字图集尺寸 */
    void *bright_text_px; size_t bright_text_px_len; /* ★ P2（datah_002）：亮度调节文字图集 RGBA（10 标签竖排 220×460） */
    int bright_text_w, bright_text_h;        /* 亮度调节文字图集尺寸 */
    void *chatlog_text_px; size_t chatlog_text_px_len; /* ★ P2（datah_029）：聊天记录文字图集 RGBA（8 标签竖排 220×368） */
    int chatlog_text_w, chatlog_text_h;      /* 聊天记录文字图集尺寸 */
    void *profile_text_px; size_t profile_text_px_len; /* ★ P2（datah_031）：个人资料文字图集 RGBA（10 标签竖排 220×460） */
    int profile_text_w, profile_text_h;      /* 个人资料文字图集尺寸 */
    void *drawdetail_text_px; size_t drawdetail_text_px_len; /* ★ P2（datah_042）：提和详情文字图集 RGBA（7 标签竖排 220×322） */
    int drawdetail_text_w, drawdetail_text_h; /* 提和详情文字图集尺寸 */
    void *setentry_text_px; size_t setentry_text_px_len; /* ★ P2（datah_003/004）：设置入口文字图集 RGBA（7 标签竖排 220×322） */
    int setentry_text_w, setentry_text_h; /* 设置入口文字图集尺寸 */
    void *imgset_text_px; size_t imgset_text_px_len; /* ★ P2（datah_007/008）：图像设置文字图集 RGBA（10 标签竖排 220×460） */
    int imgset_text_w, imgset_text_h; /* 图像设置文字图集尺寸 */
    void *join065_text_px; size_t join065_text_px_len; /* ★ P2（datah_065）：加入游戏完整版文字图集 RGBA（10 标签竖排 220×460） */
    int join065_text_w, join065_text_h; /* 加入游戏完整版文字图集尺寸 */
    void *create069_text_px; size_t create069_text_px_len; /* ★ P2（datah_069）：创建游戏完整版文字图集 RGBA（10 标签竖排 220×460） */
    int create069_text_w, create069_text_h; /* 创建游戏完整版文字图集尺寸 */
    void *rules070_text_px; size_t rules070_text_px_len; /* ★ P2（datah_070）：游戏规则完整版文字图集 RGBA（10 标签竖排 220×460） */
    int rules070_text_w, rules070_text_h; /* 游戏规则完整版文字图集尺寸 */
    void *room060_text_px; size_t room060_text_px_len; /* ★ P2（datah_060）：房间态文字图集 RGBA（8 标签竖排 220×368） */
    int room060_text_w, room060_text_h; /* 房间态文字图集尺寸 */
    void *chat063_text_px; size_t chat063_text_px_len; /* ★ P2（datah_063）：聊天完整版文字图集 RGBA（8 标签竖排 220×368） */
    int chat063_text_w, chat063_text_h; /* 聊天完整版文字图集尺寸 */
    void *about_text_px; size_t about_text_px_len; /* ★ P2（datah_000）：软件信息文字图集 RGBA（7 标签竖排 220×322） */
    int about_text_w, about_text_h; /* 软件信息文字图集尺寸 */
    void *uibat_text_px; size_t uibat_text_px_len; /* ★ SPAK2UI 批量（12 弹窗）：战况统计子菜单链 + 玩家/对局/对战方式/
                                                        创建游戏/版本更新 共用文字图集（14 标签竖排 220×644） */
    int uibat_text_w, uibat_text_h; /* 12 弹窗共用文字图集尺寸 */
    rs_glid_t tex_uibat_text[2];    /* ★ SPAK2UI 批量（12 弹窗）：共用文字图集 GL 纹理（每 slot 一套） */
    void *imen_text_px; size_t imen_text_px_len; /* ★ ☰ 菜单文字图集 RGBA（8 项竖排 160×320） */
    int imen_text_w, imen_text_h;               /* ☰ 菜单文字图集尺寸 */
    void *imen_icons_px; size_t imen_icons_px_len; /* ★ ☰ 菜单图标图集 RGBA（8 图标 110×100 4×2，原版裁剪） */
    int imen_icons_w, imen_icons_h;             /* ☰ 菜单图标图集尺寸 */
    /* ★ 原版 apk 资源真值（ch02 N45）：☰ 菜单 16 图标（本地 8 + 联机 8）
       来源 = SPAK datax PNG 原图裁 alpha bbox（datax_153 星/134 回转/128 箭头/163 纸/
       136 扳手/171 彩饼/124 蓝i/142 红X/140 握手/179 红旗/132 气泡），4×4 网格每格 96。 */
    void *imen_orig_px; size_t imen_orig_px_len;
    int imen_orig_w, imen_orig_h;
    void *hamburger_px; size_t hamburger_px_len; /* ★ ☰/↩ 图标图集 RGBA（128×64，orig_human 裁剪） */
    int hamburger_w, hamburger_h;               /* ☰/↩ 图标图集尺寸 */
    void *btn_big_px; size_t btn_big_px_len;    /* ★ 主菜单大按钮图集 RGBA（802×118，人机+两人） */
    int btn_big_w, btn_big_h;
    void *btn_small_px; size_t btn_small_px_len;/* ★ 主菜单小按钮图集 RGBA（666×116，联网+联机+四国） */
    int btn_small_w, btn_small_h;
    int sel_cell;                /* 选中格（-1=无） */
    int hl_cells[61]; int hl_n;  /* 可走格高亮（jq_touch_state_targets 结果） */
    int dirty;                   /* 脏标记（走子后重绘） */
    /* VBO 路线（GLES2 + GLSL shader，绕开 GLES1 固定管线几何 no-op） */
    rs_glid_t vbo_board[2];      /* 棋盘 6 顶点 xy+st（每 context 一套） */
    rs_glid_t vbo_piece[2];      /* 棋子动态顶点（每帧重建，每 context 一套） */
    rs_glid_t vbo_sel[2];        /* 选中高亮线（每 context 一套） */
    int gl_ok[2];                /* GL 上下文 + VBO 可用标志（每 slot） */
    /* GLES2 GLSL shader（顶点 xy+st 四元，纹理采样；MuMu 64 位 GL2 context 下几何出帧） */
    rs_glid_t prog_tex[2];       /* 纹理 quad program（棋盘/棋子，每 context 一套） */
    rs_glid_t prog_solid[2];     /* 纯色 program（选中高亮线，每 context 一套） */
    rs_glid_t uViewWH_tex[2];    /* tex program 的 uViewWH uniform 位置（像素→NDC） */
    rs_glid_t uViewWH_solid[2];  /* solid program 的 uViewWH uniform 位置 */
    int gl2_ok[2];               /* GL2 shader 编译成功标志（每 slot） */
    /* GL 首帧延迟初始化（PBuffer context 首次 makeCurrent 在渲染线程，
       GLThread window context 在 OnUpdate 内 rs_gl_rebuild slot1） */
    char *spak_dir;             /* SPAK 纹理目录（首帧建纹理用） */
    AAssetManager *am;          /* AAssetManager（首帧建纹理用） */
    int gl_now_slot;            /* 当前 GL 对象 slot（rs_render_frame 检测切换） */
    int gl_px_loaded;          /* 像素解码完成标志（跨 slot 共享） */
    int board_gl_loaded;       /* 兼容字段（纹理上传已归 rs_gl_rebuild，未用） */
} rs_render;

/* 初始化：复用 jq_glview_init 已建好的 EGL 上下文（不再自建，避免双套 EGL 冲突）。
   加载 SPAK 纹理（从 g_assets 或文件系统）。返回 0 成功。 */
int rs_render_init(rs_render *rr, const char *spak_dir, const jq_state *init_state,
                   AAssetManager *assets,
                   rs_egl_disp_t disp, rs_egl_surf_t surf, rs_egl_ctx_t ctx);
/* 每帧渲染（OnUpdate/OnTimer 调）：清屏 + 棋盘 + 棋子 + 选中高亮。 */
void rs_render_frame(rs_render *rr);
void rs_render_diag(rs_render *rr); /* 出帧诊断（swap 前读 back buffer） */
/* 状态变更（走子/吃子后调用，标记重绘 + 更新选中）。 */
void rs_render_dirty(rs_render *rr);
void rs_render_set_selected(rs_render *rr, int cell);
/* 尺寸更新（OnGraphChange）。 */
void rs_render_resize(rs_render *rr, int w, int h);
/* 释放 EGL + 纹理。 */
void rs_render_free(rs_render *rr);

#endif /* RS_RENDER_H */
