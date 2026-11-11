/* jq_imports.c — 102 个导入符号的 32 位 ABI 适配实现
 *
 * 影子地址空间中所有指针都是 32 位影子地址，宿主指针通过 jq_handle/jq_host
 * 映射；结构体布局差异（stat/timespec/dirent/sem_t/pthread_mutex_t）在此翻译。
 */
#include "jq_shadow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* GL 函数（arm64 链接 libGLESv1_CM；x86_64 测试由 jq_test.c 提供桩） */
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef float GLfloat;
typedef unsigned char GLboolean;
typedef int GLfixed;
extern void glBindTexture(GLenum, GLuint);
extern void glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum);
extern void glClear(GLbitfield);
extern void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat);
extern void glDeleteProgram(GLuint);
extern void glDeleteShader(GLuint);
extern void glDeleteTextures(GLsizei, const GLuint *);
extern void glDisable(GLenum);
extern void glDrawArrays(GLenum, GLint, GLsizei);
extern void glEnable(GLenum);
extern void glEnableVertexAttribArray(GLuint);
extern void glFlush(void);
extern GLint glGetAttribLocation(GLuint, const char *);
extern void glGetIntegerv(GLenum, GLint *);
extern const unsigned char *glGetString(GLenum);
extern GLint glGetUniformLocation(GLuint, const char *);
extern void glPixelStorei(GLenum, GLint);
extern void glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
extern void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
extern void glUniform1i(GLint, GLint);
extern void glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat *);
extern void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
extern void glViewport(GLint, GLint, GLsizei, GLsizei);

/* ---------------- 小工具 ---------------- */
static float f32(uint32_t x) { float f; memcpy(&f, &x, 4); return f; }
static uint32_t b32(float f) { uint32_t x; memcpy(&x, &f, 4); return x; }
static double f64(uint32_t lo, uint32_t hi) {
    uint64_t u = ((uint64_t)hi << 32) | lo; double d; memcpy(&d, &u, 8); return d;
}
static uint64_t u64(uint32_t lo, uint32_t hi) { return ((uint64_t)hi << 32) | lo; }
static uint32_t lo32(uint64_t v) { return (uint32_t)v; }
static uint32_t hi32(uint64_t v) { return (uint32_t)(v >> 32); }

/* 48 位对齐（8 字节）的 varargs 取参游标 */
typedef struct { uint32_t r[4]; int i, n; uint32_t sp; } va32;
static uint32_t va_next(va32 *v) {
    if (v->i < v->n) return v->r[v->i++];
    uint32_t x = MEM(v->sp);
    v->sp += 4;
    return x;
}
static uint64_t va_next64(va32 *v) {
    /* AAPCS: 64 位实参在影子栈上需 8 字节对齐 */
    if (v->i >= v->n && (v->sp & 4)) { v->sp += 4; }
    uint32_t lo = va_next(v), hi = va_next(v);
    return u64(lo, hi);
}

/* ---------------- 内存 / C++ 运行时 ---------------- */
uint32_t jq_imp__Znwj(JQ_ARGS) { return jq_malloc(r0); }
uint32_t jq_imp__Znaj(JQ_ARGS) { return jq_malloc(r0); }
uint32_t jq_imp__ZdlPv(JQ_ARGS) { jq_free(r0); return 0; }
uint32_t jq_imp_free(JQ_ARGS) { jq_free(r0); return 0; }
uint32_t jq_imp__ZdaPv(JQ_ARGS) { jq_free(r0); return 0; }

uint32_t jq_imp___cxa_guard_acquire(JQ_ARGS) {
    if (!r0) return 1;
    if (MEM(r0) != 0) return 0;
    MEM(r0) = 1;
    return 1;
}
static char g_last_puts[512];
uint32_t jq_imp___cxa_guard_release(JQ_ARGS) { if (r0) MEM(r0) = 1; return 0; }
uint32_t jq_imp___stack_chk_fail(JQ_ARGS) { jq_log("[jq] __stack_chk_fail\n"); abort(); return 0; }
uint32_t jq_imp_abort(JQ_ARGS) { jq_log("[jq] abort() 被调用（前一条 puts: %s）\n", g_last_puts); abort(); return 0; }
uint32_t jq_imp_exit(JQ_ARGS) { jq_log("[jq] exit(%d)\n", (int)r0); return 0; }
uint32_t jq_imp___errno(JQ_ARGS) { return 0x15bea0u; }

/* ---------------- 软浮点 ---------------- */
uint32_t jq_imp___aeabi_fadd(JQ_ARGS) { return b32(f32(r0) + f32(r1)); }
uint32_t jq_imp___aeabi_fsub(JQ_ARGS) { return b32(f32(r0) - f32(r1)); }
uint32_t jq_imp___aeabi_fmul(JQ_ARGS) { return b32(f32(r0) * f32(r1)); }
uint32_t jq_imp___aeabi_fdiv(JQ_ARGS) { return b32(f32(r0) / f32(r1)); }
uint32_t jq_imp___aeabi_fcmpgt(JQ_ARGS) { return f32(r0) > f32(r1); }
uint32_t jq_imp___aeabi_fcmplt(JQ_ARGS) { return f32(r0) < f32(r1); }
uint32_t jq_imp___aeabi_f2d(JQ_ARGS) {
    double d = (double)f32(r0); uint64_t u; memcpy(&u, &d, 8);
    r1 = hi32(u); JQ_RET(lo32(u));
}
uint32_t jq_imp___aeabi_i2f(JQ_ARGS) { return b32((float)(int32_t)r0); }
uint32_t jq_imp___aeabi_f2iz(JQ_ARGS) { return (uint32_t)(int32_t)f32(r0); }
uint32_t jq_imp___aeabi_l2f(JQ_ARGS) { return b32((float)(int64_t)u64(r0, r1)); }

uint32_t jq_imp___aeabi_idiv(JQ_ARGS) {
    int32_t a = (int32_t)r0, b = (int32_t)r1;
    if (!b) return 0;
    return (uint32_t)(a / b);
}
uint32_t jq_imp___aeabi_uidiv(JQ_ARGS) { return r1 ? r0 / r1 : 0; }
uint32_t jq_imp___aeabi_idivmod(JQ_ARGS) {
    int32_t a = (int32_t)r0, b = (int32_t)r1;
    if (!b) { r1 = 0; JQ_RET(0); }
    r1 = (uint32_t)(a % b);
    JQ_RET((uint32_t)(a / b));
}
uint32_t jq_imp___aeabi_uidivmod(JQ_ARGS) {
    if (!r1) { r1 = 0; JQ_RET(0); }
    uint32_t q = r0 / r1;   /* 商在 r0 */
    r1 = r0 % r1;           /* 余数在 r1 */
    JQ_RET(q);
}
uint32_t jq_imp___aeabi_ldivmod(JQ_ARGS) {
    int64_t a = (int64_t)u64(r0, r1), b = (int64_t)u64(r2, r3);
    if (!b) { r1 = 0; r2 = 0; r3 = 0; JQ_RET(0); }
    int64_t q = a / b, m = a % b;
    r1 = hi32((uint64_t)q); r2 = lo32((uint64_t)m); r3 = hi32((uint64_t)m);
    JQ_RET(lo32((uint64_t)q));
}
uint32_t jq_imp___aeabi_lmul(JQ_ARGS) {
    int64_t v = (int64_t)u64(r0, r1) * (int64_t)u64(r2, r3);
    r1 = hi32((uint64_t)v);
    JQ_RET(lo32((uint64_t)v));
}

