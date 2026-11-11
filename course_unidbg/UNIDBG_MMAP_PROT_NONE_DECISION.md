# UNIDBG_MMAP_PROT_NONE_DECISION — §124③ 288MB PROT_NONE mmap 定点实验（决策日志）

> **2026-11-11，方向 A 对抗结论（子代理 ses_e36d942b4ffdQaTSIqr2r6wJdo）**：
> 看雪《unicorn 有效的检测方案，以及魔改的对抗方案（unidbg 为什么补好了环境却还是不能用）》
> （黑色渡鸦，2026-09）的"魔改 unicorn + trace + 信号注入回调"**不直接适用**本工作区
> EABI5 ARMv5T 32 位军棋 so——该文检测点全为 ARMv8/aarch64 特性（PAC/LDP/CASP/EL0/CNTVCT 时序），
> 无一条对应 32 位 EABI5；且 §124y-152 已做过"实用等价"（objdump 24512 BL fix + syscall hook +
> init_array |1 处理），撞的是 C++ 对象图语义墙（.bss 非零率 0.2% vs MuMu 79.1%），非 unicorn 解码精度可修。
> **方向 A 维持现状（iv）**，唯一残留小实验 = §124③ 288MB PROT_NONE mmap 定点补环境（本文档）。

## 1. 对抗验证要点（为何不魔改 unicorn）

| 看雪文章检测点 | 适用本 so？ | 理由 |
|---|---|---|
| PAC 签名位检测 | 否 | ARMv8 特性，EABI5 ARMv5T 无 PAC |
| LDP 同目标 CONSTRAINED UNPREDICTABLE | 否 | ARMv8-LSE2 特性，32 位无 |
| CASP 奇数寄存器 | 否 | ARMv8-LSE 特性，32 位无 |
| EL0 越权边界 | 否 | 32 位无 EL0/EL1 概念（用户态=ring3） |
| CNTVCT 时序差 | 否 | 时序指纹，非军棋卡点（军棋无 anti-emulator 检测——.text/.rodata 明文非壳） |

→ 文章 5 类检测点 **0 条对应** 军棋 32 位 so。其"消除 unicorn vs 真 ARM Load/Store 行为差异"
是通用营销语，未实证对应我们的 UC_ERR_INSN_INVALID/READ_UNMAPPED 具体失败模式。

**根因归属（§124y-152 已钉死）**：
- 0.2% vs 79.1% 的 .bss 非零率差 = **C++ 对象图语义缺失**（模拟器无真实 C++ 运行时构造
  this[8] 列表头 + node+0xc next + this[0x24]/[0x34] 字符串），**非 unicorn 解码精度 bug 可修**
- §124y-158/168 决定性实验已闭合：原版 .bss 事件表 289B = 17 行 × 17 字段 = 全 0x01 掩码
  （高 nibble 全 0 = 事件码 0）→ **disc=0 是忠实语义，非未拿到值**
- §124y-151：scudo 堆指针 = 进程本地堆基址（跨进程回填四路不可行，F_c0ed0 析构死循环）

## 2. 残留小实验：288MB PROT_NONE mmap 定点

### 2.1 现状（CORE_CHALLENGES §124③ 实锤）

军棋 32 位 so 内有 **288MB 的 PROT_NONE 段**（保护段/大页占位，可能是内存布局探针或
C++ 全局数组预留）。unidbg 的 `mmap2` mock 处理失败 → OnCreate 推进卡住（§124③）。

### 2.2 实验设计（低 ROI，非主线）

| 项 | 设计 |
|---|---|
| 入口 | `course_unidbg/UniBlFix.java`（§124y-152 已挂 `__mmap2`/syscall hook）扩展允许 288MB PROT_NONE 大页 |
| 改动 | `__mmap2` hook 识别 `len≈0x12000000(288MB)` 且 `prot=PROT_NONE` 的调用 → 用 `mmap(NULL, len, PROT_NONE, MAP_PRIVATE\|MAP_ANONYMOUS, -1, 0)` mock 成功 + 返回有效虚拟地址区间 |
| 判据 | 实验后 .bss 非零率从 0.2% 提升（成功阈值 = ≥5% 且无需构造 C++ 对象图——即纯 mmap 补全即可推进 OnCreate 越过 §124③）；若 .bss 非零率仍 ~0.2% = §124② 对象语义墙为主阻塞（mmap 非瓶颈），维持现状 |
| 工时 | 半日（改 1 个 hook 分支 + 重跑 OnCreate 推进对照） |
| 风险 | 低（隔离实验，不改现有 4 路径回归）；失败则 §124③ 维持"mmap mock 处理失败"现状 |

### 2.3 不投入的理由（对抗结论已闭合）

- §124y-158/168 已证明 .bss 事件表真值 = disc=0 忠实（浅触发时刻高 nibble 本就全 0）
  → 即使 mmap 补齐 + 能跑更多 init_array，**拿到的仍是同一个"全 0 掩码"结果**，对
  事件表 / 棋子对象图 / 走子规则无增量（64 位 src_rs 已行为级对齐 170/170 事件回归）
- 看雪路线对 SPAK key 无增量（R5 已提取，见 AGENTS.md 诚实边界 2026-11-11 口径更新）
- 看雪路线对去广告无增量（阶段 3 已全删 smali 走 arm64 旁路）
- 看雪路线对棋子 dump 无增量（jcc IL2CPP 托管堆手法 vs 军棋纯 native C++ scudo 堆错配）

### 2.4 结论（方向 A 维持现状）

**不启动魔改 unicorn 路线**（无新证据重开 §124y-152 "不可行"判定）。
**可选**：若有闲时做 §124③ 288MB mmap 定点实验（半日，判据 = .bss 非零率 ≥5% 才推进），
但**预期结果对下游（事件表/棋子/走子）无增量**，因为 §124y-168 已闭合 disc=0 忠实语义。
**主线仍 = 64 位 src_rs 行为级重写**（已完成 170/170 + 真机 5 判据全链 PASS），
unidbg/frida 路线针对 §124①② 维持现状（模拟器精度 + C++ 对象图语义墙，非看雪路线可解）。

## 3. 可复现判据（防假绿）

| 判据 | 命令 | 期望 |
|---|---|---|
| .bss 非零率基线 | `frida-dump/bss_dump_align.py` 读 MuMu 10.0.182.146 | ~79.1%（非零） |
| unidbg .bss 非零率 | `course_unidbg/UniBlFix.java` 跑后 dump .bss | 0.2%（现状） |
| mmap 实验后 | 改 `__mmap2` hook 允许 288MB PROT_NONE → 重跑 | .bss 非零率 ≥5%（成功）或仍 ~0.2%（§124② 为主阻塞，维持现状） |
| 事件表 disc | `.bss+0x4704` 289B 高 nibble | 全 0（§124y-168 已闭合，disc=0 忠实） |
