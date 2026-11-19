# Rust 正向 64 位全栈 · 交付清单（2026-11-19 固化）

> 范围 = 人机对战（砍蓝牙/四国/联网/账号），纯 64 位（aarch64）单源码，与 C `src_rs/` 物理隔离。
> 本清单固化"能做的全做完"边界：✅ 已达成项（桌面/交叉编译/打包可验证）+ 🔲 PENDING 项（须真机）。

## 1. 全栈模块（13 模块 / 2914 行 / 82 测试全绿）

| 模块 | 行数 | 测试 | 职责（规格章节） |
|---|---|---|---|
| board.rs | 93 | 4 | §1 棋盘几何/铁路/行营(10 格)/大本营 |
| state.rs | 169 | 2 | §0 数据模型+§8 悔棋+cfg 开关 |
| rules.rs | 760 | 14 | §3-§5 走子/吃子/翻棋/困毙/胜负+铁路 BFS |
| ai.rs | 262 | 4 | §9 评分表+贪心+难度分流+翻棋候选 |
| save.rs | 278 | 6 | §7 复盘+§8 域校验+原子语义 |
| ui.rs | 282 | 7 | §10 屏态机+面板外回滚+事件→屏态迁移 |
| evt.rs | 133 | 3 | §11 事件码/名分流（7 码 6 名） |
| gametest.rs | 173 | 2 | C game_selftest 移植（双 AI 60 局+真值布局） |
| evtest.rs | 255 | 15 | C evverify run_case 移植（事件真值回归） |
| rprobe.rs | 337 | 23 | C rule_probe 36 case 移植（规则行为探针） |
| jni.rs | 191 | 0 | 14 JNI 导出（返回值契约对齐 smali） |
| render.rs | 230 | 1 | wgpu 渲染骨架+坐标映射（出屏真机待办） |
| lib.rs | 37 | 1 | 模块注册+冒烟 |

## 2. C 正向优势移植（"对齐上限"证据搬入 Rust，✅ 完成）

| C 资产 | Rust 移植 | 状态 |
|---|---|---|
| 完整对局自测（game_selftest 双 AI 60 局+C 真值布局） | gametest.rs | ✅ |
| 事件真值回归（evverify run_case 核心） | evtest.rs 15 case | ✅ |
| 规则行为探针（rule_probe 36 case 双端对照） | rprobe.rs 23 case | ✅ |
| 终局守卫（B01-065/066 双守卫） | rules.rs on_draw/on_stalemate | ✅ |
| 暗棋 0xFF 定色/翻棋/困毙分层 | rprobe F 族 + evtest dark_flip | ✅ |

**★ 移植实证价值**：C 探针 36 case 真值对照**抓出 Rust 2 个规则层分叉 bug**（非抄测试）：
1. `side_stalemated` 缺 C 64 位 R-073 军旗排除（军旗误计可翻）
2. `rail_path_engineer` 起点误查 walkable（工兵拐弯 E1/E2 全误判 false）

## 3. 交付物（可验证）

| 交付物 | 状态 | 证据 |
|---|---|---|
| 纯逻辑层 82 测试 | ✅ 全绿 | `cargo test --release`（本机，不依赖 NDK/设备） |
| arm64 cdylib so | ✅ | ELF64 AArch64，764K，md5 d2934742 |
| JNI 14 导出符号 | ✅ | `nm -D` 14 全中（com.qwsaeda） |
| APK 三签 | ✅ | dist/phase_rs_rust.apk 20M，v1+v2+v3 全 true |
| 打包脚本 | ✅ | rebuild_rust_apk.sh（前半程 [0-4] 全通） |
| 规格审计 17 条 | ✅ 全修 | SPEC_HUMAN_PLAY P0×3+P1×8+P2×4+P3×2 |
| git 入库 | ✅ | 全推 origin/main（HEAD b1c9fb8） |

## 4. 真机验收实锤（设备 10.0.182.152:5555，2026-11-19）

| 判据 | 状态 | 实锤 |
|---|---|---|
| ① 装得上 | ✅ | 装机 Success + so 对账 PASS（Rust so d2934742） |
| ② 进主界面（出屏） | 🔲 **58.8% 非黑（< C 基准 98%+）** | Rust 骨架接管（pid 存活+无崩+14 符号匹配），wgpu render pass 未实现 |
| ③ 下一局 | 🔲 | 出屏未达成，走子前置未通 |
| ④ 正常退出 | ✅ | force-stop 0 崩溃 |
| ⑤ 重进不崩 | ✅ | 重启 pid 2705 存活 |

**★ 真机实锤（10.0.182.152，pid 2705/2501）**：
- Rust so 替换 C so 成功（smali loadLibrary("junqi") 同名替换，14 JNI 符号匹配，**无 UnsatisfiedLinkError + 无崩溃**）
- 设备 base.apk 内 so md5 = d2934742（= 本地 Rust so），C so 已删
- 截图 900×1600 非黑比 58.8%（系统层/Java UI 出屏，wgpu 渲染层未接 → 内容未出屏）
- 设备 IP 变更：10.0.182.146 → **10.0.182.152:5555**（rebuild_rust_apk.sh 已更新）

## 5. PENDING 项（真机可达 10.0.182.152，wgpu 出屏攻坚进行中）

| 待办 | 内容 | 进度 |
|---|---|---|
| ① wgpu render pass | device 初始化通路（create_wgpu_device pollster）已编译；**surface（EGL 互操作）+ 棋盘纹理上传 + wgsl 顶点批次 = 真机迭代中** | 🔵 device 通路 ✓ / surface+纹理 PENDING |
| ② OnTouch 坐标提取 | 原生 JNI 反射 MotionEvent.getX/getY→handle_touch（骨架安全降级） | 🔲 真机补全 |
| ③ 真机 5 判据 | ①④⑤ PASS，②③ PENDING（wgpu 出屏达成后） | 🔵 部分达成 |

**边界声明**：①②③ 代码骨架已落（render.rs 可编译+jni.rs OnTouch 安全降级+打包全链），
真机可达时补全 ①② 实现+跑 ③ 判据。设备不可达前**不出"出屏达成"结论**（禁自欺）。

## 6. 位宽锁定（用户 2026-11-19 指令）

- Rust 正向 = **纯 64 位（aarch64）单源码**，不做 32 位变体（用户明确不要求覆盖 32 位设备）
- 32 位设备走原版 com.cnvcs.junqi（ELF32）/ 逆向 com.zxj，与 Rust 正向互不干涉
- 对照基准 = 64 位 C `src_rs`（82 测试全对照 64 位 C 行为，非 32/64 双端逆向对照）
