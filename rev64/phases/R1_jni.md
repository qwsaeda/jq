# 阶段 R1 · JNI 入口绑定（arm64 手写路线）

## 目标
- 14 个 `Java_com_cnvcs_App_On*` JNI 入口与 `smali/com/cnvcs/App.smali` 原生调用签名一致。
- 库可被 `System.loadLibrary("junqi")` 加载，`JNI_OnLoad` 返回正确版本。

## 规范（签名对齐，不可改）
| JNI 入口 | Java 签名 | 返回 |
|----------|-----------|------|
| OnCreate | (Landroid/app/Activity;)Lcom/cnvcs/GLView; | jobject（可 NULL 让 Java new GLView） |
| OnDestroy | ()V | void |
| OnPause / OnResume | ()V | void |
| OnGraphCreate | ()V | void |
| OnGraphChange | (II)V | void |
| OnTouch / OnKey | (II...)Z | jboolean |
| OnUpdate | ()Z | jboolean |
| OnEvent / OnInput / OnResult | (...)V | void |
| OnBluetooth | (Landroid/content/Intent;)V | void |
| OnTimer | ()V | void |

- `JNI_OnLoad` 返回 `JNI_VERSION_1_6`。
- **红线**：不改 `smali/com/cnvcs/App.smali` 的 native 调用签名；OnCreate 返回 NULL
  让 Java 侧走 `if-nez v0` 分支 new GLView。

## 接入（本轮）
- OnGraphCreate/OnGraphChange/OnUpdate → `junqi_render_init/frame`（互斥锁保护）。
- OnCreate → 优先 `spak_get("data/lay/default.lay")` + `jq_load_layout`，回退 `jq_dark_setup`。
- 全局：`g_state`（jq_state）、`g_mutex`（pthread）、`g_view_w/h`、`g_assets_dir`。

## 经验（→ LESSONS）
- L21：default.lay 布局接入（OnCreate 用真实布局）。
- L22：蓝红镜像放置（军旗各在自己底线）。
- OnCreate 中 `activity` 参数未用须 `(void)activity` 避免警告。

## 复用指引
- Java 侧 `App.smali` 是唯一签名来源（不改）。
- 新增 native 功能须在 `junqi_core.h` 声明 + JNI 包装（导出 `Java_com_cnvcs_App_On*`）。

## 已验证
- 14 个 JNI 导出（`readelf --dyn-syms` grep Java_com 计数=14）。
- OnCreate 返回 NULL（Java new GLView）；OnUpdate 返回 JNI_TRUE（消费帧）。
- 编译无 error（`-fvisibility=hidden`，仅导出 JNI）。

## 诚实边界
- 本库为标准军棋规则骨架，非原版逻辑；OnEvent/OnInput/OnResult/OnBluetooth/OnTimer
  目前为日志桩（联机/输入协议未逆向）。
