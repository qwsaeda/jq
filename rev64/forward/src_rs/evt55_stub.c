/* ★ R27-Q67/Q68：evt55 全链桌面链接桩——g_event_cnt/g_event_total（jni_bridge.c 不入桌面链接集，
   计数语义由桩提供，桌面 evverify 可验证 73 名逐名分发/计数链路；真实计数在 jni_bridge.c
   设备端 g_event_cnt[JQ_EVENT_NAME_COUNT]，本桩仅桌面回归用）。 */
#include "junqi.h"
#include "jq_event_names_73.h"   /* JQ_EVENT_NAME_COUNT */
int g_event_cnt[JQ_EVENT_NAME_COUNT];
int g_event_total = 0;
