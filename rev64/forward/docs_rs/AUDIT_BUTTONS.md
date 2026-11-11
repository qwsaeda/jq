# 按钮业务对抗审计表（证据链）

> 用户："所有的按钮业务没有对齐，如有出证据链，按按钮整理出来，对抗审计"。
> 证据来源：①原版 SPAK 资源解包（spak_unpack.py，GBK 解码对话框 XML）②逆向笔记 §13（0x485d0 难度菜单）③原版截图 orig_menu.png。
> 生成：2026-11-05。src_rs so `098c4062`。**本轮（11-05 对抗审计）4 强证据缺口全部修复**，so `b79c9720`。

## 一、主菜单（截图 orig_menu.png 5 按钮确证：人机/两人/联网/联机/四国军棋）

| 按钮 | 原版行为（证据） | 当前 src_rs | 缺口 |
|---|---|---|---|
| **人机对战** | 弹**难度选择** datah_041：新手区/中级区/高级区/好友区 × 布局/翻棋（8 按钮）。证据：SPAK datah_041 GBK 解码 `LowLay/LowFlip/MidLay/MidFlip/HighLay/HighFlip/RankLay/RankFlip` + §13 `0x485d0`（8 标签确证，`bl 0x82d3c` 字符串比较 + `bl 0xb4edc` 事件派发） | ✅ **UI_DIFFICULTY 弹窗**（本轮修复）：点人机 → 难度弹窗 → 选难度+布局/翻棋 → `ui_opt_select` + `g_vs_ai=1` + `jq_restart_game` → UI_GAME。真机实证：选"高级 布局"→ level3/mode0 开局 AI 走红 | **已修复** |
| **两人对战** | datah_104"新局"（我方 Me/难度 Difficulty/类型 Type/Ok/Cancel）——两人选我方颜色+类型；截图 orig_two 无难度弹窗 | ✅ 直开局（热座轮流，`g_vs_ai=0`） | 待确证（datah_104 是两人设置？现直开局对齐 orig_two 截图） |
| **联网** | 弹**方式选择** datah_072：蓝牙联机/Wi-Fi 联机/个人资料/返回（4 按钮）。证据：SPAK datah_072 GBK `Bt/Wifi/Config/Back` | ✅ **UI_WAYSEL 弹窗**（本轮修复）：点联网 → 方式选择 → 蓝牙联机 → `start_bt_session(host=1)`。Wi-Fi/个人资料=诚实边界对话框 | **已修复** |
| **联机** | 弹**方式选择** datah_072（蓝牙/WiFi） | ✅ **UI_WAYSEL 弹窗**（本轮修复）：点联机 → 方式选择 → 蓝牙联机 → `start_bt_session(host=0)` | **已修复** |
| **四国军棋** | datah_089 人数选择 2/3/4（SPAK `2/3/4`）——四国=4 人？或 datah_090 Comp/Net | ✅ 边界对话框（诚实边界） | 待确证（四国原版行为；现诚实边界 OK） |

## 二、☰ 游戏内菜单（本地模式 datah_006 确证 8 项）

| 项 | 原版 | 当前 src_rs | 缺口 |
|---|---|---|---|
| 重新开始 New | datah_006 | ✅ `jq_restart_game` | 无 |
| 悔棋 Regret | ✓ | ✅ 分模式悔棋 | 无 |
| 电脑变着 Change | ✓ | ✅ `ai_pick` 无条件 | 无 |
| 游戏规则 Rule | ✓ | ✅ 规则对话框 | 无 |
| 功能设置 Config | datah_001：声音 Sound/按钮音 BtnSound/音量 Volume + Ok/Cancel（SPAK GBK） | ✅ **UI_SETTINGS 弹窗**（本轮修复）：点功能设置 → 声音设置弹窗（静音模式/按键声音/音量 + 确定/取消），切换+确定保存 jq_settings.txt，取消重读恢复 | **已修复** |
| 战况统计 Stat | ✓ | ✅ 余子/步数 | 无 |
| 软件信息 About | ✓ | ✅ 版本 | 无 |
| 退出程序 Exit | ✓ | ✅ 回菜单 | 无 |

## 三、☰ 游戏内菜单（联机模式 datah_067 确证 8 项——与本地不同！）

