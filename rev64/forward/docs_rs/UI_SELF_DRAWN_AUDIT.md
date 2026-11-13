# UI 自绘盘点（用户"盘点有多少胡搞的东西"指令产出）

> ★ 2026-11-13。背景：用户两次严厉指正——①"功能设置点开连地方都没找到"
> ②"orig_funcset.png 不是原图，连原版 App 都搞错了，UI 完全是自己写的"。
> 本盘点 = 正向（src_rs，com.qwsaeda.junqirs）全部弹窗/面板按钮绘制方式，
> 对照原版皮肤（SPAK 资源 + 原版 so 静态，原版在 MuMu 从未运行——EM_ARM vs EM_386）。

## 原版权威（重建后唯一可靠来源）

- 原版在 MuMu/红果**从未运行**（EM_ARM 32 位 so 无法在 x86_64 加载），此前"截图对照"全作废。
- 唯一权威 = `军棋.apk` SPAK 资源（spak_unpack 883）+ 原版 32 位 so 静态分析。
- **按钮皮肤 = btn.png（★ 2026-11-13 逆向实锤，纠正旧 btn_color2 误判）**：
  - btn.cfg = datah_205 `btn.png,8,29,16,30`（9-patch left8 top29 right16 bottom30）
  - btn.png 三态 = datah_207/208/209（24×64 RGBA）：正常态中心 RGB(33,130,202) 蓝渐变 /
    按下 (42,138,210) / 禁用 (170,170,170) 灰
  - 旧"btn_color2（datax_195 244×61 4 色块）是按钮皮肤"**错误**：datax_195 实为另一资源
    （功能设置按钮 = Button style→btn.cfg→btn.png 蓝渐变，非 4 色块）
  - 高分版：datax_205 btn.cfg(8,44,16,45) + datax_207/208/209 btn.png(24×95)；btn1.cfg=datah_206
    (8,31,16,32) + btn1.png=datah_210/211/212(24×60)
  - 产物：`rev64/forward/src_rs/jq_btn_skin.h`（三态 RGBA 内嵌 C 数组，烘焙 bake_btn_skin.py）
- **弹窗皮肤 dlg.png 候选 = datah_187（480×490 米金渐变面板）**：
  - dlg.cfg = datah_215 `dlg.png,8,55,16,59`（datax_215 `dlg.png,8,83,16,87`）
  - datah_187 = 480×490，4 角 TL(231,216,192)米 → BR(91,77,55)暗棕，中心(207,180,132)米金，
    9-patch 兼容（宽≥24 高≥114）——对话框面板皮肤方向
  - ★ 诚实边界：datah_187 与 dlg.png 对应未 100% 实锤（SPAK 无显式名字表），作候选待验证
  - **弹窗浅底黑字**（datah_012 Title color=black 证实）。

## 盘点结果（纯色自绘按钮）

| 类别 | 数量 | 纯色自绘 | 原版皮肤 |
|---|---|---|---|
| 功能设置（rs_draw_gameset） | 1 | 0 | ✅ **btn.png 三态 9patch**（★ 2026-11-13，纠正旧 btn_color2 误判）+ 米色面板 + 黑字标题 |
| 主要弹窗（非 d*/ui* 批量） | ~40 | 内联纯色 quad ~90 处 + rs_solid_btn ~25 | 0 |
| datah 批量 d* 弹窗 | 120 | 123 处 rs_solid_btn | 0 |
| ui* 批量（datah 面板） | 9 | 20 处 rs_solid_btn | 0 |

**合计：~230 处纯色矩形自绘按钮，仅 1 弹窗（功能设置）用真原版皮肤。**

## 修复路线（已做 3，后续继续）

1. ✅ **功能设置按钮 → btn.png 三态 9patch**（★ 2026-11-13：jq_btn_skin.h 烘焙 +
   render.c rs_draw_btn9patch 改不对称(8/29/16/30) + rs_draw_gameset 用正常态）
2. ✅ **难度弹窗（datah_041）→ btn.png 9patch + rs_popup_shell 面板**（★ 2026-11-13：
   8 按钮布局/翻棋用正常态/选中=按下态，面板改统一弹窗皮肤（原版 BackDlg popup.cfg））
3. ✅ **主要弹窗按钮全改 btn.png + 面板全改 rs_popup_shell**（★ 2026-11-13 批量：
   声音设置 datah_001、新局 datah_062、联机方式 datah_072、四国人数 datah_089、
   人机新局 datah_104、登录 datah_038、游戏规则 datah_037、☰ 菜单 datah_006/067
   （面板 shell，图标 IconBtn 已真实位图）、布局管理 datah_016/021、保存布局
   datah_020、加入/创建 datah_034/035——全部替换旧纯色矩形（蓝/绿/紫/红/灰
   彩色区分）+ 深色半透明面板（原版 Button 同款 btn.png 蓝渐变 + Dialog/BackDlg
   弹窗皮肤）。深色面板（0.08,0.08,0.10f）残留清零）
