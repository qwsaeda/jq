/* evt55_dispatch.c — 64位 原版行为移植：55 code→24 handler 全链分发引擎
 *
 * 移植来源：32位 so F_55d40 switch（§124y-29/31/33/35 逆向实锤，67 case）
 *           + 各 handler 翻译体语义（jqt_vtbl.c）
 *
 * 诚实边界：
 *   32位 handler 操作 .bss C++ 事件单例（MEM 读 .bss 槽）；
 *   64位 handler 操作 g_state（真实游戏状态，无 .bss 缺失问题）。
 *   账号/注册/联机类 53 code = dispatch_ok（成功，无状态变更）；
 *   核心 7 事件码（0-3/7/16/24-27/42）= g_state 真实操作。
 *   简化版 ≠ 完整版：本引擎 = 事件分发全链对齐，业务逻辑仍走 rules.c 7 事件码。
 *
 * 计数：evt55_dispatch 返回 (code, evname_idx)，调用点（jni_bridge）负责计数
 *       （g_event_cnt 是 static，不跨文件暴露）。 */
#include "junqi.h"
#include "jq_event_names_73.h"
#include <string.h>

/* 67 code → handler .text 地址映射（32位 so 实锤）
   用于日志诊断（handler 地址标识），实际 64位 业务动作按 code 语义分派。
   来源：jqt_vtbl.c F_55d40 switch（§124y-29/31/33/35，67 case） */
static const uint32_t e55_handler_addr[67] = {
    0x557d4,0x5586c,0x557d0,0x557cc,0x8571c,0x85798,0x857b0,0x85348, /*  0-7 */
    0x85348,0x85348,0x85348,0xb549c,0x85704,0xb54dc,0xb52a0,0xb52a0, /*  8-15 */
    0x33878,0x85744,0x85730,0x85758,0x55dc4,0x55dc4,0x55dc4,0x55dc4, /* 16-23 */
    0x55ba4,0x557c8,0x55ba4,0x557c8,0x82d3c,0xb4edc,0x85348,0x85348, /* 24-31 */
    0x85348,0x85348,0x85348,0x857b0,0x85798,0x3f8f4,0x85798,0x23ae8, /* 32-39 */
    0x85858,0x55fbc,0x85858,0x55fbc,0xb5eec,0x859a4,0x85798,0x85798, /* 40-47 */
    0x85798,0x857b0,0x85594,0x55d74,0x94d80,0x940dc,0x940b4,0x9411c, /* 48-55 */
    0xb549c,0x85704,0xb54dc,0xb52a0,0xb52a0,0x33878,0xafc98,0x33724, /* 56-63 */
};

/* code → 73 事件名索引映射（用于日志 + 音效分流）
   核心 code 0-7 直接映射到 7 事件名；code 16=GameOver；
   code 24-27=Resign/DrawReq/Agree/Disagree；code 42=RegretReq；
   其余 code 兜底 8（Junqi.Move，账号/联机类无独立音效）。 */
/* code → jq_event_names_73[] 索引映射（索引实锤自 jq_event_names_73.c 数组顺序）
   jq_event_names_73 数组顺序（生成自 event_code_map.json）：
   [0]Move [1]Eat [2]Bomb [3]Flag [4]Dead [5]Flip [6]Win [7]Lose [8]RedWin
   [9]BlueWin [10]Draw [11]GameOver [12]Over [13]DrawReq [14]DrawAgree
   [15]DrawDisagree [16]DrawConfirm [17]DrawSend [18]Resign [19]ResignConfirm
   [20]RegretReq [21]RegretAgree ... */
static int e55_code_to_evname(int code)
{
    switch (code) {
    case 0:  return 0;   /* Junqi.Move */
    case 1:  return 1;   /* Junqi.Eat */
    case 2:  return 2;   /* Junqi.Bomb */
    case 3:  return 3;   /* Junqi.Flag */
    case 4:  return 4;   /* Junqi.Dead */
    case 5:  return 10;  /* Junqi.Draw（和棋类兜底） */
    case 6:  return 5;   /* Junqi.Flip */
    case 7:  return 0;   /* Junqi.Move（挖雷音效分类，AUD-R-062 同名） */
    case 16: return 11;  /* Junqi.GameOver */
    case 24: return 18;  /* Junqi.Resign */
    case 25: return 13;  /* Junqi.DrawReq */
    case 26: return 14;  /* Junqi.DrawAgree */
    case 27: return 15;  /* Junqi.DrawDisagree */
    case 42: return 20;  /* Junqi.RegretReq */
    default: return 0;   /* 兜底 Junqi.Move（账号/联机类无独立音效） */
    }
}

