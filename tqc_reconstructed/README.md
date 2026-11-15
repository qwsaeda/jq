# PCTQC_V1.9_20261120.exe 反编译重建源码

从 Nuitka 编译的 exe 中提取 14 个业务模块的 `.pyc`，
经 PyLingual 反编译 + 手工修正 + 字节码语义比对（sem~ 0 目标）重建。

## 模块清单（16 个）

| 文件 | 说明 |
|---|---|
| `第一步.py` | 登录/Steam 检测/窗口遮罩 |
| `第二步.py` | 云端规则拉取 + 点击参数解析 |
| `第三步.py` | 游戏引擎 haf_coins 补丁 |
| `第四步.py` | 设施/交易行准备 + 模式B 跳过流程 |
| `第五步.py` | 子弹兑换 + 弹药交换 + 时间窗口 |
| `研发扫AW.py` | 研发扫描 AW 上传 |
| `workflow_steps.py` | 步骤工作流编排总管（107 code objects） |
| `client_ui.py` | PyQt5 主界面（203 code objects） |
| `client_runner.py` | 多账号串行运行器 |
| `client_config.py` | 配置持久化 |
| `cloud_client.py` | 云端 API 客户端 |
| `config.py` | 顶层配置常量 |
| `driver_check.py` | 驱动/环境检查 |
| `diagnostics.py` | 诊断日志 |
| `runner_worker.py` | worker 进程入口 |
| `sig_verify.py` | 签名/证书验证 |

## 质量指标

- `ast.parse` 全部 0 警告
- 字节码一致 615/704（87.4%）
- 计算流差异（sem~）42 处（多为注解/类体顺序等 codegen 噪声）
- 缺失 code objects：0
- 重独有 code objects：4（client_ui 中 PyLingual 误拆）

## 限制

- Nuitka `.pyd` 函数体（runner.pyd 等）为 C 机器码，不可恢复
- 第三方库未反编译（见 `LIBRARY_NOTES.md`）
- 部分 PyLingual 输出有 cflow 断裂，已按 GT 语义 dump + 完整反汇编手工重建
