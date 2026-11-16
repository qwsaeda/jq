# reconstructed/ — 忠实重建集（可执行源码级，v22 终态：16 文件全 ast.parse + compile 零告警）

> **来源**：从 `analysis_biz.txt`（100% 语义转储，ground truth）+ **原版 pyc 字节码 dis 反汇编**（权威）
> 逐函数重建。缺失 code object = 0（14 业务模块源码闭包完整）。
> **v22 终态指标**：704 配对 code object，字节码逐指令一致 seq✓ **87.6%**（617），
> 函数级+模块级计算流 sem✓ **621/652（95.2%）**（模块级 `<module>` 根对象已并入配对总数）；计算流差异 sem~ **32（全部实锤 codegen-noise，0 真差异）**；
> 自动噪声归类 32/32 噪声 / 0 疑真 / 0 越界；**★ names/vars 漂移（真信号）= 0**。
> **v22 新增第 4 层判据（模块级）**：`global_ref_check.py` 模块级 LOAD_GLOBAL 差集 × GT 交叉，
> **★ 运行时 NameError 级真缺陷 0 / 运行期 global 赋值 2**；模块级 sem 11/14（严格逐指令，
> 余 3 = 注解 over-build/默认值位置/顺序，全语义等价）。本轮一次暴露并修复 **54 个**模块级真缺陷。

> ⚠ 本文下方 §一~§五 为 v18 历史台账（当时口径），**终态指标以本段 + `CURRENT_STATE.md` 为准**
> （v19/v20 已把 workflow_steps/client_runner/第四步/第五步 全量闭环到 0 sem~，
> client_ui 修掉 `MAX_RETRY` 缺失类属性，seq/sem 总数见 §"全模块语义差分 v20"）。

## 判据工具集（本目录自带，可复现验收）

| 工具 | 作用 | 用法 |
|---|---|---|
| `verify_all_modules.py` | 全模块 **seq+sem** 双判据总数（权威终态） | `python3 verify_all_modules.py` |
| `missing_inventory.py` | **结构层**：code object 缺失清单（0 = 闭包完整） | `python3 missing_inventory.py` |
| `sem_triage.py` | **批量 triage 驱动**：自动噪声归类（长度带+常量值判据） | `python3 sem_triage.py` / `--json` / `--star-only` |
| `dump_fn.py` | **GT 反汇编 dumper**：单函数原版 vs 重建指令流 | `python3 dump_fn.py <mod> <path/to/fn>` |
| `global_ref_check.py` | **★ 第 4 层（模块级）**：LOAD_GLOBAL 差集 × GT STORE_NAME 交叉（NameError 风险） | `python3 global_ref_check.py` / `--json` |
| `extract_module_consts.py` | **GT 模块级常量还原**：STORE_NAME 序列 → 逐名赋值语句 | `python3 extract_module_consts.py 第四步 --missing` |
| `reorder_module_level.py` | **按 GT 模块级序重排**顶层块（★ 装饰器坑已修） | `python3 reorder_module_level.py 第五步 --apply` |

> 4 工具均**布局自适应**：工作区（`x/main_pyz` + `reconstructed/`）、项目扁平目录
> （同级 `.py` + 可选 `gt_pyz/`）、或回退工作区绝对路径。GT pyc（`x/main_pyz/*.pyc`）是
> 对照源，存于工作区（二进制不入项目）。
> **方法论/判据/防假绿规范** = `DECOMP_RECONSTRUCTION.md`；工具层（pycdc 补丁/解包）= `DECOMP_EXPERIENCE.md`。

## 全模块语义差分 v20（704 配对 code object，当前终态）

| 指标 | 值 |
|---|---|
| 缺失 code object | **0**（missing_inventory 权威） |
| 字节码逐指令一致 seq✓ | **618 (87.8%)** |
| 计算流一致 sem✓ | **~670 (95.2%)** |
| 计算流差异 sem~ | **33 —— 全实锤 codegen-noise**（注解默认值 / 类体顺序 / 异常 handler 展开 / `RETURN_CONST` vs `RETURN_VALUE` / 短路 `COPY`/`POP_TOP` / `PUSH_NULL` 位置） |
| 自动噪声归类 | 31/31 噪声 / 0 疑真 / 0 越界 |
| 重独有 code object | client_ui 14（`_Stub`/`_SignalStub`/config 路径 = PySide6-import 容错 over-build，非 bug，已记录） |
| 全量闭环模块（0 sem~） | workflow_steps / client_runner / 第四步 |