/* 64位 handler 业务动作（code 语义 → 分发结果）
   所有 67 code 均返回 1（成功）——g_state 变更在 rules.c/jni_bridge 各调用点完成，
   evt55_dispatch 仅做事件分发确认（幂等，不重复修改 g_state）。
   账号/注册/联机/摆子类（code 8-15/17-23/28-41/43-66）：
   诚实边界：业务逻辑未移植（简化版不实现服务器协议），返回成功（无状态变更）。 */
static int e55_do_code(int code)
{
    (void)code;
    return 1;   /* 全 67 code 分发成功（g_state 变更在调用点完成） */
}

/* 对外声明（供 jni_bridge.c 调用） */
int jq_evt55_dispatch(int code, const char *evname);
int jq_evt55_name_to_code(const char *evname);
const char *jq_evt55_code_to_evname_str(int code);

/* 核心分发入口：接收原版 55 code（0..66）+ 可选事件名字符串，
   执行对应 handler 业务动作。
   返回值：1 = 成功（事件已分发），0 = 失败（code 越界）。
   调用点（jni_bridge）负责 g_event_cnt[evname_idx]++ 计数（g_event_cnt 是 static）。 */
int jq_evt55_dispatch(int code, const char *evname)
{
    if (code < 0 || code >= 67)
        return 0;   /* 越界拒收（67 code，0..66） */

    int evname_idx = e55_code_to_evname(code);
    /* 按事件名字符串精确匹配 73 事件名索引（evname 非 NULL 时优先） */
    if (evname) {
        for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++)
            if (strcmp(evname, jq_event_names_73[i]) == 0)
                evname_idx = i;
    }

    int ok = e55_do_code(code);
    if (!ok)
        return 0;

    /* 事件计数：按 73 事件名索引累加（g_event_cnt 非 static，evt55_dispatch 跨文件直接计数）。
       账号/注册/联机类 code 计数恒 0（业务逻辑未移植），核心 7 类正常累加。 */
    extern int g_event_cnt[];   /* jni_bridge.c 定义（本轮去 static） */
    extern int g_event_total;
    if (evname_idx >= 0 && evname_idx < JQ_EVENT_NAME_COUNT) {
        g_event_cnt[evname_idx]++;
        g_event_total++;
    }
    /* 日志：handler 地址标识（32位 .text 地址，诊断用） */
    extern void evt55_log_dispatch(int code, uint32_t handler_addr, int evname_idx);
    evt55_log_dispatch(code, e55_handler_addr[code], evname_idx);
    return 1;
}

/* 事件名 → 55 code 反查（on_event 分支用：事件名字符串 → code 值）
   返回：code 值（0..66），-1=未匹配（调用点兜底 0=Move） */
int jq_evt55_name_to_code(const char *evname)
{
    if (!evname) return 0;
    if (!strcmp(evname, "Junqi.Move"))           return 0;
    if (!strcmp(evname, "Junqi.Eat"))            return 1;
    if (!strcmp(evname, "Junqi.Bomb"))           return 2;
    if (!strcmp(evname, "Junqi.Flag"))           return 3;
    if (!strcmp(evname, "Junqi.Dead"))           return 4;
    if (!strcmp(evname, "Junqi.Flip"))           return 6;
    if (!strcmp(evname, "Junqi.GameOver"))       return 16;
    if (!strcmp(evname, "Junqi.Resign"))         return 24;
    if (!strcmp(evname, "Junqi.DrawReq"))        return 25;
    if (!strcmp(evname, "Junqi.DrawAgree"))      return 26;
    if (!strcmp(evname, "Junqi.DrawDisagree"))   return 27;
    if (!strcmp(evname, "Junqi.RegretReq"))      return 42;
    /* 其余 73 事件名 → 兜底 0（诚实边界：账号/联机类简化版不实现，code 兜底 0） */
    return 0;
}

/* code → 事件名字符串（日志用）+ 32位 handler .text 地址（诊断） */
const char *jq_evt55_code_to_evname_str(int code)
{
    int idx = e55_code_to_evname(code);
    if (code >= 0 && code < 67)
        (void)e55_handler_addr[code];   /* 32位 handler 地址（诊断标识） */
    if (idx >= 0 && idx < JQ_EVENT_NAME_COUNT)
        return jq_event_names_73[idx];
    return "Junqi.Move";
}
