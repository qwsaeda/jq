# Rust 正向渲染资源来源（原版性实证）

> 2026-11-20。用户质疑 `piece_atlas.png` / `bg_menu.png` 是否真来自原版 APK。此文档用逐字节/逐像素实证回答，防"资源来路不明"假象。

## 实证方法
从**原版 `军棋.apk`**（7,555,494B）里提取 `assets/data*.pak`（9 个：data0/1、datah0/1、datam、datax0-3），用 `rev64/tools/spak_unpack.py` **现场重新解包**（不依赖工作区已解包的 `rev64/dist/spak_res/`），得到 883 个资源（datah 272 / datam 189 / datax 301 + data 等），再与 Rust assets 对比。

## 两张图的来源结论

| Rust asset | 原版来源 | 证据 | 是否原版 |
|---|---|---|---|
| `piece_atlas.png`（212×280 RGB, 59123B） | `datah_183_9e1b7fbc4dfc09c3.bin`（从原版 datah0/h1.pak 现场解出） | md5 双方均 `4cf215a18c9fa5d228473f34e3364b2d`，`cmp` 逐字节一致 | ✅ **确定原版棋子图集** |
| `board_bg.png`（720×1280 RGBA, 712239B）<br>（原名 `bg_menu.png`，2026-11-20 改名） | 像素 = `datax_177_a94374ff3a9f01c9.bin`（原版 datax pak 解出的对局棋盘背景 JPEG） | PIL 逐像素 `ImageChops.difference` bbox=None（完全相同）；712239B 是同像素的 PNG 无损版（datax_177 是 191210B JPEG） | ✅ **像素级原版**（字节是同图的无损 PNG 重编码，非 pak 原始字节） |

## 诚实边界
- `board_bg.png` **不是** pak 里直接解出的原始字节——它是 datax_177（JPEG）的**无损 PNG 重编码**（712239B vs 191210B），像素逐点相同。原版 pak 的 883 个资源里没有 712239B 的这个文件，它是工作区早期为"PNG 无损渲染"生成的副本（`rev64/dist/spak_res/bg_menu_720.png`，在 `dist/` 被 .gitignore 排除故无 git 记录）。
- 文件名 `bg_menu` 是**历史误导**（实为对局棋盘底图，非菜单背景），2026-11-20 改名为 `board_bg.png` 并同步 `asset.rs` 的 `include_bytes!` 引用——纯命名澄清，行为不变。
- 菜单背景原版是 `datax_294`（800×1205 士兵图），由 C 正向 GL 渲染路径使用；Rust canvas 路径当前用 board_bg 作底图 + 棋子图集，这是独立的渲染实现（行为对齐边界见 SPEC_HUMAN_PLAY）。

## 可复现验证命令
```bash
# 从原版 APK 现场解包（不依赖工作区 spak_res）
python3 rev64/tools/spak_unpack.py <从军棋.apk解出的assets> /tmp/spak_verify/out
# 逐字节：piece_atlas
cmp rev64/forward/src_rs_rust/assets/piece_atlas.png \
    /tmp/spak_verify/out/datah_183_9e1b7fbc4dfc09c3.bin   # 无差异
# 逐像素：board_bg vs datax_177
python3 -c "from PIL import Image,ImageChops; a=Image.open('rev64/forward/src_rs_rust/assets/board_bg.png').convert('RGBA'); b=Image.open('/tmp/spak_verify/out/datax_177_a94374ff3a9f01c9.bin').convert('RGBA'); print(ImageChops.difference(a,b).getbbox())"  # None
```
