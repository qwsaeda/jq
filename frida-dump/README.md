# 军棋 32 位 native 反壳成果

> **干活会话系统参考**：`FRIDA_NOTES.md`（本目录）——Frida 在军棋工作流的完整用法
> （18 地址 dump 流程 / Jnitrace 看 JNI 调用流 / frida-server 部署 / 与 unidbg 两条路互补 /
> 跟陈佳林 3 本 Frida 书的关系 / 能干什么不能干什么）。unidbg 课程 ch13 13.4b 有指向本目录。

## 核心结论（已纠正）
- `libjunqi.so` 的 **`.text`（代码段）是明文**，不是加密壳！
  - 验证：redroid dump 出的 `.text` 段 与 `.so` 里的 `.text` 字节**完全一致**
  - 15 个 JNI wrapper 的函数体是真实的 ARM 指令（`push{r4,lr}; ldr r0,[pc,#8]...`）
  - 之前误判"自解密壳"是因为 `.rodata` 字符串池是高熵密文
- **`.rodata`（字符串/资源表）是加密的**，运行时才解密
  - 加密态熵 6.758，解密后变明文（低熵，含 `data/config/*.xml`、`Junqi.*` 事件名）
  - 通过 redroid 真 Android 环境跑原版 32 位壳，dlopen 后 dump 内存拿到解密态

## 已交付成果
- `rodata_strings.txt` — 1715 条解密后明文字符串，含：
  - `Junqi.Move/Flip/Flag/Bomb/Eat/Dead`（核心玩法事件）
  - `Junqi.LoginFail/Resign/Draw/Reconnecting`（联机/对局状态）
  - `data/config/{ui,font,image,sound,string,path,uitag}.xml`（资源路径）
  - `data/{320,480,480x,720,720s,800}/`（分辨率资源目录）
  - `updateUrl/statHost/cfgHost`（服务器配置）
- `rodata_dec.bin` — 解密后 `.rodata` 原始字节（116KB）
- `text_dec.bin` — `.text` 段（明文，与 .so 相同，1MB）
- 原始 dump `junqi_span_0b244000.bin`（1.43MB 完整映射）

## 环境
- redroid 容器（`registry.cn-hangzhou.aliyuncs.com/zywx/lingyan-face-check-service:android_tts_1210`）
  - x86 容器 + houdini x86 转译，跑 32 位 ARM `libjunqi.so`
  - 容器内 `su 0` 可用
  - 用法：`docker run --privileged -v 挂载:/mnt -p 5555:5555 ...` + `am start` 触发 dlopen + `dd /proc/PID/mem` dump

## 局限
- arm64 的 `libjunqi.so` 在 redroid 上跑不了（redroid 是 x86 容器跑 x86，转译 32 位 ARM）
- 32 位 v7a 库在 redroid 上 dlopen 成功，但 `OnCreate` 返回 null（GL 上下文初始化在 redroid 下不完整）
- 不影响 `.rodata`/`.text` 提取（dlopen 已触发解密）
