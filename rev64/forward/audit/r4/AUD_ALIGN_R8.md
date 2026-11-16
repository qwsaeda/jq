# AUD_ALIGN_R8（对齐对抗审计 R8，2026-11-16）

> 承接 R7 待修队列（B 20 / C 23 / D 余量 / 渲染域新扩面）；4 路子代理（C 31 条 diff 级 /
> B 21 条 / F 52 条渲染域 / G 53 条口径域）+ 会话自跑主攻 C/B/F 高价值落地。
> 状态列：已修 / 已定案 / 撤回 / 待修(R9) / 维持。
> 门禁基线（R8 落地后全绿）：align_gate **8/8**（新增 ⑧ sfx 覆盖）+ dark_align 86/86 +
> evverify 172/172（含 Σ 不变量哨兵）+ bt_semtest **93/93**（88 基线 + §5b RESTART 5 断言）+
> 全仓历史数字漂移归零（168/169/170/19:20 复扫 0 漏网）。

## §0 本轮关键定案（先读）

1. **RESTART mode+host_is_blue 帧字段（C-14/15/16 落地，`062e278`）**：on_frame 形参
   `f1=type, f2..f7=数据字段 1..6`（★ 探针实锤纠正：旧误设"字段在 f1/f2" → f1 恒=5(type)
   被字段校验 REJECT，rst_mode=5 实锤）→ 定案 **f2=mode(0明/1暗/999 v2 兼容)、
   f3=host_is_blue(0/1/999 缺省)**；收端帧 mode 权威覆盖本机 ui_opt（杜绝一明一暗双端
   分叉）、首手归属帧权威（999 回退 we_blue 角色分流 0 退化）；DISCONNECTED 下 RESTART
   放行（会话恢复路径，B04-012 闸口仅拒数据帧）+ case 入口 DISCONNECTED→CONNECTED 迁移
   （旧"放行后死会话"根因修正）；jni_bridge 11 调用点全量带参。
2. **事件单一入口（B-01/05/07/08/09/12/16/20 落地，`d050cfe`）**：by_name API +
   名级 code 覆盖表（DrawAgree→26/DrawDisagree→27，handler 诊断 0 漂移）+ _checked
   未知名拒计数 + Σ 不变量哨兵（evverify end-of-run）+ count-only 17 调用点收敛
   （手工旁路 5 + 7 码直写 8 + on_event/55codetest 11）双计审计 0；align_gate ⑧
   sfx_coverage_check.py 三层门禁（73 名→14 键→WAV + 兜底 58 + 不可达白名单 5）。
3. **触摸几何单一事实源（F-36/37 落地，`e5e7dc6`）**：view_to_base() helper +
   **119 处旧式独立 x/y 缩放全量清零**（grep 残留 0）；16:9 等价性实锤（720×1280
   s=1.0 Δ=0 门禁 0 退化）；20:9 全面屏（900×1600）触摸错位根因修正（letterbox 同几何）。
4. **全仓数字漂移归零（G 清账，`389a1ba`）**：PIPELINE 168/169→172 /
   BUSINESS_RISK_LEDGER 19/20→36 case / EXPERIENCE_LOG 2130/2174 快照标注 /
   工具注释 N/N 泛化 / AGENTS 路由 R{1..7} 同步 + 复扫 0 漏网。
5. **渲染域 52 条（F 回执）**：P1×8（UI 渲染 piece-tex 门控 F-01/02、GL 资源泄漏
   F-16/17/18、触摸 F-36/37 已修）——F-16/17/18 纹理释放表漏项 = **R9 主攻**。

## §C 蓝牙 v3 族（C-01..23 + 已落地 3 条）

| ID | 条目 | 状态 |
|---|---|---|
| C-14 | RESTART 无 mode 字段（一明一暗分叉） | **已修（062e278）** |
| C-15 | 扩字段方案 A（帧内 mode+host_is_blue） | **已修（062e278）** |
| C-16 | DISCONNECTED 终态拒开局 | **已修（放行+会话恢复定案，062e278）** |
| C-01/02 | UNDO 方向错位 + 无 ACK/NACK | 已定案（v3 请求-应答，R9 落地） |
| C-03/09 | 幂等 seq/token 缺失 | 已定案（v3 帧 10 字段 + last_req_seq，R9） |
| C-04/05 | 栈空 NACK + 锁外读竞态 | 已定案（R9） |
| C-06 | bt_semtest UNDO 桩与真实现脱节 | 待修(R9 v3 后 §5 重写) |
| C-07/08/10/11/12/13 | UNDO REQ/ACK/NACK 全表 + 双端回退 + 迁移表 + NACK 8 原因码 + UI 联动 | 已设计（回执 31 条 diff 级，R9 落地） |
| C-17/18 | 原子复位集 6 类 + 锁序 | 已定案（bt_restart_reset_all，R9） |
| C-19/20 | RESIGN 统一终局入口 | 已定案（bt_apply_terminal，R9） |
| C-21 | v3 破坏面 C/Java/推进三端 | 已定案（双栈解析窗口，R9） |
| C-22/23 | DRAW/CHAT v3 一致性 + SPEC §6 漂移 | 待修(R9) |

## §B 事件契约族（B 回执 21 条 + 已落地 8 条）