/* ---------------- 字符串 / 内存 ---------------- */
uint32_t jq_imp_memcpy(JQ_ARGS) { memcpy(MEMP(r0), MEMP(r1), r2); return r0; }
uint32_t jq_imp_memmove(JQ_ARGS) { memmove(MEMP(r0), MEMP(r1), r2); return r0; }
uint32_t jq_imp_memset(JQ_ARGS) { memset(MEMP(r0), (int)r1, r2); return r0; }
uint32_t jq_imp_memcmp(JQ_ARGS) { return (uint32_t)memcmp(MEMP(r0), MEMP(r1), r2); }
uint32_t jq_imp_memchr(JQ_ARGS) {
    void *p = memchr(MEMP(r0), (int)r1, r2);
    return p ? (uint32_t)((uint8_t *)p - g_jq_base) : 0;
}
uint32_t jq_imp_strcmp(JQ_ARGS) { return (uint32_t)strcmp(MEMP(r0), MEMP(r1)); }
uint32_t jq_imp_strncmp(JQ_ARGS) { return (uint32_t)strncmp(MEMP(r0), MEMP(r1), r2); }
uint32_t jq_imp_strlen(JQ_ARGS) { return (uint32_t)strlen(MEMP(r0)); }
uint32_t jq_imp_strrchr(JQ_ARGS) {
    char *p = strrchr(MEMP(r0), (int)r1);
    return p ? (uint32_t)((uint8_t *)p - g_jq_base) : 0;
}
uint32_t jq_imp_strtol(JQ_ARGS) {
    char *end = 0;
    long v = strtol(MEMP(r0), &end, (int)r2);
    if (r1) MEM(r1) = end ? (uint32_t)((uint8_t *)end - g_jq_base) : 0;
    return (uint32_t)v;
}
uint32_t jq_imp_wmemcpy(JQ_ARGS) { wmemcpy((wchar_t *)MEMP(r0), (const wchar_t *)MEMP(r1), r2); return r0; }
uint32_t jq_imp_wmemset(JQ_ARGS) { wmemset((wchar_t *)MEMP(r0), (wchar_t)r1, r2); return r0; }

/* ---------------- 格式化 ---------------- */
static uint32_t fmt_shadow(char *dst, uint32_t cap, const char *fmt, va32 *va) {
    uint32_t n = 0;
    const char *p = fmt;
    while (*p) {
        if (*p != '%') { if (n + 1 < cap) dst[n] = *p; n++; p++; continue; }
        p++;
        if (*p == '%') { if (n + 1 < cap) dst[n] = '%'; n++; p++; continue; }
        char spec[32]; int si = 0;
        spec[si++] = '%';
        while (*p && strchr("-+ #0", *p) && si < 24) spec[si++] = *p++;
        while (*p >= '0' && *p <= '9' && si < 24) spec[si++] = *p++;
        if (*p == '.') { spec[si++] = *p++; while (*p >= '0' && *p <= '9' && si < 24) spec[si++] = *p++; }
        int len = 0;
        if (*p == 'l') { len = 1; p++; if (*p == 'l') { len = 2; p++; } }
        else if (*p == 'h') { len = -1; p++; if (*p == 'h') p++; }
        else if (*p == 'z' || *p == 't') { len = 3; p++; }
        char conv = *p ? *p++ : '?';
        char tmp[512];
        int wrote = 0;
        if (conv == 's') {
            const char *s = (const char *)MEMP(va_next(va));
            if (!s) s = "(null)";
            spec[si] = 's'; spec[si + 1] = 0;
            if (conv == 's') wrote = snprintf(tmp, sizeof(tmp), spec, s);
        } else if (conv == 'c') {
            spec[si] = 'c'; spec[si + 1] = 0;
            wrote = snprintf(tmp, sizeof(tmp), spec, (int)va_next(va));
        } else if (conv == 'f' || conv == 'F' || conv == 'e' || conv == 'E' ||
                   conv == 'g' || conv == 'G' || conv == 'a' || conv == 'A') {
            double d = f64(0, 0);
            if (va->i + 1 < va->n) {
                uint32_t lo = va_next(va), hi = va_next(va); d = f64(lo, hi);
            } else {
                d = f64(va_next(va), va_next(va));
            }
            spec[si] = conv; spec[si + 1] = 0;
            wrote = snprintf(tmp, sizeof(tmp), spec, d);
        } else if (conv == 'd' || conv == 'i') {
            if (len == 2 || len == 3) {
                int64_t v = (int64_t)va_next64(va);
                spec[si] = 'l'; spec[si + 1] = 'l'; spec[si + 2] = conv; spec[si + 3] = 0;
                wrote = snprintf(tmp, sizeof(tmp), spec, (long long)v);
            } else {
                int v = (int)(int32_t)va_next(va);
                if (len == 1) { spec[si] = 'l'; spec[si + 1] = conv; spec[si + 2] = 0; wrote = snprintf(tmp, sizeof(tmp), spec, (long)v); }
                else { spec[si] = conv; spec[si + 1] = 0; wrote = snprintf(tmp, sizeof(tmp), spec, v); }
            }
        } else if (conv == 'u' || conv == 'o' || conv == 'x' || conv == 'X') {
            if (len == 2 || len == 3) {
                uint64_t v = va_next64(va);
                spec[si] = 'l'; spec[si + 1] = 'l'; spec[si + 2] = conv; spec[si + 3] = 0;
                wrote = snprintf(tmp, sizeof(tmp), spec, (unsigned long long)v);
            } else {
                unsigned v = va_next(va);
                if (len == 1) { spec[si] = 'l'; spec[si + 1] = conv; spec[si + 2] = 0; wrote = snprintf(tmp, sizeof(tmp), spec, (unsigned long)v); }
                else { spec[si] = conv; spec[si + 1] = 0; wrote = snprintf(tmp, sizeof(tmp), spec, v); }
            }
        } else if (conv == 'p') {
            uint32_t v = va_next(va);
            wrote = snprintf(tmp, sizeof(tmp), "0x%x", v);
        } else {
            tmp[0] = '?'; tmp[1] = 0; wrote = 1;
            va_next(va);
        }
        if (wrote > 0) {
            for (int k = 0; k < wrote; k++) { if (n + 1 < cap) dst[n] = tmp[k]; n++; }
        }
    }
    if (cap) dst[n < cap ? n : cap - 1] = 0;
    return n;
}

uint32_t jq_imp_snprintf(JQ_ARGS) {
    va32 va = {{r3, 0, 0, 0}, 0, 1, sp};
    return fmt_shadow((char *)MEMP(r0), r1, (const char *)MEMP(r2), &va);
}

uint32_t jq_imp_fprintf(JQ_ARGS) {
    FILE *f = (FILE *)jq_host(r0);
    va32 va = {{r2, r3, 0, 0}, 0, 2, sp};
    char buf[8192];
    uint32_t n = fmt_shadow(buf, sizeof(buf), (const char *)MEMP(r1), &va);
    if (f) fwrite(buf, 1, n, f);
    return n;
}

