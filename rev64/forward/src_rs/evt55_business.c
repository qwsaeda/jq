/* evt55_business.c — 64位 原版行为移植（深水区）：76 事件名 → 12 业务码 → 55 code
 * 移植来源：32位 so F_55d40 switch（§124y-29/31/33/35 实锤）+ event_code_map.json
 * 诚实边界：核心 7 类 + 胜负/和棋/投降/悔棋 = 实锤 handler；账号/注册/联机/摆子类
 *   = 分发成功无状态变更（业务逻辑未移植），简化版 ≠ 完整版。 */
#include "junqi.h"
#include "jq_event_names_73.h"
#include <string.h>

/* 12 业务码 → 55 code（representative，诚实边界：账号/联机类 55code 区无独立 g_state 变更） */
/* 12 业务码 = 0-11 索引（event_code_map.json 12 大类），55code 值由 e55_biz_to_55code 映射 */
#define E55_BIZ_MOVE     0   /* 走子/吃子（Move/Eat/Dead）→ 55code 0 */
#define E55_BIZ_BOMB     1   /* 炸弹/旗（Bomb/Flag）→ 55code 2 */
#define E55_BIZ_FLIP     2   /* 翻棋（Flip）→ 55code 6 */
#define E55_BIZ_GAMEOVER 3   /* 胜负/终局（Win/Lose/RedWin/BlueWin/Draw/GameOver/Over）→ 55code 16 */
#define E55_BIZ_DRAW     4   /* 和棋（DrawReq/Agree/Disagree/Confirm/Send）→ 55code 25 */
#define E55_BIZ_RESIGN   5   /* 投降（Resign/ResignConfirm）→ 55code 24 */
#define E55_BIZ_REGRET   6   /* 悔棋（Regret*）→ 55code 42 */
#define E55_BIZ_LOGIN    7   /* 登录/账号（Input、Login、Email、User 族）→ 55code 34（诚实边界） */
#define E55_BIZ_REG      8   /* 注册（Reg、Name、Pwd、New 族）→ 55code 36（同上） */
#define E55_BIZ_NET      9   /* 联机（Join/Ready/Peer/Leave/Reconn）→ 55code 38（同上） */
#define E55_BIZ_LAY     10   /* 摆子/布局（Lay/High/Mid/Zha/Lei/Qi）→ 55code 44（同上） */
#define E55_BIZ_MISC    11   /* 系统/杂项（Select/GameType）→ 55code 46（同上） */

