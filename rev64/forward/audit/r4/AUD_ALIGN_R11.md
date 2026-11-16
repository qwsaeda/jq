# AUD_ALIGN_R11（对齐对抗审计台账，Round 11）

> 域：64 位正向 src_rs（com.qwsaeda）"对齐"对抗审计第 11 轮。
> 结构：§1 C v3 完整帧协议（全 6 Step）/ §2 B 事件契约 / §3 自跑批次 / §4 R12 输入
> 状态列：已修（commit）/ 待修（= 下一轮输入）/ 撤回（误报，不计入待修）

## §0 基线

- 4 路子代理并行：BtSpp 双栈 + marker / 全局 seq 去重 + 窗口复位 /（GL X-macro F-01/02 = R10 已闭环）/（C v3 设计 = R10 回执 97 条）
- 门禁基线：align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **100→120**（本轮 +20：§6 双栈 7 + §7 全局去重 5 + §8 能力探测 3 + 中间修正）
- so 链：`c5112d34`（R8/R9）→ `b544d52`（R10-B）→ `7f0c9b0`（R10-F）→ `2fec797`/`e1db457`/`9bee4d1`/`8bfa39c`（R11 C v3 全 6 Step）
- 设备：红果 10.0.182.146 离线（No route to host），权威 5 判据 PENDING

## §1 C v3 完整帧协议域（R10 回执 97 条 → R11 全 6 Step 落地）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| C-14 v3 独立帧格式（v3\n + 10 字段，seq/ref 独立槽） | P0 | **已修** `2fec797`（encode_v3/decode_v3/decode_any） | bt_protocol.c:70-182 |
| C-17 全局帧窗口 seq 去重（rx_last_seq 全 type + 前置判重） | P0 | **已修** `9bee4d1`（on_frame_v3 前置判重 + 逐 case 登记） | bt_session.c on_frame_v3 |
| C-21 BtSpp.java 双栈重组（v2/v3 双头 min + 噪声双版本） | P0 | **已修** `e1db457`（读线程双头识别 + bt_smali.sh marker 扩 v3） | bt_java/BtSpp.java + smali verify 4 文件 |
| C-P1 能力探测 + 会话内锁定（peer_caps_v3 收升发） | P1 | **已修** `8bfa39c`（on_frame_v3 收 v3 升档 + 发端 encode_v3 双栈切换） | bt_session.h 字段 + jni_bridge jq_bt_ctrl_send |
| C-P1 RESTART 窗口复位（rx_last_seq/tx_seq/UNDO/能力位全清） | P1 | **已修** `9bee4d1`+`8bfa39c`（RESTART 成功尾复位 12+2 字段） | bt_session.c on_frame_v3 RESTART 分支 |
| C-P1 OnBluetooth 推进字符集扩展（strspn "v2\n"→"v23\n"） | P1 | **已修** `9bee4d1` | jni_bridge.c:10664 区 |
| C-P1 UNDO 归一化映射位对齐（seq→旧 f2/ref→旧 f3） | P1 | **已修** `9bee4d1`（on_frame_v3 UNDO 分支映射） | bt_session.c ②映射块 |
| C-P1 前置判重 UNDO 豁免（保 REQ_DUP 重发缓存可达） | P0 | **已修** `9bee4d1`（非 UNDO 丢弃 / UNDO 放行 case） | on_frame_v3 前置判重 |
| C-P2 SYNC/SYNC_CELL 逐帧独立 seq（全局去重天然生效） | P2 | **已修**（`9bee4d1` 发端统一 ++tx_seq，逐帧单调） | jni_bridge + bt_session tx_seq |
| C-P2 混跑矩阵 bt_semtest_dual（S1 v3↔v3 / S2 v3发↔v2收 / S3 v2发↔v3收 双实例对射） | P2 | **待修**（§7/§8 桌面桩已覆盖核心场景，独立对射文件 = R12 增强） | R12 输入 |
| C-P2 hash 真值校验 + mismatch 才 SYNC（R9 TODO 占位闭合） | P2 | **待修**（ACK hash 占位 0） | R12 输入 |

## §2 B 事件契约域（R11 无新增，R10 已闭环）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| B-04/B-06/jq.event_inv | P1 | **已修** `b544d52`（R10-B 批次） | R10 台账 §2 |

