# 逆向分析方法论 (从《C++反汇编与逆向分析技术揭秘》提取)
# 针对 libjunqi.so 实操指南
# 日期: 2026-11-17

## 一、核心方法论

### 1. 自顶向下 vs 自底向上
| 方法 | 适用场景 | 本项目 |
|------|----------|--------|
| 自顶向下 | 从入口追踪调用链 | JNI_OnLoad → OnCreate → 主循环 |
| 自底向上 | 从字符串/常量反推使用者 | `FindClass("com/baidu/...")` → 定位调用者 |
| 混合 | 入口 + 字符串锚点 | **推荐: 14 个 JNI 符号 + 3 个 baidu 字符串 = 17 个锚点** |

### 2. 锚点驱动逆向 (已验证)
```
锚点类型:
  1. 导出符号 (14 个 JNI + JNI_OnLoad)  ← 入口点 (已完成调用图)
  2. 字符串引用 (baidu/junqi)           ← 业务逻辑 (待追踪)
  3. PLT 调用 (GL/stdio)               ← 外部依赖
  4. .bss 全局变量                      ← 状态机

策略: 从锚点扩散, 不逐行读 5.5MB .text
```

### 3. 数据流 vs 控制流
```
数据流: 追踪"值"从哪来、到哪去
  例: 触摸坐标 OnTouch(r0,r1) → [sp+0x40] → 棋盘 cell → 事件 → 渲染

控制流: 追踪"执行路径"
  例: if (turn == AI) { ai_pick(); } else { wait_input(); }

逆向技巧: 先控制流 (函数调用图), 再数据流 (关键变量追踪)
```

## 二、实操技巧 (书中章节 → 命令映射)

### 2.1 快速读懂 ARM 指令
```
关键指令族:
  mov/ldr/str  数据搬运
  add/sub      算术
  bl/blr       函数调用 (核心! 所有 bl 目标 = 调用图)
  cbz/cbnz     条件分支
  b/bi         无条件分支
  ldm/stm      批量栈操作 (函数入口/出口)

快速扫读法:
  1. 找 bl 指令 → 标记子函数调用
  2. 找 cbz/cbz → 标记分支点
  3. 找 ldr/stm → 标记数据流
  忽略: 寄存器分配细节, 编译器优化噪声
```

### 2.2 函数边界识别
```
入口特征:
  push {r4-rN, lr}    ← 保存 callee-saved
  sub sp, sp, #0xXX   ← 栈空间

出口特征:
  pop {r4-rN, pc}     ← 恢复 + 返回
  bx lr                ← ARM32 标准返回

陷阱:
  - 内联函数无独立边界
  - 尾调用 (tail call) 用 b 而非 bl
  - stripped so 无符号表, 需人工标注 (本项目 so 有符号表, 幸运)
```

### 2.3 字符串引用追踪
```
Thumb 模式 PC-relative load:
  ldr r0, [pc, #0x10]  ← 实际地址 = (PC+4+0x10) & ~3

实操:
  1. 字符串 offset = 0x2eaa78 (com/baidu/mobads/AdView)
  2. 在 .text 中搜索 ldr rX, #0x2eaa78 模式
  3. 或直接用 objdump + grep:
     arm-none-eabi-objdump -d | grep -B10 "2eaa78\|0x2eaac"
```

### 2.4 PLT 调用解析
```
动态符号调用路径:
  函数 A → bl PLT_entry → .got[槽] → 实际地址 (linker 填充)

逆向:
  1. readelf -d 看 NEEDED 库
  2. objdump -R 看 PLT 重定位表
  3. 对每个 PLT 调用, 追踪谁在调用 (数据流)
```

## 三、针对 libjunqi.so 的逆向路线图

### Phase 1: 锚点映射 (已完成 ✓)
```
目标: 17 个锚点 (14 JNI + 3 baidu) 全部定位
产出: JNI_CALLGRAPH.md (已完成)
命令:
  arm-none-eabi-objdump -d | grep "bl" → 44269 条
  按 JNI 入口地址范围过滤 → 15 个函数的调用目标
```

### Phase 2: 状态机还原 (1 天)
```
目标: .bss 940KB 全局变量分类
产出: 状态机图 (游戏阶段: MENU → PLAY → OVER)
方法:
  1. 从 OnCreate 找 .bss 写入点 (jqt_mem_addr 调用密集区)
  2. 从 OnUpdate/OnTimer 找 .bss 读取点
  3. 分类: 渲染状态 / 游戏逻辑 / UI 弹窗 / 蓝牙
```

### Phase 3: 主循环逆向 (1 天)
```
目标: F_9e2b4 (OnUpdate 核心) + F_2af44 (AI 决策) 完整控制流
产出: 帧调度 + AI 决策流程图
方法:
  1. 反汇编 0x511d54 (F_9e2b4), 标注: 输入处理 / AI / 渲染 / 同步
  2. 反汇编 0x7f5cac (F_2af44), 追踪走子逻辑
  3. 提取 .bss 读写点 → 状态变量清单
```

### Phase 4: 广告引用评估 (0.5 天)
```
目标: baidu 3 个字符串的所有引用点
产出: 删除影响报告 (哪些 .text 段可达 baidu)
方法:
  1. 对 0x2eaa78/0x2eaacc/0x2eaae8 搜索 ldr 引用
  2. 从引用点回溯到 JNI 入口
  3. 判断: 删 smali 后是否触发 (需保留最小壳)
```

## 四、工具链

| 工具 | 用途 | 状态 |
|------|------|------|
| arm-none-eabi-objdump | 反汇编 | 已有 (/usr/bin/) |
| readelf | 段/动态信息 | 已有 |
| nm/strings | 符号/字符串 | 已有 |
| Ghidra | GUI 反汇编 + 调用图 | 可装 (推荐) |
| Frida | 运行时 hook | 已有 (frida-dump/) |
| unidbg | 纯模拟执行 | 已有 (course_unidbg/) |

### Ghidra 快速上手 (2h)
```
1. 导入 libjunqi_arm32_aligned.so
2. Auto-analysis 跑完 (10-30min)
3. 从 Java_com_cnvcs_App_OnCreate 出发:
   - F5 看伪代码
   - 点 bl 目标 → 跳转
   - 右键 → Function → Rename (重命名辅助函数)
4. 导出调用图: Analysis → Generate Call Graph
```

## 五、防坑清单

1. **Thumb 模式 PC 偏移**: `ldr r0, [pc, #X]` 实际地址 = `(指令地址+4+X) & ~3`
2. **内联展开**: 编译器可能把小函数内联, 无独立 bl → 看代码块而非函数
3. **尾调用**: 最后一个调用可能用 `b` 而非 `bl`, 不压栈直接跳
4. **异常路径**: ARM 无 C++ 异常展开表, 错误处理可能内联
5. **编译器优化**: -O2 可能重排序/公共子表达式, 不要按源码顺序读
6. **混合 ARM/Thumb**: EABI5 so 通常全 Thumb, 但 PLT 入口可能 ARM
7. **objdump 输出太大**: 9.1MB so 反汇编 > 100MB, 用 grep/sed 过滤, 别 cat 全量

## 六、与 64 位重写 (阶段 2) 的关联

```
逆向 32 位 so 不是为了"读懂每一行", 而是:
  1. 确认 14 个 JNI 签名 (已完成)
  2. 确认事件契约 (Junqi.Move/Eat/... 已完成)
  3. 确认棋盘规则 (12×5, 50 子 已完成)
  4. 确认 .bss 状态机结构 (进行中)
  5. 确认 AI 决策边界 (进行中)

产出 → src_rs 64 位重写:
  - 不需要逐指令移植
  - 需要行为对齐 (规则/事件/UI/协议)
  - 逆向是"规格提取", 不是"代码翻译"
```

## 七、推荐阅读章节 (书中目录对照)

| 优先级 | 章节 | 理由 | 状态 |
|--------|------|------|------|
| P0 | 第 2 章: ARM 指令集 | 能读懂 objdump 输出 | 已实践 |
| P0 | 第 4 章: 字符串与常量 | baidu 引用追踪 | 待实践 |
| P0 | 第 5 章: 函数与调用 | 14 个 JNI 逆向 | 已实践 (调用图) |
| P1 | 第 6 章: 控制流分析 | 主循环 / AI | 待实践 |
| P1 | 第 7 章: 栈与局部变量 | 状态还原 | 待实践 |
| P2 | 第 8 章: PLT/GOT | 动态依赖 | 可选 |
| P2 | 第 10 章: 混淆与去混淆 | 确认无壳 | 已完成 (无壳) |
| 跳过 | 第 12 章: 加密算法 | 非加密 so | N/A |
| 跳过 | 第 14 章: 高级壳处理 | 无壳 | N/A |

## 八、本次实操经验 (固化)

### 8.1 objdump 使用坑
```
❌ objdump -d --start-address=X --stop-address=Y → 输出 0 行 (架构识别失败)
✅ arm-none-eabi-objdump -d 全量输出 → grep/sed 过滤地址范围

❌ awk strtonum() → 某些 gawk 版本不支持
✅ bash $((0x$addr)) 十六进制转十进制

❌ grep -oP 'lookbehind variable length' → PCRE 不支持
✅ 简化正则, 用 grep -oP '^\s+\K[0-9a-f]{6}' 提取地址
```

