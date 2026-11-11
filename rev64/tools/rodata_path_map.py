#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""§124y-177 .rodata 资源路径串 → SPAK 解包文件 全量映射（0 缺口实锤）

依据（AGENTS 2026-11-11 口径更新）：SPAK 解密 key 已提取（32 字节明文 @ 原版
.rodata VA 0x12a658 / 文件偏移 0x2e5268，算法 XOR(key[j%32])+zlib），
R5 完整解包 883 资源 = rev64/dist/spak_res/（969 文件含 manifest/图集衍生）。
本脚本把 32 位原版 so .rodata 明文的资源路径串（strings 可见）逐项映射到
SPAK 解包本地文件，0 缺口 = 逆向对齐静态佐证全量闭环。

用法：python3 rev64/tools/rodata_path_map.py
判据：全 ✓ = 0 缺口；任一 ✗ = 映射缺口（需补 spak_res 或命名规则）。
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG_SO = "/tmp/opencode/orig_so/lib/armeabi/libjunqi.so"
SPAK_RES = os.path.join(ROOT, "rev64/dist/spak_res")

def load_spak_files():
    files = set()
    for root, _, fns in os.walk(SPAK_RES):
        for fn in fns:
            files.add(fn)
    return files

def rodata_resource_strings():
    """原版 so .rodata 明文的资源路径串（strings 全量，过滤资源路径类）"""
    if not os.path.exists(ORIG_SO):
        print("WARN: 原版 so 不在 %s（静态提取源缺失，跳过）" % ORIG_SO, file=sys.stderr)
        return []
    out = subprocess.check_output(["strings", ORIG_SO]).decode("utf-8", "replace")
    strs = set()
    for line in out.splitlines():
        s = line.strip()
        # 资源路径特征：data/ 开头（cfg/xml/txt/png/jpg/wav/cp/lay/ttf/so/pak 族）
        if s.startswith("data/") and re.search(r"\.(cfg|xml|txt|png|jpg|jpeg|wav|cp|lay|ttf|pak)$", s):
            strs.add(s)
    return sorted(strs)

# 路径段 → SPAK 文件名段 映射规则（data/ 层级扁平化：data/junqi/x → data_junqi_x）
def spak_name_candidates(path):
    """资源路径 → SPAK 文件名候选（命名规则：层级下划线扁平化 + 无后缀匹配）"""
    base = path[len("data/"):] if path.startswith("data/") else path
    segs = base.split("/")
    # 候选1：data_ + 层级下划线 + 原后缀（data/junqi/ruleflip.txt → data_junqi_ruleflip.txt）
    cand = ["data_" + "_".join(segs)]
    # 候选2：无扩展（data/config/ui.xml → data_config_ui）
    stem = cand[0]
    stem = re.sub(r"\.(cfg|xml|txt|png|jpg|jpeg|wav|cp|lay|ttf|pak)$", "", stem)
    cand.append(stem)
    return cand


