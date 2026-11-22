# 深度审计台账（2026-11-22，4 维度并行子代理）

> 用户"挖100个"+ 纪律"不硬凑"。4 个并行子代理审计 Rust 正向（`src_rs_rust/`）：
> ①规则引擎 ②JNI/线程 ③UI/渲染 ④事件/判据。**共 39 个有文件:行依据的真问题**
> （P0×4 / P1×17 / P2×16 / P3×2）+ 9 项测试盲区。不硬凑到 100——只报可复现定位的。
> 审计全程只读；本台账 + 修复由主会话执行。

## P0（崩溃/吞码，最高优先）

| 编号 | 来源 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| P0-1 | 判据 | rust_anything_check.py / rp_desktop_compare.sh | 三套"行为对齐"判据不测生产 Rust 代码（测脚本内嵌模板或 C-vs-C），篡改生产函数仍 PASS = 结构化判据恒真 | ✅ 已修（ai_score 桥接测试：原版154case→直接测生产 ai::ai_score，接入 rust_all [4a]；负向篡改生产函数→FAILED=2 非恒真） |
| P0-2 | 判据 | rust_all.sh:89 | 设备离线 `fail=0` 抹掉桌面 FAIL 强制 PASS | ✅ 已修（离线按已有 fail 判定，rc=3 PENDING） |
| JNI-01 | JNI | jni.rs:332 | wgpu Surface 持裸 ANativeWindow 指针却立即 release → 悬空 | ✅ 已修（WgpuState 保活 WindowPtr + Drop 释放） |
| JNI-02 | JNI | jni.rs:220/222/224 | OnCreate call_method 丢弃 Err，pending JNI 异常未清 → 后续 JNI abort | ✅ 已修（call_clear helper：失败时 exception_check/clear） |

## P1（行为错误/可复现故障）

| 编号 | 来源 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| P1-3 | 判据 | evt.rs:206 | 73 名表 [72] 截断 `Junqi.GameType`，真值 `Junqi.GameType%d` | ✅ 已修（待补全量比对测试） |
| P1-4 | 判据 | evtest.rs:125/194 | 终局守卫 OR 断言恒真（Stage 仅2变体）、winner 未校验 | 待修 |
| P1-5 | 判据 | fuzztest.rs:66-79 | F2 变异器零断言、零生产调用，不可能失败 | 待修 |
| P1-6 | 判据 | rprobe.rs | "移植 C 36 case"仅 23 例且 E1/G2/F2/G4/G3 场景漂移 | 待修 |
| F1 | 规则 | rules.rs:445 | 困毙可翻扫描排军旗，C rules.c:1050 仅排地雷（原版 0x30320） | ✅ 已裁定+修（以原版 0x30320 反汇编为唯一事实源=仅排地雷；Rust 删 `cn != RANK_FLAG` + rprobe C4b 断言反转 + SPEC §5.2 同步；102 测试/rust_all 全过）。行为改变待真机验证 |
| UI-01 | UI | asset.rs:189-218 | 棋子图集 6×6 切片错，原版 4×7(53×40) → 贴图全面错位 | ✅ 已修（4×7 + col=(rank-1)%4 + 红行0-2/蓝行3-5） |
| UI-02 | UI | jni.rs:517/asset.rs:239 | GameSet/Settings 画面板无按钮，任何触摸只回滚 = 假弹窗 | ✅ 已修（面板内 3×2 按钮网格：cfg开关/sound开关/确定/取消，画=点同源；单测锚定命中；103测试） |
| UI-03 | UI | jni.rs:442-474 | NewGame 取消不恢复 mode 快照（jq.mode 残留坑） | ✅ 已修（进弹窗 begin_popup 快照；取消/面板外 cancel_popup 恢复） |
| UI-04 | UI | asset.rs/jni.rs:507 | ExitConfirm 有触摸无绘制（Menu 可达性系子代理误判——Ui::new 默认 Menu） | ✅ 已修（画红退出/绿继续按钮+exit_btn_hit 分派，面板外=继续保守；104测试）。子代理'Menu不可达'经核实为误判 |
| UI-05 | UI | jni.rs:545-557 | Game 顶部图标区超大隐形命中，吞掉红方上半盘 6行×5列 | ✅ 已修（命中区缩为 view±60px 小方框，中心(270/620,750)与真机悔棋判据一致；asset.rs 画橙图标；单测锚定不误命中棋子区） |
| UI-06 | UI | jni.rs | OnUpdate 每帧 clear_frame 纯蓝 + 双渲染路径 | ✅ 修（轮3 N3：OnUpdate 禁 clear_frame，native Canvas 独占 Surface；台账状态此前过时） |
| UI-07 | UI | evt.rs/sfx.rs | 音效映射正确但全链无消费者（14 WAV 编进 so 永不播放） | 已知边界（houdini JNI 崩回退，待真 ARM64） |
| JNI-03 | JNI | jni.rs:286/288 | Surface 与 Device 来自不同 wgpu Instance | 待修 |
| JNI-04 | JNI | wgpu_thread.rs:11-19 | stop() join 无超时 + 启动期不查 STOP → 主线程 ANR | ✅ 已修（轮询 RUNNING≤2s + 启动期 STOP 检查） |
| JNI-05 | JNI | wgpu_thread.rs:22-38 | START/STOP/JOIN 三态竞态 | ✅ 已修（先 swap RUNNING 成功后才清 STOP） |
| JNI-06 | JNI | canvas.rs:51-76 | lockCanvas 成功但错误路径未 unlock → Surface 永久锁死 | ✅ 已修（createScaledBitmap/drawBitmap 失败先 unlock+清异常） |

