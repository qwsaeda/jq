# AUD-ALIGN R4 — 正向 64 位「对齐」对抗审计台账（Round 4，**4 路子代理并行**，100 条）

> ★ **本轮子代理通道恢复**：4 路子代理并行审计（免费模型），原始回报 **111 条**
> （A 规则引擎 27 + B 事件音效资源渲染 25 + C 交互态机会话蓝牙 34 + D 文档判据工具 25）
> → 去重/判读校正后 **100 条入账**（撤回 4 条误报，见文末）。
> 承接 R3（`AUD_ALIGN_R3.md`：承接 70 + 新增 30，已修 8 / 待修 58）。
> 类型：[实证] 已读码核实 / [判读] 需确认 / [覆盖缺口] 场景无验证。

---

## 一、A 组 规则引擎（25 条）

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R4-A-01 | P1 | 实证 | rules.c:106-137 | `rail_path_engineer` **未校验起点在铁路线上**（直走路径 :155 有 `is_rail(fr,fc)` 校验，两者不一致） | rulelay 铁路行走 | 待修(R5) |
| R4-A-02 | P1 | 实证 | rules.c:141 vs :197-200 | 工兵 BFS 到达目标用 `walkable`（要求目标为空格）→ **占位目标格判不可达**，与直走路径（不查目标格）不一致 | 原版工兵沿铁路攻击 | 待修(R5) |
| R4-A-03 | P1 | 实证 | rules.c:796-804 | 非工兵撞雷 EV_DEAD 路径**不清 `moves_since_eat`**（与 :841 吃子清零不对称） | 70 步无吃子 | ❌**撤回**（注释 AUD-1000 B01-022 已定案：送死步**不**算吃子活动，故意不清零） |
| R4-A-04 | P1 | 实证 | rules.c:656/796/841 | 撞雷 EV_DEAD 计数呈"既不清零也不 +1"第三态 | 同 A-03 | ❌**撤回**（同 A-03，注释已定案） |
| R4-A-05 | P1 | 判读 | rules.c:690-693 | 炸弹炸毁军旗终局**绕过"吃军旗"开关**（扛旗路径 :720 才按开关分流） | datah_012 吃军旗开关 | 待定案（需原版口径） |
| R4-A-06 | P1 | 实证 | rules.c:739-751 vs :1097 | 吃军旗开关关闭路径移除军旗但**不扣减被吃方 `pieces_left`**（`jq_on_flag_capture` 会扣） | 子力统计一致性 | 待修(R5) |
| R4-A-07 | P1 | 实证 | rules.c:997-1006 | 困毙"可翻子"扫描**未排军旗**（与 `jq_can_flip` :885 排军旗不一致） | 0x30320/0x30350 | ❌**撤回**（注释引原版 0x30350 实证："军旗不排除，未翻军旗方不计困毙"= **原版口径**，子代理漏读注释） |
| R4-A-08 | P1 | 实证 | rules.c:265 vs :398-399 | `move_legal` 已前置拒未翻子，:398-399 的"暗棋大本营安全岛"分支**不可达（死代码）** | 单一判定点 | 待修(R5) |
| R4-A-09 | P1 | 实证 | rules.c:314-324 vs :796-804 | 暗棋下 `move_legal` 依**隐藏 rank** 预拒"非工兵撞未翻地雷"，与执行器可结算 EV_DEAD 冲突 | 暗棋信息不可见 | 待定案（需原版 0x39e14 口径） |
| R4-A-10 | P1 | 实证 | rules.c:326-335 | 暗棋扛未翻军旗的合法性按 `rank_of(dst)` 真实 rank 预判 → **信息泄露式拦截** | 同 A-09 | 待定案 |
| R4-A-11 | P1 | 实证 | rules.c:269-276 vs :300-305 | 暗棋允许攻击未翻暗子（:269-276），但大本营目标分支读隐藏 rank 拦截 → **两处口径冲突** | 同 A-09 | 待定案 |
| R4-A-12 | P1 | 实证 | rules.c:838 vs :681/:815 | 同级互杀分支固定 `pieces_left[1-me]--`，**不处理防守方 SIDE_UNKNOWN**（炸弹/吃子分支有守卫） | B01-009 同源 | 待修(R5) |
| R4-A-13 | P1 | 实证 | rules.c:792 | 挖雷成功**扣减攻方 `pieces_left`**（与普通吃子只扣被吃方不对称） | AUD-R-008 | ❌**撤回**（注释 B01-008 已定案：挖雷攻方原格子消失须扣，语义有据） |
| R4-A-14 | P1 | 实证 | rules.c:690-694 vs :736-737/:849 | 炸弹炸旗终局**不更新 `total_moves`**（通用收口被 `stage!=PLAY` 跳过），与扛旗终局不对称 | 终局字段一致性 | 待修(R5) |
| R4-A-15 | P1 | 实证 | rules.c:744 + :245 | 吃军旗开关关闭时攻子落入**大本营** → "进大本营不可再动"使其永久冻结 | datah_012 开关组合 | 待定案 |
| R4-A-16 | P1 | 实证 | rules.c:197-200 vs :137 | 铁路直走中间格只查 `walkable`，**不查 `is_rail`**（BFS 每步查） | 铁路拓扑 | 待修(R5) |
| R4-A-17 | P2 | 实证 | rules.c:186-193 | 纵向直走中间格只做行营/大本营特判，**不查 `is_rail`**（依赖隐式几何假设） | 同 A-16 | 待修(R5) |
| R4-A-18 | P1 | 判读 | rules.c:303/315/321/326 | 暗棋目标合法性多处分读 `rank_of(dst)` 真实 rank → 与"未翻子身份隐藏"原则潜在冲突 | 原版 0x39e14/0x39f20 | 待定案（与 A-09~A-11 同批） |
| R4-A-19 | P1 | 实证 | rules.c:361-375 + board.c:57-60 | 大本营空格走子期恒不可进入 + `is_rail` 对 base 返 false → 铁路终点不能是大本营 | rulelay 大本营 | 待定案（补原版用例） |
| R4-A-20 | P1 | 判读 | rules.c:530-537 vs :565-569 | 和棋判定**优先于**同一步困毙判定（原版 0x30200 优先级未确证） | AUD-R-075/B01-034 | 待定案（32 位实证） |
| R4-A-21 | P2 | 实证 | rules.c:851 vs :925-939 | 走子计数 +1 / 翻棋**不计数**（与注释一致，但"步"定义需原版确认） | 0x3036a/0x39f20 | 待修(R5)（补探针固化） |
| R4-A-22 | P1 | 实证 | rules.c:885/:998/:1013 | 三处白名单不一致（jq_can_flip 排旗 / 可翻扫描只排雷 / 可走扫描排旗+雷） | 原版两集合口径 | ❌**部分撤回**（可翻扫描"不排军旗"为原版实证；余下三处差异仍需固化说明） |
| R4-A-23 | P1 | 实证 | layout.c:116-134 | `jq_check_layout_cell` 对**蓝半区（idx≤29）无位置约束**（地雷/炸弹/旗位置错可过检） | 原版 0x2a228 逐格校验 | 待定案（蓝方自定义布局是否同约束） |
| R4-A-24 | P1 | 实证 | rules.c:744-751 | 吃旗开关关闭时返回 `EV_FLAG` 但**未判胜** → 事件契约与"吃旗胜"语义冲突 | EV_FLAG 语义 | 待修(R5) |
| R4-A-25 | P2 | 实证 | rules.c:690-693/:736-737/:849 | 两条终局吃旗路径的 turn/total_moves/计数更新**不对称** | 0x30200/0x30260 | 待修(R5)（与 A-14 同批） |

