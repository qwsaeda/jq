# rust-anything PoC：原版 32 位 so 行为 → Rust 移植的可验证闭环

> 2026-11-20。用户发文章 `rust-anything`（github.com/erkamyaman/rust-anything）后选"引入工具做 PoC"。
> **核心纠正**：对齐基线必须是**原版 32 位 so 的行为**，不是正向 C 源码（重写证明重写无意义）。

## 工具结论
- 安装：`npm install -g rust-anything`（v2.1.0，/usr/bin/rust-anything）；cargo 需在 PATH。
- 流程：`analyze → bridge → record → port → build → diff → bench → gate`。
- **局限**：C 模式只能录制"能编译的 .c 源码"→ .so（ctypes，逐 case fork），**无法直接录制 stripped 32 位 ARM .so**。故行为载体 = `rev64/reverse/src_jni_translated/` 的逐字节逆向翻译体（逐条反汇编自原版 VA，如 `ai_score` 对 0x39e14 等）。

## PoC 实测（隔离目录 /tmp/opencode/ra32-poc，未碰工作区源码）
| 步骤 | 结果 |
|---|---|
| analyze 逆向翻译体 `jq_rs_rules.c` | 12 exports / 1 bridgeable（`ai_score`，纯 int→int 无指针）；其余带 `jq_state*` 被正确标记 stays-in-C |
| 最小自包含 unit（ai_score + rank_norm + 常量，逐字节照搬翻译体） | 解决单文件编译缺 `is_rail` 等符号 |
| record | **154 cases**（含 INT64_MAX/MIN、i32 边界等语义陷阱），0 threw |
| build + diff（Rust 忠实移植） | **154/154 cases match** |
| bench | ai_score 0.73x（小函数逐次调用桥接开销 > 计算，印证文章"小函数直接迁不划算"） |
| gate | **Gate passed**（fmt/clippy/test/unsafe/behavior 全 pass） |
| **负向测试**（注入 Rust 独有的 -2 空源哨兵 = R-057/R-092 已知分叉） | diff **精确检出 8+ 处分叉**（`ai_score(0,*)` expected vs got -2），证明非假绿；恢复后 154/154 |

## 本项目价值判断（诚实）
- ✅ 能证明"Rust 移植 vs 原版 32 位行为"逐 case 一致，且能检出真实分叉——比我们现有 `rule_probe_compare.py`（36 case）case 更全（语义陷阱自动注入）。
- ⚠️ 只覆盖**纯函数**（如 ai_score）。核心规则（jq_can_move/jq_try_move 带 `jq_state*` 指针 + 32 位内存模型）工具明确 stays-in-C，bridge 不了——这类仍需现有 `rule_probe_shared.inc` 同源探针 + arm32_walkregress 路径。
- ⚠️ 每轮需最小化 unit（翻译体单文件编译缺依赖符号），有一次性成本。
- 结论：**可作为现有判据体系的有力补充**（尤其纯逻辑函数的行为锁定），不宜视为"替代整个验证体系"。PoC 已在隔离目录完成，未改动仓库任何源码。