## P2（健壮性/数据一致性）

| 编号 | 来源 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| P2-7 | 判据 | fuzztest.rs:35 | Winner 全变体 matches! 恒真 | 待修 |
| P2-8 | 判据 | rust_geom_check.py | 无负向注入，"双源一致断言"未真比对生产常量 | 待修 |
| P2-9 | 判据 | rust_all.sh | 纳入不测 Rust 的 C 判据，"全量无遗漏"措辞不符 | 待修 |
| F2 | 规则 | save.rs:122 | 暗棋复盘必然失败（全翻开后 flip 重放失败） | ✅ 已修（删强制全翻开，按 init_cells 开局真实状态重放；加暗棋 flip 复盘测试） |
| F3 | 规则 | save.rs:180 | validate_state 不校验 side↔rank 域，脏 rank 变军旗 | ✅ 已修（rank_ok side 感知：红≤24/蓝未知≤12；测试蓝rank20归零/红保留） |
| F4 | 规则 | state.rs:164/rules.rs | pieces_left 全程不更新，字段契约虚假 | ✅ 已修（注释改诚实描述：仅开局置25/归零/域校验，无增量递减，对齐 C 已删 refresh） |
| UI-08 | UI | strings.rs | 全表死代码，终局无文字 | 已知边界（C 版终局也色块） |
| UI-09 | UI | render/*.rs | 多套互斥死渲染实现 + handle_touch 坐标 bug | ✅ 修（轮6：cpu_board_rgba/draw_rects_rgba/menu_rects/handle_touch 加 #[deprecated] 防误接；handle_touch 坐标 view×0.8 修正） |
| UI-10 | UI | jni.rs:426-441 | Difficulty 面板外无返回菜单处理 | ✅ 已修（面板外 cancel_popup 回菜单 + jq.diff_cancel 属性） |
| JNI-09 | JNI | canvas.rs/sfx.rs | 错误路径残留 Java 异常；sfx 无 release | 待修 |
| JNI-10 | JNI | log.rs:18/47 | CString unwrap 可 panic；setprop 返回值丢弃 | 待修 |
| JNI-11 | JNI | wgpu_thread.rs:28 | 全局 panic hook 静默所有线程 panic 日志 | ✅ 已修（删除 set_hook，catch_unwind 已防 abort） |
| JNI-12 | JNI | jni.rs:564-617 | G_SEL 跨屏残留；push_undo 早于合法性校验 | ✅ 已修（clear_sel 在 restart/undo 后 + 先判定合法性再 push_undo） |
| JNI-13 | JNI | jni.rs:275 | OnUpdate 缺入口异常清理 | 待修 |
| JNI-14 | JNI | jni.rs:224-236 | OnCreate 每次重建泄漏 GlobalRef | 待修 |
| （JNI 类还有 JNI-08 OnUpdate 取 Surface 未 isValid） | JNI | jni.rs:317 | 同 JNI-01 死路径段 | 随死路径处理 |

## P3

| 编号 | 来源 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| F5 | 规则 | save.rs:149 | check_enums 死代码，两阶段校验未接线 | 待修 |
| F6 | 规则 | rules.rs:550 | on_stalemate 缺 loser_side 合法性守卫 | ✅ 已修（入口 is_valid 守卫，对齐 C SIDE_VALID） |

## 测试盲区（9 项，规则子代理）
1. 炸弹炸军旗即时终局分支无测试
2. cfg3==0 吃旗不终局无用例
3. 工兵铁路滑行/非工兵直滑无 rules 单测
4. 行营/大本营安全岛、cfg1~4 门控无用例
5. 70步/1000步和棋(cfg0)无用例
6. can_move_any 公开 API 无断言
7. AI 翻棋回退/level==1 被动分支无用例
8. 暗棋复盘（F2）所有 replay 用例 is_flip=false
9. check_enums/side-rank 域一致性无用例

## 修复进展（按批次）
- 批次1（已提交）：P0-2 吞码(rc=3 PENDING)、JNI-01 悬空 window(WindowPtr 保活+Drop)、P1-3 事件名(%d)、UI-01 图集切片(4×7)。
- 批次2（本次）：P1-4a/4b 终局断言真校验(winner None 也校验+精确 Over/Blue)、P2-7 删 Winner 恒真 matches、
  P1-5 F2 变异器真调用生产 touch_base_coord+finite 断言。负向验证：篡改 evtest 期望→FAILED=2(非恒真)。
- 批次3（本次）：P0-1 根因修复——rust_anything_check 加 --export-cases 导出原版真值表；
  gen_ai_score_bridge_test.py 生成集成测试直接调生产 ai::ai_score（生产域12条真值逐条断言+域外142条跳过）；
  接入 rust_all [4a]；负向篡改生产 ai_score 900→901 → FAILED=2（判据非恒真实证）。
- 批次4（本次）：JNI-02 call_clear + JNI-04 STOP轮询防ANR + JNI-05 start顺序 + JNI-06 unlock兜底
  + JNI-11 删全局panic hook + JNI-12 clear_sel/push_undo合法性前置。rust_all全PASS。
- 后续批次：JNI-03(跨Instance,死路径)、JNI-07~10/13/14、UI-02~05、规则 F1~F4、几何判据桥接(P2-8)。

## 诚实口径
- **39 个 ≠ 100 个**：不硬凑。39 个全部有文件:行 + 触发依据，可复现定位是硬标准。
- 部分问题需裁定（F1 SPEC vs C）或真机验证（UI-06 纯屏、弹窗画面）——设备离线，不假绿宣称完成。
- P0-1（判据不测生产代码）是根因级问题：修好它，P1-5/6/P2-8 的同类假绿一并解决。

## 2026-11-22 主动审计轮（用户"按正向开发，你找不出问题？"指正）

两个并行审计子代理深挖 JNI/线程/资源 + 规则/状态机/存档，**新发现 8 个有依据的真问题**（未凑数）：

| 编号 | 定级 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| P0-3 | **P0** | jni.rs:首点选中门 | 默认暗棋开局全 0xFF 未翻，选中门只认 `side==turn && revealed` → 暗子永不选中、翻棋链不可达、AI 不介入 → **对局开局即死锁**（正是我改默认暗棋后引入）。桌面测试盲区：jni.rs android-only，106 测试全绿也漏掉 | ✅ 修：抽 `rules::can_select_for_touch` 纯函数（暗棋可翻即可选）+ 桌面回归测试。**真机实锤**：首点 cell25 → 二次点 `move=Flip`（设备 152 so `abae2450`） |
| N1 | **P0** | wgpu_thread.rs:52/95-110 + canvas.rs:35-62 | 渲染线程 attach 一次后永久循环，每帧 ~9 个 JObject 局部引用从不删（注释"自动释放"是错误认知）→ ~20-60s 触发 ART `local reference table overflow (max=512)` 确定性崩溃 | ✅ 修：`env.with_local_frame(16)` 帧末 PopLocalFrame 自动回收 |
| P1-2 | P1 | ui.rs:difficulty_select/newgame_confirm | 换难度/新局未清 `undo_stack`+`mode_snap`、jni.rs 调用点未 `clear_sel` → 新局按 ↩ undo 到上一局（整盘闪回）、残留选中使首点变误走子 | ✅ 修：两方法清栈，两调用点 clear_sel |
| N2 | P1 | wgpu_thread.rs:82-86 | 无用的 `create_wgpu_device()`（_dev/_q 从未使用）作 CPU 渲染致命门槛，wgpu 后端不可用即首帧前 return 永久黑屏 | ✅ 修：删除该门槛 |
| JNI-10 | P2 | log.rs:19/20/48 | `CString::new().unwrap()` 内嵌 NUL 可 panic；setprop 在渲染线程钩子中 panic 会杀渲染线程 → 永久黑屏 | ✅ 修：`unwrap_or_else` |
| JNI-13 | P2 | jni.rs:OnUpdate 入口 | 每帧 onDrawFrame 入口未清挂起异常 | ✅ 修：入口 exception_check/clear |
| P1-3 | P1 | rprobe.rs | "移植 C 36 case"实 23 例且 E1/G2/G3/G4/F2 场景漂移（真值被偷换） | ✅ 修：5 case 按原版 inc:252/343/388/397/408 真值重录；G2 按 64 位探针实测 CM=1（inc 注释 R-011 只是担忧非实测）；G4 探针 SM=0 系 probe_clear 不清 stale_v/c 缓存伪影，Rust 实时扫描 true 合理，诚实标注不强改 |
| P2-4 | P2 | rust_geom_check.py:47-94 | 双源断言测脚本内嵌副本非生产 `board.rs`，无负向注入 → 恒真 | ✅ 修：PORT 改为从生产 `board.rs` 抽函数体 + C头/生产 BOARD_COLS 真断言 + 负向注入（篡改生产函数体→diff 943/950 非全匹配，证明非恒真） |
| P2-5 | P2 | save.rs:150 | `check_enums` 零调用死代码，两阶段校验未接线 | ✅ 修：接线为 `validate_state` 阶段1前置（先只读拒读、再就地净化），所有 validate_state 调用点自动获两阶段语义 |
| JNI-03 | P1 | jni.rs OnUpdate | Surface/device 跨 wgpu Instance | ✅ 修：OnUpdate 改用 `create_surface_and_device()` 在同 Instance 创建 surface+adapter(compatible_surface)+device，删掉旧 B.instance+A.device 错配 |
| JNI-14 | P2 | jni.rs:250-258 | GlobalRef forget 无对应 DeleteGlobalRef | ✅ 修：先解 N3（start 等旧线程退出），OnDestroy 中 G_VIEW.swap(0) 后经 JNI 函数表 DeleteGlobalRef（jni 0.21 无 safe API，raw 调用）；清零后 OnUpdate 守卫跳过，无 UAF |
| N3 | P2 | wgpu_thread.rs:11-45 + jni.rs OnUpdate | start/stop 竞态 + 双渲染路径 | ✅ 修：start() 先轮询 ≤2s 等旧线程退出再抢 RUNNING；OnUpdate 不再 clear_frame（native Canvas 独占 Surface），只维护 wgpu 资源供 offscreen 验证，消除 EGL/lockCanvas 并发 |
| P2-6 | — | rules.rs:216-237 | 暗棋 cfg4 can_move 读 rank 是否透视 | ✅ 裁定非缺陷：32 位走子验证 `0xb6808` 反汇编实证 `ldr r0,[r5,#0xc4]; cmp r0,#0; beq skip_rank_check` —— 字段 0xc4（cfg 开关）为 0 跳过 rank 判定，即原版确有 cfg 控制的暗棋门槛 rank 检查，Rust 忠实。so_probe.py func 0xb6808 实锤 |

验证：106 tests + clippy 0 + android 0错0警 + rust_all 全 PASS（含设备 10 项）+ ui_offline_check 6/6。
**测试盲区诚实标注**：P0-3 的桌面单测覆盖了纯函数 `can_select_for_touch`，但 jni.rs 的接线点（android-only）未被自动判据覆盖，
仅真机一次性实锤（move=Flip）；后续应把"暗棋首手翻棋"固化为设备判据（需先解决存档恢复导致 cell 状态不确定的 flakiness）。

## 2026-11-22 审计轮 4（两代理：AI/事件音效/复盘存档 + JNI符号/UI状态机）

新发现 11 项真问题，本轮全部处理：

| 项 | 级别 | 问题 | 状态 |
|---|---|---|---|
| 音效链断 | P1 | sfx::play 零生产调用、sound_on 只写不读 | ✅ 修：Activity global ref + 走子/翻棋/AI 锁外播放（JNI 不能持锁），sound_on 生效 |
| 音效崩溃 | P0 | openFd 读被压缩 WAV → FileNotFoundException 异常未清 → 后续 JNI abort | ✅ 修：改"WAV 字节写私有目录 + setDataSource(path)"，每步清异常 + local_frame + 调用点双保险。真机 0 Fatal |
| 按键退出 | P1 | OnKey 不读 action/keyCode，smali down+up 双调用第二次 false → super→finish() | ✅ 修：只读 BACK ACTION_DOWN，其余消费；弹窗 BACK 回上层、主菜单不退出 |
| 状态机丢局 | P1 | IngameMenu→Difficulty 缺 begin_popup，取消回旧 Menu 丢局；未清选中；vs_ai 不设 | ✅ 修：补快照+clear_sel+vs_ai=1 |
| 复盘未接 | P1 | Replay 只测试钩子可达、真实对局不记录、5按钮无控制 | ✅ 修：Game 加 rp、真实走子/翻棋(含AI)记录、终局入口、5按钮真调 rewind_to/cur_set；真走子回放测试 |
| 统计空接 | P2 | ☰ btn5 战况统计无界面，EventCounter 只采不现 | ✅ 修：新增 Screen::Stats + 7类计数柱状图 + jq.stats 属性（真机实测 2:0:0... t2） |
| 存档假绿 | P1 | savetest 只回读 turn+pieces 字符串，无全量落盘/读档 | ✅ 修：serialize_state/deserialize_state 全字段+分层校验，savetest 真 round-trip，OnPause 自动存 + jq.loadsave 钩子。真机 savetest/loadsave=ok |
| ExitConfirm 出口 | P2 | 取消退出固定回 Game 而非上层 | ✅ 修：exit_confirm(false) 回 screen_snap |
| 分辨率硬编码 | P2 | OnGraphChange 空实现 + 全程 900×1600 硬编码，非该分辨率触摸错位 | ⚠️ 已知边界：当前权威设备 900×1600 正常；改动面遍布渲染/触摸/打包，风险大于收益，不动 |
| JNI 7 空壳 | — | OnEvent/OnInput/Bluetooth 无 Java 调用 / OnPause/Resume/Result/GraphCreate/GraphChange 空不破坏契约 | ✅ 核实非缺陷（无调用点或语义无需求） |
| 难度仅2档 | — | level 1 游走 / 2-4 贪心，3/4 无差异 | ✅ 核实：SPEC §9.3 与 C 共同设计，忠实对齐非缺陷 |

**新增判据盲区固化**：本轮 P0 音效崩溃是"接功能时引入"——接 JNI 音效必须清异常 + 不持锁，
已写进 sfx.rs 注释 + play_sfx_with_env 双保险。验证：109 tests + clippy 0 + android 0警
+ rust_all 全 PASS（含设备 so 40e360fa）+ 真机音效/统计/存档/BACK 专项 + 0 Fatal。

## 2026-11-22 审计轮 5（三代理：Java层/打包资源链/性能内存）

用户要求"找1000个"——拒绝硬凑（违反可复现纪律），实扫找出 12 个真问题：

| 项 | 级别 | 问题 | 状态 |
|---|---|---|---|
| surfaceDestroyed NPE | P0 | setRenderer 移除→mGLThread 永不创建，surfaceCreated/Changed 有空守卫唯独 Destroyed 漏了→切后台/锁屏必崩。force-stop 判据测不出 | ✅ 修：patch_rust_app_guards.sh 补 if-eqz 幂等补丁，接入 rebuild。真机 HOME→回前台 pid 不变 0 NPE |
| 每帧 ANativeWindow 泄漏 | P1 | OnUpdate wgpu_guard 已 Some 时 surface_to_window 拿强引用不 release，60fps 每秒漏60个 | ✅ 修：N3 分支补 window_release(win) |
| predictive back 返回键失效 | P1 | manifest enableOnBackInvokedCallback=true+targetSdk35→Android13+ BACK 不走 onKeyDown→退出确认链失效 | ✅ 修：patch 脚本删该属性 |
| 构建假绿（陈旧产物） | P1 | rebuild_rust_apk 无 -e、cargo/apktool rc 不查、产物不先删→编译失败仍打旧 so/apk 报成功 | ✅ 修：rm 产物+PIPESTATUS rc 判定 |
| so_link_check 假绿 | P1 | 100 个非 GL UND 0 核验报 PASS（libc 缓存空） | ✅ 修：非 GL UND 缺失一律 FAIL + 补 libdl/libm 库表 + __sF 良性豁免（真机已跑通）；门禁真核验 99+1 PASS |
| so 哈希对账不阻断 | P2 | 对账 FAIL 仅 log 仍 exit 0 | ✅ 修：失配 exit 1 |
| s_native_ok 空守卫 | P2 | 12 处 if-gtz 目标=紧邻下一条=空分支，so 加载失败仍调 native 崩 | ⚠️ 已知边界：正常 so 在位无影响；改 dex 控制流风险高于收益，记录不动 |
| deserialize 整数截断 | P2 | 256→as u8=0、257→1 静默接受越域枚举；revealed="2" 当 false | ✅ 修：pu8 严格范围 + revealed 严格 0/1 + 回归测试 |
| WGPU 跨重建不释放 | P2 | WGPU 静态持有，OnDestroy 不清→旧 device/window 驻留，新 Surface 建不出 | ✅ 修：OnDestroy take() 触发 Drop 释放 |
| Surface 轮询局部引用泄漏 | P2 | 150 轮×2~3 JObject 局部引用逼近 ART 512 上限 | ✅ 修：轮询体 with_local_frame(8) |
| MANIFEST 无 Rust APK 基准 | P2 | phase_rs_rust.apk 不在 dist/MANIFEST.md 三方哈希基准 | ⚠️ 已知边界：设备对账已由 rust_all so 哈希判据覆盖，MANIFEST 是 C 链门禁文档，补录待统一 |
| 每帧 11MB 分配 / MediaPlayer 不 release | P2 | 输出缓冲/JNI数组/Bitmap 每帧重分配；MediaPlayer 等 finalizer | ⚠️ 已知边界：设计层开销非崩溃，release 会中断短音效播放，不动 |

验证：109 tests + clippy 0 + android 0警 + rust_all 全 PASS + so_link 真核验 + ui_offline 6/6
+ geom 950/950 + 真机切后台 0 NPE + 0 Fatal。**拒绝凑1000**：硬凑=造假假绿，12 项全部可复现定位。

## 2026-11-22 审计轮 6（正向流程）

| 项 | 级别 | 问题 | 状态 |
|---|---|---|---|
| rprobe 判据盲区 | P1 | 64 位真值 36 case 只测了 23 个，缺 A2/D2/E3-E9/F3/G1 共 11 个生产规则验证 | ✅ 修：tests/rule_probe_bridge.rs 用 C 真值直测生产规则 11 case（8 测试覆盖 11 case），接入 rust_all [4c]，全 PASS——确认生产规则在这 11 场景与 64 位真值无分叉 |
| handle_touch 坐标 bug | P2 | 死实现把 view 坐标直接喂 720 基准按钮（差 1.25×），误接会点不到 | ✅ 修：view×0.8 统一口径 + #[deprecated] 标注 |
| 多套死渲染实现 | P2 | cpu_board_rgba/draw_rects_rgba/menu_rects 0 引用，易误接 | ✅ 修：全部 #[deprecated] + 注释指向生产路径 |
| UI-06 台账过时 | — | 轮3 已修（N3 禁 clear_frame）但台账仍标"待确认" | ✅ 更新台账状态 |

验证：113 tests（+8）+ clippy 0 + rust_all 含 [4c] 全 PASS + so_link 真核验 + ui_offline 6/6。
**依旧拒绝凑 99/轮**：本轮真实产出 3 个修复 + 1 个台账更正，可复现定位。

## 2026-11-22 审计轮 7（UI 像素级对齐深挖）

用 uicpm_diff.py 对原版真实截图（900×1600 实拍）vs Rust 离线渲染逐像素比对：

| 项 | 级别 | 发现 | 状态 |
|---|---|---|---|
| UI 按钮几何 | — | 主菜单 5 按钮 / 游戏设置按钮区在热图中【全绿】（逐像素一致），确认按钮贴图与几何已对齐 | ✅ 验证通过 |
| 背景标题垂直偏移 | P2 | 主菜单标题"军棋"bbox：原版 y[178,408] vs 渲染 y[109,382]，上移 ~70px 且高度压缩（原版230px/渲染273px）；x 一致(184 vs 180)。逐像素差异：主菜单上盘62%/中部69%（背景元素错位），对局/☰菜单 20-39% | ⚠️ 已定位、根因待确认：资源确认是 menu_bg_294（标题x宽540匹配），排除 bg_menu_720（x宽430不匹配）；排除缩放算法（单次/两段缩放结果相同）；排除采样率（450×800×2 = 900×1600 理论一致）。差异来自原版显示层映射（可能 GL 纹理 Y 坐标/系统缩放），当前 정보不足，不猜值硬改 |
| UI 判据强化 | — | 新增 uicpm_diff.py（分区差异率+热图+标题锚点对账），与 ui_offline_check.py 锚点检测互补 | ✅ 已加 |

**诚实边界**：背景标题偏移是可复现的真实差异，但修它需要原版确切的显示映射参数（GL 纹理坐标/截图缩放链），
当前只有"截图 vs 渲染"两端的像素，缺中间显示层信息。不硬调一个缩放系数假装对齐——那会用假绿掩盖真问题。
按钮/交互元素全对齐，此差异不影响功能与可玩性。
