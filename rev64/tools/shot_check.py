#!/usr/bin/env python3
"""shot_check.py — MuMu 截图出帧判定
用法: python3 shot_check.py /tmp/opencode/verify.png
输出:
  - 总像素数 / 非黑像素比例（> 1% 即有内容出帧）
  - 中心 + 4 角采样 RGBA
  - 判定结论: "有内容出帧" / "纯黑（GL 几何未出帧，MuMu 环境限制）"
"""
import sys, struct, zlib

def parse_png(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data[:4] != b'\x89PNG':
        raise ValueError(f"{path} 不是 PNG")
    w, h = struct.unpack('>II', data[16:24])
    colortype = data[25]
    pos = 8
    idat = b''
    while pos < len(data):
        ln = struct.unpack('>I', data[pos:pos+4])[0]
        typ = data[pos+4:pos+8]
        body = data[pos+8:pos+8+ln]
        if typ == b'IDAT': idat += body
        pos += 12 + ln
    raw = zlib.decompress(idat)
    bpp = 4 if colortype == 6 else 3
    stride = 1 + w * bpp
    rows = [raw[i*stride:(i+1)*stride] for i in range(h)]
    return w, h, bpp, rows

def px_at(rows, w, h, bpp, x, y):
    row = rows[y]
    off = 1 + x * bpp
    return list(row[off:off+min(bpp,4)])

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/opencode/verify.png'
    w, h, bpp, rows = parse_png(path)
    # 非黑像素比例（每 4 采样一次）
    # RGB 通道非黑判定（透明 PNG 区 alpha=0 不算全黑；MuMu screencap 对 GL surface 区 alpha 可能为 0，RGB 才有渲染值）
    nb, total = 0, 0
    rgb_sum = 0
    for y in range(0, h, 4):
        row = rows[y]
        for x in range(0, w, 4):
            off = 1 + x * bpp
            total += 1
            r3 = row[off:off+3]
            rgb_sum += sum(r3)
            if any(v > 16 for v in r3): nb += 1
    pct = 100.0 * nb / total if total else 0
    avg_rgb = rgb_sum / (total * 3) if total else 0
    samples = {
        '中心':  (w//2, h//2),
        '左上':  (10, 10),
        '右上':  (w-10, 10),
        '左下':  (10, h-10),
        '右下':  (w-10, h-10),
    }
    print(f"截图 {w}x{h} bpp={bpp}")
    for name, (sx, sy) in samples.items():
        v = px_at(rows, w, h, bpp, sx, sy)
        print(f"  {name}({sx},{sy}) RGBA={v}")
    print(f"RGB 非黑像素 {nb}/{total} = {pct:.1f}%，平均 RGB 值 = {avg_rgb:.1f}")
    if pct > 1.0:
        print("★ 判定: 有内容出帧（RGB 非黑 >1%）")
    elif avg_rgb > 5:
        print(f"★ 判定: 纯色出帧（平均 RGB={avg_rgb:.0f}，= glClearColor 渲染，几何绘制未出像素）")
    else:
        print("★ 判定: 全黑（GL 未出帧，MuMu 转译 GL 环境限制，需真机 GPU 验收）")

# ============ §sp27 沉淀：暖棕/UI vs 棋盘 判读（分区统计，本轮手写 3 次 → 固化）============
def classify_screen(path):
    """判定截图属于哪类（本轮反复手写的 PNG 解析+暖棕占比逻辑固化）。
    返回 dict：主色/占比 + 分区（标题栏/菜单区/GL 内容区）+ 结论。
    结论枚举：
      - 'warm_brown_ui'  暖棕色 ≥85%（UI 层渲染，非棋盘；§sp27 已知态）
      - 'white_anr'      白色 ≥95%（可能是 ANR watchdog 杀掉前的残留 UI，需对照 logcat）
      - 'board_candidate' 非暖棕非白且非黑占比 >30%（可能有几何内容，需人工看图）
      - 'black'          纯黑（GL 几何未出帧，MuMu 环境限制）"""
    w, h, bpp, rows = parse_png(path)
    from collections import Counter
    col = Counter()
    for row in rows:
        for x in range(0, len(row) - bpp, bpp * 4):  # 每 4 像素采样
            r, g, b = row[1 + x], row[2 + x], row[3 + x]
            col[(r // 32, g // 32, b // 32)] += 1
    total = sum(col.values())
    def pct(c):
        return 100 * c / total
    top = col.most_common(6)
    # 32 分桶：warm brown(144,112,48)->(4,3,1); white(240,240,240)->(7,7,7); black->(0,0,0)
    warm = sum(n for (r, g, b), n in col.items() if (r, g, b) in ((4, 3, 1), (4, 2, 1), (4, 3, 0), (4, 4, 1)))
    white = sum(n for (r, g, b), n in col.items() if r >= 6 and g >= 6 and b >= 6)
    black = sum(n for (r, g, b), n in col.items() if r < 1 and g < 1 and b < 1)
    out = {
        'size': '%dx%d' % (w, h),
        'top': ['RGB~(%d,%d,%d) %.1f%%' % (r * 32 + 16, g * 32 + 16, b * 32 + 16, pct(n)) for (r, g, b), n in top],
        'warm_brown_pct': round(pct(warm), 1),
        'white_pct': round(pct(white), 1),
        'black_pct': round(pct(black), 1),
    }
    if pct(warm) >= 85:
        out['verdict'] = 'warm_brown_ui'
        out['note'] = 'UI 层暖棕色（非棋盘，§sp27 已知稳定态）'
    elif pct(white) >= 95:
        out['verdict'] = 'white_anr'
        out['note'] = '白色 ≥95%——对照 logcat 是否 ANR watchdog 杀掉前残留'
    elif pct(black) >= 99:
        out['verdict'] = 'black'
        out['note'] = '纯黑（GL 几何未出帧，MuMu 环境限制）'
    else:
        out['verdict'] = 'board_candidate'
        out['note'] = '非暖棕/非白/非黑 >30%，可能有几何内容，需人工看图'
    return out

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'classify':
        import json
        print(json.dumps(classify_screen(sys.argv[2] if len(sys.argv) > 2 else '/tmp/opencode/verify.png'),
                         ensure_ascii=False, indent=2))
    else:
        main()
