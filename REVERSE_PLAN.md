# 32→64 位完整对齐逆向方案

## 目标
把 32 位原版 `libjunqi.so`（1.4MB，GLES2 + SPAK + 完整规则）完整移植进 arm64-v8a，替代当前简化重写版。

## 已完成的逆向

### 1. JNI 15 入口（报告 jni_reversal_report.md）
全 3-8 指令薄 wrapper，bl 到核心单例（300+ 字节，7 子系统 vtable 分发）。
- OnCreate → 0xbdacc → new GLView(80) → 0xb9edc 建 7 子系统 → 0xbda20 batch-init via vtable[22]
- OnTouch → 0xbdd2c → MotionEvent action → coord → hit-test 0xbd580 → 3 结果（走子/翻棋/特殊）
- OnUpdate → 0xbd5a0 → 读 board 状态 this+0x24 → 分支 mode → renderer 虚调用 → redraw-needed
- OnTimer → 0xbe930 → 4 子系统 OnTimer → 2 标志位 → renderer OnFrame 0x77800 → redraw
- OnEvent → 0xbd914 → 事件名+arg 格式化 → 虚调用派发
- OnInput/OnKey/OnResult/OnGraphCreate/OnGraphChange/OnPause/OnResume/OnDestroy/OnBluetooth

### 2. GL 渲染管线（GLES2，纠正 AGENTS.md 的 GLES1 错误）
33 个 GL 导入：glCreateShader/glCreateProgram/glDrawArrays/glBindTexture/glTexImage2D/glGetAttribLocation...
- 每棋子 = 1 次 glDrawArrays(GL_TRIANGLE_STRIP, 0, 3)（3 顶点）
- 3 顶点属性（size 2/4/2 float），客户端数组（非 VBO）
- 3 纹理槽 [obj+36/44/52]，按纹理类型批量绑定
- 0x9e760 = 每帧渲染，0x9e838 = 棋子顶点重建，0x9e0f0/0x9e204 = 纹理 glGenTexture+glTexImage2D
- 0x9d624 = shader lazy init（glCreateProgram/glShaderSource/glCompileShader）
- 0x9d458 = 清屏+视口+MVP（glClear/glClearColor/glViewport/glUniformMatrix4fv）

### 3. SPAK 资源格式（已解出 header 结构）
- magic `SPAK` + version=1 + magic_num(各 pak 不同) + body_size + header16 + entry_count
- 9 个 pak：data0/h0/h1/x0-x3/m（.pak 在 assets/，doNotCompress）
- 资源路径：data/config/{ui,font,image,sound,string,path,uitag}.xml + data/{320,480,480x,720,720s,800}/
- 加载方式：C++ std::ifstream（0x1205f8 _M_open → open@plt），fopen 读 config
- **SPAK 体加密**：body[16:] 熵 7.99（加密），需逆向解密 key

## 待做（周级工程）

### 4. SPAK 解密 key（最高价值，阻塞 64 位资源加载）
- 逆向 0xc2da8（SPAK 体解析）的解密循环
- 找 XOR/AES key 在 .text 的明文常量
- 解出后 64 位能直接读原版资源图/音效/配置

### 5. 规则引擎（38 子完整布局 + 地雷/炸弹/悔棋 + 蓝牙联机）
- 逆向 0x77800（走子/吃子/炸弹）+ 0xbd580（hit-test）
- 38 子布局在 .rodata 加密段 + data/config/ui.xml，需 SPAK 解密后读
- 蓝牙联机协议（OnBluetooth → 0xbe9cc → 0x77820 → 0xbf19c）

### 6. 逐函数翻译 arm64
- 15 JNI 入口 + 核心单例 + 7 子系统 + GL 管线 + 规则引擎
- 全部用 GLES2 + 原版 SPAK 解密 + 完整 38 子规则 + 蓝牙协议
- 替换 native64/libjunqi_arm64.so

### 7. 重编 + 重打 4 个包
- NDK 编 arm64 + x86_64（redroid 测试）
- 官方 apksigner 签，zipalign，v1+v2+v3

## 关键卡点
- SPAK 解密 key 在 .text，需逆向 0xc2da8 的解密循环
- 38 子布局在 SPAK 加密资源里，需解密后读 data/config/ui.xml
- 蓝牙联机协议需逆向 0x77820 + 0xbf19c
