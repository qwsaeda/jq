/* render_gles2.h — R4 渲染接口（arm64 手写路线） */
#ifndef RENDER_GLES2_H
#define RENDER_GLES2_H
#include "junqi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化渲染（OnGraphChange 时调用；w/h 像素） */
void junqi_render_init(int w, int h);
/* 每帧渲染（OnUpdate/GLThread 调用），传入当前状态 */
void junqi_render_frame(const jq_state *st);
/* 设置棋盘纹理（真实 PNG 解码后的 RGBA 数据），用于替换色块 */
void junqi_render_set_board_texture(const unsigned char *rgba, int w, int h);
/* 设置选中格（-1=无），渲染时高亮 */
void junqi_render_set_selected(int idx);
/* 设置棋子贴图（真实 72×72 PNG 解码 RGBA），按方+rank 注册。
   棋子格渲染时用对应贴图；未注册的回退色块。 */
void junqi_render_set_piece_texture(int side, int rank,
                                    const unsigned char *rgba, int w, int h);
/* 设置棋子图集（原版 data/sg/piece.png，480×490，12列×10行，40×49/格）。
   rank→列(0-11)，side→行(蓝0-4/红5-9)。图集渲染优先于色块/数字。 */
void junqi_render_set_piece_atlas(const unsigned char *rgba, int w, int h);
/* 屏幕像素坐标 → 棋盘格索引（与渲染棋盘区域一致，-1=棋盘外） */
int junqi_render_touch_cell(float px, float py, int w, int h);
/* 布阵模式：缩小棋盘留出底部确认按钮（与 touch 区域一致） */
void junqi_render_set_setup(int on);
/* 布阵确认按钮命中测试（像素坐标，1=命中）。需先 junqi_render_set_setup(1)。 */
int junqi_render_setup_confirm_hit(float px, float py, int w, int h);

#ifdef __cplusplus
}
#endif
#endif /* RENDER_GLES2_H */
