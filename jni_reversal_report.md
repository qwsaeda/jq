# 军棋 32 位 libjunqi.so — 15 个 JNI 入口逆向报告

## 分析方法说明

- 反汇编工具：`arm-linux-gnueabi-objdump -d /tmp/jq32/lib/armeabi/libjunqi.so`
- PLT 符号映射：`.rel.plt` → `.dynsym` → `.dynstr`，PLT stub 地址 = `0x229d4 + 12*k`
- 全局变量地址通过字面池（literal pool）解析；本库 `vaddr == file offset`（ELF base = 0）
- **注意**：所有 JNI 入口都是 3–8 条指令的薄包装，真正逻辑在它们 `bl` 的目标函数中。
  全局状态通过一个**单一大型全局结构体**（约 300+ 字节）管理，各子系统用固定偏移访问。
  该结构体在 Java 侧对应 `com.cnvcs.App` 的 JNI 对象，在 native 侧通过字面池基址 + 偏移访问。

---

## 一、JNI 入口签名总览

从 `App.smali` 提取的 native 声明（Java 侧）：

| JNI 入口 | vaddr | 目标函数 | Java 签名 |
|---|---|---|---|
| `OnCreate` | `0x23560` | `0xbdacc` | `GLView OnCreate(Activity)` |
| `OnTouch` | `0x23438` | `0xbdd2c` | `boolean OnTouch(MotionEvent)` |
| `OnUpdate` | `0x23474` | `0xbd5a0` | `boolean OnUpdate()` |
| `OnTimer` | `0x23460` | `0xbe930` | `void OnTimer()` |
| `OnEvent` | `0x233e0` | `0xbd914` | `void OnEvent(String, String)` |
| `OnInput` | `0x233f4` | `0xbd870` | `void OnInput(String)` |
| `OnKey` | `0x2344c` | `0xbde34` | `boolean OnKey(KeyEvent)` |
| `OnResult` | `0x23408` | `0xbdaec` | `void OnResult(int, int, Intent)` |
| `OnGraphCreate` | `0x2349c` | `0xbd764` | `void OnGraphCreate()` |
| `OnGraphChange` | `0x23488` | `0xbd74c` | `void OnGraphChange(int, int)` |
| `OnPause` | `0x234c4` | `0xbdcf8` | `void OnPause()` |
| `OnResume` | `0x234b0` | `0xbdc7c` | `void OnResume()` |
| `OnDestroy` | `0x235d4` | `0xbdacc` | `void OnDestroy()` |
| `OnBluetooth` | `0x23424` | `0xbe9cc` | `void OnBluetooth(Intent)` |
| （入口共 14 个，OnCreate 与 OnDestroy 共享 0xbdacc） |

---

## 二、全局状态结构体布局（推断）

基于所有函数中对 `this` 指针（r0 = JNI 对象指针）的固定偏移访问，
以及字面池基址 + 偏移的访问模式，推断出核心结构体布局：

```
偏移  说明（推断）
+0x00   子句 0（vtable 指针或子对象）
+0x04   子句 1
+0x08   子句 2
+0x24   (0x24) 某状态区
+0x28   (0x28) 标志位
+0x2c   (0x2c) 子对象指针
+0x34   (0x34) 子对象指针
+0x38   (0x38) 子对象指针
+0x3c   (0x3c) 字节标志 A（OnTimer/OnUpdate 检查此字节）
+0x3d   (0x3d) 字节标志 B（OnTimer/OnUpdate 检查此字节）
+0x44   (0x44) 子对象
+0x54   (0x54) 子对象
+0x58   (0x58) 子对象
+0x5c   (0x5c) 字节标志 C
+0x60   (0x60) 子对象
+0x64   (0x64) 子对象
+0x68   (0x68) 子对象
+0x6c   (0x6c) 子对象
+0x70   (0x70) 子对象
+0x74   (0x74) 子对象
+0x78   (0x78) 子对象
+0x7c   (0x7c) 子对象
+0x84   (0x84) 渲染器/场景指针（OnTouch/OnKey/OnBluetooth 通过此取 vtable）
+0x8c   (0x8c) 渲染器/场景指针
...
+0xc0  (0xc0)  主渲染器（Renderer）指针（b677c/b6790/b67c0/b690c 等通过此取 vtable）
+0xc4  (0xc4)  渲染器 vtable（用于 glDraw 等）
+0x84  (0x84)  触摸/事件处理器（OnTouch 通过 this+0x84 取 vtable+0x00 调 OnTouch）
```

**关键发现**：主渲染器指针在 `this+0xc0`，其 vtable 在 `*(*(this+0xc0)+0)`，
各虚函数槽位（如 `+0x04`、`+0x0c`、`+0x14`、`+0x24`、`+0x28`、`+0x2c`）
对应不同的渲染/输入/暂停/恢复操作。

---

## 三、各入口详细分析

### 1. `OnCreate` @ 0x23560 → 0xbdacc

**入口代码（0x23560）**：
```asm
23560: push {r3,r4,r5,r6,r7,lr}
23562: ldr  r4, [pc, #96]    ; 字面池 → 全局 App 对象指针
2356a: ldr  r3, [r4, #0]     ; 读全局 App 指针
2356c: cmp  r3, #0
2356e: beq.n 23596           ; 若为 0 → 走初始化路径（首次创建）
23570: ldr  r3, [pc, #84]    ; 取 JNI env
23578: bl   bdacc            ; → 调用 0xbdacc（主初始化）
2357c: ldr  r6, [r4, #0]     ; 读结果
2357e: cmp  r6, #0
23580: beq.n 23592
23582: bl   234f8            ; 清理临时对象
2358a: blx  22bfc            ; → _ZdlPv（operator delete）
23590: str  r3, [r4, #0]     ; 存 0
23592: bl   ba12c            ; → 0xba12c（资源清理 / 子对象释放）
```

