/* audio_sl.h — 音效播放接口（OpenSL ES） */
#ifndef AUDIO_SL_H
#define AUDIO_SL_H

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 OpenSL ES（OnCreate 时调用） */
void junqi_audio_init(void);
/* 播放事件音效（move/flip/flag/bomb/eat/dead/win/lose/draw/start/over/select） */
void junqi_audio_play(const char *event);
/* 释放 */
void junqi_audio_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif /* AUDIO_SL_H */
