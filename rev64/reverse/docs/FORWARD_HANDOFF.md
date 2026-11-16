# 逆向 32 位 → 正向 64 位 数据交接清单（单向喂正向会话）

> AGENTS 红线：逆向侧产物（com.zxj / rev64/reverse/*）归逆向会话，正向
> （com.qwsaeda / rev64/forward/*）归正向会话，**不跨界抢活**。
> 本清单 = **逆向会话把提取的数据/经验单向交接给正向会话消费**（正向
> 会话读这些基准文件做行为级对齐，不反向改逆向代码）。

正向方法 = "先逆向/修正/frida/unidbg 动态补齐行为后用纯 C 重写"。
逆向 32 位的价值**不在产物**（9.53MB 镜像兜底不如正向 723KB 纯 C），
**在经验/数据**（原版 .rodata 事件名 + .text 反汇编行为 + frida 动态
补齐 + SPAK 资源 + 真机实锤基准 + 走子桥序列）——这些正是正向"动态
补齐"那一段的数据源。

## 6 类交接清单

### ① 原版 .rodata 事件名 74 逐名（正向事件契约对齐基准）
- **文件**：`rev64/reverse/src_jni_translated/jq_rodata_plain.h`
  （32 位原版 so .rodata 明文字符串池，74 个 Junqi.* 事件名 + Misc）
- **提取方法**：32 位 so `.rodata` 段明文（非加密，静态文件即明文），
  objdump 直接读字符串池（AGENTS"已验证现状事实"：.rodata 明文，
  静态与运行时 dump 逐字节相同 0/118832）
- **正向会话消费**：正向 src_rs 事件契约（jni_bridge.c 事件名 /
  evverify 事件断言）逐名对照 74 个 Junqi.*（Junqi.Move/Eat/Bomb/
  Flag/Dead/Flip/Draw*/Win*/GameOver 等），防正向事件名漂移
- **对照口径**：正向 64 位 sfx_event_to_wav / 事件名表须覆盖 74 逐名
  （§1925 事件三层漏斗 76→12→37 同源）

### ② 原版 32 位 .text 反汇编（规则引擎 11 函数行为级实锤源）
- **文件**：`/tmp/orig/lib/armeabi/libjunqi.so`（原版 32 位 so，
  .text 明文机器码，objdump 可反汇编）
- **提取方法**：objdump -d 反汇编 .text（AGENTS：.text 明文熵 7.10
  正常机器码，非加密）；规则引擎 11 函数（走子/吃子/炸弹/悔棋/翻棋/
  和棋/军旗/困毙）行为级锚点（0x302b4 困毙 / 0x30320 排地雷 /
  0x30348 可动判定 / 0x39e14 翻棋判定等）
- **正向会话消费**：正向 rules.c 行为级实锤源——每改一个规则函数，
  对照原版 .text 反汇编实锤行为（如 R-073 困毙排地雷 0x30320
  `cmp r1,#11 beq` = 排地雷，正向 rules.c 已对齐）
- **对照口径**：rule_probe 双端对照（32 位 OnCreate 属性钩子 + 64 位
  桌面探针 + diff，§124y-169 逐字节同源 20 用例）

### ③ frida-dump 明文（unidbg/frida 动态补齐数据源）
- **文件**：`frida-dump/rodata_dec.bin`（118KB .rodata 解密 dump）+
  `frida-dump/rodata_strings.txt`（1715 条字符串）+
  `frida-dump/rodata_plain_strings.txt`（.rodata 明文字符串）
- **提取方法**：Frida 18 地址 dump / Jnitrace 看 JNI 调用流 / .bss
  dump（AGENTS frida-dump 工作流，FRIDA_NOTES.md §9 Stalker 指令级
  追踪模板 / §十二 .bss dump 对照基准 / §十三 事件分发数据模型）
- **正向会话消费**：正向"动态补齐"那一段的数据源——原版运行时行为
  （事件表 289B / .bss 构造态 / 事件分发 74 事件名→17 事件码→42
  code→24 handler 全链）frida 提取基准，正向 src_rs 行为级对齐参照
- **对照口径**：frida dump 真值 vs 正向桌面断言（event_code_map /
  事件表 289B 三方对照 §1912）

### ④ SPAK 解密 key + 883 资源（正向资源加载对齐基准）
- **文件**：`rev64/dist/spak_res/`（970 文件 = 408 XML / 310 PNG / 22
  JPG / 18 WAV / 3 TTF 全量解包）+ SPAK 解密 key（32 字节明文 @ 原版
  .rodata VA 0x12a658，算法 XOR(key[j%32])+zlib）