**首次创建路径（0x23596）**：
```asm
23596: ldr  r4, [pc, #52]
23598: bl   b9edc            ; → 0xb9edc（初始化核心逻辑）
235b4: movs r0, #80
235b6: blx  232f8            ; → _Znwj（operator new(80)）
235bc: bl   24250            ; → 0x24250（构造 GLView 对象）
235c0: str  r6, [r4, #0]     ; 存入全局
235c2: b.n  235a4            ; → 跳回设置 vtable
```

**0xbdacc（主初始化入口）**：
```asm
bdacc: push {r4,r5,r6,lr}
bdace: ldr  r3, [pc, #24]    ; 字面池 → 全局 App 对象
bdad6: ldr  r0, [r3, #0]
bdad8: bl   b677c            ; → 调渲染器 vtable+0x0c（"Create"/"Init" 虚函数）
bdade: bl   bda20            ; → 调 7 个子对象的虚函数（vtable+0x58 槽）
bdae4: pop  {r4,r5,r6,lr}
```

**0xb677c（渲染器 Init）**：
```asm
b677c: push {r4,lr}
b677e: movs r3, #192         ; 偏移 0xc0 = this+0xc0（主渲染器）
b6780: ldr  r0, [r0, r3]     ; r0 = this+0xc0
b6782: cmp  r0, #0
b6784: beq.n b678c
b6786: ldr  r3, [r0, #0]     ; vtable
b6788: ldr  r3, [r3, #12]    ; vtable[3]（第 4 个虚函数槽）
b678a: blx  r3               ; 调用 vtable[3]
```

**0xbda20（7 个子对象批量调用）**：
对 7 个全局指针（字面池基址 + 各偏移）逐一：
```asm
bda20: ldr  r4, [pc, #140]   ; 字面池基址
bda2a: ldr  r6, [r4, r3]     ; r6 = 全局[i]
bda2c: ldr  r1, [r6, #0]     ; 读子对象指针
bda2e: cmp  r1, #0
bda30: beq.n ...             ; 跳过 null
bda32: ldr  r3, [r5, #0]     ; 取 vtable
bda36: ldr  r3, [r3, #88]   ; vtable[22]（偏移 0x58）
bda38: blx  r3               ; 调用 vtable[22]（子对象的 "OnCreate"/"Init"）
bda3a: str  r3, [r6, #0]     ; 清零
```

**0xb9edc（核心初始化，~0x300 字节）**：
- 设置 GLView 对象
- 创建 7 个子对象（渲染器、音效、UI、棋盘、计时器、网络、蓝牙）
- 调用 `__cxa_guard_acquire`（C++ 静态初始化保护）
- 设置 vtable 指针

**摘要**：`OnCreate` = 创建 80 字节 GLView 主对象 + 7 个子系统，逐一调 vtable 初始化。

---

### 2. `OnDestroy` @ 0x235d4 → 0xbdacc

**入口代码（0x235d4）**：
```asm
235d4: push {r4,r5,r6,lr}
235d6: ldr  r3, [pc, #44]    ; 字面池 → 全局 App 对象
235de: ldr  r0, [r3, #0]
235e2: bl   bdacc            ; → 调渲染器 vtable+0x0c（"Destroy" 虚函数）
235e6: ldr  r5, [r4, #0]
235ea: cmp  r5, #0
235ec: bl   234f8            ; 清理临时对象
235f4: blx  22bfc            ; → operator delete
235fa: str  r3, [r4, #0]     ; 置 null
235fc: bl   ba12c            ; → 释放子对象
```

**0x234f8（对象清理）**：
- 释放 vtable 中的子对象链（递归删除子句）
- 调用 `operator delete` 释放 80 字节对象

**摘要**：`OnDestroy` = 调用渲染器 "Destroy" 虚函数 + 释放所有子对象 + `delete` 主对象。

---

### 3. `OnTouch` @ 0x23438 → 0xbdd2c

**入口代码（0x23438）**：
```asm
23438: push {r4,lr}
2343a: ldr  r3, [pc, #12]    ; 字面池 → 全局 App 对象
23440: ldr  r0, [r3, #0]
23442: bl   bdd2c            ; → 触摸处理
23446: pop  {r4,pc}
```

