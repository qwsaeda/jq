# 32 位 libjunqi.so 反汇编阅读清单
# 《C++反汇编与逆向分析技术揭秘》对应技巧映射
# 生成日期: 2026-11-17

## so 基本信息
- 文件: rev64/dist/libjunqi_arm32_aligned.so (9.1MB, 对齐后)
- 架构: ELF32 / ARM / EABI5 / stripped
- 段布局:
  - .rodata: 0x000c4f0 ~ 0x316a60 (3.2MB 字符串池 + 压缩资源)
  - .text:   0x323f50 ~ 0x5821d8 (5.5MB 代码段)
  - .plt:    0x8a6130 ~ 0x8a6740
  - .got:    0x8acd88 ~ 0x8ace0c
  - .data:   0x8ade0c ~ 0x8afc38
  - .bss:    0x8afec0 ~ 0x9a1b07 (~940KB 全局变量)

## 一、14 个 JNI 导出符号 (P0 优先逆向)

| # | 符号 | 地址 | 大小 | 优先级 | 说明 |
|---|------|------|------|--------|------|
| 1 | Java_com_cnvcs_App_OnCreate | 0x324dc8 | ~0x100 | P0 | 初始化入口, 构造 .bss 对象图 |
| 2 | Java_com_cnvcs_App_OnDestroy | 0x32636c | ~0x40 | P1 | 清理 |
| 3 | Java_com_cnvcs_App_OnPause | 0x32641c | ~0x60 | P2 | 暂停 |
| 4 | Java_com_cnvcs_App_OnResume | 0x32647c | ~0x80 | P2 | 恢复 |
| 5 | Java_com_cnvcs_App_OnUpdate | 0x326574 | ~0x250 | P0 | 主循环/帧更新 |
| 6 | Java_com_cnvcs_App_OnEvent | 0x3267c4 | ~0xd4 | P1 | 事件分发 |
| 7 | Java_com_cnvcs_App_OnResult | 0x32682c | ~0x6c | P2 | 结果回调 |
| 8 | Java_com_cnvcs_App_OnKey | 0x326898 | ~0x68 | P2 | 按键 |
| 9 | Java_com_cnvcs_App_OnBluetooth | 0x326900 | ~0x64 | P1 | 蓝牙 |
| 10 | Java_com_cnvcs_App_OnGraphCreate | 0x326964 | ~0x14c | P1 | GL 初始化 |
| 11 | Java_com_cnvcs_App_OnGraphChange | 0x326ac0 | ~0x15c | P1 | 视口变化 |
| 12 | Java_com_cnvcs_App_OnTimer | 0x326b7c | ~0x594 | P0 | 定时器/AI |
| 13 | Java_com_cnvcs_App_OnInput | 0x327110 | ~0x64 | P2 | 输入 |
| 14 | Java_com_cnvcs_App_OnTouch | 0x327174 | ~0x400 | P0 | 触摸交互 |
| + | JNI_OnLoad | 0x32492c | ~0x49c | P0 | 库加载入口 |

## 二、字符串引用追踪 (P0 - 广告拆除)

### 关键字符串位置 (offset in .rodata)
```
0x2eaa78  com/baidu/mobads/AdView
0x2eaacc  com/cnvcs/AdLis
0x2eaae8  (Lcom/baidu/mobads/AdViewListener;)V
```

### 追踪方法 (书中 §字符串定位)
1. `objdump -s -j .rodata --start-address=0x2eaa78` 确认字符串完整
2. 在 .text 中搜索引用 (Thumb 模式):
   ```
   # 搜索 ldr r0, #0x2eaa78 (PC-relative load)
   objdump -d -M intel | grep -B5 "2eaa78\|0x2eaa78"
   # 或搜索 push/ldr 序列加载该地址
   ```
3. 找到所有引用点 → 评估: 删 baidu smali 后这些调用是否可达
4. 如果可达 → 需 patch .text 或保留最小 baidu 壳类

### 游戏事件字符串 (参考)
```
0x0edd5   Junqi.Bomb
0x0ee07   =Junqi.%s
0x10143   Junqi.Flag
0x10577   Junqi.Move
0x124f3   Junqi.Dead
0x13207   Junqi.Eat
0x2e5754  Junqi.RedWin
0x2e577c  Junqi.BlueWin
0x2e57b4  Junqi.Select
0x2e57f8  Junqi.Start
```

