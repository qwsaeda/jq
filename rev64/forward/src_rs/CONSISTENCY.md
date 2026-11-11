# 一致测试（重写版 vs 原版 so）

## 口径（先说死，避免"能玩/完整版"误读）

本目录是**行为级重写**（`src_rs/`），不是原版 32 位 so 的逐指令翻译体。
一致测试的判据是"**喂相同输入序列，可观测输出一致**"，**不是**功能等价证明。

| 层 | 方法 | 状态 |
|---|---|---|
| ① 布局字节静态对照 | 原版 `data_lay_default.lay` 30B vs 重写 `JQ_DEFAULT_LAY` | ✅ 逐字节一致（rank 分布 `{0:5,1:1,2:1,3:2,4:2,5:2,6:2,7:3,8:3,9:3,10:2,11:3,12:1}`） |
| ② 重写侧规则自洽 | `jq_consistency selftest`：固定基准序列 → 事件码 + 终局态 | ✅ 9 行逐行一致（翻棋/走子/非法/悔棋/认输全事件面） |
| ③ 原版侧走子序列对拍 | unidbg 驱动原版 `OnEvent("move",...)` | ❌ **驱动不了**（见下"边界"） |
| ④ 渲染/音频/蓝牙 | 原版对象图缺口 + 重写桩 | ❌ 两边都非"真渲染"，不纳入可比面（红线） |

## 边界（unidbg 为何驱动不了原版走子链）

原版 32 位 so 的**全部规则/布局/走子函数都挂在 C++ vtable 对象图 + 加密常量池上**：

| 函数 | 作用 | unidbg 裸调 | 根因 |
|---|---|---|---|
| `0x39f20` 走子执行器 | 执行走子 | ❌ 崩 | 读全局单例 `0x121f4a`（init_array 构造） |
| `0x32a2c` 合法判定 | 走子判定 | ❌ 崩 | `ldr r3,[r0]; blx vtable[8/4/0xc/0x10]` |
| `0x2a340` 布局载入 | 读 lay | ❌ 崩 | 写 state `+0x78` 计数表（布局容器对象） |
| `0x39e14` 目标判定 | rank 比较 | ❌ 崩 | 读 `.rodata` 高熵解密态（`0x3e5bc`→`0x11cafe` 索引表） |

数据与指令对齐由 `init_array` 构造，unidbg/unicorn2 下 `init_array` 跑不通
（`UC_ERR_FETCH_PROT`，§124c 已知 bx bug 域）。这是 §124z 黑屏根因的完整版：
**unidbg 替代不了真机**（无 GPU + `.bss` 段映射 + init_array 精度 bug 三重缺口）。

## 用法

```bash
# 重写侧自洽基准（9 行事件 + 终局态，PASS=规则引擎没回归）
make consistency && ./dist/jq_consistency selftest

# 喂自定义序列（与原版 OnEvent 协议对齐：m 走子 / f 翻棋 / u 悔棋 / q 认输 / L 重开）
./dist/jq_consistency <seq.txt>
# 输出：每行 "OK <事件>" / "ILLEGAL"，末行 "FINAL winner=.. turn=.. total=.. since_eat=.. stage=.."
```

基准序列文件：`test_move_seq.txt`（输入）+ `test_expected.txt`（期望输出，与 selftest 内置 BENCH 同步）。

## 诚实边界（红线照旧）

- ② 的"一致"是**构造输入下行为一致**，不是任意输入/功能等价
- 原版侧走子序列对拍需**真机**（GPU + init_array + 数据与指令对齐齐全），unidbg 做不了
- 不包装成"已验证可玩"——渲染/音频/蓝牙的可玩性验收仍需真机