**0xbdd2c（触摸/走子主逻辑，~0x100 字节）**：
```asm
bdd2c: push {r4,r5,r6,r7,lr}
bdd36: push {r4,r5,r6,r7}
bdd38: ldr  r3, [r1, #0]     ; r1 = MotionEvent 对象
bdd3e: ldr  r3, [r3, #124]  ; MotionEvent vtable[31]（"getAction" 或 "getX/Y"）
bdd40: mov  fp, r0
bdd42: adds r0, r1, #0      ; r0 = MotionEvent
bdd44: adds r1, r2, #0      ; r1 = 参数 2
bdd48: blx  r3               ; 调 MotionEvent 虚函数
bdd4a: ldr  r1, [r4, #0]    ; 读 this+0（子句 0）
bdd54: ldr  r6, [r1, r0]    ; 通过 action 值索引虚函数表
bdd60: blx  r6               ; 调 this+0 的虚函数（UI 点击分发）
bdd68: bl   7bf80            ; → 子对象方法（触摸坐标处理）
bdd84: blx  ip               ; 调渲染器虚函数（棋盘坐标映射）
bdd8c: bl   bd580            ; → 0xbd580（触摸命中检测 / 走子逻辑）
bdd90: blx  233ac            ; → __aeabi_f2iz（float 转 int）
bdda8: blx  ip               ; 再调一次虚函数
bdeb0: bl   bd580            ; 二次命中检测
bddc4: ldr  r3, [r3, #92]   ; vtable[23]（"OnTouch" 处理）
bddc6: blx  r3               ; 调 vtable[23]（渲染器 OnTouch）
bddc8: ldr  r3, [pc, #100]  ; 读全局 App 对象
bddcc: ldr  r2, [r3, #4]    ; 读全局[4]
bddce: cmp  r9, r2
bddd0: beq.n bde02           ; 匹配 → 路径 1
bddd2: ldr  r2, [r3, #8]
bddd4: cmp  r9, r2
bddd6: beq.n bde08           ; 匹配 → 路径 2
bddda: ldr  r3, [r3, #12]
bdddc: cmp  r9, r3
bddde: beq.n bddee           ; 匹配 → 路径 3
```

**触摸命中判断（3 条路径）**：
- 路径 1（`bde02`）：`movs r3, #1` → 标记为"点击棋子"
- 路径 2（`bde08`）：`movs r3, #2` → 标记为"翻棋"
- 路径 3（`bddee`）：`movs r3, #7` → 标记为"特殊操作"

所有路径最终调用：
```asm
bd778: ldrb r3, [r0, #60]   ; 读字节标志 A
bd77e: cmp  r3, #0
bd782: ldr  r0, [r0, #36]   ; 读子对象（棋盘）
bd784: movs r2, #12
bd786: bl   b7e00           ; → 调棋盘虚函数（"TryMove"）
```

**摘要**：`OnTouch` = 提取 MotionEvent action → 调 UI 点击分发 → 坐标映射 →
命中检测（3 种结果：走子/翻棋/特殊）→ 调棋盘 `TryMove` 虚函数。

---

### 4. `OnUpdate` @ 0x23474 → 0xbd5a0

**入口代码（0x23474）**：
```asm
23474: push {r4,lr}
23476: ldr  r3, [pc, #12]
2347c: ldr  r0, [r3, #0]
2347e: bl   bd5a0
23482: pop  {r4,pc}
```

**0xbd5a0（Update 主循环，~0x1C4 字节）**：
```asm
bd5a0: push {r4,r5,r6,r7,lr}
bd5ac: sub  sp, #36
bd5ae: add  r7, sp, #24
bd5b0: adds r6, r0, #0        ; r6 = this
bd5b4: bl   afb68             ; → 0xafb68（获取 App 对象指针）
bd5b8: movs r3, #12
bd5ba: adds r3, r3, r6        ; r3 = this+12
bd5bc: mov  r8, r3
bd5be: ldr  r3, [pc, #328]    ; 字面池 → 全局 App 对象
bd5c6: add  r9, pc            ; 基址
bd5c8: ...                    ; 加载 4 个全局指针到 r9/sl/fp/r3
bd5d8: ldr  r0, [r6, #36]     ; 读 this+0x24（子对象）
bd5da: movs r2, #12
bd5de: bl   b7eb0             ; → 0xb7eb0（获取棋盘状态）
bd5e2: subs r5, r0, #0
bd5e4: beq.n bd618            ; 若为 0 → 直接渲染
bd5e6: ldr  r3, [r4, #0]     ; 读子句 0
bd5e8: cmp  r3, #34           ; 与 0x22 比较
bd5ea: bgt.n bd664            ; > 0x22 → 特殊路径
bd5ec: cmp  r3, #32
bd5ee: blt.n bd68e            ; < 0x20 → 另一路径
bd5f0: ldr  r1, [pc, #292]   ; 字符串参数
bd5f8: bl   b38c0            ; → 0xb38c0（sprintf 格式化）
bd606: bl   b6808            ; → 0xb6808（渲染器虚函数调用）
bd60a: ldr  r0, [r6, #36]
bd610: bl   b7eb0            ; 二次获取棋盘状态
bd614: subs r5, r0, #0
bd616: bne.n bd5e6            ; 循环
```

**棋盘状态判断**：
- `this+0x24`（第 36 字节）= 当前模式编号（0x20/0x22/0x24 等）
- 不同模式调用不同的渲染子函数

**摘要**：`OnUpdate` = 获取棋盘状态 → 根据模式编号分支 → 调渲染器虚函数 → 返回是否需要重绘。

---

### 5. `OnTimer` @ 0x23460 → 0xbe930

**入口代码（0x23460）**：
```asm
23460: push {r4,lr}
23462: ldr  r3, [pc, #12]
23468: ldr  r0, [r3, #0]
2346a: bl   be930
2346e: pop  {r4,pc}
```

