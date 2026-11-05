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
