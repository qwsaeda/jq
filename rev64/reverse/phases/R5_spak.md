# 阶段 R5 · 联机 / 蓝牙 / SPAK 层

## 目标
- **SPAK 资源包解密**：提取 key、定位解密算法、实现完整解包（**已完成**）。
- **蓝牙联机协议**：逆向 `OnBluetooth`/`OnEvent` 协议字段（**未完成**，需逐函数逆向）。
- **服务器通信**：`OnEvent` 事件（`Junqi.*`）与 `base.cfg`（`serverHost 42.121.132.86:8500`）。

## 关键决策
- 原版资源用 **SPAK** 加密（非普通 ZIP/PNG 直读）。经逆向确认算法为 **XOR(key[j%32]) + zlib**。
- key 在 `.rodata` VA `0x12a658`（明文态，原始 .so 与 redroid dump 一致）。
- 文件表条目 hash = **MD5(资源路径)**，native 用路径（含 `data/{分辨率}/` 拼接）查资源。

## 操作步骤与产物

### 1. 定位 SPAK 解析函数
```bash
# SPAK 魔数 0x4b415053 引用：ldr r3,[pc,#536] @0xc6f7c，函数 0xc6d34-0xc6f8c
arm-linux-gnueabi-objdump -D -b elf32-littlearm -m arm -M force-thumb \
  --start-address=0xc6d34 --stop-address=0xc6f8c orig32/lib/armeabi/libjunqi.so
```

### 2. 提取 key（解密循环 cc630）
```bash
# cc636: ldr r6,[pc,#36] @0xcc65c=0x0005e018; cc63c: add r6,pc → 0xcc640+0x5e018=0x12a658
# key = .so[0x12a658:0x12a658+32]（明文态，与 rodata_dec.bin[0x9a60] 一致）
python3 -c "print(open('orig32/lib/armeabi/libjunqi.so','rb').read()[0x12a658:0x12a658+32].hex())"
# → c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523
```

### 3. 解包（工具 `tools/spak_unpack.py`）
```bash
python3 tools/spak_unpack.py <assets_dir> <out_dir>
# → 883 资源（data 121 / datah 272 / datam 189 / datax 301）
```

## SPAK 格式规范（可复用）

### 头（24 字节）
```
0x00 "SPAK" | 0x04 version=1 | 0x08 f1 文件表偏移 | 0x0c f2 加密长 |
0x10 f3 校验 | 0x14 f4 = 条目数×28
```
**判据**：`f1+f2 == 合并流大小`。合并流：data=data0+data1、datah=datah0+datah1、
datam=datam、datax=datax0..3。

### 条目（28 字节）
```
hash[16] | off[4] | size[4] | usize[4]
```
- `hash == MD5(资源路径)`（29/29 验证）。
- `off[i] = off[i-1] + size[i-1]`（首项 24），数据区 = 合并流[24:f1]。
- 数据资源 = 数据区[off-24:...]，**XOR(key[j%32]) + zlib 解压**。

### key（32 字节，明文态）
```
c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523
```

## 关键资源映射（hash = MD5(路径)）

| 路径 | 资源 | 内容 |
|------|------|------|
| data/font/{all,mb,sb}.ttf | data#29/30/31 | 字体 |
| data/junqi/{move,flip,flag,bomb,eat,dead,select,start,win,lose,draw,over}.wav | data#32-46 | 音效（对应 `Junqi.*` 事件） |
| data/config/{path,sound,string,uitag}.xml | data#24-27 | 界面/路径/字符串/标签配置 |
| data/base.cfg | data#21 | `serverHost 42.121.132.86:8500`、DER 证书 |
| data/lay/default.lay | data#97 | 30 字节布阵 |
| data/junqi/{rulelay,ruleflip,sentence}.txt | data#41/42/44 | 规则文本 |
| data/sg/{help,nethelp,sentence}.txt | data#98/99/100 | 说明文本 |
| 棋盘素材 | datah#188(737×751)、datax#187/297(720×736/736×736) | 棋盘 PNG |

## 验证记录
- 4 组 `f1+f2==合并流` ✓；条目数 121/272/189/301 ✓；offset 链一致 ✓。
- XOR+zlib 解压成功（文件表与数据资源）✓；MD5(路径)==hash 29/29 ✓（Python 与 C RFC1321 一致）。
- 内容签名：PNG/JPG/WAV/TTF/XML/GBK 全部识别。

## 复用指引（后续阶段）
- **资源加载**：`src/spak_load.c` 的 `spak_load(assets_dir)` / `spak_load_assetmanager(mgr)` / `spak_get(path)` / `spak_get_by_hash(hash)`。
- **音效/贴图/字体/布局**：解包资源在 `dist/spak_res/`，按 `MD5(路径)` 定位；运行时拼接路径的用 hash。
- **联机**：`base.cfg` 的 `serverHost/serverPort`、`.rodata` 的 `Junqi.Peer*`/`Regret*`/`Draw*`
  事件名为协议字段线索（尚未逐函数逆向，`bt_spak.c` 占位）。

## AssetManager 部署（R5→R4 闭环）
- Android assets 无文件路径 → `spak_load_assetmanager(Activity.getAssets())` 读 `.pak`。
- 棋盘 PNG 路径运行时拼接（`data/{分辨率}/` 前缀）→ `spak_get_by_hash` 按 hash 加载。
- OnGraphCreate 时 `png_decode` 棋盘 PNG → `junqi_render_set_board_texture`（真实棋盘渲染）。

## 音效接入（R5→R3 闭环）
- WAV 音效已解包：`data/junqi/{move,flip,flag,bomb,eat,dead,win,lose,draw,start,over,select}.wav`。
- `junqi_audio_play(event)`：OpenSL ES 播放（`data/junqi/<event>.wav`），8/16bit PCM。
- 触发：走子→move、吃子→eat、翻棋→flip、选中→select、胜负→win/lose、悔棋→move、重开→start。
- minSdk 19 用 OpenSL ES（AAudio 需 API 26+），`-lOpenSLES`。

## 未完成（诚实边界）
- **蓝牙联机协议**：`OnBluetooth(Intent)` 字段、`Junqi.PeerLayFinish/PeerStart/RegretReq/DrawReq`
  等事件参数未逆向（需从 `.text` 逐函数，月级）。
- **服务器通信**：`OnEvent` 网络事件格式未还原。
