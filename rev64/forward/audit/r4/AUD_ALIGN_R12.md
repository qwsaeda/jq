# AUD_ALIGN_R12（对齐对抗审计台账，Round 12）

> 域：64 位正向 src_rs（com.qwsaeda）"对齐"对抗审计第 12 轮。
> 结构：§1 C 协议收尾三主线（hash 真值 / 重连强一致 / 混跑矩阵）/ §2 自跑批次 / §3 R13 输入
> 状态列：已修（commit）/ 待修（= 下一轮输入）/ 撤回（误报，不计入待修）

## §0 基线

- 3 路子代理并行：混跑矩阵 bt_semtest_dual（43 条设计）/ UNDO hash 真值 + mismatch SYNC（8 条）/ 跨重启 seq 强一致 + 能力握手（8 条，实锤重连 P0 缺陷）
- 门禁基线：align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **120→131** + bt_semtest_dual **11（新）**
- so 链：`95e0e6ba`（R11）→ `26f34b6`（R12 全收口）
- 设备：红果 10.0.182.146 离线（No route to host），权威 5 判据 PENDING

## §1 C 协议收尾域（R11 余量全闭环）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| C-hash UNDO ACK/NACK hash_lo 真值校验 + mismatch 才 SYNC（R9/R11 TODO 占位闭合） | P1 | **已修** `26f34b6`（jq_state_hash_lo FNV-1a 16-bit + ACK f4 真值 + 收端 mismatch 才 did_undo + DUP 缓存 hash） | bt_protocol.c + jni_bridge.c:10562/10619 区 |
| C-reconnect 重连 seq 窗口残留 P0（start_bt_session 仅首连 init → 重连残留 rx_last_seq → 新会话全帧误判旧包丢弃） | P0 | **已修** `26f34b6`（start_bt_session 改无条件 jq_bt_session_init，重连=新会话边界全清） | jni_bridge.c:3781 区 |
| C-dual 混跑矩阵 bt_semtest_dual（双实例对射 S1-S3 + RESTART + UNDO） | P1 | **已修** `26f34b6`（独立文件 11 条 + Makefile target） | bt_semtest_dual.c |
| C-cap 能力协商握手帧 BT_T_CAP（显式握手，双端枚举同步） | P1 | **待修**（R13 输入，需 bt_protocol 枚举扩展 + 双端同步 + DISCONNECTED 白名单） | R12 回执③ 设计 |
| C-epoch session_epoch/session_nonce（在途旧帧强防护） | P2 | **待修**（R13 输入，重连双端同复位 + 可选 epoch 拒收） | R12 回执③ §4 |
| C-dual 增强 43 条完整场景（当前 11 条核心，回执给 43 条 diff） | P2 | **待修**（R13 增强，S1m MOVE 双端一致性 + 断言补齐） | R12 回执① §9 |

## §2 自跑批次（R12 窗口）

- 3 路子代理回执全到 → 同会话落地三主线（hash/重连/dual），bt_semtest §5c-hash 6 + §9 重连 5（126→131）
- 攻坚坑：bt_semtest §8 重复块（存量）+ §9 插入撞残留尾（sed 精删）；hash 覆盖域取舍（不含 moves/winner 计数缓存避免误 SYNC）；重连无条件 init 对首连 0 副作用（init 幂等）

## §3 R13 输入

1. **BT_T_CAP 能力协商握手帧**（显式握手，双端枚举 + DISCONNECTED 白名单 + 连接建立即发；当前被动升档=互不先发 v3 时永不升档的缺口）
2. **session_epoch 在途旧帧强防护**（重连双端同复位 + epoch 不匹配拒收，N4/N5 窗口污染闭合）
3. **混跑矩阵 bt_semtest_dual 增强**（11→43 条，S1m MOVE 双端 jq_try_move 一致性 + RESTART/UNDO 完整断言）
4. **32 位设备证据**（0x39e14 暗棋 rank 泄漏，逆向域交叉验证）
5. **权威 5 判据**（红果恢复后全链复验 + dist/phase3.apk 重打包 R11/R12 全 so）
6. **续弈语义**（重连 = 传输窗口复位 + SYNC_CELL 对齐，当前仅定案传输强一致，不宣称续弈）

## 撤回 / 误报

1. **撤回**"重连后首个高 seq 帧不误判=强一致"主张：窗口层 rx_last_seq=0 放行成立，但后续新会话低 seq 帧会因高 seq 已登记被误判旧包（N5 反向污染），须双端同复位 + epoch 才强一致（R12 回执③ §4 判读，本轮定案传输窗口强一致不宣称全等）。
2. **撤回**"§8 重复块是本轮引入"：实锤为存量（bt_semtest.c §8 双份 688-716/718-747，R11 已存在），本轮 §9 插入撞残留是触发暴露，非新引入。
3. **撤回**"hash 覆盖全 jq_state 字段"主张：定案仅 60 格 + stage + turn（182 字节），不含 moves_since_eat/total_moves/winner 计数缓存（同构局面=可继续对弈等价态，避免与回退步数无关的计数分叉误 SYNC）。

## 末行统计

- 总条目：§1×6 + §2×3 + §3×6 = **15 条**
- 已修：**5**（C-hash/C-reconnect/C-dual + bt_semtest §5c-hash/§9 + dual 文件）
- 待修（= R13 输入）：**7**（BT_T_CAP/epoch/dual 增强/32 位证据/权威 5 判据/续弈）
- 撤回：**3**
- P0：**1**（C-reconnect 重连 seq 残留，已修 `26f34b6`）
- P1：**3**（hash 真值/dual/能力握手，hash+dual 已修，CAP 待修）
- P2：**2**（epoch/dual 增强，待修）

## 攻坚日志坑（R12 新增 3 条）

1. 重连=新会话边界（start_bt_session 无条件 init，非仅首连）——seq 窗口/能力位/UNDO 事务残留是跨连接 P0
2. bt_semtest §8 重复块 + 段插入锚点须避存量残留尾（§9 撞 §8 残留 → 编译错，sed 精删残留再插）
3. hash 覆盖域须与"对称回退同序天然一致"对齐（不含计数缓存，否则回退步数差异误触发 SYNC）

## 门禁基线（R12 闭环时）

- align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **131/131** + bt_semtest_dual **11/11**
- 设备：红果 10.0.182.146 离线，权威 5 判据 PENDING；dist/phase3.apk 旧包（待 R11/R12 so 全收口后重打包）
