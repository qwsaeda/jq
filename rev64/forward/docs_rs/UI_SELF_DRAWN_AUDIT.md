# UI 自绘盘点（用户"盘点有多少胡搞的东西"指令产出）

> ★ 2026-11-13。背景：用户两次严厉指正——①"功能设置点开连地方都没找到"
> ②"orig_funcset.png 不是原图，连原版 App 都搞错了，UI 完全是自己写的"。
> 本盘点 = 正向（src_rs，com.qwsaeda.junqirs）全部弹窗/面板按钮绘制方式，
> 对照原版皮肤（SPAK 资源 + 原版 so 静态，原版在 MuMu 从未运行——EM_ARM vs EM_386）。

## 原版权威（重建后唯一可靠来源）

- 原版在 MuMu/红果**从未运行**（EM_ARM 32 位 so 无法在 x86_64 加载），此前"截图对照"全作废。
- 唯一权威 = `军棋.apk` SPAK 资源（spak_unpack 883）+ 原版 32 位 so 静态分析。
- **按钮皮肤 = btn_color2（datax_195，244×61，4 块渐变圆角：橙/紫/绿/蓝 块 61×61）**。
  原版按钮 = 渐变圆角立体皮肤（顶高光→主体→底暗）。
- **弹窗皮肤 dlg.png SPAK 不可得**（诚实边界）：dlg.cfg=datah_215 `dlg.png,8,55,16,59`
  （datax_215 `dlg.png,8,83,16,87`）但 dlg.png 位图不在 883 资源；data/sg/border.png 等
  运行时文件系统资源 APK 无。引擎程序化绘制或运行时加载 fallback。
  **弹窗浅底黑字**（datah_012 Title color=black 证实）。

## 盘点结果（纯色自绘按钮）

| 类别 | 数量 | 纯色自绘 | 原版皮肤 |
|---|---|---|---|
| 功能设置（rs_draw_gameset） | 1 | 0 | ✅ btn_color2 9patch + 米色面板 + 黑字标题 |
| 主要弹窗（非 d*/ui* 批量） | ~40 | 内联纯色 quad ~90 处 + rs_solid_btn ~25 | 0 |
| datah 批量 d* 弹窗 | 120 | 123 处 rs_solid_btn | 0 |
| ui* 批量（datah 面板） | 9 | 20 处 rs_solid_btn | 0 |

**合计：~230 处纯色矩形自绘按钮，仅 1 弹窗（功能设置）用真原版皮肤。**

## 主要弹窗明细（纯色自绘，待对齐）

| 弹窗 | 绘制 | 纯色处 |
|---|---|---|
| 主菜单（rs_draw_menu） | 纹理按钮（tex_btn_big/small 原版裁剪）+ 内联 | 2 |
| ☰ 图标菜单（rs_draw_imen） | 内联 | 3 |
| 人机难度（rs_draw_difficulty） | 内联纯色（2 色，选中高亮） | 3 |
| 联机方式（rs_draw_waysel） | 内联 | 3 |
| 声音设置（rs_draw_settings） | 内联 | 3 |
| 四国人数（rs_draw_nplayer） | 内联 | 3 |
| 两人新局（rs_draw_newgame） | 内联 | 4 |
| 布局管理（rs_draw_laymgr） | 内联 | 2 |
| 登录（rs_draw_login） | 内联 | 4 |
| 游戏规则（rs_draw_rules） | 内联 | 3 |
| 新局104（rs_draw_newgame104） | 内联 | 4 |
| 联机 join（rs_draw_join） | solid=1 + 内联 9 | 10 |
| ng061 | solid=1 | 1 |
| 对局信息/亮度/聊天/档案/设置/图集/房间/创建/规则070/chat063 | solid=2~4 | 各 2-4 |
| ui44-118（datah 面板） | solid | 1-4 |
| datah 批量 d* | solid | 120 弹窗 123 处 |

## 修复路线（未做，后续）

1. 主要弹窗按钮 → btn_color2 9patch（绿/蓝块，按按钮语义选色）
2. datah 批量 d* → btn_color2（或 sbtn/btn1 小按钮皮肤）
3. 弹窗面板 → 米色渐变（datax_188 三色，功能设置已用）
4. 标题黑字（Title color=black，功能设置已烘焙 gameset_title_black.png）
5. 逐弹窗区分"按钮"vs"遮罩"（内联 quad 里遮罩保留、按钮改皮肤）

## 判定标准（防再"胡搞"）

- 按钮必须有原版皮肤（btn_color2/btn.cfg 系），禁纯色矩形自绘（除非原版程序化绘制 fallback）。
- 面板浅底黑字（原版 Dialog color=black 标题），禁深色半透明自绘（除非原版 fallback）。
- 每次改动 → 真机像素级采样（非"非黑比"糊弄）+ 原版 SPAK 几何 Δ 对照。
