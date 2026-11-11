# AUD-1000-ALL 全仓对抗审计矩阵（8 横块 × 12 功能维）

> 批次：AUD-1000-ALL（64 位 src_rs + 32 位翻译体全仓）
> 纪律：CODE_STYLE §9 假绿三型 + AUDIT_CHECKLIST §E 8 域扩展 + 审计纪律 6 条
> 目标：1000 条台账（可复现池），P 级只给精确实测，撤回误报计入，不硬凑

## 8 横块（D1-D8）

| 横块 | 文件范围 | 行数 |
|---|---|---|
| D1 规则引擎 | rev64/forward/src_rs/rules.c board.c layout.c save.c replay.c containers.c | ~9000 |
| D2 触摸/UI 状态 | touch.c touch_state.c ui_menu.c ui_state.c | ~4000 |
| D3 渲染 GL | render.c render_obj.c render_scene.c glview_gl.c gl2_probe_gl.c png_decode.c img_decode.c audio.c | ~12000 |
| D4 蓝牙 SPP | bt_protocol.c bt_session.c + BtSpp.java | ~3000 |
| D5 音效/存档 | audio.c save.c sfx 段 | ~2000 |
| D6 JNI 桥接 | jni_bridge.c（14 On* 入口 + 事件分发 + 钩子） | ~9000 |
| D7 工具链/门禁 | rev64/tools/*.sh *.py + rev64/*.sh（build_phases/rebuild_rs_apk/mumu_evtest 等） | ~8000 |
| D8 32位·64位一致性 + 32位翻译体 | rev64/reverse/src_jni_translated/*.c + so_probe.py + 双端对照 | ~116000 |

## 12 功能维（F1-F12）

| 维 | 判据焦点 |
|---|---|
| F1 逻辑正确 | 分支/边界/越界/静默降级 |
| F2 并发锁 | 锁纪律/无锁竞态/双源/读写同锁 |
| F3 内存安全 | 越界/UB/悬空/泄漏/半写 |
| F4 契约一致 | 事件契约/JNI 签名/帧协议/调用方约定 |
| F5 工具假绿 | 判据恒真/管道吞码/静默降级/缓存缺失放行 |
| F6 文档漂移 | SPEC/注释/台账 vs 代码不一致 |
| F7 资源资产 | 位图/WAV/布局表双源/尺寸核验 |
| F8 32·64 行为 | 双端规则分叉/事件码对齐/回合语义 |
| F9 错误路径 | 异常/失败分支静默吞/日志缺失 |
| F10 性能 | O(N²)/锁内长 IO/栈帧/热路径 |
| F11 钩子卫生 | 诊断钩子残留/prop 清 0/判据污染 |
| F12 可复现性 | 依赖 /tmp/环境 flaky/种子随机 |

## 子代理分派（12 并行，每代理覆盖 8 横块 × 1 功能维 = 8 任务）

| 子代理 | 功能维 | 任务（8 横块 × F_dim） |
|---|---|---|
| A1 | F1 逻辑 | D1-F1 D2-F1 D3-F1 D4-F1 D5-F1 D6-F1 D7-F1 D8-F1 |
| A2 | F2 并发 | D1-F2 D2-F2 D3-F2 D4-F2 D5-F2 D6-F2 D7-F2 D8-F2 |
| A3 | F3 内存 | D1-F3 D2-F3 D3-F3 D4-F3 D5-F3 D6-F3 D7-F3 D8-F3 |
| A4 | F4 契约 | D1-F4 D2-F4 D3-F4 D4-F4 D5-F4 D6-F4 D7-F4 D8-F4 |
| A5 | F5 假绿 | D1-F5 D2-F5 D3-F5 D4-F5 D5-F5 D6-F5 D7-F5 D8-F5 |
| A6 | F6 文档 | D1-F6 D2-F6 D3-F6 D4-F6 D5-F6 D6-F6 D7-F6 D8-F6 |
| A7 | F7 资源 | D1-F7 D2-F7 D3-F7 D4-F7 D5-F7 D6-F7 D7-F7 D8-F7 |
| A8 | F8 32·64 | D1-F8 D2-F8 D3-F8 D4-F8 D5-F8 D6-F8 D7-F8 D8-F8 |
| A9 | F9 错误 | D1-F9 D2-F9 D3-F9 D4-F9 D5-F9 D6-F9 D7-F9 D8-F9 |
| A10 | F10 性能 | D1-F10 D2-F10 D3-F10 D4-F10 D5-F10 D6-F10 D7-F10 D8-F10 |
| A11 | F11 钩子 | D1-F11 D2-F11 D3-F11 D4-F11 D5-F11 D6-F11 D7-F11 D8-F11 |
| A12 | F12 复现 | D1-F12 D2-F12 D3-F12 D4-F12 D5-F12 D6-F12 D7-F12 D8-F12 |

## 每任务 8 类固定判据（AUDIT_CHECKLIST §E）
1. 越界/UB 2. 无锁竞态 3. 双源/静默降级 4. 契约/签名 5. 工具判据恒真
6. 文档漂移 7. 资源/尺寸 8. 异常路径静默

## 台账格式（AUD_1000.md JSON line）
{"id":"ALL-Dx-Fy-n", "sev":"P0|P1|P2|P3", "file":"...", "line":N, "anchor":"语义锚点",
 "issue":"...", "evidence":"精确实测行号+代码片段", "fix_hint":"...", "_status":""}

> 纪律：P0=运行时必崩/数据错置，P1=逻辑错但不崩，P2=低影响/竞态窗口极小，P3=标注类。
> 撤回误报计入台账（_status=撤回误报），不硬凑 1000——可复现池不足时如实标注缺口。
