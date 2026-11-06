# 军棋 APK 逆向工程工作区

这是"军棋"游戏的 APK 逆向工程工作区，**不是** 源码仓库（指游戏源码；工作区本身的规范/经验/路线代码已建 git 仓库存入 `https://github.com/qwsaeda/jq.git`，remote `origin` / branch `main`，排除二进制/大文件/PDF/课程素材，见 `.gitignore`）。
用户回答任何问题（能否装、能否玩、进度、状态）时，**只能回答"在阶段 X"**，不展开细节。

## 仓库结构

- `军棋.apk` — 原始 APK（`com.cnvcs.junqi`，v152/1.52，构建于 API 19 时代）
- 解码产物放 `/tmp/opencode/jq_apk/`（apktool 输出在 `/tmp/opencode/jq_apk/dec/`）
- 各阶段产物放 `/disk/workspaces/jq/dist/`
- 32 位 native `.rodata` 明文 dump 放 `/disk/workspaces/jq/frida-dump/`（非壳解密，静态文件即明文）；Frida 工作流系统参考见 `frida-dump/FRIDA_NOTES.md`（18 地址 dump / Jnitrace 看 JNI 调用流 / 与 unidbg 两条路互补 / §9 Stalker 指令级追踪模板）

## git 仓库（工作区规范/经验/路线代码入库）

