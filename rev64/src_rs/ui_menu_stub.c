/* ui_menu_stub.c — 桌面 RS_NO_GL 桩（菜单按钮几何 + 命中，无 GL 渲染） */
#include "ui_menu.h"
const ui_btn *ui_menu_buttons(void) {
    static const ui_btn b[UI_BTN_COUNT] = {
        { 60, 980, 350, 1070, "人机对战", 0 },
        { 370, 980, 660, 1070, "两人对战", 1 },
        { 60, 1090, 350, 1180, "联网对战", 2 },
        { 370,1090, 660, 1180, "四国军棋", 3 },
        { 180,1200, 540, 1280, "悔棋重开", 4 },
    };
    return b;
}
int ui_menu_hit(const ui_btn *btns, int n, float x, float y) {
    for (int i = 0; i < n; i++)
        if (x >= btns[i].x0 && x <= btns[i].x1 && y >= btns[i].y0 && y <= btns[i].y1)
            return i;
    return -1;
}
void ui_menu_action_log(int action) { (void)action; }
/* ★ P1（datah_016/017/020）：布局存档 3 弹窗按钮桩（桌面 headless 无渲染，几何表返回空占位
   供一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。 */
static const ui_gameset_btn gs_stub[UI_GAMESET_COUNT] = {
    { 345.0f,483.0f,555.0f,543.0f,0 },{ 345.0f,558.0f,555.0f,618.0f,1 },
    { 345.0f,633.0f,555.0f,693.0f,2 },{ 345.0f,708.0f,555.0f,768.0f,3 },
    { 165.0f,793.0f,345.0f,853.0f,4 },{ 375.0f,793.0f,555.0f,853.0f,5 },
};
const ui_gameset_btn *ui_gameset_buttons(void) { return gs_stub; }
int ui_gameset_hit(const ui_gameset_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_gameset_popup_inside(float x, float y)
{ return x>=UI_GAMESET_POPUP_X0&&x<=UI_GAMESET_POPUP_X0+UI_GAMESET_POPUP_W&&
         y>=UI_GAMESET_POPUP_Y0&&y<=UI_GAMESET_POPUP_Y0+UI_GAMESET_POPUP_H; }
static const ui_loadlayout_btn ll_stub[UI_LOADLAYOUT_COUNT] = {
    { 180.0f,447.0f,540.0f,507.0f,0 },{ 180.0f,507.0f,540.0f,567.0f,1 },
    { 180.0f,567.0f,540.0f,627.0f,2 },{ 180.0f,627.0f,540.0f,687.0f,3 },
    { 180.0f,687.0f,540.0f,747.0f,4 },{ 180.0f,747.0f,540.0f,807.0f,5 },
    { 280.0f,828.0f,440.0f,888.0f,6 },
};
const ui_loadlayout_btn *ui_loadlayout_buttons(void) { return ll_stub; }
int ui_loadlayout_hit(const ui_loadlayout_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_loadlayout_popup_inside(float x, float y)
{ return x>=UI_LOADLAYOUT_POPUP_X0&&x<=UI_LOADLAYOUT_POPUP_X0+UI_LOADLAYOUT_POPUP_W&&
         y>=UI_LOADLAYOUT_POPUP_Y0&&y<=UI_LOADLAYOUT_POPUP_Y0+UI_LOADLAYOUT_POPUP_H; }
static const ui_savelayout_btn sl_stub[UI_SAVELAYOUT_COUNT] = {
    { 175.0f,828.0f,260.0f,888.0f,0 },{ 270.0f,828.0f,355.0f,888.0f,1 },
    { 365.0f,828.0f,450.0f,888.0f,2 },{ 460.0f,828.0f,545.0f,888.0f,3 },
};
const ui_savelayout_btn *ui_savelayout_buttons(void) { return sl_stub; }
int ui_savelayout_hit(const ui_savelayout_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_savelayout_popup_inside(float x, float y)
{ return x>=UI_SAVELAYOUT_POPUP_X0&&x<=UI_SAVELAYOUT_POPUP_X0+UI_SAVELAYOUT_POPUP_W&&
         y>=UI_SAVELAYOUT_POPUP_Y0&&y<=UI_SAVELAYOUT_POPUP_Y0+UI_SAVELAYOUT_POPUP_H; }
static const ui_laymgr_btn lm_stub[UI_LAYMGR_COUNT] = {
    { 185.0f,793.0f,289.0f,853.0f,0 },{ 308.0f,793.0f,412.0f,853.0f,1 },
    { 431.0f,793.0f,535.0f,853.0f,2 },
};
const ui_laymgr_btn *ui_laymgr_buttons(void) { return lm_stub; }
int ui_laymgr_hit(const ui_laymgr_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_laymgr_popup_inside(float x, float y)
{ return x>=UI_LAYMGR_POPUP_X0&&x<=UI_LAYMGR_POPUP_X0+UI_LAYMGR_POPUP_W&&
         y>=UI_LAYMGR_POPUP_Y0&&y<=UI_LAYMGR_POPUP_Y0+UI_LAYMGR_POPUP_H; }
/* ★ P1（datah_039/107）：复盘回放控制条 5 按钮桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。 */
static const ui_replay_btn rc_stub[UI_REPLAY_CTRL_COUNT] = {
    { 10.0f,705.0f,94.0f,765.0f,0 },{ 104.0f,705.0f,188.0f,765.0f,1 },
    { 198.0f,705.0f,282.0f,765.0f,2 },{ 292.0f,705.0f,376.0f,765.0f,3 },
    { 386.0f,705.0f,470.0f,765.0f,4 },
};
const ui_replay_btn *ui_replay_ctrl_buttons(void) { return rc_stub; }
int ui_replay_ctrl_hit(const ui_replay_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_replay_ctrl_popup_inside(float x, float y)
{ return x>=UI_REPLAY_CTRL_POPUP_X0&&x<=UI_REPLAY_CTRL_POPUP_X0+UI_REPLAY_CTRL_POPUP_W&&
         y>=UI_REPLAY_CTRL_POPUP_Y0&&y<=UI_REPLAY_CTRL_POPUP_Y0+UI_REPLAY_CTRL_POPUP_H; }
/* ★ P1（datah_038）：登录弹窗 6 控件桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：原版登录=账号系统（服务器协议未逆向）；游客=本地单机唯一真生效路径。 */
static const ui_login_btn ln_stub[UI_LOGIN_COUNT] = {
    { 274.0f,578.0f,519.0f,623.0f,0 },{ 274.0f,638.0f,519.0f,683.0f,1 },
    { 162.0f,703.0f,252.0f,763.0f,2 },{ 264.0f,703.0f,354.0f,763.0f,3 },
    { 366.0f,703.0f,456.0f,763.0f,4 },{ 468.0f,703.0f,558.0f,763.0f,5 },
};
const ui_login_btn *ui_login_buttons(void) { return ln_stub; }
int ui_login_hit(const ui_login_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_login_popup_inside(float x, float y)
{ return x>=UI_LOGIN_POPUP_X0&&x<=UI_LOGIN_POPUP_X0+UI_LOGIN_POPUP_W&&
         y>=UI_LOGIN_POPUP_Y0&&y<=UI_LOGIN_POPUP_Y0+UI_LOGIN_POPUP_H; }
/* ★ P2（datah_037 证据）：规则文本弹窗 3 按钮桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：规则文本=src_rs 现有简化版固定文本（2 套：翻棋/布局）。 */
static const ui_rules_btn ru_stub[UI_RULES_COUNT] = {
    { 255.0f,530.0f,465.0f,590.0f,0 },{ 255.0f,610.0f,465.0f,670.0f,1 },
    { 255.0f,690.0f,465.0f,750.0f,2 },
};
const ui_rules_btn *ui_rules_buttons(void) { return ru_stub; }
int ui_rules_hit(const ui_rules_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_rules_popup_inside(float x, float y)
{ return x>=UI_RULES_POPUP_X0&&x<=UI_RULES_POPUP_X0+UI_RULES_POPUP_W&&
         y>=UI_RULES_POPUP_Y0&&y<=UI_RULES_POPUP_Y0+UI_RULES_POPUP_H; }
/* ★ P2（datah_104 证据）：新局完整版弹窗 5 控件桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：3 项各循环切换当前值（对齐 datah_001 音量循环 + datah_012 开关循环范式）。 */
static const ui_newgame104_btn ng_stub[UI_NEWGAME104_COUNT] = {
    { 355.0f,520.0f,555.0f,580.0f,0 },{ 355.0f,595.0f,555.0f,655.0f,1 },
    { 355.0f,670.0f,555.0f,730.0f,2 },{ 165.0f,755.0f,345.0f,815.0f,3 },
    { 375.0f,755.0f,555.0f,815.0f,4 },
};
const ui_newgame104_btn *ui_newgame104_buttons(void) { return ng_stub; }
int ui_newgame104_hit(const ui_newgame104_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_newgame104_popup_inside(float x, float y)
{ return x>=UI_NEWGAME104_POPUP_X0&&x<=UI_NEWGAME104_POPUP_X0+UI_NEWGAME104_POPUP_W&&
         y>=UI_NEWGAME104_POPUP_Y0&&y<=UI_NEWGAME104_POPUP_Y0+UI_NEWGAME104_POPUP_H; }
/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗 3 控件桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：原版加入=局域网/Wi-Fi 联机协议（未逆向），src_rs 仅蓝牙 SPP。 */
static const ui_join_btn jo_stub[UI_JOIN_COUNT] = {
    { 270.0f,602.5f,510.0f,647.5f,0 },{ 185.0f,672.5f,345.0f,732.5f,1 },
    { 375.0f,672.5f,535.0f,732.5f,2 },
};
const ui_join_btn *ui_join_buttons(void) { return jo_stub; }
int ui_join_hit(const ui_join_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_join_popup_inside(float x, float y)
{ return x>=UI_JOIN_POPUP_X0&&x<=UI_JOIN_POPUP_X0+UI_JOIN_POPUP_W&&
         y>=UI_JOIN_POPUP_Y0&&y<=UI_JOIN_POPUP_Y0+UI_JOIN_POPUP_H; }
/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗 3 控件桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：原版创建=Wi-Fi/局域网协议（未逆向），src_rs 仅蓝牙 SPP。 */
static const ui_create_btn cr_stub[UI_CREATE_COUNT] = {
    { 250.0f,602.5f,470.0f,647.5f,0 },{ 185.0f,672.5f,345.0f,732.5f,1 },
    { 375.0f,672.5f,535.0f,732.5f,2 },
};
const ui_create_btn *ui_create_buttons(void) { return cr_stub; }
int ui_create_hit(const ui_create_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_create_popup_inside(float x, float y)
{ return x>=UI_CREATE_POPUP_X0&&x<=UI_CREATE_POPUP_X0+UI_CREATE_POPUP_W&&
         y>=UI_CREATE_POPUP_Y0&&y<=UI_CREATE_POPUP_Y0+UI_CREATE_POPUP_H; }
/* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗 6 控件桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：4 项各循环切换当前值（对齐 datah_104 循环切换范式）。 */
static const ui_ng061_btn n061_stub[UI_NG061_COUNT] = {
    { 355.0f,483.0f,555.0f,543.0f,0 },{ 355.0f,558.0f,555.0f,618.0f,1 },
    { 355.0f,633.0f,555.0f,693.0f,2 },{ 355.0f,708.0f,555.0f,768.0f,3 },
    { 165.0f,793.0f,345.0f,853.0f,4 },{ 375.0f,793.0f,555.0f,853.0f,5 },
};
const ui_ng061_btn *ui_ng061_buttons(void) { return n061_stub; }
int ui_ng061_hit(const ui_ng061_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ng061_popup_inside(float x, float y)
{ return x>=UI_NG061_POPUP_X0&&x<=UI_NG061_POPUP_X0+UI_NG061_POPUP_W&&
         y>=UI_NG061_POPUP_Y0&&y<=UI_NG061_POPUP_Y0+UI_NG061_POPUP_H; }
/* ★ P2（datah_050 证据）：对局信息弹窗 3 按钮桩。
   诚实边界：Info 文本=☰ 战况统计复用；删除=清复盘记录简化（原版删除对局存档未逆向）。 */
static const ui_gameinfo_btn gi_stub[UI_GAMEINFO_COUNT] = {
    { 175.0f,745.0f,285.0f,805.0f,0 },{ 305.0f,745.0f,415.0f,805.0f,1 },
    { 435.0f,745.0f,545.0f,805.0f,2 },
};
const ui_gameinfo_btn *ui_gameinfo_buttons(void) { return gi_stub; }
int ui_gameinfo_hit(const ui_gameinfo_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_gameinfo_popup_inside(float x, float y)
{ return x>=UI_GAMEINFO_POPUP_X0&&x<=UI_GAMEINFO_POPUP_X0+UI_GAMEINFO_POPUP_W&&
         y>=UI_GAMEINFO_POPUP_Y0&&y<=UI_GAMEINFO_POPUP_Y0+UI_GAMEINFO_POPUP_H; }
/* ★ P2（datah_002 证据）：亮度调节弹窗 3 控件桩。
   诚实边界：原版滑条拖拽=触摸 flaky，简化为"低/中/高"3 档循环切换（不画真滑条）。 */
static const ui_bright_btn br_stub[UI_BRIGHT_COUNT] = {
    { 152.0f,601.0f,568.0f,649.0f,0 },{ 160.0f,675.0f,345.0f,735.0f,1 },
    { 375.0f,675.0f,560.0f,735.0f,2 },
};
const ui_bright_btn *ui_bright_buttons(void) { return br_stub; }
int ui_bright_hit(const ui_bright_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_bright_popup_inside(float x, float y)
{ return x>=UI_BRIGHT_POPUP_X0&&x<=UI_BRIGHT_POPUP_X0+UI_BRIGHT_POPUP_W&&
         y>=UI_BRIGHT_POPUP_Y0&&y<=UI_BRIGHT_POPUP_Y0+UI_BRIGHT_POPUP_H; }
/* ★ P2（datah_029 证据）：聊天记录弹窗 2 按钮桩。
   诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
static const ui_chatlog_btn cl_stub[UI_CHATLOG_COUNT] = {
    { 187.0f,825.0f,343.0f,885.0f,0 },{ 377.0f,825.0f,533.0f,885.0f,1 },
};
const ui_chatlog_btn *ui_chatlog_buttons(void) { return cl_stub; }
int ui_chatlog_hit(const ui_chatlog_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_chatlog_popup_inside(float x, float y)
{ return x>=UI_CHATLOG_POPUP_X0&&x<=UI_CHATLOG_POPUP_X0+UI_CHATLOG_POPUP_W&&
         y>=UI_CHATLOG_POPUP_Y0&&y<=UI_CHATLOG_POPUP_Y0+UI_CHATLOG_POPUP_H; }
/* ★ P2（datah_031 证据）：个人资料弹窗 3 按钮桩。
   诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
static const ui_profile_btn pf_stub[UI_PROFILE_COUNT] = {
    { 175.0f,815.0f,285.0f,875.0f,0 },{ 305.0f,815.0f,415.0f,875.0f,1 },
    { 435.0f,815.0f,545.0f,875.0f,2 },
};
const ui_profile_btn *ui_profile_buttons(void) { return pf_stub; }
int ui_profile_hit(const ui_profile_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_profile_popup_inside(float x, float y)
{ return x>=UI_PROFILE_POPUP_X0&&x<=UI_PROFILE_POPUP_X0+UI_PROFILE_POPUP_W&&
         y>=UI_PROFILE_POPUP_Y0&&y<=UI_PROFILE_POPUP_Y0+UI_PROFILE_POPUP_H; }
/* ★ P2（datah_042 证据）：提和详情弹窗 7 图标桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：本地提和判定未实现（原版需对端应答），提和/认输走 BT 帧 + 边界对话框。 */
static const ui_drawdetail_btn dd_stub[UI_DRAWDETAIL_COUNT] = {
    { 150.0f,523.0f,255.0f,658.0f,0 },{ 255.0f,523.0f,360.0f,658.0f,1 },
    { 360.0f,523.0f,465.0f,658.0f,2 },{ 465.0f,523.0f,570.0f,658.0f,3 },
    { 150.0f,648.0f,255.0f,783.0f,4 },{ 255.0f,648.0f,360.0f,783.0f,5 },
    { 360.0f,648.0f,465.0f,783.0f,6 },
};
const ui_drawdetail_btn *ui_drawdetail_buttons(void) { return dd_stub; }
int ui_drawdetail_hit(const ui_drawdetail_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_drawdetail_popup_inside(float x, float y)
{ return x>=UI_DRAWDETAIL_POPUP_X0&&x<=UI_DRAWDETAIL_POPUP_X0+UI_DRAWDETAIL_POPUP_W&&
          y>=UI_DRAWDETAIL_POPUP_Y0&&y<=UI_DRAWDETAIL_POPUP_Y0+UI_DRAWDETAIL_POPUP_H; }

/* ★ P2（datah_003/004 证据）：设置入口 3/2 按钮桩（几何表返回 720 基准占位供
   一致性/evverify 链接；真机走 ui_menu.c 真实几何表）。
   诚实边界：图像设置渲染参数未实现，开关仅 UI 状态记录。 */
static const ui_setentry_btn se_stub[UI_SETEXTTRY_COUNT] = {
    { 255.0f,530.0f,465.0f,590.0f,0 },{ 255.0f,610.0f,465.0f,670.0f,1 },
    { 255.0f,690.0f,465.0f,750.0f,2 },
};
const ui_setentry_btn *ui_setentry_buttons(void) { return se_stub; }
int ui_setentry_hit(const ui_setentry_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_setentry_popup_inside(float x, float y)
{ return x>=UI_SETEXTTRY_POPUP_X0&&x<=UI_SETEXTTRY_POPUP_X0+UI_SETEXTTRY_POPUP_W&&
         y>=UI_SETEXTTRY_POPUP_Y0&&y<=UI_SETEXTTRY_POPUP_Y0+UI_SETEXTTRY_POPUP_H; }
static const ui_setentry4_btn se4_stub[UI_SETEXTTRY4_COUNT] = {
    { 255.0f,570.0f,465.0f,630.0f,0 },{ 255.0f,650.0f,465.0f,710.0f,1 },
};
const ui_setentry4_btn *ui_setentry4_buttons(void) { return se4_stub; }
int ui_setentry4_hit(const ui_setentry4_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_setentry4_popup_inside(float x, float y)
{ return x>=UI_SETEXTTRY4_POPUP_X0&&x<=UI_SETEXTTRY4_POPUP_X0+UI_SETEXTTRY4_POPUP_W&&
         y>=UI_SETEXTTRY4_POPUP_Y0&&y<=UI_SETEXTTRY4_POPUP_Y0+UI_SETEXTTRY4_POPUP_H; }

/* ★ P2（datah_007/008 证据）：图像设置 6/5 控件桩。
   诚实边界：图像/夜间模式渲染参数未实现，开关仅 UI 状态记录。 */
static const ui_imgset7_btn i7_stub[UI_IMGSET7_COUNT] = {
    { 355.0f,483.0f,555.0f,543.0f,0 },{ 355.0f,558.0f,555.0f,618.0f,1 },
    { 355.0f,633.0f,555.0f,693.0f,2 },{ 355.0f,708.0f,555.0f,768.0f,3 },
    { 165.0f,788.0f,345.0f,848.0f,4 },{ 375.0f,788.0f,555.0f,848.0f,5 },
};
const ui_imgset7_btn *ui_imgset7_buttons(void) { return i7_stub; }
int ui_imgset7_hit(const ui_imgset7_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_imgset7_popup_inside(float x, float y)
{ return x>=UI_IMGSET7_POPUP_X0&&x<=UI_IMGSET7_POPUP_X0+UI_IMGSET7_POPUP_W&&
         y>=UI_IMGSET7_POPUP_Y0&&y<=UI_IMGSET7_POPUP_Y0+UI_IMGSET7_POPUP_H; }
static const ui_imgset8_btn i8_stub[UI_IMGSET8_COUNT] = {
    { 355.0f,520.0f,555.0f,580.0f,0 },{ 355.0f,595.0f,555.0f,655.0f,1 },
    { 355.0f,670.0f,555.0f,730.0f,2 },{ 165.0f,750.0f,345.0f,810.0f,3 },
    { 375.0f,750.0f,555.0f,810.0f,4 },
};
const ui_imgset8_btn *ui_imgset8_buttons(void) { return i8_stub; }
int ui_imgset8_hit(const ui_imgset8_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_imgset8_popup_inside(float x, float y)
{ return x>=UI_IMGSET8_POPUP_X0&&x<=UI_IMGSET8_POPUP_X0+UI_IMGSET8_POPUP_W&&
         y>=UI_IMGSET8_POPUP_Y0&&y<=UI_IMGSET8_POPUP_Y0+UI_IMGSET8_POPUP_H; }

/* ★ P2（datah_065 证据）：加入游戏完整版 10 控件桩。
   诚实边界：联机开局配置（src_rs 无联机开局逻辑），8 项显示=占位标签，
   开始→边界对话框。 */
static const ui_join065_btn j65_stub[UI_JOIN065_COUNT] = {
    { 275.0f,473.0f,555.0f,508.0f,0 },{ 275.0f,513.0f,555.0f,548.0f,1 },
    { 275.0f,553.0f,555.0f,588.0f,2 },{ 275.0f,593.0f,555.0f,628.0f,3 },
    { 275.0f,633.0f,555.0f,668.0f,4 },{ 275.0f,673.0f,555.0f,708.0f,5 },
    { 275.0f,713.0f,555.0f,748.0f,6 },{ 275.0f,753.0f,555.0f,788.0f,7 },
    { 165.0f,808.0f,345.0f,868.0f,8 },{ 375.0f,808.0f,555.0f,868.0f,9 },
};
const ui_join065_btn *ui_join065_buttons(void) { return j65_stub; }
int ui_join065_hit(const ui_join065_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_join065_popup_inside(float x, float y)
{ return x>=UI_JOIN065_POPUP_X0&&x<=UI_JOIN065_POPUP_X0+UI_JOIN065_POPUP_W&&
         y>=UI_JOIN065_POPUP_Y0&&y<=UI_JOIN065_POPUP_Y0+UI_JOIN065_POPUP_H; }

/* ★ P2（datah_069 证据）：创建游戏完整版 6 控件桩。
   诚实边界：联机房间创建需 Wi-Fi/局域网协议（src_rs 仅蓝牙已实现），
   确定→边界对话框。 */
static const ui_create069_btn c69_stub[UI_CREATE069_COUNT] = {
    { 355.0f,483.0f,555.0f,543.0f,0 },{ 355.0f,558.0f,555.0f,618.0f,1 },
    { 355.0f,633.0f,555.0f,693.0f,2 },{ 355.0f,708.0f,555.0f,768.0f,3 },
    { 165.0f,793.0f,345.0f,853.0f,4 },{ 375.0f,793.0f,555.0f,853.0f,5 },
};
const ui_create069_btn *ui_create069_buttons(void) { return c69_stub; }
int ui_create069_hit(const ui_create069_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_create069_popup_inside(float x, float y)
{ return x>=UI_CREATE069_POPUP_X0&&x<=UI_CREATE069_POPUP_X0+UI_CREATE069_POPUP_W&&
         y>=UI_CREATE069_POPUP_Y0&&y<=UI_CREATE069_POPUP_Y0+UI_CREATE069_POPUP_H; }

/* ★ P2（datah_070 证据）：游戏规则完整版 6 控件桩。
   与 datah_012 同构，复用 ui_rule_*_get/set。 */
static const ui_rules070_btn r70_stub[UI_RULES070_COUNT] = {
    { 355.0f,483.0f,555.0f,543.0f,0 },{ 355.0f,558.0f,555.0f,618.0f,1 },
    { 355.0f,633.0f,555.0f,693.0f,2 },{ 355.0f,708.0f,555.0f,768.0f,3 },
    { 165.0f,793.0f,345.0f,853.0f,4 },{ 375.0f,793.0f,555.0f,853.0f,5 },
};
const ui_rules070_btn *ui_rules070_buttons(void) { return r70_stub; }
int ui_rules070_hit(const ui_rules070_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_rules070_popup_inside(float x, float y)
{ return x>=UI_RULES070_POPUP_X0&&x<=UI_RULES070_POPUP_X0+UI_RULES070_POPUP_W&&
         y>=UI_RULES070_POPUP_Y0&&y<=UI_RULES070_POPUP_Y0+UI_RULES070_POPUP_H; }

/* ★ P2（datah_060 证据）：房间态 3 按钮桩。
   诚实边界：联机房间需 Wi-Fi/局域网协议（src_rs 仅蓝牙已实现），
   开始/换桌→边界对话框。 */
static const ui_room060_btn rm_stub[UI_ROOM060_COUNT] = {
    { 160.0f,734.0f,280.0f,794.0f,0 },{ 300.0f,734.0f,420.0f,794.0f,1 },
    { 440.0f,734.0f,560.0f,794.0f,2 },
};
const ui_room060_btn *ui_room060_buttons(void) { return rm_stub; }
int ui_room060_hit(const ui_room060_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_room060_popup_inside(float x, float y)
{ return x>=UI_ROOM060_POPUP_X0&&x<=UI_ROOM060_POPUP_X0+UI_ROOM060_POPUP_W&&
         y>=UI_ROOM060_POPUP_Y0&&y<=UI_ROOM060_POPUP_Y0+UI_ROOM060_POPUP_H; }

/* ★ P2（datah_063 证据）：聊天完整版 3 按钮桩。
   诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT），
   发送→占位帧+边界对话框。 */
static const ui_chat063_btn ch_stub[UI_CHAT063_COUNT] = {
    { 175.0f,723.0f,285.0f,783.0f,0 },{ 305.0f,723.0f,415.0f,783.0f,1 },
    { 435.0f,723.0f,545.0f,783.0f,2 },
};
const ui_chat063_btn *ui_chat063_buttons(void) { return ch_stub; }
int ui_chat063_hit(const ui_chat063_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_chat063_popup_inside(float x, float y)
{ return x>=UI_CHAT063_POPUP_X0&&x<=UI_CHAT063_POPUP_X0+UI_CHAT063_POPUP_W&&
         y>=UI_CHAT063_POPUP_Y0&&y<=UI_CHAT063_POPUP_Y0+UI_CHAT063_POPUP_H; }
