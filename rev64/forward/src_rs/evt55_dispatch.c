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
 * 计数：★ R7-B-18（注释纠错）——g_event_cnt/g_event_total 为**跨文件**计数状态
 *       （jni_bridge.c 非 static 定义，本文件 extern 累加，R6-B-09 未知名 idx=-1
 *       拒计数后语义闭合）；旧"调用点负责计数/g_event_cnt 是 static"口径已废弃。 */
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
    case 24: return 18;  /* Junqi.Resign（R5-B-05：ResignConfirm 由调用方传 evname 精确覆盖） */
    case 25: return 13;  /* Junqi.DrawReq */
    case 26: return 14;  /* Junqi.DrawAgree */
    case 27: return 15;  /* Junqi.DrawDisagree */
    case 42: return 20;  /* Junqi.RegretReq（悔棋子事件由 evname 精确覆盖，R5-B-06） */
    /* ★ Round 1 对抗审计 R5-B-02（P1 真缺陷）：业务代表 55code 34/36/38/44/46
       无 case → default 返回 0 → 名称→code→名称 往返身份丢失为 Junqi.Move
       （未映射 code 全记 Move = 假绿同族）。修：按桶返代表名（73 名表 idx 实测：
       LOGIN→30 LoginFail / REG→37 RegExist / NET→50 JoinGame / LAY→65 LayError /
       MISC→71 Select，见 jq_event_names_73.c 行内 [idx] 注释），
       调用方传 evname 时精确覆盖（jq_evt55_dispatch 的 strcmp 路径不变）。 */
    case 34: return 30;  /* Junqi.LoginFail（LOGIN 桶代表，idx 30） */
    case 36: return 37;  /* Junqi.RegExist（REG 桶代表，idx 37） */
    case 38: return 50;  /* Junqi.JoinGame（NET 桶代表，idx 50） */
    case 44: return 65;  /* Junqi.LayError（LAY 桶代表，idx 65） */
    case 46: return 71;  /* Junqi.Select（MISC 桶代表，idx 71） */
    default: return -1;  /* ★ R6-B-10（P1 未知名隔离）：未映射 55code 返哨兵 -1
       （旧 default 0 使未映射 code 全归 Junqi.Move = 假绿同族静默归因）；
       调用方 jq_evt55_dispatch 对 evname 命中 73 表时精确覆盖不受影响 */
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
/* ★ R8-B-01（P0）：事件单一入口 by_name——name→查表精确 55code→dispatch(code,name)。
   计数口径 0 变化证明：jq_evt55_dispatch 以 evname 精确命中 73 表选 idx
   （:114-120），code 仅影响越界拒收与 handler 诊断（e55_handler_addr），
   故 73 合法名 by_name 与旧硬编码 code+名 同 idx 同增 1；_checked 未命中
   （NULL/非 73 名）→ 不调 dispatch 不计数 返 0（R7-B-05 拒计数语义）。 */
int jq_evt55_dispatch_by_name(const char *evname)
{
    int code = -1;
    if (!jq_evt55_name_to_55code_checked(evname, &code)) return 0;
    return jq_evt55_dispatch(code, evname);
}

/* ★ R8-B-07（P0）：Σ 计数不变量单点 API——Σ g_event_cnt[i] == g_event_total。
   仅当"计数被接受"（idx 有效）时两者同增 1（:130-133 同点双写），
   拒计数路径（idx=-1 不累加）天然满足。O(73) 纯读无锁（诊断用途，
   与 g_event_cnt/g_event_total 现有 int 单字读写契约一致）。 */
int jq_event_count_invariant_ok(void)
{
    extern int g_event_cnt[];
    extern int g_event_total;
    long long sum = 0;
    for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++) sum += g_event_cnt[i];
    return (sum == (long long)g_event_total) ? 1 : 0;
}

/* ★ R8-B-08/09（P1）：count-only API（"无独立 55code"手工旁路/7 码直写收敛，
   R7-B-08/09 定案）——仅计数不走 e55_do_code/handler 诊断；未知名拒计数。
   双计审计（R7-B 回执 13 路径 0 双计）：5 手工旁路 + 8 直写均无前置
   dispatch，收敛后每事件恰好 +1。 */
int jq_evt55_count_by_name(const char *evname)
{
    extern int g_event_cnt[];
    extern int g_event_total;
    if (!evname) return 0;
    for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++) {
        if (strcmp(evname, jq_event_names_73[i]) == 0) {
            g_event_cnt[i]++;
            g_event_total++;
            return 1;
        }
    }
    return 0;
}

