# 手写路线 R1-R6（arm64 标准军棋实现）阶段文档

> 二进制翻译路线（`tools/t32_to_a64.py`）在 R2 遇 `.data.rel.ro` 指针加宽硬墙
> （见 `phases/R2_got_plt.md`）后，**切换手写路线**：`src/*.c` 逐层还原。
> 本文件记录手写路线的 R1-R6 状态。

## 阶段目标（每阶段四要素：目标/文档/规范/经验）

| 阶段 | 目标 | 文档 | 规范 | 经验（→LESSONS） |
|------|------|------|------|------|
| R1 | JNI 入口绑定，14 个 `Java_com_cnvcs_App_On*` 签名与 App.smali 一致 | `phases/hand_route.md` | 不改 native 签名；OnCreate 返回 NULL 让 Java new GLView | L16-L20 |
| R2 | 游戏状态：12×5 棋盘、25 子/方、行营/大本营、明暗棋 | 同上 | 状态集中在 `jq_state`，互斥保护 | 同上 |
| R3 | 核心规则：走子/吃子/炸弹/军旗/地雷/翻棋/胜负 | 同上 | 标准中国军棋规则；`jq_can_move/try_move` 分层 | 同上 |
| R4 | 渲染：GLES2 棋盘色块骨架，JNI OnGraph* 驱动渲染，棋盘纹理注入接口 | `phases/hand_route.md` | GLES1 立即模式已被新 NDK 移除→GLES2；`junqi_render_init/frame/set_board_texture` | L23 |
| R5 | 联机/蓝牙/SPAK：**SPAK 资源解密完成** + **default.lay 布局接入游戏逻辑**，联机协议未逆向 | `analysis/spak_analysis.md` | SPAK=XOR+zlib；hash=MD5(路径)；default.lay 30字节降序映射 | L16-L22 |
| R6 | 打包 APK：arm64+armeabi+x86_64，v1+v2 签名 | 同上 | 重打包三件套；minSdk 19；.so STORE | L5/L7 |

## 重要声明（诚实边界）

- **本路线的 arm64 库是"标准中国军棋（陆战棋）规则"实现骨架，非原版 32 位
  libjunqi.so 的逐函数逆向产物。**
- 行为为标准军棋规则（走子/吃子/炸弹/军旗/地雷/悔棋/暗棋），**非**原版精确
  行为（原版有特定变体规则、AI 策略、资源路径、SPAK 加密）。
- **不冒充"完整版"**。完整匹配原版需从 `.text` 逐函数逆向（月级工程）。

## 阶段状态

| 阶段 | 内容 | 状态 | 产出 |
|------|------|------|------|
| R1 | JNI 入口绑定（14 个 `Java_com_cnvcs_App_On*`，签名对齐 App.smali） | ✅ | `src/junqi_jni.c` |
| R2 | 游戏状态（12×5 网格、每方 25 子、行营/大本营、明/暗棋） | ✅ | `src/game_state.c` |
| R3 | 核心规则（走子/吃子/炸弹/军旗/地雷/翻棋/胜负） | ✅ | `src/game_logic.c` |
| R4 | 渲染（GLES2 骨架，原版 GLES1 立即模式在新 NDK 不可编译） | ✅ | `src/render_gles2.c` |
| R5 | SPAK 资源解密（883 资源全解包）/ 联机协议 | ✅ 资源层 + 布局接入 / ⛔ 联机协议 | `src/spak_load.c` + `dist/spak_res/` |
| R6 | 打包 APK（arm64-v8a + armeabi + x86_64，v1+v2 签） | ✅ | `dist/phase_hand.apk` |

## 产物

- `src/junqi_core.h` — 公共定义（常量/枚举/结构/API）。
- `src/game_state.c` — 状态管理：`jq_state_init/reset`、`jq_place`、格子工具。
- `src/game_logic.c` — 规则：`jq_can_move/try_move/can_flip/flip/dark_setup/is_game_over/winner`。
- `src/junqi_jni.c` — JNI 绑定 + `JNI_OnLoad`（返回 JNI_VERSION_1_6）。OnGraphCreate/Change/Update 驱动渲染，OnCreate 用 default.lay 布局（回退暗棋）。
- `src/render_gles2.c` — GLES2 渲染（棋盘色块 + 边线，shader + 顶点缓冲）+ 真实棋盘纹理注入。
- `src/bt_spak.c` — R5 占位（联机协议未逆向）。
- `dist/libjunqi_arm64_hand.so` — 27KB arm64 库，14 JNI 导出，未定义符号纯 Android 库。
- `dist/phase_hand.apk` — 14.4MB，v1+v2+v3 签名验证通过（`Verifies`），
  minSdk 19，`lib/arm64-v8a/libjunqi.so`(STORE) + `lib/armeabi/libjunqi.so`(原版 32 位 STORE)。
