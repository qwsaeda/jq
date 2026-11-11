/* jq_img.S — 原版 .so 镜像嵌入（.incbin，虚拟地址=偏移，基址 0） */
    .section .rodata
    .align 4
    .globl g_jqt_img
    .type g_jqt_img, %object
g_jqt_img:
    .incbin "/tmp/opencode/shadow/jq_img.bin"
    .globl g_jqt_img_end
    .type g_jqt_img_end, %object
g_jqt_img_end:
