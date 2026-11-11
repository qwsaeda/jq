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

/* ★ AUD-1000-ALL A4-08（P3 已修）：头注释 8 版口径堆叠归并为单版现行口径（历史演进留痕）。
   现行口径（本轮定案）：
   · 5 按钮（横排 2+2+1）：几何 = 720×1280 基准（PBuffer 全高，y 全在 0-1280）。
   · 渲染：rs_draw_menu 按实际 view 缩放画（sx=view_w/720, sy=view_h/1280）。
   · 触摸命中：归一化 [0,1]² 相对区间（x/view_w, y/view_h）与 g_btns 归一化同区间。
   · 按钮几何 = 实际屏幕像素坐标系（view_w×view_h）；渲染 + 触摸命中同一坐标系 → 对齐。
   历史演进（旧口径留痕，已作废）：
   · v1 触摸高 ≈480（MuMu 旧分辨率）→ 归一化 720×1280（错位，已废弃）
   · v2 720×1024 缩放同一套相对位置 → sx=sy=1 PBuffer 720×1024（已废弃）
   · v3 MuMu 实测 y_input=y_actual+160（状态栏偏移，已并入基准换算公式）
   · v4 用户指正"没有高手新手"（datah_041 判误）→ 直开局无难度弹窗（datah_023/041 判误留痕） */
/* ★ 原版 UI 对齐（本轮，用户指正"它没有高手新手"+截图对比自证）：
   原版主菜单 = datax_294 卡通士兵背景 + 5 按钮在屏幕下部（实测 orig_menu.png
   900×1600 蓝色像素检测：人机(252,1084)-(645,1179)、两人(252,1227)-(645,1322)、
   联网(215,1371)-(429,1479)、联机(469,1371)-(683,1479)、四国(727,1371)-(858,1479)）。
   ★ 旧换算链（sx=1.25/sy=1.1469，900×1468 view + 132 偏移）已作废——
   被下方 orig_menu.png 真机截图 ×0.8 实测基准取代（★ A6-21 口径纠正）：
     人机 (252,952)/(1.25,1.1469)=(201.6,830.1) ... (645,1047)/(1.25,1.1469)=(516,912.9)
     两人 (201.6,954.8)-(516,1037.6)
     联网 (172,1080.3)-(343.2,1174.5)
     联机 (375.2,1080.3)-(546.4,1174.5)
     四国 (581.6,1080.3)-(686.4,1174.5)（上述为旧链留痕，现行表值见下方 g_btns）
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
    /* ★ 对抗审计 P1（批4 P1-R4）：原 case 3 日志误写"四国军棋（占位）"——
       g_btns[3] 是"联机"（action=3），"四国军棋"是 g_btns[4]（action=4）。
       case 3 应与 case 2 区分：联机=蓝牙 SPP 会话（已实装），联网=边界占位。 */
    case 3: LOGI("UI菜单：联机对战（蓝牙 SPP，host/client 会话已实装，进入蓝牙联机流程）"); break;
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
    /* x0,    y0,    x1,    y1,    action
       ★★ 原版对齐修正（用户截图指正"按钮都是错乱的"）：原版 ☰ 菜单真实布局 = 原版真机
       截图 orig_ingame_menu.png（900×1600）像素实测，×0.8 转 720×1280 基准：
         列心 x = 125 / 289 / 447 / 606（列距 ~161）
         图标行1 中心 y = 516，文字行1 中心 y = 594
         图标行2 中心 y = 703，文字行2 中心 y = 779
         图标尺寸 ≈ 96×96，文字 4 字宽 ≈ 122 / 2 字 ≈ 53
       ★ 之前错在：datah_006 XML 的 450 系坐标 1:1 平移（列距仅 105）+ 文字固定宽 134
         （900 系 ×0.8 换算，与 450 系 1:1 的按钮宽度 105 混用两套基准）→ 文字互相重叠。
       ★ 本表命中区 = 图标 + 文字整块（每项 152 宽 × 158 高）。 */
    {  49.0f, 466.0f, 201.0f, 624.0f, 0 },  /* 重新开始（列心125 图标516 文字594） */
    { 213.0f, 466.0f, 365.0f, 624.0f, 1 },  /* 悔棋（列心289） */
    { 371.0f, 466.0f, 523.0f, 624.0f, 2 },  /* 电脑变着（列心447） */
    { 530.0f, 466.0f, 682.0f, 624.0f, 3 },  /* 游戏规则（列心606） */
    {  49.0f, 653.0f, 201.0f, 809.0f, 4 },  /* 功能设置（图标703 文字779） */
    { 213.0f, 653.0f, 365.0f, 809.0f, 5 },  /* 战况统计 */
    { 371.0f, 653.0f, 523.0f, 809.0f, 6 },  /* 软件信息 */
    { 530.0f, 653.0f, 682.0f, 809.0f, 7 },  /* 退出程序 */
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
    /* x0,    y0,    x1,    y1,    action
       ★ 原版实测几何（orig_ingame_menu.png ×0.8 → 720 基准，与本地版同列位同行位）：
         列心 125/289/447/606；图标行1 y=516 文字行1 y=594；图标行2 y=703 文字行2 y=779。
       datah_067（联机版 ☰ 菜单）项目：重新开始/悔棋/提和/认输/聊天/系统设置/战况统计/游戏规则 */
    {  49.0f, 466.0f, 201.0f, 624.0f, 0 },  /* 重新开始 */
    { 213.0f, 466.0f, 365.0f, 624.0f, 1 },  /* 悔棋 */
    { 371.0f, 466.0f, 523.0f, 624.0f, 2 },  /* 提和 */
    { 530.0f, 466.0f, 682.0f, 624.0f, 3 },  /* 认输 */
    {  49.0f, 653.0f, 201.0f, 809.0f, 4 },  /* 聊天 */
    { 213.0f, 653.0f, 365.0f, 809.0f, 5 },  /* 系统设置 */
    { 371.0f, 653.0f, 523.0f, 809.0f, 6 },  /* 战况统计 */
    { 530.0f, 653.0f, 682.0f, 809.0f, 7 },  /* 游戏规则 */
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
    { 255.0f, 572.0f, 465.0f, 632.0f, 1 },  /* Wi-Fi 联机（25,100,235,160+235,477.5） */
    { 255.0f, 648.0f, 465.0f, 708.0f, 2 },  /* 个人资料（25,175,235,235+235,477.5） */
    { 255.0f, 722.0f, 465.0f, 782.0f, 3 },  /* 返回 Back（25,250,235,310+235,477.5） */
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
    { 255.0f, 530.0f, 465.0f, 590.0f, 0 },   /* 规则-翻棋（25,25,235,85+230,505） */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },   /* 规则-布局（25,105,235,165+230,505） */
    { 255.0f, 690.0f, 465.0f, 750.0f, 2 },   /* 返回 Back（25,185,235,245+230,505）→ 回对局/主菜单 */
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

/* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗 6 控件（先后手/电脑水平/玩法/明暗棋/确定/取消）。
   几何 720×1280 基准：面板 450×465 居中（偏移=(720-450)/2=135,(1280-465)/2=407.5）；
   局部 1:1 平移（取整）：
   先后手 First 220,75,420,135 → 355,482.5-555,542.5 → 355,483-555,543；
   电脑水平 Level 220,150,420,210 → 355,557.5-555,617.5 → 355,558-555,618；
   玩法 Lay 220,225,420,285 → 355,632.5-555,692.5 → 355,633-555,693；
   明暗棋 Show 220,300,420,360 → 355,707.5-555,767.5 → 355,708-555,768；
   确定 Ok 30,385,210,445 → 165,792.5-345,852.5 → 165,793-345,853；
   取消 Cancel 240,385,420,445 → 375,792.5-555,852.5 → 375,793-555,853。
   ★ 诚实边界：4 项各循环切换当前值（对齐 datah_104 循环切换范式）。 */
static const ui_ng061_btn g_ng061_btns[UI_NG061_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_061 450 坐标系精确换算（面板 450×465 居中 720，偏移+135,+407.5，取整） */
    { 355.0f, 482.0f, 555.0f, 542.0f, 0 },   /* 先后手 First（20,75,220,135+135,407.5） */
    { 355.0f, 558.0f, 555.0f, 618.0f, 1 },   /* 电脑水平 Level（20,150,220,210+135,407.5） */
    { 355.0f, 632.0f, 555.0f, 692.0f, 2 },   /* 玩法 Lay（20,225,220,285+135,407.5） */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },   /* 明暗棋 Show（20,300,220,360+135,407.5） */
    { 165.0f, 792.0f, 345.0f, 852.0f, 4 },   /* 确定 Ok（30,385,210,445+135,407.5） */
    { 375.0f, 792.0f, 555.0f, 852.0f, 5 },   /* 取消 Cancel（240,385,420,445+135,407.5） */
};

const ui_ng061_btn *ui_ng061_buttons(void) { return g_ng061_btns; }

