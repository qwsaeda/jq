# 0x39f20 逆向 + .bss 偏移提取 最终报告
# 日期: 2026-11-17
# 方法: 静态反汇编 (objdump) + 栈帧分析

## 一、0x39f20 澄清 (★ 重要纠正)

### 结论: 0x39f20 是**代码地址**, 非数据表
- 64 位 rules.c 注释 "对齐原版 0x39f20" 指的是**走子执行器函数体**
- 0x39f20 位于 **F_79858 (0x335258, 大小 ~0x2A00)** 内部
- .rodata 区 (0x0c4f0-0x316a60) 的 "7fce b27f..." 模式是**浮点常量表** (PI/e/三角函数系数), 与 0x39f20 无关

### F_79858 走子执行器特征 (已实证)
| 特征 | 证据 |
|------|------|
| 超大函数 | ~17KB 代码, 42 个子函数调用 |
| 密集 .bss 访问 | 290 次 jqt_mem_addr (F_79858 范围) |
| 虚函数分发 | 多次 jqt_blx_dispatch (C++ 多态) |
| 走子执行 | 0x39f20 处 `movw r0, #6` + `blx ip` (事件码 6 = 走子?) |
| 坐标比较 | 64 位注释 "坐标 0x6ef54 比较 [0]=col [1]=row" |

### 0x39f20 处关键指令 (实证)
```arm
339f14:  movw r0, #6              ← 事件码/参数 6
339f18:  ldr r1, [pc, #4024]      ← PC-relative 加载 (vtable/函数指针)
339f1c:  add r1, pc, r1
339f20:  ldr r2, [pc, #4020]      ← PC-relative 加载
339f24:  add r2, pc, r2
339f28:  ldr ip, [pc, #4016]
339f2c:  ldr ip, [pc, ip]         ← 双重间接 = vtable 调用
339f30:  blx ip                   ← 虚函数分发
```

## 二、.bss 偏移提取 (静态, 部分成功)

### 方法
- 提取 F_79858 内所有 "str/ldr rX, [sp, #N]" 后接 "bl jqt_mem_addr" 模式
- 共 **290 个调用点**