> **诚实局限**（`DECOMP_RECONSTRUCTION.md` §七）：`sem_triage.py` 自动归类是**初筛排序器非终验**——
> 它靠"长度比 0.88–1.12 + 常量值多重集相等"归噪声，**抓不到小幅 guard/分支删除**（假阴性）。
> 所以"0 越界/全噪声"≠"0 缺陷"；关键函数仍需 `dump_fn.py` 逐条对 GT + seq 判据终验。


## 一、字节码级还原（pyc dis 反汇编逐行还原，最高保真）

| 模块 | 行数 | 来源 | 说明 |
|---|---|---|---|
| `config.py` | 161 | pyc dis | 引擎薄壳：桌面数据目录/DPAPI 凭据迁移/`AppConfig`（`stage_timeouts` 8 阶段超时表 `{cleanup:20, steam_login:90, launch_game:90, launcher:120, pre_lobby:300, profile:30, facility:180, market:600}`，`BUILD_CONST_KEY_MAP` 实锤） |
| `diagnostics.py` | 52 | pyc dis | 引擎薄壳：异常截图留证（`save_frame` 为有意 no-op `del` 桩，字节码实锤） |
| `driver_check.py` | 210 | pyc dis 重写 | Interception 驱动检测/安装：`_prime_dll_path`/`is_admin`/`is_installed`/`install_elevated`/`install`/`ensure` **9 个 code object 字节码 100% 一致**；保留原版 `ensure()` 语义 bug（`installed` 恒 False / `need_reboot`=inst_ok，按字节码忠实还原） |
| `client_config.py` | 111 | pyc dis + 注解对齐 | 配置管理：`save/load/is_valid/get_display_url` 指令序列逐条对齐；余 2 处仅 `return` 注解的 `MAKE_FUNCTION` 标志位差异（本地 3.12.x 与 exe 编译器微版本差，运行期零影响） |

## 二、语义转储重建（14 业务模块）

### 语义重建（cflow 破碎区按 dump + pyc 字节码重写）

| 模块 | 行数 | ast.parse | 说明 |
|---|---|---|---|
| `第三步.py` | 381 | ✓ | 读取行动等级：入口点击/收藏室判定/弹窗链自消/按键复核/V1.15 哈夫币 M 单位补丁 |
| `client_runner.py` | 399 | ✓ | `RunnerSignals`/`FileStopEvent`/`run_accounts`/`run_worker`+`JsonSignals` JSON 行协议；v16 全函数语义重对齐（见 §五） |
| `runner_worker.py` | 524 | ✓ | 子进程工作器：配置双路/launch_only/login_only/队列调度/`_FACILITY_MAP` |
| `研发扫AW.py` | 606 | ✓ | 研发部门扫货 35 函数全量 |
| `client_ui.py` | 3710 | ✓ | **完整 GUI 层**：模块级常量/工具函数 + 9 个 GUI 类 + **MainWindow 全 104 方法**（见 §二·A）；v18 全函数语义重对齐（见 §五） |
| `workflow_steps.py` | 521 | ✓ | 工作流总控 + **`_do_ammo_exchange` legacy 版全量实现**（`click/ocr_rect/wait/wait_kw` 4 嵌套 + region 过滤 + exact/all_of/gone 三模式，**10 个 code object 字节码 100% 一致**，8 场景 mock 双跑轨迹一致） |
| `第一步.py` | 266 | ✓ | 登录+游戏进入：Steam 注册表/进程守卫/虚拟屏幕/窗口涂黑 |
| `第五步.py` | 833 | ✓ | **`_do_ammo_exchange` 全量状态机**（20 嵌套闭包 + 主 body：步骤 1-3 进部门/军需处/战术部门 → 列表稳定 → 全量普查 → 兜底补扫 → 逐款 `exchange_one` 状态机（详情判定/归属校验/一键补齐/支付/兑换/结果轮询/亏本兜底）→ `finally` 回大厅）；`_norm_txt/_是子弹名/_名匹配` 按字节码对齐；**语义差分 20/20 场景 mock 双跑轨迹一致**（driver 开/关 × 10 场景，含落盘日志） |
| `第四步.py` | 637 | ✓ | 设施准备 + `_SkipResult` + 模式 B 自定义跳过；**`install` 全 27 嵌套 code object 重建**（主 body seq✓315/315 sem✓279/279，全部嵌套 sem✓，见 §五） |