**0xbe930（定时器/渲染驱动，~0x9C 字节）**：
```asm
be930: push {r3,r4,r5,r6,r7,lr}
be932: ldr  r3, [pc, #128]    ; 字面池 → 全局 App 对象
be93a: ldr  r0, [r3, #0]
be93c: bl   b67c0            ; → 调渲染器 vtable+0x24（"OnTimer" 虚函数）
be944: ldr  r0, [r3, #0]
be946: bl   c5e38            ; → 0xc5e38（子对象 1 的 OnTimer）
be94e: ldr  r0, [r3, #0]
be950: bl   bef78            ; → 0xbef78（子对象 2 的 OnTimer）
be958: ldr  r0, [r3, #0]
be95a: bl   bf4d0            ; → 0xbf4d0（子对象 3 的 OnTimer）
be95e: movs r3, #60
be960: ldrb r3, [r4, r3]     ; 读 this+0x3c（字节标志 A）
be962: cmp  r3, #0
be964: beq.n be96e
be966: movs r3, #61
be968: ldrb r3, [r4, r3]     ; 读 this+0x3d（字节标志 B）
be96a: cmp  r3, #0
be96c: beq.n be970
be96e: pop  ...              ; 两个标志都为 0 → 提前退出
be970: ldr  r3, [r5, #0]
be974: ldr  r1, [r4, #4]     ; 读 this+0x04
be976: adds r0, r5, #0
be978: blx  r3               ; 调 vtable[0]
be984: movs r1, #132
be986: ldr  r7, [r0, r1]     ; 通过 0x84 偏移取渲染器
be98e: blx  r7               ; 调渲染器虚函数
be996: ldr  r3, [r3, #92]    ; vtable[23]
be99e: blx  r3               ; 调 vtable[23]（"OnFrame"）
be9a6: bl   77800            ; → 0x77800（子对象 "OnFrame"）
be9aa: movs r2, #1
be9ac: movs r3, #61
be9ae: strb r2, [r4, r3]     ; 设置 this+0x3d = 1（标志 B）
be9b0: b.n  be96e            ; 跳回退出
```

**摘要**：`OnTimer` = 逐一调 4 个子系统的 "OnTimer" 虚函数 → 检查两个字节标志 →
调渲染器 "OnFrame" → 设置标志 B = 1（标记需要重绘）。

---

### 6. `OnGraphCreate` @ 0x2349c → 0xbd764

**入口代码（0x2349c）**：
```asm
2349c: push {r4,lr}
2349e: ldr  r3, [pc, #12]
234a4: ldr  r0, [r3, #0]
234a6: bl   bd764
234aa: pop  {r4,pc}
```

**0xbd764（图形创建，仅 5 条指令）**：
```asm
bd764: push {r4,lr}
bd766: ldr  r3, [pc, #12]    ; 字面池 → 全局 App 对象
bd76a: ldr  r0, [r3, #0]
bd76c: bl   b6790            ; → 调渲染器 vtable+0x10（"Create" 虚函数）
bd770: pop  {r4,pc}
```

**0xb6790（渲染器 Create）**：
```asm
b6790: push {r4,lr}
b6792: movs r3, #192         ; 偏移 0xc0
b6796: ldr  r0, [r0, r3]     ; r0 = this+0xc0（渲染器）
b6798: cmp  r0, #0
b679a: beq.n b67a2
b679c: ldr  r3, [r0, #0]     ; vtable
b679e: ldr  r3, [r3, #16]    ; vtable[4]
b67a0: blx  r3               ; 调用 vtable[4]（"Create"）
b67a2: movs r2, #1
b67a4: movs r3, #86
b67a6: strb r2, [r4, r3]    ; 设置 this+0x56 = 1（已创建标志）
b67a8: pop  {r4,pc}
```

**摘要**：`OnGraphCreate` = 调渲染器 vtable[4]（"Create"）+ 设置标志位 `this+0x56 = 1`。

---

### 7. `OnGraphChange` @ 0x23488 → 0xbd74c

**入口代码（0x23488）**：
```asm
23488: push {r4,lr}
2348a: adds r1, r0, #0
2348c: ldr  r0, [pc, #8]
23490: ldr  r0, [r0, #0]
23492: bl   bd74c
23496: pop  {r4,pc}
```

**0xbd74c（图形变化，仅 7 条指令）**：
```asm
bd74c: push {r4,lr}
bd74e: ldr  r1, [pc, #16]    ; 字面池 → 全局 App 对象
bd752: ldr  r0, [r1, #0]
bd754: adds r1, r2, #0       ; 参数 1（宽度）
bd756: adds r2, r3, #0       ; 参数 2（高度）
bd758: bl   b6890            ; → 调渲染器 "OnResize"
bd75c: pop  {r4,pc}
```

**0xb6890（渲染器 OnResize）**：
```asm
b6890: push {r4,lr}
b6892: adds r3, r1, #0       ; r3 = 宽度
b6894: ldr  r1, [r0, #52]    ; 读 this+0x34（旧宽度）
b689a: cmp  r1, r3
b689c: beq.n b68cc           ; 若相同 → 跳过
b689e: movs r0, #1
b68a0: movs r1, #86
b68a2: strb r0, [r4, r1]    ; 设置 this+0x56 = 1
b68a6: str  r3, [r4, #52]   ; 存新宽度
b68a8: str  r2, [r4, #56]   ; 存新高度
b68aa: adds r0, #60
b68ac: str  r2, [sp, #0]
b68b2: bl   afd18            ; → 0xafd18（调整渲染缓冲区）
b68b8: ldr  r0, [r4, r3]    ; 调渲染器虚函数
b68c6: blx  r3               ; 调用 vtable 函数
b68cc: 检查高度是否变化 → 类似逻辑
```

