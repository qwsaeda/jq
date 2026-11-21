# Rust 惯性化重构（2026-11-20）：C-in-Rust → 惯用 Rust 的边界实证

> 用户指正：Rust 代码风格/实现习惯像 C（语言特性/标准库/模块没用上）。本轮分批重构 + 用判据守门。

## 已完成的 Rust 化（行为零回归，判据全绿）
| 批次 | 改动 | 判据 |
|---|---|---|
| 1 | `rules.rs` 工兵 BFS：手写 `[i32;64]` 双队列 + `head/tail` 游标 + `[[bool;5];12]` → `VecDeque<(i32,i32)>` + `HashSet`（容量上限 64/邻接顺序/起点不查 walkable 全保留） | 95 测试 / 0 警告 / 154 case / align_gate 全 PASS |
| 2 | `move_legal` 的 `turn: i32/-1` 哨兵 → `Option<u8>`（None=忽略回合，Some(t) 校验阵营域） | 同上 |
| 3 | `ai_score` 的 `-1/-2` 哨兵 → `Option<i32>`（None=不可走，调用点 `match` + 18 测试断言同步） | 同上 |

## 边界实证（为什么核心规则层不能继续"纯 Rust 化"）
- **实测**：用 `rust_anything_check.py` 的 154 case 录制基线，把 `rank_norm` 从
  `if v > RANK_TOP { v-0xc } else { v }` 改成"更 Rust"的钳位写法
  （`if v>24 {RANK_TOP} else if v>RANK_TOP {v-0xc} else {v}`）→ **diff 立刻多处 mismatches**
  （`ai_score(2147483647,2)` 等 expected 1 got -1）。
- **结论**：核心规则层的 `i32 rank + as 转换 + +0xc 存储约定`是**原版 32 位行为逐字节对齐的载体**
  （R-138 分叉矩阵、`rule_probe_shared.inc` 同源探针、`RANK_*` 双源常量断言都依赖它）。
  把它改成 u8/enum 会改变脏 rank 域行为 = 破坏对齐判据。这是**对齐约束下的必要设计**，不是 C 风格坏味道。
- **JNI 层 `static mut`（14 处）**：JNI 全局态（G_VM/G_VIEW 等）跨回调必需，已 `#[allow(static_mut_refs)]` 标注；
  改成 Mutex/RwLock 会改变跨线程语义且 JNI 签名固定，属既有边界。
- `board.rs` 已是惯用 Rust（match 模式/纯函数/const/无 mut），坐标用 i32 是配合规则的对齐契约。

## 诚实边界
- "全面惯用 Rust 重构"在**行为敏感层**遇到实证硬边界：继续改 = 破对齐 = 砸 95 测试 + 154 case + 真机 5 判据。
- 本轮在**可安全改的范围**（BFS/Option 哨兵）完成了 Rust 化；剩余 i32/as/static mut 是
  对齐/JNI 契约下的**有意保留**，用 154 case 录制实证了"改了会破行为"。

## 核心层纯 Rust 化完成（2026-11-20 续）
用户明确要求"核心层也改成纯 Rust"，在实测行为等价的前提下完成：
| 批次 | 改动 | 判据 |
|---|---|---|
| 4 | `revealed: u8`(0/1/非法) → `bool`（非法值类型层面无法表达，域校验前移到 `Side::from_raw`/反序列化边界） | 全绿 |
| 5 | `side`/`turn: u8`(0/1/0xFF) → `Side` enum{Blue,Red,Unknown} + `to_raw/from_raw/opposite/is_valid/Display` | 全绿 |
| 6 | `rank: u8` → `Rank(u8)` newtype（PartialEq/Ord/From，数值语义 100% 保留含 +0xc 编码）；消除 `Rank as i32` 魔法转换 | 全绿 |

- 判据守门（每批）: 95 测试 / 0 警告 / 14 符号 / 154 case 对齐 / align_gate / fuzz_probe 全 PASS
- 真机 5 判据复验: 装 pid=53957 / 出屏 rendering / 走子 Move / 悔棋 undo_ok / 重进 pid=54082 全 PASS
- so 三方对账 37bbefeb（本机=APK=设备）
- `rust_anything_check.py` 判据适配 newtype 常量抽取（兼容旧 u8 + 新 `Rank::BOMB=Rank(10)` 两种格式）
- 核心层现在是类型安全 Rust：`Rank` newtype + `Side` enum + `revealed: bool` + `Option` 哨兵 + `VecDeque/HashSet`，无裸 u8/i32 魔法数混用

