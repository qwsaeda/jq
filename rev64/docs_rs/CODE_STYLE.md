# 军棋 C 代码规范（src_rs 强制准则）

> 定位：src_rs C 源码的**强制编码规范**，从经验库 8 坑 + 坑清单沉淀成文。
> 每准则附"违反后果"（真机崩溃/链接失败/静默失败实证）。

## 1. 命名

- 前缀：游戏逻辑 `jq_` / 渲染 `rs_` / UI `ui_` / 蓝牙 `bt_` / 棋盘格几何 `bg_`（datax_177 背景棋盘格，render+input 共享：`bg_row_cy`/`bg_cell_img_center`，junqi.h）。
- 函数体完整 = 命名参数（`_r0` 类）；存根 = 匿名参数 `(uint32_t,uint32_t,...){return 0;}`（翻译体区分；src_rs 不用存根）。
- 静态函数（static）完整体；跨文件函数在 .h 声明。

## 2. 返回值

- 规则引擎返回 **int**（**禁用 enum 返回值**，符号性坑）。
- **0=成功契约显式 `==0`**（`if(fn())` 只用于布尔/真值函数）。
- `jq_try_move` 返回事件码/-1（非法）。

## 3. JNI 反射（8 坑头号来源）

- **GetMethodID/GetFieldID 后必须 `ExceptionCheck + ExceptionClear`**（NoSuchMethodError 累积 → abort）。
- **MediaPlayer 重载实测**：`setDataSource(String)` / `(FD,off,len)` 有，**无 File/(AFD,JJ) 重载**（勿猜）。
- **local ref 用完即删（DeleteLocalRef），删后不得再用**（悬空 → SIGSEGV 0x70）。
- **类/对象 ref 重新获取**（act_cls DeleteLocalRef 后再 GetMethodID → 必须拿 act_cls2）。
- **每个 GetMethodID 后 ExceptionClear**（jni_method_audit.py 自动化扫 8 行内有无清理）。

## 4. 链接安全（so_link_check.py / link_audit.py）

- **内部符号不得悬空**（`F_xxx` 无函数体 → NOTYPE UND → MuMu fail to link）。
- **每次重建 so 后必跑 UND 审计**（`readelf -sW so | awk '$7=="UND"'` vs 基线 diff）。
- **GL 符号直接链接**（-lEGL -lGLESv1_CM -lGLESv2 -landroid），**勿 dlsym 乱改**（引入新 UND）。
- **AAsset 符号 dlsym 运行时解析**（转译层不导出 AAssetManager_fromJava 等 → 直接 UND 会 fail to link）。

## 5. 内存/并发

- 全局状态（g_state/g_undo/g_ts）读写必须 g_mutex（**勿裸读写**，野指针坑）。
- 静态完整体避免大全局（g_undo[UNDO_MAX] 完整快照栈，UNDO_MAX 定界）。
- 循环防护：递归深度阀/调用预算/越界内存阀（翻译体；src_rs 用规则引擎自检）。

## 6. 错误处理

- 日志：`LOGI/LOGE`（android/log.h），走子/事件/音效关键路径必日志（真机诊断）。
- 失败路径：返回值契约 + LOGI（勿静默失败）。
- 音效降级：g_mp 未建 → sfx 降级（日志"实例未建"，勿崩）。

## 7. 编译

- Makefile desk（桌面 headless，-DRS_NO_GL）+ so（NDK arm64，-lEGL -lGLESv1_CM -lGLESv2 -landroid）。
- `make evverify/walkverify/laytest/touchtest/gametest/bttest` 各 target 判据 grep 同步（N25）。
- 回归 12/12 + 真机 9/9 全过才交付。

## 8. 防复发（坑清单）

1. 返回 enum → int（jq_try_move）。
2. 0=成功契约显式 `==0`（ai_pick 3 调用点）。
3. GetMethodID 后 ExceptionClear（jni_method_audit.py）。
4. local ref 悬空（DeleteLocalRef 后不得再用）。
5. 内部符号悬空（so_link_check.py UND 审计）。
6. 锁竞争（g_mutex 统一；AAsset 走文件系统优先）。
7. 触摸 ACTION_DOWN 过滤。
8. 音效/重开/悔棋统一入口（sfx_for_move/sfx_game_over/jq_restart_game/undo 连续 pop）。
9. rebuild 前先 make so（rs_cycle 一体）。

## 9. 脚本/门禁防假绿（§2026-11-08 AUD-1000 三型假绿归纳）

> 本轮 8 域对抗审计最大主题 = 假绿（校验失败却报 PASS / 产物坏却继续）。三型 + 判据：