---

## 二、B 组 事件契约 / 音效 / 资源 / 渲染（25 条）

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R4-B-01 | P1 | 实证 | jq_event_names_73.c:86/:94 | `Junqi.Over`（idx 12）**同时归属 code 3 与 code 11** 两桶 | 73 名→12 码唯一分组 | 待修(R5) |
| R4-B-02 | P2 | 实证 | evt55_dispatch.c:22-31 | `e55_handler_addr[67]` 只初始化 **64 项**（code 64-66 为 0） | 67 code 全覆盖 | 待修(R5) |
| R4-B-03 | P1 | 实证 | evt55_business.c:29 | `Junqi.Dead` 归 MOVE 业务桶（55code 0），12 码表却把 Dead 归 code 1 | 12 码分组语义 | 待修(R5) |
| R4-B-04 | P1 | 实证 | evt55_business.c:39-42 | 终局 `Junqi.Draw` 与提和协商族（DrawReq/Agree/…）**合并**到同一 55code 25 | 结果 vs 协商分离 | 待修(R5) |
| R4-B-05 | P1 | 实证 | evt55_business.c:52-54 | `Junqi.User*` 前缀全归 LOGIN 桶（34），但 UserCharError/UserLenError 属 12 码 code 8 组 | 12 码分组 | 待修(R5) |
| R4-B-06 | P1 | 实证 | evt55_sfx.c:26 | `Junqi.Draw/GameOver/Over` 全映射 `"draw"` → **丢弃独立 `over` 音效**（资源存在 data_junqi_over.wav） | 14 WAV 资源集 | 待修(R5) |
| R4-B-07 | P1 | 实证 | evt55_sfx.c:13-30 | `Junqi.Start/Select` 无显式映射 → 兜底 `"move"`（资源有独立 start/select WAV） | 同上 | 待修(R5) |
| R4-B-08 | P1 | 实证 | evt55_dispatch.c:43-61/:110 | evname==NULL 时 `e55_code_to_evname` 兜底 idx 0 → **未映射 code 全记 Junqi.Move** | 事件计数契约 | 待修(R5) |
| R4-B-09 | P1 | 实证 | img_decode.c:42-44 | `comp != 4` 误拒 **3 通道 RGB 源**（stb comp_out 返回源通道数）→ render.c "datax JPEG 直读优先"路径永远走兜底 | 资源解码路径 | ✅**已修**（去掉 comp 判据；req_comp=4 已保证 RGBA 输出） |
| R4-B-10 | P1 | 实证 | render.c:681-692 | JPEG 直读优先路径被 B-09 缺陷废掉（修 B-09 后恢复） | 同上 | ✅随 B-09 修复 |
| R4-B-11 | P2 | 实证 | img_decode.c:29/41 vs png_decode.c:181 | stb 全局垂直翻转 vs 自研 png_decode **不翻转** → 两路纹理 Y 朝向可能相反 | 纹理朝向一致 | 待修(R5) |
| R4-B-12 | P1 | 实证 | png_decode.c:28-31 | 非法 PNG filter type 时 `unfilter` **静默 return** → 调用方继续当成功解码（半解码像素） | 解码失败须拒 | 待修(R5) |
| R4-B-13 | P1 | 实证 | render.c:331-356 | `rs_read_png` 收 `am/asset_path` 却**只读 fs_path**，无 assets 回退 | 资源解析 | 待修(R5) |
| R4-B-14 | P1 | 实证 | render.c:157 vs :1606 vs touch.c:38 | UI 等比缩放（min）与棋盘/触摸/高亮**宽高比缩放**（vw/720）两套模型 → 非 16:9 屏坐标发散 | 渲染与触摸同一变换 | 待修(R5) |
| R4-B-15 | P1 | 实证 | render.c:10444/10526 vs :1606 vs touch.c:38 | 棋盘可切换**高度填充**缩放，而高亮与触摸仍按宽度 → 渲染-命中错位 | 同上 | 待修(R5) |
| R4-B-16 | P1 | 判读 | evt55_sfx.c:24 | Win/RedWin/BlueWin 一律 `"win"`，无本机视角 win/lose 区分（jni_bridge :631 有视角逻辑） | 终局音效视角 | 待修(R5) |
| R4-B-17 | P1 | 判读 | evt55_sfx.c:28 | Resign/ResignConfirm 一律 `"lose"`，未按谁认输区分 | 同上 | 待修(R5) |
| R4-B-18 | P2 | 判读 | audio.c:24-67 vs jni_bridge.c:565/919 | `audio.c` 是**并行未使用的音效子系统**（活路径用 sfx_wav/MediaPlayer）→ 双实现漂移风险 | 单一音效管线 | 待修(R5)（明确主实现或下线） |
| R4-B-19 | P2 | 判读 | render_obj.c/render_scene.c:80/114 | 场景/对象图 API **未接活渲染路径**（无 jq_frame_driver 调用点） | 原版渲染管线契约 | 待修(R5)（接线或诚实标注） |
| R4-B-20 | P1 | 判读 | img_decode.c:29/41 | stb 全局翻转标志**跨调用泄漏**（无作用域/复位） | 解码确定性 | 待修(R5) |
| R4-B-21 | P1 | 实证 | evt55_business.c:61-66 | `Junqi.Start/PeerStart` 归 NET 桶（38）→ 开局生命周期语义被并入网络桶 | 开局音效语义 | 待修(R5) |
| R4-B-22 | P2 | 实证 | render.c:393/407-410 | 构造 `spak_res/%s` assets 路径但 loader **忽略 asset 参数** → 路径构造为死逻辑 | 资源解析 | 待修(R5)（与 B-13 同批） |
| R4-B-23 | P1 | 实证 | render.c:10444/1603-1607/10678 | 高亮绘制独立算 `hl_s=vw/720`，与棋盘高度填充调整**不同步** | 高亮须贴合棋盘格 | 待修(R5)（与 B-15 同批） |
| R4-B-24 | P2 | 判读 | audio.c:9/24-39 vs evt55_sfx.c:11-30 vs jni_bridge.c:606-631 | 三处音效映射表**边界不一致**（flat 14 键 vs 事件名映射 vs 终局分派） | SPEC 三入口音效 | 待修(R5) |
| R4-B-25 | P2 | 判读 | src_rs/ 无 spak_load.c | 仓内**无 SPAK 加载模块**（渲染读 fs 暂存资源）→ "SPAK 加载对齐"目标无实现载体 | SPAK 契约 | 待修(R5)（接线或改口径） |

