/* jq_event_names_73.c — 原版 73 事件名常量表实现（生成自 event_code_map.json）
 * 生成：python3 rev64/tools/gen_event_names.py（幂等，改 json 后重跑） */
#include "jq_event_names_73.h"

const char *const jq_event_names_73[JQ_EVENT_NAME_COUNT] = {
    "Junqi.Move",  /* [0] */
    "Junqi.Eat",  /* [1] */
    "Junqi.Bomb",  /* [2] */
    "Junqi.Flag",  /* [3] */
    "Junqi.Dead",  /* [4] */
    "Junqi.Flip",  /* [5] */
    "Junqi.Win",  /* [6] */
    "Junqi.Lose",  /* [7] */
    "Junqi.RedWin",  /* [8] */
    "Junqi.BlueWin",  /* [9] */
    "Junqi.Draw",  /* [10] */
    "Junqi.GameOver",  /* [11] */
    "Junqi.Over",  /* [12] */
    "Junqi.DrawReq",  /* [13] */
    "Junqi.DrawAgree",  /* [14] */
    "Junqi.DrawDisagree",  /* [15] */
    "Junqi.DrawConfirm",  /* [16] */
    "Junqi.DrawSend",  /* [17] */
    "Junqi.Resign",  /* [18] */
    "Junqi.ResignConfirm",  /* [19] */
    "Junqi.RegretReq",  /* [20] */
    "Junqi.RegretAgree",  /* [21] */
    "Junqi.RegretDisagree",  /* [22] */
    "Junqi.RegretConfirm",  /* [23] */
    "Junqi.RegretSend",  /* [24] */
    "Junqi.RegretFail",  /* [25] */
    "Junqi.InputUser",  /* [26] */
    "Junqi.InputEmail",  /* [27] */
    "Junqi.InputPassword",  /* [28] */
    "Junqi.InputConfirmPwd",  /* [29] */
    "Junqi.LoginFail",  /* [30] */
    "Junqi.LoginAuthFail",  /* [31] */
    "Junqi.LoginMaxUser",  /* [32] */
    "Junqi.LoginVerOld",  /* [33] */
    "Junqi.EmailError",  /* [34] */
    "Junqi.EmailLenError",  /* [35] */
    "Junqi.InputName",  /* [36] */
    "Junqi.RegExist",  /* [37] */
    "Junqi.RegFail",  /* [38] */
    "Junqi.RegOk",  /* [39] */
    "Junqi.UserCharError",  /* [40] */
    "Junqi.UserLenError",  /* [41] */
    "Junqi.NameCharError",  /* [42] */
    "Junqi.NameLenError",  /* [43] */
    "Junqi.PwdLenError",  /* [44] */
    "Junqi.PwdNotMatch",  /* [45] */
    "Junqi.NewFail",  /* [46] */
    "Junqi.NewPwdLenError",  /* [47] */
    "Junqi.NewPwdNotMatch",  /* [48] */
    "Junqi.OriPwdLenError",  /* [49] */
    "Junqi.JoinGame",  /* [50] */
    "Junqi.Ready",  /* [51] */
    "Junqi.Start",  /* [52] */
    "Junqi.PeerStart",  /* [53] */
    "Junqi.PeerFirst",  /* [54] */
    "Junqi.PeerLast",  /* [55] */
    "Junqi.PeerLayFinish",  /* [56] */
    "Junqi.PeerWait",  /* [57] */
    "Junqi.LeaveGame",  /* [58] */
    "Junqi.PlayerLeave",  /* [59] */
    "Junqi.Reconnecting",  /* [60] */
    "Junqi.ReconnFail",  /* [61] */
    "Junqi.ConnectFail",  /* [62] */
    "Junqi.ExitFail",  /* [63] */
    "Junqi.ExitConfirm",  /* [64] */
    "Junqi.LayError",  /* [65] */
    "Junqi.HighAreaFail",  /* [66] */
    "Junqi.MidAreaFail",  /* [67] */
    "Junqi.ZhaError",  /* [68] */
    "Junqi.LeiError",  /* [69] */
    "Junqi.QiError",  /* [70] */
    "Junqi.Select",  /* [71] */
    "Junqi.GameType",  /* [72] */
};

/* [code][idx] = 名称下标，-1 结束；12 事件码 × 最多 16 名/码 */
const int jq_event_code_names[12][16] = {
    /* code 0 (2 名) */ {0, 1, -1},
    /* code 1 (3 名) */ {2, 3, 4, -1},
    /* code 2 (1 名) */ {5, -1},
    /* code 3 (7 名) */ {6, 7, 8, 9, 10, 11, 12, -1},
    /* code 4 (5 名) */ {13, 14, 15, 16, 17, -1},
    /* code 5 (2 名) */ {18, 19, -1},
    /* code 6 (6 名) */ {20, 21, 22, 23, 24, 25, -1},
    /* code 7 (11 名) */ {26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, -1},
    /* code 8 (13 名) */ {37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, -1},
    /* code 9 (15 名) */ {50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, -1},
    /* code 10 (6 名) */ {65, 66, 67, 68, 69, 70, -1},
    /* code 11 (3 名) */ {71, 72, 12, -1}
};
