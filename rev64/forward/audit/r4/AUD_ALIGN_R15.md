# AUD_ALIGN_R15（对齐对抗审计台账，Round 15）

> 域：64 位正向 src_rs（com.qwsaeda）"对齐"对抗审计第 15 轮。
> 结构：§1 续弈 SYNC_CELL 对账接线 / §2 send_sync 副作用根除 / §3 epoch 域限评估 / §4 R16 输入
> 状态列：已修（commit）/ 待修（= 下一轮输入）/ 撤回（误报）

## §0 基线

- R14 回执③（续弈语义）定案落地：重连 = g_state 保留断点续弈 vs RESTART 显式重开；SYNC_CELL 对账缺口 R15 接线
- 门禁基线：align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **145→152** + bt_semtest_dual **43/43**
- so 链：`f654d603`（R14）→ `1240e6f7`（R15 全收口）
- 设备：红果 10.0.182.146 离线（No route to host），权威 5 判据 PENDING
- 重打包产物：`phase_rs_a64.apk`（so `1240e6f7` + BtSpp 双栈 + 全量 spak_res + STORE + 双签）

## §1 续弈 SYNC_CELL 对账接线（R15-① 全落）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| Y-start_bt_session 锁外发 60 帧 SYNC（重连=续弈双端对账收敛漂移） | P0 | **已修** `468337d` | jni_bridge.c start_bt_session 锁外段（复用 UNDO mismatch :10757 范式） |
| Y-双端同版才互通 epoch/v3；SYNC_CELL v2 帧旧对端 0 退化 | P1 | **定案** | jni_bridge.c SYNC 注释（SYNC_CELL 逐格覆盖 st.cells，旧对端合法消费） |

## §2 send_sync 副作用根除（R15-②，P0 防复发）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| S-send_sync :433 强置 THEIR_TURN 副作用根除（host/client 角色/续弈场景均错） | P0 | **已修** `468337d` | bt_session.c 抽 `jq_bt_session_sync_encode` 纯编码函数，send_sync 复用它 |
| S-g_bt_payload 作用域（OnBluetooth 局部非文件级 static，start_bt_session 区不可见） | P1 | **已修** `468337d` | jni_bridge.c 独立 `g_sync_payload` 8KB + `g_sync_sbuf`（容量守卫 8KB） |

## §3 epoch 16 位域限评估（回执④，定案非阻塞）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| E-UNDO ref_lo 16 位承载 req seq（tx_seq>65535 极端长会话截断） | P2 | **定案非阻塞** | 典型对局帧量 <10K 帧（60 格 × 几十手），16 位 65535 充裕；未来超长会话可改 ref 承载位或 seq 重置 |
| E-epoch 16 位域（65535 次重连/会话边界回绕跳 0） | P2 | **定案** | bt_session.c:39 回绕修正（0xFFFF→0 跳 1） |

## §4 R16 输入

1. **权威 5 判据 + 装机冒烟**（红果恢复后 phase_rs_a64.apk 全链：装/board=OK/movetest/退出/重进 + BtSpp 双栈 v3 收帧 + 版本日志/jq.bt_ver 日志判读 + 续弈 SYNC 对账冒烟）
2. **32 位设备证据 0x39e14**（逆向 com.zxj 域，frida-dump 静态或真机 32 位 so 实跑；正向只读核对 callgraph 调用点 00039e14→000b9c98/0003e5bc）
3. **双端真机部署冒烟**（双端同版 v3/CAP/epoch 互通 + 单端升级降级判读 + 续弈 SYNC 双端对账，无 BT 硬件 PENDING）
4. **epoch 域限承载位改造**（若超长会话需求出现：UNDO ref_lo 16 位→32 位 ref 高位扩展，非当前阻塞）

## 撤回 / 误报

1. **撤回**"send_sync 无运行调用点"（R14 回执③ bt_session.c:177 注释口径）：实锤 :10744 UNDO mismatch 有调用点（R14 已撤回），R15 续弈接线后 start_bt_session 新增第 2 运行调用点。
2. **撤回**"jq_bt_send 返回 int 可逐帧判发送成功"（R15 攻坚坑①）：实锤 :541 `static void jq_bt_send`，60 帧须拼接 payload 单次发（复用 :10757 范式），非逐帧判返回值。

## 末行统计

- 总条目：§1×2 + §2×2 + §3×2 + §4×4 = **10 条**
- 已修：**4**（Y-start_bt_session + S-副作用根除 + S-g_sync_payload + §15 7 断言）
- 定案非阻塞：**2**（E 域限 2 项）
- 待修（= R16 输入）：**4**（权威 5 判据 + 32 位证据 + 双端真机 + epoch 承载位）
- 撤回：**2**
- P0：**3**（Y-接线 + S-副作用根除 + S-作用域），P1：**2**，P2：**2**

## 攻坚日志坑（R15 新增 4 条）

1. jq_bt_send 返回 void（非 int）——60 帧须拼接 payload 单次发（复用 :10757 UNDO mismatch 范式），非逐帧判返回值
2. g_bt_payload 是 OnBluetooth 局部 static（非文件级）——start_bt_session 区不可见，须独立 g_sync_payload（容量守卫 8KB）
3. send_sync :433 强置 THEIR_TURN 副作用须根除（host/client 角色/续弈场景均错）——抽 jq_bt_session_sync_encode 纯编码函数
4. epoch 16 位域限：UNDO ref_lo 16 位承载 req seq，典型对局 <10K 帧安全，超长会话才需改 ref 承载位（非当前阻塞）

## 门禁基线（R15 闭环时）

- align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **152/152** + bt_semtest_dual **43/43**
- 设备：红果 10.0.182.146 离线，权威 5 判据 PENDING；重打包产物 phase_rs_a64.apk（so 1240e6f7 + BtSpp + 全量 spak_res + STORE + 双签）