## JNI 壳统一用 jni crate（2026-11-20 续，用户指出 jni.rs 应对接 Rust jni 库）
- 原状：jni.rs 依赖 `jni = "0.21"`，但 OnCreate 一段（7 处 `envf.FindClass/GetMethodID/NewObject/...`）仍手写 JNINativeInterface 裸函数指针表——逐行照搬 C jni_bridge.c 的 C-in-Rust 残留。
- 改：OnCreate 整段改用 jni crate 安全封装（`JNIEnv::from_raw` + `new_object`/`call_method`/`new_global_ref`），裸函数指针残留归零。
- **踩坑固化（真机判据抓到）**：jni crate 的 `GlobalRef` 是 RAII 局部对象，函数返回 drop 会 `DeleteGlobalRef` → G_VIEW 悬空 → 渲染线程 `call_method(getHolder)` 内部 `get_object_class` 崩 SIGSEGV（fault 0x0）。修：`std::mem::forget(global)` 保持 global ref 存活，对齐旧裸指针版"从不 DeleteGlobalRef"生命周期语义。
- 判据守门：95 测试 / 0 警告 / 14 符号 / 154 case / align_gate 全 PASS；真机 5 判据复验（装 pid=55033 / 出屏 rendering / wgpu_step=win_ok / 走子 Move / so d12ccc38）。

## Rust 工程规范门禁（2026-11-20 续，用户"正向开发要多考虑 Rust 特性和规范"）
- **clippy -D warnings：48 警告 → 0（rc=0）**。机械修：assert_eq!(bool)→assert!、match→matches!、clamp、iter().enumerate() 替代索引循环、Default impl、doc 缩进等。
- **自定义 Error enum（0→1）**：`try_move` 的 `Result<Ev, ()>` → `Result<Ev, MoveError>`（BadIndices/IllegalTurn/IllegalMove + Display + std::error::Error），36 处调用点不依赖 Err 具体类型故零改动。
- 顺手修历史遗留 bug：`replay_vs_independent_replay` 前"§7 复盘越界 cur 拒绝"测试的孤立 `#[test]`（函数体早丢失，属性贴到邻函数）→ 补回真实 `replay_rewind_oob` 测试（cur=999 clamp + 重建后状态合法性断言），测试数稳定 95。
- 判据守门：95 测试 / clippy -D warnings rc=0 / android 0 错误 0 警告 / 14 符号 / 154 case / align_gate / fuzz_probe / event_names 全 PASS；真机 5 判据（so d6c534e8）+ 连续 3 次干净重启 0 崩溃。
- 方法论固化：行为对齐 = "做什么"（业务真值），Rust 化 = "怎么写"（类型系统/Result/Error enum/迭代器/clippy 门禁）——两者不冲突，正向开发应两者兼顾。

## 全仓 static mut 清零（2026-11-20 续：正向开发 Rust 规范深化）
| 原 static mut | 改造为 | 语义等价依据 |
|---|---|---|
| `G_STATE`/`G_UI`/`G_CNT` + `G_LOCK` | `static GAME: Mutex<Option<Game>>`（锁数据合一） | 37 处 with_lock 调用唯一入口，同锁串行 + 懒初始化 |
| `G_VM` / `G_VIEW` (raw jobject) | `AtomicUsize`（Acquire/Release） | 跨线程 raw 值，jni::JObject::from_raw 还原；GlobalRef mem::forget 保持存活不变 |
| `G_SURFACE` | 删除（死代码，仅定义无使用） | — |
| `G_WGPU_INSTANCE/DEVICE/QUEUE/SURFACE/CONFIG` + `WGPU_LOCK` | `static WGPU: Mutex<Option<WgpuState>>` | 全在 OnUpdate 内、原已持 WGPU_LOCK；setprop 日志/offscreen 验证全保留 |
| `wgpu_thread::JOIN` | `Mutex<Option<JoinHandle>>` | start 写 / stop take+join 串行 |