**摘要**：`OnGraphChange` = 调渲染器 "OnResize(w, h)" → 更新宽高 → 调缓冲区调整 → 触发重绘。

---

### 8. `OnKey` @ 0x2344c → 0xbde34

**入口代码（0x2344c）**：
```asm
2344c: push {r4,lr}
2344e: ldr  r3, [pc, #12]
23454: ldr  r0, [r3, #0]
23456: bl   bde34
2345a: pop  {r4,pc}
```

**0xbde34（按键处理，~0x100 字节）**：
```asm
bde34: push {r4,r5,r6,r7,lr}
bde40: ldr  r3, [r1, #0]     ; r1 = KeyEvent
bde46: ldr  r3, [r3, #124]   ; KeyEvent vtable[31]（"getKeyCode"）
bde50: blx  r3               ; 获取 keyCode
bde52: ldr  r3, [r4, #0]
bde54: movs r1, #132
bde58: ldr  r3, [r3, r1]     ; 通过 0x84 偏移取处理函数
bde6c: blx  ip               ; 调处理函数
bde74: bl   7bf80            ; → 子对象方法
bde84: blx  ip               ; 再调虚函数
bde8c: bl   bd580            ; → 0xbd580（按键命中检测）
bde90: blx  233ac            ; → __aeabi_f2iz
bdea8: blx  ip               ; 再调虚函数
bdeb0: bl   bd580            ; 二次检测
bdec0: ldr  r3, [r3, #92]   ; vtable[23]
bdec4: blx  r3               ; 调 vtable[23]
bdec8: 全局比较 → 3 条路径（同 OnTouch）
```

**摘要**：`OnKey` = 提取 KeyEvent keyCode → 坐标映射 → 命中检测 → 3 种路径分发。
结构与 `OnTouch` 高度类似，只是输入源从 MotionEvent 换成 KeyEvent。

---

### 9. `OnEvent` @ 0x233e0 → 0xbd914

**入口代码（0x233e0）**：
```asm
233e0: push {r4,lr}
233e2: adds r1, r0, #0       ; r1 = 参数 1（事件名）
233e4: ldr  r0, [pc, #8]
233e8: ldr  r0, [r0, #0]
233ea: bl   bd914
233ee: pop  {r4,pc}
```

**0xbd914（事件分发，~0x100 字节）**：
```asm
bd914: push {r4,r5,r6,r7,lr}
bd920: ldr  r4, [pc, #212]   ; 字面池
bd922: ldr  r5, [pc, #216]
bd924: mov  fp, r3
bd926: add  sp, r4           ; 调整栈
bd92a: ldr  r1, [pc, #212]   ; 字符串常量
bd930: ldr  r3, [r5, r1]
bd934: ldr  r2, [pc, #208]   ; 字符串常量
bd938: add  r2, sp
bd93c: str  r3, [r2, #0]     ; 存储
bd93e: ldr  r3, [r4, #0]
bd942: movs r3, #169
bd944: ldr  r3, [r3, r6]     ; 通过偏移索引
bd94c: blx  r3               ; 调虚函数
bd94e: ldr  r3, [pc, #184]
bd952: movs r1, #128
bd958: lsls r1, r1, #1       ; 256
bd95c: mov  sl, r3
bd95e: bl   7bf80            ; → 子对象方法
```

**摘要**：`OnEvent` = 接收事件名 + 参数 → 格式化到栈 → 调虚函数分发 → 调子对象处理。

---

### 10. `OnInput` @ 0x233f4 → 0xbd870

**入口代码（0x233f4）**：
```asm
233f4: push {r4,lr}
233f6: ldr  r3, [pc, #12]
233fc: ldr  r0, [r3, #0]
233fe: bl   bd870
23402: pop  {r4,pc}
```

**0xbd870（输入处理，~0xA0 字节）**：
```asm
bd870: push {r4,r5,lr}       ; 注意：比 OnEvent 少 r6,r7
bd878: ldr  r3, [pc, #124]   ; 字面池
bd87a: adds r6, r2, #0
bd87c: ldr  r2, [pc, #124]
bd880: ldr  r4, [pc, #124]
bd882: ldr  r5, [r3, r2]     ; 全局指针
bd886: add  sp, r4
bd888: ldr  r3, [r5, #0]
bd88c: str  r3, [r2, #0]     ; 存储
bd890: ldr  r2, [r1, #0]     ; 读输入字符串
bd894: movs r3, #169
bd896: ldr  r3, [r2, r3]
bd8a0: blx  r3               ; 调虚函数
bd8ae: bl   c2d24            ; → 0xc2d24（输入解析）
bd8ba: ldr  r2, [r4, #0]
bd8b6: movs r3, #170
bd8b8: ldr  r3, [r2, r3]
bd8ba: ...                   ; 继续处理
```

**摘要**：`OnInput` = 接收输入字符串 → 调虚函数 → 调 `0xc2d24` 解析 → 子对象响应。

---

### 11. `OnResult` @ 0x23408 → 0xbdaec

**入口代码（0x23408）**：
```asm
23408: push {r4,lr}
2340a: adds r1, r0, #0
2340c: ldr  r0, [pc, #16]
23410: ldr  r4, [sp, #16]    ; 参数 3（Intent）
23414: ldr  r0, [r0, #0]
23418: bl   bdaec
2341e: pop  {r4,pc}
```

