# 逆向移植规范（R0-R6 通用）

> 所有阶段统一遵守。违反 = 返工。

## 1. 地址与命名

- **地址基准**：一律用原版 `.so` 的**虚拟地址**（如 `0x24630`），不用文件偏移。
- **函数命名**：未知函数 `sub_XXXXXX`（XXXXXX = 8位hex 虚拟地址）。
- **已知函数**（JNI/库导出）：保留原符号名，如 `Java_com_cnvcs_App_OnCreate`。
- **Thumb 位**：跳转目标若为 Thumb 函数，用 `addr|1`（Bit0=1），与 `blx` 语义一致。
- **全局变量**：`g_XXXXXX`（XXXXXX = 数据段虚拟地址）。

## 2. 逆向顺序（每函数固定流程）

1. 用 `func_boundaries.txt` 查函数地址范围。
2. 用 `full_text_disasm3.txt`（objdump 完整反汇编）看该函数全部指令。
3. 识别 prologue（`push {...lr}`）保存的寄存器 → 这是本函数 live 寄存器。
4. 识别 epilogue（`pop {...pc}`）→ 返回值在 r0。
5. 按调用图追 `bl/blx` 目标，先逆向被调函数（自底向上）。

## 3. 调用图构建

- `bl <target>`：直接调用（Thumb 内，PC 相对）。
- `blx <target>`：可能是寄存器调用（`blx rX`）或间接（`blx <abs>`）。
- **间接调用难点**：`blx rX` 需先看 rX 来源（vtable 读 `ldr r3,[r3,#off]`），
  还原虚函数表偏移。
- JNI 调用：`ldr r3,[r1,#0]` + `ldr r3,[r3,#off]` + `blx r3` = 调 JNI 方法，
  off 是 JNI 函数表偏移（如 0x7c = getAction）。

## 4. 数据段还原

- `.rodata` 明文已 dump（`/disk/workspaces/jq/frida-dump/rodata_dec.bin`）。
- 字符串引用：`ldr r3,[pc,#off]` + `add r3,pc` + `ldr r0,[r3,#0]` = 取字符串指针。
  （Thumb 的 PC 偏移须 +4 对齐校正。）
- 全局对象：`ldr r3,[r4,#off]` 读结构体字段；字段偏移在 `analysis/state_layout.md`。

## 5. JNI 签名（红线：与 smali 完全一致，不改）

```
OnEvent(String)→void            OnInput(int,int)→void
OnResult(int,int)→void          OnBluetooth(Intent)→void
OnTouch(MotionEvent)→boolean    OnKey(int)→void
OnTimer()→void                  OnUpdate()→void
OnGraphChange(int,int)→void     OnGraphCreate(int,int)→void
OnResume()→void                 OnPause()→void
OnCreate(Activity)→GLView       OnDestroy()→void
```
GLView 是 `com.cnvcs.GLView`（SurfaceView 子类，含 GL 线程 + EGL helper）。

## 6. 移植到 arm64 的映射

| 32位 ARM | arm64 AArch64 | 说明 |
|----------|---------------|------|
| r0-r3 参数 | x0-x7 参数 | 前8参用寄存器 |
| r0 返回值 | x0/w0 返回值 | |
| push {..lr} | stp x29,x30,[sp,#-N]! | frame 建立 |
| pop {..pc} | ldp x29,x30,[sp],#N; ret | frame 销毁 |
| bl | bl | 相对调用 |
| blx rX | blr xX | 间接调用 |
| ldr r3,[r1,#0]+[r3,#off] | ldr x3,[x1]; ldr x3,[x3,#off] | JNI 方法表 |
| Thumb 16位 | 32位指令 | 2条 Thumb ≈ 1条 arm64 |

## 7. 内存模型注意事项

- 原版是 **32 位指针**，arm64 是 **64 位指针**。所有结构体布局须重新定义
  （`int` 仍 4B，但 `void*`/`jobject` 变 8B）。**这是最大返工源**。
- JNI `jobject` 在 arm64 是 8 字节句柄，原版代码按 4 字节读的字段偏移
  **不能直接搬**，须按 arm64 JNI 规范重排。
- 事件/状态标志用 `int`（4B）保持，避免依赖指针宽度的逻辑用 `uint32_t`。

## 8. 每阶段交付清单（提交前自查）

- [ ] `phases/Rx_*.md`：目标 / 操作步骤 / 验证记录 / 结论
- [ ] `docs/Rx_*.md`：该阶段可复用知识（函数地址、数据布局、坑）
- [ ] 代码编译通过（NDK r25b，`-march=armv8-a`）
- [ ] 若有运行行为：mumu 验收截图 + 崩溃计数