/* 事件名字符串 → 12 业务码（73 事件名实锤） */
static int e55_name_to_biz(const char *evname)
{
    if (!evname) return E55_BIZ_MOVE;
    /* 核心 7 类 */
    if (!strcmp(evname, "Junqi.Move")   || !strcmp(evname, "Junqi.Dead")) return E55_BIZ_MOVE;
    if (!strcmp(evname, "Junqi.Eat"))                     return E55_BIZ_MOVE;
    if (!strcmp(evname, "Junqi.Bomb") || !strcmp(evname, "Junqi.Flag")) return E55_BIZ_BOMB;
    if (!strcmp(evname, "Junqi.Flip"))                    return E55_BIZ_FLIP;
    /* 胜负/终局 */
    if (!strcmp(evname, "Junqi.GameOver") || !strcmp(evname, "Junqi.Over")
     || !strcmp(evname, "Junqi.Win") || !strcmp(evname, "Junqi.Lose")
     || !strcmp(evname, "Junqi.RedWin") || !strcmp(evname, "Junqi.BlueWin"))
        return E55_BIZ_GAMEOVER;
    /* 和棋 */
    if (!strcmp(evname, "Junqi.Draw") || !strcmp(evname, "Junqi.DrawReq")
     || !strcmp(evname, "Junqi.DrawAgree") || !strcmp(evname, "Junqi.DrawDisagree")
     || !strcmp(evname, "Junqi.DrawConfirm") || !strcmp(evname, "Junqi.DrawSend"))
        return E55_BIZ_DRAW;
    /* 投降 */
    if (!strcmp(evname, "Junqi.Resign") || !strcmp(evname, "Junqi.ResignConfirm"))
        return E55_BIZ_RESIGN;
    /* 悔棋 */
    if (!strcmp(evname, "Junqi.RegretReq") || !strcmp(evname, "Junqi.RegretAgree")
     || !strcmp(evname, "Junqi.RegretDisagree") || !strcmp(evname, "Junqi.RegretConfirm")
     || !strcmp(evname, "Junqi.RegretSend") || !strcmp(evname, "Junqi.RegretFail"))
        return E55_BIZ_REGRET;
    /* 登录/账号（Input族/Login族/Email族/User族）→ 诚实边界（分发成功，无 g_state 变更） */
    if (!strncmp(evname, "Junqi.Input", 11) || !strncmp(evname, "Junqi.Login", 11)
     || !strncmp(evname, "Junqi.Email", 11) || !strncmp(evname, "Junqi.User", 10))
        return E55_BIZ_LOGIN;
    /* 注册（Name族/Pwd族/New族/Ori族 + RegExist/RegFail/RegOk 精确匹配）→ 同上 */
    if (!strncmp(evname, "Junqi.Name", 10) || !strncmp(evname, "Junqi.Pwd", 9)
     || !strncmp(evname, "Junqi.New", 9)   || !strncmp(evname, "Junqi.Ori", 9)
     || !strcmp(evname, "Junqi.RegExist") || !strcmp(evname, "Junqi.RegFail")
     || !strcmp(evname, "Junqi.RegOk"))
        return E55_BIZ_REG;
    if (!strncmp(evname, "Junqi.Join", 10) || !strncmp(evname, "Junqi.Ready", 11)
     || !strncmp(evname, "Junqi.Start", 11) || !strncmp(evname, "Junqi.Peer", 10)
     || !strncmp(evname, "Junqi.Leave", 11) || !strncmp(evname, "Junqi.Player", 12)
     || !strncmp(evname, "Junqi.Reconn", 12) || !strncmp(evname, "Junqi.Connect", 13)
     || !strncmp(evname, "Junqi.Exit", 10))
        return E55_BIZ_NET;
    if (!strncmp(evname, "Junqi.Lay", 9) || !strncmp(evname, "Junqi.High", 10)
     || !strncmp(evname, "Junqi.Mid", 9) || !strncmp(evname, "Junqi.Zha", 9)
     || !strncmp(evname, "Junqi.Lei", 9) || !strncmp(evname, "Junqi.Qi", 8))
        return E55_BIZ_LAY;
    if (!strncmp(evname, "Junqi.Select", 12) || !strncmp(evname, "Junqi.GameType", 14))
        return E55_BIZ_MISC;
    return E55_BIZ_MOVE;   /* 兜底 */
}

/* 12 业务码 → 55 code（representative） */
static int e55_biz_to_55code(int biz)
{
    switch (biz) {
    case E55_BIZ_MOVE:     return 0;
    case E55_BIZ_BOMB:     return 2;
    case E55_BIZ_FLIP:     return 6;
    case E55_BIZ_GAMEOVER: return 16;
    case E55_BIZ_DRAW:     return 25;
    case E55_BIZ_RESIGN:   return 24;
    case E55_BIZ_REGRET:   return 42;
    case E55_BIZ_LOGIN:    return 34;
    case E55_BIZ_REG:      return 36;
    case E55_BIZ_NET:      return 38;
    case E55_BIZ_LAY:      return 44;
    case E55_BIZ_MISC:     return 46;
    default:               return 0;
    }
}

/* 对外入口：事件名字符串 → 55 code（76 事件名全链，原版行为移植深水区）
   返回：55 code 值（0..66），越界/未匹配兜底 0（Junqi.Move） */
int jq_evt55_name_to_55code(const char *evname)
{
    int biz = e55_name_to_biz(evname);
    return e55_biz_to_55code(biz);
}

/* 对外入口：事件名 → 12 业务码（诊断/统计用） */
int jq_evt55_name_to_biz(const char *evname)
{
    return e55_name_to_biz(evname);
}