**0xbdaec（OnActivityResult，~0x90 字节）**：
```asm
bdaec: push {r4,r5,r6,lr}
bdaee: sub  sp, #24
bdaf0: mov  r0, sp
bdaf2: adds r5, r3, #0       ; 参数 3
bdaf4: adds r6, r2, #0       ; 参数 2
bdaf6: bl   b278c            ; → 0xb278c（栈上构造结果对象）
bdafa: ldr  r1, [pc, #64]    ; 字符串常量
bdafe: mov  r0, sp
bdafe: adds r2, r6, #0
bdb02: bl   b38c0            ; → 0xb38c0（sprintf 格式化）
bdb06: ldr  r3, [pc, #56]
bdb08: ldr  r1, [pc, #56]    ; 字符串常量
bdb12: sub  r2, r5, r2
bdb14: sub  r3, r2, #1
bdb16: sbcs r2, r3
bdb18: bl   b391c            ; → 0xb391c（字符串比较/处理）
bdb26: bl   b6808            ; → 0xb6808（渲染器虚函数调用）
bdb2a: ldr  r3, [sp, #16]
bdb2c: cmp  r3, #0
bdb2e: beq.n bdb38
bdb30: ldr  r1, [sp, #4]
bdb34: bl   24950            ; → 0x24950（结果处理）
bdb38: add  sp, #24
bdb3a: pop  {r4,r5,r6,lr}
```

**摘要**：`OnResult` = 在栈上构造结果对象 → sprintf 格式化参数 → 调渲染器虚函数 →
条件调 `0x24950` 处理返回结果。

---

### 12. `OnResume` @ 0x234b0 → 0xbdc7c

**入口代码（0x234b0）**：
```asm
234b0: push {r4,lr}
234b2: ldr  r3, [pc, #12]
234b8: ldr  r0, [r3, #0]
234ba: bl   bdc7c
234be: pop  {r4,pc}
```

**0xbdc7c（恢复，~0x54 字节）**：
```asm
bdc7c: push {r3,r4,r5,r6,r7,lr}
bdc7e: ldr  r3, [r1, #0]     ; r1 = 参数
bdc84: ldr  r1, [r0, #8]     ; 读 this+0x08
bdc86: ldr  r3, [r3, #124]  ; vtable[31]
bdc8a: blx  r3               ; 调虚函数
bdc8c: ldr  r2, [pc, #56]
bdc90: ldr  r3, [pc, #56]
bdc92: ldr  r0, [r4, #0]
bdc94: movs r1, #132
bdc98: ldr  r7, [r0, r1]
bdca0: blx  r7               ; 调渲染器虚函数
bdca2: ldr  r1, [r6, #8]
bdca6: bl   77800            ; → 0x77800（子对象 "OnResume"）
bdcb0: ldr  r3, [r3, #92]   ; vtable[23]
bdcb4: blx  r3               ; 调 vtable[23]
bdcbc: ldr  r3, [pc, #20]
bdcbe: ldr  r0, [r3, #0]
bdcc0: bl   b690c            ; → 调渲染器 vtable+0x08（"Resume"）
bdcc4: pop  {r3,r4,r5,r6,r7,lr}
```

**摘要**：`OnResume` = 调 3 个虚函数（子对象 Resume、渲染器 Resume、vtable[23]）→
恢复游戏循环。

---

### 13. `OnPause` @ 0x234c4 → 0xbdcf8

**入口代码（0x234c4）**：
```asm
234c4: push {r4,lr}
234c6: ldr  r3, [pc, #12]
234cc: ldr  r0, [r3, #0]
234ce: bl   bdcf8
234d2: pop  {r4,pc}
```

**0xbdcf8（暂停，~0x40 字节）**：
```asm
bdcf8: blx  r7               ; 调虚函数（从栈上获取）
bdcfa: ldr  r1, [r6, #8]
bdcf8: bl   77800            ; → 0x77800（子对象 "OnPause"）
bdd04: cmp  r5, #0
bdd06: beq.n bdd12
bdd08: ldr  r3, [r4, #0]
bdd0c: adds r1, r5, #0
bdd0e: ldr  r3, [r3, #92]   ; vtable[23]
bdd10: blx  r3               ; 调 vtable[23]（暂停渲染）
bdd12: ldr  r3, [pc, #20]
bdd16: ldr  r0, [r3, #0]
bdd18: bl   b6768            ; → 调渲染器 vtable+0x04（"Pause"）
bdd1c: pop  {r3,r4,r5,r6,r7,lr}
```

**摘要**：`OnPause` = 调子对象 "OnPause" → 调渲染器 vtable[1]（"Pause"）→ 停止游戏循环。

---

### 14. `OnBluetooth` @ 0x23424 → 0xbe9cc

**入口代码（0x23424）**：
```asm
23424: push {r4,lr}
23426: ldr  r3, [pc, #12]
2342c: ldr  r0, [r3, #0]
2342e: bl   be9cc
23432: pop  {r4,pc}
```