### sp 偏移分布 (Top 10)
| sp 偏移 | 次数 | 推测用途 |
|---------|------|----------|
| [sp, #48] | 19 | 局部变量区 |
| [sp, #52] | 18 | 局部变量区 |
| [sp, #44] | 18 | 局部变量区 |
| [sp, #56] | 17 | 局部变量区 |
| [sp, #60] | 17 | 局部变量区 |
| [sp, #40] | 16 | 局部变量区 |
| [sp, #36] | 13 | 参数保存区 |
| [sp, #96] | 12 | 深度局部 |
| [sp, #64] | 10 | 局部变量区 |
| [sp, #72] | 10 | 局部变量区 |

### 诚实边界
- **sp 偏移 ≠ .bss 偏移**: 栈帧 228 字节, sp 偏移是**函数内局部变量**
- 要得到真正 .bss 偏移, 需追踪:
  1. 入口参数 (r0-r3) → 存 [fp, #-32..-44]
  2. 参数值 = .bss 基址 + 偏移
  3. 局部变量 = 参数 + 子偏移
- **静态无法完全还原** (需编译器栈帧分析 + 数据流)

### 可行方案
| 方案 | 状态 | 产出 |
|------|------|------|
| Frida 运行时 hook jqt_mem_addr | 脚本已写 (frida-dump/bss_offset_trace.py) | 需 Frida + 设备 |
| unidbg 模拟执行 | 已有 (course_unidbg/) | .bss dump |
| 已有 §124y 数据 | 已回填 289B 事件表 | 复用 |
| 静态 sp 偏移分布 | 已完成 (bss_offsets_static.json) | 290 调用点 + 分布 |

## 三、下一步 (如需继续)

### P0: Frida 运行时 dump (需设备)
```bash
# 1. 装 Frida
pip3 install frida-tools

# 2. 连设备 (MuMu 10.0.182.146)
adb connect 10.0.182.146:5555

# 3. 跑 hook
python3 frida-dump/bss_offset_trace.py 10.0.182.146:5555

# 4. 输出: frida-dump/bss_offsets.json (真实 .bss 偏移)
```

### P1: 反汇编 F_79858 完整控制流 (1-2h)
- 42 个子函数调用图
- 虚函数分发点 (jqt_blx_dispatch)
- 事件码 6 的语义 (走子/翻棋/吃子?)

### P2: 对比 64 位 src_rs 走子执行器
- 64 位: jq_try_move (rules.c:656)
- 32 位: F_79858 + 0x39f20
- 差异点: 虚函数 vs 简化实现

## 五、正向开发关联 (★ 已写入 AUD 台账)

### 5.1 rules.c 注释纠错 (AUD_ALIGN_R16.md)
- 正向 `rev64/forward/src_rs/rules.c` 第 14/440/514/1213/1216/1219/1224/1232/1263 行注释
  引用 0x39f20/0x55d40/0x3a15c 时**未标注"原版 32 位 so 地址"**
- 易误读为翻译体地址 (翻译体 0x55d40 = .rodata 浮点表, 完全不同)
- 建议: 加 "(原版 32 位 so 地址)" 前缀 + 评分逻辑真实地址 = 0x557d4 (0x55d40 thunk 的 bl 目标)
- 台账: `rev64/forward/audit/r4/AUD_ALIGN_R16.md` (P2, 逆向会话只写台账, 不动 forward/src_rs)

### 5.2 地址空间区分 (固化到 REVERSE_METHODOLOGY §8.7)
```
原版 32 位 so (lib/armeabi/libjunqi.so, 1.4MB):
  .text: 0x233e0 - 0xfd814
  .rodata: 0x120bf8 - 0x13dbc8
  0x55d40 = .text 事件业务 thunk (16 字节, 调 0x557d4)
  0x39f20 = .text 走子执行器区

翻译体 32 位 so (rev64/dist/libjunqi_arm32_aligned.so, 9.1MB):
  .text: 0x323f50 - 0x5821d8
  .rodata: 0xc4f0 - 0x316a60
  0x55d40 = .rodata 浮点常量表 (与原版完全不同)
  0x39f20 = .rodata 浮点常量表 (与原版完全不同)

逆向/正向引用地址时, 必须标注 so 来源 (原版/翻译体), 避免对照错对象
```

## 四、本次实操经验 (固化到 REVERSE_METHODOLOGY)

### 4.1 地址类型判断
```
逆向前先判断地址类型:
  1. 查段: readelf -S → .text/.rodata/.data/.bss
  2. 0x39f20 在 .text (0x323f50-0x5821d8) → 代码
  3. .rodata 数据 → 可能是常量表 (浮点/字符串/vtable)
  4. 64 位注释引用 0x39f20 → 走子执行器 (代码地址)
```

### 4.2 栈帧分析限制
```
ARM 栈帧:
  fp = sp + 28 (入口 add fp, sp, #28)
  局部变量: [fp, #-32] 起 (负偏移)
  sp 偏移: [sp, #N] 是运行时值, 非静态常量
  
静态提取限制:
  - sp 偏移分布可统计 (290 调用点)
  - 但 .bss 真实偏移需运行时 (参数传递链)
  - 结论: 静态只能到 "sp 偏移分布", 不到 ".bss 偏移清单"
```

### 4.3 虚函数分发 (诚实边界)
```
jqt_blx_dispatch (0x4af4f4):
  - C++ 多态, 静态看不到真实目标
  - 需 Frida hook / Ghidra vtable 分析
  - F_79858 内多次调用 = 走子逻辑涉及多态

与 §124y 17 handler 问题同源:
  - 编译期固定映射 (thunk 区)
  - 静态收表需逐一反汇编
  - 本次未做 (42 子函数太多)
```
