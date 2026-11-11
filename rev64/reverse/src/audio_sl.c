/*
 * audio_sl.c — 音效播放（OpenSL ES，API 9+，minSdk 19 兼容）
 * 播放解包 WAV（8/16bit PCM，单/双声道）。走子/吃子/翻棋/胜负事件触发。
 * WAV 资源经 spak_get("data/junqi/<evt>.wav") 获取，按需解析。
 */
#include "audio_sl.h"
#include "spak_load.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "junqi64"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static SLObjectItf g_engine_obj = NULL;
static SLEngineItf g_engine = NULL;
static SLObjectItf g_mix_obj = NULL;
static SLObjectItf g_player_obj = NULL;
static SLPlayItf g_play = NULL;
static SLAndroidSimpleBufferQueueItf g_bufq = NULL;

static int16_t *g_pcm = NULL;      /* 当前播放缓冲（保持存活） */
static int g_ready = 0;

/* ---- WAV 解析（RIFF/PCM，转 16bit 单声道）---- */
typedef struct { int16_t *pcm; int frames; SLuint32 rate; } wav_info;

static int wav_parse(const unsigned char *data, size_t len, wav_info *out) {
    if (!data || len < 44 || memcmp(data, "RIFF", 4) != 0 || memcmp(data+8, "WAVE", 4) != 0)
        return -1;
    size_t pos = 12;
    unsigned short ch = 0; unsigned rate = 0; unsigned short bits = 0;
    const unsigned char *pcm = NULL; size_t pcm_len = 0;
    while (pos + 8 <= len) {
        unsigned clen = ((unsigned)data[pos+4] | ((unsigned)data[pos+5]<<8) |
                         ((unsigned)data[pos+6]<<16) | ((unsigned)data[pos+7]<<24));
        if (memcmp(data+pos, "fmt ", 4) == 0 && pos + 8 + 16 <= len) {
            const unsigned char *f = data + pos + 8;
            ch = (unsigned short)(f[2] | (f[3]<<8));
            rate = (unsigned)(f[4] | (f[5]<<8) | (f[6]<<16) | ((unsigned)f[7]<<24));
            bits = (unsigned short)(f[14] | (f[15]<<8));
        } else if (memcmp(data+pos, "data", 4) == 0 && pos + 8 <= len) {
            pcm = data + pos + 8;
            pcm_len = clen;
            break;
        }
        pos += 8 + clen + (clen & 1);
    }
    if (!pcm || !ch || !rate || (bits != 8 && bits != 16)) return -1;
    int frames = (int)(pcm_len / (ch * bits / 8));
    int16_t *mono = (int16_t *)malloc((size_t)frames * 2);
    if (!mono) return -1;
    for (int i = 0; i < frames; i++) {
        int acc = 0;
        for (unsigned c = 0; c < ch; c++) {
            int v;
            if (bits == 8) v = (pcm[i*ch+c] - 128) << 8;
            else v = (int16_t)(pcm[(i*ch+c)*2] | (pcm[(i*ch+c)*2+1]<<8));
            acc += v;
        }
        mono[i] = (int16_t)(acc / (int)ch);
    }
    out->pcm = mono; out->frames = frames; out->rate = rate;
    return 0;
}

/* ---- OpenSL ES 初始化 ---- */
void junqi_audio_init(void) {
    if (g_ready) return;
    if (slCreateEngine(&g_engine_obj, 0, NULL, 0, NULL, NULL) != SL_RESULT_SUCCESS) {
        LOGE("audio: slCreateEngine fail"); return;
    }
    (*g_engine_obj)->Realize(g_engine_obj, SL_BOOLEAN_FALSE);
    (*g_engine_obj)->GetInterface(g_engine_obj, SL_IID_ENGINE, &g_engine);
    if (!g_engine) { LOGE("audio: no engine"); return; }
    (*g_engine)->CreateOutputMix(g_engine, &g_mix_obj, 0, NULL, NULL);
    (*g_mix_obj)->Realize(g_mix_obj, SL_BOOLEAN_FALSE);
    g_ready = 1;
    LOGI("audio: OpenSL ES 初始化完成");
}

/* ---- 播放一段 16bit 单声道 PCM ---- */
static void audio_play_pcm(int16_t *pcm, int frames, SLuint32 rate) {
    if (!g_ready || !pcm || frames <= 0) return;
    if (g_player_obj) { (*g_player_obj)->Destroy(g_player_obj); g_player_obj = NULL; g_play=NULL; g_bufq=NULL; }
    if (g_pcm) { free(g_pcm); g_pcm = NULL; }
    g_pcm = pcm;   /* 保持存活直到下个播放 */

    SLDataLocator_AndroidSimpleBufferQueue loc = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
    SLDataFormat_PCM fmt = {
        SL_DATAFORMAT_PCM, 1, rate * 1000,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource src = {&loc, &fmt};
    SLDataLocator_OutputMix outloc = {SL_DATALOCATOR_OUTPUTMIX, g_mix_obj};
    SLDataSink sink = {&outloc, NULL};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    if ((*g_engine)->CreateAudioPlayer(g_engine, &g_player_obj, &src, &sink, 1, ids, req) != SL_RESULT_SUCCESS) {
        LOGE("audio: CreateAudioPlayer fail"); free(pcm); g_pcm=NULL; return;
    }
    (*g_player_obj)->Realize(g_player_obj, SL_BOOLEAN_FALSE);
    (*g_player_obj)->GetInterface(g_player_obj, SL_IID_PLAY, &g_play);
    (*g_player_obj)->GetInterface(g_player_obj, SL_IID_BUFFERQUEUE, &g_bufq);
    if (!g_play || !g_bufq) { LOGE("audio: no play/bufq"); return; }
    (*g_bufq)->Enqueue(g_bufq, g_pcm, (SLuint32)frames * 2);
    (*g_play)->SetPlayState(g_play, SL_PLAYSTATE_PLAYING);
}

/* ---- 按事件名播放（data/junqi/<event>.wav）---- */
void junqi_audio_play(const char *event) {
    if (!g_ready || !event) return;
    char path[128];
    snprintf(path, sizeof(path), "data/junqi/%s.wav", event);
    uint32_t len = 0;
    const unsigned char *wav = spak_get(path, &len);
    if (!wav || len < 44) { LOGI("audio: 无 %s", path); return; }
    wav_info wi;
    if (wav_parse(wav, len, &wi) != 0) { LOGI("audio: 解析失败 %s", path); return; }
    audio_play_pcm(wi.pcm, wi.frames, wi.rate);
    LOGI("audio: 播放 %s (%d frames)", event, wi.frames);
}

/* ---- 释放 ---- */
void junqi_audio_shutdown(void) {
    if (g_player_obj) { (*g_player_obj)->Destroy(g_player_obj); g_player_obj=NULL; }
    if (g_mix_obj) { (*g_mix_obj)->Destroy(g_mix_obj); g_mix_obj=NULL; }
    if (g_engine_obj) { (*g_engine_obj)->Destroy(g_engine_obj); g_engine_obj=NULL; }
    if (g_pcm) { free(g_pcm); g_pcm=NULL; }
    g_ready = 0;
}