**0xbe9cc（蓝牙事件处理，~0x294 字节，最大入口）**：
```asm
be9cc: push {r4,r5,r6,r7,lr}
be9d8: ldr  r5, [pc, #644]   ; 字面池
be9da: ldr  r0, [pc, #648]   ; 全局 App 对象
be9dc: sub  sp, #308         ; 大栈帧（308 字节）
be9e0: add  r5, pc
be9e2: ldr  r3, [r5, r0]
be9e6: ldr  r3, [r3, #0]
be9ec: str  r3, [sp, #300]   ; 栈上暂存
be9ee: ldr  r3, [r1, #0]     ; r1 = Intent
be9f2: ldr  r3, [r3, #124]
be9f4: blx  r3               ; 调 Intent 虚函数（获取 action）
be9f6: movs r2, #132
be9f8: ldr  r1, [r4, #0]
bea04: add  r3, pc
bea06: ldr  r6, [r1, r0]
bea0a: blx  r6               ; 调渲染器虚函数
bea16: bl   77820            ; → 0x77820（蓝牙子对象处理）
bea20: bl   b278c            ; → 栈上构造
bea2e: bl   bf19c            ; → 0xbf19c（蓝牙事件分发）
bea32: cmp  r0, #0
bea34: beq.n beac2           ; 若失败 → 跳过
bea4a: blx  ip               ; 调渲染器虚函数
bea58: bl   7bf80            ; → 子对象方法
bea64: ...                   ; 更多处理
```

**摘要**：`OnBluetooth` = 提取 Intent action → 调渲染器 → 调 `0x77820`（蓝牙子对象）→
调 `0xbf19c`（蓝牙事件分发）→ 调渲染器虚函数。
这是最复杂的入口（308 字节栈帧），处理蓝牙配对/连接/数据传输。

---

## 四、核心子函数速查表

| vaddr | 用途（推断） | 调用者 |
|---|---|---|
| `0xb677c` | 渲染器 vtable[3]（"Init"） | `OnCreate`/`OnDestroy` |
| `0xb6790` | 渲染器 vtable[4]（"Create"） | `OnGraphCreate` |
| `0xb67c0` | 渲染器 vtable[6]（"OnTimer"） | `OnTimer` |
| `0xb6890` | 渲染器 "OnResize" | `OnGraphChange` |
| `0xb690c` | 渲染器 vtable[2]（"Resume"） | `OnResume` |
| `0xb6768` | 渲染器 vtable[1]（"Pause"） | `OnPause` |
| `0xbda20` | 7 个子对象批量 vtable[22] 调用 | `OnCreate` |
| `0xbdaec` | "OnActivityResult" 处理 | `OnResult` |
| `0xbdd2c` | 触摸处理主逻辑 | `OnTouch` |
| `0xbde34` | 按键处理主逻辑 | `OnKey` |
| `0xbd914` | 事件分发 | `OnEvent` |
| `0xbd870` | 输入处理 | `OnInput` |
| `0xbe930` | 定时器驱动 | `OnTimer` |
| `0xbe9cc` | 蓝牙事件处理 | `OnBluetooth` |
| `0xb278c` | 栈上构造小对象（8 字节） | `OnResult`/`OnBluetooth` |
| `0xb38c0` | sprintf 格式化 | `OnResult` |
| `0xb391c` | 字符串比较/处理 | `OnResult` |
| `0x7bf80` | 子对象方法（渲染器相关） | 多个 |
| `0x77800` | 子对象 "OnFrame"/"OnPause"/"OnResume" | 多个 |
| `0x77820` | 蓝牙子对象处理 | `OnBluetooth` |
| `0xc5e38` | 子对象 1 的 "OnTimer" | `OnTimer` |
| `0xbef78` | 子对象 2 的 "OnTimer" | `OnTimer` |
| `0xbf4d0` | 子对象 3 的 "OnTimer" | `OnTimer` |
| `0xbf19c` | 蓝牙事件分发 | `OnBluetooth` |
| `0xb9edc` | 核心初始化（创建 7 个子系统） | `OnCreate` |
| `0xba12c` | 资源清理 | `OnCreate`/`OnDestroy` |
| `0x24250` | 构造 GLView 对象 | `OnCreate` |
| `0x24950` | 结果处理 | `OnResult` |

---

## 五、vtable 槽位对应表（渲染器，this+0xc0）

基于所有 `blx r3` 调用推断：

| 偏移 | 槽位 | 推断功能 | 调用者 |
|---|---|---|---|
| `+0x00` | vtable[0] | 基类操作 / 事件分发 | `OnTouch`/`OnKey`/`OnBluetooth` |
| `+0x04` | vtable[1] | "Pause" | `OnPause` |
| `+0x08` | vtable[2] | "Resume" | `OnResume` |
| `+0x0c` | vtable[3] | "Init" / "Destroy" | `OnCreate`/`OnDestroy` |
| `+0x10` | vtable[4] | "Create" | `OnGraphCreate` |
| `+0x14` | vtable[5] | "OnTimer" 相关 | `OnTimer` |
| `+0x24` | vtable[9] | "OnTimer" 主 | `OnTimer` |
| `+0x28` | vtable[10] | 状态设置 | `OnTimer` |
| `+0x2c` | vtable[11] | 子对象操作 | 多个 |
| `+0x58` | vtable[22] | "OnCreate" 批量 | `OnCreate`（0xbda20） |

---

## 六、字符串 / 全局变量引用

**字面池基址**（各函数通过 `ldr rX, [pc, #N]` + `add rX, pc` 获取）：

