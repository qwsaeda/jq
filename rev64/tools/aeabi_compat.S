/* ARM EABI helper compat for AArch64.
 * 原版 32 位 libjunqi.so 是 armeabi(软浮点)，调用 __aeabi_fadd/__aeabi_uidiv 等
 * ARM EABI 助手。arm64 无这些符号（浮点原生 + div 原生），需提供兼容实现。
 * 调用约定：翻译后的代码用 x0-x7 传参，结果在 x0（32 位值在 w0）。
 * 浮点结果以位模式放在 w0（ARM EABI 约定 r0=浮点位模式）。
 */
.text

// ---- 整数除/模 ----
.global __aeabi_uidiv
.type __aeabi_uidiv,%function
__aeabi_uidiv:
    udiv w2, w0, w1
    mov w0, w2
    ret
.size __aeabi_uidiv,.-__aeabi_uidiv

.global __aeabi_uidivmod
.type __aeabi_uidivmod,%function
__aeabi_uidivmod:
    udiv w2, w0, w1
    msub w3, w2, w1, w0
    mov w0, w2
    mov w1, w3
    ret
.size __aeabi_uidivmod,.-__aeabi_uidivmod

.global __aeabi_idiv
.type __aeabi_idiv,%function
__aeabi_idiv:
    sdiv w2, w0, w1
    mov w0, w2
    ret
.size __aeabi_idiv,.-__aeabi_idiv

.global __aeabi_idivmod
.type __aeabi_idivmod,%function
__aeabi_idivmod:
    sdiv w2, w0, w1
    msub w3, w2, w1, w0
    mov w0, w2
    mov w1, w3
    ret
.size __aeabi_idivmod,.-__aeabi_idivmod

.global __aeabi_ldivmod
.type __aeabi_ldivmod,%function
__aeabi_ldivmod:
    sdiv x2, x0, x1
    msub x3, x2, x1, x0
    mov x0, x2
    mov x1, x3
    ret
.size __aeabi_ldivmod,.-__aeabi_ldivmod

.global __aeabi_uldivmod
.type __aeabi_uldivmod,%function
__aeabi_uldivmod:
    udiv x2, x0, x1
    msub x3, x2, x1, x0
    mov x0, x2
    mov x1, x3
    ret
.size __aeabi_uldivmod,.-__aeabi_uldivmod

.global __aeabi_lmul
.type __aeabi_lmul,%function
__aeabi_lmul:
    mul x0, x0, x1
    ret
.size __aeabi_lmul,.-__aeabi_lmul

// ---- 单精度浮点（位模式经 w0/w1，结果位模式回 w0）----
.macro F2OP name, op
.global \name
.type \name,%function
\name:
    fmov s0, w0
    fmov s1, w1
    \op s2, s0, s1
    fmov w0, s2
    ret
.size \name, .-\name
.endm
F2OP __aeabi_fadd, fadd
F2OP __aeabi_fsub, fsub
F2OP __aeabi_fmul, fmul
F2OP __aeabi_fdiv, fdiv

.macro FCMPOP name, cond
.global \name
.type \name,%function
\name:
    fmov s0, w0
    fmov s1, w1
    fcmp s0, s1
    cset w0, \cond
    ret
.size \name, .-\name
.endm
FCMPOP __aeabi_fcmpeq, eq
FCMPOP __aeabi_fcmpgt, gt
FCMPOP __aeabi_fcmplt, lt

.global __aeabi_f2iz
.type __aeabi_f2iz,%function
__aeabi_f2iz:
    fmov s0, w0
    fcvtzs w0, s0
    ret
.size __aeabi_f2iz,.-__aeabi_f2iz

.global __aeabi_f2d
.type __aeabi_f2d,%function
__aeabi_f2d:
    fmov s0, w0
    fcvt d1, s0
    fmov x0, d1
    ret
.size __aeabi_f2d,.-__aeabi_f2d

.global __aeabi_i2f
.type __aeabi_i2f,%function
__aeabi_i2f:
    scvtf s0, w0
    fmov w0, s0
    ret
.size __aeabi_i2f,.-__aeabi_i2f

.global __aeabi_i2d
.type __aeabi_i2d,%function
__aeabi_i2d:
    scvtf d0, w0
    fmov x0, d0
    ret
.size __aeabi_i2d,.-__aeabi_i2d

.global __aeabi_ui2f
.type __aeabi_ui2f,%function
__aeabi_ui2f:
    ucvtf s0, w0
    fmov w0, s0
    ret
.size __aeabi_ui2f,.-__aeabi_ui2f

.global __aeabi_ui2d
.type __aeabi_ui2d,%function
__aeabi_ui2d:
    ucvtf d0, w0
    fmov x0, d0
    ret
.size __aeabi_ui2d,.-__aeabi_ui2d

.global __aeabi_l2f
.type __aeabi_l2f,%function
__aeabi_l2f:
    scvtf s0, x0
    fmov w0, s0
    ret
.size __aeabi_l2f,.-__aeabi_l2f

.global __aeabi_ul2d
.type __aeabi_ul2d,%function
__aeabi_ul2d:
    ucvtf d0, x0
    fmov x0, d0
    ret
.size __aeabi_ul2d,.-__aeabi_ul2d

// ---- 双精度浮点（位模式经 x0/x1，结果位模式回 x0）----
.macro D2OP name, op
.global \name
.type \name,%function
\name:
    fmov d0, x0
    fmov d1, x1
    \op d2, d0, d1
    fmov x0, d2
    ret
.size \name, .-\name
.endm
D2OP __aeabi_dadd, fadd
D2OP __aeabi_dsub, fsub
D2OP __aeabi_dmul, fmul
D2OP __aeabi_ddiv, fdiv

.macro DCMPOP name, cond
.global \name
.type \name,%function
\name:
    fmov d0, x0
    fmov d1, x1
    fcmp d0, d1
    cset w0, \cond
    ret
.size \name, .-\name
.endm
DCMPOP __aeabi_dcmpeq, eq
DCMPOP __aeabi_dcmpge, ge
DCMPOP __aeabi_dcmpgt, gt
DCMPOP __aeabi_dcmplt, lt

.global __aeabi_d2f
.type __aeabi_d2f,%function
__aeabi_d2f:
    fmov d0, x0
    fcvt s1, d0
    fmov w0, s1
    ret
.size __aeabi_d2f,.-__aeabi_d2f

.global __aeabi_d2lz
.type __aeabi_d2lz,%function
__aeabi_d2lz:
    fmov d0, x0
    fcvtzs x0, d0
    ret
.size __aeabi_d2lz,.-__aeabi_d2lz

.global __aeabi_d2ulz
.type __aeabi_d2ulz,%function
__aeabi_d2ulz:
    fmov d0, x0
    fcvtzu x0, d0
    ret
.size __aeabi_d2ulz,.-__aeabi_d2ulz

// __aeabi_atexit 在 arm64 无此符号（glibc 用 __cxa_atexit）
.global __aeabi_atexit
.type __aeabi_atexit,%function
__aeabi_atexit:
    // __aeabi_atexit(obj, func, dso) -> __cxa_atexit(func, obj, dso)
    mov x2, x0
    mov x0, x1
    mov x1, x2
    b __cxa_atexit
.size __aeabi_atexit,.-__aeabi_atexit

.section .note.GNU-stack,"",@progbits
