#ifndef RS_AUDIO_H
#define RS_AUDIO_H
#include <stddef.h>

/* 原版 0xb7500 音频容器 + SPAK WAV（data/junqi/*.wav 12 + data/ui/*.wav 2） */
#define JQ_AUDIO_MAX 16

typedef struct {
    const char *res_dir;
    int   ready;
    int   n;
    unsigned char *pcm[JQ_AUDIO_MAX];
    size_t pcm_len[JQ_AUDIO_MAX];
    int    rate[JQ_AUDIO_MAX];
    int    bits[JQ_AUDIO_MAX];
    int    chans[JQ_AUDIO_MAX];
} jq_audio;

int  jq_audio_init(jq_audio *au, const char *res_dir);
/* 播放事件对应音效（AudioTrack 桩，出声需宿主接）。返回音效索引或 -1。 */
int  jq_audio_play(jq_audio *au, const char *event);
void jq_audio_free(jq_audio *au);
/* WAV RIFF PCM 解码（8/16-bit）。返回 malloc 的 PCM，填 rate/bits。 */
void *jq_wav_load(const char *path, size_t *out_len, int *out_rate, int *out_bits);

#endif
