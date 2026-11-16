# CURRENT_STATE — v22 终态（2026-11-15）

> **会话交接 / 接力规范** → `HANDOFF.md`（判据口径 + 已知缺口 + 继续做路线 + 接手 5 分钟自检）。
> 本文 = 进度台账；方法论 = `DECOMP_RECONSTRUCTION.md`；工具层 = `DECOMP_EXPERIENCE.md`。

## 总览

- **16 个重建文件**（`reconstructed/*.py`），全量 `ast.parse` + `compile` 零告警零错误。
- 覆盖：2 个引擎薄壳（`config`/`diagnostics`）+ `driver_check`/`client_config` 字节码级 +
  14 个业务模块（含完整 `client_ui` GUI 层）。
- **缺失 code object = 0**（v16 核心里程碑：14 业务模块源码闭包完整，维持）。

## 批量处理 / 方法论固化（v19 交付物）

本轮"总结经验规范，批量处理"指令的落地产物（规范单一文档原则，AGENTS 路由）：

| 产物 | 位置 | 内容 |
|---|---|---|
| **方法论规范** | `DECOMP_RECONSTRUCTION.md` | 三层判据（parse/结构/语义）+ codegen-noise 白名单 + 批量 6 步 + 高频真 bug 模式表 + 诚实边界 |
| **批量 triage 驱动** | `sem_triage.py` | 一条命令跑全模块 seq/sem + 长度比 ★ 启发式 + 噪声白名单，输出待修/噪声清单（`--json`/`--star-only`；工作区与项目扁平目录两布局自适应） |
| 工具层规范 | `DECOMP_EXPERIENCE.md`（追加路由） | 姊妹篇：pycdc 补丁/3.11 异常表/解包链 |

**批量处理纪律（固化）**：
1. 单模块 6 步：定位 → 拉 GT 反汇编 → sem 比对 → 白名单判读 → 只修真 bug → 复验。
2. 顺序：总 sem~ 最多者先做；★ 长度比越界优先于 0.9–1.1x 噪声。
3. **防假绿**：`missing_inventory` 0 ≠ 0 缺陷（只查缺失不查"在但语义错"）；真 bug 全靠 sem 比对 GT；
   重独有 code object 须清零（本轮 client_ui 14 个已判读=PySide6 容错 over-build，非 bug，维持并记录）；
   文档只在批量全过 + verify 总数确认下降后固化。
4. 每条真 bug 修复必须有 GT 反汇编证据（某指令/某常量/某 varname），"我觉得应该"不算依据。
5. **`sem_triage.py` 自动归类 = 初筛排序器，非终验**（v20 回归测试实锤其局限）：它靠
   "长度比 0.88–1.12 + 常量值多重集相等"自动归噪声，**抓不到小幅 guard/分支删除**（不改长度带
   也不改常量表的小 bug 会假阴性归"噪声"）。所以"0 越界 / 全噪声"只等于"没有整段缺失/过写迹象"，
   **不等于 0 缺陷**；关键函数仍需 §3.1 六步逐条对 GT + seq 判据终验。见 `DECOMP_RECONSTRUCTION.md` §七。


## 本轮（v22）模块级判据盲区清零 + 业务流程提炼（2026-11-15）

> "总结经验规范，批量处理" 第 5 次指令 → 发现前 4 轮判据**全部盲区**：
> 三层判据（parse/结构/函数级 sem）不含 `<module>` 根 code object，模块级常量/导入/注解/
> 默认值缺失 = 运行时 NameError 级真缺陷却 0 覆盖。本轮一次性暴露 **54 个**并全部 GT 字节码驱动修复。