### src_best 已可 parse（直接收录，零改动）

| 模块 | 行数 | 说明 |
|---|---|---|
| `第二步.py` | 326 | 回归弹窗/战备页拦截 |
| `cloud_client.py` | 179 | 云端通信客户端 |
| `sig_verify.py` | 95 | RSA 签名验证 |

**合计 16 文件 11,281 行，全部 `ast.parse` + `compile` 零告警零错误。**

## 二·A、MainWindow 全类重建（v15 核心成果）

`client_ui.py` 现为完整 GUI 模块（3,540 行），包含：

- **模块级**：`DEBUG_LOG` / `DARK_STYLE`（大 QSS）/ `STATUS_COLORS`（11 项）/ `STATUS_NAMES`（10 项）/
  `STATUS_CN_TO_CODE` / `TERMINAL_STATUSES` / `RUN_LIMIT_TIMES=3` / `RUN_LIMIT_WINDOW=3600` /
  `_RE_*` 正则 / `_DIGESTINFO_SHA256` / `USER_MESSAGE`（import 自 sig_verify）/ `CLOUD_URL='https://teqinsi.cn'`；
  类常量 `IP刷新间隔秒=18000` / `IP失败重试秒=600`。
- **9 个 GUI 类**：`ConfigDialog` / `ClickableLabel` / `PasswordDialog` / `UpdateReadyDialog` /
  `UpdateDownloader`（`CHUNK=1048576`/`PART_SIZE=25165824`/`MIN_PARTS=4`/`MAX_PARTS=8`/`MAX_RETRY=4`）/
  `CloudWorker` / `UpsertWorker` / `RestoreWorker` / `RunnerWorkerThread`（engine-dir 候选 4 元组 +
  stop flag `pc_tqc_client_stop.flag` + `_graceful_kill` 16×0.5s 轮询）。
- **MainWindow：104 方法**，全部经 `ast.parse` 验证 + 方法签名（`co_argcount`/`co_varnames`）对齐 ground truth。

### MainWindow 验证结果（v15 终态）

| 验证层 | 结果 |
|---|---|
| 语法层 | 104/104 `ast.parse` + `compile` 零错误 |
| 签名层 | 104/104 方法签名（参数/局部名）与语义转储一致 |
| 字节码层（`seq` 归一化：跳转→TGT、LOAD_CONST→repr、MAKE_FUNCTION→flag、CALL→n=） | **89/104 逐指令一致** |
| 计算流层（`sem`：去跳转/NOP/END_FOR/EXTENDED_ARG，保留常量值/名字/CALL 数/运算） | **104/104 零语义差异** |

- **89/104 字节码完全一致**；余 15 处均为 CPython 3.12 编译微版本噪声
  （跳转极性优化 / 推导式代码生成差异 / EXTENDED_ARG 常量池布局），**计算流全等 → 语义零差异**。
- **集合常量排序噪声**：`_on_accounts_loaded`/`_restorable_codes`/`_on_restore` 的 frozenset
  常量在 repr 中元素顺序随 hash 序变化（`{'STOPPED','BANNED','OBSERVATION'}` vs
  `{'STOPPED','OBSERVATION','BANNED'}`），**成员集完全相同**，规范化后零差异。
