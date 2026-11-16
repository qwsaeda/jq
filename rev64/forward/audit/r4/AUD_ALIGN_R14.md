# AUD_ALIGN_R14（对齐对抗审计台账，Round 14）

> 域：64 位正向 src_rs（com.qwsaeda）"对齐"对抗审计第 14 轮。
> 结构：§1 双端同版部署判据 / §2 BtSpp 同步缺口根治 / §3 续弈语义（PENDING）/ §4 R15 输入
> 状态列：已修（commit）/ 待修（= 下一轮输入）/ 撤回（误报）

## §0 基线

- 3 路子代理并行：dec_rs BtSpp 同步缺口（68 条设计）/ 双端同版部署判据（8 条设计）/ 续弈语义（9 条设计）
- 门禁基线：align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **139→145** + bt_semtest_dual **43/43**
- so 链：`dea773ac`（R13）→ `f654d603`（R14 全收口）
- 设备：红果 10.0.182.146 离线（No route to host），权威 5 判据 PENDING
- 重打包产物：`rev64/forward/src_rs/dist/phase_rs_a64.apk`（so `f654d603` + BtSpp 双栈 dex 实证 + 全量 spak_res + STORE + 双签）

## §1 双端同版部署判据（R14-② 全落）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| D-版本日志 JNI_OnLoad 打印 BT 协议版本（双端同版可机判） | P1 | **已修** `2040ad5` | jni_bridge.c JNI_OnLoad `LOGI("BT 协议版本 = v%d...")` |
| D-jq.bt_ver 属性钩子（55codetest/event_inv 同族，能力矩阵诊断） | P1 | **已修** `2040ad5` | jni_bridge.c OnCreate 钩子段 `jq.bt_ver`（纯 native 无锁一次性清 0，降级判定 SUSPECT_OLD_PEER） |
| D-单端升级检测（对端纯 v2 → 降级 v2 维持日志） | P1 | **已修** `2040ad5` | jni_bridge.c 收帧侧 `rc>=0 && v3_seq==0 && peer_caps_v3==0` → LOGI 降级（session_epoch 防刷屏） |
| D-CAP ver 门槛（f1>=JQ_BT_CAP_VER 才升档，部署同版约束） | P1 | **已修** `2040ad5` | bt_session.c CAP case `f1 >= JQ_BT_CAP_VER && f2 & JQ_BT_CAP_V3` |
| D-CAP case 形参映射纠正（f1=ver/f2=mask，旧 f2/f3 错位） | P0 | **已修** `2040ad5` | bt_session.c（on_frame_v3 非 UNDO 映射 f1..f6，ver 落 f1 非 f2） |
| D-bt_semtest §14 ver 比对 6 断言（匹配/ver 低/无 V3 位/>ver 兼容/双不匹配/RESTART 重升） | P1 | **已修** `2040ad5` | bt_semtest.c §14（139→145） |

## §2 BtSpp 同步缺口根治（R14-①，P0 前置阻塞）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| B-dec_rs 缺 qwsaeda BtSpp smali（App.smali 静态引用 3 处 → NoClassDefFoundError） | P0 | **已修**（R14 自跑） | dec_rs/smali/com/qwsaeda/BtSpp{,.smali $1/$2/$3}（sed cnvcs→qwsaeda 4 文件，dex 实证 indexOf/sendFrame/startHost） |
| B-bt_smali.sh 增 qwsaeda 合并目标（防复发，改 BtSpp.java 一条命令 3 树同步） | P1 | **已修** `2040ad5` | bt_smali.sh `if [ -d dec_rs/qwsaeda ]` sed Lcom/cnvcs→Lcom/qwsaeda 合入 4 文件 |

## §3 续弈语义（回执③，定案 + PENDING）

| ID | 严重级 | 状态 | 证据 |
|---|---|---|---|
| S-续弈定案（重连 = g_state 进程级保留断点，仅传输层复位） | P0 | **定案**（回执③ §1） | jni_bridge.c:193 g_state 全局 + bt_session.c:31-44 init 只清 peer_state 镜像/传输域，不写 st |
| S-续弈 vs RESTART 区分（隐式续弈保留 vs 显式重开重载布局） | P0 | **定案**（回执③ §4） | bt_session.c:196-270 RESTART 重载 st，:31-44 init 不重载 |
| S-SYNC_CELL 对账接线（重连后全量 60 帧对账，send_sync 无重连调用点） | P0 | **PENDING**（回执③ §2） | jni_bridge.c:10707 send_sync 仅 UNDO mismatch 调用，重连路径无 = 续弈对账缺口（R15 接线） |
| S-send_sync 状态副作用（:433 强置 THEIR_TURN，接线须剥离） | P1 | **PENDING** | bt_session.c:433 |
| S-bt_semtest C1-C8 续弈 8 断言（init 不清 st/SYNC 覆盖/重连≠RESTART/副作用） | P1 | **PENDING**（R15 落） | 回执③ §7 设计 |