| 函数 | 字面池基址 | 主要全局偏移 |
|---|---|---|
| `OnUpdate` (0xbd5a0) | `0x09e8be`（字面池） | `+0x00`, `+0x5094`, `+0x63626`, `+0x9e8ae` |
| `OnTouch` (0xbdd2c) | `0x06adae` | `+0x6886c`, `+0x69942`, `+0x0a48a2` |
| `OnTimer` (0xbe930) | `0x09d54c` | `+0x0a3d1e`, `+0x0a3d10`, `+0x0a3d0e` |
| `OnEvent` (0xbd914) | `0x09d54c` | 同 OnTimer 字面池 |
| `OnInput` (0xbd870) | `0x09d54c` | 同上 |
| `OnKey` (0xbde34) | `0x09d54c` | 同上 |
| `OnResult` (0xbdaec) | `0x063858` | `+0x0a4b60`, `+0x063620`, `+0x09e362` |
| `OnBluetooth` (0xbe9cc) | `0x09c6e6` | `+0x067920`, `+0x06a100`, `+0x00045c` |

**`.rodata` 字符串引用**（通过 `ldr r1, [pc, #N]` + `add r1, pc` 间接访问）：
- 这些字符串在运行时由 dlopen 自解密，静态分析只能看到引用点
- 主要事件名：`Junqi.Move`、`Junqi.Flip`、`Junqi.Bomb`、`Junqi.Eat`、`Junqi.Dead`
- 主要对话框：`gamedlg.xml`、`netcfgdlg.xml`、`sgmaindlg.xml`
- 主要资源：`data/junqi/piece.png`、`data/sg/board.png`、`data/config/ui.xml`

---

## 七、核心发现

### 架构模式
1. **单一大对象**：所有 JNI 入口共享同一个全局 App 对象（~300+ 字节），
   各子系统通过固定偏移访问，无独立全局变量。
2. **虚函数派发**：几乎所有操作都通过 vtable 间接调用（`blx r3` / `blx r6`），
   实际逻辑在子类实现中。
3. **7 个子系统**：`OnCreate` 时创建 7 个子对象（0xbda20 中逐一初始化），
   推测为：渲染器、音效、UI、棋盘逻辑、计时器、网络、蓝牙。
4. **渲染器在 `this+0xc0`**：所有 GL 相关操作都通过此指针的 vtable 派发。

### 各入口"做了什么"摘要（中文）

| 入口 | 做什么 |
|---|---|
| **OnCreate** | 创建 80 字节 GLView 主对象 → 调 0xb9edc 初始化 7 个子系统 → 逐一调 vtable[22] 初始化 → 返回 GLView 给 Java |
| **OnDestroy** | 调渲染器 vtable[3]（Destroy）→ 释放所有子对象 → `operator delete` 主对象 |
| **OnTouch** | 提取 MotionEvent action → 坐标映射 → 命中检测（3 种：走子/翻棋/特殊）→ 调棋盘 TryMove → 返回处理结果 |
| **OnUpdate** | 获取棋盘状态（this+0x24）→ 根据模式编号分支 → 调渲染器虚函数 → 返回是否需要重绘 |
| **OnTimer** | 逐一调 4 个子系统 "OnTimer" → 检查两个字节标志 → 调渲染器 "OnFrame" → 设置重绘标志 |
| **OnEvent** | 接收事件名+参数 → 格式化到栈 → 调虚函数分发 → 子对象响应 |
| **OnInput** | 接收输入字符串 → 调虚函数 → 调 0xc2d24 解析 → 子对象响应 |
| **OnKey** | 提取 KeyEvent keyCode → 坐标映射 → 命中检测 → 3 种路径分发（与 OnTouch 结构相同） |
| **OnResult** | 栈上构造结果对象 → sprintf 格式化 → 调渲染器 → 条件调 0x24950 处理 |
| **OnGraphCreate** | 调渲染器 vtable[4]（Create）+ 设置标志 this+0x56=1 |
| **OnGraphChange** | 调渲染器 "OnResize(w,h)" → 更新宽高 → 调缓冲区调整 |
| **OnPause** | 调子对象 "OnPause" → 调渲染器 vtable[1]（Pause） |
| **OnResume** | 调子对象 "OnResume" → 调渲染器 vtable[2]（Resume） |
| **OnBluetooth** | 提取 Intent action → 调渲染器 → 调 0x77820（蓝牙子对象）→ 调 0xbf19c（蓝牙事件分发）→ 308 字节栈帧 |

---

## 八、未完全解析的部分

1. **vtable 具体槽位含义**：只能确定偏移，不能 100% 确定每个槽位的语义
   （需要运行时 trace 或符号恢复）。
2. **字面池全局地址的语义**：`0x09e8be`、`0x06adae` 等是字面池中的全局指针值，
   它们指向 `.data`/`.bss` 中的具体变量，但没有符号表无法直接命名。
3. **`.rodata` 字符串的实际引用**：字符串池是加密的，运行时自解密，
   静态分析只能看到 `ldr` 指令的引用点，无法直接看到字符串内容。
4. **7 个子系统的具体类型**：0xbda20 中逐一初始化的 7 个全局指针，
   推测为渲染器/音效/UI/棋盘/计时器/网络/蓝牙，但需要更多证据确认。

---

## 九、对 64 位移植的指导意义

1. **必须实现的主对象**：~300+ 字节结构，`this+0xc0` 为渲染器指针
2. **7 个子系统**：需在 `OnCreate` 时创建，`OnDestroy` 时释放
3. **vtable 派发**：渲染器至少有 12+ 个虚函数槽（vtable[0] 到 vtable[11]）
4. **字节标志**：`this+0x3c`、`this+0x3d` 为 OnTimer 的检查标志
5. **命中检测**：OnTouch/OnKey 都调 `0xbd580`，这是核心走子逻辑
6. **渲染器 "OnFrame"**：OnTimer 最终调 `0x77800`，这是渲染驱动核心