/* ---------------- 文件 ---------------- */
uint32_t jq_imp_fopen(JQ_ARGS) {
    FILE *f = fopen((const char *)MEMP(r0), (const char *)MEMP(r1));
    if (!f) return 0;
    return jq_handle(f);
}
uint32_t jq_imp_fclose(JQ_ARGS) { FILE *f = (FILE *)jq_host(r0); return f ? (uint32_t)fclose(f) : 0; }
uint32_t jq_imp_fflush(JQ_ARGS) { FILE *f = r0 ? (FILE *)jq_host(r0) : NULL; return (uint32_t)fflush(f); }
uint32_t jq_imp_fwrite(JQ_ARGS) {
    FILE *f = (FILE *)jq_host(r3);
    return f ? (uint32_t)fwrite(MEMP(r0), r1, r2, f) : 0;
}
uint32_t jq_imp_fseek(JQ_ARGS) {
    FILE *f = r0 ? (FILE *)jq_host(r0) : NULL;
    return (uint32_t)fseek(f, (long)(int32_t)r1, (int)r2);
}
uint32_t jq_imp_fgetpos(JQ_ARGS) {
    FILE *f = (FILE *)jq_host(r0);
    fpos_t pos;
    int rc = f ? fgetpos(f, &pos) : -1;
    /* 32 位 fpos_t = 8 字节（off_t） */
    uint64_t v = 0;
    memcpy(&v, &pos, sizeof(pos) < 8 ? sizeof(pos) : 8);
    MEM(r1) = lo32(v); MEM(r1 + 4) = hi32(v);
    return (uint32_t)rc;
}
uint32_t jq_imp_fsetpos(JQ_ARGS) {
    FILE *f = (FILE *)jq_host(r0);
    fpos_t pos;
    uint64_t v = u64(MEM(r1), MEM(r1 + 4));
    memcpy(&pos, &v, sizeof(pos) < 8 ? sizeof(pos) : 8);
    return f ? (uint32_t)fsetpos(f, &pos) : (uint32_t)-1;
}
uint32_t jq_imp_fputc(JQ_ARGS) { FILE *f = (FILE *)jq_host(r1); return f ? (uint32_t)fputc((int)r0, f) : 0; }
uint32_t jq_imp_putc(JQ_ARGS) { return jq_imp_fputc(JQ_PASS); }
uint32_t jq_imp_getc(JQ_ARGS) { FILE *f = (FILE *)jq_host(r0); return f ? (uint32_t)fgetc(f) : (uint32_t)-1; }
uint32_t jq_imp_ungetc(JQ_ARGS) { FILE *f = (FILE *)jq_host(r1); return f ? (uint32_t)ungetc((int)r0, f) : (uint32_t)-1; }
uint32_t jq_imp_fputs(JQ_ARGS) { FILE *f = (FILE *)jq_host(r1); return f ? (uint32_t)fputs((const char *)MEMP(r0), f) : 0; }
uint32_t jq_imp_puts(JQ_ARGS) {
    const char *s = (const char *)MEMP(r0);
    if (s) { strncpy(g_last_puts, s, 511); g_last_puts[511] = 0; }
    return (uint32_t)puts(s);
}
uint32_t jq_imp_setvbuf(JQ_ARGS) {
    FILE *f = (FILE *)jq_host(r0);
    return f ? (uint32_t)setvbuf(f, MEMP(r1), (int)r2, (size_t)r3) : 0;
}
uint32_t jq_imp_opendir(JQ_ARGS) { DIR *d = opendir((const char *)MEMP(r0)); return d ? jq_handle(d) : 0; }
uint32_t jq_imp_closedir(JQ_ARGS) { DIR *d = (DIR *)jq_host(r0); return d ? (uint32_t)closedir(d) : 0; }
uint32_t jq_imp_readdir(JQ_ARGS) {
    DIR *d = (DIR *)jq_host(r0);
    if (!d) return 0;
    struct dirent *de = readdir(d);
    if (!de) return 0;
    /* bionic/glibc 的 32 位 dirent 布局相同（d_name 偏移 19） */
    static uint8_t buf[288];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, de, 19 + 256 < sizeof(buf) ? 19 + 256 : sizeof(buf));
    return jq_push_bytes(buf, 19 + 256);
}

/* 32 位 bionic struct stat（80 字节）← 宿主 struct stat */
static void stat_to_shadow(const struct stat *st, uint32_t out) {
    memset(MEMP(out), 0, 80);
    uint64_t dev = (uint64_t)st->st_dev;
    MEM(out + 0) = lo32(dev); MEM(out + 4) = hi32(dev);
    MEM(out + 8) = (uint32_t)st->st_ino;
    MEM(out + 12) = (uint32_t)st->st_mode;
    MEM(out + 16) = (uint32_t)st->st_nlink;
    MEM(out + 20) = (uint32_t)st->st_uid;
    MEM(out + 24) = (uint32_t)st->st_gid;
    uint64_t rdev = (uint64_t)st->st_rdev;
    MEM(out + 28) = lo32(rdev); MEM(out + 32) = hi32(rdev);
    MEM(out + 36) = (uint32_t)st->st_size;
    MEM(out + 40) = (uint32_t)st->st_blksize;
    MEM(out + 44) = (uint32_t)st->st_blocks;
    MEM(out + 48) = (uint32_t)st->st_atime;
    MEM(out + 52) = (uint32_t)st->st_atim.tv_nsec;
    MEM(out + 56) = (uint32_t)st->st_mtime;
    MEM(out + 60) = (uint32_t)st->st_mtim.tv_nsec;
    MEM(out + 64) = (uint32_t)st->st_ctime;
    MEM(out + 68) = (uint32_t)st->st_ctim.tv_nsec;
}
uint32_t jq_imp_stat(JQ_ARGS) {
    struct stat st;
    int rc = stat((const char *)MEMP(r0), &st);
    if (rc == 0) stat_to_shadow(&st, r1);
    return (uint32_t)rc;
}

/* ---------------- 时间 ---------------- */
uint32_t jq_imp_clock_gettime(JQ_ARGS) {
    struct timespec ts;
    int rc = clock_gettime((clockid_t)r0, &ts);
    if (rc == 0 && r1) {
        MEM(r1) = (uint32_t)ts.tv_sec;
        MEM(r1 + 4) = (uint32_t)ts.tv_nsec;
    }
    return (uint32_t)rc;
}
uint32_t jq_imp_time(JQ_ARGS) {
    time_t t = time(NULL);
    if (r0) MEM(r0) = (uint32_t)t;
    return (uint32_t)t;
}
uint32_t jq_imp_srand48(JQ_ARGS) { srand48((long)(int32_t)r0); return 0; }

/* ---------------- pthread / sem（影子结构映射） ---------------- */
#define PMAX 64
static struct { uint32_t key; pthread_mutex_t m; int used; } g_pm[PMAX];
static pthread_mutex_t *pm_get(uint32_t addr) {
    for (int i = 0; i < PMAX; i++)
        if (g_pm[i].used && g_pm[i].key == addr) return &g_pm[i].m;
    for (int i = 0; i < PMAX; i++) {
        if (!g_pm[i].used) {
            g_pm[i].used = 1; g_pm[i].key = addr;
            pthread_mutex_init(&g_pm[i].m, NULL);
            return &g_pm[i].m;
        }
    }
    return NULL;
}
uint32_t jq_imp_pthread_mutex_lock(JQ_ARGS) {
    pthread_mutex_t *m = pm_get(r0);
    return m ? (uint32_t)pthread_mutex_lock(m) : 0;
}
uint32_t jq_imp_pthread_mutex_unlock(JQ_ARGS) {
    pthread_mutex_t *m = pm_get(r0);
    return m ? (uint32_t)pthread_mutex_unlock(m) : 0;
}
uint32_t jq_imp_pthread_mutex_destroy(JQ_ARGS) {
    for (int i = 0; i < PMAX; i++)
        if (g_pm[i].used && g_pm[i].key == r0) {
            pthread_mutex_destroy(&g_pm[i].m); g_pm[i].used = 0; break;
        }
    return 0;
}

static struct { uint32_t key; sem_t *sem; int used; } g_sem[PMAX];
uint32_t jq_imp_sem_init(JQ_ARGS) {
    for (int i = 0; i < PMAX; i++) {
        if (!g_sem[i].used) {
            sem_t *s = (sem_t *)malloc(sizeof(sem_t));
            if (!s) return (uint32_t)-1;
            sem_init(s, (int)r1, (unsigned)r2);
            g_sem[i].used = 1; g_sem[i].key = r0; g_sem[i].sem = s;
            return 0;
        }
    }
    return (uint32_t)-1;
}
uint32_t jq_imp_sem_post(JQ_ARGS) {
    for (int i = 0; i < PMAX; i++)
        if (g_sem[i].used && g_sem[i].key == r0) return (uint32_t)sem_post(g_sem[i].sem);
    return (uint32_t)-1;
}
uint32_t jq_imp_sem_destroy(JQ_ARGS) {
    for (int i = 0; i < PMAX; i++)
        if (g_sem[i].used && g_sem[i].key == r0) {
            sem_destroy(g_sem[i].sem); free(g_sem[i].sem);
            g_sem[i].used = 0; return 0;
        }
    return (uint32_t)-1;
}

/* ---------------- 网络 ---------------- */
uint32_t jq_imp_socket(JQ_ARGS) { return (uint32_t)socket((int)r0, (int)r1, (int)r2); }
uint32_t jq_imp_bind(JQ_ARGS) { return (uint32_t)bind((int)r0, (const struct sockaddr *)MEMP(r1), (socklen_t)r2); }
uint32_t jq_imp_connect(JQ_ARGS) { return (uint32_t)connect((int)r0, (const struct sockaddr *)MEMP(r1), (socklen_t)r2); }
uint32_t jq_imp_listen(JQ_ARGS) { return (uint32_t)listen((int)r0, (int)r1); }
uint32_t jq_imp_setsockopt(JQ_ARGS) {
    return (uint32_t)setsockopt((int)r0, (int)r1, (int)r2, (const void *)MEMP(r3), (socklen_t)MEM(sp));
}
uint32_t jq_imp_shutdown(JQ_ARGS) { return (uint32_t)shutdown((int)r0, (int)r1); }
uint32_t jq_imp_close(JQ_ARGS) { return (uint32_t)close((int)r0); }
uint32_t jq_imp_inet_addr(JQ_ARGS) { return (uint32_t)inet_addr((const char *)MEMP(r0)); }

/* ---------------- 其它 libc ---------------- */
uint32_t jq_imp_powf(JQ_ARGS) { return b32(powf(f32(r0), f32(r1))); }
uint32_t jq_imp_sqrtf(JQ_ARGS) { return b32(sqrtf(f32(r0))); }
uint32_t jq_imp_setjmp(JQ_ARGS) { return 0; }

/* ---------------- GL（float 实参按位还原） ---------------- */
uint32_t jq_imp_glBindTexture(JQ_ARGS) { glBindTexture(r0, r1); return 0; }
uint32_t jq_imp_glBlendFuncSeparate(JQ_ARGS) { glBlendFuncSeparate(r0, r1, r2, r3); return 0; }
uint32_t jq_imp_glClear(JQ_ARGS) { glClear(r0); return 0; }
uint32_t jq_imp_glClearColor(JQ_ARGS) { glClearColor(f32(r0), f32(r1), f32(r2), f32(r3)); return 0; }
uint32_t jq_imp_glDeleteProgram(JQ_ARGS) { glDeleteProgram(r0); return 0; }
uint32_t jq_imp_glDeleteShader(JQ_ARGS) { glDeleteShader(r0); return 0; }
uint32_t jq_imp_glDeleteTextures(JQ_ARGS) { glDeleteTextures((GLsizei)r0, (const GLuint *)MEMP(r1)); return 0; }
uint32_t jq_imp_glDisable(JQ_ARGS) { glDisable(r0); return 0; }
uint32_t jq_imp_glDrawArrays(JQ_ARGS) { glDrawArrays(r0, (GLint)r1, (GLsizei)r2); return 0; }
uint32_t jq_imp_glEnable(JQ_ARGS) { glEnable(r0); return 0; }
uint32_t jq_imp_glEnableVertexAttribArray(JQ_ARGS) { glEnableVertexAttribArray(r0); return 0; }
uint32_t jq_imp_glFlush(JQ_ARGS) { glFlush(); return 0; }
uint32_t jq_imp_glGetAttribLocation(JQ_ARGS) { return (uint32_t)glGetAttribLocation(r0, (const char *)MEMP(r1)); }
uint32_t jq_imp_glGetIntegerv(JQ_ARGS) {
    GLint v[16];
    int n = (r0 == 0x0D33 /*GL_MAX_TEXTURE_SIZE*/) ? 1 : 16;
    glGetIntegerv(r0, v);
    for (int i = 0; i < n; i++) MEM(r1 + i * 4) = (uint32_t)v[i];
    return 0;
}
uint32_t jq_imp_glGetString(JQ_ARGS) {
    const unsigned char *s = glGetString(r0);
    if (!s) return 0;
    static uint32_t cached_arg = 0xffffffffu, cached = 0;
    if (cached_arg != r0) { cached = jq_push_string((const char *)s); cached_arg = r0; }
    return cached;
}
uint32_t jq_imp_glGetUniformLocation(JQ_ARGS) { return (uint32_t)glGetUniformLocation(r0, (const char *)MEMP(r1)); }
uint32_t jq_imp_glPixelStorei(JQ_ARGS) { glPixelStorei(r0, (GLint)r1); return 0; }
uint32_t jq_imp_glReadPixels(JQ_ARGS) {
    glReadPixels((GLint)r0, (GLint)r1, (GLsizei)r2, (GLsizei)r3,
                 (GLenum)MEM(sp), (GLenum)MEM(sp + 4), MEMP(MEM(sp + 8)));
    return 0;
}
uint32_t jq_imp_glTexSubImage2D(JQ_ARGS) {
    glTexSubImage2D(r0, (GLint)r1, (GLint)r2, (GLint)r3,
                    (GLsizei)MEM(sp), (GLsizei)MEM(sp + 4),
                    (GLenum)MEM(sp + 8), (GLenum)MEM(sp + 12), MEMP(MEM(sp + 16)));
    return 0;
}
uint32_t jq_imp_glUniform1i(JQ_ARGS) { glUniform1i((GLint)r0, (GLint)r1); return 0; }
uint32_t jq_imp_glUniformMatrix4fv(JQ_ARGS) {
    glUniformMatrix4fv((GLint)r0, (GLsizei)r1, (GLboolean)r2, (const GLfloat *)MEMP(r3));
    return 0;
}
uint32_t jq_imp_glVertexAttribPointer(JQ_ARGS) {
    glVertexAttribPointer(r0, (GLint)r1, (GLenum)r2, (GLboolean)r3,
                          (GLsizei)MEM(sp), MEMP(MEM(sp + 4)));
    return 0;
}
uint32_t jq_imp_glViewport(JQ_ARGS) {
    glViewport((GLint)r0, (GLint)r1, (GLsizei)r2, (GLsizei)r3);
    return 0;
}