| 项 | 结论 |
|---|---|
| **判据强化** | 新增第 4 层（模块级）：`global_ref_check.py`（LOAD_GLOBAL 差集 × GT STORE_NAME 交叉）+ `verify_all_modules.py` 扩 `<module>` 比对 + `extract_module_consts.py`（GT 模块级常量还原）+ `reorder_module_level.py`（按 GT 序重排顶层块） |
| **54 真缺陷修复** | `第一步` 29（STEAM_守卫_*/TIMEOUT_*/_SUB_STEPS/_OWN_WIN/_STEAM_WIN…）/ `第四步` 23（TIMEOUT/VK_ESCAPE/参考分辨率/_候选表…）/ `workflow_steps` 9（STEP_MODULES/GAME_PROCS/_CAP_GUARD/_DIAG/_originals/验证码双词…）/ `client_ui` 2（threading/__doc__） |
| **值错修复** | `workflow_steps._AMMO_EXCHANGE_TABLE` `ammo_exchange`→`ammo_exchange_log`、`第五步` 同、3 个 STEP_NAME 短名→长名（`第四步 · 特勤处设施` 等）、`client_ui.APP_VERSION` `1.9.20261115`→`1.9.20261120`、logger 名 `client_ui`→`pc_tqc.ui`、`研发扫AW._默认配置` 全量重写（坐标/等待/关键词 56 键值全对 GT） |
| **注解/默认值对齐** | `client_config` 补 `-> Path`、`第一步._shrink_for_ocr(frame, max_width=1280)`、`第五步` 删 8 处 over-build 注解、`client_ui.cloud_handshake(client: CloudClient)->tuple` 等 |
| **定义顺序对齐** | `第五步`/`第一步`/`研发扫AW`/`workflow_steps`/`client_ui` 顶层块按 GT 模块级 STORE_NAME 序重排（`reorder_module_level.py`，装饰器坑已修） |
| **names/vars 漂移清零** | `client_runner._on_observation_detected_cb`（short-circuit 拆嵌套 + `account_name` 形参）/ `研发扫AW._muted(username, event)` / `第一步.set_screen_check` 注解 / `第三步.install/wrapped`（局部 `e`）→ **全模块 0 漂移** |
| **v22 终态** | parse 16/16 · 缺失 0 · 模块级 sem 11/14（严格；余 3 = 注解 over-build/默认值位置/顺序，全语义等价）· 函数级 sem 621/652 (95.2%) · 自动归类 32/32 噪声 0 疑真 · names/vars 漂移 **0** |

**诚实边界（维持）**：
- 模块级 sem 余 3 模块（`client_ui`/`workflow_steps`/`研发扫AW`）逐指令不一致 = ① 注解 over-build
  （recon 加而 GT 无）② 默认值 `BUILD_TUPLE` vs `MF` flag 布局 ③ 顶层块顺序——**全部语义等价，
  无运行时缺陷**（NameError 风险 0，`global_ref_check` ★真缺陷 0 实锤）。
- 函数级 sem 余 31 处 = §二 白名单 codegen-noise（handler 展开/短路/PUSH_NULL/RETURN_CONST）。
- 重独有 14（`client_ui` PySide6-import 容错 over-build，非 bug，维持并记录）。
- 业务流程提炼：见下节（基于完整源码程序化提取，非记忆/推测）。

## 业务流程提炼（v22，基于完整重建源码程序化提取）

> 以下事实全部从 `reconstructed/*.py` 的 AST/字节码程序化提取（STEP_NAME/install 入口/
> STEP_MODULES 编排/调用链），非记忆推测。

### 6 步自动化流水线（workflow_steps.STEP_MODULES 编排）

`STEP_MODULES = ('第一步','第二步','第三步','第四步','第五步','研发扫AW','第六步','设施等级')`
—— `install_step_wrappers` 顺序 `importlib.import_module` + `mod.install(cls, ctx)` 装到
`AccountRunner` 类上（patch 原方法，不改内部字节码）。

| 步 | STEP_NAME | 职责 | 入口 |
|---|---|---|---|
| 第一步 | `第一步 · 打开Steam进入游戏` | Steam 活跃用户校验 + 杀进程守卫 + 登录/启动/处理启动器（`_SUB_STEPS` 6 子步） | `install(cls, ctx)` |
| 第二步 | `第二步 · 进入游戏大厅` | 拉云端规则 + 匹配 + 找关键词坐标进大厅 | `install` + `_flow` |
| 第三步 | `第三步 · 读取行动等级` | win32 点击读等级 + 收藏室界面判定 + 哈夫币 M 单位 patch | `install` + `_flow` |
| 第四步 | `第四步 · 特勤处设施` | 设施/交易行准备 + 模式B 自定义跳过（OCR 判锁标，全锁跳本轮） | `install` + `_自定义跳过流程` |
| 第五步 | `第五步 · 军需处兑换` | 军需处弹药兑换（`ammo_exchange_log` 去重） | `install` + `_do_ammo_exchange` |
| 研发扫AW | `研发扫AW` | 研发部门扫 .338 Lap Mag AP（12/20 点档，dry_run 安全） | `_run_aw_scan` |
| 第六步 / 设施等级 | （exe PYZ 无对应 .pyc） | STEP_MODULES 引用但运行时 `importlib` 失败（诚实边界：模块未含在 PYZ） | — |