- `src/spak_load.c` / `src/spak_load.h` — R5 SPAK 资源加载器（arm64 原生）。
- `dist/spak_res/` — **883 个 SPAK 解包资源** + `manifest.tsv`。
- `dist/spak_assets/` — 棋盘/棋子 PNG 素材副本。

## R4/R5 本轮新增（真实资源接入）

### default.lay 布局接入游戏逻辑（R5）
- **真实布局**：`data/lay/default.lay` = 30 字节 = 每方 6行×5列（含行营空位）。
- **值编码（降序）**：0=空；1=司令40,2=军长39,3=师长38,4=旅长37,5=团长36,6=营长35,
  7=连长34,8=排长33,9=工兵32,10=炸弹,11=地雷,12=军旗。
- **验证**：25 子/方分布与标准中国军棋精确匹配（司令1军长1师长2旅长2团长2营长2
  连长3排长3工兵3炸弹2地雷3军旗1）。
- **放置**：蓝方占棋盘行0-5（`行=5-lrow` 翻转，军旗→行0）；红方占行6-11
  （`行=6+lrow, 列=4-lcol` 镜像，军旗→行11）。
- API：`jq_load_layout(st, layout[30])`；JNI OnCreate 优先加载，失败回退暗棋随机。

### 渲染接入（R4）
- `junqi_render_init(w,h)`/`junqi_render_frame(st)`/`junqi_render_set_board_texture(rgba,w,h)`。
- JNI `OnGraphCreate/OnGraphChange/OnUpdate` 调用渲染（互斥锁保护）。
- GLES1 立即模式（原版）已被新 NDK 移除 → GLES2 shader + 顶点缓冲。

### 走子规则验证（R3/R4 边界）
- 空位相邻=可走；己方子/敌方行营/军旗/地雷=不可；大本营内子不可动。
- 铁路直线须同行/同列 + 中间全空；工兵 BFS 可拐弯。
- 吃子：大压小；同级/炸弹=同归于尽；工兵挖雷；其他撞雷=被炸；吃旗=胜。
- 低撞高是**允许**走法（`jq_can_move`=1，`jq_try_move` 才判负/送死）。

## R5 · SPAK 资源解密（完整验证，已完成）

**算法**：`SPAK = XOR(key[j%32]) + zlib`。key 32 字节 @ `.rodata` VA `0x12a658`：
`c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523`。

**头（24 字节）**：`SPAK` + version + f1(文件表偏移) + f2(文件表加密长) + f3(校验) + f4(条目数×28)。
**关键**：`f1+f2 = 合并流大小`，文件表在合并流末尾。合并流按组：
data(data0+data1)=1,236,875 / datah(datah0+datah1)=1,406,813 / datam=456,917 / datax(datax0..3)=3,103,732。

**条目（28 字节）**：`hash[16]+off[4]+size[4]+usize[4]`，`offset[i]=offset[i-1]+size[i-1]`。
**hash = MD5(资源路径)**（29/29 验证）。数据资源 = 数据区[off-24:...]，XOR+zlib。

**解包结果（883 资源）**：408 XML + 310 PNG + 22 JPG + 18 WAV + 3 TTF + 64 text + 58 其他。
- 音效 `data/junqi/{move,flip,flag,bomb,eat,dead,select,start,win,lose,draw,over}.wav`（对应 `.rodata` `Junqi.*` 事件名）。
- 字体 `data/font/{all,mb,sb}.ttf`、`936.cp`（GBK 码表）。
- 配置 `data/config/{path,sound,string,uitag}.xml`、`base.cfg`（含 `siteUrl http://www.cnvcs.com`、`serverHost 42.121.132.86`、DER 证书）。
- 布局 `data/lay/default.lay`（30 字节布阵）。
- 规则文本 `data/junqi/{rulelay,ruleflip,sentence}.txt`、`data/sg/{help,nethelp,sentence}.txt`。
- 棋盘素材 `datah#188`(737×751)、`datax#187`(720×736)、`datax#297`(736×736)；棋子格 72×72。