/* ================= 其余 112 个导入符号 ================= */
#include <wctype.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <netdb.h>
#include <locale.h>
#include <ctype.h>

/* --- 双精度软浮点 --- */
static uint32_t ret_d(double d) {
    uint64_t u; memcpy(&u, &d, 8);
    g_jq_ret1 = hi32(u); g_jq_ret2 = 0; g_jq_ret3 = 0;
    return lo32(u);
}
uint32_t jq_imp___aeabi_dadd(JQ_ARGS) { return ret_d(f64(r0, r1) + f64(r2, r3)); }
uint32_t jq_imp___aeabi_dsub(JQ_ARGS) { return ret_d(f64(r0, r1) - f64(r2, r3)); }
uint32_t jq_imp___aeabi_dmul(JQ_ARGS) { return ret_d(f64(r0, r1) * f64(r2, r3)); }
uint32_t jq_imp___aeabi_ddiv(JQ_ARGS) { return ret_d(f64(r0, r1) / f64(r2, r3)); }
uint32_t jq_imp___aeabi_dcmpeq(JQ_ARGS) { return f64(r0, r1) == f64(r2, r3); }
uint32_t jq_imp___aeabi_dcmpge(JQ_ARGS) { return f64(r0, r1) >= f64(r2, r3); }
uint32_t jq_imp___aeabi_dcmpgt(JQ_ARGS) { return f64(r0, r1) > f64(r2, r3); }
uint32_t jq_imp___aeabi_dcmplt(JQ_ARGS) { return f64(r0, r1) < f64(r2, r3); }
uint32_t jq_imp___aeabi_d2f(JQ_ARGS) { return b32((float)f64(r0, r1)); }
uint32_t jq_imp___aeabi_d2lz(JQ_ARGS) {
    int64_t v = (int64_t)f64(r0, r1);
    r1 = hi32((uint64_t)v); JQ_RET(lo32((uint64_t)v));
}
uint32_t jq_imp___aeabi_d2ulz(JQ_ARGS) {
    uint64_t v = (uint64_t)f64(r0, r1);
    r1 = hi32(v); JQ_RET(lo32(v));
}
uint32_t jq_imp___aeabi_i2d(JQ_ARGS) { return ret_d((double)(int32_t)r0); }
uint32_t jq_imp___aeabi_ui2d(JQ_ARGS) { return ret_d((double)r0); }
uint32_t jq_imp___aeabi_ui2f(JQ_ARGS) { return b32((float)r0); }
uint32_t jq_imp___aeabi_ul2d(JQ_ARGS) { return ret_d((double)u64(r0, r1)); }
uint32_t jq_imp___aeabi_uldivmod(JQ_ARGS) {
    uint64_t a = u64(r0, r1), b = u64(r2, r3);
    if (!b) { r1 = 0; r2 = 0; r3 = 0; JQ_RET(0); }
    uint64_t q = a / b, m = a % b;
    r1 = hi32(q); r2 = lo32(m); r3 = hi32(m);
    JQ_RET(lo32(q));
}
uint32_t jq_imp___aeabi_fcmpeq(JQ_ARGS) { return f32(r0) == f32(r1); }

/* --- Android log --- */
uint32_t jq_imp___android_log_print(JQ_ARGS) {
    va32 va = {{r3, 0, 0, 0}, 0, 1, sp};
    char buf[4096];
    fmt_shadow(buf, sizeof(buf), (const char *)MEMP(r2), &va);
    fprintf(stderr, "[log] %s\n", buf);
    return (uint32_t)strlen(buf);
}

uint32_t jq_imp___cxa_pure_virtual(JQ_ARGS) {
    jq_log("[jq] __cxa_pure_virtual\n");
    abort();
    return 0;
}
uint32_t jq_imp___aeabi_atexit(JQ_ARGS) { (void)r0; return 0; }
uint32_t jq_imp_longjmp(JQ_ARGS) {
    jq_log("[jq] longjmp(buf=0x%x, val=%d) — 不支持回跳\n", r0, (int)r1);
    return 0;
}

/* --- 内存分配 --- */
uint32_t jq_imp_malloc(JQ_ARGS) { return jq_malloc(r0); }
uint32_t jq_imp_calloc(JQ_ARGS) { return jq_calloc(r0, r1); }
uint32_t jq_imp_realloc(JQ_ARGS) { return jq_realloc(r0, r1); }