int ui_ng061_hit(const ui_ng061_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_ng061_popup_inside(float x, float y)
{
    return x >= UI_NG061_POPUP_X0 && x <= UI_NG061_POPUP_X0 + UI_NG061_POPUP_W &&
           y >= UI_NG061_POPUP_Y0 && y <= UI_NG061_POPUP_Y0 + UI_NG061_POPUP_H;
}

/* ★ P2（datah_050 证据）：对局信息弹窗 3 按钮（复盘/删除/返回）。
   几何 720×1280 基准：面板 420×380 居中（偏移=(720-420)/2=150,(1280-380)/2=450）；
   局部 1:1 平移：
   复盘 View 25,295,135,355 → 175,745-285,805；
   删除 Remove 155,295,265,355 → 305,745-415,805；
   返回 Close 285,295,395,355 → 435,745-545,805。
   ★ 诚实边界：Info 文本=☰ 战况统计复用；删除=清复盘记录简化（原版删除对局存档未逆向）。 */
static const ui_gameinfo_btn g_gameinfo_btns[UI_GAMEINFO_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_050 420 坐标系精确换算（面板 420×380 居中 720，偏移+150,+450） */
    { 175.0f, 745.0f, 285.0f, 805.0f, 0 },   /* 复盘 View（25,295,135,355）→ 进 UI_REPLAY 复盘态 */
    { 305.0f, 745.0f, 415.0f, 805.0f, 1 },   /* 删除 Remove（155,295,265,355）→ 清复盘记录 */
    { 435.0f, 745.0f, 545.0f, 805.0f, 2 },   /* 返回 Close（285,295,395,355）→ 回对局 */
};

const ui_gameinfo_btn *ui_gameinfo_buttons(void) { return g_gameinfo_btns; }

int ui_gameinfo_hit(const ui_gameinfo_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_gameinfo_popup_inside(float x, float y)
{
    return x >= UI_GAMEINFO_POPUP_X0 && x <= UI_GAMEINFO_POPUP_X0 + UI_GAMEINFO_POPUP_W &&
           y >= UI_GAMEINFO_POPUP_Y0 && y <= UI_GAMEINFO_POPUP_Y0 + UI_GAMEINFO_POPUP_H;
}

/* ★ P2（datah_002 证据）：亮度调节弹窗 3 控件（亮度/确定/取消）。
   几何 720×1280 基准：面板 460×240 居中（偏移=(720-460)/2=130,(1280-240)/2=520）；
   局部 1:1 平移：
   亮度滑条 Bright 22,81,438,129 → 152,601-568,649（简化：不画真滑条，循环切换区）；
   确定 Ok 30,155,215,215 → 160,675-345,735；
   取消 Cancel 245,155,430,215 → 375,675-560,735。
   ★ 诚实边界：原版滑条拖拽=触摸 flaky，简化为"低/中/高"3 档循环切换（对齐 datah_001 音量循环范式）。 */
static const ui_bright_btn g_bright_btns[UI_BRIGHT_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_002 460 坐标系精确换算（面板 460×240 居中 720，偏移+130,+520） */
    { 152.0f, 601.0f, 568.0f, 649.0f, 0 },   /* 亮度 Bright（22,81,438,129）→ 低/中/高 循环（不画真滑条） */
    { 160.0f, 675.0f, 345.0f, 735.0f, 1 },   /* 确定 Ok（30,155,215,215）→ 保存 g_bright/ui_volume */
    { 375.0f, 675.0f, 560.0f, 735.0f, 2 },   /* 取消 Cancel（245,155,430,215）→ 恢复原值 */
};

const ui_bright_btn *ui_bright_buttons(void) { return g_bright_btns; }

int ui_bright_hit(const ui_bright_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_bright_popup_inside(float x, float y)
{
    return x >= UI_BRIGHT_POPUP_X0 && x <= UI_BRIGHT_POPUP_X0 + UI_BRIGHT_POPUP_W &&
           y >= UI_BRIGHT_POPUP_Y0 && y <= UI_BRIGHT_POPUP_Y0 + UI_BRIGHT_POPUP_H;
}

/* ★ P2（datah_029 证据）：聊天记录弹窗 2 按钮（清除/关闭）。
   几何 720×1280 基准：面板 420×530 居中（偏移=(720-420)/2=150,(1280-530)/2=375）；
   局部 1:1 平移：
   清除 Clear 37,450,193,510 → 187,825-343,885；
   关闭 Close 227,450,383,510 → 377,825-533,885。
   ★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
static const ui_chatlog_btn g_chatlog_btns[UI_CHATLOG_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_029 420 坐标系精确换算（面板 420×530 居中 720，偏移+150,+375） */
    { 187.0f, 825.0f, 343.0f, 885.0f, 0 },   /* 清除 Clear（37,450,193,510）→ 清本地占位记录 */
    { 377.0f, 825.0f, 533.0f, 885.0f, 1 },   /* 关闭 Close（227,450,383,510）→ 回对局 */
};

const ui_chatlog_btn *ui_chatlog_buttons(void) { return g_chatlog_btns; }

int ui_chatlog_hit(const ui_chatlog_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_chatlog_popup_inside(float x, float y)
{
    return x >= UI_CHATLOG_POPUP_X0 && x <= UI_CHATLOG_POPUP_X0 + UI_CHATLOG_POPUP_W &&
           y >= UI_CHATLOG_POPUP_Y0 && y <= UI_CHATLOG_POPUP_Y0 + UI_CHATLOG_POPUP_H;
}

/* ★ P2（datah_031 证据）：个人资料弹窗 3 按钮（修改/帮助/返回）。
   几何 720×1280 基准：面板 420×520 居中（偏移=(720-420)/2=150,(1280-520)/2=380）；
   局部 1:1 平移：
   修改 Modify 25,435,135,495 → 175,815-285,875；
   帮助 Help 155,435,265,495 → 305,815-415,875；
   返回 Close 285,435,395,495 → 435,815-545,875。
   ★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
static const ui_profile_btn g_profile_btns[UI_PROFILE_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_031 420 坐标系精确换算（面板 420×520 居中 720，偏移+150,+380） */
    { 175.0f, 815.0f, 285.0f, 875.0f, 0 },   /* 修改 Modify（25,435,135,495）→ 诚实边界对话框 */
    { 305.0f, 815.0f, 415.0f, 875.0f, 1 },   /* 帮助 Help（155,435,265,495）→ 显示帮助文本 */
    { 435.0f, 815.0f, 545.0f, 875.0f, 2 },   /* 返回 Close（285,435,395,495）→ 回主菜单 */
};

const ui_profile_btn *ui_profile_buttons(void) { return g_profile_btns; }

int ui_profile_hit(const ui_profile_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_profile_popup_inside(float x, float y)
{
    return x >= UI_PROFILE_POPUP_X0 && x <= UI_PROFILE_POPUP_X0 + UI_PROFILE_POPUP_W &&
           y >= UI_PROFILE_POPUP_Y0 && y <= UI_PROFILE_POPUP_Y0 + UI_PROFILE_POPUP_H;
}

/* ★ P2（datah_042 证据）：提和详情弹窗 7 图标（提和/认输/聊天/系统设置/玩家资料/
   战况统计/软件信息）。几何 720×1280 基准：面板 450×285 居中（偏移+135,+497.5）；
   局部 1:1 平移（取整）：
   提和 15,25,120,160 → 150,522.5-255,657.5 → 150,523-255,658；
   认输 120,25,225,160 → 255,523-360,658；聊天 225,25,330,160 → 360,523-465,658；
   系统设置 330,25,435,160 → 465,523-570,658；
   玩家资料 15,150,120,285 → 150,647.5-255,782.5 → 150,648-255,783；
   战况统计 120,150,225,285 → 255,648-360,783；软件信息 225,150,330,285 → 360,648-465,783。
   业务：提和/认输/聊天 = 发 BT 帧 + 边界对话框（本地提和判定未实现，诚实边界）；
   系统设置 → UI_GAMESET；玩家资料 → UI_PROFILE；战况统计/软件信息 复用 ☰ 菜单逻辑。
   ★ 诚实边界：原版提和需对端应答（判定未实现），src_rs 离线单机无对端，
   提和/认输走 BT_T_DRAW/BT_T_RESIGN 帧 + 边界对话框，不包装成"已提和判定"。 */
static const ui_drawdetail_btn g_drawdetail_btns[UI_DRAWDETAIL_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_042 450 坐标系精确换算（面板 450×285 居中 720，偏移+135,+497.5，取整） */
    { 150.0f, 523.0f, 255.0f, 658.0f, 0 },  /* 提和 Draw（15,25）→ BT_T_DRAW 帧 + 边界对话框 */
    { 255.0f, 523.0f, 360.0f, 658.0f, 1 },  /* 认输 Resign（120,25）→ winner=对方 stage=OVER + BT_T_RESIGN */
    { 360.0f, 523.0f, 465.0f, 658.0f, 2 },  /* 聊天 Chat（225,25）→ BT_T_CHAT 帧 + 边界对话框 */
    { 465.0f, 523.0f, 570.0f, 658.0f, 3 },  /* 系统设置 Config（330,25）→ UI_GAMESET（datah_012） */
    { 150.0f, 648.0f, 255.0f, 783.0f, 4 },  /* 玩家资料 View（15,150）→ UI_PROFILE（datah_031） */
    { 255.0f, 648.0f, 360.0f, 783.0f, 5 },  /* 战况统计 Stat（120,150）→ 复用 ☰ 菜单遍历 cells 逻辑 */
    { 360.0f, 648.0f, 465.0f, 783.0f, 6 },  /* 软件信息 About（225,150）→ jq_show_dialog 版本（复用 ☰ 菜单逻辑） */
};

const ui_drawdetail_btn *ui_drawdetail_buttons(void) { return g_drawdetail_btns; }

int ui_drawdetail_hit(const ui_drawdetail_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_drawdetail_popup_inside(float x, float y)
{
    return x >= UI_DRAWDETAIL_POPUP_X0 && x <= UI_DRAWDETAIL_POPUP_X0 + UI_DRAWDETAIL_POPUP_W &&
           y >= UI_DRAWDETAIL_POPUP_Y0 && y <= UI_DRAWDETAIL_POPUP_Y0 + UI_DRAWDETAIL_POPUP_H;
}

/* ★ P2（datah_003 证据）：设置入口弹窗（完整版）3 按钮（图像/声音/游戏设置入口）。
    几何 720×1280 基准：面板 260×270 居中（偏移+230,+505）；局部 1:1 平移：
    图像设置 Sys 25,25,235,85 → 255,530-465,590；
    声音设置 Audio 25,105,235,165 → 255,610-465,670；
    游戏设置 Game 25,185,235,245 → 255,690-465,750。
    业务：☰ 菜单"功能设置"二级入口（原版 datah_003 设置总入口）——
    图像→UI_IMGSET7（datah_008 简化版，图像设置简化合并）；
    声音→UI_SETTINGS（datah_001 已实现）；游戏→UI_GAMESET（datah_012 已实现）。
    ★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），开关仅 UI 状态记录。 */
static const ui_setentry_btn g_setentry_btns[UI_SETEXTTRY_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_003 260 坐标系精确换算（面板 260×270 居中 720，偏移+230,+505） */
    { 255.0f, 530.0f, 465.0f, 590.0f, 0 },   /* 图像设置 Sys（25,25,235,85）→ UI_IMGSET7 */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },   /* 声音设置 Audio（25,105,235,165）→ UI_SETTINGS */
    { 255.0f, 690.0f, 465.0f, 750.0f, 2 },   /* 游戏设置 Game（25,185,235,245）→ UI_GAMESET */
};

const ui_setentry_btn *ui_setentry_buttons(void) { return g_setentry_btns; }

int ui_setentry_hit(const ui_setentry_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_setentry_popup_inside(float x, float y)
{
    return x >= UI_SETEXTTRY_POPUP_X0 && x <= UI_SETEXTTRY_POPUP_X0 + UI_SETEXTTRY_POPUP_W &&
           y >= UI_SETEXTTRY_POPUP_Y0 && y <= UI_SETEXTTRY_POPUP_Y0 + UI_SETEXTTRY_POPUP_H;
}

/* ★ P2（datah_004 证据）：设置入口（简化版）2 按钮（图像/声音，无游戏设置）。
    几何 720×1280 基准：面板 260×190 居中（偏移+230,+545）；局部 1:1 平移：
    图像设置 Sys 25,25,235,85 → 255,570-465,630；
    声音设置 Audio 25,105,235,165 → 255,650-465,710。
    业务：datah_004 是 datah_003 的简化版（联机态/蓝牙态设置入口，无游戏设置项）。
    ★ 诚实边界：图像设置渲染参数未实现，开关仅 UI 状态记录。 */
static const ui_setentry4_btn g_setentry4_btns[UI_SETEXTTRY4_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_004 260 坐标系精确换算（面板 260×190 居中 720，偏移+230,+545） */
    { 255.0f, 570.0f, 465.0f, 630.0f, 0 },   /* 图像设置 Sys（25,25,235,85）→ UI_IMGSET8 */
    { 255.0f, 650.0f, 465.0f, 710.0f, 1 },   /* 声音设置 Audio（25,105,235,165）→ UI_SETTINGS */
};

const ui_setentry4_btn *ui_setentry4_buttons(void) { return g_setentry4_btns; }

int ui_setentry4_hit(const ui_setentry4_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_setentry4_popup_inside(float x, float y)
{
    return x >= UI_SETEXTTRY4_POPUP_X0 && x <= UI_SETEXTTRY4_POPUP_X0 + UI_SETEXTTRY4_POPUP_W &&
           y >= UI_SETEXTTRY4_POPUP_Y0 && y <= UI_SETEXTTRY4_POPUP_Y0 + UI_SETEXTTRY4_POPUP_H;
}

/* ★ P2（datah_007 证据）：图像设置弹窗（完整版）6 控件（4 开关 + 确定/取消）。
    几何 720×1280 基准：面板 450×465 居中（偏移+135,+407.5）；局部 1:1 平移取整：
    界面动画 220,75,420,135 → 355,483-555,543；棋子动画 220,150,420,210 → 355,558-555,618；
    夜间模式 220,225,420,285 → 355,633-555,693；对方棋子 220,300,420,360 → 355,708-555,768；
    确定 30,380,210,440 → 165,788-345,848；取消 240,380,420,440 → 375,788-555,848。
    业务：图像设置 4 开关（各循环切换 开/关，对齐 datah_001/012 循环范式）+
    确定保存（ui_imgset_save，jq_imgset.txt 4 字段）+ 取消恢复。
    ★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
    开关只是 UI 状态记录（不实际改渲染）。 */
static const ui_imgset7_btn g_imgset7_btns[UI_IMGSET7_COUNT] = {
    /* x0, y0, x1, y1, action；★ 对抗审计（2026-11-11）：datah_008 面板 450×390 居中 origin=(135,445)
       （旧宏误用 datah_012 的 465/407.5 值，已纠正）。★ 诚实边界：第 4 开关"对方棋子"（action=3）
       非原版 XML 控件（datah_008 只有 3 开关），几何保留 Night 行下方原位区间，不删以保 ui_imgset_opp 业务链 */
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },   /* 界面动画 Trans（20,75,220,135+135,445）→ 开/关 循环 */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },   /* 棋子动画 PieceAni（20,150,220,210+135,445）→ 开/关 循环 */
    { 355.0f, 670.0f, 555.0f, 730.0f, 2 },   /* 夜间模式 Night（20,225,220,285+135,445）→ 开/关 循环 */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },   /* 对方棋子 Opposite（220,300,420,360）→ 开/关 循环 */
    { 165.0f, 750.0f, 345.0f, 810.0f, 4 },   /* 确定 Ok（30,385,210,445+135,445）→ 保存 + 回主菜单 */
    { 375.0f, 750.0f, 555.0f, 810.0f, 5 },   /* 取消 Cancel（240,385,420,445+135,445）→ 恢复 + 回主菜单 */
};

const ui_imgset7_btn *ui_imgset7_buttons(void) { return g_imgset7_btns; }

int ui_imgset7_hit(const ui_imgset7_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_imgset7_popup_inside(float x, float y)
{
    return x >= UI_IMGSET7_POPUP_X0 && x <= UI_IMGSET7_POPUP_X0 + UI_IMGSET7_POPUP_W &&
           y >= UI_IMGSET7_POPUP_Y0 && y <= UI_IMGSET7_POPUP_Y0 + UI_IMGSET7_POPUP_H;
}

/* ★ P2（datah_008 证据）：图像设置弹窗（简化版）5 控件（3 开关 + 确定/取消）。
    几何 720×1280 基准：面板 450×390 居中（偏移+135,+445）；局部 1:1 平移：
    界面动画 220,75,420,135 → 355,520-555,580；棋子动画 220,150,420,210 → 355,595-555,655；
    夜间模式 220,225,420,285 → 355,670-555,730；确定 30,305,210,365 → 165,750-345,810；
    取消 240,305,420,365 → 375,750-555,810。
    业务：datah_008 是 datah_007 简化版（3 开关，蓝牙态）。与 007 同范式。
    ★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
    开关只是 UI 状态记录（不实际改渲染）。 */
static const ui_imgset8_btn g_imgset8_btns[UI_IMGSET8_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_008 450 坐标系精确换算（面板 450×390 居中 720，偏移+135,+445） */
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },   /* 界面动画 Trans（220,75,420,135）→ 开/关 循环 */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },   /* 棋子动画 PieceAni（220,150,420,210）→ 开/关 循环 */
    { 355.0f, 670.0f, 555.0f, 730.0f, 2 },   /* 夜间模式 Night（220,225,420,285）→ 开/关 循环 */
    { 165.0f, 750.0f, 345.0f, 810.0f, 3 },   /* 确定 Ok（30,305,210,365）→ 保存 + 回主菜单 */
    { 375.0f, 750.0f, 555.0f, 810.0f, 4 },   /* 取消 Cancel（240,305,420,365）→ 恢复 + 回主菜单 */
};

const ui_imgset8_btn *ui_imgset8_buttons(void) { return g_imgset8_btns; }

int ui_imgset8_hit(const ui_imgset8_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_imgset8_popup_inside(float x, float y)
{
    return x >= UI_IMGSET8_POPUP_X0 && x <= UI_IMGSET8_POPUP_X0 + UI_IMGSET8_POPUP_W &&
           y >= UI_IMGSET8_POPUP_Y0 && y <= UI_IMGSET8_POPUP_Y0 + UI_IMGSET8_POPUP_H;
}

/* ★ P2（datah_065 证据）：加入游戏完整版弹窗 10 控件（8 显示项 + 开始/取消）。
    几何 720×1280 基准：面板 450×505 居中（偏移+135,+387.5）；局部 1:1 平移取整：
    先手 140,85,420,120 → 275,473-555,508；后手 140,125,420,160 → 275,513-555,548；
    玩法 140,165,420,200 → 275,553-555,588；明暗棋 140,205,420,240 → 275,593-555,628；
    和棋 140,245,420,280 → 275,633-555,668；大本营 140,285,420,320 → 275,673-555,708；
    吃地雷 140,325,420,360 → 275,713-555,748；吃军旗 140,365,420,400 → 275,753-555,788；
    开始 30,420,210,480 → 165,808-345,868；取消 240,420,420,480 → 375,808-555,868。
    业务：联机"加入游戏"完整版（8 项显示用 TransLabel 占位标签
    "蓝方/红方/布局/暗棋/开/开/开/开"），开始/取消 2 按钮可点。
    ★ 诚实边界：先手/后手/玩法/明暗棋/4 规则 = 联机开局配置（src_rs 无联机开局逻辑，
    显示占位标签，不画真数据，触摸 flaky 下输入框无意义）；
    开始 → 边界对话框（"加入游戏完整版：联机开局需 Wi-Fi/局域网房间协议，
    src_rs 仅蓝牙联机已实现，请选蓝牙联机"）+ 回主菜单；取消 → 回主菜单。 */
static const ui_join065_btn g_join065_btns[UI_JOIN065_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_065 450 坐标系精确换算（面板 450×505 居中 720，偏移+135,+387.5，取整） */
    { 275.0f, 473.0f, 555.0f, 508.0f, 0 },   /* 先手 Player1（140,85,420,120）→ 占位标签"蓝方" */
    { 275.0f, 513.0f, 555.0f, 548.0f, 1 },   /* 后手 Player2（140,125,420,160）→ 占位标签"红方" */
    { 275.0f, 553.0f, 555.0f, 588.0f, 2 },   /* 玩法 Lay（140,165,420,200）→ 占位标签"布局" */
    { 275.0f, 593.0f, 555.0f, 628.0f, 3 },   /* 明暗棋 Show（140,205,420,240）→ 占位标签"暗棋" */
    { 275.0f, 633.0f, 555.0f, 668.0f, 4 },   /* 和棋 Draw（140,245,420,280）→ 占位标签"开" */
    { 275.0f, 673.0f, 555.0f, 708.0f, 5 },   /* 大本营 CmdSafe（140,285,420,320）→ 占位标签"开" */
    { 275.0f, 713.0f, 555.0f, 748.0f, 6 },   /* 吃地雷 BingLei（140,325,420,360）→ 占位标签"开" */
    { 275.0f, 753.0f, 555.0f, 788.0f, 7 },   /* 吃军旗 BingQi（140,365,420,400）→ 占位标签"开" */
    { 165.0f, 808.0f, 345.0f, 868.0f, 8 },   /* 开始 Ok（30,420,210,480）→ 诚实边界对话框 + 回主菜单 */
    { 375.0f, 808.0f, 555.0f, 868.0f, 9 },   /* 取消 Cancel（240,420,420,480）→ 回主菜单 */
};

const ui_join065_btn *ui_join065_buttons(void) { return g_join065_btns; }

int ui_join065_hit(const ui_join065_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_join065_popup_inside(float x, float y)
{
    return x >= UI_JOIN065_POPUP_X0 && x <= UI_JOIN065_POPUP_X0 + UI_JOIN065_POPUP_W &&
           y >= UI_JOIN065_POPUP_Y0 && y <= UI_JOIN065_POPUP_Y0 + UI_JOIN065_POPUP_H;
}

/* ★ P2（datah_069 证据）：创建游戏完整版弹窗 6 控件（4 选择 + 确定/取消）。
    几何 720×1280 基准：面板 450×465 居中（偏移+135,+407.5）；局部 1:1 平移取整：
    先后手 220,75,420,135 → 355,483-555,543；玩法 220,150,420,210 → 355,558-555,618；
    明暗棋 220,225,420,285 → 355,633-555,693；游戏规则 220,300,420,360 → 355,708-555,768
    （text="设置"，点→UI_RULES070 游戏规则弹窗）；
    确定 30,385,210,445 → 165,793-345,853；取消 240,385,420,445 → 375,793-555,853。
    业务：联机"创建游戏"完整版（4 选择各循环切换当前值，对齐 datah_104 范式），
    确定→边界对话框（"创建游戏完整版：联机房间创建需 Wi-Fi/局域网协议，
    src_rs 仅蓝牙已实现"），取消→回主菜单。
    ★ 诚实边界：先后手/玩法/明暗棋 = 联机开局配置（src_rs 无联机房间创建逻辑，
    循环切换仅 UI 状态记录） */
static const ui_create069_btn g_create069_btns[UI_CREATE069_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_069 450 坐标系精确换算（面板 450×465 居中 720，偏移+135,+407.5，取整） */
    { 355.0f, 482.0f, 555.0f, 542.0f, 0 },   /* 先后手 First（20,75,220,135+135,407.5） */
    { 355.0f, 558.0f, 555.0f, 618.0f, 1 },   /* 玩法 Lay（220,150,420,210）→ 布局/翻棋 切换 */
    { 355.0f, 633.0f, 555.0f, 693.0f, 2 },   /* 明暗棋 Show（220,225,420,285）→ 明/暗 切换 */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },   /* 游戏规则 Rule（220,300,420,360）text="设置"→UI_RULES070 */
    { 165.0f, 793.0f, 345.0f, 853.0f, 4 },   /* 确定 Ok（30,385,210,445）→ 诚实边界对话框 + 回主菜单 */
    { 375.0f, 792.0f, 555.0f, 852.0f, 5 },   /* 取消 Cancel（240,385,420,445+135,407.5） */
};

const ui_create069_btn *ui_create069_buttons(void) { return g_create069_btns; }

int ui_create069_hit(const ui_create069_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_create069_popup_inside(float x, float y)
{
    return x >= UI_CREATE069_POPUP_X0 && x <= UI_CREATE069_POPUP_X0 + UI_CREATE069_POPUP_W &&
           y >= UI_CREATE069_POPUP_Y0 && y <= UI_CREATE069_POPUP_Y0 + UI_CREATE069_POPUP_H;
}

/* ★ P2（datah_070 证据）：游戏规则完整版弹窗 6 控件（4 规则开关 + 确定/取消）。
    与 datah_012 游戏设置同构，复用 ui_rule_*_get/set + 4 开关各循环切换
    + 确定保存（ui_rules_save）/取消恢复（ui_rules_load）。
    几何 720×1280 基准：面板 450×465 居中（偏移+135,+407.5）；局部 1:1 平移取整：
    和棋 220,75,420,135 → 355,483-555,543；大本营 220,150,420,210 → 355,558-555,618；
    吃地雷 220,225,420,285 → 355,633-555,693；吃军旗 220,300,420,360 → 355,708-555,768；
    确定 30,385,210,445 → 165,793-345,853；取消 240,385,420,445 → 375,793-555,853。
    业务：联机"游戏规则"弹窗（datah_069 创建游戏"游戏规则"按钮切此弹窗）。
    ★ 与 datah_012 区分：012 是本地 ☰ 菜单"功能设置"→UI_GAMESET，
    070 是联机"游戏规则"弹窗（datah_069 入口），复用同一组 ui_rule_* 开关。 */
static const ui_rules070_btn g_rules070_btns[UI_RULES070_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_070 450 坐标系精确换算（面板 450×465 居中 720，偏移+135,+407.5，取整） */
    { 355.0f, 482.0f, 555.0f, 542.0f, 0 },   /* 和棋 Draw 开关（35,75,235,135+135,407.5，datah_070） */
    { 355.0f, 558.0f, 555.0f, 618.0f, 1 },   /* 大本营 Command 开关（35,150,235,210+135,407.5） */
    { 355.0f, 632.0f, 555.0f, 692.0f, 2 },   /* 吃地雷 BingLei 开关（35,225,235,285+135,407.5） */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },   /* 吃军旗 BingQi 开关（35,300,235,360+135,407.5） */
    { 165.0f, 792.0f, 345.0f, 852.0f, 4 },   /* 确定 Ok（30,385,210,445+135,407.5） */
    { 375.0f, 792.0f, 555.0f, 852.0f, 5 },   /* 取消 Cancel（240,385,420,445+135,407.5） */
};

const ui_rules070_btn *ui_rules070_buttons(void) { return g_rules070_btns; }

int ui_rules070_hit(const ui_rules070_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_rules070_popup_inside(float x, float y)
{
    return x >= UI_RULES070_POPUP_X0 && x <= UI_RULES070_POPUP_X0 + UI_RULES070_POPUP_W &&
           y >= UI_RULES070_POPUP_Y0 && y <= UI_RULES070_POPUP_Y0 + UI_RULES070_POPUP_H;
}

/* ★ P2（datah_060 证据）：房间态弹窗 3 按钮（开始/换桌/离开）。
    几何 720×1280 基准：面板 450×352 居中（偏移+135,+464）；局部 1:1 平移：
    开始 Start 25,270,145,330 → 160,734-280,794；
    换桌 Change 165,270,285,330 → 300,734-420,794；
    离开 Cancel 305,270,425,330 → 440,734-560,794。
    Player1/Player2/Ready1/2/Score1/2/Rate1/2/View1/2 是 TransText 显示区（非按钮，
    简化为占位标签"蓝方/红方"）。
    业务：联机"房间态"（2 玩家房间列表 + 开始/换桌/离开），
    开始/换桌→边界对话框（"房间需 Wi-Fi/局域网协议"），离开→回主菜单。
    ★ 诚实边界：2 玩家显示区 = 占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
static const ui_room060_btn g_room060_btns[UI_ROOM060_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_060 450 坐标系精确换算（面板 450×352 居中 720，偏移+135,+464） */
    { 160.0f, 734.0f, 280.0f, 794.0f, 0 },   /* 开始 Start（25,270,145,330）→ 诚实边界对话框 */
    { 300.0f, 734.0f, 420.0f, 794.0f, 1 },   /* 换桌 Change（165,270,285,330）→ 诚实边界对话框 */
    { 440.0f, 734.0f, 560.0f, 794.0f, 2 },   /* 离开 Cancel（305,270,425,330）→ 回主菜单 */
};

const ui_room060_btn *ui_room060_buttons(void) { return g_room060_btns; }

int ui_room060_hit(const ui_room060_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_room060_popup_inside(float x, float y)
{
    return x >= UI_ROOM060_POPUP_X0 && x <= UI_ROOM060_POPUP_X0 + UI_ROOM060_POPUP_W &&
           y >= UI_ROOM060_POPUP_Y0 && y <= UI_ROOM060_POPUP_Y0 + UI_ROOM060_POPUP_H;
}

/* ★ P2（datah_063 证据）：聊天完整版弹窗 3 按钮（发送/记录/关闭）。
    几何 720×1280 基准：面板 420×325 居中（偏移+150,+477.5）；局部 1:1 平移取整：
    发送 Send 25,245,135,305 → 175,723-285,783；
    记录 Record 155,245,265,305 → 305,723-415,783；
    关闭 Cancel 285,245,395,305 → 435,723-545,783。
    Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"。
    业务：联机"聊天"完整版，发送→BT_T_CHAT 占位帧+边界对话框，
    记录→UI_CHATLOG（datah_029 聊天记录已实现），关闭→回对局。
    ★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
    无真实文本渲染） */
static const ui_chat063_btn g_chat063_btns[UI_CHAT063_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_063 420 坐标系精确换算（面板 420×325 居中 720，偏移+150,+477.5，取整） */
    { 175.0f, 723.0f, 285.0f, 783.0f, 0 },   /* 发送 Send（25,245,135,305）→ BT_T_CHAT 占位帧 + 边界对话框 */
    { 305.0f, 723.0f, 415.0f, 783.0f, 1 },   /* 记录 Record（155,245,265,305）→ UI_CHATLOG */
    { 435.0f, 723.0f, 545.0f, 783.0f, 2 },   /* 关闭 Cancel（285,245,395,305）→ 回对局 */
};

const ui_chat063_btn *ui_chat063_buttons(void) { return g_chat063_btns; }

int ui_chat063_hit(const ui_chat063_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_chat063_popup_inside(float x, float y)
{
    return x >= UI_CHAT063_POPUP_X0 && x <= UI_CHAT063_POPUP_X0 + UI_CHAT063_POPUP_W &&
           y >= UI_CHAT063_POPUP_Y0 && y <= UI_CHAT063_POPUP_Y0 + UI_CHAT063_POPUP_H;
}

/* ★ P2（datah_000 证据）：软件信息弹窗 2 按钮（反馈/关闭）。
   几何 720×1280 基准：面板 400×280 居中（偏移=(720-400)/2=160,(1280-280)/2=500）；
   局部 1:1 平移取整：
   反馈 Feedback 40,198,180,258 → 200,698-340,758；
   关闭 Ok 220,198,360,258 → 380,698-520,758。
   ★ 诚实边界：原版反馈=邮件/客服通道（未逆向），src_rs 无反馈通道，
   反馈按钮→诚实边界对话框，不包装成"已反馈"。 */
static const ui_about_btn g_about_btns[UI_ABOUT_COUNT] = {
    /* x0, y0, x1, y1, action；★ 原版 datah_000 400 坐标系精确换算（面板 400×280 居中 720，偏移+160,+500，取整） */
    { 200.0f, 698.0f, 340.0f, 758.0f, 0 },   /* 反馈 Feedback（40,198,180,258）→ 诚实边界对话框（原版反馈通道未逆向） */
    { 380.0f, 698.0f, 520.0f, 758.0f, 1 },   /* 关闭 Ok（220,198,360,258）→ 回对局/主菜单 */
};

const ui_about_btn *ui_about_buttons(void) { return g_about_btns; }

int ui_about_hit(const ui_about_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}

int ui_about_popup_inside(float x, float y)
{
    return x >= UI_ABOUT_POPUP_X0 && x <= UI_ABOUT_POPUP_X0 + UI_ABOUT_POPUP_W &&
           y >= UI_ABOUT_POPUP_Y0 && y <= UI_ABOUT_POPUP_Y0 + UI_ABOUT_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_044 证据）：战况统计·消息/排行 2 按钮。
   ★ 固定位置类（BackDlg 45,620，面板 180×175 局部平移，不再居中偏移；
   gen 按居中算（偏移 270,552）有误，手校正 45,620）：
   消息 Chat 20,20,160,80 → 65,640-205,700；
   排行 Rank 20,95,160,155 → 65,715-205,775。
   ★ 诚实边界：排行榜/消息数据未逆向，按钮→边界对话框占位。 */
static const ui_ui44_btn g_ui44_btns[UI_UI44_COUNT] = {
    { 290.0f, 572.0f, 430.0f, 632.0f, 0 },   /* 消息 Chat（20,20,160,80 + 居中 270,552.5）→ 边界对话框（排行榜数据未逆向） */
    { 290.0f, 648.0f, 430.0f, 708.0f, 1 },   /* 排行 Rank（20,95,160,155 + 居中 270,552.5）→ 边界对话框（排行数据未逆向） */
};

const ui_ui44_btn *ui_ui44_buttons(void) { return g_ui44_btns; }
int ui_ui44_hit(const ui_ui44_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui44_popup_inside(float x, float y)
{
    return x >= UI_UI44_POPUP_X0 && x <= UI_UI44_POPUP_X0 + UI_UI44_POPUP_W &&
           y >= UI_UI44_POPUP_Y0 && y <= UI_UI44_POPUP_Y0 + UI_UI44_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_045 证据）：战况统计·详细 1 按钮（BackDlg 固定位置 45,620，
   面板 w390,h135 局部平移；gen 居中换算有误，手校正；与 datah_100 同构）：
   详细 Detail 275,38,367,98 → 320,658-412,718。
   ★ 诚实边界：详细战绩数据未逆向。 */
static const ui_ui45_btn g_ui45_btns[UI_UI45_COUNT] = {
    { 320.0f, 658.0f, 412.0f, 718.0f, 0 },  /* 详细 Detail（275,38,367,98 + 45,620）→ 边界对话框（详细战绩未逆向） */
};

const ui_ui45_btn *ui_ui45_buttons(void) { return g_ui45_btns; }
int ui_ui45_hit(const ui_ui45_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui45_popup_inside(float x, float y)
{
    return x >= UI_UI45_POPUP_X0 && x <= UI_UI45_POPUP_X0 + UI_UI45_POPUP_W &&
           y >= UI_UI45_POPUP_Y0 && y <= UI_UI45_POPUP_Y0 + UI_UI45_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_046 证据）：玩家资料 1 按钮（420×420 居中，偏移+150,+430，
   局部 1:1 平移取整）：返回 Close 120,335,300,395 → 270,765-450,825。
   ★ 诚实边界：玩家资料内容未逆向（账号系统未实现），Info 占位标签。 */
static const ui_ui46_btn g_ui46_btns[UI_UI46_COUNT] = {
    { 270.0f, 765.0f, 450.0f, 825.0f, 0 },  /* 返回 Close（120,335,300,395）→ 回对局 */
};

const ui_ui46_btn *ui_ui46_buttons(void) { return g_ui46_btns; }
int ui_ui46_hit(const ui_ui46_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui46_popup_inside(float x, float y)
{
    return x >= UI_UI46_POPUP_X0 && x <= UI_UI46_POPUP_X0 + UI_UI46_POPUP_W &&
           y >= UI_UI46_POPUP_Y0 && y <= UI_UI46_POPUP_Y0 + UI_UI46_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_047 证据）：玩家列表 1 按钮（420×305 居中，偏移+150,+487.5，
   局部 1:1 平移取整）：关闭 Close 120,220,300,280 → 270,708-450,768。
   ★ 诚实边界：玩家列表未逆向，ListBox 占位标签。 */
static const ui_ui47_btn g_ui47_btns[UI_UI47_COUNT] = {
    { 270.0f, 708.0f, 450.0f, 768.0f, 0 },  /* 关闭 Close（120,220,300,280）→ 回对局 */
};

const ui_ui47_btn *ui_ui47_buttons(void) { return g_ui47_btns; }
int ui_ui47_hit(const ui_ui47_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui47_popup_inside(float x, float y)
{
    return x >= UI_UI47_POPUP_X0 && x <= UI_UI47_POPUP_X0 + UI_UI47_POPUP_W &&
           y >= UI_UI47_POPUP_Y0 && y <= UI_UI47_POPUP_Y0 + UI_UI47_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_049 证据）：对局详情 1 按钮（420×610 居中，偏移+150,+335，
   局部 1:1 平移取整）：关闭 Close 120,525,300,585 → 270,860-450,920。
   ★ 诚实边界：对局详情内容未逆向，ListBox 占位标签。 */
static const ui_ui49_btn g_ui49_btns[UI_UI49_COUNT] = {
    { 270.0f, 860.0f, 450.0f, 920.0f, 0 },  /* 关闭 Close（120,525,300,585）→ 回对局 */
};

const ui_ui49_btn *ui_ui49_buttons(void) { return g_ui49_btns; }
int ui_ui49_hit(const ui_ui49_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui49_popup_inside(float x, float y)
{
    return x >= UI_UI49_POPUP_X0 && x <= UI_UI49_POPUP_X0 + UI_UI49_POPUP_W &&
           y >= UI_UI49_POPUP_Y0 && y <= UI_UI49_POPUP_Y0 + UI_UI49_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_051 证据）：我的对局 2 按钮（450×610 居中，偏移+135,+335，
   局部 1:1 平移取整；与 datah_054/110 同构）：
   清空 Clear 45,525,202,585 → 180,860-337,920；
   关闭 Close 248,525,405,585 → 383,860-540,920。
   ★ 诚实边界：我的对局存档未逆向，清空=清复盘记录简化。 */
static const ui_ui51_btn g_ui51_btns[UI_UI51_COUNT] = {
    { 180.0f, 860.0f, 337.0f, 920.0f, 0 },  /* 清空 Clear（45,525,202,585）→ 清复盘记录（ui_replay_init） */
    { 383.0f, 860.0f, 540.0f, 920.0f, 1 },  /* 关闭 Close（248,525,405,585）→ 回对局 */
};

const ui_ui51_btn *ui_ui51_buttons(void) { return g_ui51_btns; }
int ui_ui51_hit(const ui_ui51_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui51_popup_inside(float x, float y)
{
    return x >= UI_UI51_POPUP_X0 && x <= UI_UI51_POPUP_X0 + UI_UI51_POPUP_W &&
           y >= UI_UI51_POPUP_Y0 && y <= UI_UI51_POPUP_Y0 + UI_UI51_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_054 证据）：历史对局 2 按钮（450×610 居中，偏移+135,+335，
   与 datah_051 同构）。★ 诚实边界：历史对局未逆向，清空=清复盘记录简化。 */
static const ui_ui54_btn g_ui54_btns[UI_UI54_COUNT] = {
    { 180.0f, 860.0f, 337.0f, 920.0f, 0 },  /* 清空 Clear（45,525,202,585）→ 清复盘记录（ui_replay_init） */
    { 383.0f, 860.0f, 540.0f, 920.0f, 1 },  /* 关闭 Close（248,525,405,585）→ 回对局 */
};

const ui_ui54_btn *ui_ui54_buttons(void) { return g_ui54_btns; }
int ui_ui54_hit(const ui_ui54_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui54_popup_inside(float x, float y)
{
    return x >= UI_UI54_POPUP_X0 && x <= UI_UI54_POPUP_X0 + UI_UI54_POPUP_W &&
           y >= UI_UI54_POPUP_Y0 && y <= UI_UI54_POPUP_Y0 + UI_UI54_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_073 证据）：创建游戏 2 按钮（450×270 居中，偏移+135,+505，
   局部 1:1 平移取整）：
   开始 Ok 30,190,210,250 → 165,695-345,755；
   取消 Cancel 240,190,420,250 → 375,695-555,755。
   ★ 诚实边界：联机创建游戏（Wi-Fi/局域网协议）未逆向，开始=本地开局简化。 */
static const ui_ui73_btn g_ui73_btns[UI_UI73_COUNT] = {
    { 165.0f, 695.0f, 345.0f, 755.0f, 0 },  /* 开始 Ok（30,190,210,250）→ 本地开局（jq_restart_game 简化） */
    { 375.0f, 695.0f, 555.0f, 755.0f, 1 },  /* 取消 Cancel（240,190,420,250）→ 回主菜单 */
};

const ui_ui73_btn *ui_ui73_buttons(void) { return g_ui73_btns; }
int ui_ui73_hit(const ui_ui73_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui73_popup_inside(float x, float y)
{
    return x >= UI_UI73_POPUP_X0 && x <= UI_UI73_POPUP_X0 + UI_UI73_POPUP_W &&
           y >= UI_UI73_POPUP_Y0 && y <= UI_UI73_POPUP_Y0 + UI_UI73_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_090 证据）：对战方式 2 按钮（★ 固定位置类 BackDlg 135,570，
   面板 w210,h150 局部平移，不再居中偏移；gen 按居中算（W=75 误读）有误，手校正）：
   人机 Comp 0,0,210,60 → 135,570-345,630；
   联网 Net 0,90,210,150 → 135,660-345,720。
   ★ 诚实边界：联网对战=Wi-Fi/局域网协议未逆向（src_rs 仅蓝牙 SPP）。 */
static const ui_ui90_btn g_ui90_btns[UI_UI90_COUNT] = {
    { 135.0f, 570.0f, 345.0f, 630.0f, 0 },  /* 人机对战 Comp（0,0,210,60 + 135,570）→ UI_DIFFICULTY */
    { 135.0f, 660.0f, 345.0f, 720.0f, 1 },  /* 联网对战 Net（0,90,210,150 + 135,570）→ 边界对话框 */
};

const ui_ui90_btn *ui_ui90_buttons(void) { return g_ui90_btns; }
int ui_ui90_hit(const ui_ui90_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui90_popup_inside(float x, float y)
{
    return x >= UI_UI90_POPUP_X0 && x <= UI_UI90_POPUP_X0 + UI_UI90_POPUP_W &&
           y >= UI_UI90_POPUP_Y0 && y <= UI_UI90_POPUP_Y0 + UI_UI90_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_100 证据）：详细战况 1 按钮（★ 固定位置类 BackDlg 45,620，
   与 datah_045 同构）：详细 Detail 275,38,367,98 → 320,658-412,718。
   ★ 诚实边界：详细战况数据未逆向。 */
static const ui_ui100_btn g_ui100_btns[UI_UI100_COUNT] = {
    { 320.0f, 658.0f, 412.0f, 718.0f, 0 },  /* 详细 Detail（275,38,367,98 + 45,620）→ 边界对话框（详细战况未逆向） */
};

const ui_ui100_btn *ui_ui100_buttons(void) { return g_ui100_btns; }
int ui_ui100_hit(const ui_ui100_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui100_popup_inside(float x, float y)
{
    return x >= UI_UI100_POPUP_X0 && x <= UI_UI100_POPUP_X0 + UI_UI100_POPUP_W &&
           y >= UI_UI100_POPUP_Y0 && y <= UI_UI100_POPUP_Y0 + UI_UI100_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_110 证据）：历史对局（联机版）2 按钮（450×610 居中，
   偏移+135,+335，与 datah_054 同构）。★ 诚实边界：历史对局未逆向。 */
static const ui_ui110_btn g_ui110_btns[UI_UI110_COUNT] = {
    { 180.0f, 860.0f, 337.0f, 920.0f, 0 },  /* 清空 Clear（45,525,202,585）→ 清复盘记录（ui_replay_init） */
    { 383.0f, 860.0f, 540.0f, 920.0f, 1 },  /* 关闭 Close（248,525,405,585）→ 回对局 */
};

const ui_ui110_btn *ui_ui110_buttons(void) { return g_ui110_btns; }
int ui_ui110_hit(const ui_ui110_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui110_popup_inside(float x, float y)
{
    return x >= UI_UI110_POPUP_X0 && x <= UI_UI110_POPUP_X0 + UI_UI110_POPUP_W &&
           y >= UI_UI110_POPUP_Y0 && y <= UI_UI110_POPUP_Y0 + UI_UI110_POPUP_H;
}

/* ★ SPAK2UI 批量（datah_118 证据）：版本更新 2 按钮（450×360 居中，偏移+135,+460，
   局部 1:1 平移取整）：
   更新 Ok 40,275,205,335 → 175,735-340,795；
   取消 Cancel 245,275,410,335 → 380,735-545,795。
   ★ 诚实边界：版本更新通道（下载/升级）未逆向。 */
static const ui_ui118_btn g_ui118_btns[UI_UI118_COUNT] = {
    { 175.0f, 735.0f, 340.0f, 795.0f, 0 },  /* 更新 Ok（40,275,205,335）→ 边界对话框（版本更新通道未逆向） */
    { 380.0f, 735.0f, 545.0f, 795.0f, 1 },  /* 取消 Cancel（245,275,410,335）→ 关闭回对局 */
};

const ui_ui118_btn *ui_ui118_buttons(void) { return g_ui118_btns; }
int ui_ui118_hit(const ui_ui118_btn *btns, int n, float x, float y)
{
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
int ui_ui118_popup_inside(float x, float y)
{
    return x >= UI_UI118_POPUP_X0 && x <= UI_UI118_POPUP_X0 + UI_UI118_POPUP_W &&
           y >= UI_UI118_POPUP_Y0 && y <= UI_UI118_POPUP_Y0 + UI_UI118_POPUP_H;
}

/* ★ 返回键 3 态确认弹窗按钮表（原版 datah_025 对齐：450×220 居中（偏移 +135,+530），
   确定 30,135,210,195 → 165,665,345,725；取消 240,135,420,195 → 375,665,555,725）
   ★ 2026-11-10 用户"返回键点击无效/这不是悔棋"指正：几何从旧 400×280 近似
   升级为 datah_025 精确（面板 + 按钮 + Msg 文本 + 确定/取消）。 */
static const ui_exitconfirm_btn g_exitconfirm_btns[UI_EXITCONFIRM_COUNT] = {
    { 165.0f, 665.0f, 345.0f, 725.0f, 0 },   /* 确定（退出对局） */
    { 375.0f, 665.0f, 555.0f, 725.0f, 1 },   /* 取消（继续对局） */
};
const ui_exitconfirm_btn *ui_exitconfirm_buttons(void) { return g_exitconfirm_btns; }
int ui_exitconfirm_hit(const ui_exitconfirm_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }

static const ui_resignconfirm_btn g_resignconfirm_btns[UI_RESIGNCONFIRM_COUNT] = {
    { 180.0f, 690.0f, 340.0f, 750.0f, 0 },   /* 认输 */
    { 380.0f, 690.0f, 540.0f, 750.0f, 1 },   /* 取消 */
};
const ui_resignconfirm_btn *ui_resignconfirm_buttons(void) { return g_resignconfirm_btns; }
int ui_resignconfirm_hit(const ui_resignconfirm_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }

static const ui_regretconfirm_btn g_regretconfirm_btns[UI_REGRETCONFIRM_COUNT] = {
    { 180.0f, 690.0f, 340.0f, 750.0f, 0 },   /* 悔棋 */
    { 380.0f, 690.0f, 540.0f, 750.0f, 1 },   /* 取消 */
};
const ui_regretconfirm_btn *ui_regretconfirm_buttons(void) { return g_regretconfirm_btns; }
int ui_regretconfirm_hit(const ui_regretconfirm_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }

/* ★ 批量对齐（datah_000）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d000_btn g_d000_btns[UI_D000_COUNT] = {
    { 200.0f, 698.0f, 340.0f, 758.0f, 0 },  /* 反馈 */
    { 380.0f, 698.0f, 520.0f, 758.0f, 1 },  /* 关闭 */
};
const ui_d000_btn *ui_d000_buttons(void) { return g_d000_btns; }
int ui_d000_hit(const ui_d000_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_001）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d001_btn g_d001_btns[UI_D001_COUNT] = {
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },  /* Sound */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },  /* BtnSound */
    { 255.0f, 677.0f, 562.0f, 725.0f, 2 },  /* ★ 音量滑条区（原版 Volume Slider 逐字节）→ 位置映射 1-3 */
    { 165.0f, 750.0f, 345.0f, 810.0f, 3 },  /* 确定 */
    { 375.0f, 750.0f, 555.0f, 810.0f, 4 },  /* 取消 */
};
const ui_d001_btn *ui_d001_buttons(void) { return g_d001_btns; }
int ui_d001_hit(const ui_d001_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_002）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d002_btn g_d002_btns[UI_D002_COUNT] = {
    { 152.0f, 601.0f, 568.0f, 649.0f, 0 },  /* ★ 亮度滑条区（原版 Bright Slider 逐字节）→ 位置映射 0-2 */
    { 160.0f, 675.0f, 345.0f, 735.0f, 1 },  /* 确定 */
    { 375.0f, 675.0f, 560.0f, 735.0f, 2 },  /* 取消 */
};
const ui_d002_btn *ui_d002_buttons(void) { return g_d002_btns; }
int ui_d002_hit(const ui_d002_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_003）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d003_btn g_d003_btns[UI_D003_COUNT] = {
    { 255.0f, 530.0f, 465.0f, 590.0f, 0 },  /* 图像设置 */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },  /* 声音设置 */
    { 255.0f, 690.0f, 465.0f, 750.0f, 2 },  /* 游戏设置 */
};
const ui_d003_btn *ui_d003_buttons(void) { return g_d003_btns; }
int ui_d003_hit(const ui_d003_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_004）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d004_btn g_d004_btns[UI_D004_COUNT] = {
    { 255.0f, 570.0f, 465.0f, 630.0f, 0 },  /* 图像设置 */
    { 255.0f, 650.0f, 465.0f, 710.0f, 1 },  /* 声音设置 */
};
const ui_d004_btn *ui_d004_buttons(void) { return g_d004_btns; }
int ui_d004_hit(const ui_d004_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_005）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d005_btn g_d005_btns[UI_D005_COUNT] = {
    { 113.0f, 415.0f, 175.0f, 460.0f, 0 },  /* Menu */
    { 303.0f, 415.0f, 365.0f, 460.0f, 1 },  /* Back */
};
const ui_d005_btn *ui_d005_buttons(void) { return g_d005_btns; }
int ui_d005_hit(const ui_d005_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_006）：8 按钮（720 基准，局部 1:1 平移） */
static const ui_d006_btn g_d006_btns[UI_D006_COUNT] = {
    { 150.0f, 522.0f, 255.0f, 658.0f, 0 },  /* 重新开始 */
    { 255.0f, 522.0f, 360.0f, 658.0f, 1 },  /* 悔棋 */
    { 360.0f, 522.0f, 465.0f, 658.0f, 2 },  /* 电脑变着 */
    { 465.0f, 522.0f, 570.0f, 658.0f, 3 },  /* 游戏规则 */
    { 150.0f, 648.0f, 255.0f, 782.0f, 4 },  /* 功能设置 */
    { 255.0f, 648.0f, 360.0f, 782.0f, 5 },  /* 战况统计 */
    { 360.0f, 648.0f, 465.0f, 782.0f, 6 },  /* 软件信息 */
    { 465.0f, 648.0f, 570.0f, 782.0f, 7 },  /* 退出程序 */
};
const ui_d006_btn *ui_d006_buttons(void) { return g_d006_btns; }
int ui_d006_hit(const ui_d006_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_007）：6 按钮（720 基准，局部 1:1 平移） */
static const ui_d007_btn g_d007_btns[UI_D007_COUNT] = {
    { 355.0f, 482.0f, 555.0f, 542.0f, 0 },  /* Trans */
    { 355.0f, 558.0f, 555.0f, 618.0f, 1 },  /* PieceAni */
    { 355.0f, 632.0f, 555.0f, 692.0f, 2 },  /* Night */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },  /* Opposite */
    { 165.0f, 788.0f, 345.0f, 848.0f, 4 },  /* 确定 */
    { 375.0f, 788.0f, 555.0f, 848.0f, 5 },  /* 取消 */
};
const ui_d007_btn *ui_d007_buttons(void) { return g_d007_btns; }
int ui_d007_hit(const ui_d007_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_008）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d008_btn g_d008_btns[UI_D008_COUNT] = {
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },  /* Trans */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },  /* PieceAni */
    { 355.0f, 670.0f, 555.0f, 730.0f, 2 },  /* Night */
    { 165.0f, 750.0f, 345.0f, 810.0f, 3 },  /* 确定 */
    { 375.0f, 750.0f, 555.0f, 810.0f, 4 },  /* 取消 */
};
const ui_d008_btn *ui_d008_buttons(void) { return g_d008_btns; }
int ui_d008_hit(const ui_d008_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_010）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d010_btn g_d010_btns[UI_D010_COUNT] = {
    { 200.0f, 726.0f, 340.0f, 786.0f, 0 },  /* 提交 */
    { 380.0f, 726.0f, 520.0f, 786.0f, 1 },  /* 取消 */
};
const ui_d010_btn *ui_d010_buttons(void) { return g_d010_btns; }
int ui_d010_hit(const ui_d010_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_012）：6 按钮（720 基准，局部 1:1 平移） */
static const ui_d012_btn g_d012_btns[UI_D012_COUNT] = {
    { 345.0f, 482.0f, 555.0f, 542.0f, 0 },  /* Draw */
    { 345.0f, 558.0f, 555.0f, 618.0f, 1 },  /* Command */
    { 345.0f, 632.0f, 555.0f, 692.0f, 2 },  /* BingLei */
    { 345.0f, 708.0f, 555.0f, 768.0f, 3 },  /* BingQi */
    { 165.0f, 792.0f, 345.0f, 852.0f, 4 },  /* 确定 */
    { 375.0f, 792.0f, 555.0f, 852.0f, 5 },  /* 取消 */
};
const ui_d012_btn *ui_d012_buttons(void) { return g_d012_btns; }
int ui_d012_hit(const ui_d012_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_013）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d013_btn g_d013_btns[UI_D013_COUNT] = {
    { 260.0f, 540.0f, 460.0f, 600.0f, 0 },  /* 0 */
    { 260.0f, 610.0f, 460.0f, 670.0f, 1 },  /* 1 */
    { 260.0f, 680.0f, 460.0f, 740.0f, 2 },  /* 2 */
};
const ui_d013_btn *ui_d013_buttons(void) { return g_d013_btns; }
int ui_d013_hit(const ui_d013_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_015）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d015_btn g_d015_btns[UI_D015_COUNT] = {
    { 303.0f, 415.0f, 365.0f, 460.0f, 0 },  /* Back */
};
const ui_d015_btn *ui_d015_buttons(void) { return g_d015_btns; }
int ui_d015_hit(const ui_d015_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_016）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d016_btn g_d016_btns[UI_D016_COUNT] = {
    { 505.0f, 408.0f, 551.0f, 454.0f, 0 },  /* Rename */
    { 185.0f, 792.0f, 289.0f, 852.0f, 1 },  /* 载入 */
    { 308.0f, 792.0f, 412.0f, 852.0f, 2 },  /* 删除 */
    { 431.0f, 792.0f, 535.0f, 852.0f, 3 },  /* 取消 */
};
const ui_d016_btn *ui_d016_buttons(void) { return g_d016_btns; }
int ui_d016_hit(const ui_d016_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_017）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d017_btn g_d017_btns[UI_D017_COUNT] = {
    { 280.0f, 828.0f, 440.0f, 888.0f, 0 },  /* 取消 */
};
const ui_d017_btn *ui_d017_buttons(void) { return g_d017_btns; }
int ui_d017_hit(const ui_d017_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_019）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d019_btn g_d019_btns[UI_D019_COUNT] = {
    { 150.0f, 455.0f, 255.0f, 590.0f, 0 },  /* 清空布局 */
    { 255.0f, 455.0f, 360.0f, 590.0f, 1 },  /* 载入布局 */
    { 360.0f, 455.0f, 465.0f, 590.0f, 2 },  /* 保存布局 */
    { 465.0f, 455.0f, 570.0f, 590.0f, 3 },  /* 完成布局 */
};
const ui_d019_btn *ui_d019_buttons(void) { return g_d019_btns; }
int ui_d019_hit(const ui_d019_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_020）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d020_btn g_d020_btns[UI_D020_COUNT] = {
    { 175.0f, 828.0f, 260.0f, 888.0f, 0 },  /* 保存 */
    { 270.0f, 828.0f, 355.0f, 888.0f, 1 },  /* 改名 */
    { 365.0f, 828.0f, 450.0f, 888.0f, 2 },  /* 删除 */
    { 460.0f, 828.0f, 545.0f, 888.0f, 3 },  /* 关闭 */
};
const ui_d020_btn *ui_d020_buttons(void) { return g_d020_btns; }
int ui_d020_hit(const ui_d020_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_021）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d021_btn g_d021_btns[UI_D021_COUNT] = {
    { 260.0f, 540.0f, 460.0f, 600.0f, 0 },  /* 0 */
    { 260.0f, 610.0f, 460.0f, 670.0f, 1 },  /* 1 */
    { 260.0f, 680.0f, 460.0f, 740.0f, 2 },  /* 2 */
};
const ui_d021_btn *ui_d021_buttons(void) { return g_d021_btns; }
int ui_d021_hit(const ui_d021_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_022）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d022_btn g_d022_btns[UI_D022_COUNT] = {
    { 260.0f, 470.0f, 460.0f, 530.0f, 0 },  /* 0 */
    { 260.0f, 540.0f, 460.0f, 600.0f, 1 },  /* 1 */
    { 260.0f, 610.0f, 460.0f, 670.0f, 2 },  /* 2 */
    { 260.0f, 680.0f, 460.0f, 740.0f, 3 },  /* 3 */
    { 260.0f, 750.0f, 460.0f, 810.0f, 4 },  /* 4 */
};
const ui_d022_btn *ui_d022_buttons(void) { return g_d022_btns; }
int ui_d022_hit(const ui_d022_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_023）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d023_btn g_d023_btns[UI_D023_COUNT] = {
    { 135.0f, 540.0f, 345.0f, 600.0f, 0 },  /* 人机对战 */
    { 135.0f, 615.0f, 345.0f, 675.0f, 1 },  /* 两人对战 */
    { 115.0f, 690.0f, 230.0f, 750.0f, 2 },  /* 联网 */
    { 250.0f, 690.0f, 365.0f, 750.0f, 3 },  /* 联机 */
};
const ui_d023_btn *ui_d023_buttons(void) { return g_d023_btns; }
int ui_d023_hit(const ui_d023_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_024）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d024_btn g_d024_btns[UI_D024_COUNT] = {
    { 388.0f, 698.0f, 458.0f, 762.0f, 0 },  /* Type */
};
const ui_d024_btn *ui_d024_buttons(void) { return g_d024_btns; }
int ui_d024_hit(const ui_d024_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_025）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d025_btn g_d025_btns[UI_D025_COUNT] = {
    { 165.0f, 665.0f, 345.0f, 725.0f, 0 },  /* 确定 Confirm（165,665,345,725；原版 Ok 270,665 为备用宽按钮，★ 2026-11-11 审计） */
};
const ui_d025_btn *ui_d025_buttons(void) { return g_d025_btns; }
int ui_d025_hit(const ui_d025_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_026）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d026_btn g_d026_btns[UI_D026_COUNT] = {
    { 355.0f, 595.0f, 555.0f, 655.0f, 0 },  /* Match */
    { 165.0f, 675.0f, 345.0f, 735.0f, 1 },  /* 确定 */
    { 375.0f, 675.0f, 555.0f, 735.0f, 2 },  /* 取消 */
};
const ui_d026_btn *ui_d026_buttons(void) { return g_d026_btns; }
int ui_d026_hit(const ui_d026_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_027）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d027_btn g_d027_btns[UI_D027_COUNT] = {
    { 260.0f, 540.0f, 460.0f, 600.0f, 0 },  /* 随机 */
    { 260.0f, 610.0f, 460.0f, 670.0f, 1 },  /* 默认 */
    { 260.0f, 680.0f, 460.0f, 740.0f, 2 },  /* 自定义 */
};
const ui_d027_btn *ui_d027_buttons(void) { return g_d027_btns; }
int ui_d027_hit(const ui_d027_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_028）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d028_btn g_d028_btns[UI_D028_COUNT] = {
    { 170.0f, 834.0f, 284.0f, 894.0f, 0 },  /* 常用语 */
    { 303.0f, 834.0f, 417.0f, 894.0f, 1 },  /* 记录 */
    { 436.0f, 834.0f, 550.0f, 894.0f, 2 },  /* 关闭 */
};
const ui_d028_btn *ui_d028_buttons(void) { return g_d028_btns; }
int ui_d028_hit(const ui_d028_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_029）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d029_btn g_d029_btns[UI_D029_COUNT] = {
    { 187.0f, 825.0f, 343.0f, 885.0f, 0 },  /* 清除 */
    { 377.0f, 825.0f, 533.0f, 885.0f, 1 },  /* 关闭 */
};
const ui_d029_btn *ui_d029_buttons(void) { return g_d029_btns; }
int ui_d029_hit(const ui_d029_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_030）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d030_btn g_d030_btns[UI_D030_COUNT] = {
    { 85.0f, 415.0f, 147.0f, 460.0f, 0 },  /* Menu */
    { 147.0f, 415.0f, 209.0f, 460.0f, 1 },  /* Chat */
};
const ui_d030_btn *ui_d030_buttons(void) { return g_d030_btns; }
int ui_d030_hit(const ui_d030_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_031）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d031_btn g_d031_btns[UI_D031_COUNT] = {
    { 175.0f, 815.0f, 285.0f, 875.0f, 0 },  /* 修改 */
    { 305.0f, 815.0f, 415.0f, 875.0f, 1 },  /* 帮助 */
    { 435.0f, 815.0f, 545.0f, 875.0f, 2 },  /* 返回 */
};
const ui_d031_btn *ui_d031_buttons(void) { return g_d031_btns; }
int ui_d031_hit(const ui_d031_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_032）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d032_btn g_d032_btns[UI_D032_COUNT] = {
    { 15.0f, 110.0f, 99.0f, 170.0f, 0 },  /* 退出 */
    { 110.0f, 110.0f, 194.0f, 170.0f, 1 },  /* 设置 */
    { 205.0f, 110.0f, 289.0f, 170.0f, 2 },  /* 用户 */
    { 300.0f, 110.0f, 384.0f, 170.0f, 3 },  /* 更多 */
};
const ui_d032_btn *ui_d032_buttons(void) { return g_d032_btns; }
int ui_d032_hit(const ui_d032_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_033）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d033_btn g_d033_btns[UI_D033_COUNT] = {
    { 200.0f, 726.0f, 340.0f, 786.0f, 0 },  /* 提交 */
    { 380.0f, 726.0f, 520.0f, 786.0f, 1 },  /* 取消 */
};
const ui_d033_btn *ui_d033_buttons(void) { return g_d033_btns; }
int ui_d033_hit(const ui_d033_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_034）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d034_btn g_d034_btns[UI_D034_COUNT] = {
    { 185.0f, 672.0f, 345.0f, 732.0f, 0 },  /* 确定 */
    { 375.0f, 672.0f, 535.0f, 732.0f, 1 },  /* 取消 */
};
const ui_d034_btn *ui_d034_buttons(void) { return g_d034_btns; }
int ui_d034_hit(const ui_d034_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_035）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d035_btn g_d035_btns[UI_D035_COUNT] = {
    { 185.0f, 672.0f, 345.0f, 732.0f, 0 },  /* 确定 */
    { 375.0f, 672.0f, 535.0f, 732.0f, 1 },  /* 取消 */
};
const ui_d035_btn *ui_d035_buttons(void) { return g_d035_btns; }
int ui_d035_hit(const ui_d035_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_036）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d036_btn g_d036_btns[UI_D036_COUNT] = {
    { 508.0f, 580.0f, 556.0f, 628.0f, 0 },  /* View1 */
    { 508.0f, 670.0f, 556.0f, 718.0f, 1 },  /* View2 */
    { 177.0f, 754.0f, 337.0f, 814.0f, 2 },  /* 开始 */
    { 383.0f, 754.0f, 543.0f, 814.0f, 3 },  /* 离开 */
};
const ui_d036_btn *ui_d036_buttons(void) { return g_d036_btns; }
int ui_d036_hit(const ui_d036_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_037）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d037_btn g_d037_btns[UI_D037_COUNT] = {
    { 255.0f, 530.0f, 465.0f, 590.0f, 0 },  /* 规则 - 翻棋（25,25,235,85+230,505） */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },  /* 规则 - 布局 */
    { 255.0f, 690.0f, 465.0f, 750.0f, 2 },  /* 返回 */
};
const ui_d037_btn *ui_d037_buttons(void) { return g_d037_btns; }
int ui_d037_hit(const ui_d037_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_038）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d038_btn g_d038_btns[UI_D038_COUNT] = {
    { 162.0f, 702.0f, 252.0f, 762.0f, 0 },  /* 登录 */
    { 264.0f, 702.0f, 354.0f, 762.0f, 1 },  /* 注册 */
    { 366.0f, 702.0f, 456.0f, 762.0f, 2 },  /* 游客 */
    { 468.0f, 702.0f, 558.0f, 762.0f, 3 },  /* 返回 */
};
const ui_d038_btn *ui_d038_buttons(void) { return g_d038_btns; }
int ui_d038_hit(const ui_d038_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_039）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d039_btn g_d039_btns[UI_D039_COUNT] = {
    { 42.0f, 665.0f, 132.0f, 725.0f, 0 },  /* 复盘 */
};
const ui_d039_btn *ui_d039_buttons(void) { return g_d039_btns; }
int ui_d039_hit(const ui_d039_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_041）：8 按钮（720 基准，局部 1:1 平移） */
static const ui_d041_btn g_d041_btns[UI_D041_COUNT] = {
    { 290.0f, 498.0f, 410.0f, 558.0f, 0 },  /* 布局 */
    { 435.0f, 498.0f, 555.0f, 558.0f, 1 },  /* 翻棋 */
    { 290.0f, 572.0f, 410.0f, 632.0f, 2 },  /* 布局 */
    { 435.0f, 572.0f, 555.0f, 632.0f, 3 },  /* 翻棋 */
    { 290.0f, 648.0f, 410.0f, 708.0f, 4 },  /* 布局 */
    { 435.0f, 648.0f, 555.0f, 708.0f, 5 },  /* 翻棋 */
    { 290.0f, 722.0f, 410.0f, 782.0f, 6 },  /* 布局 */
    { 435.0f, 722.0f, 555.0f, 782.0f, 7 },  /* 翻棋 */
};
const ui_d041_btn *ui_d041_buttons(void) { return g_d041_btns; }
int ui_d041_hit(const ui_d041_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_042）：7 按钮（720 基准，局部 1:1 平移） */
static const ui_d042_btn g_d042_btns[UI_D042_COUNT] = {
    { 150.0f, 522.0f, 255.0f, 658.0f, 0 },  /* 提和 */
    { 255.0f, 522.0f, 360.0f, 658.0f, 1 },  /* 认输 */
    { 360.0f, 522.0f, 465.0f, 658.0f, 2 },  /* 聊天 */
    { 465.0f, 522.0f, 570.0f, 658.0f, 3 },  /* 系统设置 */
    { 150.0f, 648.0f, 255.0f, 782.0f, 4 },  /* 玩家资料 */
    { 255.0f, 648.0f, 360.0f, 782.0f, 5 },  /* 战况统计 */
    { 360.0f, 648.0f, 465.0f, 782.0f, 6 },  /* 软件信息 */
};
const ui_d042_btn *ui_d042_buttons(void) { return g_d042_btns; }
int ui_d042_hit(const ui_d042_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_043）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d043_btn g_d043_btns[UI_D043_COUNT] = {
    { 185.0f, 672.0f, 345.0f, 732.0f, 0 },  /* 确定 */
    { 375.0f, 672.0f, 535.0f, 732.0f, 1 },  /* 取消 */
};
const ui_d043_btn *ui_d043_buttons(void) { return g_d043_btns; }
int ui_d043_hit(const ui_d043_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_044）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d044_btn g_d044_btns[UI_D044_COUNT] = {
    { 290.0f, 572.0f, 430.0f, 632.0f, 0 },  /* 消息 */
    { 290.0f, 648.0f, 430.0f, 708.0f, 1 },  /* 排行 */
};
const ui_d044_btn *ui_d044_buttons(void) { return g_d044_btns; }
int ui_d044_hit(const ui_d044_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_045）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d045_btn g_d045_btns[UI_D045_COUNT] = {
    { 320.0f, 658.0f, 412.0f, 718.0f, 0 },  /* 详细 */
};
const ui_d045_btn *ui_d045_buttons(void) { return g_d045_btns; }
int ui_d045_hit(const ui_d045_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_046）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d046_btn g_d046_btns[UI_D046_COUNT] = {
    { 270.0f, 765.0f, 450.0f, 825.0f, 0 },  /* 返回 */
};
const ui_d046_btn *ui_d046_buttons(void) { return g_d046_btns; }
int ui_d046_hit(const ui_d046_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_047）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d047_btn g_d047_btns[UI_D047_COUNT] = {
    { 270.0f, 708.0f, 450.0f, 768.0f, 0 },  /* 关闭 */
};
const ui_d047_btn *ui_d047_buttons(void) { return g_d047_btns; }
int ui_d047_hit(const ui_d047_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_049）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d049_btn g_d049_btns[UI_D049_COUNT] = {
    { 270.0f, 860.0f, 450.0f, 920.0f, 0 },  /* 关闭 */
};
const ui_d049_btn *ui_d049_buttons(void) { return g_d049_btns; }
int ui_d049_hit(const ui_d049_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_050）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d050_btn g_d050_btns[UI_D050_COUNT] = {
    { 175.0f, 745.0f, 285.0f, 805.0f, 0 },  /* 复盘 */
    { 305.0f, 745.0f, 415.0f, 805.0f, 1 },  /* 删除 */
    { 435.0f, 745.0f, 545.0f, 805.0f, 2 },  /* 返回 */
};
const ui_d050_btn *ui_d050_buttons(void) { return g_d050_btns; }
int ui_d050_hit(const ui_d050_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_051）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d051_btn g_d051_btns[UI_D051_COUNT] = {
    { 180.0f, 860.0f, 337.0f, 920.0f, 0 },  /* 清空 */
    { 383.0f, 860.0f, 540.0f, 920.0f, 1 },  /* 关闭 */
};
const ui_d051_btn *ui_d051_buttons(void) { return g_d051_btns; }
int ui_d051_hit(const ui_d051_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_052）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d052_btn g_d052_btns[UI_D052_COUNT] = {
    { 180.0f, 665.0f, 345.0f, 725.0f, 0 },  /* 重试 */
    { 375.0f, 665.0f, 540.0f, 725.0f, 1 },  /* 取消 */
};
const ui_d052_btn *ui_d052_buttons(void) { return g_d052_btns; }
int ui_d052_hit(const ui_d052_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_053）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d053_btn g_d053_btns[UI_D053_COUNT] = {
    { 185.0f, 784.0f, 345.0f, 844.0f, 0 },  /* 注册 */
    { 375.0f, 784.0f, 535.0f, 844.0f, 1 },  /* 取消 */
};
const ui_d053_btn *ui_d053_buttons(void) { return g_d053_btns; }
int ui_d053_hit(const ui_d053_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_054）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d054_btn g_d054_btns[UI_D054_COUNT] = {
    { 180.0f, 860.0f, 337.0f, 920.0f, 0 },  /* 清空 */
    { 383.0f, 860.0f, 540.0f, 920.0f, 1 },  /* 关闭 */
};
const ui_d054_btn *ui_d054_buttons(void) { return g_d054_btns; }
int ui_d054_hit(const ui_d054_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_055）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d055_btn g_d055_btns[UI_D055_COUNT] = {
    { 268.0f, 737.0f, 452.0f, 797.0f, 0 },  /* 确定 */
};
const ui_d055_btn *ui_d055_buttons(void) { return g_d055_btns; }
int ui_d055_hit(const ui_d055_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_057）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d057_btn g_d057_btns[UI_D057_COUNT] = {
    { 185.0f, 715.0f, 344.0f, 775.0f, 0 },  /* 修改密码 */
    { 376.0f, 715.0f, 535.0f, 775.0f, 1 },  /* 返回 */
};
const ui_d057_btn *ui_d057_buttons(void) { return g_d057_btns; }
int ui_d057_hit(const ui_d057_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_058）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d058_btn g_d058_btns[UI_D058_COUNT] = {
    { 185.0f, 734.0f, 344.0f, 794.0f, 0 },  /* 确定 */
    { 376.0f, 734.0f, 535.0f, 794.0f, 1 },  /* 取消 */
};
const ui_d058_btn *ui_d058_buttons(void) { return g_d058_btns; }
int ui_d058_hit(const ui_d058_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_059）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d059_btn g_d059_btns[UI_D059_COUNT] = {
    { 410.0f, 544.0f, 520.0f, 604.0f, 0 },  /* 换一张 */
    { 185.0f, 734.0f, 344.0f, 794.0f, 1 },  /* 确定 */
    { 376.0f, 734.0f, 535.0f, 794.0f, 2 },  /* 取消 */
};
const ui_d059_btn *ui_d059_buttons(void) { return g_d059_btns; }
int ui_d059_hit(const ui_d059_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_060）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d060_btn g_d060_btns[UI_D060_COUNT] = {
    { 508.0f, 560.0f, 556.0f, 608.0f, 0 },  /* View1 */
    { 508.0f, 650.0f, 556.0f, 698.0f, 1 },  /* View2 */
    { 160.0f, 734.0f, 280.0f, 794.0f, 2 },  /* 开始 */
    { 300.0f, 734.0f, 420.0f, 794.0f, 3 },  /* 换桌 */
    { 440.0f, 734.0f, 560.0f, 794.0f, 4 },  /* 离开 */
};
const ui_d060_btn *ui_d060_buttons(void) { return g_d060_btns; }
int ui_d060_hit(const ui_d060_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_061）：6 按钮（720 基准，局部 1:1 平移） */
static const ui_d061_btn g_d061_btns[UI_D061_COUNT] = {
    { 355.0f, 482.0f, 555.0f, 542.0f, 0 },  /* First */
    { 355.0f, 558.0f, 555.0f, 618.0f, 1 },  /* Level */
    { 355.0f, 632.0f, 555.0f, 692.0f, 2 },  /* Lay */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },  /* Show */
    { 165.0f, 792.0f, 345.0f, 852.0f, 4 },  /* 确定 */
    { 375.0f, 792.0f, 555.0f, 852.0f, 5 },  /* 取消 */
};
const ui_d061_btn *ui_d061_buttons(void) { return g_d061_btns; }
int ui_d061_hit(const ui_d061_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_062）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d062_btn g_d062_btns[UI_D062_COUNT] = {
    { 355.0f, 558.0f, 555.0f, 618.0f, 0 },  /* Lay */
    { 355.0f, 632.0f, 555.0f, 692.0f, 1 },  /* Show */
    { 165.0f, 718.0f, 345.0f, 778.0f, 2 },  /* 确定 */
    { 375.0f, 718.0f, 555.0f, 778.0f, 3 },  /* 取消 */
};
const ui_d062_btn *ui_d062_buttons(void) { return g_d062_btns; }
int ui_d062_hit(const ui_d062_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_063）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d063_btn g_d063_btns[UI_D063_COUNT] = {
    { 175.0f, 722.0f, 285.0f, 782.0f, 0 },  /* 发送 */
    { 305.0f, 722.0f, 415.0f, 782.0f, 1 },  /* 记录 */
    { 435.0f, 722.0f, 545.0f, 782.0f, 2 },  /* 关闭 */
};
const ui_d063_btn *ui_d063_buttons(void) { return g_d063_btns; }
int ui_d063_hit(const ui_d063_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_064）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d064_btn g_d064_btns[UI_D064_COUNT] = {
    { 85.0f, 415.0f, 147.0f, 460.0f, 0 },  /* Menu */
    { 147.0f, 415.0f, 209.0f, 460.0f, 1 },  /* Chat */
    { 303.0f, 415.0f, 365.0f, 460.0f, 2 },  /* Back */
};
const ui_d064_btn *ui_d064_buttons(void) { return g_d064_btns; }
int ui_d064_hit(const ui_d064_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_065）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d065_btn g_d065_btns[UI_D065_COUNT] = {
    { 165.0f, 808.0f, 345.0f, 868.0f, 0 },  /* 开始 */
    { 375.0f, 808.0f, 555.0f, 868.0f, 1 },  /* 取消 */
};
const ui_d065_btn *ui_d065_buttons(void) { return g_d065_btns; }
int ui_d065_hit(const ui_d065_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_066）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d066_btn g_d066_btns[UI_D066_COUNT] = {
    { 255.0f, 535.0f, 465.0f, 595.0f, 0 },  /* 创建游戏 */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },  /* 加入游戏 */
    { 255.0f, 685.0f, 465.0f, 745.0f, 2 },  /* 返回 */
};
const ui_d066_btn *ui_d066_buttons(void) { return g_d066_btns; }
int ui_d066_hit(const ui_d066_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_067）：8 按钮（720 基准，局部 1:1 平移） */
static const ui_d067_btn g_d067_btns[UI_D067_COUNT] = {
    { 150.0f, 522.0f, 255.0f, 658.0f, 0 },  /* 重新开始 */
    { 255.0f, 522.0f, 360.0f, 658.0f, 1 },  /* 悔棋 */
    { 360.0f, 522.0f, 465.0f, 658.0f, 2 },  /* 提和 */
    { 465.0f, 522.0f, 570.0f, 658.0f, 3 },  /* 认输 */
    { 150.0f, 648.0f, 255.0f, 782.0f, 4 },  /* 聊天 */
    { 255.0f, 648.0f, 360.0f, 782.0f, 5 },  /* 系统设置 */
    { 360.0f, 648.0f, 465.0f, 782.0f, 6 },  /* 战况统计 */
    { 465.0f, 648.0f, 570.0f, 782.0f, 7 },  /* 游戏规则 */
};
const ui_d067_btn *ui_d067_buttons(void) { return g_d067_btns; }
int ui_d067_hit(const ui_d067_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_068）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d068_btn g_d068_btns[UI_D068_COUNT] = {
    { 175.0f, 672.0f, 285.0f, 732.0f, 0 },  /* 确定 */
    { 305.0f, 672.0f, 415.0f, 732.0f, 1 },  /* 设备名 */
    { 435.0f, 672.0f, 545.0f, 732.0f, 2 },  /* 取消 */
};
const ui_d068_btn *ui_d068_buttons(void) { return g_d068_btns; }
int ui_d068_hit(const ui_d068_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_069）：6 按钮（720 基准，局部 1:1 平移） */
static const ui_d069_btn g_d069_btns[UI_D069_COUNT] = {
    { 355.0f, 482.0f, 555.0f, 542.0f, 0 },  /* First */
    { 355.0f, 558.0f, 555.0f, 618.0f, 1 },  /* Lay */
    { 355.0f, 632.0f, 555.0f, 692.0f, 2 },  /* Show */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },  /* 设置 */
    { 165.0f, 792.0f, 345.0f, 852.0f, 4 },  /* 确定 */
    { 375.0f, 792.0f, 555.0f, 852.0f, 5 },  /* 取消 */
};
const ui_d069_btn *ui_d069_buttons(void) { return g_d069_btns; }
int ui_d069_hit(const ui_d069_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_070）：6 按钮（720 基准，局部 1:1 平移） */
static const ui_d070_btn g_d070_btns[UI_D070_COUNT] = {
    { 355.0f, 482.0f, 555.0f, 542.0f, 0 },  /* Draw */
    { 355.0f, 558.0f, 555.0f, 618.0f, 1 },  /* Command */
    { 355.0f, 632.0f, 555.0f, 692.0f, 2 },  /* BingLei */
    { 355.0f, 708.0f, 555.0f, 768.0f, 3 },  /* BingQi */
    { 165.0f, 792.0f, 345.0f, 852.0f, 4 },  /* 确定 */
    { 375.0f, 792.0f, 555.0f, 852.0f, 5 },  /* 取消 */
};
const ui_d070_btn *ui_d070_buttons(void) { return g_d070_btns; }
int ui_d070_hit(const ui_d070_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_071）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d071_btn g_d071_btns[UI_D071_COUNT] = {
    { 195.0f, 808.0f, 345.0f, 868.0f, 0 },  /* 搜索 */
    { 375.0f, 808.0f, 525.0f, 868.0f, 1 },  /* 取消 */
};
const ui_d071_btn *ui_d071_buttons(void) { return g_d071_btns; }
int ui_d071_hit(const ui_d071_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_072）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d072_btn g_d072_btns[UI_D072_COUNT] = {
    { 255.0f, 498.0f, 465.0f, 558.0f, 0 },  /* 蓝牙联机 */
    { 255.0f, 572.0f, 465.0f, 632.0f, 1 },  /* Wi-Fi 联机 */
    { 255.0f, 648.0f, 465.0f, 708.0f, 2 },  /* 个人资料（25,175,235,235+235,477.5） */
    { 255.0f, 722.0f, 465.0f, 782.0f, 3 },  /* 返回 */
};
const ui_d072_btn *ui_d072_buttons(void) { return g_d072_btns; }
int ui_d072_hit(const ui_d072_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_073）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d073_btn g_d073_btns[UI_D073_COUNT] = {
    { 165.0f, 695.0f, 345.0f, 755.0f, 0 },  /* 开始 */
    { 375.0f, 695.0f, 555.0f, 755.0f, 1 },  /* 取消 */
};
const ui_d073_btn *ui_d073_buttons(void) { return g_d073_btns; }
int ui_d073_hit(const ui_d073_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_075）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d075_btn g_d075_btns[UI_D075_COUNT] = {
    { 60.0f, 415.0f, 122.0f, 460.0f, 0 },  /* Prev */
    { 169.0f, 415.0f, 231.0f, 460.0f, 1 },  /* Next */
    { 275.0f, 415.0f, 337.0f, 460.0f, 2 },  /* Menu */
    { 337.0f, 415.0f, 399.0f, 460.0f, 3 },  /* Back */
};
const ui_d075_btn *ui_d075_buttons(void) { return g_d075_btns; }
int ui_d075_hit(const ui_d075_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_077）：6 按钮（720 基准，局部 1:1 平移） */
static const ui_d077_btn g_d077_btns[UI_D077_COUNT] = {
    { 150.0f, 522.0f, 255.0f, 658.0f, 0 },  /* 开局 */
    { 255.0f, 522.0f, 360.0f, 658.0f, 1 },  /* 终局 */
    { 360.0f, 522.0f, 465.0f, 658.0f, 2 },  /* 切换显示 */
    { 465.0f, 522.0f, 570.0f, 658.0f, 3 },  /* 对局信息 */
    { 150.0f, 648.0f, 255.0f, 782.0f, 4 },  /* 功能设置 */
    { 255.0f, 648.0f, 360.0f, 782.0f, 5 },  /* 软件信息 */
};
const ui_d077_btn *ui_d077_buttons(void) { return g_d077_btns; }
int ui_d077_hit(const ui_d077_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_078）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d078_btn g_d078_btns[UI_D078_COUNT] = {
    { 160.0f, 665.0f, 280.0f, 725.0f, 0 },  /* 新局 */
    { 300.0f, 665.0f, 420.0f, 725.0f, 1 },  /* 复盘 */
    { 440.0f, 665.0f, 560.0f, 725.0f, 2 },  /* 关闭 */
};
const ui_d078_btn *ui_d078_buttons(void) { return g_d078_btns; }
int ui_d078_hit(const ui_d078_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_079）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d079_btn g_d079_btns[UI_D079_COUNT] = {
    { 270.0f, 900.0f, 450.0f, 960.0f, 0 },  /* 确定 */
};
const ui_d079_btn *ui_d079_buttons(void) { return g_d079_btns; }
int ui_d079_hit(const ui_d079_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_082）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d082_btn g_d082_btns[UI_D082_COUNT] = {
    { 10.0f, 705.0f, 94.0f, 765.0f, 0 },  /* 菜单 */
    { 104.0f, 705.0f, 188.0f, 765.0f, 1 },  /* 新局 */
    { 198.0f, 705.0f, 282.0f, 765.0f, 2 },  /* 悔棋 */
    { 292.0f, 705.0f, 376.0f, 765.0f, 3 },  /* 设置 */
    { 386.0f, 705.0f, 470.0f, 765.0f, 4 },  /* 返回 */
};
const ui_d082_btn *ui_d082_buttons(void) { return g_d082_btns; }
int ui_d082_hit(const ui_d082_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_083）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d083_btn g_d083_btns[UI_D083_COUNT] = {
    { 150.0f, 588.0f, 255.0f, 724.0f, 0 },  /* 游戏规则 */
    { 255.0f, 588.0f, 360.0f, 724.0f, 1 },  /* 功能设置 */
    { 360.0f, 588.0f, 465.0f, 724.0f, 2 },  /* 软件信息 */
    { 465.0f, 588.0f, 570.0f, 724.0f, 3 },  /* 退出程序 */
};
const ui_d083_btn *ui_d083_buttons(void) { return g_d083_btns; }
int ui_d083_hit(const ui_d083_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_084）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d084_btn g_d084_btns[UI_D084_COUNT] = {
    { 355.0f, 558.0f, 555.0f, 618.0f, 0 },  /* Expand */
    { 355.0f, 632.0f, 555.0f, 692.0f, 1 },  /* Clockwise */
    { 165.0f, 718.0f, 345.0f, 778.0f, 2 },  /* 确定 */
    { 375.0f, 718.0f, 555.0f, 778.0f, 3 },  /* 取消 */
};
const ui_d084_btn *ui_d084_buttons(void) { return g_d084_btns; }
int ui_d084_hit(const ui_d084_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_085）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d085_btn g_d085_btns[UI_D085_COUNT] = {
    { 10.0f, 705.0f, 94.0f, 765.0f, 0 },  /* 载入 */
    { 104.0f, 705.0f, 188.0f, 765.0f, 1 },  /* 保存 */
    { 198.0f, 705.0f, 282.0f, 765.0f, 2 },  /* 清空 */
    { 292.0f, 705.0f, 376.0f, 765.0f, 3 },  /* 完成 */
    { 386.0f, 705.0f, 470.0f, 765.0f, 4 },  /* 取消 */
};
const ui_d085_btn *ui_d085_buttons(void) { return g_d085_btns; }
int ui_d085_hit(const ui_d085_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_086）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d086_btn g_d086_btns[UI_D086_COUNT] = {
    { 485.0f, 402.0f, 531.0f, 448.0f, 0 },  /* Rename */
    { 201.0f, 798.0f, 295.0f, 858.0f, 1 },  /* 载入 */
    { 313.0f, 798.0f, 407.0f, 858.0f, 2 },  /* 删除 */
    { 425.0f, 798.0f, 519.0f, 858.0f, 3 },  /* 取消 */
};
const ui_d086_btn *ui_d086_buttons(void) { return g_d086_btns; }
int ui_d086_hit(const ui_d086_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_089）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d089_btn g_d089_btns[UI_D089_COUNT] = {
    { 260.0f, 540.0f, 460.0f, 600.0f, 0 },  /* 2 */
    { 260.0f, 610.0f, 460.0f, 670.0f, 1 },  /* 3 */
    { 260.0f, 680.0f, 460.0f, 740.0f, 2 },  /* 4 */
};
const ui_d089_btn *ui_d089_buttons(void) { return g_d089_btns; }
int ui_d089_hit(const ui_d089_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_090）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d090_btn g_d090_btns[UI_D090_COUNT] = {
    { 135.0f, 570.0f, 345.0f, 630.0f, 0 },  /* 人机对战 */
    { 135.0f, 660.0f, 345.0f, 720.0f, 1 },  /* 联网对战 */
};
const ui_d090_btn *ui_d090_buttons(void) { return g_d090_btns; }
int ui_d090_hit(const ui_d090_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_094）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d094_btn g_d094_btns[UI_D094_COUNT] = {
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },  /* Expand */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },  /* Clockwise */
    { 355.0f, 670.0f, 555.0f, 730.0f, 2 },  /* Chat */
    { 165.0f, 750.0f, 345.0f, 810.0f, 3 },  /* 确定 */
    { 375.0f, 750.0f, 555.0f, 810.0f, 4 },  /* 取消 */
};
const ui_d094_btn *ui_d094_buttons(void) { return g_d094_btns; }
int ui_d094_hit(const ui_d094_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_095）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d095_btn g_d095_btns[UI_D095_COUNT] = {
    { 10.0f, 705.0f, 94.0f, 765.0f, 0 },  /* 菜单 */
    { 104.0f, 705.0f, 188.0f, 765.0f, 1 },  /* 复盘 */
    { 198.0f, 705.0f, 282.0f, 765.0f, 2 },  /* 设置 */
    { 292.0f, 705.0f, 376.0f, 765.0f, 3 },  /* 投降 */
    { 386.0f, 705.0f, 470.0f, 765.0f, 4 },  /* 提和 */
};
const ui_d095_btn *ui_d095_buttons(void) { return g_d095_btns; }
int ui_d095_hit(const ui_d095_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_096）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d096_btn g_d096_btns[UI_D096_COUNT] = {
    { 175.0f, 748.0f, 285.0f, 808.0f, 0 },  /* 修改 */
    { 305.0f, 748.0f, 415.0f, 808.0f, 1 },  /* 帮助 */
    { 435.0f, 748.0f, 545.0f, 808.0f, 2 },  /* 返回 */
};
const ui_d096_btn *ui_d096_buttons(void) { return g_d096_btns; }
int ui_d096_hit(const ui_d096_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_097）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d097_btn g_d097_btns[UI_D097_COUNT] = {
    { 15.0f, 110.0f, 99.0f, 170.0f, 0 },  /* 退出 */
    { 110.0f, 110.0f, 194.0f, 170.0f, 1 },  /* 设置 */
    { 205.0f, 110.0f, 289.0f, 170.0f, 2 },  /* 消息 */
    { 300.0f, 110.0f, 384.0f, 170.0f, 3 },  /* 用户 */
};
const ui_d097_btn *ui_d097_buttons(void) { return g_d097_btns; }
int ui_d097_hit(const ui_d097_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_098）：6 按钮（720 基准，局部 1:1 平移） */
static const ui_d098_btn g_d098_btns[UI_D098_COUNT] = {
    { 290.0f, 535.0f, 410.0f, 595.0f, 0 },  /* 四暗 */
    { 435.0f, 535.0f, 555.0f, 595.0f, 1 },  /* 双明 */
    { 290.0f, 610.0f, 410.0f, 670.0f, 2 },  /* 四暗 */
    { 435.0f, 610.0f, 555.0f, 670.0f, 3 },  /* 双明 */
    { 290.0f, 685.0f, 410.0f, 745.0f, 4 },  /* 四暗 */
    { 435.0f, 685.0f, 555.0f, 745.0f, 5 },  /* 双明 */
};
const ui_d098_btn *ui_d098_buttons(void) { return g_d098_btns; }
int ui_d098_hit(const ui_d098_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_099）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d099_btn g_d099_btns[UI_D099_COUNT] = {
    { 150.0f, 588.0f, 255.0f, 724.0f, 0 },  /* 对局信息 */
    { 255.0f, 588.0f, 360.0f, 724.0f, 1 },  /* 游戏规则 */
    { 360.0f, 588.0f, 465.0f, 724.0f, 2 },  /* 功能设置 */
    { 465.0f, 588.0f, 570.0f, 724.0f, 3 },  /* 软件信息 */
};
const ui_d099_btn *ui_d099_buttons(void) { return g_d099_btns; }
int ui_d099_hit(const ui_d099_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_100）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d100_btn g_d100_btns[UI_D100_COUNT] = {
    { 320.0f, 658.0f, 412.0f, 718.0f, 0 },  /* 详细 */
};
const ui_d100_btn *ui_d100_buttons(void) { return g_d100_btns; }
int ui_d100_hit(const ui_d100_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_101）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d101_btn g_d101_btns[UI_D101_COUNT] = {
    { 10.0f, 705.0f, 94.0f, 765.0f, 0 },  /* 布局 */
    { 104.0f, 705.0f, 188.0f, 765.0f, 1 },  /* 出征 */
    { 198.0f, 705.0f, 282.0f, 765.0f, 2 },  /* 取消 */
    { 386.0f, 705.0f, 470.0f, 765.0f, 3 },  /* 返回 */
};
const ui_d101_btn *ui_d101_buttons(void) { return g_d101_btns; }
int ui_d101_hit(const ui_d101_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_102）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d102_btn g_d102_btns[UI_D102_COUNT] = {
    { 268.0f, 742.0f, 452.0f, 802.0f, 0 },  /* 确定 */
};
const ui_d102_btn *ui_d102_buttons(void) { return g_d102_btns; }
int ui_d102_hit(const ui_d102_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_104）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d104_btn g_d104_btns[UI_D104_COUNT] = {
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },  /* Me */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },  /* Difficulty */
    { 355.0f, 670.0f, 555.0f, 730.0f, 2 },  /* Type */
    { 165.0f, 755.0f, 345.0f, 815.0f, 3 },  /* 确定 */
    { 375.0f, 755.0f, 555.0f, 815.0f, 4 },  /* 取消 */
};
const ui_d104_btn *ui_d104_buttons(void) { return g_d104_btns; }
int ui_d104_hit(const ui_d104_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_107）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d107_btn g_d107_btns[UI_D107_COUNT] = {
    { 10.0f, 705.0f, 94.0f, 765.0f, 0 },  /* 菜单 */
    { 104.0f, 705.0f, 188.0f, 765.0f, 1 },  /* 回退 */
    { 198.0f, 705.0f, 282.0f, 765.0f, 2 },  /* 前进 */
    { 292.0f, 705.0f, 376.0f, 765.0f, 3 },  /* 跳转 */
    { 386.0f, 705.0f, 470.0f, 765.0f, 4 },  /* 返回 */
};
const ui_d107_btn *ui_d107_buttons(void) { return g_d107_btns; }
int ui_d107_hit(const ui_d107_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_108）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d108_btn g_d108_btns[UI_D108_COUNT] = {
    { 175.0f, 781.0f, 285.0f, 841.0f, 0 },  /* 复盘 */
    { 305.0f, 781.0f, 415.0f, 841.0f, 1 },  /* 删除 */
    { 435.0f, 781.0f, 545.0f, 841.0f, 2 },  /* 返回 */
};
const ui_d108_btn *ui_d108_buttons(void) { return g_d108_btns; }
int ui_d108_hit(const ui_d108_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_109）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d109_btn g_d109_btns[UI_D109_COUNT] = {
    { 245.0f, 610.0f, 315.0f, 670.0f, 0 },  /* 开局 */
    { 325.0f, 610.0f, 395.0f, 670.0f, 1 },  /* 终局 */
    { 405.0f, 610.0f, 475.0f, 670.0f, 2 },  /* 定位 */
};
const ui_d109_btn *ui_d109_buttons(void) { return g_d109_btns; }
int ui_d109_hit(const ui_d109_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_110）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d110_btn g_d110_btns[UI_D110_COUNT] = {
    { 180.0f, 860.0f, 337.0f, 920.0f, 0 },  /* 清空 */
    { 383.0f, 860.0f, 540.0f, 920.0f, 1 },  /* 关闭 */
};
const ui_d110_btn *ui_d110_buttons(void) { return g_d110_btns; }
int ui_d110_hit(const ui_d110_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_112）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d112_btn g_d112_btns[UI_D112_COUNT] = {
    { 150.0f, 522.0f, 255.0f, 658.0f, 0 },  /* 切换显示 */
    { 255.0f, 522.0f, 360.0f, 658.0f, 1 },  /* 对局信息 */
    { 360.0f, 522.0f, 465.0f, 658.0f, 2 },  /* 游戏规则 */
    { 465.0f, 522.0f, 570.0f, 658.0f, 3 },  /* 功能设置 */
    { 150.0f, 648.0f, 255.0f, 782.0f, 4 },  /* 软件信息 */
};
const ui_d112_btn *ui_d112_buttons(void) { return g_d112_btns; }
int ui_d112_hit(const ui_d112_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_113）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d113_btn g_d113_btns[UI_D113_COUNT] = {
    { 260.0f, 505.0f, 460.0f, 565.0f, 0 },  /* 0 */
    { 260.0f, 575.0f, 460.0f, 635.0f, 1 },  /* 1 */
    { 260.0f, 645.0f, 460.0f, 705.0f, 2 },  /* 2 */
    { 260.0f, 715.0f, 460.0f, 775.0f, 3 },  /* 3 */
};
const ui_d113_btn *ui_d113_buttons(void) { return g_d113_btns; }
int ui_d113_hit(const ui_d113_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_114）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d114_btn g_d114_btns[UI_D114_COUNT] = {
    { 30.0f, 700.0f, 90.0f, 760.0f, 0 },  /* Site */
};
const ui_d114_btn *ui_d114_buttons(void) { return g_d114_btns; }
int ui_d114_hit(const ui_d114_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_118）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d118_btn g_d118_btns[UI_D118_COUNT] = {
    { 175.0f, 735.0f, 340.0f, 795.0f, 0 },  /* 更新 */
    { 380.0f, 735.0f, 545.0f, 795.0f, 1 },  /* 取消 */
};
const ui_d118_btn *ui_d118_buttons(void) { return g_d118_btns; }
int ui_d118_hit(const ui_d118_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_119）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d119_btn g_d119_btns[UI_D119_COUNT] = {
    { 185.0f, 702.0f, 344.0f, 762.0f, 0 },  /* 确定 */
    { 376.0f, 702.0f, 535.0f, 762.0f, 1 },  /* 取消 */
};
const ui_d119_btn *ui_d119_buttons(void) { return g_d119_btns; }
int ui_d119_hit(const ui_d119_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_120）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d120_btn g_d120_btns[UI_D120_COUNT] = {
    { 255.0f, 535.0f, 465.0f, 595.0f, 0 },  /* 注册用户 */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },  /* 找回密码 */
    { 255.0f, 685.0f, 465.0f, 745.0f, 2 },  /* 返回 */
};
const ui_d120_btn *ui_d120_buttons(void) { return g_d120_btns; }
int ui_d120_hit(const ui_d120_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_121）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d121_btn g_d121_btns[UI_D121_COUNT] = {
    { 185.0f, 702.0f, 344.0f, 762.0f, 0 },  /* 确定 */
    { 376.0f, 702.0f, 535.0f, 762.0f, 1 },  /* 取消 */
};
const ui_d121_btn *ui_d121_buttons(void) { return g_d121_btns; }
int ui_d121_hit(const ui_d121_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_122）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d122_btn g_d122_btns[UI_D122_COUNT] = {
    { 185.0f, 709.0f, 344.0f, 769.0f, 0 },  /* 确定 */
    { 376.0f, 709.0f, 535.0f, 769.0f, 1 },  /* 取消 */
};
const ui_d122_btn *ui_d122_buttons(void) { return g_d122_btns; }
int ui_d122_hit(const ui_d122_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_123）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d123_btn g_d123_btns[UI_D123_COUNT] = {
    { 307.0f, 610.0f, 382.0f, 678.0f, 0 },  /* Wall */
};
const ui_d123_btn *ui_d123_buttons(void) { return g_d123_btns; }
int ui_d123_hit(const ui_d123_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_246）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d246_btn g_d246_btns[UI_D246_COUNT] = {
    { 113.0f, 442.0f, 175.0f, 487.0f, 0 },  /* Menu */
    { 303.0f, 442.0f, 365.0f, 487.0f, 1 },  /* Back */
};
const ui_d246_btn *ui_d246_buttons(void) { return g_d246_btns; }
int ui_d246_hit(const ui_d246_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_247）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d247_btn g_d247_btns[UI_D247_COUNT] = {
    { 303.0f, 442.0f, 365.0f, 487.0f, 0 },  /* Back */
};
const ui_d247_btn *ui_d247_buttons(void) { return g_d247_btns; }
int ui_d247_hit(const ui_d247_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_248）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d248_btn g_d248_btns[UI_D248_COUNT] = {
    { 135.0f, 575.0f, 345.0f, 635.0f, 0 },  /* 人机对战 */
    { 135.0f, 650.0f, 345.0f, 710.0f, 1 },  /* 两人对战 */
    { 115.0f, 725.0f, 230.0f, 785.0f, 2 },  /* 联网 */
    { 250.0f, 725.0f, 365.0f, 785.0f, 3 },  /* 联机 */
};
const ui_d248_btn *ui_d248_buttons(void) { return g_d248_btns; }
int ui_d248_hit(const ui_d248_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_249）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d249_btn g_d249_btns[UI_D249_COUNT] = {
    { 388.0f, 731.0f, 458.0f, 795.0f, 0 },  /* Type */
};
const ui_d249_btn *ui_d249_buttons(void) { return g_d249_btns; }
int ui_d249_hit(const ui_d249_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_250）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d250_btn g_d250_btns[UI_D250_COUNT] = {
    { 85.0f, 442.0f, 147.0f, 487.0f, 0 },  /* Menu */
    { 147.0f, 442.0f, 209.0f, 487.0f, 1 },  /* Chat */
};
const ui_d250_btn *ui_d250_buttons(void) { return g_d250_btns; }
int ui_d250_hit(const ui_d250_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_251）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d251_btn g_d251_btns[UI_D251_COUNT] = {
    { 42.0f, 705.0f, 132.0f, 765.0f, 0 },  /* 复盘 */
};
const ui_d251_btn *ui_d251_buttons(void) { return g_d251_btns; }
int ui_d251_hit(const ui_d251_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_253）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d253_btn g_d253_btns[UI_D253_COUNT] = {
    { 320.0f, 698.0f, 412.0f, 758.0f, 0 },  /* 详细 */
};
const ui_d253_btn *ui_d253_buttons(void) { return g_d253_btns; }
int ui_d253_hit(const ui_d253_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_255）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d255_btn g_d255_btns[UI_D255_COUNT] = {
    { 85.0f, 442.0f, 147.0f, 487.0f, 0 },  /* Menu */
    { 147.0f, 442.0f, 209.0f, 487.0f, 1 },  /* Chat */
    { 303.0f, 442.0f, 365.0f, 487.0f, 2 },  /* Back */
};
const ui_d255_btn *ui_d255_buttons(void) { return g_d255_btns; }
int ui_d255_hit(const ui_d255_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_256）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d256_btn g_d256_btns[UI_D256_COUNT] = {
    { 60.0f, 442.0f, 122.0f, 487.0f, 0 },  /* Prev */
    { 169.0f, 442.0f, 231.0f, 487.0f, 1 },  /* Next */
    { 275.0f, 442.0f, 337.0f, 487.0f, 2 },  /* Menu */
    { 337.0f, 442.0f, 399.0f, 487.0f, 3 },  /* Back */
};
const ui_d256_btn *ui_d256_buttons(void) { return g_d256_btns; }
int ui_d256_hit(const ui_d256_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_257）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d257_btn g_d257_btns[UI_D257_COUNT] = {
    { 10.0f, 750.0f, 94.0f, 810.0f, 0 },  /* 菜单 */
    { 104.0f, 750.0f, 188.0f, 810.0f, 1 },  /* 新局 */
    { 198.0f, 750.0f, 282.0f, 810.0f, 2 },  /* 悔棋 */
    { 292.0f, 750.0f, 376.0f, 810.0f, 3 },  /* 设置 */
    { 386.0f, 750.0f, 470.0f, 810.0f, 4 },  /* 返回 */
};
const ui_d257_btn *ui_d257_buttons(void) { return g_d257_btns; }
int ui_d257_hit(const ui_d257_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_258）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d258_btn g_d258_btns[UI_D258_COUNT] = {
    { 10.0f, 750.0f, 94.0f, 810.0f, 0 },  /* 载入 */
    { 104.0f, 750.0f, 188.0f, 810.0f, 1 },  /* 保存 */
    { 198.0f, 750.0f, 282.0f, 810.0f, 2 },  /* 清空 */
    { 292.0f, 750.0f, 376.0f, 810.0f, 3 },  /* 完成 */
    { 386.0f, 750.0f, 470.0f, 810.0f, 4 },  /* 取消 */
};
const ui_d258_btn *ui_d258_buttons(void) { return g_d258_btns; }
int ui_d258_hit(const ui_d258_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_259）：2 按钮（720 基准，局部 1:1 平移） */
static const ui_d259_btn g_d259_btns[UI_D259_COUNT] = {
    { 135.0f, 610.0f, 345.0f, 670.0f, 0 },  /* 人机对战 */
    { 135.0f, 700.0f, 345.0f, 760.0f, 1 },  /* 联网对战 */
};
const ui_d259_btn *ui_d259_buttons(void) { return g_d259_btns; }
int ui_d259_hit(const ui_d259_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_260）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d260_btn g_d260_btns[UI_D260_COUNT] = {
    { 10.0f, 750.0f, 94.0f, 810.0f, 0 },  /* 菜单 */
    { 104.0f, 750.0f, 188.0f, 810.0f, 1 },  /* 复盘 */
    { 198.0f, 750.0f, 282.0f, 810.0f, 2 },  /* 设置 */
    { 292.0f, 750.0f, 376.0f, 810.0f, 3 },  /* 投降 */
    { 386.0f, 750.0f, 470.0f, 810.0f, 4 },  /* 提和 */
};
const ui_d260_btn *ui_d260_buttons(void) { return g_d260_btns; }
int ui_d260_hit(const ui_d260_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_261）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d261_btn g_d261_btns[UI_D261_COUNT] = {
    { 320.0f, 708.0f, 412.0f, 768.0f, 0 },  /* 详细 */
};
const ui_d261_btn *ui_d261_buttons(void) { return g_d261_btns; }
int ui_d261_hit(const ui_d261_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_262）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d262_btn g_d262_btns[UI_D262_COUNT] = {
    { 10.0f, 750.0f, 94.0f, 810.0f, 0 },  /* 布局 */
    { 104.0f, 750.0f, 188.0f, 810.0f, 1 },  /* 出征 */
    { 198.0f, 750.0f, 282.0f, 810.0f, 2 },  /* 取消 */
    { 386.0f, 750.0f, 470.0f, 810.0f, 3 },  /* 返回 */
};
const ui_d262_btn *ui_d262_buttons(void) { return g_d262_btns; }
int ui_d262_hit(const ui_d262_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_263）：5 按钮（720 基准，局部 1:1 平移） */
static const ui_d263_btn g_d263_btns[UI_D263_COUNT] = {
    { 10.0f, 750.0f, 94.0f, 810.0f, 0 },  /* 菜单 */
    { 104.0f, 750.0f, 188.0f, 810.0f, 1 },  /* 回退 */
    { 198.0f, 750.0f, 282.0f, 810.0f, 2 },  /* 前进 */
    { 292.0f, 750.0f, 376.0f, 810.0f, 3 },  /* 跳转 */
    { 386.0f, 750.0f, 470.0f, 810.0f, 4 },  /* 返回 */
};
const ui_d263_btn *ui_d263_buttons(void) { return g_d263_btns; }
int ui_d263_hit(const ui_d263_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_264）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d264_btn g_d264_btns[UI_D264_COUNT] = {
    { 30.0f, 735.0f, 90.0f, 795.0f, 0 },  /* Site */
};
const ui_d264_btn *ui_d264_buttons(void) { return g_d264_btns; }
int ui_d264_hit(const ui_d264_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }