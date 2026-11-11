# so→Java 反向回调补齐计划（P0 最小集）

> 基于原版 `libjunqi.so` `.rodata` 全量提取（82 个 JNI 签名 + ~45 类名，集中在 `0x114b40–0x117e00`）。
> 现状：仅 `GLView.<init>` + `setContentView` 一条链已接通（§124u/x，MuMu 主菜单渲染）。
> 其余 81 条在机械翻译体里全是 `bl` 存根 r0=0（AGENTS 红线：bl 全存根双向对齐），**反向回调面基本没接**。

## 现状事实（核实记录）

- 正向 14 个 `On*` JNI 入口已补齐（`jni_aligned.c`，签名与 `App.smali` 一致）
- 反向：`jni_aligned.c` 只有 `jq_jni_make_glview`（FindClass GLView + NewObject + setContentView），用 `g_jni_env`+`g_activity`
- 翻译体 `jqt_*.c` 里没有 JNI bridge 机制——机械翻译时 `bl` 全存根，JNI 边界只能**适配层补语义**
- 关键 JNI 串偏移已定位（getParent 0x114c20 / registerReceiver 0x1155f8 / openFileInput 0x117308 / AudioTrack 0x11631c / BtReceiver 0x115668）
- `BtReceiver` 是 so 直接 FindClass（`com/cnvcs/BtReceiver`，0x115668），不是 smali 层，删 baidu 前必须保留
- 广告组（H：AdView/AdLis/广告 HTTP/adHost）阶段 3 全删，**不补**

## 关键设计：JQJNI 桥机制（本次新增）

**原则**：不再在 `jni_aligned.c` 适配层逐个 fallback（旧 `jq_jni_make_glview` 只补了 1 条），
而是建一个**统一 JNI 调用表 + 翻译体侧 lookup 钩子**，让 82 个 so→Java 调用都能走同一套
`FindClass/GetMethodID/CallXxx` 语义。

### 架构（3 件）

1. **`jqt_jnibr.c/.h`（新增）**：JNI bridge 层
   - `jqt_jni_call(uint32_t jni_id, uint32_t n_args, uint32_t *args)` —— 翻译体侧按 jni_id 调 Java
   - 维护 `g_jq_jni_calls[]` 表：`{cls_str_off, name, sig}` → `jclass/jmethodID` 缓存
   - 首次 FindClass+GetMethodID，之后复用（JNI 标准做法）
   - 入参/返回类型按签名分发：`(I)V`→CallVoidMethod, `()L...;`→CallObjectMethod, `(FF)I`→CallIntMethod 等

2. **`jq_jni_calls.json`（生成物，放 `/tmp/opencode/jq_jni/`）**：82 个 JNI 调用的结构化清单
   - `{id, cls, name, sig, ret_type, arg_types, rodata_off}`
   - 由脚本从 `.rodata` 提取（方法名\x00签名\x00 相邻 + 类名串反查）

3. **翻译体接线**：每个 so→Java 调用点（原 `bl` 存根）改调 `jqt_jni_call(id, ...)`
   - 不重生成 `jqt_*.c`（避免覆盖 wire_blx_dispatch），而是**适配层 override**：
     在 `jni_aligned.c` 里把对应 F_xxx 的 JNI 边界函数（如 F_b6790 内的 vtable 调用）替换成 jqt_jni_call
   - 或按 P0 逐个加 override 函数

### 类型映射（签名 → 返回 jtype）
```
()Landroid/...;      → jobject  CallObjectMethod
()V                  → void     CallVoidMethod
()Z                  → jboolean CallBooleanMethod
()I / (FF)I          → jint     CallIntMethod
()J                  → jlong    CallLongMethod
([B)I               → jint     CallIntMethod + GetPrimitiveArrayElements
(Lcom/.../AdView)   → jobject  CallObjectMethod
()Landroid/bluetooth/... → jobject
```

## 分档（对齐 4 阶段 + R6）

### P0 核心（不补则 arm32 翻译体 / arm64 对齐版"不能玩"）—— 先做

