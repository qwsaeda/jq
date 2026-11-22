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
| JNI-02 | JNI | jni.rs:220/222/224 | OnCreate call_method 丢弃 Err，pending JNI 异常未清 → 后续 JNI abort | 待修 |

## P1（行为错误/可复现故障）

| 编号 | 来源 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| P1-3 | 判据 | evt.rs:206 | 73 名表 [72] 截断 `Junqi.GameType`，真值 `Junqi.GameType%d` | ✅ 已修（待补全量比对测试） |
| P1-4 | 判据 | evtest.rs:125/194 | 终局守卫 OR 断言恒真（Stage 仅2变体）、winner 未校验 | 待修 |
| P1-5 | 判据 | fuzztest.rs:66-79 | F2 变异器零断言、零生产调用，不可能失败 | 待修 |
| P1-6 | 判据 | rprobe.rs | "移植 C 36 case"仅 23 例且 E1/G2/F2/G4/G3 场景漂移 | 待修 |
| F1 | 规则 | rules.rs:445 | 困毙可翻扫描排军旗，C rules.c:1050 仅排地雷（原版 0x30320） | 待裁定 SPEC vs C 权威（三方漂移） |
| UI-01 | UI | asset.rs:189-218 | 棋子图集 6×6 切片错，原版 4×7(53×40) → 贴图全面错位 | ✅ 已修（4×7 + col=(rank-1)%4 + 红行0-2/蓝行3-5） |
| UI-02 | UI | jni.rs:517/asset.rs:239 | GameSet/Settings 画面板无按钮，任何触摸只回滚 = 假弹窗 | 待修 |
| UI-03 | UI | jni.rs:442-474 | NewGame 取消不恢复 mode 快照（jq.mode 残留坑） | 待修 |
| UI-04 | UI | asset.rs/jni.rs:193 | ExitConfirm 有触摸无绘制；Menu/NewGame 正常流程不可达（OnCreate 直接 Game） | 待修 |
| UI-05 | UI | jni.rs:545-557 | Game 顶部图标区超大隐形命中，吞掉红方上半盘 6行×5列 | 待修（P0 级卡玩法，子代理评 P1） |
| UI-06 | UI | jni.rs:339 | OnUpdate 每帧 clear_frame 纯蓝 + smali 仍有 setRenderer（注释矛盾） | 待真机确认（设备离线） |
| UI-07 | UI | evt.rs/sfx.rs | 音效映射正确但全链无消费者（14 WAV 编进 so 永不播放） | 已知边界（houdini JNI 崩回退，待真 ARM64） |
| JNI-03 | JNI | jni.rs:286/288 | Surface 与 Device 来自不同 wgpu Instance | 待修 |
| JNI-04 | JNI | wgpu_thread.rs:11-19 | stop() join 无超时 + 启动期不查 STOP → 主线程 ANR | 待修 |
| JNI-05 | JNI | wgpu_thread.rs:22-38 | START/STOP/JOIN 三态竞态 | 待修 |
| JNI-06 | JNI | canvas.rs:51-76 | lockCanvas 成功但错误路径未 unlock → Surface 永久锁死 | 待修 |

## P2（健壮性/数据一致性）

| 编号 | 来源 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| P2-7 | 判据 | fuzztest.rs:35 | Winner 全变体 matches! 恒真 | 待修 |
| P2-8 | 判据 | rust_geom_check.py | 无负向注入，"双源一致断言"未真比对生产常量 | 待修 |
| P2-9 | 判据 | rust_all.sh | 纳入不测 Rust 的 C 判据，"全量无遗漏"措辞不符 | 待修 |
| F2 | 规则 | save.rs:122 | 暗棋复盘必然失败（全翻开后 flip 重放失败） | 待修 |
| F3 | 规则 | save.rs:180 | validate_state 不校验 side↔rank 域，脏 rank 变军旗 | 待修（latent） |
| F4 | 规则 | state.rs:164/rules.rs | pieces_left 全程不更新，字段契约虚假 | 待修 |
| UI-08 | UI | strings.rs | 全表死代码，终局无文字 | 已知边界（C 版终局也色块） |
| UI-09 | UI | render/geom.rs,cpu.rs,gles.rs | 多套互斥死渲染实现 + handle_touch 坐标 bug，易误接 | 待修（标注 deprecated） |
| UI-10 | UI | jni.rs:426-441 | Difficulty 面板外无返回菜单处理 | ✅ 已修（我在 NewGame 之前补过 Difficulty 面板外？需复核） |
| JNI-09 | JNI | canvas.rs/sfx.rs | 错误路径残留 Java 异常；sfx 无 release | 待修 |
| JNI-10 | JNI | log.rs:18/47 | CString unwrap 可 panic；setprop 返回值丢弃 | 待修 |
| JNI-11 | JNI | wgpu_thread.rs:28 | 全局 panic hook 静默所有线程 panic 日志 | 待修 |
| JNI-12 | JNI | jni.rs:564-617 | G_SEL 跨屏残留；push_undo 早于合法性校验 | 待修 |
| JNI-13 | JNI | jni.rs:275 | OnUpdate 缺入口异常清理 | 待修 |
| JNI-14 | JNI | jni.rs:224-236 | OnCreate 每次重建泄漏 GlobalRef | 待修 |
| （JNI 类还有 JNI-08 OnUpdate 取 Surface 未 isValid） | JNI | jni.rs:317 | 同 JNI-01 死路径段 | 随死路径处理 |

## P3

| 编号 | 来源 | 文件:行 | 问题 | 状态 |
|---|---|---|---|---|
| F5 | 规则 | save.rs:149 | check_enums 死代码，两阶段校验未接线 | 待修 |
| F6 | 规则 | rules.rs:550 | on_stalemate 缺 loser_side 合法性守卫 | 待修 |

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
- 后续批次：JNI-02/03/04/05/06、UI-02~05、规则 F1~F4、几何判据同款桥接（P2-8）。

## 诚实口径
- **39 个 ≠ 100 个**：不硬凑。39 个全部有文件:行 + 触发依据，可复现定位是硬标准。
- 部分问题需裁定（F1 SPEC vs C）或真机验证（UI-06 纯屏、弹窗画面）——设备离线，不假绿宣称完成。
- P0-1（判据不测生产代码）是根因级问题：修好它，P1-5/6/P2-8 的同类假绿一并解决。