**关键不变量**：
- **视觉层收口 Nuitka `.pyd`**：`ctx.grab_frame`/`ocr_scan` 是抽象接口，native 引擎收口，
  源码层**绝不读游戏内存**（只 OCR 截图 + win32 点击 + 进程/注册表存在性检查）。
- **验证码守卫**：`_install_captcha_guard`（安全验证/安全挑战弹窗 → `CaptchaError` → 账号判失败人工处理）。
- **弹药交换去重**：`_is_ammo_exchanged_today`/`_mark_ammo_exchanged_today`（`ammo_exchange_log` 表，username+日期）。
- **云端协议**：`CloudClient`（teqinsi.cn）+ `sig_verify`（Ed25519 响应签名，防伪造云端）+
  `runner_worker` 回传（账号数据/封禁/观察期）。
- **GUI 入口**：`client_ui.MainWindow`（PySide6）+ `RunnerWorkerThread`（QThread 跑
  `runner_worker.main` 子进程）+ `cloud_handshake`（设备指纹 + 账号 upsert）。
## 本轮（v21）批量实锤 + 判据强化

> "总结经验规范，批量处理" 第 4 次指令 → `sem_triage.py` 加**第 3 信号**（`co_names`/`co_varnames`
> 集合漂移 = 真-bug 强指标），把前 2 轮 2-signal 分类器漏报的 3 个真实注解/typing 缺陷揪出并修掉。

| 项 | 结论 |
|---|---|
| **判据强化** | `sem_triage.py` 三信号（长度带 + 常量值多重集 + ★names/varnames 漂移）；自动归类 31/31 噪声 / 0 疑真 / 0 越界 |
| 修 `第五步/_is_in_time_window` | 局部 `time` 遮蔽模块 `time` → 改 `from datetime import time as dtime`（vars 漂移清除） |
| 修 `client_ui/RestoreWorker._payload` | 补 `item: tuple -> tuple` 注解（names 漂移清除） |
| 修 `client_config/ClientConfig.load` | `-> ClientConfig` 求值注解 → 改字符串 `-> 'ClientConfig'`（GT 用字符串注解，const 非 LOAD_NAME） |
| **全量验证** | 结构 0 缺失 / seq✓ 618 (87.8%) / sem~ 33（全实锤 codegen-noise）/ 16 文件 parse-clean / 自动归类 0 疑真 |

## 全模块语义差分（v21 终态，704 配对 code object）

- 字节码逐指令一致（seq✓）：**618 (87.8%)**
- 计算流一致（sem✓）：**~670 (95.2%)**
- 计算流差异（sem~）：**33** —— 全为 codegen-noise；`sem_triage.py` 三信号自动归类 **31/31 噪声 / 0 疑真 / 0 越界**
- 缺失 code object：**0**
- 重独有：client_ui 14（PySide6-import 容错 over-build，非 bug）