## §3 自跑批次（R11 窗口）

- R11-C Step 1（bt_protocol v3 双栈 API + 桌面 §6）：`2fec797`
- R11-C Step 3（BtSpp 双栈 + marker）：`e1db457`
- R11-C Step 4/5（全局 seq 去重 + RESTART 复位 + 收帧 decode_any + 推进）：`9bee4d1`
- R11-C Step 6（发端 encode_v3 + 能力探测 + §8）：`8bfa39c`

## §4 R12 主攻输入（回执/缺口）

1. **混跑矩阵 bt_semtest_dual**（独立双实例对射文件，S1/S2/S3 三场景 + 断言清单）
2. **hash 真值校验**（ACK 携带 hash_lo 真值 + mismatch 才 SYNC，R9 TODO 闭合）
3. **32 位设备证据**（0x39e14 暗棋 rank 泄漏，逆向域交叉验证）
4. **权威 5 判据**（红果恢复后全链复验）
5. **RESTART 跨重启强一致**（重连 seq 窗口对齐，R11 诚实边界 → R12 闭合）

## 撤回 / 误报

1. **撤回**"基线 100 CHECK"口径：实测 bt_semtest 静态 `grep -c CHECK(` 经多轮演进，R11 终值 = 120 运行时 PASS（§6/§7/§8 累计），以二进制实跑输出为权威计数源。
2. **撤回**"RESTART 仅 R8-C-17 待办无字段可复位"：R9 已落 UNDO 事务字段 + R11 加窗口/能力位，RESTART 成功尾复位 12+2 字段（防跨局污染），实修落地。
3. **撤回**"v3 API 从零实现"表述：`bt_protocol.h:31-44` + `.c:70-182` 已存在 encode_v3/decode_v3/decode_any（R11 Step 1 落码），会话/桥接层接线（on_frame_v3/收帧/发端）= 本轮缺口，非协议编解码从零。

## 末行统计

- 总条目：§1×11 + §2×1 + §3×4 + §4×5 = **21 条**
- 已修：**12**（§1 全 6 Step + 子项 9 + §2×1）
- 待修（= R12 输入）：**4**（混跑矩阵 / hash 真值 / 跨重启强一致 / 32 位设备证据）
- 撤回：**3**
- P0：**4**（C-14/C-17/C-21 + 前置判重 UNDO 豁免，全已修）
- P1：**6**（能力探测/RESTART 复位/推进/UNDO 映射/SYNC seq/UNDO 豁免，全已修）
- P2：**4**（混跑矩阵/hash/跨重启/32 位证据，全待修 = R12）

## 帧形态实锤（防字段数误判）

- v2 帧 = `v2\n` + 7 CSV 字段（type + f1..f6）：实锤 `v2\n1,5,0,0,0,0,0\n`（17 字节，bt_protocol jq_bt_frame_encode）
- v3 帧 = `v3\n` + 9 CSV 字段（type + seq + ref + f1..f6）：实锤 `v3\n1,7,0,5,0,0,0,0,0\n`（21 字节，jq_bt_frame_encode_v3）
- **口径澄清**：v3 "10 字段" = 10 个**参数位**（type/seq/ref/f1..f6），线侧 9 CSV 列（type 列 + seq/ref + 6 数据域）；
  旧"10 字段"措辞指 API 形参，非 CSV 列数——防未来审计按"10 列"误读。decode_v3 sscanf 9 值（`%d,%u,%u,%d×6`）。

## 攻坚日志坑（R11 新增 5 条）

1. C99 goto 跨 int 声明（跳入带初始化声明）→ 改显式分支
2. UNDO 归一化 seq/ref 映射须对齐旧 case 读位（f2=seq/f3=ref，非直填）
3. 前置全局判重须 UNDO 豁免（否则 REQ_DUP 重发缓存被吞 → 事务悬挂）
4. `g_bt_sess_init_done` 定义 :1088 晚于 jq_bt_ctrl_send :574 → 前向声明（单一实定义）
5. §7/§8 块插入残留旧尾（脚本 anchor 未清 §7 尾 `g_bt_undo_avail=0;}`）→ sed 精删

## 门禁基线（R11 闭环时）

- align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **120/120**
- 设备：红果 10.0.182.146 离线，权威 5 判据 PENDING
