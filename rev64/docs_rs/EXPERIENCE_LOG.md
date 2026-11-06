
## §2026-11-06 真机验证链路三坑（"改了没效果"假象的真凶）+ ☰ 菜单原版对齐

> **一句话**：连续多轮"改了 native 代码但真机行为不变 / setprop 钩子不触发 / 截图仍是旧样子"，
> 根因**不在代码**，而在**装机链路**：① 相对路径 OUT 让 `cp` 静默失败（APK 从未更新）；
> ② "哈希对账"是自 push 副本对比自己（永远相等 = 假对账）；③ 源 APK 包名选错
> （`raw64_shadow18.apk` = `com.cnvcs.junqi`，不是 `com.cnvcs.junqirs`）。
> 三坑叠加 → 设备长期跑老 so，而"自检/对账"全报 PASS → **一切真机验证结论作废**。

### ① 相对路径 OUT → cp 静默失败（APK 永不更新）
`mumu_swap_so.sh` 里 `OUT=${1:-...}` 传相对路径时，脚本 `cd $WORK` 后 `cp ... "$OUT"` → 目标目录不存在 →
cp 报错被忽略 → **OUT 永不更新**。
- 固化：`OUT` 统一转绝对路径（相对路径按调用 CWD 解析）；打包后**自检** `unzip -p aligned_new.apk
  lib/arm64-v8a/libjunqi.so | md5sum` == `dist/libjunqi_rs.so` md5，不一致直接中止。

### ② 假哈希对账（自 push 副本对比自己）
旧 [4.5/6] 步骤：`adb push dist/libjunqi_rs.so /data/local/tmp/so_for_hash/` → 对比 `/data/local/tmp/...`
与本地 → **必然相等**，从没查设备上**已安装 APK 里**的 so。
- 固化：改查 `pm path $PKG` → `adb pull base.apk` → 读其 `lib/arm64-v8a/libjunqi.so` 的 sha256，
  对不上则**强制 uninstall + install** 重试，仍不一致则中止。

### ③ 源 APK 包名错（装到另一个包）
`SRCAPK=$(ls -t raw64_shadow*.apk raw64_v*.apk | head -1)` → 选中 `raw64_shadow18.apk`
（包名 `com.cnvcs.junqi`）→ 装出来的**不是** `com.cnvcs.junqirs`；设备上 junqirs 的 so 永远不变。
- 固化：源 APK 必须**按包名筛选**（`aapt dump badging` 校验 = `com.cnvcs.junqirs`），
  优先 `rs168_wav.apk` / `raw168_168.apk`；找不到就中止（宁可不装，不装错包）。

### ④ rs_read_png 参数序（asset 名在前、绝对 fs 路径在后）
`rs_read_png(AAssetManager*, const char *asset_path, const char *fs_path, ...)`：
`fs_path` 用 `fopen` 读（需**绝对路径**），`asset_path` 走 AAssetManager（需**相对 assets 路径**）。
写反 → 文件读不到 → 位图静默 MISS → 渲染退化（深色矩形/纯色块），日志无明显线索。
- 固化：`rs_read_png(am, "spak_res/xxx.png", p_abs, &raw, &len)`；新增位图一律加"加载 OK/FAIL"诊断日志。

### ⑤ 沉浸式/全屏 theme（view 偏移 → 触摸坐标全偏）
`dec_rs` 树 manifest 无 `android:theme` → 默认 theme 带 ActionBar + 状态栏 → GLView 不全屏 →
`MotionEvent.getY()` 相对 view 顶 → 与 720×1280 基准差一截 → **所有触摸命中失效**（误判为"按钮点了没反应"）。
- 固化：manifest `application` 加 `android:theme="@android:style/Theme.NoTitleBar.Fullscreen"`
  + App.smali 的 `FLAG_FULLSCREEN(0x400)` + `setSystemUiVisibility(0x1706)`（沉浸式 sticky）。

### ⑥ ☰ 菜单（datah_006/067）"按钮错乱"根因 = 两套坐标基准混用
- 原版 XML（450 系）1:1 平移 → 按钮列距仅 **105**；而文字宽度用了原版 900 系 ×0.8 的 **134**
  → 文字比按钮宽 29px，列距 105 → **4 列文字互相重叠**（用户截图证据）。
- 修正：☰ 菜单几何改按**原版真机截图实测**（`orig_ingame_menu.png` 900×1600 ×0.8 → 720 基准）：
  列心 x = **125/289/447/606**；图标行1 y=**516** / 文字行1 y=**594** / 图标行2 y=**703** / 文字行2 y=**779**；
  图标 96×96；文字宽按字数（4 字 122、2 字 61，源图集 160 宽 → tw=130 固定，字形随字数缩放）。
- 图标改用**apk 资源真值**（SPAK datax PNG 原图，4×4 合成图集 `imen_orig_icons.png` 384×384，每格 96）：
  本地 8 = datax_153 星 / 134 回转 / 128 箭头 / 163 纸 / 136 扳手 / 171 彩饼 / 124 蓝i / 142 红X；
  联机 8 = 星 / 回转 / 140 握手 / 179 红旗 / 132 气泡 / 扳手 / 彩饼 / 纸。
  ★ 不再用"从截图裁剪"的二手像素（裁剪会带入棋盘背景）。
- 面板：原版暗区实测 22..698 × 424..844（676×420）；XML 的 450×285 偏小。
- ★ 诚实边界：datah_006 的 XML rect 与实际渲染**不一致**（IconBtn 容器 rect ≠ 图标实际尺寸），
  故以截图实测为准并标注；原版弹窗面板底图 = `popup.cfg` 9-patch，真值未在 datax 定位
  （datax_246 实为「圆形按钮」素材，非面板 → 拉伸会盖内容，已撤下），面板仍用深色半透明矩形（退化底）。

### ⑦ 证据层级（本轮明确，供后续对齐引用）
| 层 | 来源 | 性质 | 用法 |
|---|---|---|---|
| apk 资源真值 | SPAK `datax_*.bin` PNG / `datah_*.bin` XML | 一手（apk 内） | 位图像素 + 布局声明 |
| 运行态截图 | `orig_*.png`（MuMu 900×1600 跑原版 32 位 APK） | 二手但反映实际渲染 | 几何实测（当 XML 与实际冲突时以它为准） |
| 我方自烘焙 | PIL + `data_font_all.ttf` | 自造 | 仅文字（字体本身=原版 ✓） |

### ⑧ 回归不变性
☰ 菜单几何改动**只动视觉与命中区**，走子逻辑未动：一致性 selftest **9/9**、evverify **47/47** 全过。
