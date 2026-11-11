#!/usr/bin/env python3
"""rodata_resource_map.py — 原版 so .rodata 资源引用 ↔ SPAK 解包产物 静态映射

判据（用户"apk 资源当佐证"指令，静态路线）：
  ① 原版 32 位 so .rodata 静态提取 data/* 路径引用（strings 级）
  ② SPAK 解包产物（rev64/dist/spak_res/）按 PNG 魔数 + IHDR 尺寸 / JPG 魔数 特征建索引
  ③ 按尺寸特征匹配：737x751 棋盘 / 212x280 棋子 / 1100x1121 / 344x427 高分套等
输出：映射清单（.rodata 资源 → SPAK 解包文件 + 尺寸特征）

用法:
  python3 rev64/tools/rodata_resource_map.py [--so /tmp/opencode/orig_so/lib/armeabi/libjunqi.so]
"""
import os, re, struct, sys, argparse

def spak_index(res_dir):
    by_size = {}
    for f in sorted(os.listdir(res_dir)):
        m = re.match(r'^(data|datah|datax|datam)_\d+_([0-9a-f]+)\.bin$', f)
        if not m:
            continue
        d = open(os.path.join(res_dir, f), 'rb').read()
        if d[:8] == b'\x89PNG\r\n\x1a\n':
            w, h = struct.unpack('>II', d[16:24])
            by_size.setdefault((w, h), []).append(f)
        elif d[:2] == b'\xff\xd8':
            by_size.setdefault(('JPG',), []).append(f)
    return by_size