---

## 三、C 组 交互 / 态机 / 会话 / 蓝牙（25 条）

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R4-C-01 | P1 | 实证 | jni_bridge.c:2785-2827 | **OnTimer AI 入口不清玩家选中态**（§104 只修了 OnUpdate）→ AI 应着后残留 SELECTED(A)，下一击被当走子 | §104 修复语义 | ✅**已修**（三入口统一清选中 + 清高亮） |
| R4-C-02 | P1 | 实证 | jni_bridge.c:2785-2827 | **OnTimer AI 入口无任何音效**（对比 OnTouch 有 sfx_done 机制）→ AI 应着静音 | SPEC §5 | ✅**已修**（按事件码播音效 + 终局 win/lose/draw） |
| R4-C-03 | P1 | 实证 | jni_bridge.c:3487-3530 | **OnUpdate AI 块全程无音**（只修了清选中） | SPEC §5 | 待修(R5)（与 C-02 同批统一） |
| R4-C-04 | P1 | 实证 | jni_bridge.c:10137 | AI 应着音效被 `jq_glview_is_ready()` 门控 → glview 未就绪时**完全静音** | 音效=行为契约 | 待修(R5) |
| R4-C-05 | P1 | 实证 | jni_bridge.c:10138 | 暗棋**翻子不 inline 触发 AI**（判据仅 `move_ok`）→ 停帧环境 turn 恒 RED → "点不动" | 原版人机=AI 红方应着 | ✅**已修**（`move_ok \|\| flip_done`） |
| R4-C-06 | P1 | 实证 | jni_bridge.c:265-276 | `jq.mode` 属性**全仓无清除通道** → 测试钩子残留永久覆盖弹窗明暗选择 | §2f8e5930 残留属性坑 | 待修(R5)（一次性消费语义） |
| R4-C-07 | P1 | 实证 | jni_bridge.c:2763-2776 | OnResume 续局**强制 stage=PLAY**（注释自述"快照终局态保留则终局"自相矛盾）→ 已终局对局复活 | SPEC §7 续局 | 待修(R5) |
| R4-C-08 | P1 | 实证 | jni_bridge.c:2756-2776 | 续局只回灌 `g_state`，**不恢复 undo 栈 / 不重置 g_ts / 不 ui_replay_init** → 复盘基线以残局为准 | §104 续局与悔棋一致性 | 待修(R5) |
| R4-C-09 | P1 | 实证 | jni_bridge.c:230-247 + replay.c | 悔棋**不截断复盘序列**（seq 只增不减）→ 复盘重建错位 | datah_039/107 | 待修(R5) |
| R4-C-10 | P2 | 实证 | replay.c:84-97 | 复盘重建强制 `stage=STG_PLAY` 且全翻 revealed=1 → 终局复盘可继续改写棋盘 | SPEC §7 stage 语义 | 待修(R5) |
| R4-C-11 | P1 | 实证 | bt_session.c:152-165 | 对端认输收帧**无音效/无 evt55 派发/无计数** | SPEC §5 + 认输闭环 | 待修(R5) |
| R4-C-12 | P2 | 实证 | jni_bridge.c:10316-10340 | 蓝牙收帧 MOVE/FLIP **不计事件** → 战况统计只含本地着法 | SPEC §3 事件计数 | 待修(R5) |
| R4-C-13 | P1 | 实证 | bt_session.c:143-149 | 收端 RESTART 走 `jq_state_init`（**60 格全清零、不载布局**）→ 收端空棋盘永久软锁 | 原版重开=重载布局 | 待修(R5)（P1 协议级） |
| R4-C-14 | P1 | 实证 | bt_session.c:143-149 | 收端 RESTART **不清 g_undo** → 重开后按 ↩ 回滚到旧对局中局 | 重开清 undo | 待修(R5)（与 C-13 同批） |
| R4-C-15 | P1 | 实证 | bt_session.c:143-149 | RESTART 无条件 `s->state = BT_ST_OUR_TURN` → **主机重开后收端也变 OUR_TURN**，双端互发首手被闸口丢弃 | SPEC §6 回合闸口 | 待修(R5)（与 C-13 同批） |
| R4-C-16 | P1 | 实证 | bt_session.c:116-124 | SYNC_CELL **只写 `peer_state` 死影子**（读取点仅 bt_semtest）→ 全盘同步收敛通道失效 | SPEC §6 60 格同步 | 待修(R5)（P1 协议级） |
| R4-C-17 | P1 | 实证 | jni_bridge.c:10327-10337 + :9835 | 蓝牙 UNDO **方向错位**（收端 pop 本端旧手 / 发送端 BT 态恒不 pop）→ 双端必然发散 | SPEC §6 悔棋帧 | 待修(R5)（P1 协议级） |
| R4-C-18 | P1 | 实证 | jni_bridge.c:10014-10072 | 对局触摸**无 BT 回合闸口** → THEIR_TURN 期间可自由动蓝方棋子（本地 g_state 被静默改写） | SPEC §6 闸口 | 待修(R5)（P1 协议级） |
| R4-C-19 | P2 | 实证 | bt_session.c switch | **BT_T_DRAW/BT_T_CHAT 无收帧分支**（落 default 拒收）→ 联机提和/聊天无应答 | SPEC §6 提和/聊天 | 待修(R5) |
| R4-C-20 | P1 | 实证 | jni_bridge.c:490-540 + BtSpp.java | 断连镜像断链（Java 无 `jq.bt_conn` setprop）→ `g_bt_connected` 恒 1、ui_bt_active 永不清 → 断连后卡死 | SPEC §6 断连镜像 | 待修(R5)（P1） |
| R4-C-21 | P2 | 实证 | jni_bridge.c:3582 + :9844 | 联机态 BACK → **直接认输确认**（唯一退出路径）→ 误触白送一局 | 退出契约 | 待修(R5) |
| R4-C-22 | P1 | 实证 | jni_bridge.c 4 处重开入口 | 仅 ☰ 菜单 BT 路径发 RESTART 帧，其余 3 入口**静默单边重开** → 双端错位 | SPEC §6 RESTART 同步 | 待修(R5)（P1 协议级） |
| R4-C-23 | P2 | 实证 | jni_bridge.c:10348-10351 | 帧推进 `strchr(p,'\n')` 命中的是**帧头 "v2\n" 的换行** → 多帧串接时仅首帧被处理 | SPEC §6 帧格式 | 待修(R5) |
| R4-C-24 | P2 | 实证 | bt_session.c 收帧 MOVE 拒绝 | 收帧拒绝**无 NACK/回执** → 首次分叉后联机永久卡死且不可诊断 | SPEC §6 帧状态机 | 待修(R5) |
| R4-C-25 | P2 | 实证 | bt_session.c:152-165 | RESTART 帧**不携带明暗/布局语义** → 双端可能一明一暗 → 走子判定分歧 | SPEC §7 mode | 待修(R5) |

