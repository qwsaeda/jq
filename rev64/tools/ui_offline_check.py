#!/usr/bin/env python3
"""ui_offline_check.py — Rust UI 离线渲染锚点判据（防 UI 退化成纯色块/资源丢失）

先 cargo run --release --example render_ui 生成 /tmp/opencode/ui_offline/*.png，
再逐区域统计「原版资源特征色」像素数——若按钮贴图和图标丢失、退化成单色，计数会骤降 → FAIL。
零设备依赖，可复跑。接入 rust_all.sh [4c]。
"""
import os
import subprocess
import sys

from PIL import Image

ROOT = "/disk/workspaces/jq/rev64/forward/src_rs_rust"
OUT = "/tmp/opencode/ui_offline"

# (屏幕, 判据名, 区域 box(900系), 颜色谓词, 最小特征像素数)
CHECKS = [
    ("menu", "主菜单人机按钮=原版蓝色渐变", (252, 1084, 645, 1179),
     lambda r, g, b: b > 150 and b > r + 40 and b > g + 20, 8000),
    ("menu", "主菜单房子白墙=原版资源", (60, 1330, 165, 1485),
     lambda r, g, b: r > 200 and g > 200 and b > 200, 400),
    ("ingame", "☰菜单星形图标=原版黄色", (100, 580, 215, 680),
     lambda r, g, b: r > 200 and g > 150 and b < 80, 400),
    ("ingame", "☰菜单半透明深面板", (400, 700, 650, 780),
     lambda r, g, b: r < 60 and g < 60 and b < 60, 6000),
    ("difficulty", "难度布局按钮=原版蓝(58,103,189)", (362, 621, 512, 696),
     lambda r, g, b: 50 <= r <= 66 and 95 <= g <= 111 and 181 <= b <= 197, 3000),
    ("gameset", "游戏设置确定=原版蓝按钮", (206, 991, 431, 1066),
     lambda r, g, b: b > 180 and r < 100 and g < 160, 3000),
]


def main() -> int:
    env = dict(os.environ)
    env["PATH"] = os.path.expanduser("~/.cargo/bin") + ":" + env.get("PATH", "")
    r = subprocess.run(["cargo", "run", "--release", "--example", "render_ui"],
                       cwd=ROOT, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if r.returncode != 0:
        print("FAIL cargo render_ui:", r.stderr.decode("utf-8", "replace")[-500:])
        return 1
    failed = 0
    for screen, name, box, pred, min_n in CHECKS:
        p = os.path.join(OUT, screen + ".png")
        if not os.path.exists(p):
            print(f"FAIL {name}: 缺图 {p}")
            failed += 1
            continue
        im = Image.open(p).convert("RGB").crop(box)
        n = sum(1 for px in im.getdata() if pred(*px))
        ok = n >= min_n
        print(f"{'PASS' if ok else 'FAIL'} {name}: {n} >= {min_n}")
        if not ok:
            failed += 1
    print(f"判定: {'FAIL' if failed else 'PASS'}（{len(CHECKS) - failed}/{len(CHECKS)}）")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
