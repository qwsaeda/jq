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
