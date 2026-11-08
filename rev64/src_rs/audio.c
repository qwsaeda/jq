/*
 * audio.c — 音效层（⑥ 逆源码：原版 0xb7500 音频容器 + SPAK WAV + Junqi.* 事件映射）
 * 依据原版（capstone + SPAK 解包）：
 *   0xb7500：音频容器 = C++ vector，元素 0x10 字节，容量 0x80<<10=1280（jq_audio_vec 已逆）。
 *   音效资源：SPAK data/junqi/*.wav（18 个，RIFF PCM，8/16-bit mono/stereo 11025/22050/8000Hz）
 *     + data/ui/{close,pop}.wav。
 *   事件→音效映射（原版 0x120f14-0x120f50 的 Junqi.* 事件，sound.xml 清单）：
 *     走子 6：move/eat/bomb/flag/dead/flip（音效文件同名）
 *     胜负 6：win/lose/draw/start/over/select
 *     UI 2：  close/pop
 * 诚实边界：原版 AudioTrack 经 JNI（0x8598 分发，AudioManager/AudioTrack 类名
 *   0x1275d2/0x12766a）播放 PCM；本重写用 miniaudio 思路的自解码 + 宿主播放桩
 *   （无 Android 音频设备环境，桌面/NDK 编译通过，真机出声需 AudioTrack 接）。
 *   解码（WAV PCM）已实做，播放端为桩（诚实：不假装"出声"）。
 */
#include "audio.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* 事件→音效文件名（原版 sound.xml 清单 + Junqi.* 事件名，L71 音效分类） */
/* 音效文件名对齐 SPAK 解包产物（data/junqi/*.wav + data/ui/*.wav，
   spak_res 里实际名 data_junqi_*.wav / data_ui_*.wav）。 */
static const struct { const char *ev; const char *wav; } EV2WAV[] = {
    {"move",   "data_junqi_move.wav"},
    {"eat",    "data_junqi_eat.wav"},
    {"bomb",   "data_junqi_bomb.wav"},
    {"flag",   "data_junqi_flag.wav"},
    {"dead",   "data_junqi_dead.wav"},
    {"flip",   "data_junqi_flip.wav"},
    {"win",    "data_junqi_win.wav"},
    {"lose",   "data_junqi_lose.wav"},
    {"draw",   "data_junqi_draw.wav"},
    {"start",  "data_junqi_start.wav"},
    {"over",   "data_junqi_over.wav"},
    {"select", "data_junqi_select.wav"},
    {"close",  "data_ui_close.wav"},
    {"pop",    "data_ui_pop.wav"},
};
#define N_EV2WAV (sizeof(EV2WAV)/sizeof(EV2WAV[0]))

int jq_audio_init(jq_audio *au, const char *res_dir)
{
    /* ★ 对抗审计（§2026-11-11）：重复 init 会 memset 掉旧 pcm 指针 → 泄漏
       （每次设置变更/重进界面若再 init 一次就漏 9 个 WAV buffer）。先释放再清零。 */
    if (!au) return -1;
    jq_audio_free(au);
    memset(au, 0, sizeof(*au));
    au->res_dir = res_dir ? res_dir : "";
    /* 加载全部音效（SPAK 解包 WAV → PCM） */
    for (size_t i = 0; i < N_EV2WAV; i++) {
        char path[512];
        snprintf(path, sizeof path, "%s/%s", au->res_dir, EV2WAV[i].wav);
        au->pcm[i] = jq_wav_load(path, &au->pcm_len[i], &au->rate[i], &au->bits[i]);
        au->chans[i] = au->pcm[i] ? 1 : 0;   /* 简化单声道（原版有立体声 8000Hz） */
    }
    au->n = N_EV2WAV;
    au->ready = 1;
    return 0;
}

/* 播放（宿主 AudioTrack 桩）：桌面/NDK 编译通过，出声需接 AudioTrack。
   返回该事件对应的音效索引（-1 无）。 */
int jq_audio_play(jq_audio *au, const char *event)
{
    if (!au || !au->ready) return -1;
    for (size_t i = 0; i < N_EV2WAV; i++) {
        if (strcmp(EV2WAV[i].ev, event) == 0) {
            /* 原版 0xb7500 容器 + AudioTrack.write(pcm) 播放；
               本重写：PCM 已解码就位，播放端为桩（诚实边界）。 */
            (void)au->pcm[i]; (void)au->pcm_len[i];
            return (int)i;
        }
    }
    return -1;
}

void jq_audio_free(jq_audio *au)
{
    if (!au) return;
    for (size_t i = 0; i < N_EV2WAV; i++) free(au->pcm[i]);
    memset(au, 0, sizeof(*au));
}

/* ---- WAV PCM 解码（RIFF 头解析，8/16-bit PCM）----
   返回 malloc 的 PCM 数据（8-bit=原始字节，16-bit=小端 int16 数组），
   填 rate/bits。失败返回 NULL。 */
void *jq_wav_load(const char *path, size_t *out_len, int *out_rate, int *out_bits)
{
    FILE *f = fopen(path, "rb");
    if (!f) { if (out_len) *out_len = 0; return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc(sz > 0 ? sz : 1);
    size_t rd = fread(buf, 1, sz > 0 ? sz : 1, f);
    fclose(f);
    if (rd < 44 || memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) {
        free(buf); if (out_len) *out_len = 0; return NULL;
    }
    /* 找 fmt + data 块 */
    unsigned char *p = buf + 12;
    int fmt_pos = -1, data_pos = -1;
    unsigned int data_len = 0;
    while (p + 8 <= buf + rd) {
        char tag[5];
        memcpy(tag, p, 4); tag[4] = 0;
        unsigned int csize = (unsigned)p[4] | ((unsigned)p[5] << 8) |
                             ((unsigned)p[6] << 16) | ((unsigned)p[7] << 24);
        if (strcmp(tag, "fmt ") == 0) {
            fmt_pos = (int)(p - buf);
            /* p 指向 'fmt ' 标签。payload 从 p+8（跳 tag4+size4）起：
               format=p[8..9] ch=p[10..11] rate=p[12..13] brate=p[14..15]
               align=p[16..17] bits=p[18..19]（实测 move.wav bits=8 在 buf34=p22，
               即 p[18..19] 对应 buf30..31=brate；位深实际在 p[22..23]） */
            if (out_rate) *out_rate = (int)(p[12] | (p[13] << 8));
            if (out_bits) *out_bits = (int)(p[22] | (p[23] << 8));
            (void)fmt_pos;
        } else if (strcmp(tag, "data") == 0) {
            data_pos = (int)(p - buf);
            data_len = csize;
        }
        p += 8 + csize;
        if ((csize & 1)) p++;   /* RIFF 块 2 字节对齐 */
    }
    if (data_pos < 0 || data_len == 0) { free(buf); if (out_len) *out_len = 0; return NULL; }
    /* 拷出 data 段 */
    void *pcm = malloc(data_len);
    if (!pcm) { free(buf); if (out_len) *out_len = 0; return NULL; }
    memcpy(pcm, buf + data_pos + 8, data_len);
    if (out_len) *out_len = data_len;
    free(buf);
    return pcm;
}