## 三、函数边界与调用约定 (P0 - 逆向入口)

### 书中方法 → 实操映射
| 技巧 | 实操命令 |
|------|----------|
| 识别函数入口 | `objdump -d --start-address=0x324dc8 --stop-address=0x324ec8` (每个 JNI 入口前 512B) |
| 识别函数出口 | 找 `pop {r4-r7,pc}` / `bx lr` 模式 |
| 栈帧分析 | 看 `push {r4-r7,lr}` 数量 → 局部变量 |
| 寄存器角色 | r0-r3=参数, r4=首个局部, sp=栈指针, lr=返回 |

### 重点逆向顺序
1. **JNI_OnLoad** (0x32492c): 看构造了哪些全局单例
2. **OnCreate** (0x324dc8): 看 .bss 初始化链
3. **OnUpdate** (0x326574): 主循环, 帧调度
4. **OnTouch** (0x327174): 触摸→事件分发
5. **OnTimer** (0x326b7c): AI 走子逻辑

### 调用图提取
```bash
# 从 OnCreate 出, 追踪所有 bl (call) 目标
objdump -d --start-address=0x324dc8 --stop-address=0x324ec8 \
  rev64/dist/libjunqi_arm32_aligned.so | \
  grep -oP 'bl\s+0x[0-9a-f]+' | sort -u
```

## 四、PLT/GOT 与动态链接 (P1)

### 段信息
```
.plt:   0x8a6130  大小 0x610 (24 条 PLT 项)
.got:   0x8acd88  大小 0x84  (9 个 GOT 槽)
.got.plt: 0x8afd38 大小 0x188
```

### 分析
```bash
# 看动态段, 确认 NEEDED 库
readelf -d rev64/dist/libjunqi_arm32_aligned.so | grep NEEDED
# 预期: libc, libstdc++, libm, libGLESv1_CM, libandroid, libdl

# 看 PLT 条目
objdump -R rev64/dist/libjunqi_arm32_aligned.so | head -30
```

## 五、控制流分析 (P1 - 主循环)

### OnUpdate 主循环结构
```
预期模式:
  1. 读取游戏状态 (.bss 全局)
  2. 处理输入事件
  3. AI 决策 (if 轮到 AI)
  4. 渲染调用
  5. 帧同步
```

### 追踪方法
```bash
# 提取 OnUpdate 完整反汇编
objdump -d --start-address=0x326574 --stop-address=0x32682c \
  rev64/dist/libjunqi_arm32_aligned.so > /tmp/onupdate.s
# 人工标注: 入口/分支/循环/调用
```

## 六、栈帧与局部变量恢复 (P2)

### 书中方法 → 实操
```
每个函数入口:
  push {r4-rN, lr}   ← N 决定栈帧深度
  sub sp, sp, #0x40  ← 额外栈空间

恢复:
  add r0, sp, #0x4   ← 第 1 个局部变量
  ldr r0, [sp, #0x8] ← 第 2 个局部变量
```

## 七、混淆判断 (P2 - 确认无壳)

### 已验证结论 (无需书中壳处理章节)
- .text 明文 (objdump 出真实 ARM 指令)
- .rodata 明文 (字符串直接可见)
- 静态 vs 运行时 0/118832 字节不同
- 无 .init/.fini 自解密例程
- **无壳, 直接反汇编即可**

## 八、建议阅读路线 (时间盒)

| 阶段 | 时长 | 内容 | 产出 |
|------|------|------|------|
| Day 1 | 3h | ARM 指令速查 + 寄存器约定 | 能读懂 objdump |
| Day 2 | 2h | 字符串定位章节 | 找到 baidu 引用点 |
| Day 3 | 4h | 函数边界 + 调用约定 | 逆向 JNI_OnLoad + OnCreate |
| Day 4 | 4h | 控制流分析 | 主循环结构图 |
| Day 5 | 按需 | PLT/GOT / 栈帧 / 混淆 | 补充 |

## 九、与当前项目关联

| 任务 | 所需章节 | 优先级 |
|------|----------|--------|
| 阶段 0 去广告 | 字符串定位 + 引用追踪 | P0 |
| 阶段 1 双 ABI | 函数边界 (JNI 签名对照) | P1 |
| 阶段 2 64位重写 | 控制流 + 调用图 | P1 |
| 阶段 3 终态 | 全量反汇编 | P2 |