| 项目 | 状态 |
|---|---|
| **缺失 code object 归零** | ✅ 0（维持） |
| `workflow_steps` sem-triage | ✅ 17 → **0**（12 处真实修复） |
| `client_runner` sem-triage | ✅ 2 → **0**（注解补齐 + 删杂散类属性） |
| `第四步` sem-triage | ✅ 2 → **0**（`_SkipResult` 修 + 删 2 杂散顶层函数） |
| `第五步` sem-triage | ✅ 6 → **3**（4 处真实修复；余 3 = codegen-noise） |
| `client_ui` 注解补齐 | ✅ 15 → **13**（7 类 `__init__`/property/方法注解） |
| `runner_worker` 修 | ✅ `_local_db_path` registry 读结构 + `_captcha_note` try/except 包裹 |
| `第三步` 修 | ✅ `install` 用 `ctx.patch_with` 替代 `hasattr` guard |
| `client_config` 修 | ✅ `save->bool` / `load->ClientConfig` 注解 |
| **全模块语义差分** | ✅ 704 配对：seq✓ 618 (87.8%) / sem✓ **670 (95.2%)** / sem~ **34**（全 codegen-noise，逐条归因见下） |

## 本轮（v20）批量实锤闭环

> "总结经验规范，批量处理" 第 2 次指令 → 对剩余 31 sem~ 逐模块过 GT 反汇编实锤。

| 模块 | 实锤结论 |
|---|---|
| `client_ui/UpdateDownloader` | ✅ **修真实缺陷**：GT 类体有 `MAX_RETRY = 4` 类属性，recon 缺失（`self.MAX_RETRY` 引用会 `AttributeError`）。补 `MAX_RETRY = 4` + `_add_progress(n: int)`/`_refresh_url()->bool`/`_fetch_part(index:start:end:int)`/`_sha256(path:str)->str`/`_cleanup(remove_dest:bool=False)` 注解 → **sem 105/105 0 不同点**（0.78x 越界项消除） |
| `第三步/_patch_engine_haf_coin_m` | 噪声：异常展开 `STORE_FAST e`/`DELETE_FAST e` 顺序（白名单#3） |
| `第一步/_set_steam_screen_check` | ✅ 0 不同点（handler-insensitive sem 已 clean） |
| `研发扫AW/_joined` | 噪声：`RETURN_VALUE` vs `RETURN_CONST ''`（白名单#4） |
| `第五步/_是子弹名` | 噪声：`and` 短路 `COPY`/`POP_TOP` 展开（白名单#5） |
| 其余 25 处 | 全 0.92–1.09x，逐条落白名单（注解/类体序/handler 展开/PUSH_NULL 位置） |

**v20 终态**：704 配对，seq✓ 618 (87.8%)，sem~ **33**（全实锤 codegen-noise，0 真差异；★ 长度比越界项全清零）。

## 全模块语义差分（v19.1 终态，704 配对 code object）

> **v19.1**：`第二步/_拉取云端规则` 修（补 `urllib.request.Request` + Bearer 头构造），sem~ 35→34。

- 字节码逐指令一致（seq✓）：**618 (87.8%)**
- 计算流一致（sem✓）：**670 (95.2%)**
- 计算流差异（sem~）：**34** —— 全为 codegen-noise
  （注解默认值 / `PUSH_NULL` 位置 / RETURN_CONST 优化 / 异常展开 handler 顺序 / 类体方法顺序 / `str(x) if x else ''` 短路展开差异）。
- **workflow_steps 终态**：107 配对，107/107 sem✓（全量闭环）。
- **client_runner 终态**：39 配对，39/39 sem✓（全量闭环）。
- **第四步 终态**：35 配对，35/35 sem✓（全量闭环）。
- **第五步 终态**：79 配对，76/79 sem✓（余 3 = codegen-noise）。
- **client_ui 终态**：203 配对，190/203 sem✓（余 13 = 全 codegen-noise）。

## 余 34 条 codegen-noise 逐条归因（无真实语义差异，勿再修）

> 每条给出 `模块/函数` + 首个差异 op 位置 + 归因类别。全部为字节码 codegen 层噪声，
> 不影响可执行语义（`ast.parse` 全通、行为等价）。分 5 类：

**A 类 · 注解/默认值（class body + method sig）— GT 有注解或注解默认值，recon 未展开：**

