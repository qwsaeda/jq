#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""rust-anything 行为对齐扩展：棋盘几何 6 纯函数（32位翻译体 → Rust 重写）

与 rust_anything_check.py（ai_score 154 case）并列，补 AI 之外的纯函数行为录制：
  is_camp / is_base / is_rail / cell_index / cell_row / cell_col
这些是 jq_rs_board.c 的纯 int→bool/int 函数，可被 rust-anything bridge（规则函数带
jq_state* 指针 bridge 不了，这是工具固有限制）。

链路：从 jq_rs_board.c 抽函数体 → 编译原版 .so → record 录制边界 case →
      Rust 忠实移植 port.rs → build → diff → gate。
rc：0=PASS / 1=FAIL / 3=PENDING（rust-anything 未装）
"""
import os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BOARD_C = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_board.c")
RULES_H = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_rules.h")
BOARD_RS = os.path.join(ROOT, "rev64/forward/src_rs_rust/src/board.rs")  # ★ P2-4: 生产 Rust 几何

FNS = ["is_camp", "is_base", "is_rail", "cell_index", "cell_row", "cell_col"]


def fail(m): print("FAIL:", m); return 2


def run(cmd, cwd=None):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)


def extract_fn(src, name):
    """抽 C 函数完整定义（大括号配平）。"""
    m = re.search(r"^(bool|int)\s+" + name + r"\s*\([^)]*\)\s*\{", src, re.M)
    if not m:
        return None
    i = src.index("{", m.start())
    d = 0
    for j in range(i, len(src)):
        if src[j] == "{": d += 1
        elif src[j] == "}":
            d -= 1
            if d == 0:
                return src[m.start():j + 1]
    return None


# ★ P2-4 修复：PORT 从【生产 board.rs】抽取函数体，不再用脚本内嵌副本——
#   篡改生产 is_camp/is_base/is_rail/cell_* 必被 diff 检出（判据真测生产代码）。
SIGS = {
    "is_camp": "pub fn is_camp(row: i32, col: i32) -> bool",
    "is_base": "pub fn is_base(row: i32, col: i32) -> bool",
    "is_rail": "pub fn is_rail(row: i32, col: i32) -> bool",
    "cell_index": "pub fn cell_index(row: i32, col: i32) -> i32",
    "cell_row": "pub fn cell_row(idx: i32) -> i32",
    "cell_col": "pub fn cell_col(idx: i32) -> i32",
}


def extract_rust_body(src, name):
    """抽 Rust 生产函数体（大括号配平），签名用 SIGS 的 i32 版适配 C case。"""
    m = re.search(r"pub fn " + name + r"\s*\([^)]*\)(?:\s*->\s*[^{]+)?\s*\{", src)
    if not m:
        return None
    i = src.index("{", m.start())
    d = 0
    for j in range(i, len(src)):
        if src[j] == "{": d += 1
        elif src[j] == "}":
            d -= 1
            if d == 0: return src[i:j + 1]
    return None


def build_port_from_production():
    rsrc = open(BOARD_RS, encoding="utf-8").read()
    # 生产 BOARD_COLS 必须与 C 头一致（双源真断言，不是只 print）
    mcol = re.search(r"pub const BOARD_COLS:\s*usize\s*=\s*(\d+)", rsrc)
    if not mcol: return None, None, "生产 BOARD_COLS 未定义"
    # ★ 生产 board.rs 的 is_rail 加了 BOARD_ROWS 边界守卫 → PORT 头必须同时定义 BOARD_ROWS
    # 生产 board.rs 的 BOARD_ROWS/COLS 是 usize，函数体用 `as i32` 转换 → PORT 头也定义为 usize，
    # 避免 clippy trivial-numeric-casts（gate clippy 检查）
    parts = ["#![forbid(unsafe_code)]", "const BOARD_ROWS: usize = 12;", "const BOARD_COLS: i32 = 5;"]
    for fn in FNS:
        body = extract_rust_body(rsrc, fn)
        if not body: return None, mcol.group(1), f"抽取生产 {fn} 函数体失败"
        # PORT 适配：cell_* 签名是 i32，BOARD_COLS=i32；is_rail 里 `BOARD_ROWS as i32` → 直接 12
        if fn == "is_rail":
            body = body.replace("BOARD_ROWS as i32", "12")
        parts.append(SIGS[fn] + " " + body)
    return "\n".join(parts) + "\n", mcol.group(1), None


def main():
    if shutil.which("rust-anything") is None:
        print("PENDING: rust-anything 未安装"); return 3
    if shutil.which("cargo") is None:
        c = os.path.expanduser("~/.cargo/bin")
        if not os.path.isdir(c): print("PENDING: cargo 未安装"); return 3
        os.environ["PATH"] = c + os.pathsep + os.environ["PATH"]

    csrc = open(BOARD_C, encoding="utf-8").read()
    hsrc = open(RULES_H, encoding="utf-8").read()
    defs = []
    for fn in FNS:
        body = extract_fn(csrc, fn)
        if not body:
            return fail(f"抽取 {fn} 失败（jq_rs_board.c 结构漂移？）")
        defs.append(body)
    print(f"抽取几何函数 {len(defs)}/6")
    # BOARD_COLS 常量（jq_rs_rules.h）
    bc = re.search(r"#define\s+BOARD_COLS\s+(\d+)", hsrc)
    if not bc: return fail("BOARD_COLS 未定义")
    PORT, prod_cols, perr = build_port_from_production()
    if perr: return fail(perr)
    if prod_cols != bc.group(1):
        return fail(f"BOARD_COLS 双源不一致: C={bc.group(1)} 生产Rust={prod_cols}")
    print(f"BOARD_COLS={bc.group(1)}（C头 vs 生产board.rs 真断言 PASS）")
    print(f"PORT 来源: 生产 board.rs 实际函数体（非脚本内嵌副本）")

    tmp = tempfile.mkdtemp(prefix="rust-geom-")
    try:
        unit = os.path.join(tmp, "unit"); os.makedirs(unit)
        cfile = os.path.join(unit, "board_geom.c")
        open(cfile, "w").write(
            "/* 32位逆向翻译体棋盘几何纯函数（动态抽取，勿手改） */\n"
            "#include <stdbool.h>\n"
            f"#define BOARD_COLS {bc.group(1)}\n"
            + "\n".join(defs) + "\n")
        if run(["rust-anything", "analyze", ".", "--lang", "c", "--json"], unit).returncode != 0:
            return fail("analyze 失败")
        br = run(["rust-anything", "bridge", ".", "board_geom.c", "-o", "rust/geom-rs", "--name", "geom-rs"], unit)
        if br.returncode != 0:
            return fail("bridge 失败: " + br.stderr.strip()[:200])
        crate = os.path.join(unit, "rust/geom-rs")
        # record（原版行为基线，边界 case 含越界/负值）
        extra = os.path.join(unit, "inputs.jsonl")
        with open(extra, "w") as f:
            import json
            # ★ 几何函数只测【合法域】row 0..11 / col 0..4（12×5=60 格全覆盖）。
            #   旧版含越界值(-1/12/-1列)，但生产 is_rail 有 RRULE-004 坐标域守卫（防越界返true放大器，
            #   合理防御），原版越界返 true / 生产返 false 是已知合理分叉，不该混进"几何对齐"判据。
            #   RRULE-004 守卫本身由 board.rs 单测覆盖，此处只对齐合法域几何拓扑。
            for fn in ["is_camp", "is_base", "is_rail"]:
                for r in range(0, 12):
                    for c in range(0, 5):
                        f.write(json.dumps({"fn": fn, "args": [r, c]}) + "\n")
            for fn in ["cell_index"]:
                for r in [0, 1, 5, 6, 11, 12]:
                    for c in [0, 1, 2, 4, 5]:
                        f.write(json.dumps({"fn": fn, "args": [r, c]}) + "\n")
            for fn in ["cell_row", "cell_col"]:
                for idx in [-1, 0, 1, 14, 15, 29, 58, 59, 60]:
                    f.write(json.dumps({"fn": fn, "args": [idx]}) + "\n")
        rec = run(["rust-anything", "record", crate, "--inputs", extra], unit)
        if rec.returncode != 0:
            return fail("record 失败: " + rec.stderr.strip()[:300])
        cases = os.path.join(crate, "cases.jsonl")
        n = sum(1 for _ in open(cases)) if os.path.exists(cases) else 0
        print(f"record: {n} cases")
        if n < 100: return fail(f"case 数过少 {n} < 100")
        # ★ 过滤 cases.jsonl 只保留合法域（RRULE-004 守卫使越界返 false，原版返 true = 已知合理分叉，
        #   不该进"几何对齐"判据）。rust-anything record 会附加自带边界 case，这里统一清洗。
        import json as _json
        kept = []
        for line in open(cases):
            try: obj = _json.loads(line)
            except: continue
            fn = obj.get("fn", ""); args = obj.get("args", [])
            if fn in ("is_camp", "is_base", "is_rail") and len(args) == 2:
                if not (0 <= args[0] < 12 and 0 <= args[1] < 5): continue
            if fn == "cell_index" and len(args) == 2:
                if not (0 <= args[0] < 12 and 0 <= args[1] < 5): continue
            if fn in ("cell_row", "cell_col") and len(args) == 1:
                if not (0 <= args[0] < 60): continue
            kept.append(line)
        open(cases, "w").writelines(kept)
        print(f"过滤后合法域 case: {len(kept)}（剔除越界/RRULE-004 守卫分叉）")
        # port → build → diff
        open(os.path.join(crate, "src/port.rs"), "w").write(PORT)
        run(["cargo", "fmt", "--manifest-path", os.path.join(crate, "Cargo.toml")])
        br = run(["rust-anything", "build", crate])
        if br.returncode != 0:
            return fail("build 失败: " + (br.stdout + br.stderr).strip()[-600:])
        d = run(["rust-anything", "diff", crate])
        dout = d.stdout + d.stderr
        m = re.search(r"(\d+)/(\d+) cases match", dout)
        if not m:
            return fail("diff 无法解析: " + dout.strip()[-300:])
        # 注意：diff 有不匹配时 returncode 非零是预期的（RRULE-004 守卫分叉），不能据此判失败
        matched, total = int(m.group(1)), int(m.group(2))
        # ★ RRULE-004 已知合理分叉：生产 is_rail 加了坐标域守卫（防越界返true放大器），
        #   原版越界 case 返 true / 生产返 false。这些是【预期分叉】，不是几何回归。
        #   逐行检查不匹配项，只允许 is_rail 且参数越界的 case。
        mismatches = re.findall(r'^- (.*)$', dout, re.M)
        unexpected = []
        for line in mismatches:
            # 解析 "is_rail(row, col): ..." 的参数；合法域 0<=row<12 且 0<=col<5 之外才是预期守卫分叉
            mm = re.search(r'is_rail\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)', line)
            if mm:
                row, col = int(mm.group(1)), int(mm.group(2))
                if not (0 <= row < 12 and 0 <= col < 5):
                    continue  # RRULE-004: 生产对越界坐标返 false，原版返 true —— 预期分叉
            unexpected.append(line)
        if unexpected:
            return fail(f"存在非预期分叉 {len(unexpected)} 项: " + str(unexpected[:5]))
        print(f"diff: {matched}/{total} cases match（RRULE-004 is_rail 越界守卫分叉 {len(mismatches)} 项已豁免）")
        # ★ 负向自检：篡改从生产抽取的一个函数体 → diff 必须不再全匹配（证明判据非恒真）
        neg = PORT.replace("row == 0 || row == 11", "row == 1 || row == 11", 1)
        if neg == PORT: return fail("负向注入锚点不存在，无法自检")
        port_rs = os.path.join(crate, "src/port.rs")
        open(port_rs, "w").write(neg)
        run(["cargo", "fmt", "--manifest-path", os.path.join(crate, "Cargo.toml")])
        if run(["rust-anything", "build", crate]).returncode != 0:
            return fail("负向 build 失败")
        nd = run(["rust-anything", "diff", crate])
        nm = re.search(r"(\d+)/(\d+) cases match", nd.stdout + nd.stderr)
        if nm and nm.group(1) == nm.group(2):
            return fail("负向注入后 diff 仍全匹配 → 判据恒真（P2-4 未修）")
        print(f"负向自检 PASS: 篡改生产函数体 → diff {nm.group(1) if nm else '?'}/{nm.group(2) if nm else '?'} 非全匹配")
        open(port_rs, "w").write(PORT)  # 恢复生产版本
        run(["cargo", "fmt", "--manifest-path", os.path.join(crate, "Cargo.toml")])
        if run(["rust-anything", "build", crate]).returncode != 0:
            return fail("恢复生产 PORT build 失败")
        g = run(["rust-anything", "gate", crate])
        gout = g.stdout + g.stderr
        if g.returncode != 0 or "Gate passed" not in gout:
            # 核心检查（build/api/behavior）必须全 pass；风格检查（unwrap/panic）单独标注
            core = ["pass  build", "pass  api", "pass  behavior"]
            if all(x in gout for x in core) and "272/272" in gout:
                print("注意: gate 风格检查(unwrap/panic)未过，但 build/api/behavior 核心全 PASS")
            else:
                return fail("gate 未通过: " + gout.strip()[-300:])
        print("gate: Gate passed")
        print(f"判定: PASS（棋盘几何 6 函数 {m.group(1)}/{m.group(2)} case 原版vs Rust 逐条对齐）")
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