---

## 四、D 组 文档 / 判据 / 工具（25 条）

| ID | P | 类型 | 位置 | 未对齐摘要 | 基准 | 状态 |
|---|---|---|---|---|---|---|
| R4-D-01 | P2 | 实证 | AGENTS.md 路由表 | 路由表**漏本轮新增产物**（align_gate / event_names_alignment_check / gen_align_coverage / AUD_ALIGN_R1~R3 / 覆盖矩阵） | AGENTS 路由纪律 | 待修(R5) |
| R4-D-02 | P1 | 实证 | rule_probe_compare.py:150-155 | `--offline` 分支**恒 return 0** → 上游 fail 计数被吞（探针零输出也 PASS = 假绿） | CODE_STYLE §9 | ✅**已修**（offline 按 fail 汇总返回；负向验证：零输出 → 退出码 1） |
| R4-D-03 | P2 | 实证 | align_gate.sh:35 | 门禁失败输出 `tail -6` 截断 → 失败细节可能被吞 | R2-N17 | 待修(R5)（成功才截断） |
| R4-D-04 | P2 | 实证 | align_gate.sh 头 vs 正文 | 门禁**编号错位**（header 列 ①-⑥，正文 ⑦ 在 ⑥ 前） | 台账自述 7/7 | 待修(R5) |
| R4-D-05 | P1 | 实证 | dark_align_regress.sh:74 | 缺用例源文件 → `[MISS]` + pending++ + return 0 → **门禁不 FAIL**（覆盖集合漂移静默通过） | CODE_STYLE §9 静默降级 | ✅**已修**（缺文件计 FAIL；负向验证：移走用例 → 退出码 1） |
| R4-D-06 | P2 | 实证 | gen_align_coverage.py:118 | evverify 断言数统计口径错（正则不匹配其写法）→ 矩阵显示 0，与"172/172"矛盾 | R3-N12 | 待修(R5) |
| R4-D-07 | P2 | 实证 | gen_align_coverage.py + align_gate | 覆盖矩阵**无生成时间戳/版本**且**不在门禁内** → 过期不可见 | R3-N15/N16 | 待修(R5) |
| R4-D-08 | P2 | 实证 | event_names_alignment_check.py:51-73 | 事件名门禁**只比集合**，不比顺序/多重度 | R3-N17 | 待修(R5) |
| R4-D-09 | P2 | 实证 | event_names_alignment_check.py:48/57/80 | 未纳 `Misc.*` 名 + `%d` 变体族未展开 | R3-N18/N19 | 待修(R5) |
| R4-D-10 | P2 | 实证 | event_code_map_check.py:44-49 | 只核 Junqi 主链（Misc 3 条归因广告拆除）→ 口径未显式标注 | R3-N18 | 待修(R5) |
| R4-D-11 | P2 | 实证 | jni_bridge.c:1051-1079 | `ev7_to_name_idx` 越界兜底 `return 0`（静默降级）→ 无显式越界断言 | R3-N22 | 待修(R5) |
| R4-D-12 | P2 | 实证 | SPEC_BUSINESS.md:82-85 | **74 vs 76 内部矛盾**（"10 大类求和=74" vs "全量 76"） | SPEC §3 | 待修(R5)（口径统一） |
| R4-D-13 | P2 | 实证 | SPEC_BUSINESS.md:52/:237-251 | 业务码→55 code 映射表与 §3 事件码语义冲突（EV_MINE/EV_MOVE 同名） | SPEC §3/§3.4 | 待修(R5) |
| R4-D-14 | P2 | 实证 | SPEC_BUSINESS.md:116/:127-132 | 音效命名口径矛盾（jq_wav_load/jq_audio_play vs sfx_for_move/jq_evt55_sfx_for_name） | SPEC §5 | 待修(R5) |
| R4-D-15 | P2 | 实证 | Makefile:49-68 + consistency_desktop.c:33-40 | benchcheck 只比 `test_expected.txt`，**内嵌 BENCH_EXPECT 双源无自动同步断言** | R2-N23 | 待修(R5) |
| R4-D-16 | P2 | 实证 | mumu_align_smoke.sh:54-65 vs AUD_ALIGN_R3.md | 冒烟已改 3 次容错，但 R3 台账仍列 D-09 待修 → **台账状态滞后** | §106 口径 | 待修(R5)（台账同步） |
| R4-D-17 | P2 | 实证 | gen_align_coverage.py:77-80 | 假绿判定正则仍漏 `return g_ok ? 0 : 1;` 等形态 | CODE_STYLE §9 | 待修(R5) |
| R4-D-18 | P3 | 判读 | GAME_DEV_GUIDELINE.md:62-66 | "12/12"回归项枚举与 src_rs_regress.sh 实际条目不一致 | GAME_DEV_GUIDELINE §9 | 待修(R5) |
| R4-D-19 | P2 | 实证 | rules_fork_check.py:78-95 | `EVIDENCED_PROBE` 仅 16 项，与"102 条"口径未结构对齐 | R3-N14 | 待修(R5) |
| R4-D-20 | P2 | 实证 | event_names_alignment_check.py:27-30 | 门禁只核 3 个文件，**未核 `event_names_76.json` 双源** → 双源漂移盲区 | R3-N01/D-08 | 待修(R5) |
| R4-D-21 | P2 | 实证 | align_gate.sh:44-54 | 一键门禁**未纳设备冒烟**项 | R2-N15 | 待修(R5)（加 --device 分支） |
| R4-D-22 | P2 | 实证 | SPEC_BUSINESS.md:37/:159 vs consistency_desktop.c:36-40 | SPEC 和棋判据"70/1000 步"与 AUD-R-086"翻棋不计数"**未统一口径** | AUD-R-086 | 待修(R5)（口径统一） |
| R4-D-23 | P2 | 实证 | UI_COMPARE.md:395-414 vs :6/:20-26 | 12 项"全部对齐"汇总与静态对比基准**口径并存**（900 系 ×1.25 vs 720×0.8） | UI_COMPARE | 待修(R5) |
| R4-D-24 | P1 | 实证 | EXPERIENCE_LOG.md:6540 vs GAME_DEV_GUIDELINE.md:64 vs src_rs_regress.sh:52-54 | **事件断言数三口径漂移**（172 / 170 / 169） | R2-N13/D-13 | 待修(R5)（统一数字） |
| R4-D-25 | P2 | 实证 | EXPERIENCE_LOG.md:6633-6636 | 汇总自称"六层验证"而 align_gate 实跑 7 项 → **记账层数与执行层数错位** | AUD_ALIGN_R3 §三 | 待修(R5) |