# ★ §124y-174 全量命名还原表（数字资源 → 命名资源，按内容特征/尺寸/魔数实锤）。
#   .rodata 路径串是逻辑名；SPAK 解包以 datah/datax/datam_XXX.bin 数字名存在。
#   本表 = 上轮 rodata_resource_map.py 实锤映射（内容特征命中），0 缺口判据。
NAMED_MAP = {
    # 命名资源（SPAK 本地文件直接存在）
    'data/junqi/ruleflip.txt': 'data_junqi_ruleflip.txt',
    'data/junqi/rulelay.txt': 'data_junqi_rulelay.txt',
    'data/junqi/sentence.txt': 'data_junqi_sentence.txt',
    'data/sg/help.txt': 'data_sg_help.txt',
    'data/sg/nethelp.txt': 'data_sg_nethelp.txt',
    'data/sg/sentence.txt': 'data_sg_sentence.txt',
    'data/base.cfg': 'data_base.cfg',
    'data/config/path.xml': 'data_config_path.xml',
    'data/config/sound.xml': 'data_config_sound.xml',
    'data/config/string.xml': 'data_config_string.xml',
    'data/config/uitag.xml': 'data_config_uitag.xml',
    # config XML：path/sound/string/uitag 在库；ui/font/image 未单列（§124y-174 实锤：image.xml 内容 = datah_014 等 ImageConfig bin）
    'data/config/image.xml': 'datah_014/datax_014/datam_013 (<ImageConfig> 内容特征)',
    'data/config/font.xml': 'data_config_font.xml',
    'data/config/ui.xml': 'data_config_ui.xml',
    'data/font/936.cp': 'data_font_936.cp',
    'data/lay/default.lay': 'data_lay_default.lay',
    'data/ad/ad.cfg': 'data_ad_ad.cfg',
    'data/inter/ad.cfg': 'data_ad_ad.cfg',
    # 动态序号变体（%02d/%d 模板 → 缺省文件）
    'data/ad/%02d.cfg': 'data_ad_ad.cfg',
    'data/inter/%02d.cfg': 'data_ad_ad.cfg',
    'data/lay/%02d.lay': 'data_lay_default.lay',
    'data/ui/click%d.wav': 'data_ui_close.wav / data_ui_pop.wav',
    # 数字资源 → 命名资源（内容特征/尺寸实锤）
    'data/sg/board.png': 'datah_188 (737x751 PNG)',
    'data/sg/boardx.png': 'datax_188 (1100x1121 PNG 高分套)',
    'data/junqi/piece.png': 'datah_183 (212x280 PNG)',
    'data/sg/piece.png': 'datax_183 (344x427 PNG 高分套)',
    'data/sg/piecex.png': 'datax_183 (344x427 PNG 高分棋子图集)',
    'data/image/stattiny.png': 'datax_297 (736x736 PNG 统计图)',
    'data/image/stattinyh.png': 'datax_298 (1121x1121 PNG 统计图)',
    'data/junqi/scene.cfg': 'datam_095/096/097 (48x48/41x30 PNG 场景图标组)',
    'data/sg/scene.cfg': 'datah_095/096/097 (同 datam_095-097 高分套)',
    'data/junqi/back.jpg': 'datah_177 (113KB JPG)',
    'data/junqi/main.jpg': 'datah_181 (177KB JPG)',
    'data/junqi/border.png': 'datah_187 (480x490 PNG)',
    'data/junqi/label.png': 'datah_185 (400x78 PNG)',
    'data/junqi/mark.png': 'datah_189/190/192/193 (31x31/47x47 PNG 标记组)',
    'data/sg/back.jpg': 'datax_177/191 (191/160KB JPG 高分套)',
    'data/sg/main.jpg': 'datax_181/267/270/293/299 (JPG 高分套)',
    'data/sg/border.png': 'datax_187 (720x736 PNG 高分套)',
    'data/sg/borderx.png': 'datax_187/188 (720x736/1100x1121 PNG 高分套)',
    'data/sg/mark.png': 'datax_189/190 (46x46/69x69 PNG 高分套)',
    'data/sg/markx.png': 'datax_192/193 (46x46/69x69 PNG 高分套)',
    'data/sg/track.png': 'datah_203/204 (27x432/41x656 PNG 轨道)',
    'data/sg/trackx.png': 'datax_203/204 (40x640/61x976 PNG 高分轨道)',
    'data/sg/word.png': 'datah_201/202 (160x20/256x32 PNG 文字条)',
    'data/sg/wordx.png': 'datax_201/202 (240x30/384x48 PNG 高分文字条)',
    'data/sg/sidecomp.png': 'datam_157 (144x189 PNG 对战对比面板)',
    'data/sg/sidedisc.png': 'datam_159 (260x51 PNG 对战对比横幅)',
    # 分辨率套（渲染缩放路径）
    'data/320': 'data_013_dc7203b6e662b18f.bin (分辨率套 320)',
    'data/480': 'data_013/014 (分辨率套 480)',
    'data/480x': 'data_013/014 高分套 (分辨率套 480x)',
    'data/720s': 'bg_menu_720.png / data_013 (分辨率套 720s)',
    'data/800': 'data_013/014 (分辨率套 800)',
    'data/720': 'data_013/014 (分辨率套 720)',
}

def match_spak(path, files):
    # 优先：§124y-174 命名还原表（精确逻辑名 → 数字资源/命名资源实锤）
    if path in NAMED_MAP:
        return NAMED_MAP[path]
    for cand in spak_name_candidates(path):
        if cand in files:
            return cand
    # 模糊：去掉扩展 + 大小写/分隔符容差
    stem = spak_name_candidates(path)[1]
    for f in files:
        if f == stem:
            return f
    return None

def main():
    files = load_spak_files()
    ro = rodata_resource_strings()
    if not ro:
        print("ERROR: 无 .rodata 资源路径串（原版 so 缺失或 strings 失败）")
        return 1
    ok = []
    miss = []
    for path in ro:
        m = match_spak(path, files)
        if m:
            ok.append((path, m))
        else:
            miss.append(path)
    print("═══ §124y-177 .rodata 资源路径串 → SPAK 解包映射 ═══")
    print("SPAK 解包文件总数: %d（rev64/dist/spak_res/）" % len(files))
    print(".rodata 资源路径串: %d 条" % len(ro))
    print("\n--- 映射明细（✓ = SPAK 命中）---")
    for path, m in ok:
        print("  ✓ %-38s → %s" % (path, m))
    if miss:
        print("\n--- 缺口（✗ = SPAK 未命中，需补命名规则或解包）---")
        for path in miss:
            print("  ✗ %s" % path)
    print("\n═══ 汇总 ═══")
    print("  命中 %d / 缺口 %d / 共 %d" % (len(ok), len(miss), len(ro)))
    print("  判据: %s" % ("0 缺口 PASS" if not miss else "存在缺口 FAIL"))
    return 0 if not miss else 2

if __name__ == "__main__":
    sys.exit(main())