| ID | 条目 | 状态 |
|---|---|---|
| B-01 | by_name 单一入口 | **已修（d050cfe）** |
| B-02/03 | on_event 6 处 + 55codetest 11 处替换 | **已修（d050cfe，同型 11 处）** |
| B-04 | ev7 查表一致性 assert | 待修(R9，0 行为变化批) |
| B-05/06 | _checked + 前缀白名单 | **已修（d050cfe _checked；白名单=全表精确命中承担）** |
| B-07 | Σ 不变量 | **已修（d050cfe evverify 哨兵 + 设备端 jq.event_inv 钩子 R9）** |
| B-08/09 | 旁路 5 + 直写 8 收敛 | **已修（d050cfe count API，双计审计 0）** |
| B-12 | DRAW 子态码 25/26/27 + 名级覆盖表 | **已修（d050cfe exact 表）** |
| B-14/15 | sfx 映射显式化 + 双路径门禁 | **已修（align_gate ⑧ sfx_coverage_check.py）** |
| B-16 | sfx_coverage_check.py ⑧ 项 | **已修（ffe570a，负向验证删 WAV→FAIL 实锤）** |
| B-10/11/18/19 | 子态塌缩定案（Resign/悔棋代表码保持） | 已定案（R8-B-10/11 维持，(biz,sub) 双键不引入） |

## §F 渲染/GL/UI 资产族（F 回执 52 条，P1=8）

| 维度 | 条目 | P1 高价值（R9 主攻） |
|---|---|---|
| 渲染正确性 15 | F-01~15 | **F-01/02：UI/菜单/弹窗/overlay 全被 piece-tex && gl2_ok 门控**（render.c:10510——纹理缺失时全 UI 不渲染） |
| GL 资源 10 | F-16~25 | **F-16/17/18：rs_gl_rebuild 预删表 + rs_render_free 释放表漏 tex_house/tex_panel/tex_panel_btn/tex_btn_color2/tex_gameset_title/tex_panel_icon/tex_sound_text/tex_exit_text 8 项**（rebuild 覆盖旧 ID 不回收 + 泄漏） |
| UI 状态机 10 | F-26~35 | 无 P1（P2 锁序/返回契约集中化 = R9 批次） |
| 触摸几何 10 | F-36~45 | **F-36/37 已修（e5e7dc6 119 处清零）**；F-40/41 棋盘格命中同源化 + layout_align_check 触摸路径 = R9 |
| 资产链 5 | F-46~50 | F-46 fs 遮蔽坑复发判据（spak_res 版本校验）= R9 |
| 补充 2 | F-51/52 | F-51 背景 by0/by1 未随 bg_s 重算（瘦高屏 PENDING） |

## §G 口径/工具/设备族（G 回执 53 条）

| 组 | 条目 | 状态 |
|---|---|---|
| 台账/路由 | G-01/02 AGENTS R{1..7} 同步 | **已修（389a1ba）** |
| 数字漂移 | G-06/07/09/10/11/46/47/48 | **已修（389a1ba 全仓归零复扫 0）** |
| 设备验收 | G-33/35/36 红果在线但 5 判据未复验 + 桌面等价映射表 + 恢复 SOP | 已定案（设备侧 PENDING；so 哈希链 c5112d34 工作树为 R8 真值） |
| 打包卫生 | G-38 dist 包缺失 + /tmp 包 so 不一致 | **已修（build_phases.sh phase3 重打 + 装机 Success 实锤）** |
| 跨界检查 | G-43 R7/R8 提交 0 碰 reverse/com.zxj | **已实锤 PASS（git show --name-only 逐 commit 核对）** |
| 工具健壮性 | G-12/13/14 门禁失败模式分级 + 矩阵时间戳/新鲜度 | 待修(R9) |
| 双源漂移 | G-25/27/28 矩阵再生判据 + 探针单一事实源 + dark_align 86 引用 | 待修(R9) |
| 默认设备 | G-34 mumu_regress.sh 默认 emulator-5554 漂移 | 待修(R9 统一 10.0.182.146) |

## §撤回/误报（R8 汇总）

| 项 | 撤回理由 |
|---|---|
| R8-C-14 初版"字段在 f1/f2" | 探针实锤 f1 恒=type(5)，rst_mode=5 REJECT；on_frame 形参序 f1=type/f2..f7=数据，定案 f2/f3 |
| jq_evt55_name_to_55code 改 checked 后未知名 → -1 | 旧契约"未匹配兜底 0（MOVE）"为存量调用点依赖；R8 保留旧 API 兜底 0 语义 + 新 _checked -1 双轨（防存量漂移） |
| F-36 "238 处"口径 | grep `g_view_w \* 720` = 238 含注释/读取点；实际**调用点 119 处**（2 行配对形态 119 全替），238 为全仓 grep 命中数（含非调用点） |
| sfx 键解析"14 键 0 命中" | 初版正则双右括号形态 0 命中 → `\)\s*\)?\s*return` 容忍修复（14 键实锤） |

## 末行统计

- 条目：C 23 + B 21 + F 52 + G 53 = **149**（台账口径，回执全量）
- 本会话已修：**22 条**（C-14/15/16 + B-01/02/03/05/06/07/08/09/12/14/15/16 + F-36/37 + G-01/02/06-11/38/46/47/48，跨 commit 062e278/ffe570a/389a1ba/d050cfe/e5e7dc6）
- 已定案/设计（R9 落地输入）：C v3 完整协议 17 + B 余量 6 + F P1 6（GL 泄漏 3 + UI 门控 2 + fs 判据 1）+ G 工具 8
- 撤回/误报：**4 条**
- 门禁基线：align_gate 8/8 + dark_align 86/86 + evverify 172/172（Σ 哨兵）+ bt_semtest 93/93 + 漂移复扫 0
- so 哈希链：`d9d1c325`(R6) → R7 工作树 → **`c5112d34`（R8 工作树，phase3 已重打装机 Success；设备端 5 判据 PENDING 红果断连）**
