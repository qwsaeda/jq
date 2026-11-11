/* jq_strings.h — 原版界面字符串表真值（自动生成，勿手改）
 *
 * 来源: rev64/dist/spak_res/data_config_string.xml（SPAK data 组，StringConfig/StringSet/String）
 *        = 原版 32 位 APK 的界面文案权威表（79 条）。
 * 生成: python3 rev64/tools/spak_strings.py --list  → 本文件
 * ★ 用途: 我方 UI 文案一律引用本表（不再自编文案，避免与原版不一致）。
 *   例: jq_show_dialog(env, "退出", JQ_STR_ExitConfirm);
 *   注: 原版 {#Y}..{/#Y} 是富文本高亮标记，src_rs 不渲染富文本 → 用纯文本副本。
 */
#ifndef JQ_STRINGS_H
#define JQ_STRINGS_H

/* ---- Junqi ---- */
#define JQ_STR_BlueWin "蓝方胜利！"   /* BlueWin */
#define JQ_STR_ConnLost "连接断开了"   /* ConnLost */
#define JQ_STR_ConnectFail "建立连接失败"   /* ConnectFail */
#define JQ_STR_Draw "和棋！"   /* Draw */
#define JQ_STR_DrawAgree "对方同意和棋"   /* DrawAgree */
#define JQ_STR_DrawDisagree "对方拒绝和棋"   /* DrawDisagree */
#define JQ_STR_EmailError "请输入有效的电子邮箱"   /* EmailError */
#define JQ_STR_EmailLenError "电子邮箱长度不能超过40个字符"   /* EmailLenError */
#define JQ_STR_ExitConfirm "真的要退出吗？"   /* ExitConfirm */
#define JQ_STR_ExitFail "游戏进行中，无法退出"   /* ExitFail */
#define JQ_STR_GameOver "游戏已结束"   /* GameOver */
#define JQ_STR_GameType1 "翻棋 - 暗棋"   /* GameType1 */
#define JQ_STR_GameType3 "布局 - 暗棋"   /* GameType3 */
#define JQ_STR_HighAreaFail "您的积分低于1800，不能进入高级区"   /* HighAreaFail */
#define JQ_STR_InputConfirmPwd "请再次输入密码："   /* InputConfirmPwd */
#define JQ_STR_InputEmail "请输入电子邮箱："   /* InputEmail */
#define JQ_STR_InputName "请输入昵称："   /* InputName */
#define JQ_STR_InputPassword "请输入密码："   /* InputPassword */
#define JQ_STR_InputUser "请输入用户名："   /* InputUser */
#define JQ_STR_JoinGame "%s{/#Y}加入游戏"   /* JoinGame */
#define JQ_STR_LayError "还有未摆放的棋子"   /* LayError */
#define JQ_STR_LeaveGame "%s{/#Y}离开游戏"   /* LeaveGame */
#define JQ_STR_LeiError "地雷只能放在最后两行"   /* LeiError */
#define JQ_STR_Load "布局%d"   /* Load */
#define JQ_STR_LoadExam "参考布局%d"   /* LoadExam */
#define JQ_STR_LoginAuthFail "用户名或密码错误"   /* LoginAuthFail */
#define JQ_STR_LoginFail "登录失败"   /* LoginFail */
#define JQ_STR_LoginMaxUser "系统忙，请稍后再试"   /* LoginMaxUser */
#define JQ_STR_LoginVerOld "游戏版本过旧，请下载安装最新版本"   /* LoginVerOld */
#define JQ_STR_Lose "失败！"   /* Lose */
#define JQ_STR_MidAreaFail "您的积分低于1400，不能进入中级区"   /* MidAreaFail */
#define JQ_STR_NameCharError "昵称不能含有特殊字符"   /* NameCharError */
#define JQ_STR_NameLenError "昵称必须在4～14个字符之间"   /* NameLenError */
#define JQ_STR_NewFail "游戏进行中，无法重新开始"   /* NewFail */
#define JQ_STR_NewPwdLenError "新密码必须在6～16个字符之间"   /* NewPwdLenError */
#define JQ_STR_NewPwdNotMatch "两次输入的新密码不一样"   /* NewPwdNotMatch */
#define JQ_STR_OriPwdLenError "原密码必须在6～16个字符之间"   /* OriPwdLenError */
#define JQ_STR_PeerFirst "先手方: %s{/#Y}"   /* PeerFirst */
#define JQ_STR_PeerLast "后手方: %s{/#Y}"   /* PeerLast */
#define JQ_STR_PeerLayFinish "对方完成布局"   /* PeerLayFinish */
#define JQ_STR_PeerStart "对方准备好了"   /* PeerStart */
#define JQ_STR_PeerWait "请对方在 %d{/#Y} 秒内加入游戏"   /* PeerWait */
#define JQ_STR_PlayerLeave "对方离开游戏"   /* PlayerLeave */
#define JQ_STR_PwdLenError "密码必须在6～16个字符之间"   /* PwdLenError */
#define JQ_STR_PwdNotMatch "两次输入的密码不一样"   /* PwdNotMatch */
#define JQ_STR_QiError "军旗只能放在大本营"   /* QiError */
#define JQ_STR_Ready "准备"   /* Ready */
#define JQ_STR_ReconnFail "重连失败！"   /* ReconnFail */
#define JQ_STR_Reconnecting "连接断开，正在重连..."   /* Reconnecting */
#define JQ_STR_RedWin "红方胜利！"   /* RedWin */
#define JQ_STR_RegExist "用户名已存在，请换一个试试"   /* RegExist */
#define JQ_STR_RegFail "注册失败"   /* RegFail */
#define JQ_STR_RegOk "注册成功！请牢记您的用户名和密码。"   /* RegOk */
#define JQ_STR_RegretAgree "对方同意悔棋"   /* RegretAgree */
#define JQ_STR_RegretConfirm "您真的要悔棋吗？"   /* RegretConfirm */
#define JQ_STR_RegretDisagree "对方拒绝悔棋"   /* RegretDisagree */
#define JQ_STR_RegretFail "现在不能悔棋"   /* RegretFail */
#define JQ_STR_RegretReq "对方悔棋，您同意吗？"   /* RegretReq */
#define JQ_STR_RegretSend "向对方请求悔棋"   /* RegretSend */
#define JQ_STR_RemoveLay "删除%s{/#Y}？"   /* RemoveLay */
#define JQ_STR_Resign "对方认输"   /* Resign */
#define JQ_STR_UserCharError "用户名不能含有特殊字符"   /* UserCharError */
#define JQ_STR_UserLenError "用户名必须在4～16个字符之间"   /* UserLenError */
#define JQ_STR_Win "胜利！"   /* Win */
#define JQ_STR_ZhaError "炸弹不能放在第一行"   /* ZhaError */
/* ---- Common ---- */
#define JQ_STR_ClearConfirm "确定清空吗？"   /* ClearConfirm */
#define JQ_STR_DeleteConfirm "确定删除吗？"   /* DeleteConfirm */
#define JQ_STR_DownloadConfirm "是否下载？"   /* DownloadConfirm */
#define JQ_STR_SaveFail "保存失败！"   /* SaveFail */
#define JQ_STR_SaveOk "保存成功！"   /* SaveOk */
#define JQ_STR_Update "有新的版本，是否更新？"   /* Update */
/* ---- Sg ---- */
#define JQ_STR_DrawConfirm "您真的要提和吗？(%d)"   /* DrawConfirm */
#define JQ_STR_DrawReq "%s提和，您同意吗？"   /* DrawReq */
#define JQ_STR_DrawSend "向其他人提和"   /* DrawSend */
#define JQ_STR_LeaveConfirm "您真的要离开吗？"   /* LeaveConfirm */
#define JQ_STR_ResignConfirm "您真的要投降吗？"   /* ResignConfirm */
/* ---- Misc ---- */
#define JQ_STR_Graph0 "原始大小"   /* Graph0 */
#define JQ_STR_Graph1 "维持比例"   /* Graph1 */
#define JQ_STR_Graph2 "适合屏幕"   /* Graph2 */


#endif /* JQ_STRINGS_H */