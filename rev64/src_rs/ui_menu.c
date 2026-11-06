/*
 * ui_menu.c — 主菜单 + 触摸高亮（正向开发 UI 层）
 * 原版主菜单文案对齐 string.xml/uitag.xml（GBK 解包：人机对战/两人对战/联网/联机/四国军棋）。
 * 本重写用 5 按钮（屏幕像素几何，720×1280 基准）：
 *   人机对战（开局 STG_PLAY 蓝先手）/ 两人对战（占位）/ 联网对战（占位）/
 *   四国军棋（占位）/ 悔棋重开（restart）。
 * 触摸命中：OnTouch 菜单态查 ui_menu_hit → 进对局；对局态走 jq_touch_state（已有两步走子）。
 * 诚实边界：两人/联网/四国 = 占位（日志记录，不实现联机 UI）；人机 = 直接开局。
 */
#include "ui_menu.h"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "junqi_rs", __VA_ARGS__)

/* 5 按钮：横排 2+2+1，屏幕 720×1280（y 从上到下）。
   ★ 命中判定用"相对实际触摸高度"而非归一化到 720×1280 基准（MuMu 触摸坐标系
     ≈ 渲染 viewport 的 2/3，直接归一化会错位）。故命中 = 触摸坐标相对 g_view_h
     归一化后，与按钮相对 g_view_h 归一化同区间（按钮几何本身按 720×1280 画，
     触摸按实际 view 高折算）。 */
/* 按钮几何 = 实际触摸有效坐标（MuMu 触摸高 ≈ 480；渲染按 view 720×1024 缩放同一套相对位置） */
/* 5 按钮几何（720×1280 基准，y 从上到下）；渲染 rs_draw_menu 按实际 view 缩放画到屏幕，
   触摸命中用归一化 [0,1]² 相对区间（x/view_w、y/view_h）与 g_btns 归一化同区间。 */
/* 按钮几何 720×1280 基准（PBuffer 全高），y 全在 0-1280 */
/* 720x1280 坐标，sx=sy=1（PBuffer 720x1280） */
/* ★ 按钮几何 = 实际屏幕像素坐标系（view_w×view_h，MuMu 720×1024）；
   渲染 rs_draw_menu 用 sx=sy=1 直接画，触摸命中归一化（x/view_w, y/view_h）
   vs 按钮归一化（x0/view_w...）同一坐标系。屏幕/触摸完全对齐。 */
/* ★ 按钮几何 = MuMu 实测触摸坐标系（native 收到的 x/y；实测 y_input=y_actual+160）。
   渲染 PBuffer 同坐标系 + 触摸命中同坐标系 → 屏幕/触摸对齐。y 300-600 区。 */
/* ★ 原版 UI 对齐（本轮，用户指正"它没有高手新手"+截图对比自证）：
   原版主菜单 = datax_294 卡通士兵背景 + 5 按钮在屏幕下部（实测 orig_menu.png
   900×1600 蓝色像素检测：人机(252,1084)-(645,1179)、两人(252,1227)-(645,1322)、
   联网(215,1371)-(429,1479)、联机(469,1371)-(683,1479)、四国(727,1371)-(858,1479)）。
   转 src_rs 720×1280 基准（src_rs view=900×1468+132 状态栏偏移，
   sx=view_w/720=1.25, sy=view_h/1280=1468/1280=1.1469）：
     基准 = (屏幕坐标 - 132偏移)/sx,sy
     人机 (252,952)/(1.25,1.1469)=(201.6,830.1) ... (645,1047)/(1.25,1.1469)=(516,912.9)
     两人 (201.6,954.8)-(516,1037.6)
     联网 (172,1080.3)-(343.2,1174.5)
     联机 (375.2,1080.3)-(546.4,1174.5)
     四国 (581.6,1080.3)-(686.4,1174.5)
   ★ datah_023（y540-750）判误：非主菜单按钮实测位置（原版主菜单按钮全在下部 y1084+）。
   ★ datah_041（新手/中级/高级/好友区）判误：非人机难度弹窗——原版人机/两人直开局
     （orig_human.png/orig_two3.png 暗棋直开局无弹窗），datah_041 疑联网房间选择。
   渲染 rs_draw_menu 用 sx=view_w/720 sy=view_h/1280 缩放；OnTouch 命中同缩放（一致）。 */
/* ★ 原版主菜单对齐基准 = orig_menu.png 真机截图实测（900×1600 → 720×1280 基准 ×0.8）：
   5 按钮：人机(202,867)-(516,943) 两人(202,982)-(516,1058) 联网(172,1097)-(343,1183)
   联机(375,1097)-(546,1183) 四国(582,1097)-(686,1183)。
   ★ 诚实纠正：datah_023 对话框 XML 450 坐标系不能直接"全屏映射 1280"换算（换算链错，
   人机 Δy+88 偏离实测）——主菜单按钮几何以**原版真机截图 orig_menu.png 实测**为准。
   5 按钮（屏幕像素 720×1280 基准）：触摸命中 + 渲染 rs_draw_menu 共用。 */