**工具**：`tools/spak_unpack.py`（可复用解包器）。**C 侧**：`spak_load.c`（`spak_load(dir)`/`spak_get(path)`，已编译链接 `-lz`）。

**验证**：4 组 `f1+f2==合并流` ✓；条目数 121/272/189/301 ✓；offset 链一致 ✓；XOR+zlib 解压成功 ✓；MD5(路径)==hash 29/29 ✓（Python 与 C RFC1321 结果一致）。

## 验证记录

### 编译（arm64，NDK r25b）
```
aarch64-linux-android24-clang -shared -fPIC -O2 -fvisibility=hidden -I src \
  src/game_state.c src/game_logic.c src/junqi_jni.c src/render_gles2.c src/bt_spak.c src/spak_load.c \
  -o dist/libjunqi_arm64_hand.so -llog -lGLESv2 -landroid -lz -lm
```
- 退出码 0，无 error（仅 const 限定警告，可忽略）。
- 产物：ELF64 aarch64，14 JNI 导出，未定义符号全为 liblog/libGLESv2/libandroid/libz/libm/libdl/libc。

### 打包（R6，AGENTS.md 重打包三件套）
1. `apktool d 军棋.apk -o dec` → 替换 `lib/arm64-v8a/libjunqi.so`（最新编译版）。
2. `apktool b dec -o raw.apk`。
3. `store_compressed.py raw.apk stored.apk`（强制 .so STORE）。
4. `SignV2 stored.apk keystore junqi2026 junqi junqi2026`（v1+v2）。
5. **关键修复**：`apktool.yml` minSdkVersion `'4'→'19'`（v1 SHA-256 签名在 API 9-17
   不通过；阶段 0 规范本就要求 minSdk 19）。

### 校验
- **签名**：`apksigner verify --verbose --print-certs` → `Verifies`，v1/v2/v3 全部 true，签名者 CN=JunqiLocal。
- **库一致性**：APK 内 `lib/arm64-v8a/libjunqi.so` MD5 == 本地编译产物 MD5（完全一致）。
- **SPAK 资源**：assets/ 下 `data0/data1/datah0/datah1/datam/datax0..3.pak`（9 个 .pak）全部在。
- **manifest 头**：`AndroidManifest.xml` 头 == 0x00080003（合法二进制资源）。
- 库压缩：`lib/arm64-v8a/libjunqi.so` compress_type=0（STORE）。

### 库符号完整性
- 14 个 `Java_com_cnvcs_App_On*` 导出（readelf grep Java_com 计数=14）。
- `spak_load/get/free`、`junqi_render_init/frame/set_board_texture`、`jq_load_layout/dark_setup`
  全部编译进库（nm 静态符号确认，未被优化掉）。
- 未定义符号纯 Android 库：liblog/libGLESv2/libz/libc/libm（+pthread）。

## 关键决策与教训

- **GLES1 立即模式不可用**：原版用 libGLESv1_CM（glBegin/glEnd）。但新 NDK 的
  GLES/gl.h 和 libGLESv1_CM.so stub **已移除立即模式符号**（glBegin/glEnd/glVertex2f
  不在导出表）。arm64 下无法编译链接 → **改用 GLES2**（视觉等价，实现自洽）。
- **minSdk 必须 19**：apktool.yml 默认沿用原版 minSdk 4，v1 SHA-256 签名在
  API 9-17 验证失败。改 19 后签名验证通过。
- **JNI 签名严格对齐**：OnCreate 返回 `Lcom/cnvcs/GLView;`（可返回 NULL 让 Java
  侧 new GLView，见 App.smali `if-nez v0` 分支）；OnTouch/OnKey/OnUpdate 返回
  jboolean。

## 边界说明

- **能装**：`phase_hand.apk` 签名验证通过、arm64 库 STORE、minSdk 19 → 64 位设备可安装。
- **能玩**：需**真实 arm64 设备**验证（redroid 无 GPU + 无 arm64 native bridge）。
  且本库为标准军棋规则，非原版行为；原版资源加载（.pak 解密）依赖 SPAK key。
- **R5 阻塞**：SPAK 解密 key 未提取（需从 32 位 `.text` 逆向定位解密循环），
  原版联机协议字段未知。`bt_spak.c` 为占位。
