#ifndef RS_CONTAINERS_H
#define RS_CONTAINERS_H
#include "junqi.h"
#define JQ_LAYOUT_CAP 240
typedef struct {
    int count_tbl[RANK_COUNT];
    int layout_vec[JQ_LAYOUT_CAP];
    int layout_size;
} jq_layout_container;
jq_layout_err jq_container_validate(jq_layout_container *cc, const jq_state *st);
void jq_container_fill_layout(jq_layout_container *cc, const uint8_t layout[30]);
#endif