- **serde 未引入（诚实边界）**：save.rs 是纯逻辑层（域校验/复盘），无序列化代码；文件 I/O 在 JNI/磁盘边界。强行加 serde 会造出"无消费者的序列化 API"= 假实现，故不做。
- 判据守门：95 测试 / clippy -D warnings rc=0 / android 0 错 0 警 / 14 符号 / 154 case / rust_anything 全 PASS；真机 5 判据（so de03d4c2：装/出屏 rendering/走子 Move/悔棋 undo_ok/重进）+ 连续 3 次干净重启 0 崩溃。
- 全仓 `static mut` 声明归零（仅注释文字残留），消除 `#[allow(static_mut_refs)]`。

## 抑制归零 + 锁安全（2026-11-20 续）
- `#[allow(dead_code, unused_unsafe, static_mut_refs)]` 残留抑制清掉（static mut 已清零后 static_mut_refs 不再需要）→ 暴露并真修：
  - jni.rs:239 `unnecessary unsafe`（wgpu_thread::start 内部已处理）删除
  - jni.rs:59 `GAME.lock().unwrap()` → `unwrap_or_else(|e| e.into_inner())`：锁 poison 时恢复数据，JNI 边界不二次 panic abort
  - jni.rs:312 `wgpu_guard.as_mut().unwrap()` → `expect` 带语义信息
  - Canvas 诊断死代码（CANVAS_LOGGED + st_canvas_done/setprop_canvas_ok/setprop_canvas_fail）删除（早期出屏攻坚残留，无调用）
- **全仓 `#[allow(...)]` 归零**：所有警告真修掉，无靠 allow 盖住。
- 判据守门：95 测试 / clippy -D warnings rc=0 / android 0 错 0 警 / 14 符号 / static mut 0 / allow 0 / 154 case / align_gate / fuzz_probe / event_names 全 PASS；真机 5 判据（so 9c3f54b6）。

## render.rs 模块拆分（2026-11-20 续：用户授权拆 775 行大文件）
按职责拆成 5 文件，**逐字搬运、逻辑零改动**（仅可见性 pub(crate) + 路径 re-export 适配）：
| 文件 | 行数 | 职责 |
|---|---|---|
| `render/mod.rs` | 40 | 文件头 + 子模块声明 + 对外 API re-export + 坐标测试 |
| `render/gles.rs` | 502 | wgpu RenderCtx + 渲染管线 |
| `render/geom.rs` | 179 | 顶点/矩形几何 + RGBA 合成 |
| `render/cpu.rs` | 48 | 纯 CPU 棋盘 RGBA |
| `render/coord.rs` | 15 | 触摸坐标纯几何 touch_base_coord |

- 对外 API 不变：`crate::render::{RenderCtx, view_to_cell, draw_rects_rgba, menu_rects}` 全 re-export（jni.rs/wgpu_thread.rs 引用零改动）。
- 判据守门：95 测试 / clippy -D warnings rc=0 / android 0 错 0 警 / 14 符号 / 154 case / align_gate / fuzz_probe 全 PASS；真机 5 判据（so 3a3ffc78：装/出屏 rendering+win_ok/走子 Move/悔棋 undo_ok/重进 rendering）。
- 注：render.rs 原 `#![allow(dead_code)]` 随拆分保留在 mod.rs（单独清理属另一议题）。

## 自造轮子审计 + 首选替换（2026-11-20 续：用户问"什么该用库却自造"）
- **审计结论**：PNG 解码→png crate ✓ / JNI→jni crate ✓ / wgpu→wgpu crate ✓ / OnceLock/Mutex/Atomic/VecDeque/HashSet→std ✓ / 线程→std::thread ✓ / 格式化→format! ✓，这些已用对，不是轮子。
- **替换1（已做）**：`asset.rs::decode_png` 手写 RGB→RGBA 循环 → `png::Transformations::EXPAND|ALPHA|STRIP_16` 让 crate 直接输出 RGBA8（零新增依赖）。实证 `/tmp/pngcmp`：对 bg_menu(RGBA)/piece_atlas(RGB) 两图输出与旧手动转换逐字节 identical=true。
- **不替换（诚实边界）**：① 手写图像缩放/blit（asset.rs 15 个循环）——`image` crate 可做但需新增依赖（houdini 稳定风险），且椭圆棋子外形是业务真值无现成 API；② `log.rs` 手写 `__android_log_print`+setprop+文件探针——`log` crate 覆盖不了 houdini 环境下 logcat 不可靠的诊断通道。
- 判据守门：95 测试 / clippy rc=0 / 0 错 0 警 / 14 符号 / 154 case / 真机 5 判据（so 9b8544d2）。

