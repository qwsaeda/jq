/* jq_event_names_73.h — 原版 73 事件名常量表（32位 so .rodata 0x120xxx 区提取）
 * 生成自 rev64/dist/event_code_map.json（frida-dump/rodata_strings.txt 实锤）。
 * 64位 src_rs 简化版用 7 事件码（EV_MOVE..EV_FLIP）= 层1核心；本表=原版全量契约，
 * 音效/事件名分流按本表索引（原版 .rodata 73 名 1:1 对齐）。
 * 诚实边界：账号/注册/联机类事件名未逆向服务器协议（仅常量表对齐，业务逻辑未移植）。
 * 红线：64位简化版不包装成完整版——本表=契约对齐，非行为等价。 */
#ifndef JQ_EVENT_NAMES_73_H
#define JQ_EVENT_NAMES_73_H
#define JQ_EVENT_NAME_COUNT 73
#define JQ_EVENT_CODE_MAX 11   /* 12 事件码（0-11，code 12-16 预留） */
extern const char *const jq_event_names_73[JQ_EVENT_NAME_COUNT];
/* 事件码 → 名称索引数组（code 0-11，每项为该码下名称在 jq_event_names_73 的下标） */
extern const int jq_event_code_names[12][16]; /* [code][idx]，-1 结束 */
#endif
