# AUD_ALIGN_R13（对齐对抗审计台账，Round 13）

> 域：64 位正向 src_rs（com.qwsaeda）"对齐"对抗审计第 13 轮。
> 结构：§1 C 协议余量全闭环（CAP 握手 / epoch 强防护 / 混跑矩阵 43）/ §2 自跑批次 / §3 R14 输入
> 状态列：已修（commit）/ 待修（= 下一轮输入）/ 撤回（误报，不计入待修）

## §0 基线

- 3 路子代理并行：BT_T_CAP 握手（11 条设计）/ session_epoch 强防护（72 条设计）/ dual 43 增强（12 条设计）
- 门禁基线：align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **131→139** + bt_semtest_dual **11→43**
- so 链：`3fc70790`（R12）→ `80e31fc`（R13 全收口）
- 设备：红果 10.0.182.146 离线（No route to host），权威 5 判据 PENDING

## §1 C 协议余量域（R12 输入 7 项全闭环 3 主线）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| C-CAP BT_T_CAP 能力协商握手帧（闭合"互不先发 v3 永不升档"死锁） | P1 | **已修** `80e31fc`（枚举=9 + 收端 case 升档 + DISCONNECTED 白名单 + jq_bt_send_cap 收帧段首连发） | bt_protocol.h + bt_session.c + jni_bridge.c |
| C-epoch session_epoch 在途旧帧强防护（N5 窗口污染闭合） | P0 | **已修** `80e31fc`（wire_ref 高 16 位=epoch + 前置校验拒收 + init/RESTART 单调 ++ + UNDO ref_lo 拆分） | bt_session.h/.c + jni_bridge.c |
| C-dual 混跑矩阵 11→43 增强（S1m MOVE 双端一致性 + 完整 seq/能力/UNDO/去重/混帧矩阵） | P1 | **已修** `80e31fc`（dual 文件 43 条，S1m-S7m） | bt_semtest_dual.c |
| C-cap 双端同版握手（旧对端 default 拒收 CAP = v2-only 降级维持） | P2 | **诚实边界**（双端同版才互通，单端升级不宣称自动兼容） | R13 回执① §8 |
| C-epoch 单边重连不对称（对端未重连链路静默丢帧） | P2 | **诚实边界**（epoch 隔离正确拒收不可证帧，恢复须双端同会话） | R13 回执② §9 |
| C-32位证据 0x39e14 暗棋 rank 泄漏 | P1 | **归逆向 com.zxj 会话**（正向只读核对 callgraph 调用点 00039e14→000b9c98/0003e5bc，32 位 so 本机缺失） | 归属判断红线 |
| C-权威 5 判据 + 重打包 dist/phase3.apk | P0 | **PENDING**（红果 No route；rebuild_rs_apk.sh --no-install 桌面可打包待红果恢复复验） | 设备离线 |

## §2 自跑批次（R13 窗口）

- 3 路子代理回执全到 → 同会话落地三主线（CAP/epoch/dual），bt_semtest §10 CAP 8 条 + §11 epoch 8 条（131→139），dual S1m-S7m 净 +32（11→43）
- 攻坚坑：① on_frame_v3 epoch 代码首次 replace 文本漂移未真正落盘（grep 0 命中暴露，重做实锤）② dual S2m-4 重放 seq=2 实为旧包 <3 走 -1（非重复==断言纠正）③ jq_bt_send_cap 调用方持 g_mutex（收帧段已 lock 防嵌套死锁）④ §11 局部 session 需 memset（模拟 g_bt_sess 静态零初始化，栈未初始化 prev_epoch 读垃圾）

## §3 R14 输入

1. **权威 5 判据 + dist/phase3.apk 重打包**（红果恢复后全链复验 + 注入 R11/R12/R13 全 so + BtSpp smali）
2. **32 位设备证据 0x39e14**（逆向域交叉，frida-dump 静态或真机 32 位 so 实跑）
3. **续弈语义定案**（重连 = 传输窗口复位 + SYNC_CELL 对齐，当前仅传输强一致）
4. **双端同版部署判据**（CAP/v3/epoch 需双端同版；单端升级降级维持的自动化检测）
5. **epoch 16 位域限评估**（UNDO ref_lo 16 位 + 极端长会话 tx_seq>65535 截断，后续可改承载位）

## 撤回 / 误报

1. **撤回**"on_frame_v3 签名变更必致 79 处调用点级联"（R13 回执②误报）：源级 21 处 + 推荐方案 A（内部拆 wire_ref，签名 0 变）将级联降为 0，仅调用方构造/透传 ref 语义升级。
2. **撤回**"CAP 需要新 encode/decode 函数"：现有 encode_v3/decode_any 已支持任意 type v3 10 字段，复用即可。
3. **撤回**"start_bt_session init 后立即发 CAP 可满足建立即发"：jq_bt_start 异步（BtSpp listen/connecting），sendFrame 需 attach 后 s_connected/s_out → init 后即发高概率未连接拒发，改收帧段首连 init 后发（连接确认最可靠信号）。

## 末行统计

- 总条目：§1×7 + §2×4 + §3×5 = **16 条**
- 已修：**4**（C-CAP/C-epoch/C-dual + bt_semtest §10/§11 + dual 43）
- 待修（= R14 输入）：**5**（权威 5 判据/重打包 + 32 位证据 + 续弈 + 双端部署 + epoch 域限）
- 诚实边界：**2**（双端同版 + 单边重连不对称）
- 撤回：**3**
- P0：**2**（C-epoch 在途强防护 + 权威 5 判据）——epoch 已修，5 判据 PENDING
- P1：**3**（CAP/dual/32 位证据），P2：**3**（双端部署/单边重连/epoch 域限）

## 攻坚日志坑（R13 新增 4 条）

1. on_frame_v3 大块 replace 须 grep 实锤"代码真落盘"（文本漂移致 assert old==1 过但 replace 未生效，epoch 代码 grep 0 命中暴露）
2. 前置判重语义精确：seq==rx 才是重复（== 分支），seq<rx 是旧包（< 分支）——测试断言勿混淆（S2m-4 纠正）
3. 收帧段已持 g_mutex 的发送 helper 须无锁版（内部再 lock = 嵌套死锁，jq_bt_send_cap 改调用方持锁契约）
4. 局部 jq_bt_session 测试须先 memset（模拟全局静态零初始化，栈未初始化 prev_epoch 读垃圾致 epoch 乱跳）

## 门禁基线（R13 闭环时）

- align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **139/139** + bt_semtest_dual **43/43**
- 设备：红果 10.0.182.146 离线，权威 5 判据 PENDING；dist/phase3.apk 旧包（待 R11/R12/R13 全 so 收口后重打包）