int jq_evt55_count_7code(int ev)
{
    /* ★ R8-B-09 名级映射定案（实锤）：7 码域 0..6 → 73 名表索引（与 jni_bridge.c
       ev7_to_name_idx 同口径——EV_MINE(5) 名级 = Junqi.Move（同 Junqi.Move 走子
       语义，name idx 0），EV_FLIP(6) = Junqi.Flip（idx 5））：
         ev 0..4 → name idx 0..4（Move/Eat/Bomb/Flag/Dead）
         ev 5    → name idx 0（EV_MINE = Move）
         ev 6    → name idx 5（EV_FLIP = Flip）
       经 jq_evt55_count_by_name 名级命中 73 表 → g_event_cnt[idx]++/g_event_total++。 */
    static const char *const ev7_name[7] = {
        "Junqi.Move", "Junqi.Eat", "Junqi.Bomb", "Junqi.Flag",
        "Junqi.Dead", "Junqi.Move", "Junqi.Flip"
    };
    return jq_evt55_count_by_name((ev >= 0 && ev < 7) ? ev7_name[ev] : NULL);
}

int jq_evt55_name_to_code(const char *evname);
const char *jq_evt55_code_to_evname_str(int code);

/* 核心分发入口：接收原版 55 code（0..66）+ 可选事件名字符串，
   执行对应 handler 业务动作。
   ★ Round 1 对抗审计 P2（§104 口径诚实化）：**本入口不执行原版 handler 业务
     动作**——e55_do_code() 为恒 1 空实现（原版业务由调用点 rules.c 完成），函数
     实际职责 = ①code 越界拒收 ②code→73 事件名索引解析（evname 字符串优先）
     ③事件计数累加 ④handler 地址诊断标识。旧注释"执行对应 handler 业务动作"
     与实现不符（假对齐口径），本轮按实现改写注释（不改行为）。
   返回值：1 = 成功（事件已分发），0 = 失败（code 越界）。
   计数在 dispatch 内跨文件累加（g_event_cnt/g_event_total 非 static，jni_bridge.c
   定义；★ R7-B-18 注释纠错：旧"g_event_cnt 是 static 不跨文件"口径废弃）。 */
int jq_evt55_dispatch(int code, const char *evname)
{
    if (code < 0 || code >= 67)
        return 0;   /* 越界拒收（67 code，0..66） */

    int evname_idx = e55_code_to_evname(code);
    /* 按事件名字符串精确匹配 73 事件名索引（evname 非 NULL 时优先）
       ★ R6-B-09（P1 未知名隔离）：evname 非空但**未命中 73 表**时旧实现保留
       code 默认 idx（哨兵 -1 或桶代表）照常计数 → 未知名事件误归因（假绿同族）。
       修：未命中 → evname_idx = -1（拒计数，与 code 未映射哨兵同语义），
       LOGV 可诊断；合法调用点全部传 73 表内名（on_event/AI/走子路径实证），
       行为 0 退化（负向：传 "Junqi.NotExist" 不再计入 g_event_cnt）。 */
    if (evname) {
        int hit = -1;
        for (int i = 0; i < JQ_EVENT_NAME_COUNT; i++)
            if (strcmp(evname, jq_event_names_73[i]) == 0)
                hit = i;
        evname_idx = hit;   /* 命中=精确 idx；未命中=-1 拒计数 */
    }

    int ok = e55_do_code(code);
    if (!ok)
        return 0;

    /* 事件计数：按 73 事件名索引累加（g_event_cnt/g_event_total 为跨文件计数状态，
       jni_bridge.c 定义，此处 extern）。★ R7-B-17（注释定案，行为 0 变化）：旧注释
       "账号/注册/联机类 code 计数恒 0（业务逻辑未移植）"与实现矛盾——实现为**分发即
       计数**（idx 有效即累加）。定案口径：64 位无账号/注册/联机业务触发路径，
       若经 dispatch 传入合法 73 名仍按名级索引**防御计数**（与计数不变量
       Σ g_event_cnt==g_event_total 一致），不因"未移植业务"而抑制计数；
       核心 7 类与一切 73 表命中名按名级累加。 */
    extern int g_event_cnt[];   /* jni_bridge.c 定义（本轮去 static） */
    extern int g_event_total;
    if (evname_idx >= 0 && evname_idx < JQ_EVENT_NAME_COUNT) {
        g_event_cnt[evname_idx]++;
        g_event_total++;
    }
    /* handler 地址（32位 .text 标识）：e55_handler_addr[code] 用于诊断日志
       （evt55_log_dispatch 未实现，此处标记使用消除 unused 告警） */
    (void)e55_handler_addr[code];
    return 1;
}

/* 事件名 → 55 code 反查（原版行为移植深水区：76 事件名全链，evt55_business.c）
   返回：55 code 值（0..66），未匹配兜底 0（Junqi.Move）。
   诚实边界：账号/注册/联机/摆子类 55code 区 = 分发成功无 g_state 变更（业务逻辑未移植）。 */
int jq_evt55_name_to_code(const char *evname)
{
    return jq_evt55_name_to_55code(evname);   /* evt55_business.c 76 事件名全链 */
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
