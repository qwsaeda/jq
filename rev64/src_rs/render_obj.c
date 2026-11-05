/*
 * render_obj.c — 渲染类 vtable 对象图（③ 逆源码实现）
 * 对齐原版构造链（0xba12c → 0xba034/0xba080/0xc5d54/0xb7500/0xc0ed0，capstone 确证）。
 * 行为等价重写：链表/vtable 派发语义对齐原版，对象自建堆（不逆堆快照）。
 */
#include "render_obj.h"
#include <string.h>

void jq_subsys_list_init(jq_subsys_list *l)
{
    memset(l, 0, sizeof(*l));
}

/* 0xba034/0xba080 语义：节点入链（+0x2c size / +0x30 end / +0x34 ptr / +0x38 cap） */
void jq_subsys_list_push(jq_subsys_list *l, jq_subsys *s)
{
    if (l->n >= 8) return;
    jq_subsys_node *nd = &l->nodes[l->n++];
    nd->_raw._n.ptr = s;
    nd->next = NULL;
    l->size++;
    l->end_ = s;
    l->ptr = s;
    l->cap = 8;
}

/* 0xc5d54 语义：EGL 对象 4 子指针槽清零（0x25988 memset）+ 尺寸 */
void jq_egl_obj_init(jq_egl_obj *e)
{
    memset(e, 0, sizeof(*e));   /* 对齐原版：5 个子 vtable 槽全清零 */
}

/* 0xc0ed0 语义：renderer 析构经 vtable[1]（+4 槽）派发 → 重写为直接函数指针 */
void jq_renderer_dtor(jq_renderer *r)
{
    if (r && r->dtor) r->dtor(r);
    r->ref = 0;
}

/* 0xb7500 语义：std::vector 容量 1280 清零 */
void jq_audio_vec_init(jq_audio_vec *v)
{
    memset(v, 0, sizeof(*v));
}