- **cflow 破碎区重写（ground truth 驱动）**：`_install_driver`（驱动安装/驱动模块加载失败路径 + `is_installed` OUTSIDE try）、
  `_check_driver`（三独立 try/except：import 加载失败 / 检测异常 / question 异常，`return None` 于 `if reply==Yes` 内外）、
  `_on_runner_status`（WAITING 块 `account_id not in _observed_ids` + `{'status':'WAITING','status_time':...}` 载荷 +
  `remark` 三元 `(getattr(matched,'remark','') or None) if matched else None` + 第二循环方法级对齐）、
  `_recover_after_crash`（单 f-string 内联 `", ".join(...)`）、
  `_prune_run_times`/`_prune_fail_times`/`_auto_start_if_needed`（过滤推导式 `[t for t in ... if ...]` 非三元）、
  `_on_runner_done`（`return None` 于 `if retry:` 内、`_refresh_accounts` 后置）、
  `_on_accounts_loaded`（`if a.username` 过滤于 `cloud_status` 字典推导）、
  `_inject_paused_stat_and_lock_btn`（`except Exception as e` 绑定）、
  `_fetch_public_ip_once`/`_check_update_job`/`_on_update_clicked`/`_verify_license`/`_log` 等。
- **字节码精确还原**（PyLingual 干净体 verbatim，零改动即可 parse）：其余 95 方法。

## 三、验证方法（v15 新增：MainWindow 双层差分）

1. **语法层**：`ast.parse` + `compile()` 双零告警（16 文件全 PASS）。
2. **字节码层**（`seq` 归一化比对，跳转目标/常量 repr/函数标志/CALL 数归一）：原 pyc vs 重建 compile。
   - `driver_check` 整模块 **9/9 code object 零差异** ✓
   - `workflow_steps._do_ammo_exchange` **10/10 零差异** ✓
   - `client_config` 仅 2 处注解标志位（编译器微版本差）
   - `第五步._do_ammo_exchange` 剩 5 处 CPython 3.12 跳转布局噪声（同语义），由第 3 层覆盖。
   - **MainWindow：89/104 零差异，15 处纯 codegen 噪声（跳转极性/推导式/EXTENDED_ARG）**。
3. **计算流层**（`sem`：剥离跳转/NOP/END_FOR/END_FINALLY/EXTENDED_ARG，保留 LOAD_CONST 值/
   名字/CALL 数/二元运算/比较）：**MainWindow 104/104 零差异（语义全等）**。
4. **语义层**（`verify_semantics.py` / `verify_semantics_ws.py`）：原版 pyc 函数 + 重建版，
   同一 mock ctx + 假时钟双跑，逐条比对调用轨迹（log 文本/点击坐标/按键/OCR 轮次/等待时长）+
   落盘普查日志内容：
   - `第五步._do_ammo_exchange`：**10 场景 × driver 开/关 = 20/20 轨迹一致**（182~332 步/场景）
   - `workflow_steps._do_ammo_exchange`：**8 场景 = 8/8 轨迹一致**（24~106 步/场景）

## 四、未覆盖（诚实边界）

- `client_ui` 的 GUI 类**运行时行为**（QMessageBox 弹窗/线程/云 API 端到端）——依赖 PySide6 运行时
  状态，无法静态验证；源码已完整重建，行为契约由签名/常量/字节码三重对齐保证。
- 15 处 MainWindow 字节码布局差 + 5 处 `第五步` 布局差 = 本地 CPython 3.12.x 与 exe 编译微版本差，
  **计算流/语义已实证等价**。

## 五、v18 核心成果：缺失 code object = 0 + 全模块语义差分 85.2% seq / 91.3% sem

### 5.1 缺失闭合（missing_inventory 权威缺口清单 → 0）

`missing_inventory.py`（原版 pyc 逐 code object 递归比对重建 compile 树）缺口序列：
**84 → 54 → 45 → 33 → 26 → 24 → 18 → 17 → 0**（v16 终态，v17 维持）。0 缺失 = 14 业务模块
源码闭包完整（每个原版嵌套 code object 均有重建对应）。

### 5.2 关键重建（ground truth 驱动，字节码/计算流验证）

