/*
 * rule_probe.c — ★ 32/64 位规则引擎**行为探针**（共享源，双端编译）
 *
 * 目的（对抗审计 §124y R-138 分叉实证专项）：
 *   分叉矩阵 102 条目前"已标记 101 + 已实证 0"——标记只是**读代码判读**，
 *   不是行为实证。本探针让**同一批棋盘位置**在两端规则引擎上跑同一组查询，
 *   输出规范化行（"RULE-PROBE <case> <result>"）→ 机器 diff → 行为一致即
 *   **已实证**，不一致即**实证分歧**（分叉坐实）。
 *
 * 为什么能共享一份源（关键前提，已核实）：
 *   · 两端 jq_cell 布局一致（rank/side/revealed/flags，4 字节紧凑）
 *   · 两端 jq_state 关键字段同名同义（cells/turn/stage/winner/
 *     moves_since_eat/total_moves/mines_cleared）
 *   · 两端规则 API 同名同签名（jq_state_init / jq_can_move / jq_try_move /
 *     jq_side_stalemated / jq_flip / cell_index）
 *   → 探针源码不含任何规则逻辑，只**摆棋 + 提问**，规则判定全在各自引擎内。
 *
 * ★ 防复发（关键）：探针必须**只依赖规则 API**，不得引用任何一方独有的辅助
 *   函数（如 ui_rule_cfg_get / rank_of），否则"共享"名不副实、双端编译失败
 *   或行为不可比。摆棋用**原始 rank 字节**（与端无关的存储约定）。
 *
 * 编译：
 *   · 64 位：make ruleprobe（桌面）→ dist/jq_ruleprobe
 *   · 32 位：随翻译体 so 编译（jq_rs_ruleprobe.c 包装 + jq.rule_probe 属性钩子）
 *
 * 用法（两端一致）：
 *   jq_rule_probe_run() → 逐用例打印 "RULE-PROBE <id> <k=v,...>"
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

#ifndef PROBE_LOG
/* 默认走 stdout（桌面）；设备端由包装文件重定义为 __android_log_print */
#define PROBE_LOG(...) do { printf(__VA_ARGS__); putchar('\n'); } while (0)
#endif

/* ★ 共享探针本体（32/64 位逐字节同源，见 rule_probe_shared.inc 头注释） */
#include "rule_probe_shared.inc"