### 8.2 调用图提取流程 (可复用)
```bash
# 1. 全量提取 bl/blx (一次性, 10s)
arm-none-eabi-objdump -d so | grep -E "bl|blx" > /tmp/all_bl.txt

# 2. 按地址范围过滤 (每个函数 1s)
grep "^ *324dc8:" -A 99999 /tmp/all_bl.txt | \
  grep -oP '^[0-9a-f]{6}' | \
  while read addr; do
    if [ $((0x$addr)) -ge $((0x324dc8)) ] && [ $((0x$addr)) -lt $((0x32636c)) ]; then
      grep "^ *${addr}:" /tmp/all_bl.txt
    fi
  done | sort -u
```

### 8.3 符号命名规律
```
jqt_*       = 翻译体新增 (jqt_mem_init, jqt_jni_setenv, ...)
jq_*        = 游戏业务 (jq_game_test_hook, jq_rule_probe_run, ...)
F_xxxxx     = 原版函数 (stripped 后 objdump 自动命名)
_Znwj       = C++ operator new (malloc 封装)
__aeabi_*   = ARM EABI 运行时 (memset, memcpy, ...)
```

### 8.4 地址类型判断 (PHASE3_0x39f20 教训)
```
逆向前先判断地址类型:
  1. 查段: readelf -S → .text/.rodata/.data/.bss
  2. 0x39f20 在 .text (0x323f50-0x5821d8) → 代码 (走子执行器)
  3. .rodata 数据 → 可能是常量表 (浮点/字符串/vtable)
  4. 64 位注释引用 0x39f20 → 走子执行器 (代码地址, 非数据表)
  
陷阱: 64 位 rules.c 注释 "对齐原版 0x39f20" 容易被误读为数据表
      实际是 F_79858 (0x335258, ~17KB) 内部的代码地址
```

### 8.5 栈帧分析限制 (PHASE3 .bss 提取教训)
```
ARM 栈帧:
  fp = sp + 28 (入口 add fp, sp, #28)
  局部变量: [fp, #-32] 起 (负偏移)
  sp 偏移: [sp, #N] 是运行时值, 非静态常量
  
静态提取限制:
  - sp 偏移分布可统计 (F_79858 内 290 个 jqt_mem_addr 调用点)
  - 但 .bss 真实偏移需运行时 (参数传递链: 入口 r0-r3 → [fp, #-32..-44] → 局部)
  - 结论: 静态只能到 "sp 偏移分布", 不到 ".bss 偏移清单"
  - 需 Frida hook jqt_mem_addr 记录 r0 (脚本: frida-dump/bss_offset_trace.py)
```

### 8.6 虚函数分发 (诚实边界)
```
jqt_blx_dispatch (0x4af4f4):
  - C++ 多态, 静态看不到真实目标
  - 需 Frida hook / Ghidra vtable 分析
  - F_79858 内多次调用 = 走子逻辑涉及多态

与 §124y 17 handler 问题同源:
  - 编译期固定映射 (thunk 区)
  - 静态收表需逐一反汇编 (46 目标太多, 本次未全做)
```

### 8.7 地址空间区分 (★ 重要, 防对照错对象)
```
逆向/正向引用地址时, 必须标注 so 来源 (原版/翻译体), 避免对照错对象:

原版 32 位 so (lib/armeabi/libjunqi.so, 1.4MB, from 军棋.apk):
  .text:   0x233e0 - 0xfd814
  .rodata: 0x120bf8 - 0x13dbc8
  0x55d40 = .text 事件业务 thunk 区 (0x55d40-0x56300 共 75 thunk, 41 独立目标;
           thunk bl 目标多为析构/setter/虚函数分发, 0x557d4 = getter thunk 读对象 +132, 非评分)
  0x39f20 = .text 走子执行器区 (短函数, 读对象 +168/164)
  0x3a15c = .text 坐标/rank 数组访问区

翻译体 32 位 so (rev64/dist/libjunqi_arm32_aligned.so, 9.1MB):
  .text:   0x323f50 - 0x5821d8
  .rodata: 0xc4f0 - 0x316a60
  0x55d40 = .rodata 浮点常量表 (与原版完全不同!)
  0x39f20 = .rodata 浮点常量表 (与原版完全不同!)

教训 (PHASE3_0x39f20_AND_BSS.md §五):
  64 位 rules.c 注释引用 "0x39f20/0x55d40" 时未标注 so 来源
  → 易误读为翻译体地址 → 对照错对象 (浮点表 vs thunk)
  → 已写 AUD_ALIGN_R16.md 台账, 建议正向会话加 "(原版 32 位 so 地址)" 前缀
```
