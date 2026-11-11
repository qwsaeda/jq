# 阶段 R0 · 基础设施与静态分析

## 目标
建立可复用的分析管线（函数边界表、调用图、反汇编、.rodata 明文映射），
为 R1-R6 的 arm64 移植提供地基。

## 关键决策
用户明确否决 32 位包作为 64 位设备最终方案，选择**完整逆向移植 arm64**。
原版 `.text` 段 = 1,105,428 字节 / 514,762 条 Thumb 指令 / 7,480 个函数。
**月级工程，分阶段推进**。

## 操作步骤与产物

### 1. 完整反汇编（可复用管线）
```bash
arm-linux-gnueabi-objdump -D -b elf32-littlearm -m arm -M force-thumb \
  --start-address=0x233e0 --stop-address=0x1311f4 \
  orig32/lib/armeabi/libjunqi.so > full_text_disasm3.txt
```
- 输出 517,329 行，514,762 条指令。
- **坑**：objdump 指令行首是 3 空格 + **7位hex**（省略前导0），且 body 是
  `字节hex \t 助记符 \t 操作数`，须按 `\t` 拆分，不能 `startswith('push')`。
- capstone 遇非法指令（数据混排）会停，**必须用 objdump**（force-thumb 跨数据）。

### 2. 函数边界表（R0 核心）
- 7480 个函数（`push {...,lr}` 入口）。
- 已生成 `analysis/func_boundaries.txt`（`start end size`，16进制）。

### 3. 调用图（R0 核心）
- 24,642 条直接调用边（`bl`），3,519 条间接调用（`blx rX`）。
- 已生成 `analysis/callgraph.txt`（`caller -> callee`，16进制）。
- 热点函数（被调最多，逆向优先级最高）：
  `0xb4edc(590) 0x82d3c(465) 0x23bec(379) 0x6e9cc(350) 0x23ae8(344)`
  `0x85798(319) 0xb4bd0(292) 0x25988(276) 0x85858(272) 0x857b0(270)`

### 4. JNI 入口（14 个，已确认）
```
0x233e0 OnEvent    0x233f4 OnInput   0x23408 OnResult  0x23424 OnBluetooth
0x23438 OnTouch    0x2344c OnKey     0x23460 OnTimer   0x23474 OnUpdate
0x23488 OnGraphChange 0x2349c OnGraphCreate 0x234b0 OnResume 0x234c4 OnPause
0x23560 OnCreate   0x235d4 OnDestroy
```

### 5. .rodata 明文（R0 已有）
- `/disk/workspaces/jq/frida-dump/rodata_dec.bin`（118,832B）
- `rodata_strings.txt`（1729 条）：
  - 事件名 `Junqi.Move/Flip/Flag/Bomb/Eat/Dead/RedWin/BlueWin/...`
  - 资源 `data/config/{ui,font,image,sound,string,path,uitag}.xml` + `data/{320,480,...}/`
  - 服务器 `updateUrl/statHost/cfgHost`

## 结论
R0 地基完成：函数边界表 + 调用图 + 完整反汇编 + JNI 表 + .rodata 明文映射，
全部落盘，供 R1-R6 直接复用。

## 规范（R0 复用管线判据）
- 反汇编必须用 **objdump**（capstone 遇非法指令会停），`force-thumb` 跨数据段。
- 指令行解析：3 空格 + 7 位 hex（省略前导0），body 按 `\t` 拆分。
- 函数边界：`push {...,lr}` 入口（7480 个）；调用图：`bl` 直接 + `blx rX` 间接。
- JNI 表：14 个 `Java_com_cnvcs_App_On*` 入口地址（0x233e0 起）。
- `.rodata` 明文：`frida-dump/rodata_dec.bin`（redroid 壳自解密 dump）。

## 复用指引
- 任意函数地址 → `func_boundaries.txt` 查范围。
- 任意调用链 → `callgraph.txt` 追。
- 字符串语义 → `rodata_strings.map`（下一步生成偏移映射）。
- 规范 → `docs/CONVENTIONS.md`；经验 → `docs/LESSONS.md`。