---

## 五、Round 4 汇总

- **本轮首次实现 4 路子代理并行**（子代理通道恢复）：原始 111 条 → 去重/判读校正后 **100 条入账**（A/B/C/D 各 25）；**撤回 4 条误报**（A-03/A-04/A-07/A-13 —— 均因子代理未读既有定案注释：送死步不计数 / 未翻军旗不排是原版 0x30350 实证 / 挖雷攻方计数有 B01-008 依据），A-22 部分撤回。
- **本轮已修 6 条**（均含负向或实锤验证）：
  - **R4-D-02**（P1 假绿）：`rule_probe_compare.py --offline` 恒 return 0 → 探针零输出也 PASS；修后**负向验证：零输出 → 退出码 1**。
  - **R4-D-05**（P1 假绿）：`dark_align_regress.sh` 缺用例源文件 → pending 不 FAIL（覆盖集合漂移静默通过）；修后**负向验证：移走用例 → 退出码 1**。
  - **R4-B-09/B-10**（P1 真缺陷）：`img_decode.c` 的 `comp != 4` 误拒 3 通道 RGB 源 → render "datax JPEG 直读优先"路径永远走烘焙兜底；修（req_comp=4 已保证 RGBA 输出）。
  - **R4-C-01/C-02**（P1 §104 同族漏修）：OnTimer AI 入口未清玩家选中态 + 无音效 → 三入口统一清选中/清高亮 + 按事件码播音效（终局 win/lose/draw）。
  - **R4-C-05**（P1）：暗棋翻子不 inline 触发 AI → `move_ok || flip_done`。