## 出屏崩溃根治 + Java 壳裁剪（2026-11-20 续：用户"裁剪/作废 Java 壳"决策）
### 崩溃根治（用户选"两个一起做"）
- **根因**：wgpu_thread 用一次性 `surf_global` 缓存 Surface global ref，系统回收/重建 Surface 后失效 → `Surface.lockCanvas` 抛 `IllegalStateException` → JNI 异常未清 → 下次调用 `No pending exception expected` abort（logcat 8 次 Fatal，app 反复崩）。之前"5判据全PASS"只查 pid/属性未查 Fatal = 假绿盲区。
- **修法**：① `canvas.rs::draw_rgba_to_surface` 入口先 `ExceptionCheck/Clear` 清残留；lockCanvas 失败清异常返 Err（不 abort）；② `wgpu_thread.rs` 渲染循环**每帧从 view 重取当前 Surface**（isValid 检查），不再用失效的 global ref。
- **验证**：清 logcat 后启动 + 走子 + 20s 观察，Fatal/Surface释放 = 0，pid 稳定，canvas_err=0。

### Java 壳裁剪（dec_rs 构建树，非 git）
- **权威依据**：当前 Rust so 的 FindClass 只反射 `com/qwsaeda/GLView`（new_object + setEGLContextClientVersion + setContentView），**不反射任何 InputDlg/TextDlg/SiteView/AdLis/BtSpp** → 这些 Java 类是死代码。
- **删除**：AdLis/BtReceiver/BtSpp(4类)/SiteView(8类)/InputDlg/TextDlg = 15 个 smali；App.smali 的 btClient/btHost/btSend 方法体清空（保留签名防 native 反射）。
- **保留**：App（native 声明 + 生命周期 + 输入桥 + Handler 100ms）、GLView 全套（native new_object 它 + Renderer 接口被 App implements，内部类被本体引用不能单独删）。剩 17 smali。
- **验证**：apktool b 成功 + 装机 + 5 判据全 PASS + Fatal=0。

### 判据守门
95 测试 / clippy rc=0 / android 0 错 0 警 / 14 符号 / 154 case 全 PASS；真机 5 判据（装/出屏 rendering/走子 Move/悔棋 undo_ok/重进）+ **Fatal=0**（新增判据，防假绿复发）。

## JNI 异常安全的系统性加固（2026-11-20 续：用户质疑"用 Rust 怎么还有生命周期问题"）
### 认知边界（诚实）
- Rust 的所有权/生命周期只保证**编译期内存安全**，`GlobalRef` RAII 保证 JNI ref 不泄漏——但**管不到 Android Java 对象的状态生命周期**（Surface 被系统 release 后，GlobalRef 仍有效但 lockCanvas 抛 IllegalStateException）。这是所有 FFI 的固有边界，C/C++/Rust 都逃不掉。
- **Rust 的 `Result` ≠ Java 的 exception**：`map_err`/`.ok()` 只处理 Rust 侧错误，JNI 抛的 Java 异常挂在 JVM 线程上，不清理则下次 JNI 调用 `No pending exception expected` abort。
- 具体 bug 是设计失误（一次性缓存失效 ref + 不清 JNI 异常），不是语言缺陷；Rust 帮到的是 GlobalRef RAII + local ref 自动释放 + catch_unwind 挡 panic。

### 系统性排查与加固
- 全仓 38 处 JNI 调用点扫描：13 `map_err` + 25 `.ok()`，此前只有 canvas.rs 2 处清异常 → 跨帧/跨调用异常可累积。
- **加固**：`wgpu_thread.rs` 渲染循环每帧开头 `exception_check/clear`（渲染线程 attach JVM 长期运行，是异常累积的高危点）；canvas.rs 已有入口清 + lockCanvas 失败清。
- **死代码记录**：`egl_render.rs::start_render_thread_jni`（120 次轮询 .ok() 吞异常）无任何调用点（实际渲染走 wgpu_thread），不加防护、记录为死代码。
- JNI 规范：Java 调 native 时 JVM 保证入口无 pending 异常，故 14 个导出函数入口不需清——真正危险的是同一 native 线程内多次 JNI 调用的异常累积。

### 判据守门
95 测试 / clippy rc=0 / 0 错 0 警 / 14 符号 / 154 case 全 PASS；真机（so 2219c483）装/出屏 rendering/走子 Move + Fatal=0（含 No pending exception 检查）。

