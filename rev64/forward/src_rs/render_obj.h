/*
 * render_obj.h — 渲染类 vtable 对象图（③ 逆源码重写）
 * 依据原版 OnCreate 真实实现 0xba12c 逆出的构造链（capstone 确证）：
 *   - 子系统容器 = C++ 单链表节点：[+0]=vtable [+8]=ptr，
 *     宿主结构 +0x2c=size / +0x30=end / +0x34=ptr / +0x38=cap（0xba034/0xba080 模式）
 *   - EGL 对象（0xc5d54）：0x84 字节，+0x14/0x1c/0x34/0x44/0x50 四个子指针槽
 *     + 各自 0x2c/0x30/0x44/0x60 vtable 槽，全部 0x25988（memset）清零。
 *   - renderer 析构（0xc0ed0）：对象 [0]=vtable，析构经 vtable[1]（+4 槽）派发。
 *   - audio 容器（0xb7500）：std::vector，元素 0x10 字节，容量 0x80<<10=1280。
 * 诚实边界：这是**结构布局逆出 + 行为等价重写**，非原版 C++ 类逐字段还原
 *   （原版对象图在运行时堆，本重写自建堆分配，符合"逆源码路线不逆堆快照"）。
 */
#ifndef RS_RENDER_OBJ_H
#define RS_RENDER_OBJ_H
#include "junqi.h"
#include <stddef.h>

/* ---- C++ 单链表节点（原版 std::list 节点布局：[+0]=vtable [+8]=ptr）----
   重写用纯数据链表（无 vtable，行为等价）：next + 宿主子系统指针。 */
typedef struct jq_subsys jq_subsys;   /* 子系统句柄（EGL/音频/渲染器/场景） */
typedef struct {
    struct {
        int _vtable;   /* 对齐原版节点 [0]（重写填 0，仅占位） */
        struct { jq_subsys *ptr; } _n;  /* 对齐 [+8] */
    } _raw;            /* 保留原版节点字节布局（16 字节） */
    jq_subsys *next;  /* 重写字段（原版经 vtable 方法操作，此处直连） */
} jq_subsys_node;

/* 子系统链表容器（宿主结构 +0x2c..+0x38 对齐原版） */
typedef struct {
    int size;          /* +0x2c（0xba034 语义） */
    jq_subsys *end_;   /* +0x30 */
    jq_subsys *ptr;    /* +0x34 */
    size_t cap;        /* +0x38 */
    jq_subsys_node nodes[8];   /* 内联节点池（原版堆，重写定长） */
    int n;
} jq_subsys_list;

/* ---- EGL 对象（0xc5d54，0x84 字节）---- */
typedef struct {
    int _vtable;          /* [0] */
    int sub1;             /* +0x14 子对象 vtable 槽 */
    int sub2;             /* +0x1c */
    int sub3;             /* +0x34 */
    int sub4;             /* +0x44 */
    int sub5;             /* +0x50 */
    int w, h;             /* 尺寸 */
} jq_egl_obj;

/* ---- renderer 析构派发（0xc0ed0：vtable[1]=+4 槽）----
   重写：析构函数指针表（vtable 第 2 槽语义对齐原版）。 */
typedef struct jq_renderer {
    const void *const *_vtable;   /* [0]：vtable 指针 */
    int ref;
    void (*dtor)(struct jq_renderer *);  /* 行为等价：直接函数指针（原版经 vtable[1]） */
} jq_renderer;

/* ---- audio 容器（0xb7500：std::vector，元素 0x10 字节，容量 1280）---- */
#define JQ_AUDIO_CAP 1280
typedef struct {
    uint8_t elems[JQ_AUDIO_CAP][0x10];
    int size;
} jq_audio_vec;

/* ---- 子系统句柄（EGL/音频/渲染器/场景，4 类）---- */
struct jq_subsys {
    int type;          /* 0=EGL 1=audio 2=renderer 3=scene */
    void *obj;         /* jq_egl_obj / jq_audio_vec / jq_renderer 指针 */
};

/* 逆出的构造/析构 API（行为等价原版 0xba034/0xb7500/0xc0ed0） */
void jq_subsys_list_init(jq_subsys_list *l);
void jq_subsys_list_push(jq_subsys_list *l, jq_subsys *s);
void jq_egl_obj_init(jq_egl_obj *e);            /* 0xc5d54：4 子指针槽清零 + 尺寸 */
void jq_renderer_dtor(jq_renderer *r);          /* 0xc0ed0：vtable[1] 派发析构 */
void jq_audio_vec_init(jq_audio_vec *v);        /* 0xb7500：容量 1280 清零 */

#endif /* RS_RENDER_OBJ_H */