- **提取方法**：`rev64/tools/spak_unpack.py` 完整解包 883 资源
  （AGENTS 已实锤 SPAK 解密 key 提取 + R5 完整解包）
- **正向会话消费**：正向 src_rs 资源加载对齐（14 WAV 音效 / 310 PNG
  纹理 / 408 XML 布局 / 3 TTF 字体），spak_res 全量推设备实锤出屏
- **对照口径**：正向 spak_load.c / 资源图部分对照 spak_res 全量
  （§1935 音效 14 WAV = SPAK 14 data_*.wav 同源）

### ⑤ 32 位真机 5 判据实锤记录（正向"能玩"验收基准）
- **文件**：`rev64/reverse/docs/REVERSE_LOG.md`（9298 处实锤记录，
  真机 5 判据 = 装/出屏/下一局/退出/重进）+ `rev64/reverse/docs/
  REAL_DEVICE_ACCEPTANCE.md`（真机 5 判据 SOP 逐步命令 + so 三方哈希
  核对）
- **提取方法**：红果 MuMu 10.0.182.146 真机实锤（32 位 so 转译跑通
  记录），device_regress_check.sh 8 步全链实锤达成记录（§1957）
- **正向会话消费**：正向 64 位"能玩"验收基准（AGENTS"64 位真机能玩
  验收标准"五判据同源——装/进主界面/下一局/退出/重进），正向设备侧
  实锤口径参照逆向 32 位真机记录（出屏 100% 非黑比 / 走子桥 8 步 /
  Junqi.* 事件通路）
- **对照口径**：正向 5 判据实锤 = 逆向 32 位 5 判据同源（AGENTS 验收
  五判据统一口径），模拟器结论不可外推真机（§124y-279）

### ⑥ 32 位走子桥 8 步序列（正向 60 局对弈行为级基准）
- **文件**：`rev64/forward/src_rs/test_move_seq.txt`（8 步序列
  f6/f33/m6-11/m33-38/m12-13/u/m10-11/q0）+ `rev64/reverse/src_jni_
  translated/jq_rs_walkbridge_test.c`（32 位走子桥 8 步基准，期望值
  Flip/Flip/Move/Move/ILLEGAL/Undo/ILLEGAL/Resign）
- **提取方法**：32 位翻译体 jq_rs_rules 引擎桌面 8/8 PASS（§1951）+
  64 位 test_move_seq 基准同源（§1931 走子桥 8 步 = 64 位基准）
- **正向会话消费**：正向 60 局对弈行为级基准（完整对局 50 子 12×5
  棋盘逐域行为级对照），jq_gametest 60 局离线断言同源
- **对照口径**：8 步序列 32/64 位同源（走子桥逐域行为级 8/8），
  完整 60 局 = 8 步序列扩展（§1939 jq_gametest 蓝走子10/红AI10/
  翻转20/困毙0）

## 交接方式（单向，不跨界抢活）

1. **本清单 = 逆向会话产出的交接文档**，正向会话读 6 类基准文件做行为
   级对齐（事件名 74 逐名 / .text 反汇编 11 函数 / frida 动态补齐 /
   SPAK 资源 / 真机 5 判据 / 走子桥 8 步）
2. **不反向改逆向代码**（AGENTS 红线"不抢活"）——正向会话消费逆向数据
   源，不碰 rev64/reverse/* 翻译体
3. **对照口径单向**：逆向提取的基准（.rodata 事件名 / .text 反汇编 /
   frida dump）→ 正向 src_rs 行为级对齐（正向 rules.c / jni_bridge.c /
   spak_load.c 对照基准），非逆向改产物追正向
4. **交接产物**：本清单 6 类文件路径 + 提取方法 + 对照口径，正向会话
   可直接消费（无需逆向会话介入，单向喂数据）

## 实锤（本清单 6 类全在位）

- ① 事件名 74 逐名（jq_rodata_plain.h，Junqi.* 74 个实列）
- ② .text 反汇编（原版 so 在位 /tmp/orig/lib/armeabi/libjunqi.so，
  规则引擎 11 函数行为级锚点）
- ③ frida-dump 明文（rodata_dec.bin 118KB + strings 1715 条）
- ④ SPAK 资源（spak_res 970 文件 + 14 WAV 实列）
- ⑤ 真机 5 判据实锤（REVERSE_LOG 9298 处 + REAL_DEVICE_ACCEPTANCE.md）
- ⑥ 走子桥 8 步序列（test_move_seq.txt + jq_rs_walkbridge_test.c）
