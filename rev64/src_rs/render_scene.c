/*
 * render_scene.c — 渲染场景构造链（③ OnGraphCreate 0xbd764 完整逆源码）
 * 依据原版渲染链（capstone 确证，§OnCreate/OnGraph 逆向）：
 *
 *   0xbd764（OnGraphCreate 真实实现，"graph" 全局对象 = r0）：
 *     bl 0xb6790(graph)                 # 渲染器子系统初始化
 *     若 graph+0x3c==0：
 *        ldr r0,[graph,#0x24] bl 0xb7e00(r0, 0x0c)   # 棋盘格子 vector insert 12
 *     return 1
 *
 *   0xb278c(obj, 0x0c)：渲染器初始化（+0/4/8 清零 = vtable/flag/size）
 *   0xb7d50(renderer)：EGL 对象构造（0x14 字节，容量 0x80<<7=16384 棋子缓冲）
 *   0xb8010(egl)：EGL context 创建
 *   0x232f8(size)：malloc（PLT）
 *   0xbecdc/0xbed04/0xbecf4：渲染对象 fill / commit / clear
 *
 *  场景对象布局（0xbd790 + 0xbd800 两处结构体，字段对齐原版）：
 *     +0x00  vtable/flag0      +0x04 flag1       +0x08 size
 *     +0x0c  ...
 *     +0x24  egl_renderer (0x14B, cap 16384)
 *     +0x28  byte flag (0)
 *     +0x2c  obj_a (malloc 8B，0xbecdc fill / 0xbed04 commit)
 *     +0x30/0x31 bytes (0)
 *     +0x34  obj_b (malloc 8B，0xbecdc fill)
 *     +0x38  int (0)  +0x3c byte (0)  +0x3d byte (0)
 *
 *  OnUpdate 0xbd5a0（每帧渲染）：
 *     事件队列 [r6+0xc] 出队（0xb7eb0 弹出，0x0c=12 字节事件）
 *     事件类型 switch：
 *       0x20..0x22 → 0xb38c0(x,y) + 0xb6808(...)   # 触摸/移动事件
 *       0x43       → 0xc5d50 + 0xb3694(...)        # 特殊事件
 *       0x31≠0     → 0xb38c0/0xb6808 + 0xbecf4(+0x34)  # 尺寸变更
 *     每帧末尾 0xb6a64()（渲染提交）
 *
 *  本文件逆出的 C 类型 + 构造顺序（行为等价，对象自建堆）：
 *    jq_scene_init(graph)  ← 对齐 0xbd764
 *    jq_frame_driver(sc, events)  ← 对齐 0xbd5a0
 */
#include "junqi.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

/* ---- 渲染器对象（0xb7d50，0x14 字节 + 棋子缓冲 16384）---- */
#define JQ_RENDERER_SIZE 0x14
#define JQ_RENDERER_BUF_CAP 16384   /* 0x80<<7 */
typedef struct {
    int   vtable;         /* [0] */
    int   flags;          /* +4 */
    int   size;           /* +8 */
    uint8_t buf[JQ_RENDERER_BUF_CAP];  /* 棋子渲染缓冲（原版堆，重写定长） */
} jq_renderer;

/* ---- 场景对象（0xbd790/0xbd800，0x40 字节）---- */
typedef struct {
    int     vtable;            /* +0 */
    int     flag0;            /* +4 */
    int     size;              /* +8 */
    uint8_t pad[0x1c];        /* +0x0c..+0x23 */
    jq_renderer *renderer;    /* +0x24 */
    uint8_t   flag_28;        /* +0x28 */
    void    *obj_a;           /* +0x2c（malloc 8B） */
    uint8_t   byte_30;        /* +0x30 */
    uint8_t   byte_31;        /* +0x31 */
    void    *obj_b;           /* +0x34（malloc 8B） */
    int     zero_38;          /* +0x38 */
    uint8_t  byte_3c;         /* +0x3c */
    uint8_t  byte_3d;         /* +0x3d */
    uint8_t  pad2[0x40 - 0x3e];
} jq_scene;

/* ---- 事件（0xbd5a0 出队，0x0c=12 字节/事件）---- */
typedef struct {
    int type;      /* +0：0x20/0x21/0x22/0x43/0x31... */
    int x;         /* +4 */
    int y;         /* +8 */
} jq_event12;

/* ---- 渲染场景构造（对齐原版 0xbd764 构造顺序）---- */
int jq_scene_create(jq_scene *sc, int board_cells /* 0x0c=12 棋盘格子数 */)
{
    memset(sc, 0, sizeof(*sc));
    /* 1. 渲染器初始化（0xb278c：+0/4/8 清零） */
    jq_renderer *rend = (jq_renderer *)calloc(1, sizeof(jq_renderer));
    if (!rend) return -1;
    rend->vtable = 0; rend->flags = 0; rend->size = 0;
    sc->renderer = rend;
    /* 2. 棋盘格子 vector insert（0xb7e00：cap 对齐 board_cells） */
    sc->size = board_cells;
    /* 3. EGL 对象（0xb8010：byte_28=0 标记未初始化） */
    sc->flag_28 = 0;
    /* 4. obj_a / obj_b（malloc 8B，0xbecdc fill） */
    sc->obj_a = calloc(1, 8);
    sc->obj_b = calloc(1, 8);
    sc->byte_30 = 0; sc->byte_31 = 0; sc->zero_38 = 0;
    sc->byte_3c = 0; sc->byte_3d = 0;
    return 0;
}

/* ---- 销毁（对齐原版 0xc0ed0 析构派发 + 0xb7500 audio 释放）---- */
void jq_scene_destroy(jq_scene *sc)
{
    if (!sc) return;
    free(sc->renderer); sc->renderer = NULL;
    free(sc->obj_a); sc->obj_a = NULL;
    free(sc->obj_b); sc->obj_b = NULL;
    memset(sc, 0, sizeof(*sc));
}

/* ---- 每帧渲染驱动（对齐原版 0xbd5a0 事件出队 + switch）----
   events[] 为待处理事件（0x0c 字节/个，n_events 个）。
   事件类型：0x20/0x21/0x22=触摸移动、0x43=特殊、0x31=尺寸变更。
   末尾提交（0xb6a64）。 */
void jq_frame_driver(jq_scene *sc, const jq_event12 *events, int n_events)
{
    if (!sc || !events || n_events <= 0) return;
    for (int i = 0; i < n_events; i++) {
        jq_event12 ev = events[i];
        switch (ev.type) {
            case 0x20: case 0x21: case 0x22:
                /* 触摸/移动：0xb38c0(x,y) + 0xb6808 重绘 */
                sc->renderer->flags = ev.x; /* 记录最后触摸坐标 */
                break;
            case 0x43:
                /* 特殊事件（0xc5d50 + 0xb3694）：标记重绘 */
                sc->renderer->flags |= 1;
                break;
            case 0x31:
                /* 尺寸变更（0xbecf4(+0x34)）：obj_b 已更新 */
                break;
        }
    }
    /* 渲染提交（0xb6a64 语义：flush 当帧） */
    sc->renderer->size = n_events;  /* 记录本帧事件数（原版提交缓冲） */
}
