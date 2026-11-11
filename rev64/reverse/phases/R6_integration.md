# 阶段 R6 · 集成与验收（打包 + 签名 + 校验）

## 目标
- 把 R1-R5 产物（JNI 绑定、游戏状态、规则、渲染、SPAK 资源层）集成进可安装 APK。
- 产出 64 位设备可安装、arm64 库可加载的 `dist/phase_hand.apk`。
- 完整走一遍"重打包三件套"并验证签名/结构/资源完整性。

## 关键决策
- **三 ABI 共存**：`arm64-v8a`（手写标准军棋）+ `armeabi`（原版 32 位）+ `x86_64`。
  64 位设备走 arm64，32 位设备走 armeabi（原版完整逻辑）。
- **minSdk 19**：apktool.yml 默认沿用原版 minSdk 4，v1 SHA-256 签名在 API 9-17
  验证失败 → 改 19（AGENTS.md 阶段 0 规范本就要求）。
- **`.so` 必须 STORE**：apktool 默认压缩 .so，64 位安装失败 → `store_compressed.py`。

## 操作步骤（重打包三件套）

### 1. 解码 → 改 → 重打包
```bash
apktool d -f 军棋.apk -o /tmp/opencode/jq_apk/dec
# 替换 lib/arm64-v8a/libjunqi.so（最新编译产物）
cp dist/libjunqi_arm64_hand.so /tmp/opencode/jq_apk/dec/lib/arm64-v8a/libjunqi.so
# 改 apktool.yml minSdkVersion 4→19
apktool b /tmp/opencode/jq_apk/dec -o /tmp/opencode/jq_apk/out_hand.apk
```

### 2. .so 强制 STORE
```bash
python3 /tmp/opencode/jq_apk/store_compressed.py /tmp/opencode/jq_apk/out_hand.apk \
  /tmp/opencode/jq_apk/out_hand_stored.apk
```

### 3. v1+v2 签名（minSdk 19 必须 v1 SHA-256，v2 新版必须）
```bash
java -cp /tmp/opencode/apksigner/apksig.jar:/tmp/opencode/apksigner SignV2 \
  /tmp/opencode/jq_apk/out_hand_stored.apk /disk/workspaces/jq/junqi.keystore \
  junqi2026 junqi junqi2026
```

## 验收规范（R6 判据）

### 签名验证（官方 apksigner）
```bash
APKSIGNER=/disk/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/741/fs/opt/android/build-tools/36.0.0/apksigner
$APKSIGNER verify --verbose --print-certs dist/phase_hand.apk
# 期望：Verifies；v1/v2/v3 scheme 全部 true
```

### 结构校验
- `lib/arm64-v8a/libjunqi.so` compress_type=0（STORE）。
- APK 内 arm64 库 MD5 == 本地编译产物 MD5（确保是最新版）。
- `AndroidManifest.xml` 头 == 0x00080003（合法二进制资源，否则真机"解析包失败"）。
- assets/ 下 SPAK `.pak` 全部在（data0/1、datah0/1、datam、datax0..3）。

### 安装校验
- 64 位 only 设备：`pm install` 成功，`dlopen libjunqi.so` 成功。
- 32 位设备：走 `armeabi` 原版完整逻辑。

## 经验（→ LESSONS）
- L5：安装失败排查顺序（.so 压缩→签名→ABI→arsc 对齐）。
- L7：真机 32 位警告（arm64 库解决）。
- L22：蓝红镜像放置（军旗各在自己底线）。

## 复用指引
- 重打包流程在 `AGENTS.md` 核心规范（三件套）与 `docs/LESSONS.md` L5。
- 每次改 `src/*.c` 后：重编 arm64 → 替换 dec → apktool b → store → SignV2 → 验证 MD5/签名。

## 已验证（本轮）
- apktool b 成功 → store → SignV2（14454784B）。
- `apksigner verify --verbose`：**Verifies**，v1/v2/v3 全部 true，签名者 CN=JunqiLocal。
- arm64 库 MD5 与编译产物一致（29600B 触屏+AI 版）；`.so` STORE；SPAK .pak 全部在。
- 产物 `dist/phase_hand.apk`（14454784B）。