## §4 R15 输入

1. **权威 5 判据 + 装机冒烟**（红果恢复后 phase_rs_a64.apk 全链：装/board=OK/movetest/退出/重进 + BtSpp 双栈 v3 收帧）
2. **续弈 SYNC_CELL 对账接线**（重连确认后 send_sync 60 帧，剥离 :433 状态副作用 + bt_semtest C1-C8 8 断言）
3. **32 位设备证据 0x39e14**（逆向 com.zxj 域，frida-dump 静态或真机 32 位 so 实跑；正向只读核对 callgraph 调用点 00039e14→000b9c98/0003e5bc）
4. **epoch 16 位域限评估**（UNDO ref_lo 极端长会话 tx_seq>65535 截断，后续可改承载位）
5. **双端真机部署冒烟**（双端同版 v3/CAP/epoch 互通 + 单端升级降级判读，无 BT 硬件 PENDING）

## 撤回 / 误报

1. **撤回**"CAP ver 不匹配时 peer_caps_v3==0"（回执② V2/V3/V5 原设计）：实锤**被动升档（seq>0 && rc>=0）与 ver 门槛正交**——CAP 收帧 rc=0 即被动置位 peer_caps_v3=1，ver 门槛仅控显式 CAP 分支。§14 V2/V3/V5 断言改为"显式分支不升但被动分支独立置位"。
2. **撤回**"BtSpp 双栈在主类 BtSpp.smali"（回执①误报 1）：v3/indexOf 帧解析在 `BtSpp$3.smali`（读线程 Runnable），非主类。
3. **撤回**"qwsaeda 蓝牙走 App 直连可忽略 BtSpp"（回执①误报 2）：App.smali:186/195/205 静态直调 `Lcom/qwsaeda/BtSpp`，BtSpp 必需。
4. **撤回**"BtSpp 缺失报 UnsatisfiedLinkError"（回执①误报 3）：BtSpp 纯 Java 静态类引用，预期 NoClassDefFoundError（非 JNI 链接失败）。

## 末行统计

- 总条目：§1×6 + §2×2 + §3×5 + §4×5 = **18 条**
- 已修：**7**（D 全 6 + B 全 2，去 B-dec_rs 自跑计入）
- 待修（= R15 输入）：**5**（权威 5 判据 + 续弈 SYNC 接线 + 32 位证据 + epoch 域限 + 双端真机冒烟）
- 撤回：**4**
- P0：**3**（CAP 映射错位已修 / BtSpp 缺口已修 / 续弈 SYNC 接线 PENDING）
- P1：**8**，P2：**1**

## 攻坚日志坑（R14 新增 4 条）

1. CAP case 形参映射错位（on_frame_v3 非 UNDO 映射 f1..f6，ver 落 f1 非 f2；旧 R13 f2/f3 错位，R14 纠正 f1=ver/f2=mask）
2. 被动升档（seq>0 rc>=0）与 CAP ver 门槛正交——CAP 收帧成功即被动置位 peer_caps_v3，ver 不匹配断言须识"显式分支不升但被动分支独立置位"
3. BtSpp 双栈帧解析在 BtSpp$3（读线程），漏拷内部类 = 运行期读线程缺失
4. bt_smali.sh 只合并 cnvcs 树（dec_dual/dec_full），qwsaeda 树（dec_rs）需 sed 改包名同步——改 BtSpp.java 须 3 树同步防漂移

## 门禁基线（R14 闭环时）

- align_gate **8/8** + evverify **172/172** + dark_align **86/86** + bt_semtest **145/145** + bt_semtest_dual **43/43**
- 设备：红果 10.0.182.146 离线，权威 5 判据 PENDING；重打包产物 phase_rs_a64.apk（so f654d603 + BtSpp + 全量 spak_res + STORE + 双签）
