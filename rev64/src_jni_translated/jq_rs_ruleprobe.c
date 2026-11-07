/*
 * jq_rs_ruleprobe.c — 32 位翻译体侧规则行为探针（与 64 位 src_rs/rule_probe.c 同源）
 *
 * ★ §124y R-138 分叉实证专项：让**同一批棋盘位置**在 32 位翻译体规则引擎与
 *   64 位重写规则引擎上跑同一组查询，输出规范化 "RULE-PROBE <case> <result>"
 *   行 → 机器 diff → 一致即**行为实证**，不一致即**实证分歧**。
 *
 * 本文件是 64 位 rule_probe.c 的**薄包装**（唯一差异）：
 *   ① 头文件 jq_rs_rules.h（32 位）而非 junqi.h（64 位）
 *   ② PROBE_LOG 走 __android_log_print（tag=junqi）→ logcat 可取
 *   ③ 提供 jq_rule_probe_run() 供属性钩子调用
 * 规则探针本体（摆棋 + 提问）**不重复实现**，直接包含共享源，保证两端
 * 探针用例**逐字节同源**（否则用例漂移会让对比失效）。
 */
#include <android/log.h>
#include "jq_rs_rules.h"

/* PROBE_LOG 由共享源使用（rule_probe.c 内 #ifndef PROBE_LOG 定义 stdout 版） */
#define PROBE_LOG(...) __android_log_print(4, "junqi", __VA_ARGS__)

/* 共享探针源（32 位编译）：仅换头文件 + 日志宏，用例本体与 64 位逐字节同源 */
#include "../src_rs/rule_probe_shared.inc"

/* 供属性钩子调用（jni_aligned.c / 走子桥 tick） */
void jq_rule_probe_run(void);