## 触屏交互 + AI + 音频 + 悔棋（本轮新增，R3/R4/R5/R6）
- OnTouch 坐标→格映射（`col=x*5/w`, `row=y*12/h`），选中→走子/翻棋状态机。
- `junqi_render_set_selected(idx)` 高亮选中格。
- 单机对弈：蓝方玩家触屏，红方 `jq_ai_move` 在 OnUpdate 自动回应。
- 音效：`junqi_audio_play(event)` OpenSL ES 播放（move/eat/flip/select/win/lose/start）。
- 悔棋/重开：`jq_undo`（OnEvent "undo"）、重载布局（OnEvent "restart"）。
- 反汇编确认 OnTouch 调用 `jq_cell_index/jq_try_move/jq_flip/junqi_render_set_selected`。
- 主机验证：60 格映射全正确；AI 对弈 29 步不卡死；悔棋源/目标格全恢复。

## 最终产物（全部验证通过）
- `dist/libjunqi_arm64_hand.so`（41480B，14 JNI 导出，jq_*/render/spak/png_decode/audio/notify_game_over 全进库）。
- `dist/phase_hand.apk`（14467072B）：`apksigner verify` Verifies（v1/v2/v3 true），
  arm64 MD5 一致、STORE，SPAK `.pak`×9，manifest 合法。

## 胜负通知（本轮新增，L31）
- native→Java Toast：`notify_game_over(env)`，stage==OVER → `Toast.makeText(activity,"蓝方胜/红方胜",LENGTH_SHORT).show()`。
- `g_activity` 全局引用（OnCreate NewGlobalRef / OnDestroy DeleteGlobalRef），`g_over_notified` 防重复（restart 重置）。
- OnTouch/OnUpdate 检测胜负后调用；与 `win/lose` 音效协同。

## HUD（本轮新增，L32）
- 顶部条：当前方彩块（蓝/红）+ 双方剩余子数（`pieces_left`），5×7 像素字模数字。
- 无 TTF 依赖（原版 TTF `data_font_all.ttf` 存在但无 FreeType，像素字模是诚实替代）。
- `draw_quad` 前向声明（定义在后部）。

## 困毙判定（本轮新增，L33）
- 胜负两途径：吃军旗 → 胜；对方无子可动 → 胜（困毙）。
- `side_has_movable`（暗棋可翻算可动）+ `check_stalemate`（move/flip 后调用）。
- 验证：红方全灭可动子 → stage=OVER(2)/winner=蓝(0)。

## 棋盘渲染修复（本轮新增，L35）
- 棋盘 12×5（60格）确认：default.lay 30字节/方，军旗底线，格方形。
- 棋盘居中 + 格方形（5:12），纹理映射棋盘主体（PNG 中央 0.269-0.730）。
- `junqi_render_touch_cell` 与渲染共用 board_region，touch 同步（棋盘外 -1）。

## AI 音效区分（本轮新增，L36）
- `jq_ai_move` 返回 int（0=无/1=移动/2=翻棋/3=吃子），OnUpdate 按动作播音效。
- AI vs AI 完整对局 43 步终局（winner=蓝，不无限循环）。

## 暗棋开局 + 困毙修正（本轮新增，L37）
- bug 修复：`jq_place` revealed=1 → load_layout 末尾统一置暗（50 子全暗棋开局）。
- 暗棋：未翻禁走、翻棋切 turn、AI 先翻再走。
- 困毙修正：暗棋可翻计数排除军旗/地雷。
- 综合回归 15/15 通过（暗棋开局/翻棋切turn/困毙/AI 64步终局）。

## 棋子渲染 + 多步悔棋 + 终局禁用（本轮新增，L38-L42）
- 棋子 rank 数字渲染替代误判 UI 图标（L38）；军棋规则完整验证（L39）。
- 多步悔棋（循环栈 8 层，L40）；终局输入禁用 + 悔棋选中重置（L41）。
- 空格不画色块（棋盘纹理露出，L42）。

## 诚实边界
- **能装**：签名/结构/STORE 全部验证通过 → 64 位设备可安装。
- **能玩**：需真实 arm64 设备（有 GPU）验证 dlopen+OnCreate+渲染。redroid 无 GPU +
  无 arm64 native bridge，只能验证"可装 + 32 位库 dlopen + 壳自解密"。
- 本库为标准军棋规则骨架，**非**原版精确行为（特定变体/AI/联机协议）。
