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

/* ★ P2（datah_000 证据）：软件信息弹窗 2 按钮桩（反馈/关闭）。
   诚实边界：原版反馈=邮件/客服通道（未逆向），src_rs 无反馈通道，
   反馈按钮→诚实边界对话框（几何表返回 720 基准占位供一致性/evverify 链接；
   真机走 ui_menu.c 真实几何表）。 */
static const ui_about_btn ab_stub[UI_ABOUT_COUNT] = {
    { 200.0f,698.0f,340.0f,758.0f,0 },{ 380.0f,698.0f,520.0f,758.0f,1 },
};
const ui_about_btn *ui_about_buttons(void) { return ab_stub; }
int ui_about_hit(const ui_about_btn *b, int n, float x, float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_about_popup_inside(float x, float y)
{ return x>=UI_ABOUT_POPUP_X0&&x<=UI_ABOUT_POPUP_X0+UI_ABOUT_POPUP_W&&
         y>=UI_ABOUT_POPUP_Y0&&y<=UI_ABOUT_POPUP_Y0+UI_ABOUT_POPUP_H; }

/* ★ SPAK2UI 批量（12 弹窗）：战况统计子菜单链 + 玩家/对局/对战方式/创建游戏/版本更新。
   桌面桩与 ui_menu.c 真实几何表同步（诚实边界占位不包装成"已实现"）。 */
static const ui_ui44_btn ui44_stub[UI_UI44_COUNT] = {
    { 65.0f,640.0f,205.0f,700.0f,0 },{ 65.0f,715.0f,205.0f,775.0f,1 },
};
const ui_ui44_btn *ui_ui44_buttons(void) { return ui44_stub; }
int ui_ui44_hit(const ui_ui44_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui44_popup_inside(float x,float y)
{ return x>=UI_UI44_POPUP_X0&&x<=UI_UI44_POPUP_X0+UI_UI44_POPUP_W&&
         y>=UI_UI44_POPUP_Y0&&y<=UI_UI44_POPUP_Y0+UI_UI44_POPUP_H; }
static const ui_ui45_btn ui45_stub[UI_UI45_COUNT] = {
    { 320.0f,658.0f,412.0f,718.0f,0 },
};
const ui_ui45_btn *ui_ui45_buttons(void) { return ui45_stub; }
int ui_ui45_hit(const ui_ui45_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui45_popup_inside(float x,float y)
{ return x>=UI_UI45_POPUP_X0&&x<=UI_UI45_POPUP_X0+UI_UI45_POPUP_W&&
         y>=UI_UI45_POPUP_Y0&&y<=UI_UI45_POPUP_Y0+UI_UI45_POPUP_H; }
static const ui_ui46_btn ui46_stub[UI_UI46_COUNT] = {
    { 270.0f,765.0f,450.0f,825.0f,0 },
};
const ui_ui46_btn *ui_ui46_buttons(void) { return ui46_stub; }
int ui_ui46_hit(const ui_ui46_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui46_popup_inside(float x,float y)
{ return x>=UI_UI46_POPUP_X0&&x<=UI_UI46_POPUP_X0+UI_UI46_POPUP_W&&
         y>=UI_UI46_POPUP_Y0&&y<=UI_UI46_POPUP_Y0+UI_UI46_POPUP_H; }
static const ui_ui47_btn ui47_stub[UI_UI47_COUNT] = {
    { 270.0f,708.0f,450.0f,768.0f,0 },
};
const ui_ui47_btn *ui_ui47_buttons(void) { return ui47_stub; }
int ui_ui47_hit(const ui_ui47_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui47_popup_inside(float x,float y)
{ return x>=UI_UI47_POPUP_X0&&x<=UI_UI47_POPUP_X0+UI_UI47_POPUP_W&&
         y>=UI_UI47_POPUP_Y0&&y<=UI_UI47_POPUP_Y0+UI_UI47_POPUP_H; }
static const ui_ui49_btn ui49_stub[UI_UI49_COUNT] = {
    { 270.0f,860.0f,450.0f,920.0f,0 },
};
const ui_ui49_btn *ui_ui49_buttons(void) { return ui49_stub; }
int ui_ui49_hit(const ui_ui49_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui49_popup_inside(float x,float y)
{ return x>=UI_UI49_POPUP_X0&&x<=UI_UI49_POPUP_X0+UI_UI49_POPUP_W&&
         y>=UI_UI49_POPUP_Y0&&y<=UI_UI49_POPUP_Y0+UI_UI49_POPUP_H; }
static const ui_ui51_btn ui51_stub[UI_UI51_COUNT] = {
    { 180.0f,860.0f,337.0f,920.0f,0 },{ 383.0f,860.0f,540.0f,920.0f,1 },
};
const ui_ui51_btn *ui_ui51_buttons(void) { return ui51_stub; }
int ui_ui51_hit(const ui_ui51_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui51_popup_inside(float x,float y)
{ return x>=UI_UI51_POPUP_X0&&x<=UI_UI51_POPUP_X0+UI_UI51_POPUP_W&&
         y>=UI_UI51_POPUP_Y0&&y<=UI_UI51_POPUP_Y0+UI_UI51_POPUP_H; }
static const ui_ui54_btn ui54_stub[UI_UI54_COUNT] = {
    { 180.0f,860.0f,337.0f,920.0f,0 },{ 383.0f,860.0f,540.0f,920.0f,1 },
};
const ui_ui54_btn *ui_ui54_buttons(void) { return ui54_stub; }
int ui_ui54_hit(const ui_ui54_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui54_popup_inside(float x,float y)
{ return x>=UI_UI54_POPUP_X0&&x<=UI_UI54_POPUP_X0+UI_UI54_POPUP_W&&
         y>=UI_UI54_POPUP_Y0&&y<=UI_UI54_POPUP_Y0+UI_UI54_POPUP_H; }
static const ui_ui73_btn ui73_stub[UI_UI73_COUNT] = {
    { 165.0f,695.0f,345.0f,755.0f,0 },{ 375.0f,695.0f,555.0f,755.0f,1 },
};
const ui_ui73_btn *ui_ui73_buttons(void) { return ui73_stub; }
int ui_ui73_hit(const ui_ui73_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui73_popup_inside(float x,float y)
{ return x>=UI_UI73_POPUP_X0&&x<=UI_UI73_POPUP_X0+UI_UI73_POPUP_W&&
         y>=UI_UI73_POPUP_Y0&&y<=UI_UI73_POPUP_Y0+UI_UI73_POPUP_H; }
static const ui_ui90_btn ui90_stub[UI_UI90_COUNT] = {
    { 135.0f,570.0f,345.0f,630.0f,0 },{ 135.0f,660.0f,345.0f,720.0f,1 },
};
const ui_ui90_btn *ui_ui90_buttons(void) { return ui90_stub; }
int ui_ui90_hit(const ui_ui90_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui90_popup_inside(float x,float y)
{ return x>=UI_UI90_POPUP_X0&&x<=UI_UI90_POPUP_X0+UI_UI90_POPUP_W&&
         y>=UI_UI90_POPUP_Y0&&y<=UI_UI90_POPUP_Y0+UI_UI90_POPUP_H; }
static const ui_ui100_btn ui100_stub[UI_UI100_COUNT] = {
    { 320.0f,658.0f,412.0f,718.0f,0 },
};
const ui_ui100_btn *ui_ui100_buttons(void) { return ui100_stub; }
int ui_ui100_hit(const ui_ui100_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui100_popup_inside(float x,float y)
{ return x>=UI_UI100_POPUP_X0&&x<=UI_UI100_POPUP_X0+UI_UI100_POPUP_W&&
         y>=UI_UI100_POPUP_Y0&&y<=UI_UI100_POPUP_Y0+UI_UI100_POPUP_H; }
static const ui_ui110_btn ui110_stub[UI_UI110_COUNT] = {
    { 180.0f,860.0f,337.0f,920.0f,0 },{ 383.0f,860.0f,540.0f,920.0f,1 },
};
const ui_ui110_btn *ui_ui110_buttons(void) { return ui110_stub; }
int ui_ui110_hit(const ui_ui110_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui110_popup_inside(float x,float y)
{ return x>=UI_UI110_POPUP_X0&&x<=UI_UI110_POPUP_X0+UI_UI110_POPUP_W&&
         y>=UI_UI110_POPUP_Y0&&y<=UI_UI110_POPUP_Y0+UI_UI110_POPUP_H; }
static const ui_ui118_btn ui118_stub[UI_UI118_COUNT] = {
    { 175.0f,735.0f,340.0f,795.0f,0 },{ 380.0f,735.0f,545.0f,795.0f,1 },
};
const ui_ui118_btn *ui_ui118_buttons(void) { return ui118_stub; }
int ui_ui118_hit(const ui_ui118_btn *b,int n,float x,float y)
{ for (int i=0;i<n;i++) if (x>=b[i].x0&&x<=b[i].x1&&y>=b[i].y0&&y<=b[i].y1) return i; return -1; }
int ui_ui118_popup_inside(float x,float y)
{ return x>=UI_UI118_POPUP_X0&&x<=UI_UI118_POPUP_X0+UI_UI118_POPUP_W&&
         y>=UI_UI118_POPUP_Y0&&y<=UI_UI118_POPUP_Y0+UI_UI118_POPUP_H; }

/* ★ 返回键 3 态确认弹窗按钮表（原版 .rodata ExitConfirm/ResignConfirm/RegretConfirm）
   720×1280 居中 400×280 面板（偏移 +160,+500），2 按钮 720 基准。
   ★ 诚实边界：原版确认对话框几何未逆向，简化近似（对齐原版 3 态行为） */
static const ui_exitconfirm_btn g_exitconfirm_btns[UI_EXITCONFIRM_COUNT] = {
    { 180.0f, 690.0f, 340.0f, 750.0f, 0 },   /* 退出 */
    { 380.0f, 690.0f, 540.0f, 750.0f, 1 },   /* 继续 */
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

/* ★ 批量对齐（datah_001）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d001_btn g_d001_btns[UI_D001_COUNT] = {
    { 355.0f, 520.0f, 555.0f, 580.0f, 0 },  /* Sound */
    { 355.0f, 595.0f, 555.0f, 655.0f, 1 },  /* BtnSound */
    { 165.0f, 750.0f, 345.0f, 810.0f, 2 },  /* 确定 */
    { 375.0f, 750.0f, 555.0f, 810.0f, 3 },  /* 取消 */
};
const ui_d001_btn *ui_d001_buttons(void) { return g_d001_btns; }
int ui_d001_hit(const ui_d001_btn *b, int n, float x, float y)
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
/* ★ 批量对齐（datah_013）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d013_btn g_d013_btns[UI_D013_COUNT] = {
    { 260.0f, 540.0f, 460.0f, 600.0f, 0 },  /* 0 */
    { 260.0f, 610.0f, 460.0f, 670.0f, 1 },  /* 1 */
    { 260.0f, 680.0f, 460.0f, 740.0f, 2 },  /* 2 */
};
const ui_d013_btn *ui_d013_buttons(void) { return g_d013_btns; }
int ui_d013_hit(const ui_d013_btn *b, int n, float x, float y)
{ for (int i = 0; i < n; i++) if (x>=b[i].x0 && x<=b[i].x1 && y>=b[i].y0 && y<=b[i].y1) return i; return -1; }
/* ★ 批量对齐（datah_019）：4 按钮（720 基准，局部 1:1 平移） */
static const ui_d019_btn g_d019_btns[UI_D019_COUNT] = {
    { -210.0f, -185.0f, -105.0f, -50.0f, 0 },  /* 清空布局 */
    { -105.0f, -185.0f, 0.0f, -50.0f, 1 },  /* 载入布局 */
    { 0.0f, -185.0f, 105.0f, -50.0f, 2 },  /* 保存布局 */
    { 105.0f, -185.0f, 210.0f, -50.0f, 3 },  /* 完成布局 */
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
/* ★ 批量对齐（datah_025）：1 按钮（720 基准，局部 1:1 平移） */
static const ui_d025_btn g_d025_btns[UI_D025_COUNT] = {
    { 270.0f, 665.0f, 450.0f, 725.0f, 0 },  /* 确定 */
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
    { 255.0f, 530.0f, 465.0f, 590.0f, 0 },  /* 规则 - 翻棋 */
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
/* ★ 批量对齐（datah_050）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d050_btn g_d050_btns[UI_D050_COUNT] = {
    { 175.0f, 745.0f, 285.0f, 805.0f, 0 },  /* 复盘 */
    { 305.0f, 745.0f, 415.0f, 805.0f, 1 },  /* 删除 */
    { 435.0f, 745.0f, 545.0f, 805.0f, 2 },  /* 返回 */
};
const ui_d050_btn *ui_d050_buttons(void) { return g_d050_btns; }
int ui_d050_hit(const ui_d050_btn *b, int n, float x, float y)
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
/* ★ 批量对齐（datah_066）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d066_btn g_d066_btns[UI_D066_COUNT] = {
    { 255.0f, 535.0f, 465.0f, 595.0f, 0 },  /* 创建游戏 */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },  /* 加入游戏 */
    { 255.0f, 685.0f, 465.0f, 745.0f, 2 },  /* 返回 */
};
const ui_d066_btn *ui_d066_buttons(void) { return g_d066_btns; }
int ui_d066_hit(const ui_d066_btn *b, int n, float x, float y)
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
    { 255.0f, 648.0f, 465.0f, 708.0f, 2 },  /* 个人资料 */
    { 255.0f, 722.0f, 465.0f, 782.0f, 3 },  /* 返回 */
};
const ui_d072_btn *ui_d072_buttons(void) { return g_d072_btns; }
int ui_d072_hit(const ui_d072_btn *b, int n, float x, float y)
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
    { 15.0f, 0.0f, 85.0f, 90.0f, 0 },  /* 开局 */
    { 95.0f, 0.0f, 165.0f, 90.0f, 1 },  /* 终局 */
    { 175.0f, 0.0f, 245.0f, 90.0f, 2 },  /* 定位 */
};
const ui_d109_btn *ui_d109_buttons(void) { return g_d109_btns; }
int ui_d109_hit(const ui_d109_btn *b, int n, float x, float y)
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
/* ★ 批量对齐（datah_120）：3 按钮（720 基准，局部 1:1 平移） */
static const ui_d120_btn g_d120_btns[UI_D120_COUNT] = {
    { 255.0f, 535.0f, 465.0f, 595.0f, 0 },  /* 注册用户 */
    { 255.0f, 610.0f, 465.0f, 670.0f, 1 },  /* 找回密码 */
    { 255.0f, 685.0f, 465.0f, 745.0f, 2 },  /* 返回 */
};
const ui_d120_btn *ui_d120_buttons(void) { return g_d120_btns; }
int ui_d120_hit(const ui_d120_btn *b, int n, float x, float y)
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