- 远程：`https://github.com/qwsaeda/jq.git`（`origin`，branch `main`）
- 入库：顶层规范（AGENTS.md / REVERSE_PLAN.md / jni_reversal_report.md / .gitignore）+ `rev64/docs_rs/`（64 位正向规范）+ `rev64/phases/` + `rev64/analysis/` + `rev64/src_jni_translated/`（32 位 R6 翻译体）+ `course_unidbg/`（31 章课程 md）+ `frida-dump/`（Frida 工作流脚本）
- 排除（.gitignore）：二进制（*.so / *.apk / *.dex / *.keystore）/ 大文件（*.pdf）/ 自动生成大头（jq_board_px.h / jq_texdata_board.h / jqt_vtbl.c / jq_data_blob.* / jq_rel_data.h）/ 二进制 key（spak_key*.bin）/ 课程素材（videos / assets）/ 构建中间产物（rev64/tools/bin / rev64/out / rev64/jq_img.bin / rev64/native64/*.o）
- 推送：`git push -u origin main`（需 GitHub credentials，本机 `~/.git-credentials` 已配）
- 详细清单 + 红线见 `rev64/docs_rs/EXPERIENCE_LOG.md` §"仓库 / 规范固化"

## 已验证的现状事实

- `minSdk 4` / `targetSdk 19` / `installLocation=preferExternal`
- 无源码，Java 侧只有 ~7 个类（`com.cnvcs.App`、`GLView`、`SiteView`、`AdLis`、`BtReceiver`、`TextDlg`、`InputDlg`），几百行 smali
- 游戏核心逻辑在 native 层：`lib/armeabi/libjunqi.so`，**ELF32 / ARM / EABI5 / stripped**，依赖 `libstdc++.so`(32 位)、`libGLESv1_CM.so`（GL1 渲染），导出 15 个 `Java_com_cnvcs_App_On*` JNI 符号
- **`.text` 代码段是明文**（objdump 反汇编出真实 ARM/Thumb 指令 + JNI 符号名，熵 7.10 = 正常机器码）
- **`.rodata` 字符串池也是明文**（非加密）：已知串 `Junqi.Move/updateUrl/statHost/com/baidu/mobads/AdView` 在**静态文件里即可见**；静态 `.rodata` 与运行时 dump 逐字节相同（0/118832 不同）→ **无运行时自解密**
  - 纠正：之前写"`.rodata` 加密、dlopen 自解密"是**错的**。唯一高熵点 0x138000(7.87) 在 `.rodata` 只读数据段内，是压缩/索引表（值非有效地址），**非加密代码**
  - **结论：这不是加壳 so**。无 `.text` 加密、无 `.rodata` 加密、无运行时自解密。unidbg 失败根因是 `.got` 零槽位（未重定位）+ 初始化例程反检测，与壳无关
- 广告层（拆除目标）：
  - `assets/__xadsdk__remote__final__.jar`（内嵌第二个 dex 的远程广告 SDK）
  - `smali/com/baidu/mobads/` + `smali/com/baidu/mobad/`（数百个类）
  - manifest 中 `com.baidu.mobads.AppActivity` + `UMENG_APPKEY`
  - native 层 `FindClass("com/baidu/mobads/AdView")` 直接引用（全删 baidu smali 前必须先处理这个引用点）
- 工具：`apktool`、`baksmali`、`java 17`、`keytool`、NDK r25b 已装；签名用 `apksig.jar` 的 `SignV2`（v1+v2）
- 全面屏/沉浸式**不是障碍**（manifest 写法在新系统上被自动接管）；真正卡安装的是 `minSdk 4` 太旧

## 核心规范（先读这个，避免重踩坑）

### 重打包三件套（缺一个都装不上）

```
# 1. 解码 → 改 → 重打包
apktool d -f 军棋.apk -o /tmp/opencode/jq_apk/dec
apktool b /tmp/opencode/jq_apk/dec -o out.apk

# 2. 强制 .so/.arsc/.png/.pak 为 STORE（不压缩）—— apktool 默认会压缩，装必失败
python3 /tmp/opencode/jq_apk/store_compressed.py out.apk out_stored.apk

# 3. v1+v2 双签 —— 旧版 Android 也接受 v2，新版必须
java -cp /tmp/opencode/apksigner/apksig.jar:/tmp/opencode/apksigner SignV2 \
     out_stored.apk /disk/workspaces/jq/junqi.keystore junqi2026 junqi junqi2026
```

安装失败排查顺序：①`.so` 是否被 zip 压缩（必须 store）→ ②是否缺 v2 签名 → ③`armeabi`（非 v7a）在 64 位 only 设备装不上 → ④`resources.arsc` 未 4 字节对齐（`aapt dump badging` 报 "stored uncompressed and aligned on a 4-byte boundary"）。

### MuMu 翻译体内存访问规范（§124x 沉淀，先读这个）

- **镜像偏移**：`jq_img.bin` 从 `.text` 起点 `0x233e0` 开始（`gen_shadow.py IMG_LO=0x233e0`，`img[i]=原版[0x233e0+i]`），必须 `memcpy(shadow+0x233e0, img, sz)`；`img_end=0x233e0+sz=0x15be80`，`.bss` 从 `img_end` 清零。**拷到 `shadow+0` 是错**（所有 `.text`/`.rodata` MEM 读偏移 `+0x233e0`）。验证：`MEM(0x6e9c0)=0xec728`、`MEM(0x120bf8)="text"`
- **MEM 必须支持真实映射地址**：`JQT_VALID` 不能只允许影子区 `[g_jqt_shadow, g_jqt_shadow+MAP_HI)`——`_Znwj`/malloc 返回的真实堆指针（`0xea58feb0`，`[anon:scudo:primary]`）会被拒落零页读 0 → 链表遍历永不终止（无限循环，主线程 spin）。必须 `jqt_mem_valid()`（偏移/影子区/`/proc/self/maps` 真实映射区表三路命中）
- **真实映射区表上限 ≥4096**：设备 `/proc/self/maps` 有 **~1943 条**，`JQT_REAL_MAX=256` 满表漏 scudo 堆（实测 `MISS n=256`）→ 扩 `4096` + `qsort` + 二分（实测 `real_n=1799`）；线性扫 1799 条/次会拖慢 MEM
- **真实映射区表快照时机**：`jqt_mem_init`（JNI_OnLoad）建表，scudo 堆在 OnCreate 前已映射，能覆盖；堆后续增长新区不在表内（当前够用，若 MISS 可重建）
- **`.data` 段 + RELATIVE 重定位**：`jq_rel_data.h`（28288B `.data` 0x155000-0x15be80 + 5312 RELATIVE 槽），`memcpy(shadow+0x155000, jq_data_blob, 28288)` + 每槽 `+= g_jqt_shadow`（影子"load_base"=g_jqt_shadow）
- **外部函数指针参数转绝对**：`JQT_ABS_PTR(a)=(a<MAP_HI ? shadow+a : a)`（偏移→绝对；已是绝对保持）

### 拿 32 位 native 明文的正确路线（按性价比排序）

1. **redroid / 真 Android 容器跑原版 32 位 so**（最终验证可行）：
   - 容器：`registry.cn-hangzhou.aliyuncs.com/zywx/lingyan-face-check-service:android_tts_1210`（x86 宿主 + houdini 转译 ARM32，无需 KVM）
   - 起容器：`docker run -d --privileged -p 5555:5555 <image>`，`adb connect localhost:5555`
   - 触发：`am start` 让 so dlopen → `dd /proc/PID/mem` dump `.rodata`（静态即明文，dump 仅为确认）
   - 产物：`frida-dump/rodata_dec.bin`（118KB）+ `rodata_strings.txt`（1715 条，含 `Junqi.Move/Flip/Flag/Bomb/Eat/Dead`、`data/config/*.xml`、`updateUrl/statHost/cfgHost`）
   - 限制：redroid 无 GPU（GL 渲染不可用，app 进不了主界面）、64 位 native bridge 不完整（arm64 库 dlopen 失败）。只用于**提取数据**，不能验证"能玩"
2. **unidbg/unicorn2 纯 x86 模拟 32 位 so**（**已验证可突破 init_array + 重定位，非死路**——见 REVERSE_NOTES §84）：12 个 init_array 全通、RELATIVE 5312 槽全重定位、PLT→GOT→符号映射 214/214、GLOB_DAT 6 符号从 docker 真 ARM libc 解析；依赖 libc/libstdc++/libm/libdl/libGLESv1_CM/libGLESv2 可加载。**★ PC off-by-2 已实证修复：切换 Unicorn2 后端**（dynarmic 对 Thumb `add rX,pc` 用 `(addr+4)&~3` mask，unicorn2 用 `(addr+4)` 不 mask，与真实 ARM 一致；DiagTracePC 实证 0xa37fc 存 0x159880=不 mask 值）。**★ JNIEnv/VM 补环境实证可行**（DiagJni：createDalvikVM + resolveClass + callStaticJniMethodObject，OnCreate 从空 env 入口即崩推进到 std::vector 数据与指令对齐点 0x43ed8）。**★ 本轮（§124）纠正**：checkpoint 的"BL off-by-2 -0xa"是**误读**（`F000 F800` 正确目标是 0x10000004 非 0x10000020，隔离环境 BL 全对）；但**加载 .so 后 BL 0x7965a 上下文相关错位到 0x6eaea**（非 0x43ed8，memset 循环，非函数入口）——**模拟器精度 bug**（unicorn2 上下文相关），**非翻译/逻辑 bug**。剩余阻塞 = **① 上下文相关 BL 解码 bug**（新，需 workaround）+ **② 数据与指令对齐对应缺口**（std::vector begin/end 未初始化，N44：业务数据 apk 静态可推，缺运行时内存对齐；原"C++ 对象图语义缺失"措辞已纠正）+ **③ 288MB PROT_NONE mmap**（unidbg mmap2 处理失败）。unidbg 强项（补环境/反检测/花指令/加密壳处理）正可用于攻克这些——**别轻易下"死路"结论**
3. **反汇编直接看**：`.text` 和 `.rodata` 都是明文，`objdump` 可直接反汇编 `.text`、直接读 `.rodata` 字符串

### 64 位 native 的诚实边界

- 当前 arm64 `libjunqi.so`（`native64/junqi.cpp`，~97KB）是**简化重写版**（10×9 棋盘、颜色棋子、自动走子、GLES2 渲染），**不是** 32 位 1.43MB/1435156B native 的汇编级逆向
- 它能在 64 位设备"跑起来"，但玩法/规则/资源与原版不一致
- 完整对齐（原版 38 子布局、地雷/炸弹/悔棋规则、蓝牙联机协议、SPAK 解密 key）需后续把 32 位 `.rodata` 明文逻辑移植进 arm64，工作量另计
- **不要在交付物里把"简化版"包装成"完整版"**

## 四个阶段

验收判据（统一）：装上 → 进主界面 → 下一局（本地或蓝牙）→ 正常退出 → 重进不崩。

### 阶段 0：去广告 + 旧 ABI 快速包

- 产物：`dist/phase0.apk`（v1+v2，`armeabi` 原样）
- 改动：`minSdk 19`、`targetSdk 34`、去广告（删 `__xadsdk__remote__final__.jar`、删 manifest 中 `com.baidu.mobads.AppActivity` 和 `UMENG_APPKEY`、去广告相关权限 `INTERNET`/`ACCESS_NETWORK_STATE`/`ACCESS_WIFI_STATE`/`READ_PHONE_STATE`/`ACCESS_COARSE_LOCATION`；保留 `BLUETOOTH(_ADMIN)`）
- 验收：32 位设备（Android 5.0–13）能装能玩；断网启动无广告浮层
- 已知限制：64 位 only 设备装得上但加载库失败（预期内）

### 阶段 1：双 ABI 可装

- 产物：`dist/phase1.apk`（v1+v2，`armeabi` + `arm64-v8a` 双目录，arm64 为 5KB 占位桩）
- 验收：32 位设备同阶段 0；64 位 only 设备能装但 arm64 桩跑不起来（预期，作为阶段 2 入口）

### 阶段 2：真 64 位 native（全设备可装，64 位可玩）

- 产物：`dist/phase2.apk`（v1+v2，`armeabi-v7a` + `arm64-v8a`，`minSdk 21` / `targetSdk 34`）
- 改动：`armeabi` 升级为 `armeabi-v7a`（v7a 兼容 v5，32 位 native 原样复用）；arm64 用 NDK 编译**简化版**C++ 实现（`native64/junqi.cpp` → `lib/arm64-v8a/libjunqi.so`，~97KB，无 GL 依赖，仅 `liblog/libm/libandroid`）：
  - 15 个 JNI 入口签名与 `smali/com/cnvcs/App.smali` 一致
  - 军棋棋盘（10×9，38 子）+ 走子/吃子/炸弹/悔棋规则 + 简单 AI（blue 方）
  - 事件名对齐 32 位 `.rodata` 明文（`Junqi.Move/Flip/Flag/Bomb/Eat/Dead` 等）
- 注意：arm64 是**简化重写版**，非原版汇编级逆向；SPAK 解密 key 未提取，资源图占位；32 位设备走 v7a 能玩原版完整逻辑
- 验收：64 位 only 设备能装，arm64 简化版可渲染可交互；32 位设备走 v7a 能装能玩原版
- 已知代价：`minSdk 21` 起，Android 4.4–5.0 旧设备装不了

### 阶段 3：去广告终态（面向 64 位设备）

- 产物：`dist/phase3.apk` = 阶段 2 基础上完整去广告
- 改动：删 `smali/com/baidu/*` 全量、删 `smali/com/cnvcs/AdLis.smali`、删 `assets/__xadsdk__remote__final__.jar`、manifest 去全部广告项（INTERNET/NETWORK/WIFI/PHONE/LOCATION 权限、baidu AppActivity、UMENG_APPKEY）
- 验收：64 位设备断网能装、无广告浮层、无 baidu 类、无 ad jar、网络权限 0 项；32 位设备不在本阶段验收范围

## 红线（不要碰）

- 不要改动 `assets/data*.pak` 的 `doNotCompress` 标记（动了会破坏资源加载）
- 全删 `smali/com/baidu/*` 前必须处理 native 里 `FindClass("com/baidu/mobads/AdView")` 引用：32 位 `lib/armeabi-v7a/libjunqi.so` 仍引用 baidu，阶段 2 保留 `smali/com/baidu/*` + `smali/com/cnvcs/AdLis.smali`；阶段 3 删全量后仅 arm64 native 可用
- 重打包必须用新 keystore 签名（`/disk/workspaces/jq/junqi.keystore`，密码 `junqi2026`，alias `junqi`；原签名证书不可用）
- 不要做 64 位 native 时用 `dlopen` 32 位库的"混合方案"（Android 不支持，走不通）
- 不要修改 `smali/com/cnvcs/App.smali` 里 native 调用签名
- **不要在交付物里把 arm64 简化版说成"完整版"**

## 用户约束

- 回答任何问题（能装吗/能玩吗/进度如何）：**只说"在阶段 X"**，不展开
- 严格按上面 4 个阶段顺序推进，不做跳阶段


## 文档路由表（AGENTS.md 只做路由，规范/经验在单一文档）

> AGENTS.md = 工作区顶层规范 + 路由索引。**各主题内容在单一文档**，勿全塞 AGENTS.md。

### 正向开发（src_rs 64 位逆源码重写）

| 文档 | 内容 | 状态 |
|---|---|---|
| `rev64/docs_rs/SPEC_BUSINESS.md` | 业务规格说明书（规则条款/事件契约/UI 流程/音效映射/布局/蓝牙） | ✅ 成文（本轮） |
| `rev64/docs_rs/GAME_DEV_GUIDELINE.md` | 游戏开发规范（状态机/返回契约/锁纪律/音效三入口/重开悔棋/回归） | ✅ 成文（本轮） |
| `rev64/docs_rs/CODE_STYLE.md` | 代码规范（JNI 反射/链接安全/命名/坑防复发） | ✅ 成文（本轮） |
| `rev64/docs_rs/EXPERIENCE_LOG.md` | src_rs 经验库（闭环事实/坑清单/工具清单/方法论） | ✅ 成文 + 对抗审计修复 + 层 2 视觉对齐追加（菜单几何/字体/背景 + 对局态背景/棋子 datax_177）+ 原版 UI 对齐纠正（datah_023/041 判误、datax_294 菜单背景、☰ 菜单 datah_006 8 项、直开局去难度弹窗、菜单/对局背景分离、明棋/暗棋诚实差异）+ 对抗审计 2 阻塞项修复（☰ 图标 UI_GAME 守卫、悔棋连续 pop 到 BLUE）+ 用户指正"UI 选项/棋子间隙"修复（☰ 菜单实测几何+真实图标位图裁剪、棋子 0.59×0.82 间隙比例对齐原版）+ 主菜单按钮位图/☰↩ 真实图标对齐（orig_menu/orig_human 裁剪，so b6301851）+ 用户指正"重新开始/悔棋按钮小问题"修复（☰ 菜单文字 160→134 对齐原版 157×37，so af0e8ad4）+ 用户指正"顶部栏黑边"→全面屏（App.smali FLAG_FULLSCREEN+IMMERSIVE_STICKY，view 900×1600，walk_coords/菜单 tap 校准更新）+ 用户指正"各个按键功能对齐"（☰ 菜单 2-6 电脑变着/游戏规则/功能设置/战况统计/软件信息 + 主菜单 2/3 联网联机蓝牙边界，App.smali showInfo AlertDialog，so a8412714）+ 阶段 2 双 ABI 交付物构建（phase2.apk，dec_dual = src_rs Java + baidu 334 类 + 原版 armeabi-v7a so + src_rs arm64 so，minSdk 21，STORE+双签）+ 暗棋开局对齐原版（g_opt_mode 默认 1 + jq.mode 测试钩子，so 57acb0b7）+ 四阶段交付物全量补齐（phase0-3，dist/，phase0/1 从零构建 + phase2/3 重建）+ 音效开关持久化（sfx_sound_load/save + g_files_dir，so 044964b1）+ 真机验收（MuMu 恢复：phase2/3 装+跑+暗棋默认翻牌+音效持久化+☰ 菜单全过；phase0/1 32 位 so 硬依赖广告 jar→保留 jar+删权限，houdini 转译 flaky 诚实边界）+ 过程改进批处理（build_phases.sh 一键构建+校验四阶段，--verify-only）+ 四国诚实边界修复（action=4 静默开 2 人局→边界对话框，so e5bc4fe4）+ redroid 全量事件回归 8 项 PASS（确认 MuMu fulltest 失败=环境非回归，redroid 只跑 setprop 不跑触摸 walktest 分辨率不匹配）+ 蓝牙联机 SPP socket 全链接线（BtSpp.java host/client/sendFrame + App.smali btState/btSend/btHost/btClient + 原生 jq_bt_start/jq_bt_send + 发送侧 jq_bt_session_send_move 接线，so 2aad85e1；redroid 实证 host 监听/发送 v2 帧优雅未连接/8 项回归 PASS，无 BT 硬件端到端诚实边界）+ 蓝牙对局自动开局 + we_blue 修复（btState host_connected→runOnUiThread→App$BtStart→OnEvent menu 1 两人自动开局，host=蓝/client=红，so f688671c；btconn 钩子实证暗棋棋盘渲染，JNI 陷阱固化：native 线程 FindClass ClassNotFound/runOnUiThread 主线程同步重入挂死）+ 蓝牙回合闸口修复（client=THEIR_TURN 等 host 首帧 + 发送守卫 OUR_TURN，host 蓝 OUR_TURN→发→THEIR_TURN→收→交替，so b223dc80；btconn 回归 PASS）+ 批量推进 A（BT 悔棋/重开帧 jq_bt_ctrl_send UNDO/RESTART + bttest setprop 钩子 jq_bt_test_run 提取复用 + SYNC 诚实评估不接线；so 0c2d3add，坑固化：OnCreate 已持 g_mutex 非递归死锁/bttest 前向声明，bttest 钩子实测属性清 0）+ UI 纹理根因修复（spak_res PNG 未推设备→全退化，rebuild_rs_apk.sh/mumu_swap_so.sh 单文件循环推全量，不 rm PNG）+ 按钮业务对齐（g_vs_ai 人机/两人区分：人机=AI走红/两人热座/蓝牙AI不介入，三处 AI 块守卫；jq_can_move/jq_can_flip 回合校验；☰ 悔棋分模式；consistency 基准合法交替；so b7f057b6，真机人机蓝翻→AI应着/两人蓝翻→无AI红玩家翻，8/8 桌面单测）+ 悔棋计数 bug 修复（`popped=1`→`popped++`，人机悔棋 popped=2 AI步+玩家步/两人悔棋 popped=1 悔红步，真机实证，so 098c4062，undo_push/pop 诊断日志）+ 对抗审计 4 强证据缺口修复（SPAK 解包最强证据链：datah_041 人机难度 8 按钮弹窗 UI_DIFFICULTY、datah_072 联机方式 4 按钮弹窗 UI_WAYSEL、datah_067 联机 ☰ 8 项分流 ui_bt_set_active+BT_T_DRAW/RESIGN/CHAT、datah_001 声音设置 5 按钮弹窗 UI_SETTINGS+jq_settings.txt 持久化；真机实证全过；so b79c9720）+ btconn 源 APK 根因修复（rs168_wav.apk 被从 dec_rs 重建丢失 bt 接线→从 dec_dual 重建，btconn 回归 9/9 PASS）+ P1 登录弹窗 datah_038 对齐（UI_LOGIN 6 控件 用户名/密码占位标签+登录/注册/游客/返回，原版 432×285 居中，Δ=0；datah_072 方式弹窗"个人资料"入口改造→UI_LOGIN + setprop jq.login 钩子 OnCreate/OnUpdate 双路；诚实边界：账号系统/登录注册服务器协议未逆向，游客=本地单机唯一真生效路径，输入框简化为占位标签；登录/注册点击→诚实边界对话框+回主菜单，游客→g_vs_ai=0+两人热座开局；evverify run_login 桌面 12/12 PASS（含此项），真机 rs_draw_login 出帧+6 控件截图非黑 92-94%，so 5908d27d；AUDIT §十八 18.1-18.5 全文，make_login_atlas.py login_text_atlas.png 8 标签烘焙） |
| `rev64/docs_rs/AUDIT_CHECKLIST.md` | 对抗审计 checklist（代码 vs 规格/规范/防复发，下轮 subagent 套用） | ✅ 成文（本轮固化） |
| `rev64/docs_rs/PIPELINE.md` | **逆向→正向→验收→补差异 四段流水线 + 三回路 + 5 固化原则**（段 1 提取 / 段 2 src_rs 回归 / 段 3 真机判据门 / 段 4 unidbg-frida 对象图） | ✅ 成文（本轮） |
| `rev64/docs_rs/R-route_reversal.md` | 逆向证据链（核心函数/控制流/常量/布局） | ✅ 已有 |
| `rev64/src_rs/CONSISTENCY.md` | 一致测试口径（行为级重写判据） | ✅ 已有 |

### 32 位逆向路线（R6 / 翻译体 / unidbg / frida / MuMu）

| 文档 | 内容 | 状态 |
|---|---|---|
| `rev64/phases/REVERSE_NOTES.md` | 主逆向笔记（§1-§121） | ✅ 已有 |
| `rev64/docs_rs/REVERSE_LOG.md` | 32 位逆向经验库（红线/终局反思/量化/§124y-10~31 触摸+游戏状态构造+dispatch 全链+gs[8] vtable 定位+探查工具+工具对抗审计+游戏方法槽完整翻译体+事件发送器收表+会话 B 规则引擎走子桥+32 位原生布局加载链+原生事件发送器 F_3ab80 补全+真实事件对象构造（F_3a92c 分发字段逆向）+三条走子路径回归脚本（arm32_walkregress.sh）+全局单例事件发送逆向+事件名写入模式逆向（F_3432c 深层事件底层）+§124y-26 端到端"游戏可玩"判据（jq.game_test 钩子：reset+flip×2+触摸注入+move×2=Junqi.Move+终局，4 路径回归 4/4 PASS+0 崩溃；命中几何 32 位 libc 设备端 0/60 诚实边界）+§124y-27 F_3a92c disc 非零闭环（真实事件表匹配器，MuMu 实锤 disc=0x00000001）+§124y-28 F_55d40 事件业务 thunk 收表 + F_3ab64 ok 逻辑修正（成功路径真返回 1；73 handler 深水区边界）+§124y-29 F_55d40 事件业务 thunk 区 code→handler 分发实锤 + 21 handler 收表（0x55d40 区 73 thunk 编译期固定映射非 .bss 表，code4→F_8571c/code16→F_55ba4，13 未收表 handler 完整翻译体 + 4 auto-stub 去重坑，F_8571c/F_85704 vtable 偏移 getter + .bss 未构造返回 2 兜底，MuMu 实锤 F_3a92c disc 非零→F_55d40 code=4→F_8571c ret=2→F_3ab64 ok=1 全链 + arm32_walkregress native_event 判据=F_3ab64 ok=1 4/4 PASS）+§124y-30 F_55d40 全量 42 code 映射 + 3 新 handler（codes 34-41）收表（F_3deb0/F_853c0/F_3f8f4，codes 0-41 全量 switch，MuMu ok=1 + 4/4 PASS）+§124y-31 F_55d40 switch codes 53-65 补全（13 case 复用已收表 handler，24 handler 全有定义体；pack_arm32 link_audit 门禁 FAIL 静默中止→设备跑旧 so 坑固化；4 auto-stub 忠实语义=返回 r0 非 2）+§124y-32 .bss 单例构造者定位（F_3ab64 single 槽精确纠正 0x15bfe8→0x15bfec .bss 偏移 0x168→0x16c；12 个 init_array 构造器逐个反汇编无一直接写 0x15bfec 区；.bss 区 5 条 RELATIVE 槽全在开头 0x15be00-0x15be2c 指向 .rodata vtable；.bss 文件偏移误读澄清=读到 .comment 非 .bss 数据；运行时 dump 闭环：翻译体 so jq.bss_dump 钩子 + jqt_init_empty_containers 还原 init[0] 填 .bss 0x15c096 5 元组 + MuMu 实锤 REL 槽重定位/0x15bfec=0 惰性单例/F_3ab64 ok=1 主链 + 4 路径回归 PASS；0x15bfec 真实单例指针需 unidbg/frida 追踪，非阻塞）+§124y-34~71 事件底层 C++ 对象生命周期全链（F_3ab80 6 步链+创建工厂 F_5a104+24 handler 忠实语义+事件表 0x15af08 真实结构 3 事件组+.bss 段 0x15be80-0x162e88 完整布局+构造 .init_array 12↔析构 __aeabi_atexit 12 对偶+字符串表 .got/.rodata 三套常量区区分+74 事件名 72 Junqi.*/2 Misc.*（修正 §124y-59 的 73 条）+REL 重定位 5312 槽段分布（.data.rel.ro 4800 vtable/.got 458/.data 42/.bss 0）+.got 681 槽+PLT 214 分类（GLES2 35/socket 12/pthread 17，修正 AGENTS.md GL1 认知=实锤 GL2）+GLOB_DAT 6+去广告链 com.cnvcs.junqi.removead SP key→JNI getString→Misc.RemoveAdOk/Fail+.rodata 1801 字符串 8 功能面+JNI 14 入口（修正 AGENTS.md 15 个）读 .bss 0x15be80 根对象槽+MuMu .bss 运行时 dump 16 槽实锤 87% 场景对象图+bss_dump_align.py 对照基准） | ✅ 成文（§124y-34~101 全量闭环，本轮固化） |

