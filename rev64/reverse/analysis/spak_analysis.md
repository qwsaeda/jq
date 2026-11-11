# R5 · 军棋 SPAK 资源包完整解密（已完成）

> 原版资源打包 `assets/data*.pak` 用 SPAK 加密。R5 目标：提取 key、定位解密
> 算法、实现解包。**R5 已完全完成**：883 个资源全部解包，算法 + key + 路径
> 映射全部验证。

## 核心结论（全链路验证）

### 1. SPAK 加密算法 = XOR(key[j%32]) + zlib 压缩
- **解密循环** `cc630`（原版 .text）：`data[j] ^= key[j % 32]`（j 从 0 起，key 索引循环 0..31）。
- **key（32 字节，明文态）**：`c2cbe5e7ce5c4ac411ca6c9906f476f86d4fcbb62621de0e6a67874c93f0d523`
  - 位于原版 `.rodata` VA `0x12a658`（redroid 解密 dump 与原始 .so 字节一致，确认该区未被二次加密）。
  - 紧邻 "No error"/"Failed to open file" 字符串。
- **数据 = zlib(XOR(密文))**，解压后才是明文资源。已验证：文件表 + 数据资源均此流程。

### 2. SPAK 头结构（24 字节）
```
0x00: "SPAK" (magic)
0x04: version = 1
0x08: f1 = 文件表偏移（合并流内）
0x0c: f2 = 文件表加密+压缩长度
0x10: f3 = 文件表校验（c3238 计算，未用）
0x14: f4 = 文件表解压后大小 = 条目数 × 28
```
**关键**：`f1 + f2 = 合并流大小`（已验证 data/datah/datam/datax 全部精确吻合），
即**文件表在合并流末尾 f2 字节**。

### 3. 合并流组成（.pak 按组拼接）
| 组 | 成员 | 合并大小 | f1 | f2 | 条目数 |
|----|------|---------|-----|-----|--------|
| data  | data0.pak + data1.pak | 1,236,875 | 0x12d4f3 | 2712 | 121 |
| datah | datah0.pak + datah1.pak | 1,406,813 | 0x155db2 | 6571 | 272 |
| datam | datam.pak | 456,917 | 0x6e6f9 | 4572 | 189 |
| datax | datax0..3.pak | 3,103,732 | 0x2f3f86 | 7278 | 301 |

### 4. 文件表条目（28 字节）= hash[16] + off[4] + size[4] + usize[4]
- `hash`：16 字节，**= MD5(资源路径)**（29/29 精确验证）。
- `off`：数据区偏移（数据区 = 合并流[24:f1]），`offset[i] = offset[i-1] + size[i-1]`（全链验证）。
- `size`：加密+压缩大小；`usize`：解压后大小。
- 数据资源 = 数据区[off-24:off-24+size]，XOR 解密 + zlib 解压。

### 5. 资源内容（883 个全部解包）
- **408 XML**（GBK 界面/布局/配置：`<Window>`、`<Path>`、`<StringConfig>` 等）
- **310 PNG**（UI 图标 72×72/62×45/48×48、棋盘素材）
- **22 JPG**、**18 WAV**（音效）、**3 TTF**（字体）、**64 text**、**58 其他**

### 6. 关键资源映射（hash = MD5(路径)，已验证）
- **字体**：`data/font/all.ttf`→data#29、`mb.ttf`→#30、`sb.ttf`→#31、`936.cp`→#28
- **音效**（与 .rodata 明文 `Junqi.*` 事件名对应）：
  `data/junqi/{move,flip,flag,bomb,eat,dead,select,start,win,lose,draw,over}.wav`→data#32-46
- **配置**：`data/config/{path,sound,string,uitag}.xml`→data#24-27、`base.cfg`→#21、`ad/ad.cfg`→#20
- **规则文本**：`data/junqi/{rulelay,ruleflip,sentence}.txt`→data#41/42/44
- **布局**：`data/lay/default.lay`→data#97（30 字节布阵）
- **说明**：`data/sg/{help,nethelp,sentence}.txt`→data#98/99/100
- **服务器**：`data/base.cfg`→#21 含 `siteUrl http://www.cnvcs.com`、`updateUrl`、`serverHost 42.121.132.86`、`serverPort 8500`、DER 证书

## 逆向证据链

1. **SPAK 魔数引用**：`0xc6d62: ldr r3,[pc,#536] @0xc6f7c` = 0x4b415053，函数 `0xc6d34-0xc6f8c`。
2. **解析函数语义**（反汇编）：
   - `c6d58: c6150(fd,0,24,buf)` 读 24 字节头；`c6d64: cmp header[0],"SPAK"`。
   - `c6d84: header[0x14] 除 28`（校验条目数整除）；`c6d90: _Znaj(header[0x0c])` 分配文件表缓冲。
   - `c6d98: c6150(fd, header[0x08], header[0x0c], buf)` 读文件表（f1 偏移, f2 长度）。
   - `c6dbc: c3238(buf, f2)` 校验（与 header[0x10]=f3 比较）。
   - `c6dd2: cc630(buf, f2)` **XOR 解密**。
   - `c6dec: c3270(buf, f2, out, f4)` **zlib 解压**（2712→3388 等，= f4）。
   - `c6e40: b82a4(entry,16,dst,64)` hex 编码 hash[16]→32 字符（`id  <32hex>` 形式）。
3. **key 定位**：`cc636: ldr r6,[pc,#36]; cc63c: add r6,pc` → `0xcc640+0x5e018 = 0x12a658`。
4. **MD5 确认**：文件表 hash 16 字节 = MD5(路径)，29 资源全匹配；原版 MD5 实现在 `0xc5130`（`.text`，MD5 常数池 + `c4b84` 轮函数）。

## 产物

- **工具**：`tools/spak_unpack.py`（完整 SPAK 解包器，可复用）。
- **解包资源**：`dist/spak_res/`（883 个 `.bin` + `manifest.tsv`，含按路径命名的副本）。
- **C 侧加载器**：`src/spak_load.c`（arm64 原生：SPAK 解包 + MD5(路径) 查找，已编译链接）。
- **分析记录**：本文件。

## 验证记录

- **全部 4 组** `f1+f2 == 合并流大小` ✓；条目数 121/272/189/301 ✓；offset 链全一致 ✓。
- **XOR+zlib 解压** 文件表与数据资源全部成功 ✓。
- **MD5(路径)==hash** 29/29 ✓（Python 与 C 侧 RFC1321 实现结果一致）。
- **内容签名**：PNG（`\x89PNG`）、JPG（`\xff\xd8`）、WAV（`RIFF`）、TTF（`\x00\x01\x00\x00`+cmap）、XML（`<?xml`）、GBK 文本、30 字节布阵表。

## 对后续的意义

- **R5 资源层解锁**：原版 UI 布局、音效、字体、图片、规则文本、布局方案、服务器配置全部可得。
- **C 侧 `spak_load.c`** 提供 `spak_load(dir)` / `spak_get(path)`，arm64 库可加载原版资源。
- **"能玩"路径**：渲染层（render_gles2）可替换色块为真实 PNG（datah/datam/datax 图片），
  音效可用 WAV，字体可用 TTF，布局可用 `default.lay`。
- **诚实边界**：资源解包已完成，但**界面渲染逻辑**（XML→控件）、**AI 策略**、**蓝牙联机协议**
  仍需逐函数逆向（原版 .text 7480 函数，月级工程），非当前手写骨架可替代。