1. **管道吞码型**（最高发）：`cmd 2>&1 | tail -1` 后 `[ $? -ne 0 ]` 查的是 tail 退出码。
   **判据/修法**：脚本头 `set -o pipefail`（或显式 `PIPESTATUS[0]`）；"校验+继续"步骤
   必须断言被校验命令的退出码，展示分支（echo 异常）必须 `return 1`。
   实证坑：rebuild_rs_apk.sh apksigner（签名失败假绿装机全挂）、pack_arm32.sh apktool b、mumu_bt_test.sh 包名错配。
2. **判据恒真型**：grep 字符串含目标词的否定式也命中（`grep -q "Verified"` 命中 "Verified: false"）；
   判据函数恒 `exit 0`（结果只打印）。**判据/修法**：grep 锚定值（`= true`/正则全串），
   判据函数 FAIL 分支 `exit 1`。实证坑：sign_apk.sh verify、popup_sweep.sh 恒 exit 0、
   build_phases.sh verify-only "so ≠ 基准"展示分支 RC=0。
3. **静默降级型**：门禁依赖的缓存/基线文件缺失时退化放行（或反向误 FAIL）。
   **判据/修法**：工具缓存缺失时退出码必须区分"缓存缺失"（=1）与"审计 PASS"（=0）/
   "审计 FAIL"（=2），调用方对每种码分别处理，缺失禁止映射到 PASS。
   实证坑：so_link_check.py 缓存缺失 return 1 被调用方只判 `-eq 2` → 门禁全失效；link_audit.py proven json 缺失双向误判。

## 10. 忠实 ≠ 已实现（判功能看消费点返回值，不看崩溃计数）

- 32 位翻译体 .bss 全 0 是有意忠实兜底（§124y-151/160），但兜底使成功路径被跳过
  （如 F_3a92c disc 恒 0 → F_3ab64/F_55d40 不执行 = 事件业务分发死代码）。
- **判据**：回归 PASS + 0 崩溃 ≠ 功能通；必须 grep 消费点日志（如 F_3ab64 ok=1、
  DRAWOBJ-PCE 顶点写成功行）确认成功路径真的走到；"兜底没崩"只证明忠实，不证明实现。

## 11. 回归判据防 flaky（固定 sleep → 轮询）

- 固定 `sleep N` 后抓日志 = flaky 头号来源（慢机未就绪假 FAIL / 快机已过去假 PASS）。
- **判据/修法**：`wait_log <grep 锚> [timeout]` 轮询（仓内 mumu_evtest.sh 已有 helper，
  跨脚本复用时拷 helper 非跨源）；判据锚用**语义串**（函数名+结果词）非行号，
  锚文案改动即全路径假红（arm32_walkregress 4 处中文全串锚 = 文案一改全灭）。
- 设备/包名硬编码（emulator-5554 / com.cnvcs.junqi32 / junqirs / junqi 四包名）全部参数化，
  默认值必须与头注释一致（mumu_regress.sh 默认 redroid 注释写 MuMu = 裸跑打错设备）。
  ★ redroid 已禁用（2026-11-09 容器/镜像已删）：emulator-5554 不再指向 redroid，
  默认设备一律参数化为红果 10.0.182.146:5555（出图实证权威口径），勿硬编码旧 redroid 地址。

## 12. 多线程 GL/static 并发

- static 计数器（限流 `_sd/_gd/_e0` 类）被 GLThread + frame-loop 双线程并发调（F_9d43c/F_9e760）
  = 数据竞争；g_cur_fn volatile 非原子（1575 处跨线程写读）。
- jqt_mem_rebuild "清零再填充" 非原子，rebuild 瞬间真映射区表半空 → 指针解析错区。
  **判据/修法**：rebuild 用双缓冲（新表建成原子换指针）；跨线程计数器改原子（或接受非精确限流注释标注）。
- EGL 多 context 必须 `eglCreateContext(..., share_ctx, ...)` 共享（NO_CONTEXT → 纹理/program
  跨 context 无效，texID 泄漏 + 采样黑 = 棋子不出屏根因）。

## 13. 全量弹窗 UI 对齐审计 + 交付物 so 时效门禁（§2026-11-11 批次）

背景：用户指出"功能设置没对齐 UI，类似问题很多"。全量审计（`audit/ui_align_audit.py`，
单一真相源 `spak2ui.panel_origin`）扫 ui_menu.h 163 个弹窗宏组 × datah 原版 720 基准逐按钮
配对（贪心 + 备用隐藏按钮 Ok/Ok2 语义），分三类台账 `audit/r2/UI_ALIGN_PROB.json`：
需修（真误配/硬错位）/ 诚实边界占位（不修已标注）/ 非 datah 实测基准（不修）。