### 流水线 orchestrator（一条命令跑通"逆向→正向→验收→补差异"全链）

| 脚本 | 作用 | 用法 |
|---|---|---|
| `rev64/tools/pipeline_run.sh` | 四段流水线 + 回路 C 口径，**段 3/4 真机部分只报 PENDING 不误判 PASS** | `bash rev64/tools/pipeline_run.sh --all`（本机段 PASS，真机 PENDING） |
| `rev64/tools/rs_cycle.sh` | **src_rs 全周期一条龙**（make so + 桌面 12/12 回归 + rebuild_rs_apk 装机+启动 + `--device-regress` 设备事件回归含 btconn） | `bash rev64/tools/rs_cycle.sh --dev emulator-5554` / `--device-regress` / `--no-regress` |
| `rev64/tools/build_phases.sh` | **四阶段交付物一键构建+校验**（apktool b→STORE→zipalign→双签→dist/phaseX.apk→verify，幂等，资产全复用） | `bash rev64/tools/build_phases.sh all` / `phase2` / `--verify-only` |
| `rev64/tools/bt_smali.sh` | **蓝牙 BtSpp.java→dex→smali→合并构建树**（javac+android-32+stub App→d8→baksmali→dec_dual/dec_full，幂等，--verify；**含 App.smali/BtStart 双树同步**） | `bash rev64/tools/bt_smali.sh` / `--verify` |
| `rev64/tools/mumu_regress.sh` | **事件回归一键跑**（flag/mine/restart/draw/resign/stalemate/close/pop/btconn，首跑失败自动重跑一次容错时序，汇总 PASS/FAIL，--only 子集） | `bash rev64/tools/mumu_regress.sh --dev emulator-5554` / `--only flag,btconn` |
| `rev64/tools/redroid_test.sh` | **redroid 测试钩子批量验证**（装机→清 prop→启→触发 jq.test_game=1→抓 GS8 诊断日志→判 miss/崩溃/存活，MuMu 宕机期间的确定性验证入口） | `bash rev64/tools/redroid_test.sh --no-install --wait 4` |
| `rev64/tools/so_probe.py` | **32 位 so 静态探查**（vtable 槽/函数反汇编/槽反汇编/dispatch 表查询/RELATIVE 重定位/字符串搜，把本轮高频手敲的 objdump+struct+grep 固化成 6 子命令） | `python3 rev64/tools/so_probe.py vtable 0x159c80` / `dispatch 0xad248` / `slot 0x159c80 0x28` |
| `rev64/tools/dispatch_insert.py` | **dispatch 表批量插入**（offs/fns/jqt_vtbl_n 三处同步 + 可选 stub 生成，幂等，fns/offs 逐行对齐） | `python3 rev64/tools/dispatch_insert.py 0xad248 0x78890 --gen-stub` |
| `rev64/tools/thunk_code_map.py` | **0x55d40 事件业务 thunk 区 code→handler 提取 + switch 生成**（§124y-31 固化：提取 42 code 全量映射 / 生成 F_55d40 switch case 体 / 查 24 handler 收表状态；坑固化：objdump 地址必须带 0x 前缀，否则 bad number 静默 0 提取） | `python3 rev64/tools/thunk_code_map.py` / `--gen-switch` / `--check` |
| `rev64/tools/probe_cycle.sh` | **"红果测试+对象图探查"一条龙**（redroid_test.sh 抓 gs8 → so_probe.py 查 vtable 各槽是否收进 dispatch 表 → 汇总，一条命令定位"哪个游戏方法槽还没收表"） | `bash rev64/tools/probe_cycle.sh --no-install --slots 0x10,0x28,0x48` |
| `rev64/tools/layout_align_check.py` | **布局/按钮对齐可复现判据**（几何 Δ=0 + 截图按钮区非黑比，防"非黑%糊弄"。主菜单基准=orig_menu 真机×0.8，弹窗基准=datah 450/250/230 系换算。机读 `--json` / `--baseline` 只几何） | `python3 rev64/tools/layout_align_check.py --screenshot /tmp/xxx.png` / `--baseline` |
| `rev64/docs_rs/PIPELINE.md` | 流水线设计文档（每段一条命令 + 一组判据 + 一个诚实边界 + 5 固化原则） | 读 |

### 顶层规范（保留在本文件）

- 仓库结构 / 已验证现状事实 / 核心规范（重打包三件套 / MuMu 内存 / 拿明文路线 / 64 位诚实边界）
- 四个阶段（阶段 0-3） / 红线（不要碰） / 用户约束
- **规范单一文档原则**：新增闭环事实/坑/规范 → 写对应单一文档（EXPERIENCE_LOG / SPEC_BUSINESS /
  GAME_DEV_GUIDELINE / CODE_STYLE / REVERSE_LOG），AGENTS.md 只更新路由表条目（不塞全文）。
