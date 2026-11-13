#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""烘焙原版按钮皮肤 btn.png 三态 → C 数组头文件。

★ 2026-11-13 逆向实锤（此前"btn.png 不可得"结论错误）：
  - datah_205 = btn.cfg：btn.png,8,29,16,30（9-patch 边 left8 top29 right16 bottom30）
  - datah_207/208/209 = btn.png 三态（24×64 RGBA）：
      207 正常态（中心 RGB 33,130,202 蓝渐变） / 208 按下（42,138,210） / 209 禁用（170,170,170 灰）
  - datah_206 = btn1.cfg（8,31,16,32），datah_210/211/212 = btn1.png 三态（24×60 同色系）
  - datax_207/208/209 = 高分 btn.png 三态（24×95）；datax_205 = 高分 btn.cfg（8,44,16,45）

产物：rev64/forward/src_rs/jq_btn_skin.h（三态 RGBA 数组 + 9-patch 参数）
用法：python3 rev64/tools/bake_btn_skin.py
幂等：重复运行覆盖。
"""
import os, struct, zlib

BASE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPAK = "/tmp/opencode/spak_full"
DST = os.path.join(BASE, "rev64/forward/src_rs/jq_btn_skin.h")

# 低分 btn.png 三态（datah_207/208/209，24×64，btn.cfg 9patch 8/29/16/30）
BTN_STATES = [
    ("btn_normal", "datah_207_ec646c1fcb54c387.bin", 24, 64),
    ("btn_press",  "datah_208_8c0c41cbf1c1095b.bin", 24, 64),
    ("btn_disable","datah_209_42d6257826ab79b4.bin", 24, 64),
]


def decode_png(path):
    d = open(path, "rb").read()
    if d[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"非 PNG: {path}")
    pos = 8; w = h = ct = bd = None; idat = b""
    while pos < len(d):
        length = struct.unpack(">I", d[pos:pos+4])[0]
        ctype = d[pos+4:pos+8].decode()
        data = d[pos+8:pos+8+length]
        if ctype == "IHDR":
            w, h, bd, ct, cm, fm, im = struct.unpack(">IIBBBBB", data)
        elif ctype == "IDAT":
            idat += data
        elif ctype == "IEND":
            break
        pos += 12 + length
    raw = zlib.decompress(idat)
    nch = 4 if ct == 6 else 3
    bpp = nch * (bd // 8)
    stride = w * bpp
    out = bytearray(); prev = bytearray(stride)
    for y in range(h):
        ft = raw[y*(stride+1)]
        row = bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
        if ft == 1:
            for i in range(bpp, stride): row[i] = (row[i]+row[i-bpp]) & 0xff
        elif ft == 2:
            for i in range(stride): row[i] = (row[i]+prev[i]) & 0xff
        elif ft == 3:
            for i in range(stride):
                a = row[i-bpp] if i >= bpp else 0
                row[i] = (row[i]+((a+prev[i])>>1)) & 0xff
        elif ft == 4:
            for i in range(stride):
                a = row[i-bpp] if i >= bpp else 0
                b = prev[i]; c = prev[i-bpp] if i >= bpp else 0
                pa = abs(b-c); pb = abs(a-c); pc = abs(a+b-2*c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                row[i] = (row[i]+pr) & 0xff
        out += row; prev = row
    # 转 RGBA（若 RGB 补 alpha=255）
    if nch == 3:
        rgba = bytearray()
        for i in range(0, len(out), 3):
            rgba += out[i:i+3] + b"\xff"
        return w, h, bytes(rgba)
    return w, h, bytes(out)


def main():
    lines = []
    lines.append("/* 原版按钮皮肤 btn.png 三态（★ 2026-11-13 逆向实锤）")
    lines.append("   btn.cfg=btn.png,8,29,16,30（9-patch left8 top29 right16 bottom30）")
    lines.append("   正常态=btn_normal（蓝渐变 RGB33,130,202）按下=btn_press（42,138,210）禁用=btn_disable（170,170,170 灰）")
    lines.append("   源：SPAK datah_205(btn.cfg)+datah_207/208/209(btn.png 三态 24×64 RGBA)。")
    lines.append("   ★ 纠正：此前误用 datax_195(244×61) 当按钮皮肤，实为另一资源。 */")
    lines.append("#ifndef JQ_BTN_SKIN_H")
    lines.append("#define JQ_BTN_SKIN_H")
    lines.append("#include <stddef.h>")
    lines.append("")
    for name, srcf, w, h in BTN_STATES:
        wp, hp, rgba = decode_png(os.path.join(SPAK, srcf))
        assert (wp, hp) == (w, h), f"{srcf}: {wp}x{hp} != {w}x{h}"
        lines.append(f"/* {name}：{w}x{h} RGBA，9-patch(8,29,16,30) */")
        lines.append(f"static const unsigned char jq_{name}_px[] = {{")
        # 每行 16 字节
        for i in range(0, len(rgba), 16):
            chunk = rgba[i:i+16]
            lines.append("    " + ",".join(f"0x{b:02x}" for b in chunk) + ",")
        lines.append("};")
        lines.append(f"static const int jq_{name}_w = {w};")
        lines.append(f"static const int jq_{name}_h = {h};")
        lines.append("")
    lines.append("/* 9-patch 边（btn.cfg：left8 top29 right16 bottom30） */")
    lines.append("#define JQ_BTN_9PATCH_L 8")
    lines.append("#define JQ_BTN_9PATCH_T 29")
    lines.append("#define JQ_BTN_9PATCH_R 16")
    lines.append("#define JQ_BTN_9PATCH_B 30")
    lines.append("")
    lines.append("#endif /* JQ_BTN_SKIN_H */")
    with open(DST, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"OK {DST} ({sum(1 for n,_,_,_ in BTN_STATES)} 态)")
    for name, srcf, w, h in BTN_STATES:
        _, _, rgba = decode_png(os.path.join(SPAK, srcf))
        print(f"  {name}: {w}x{h} RGBA {len(rgba)}B")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