/* --- 数字/字符串 --- */
uint32_t jq_imp_atoi(JQ_ARGS) { return (uint32_t)atoi((const char *)MEMP(r0)); }
uint32_t jq_imp_strtod(JQ_ARGS) {
    char *end = 0;
    double d = strtod((const char *)MEMP(r0), &end);
    if (r1) MEM(r1) = end ? (uint32_t)((uint8_t *)end - g_jq_base) : 0;
    return ret_d(d);
}
uint32_t jq_imp_strtoul(JQ_ARGS) {
    char *end = 0;
    unsigned long v = strtoul((const char *)MEMP(r0), &end, (int)r2);
    if (r1) MEM(r1) = end ? (uint32_t)((uint8_t *)end - g_jq_base) : 0;
    return (uint32_t)v;
}
uint32_t jq_imp_strcat(JQ_ARGS) { strcat((char *)MEMP(r0), (const char *)MEMP(r1)); return r0; }
uint32_t jq_imp_strchr(JQ_ARGS) {
    char *p = strchr((const char *)MEMP(r0), (int)r1);
    return p ? (uint32_t)((uint8_t *)p - g_jq_base) : 0;
}
uint32_t jq_imp_strcpy(JQ_ARGS) { strcpy((char *)MEMP(r0), (const char *)MEMP(r1)); return r0; }
uint32_t jq_imp_strncpy(JQ_ARGS) { strncpy((char *)MEMP(r0), (const char *)MEMP(r1), r2); return r0; }
uint32_t jq_imp_strstr(JQ_ARGS) {
    char *p = strstr((const char *)MEMP(r0), (const char *)MEMP(r1));
    return p ? (uint32_t)((uint8_t *)p - g_jq_base) : 0;
}
uint32_t jq_imp_wcscmp(JQ_ARGS) { return (uint32_t)wcscmp((const wchar_t *)MEMP(r0), (const wchar_t *)MEMP(r1)); }
uint32_t jq_imp_wcslen(JQ_ARGS) { return (uint32_t)wcslen((const wchar_t *)MEMP(r0)); }
uint32_t jq_imp_wcsncpy(JQ_ARGS) { wcsncpy((wchar_t *)MEMP(r0), (const wchar_t *)MEMP(r1), r2); return r0; }
uint32_t jq_imp_wmemmove(JQ_ARGS) { wmemmove((wchar_t *)MEMP(r0), (const wchar_t *)MEMP(r1), r2); return r0; }
uint32_t jq_imp_iswalpha(JQ_ARGS) { return iswalpha((wint_t)r0) != 0; }
uint32_t jq_imp_iswcntrl(JQ_ARGS) { return iswcntrl((wint_t)r0) != 0; }
uint32_t jq_imp_iswdigit(JQ_ARGS) { return iswdigit((wint_t)r0) != 0; }
uint32_t jq_imp_iswlower(JQ_ARGS) { return iswlower((wint_t)r0) != 0; }
uint32_t jq_imp_iswprint(JQ_ARGS) { return iswprint((wint_t)r0) != 0; }
uint32_t jq_imp_iswpunct(JQ_ARGS) { return iswpunct((wint_t)r0) != 0; }
uint32_t jq_imp_iswspace(JQ_ARGS) { return iswspace((wint_t)r0) != 0; }
uint32_t jq_imp_iswupper(JQ_ARGS) { return iswupper((wint_t)r0) != 0; }
uint32_t jq_imp_iswxdigit(JQ_ARGS) { return iswxdigit((wint_t)r0) != 0; }
uint32_t jq_imp_towlower(JQ_ARGS) { return (uint32_t)towlower((wint_t)r0); }
uint32_t jq_imp_towupper(JQ_ARGS) { return (uint32_t)towupper((wint_t)r0); }
uint32_t jq_imp_qsort(JQ_ARGS) {
    uint32_t base = r0, n = r1, sz = r2, cmp = r3;
    uint8_t *tmp = (uint8_t *)malloc(sz ? sz : 1);
    for (uint32_t i = 1; i < n; i++) {
        memcpy(tmp, MEMP(base + i * sz), sz);
        uint32_t j = i;
        while (j > 0) {
            int c = (int)jq_call(cmp, base + (j - 1) * sz, base + i * sz, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 jq_shadow_sp());
            if (c <= 0) break;
            memcpy(MEMP(base + j * sz), MEMP(base + (j - 1) * sz), sz);
            j--;
        }
        memcpy(MEMP(base + j * sz), tmp, sz);
    }
    free(tmp);
    return 0;
}

/* --- sprintf / sscanf / vsnprintf --- */
uint32_t jq_imp_sprintf(JQ_ARGS) {
    va32 va = {{r2, r3, 0, 0}, 0, 2, sp};
    return fmt_shadow((char *)MEMP(r0), 0x7fffffffu, (const char *)MEMP(r1), &va);
}
uint32_t jq_imp_vsnprintf(JQ_ARGS) {
    va32 va = {{0, 0, 0, 0}, 0, 0, r3};
    return fmt_shadow((char *)MEMP(r0), r1, (const char *)MEMP(r2), &va);
}
uint32_t jq_imp_sscanf(JQ_ARGS) {
    va32 va = {{r2, r3, 0, 0}, 0, 2, sp};
    const char *s = (const char *)MEMP(r0);
    const char *f = (const char *)MEMP(r1);
    int cnt = 0;
    while (*f) {
        if (*f != '%') { if (isspace((unsigned char)*f)) { while (isspace((unsigned char)*s)) s++; f++; continue; } if (*s == *f) s++; f++; continue; }
        f++;
        if (*f == '%') { if (*s == '%') s++; f++; continue; }
        int is_long = 0;
        while (*f == 'l' || *f == 'h') { if (*f == 'l') is_long = 1; f++; }
        char conv = *f ? *f++ : 0;
        while (isspace((unsigned char)*s)) s++;
        if (conv == 'd' || conv == 'i' || conv == 'u' || conv == 'x' || conv == 'X') {
            char *end = 0;
            long v = (conv == 'x' || conv == 'X') ? strtol(s, &end, 16) : strtol(s, &end, 10);
            if (end == s) break;
            s = end;
            uint32_t dst = va_next(&va);
            if (is_long) { MEM(dst) = lo32((uint64_t)v); MEM(dst + 4) = hi32((uint64_t)v); }
            else MEM(dst) = (uint32_t)v;
            cnt++;
        } else if (conv == 'f' || conv == 'g' || conv == 'e') {
            char *end = 0;
            double d = strtod(s, &end);
            if (end == s) break;
            s = end;
            uint32_t dst = va_next(&va);
            if (is_long) { uint64_t u; memcpy(&u, &d, 8); MEM(dst) = lo32(u); MEM(dst + 4) = hi32(u); }
            else MEM(dst) = b32((float)d);
            cnt++;
        } else if (conv == 's') {
            uint32_t dst = va_next(&va);
            int k = 0;
            while (*s && !isspace((unsigned char)*s)) MEMB(dst + k++) = *s++;
            MEMB(dst + k) = 0;
            cnt++;
        } else if (conv == 'c') {
            uint32_t dst = va_next(&va);
            if (*s) { MEMB(dst) = *s++; cnt++; }
        } else break;
    }
    return (uint32_t)cnt;
}

/* --- 目录/文件/系统 --- */
uint32_t jq_imp_open(JQ_ARGS) { return (uint32_t)open((const char *)MEMP(r0), (int)r1, (mode_t)r2); }
uint32_t jq_imp_read(JQ_ARGS) { return (uint32_t)read((int)r0, MEMP(r1), (size_t)r2); }
uint32_t jq_imp_write(JQ_ARGS) { return (uint32_t)write((int)r0, MEMP(r1), (size_t)r2); }
uint32_t jq_imp_lseek(JQ_ARGS) { return (uint32_t)lseek((int)r0, (off_t)(int32_t)r1, (int)r2); }
uint32_t jq_imp_fcntl(JQ_ARGS) { return (uint32_t)fcntl((int)r0, (int)r1, r2); }
uint32_t jq_imp_mkdir(JQ_ARGS) { return (uint32_t)mkdir((const char *)MEMP(r0), (mode_t)r1); }
uint32_t jq_imp_remove(JQ_ARGS) { return (uint32_t)remove((const char *)MEMP(r0)); }
uint32_t jq_imp_rename(JQ_ARGS) { return (uint32_t)rename((const char *)MEMP(r0), (const char *)MEMP(r1)); }
uint32_t jq_imp_sysconf(JQ_ARGS) { return (uint32_t)sysconf((int)r0); }
uint32_t jq_imp_usleep(JQ_ARGS) { return (uint32_t)usleep((useconds_t)r0); }
uint32_t jq_imp_fstat(JQ_ARGS) {
    struct stat st;
    int rc = fstat((int)r0, &st);
    if (rc == 0) stat_to_shadow(&st, r1);
    return (uint32_t)rc;
}
uint32_t jq_imp_ftell(JQ_ARGS) { FILE *f = (FILE *)jq_host(r0); return f ? (uint32_t)ftell(f) : (uint32_t)-1; }
uint32_t jq_imp_feof(JQ_ARGS) { FILE *f = (FILE *)jq_host(r0); return f ? (uint32_t)feof(f) : 1; }
uint32_t jq_imp_ferror(JQ_ARGS) { FILE *f = (FILE *)jq_host(r0); return f ? (uint32_t)ferror(f) : 1; }
uint32_t jq_imp_rewind(JQ_ARGS) { FILE *f = (FILE *)jq_host(r0); if (f) rewind(f); return 0; }
uint32_t jq_imp_fread(JQ_ARGS) {
    FILE *f = (FILE *)jq_host(r3);
    return f ? (uint32_t)fread(MEMP(r0), r1, r2, f) : 0;
}
uint32_t jq_imp_mmap(JQ_ARGS) {
    /* 影子地址空间无法映射宿主地址：分配影子缓冲并按 fd 预读 */
    uint32_t len = r1 ? r1 : 0x1000;
    uint32_t p = jq_malloc(len);
    if (!p) return (uint32_t)-1;
    memset(MEMP(p), 0, len);
    int fd = (int)r4;
    if (!(r3 & 0x20 /* MAP_ANONYMOUS */) && fd >= 0) {
        off_t save = lseek(fd, 0, SEEK_CUR);
        if ((int32_t)r5 >= 0) lseek(fd, (off_t)(int32_t)r5, SEEK_SET);
        ssize_t got = read(fd, MEMP(p), len);
        if (got < 0) got = 0;
        lseek(fd, save, SEEK_SET);
    }
    return p;
}
uint32_t jq_imp_munmap(JQ_ARGS) { jq_free(r0); return 0; }

