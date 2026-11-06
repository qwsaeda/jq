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