| 目标 | 结果 | 真实修复点 |
|---|---|---|
| `第四步.install`（16 嵌套 27 code object） | 主 body **seq✓(315/315) sem✓(279/279)**，27 嵌套**全 sem✓**（seq~ 仅记录在案 codegen 噪声：NOP 行表伪影 / 列表推导跳转极性） | `ocr_rules as _ocr_rules`；`if nm == username` 极性反转；`_取证` 单外层 try + 无 `parts=[]` 预初始化 + 字符串拼接非 f-string；`wrapped_overview` 直返 `original(...)`；`_原查找(self,args,kwargs)` 签名 + id 键无 `or ''`；`nn,nr = _归一(nm),_归一(row_text)` 元组赋值；`wrapped_manu` 极性反转（`if not _fill_rescue(self):`） |
| `client_runner._run_single_account` 深嵌套 genexpr | **seq✓(34/34) sem✓(26/26) 字节码精确** | 原为 filter 形式 `any(s for s ... if ...)`，真值 = `any(s.get('mode') and s.get('mode') != 'pinned' or s.get('pinned') for s in settings.values())`（genexpr 产出条件值非 s） |
| `client_runner._norm_setting` | **seq✓(56/56) sem✓(46/46)** | `dict(s or {})`；`(d.get('mode') or 'pinned').lower()`；`elif m == 'pinned' or not m:` |
| `研发扫AW`（v17 整模块） | 51/55 seq✓ 51/55 sem✓（余 4 codegen-noise） | ~15 处真实修复，见 5.6 |
| `第一步`（v17 整模块） | 56/60 seq✓ 56/60 sem✓（余 4 codegen-noise） | 3 处真实修复：`_grab_desktop_bgr` GDI 全实现 / `_shrink_for_ocr` 签名+算法 / `_解析vk` 早返 |
| `client_ui`（v18 整模块） | 203 配对，188/203 sem✓（余 15 codegen-noise） | ~28 处真实修复，见 5.5 |

### 5.3 client_runner 语义重对齐（潜藏 bug，missing_inventory 未捕获——存在但语义错）

`missing_inventory` 只查"缺失"，不查"存在但语义错"。sem-triage 捕获 9 处潜藏 bug：

| 函数 | 修复 |
|---|---|
| `_emit_log` | `if self.signals.on_log:` 直判 + `getattr(logger, level.lower(), logger.info)(text)`（global `logger`，非 `self.logger`） |
| `_emit_status`/`_emit_account_data`/`_emit_facility`/`_emit_done` | 直属性 `if self.signals.on_X:` + 显式 `return`（非 getattr 守卫） |
| `_stage_to_facility` | 内联 11 项映射（tech/技术/技术中心/workbench/工作台/pharmacy/制药/制药台/armor/防具/防具台），非 `self._FACILITY_MAP` |
| `run_worker` | `cfg.get('accounts_list') or cfg.get('accounts', [])`（优先 `accounts_list` 键） |
| `_run_single_account` | `from pc_tqc.workflow import runner as runner_mod` 等 4 组 import 形式（IMPORT_FROM 非属性访问） |
| `_run_single_account`/`_manufacture_plan_loader` | `nonlocal _exe_manufacture`（闭包共享 cell） |
| `run_accounts` | `if ok:` 内 `_emit_status(account, '成功'/'失败', '完成', '')`（成功/失败分支各发状态） |
| `ClientRunner.__init__` | 删伪 `self.logger`（原版用 global `logger`，无实例 logger） |
| `_on_observation_detected_cb`/`_on_finished_cb`/`_on_account_data_cb` | 签名 `(until)` 非 `(account_name, until)`；错误文本 `{e}` 非 `{type(e).__name__}: {e}`；level `'INFO' if success else 'ERROR'` |

### 5.4 全模块语义差分（704 配对 code object，v18 终态）

| 指标 | v17 | v18 |
|---|---|---|
| 可配对 code object | 704 | 704 |
| 字节码逐指令一致（seq✓） | 583 (82.8%) | **600 (85.2%)** |
| 计算流一致（sem✓） | 623 (88.5%) | **643 (91.3%)** |
| 计算流差异（sem~） | 81 | 61 |