/* --- 时间 --- */
static uint32_t g_tm_shadow = 0x162000u;
uint32_t jq_imp_localtime(JQ_ARGS) {
    time_t t = r0 ? (time_t)MEM(r0) : time(NULL);
    struct tm *tm = localtime(&t);
    if (!tm) return 0;
    memset(MEMP(g_tm_shadow), 0, 44);
    int f[9] = {tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon,
                tm->tm_year, tm->tm_wday, tm->tm_yday, tm->tm_isdst};
    for (int i = 0; i < 9; i++) MEM(g_tm_shadow + i * 4) = (uint32_t)f[i];
    MEM(g_tm_shadow + 36) = (uint32_t)tm->tm_gmtoff;
    MEM(g_tm_shadow + 40) = 0;
    return g_tm_shadow;
}
uint32_t jq_imp_mktime(JQ_ARGS) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    if (r0) {
        tm.tm_sec = (int)MEM(r0); tm.tm_min = (int)MEM(r0 + 4); tm.tm_hour = (int)MEM(r0 + 8);
        tm.tm_mday = (int)MEM(r0 + 12); tm.tm_mon = (int)MEM(r0 + 16); tm.tm_year = (int)MEM(r0 + 20);
        tm.tm_wday = (int)MEM(r0 + 24); tm.tm_yday = (int)MEM(r0 + 28); tm.tm_isdst = (int)MEM(r0 + 32);
        tm.tm_gmtoff = (long)(int32_t)MEM(r0 + 36);
    }
    return (uint32_t)mktime(&tm);
}
uint32_t jq_imp_strftime(JQ_ARGS) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    if (r3) {
        tm.tm_sec = (int)MEM(r3); tm.tm_min = (int)MEM(r3 + 4); tm.tm_hour = (int)MEM(r3 + 8);
        tm.tm_mday = (int)MEM(r3 + 12); tm.tm_mon = (int)MEM(r3 + 16); tm.tm_year = (int)MEM(r3 + 20);
        tm.tm_wday = (int)MEM(r3 + 24); tm.tm_yday = (int)MEM(r3 + 28); tm.tm_isdst = (int)MEM(r3 + 32);
    }
    return (uint32_t)strftime((char *)MEMP(r0), r1, (const char *)MEMP(r2), &tm);
}

/* --- 数学 --- */
uint32_t jq_imp_sinf(JQ_ARGS) { return b32(sinf(f32(r0))); }
uint32_t jq_imp_cosf(JQ_ARGS) { return b32(cosf(f32(r0))); }
uint32_t jq_imp_tanf(JQ_ARGS) { return b32(tanf(f32(r0))); }
uint32_t jq_imp_acosf(JQ_ARGS) { return b32(acosf(f32(r0))); }
uint32_t jq_imp_expf(JQ_ARGS) { return b32(expf(f32(r0))); }

/* --- 环境/区域 --- */
uint32_t jq_imp_getenv(JQ_ARGS) {
    static struct { char name[64]; uint32_t sh; } cache[16];
    static int n = 0;
    const char *nm = (const char *)MEMP(r0);
    for (int i = 0; i < n; i++) if (!strcmp(cache[i].name, nm)) return cache[i].sh;
    const char *v = getenv(nm);
    if (!v) return 0;
    uint32_t sh = jq_push_string(v);
    if (n < 16) { strncpy(cache[n].name, nm, 63); cache[n].sh = sh; n++; }
    return sh;
}
uint32_t jq_imp_setlocale(JQ_ARGS) {
    const char *loc = setlocale((int)r0, r1 ? (const char *)MEMP(r1) : NULL);
    return loc ? jq_push_string(loc) : 0;
}

/* --- 套接字 --- */
uint32_t jq_imp_accept(JQ_ARGS) {
    socklen_t sl = (socklen_t)MEM(r2);
    int fd = accept((int)r0, (struct sockaddr *)MEMP(r1), &sl);
    if (r2) MEM(r2) = sl;
    return (uint32_t)fd;
}
uint32_t jq_imp_recv(JQ_ARGS) { return (uint32_t)recv((int)r0, MEMP(r1), (size_t)r2, (int)r3); }
uint32_t jq_imp_send(JQ_ARGS) { return (uint32_t)send((int)r0, MEMP(r1), (size_t)r2, (int)r3); }
uint32_t jq_imp_recvfrom(JQ_ARGS) {
    socklen_t sl = (socklen_t)MEM(sp + 4);
    ssize_t n = recvfrom((int)r0, MEMP(r1), (size_t)r2, (int)r3,
                         (struct sockaddr *)MEMP(MEM(sp)), &sl);
    MEM(sp + 4) = sl;
    return (uint32_t)n;
}
uint32_t jq_imp_sendto(JQ_ARGS) {
    return (uint32_t)sendto((int)r0, MEMP(r1), (size_t)r2, (int)r3,
                            (const struct sockaddr *)MEMP(MEM(sp)), (socklen_t)MEM(sp + 4));
}
uint32_t jq_imp_select(JQ_ARGS) {
    struct timeval tv, *ptv = NULL;
    if (r4) {
        tv.tv_sec = (time_t)(int32_t)MEM(r4);
        tv.tv_usec = (suseconds_t)(int32_t)MEM(r4 + 4);
        ptv = &tv;
    }
    int rc = select((int)r0, (fd_set *)MEMP(r1), (fd_set *)MEMP(r2), (fd_set *)MEMP(r3), ptv);
    if (r4 && ptv) { MEM(r4) = (uint32_t)tv.tv_sec; MEM(r4 + 4) = (uint32_t)tv.tv_usec; }
    return (uint32_t)rc;
}
uint32_t jq_imp_inet_ntoa(JQ_ARGS) {
    struct in_addr a; a.s_addr = r0;
    char *s = inet_ntoa(a);
    return s ? jq_push_string(s) : 0;
}
uint32_t jq_imp_getaddrinfo(JQ_ARGS) { (void)r0; (void)r1; (void)r2; (void)r3; return (uint32_t)-4; }
uint32_t jq_imp_freeaddrinfo(JQ_ARGS) { (void)r0; return 0; }

