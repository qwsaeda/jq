# JNI 入口调用图 (Phase 1 锚点映射)
# so: libjunqi_arm32_aligned.so (9.1MB, 有符号表)
# 生成: 2026-11-17
# 方法: objdump -d 提取 44269 条 bl/blx 指令, 按 15 个 JNI 入口地址范围过滤

## 调用图汇总

| 函数 | 起始 | 大小 | 独立 bl 目标 | 关键符号调用 |
|------|------|------|-------------|-------------|
| JNI_OnLoad | 0x32492c | 0x49c | 7 | jqt_mem_init, jqt_init_empty_containers, jqt_build_event_base, jqt_jni_resolve_only |
| OnCreate | 0x324dc8 | 0x1584 | 23 | jq_game_test_hook, jq_native_event_hook, F_b9edc, F_75210, _Znwj, F_24250, F_be9cc, F_23560, F_2a380, F_3ab80 |
| OnDestroy | 0x32636c | 0xb0 | 1 | jqt_jni_setenv |
| OnPause | 0x32641c | 0x60 | 1 | jqt_jni_setenv |
| OnResume | 0x32647c | 0xf8 | 4 | jqt_jni_setenv, jqt_jni_selftest, __aeabi_memclr |
| OnUpdate | 0x326574 | 0x250 | 9 | jqt_jni_setenv, jqt_jni_selftest, F_9e2b4, F_23475 |
| OnEvent | 0x3267c4 | 0x68 | 1 | jqt_jni_setenv |
| OnResult | 0x32682c | 0x6c | 1 | jqt_jni_setenv |
| OnKey | 0x326898 | 0x68 | 1 | jqt_jni_setenv |
| OnBluetooth | 0x326900 | 0x64 | 1 | jqt_jni_setenv |
| OnGraphCreate | 0x326964 | 0x15c | 6 | jqt_jni_setenv, jqt_jni_selftest, __aeabi_memclr |
| OnGraphChange | 0x326ac0 | 0x13c | 4 | jqt_jni_setenv, jqt_jni_selftest |
| OnTimer | 0x326b7c | 0x594 | 14 | jqt_jni_setenv, jqt_jni_selftest, _Znwj, F_2af44, jq_rule_probe_run, F_23461 |
| OnInput | 0x327110 | 0x64 | 1 | jqt_jni_setenv |
| OnTouch | 0x327174 | 0x400 | 8 | jqt_OnTouch, __aeabi_memclr |

## 调用模式分析

### 通用模式 (所有 JNI 入口)
```
入口 → jqt_jni_setenv (获取 JNIEnv*)
     → jqt_jni_selftest (验证环境)
     → [业务逻辑]
     → __aeabi_memclr* (memset 封装, 大量使用)
     → _Znwj (malloc 封装, C++ new)
     → jqt_mem_addr (.bss 地址访问)
     → 返回
```

### 关键业务函数 (F_xxxx 内部函数)

| 函数 | 地址 | 调用者 | 推测功能 |
|------|------|--------|----------|
| F_b9edc | 0x439194 | OnCreate | 初始化 (C++ 对象构造) |
| F_75210 | 0x6c8a8c | OnCreate | 资源加载 / 事件注册 |
| F_24250 | 0x3bd5c0 | OnCreate | 游戏状态初始化 |
| F_be9cc | 0x5f150c | OnCreate | 配置解析 |
| F_23560 | 0x4385b8 | OnCreate | 渲染器初始化 |
| F_2a380 | 0x7ef984 | OnCreate | 高级配置 |
| F_3ab80 | 0x7ef300 | OnCreate | 事件发送器 (F_3ab80 已在 REVERSE_NOTES 中收表) |
| F_9e2b4 | 0x511d54 | OnUpdate | 帧更新核心逻辑 |
| F_23475 | 0x472cdc | OnUpdate | 渲染调度 |
| F_2af44 | 0x7f5cac | OnTimer | AI 决策 / 定时器处理 |
| F_23461 | 0x471b38 | OnTimer | 辅助定时器 |

### 关键调用图 (Mermaid)

```mermaid
graph TD
    subgraph "JNI 入口"
        A1[JNI_OnLoad] --> B1[jqt_mem_init]
        A1 --> B2[jqt_init_empty_containers]
        A1 --> B3[jqt_build_event_base]
        
        A2[OnCreate] --> C1[jq_game_test_hook]
        A2 --> C2[jq_native_event_hook]
        A2 --> D1[F_b9edc]
        A2 --> D2[F_75210]
        A2 --> D3[F_24250]
        A2 --> D4[F_be9cc]
        A2 --> D5[F_23560]
        A2 --> D6[F_2a380]
        A2 --> D7[F_3ab80]
        
        A3[OnUpdate] --> E1[F_9e2b4]
        A3 --> E2[F_23475]
        
        A4[OnTimer] --> F1[F_2af44]
        A4 --> F2[F_23461]
        A4 --> F3[jq_rule_probe_run]
        
        A5[OnTouch] --> G1[jqt_OnTouch]
    end
    
    subgraph "C++ 业务层"
        D1[D2[D3[D4[D5[D6[D7
        E1 --> E2
        F1 --> F2
    end
```

## 逆向优先级建议

### P0 - 先读这些 (理解核心逻辑)
1. **F_9e2b4** (0x511d54) - OnUpdate 主循环核心
2. **F_2af44** (0x7f5cac) - OnTimer AI 决策
3. **jqt_OnTouch** (0x48edd4) - 触摸处理
4. **F_3ab80** (0x7ef300) - 事件发送器 (已有 REVERSE_NOTES)

### P1 - 状态初始化
5. **F_b9edc** (0x439194) - C++ 对象构造
6. **F_75210** (0x6c8a8c) - 资源/事件注册
7. **F_24250** (0x3bd5c0) - 游戏状态

### P2 - 辅助
8. F_be9cc / F_23560 / F_2a380 - 配置/渲染初始化

## 下一步行动

| 步骤 | 内容 | 时长 |
|------|------|------|
| 1 | 反汇编 F_9e2b4 (OnUpdate 核心), 识别控制流 | 2h |
| 2 | 反汇编 F_2af44 (AI 决策), 追踪 .bss 状态 | 2h |
| 3 | 追踪 jqt_OnTouch → 事件分发链 | 1h |
| 4 | 定位 baidu 字符串引用点 (0x2eaa78/0x2eaacc/0x2eaae8) | 1h |
| 5 | 整理 .bss 940KB 全局变量分类 | 2h |