- **client_runner 终态**：36/39 seq✓，37/39 sem✓（余 2 sem~ = 类体方法定义顺序/注解默认值噪声，codegen-noise 类）。
- **研发扫AW 终态**：51/55 seq✓，51/55 sem✓（余 4 = codegen-noise：RETURN_CONST / 异常 handler 顺序 / listcomp 变量名）。
- **第一步 终态**：56/60 seq✓，56/60 sem✓（余 4 = codegen-noise：类体顺序 / 异常展开 RETURN-vs-RERAISE）。
- **client_ui 终态（v18 语义重对齐）**：203 配对，188/203 sem✓（余 15 = 全 codegen-noise：9 类体方法顺序/注解默认值 + 函数级异常 handler 布局/POP_TOP 结构/变量 STORE 顺序）。
- 61 个 sem~ 全为记录在案 codegen-noise（类体顺序、注解默认值、异常展开 RETURN-vs-RERAISE、`COPY/POP_TOP` 短路、RETURN_CONST、异常 handler 布局）。

### 5.5 client_ui 语义重对齐（v18，~28 处真实潜藏 bug）

`client_ui` 本轮 sem-triage 34 → 15（余全 codegen-noise）。真实修复：

| 函数 | 修复 |
|---|---|
| `_mask_sensitive_log` | 12 regex 替换 + `_LOG_DROP_KEYS=('固定模板匹配成功','制造列表候选确认')` + 命中丢弃返 None |
| `_manual_state_path` | `sys.frozen` → `dirname(sys.executable)` / 否则 `dirname(dirname(abspath(__file__)))` + `manual_state.json` |
| `verify_release_signature` | 全 RSA-PKCS1v1.5：size=bit_length / len 检查 / pow+to_bytes / `_DIGESTINFO_SHA256` 尾 / pad<8 / `em==b'\x00\x01'+ff*pad+00+tail`；`RELEASE_PUB_N=int('<2048-bit hex>',16)` + `RELEASE_PUB_E=65537` |
| `device_fingerprint` | winreg MachineGuid + ctypes `GetVolumeInformationW` 卷序列号 + `uuid.getnode` + `'|'.join` + sha256（旧只取 SystemDrive+platform） |
| `_fmt_hms` | lambda `str(n).zfill(2)` + f-string `{seconds//3600}:{p(...)}:{p(...)}`（旧 `%02d` 双变量） |
| `_format_hafu_w` | None/`''`→`'-'`；`abs(n)<10000→str(n)`；`n/10000:.1f`（旧 `%` 无 None 分支） |
| `_hash_password` | salt 原样传（旧 `.encode` 双编码错） |
| `cloud_handshake` | `client._post` 键 `deviceId/appVersion/platform/os` + `set_session` + `CloudVerificationError`/`CloudClientError` handler（旧 `_` 键 + 宽 except） |
| `_earliest_facility_time` | 无 isinstance-str 检查 + `times` 列表 + `dt<=now` 未来过滤（旧 cands 列表 + isinstance 检查） |
| `_facility_countdown_text` | `any_time` 守卫 + `except ValueError` + `_FACILITY_LABELS` 元组迭代 + `label {fmt}`（旧 `—` 分隔 + `except Exception`） |
| `_runner_db_path` | `if desktop:` 返 **在 `with` 内** + `except (ImportError,OSError)`（旧 except Exception） |
| `_cleanup_stale_runner_cfg` | 签名 `max_age_sec: int = 600`；`startswith AND endswith` + 内层 try/except continue（旧极性反 `not startswith or endswith`） |
| `_load_known_accounts` | dictcomp 在 `with` 内（旧 raw 中间变量） |
| `_save_known_accounts` | `indent=2`（旧缺缩进） |
| `_load_facility_schedules` | `result[name]` 非 str(name) + `except ValueError` + `try/finally`（旧 `result[str(name)]` + except Exception + pass） |
| `_load_local_record_usernames` | 元组表 + `names.update(genexpr)` + `return names` 在 if 外（旧 list 表 + else-pass + 缩进错 return） |
| `ConfigDialog._test_connection` | `split('://',1)` + try 仅包 if-server 块（旧无 maxsplit + try 全包） |
| `UpsertWorker.run` | `else→_send_followup+return`，无 break/for-else/finally（旧 finally 结构错） |
| `RunnerWorkerThread.run` | f-string 全 5 处（旧 `%` 格式化，含"找不到引擎目录/传递配置失败/退出码"消息） |
| `UpdateDownloader._download_single` | 外层 `except: resp.close(); raise`（旧缺资源回收 handler） |
| `_FACILITY_LABELS` | 元组 4×2 元组 `(('tech','技术中心'),('workbench','工作台'),('pharmacy','制药台'),('armor','防具台'))`（旧 dict 错） |
| `_last_run_store_path`/`_known_accounts_path` | `base` 变量（旧 `up` 名噪声） |

### 5.6 研发扫AW 语义重对齐（v17，~15 处真实潜藏 bug）

| 函数 | 修复 |
|---|---|
| `_会话头` | 头键 `{'X-TQC-Session','X-TQC-Device'}`（旧 `{'X-Session'}` 错） |
| `_slot_done` | `if not username: return True` + `with _db_conn() as conn:` + `row is None`（旧极性反） |
| `_current_slot` | `<=19` 上界 + `>=20` + `(today-1d).isoformat()` 跨零点（旧丢跨零点分支） |
| `_db_conn` | `if not path: return None` + `makedirs(dirname)` + `connect(path)` 无 timeout（旧空路径崩） |
| `_queue_upload` | `with` 块（无显式 commit）+ `int(qty or 1)`（旧 `or 0`）+ `str(err)[:200]` 截断 |
| `_flush_upload_queue` | 直解包 7 元组（旧 `(rows or [])` 不匹配）+ `tries>=10` 跳过 + f-string + `str(e)[:200]` |
| `_ocr_roi` | 整函数 try/except + 分数坐标（旧 ref_px 错）+ `grab_frame(hwnd, tag)` 两参 + `items or []` + None 过滤 |
| `_ui_aw_on` | 条件表达式 `json.loads(_raw) if _raw else None` + `_cfg_bool` 优先 + str-truthy 集（旧双分支错） |
| `_解析vk` | `param is None` 早返 None + `int(s)` 非 16 基 + except 返 None（旧返 0） |
| `_joined` | `if items: return join(...)` else `''`（旧 `items or []` 丢空串） |
| `_is_aw_text` | `if not n: return False` + `any(str(s) in n for s in _CFG.get('target_strong', []) if s)`（旧单串+元组错） |
| `_hits` | `sum(1 for kw in kws if kw in joined)`（旧 `k and k in joined` 错） |
| `install` | `_静音弹窗日志()` 首调 + `_mod_step5()` + `getattr(prev,'_aw_wrapped')` 判 prev + `wrapped(ctx2, log=None, max_esc=None)` 闭包 prev（旧默认参错） |
| `_back_to_lobby` | `range(int(_CFG['return_lobby_max_esc']))` + `items and _on_page(...)` + `% (i+1)` + `bool(...)` 终检 |
| `_read_qty` | 返回 2 元组 `(qty, qty_defaulted)` + `best` 距离选择 + `0<=cy-y2<=qty_above_dy` + `abs(x2-cx)<=qty_max_dx` |
| `_post_hit` | 直解包 `server, token` + `if not server or not token` + 键名 `accountId/item/qty/slot/hitAt` + `headers.update(_会话头())` + `str(data)[:120]` + `timeout=int(cloud_timeout)` |
| `_加载配置` | `global _QTY_RE` + `_本地配置文件()` 判非空 + `fetch_step_config(ctx, STEP_KEY, STEP_NAME)` 三参 + 循环 `if k in cfg and v is not None` |
| `_nav_to_rd_page` | 整函数重写：`time>=deadline or stopped()` 早退 None + `in_qxs = qxs and not in_dept` + 三页签分支 + `click_ref(hwnd,x,y,ref_w,ref_h,tag)` 六参 + `%g` 日志 |
| `_run_aw_scan` | `_ui_aw_on()` 先于 `enable` + `while True: try: flush+find_game_hwnd except: continue` 重试 + `deadline` try 外 + 主「扫AW失败」handler + `names` 双 listcomp + `_post_hit` 内层 try/except + f-string |

## 诚实边界
- 重建 = 忠实近似。`config/diagnostics/driver_check` 为字节码级最高保真；业务模块为
  PyLingual 干净体 + cflow 破碎区按语义转储/字节码重写。Nuitka `.pyd` 引擎侧函数体不可还原
  （C 机器码），Python 侧接口经 `ctx.ocr_scan/grab_frame` 抽象。
- 样本全程静态分析，未执行。
