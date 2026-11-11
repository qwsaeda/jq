/*
 * rule_probe_main.c — 规则行为探针的桌面入口（64 位侧）
 * 打印 RULE-PROBE 行 → 与 32 位设备端 logcat 的同名行做 diff → 行为对比实证。
 */
#include <stdio.h>
#include "junqi.h"

void jq_rule_probe_run(void);

int main(void)
{
    jq_rule_probe_run();
    return 0;
}
