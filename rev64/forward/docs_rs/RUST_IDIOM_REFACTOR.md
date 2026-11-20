# Rust 惯性化重构（2026-11-20）：C-in-Rust → 惯用 Rust 的边界实证

> 用户指正：Rust 代码风格/实现习惯像 C（语言特性/标准库/模块没用上）。本轮分批重构 + 用判据守门。

## 已完成的 Rust 化（行为零回归，判据全绿）
| 批次 | 改动 | 判据 |
|---|---|---|
| 1 | `rules.rs` 工兵 BFS：手写 `[i32;64]` 双队列 + `head/tail` 游标 + `[[bool;5];12]` → `VecDeque<(i32,i32)>` + `HashSet`（容量上限 64/邻接顺序/起点不查 walkable 全保留） | 95 测试 / 0 警告 / 154 case / align_gate 全 PASS |
| 2 | `move_legal` 的 `turn: i32/-1` 哨兵 → `Option<u8>`（None=忽略回合，Some(t) 校验阵营域） | 同上 |
| 3 | `ai_score` 的 `-1/-2` 哨兵 → `Option<i32>`（None=不可走，调用点 `match` + 18 测试断言同步） | 同上 |

## 边界实证（为什么核心规则层不能继续"纯 Rust 化"）
- **实测**：用 `rust_anything_check.py` 的 154 case 录制基线，把 `rank_norm` 从
  `if v > RANK_TOP { v-0xc } else { v }` 改成"更 Rust"的钳位写法
  （`if v>24 {RANK_TOP} else if v>RANK_TOP {v-0xc} else {v}`）→ **diff 立刻多处 mismatches**
  （`ai_score(2147483647,2)` 等 expected 1 got -1）。
- **结论**：核心规则层的 `i32 rank + as 转换 + +0xc 存储约定`是**原版 32 位行为逐字节对齐的载体**
  （R-138 分叉矩阵、`rule_probe_shared.inc` 同源探针、`RANK_*` 双源常量断言都依赖它）。
  把它改成 u8/enum 会改变脏 rank 域行为 = 破坏对齐判据。这是**对齐约束下的必要设计**，不是 C 风格坏味道。
- **JNI 层 `static mut`（14 处）**：JNI 全局态（G_VM/G_VIEW 等）跨回调必需，已 `#[allow(static_mut_refs)]` 标注；
  改成 Mutex/RwLock 会改变跨线程语义且 JNI 签名固定，属既有边界。
- `board.rs` 已是惯用 Rust（match 模式/纯函数/const/无 mut），坐标用 i32 是配合规则的对齐契约。

## 诚实边界
- "全面惯用 Rust 重构"在**行为敏感层**遇到实证硬边界：继续改 = 破对齐 = 砸 95 测试 + 154 case + 真机 5 判据。
- 本轮在**可安全改的范围**（BFS/Option 哨兵）完成了 Rust 化；剩余 i32/as/static mut 是
  对齐/JNI 契约下的**有意保留**，用 154 case 录制实证了"改了会破行为"。