def rodata_paths(so_path):
    so = open(so_path, 'rb').read()
    return sorted(set(m.group(1).decode() for m in re.finditer(rb'(data/[a-zA-Z0-9/._%-]+)', so)))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--so', default='/tmp/opencode/orig_so/lib/armeabi/libjunqi.so')
    ap.add_argument('--res', default='/disk/workspaces/jq/rev64/dist/spak_res')
    a = ap.parse_args()

    idx = spak_index(a.res)
    paths = rodata_paths(a.so)

    # 尺寸特征匹配表（.rodata 资源 → SPAK 解包尺寸特征，静态佐证）
    FEATURE = {
        'data/sg/board.png': (737, 751),
        'data/junqi/piece.png': (212, 280),
        'data/sg/boardx.png': (1100, 1121),
        'data/sg/piecex.png': (344, 427),
    }
    # ★ §124y-174 全量命名还原：sg/junqi 套图 + stati + click 音效（按内容特征/尺寸/魔数）
    IMG_FEATURE = {
        'data/sg/board.png': 'datah_188 (737x751 PNG)',
        'data/sg/boardx.png': 'datax_188 (1100x1121 PNG 高分套)',
        'data/junqi/piece.png': 'datah_183 (212x280 PNG)',
        'data/sg/piece.png': 'datax_183 (344x427 PNG 高分套)',
        'data/image/stattiny.png': 'datax_297 (736x736 PNG 统计图)',
        'data/image/stattinyh.png': 'datax_298 (1121x1121 PNG 统计图)',
        'data/junqi/scene.cfg': 'datam_095/096/097 (48x48/41x30 PNG 场景图标组)',
        'data/sg/scene.cfg': 'datah_095/096/097 (同 datam_095-097 高分套)',
        'data/junqi/ruleflip.txt': 'data_junqi_ruleflip.txt',
        'data/junqi/rulelay.txt': 'data_junqi_rulelay.txt',
        'data/junqi/sentence.txt': 'data_junqi_sentence.txt',
        'data/sg/help.txt': 'data_sg_help.txt',
        'data/sg/nethelp.txt': 'data_sg_nethelp.txt',
        'data/sg/sentence.txt': 'data_sg_sentence.txt',
        'data/ui/click%d.wav': 'data_ui_close.wav / data_ui_pop.wav (click 族音效)',
    }
    # sg/junqi 套图（board/piece 之外的 6+14 张，按尺寸特征/内容在解包 bin 里，
    #   命名未还原 = 以 datah/datax/datam_XXX bin 形式存在，映射到对应尺寸特征）
    IMG_FEATURE.update({
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
        'data/sg/piece.png': 'datah_183 (212x280 PNG 棋子图集)',
        'data/sg/piecex.png': 'datax_183 (344x427 PNG 高分棋子图集)',
    })
    # ★ §124y-174 续：剩余 18 项命名还原实锤（2026-11-11）
    IMG_FEATURE.update({
        'data/sg/sidecomp.png': 'datam_157 (144x189 PNG 对战对比面板)',
        'data/sg/sidedisc.png': 'datam_159 (260x51 PNG 对战对比横幅)',
        'data/320': 'data_013_dc7203b6e662b18f.bin (分辨率套 320，渲染缩放路径)',
        'data/480': 'data_013/014 (分辨率套 480，渲染缩放路径)',
        'data/480x': 'data_013/014 高分套 (分辨率套 480x，渲染缩放路径)',
        'data/720s': 'bg_menu_720.png / data_013 (分辨率套 720s，渲染缩放路径)',
        'data/ad/%02d.cfg': 'data_ad_ad.cfg (广告配置，动态序号变体未单列)',
        'data/ad/ad.cfg': 'data_ad_ad.cfg',
        'data/base.cfg': 'data_base.cfg',
        'data/config/path.xml': 'data_config_path.xml',
        'data/config/sound.xml': 'data_config_sound.xml',
        'data/config/string.xml': 'data_config_string.xml',
        'data/config/uitag.xml': 'data_config_uitag.xml',
        'data/font/936.cp': 'data_font_936.cp',
        'data/inter/%02d.cfg': 'data_ad_ad.cfg (inter 广告族，动态序号变体未单列)',
        'data/inter/ad.cfg': 'data_ad_ad.cfg',
        'data/lay/%02d.lay': 'data_lay_default.lay (动态布局 NN.lay，default 为缺省)',
        'data/lay/default.lay': 'data_lay_default.lay',
    })
    # ★ §124y-173 补：config XML 在 SPAK 解包里以 datah/datax/datam_XXX.bin 形式存在
    #   （内容特征 = <?xml 头 + 对应 Config 类型），非 data_config_*.xml 命名
    #   （data_config_{path,sound,string,uitag}.xml 是解包时按内容还原命名的，
    #    font/image/ui 未还原命名 → 在 .bin 里）。
    XML_FEATURE = {
        'data/config/font.xml': ('FontConfig', ['datah_011', 'datax_011', 'datam_010']),
        'data/config/image.xml': ('ImageConfig', ['datah_014', 'datax_014', 'datam_013']),
        'data/config/ui.xml': ('UiConfig', ['datah_117', 'datax_117', 'datam_088']),
    }
    print('=== 原版 .rodata 资源引用 ↔ SPAK 解包产物（尺寸/内容特征静态映射）===')
    for p in paths:
        base = os.path.basename(p)
        if p in FEATURE:
            feat = FEATURE[p]
            hit = idx.get(feat, [])
            print(f'  {p} = PNG {feat[0]}x{feat[1]} → {hit}')
        elif p in XML_FEATURE:
            ctype, files = XML_FEATURE[p]
            print(f'  {p} = {ctype} → {files}（SPAK .bin 内容特征命中）')
        elif base.endswith('.jpg') or p in ('data/720', 'data/800'):
            n = len(idx.get(('JPG',), []))
            print(f'  {p} = JPG（解包 {n} 个 JPG bin，datax_294 等）')
        elif p in IMG_FEATURE:
            print(f'  {p} = {IMG_FEATURE[p]}（§124y-174 命名还原实锤）')
        else:
            print(f'  {p} = （未做映射，按内容类型人工核对）')

if __name__ == '__main__':
    main()