static const ui_btn g_btns[UI_BTN_COUNT] = {
    /* x0,    y0,    x1,    y1,    label,        action */
    { 201.6f, 867.2f, 516.0f, 943.2f, "人机对战", 0 },   /* 人机 orig_menu 900系(252,1084)-(645,1179)×0.8 */
    { 201.6f, 981.6f, 516.0f, 1057.6f, "两人对战", 1 },   /* 两人 (252,1227)-(645,1322)×0.8 */
    { 172.0f, 1096.8f, 343.2f, 1183.2f, "联网",     2 },   /* 联网 (215,1371)-(429,1479)×0.8 */
    { 375.2f, 1096.8f, 546.4f, 1183.2f, "联机",     3 },   /* 联机 (469,1371)-(683,1479)×0.8 */
    { 581.6f, 1096.8f, 686.4f, 1183.2f, "四国军棋", 4 },   /* 四国 (727,1371)-(858,1479)×0.8 */
};

const ui_btn *ui_menu_buttons(void) { return g_btns; }

int ui_menu_hit(const ui_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

/* ★ 人机难度选择弹窗（对抗审计 datah_041 证据：新手/中级/高级/好友区 × 布局/翻棋 8 按钮）。
   几何 720×1280 基准：面板 180,420-540,820（居中，datah_041 450×335 比例缩放 ×0.8）。
   8 按钮 2 列 × 4 行（datah_041 布局/翻棋列 = 155-275/300-420 缩放 0.8 → 124-220/240-336，
   偏移面板 x0=180 → 304-400/420-516；行 y 25-85/100-160/175-235/250-310 缩放 1.19 →
   450-521/539-610/628-700/718-789）。region 标签列 x 200-300（新手/中级/高级/好友区）。
   action = level/mode 编码（level=action/2+1，mode=action%2）。 */
static const ui_diff_btn g_diff_btns[UI_DIFF_COUNT] = {
    /* x0, y0, x1, y1, action（level/mode）；★ 原版 datah_041 450 坐标系精确换算（面板 450×335 居中 720，偏移+135,+472） */
    { 290.0f, 497.0f, 410.0f, 557.0f, 0 },   /* 新手 布局（level1 mode0） */
    { 435.0f, 497.0f, 555.0f, 557.0f, 1 },   /* 新手 翻棋（level1 mode1） */
    { 290.0f, 572.0f, 410.0f, 632.0f, 2 },   /* 中级 布局（level2 mode0） */
    { 435.0f, 572.0f, 555.0f, 632.0f, 3 },   /* 中级 翻棋（level2 mode1） */
    { 290.0f, 647.0f, 410.0f, 707.0f, 4 },   /* 高级 布局（level3 mode0） */
    { 435.0f, 647.0f, 555.0f, 707.0f, 5 },   /* 高级 翻棋（level3 mode1） */
    { 290.0f, 722.0f, 410.0f, 782.0f, 6 },   /* 好友 布局（level4 mode0） */
    { 435.0f, 722.0f, 555.0f, 782.0f, 7 },   /* 好友 翻棋（level4 mode1） */
};

const ui_diff_btn *ui_diff_buttons(void) { return g_diff_btns; }

int ui_diff_hit(const ui_diff_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_diff_popup_inside(float x, float y)
{
    return x >= UI_DIFF_POPUP_X0 && x <= UI_DIFF_POPUP_X0 + UI_DIFF_POPUP_W &&
           y >= UI_DIFF_POPUP_Y0 && y <= UI_DIFF_POPUP_Y0 + UI_DIFF_POPUP_H;
}

void ui_menu_action_log(int action)
{
    switch (action) {
    case 0: LOGI("UI菜单：人机对战 → 难度选择弹窗（★ 对抗审计 datah_041 证据：新手/中级/高级/好友区 × 布局/翻棋）"); break;
    case 1: LOGI("UI菜单：两人对战 → 直开局（★ 原版 UI 对齐：两人无难度弹窗，orig_two 截图自证）"); break;
    case 2: LOGI("UI菜单：联网对战（占位，SPP socket 由 Java 宿主承担）"); break;
    case 3: LOGI("UI菜单：四国军棋（占位，未实现四国规则）"); break;
    case 4: LOGI("UI菜单：四国军棋 → UI_NPLAYER 四国人数选择弹窗（★ P0 datah_089 证据：2/3/4 人 3 按钮，诚实边界）"); break;
    default: break;
    }
}

/* ★ 原版 UI 对齐：☰ 游戏内菜单（原版实测 orig_ingame_menu.png 900×1600，
   非 datah_006 定义——实测图标中心 4 列 x=123/284/445/606（列距 161）、
   2 行 y=514/702（行距 188）、图标 88×80、文字行中心 594/780）。
   ★ 对抗审计（datah_041 证据修正）：人机主菜单点"人机对战"→ datah_041 难度选择弹窗
     （新手/中级/高级/好友区 × 布局/翻棋 8 按钮），选完开局；datah_006 才是游戏内"选项 UI"。
   ★ datah_006 的 450×285 面板 + 8 按钮 105×135（列距 105）非实际渲染尺寸：
     实测面板更宽（图标 4 列分散列距 161），datah_006 定义仅窗口类声明。
   ☰/↩ 图标在棋盘中部（原版 orig_human.png ~(270,900)/(620,900) 屏幕 900×1600）：
     转 720×1280 基准：x 270/900*720=216, y 900/1600*1280=720 → ☰ 中心 (216,720)；
     ↩ x 620/900*720=496 → 中心 (496,720)。图标范围 70×70。 */
static const ui_imen_btn g_imen_btns[UI_IMENU_COUNT] = {
    /* x0,    y0,    x1,    y1,    action；★ 原版 datah_067 450 坐标系精确换算（面板 450×285 居中 720，偏移+135,+498；图标 105×135） */
    { 150.0f, 523.0f, 255.0f, 658.0f, 0 },  /* 重新开始（中心 202,590） */
    { 255.0f, 523.0f, 360.0f, 658.0f, 1 },  /* 悔棋（中心 308,590） */
    { 360.0f, 523.0f, 465.0f, 658.0f, 2 },  /* 电脑变着（中心 412,590） */
    { 465.0f, 523.0f, 570.0f, 658.0f, 3 },  /* 游戏规则（中心 518,590） */
    { 150.0f, 648.0f, 255.0f, 783.0f, 4 },  /* 功能设置（中心 202,715） */
    { 255.0f, 648.0f, 360.0f, 783.0f, 5 },  /* 战况统计（中心 308,715） */
    { 360.0f, 648.0f, 465.0f, 783.0f, 6 },  /* 软件信息（中心 412,715） */
    { 465.0f, 648.0f, 570.0f, 783.0f, 7 },  /* 退出程序（中心 518,715） */
};

const ui_imen_btn *ui_imen_buttons(void) { return g_imen_btns; }

int ui_imen_hit(const ui_imen_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_imen_popup_inside(float x, float y)
{
    return x >= UI_IMENU_POPUP_X0 && x <= UI_IMENU_POPUP_X0 + UI_IMENU_POPUP_W &&
           y >= UI_IMENU_POPUP_Y0 && y <= UI_IMENU_POPUP_Y0 + UI_IMENU_POPUP_H;
}

void ui_imen_icon_rect(int which, float *x0, float *y0, float *x1, float *y1)
{
    /* which 0=☰（游戏内菜单）1=↩（悔棋）；中心 720×1280 基准 */
    float cx = (which == 0) ? 216.0f : 496.0f;
    float cy = 720.0f, r = 35.0f;
    *x0 = cx - r; *y0 = cy - r; *x1 = cx + r; *y1 = cy + r;
}

/* ★ 对抗审计（datah_067 证据）：蓝牙联机 ☰ 菜单（New/Regret/Draw/Resign/Chat/Config/Stat/Rule）。
   与本地菜单（datah_006）同 4 列 × 2 行几何（实测图标中心 123/284/445/606 × 514/702），
   但第 2/3/4 项不同：本地=电脑变着/游戏规则/软件信息；联机=提和/认输/聊天/游戏规则。 */
static const ui_imen_btn g_bt_imen_btns[UI_IMENU_COUNT] = {
    /* x0,    y0,    x1,    y1,    action；★ 原版 datah_067 450 坐标系精确换算（面板 450×285 居中 720，偏移+135,+498；图标 105×135） */
    { 150.0f, 523.0f, 255.0f, 658.0f, 0 },  /* 重新开始（中心 202,590） */
    { 255.0f, 523.0f, 360.0f, 658.0f, 1 },  /* 悔棋（中心 308,590） */
    { 360.0f, 523.0f, 465.0f, 658.0f, 2 },  /* 提和（中心 412,590） */
    { 465.0f, 523.0f, 570.0f, 658.0f, 3 },  /* 认输（中心 518,590） */
    { 150.0f, 648.0f, 255.0f, 783.0f, 4 },  /* 聊天（中心 202,715） */
    { 255.0f, 648.0f, 360.0f, 783.0f, 5 },  /* 系统设置（中心 308,715） */
    { 360.0f, 648.0f, 465.0f, 783.0f, 6 },  /* 战况统计（中心 412,715） */
    { 465.0f, 648.0f, 570.0f, 783.0f, 7 },  /* 游戏规则（中心 518,715） */
};

const ui_imen_btn *ui_imen_bt_buttons(void) { return g_bt_imen_btns; }

/* ★ 联网/联机方式选择弹窗（对抗审计 datah_072 证据：蓝牙联机/Wi-Fi 联机/个人资料/返回）。
   几何 720×1280 基准：面板 210,440-510,830（居中，datah_072 250×325 ×1.2），
   4 按钮纵排（datah_072 按钮 x 20-230/250 缩放 1.2 → 24-276，偏移面板 x0=210 → 234-486；
   行 y 20-80/95-155/170-230/245-305 缩放 1.2 → 24-96/114-186/204-276/294-366，偏移 440 →
   464-536/554-626/644-716/734-806）。 */
static const ui_way_btn g_way_btns[UI_WAYSEL_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_072 250 坐标系精确换算（面板 250×325 居中 720，偏移+235,+478） */
    { 255.0f, 498.0f, 465.0f, 558.0f, 0 },  /* 蓝牙联机 */
    { 255.0f, 573.0f, 465.0f, 633.0f, 1 },  /* Wi-Fi 联机 */
    { 255.0f, 648.0f, 465.0f, 708.0f, 2 },  /* 个人资料 */
    { 255.0f, 723.0f, 465.0f, 783.0f, 3 },  /* 返回 */
};

const ui_way_btn *ui_way_buttons(void) { return g_way_btns; }

int ui_way_hit(const ui_way_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_way_popup_inside(float x, float y)
{
    return x >= UI_WAYSEL_POPUP_X0 && x <= UI_WAYSEL_POPUP_X0 + UI_WAYSEL_POPUP_W &&
           y >= UI_WAYSEL_POPUP_Y0 && y <= UI_WAYSEL_POPUP_Y0 + UI_WAYSEL_POPUP_H;
}

/* ★ 声音设置弹窗（对抗审计 datah_001 证据：静音模式/按键声音/音量/确定/取消）。
   几何 720×1280 基准：面板 170,400-550,870（居中，datah_001 450×390 ×1.2）。
   5 按钮：2 行开关 + 音量行 + 确定/取消。 */
static const ui_set_btn g_set_btns[UI_SETTINGS_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_001 450 坐标系精确换算（面板 450×390 居中 720，偏移+135,+445） */
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },  /* 静音模式 开关 */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },  /* 按键声音 开关 */
    { 255.0f, 677.0f, 562.0f, 725.0f, 2 },  /* 音量 滑条 */
    { 165.0f, 750.0f, 345.0f, 810.0f, 3 },  /* 确定 */
    { 375.0f, 750.0f, 555.0f, 810.0f, 4 },  /* 取消 */
};

const ui_set_btn *ui_set_buttons(void) { return g_set_btns; }

int ui_set_hit(const ui_set_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_set_popup_inside(float x, float y)
{
    return x >= UI_SETTINGS_POPUP_X0 && x <= UI_SETTINGS_POPUP_X0 + UI_SETTINGS_POPUP_W &&
           y >= UI_SETTINGS_POPUP_Y0 && y <= UI_SETTINGS_POPUP_Y0 + UI_SETTINGS_POPUP_H;
}

/* ★ P0（datah_089 证据）：四国人数选择弹窗——2/3/4 人 3 按钮（无 text 标签，纯数字）。
   几何 720×1280 基准：面板 245,525-475,755（datah_089 230×230 居中，偏移+245,+525；
   局部 1:1 平移：2 人 15,15,215,75 → 260,540-460,600；3 人 15,85,215,145 → 260,610-460,670；
   4 人 15,155,215,215 → 260,680-460,740）。★ 诚实边界：四国=未实现 4 方规则。 */
static const ui_nplayer_btn g_nplayer_btns[UI_NPLAYER_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_089 230 坐标系精确换算（面板 230×230 居中 720，偏移+245,+525） */
    { 260.0f, 540.0f, 460.0f, 600.0f, 0 },   /* 2 人 */
    { 260.0f, 610.0f, 460.0f, 670.0f, 1 },   /* 3 人 */
    { 260.0f, 680.0f, 460.0f, 740.0f, 2 },   /* 4 人（默认：四国=4 人） */
};

const ui_nplayer_btn *ui_nplayer_buttons(void) { return g_nplayer_btns; }

int ui_nplayer_hit(const ui_nplayer_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_nplayer_popup_inside(float x, float y)
{
    return x >= UI_NPLAYER_POPUP_X0 && x <= UI_NPLAYER_POPUP_X0 + UI_NPLAYER_POPUP_W &&
           y >= UI_NPLAYER_POPUP_Y0 && y <= UI_NPLAYER_POPUP_Y0 + UI_NPLAYER_POPUP_H;
}

/* ★ P0（datah_062 证据）：两人新局简化弹窗——玩法(Lay)/明暗棋(Show)/确定/取消 4 控件。
   几何 720×1280 基准：面板 135,482.5-585,797.5（datah_062 450×315 居中，偏移+135,+482.5；
   局部 1:1 平移：玩法 220,75,420,135 → 355,557-555,617；明暗棋 220,150,420,210 →
   355,632-555,692；确定 30,235,210,295 → 165,717-345,777；取消 240,235,420,295 → 375,717-555,777）。
   ★ 诚实边界：两人热座（AI 不介入）；默认翻棋暗棋（对齐原版 g_opt_mode 默认 1）。 */
static const ui_newgame_btn g_newgame_btns[UI_NEWGAME_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_062 450 坐标系精确换算（面板 450×315 居中 720，偏移+135,+482.5） */
    { 355.0f, 557.0f, 555.0f, 617.0f, 0 },   /* 玩法 Lay（明棋/翻棋暗棋切换，默认翻棋暗棋） */
    { 355.0f, 632.0f, 555.0f, 692.0f, 1 },   /* 明暗棋 Show（同玩法切换） */
    { 165.0f, 717.0f, 345.0f, 777.0f, 2 },   /* 确定 Ok（随模式直开局） */
    { 375.0f, 717.0f, 555.0f, 777.0f, 3 },   /* 取消 Cancel（回主菜单） */
};

const ui_newgame_btn *ui_newgame_buttons(void) { return g_newgame_btns; }

int ui_newgame_hit(const ui_newgame_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_newgame_popup_inside(float x, float y)
{
    return x >= UI_NEWGAME_POPUP_X0 && x <= UI_NEWGAME_POPUP_X0 + UI_NEWGAME_POPUP_W &&
           y >= UI_NEWGAME_POPUP_Y0 && y <= UI_NEWGAME_POPUP_Y0 + UI_NEWGAME_POPUP_H;
}

/* ★ P1（datah_012 证据）：游戏设置弹窗——4 规则开关（和棋/大本营/吃地雷/吃军旗）+ 确定/取消。
   几何 720×1280 基准：面板 135,407.5-585,872.5（datah_012 450×465 居中，偏移+135,+407.5；
   局部 1:1 平移：和棋 210,75,420,135 → 345,483-555,543；大本营 210,150,420,210 →
   345,558-555,618；吃地雷 210,225,420,285 → 345,633-555,693；吃军旗 210,300,420,360 →
   345,708-555,768；确定 30,385,210,445 → 165,793-345,853；取消 240,385,420,445 → 375,793-555,853）。
   ★ 诚实边界：开关"开"状态的原版精确可选值 .rodata 未读全，默认全开（对齐原版默认，行为不变）。 */
static const ui_gameset_btn g_gameset_btns[UI_GAMESET_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_012 450 坐标系精确换算（面板 450×465 居中 720，偏移+135,+407.5） */
    { 345.0f, 483.0f, 555.0f, 543.0f, 0 },   /* 和棋 Draw 开关（默认开） */
    { 345.0f, 558.0f, 555.0f, 618.0f, 1 },   /* 大本营 Command 开关（默认开） */
    { 345.0f, 633.0f, 555.0f, 693.0f, 2 },   /* 吃地雷 BingLei 开关（默认开） */
    { 345.0f, 708.0f, 555.0f, 768.0f, 3 },   /* 吃军旗 BingQi 开关（默认开） */
    { 165.0f, 793.0f, 345.0f, 853.0f, 4 },   /* 确定 Ok（保存+应用） */
    { 375.0f, 793.0f, 555.0f, 853.0f, 5 },   /* 取消 Cancel（恢复原值） */
};

const ui_gameset_btn *ui_gameset_buttons(void) { return g_gameset_btns; }

int ui_gameset_hit(const ui_gameset_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_gameset_popup_inside(float x, float y)
{
    return x >= UI_GAMESET_POPUP_X0 && x <= UI_GAMESET_POPUP_X0 + UI_GAMESET_POPUP_W &&
           y >= UI_GAMESET_POPUP_Y0 && y <= UI_GAMESET_POPUP_Y0 + UI_GAMESET_POPUP_H;
}

/* ★ P1（datah_017 证据）：载入布局弹窗——布局名文本列表 6 行（ListBox 20,75,380,440 400 系 →
   720 系 180,447-540,807，行高 60px 每行 1 可点区）+ 取消（原版 Cancel 120,455,280,515 →
   280,828-440,888）。面板 400×535 居中（偏移+160,+372.5）。
   ★ 诚实边界：ListBox 滚动未实现（触摸 flaky），文本列表 6 条截断 + 选中高亮。 */
static const ui_loadlayout_btn g_loadlayout_btns[UI_LOADLAYOUT_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_017 400 坐标系精确换算（面板 400×535 居中 720，偏移+160,+372.5） */
    { 180.0f, 447.0f, 540.0f, 507.0f, 0 },   /* 列表项 0（第 1 条，默认选中高亮） */
    { 180.0f, 507.0f, 540.0f, 567.0f, 1 },   /* 列表项 1 */
    { 180.0f, 567.0f, 540.0f, 627.0f, 2 },   /* 列表项 2 */
    { 180.0f, 627.0f, 540.0f, 687.0f, 3 },   /* 列表项 3 */
    { 180.0f, 687.0f, 540.0f, 747.0f, 4 },   /* 列表项 4 */
    { 180.0f, 747.0f, 540.0f, 807.0f, 5 },   /* 列表项 5（第 6 条，超出截断） */
    { 280.0f, 828.0f, 440.0f, 888.0f, 6 },   /* 取消 Cancel（120,455,280,515 +160,+372.5） */
};

const ui_loadlayout_btn *ui_loadlayout_buttons(void) { return g_loadlayout_btns; }

int ui_loadlayout_hit(const ui_loadlayout_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_loadlayout_popup_inside(float x, float y)
{
    return x >= UI_LOADLAYOUT_POPUP_X0 && x <= UI_LOADLAYOUT_POPUP_X0 + UI_LOADLAYOUT_POPUP_W &&
           y >= UI_LOADLAYOUT_POPUP_Y0 && y <= UI_LOADLAYOUT_POPUP_Y0 + UI_LOADLAYOUT_POPUP_H;
}

/* ★ P1（datah_020 证据）：保存布局弹窗——保存/改名/删除/关闭 4 按钮
   （原版 Ok 15,455,100,515 → 175,828-260,888；Rename 110,455,195,515 → 270,828-355,888；
   Delete 205,455,290,515 → 365,828-450,888；Cancel 300,455,385,515 → 460,828-545,888）。
   布局名列表显示区 = 017 ListBox 同区（180,447-540,807，文本列表 6 条截断）。
   ★ 诚实边界：改名简化为自动编号（旧名+数字后缀，无文本输入框）。 */
static const ui_savelayout_btn g_savelayout_btns[UI_SAVELAYOUT_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_020 400 坐标系精确换算（面板 400×535 居中 720，偏移+160,+372.5） */
    { 175.0f, 828.0f, 260.0f, 888.0f, 0 },   /* 保存 Ok（写当前棋盘 30 字节 .lay） */
    { 270.0f, 828.0f, 355.0f, 888.0f, 1 },   /* 改名 Rename（旧名+数字后缀） */
    { 365.0f, 828.0f, 450.0f, 888.0f, 2 },   /* 删除 Delete（删选中项 .lay） */
    { 460.0f, 828.0f, 545.0f, 888.0f, 3 },   /* 关闭 Cancel（回对局） */
};

const ui_savelayout_btn *ui_savelayout_buttons(void) { return g_savelayout_btns; }

int ui_savelayout_hit(const ui_savelayout_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_savelayout_popup_inside(float x, float y)
{
    return x >= UI_SAVELAYOUT_POPUP_X0 && x <= UI_SAVELAYOUT_POPUP_X0 + UI_SAVELAYOUT_POPUP_W &&
           y >= UI_SAVELAYOUT_POPUP_Y0 && y <= UI_SAVELAYOUT_POPUP_Y0 + UI_SAVELAYOUT_POPUP_H;
}

/* ★ P1（datah_016 证据）：布局管理弹窗——载入/删除/取消 3 按钮（原版 Ok 25,390,129,450 →
   185,793-289,853；Delete 148,390,252,450 → 308,793-412,853；Cancel 271,390,375,450 →
   431,793-535,853）。30 格棋子预览区（原版 Piece0-29 20,75,380,360 → 180,477-540,762）
   简化小方块 6×5 占位（有子=蓝/红方块，真棋子图未画）。
   面板 400×475 居中（偏移+160,+402.5）。★ 诚实边界：原版 Ok2/Cancel2 重叠备用按钮未实现。 */
static const ui_laymgr_btn g_laymgr_btns[UI_LAYMGR_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_016 400 坐标系精确换算（面板 400×475 居中 720，偏移+160,+402.5） */
    { 185.0f, 793.0f, 289.0f, 853.0f, 0 },   /* 载入 Ok（应用到棋盘 + 清 undo） */
    { 308.0f, 793.0f, 412.0f, 853.0f, 1 },   /* 删除 Delete（删选中项文件） */
    { 431.0f, 793.0f, 535.0f, 853.0f, 2 },   /* 取消 Cancel（回对局） */
};

const ui_laymgr_btn *ui_laymgr_buttons(void) { return g_laymgr_btns; }

int ui_laymgr_hit(const ui_laymgr_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++) {
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    }
    return -1;
}

int ui_laymgr_popup_inside(float x, float y)
{
    return x >= UI_LAYMGR_POPUP_X0 && x <= UI_LAYMGR_POPUP_X0 + UI_LAYMGR_POPUP_W &&
           y >= UI_LAYMGR_POPUP_Y0 && y <= UI_LAYMGR_POPUP_Y0 + UI_LAYMGR_POPUP_H;
}

/* ★ P1（datah_107 证据）：复盘回放控制条 5 按钮（菜单/回退/前进/跳转/返回）。
   几何 720×1280 基准：面板 460×60 @ 绝对 10,705（不居中，局部 1:1 平移）；
   5 按钮各 84×60（局部 x 0/94/188/282/376 → 720 系 10/104/188/292/386；y 705-765）。
   ★ 诚实边界：跳转=回开头/跳结尾循环（无步进输入框）；菜单=退复盘（无子菜单）。 */
static const ui_replay_btn g_replay_ctrl_btns[UI_REPLAY_CTRL_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_107 局部坐标精确平移（面板 @10,705，
       局部 1:1 平移：Menu 0,0,84,60 / Prev 94,0,178,60 / Next 188,0,272,60 /
       Jump 282,0,366,60 / Back 376,0,460,60 → 720 系 10,705/104,705/198,705/
       292,705/386,705，各 84×60；layout_align_check datah_107_replay 逐按钮 Δ 校验） */
    { 10.0f,  705.0f, 94.0f,  765.0f, 0 },  /* 菜单 Menu（简化=退复盘回对局） */
    { 104.0f, 705.0f, 188.0f, 765.0f, 1 },  /* 回退 Prev（cur-- 重放） */
    { 198.0f, 705.0f, 282.0f, 765.0f, 2 },  /* 前进 Next（cur++ 走 1 步） */
    { 292.0f, 705.0f, 376.0f, 765.0f, 3 },  /* 跳转 Jump（回开头/跳结尾循环，简化） */
    { 386.0f, 705.0f, 470.0f, 765.0f, 4 },  /* 返回 Back（退复盘回对局/主菜单） */
};

const ui_replay_btn *ui_replay_ctrl_buttons(void) { return g_replay_ctrl_btns; }

int ui_replay_ctrl_hit(const ui_replay_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_replay_ctrl_popup_inside(float x, float y)
{
    return x >= UI_REPLAY_CTRL_POPUP_X0 && x <= UI_REPLAY_CTRL_POPUP_X0 + UI_REPLAY_CTRL_POPUP_W &&
           y >= UI_REPLAY_CTRL_POPUP_Y0 && y <= UI_REPLAY_CTRL_POPUP_Y0 + UI_REPLAY_CTRL_POPUP_H;
}

/* ★ P1（datah_038 证据）：登录弹窗 6 控件（用户名/密码占位标签 + 登录/注册/游客/返回）。
   几何 720×1280 基准：面板 432×285 居中（偏移+144,+497.5）；局部 1:1 平移：
   用户名 EditLabel 130,80,375,125 → 274,578-519,623；
   密码 EditLabel 130,140,375,185 → 274,638-519,683；
   登录 Login 18,205,108,265 → 162,703-252,763；
   注册 Register 120,205,210,265 → 264,703-354,763；
   游客 Guest 222,205,312,265 → 366,703-456,763；
   返回 Cancel 324,205,414,265 → 468,703-558,763。
   ★ 诚实边界：原版登录=账号系统（服务器协议未逆向）；输入框简化为占位标签
   （触摸 flaky 下输入框无意义）；游客=本地单机唯一真生效路径。 */
static const ui_login_btn g_login_btns[UI_LOGIN_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_038 432 坐标系精确换算（面板 432×285 居中 720，偏移+144,+497.5） */
    { 274.0f, 578.0f, 519.0f, 623.0f, 0 },   /* 用户名 EditLabel 占位标签（130,80,375,125） */
    { 274.0f, 638.0f, 519.0f, 683.0f, 1 },   /* 密码 EditLabel 占位标签（130,140,375,185） */
    { 162.0f, 703.0f, 252.0f, 763.0f, 2 },   /* 登录 Login（18,205,108,265）→ 诚实边界对话框 */
    { 264.0f, 703.0f, 354.0f, 763.0f, 3 },   /* 注册 Register（120,205,210,265）→ 诚实边界对话框 */
    { 366.0f, 703.0f, 456.0f, 763.0f, 4 },   /* 游客 Guest（222,205,312,265）→ 本地单机开局 */
    { 468.0f, 703.0f, 558.0f, 763.0f, 5 },   /* 返回 Cancel（324,205,414,265）→ 回主菜单 */
};

const ui_login_btn *ui_login_buttons(void) { return g_login_btns; }

int ui_login_hit(const ui_login_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_login_popup_inside(float x, float y)
{
    return x >= UI_LOGIN_POPUP_X0 && x <= UI_LOGIN_POPUP_X0 + UI_LOGIN_POPUP_W &&
           y >= UI_LOGIN_POPUP_Y0 && y <= UI_LOGIN_POPUP_Y0 + UI_LOGIN_POPUP_H;
}

/* ★ P2（datah_037 证据）：规则文本弹窗 3 按钮（规则-翻棋/规则-布局/返回）。
   几何 720×1280 基准：面板 260×270 居中（偏移+230,+505）；局部 1:1 平移：
   翻棋 25,25,235,85 → 255,530-465,590；布局 25,105,235,165 → 255,610-465,670；
   返回 25,185,235,245 → 255,690-465,750。
   ★ 诚实边界：规则文本=src_rs 现有简化版固定文本（2 套：翻棋/布局）。 */
static const ui_rules_btn g_rules_btns[UI_RULES_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_037 260 坐标系精确换算（面板 260×270 居中 720，偏移+230,+505） */
    { 255.0f, 530.0f, 465.0f, 590.0f, 0 },   /* 规则-翻棋（25,25,235,85）→ 显示翻棋模式规则文本 */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },   /* 规则-布局（25,105,235,165）→ 显示布局模式规则文本 */
    { 255.0f, 690.0f, 465.0f, 750.0f, 2 },   /* 返回 Back（25,185,235,245）→ 回对局/主菜单 */
};

const ui_rules_btn *ui_rules_buttons(void) { return g_rules_btns; }

int ui_rules_hit(const ui_rules_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_rules_popup_inside(float x, float y)
{
    return x >= UI_RULES_POPUP_X0 && x <= UI_RULES_POPUP_X0 + UI_RULES_POPUP_W &&
           y >= UI_RULES_POPUP_Y0 && y <= UI_RULES_POPUP_Y0 + UI_RULES_POPUP_H;
}

/* ★ P2（datah_104 证据）：新局完整版弹窗 5 控件（我方/电脑水平/玩法/确定/取消）。
   几何 720×1280 基准：面板 450×390 居中（偏移+135,+445）；局部 1:1 平移：
   我方 220,75,420,135 → 355,520-555,580；电脑水平 220,150,420,210 → 355,595-555,655；
   玩法 220,225,420,285 → 355,670-555,730；确定 30,310,210,370 → 165,755-345,815；
   取消 240,310,420,370 → 375,755-555,815。
   ★ 诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。 */
static const ui_newgame104_btn g_newgame104_btns[UI_NEWGAME104_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_104 450 坐标系精确换算（面板 450×390 居中 720，偏移+135,+445） */
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },   /* 我方 Me（220,75,420,135）→ 蓝/红 切换 */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },   /* 电脑水平 Difficulty（220,150,420,210）→ 难度 1-4 循环 */
    { 355.0f, 670.0f, 555.0f, 730.0f, 2 },   /* 玩法 Type（220,225,420,285）→ 布局/翻棋 切换 */
    { 165.0f, 755.0f, 345.0f, 815.0f, 3 },   /* 确定 Ok（30,310,210,370）→ 按选择开局 */
    { 375.0f, 755.0f, 555.0f, 815.0f, 4 },   /* 取消 Cancel（240,310,420,370）→ 回主菜单 */
};

const ui_newgame104_btn *ui_newgame104_buttons(void) { return g_newgame104_btns; }

int ui_newgame104_hit(const ui_newgame104_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_newgame104_popup_inside(float x, float y)
{
    return x >= UI_NEWGAME104_POPUP_X0 && x <= UI_NEWGAME104_POPUP_X0 + UI_NEWGAME104_POPUP_W &&
           y >= UI_NEWGAME104_POPUP_Y0 && y <= UI_NEWGAME104_POPUP_Y0 + UI_NEWGAME104_POPUP_H;
}

/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗 3 控件（桌号/确定/取消）。
   几何 720×1280 基准：面板 420×235 居中（偏移+150,+522.5）；局部 1:1 平移：
   桌号 120,80,360,125 → 270,603-510,648；确定 35,150,195,210 → 185,673-345,733；
   取消 225,150,385,210 → 375,673-535,733。
   ★ 诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP；
   确定 → 诚实边界对话框 + 回主菜单；取消 → 回主菜单。 */
static const ui_join_btn g_join_btns[UI_JOIN_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_034 420 坐标系精确换算（面板 420×235 居中 720，
       偏移=(720-420)/2=150, (1280-235)/2=522.5；局部 1:1 平移：
       桌号 120,80,360,125 → 270,602.5-510,647.5 → 四舍五入 270,603-510,648；
       确定 35,150,195,210 → 185,672.5-345,732.5 → 四舍五入 185,673-345,733；
       取消 225,150,385,210 → 375,672.5-535,732.5 → 四舍五入 375,673-535,733） */
    { 270.0f, 602.5f, 510.0f, 647.5f, 0 },   /* 桌号 Table（120,80,360,125）→ 占位标签 */
    { 185.0f, 672.5f, 345.0f, 732.5f, 1 },   /* 确定 Ok（35,150,195,210）→ 诚实边界对话框 */
    { 375.0f, 672.5f, 535.0f, 732.5f, 2 },   /* 取消 Cancel（225,150,385,210）→ 回主菜单 */
};

const ui_join_btn *ui_join_buttons(void) { return g_join_btns; }

int ui_join_hit(const ui_join_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_join_popup_inside(float x, float y)
{
    return x >= UI_JOIN_POPUP_X0 && x <= UI_JOIN_POPUP_X0 + UI_JOIN_POPUP_W &&
           y >= UI_JOIN_POPUP_Y0 && y <= UI_JOIN_POPUP_Y0 + UI_JOIN_POPUP_H;
}

/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗 3 控件（密码/确定/取消）。
   几何 720×1280 基准：面板 420×235 居中（偏移同 034=+150,+522.5）；局部 1:1 平移：
   密码 100,80,320,125 → 250,603-470,648；确定 35,150,195,210 → 185,673-345,733；
   取消 225,150,385,210 → 375,673-535,733。
   ★ 诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP；
   确定 → 诚实边界对话框 + 回主菜单；取消 → 回主菜单。 */
static const ui_create_btn g_create_btns[UI_CREATE_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_035 420 坐标系精确换算（面板 420×235 居中 720，
       偏移=(720-420)/2=150, (1280-235)/2=522.5；局部 1:1 平移：
       密码 100,80,320,125 → 250,602.5-470,647.5 → 四舍五入 250,603-470,648；
       确定 35,150,195,210 → 185,672.5-345,732.5 → 四舍五入 185,673-345,733；
       取消 225,150,385,210 → 375,672.5-535,732.5 → 四舍五入 375,673-535,733） */
    { 250.0f, 602.5f, 470.0f, 647.5f, 0 },   /* 密码 Pwd（100,80,320,125）→ 占位标签 */
    { 185.0f, 672.5f, 345.0f, 732.5f, 1 },   /* 确定 Ok（35,150,195,210）→ 诚实边界对话框 */
    { 375.0f, 672.5f, 535.0f, 732.5f, 2 },   /* 取消 Cancel（225,150,385,210）→ 回主菜单 */
};

const ui_create_btn *ui_create_buttons(void) { return g_create_btns; }

int ui_create_hit(const ui_create_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_create_popup_inside(float x, float y)
{
    return x >= UI_CREATE_POPUP_X0 && x <= UI_CREATE_POPUP_X0 + UI_CREATE_POPUP_W &&
           y >= UI_CREATE_POPUP_Y0 && y <= UI_CREATE_POPUP_Y0 + UI_CREATE_POPUP_H;
}