4. ✅ **次级弹窗按钮全改 btn.png**（★ 2026-11-13 批量：新局补充 datah_061、
   对局信息 datah_050、亮度 datah_002、聊天记录 datah_029、个人资料 datah_031、
   聊天 datah_063、游戏规则完整版 datah_070、房间 datah_060、加入完整版
   datah_065、创建完整版 datah_069、软件信息 datah_000——全部替换旧纯色矩形
   （蓝/紫/橙/灰彩色区分 + 金色高亮描边），原版 Button 同款 btn.png 蓝渐变；
   面板均已 rs_popup_shell）
5. 对局控制条（d255-d261，datah_255-261 BackDlg 控制条 icon/按钮）→ btn.png/icon
   （次要对局内按钮，待做）
6. 弹窗面板 → dlg.png（datah_187 候选，需先验证）
7. 标题黑字（Title color=black，功能设置已烘焙 gameset_title_black.png）
8. 逐弹窗区分"按钮"vs"遮罩"（内联 quad 里遮罩保留、按钮改皮肤）

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

## ★ 2026-11-13 深层原因（用户"找深层原因"指令，hg_funcset.png 实锤）

**用户 hg_funcset.png = 旧版**（深色半透明面板 + 纯色矩形绿(76,178,127)/蓝(51,115,229)/灰取消），
正是指正的问题。**深层原因链（非表面修复）**：

1. **btn_color1/2 是误命名**：spak_unpack.py 按 `组_索引_hash.bin` 命名**无文件名映射**，
   之前会话把 datax_194/195 手动命名为 btn_color1/2.png。原版按钮皮肤**不是** btn_color1/2。
2. **原版按钮皮肤 = btn.png**（UiConfig datah_117 Button→btn.cfg；btn.cfg=datah_205
   `btn.png,8,29,16,30` / datax_205 `btn.png,8,44,16,45`，9-patch top44+bottom45=89）。
   **★ 2026-11-13 实锤推翻旧"btn.png 位图 SPAK 不可得"**：btn.png 三态 =
   **datah_207/208/209**（24×64 RGBA，正常态中心 RGB(33,130,202) 蓝渐变 / 按下(42,138,210) /
   禁用(170,170,170)灰），完美匹配 9-patch 宽 24=left8+right16；btn1.png = datah_210/211/212。
   datax_195(244×61) 高 61 < 89 **不满足 btn.cfg 9-patch 约束** → **datax_195 ≠ btn.png**。
3. **我用 datax_195 当功能设置按钮皮肤 = 错误资源**（不满足 btn.cfg 9-patch，且 4 色条
   非单按钮皮肤；datax_195 可能被原版代码直接索引用于主菜单/特定大按钮，非 XML style=Button）。
   **★ 已修**：功能设置 6 按钮改用 btn.png 正常态 9-patch（jq_btn_skin.h + rs_draw_gameset）。
4. **弹窗皮肤 dlg.png 候选 = datah_187**（480×490 米金渐变，9-patch 兼容）：
   dlg.cfg=datah_215 `dlg.png,8,55,16,59` / datax_215 `dlg.png,8,83,16,87`；
   ★ 诚实边界：datah_187 与 dlg.png 对应未 100% 实锤（SPAK 无显式名字表），作候选待验证。
   旧用 datax_188 米色**自绘近似**（非原版位图）。
5. **标题黑字是烘焙**（gameset_text_atlas row0 转黑），非原版资源。

**结论（★ 2026-11-13 修正）**：原版按钮皮肤 btn.png **已定位**（datah_207/208/209 三态，
9-patch 8/29/16/30），已烘焙 jq_btn_skin.h + 功能设置改用。dlg.png 候选 datah_187（待验证）。
旧"btn.png/dlg.png SPAK 不可得"是**误判**（SPAK 无文件名映射，btn.cfg/dlg.cfg 内容引用名，
位图按 9-patch 尺寸匹配定位成功）。原版引擎用**位图 9-patch 拉伸**（非纯程序化绘制）——
GL2 纯色四边形路径（0x9e3e4 glDrawArrays(5,0,4)）是**非按钮**控件 fallback/选中框。

**真对齐路径（★ 已突破）**：SPAK cfg 内容（btn.cfg=`btn.png,8,29,16,30`）→ 按 9-patch
尺寸匹配定位位图（btn.png=datah_207/208/209）→ 烘焙 → 渲染 9-patch 拉伸。剩余：
dlg.png 确认（datah_187 候选）+ 主要弹窗按钮逐一改 btn.png 9-patch。
