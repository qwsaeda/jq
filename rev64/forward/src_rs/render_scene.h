#ifndef RS_RENDER_SCENE_H
#define RS_RENDER_SCENE_H
#include "junqi.h"
#include <stddef.h>

#define JQ_RENDERER_BUF_CAP 16384

typedef struct {
    int   vtable, flags, size;
    uint8_t buf[JQ_RENDERER_BUF_CAP];
} jq_renderer;

typedef struct {
    int     vtable;
    int     flag0, size;
    uint8_t pad[0x1c];
    jq_renderer *renderer;    /* +0x24 */
    uint8_t flag_28;
    void   *obj_a, *obj_b;
    int     zero_38;
    uint8_t byte_30, byte_31, byte_3c, byte_3d;
} jq_scene;

typedef struct { int type, x, y; } jq_event12;

int  jq_scene_create(jq_scene *sc, int board_cells);
void jq_scene_destroy(jq_scene *sc);
void jq_frame_driver(jq_scene *sc, const jq_event12 *events, int n);

#endif
