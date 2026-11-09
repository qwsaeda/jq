#ifndef JQ_DEFAULT_LAY_H
#define JQ_DEFAULT_LAY_H
/* ★ AUD-1000-ALL A4-13（P1 已修）：JQ_DEFAULT_LAY[30] 双端共享头——旧版 64 位
   src_rs/layout.c:21 与 32 位 jq_rs_layout.c:16 各独立定义一份（现逐字节一致），
   改一处忘另一处 → 32/64 位开局布局分叉。抽单点定义双端 #include 同源。
   数据 = default.lay 实测（L52 决定性证据），32 位 .rodata 0x300f0 区对照一致。 */
#include <stdint.h>
const uint8_t JQ_DEFAULT_LAY[30] = {
    3,7,6,5,8, 10,0,9,0,1,
    2,10,0,9,5, 4,0,6,0,7,
    8,3,4,9,11, 11,12,11,8,7
};
#endif