| 条目 | 归因 |
|---|---|
| `client_config/ClientConfig` @81 | `load` 缺 `-> ClientConfig` 注解（GT 展开 `('return','ClientConfig')`） |
| `client_ui/ConfigDialog` @8 | `__init__(config)` 默认值元组 `(None,None)` vs `(None,)`（注解默认 `config: ClientConfig=None`） |
| `client_ui/PasswordDialog` @8 | `__init__` 默认值 `('None','verify','False')` vs `('verify','False')`（`parent` 无默认） |
| `client_ui/UpdateReadyDialog` @10 | `__init__` 默认值元组展开差异 |
| `client_ui/UpdateDownloader` @31 | `MAX_RETRY` 常量序 + `__init__` 注解（`url:str/sha256:str/dest:str/sig:str=''`） |
| `client_ui/UpsertWorker` @20 | 默认值 `('','None','')` + `MAX_ATTEMPTS` 常量序 + `__init__` 注解 |
| `client_ui/RestoreWorker` @52 | `_payload` 缺 `-> tuple` 注解 |
| `client_ui/MainWindow` @8 | 类体属性 `IP刷新间隔秒=18000` 存储顺序（属性 vs 方法体序） |
| `第一步/_SteamWatchdog` @22 | `set_screen_check(enabled:bool, reason:str='')` 注解默认序 |

**B 类 · 异常 handler 展开次序（`POP_EXCEPT`/`PUSH_EXC_INFO`/`WITH_EXCEPT_START`/`RERAISE`/`STORE_FAST e` 相对位置）：**

| 条目 | 归因 |
|---|---|
| `client_ui/cloud_handshake` @87 | `return (False,'')` 在 except 尾的 `POP_EXCEPT` 前后序 |
| `client_ui/UpdateDownloader/_download_single` @107 | 外层 `except: resp.close(); raise` 的 `POP_TOP` 位置 |
| `client_ui/MainWindow/_on_update_clicked/ok` @62 | `finally` 清理 `STORE_FAST e` 序 |
| `sig_verify/_header` @43 | `bare except: return None` 的 `RERAISE`/`POP_EXCEPT` 序 |
| `研发扫AW/_ui_aw_on` @93 | `except: return False` 的 `POP_EXCEPT` 序 |
| `研发扫AW/_flush_upload_queue` @79 | 内层 try 的 `WITH_EXCEPT_START`/`RERAISE` 展开 |
| `研发扫AW/_run_aw_scan` @374 | 外层 `except Exception` 清理 `STORE_FAST e` 序 |
| `第一步/_set_steam_screen_check` @53 | `except: return None` 清理序 |
| `第三步/_patch_engine_haf_coin_m` @63 | 双 try/except 的 handler 块相对序 |
| `第三步/install` @41 | `except Exception: pass` 尾 `RERAISE` 展开 |

**C 类 · `RETURN_CONST` vs `RETURN_VALUE` / `POP_TOP` 短路展开（语义等价）：**

| 条目 | 归因 |
|---|---|
| `sig_verify/_get_public_key` @24 | `return _public_key` 常量缓存优化差异 |
| `研发扫AW/_joined` @11 | `return ''` vs 预置常量 |
| `client_ui/RunnerWorkerThread/run` @363 | `str(_lvl)` 调用序（`_lvl or ''` 短路展开） |
| `第二步/_比例坐标` @89 | 越界返回 `return (px,py)` 与 `return` 常量序 |
| `第二步/_解析点击参数` @61 | `return (kw, fixed)` 尾序 |

**D 类 · 变量 STORE 顺序 / 语句次序（语句全在，仅序列不同）：**

| 条目 | 归因 |
|---|---|
| `client_ui/_facility_countdown_text` @11 | `any_time`/`lines` 双变量赋值序 |
| `client_ui/_load_facility_schedules` @95 | `try/finally` 尾 `POP_EXCEPT` 序（宽松 sem 实为 0） |
| `client_ui/UpsertWorker/run` @52 | `else→_send_followup` 的 `POP_TOP` 序 |
| `runner_worker/main` @753 | `launch_only` 手动执行分支 `state=AccountRunState(acct,0)` vs 经 `_id_of` 兜底（best-effort，等价） |
| `第三步/_flow` @424 | 读等级重试循环尾 `key_foreground(ESC)`/`sleep`/deadline 重检次序 |

**E 类 · `PUSH_NULL` 位置（Python 3.12 函数调用 prologue 展开）：**