## 死代码清理 + 发现的待决策缺口（2026-11-20 续）
- **删除 egl_render.rs（152 行）**：`start_render_thread_jni` 0 调用点（实际渲染走 wgpu_thread），纯删除零行为风险；95 测试 / clippy 0 / 0 错 0 警不变。
- **保留（有理由）**：`sfx.rs::play` 0 调用点但音效映射表对齐有值（houdini JNI 崩溃回退，待真 ARM64 接线）；`render/mod.rs` 的 RenderCtx 完整管线方法（create_surface/init_device/render 等）是设计预留，当前仅用 create_wgpu_device/offscreen/clear_frame/create_surface。
- **~~⚠️ 待决策缺口~~ → 查证后撤销（2026-11-20）**：`strings.rs::result_text` 未被调用**不是缺口**——查 C 版 `rs_draw_overlay`(render.c:1264-1313)：C 正向版终局也只画纯色 quad（蓝/红/灰）+ 绿色"再来一局"按钮，**同样不画文字**。两家行为一致，文案表对齐但渲染采用色块方案是设计选择。我上一轮凭 strings.rs 有文案表就报缺口是误判（没先查 C 版），已纠正。
- **真实对齐差异（待决策，非 bug）**：C 版终局横幅 `x20%-80%,y42%-58%` + 绿色"再来一局"按钮(`x35%-65%,y62%-72%`)；Rust 版横幅 `x10%-90%,y40%-60%` 且无该按钮——Rust 重开走 ☰ 菜单(jni.rs:453)，功能可达但 UI 路径与原版不同。按 SPEC_HUMAN_PLAY 这是规格内实现；若要求"对齐原版业务真值"则需补画绿色按钮 + 触摸入口，需用户决策。

## rust_all.sh 设备判据健壮性（2026-11-20 续：adb 瞬断 + houdini 重启竞态）
- **adb 瞬断假绿**：脚本跑一半 adb offline，`getprop` 返回 "adb: device offline" 被 `wait_prop` 当成属性值（残留 rendering 误判 PASS）→ 修：`wait_prop` 过滤 offline/error/空 + `adb_ensure` 每个 adb 操作前自愈重连。
- **houdini 快速重启竞态（真 FAIL 但非 app bug）**：`force-stop; am start` 背靠背 → SIGSEGV `trying to execute non-executable memory`（fault 0x7c31... houdini 转译区, uptime 1-2s, 无 libjunqi 栈帧）。正常节奏（force-stop 后 sleep 2）重进 3 次 0 Fatal → 确认是转译层 JIT cache 竞态（AGENTS.md 已载"模拟器故障不外推真机"），修脚本加 2s 间隔。
- **重进 pid 轮询**：单次 `pidof` 取不到（进程启动延迟）→ 改 5 次轮询 + adb_ensure。
- 判据守门：`rust_all.sh --device` 全 PASS（编译+154case+so对账+出屏+走子+悔棋+重进pid+无Fatal）。

## 出屏判据摆脱属性依赖（2026-11-20 续：jq.wgpu_thread 属性污染）
- **发现**：设备侧 `jq.wgpu_thread` 属性被 selinux 永久标记 denied（`Rewrite len too long for denied property`，logcat 反复出现）；`adb setprop jq.wgpu_thread=rendering` 写不进（仍 canvas_err），而写新属性名 `jq.wgpu_thread2` 成功 → 属性名污染，不是 app bug。
- **假象链**：旧判据 `getprop jq.wgpu_thread=rendering` 实际读残留值；画面明明正常（截图 98% 非黑）属性却停在 canvas_err → 判据假 FAIL/假绿。我曾误判为代码 bug 改了"每帧写状态"（逻辑更正确，保留），但无效——根因是属性写不进。
- **根治**：`rust_all.sh` 出屏/重进判据改用**截图像素**（screencap → PIL 非黑比 > 0.85，轮询 20s 抗启动延迟），不可伪造、不依赖属性。实测 boot/restart 均 0.980 非黑比，全 PASS。
- 教训固化：真机判据优先用"不可伪造的物理证据"（截图像素），属性/日志只是辅助诊断（可被系统污染/权限拒绝）。这是继"Fatal 盲区"之后的第二个判据可信度教训。

