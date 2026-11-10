/* evt55_sfx.c — 原版 73 事件名 → 音效 WAV 名映射（64 位实现，替代旧死函数 sfx_for_event_name）
 *
 * 对齐原版 .rodata 音效分类：核心 7 类走独立 WAV（move/eat/bomb/flag/dead/flip），
 * 胜负/和棋/投降走 win/lose/draw，账号/联机/摆子类无独立 WAV → 兜底 move（诚实边界，★ A6-01 注释纠正）。
 * 诚实边界：73 事件名中账号/注册/联机类 44 名音效=UI 通用音（原版无独立业务 WAV），不造新音效。
 */
#include "junqi.h"
#include "jq_event_names_73.h"
#include <string.h>

/* 事件名字符串 → 音效 WAV 名（sfx_wav 表键名，jni_bridge sfx_wav() 识别）
   核心 7 类走独立 WAV；胜负/和棋/投降走 win/lose/draw；其余兜底 move（诚实边界）。 */
const char *jq_evt55_sfx_for_name(const char *evname)
{
    if (!evname) return "move";
    /* 核心 7 类 */
    if (!strcmp(evname, "Junqi.Move")   ) return "move";
    if (!strcmp(evname, "Junqi.Eat"))    return "eat";
    if (!strcmp(evname, "Junqi.Bomb"))   return "bomb";
    if (!strcmp(evname, "Junqi.Flag"))   return "flag";
    if (!strcmp(evname, "Junqi.Dead"))   return "dead";
    if (!strcmp(evname, "Junqi.Flip"))   return "flip";
    /* 胜负/终局 */
    if (!strcmp(evname, "Junqi.Win") || !strcmp(evname, "Junqi.RedWin") || !strcmp(evname, "Junqi.BlueWin")) return "win";
    if (!strcmp(evname, "Junqi.Lose"))   return "lose";
    if (!strcmp(evname, "Junqi.Draw") || !strcmp(evname, "Junqi.GameOver") || !strcmp(evname, "Junqi.Over")) return "draw";
    /* 投降确认 = 本机败 */
    if (!strcmp(evname, "Junqi.Resign") || !strcmp(evname, "Junqi.ResignConfirm")) return "lose";
    /* 其余 66 名（悔棋/账号/注册/联机/摆子类）→ 无独立 WAV，兜底 move（诚实边界） */
    return "move";
}