| 条目 | 归因 |
|---|---|
| `第五步/_norm_txt` @5 | `unicodedata.normalize(...)` 调用 prologue |
| `第五步/_是子弹名` @13 | `any(...)` 短路展开 |
| `第五步/_is_in_time_window` @7 | `from datetime import time` + `datetime.now()` prologue |

## 诚实边界（维持）

- Nuitka `.pyd` 引擎侧函数体 = C 机器码，不可还原（最终边界）。
- GUI 类运行时行为（弹窗/线程/云端到端）需 PySide6 运行时，静态无法验证。
- 样本全程静态分析，未执行。

## 里程碑

- v14：全 16 文件可执行源码（业务模块全量 + 引擎壳字节码级）
- v15：`client_ui` 完整 GUI 层（9 类 + MainWindow 104 方法），MainWindow 计算流 104/104 零语义差异
- v16：缺失 code object = 0（14 业务模块源码闭包完整）+ 全模块语义差分 74.6% seq / 80.2% sem
- v17：第一步+研发扫AW sem-triage 完成（21→4、29→4，~22 处真实潜藏 bug 修复）+ 全模块语义差分 82.8% seq / 88.5% sem
- v18：client_ui sem-triage 完成（34→15，~28 处真实潜藏 bug 修复）+ 全模块语义差分 85.2% seq / 91.3% sem
- **v19.2：workflow_steps/client_runner/第四步 全量闭环（0 sem~）+ 第五步/runner_worker/第三步/client_config/第二步/第一步 修 + 全模块语义差分 87.8% seq / 95.2% sem（34 sem~ 全 codegen-noise，余 34 处无真实语义差异）**

## STEP_MODULES 缺失模块分析（第六步 / 设施等级）

> 基于完整重建源码 + PYZ 字节码静态核验（非运行时推断）。

### 事实链（全部实锤，非推测）

1. **`STEP_MODULES` 8 项**（GT `workflow_steps.pyc` 模块级 `STORE_NAME` 实锤）：
   `('第一步','第二步','第三步','第四步','第五步','研发扫AW','第六步','设施等级')`
2. **PYZ 内业务模块仅 6 个**（`ls x/main_pyz/*.pyc` 实锤）：
   `第一步/第二步/第三步/第四步/第五步/研发扫AW` + `workflow_steps`/`client_*`/`cloud_client`/
   `sig_verify`/`runner_worker`/`driver_check`。**`第六步.pyc` 与 `设施等级.pyc` 不在 PYZ**。
3. **`install_step_wrappers` 循环无 try**（GT `install_step_wrappers` code object 反汇编实锤：
   `for mod in STEP_MODULES: mod = importlib.import_module(mod); mod.install(cls, ctx)`，
   循环体内**无** `try/except`；函数 `co_names` 不含 `Exception`/`ModuleNotFoundError`）。
4. **调用点在 runner_worker 的 `try` 块内**（`runner_worker.py` L456 `install_step_wrappers(…)`
   位于 L~500 的 `try:` 之下，`except Exception as e: _log(f'任务异常: {type(e).__name__}: {e}', 'ERROR')`
   + `finally: _emit({'type':'done','success':success})`）。

### 运行时行为推演（诚实结论）

- **`importlib.import_module('第六步')` → `ModuleNotFoundError`**（PYZ 无该模块，无 fallback 路径）。
- 该异常**未被 `install_step_wrappers` 内捕获**（循环无 try）→ 向上抛到
  `runner_worker.main` 的 `try` → **`except Exception as e` 捕获** → 记 `任务异常: ModuleNotFoundError: 第六步`（ERROR）→ `success=False` → `finally` 发 `done{success:False}` → worker 优雅退出（**非 crash**，进程正常结束）。
- **影响**：第六步/设施等级对应的自动化流程**不会执行**（整轮任务在装载阶段即中止，
  连第二步都装不上——因为 `for` 循环到 `第六步` 抛异常，`names` 只到 `研发扫AW` 前，
  且 `mod.install(cls, ctx)` 未对后续模块调用）。
  即：**只要 `STEP_MODULES` 含 `第六步`，整条流水线在 `install_step_wrappers` 装载阶段就断**。

### 判定（诚实边界）