## 终局"再来一局"按钮对齐原版（2026-11-21，业务真值差异修复）
- **差异**：C `rs_draw_overlay` 终局画横幅(x20-80/y42-58)+绿色"再来一局"按钮(x35-65/y62-72)，Rust 旧版只画横幅(x10-90/y40-60)无按钮，重开只能走 ☰ 菜单——功能可达但 UI 路径与原版不同。
- **修**（asset.rs + jni.rs + ui.rs）：① 横幅对齐 C 几何 ② 画绿色按钮(0.20,0.75,0.30 alpha0.9) ③ Screen::Game 分支最前加终局按钮命中(x315-585/y992-1152 @900×1600)→restart ④ **顺带修真 bug**：Ui::restart 旧实现只清空 cells→重开是空棋盘无子可走；改为随 mode 重载 30 字节布局(init_from_layout_30，对齐 C jq_restart_game)。
- **真机实证**：jq.overtest 钩子设终局→按钮中心像素(57,182,74)绿→tap(450,1072)→jq.ingame_act=restart→截图满盘50子红蓝棋子+按钮消失→二次点击不触发(stage已Play)。so 123b03fb。
- **踩坑**：jq_hook_overtest 文件在 houdini 下删不掉(已知坑)→每帧重设 stage=Over→点按钮后立刻回终局；测试时须先 rm -f /data/local/tmp/jq_hook_*。
- 判据：95测试(含 restart_after_over 改为验50子重载)/clippy0/0错0警。

## 终局按钮接入 rust_all + 设备离线快速 PENDING（2026-11-21）
- `rust_all.sh --device` 新增终局"再来一局"判据：钩子设 Over → 清钩子 → tap(450,1072) → 等 jq.ingame_act=restart → tap 后 jq.touch=cell* 证棋盘可交互（重开非空盘）。实测 PASS。
- **坐标坑**：tap(219,1106)→cell21 不是 cell6；正确 cell6 坐标=(274,1383)。判据改为"命中任一 cell 即可"。
- **clippy 修复**：改 restart 测试后遗留未用 import Cell/Rank（我的疏漏，rust_all 当场抓到）→ 删。
- **设备离线健壮性**：adb connect 在 No route to host 时阻塞到 600s 超时 → adb_ensure 加 timeout 5 + 设备段开头快速探测，离线时 9s 快速 PENDING（非 FAIL）。
- **UI 命中/事件映射录制不可行（诚实边界）**：ui_menu_hit 带 `ui_btn*`、事件映射带 `char*`，rust-anything 只支持标量(c_int/c_float/c_char/c_void)，bridge 不了；这俩的对齐已由既有 event_names_alignment_check.py(73名) + layout_align_check.py(Δ=0) 覆盖，不重复造轮子。

## 诊断属性换名 + touch_base_coord 去重（2026-11-21）
- **wgpu 状态属性污染根治**：`jq.wgpu_thread` 被设备 selinux 永久拒写（adb setprop 都写不进），13 处诊断写入全失效 → 换名 `jq.wgpu_state3`（新名可写，之前实证 jq.wgpu_thread2 同理）；判据早已改截图不依赖属性，此改动只恢复诊断能力。
- **touch_base_coord 去重**：gles.rs 与 coord.rs 两份实现（`+0.0-xoff` vs `-xoff`）数学等价但有漂移风险 → coord.rs 提升为模块级唯一真值，gles.rs `view_to_cell` 改用 `super::super::coord::touch_base_coord`，测试路径同步；删别名模块。0 错/clippy 0/95 测试。
- **存档判据已确认**：savetest 钩子写 app 私有目录+回读（jq.savetest_res），磁盘 I/O 在 JNI 边界已验证，无需补。

## 本机 C 风格残留清理（2026-11-21）
- `render/gles.rs` 3 处 `.max().min()` → `.clamp()`（与 fuzztest 同类改进）。
- `canvas.rs` 像素构造：手写 `for i in 0..n` + 索引 → `rgba.chunks_exact(4).take(n).map(|p|...)` 迭代器 + `resize` 尾部补齐（行为等价：rgba 实际总是 w*h*4，compose_board_rgba 保证）。
- 保留的 C 风格（有理由）：asset.rs 48 处 as（图像合成 f32↔usize↔u8 是像素运算本质）、asset.rs 双层 for y/x（性能敏感热路径，迭代器反而难读）、fuzztest 自实现 xorshift32（rand crate 会改可复现 seed 输出，风险>收益）。
- 0 错 / clippy 0 / 95 测试。