| 序号 | 组 | so→Java 调用 | 目的 | 落在哪 |
|---|---|---|---|---|
| 1 | A | `View.getParent()` | 渲染层移除旧 GLView | 渲染链 |
| 2 | A | `View.removeView(View)V` | 同上 | 渲染链 |
| 3 | A | `View.setVisibility(I)V` | 控件显隐 | UI |
| 4 | A | `View.getLayoutParams()` | 布局 | UI |
| 5 | A | `ViewGroup$LayoutParams.setMargins(IIII)V` | 边距 | UI |
| 6 | A | `View.requestLayout()` | 重排 | UI |
| 7 | A | `LinearLayout.<init>(Context)V` | 容器 | UI |
| 8 | A | `Activity.addContentView(View,LP)V` | 加内容 | UI |
| 9 | A | `ViewGroup$LayoutParams.<init>(II)V` | 布局 | UI |
| 10 | C | `TextDlg.<init>(Context,Str,Str,Str,I)V` | 认输/求和对话框 | 对局 |
| 11 | C | `SiteView.create(Activity,Str,Str,II,Z)V` | 站点视图 | 对局 |
| 12 | C | `Intent.getParcelableExtra(String)Parcelable` | 对话框结果 | 对局 |
| 13 | E | `Context.openFileInput(String)FileInputStream` | 读 one.sjg | 存档 |
| 14 | E | `Context.openFileOutput(String,I)FileOutputStream` | 写 one.sjg | 存档 |
| 15 | E | `Context.getAssets()AssetManager` | 资源 | 资源 |
| 16 | G | `BluetoothAdapter.getDefaultAdapter()` | 蓝牙入口 | 联机 |
| 17 | G | `BluetoothAdapter.startDiscovery()Z` | 扫描 | 联机 |
| 18 | G | `Context.registerReceiver(BtReceiver,IntentFilter)Intent` | 广播 | 联机 |
| 19 | G | `BluetoothDevice.createRfcommSocketToServiceRecord(UUID)` | 连接 | 联机 |

### P1 体验（完整）
- B：`getWindow/requestFeature/setFlags/getWindowManager`（全屏）
- D：`getSystemService/startActivity/stopService/getMemoryInfo/checkCallingOrSelfPermission`
- F：`AudioManager.<init>/AudioTrack` 全（音效）
- C：`Toast.makeText/show`
- E：`URL.openConnection` 全家（更新检查，可随广告一起去）

### P2 不补（阶段 3 拆广告删）
- H：`AdView` 全 / `AdLis` / 广告 HTTP / `adHost` 配置串

## 实施步骤

### Step 0：建桥机制（前置，1 天）
1. 写 `rev64/tools/gen_jni_calls.py`：扫 `.rodata`（0x114b40–0x117e00）提取 `{cls,name,sig,off}`
   → `/tmp/opencode/jq_jni/jq_jni_calls.json`（82 条，含类名反查）
2. 写 `rev64/reverse/src_jni_translated/jqt_jnibr.c/.h`：`jqt_jni_call(id,n,*)` + 缓存表 + 类型分发
3. 编译验证（arm32 库 + x86_64 harness 跑 `jqt_jni_call` 自测）

### Step 1：P0 接线（按组，每组 1 天）
**A 组（9 个，渲染/UI）**：
4. 从 `jq_jni_calls.json` 取 9 个 id，写 override 函数（仿 `jq_jni_make_glview`）
5. 接入渲染链（`F_b6790`/`F_24108` 的 vtable 边界点）
6. `incremental run` 回归 + MuMu 实机（渲染层出帧验证）

**C 组（3 个，对局 UI）**：
7. `TextDlg` 构造器 + `SiteView.create` 静态方法
8. MuMu 点"人机对战"→ 进对局验证（当前 §124u 实测"点击未进对局"，这步是关键）

**E 组（3 个，存档）**：
9. `openFileInput/Output/getAssets`
10. 对局结束存 `one.sjg` → 重启续局验证

**G 组（4 个，蓝牙最小）**：
11. `getDefaultAdapter/startDiscovery/registerReceiver/createRfcomm`
12. `BtReceiver` 保留（so 直接引用），只接最小链路；真机蓝牙验证后续

### Step 2：验证 + 记录
13. 每组做完跑：`rev64/build_arm32_aligned.sh` → `pack_arm32.sh` → MuMu `pm install` → 截图/触摸验证
14. `incremental run` 全量回归（PASS 冻结不变，MISMATCH 不增）
15. 更新 REVERSE_NOTES + AGENTS"已完成验证记录"（诚实边界：so→Java 接通 X/82 条）

### 风险 / 诚实边界
- **vtable 间接调用无法机械翻译**（AGENTS §124z 红线）：JNI 边界在 vtable 槽 → 只能适配层 override，
  不能 `bl` 存根改真调（会 C/unicorn 不对称假 MISMATCH）
- **BtReceiver 是 so 直接 FindClass**：阶段 3 删 baidu smali 前必须保留 + 处理 `FindClass("com/baidu/mobads/AdView")` 引用（AGENTS 红线）
- **GL 棋盘未出帧**（§124x/z 诚实边界）：P0 接通 so→Java 不自动让渲染出帧，渲染出帧还要修对象图（glClear 类 vtable=0x1595f4）
- **82 条不是全都要补**：H 组广告随阶段 3 删；E 组 HTTP 更新检查可删；真正对"能玩"必要的是 P0 的 A+C+E+G 共 19 条

## 不做的事
- 不重生成 `jqt_*.c`（避免覆盖 `wire_blx_dispatch.py`，§124z 顺序红线）
- 不把"接通 so→Java"包装成"能玩"——渲染出帧 + 触摸进对局才是能玩判据
- 不动广告组（P2），留阶段 3 统一删