/* --- pthread --- */
typedef struct { uint32_t fn, arg; } jq_thr_arg;
static void *jq_thr_tramp(void *p) {
    jq_thr_arg *t = (jq_thr_arg *)p;
    uint32_t stk = jq_malloc(0x40000);
    uint32_t sp = stk + 0x3f000;
    jq_call(t->fn, t->arg, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sp);
    jq_free(stk);
    free(t);
    return NULL;
}
#define MAXTHR 64
static pthread_t g_thr[MAXTHR];
static int g_thr_n;
uint32_t jq_imp_pthread_create(JQ_ARGS) {
    if (g_thr_n >= MAXTHR) return (uint32_t)-1;
    jq_thr_arg *a = (jq_thr_arg *)malloc(sizeof(*a));
    a->fn = r2; a->arg = r3;
    pthread_attr_t attr, *pa = NULL;
    if (r1) {
        /* 影子 attr 是不透明块，忽略自定义栈大小，统一用宿主默认 */
        pthread_attr_init(&attr);
        pa = &attr;
    }
    int rc = pthread_create(&g_thr[g_thr_n], pa, jq_thr_tramp, a);
    if (pa) pthread_attr_destroy(pa);
    if (rc == 0) {
        uint32_t idx = (uint32_t)g_thr_n++;
        if (r0) MEM(r0) = idx;
        return 0;
    }
    free(a);
    return (uint32_t)rc;
}
uint32_t jq_imp_pthread_join(JQ_ARGS) {
    uint32_t idx = r0;
    if (idx >= (uint32_t)g_thr_n) return (uint32_t)-1;
    void *ret = NULL;
    int rc = pthread_join(g_thr[idx], &ret);
    if (r1) MEM(r1) = (uint32_t)(uintptr_t)ret;
    return (uint32_t)rc;
}
uint32_t jq_imp_pthread_attr_init(JQ_ARGS) { (void)r0; return 0; }
uint32_t jq_imp_pthread_attr_destroy(JQ_ARGS) { (void)r0; return 0; }
uint32_t jq_imp_pthread_attr_setdetachstate(JQ_ARGS) { (void)r0; (void)r1; return 0; }
uint32_t jq_imp_pthread_attr_setstacksize(JQ_ARGS) { (void)r0; (void)r1; return 0; }
#define MAXKEY 32
static pthread_key_t g_key[MAXKEY];
static int g_key_n;
uint32_t jq_imp_pthread_key_create(JQ_ARGS) {
    if (g_key_n >= MAXKEY) return (uint32_t)-1;
    int rc = pthread_key_create(&g_key[g_key_n], NULL);
    if (rc == 0) { if (r0) MEM(r0) = (uint32_t)g_key_n; g_key_n++; }
    return (uint32_t)rc;
}
uint32_t jq_imp_pthread_setspecific(JQ_ARGS) {
    if (r0 >= (uint32_t)g_key_n) return (uint32_t)-1;
    return (uint32_t)pthread_setspecific(g_key[r0], (void *)(uintptr_t)r1);
}
uint32_t jq_imp_pthread_getspecific(JQ_ARGS) {
    if (r0 >= (uint32_t)g_key_n) return 0;
    return (uint32_t)(uintptr_t)pthread_getspecific(g_key[r0]);
}
uint32_t jq_imp_pthread_mutex_init(JQ_ARGS) { pm_get(r0); return 0; }

/* --- 信号量 --- */
uint32_t jq_imp_sem_wait(JQ_ARGS) {
    for (int i = 0; i < PMAX; i++)
        if (g_sem[i].used && g_sem[i].key == r0) return (uint32_t)sem_wait(g_sem[i].sem);
    return (uint32_t)-1;
}

/* --- GL 补充 --- */
extern void glAttachShader(GLuint, GLuint);
extern void glCompileShader(GLuint);
extern GLuint glCreateProgram(void);
extern GLuint glCreateShader(GLenum);
extern void glGenTextures(GLsizei, GLuint *);
extern void glGetProgramiv(GLuint, GLenum, GLint *);
extern void glGetShaderiv(GLuint, GLenum, GLint *);
extern void glLinkProgram(GLuint);
extern void glShaderSource(GLuint, GLsizei, const char *const *, const GLint *);
extern void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
extern void glTexParameterf(GLenum, GLenum, GLfloat);
extern void glUseProgram(GLuint);

uint32_t jq_imp_glAttachShader(JQ_ARGS) { glAttachShader(r0, r1); return 0; }
uint32_t jq_imp_glCompileShader(JQ_ARGS) { glCompileShader(r0); return 0; }
uint32_t jq_imp_glCreateProgram(JQ_ARGS) { return glCreateProgram(); }
uint32_t jq_imp_glCreateShader(JQ_ARGS) { return glCreateShader(r0); }
uint32_t jq_imp_glGenTextures(JQ_ARGS) {
    GLuint ids[64];
    int n = (int)r0;
    if (n > 64) n = 64;
    glGenTextures(n, ids);
    for (int i = 0; i < n; i++) MEM(r1 + i * 4) = ids[i];
    return 0;
}
uint32_t jq_imp_glGetProgramiv(JQ_ARGS) {
    GLint v = 0;
    glGetProgramiv(r0, r1, &v);
    MEM(r2) = (uint32_t)v;
    return 0;
}
uint32_t jq_imp_glGetShaderiv(JQ_ARGS) {
    GLint v = 0;
    glGetShaderiv(r0, r1, &v);
    MEM(r2) = (uint32_t)v;
    return 0;
}
uint32_t jq_imp_glLinkProgram(JQ_ARGS) { glLinkProgram(r0); return 0; }
uint32_t jq_imp_glShaderSource(JQ_ARGS) {
    int n = (int)r1;
    if (n > 32) n = 32;
    const char *strs[32];
    GLint lens[32];
    for (int i = 0; i < n; i++) {
        strs[i] = (const char *)MEMP(MEM(r2 + i * 4));
        lens[i] = (GLint)MEM(r3 + i * 4);
    }
    glShaderSource(r0, n, strs, n ? lens : NULL);
    return 0;
}
uint32_t jq_imp_glTexImage2D(JQ_ARGS) {
    glTexImage2D(r0, (GLint)r1, (GLint)r2, (GLsizei)r3,
                 (GLsizei)MEM(sp), (GLint)MEM(sp + 4),
                 (GLenum)MEM(sp + 8), (GLenum)MEM(sp + 12), MEMP(MEM(sp + 16)));
    return 0;
}
uint32_t jq_imp_glTexParameterf(JQ_ARGS) { glTexParameterf(r0, r1, f32(r2)); return 0; }
uint32_t jq_imp_glUseProgram(JQ_ARGS) { glUseProgram(r0); return 0; }

uint32_t jq_imp_lrand48(JQ_ARGS) { (void)r0; return (uint32_t)(lrand48() & 0x7fffffff); }
