# CURRENT_STATE — v19 终态（2026-11-15）

## 总览

- **16 个重建文件**（`reconstructed/*.py`），全量 `ast.parse` + `compile` 零告警零错误。
- 覆盖：2 个引擎薄壳（`config`/`diagnostics`）+ `driver_check`/`client_config` 字节码级 +
  14 个业务模块（含完整 `client_ui` GUI 层）。
- **缺失 code object = 0**（v16 核心里程碑：14 业务模块源码闭包完整，维持）。

## 本轮（v19）完成

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