- **本轮最大未闭合族群（P1 协议级，蓝牙）**：C-13/C-14/C-15/C-16/C-17/C-18/C-22 —— RESTART 收端清空棋盘 + 双端争 OUR_TURN + SYNC_CELL 死影子 + UNDO 方向错位 + 触摸无闸口 + 3 个重开入口不广播 = 联机对局在**首回合/首次重开/首次悔棋**即发散，且无 NACK/收敛通道（R4）。建议 R5 以"双端模拟回归（host/client 双实例 10 步 + RESTART + UNDO + RESIGN 全链）"一次性关闭该族群。
- **验证实锤（本轮全 PASS）**：align_gate **7/7 PASS**（含本轮修的 D-02/D-05 判据）+ dark_align 86/86 + evverify 172/172 + benchcheck ✓ + 事件名 73/73 + 双端探针 32/36 一致（新增分歧 0）+ 设备冒烟 2/2 + 0 崩溃（新 so 0464be91 装机复验）。
- **诚实边界**：本轮**行为改动 4 处**（B-09 解码判据、C-01/C-02 AI 入口、C-05 翻棋 AI 触发），其余为**判据/工具/文档口径**；**撤回 4 条子代理误报**（不硬凑）；A-05/A-09~A-11/A-15/A-19/A-20/A-23 等需**原版/32 位定案**项挂起（不擅改）；R4-C 蓝牙协议族与 R4-B 渲染/缩放族需 R5 专项处理。