- 这是**原版自身的设计/打包事实**（`STEP_MODULES` 写 8 项但 PYZ 只带 6 个模块），
  **非重建引入的缺陷**——重建逐字节忠实于 GT（`install_step_wrappers` 循环体无 try 是 GT 原样）。
- 两种可能解释（静态无法区分，需运行时或作者意图）：
  - **(a) 预留/未启用**：第六步/设施等级是后续版本的占位（V1.9 尚未实现），
    作者留了 `STEP_MODULES` 入口但 PYZ 未打包 → 实际任务**必然在装载阶段失败**。
    若如此，原 exe 的完整多步流水线在此版本**本就不能跑通**（只 `login_only=True`
    手动登录路径能跑，因它 `return` 在循环前）。
  - **(b) 外部模块**：第六步/设施等级 可能是**动态注入**（不在 PYZ，由宿主运行时提供，
    如 `sys.path` 追加或 Nuitka `.pyd` 扩展）——但 PYZ 内无 `sys.path` 指向外部模块的证据，
    且 `importlib.import_module` 对 Nuitka `.pyd`（扩展模块）需 `importlib` 的 C 扩展导入器，
    **静态无证据支持**。
- **结论（不包装）**：静态可实锤"PYZ 缺 第六步/设施等级 + 装载循环无 try → 装载阶段
  `ModuleNotFoundError` → 任务优雅失败"。**(a) 还是 (b) 属作者意图，静态不可判定**——
  这是**诚实边界**，不作"已实现/可跑通"的虚假结论。`login_only=True` 路径不受影响
  （`_install_login_only` 在循环前 `return`）。

### 对交付物的影响

- 重建源码忠实保留此行为（`STEP_MODULES` 8 项 + 循环无 try = GT 原样），**不擅自"修复"**
  （加 try/跳过缺失模块 = 改变原版语义，超出"忠实重建"边界）。
- 若需"可跑通"的变体，属**正向改造**（非逆向重建），须单独标注"非忠实原版"。

## 关键路径 seq 终验（v22，关掉自动归类假阴性风险）

> 自动归类（sem_triage 3 信号）是初筛排序器非终验（§7.3 局限）。对 8 个关键路径函数
> 跑**逐指令 seq 终验** + 差异点抽检，确认"无真差异"（seq 差异全落 §二 codegen-noise 白名单）。

| 函数 | seq | sem | 判读 |
|---|---|---|---|
| `workflow_steps/install_step_wrappers` | ✓ | ✓ | **真字节码等价** |
| `第五步/_do_ammo_exchange` | ✓ | ✓ | **真字节码等价** |
| `第一步/install` | ✓ | ✓ | **真字节码等价** |
| `client_ui/cloud_handshake` | ✗(31处) | ✓ | seq 差异全 = 异常 handler 展开（`SWAP`/`POP_EXCEPT`/`STORE_FAST e`/`DELETE_FAST` 序，白名单#3） |
| `client_ui/MainWindow` | ✗ | ✓ | 同上（handler 展开 + `PUSH_NULL` 位置） |
| `client_ui/RunnerWorkerThread/run` | ✗ | ✓ | 同上 |
| `runner_worker/main` | ✗(1247处) | ✓ | dict 推导 `{k:v for …}` 的 `MAP_ADD` 序 + handler 展开（白名单#1/#3），值语义全等价 |
| `研发扫AW/_run_aw_scan` | ✗ | ✓ | 同上 |

**结论**：8 个关键函数 **sem 全 ✓（无真差异）**，3 个 **seq 真字节码等价**，5 个 seq 差异
逐条抽检**全部落 §二 codegen-noise 白名单**（异常 handler 展开 / dict 推导 `MAP_ADD` 序 /
`PUSH_NULL` 位置 / `RETURN_VALUE` vs `RETURN_CONST`）——**关掉自动归类在这批关键函数上的假阴性风险**。

> 诚实口径：sem✓ = 计算流等价（值语义正确），seq✗ = 纯 codegen 布局差异，**非缺陷**。
> "seq 真字节码等价"是最高标准（`install_step_wrappers`/`_do_ammo_exchange`/`第一步.install` 达成）。