★ 修根因（收口）：
- UI_UI44：datah_044 是 Dialog 居中（panel_origin 270,552.5），旧误读为 BackDlg 固定 45,620
  → 面板/按钮 225px 全错位（与 UI_D044 宏组 1:1 对齐）。
- UI_IMGSET7：datah_008 面板 450×390 居中 origin=(135,445)，旧宏误用 datah_012 的 407.5/465
  → 面板 H 75px + 按钮 37px 串号；Y0/H 已纠正，按钮按局部 rect 1:1 平移。
- UI_D037/UI_D012/NG061 等：按 datah 原版真值逐表对齐（RULES 450→260 面板真值 / NG061 450×465 /
  D012 450×465 origin 407.5），evverify 期望 + ui_menu_stub 三处同步。

★ 门禁（防"旧 so 打进交付物"假绿，本轮实锤 2 次）：
- 构建类脚本（build_phases）在阶段 2/3 构建前**强制 `make so` 重编**（`make_so_rs`），
  否则 src_rs 源码改了但 so 没重编 → 交付物/APK 跑旧 so → 真机"钩子未命中/文案旧"全是假象。
  判据：APK 内 `lib/arm64-v8a/libjunqi.so` md5 必须 == `src_rs/dist/libjunqi_rs.so` md5。
- 验证脚本（verify_dialogs_device.py）的 APK 重装路径必须指向**仓库根 dist/**（build_phases 产物
  目录），旧脚本误用 rev64/dist/（不存在）→ 跳过重装 → 设备跑旧 so → "钩子未命中"假 FAIL。
  两处候选路径（根 dist/ + rev64/dist/）都找，谁存在装谁。
- 探针 `artifact`（so 比源码旧）必须用**精确 os.path.getmtime**，`ui_menu_stub.c` 等桌面桩
  不参与 so 链接 → 进 EXCLUDE（改 stub 不触发假红）。

★ 探针反噪（A-ai / B-doc-num / array-index / geom-delta）：
- A-ai：用 `_in_block_comment` 状态机判行是否在 /*...*/ 块内（续行不以 * 开头也判注释），
  跳过 rules.c 的 AUD-R-057 守卫注释续行。
- B-doc-num：历史快照/闭环叙述词（"历史快照/当时/已固化/本轮/loop/五代/→/新增 --all"）加入
  跳过表，只报**无历史限定**的现状数字漂移。
- array-index：函数体任意位置的 `idx >= a && idx < b` 守卫 / 编译期宏边界循环变量识别。
- 判定不可信时先人工复核（本轮 17 条 array-index 逐条核：多数已有守卫或是测试代码，
  非真越界）。

## 14. 锁纪律 / 死锁 / 栈守卫探针反噪范式（§2026-11-11 批次）

- **真块注释状态机**：doc 注释里的持锁声明（"调用方需已持有 g_mutex"）常跨多行且续行
  不以 `*` 开头 → 逐行 `startswith` 判会清空。改**单次 O(n) 前扫**建 `in_block[i]`
  布尔表（/* 开、*/ 关，行内多次开闭也处理），再收集块注释内含关键词的行。
- **函数定义收紧**：`^static [type] name(` 正则会把**函数调用行**（jq_xxx(...)、
  if (...)）误判为定义 → g_state 写入向上找到"最近伪函数"遮蔽真定义。收紧为
  行尾带 `{`/独立行 + 排除控制流/宏/调用行前缀。
- **真写判据**：`g_state.x == y ? a : b`（ternary 右值）/ `if (g_state.x==y||...)`（条件）
  是**读**不是写。只认 `g_state.<field> [^=] =`（行内真实赋值）才算写。
- **锁窗口双向**：`pthread_mutex_lock(&g_mutex)` 常写在 g_state 写语句**前一行**
  （先 lock 后写范式），旧版只往后看 80 行漏掉 → 往前 80 行 + 往后 80 行都看。
- **注释内 lock 字面量干扰 deadlock 检测**：说明注释里写 `pthread_mutex_lock(&g_mutex)`
  会被线性 depth 扫描算进 → ① 注释里避免写完整 lock 字面量（改文字描述）；
  ② 探针按行尾 `//` 截断取代码部分再判。
- **栈守卫 `>=` 形态**：undo_push 用 `g_undo_top >= UNDO_MAX` 拒写（非 `< UNDO_MAX`），
  探针任一形态命中即视为有上界检查。
- **真嵌套 vs 独立配对**：文件线性 depth 会把**跨函数**的独立 lock/unlock 误算嵌套；
  真死锁判据 = 同一函数体内未 unlock 就再 lock（认输分支 B03-009 双层 lock 是实锤，已修）。