| 项 | 原版（datah_067） | 当前 src_rs | 缺口 |
|---|---|---|---|
| 重新开始 New | ✓ | ✅ | 无 |
| 悔棋 Regret | ✓ | ✅ | 无 |
| **提和 Draw** | ✓（datah_067 `Draw` 提和） | ✅ **BT_T_DRAW 帧**（本轮修复）：蓝牙联机 ☰ 点提和 → `jq_bt_ctrl_send(BT_T_DRAW)` + 诚实边界对话框 | **已修复** |
| **认输 Resign** | ✓（datah_067 `Resign` 认输） | ✅ **BT_T_RESIGN 帧**（本轮修复）：蓝牙联机 ☰ 点认输 → 对方胜 + `jq_bt_ctrl_send(BT_T_RESIGN)` | **已修复** |
| **聊天 Chat** | ✓（datah_067 `Chat` 聊天） | ✅ **BT_T_CHAT 帧**（本轮修复）：蓝牙联机 ☰ 点聊天 → `jq_bt_ctrl_send(BT_T_CHAT)` + 诚实边界对话框 | **已修复** |
| 功能设置 Config | ✓ | ✅ → UI_SETTINGS 声音设置 | 无 |
| 战况统计 Stat | ✓ | ✅ 余子/步数 | 无 |
| 游戏规则 Rule | ✓（联机版含 Rule，无 Change/About/Exit） | ✅ | 无 |

> 联机 ☰ = New/Regret/**Draw/Resign/Chat**/Config/Stat/Rule（**无**电脑变着/软件信息/退出）。
> 本地 ☰ = New/Regret/Change/Rule/Config/Stat/About/Exit。
> ✅ 本轮修复：`ui_bt_set_active()` 标志区分两套菜单 + `ui_imen_bt_buttons()` 联机版按钮表 + `btimen_text_atlas.png` 文字图集。

## 四、对局设置（datah_104"新局"：我方 Me/难度 Difficulty/类型 Type）

- datah_104 = 新局设置弹窗（450×390，我方/难度/类型/Ok/Cancel）——人机或两人新局设置。
- 当前 src_rs：人机走 datah_041 难度弹窗（本轮修复），两人直开局（orig_two 截图自证）。**datah_104 待确证**（原版两人是否弹此设置？现直开局对齐截图）。

## 五、证据链总结（4 强证据缺口 → 本轮全部修复）

1. **人机**：原版弹难度选择（datah_041 8 标签：新手/中级/高级/好友 × 布局/翻棋）——✅ **UI_DIFFICULTY 弹窗**（本轮修复，真机实证选"高级 布局"→ level3/mode0 AI 走红）。
2. **联网/联机**：原版弹方式选择（datah_072：蓝牙/Wi-Fi/个人资料）——✅ **UI_WAYSEL 弹窗**（本轮修复，真机实证蓝牙联机→ host=1 OUR_TURN）。
3. **☰ 联机菜单**：原版 datah_067（含提和/认输/聊天，无电脑变着/软件信息/退出）——✅ **联机 ☰ 菜单分流**（本轮修复，真机实证三项 action 均触发 DRAW/RESIGN/CHAT 帧）。
4. **功能设置**：原版 datah_001（声音/按钮音/音量）——✅ **UI_SETTINGS 弹窗**（本轮修复，真机实证静音切换+确定保存 jq_settings.txt）。

## 六、已对齐（无缺口）

- ☰ 本地菜单 datah_006 8 项标签/顺序与当前一致。
- 主菜单按钮几何（x/y 对齐原版截图）、UI 纹理、回合校验、人机/两人 AI 区分、悔棋分模式。

## 七、本轮修复记录

- **btconn 回归失败根因**：源 APK `rs168_wav.apk` 被从 `dec_rs`（无 bt 接线）重建，丢失 `BtSpp.smali`/`App$BtStart.smali`/`btState`——native `GetStaticMethodID("btState")` 失败。修复：从 `dec_dual`（含 bt 接线）重建 `rs168_wav.apk` + 加 3 新 PNG（waysel/btimen/settings text atlas），btconn 回归 8/9 → 9/9 PASS。
- **文字图集**：`waysel_text_atlas.png`（4 标签 220×184）、`btimen_text_atlas.png`（8 项 160×320）、`settings_text_atlas.png`（10 标签 200×460），均用 `data_font_all.ttf` sz38（与现有 imen_text_atlas 逐像素匹配 cnt=2585）。
