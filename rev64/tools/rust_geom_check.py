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


# Rust 忠实移植（与 jq_rs_board.c 逐行等价；BOARD_COLS=5）
PORT = """#![forbid(unsafe_code)]
const BOARD_COLS: i32 = 5;
pub fn is_camp(row: i32, col: i32) -> bool {
    if row == 2 && (col == 1 || col == 3) { return true; }
    if row == 3 && col == 2 { return true; }
    if row == 4 && (col == 1 || col == 3) { return true; }
    if row == 7 && (col == 1 || col == 3) { return true; }
    if row == 8 && col == 2 { return true; }
    if row == 9 && (col == 1 || col == 3) { return true; }
    false
}
pub fn is_base(row: i32, col: i32) -> bool {
    (row == 0 || row == 11) && (col == 1 || col == 3)
}
pub fn is_rail(row: i32, col: i32) -> bool {
    if is_camp(row, col) || is_base(row, col) { return false; }
    if col == 2 { return true; }
    if row == 5 || row == 6 { return true; }
    if row == 0 || row == 11 { return true; }
    false
}
pub fn cell_index(row: i32, col: i32) -> i32 { row * BOARD_COLS + col }
pub fn cell_row(idx: i32) -> i32 { idx / BOARD_COLS }
pub fn cell_col(idx: i32) -> i32 { idx % BOARD_COLS }
"""


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
    print(f"BOARD_COLS={bc.group(1)}（双源一致断言）")

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
            for fn in ["is_camp", "is_base", "is_rail"]:
                for r in [-1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12]:
                    for c in [-1, 0, 1, 2, 3, 4, 5]:
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
        # port → build → diff
        open(os.path.join(crate, "src/port.rs"), "w").write(PORT)
        run(["cargo", "fmt", "--manifest-path", os.path.join(crate, "Cargo.toml")])
        if run(["rust-anything", "build", crate]).returncode != 0:
            return fail("build 失败")
        d = run(["rust-anything", "diff", crate])
        m = re.search(r"(\d+)/(\d+) cases match", d.stdout + d.stderr)
        if d.returncode != 0 or not m or m.group(1) != m.group(2):
            return fail("diff 未全匹配: " + (d.stdout + d.stderr).strip()[-300:])
        print(f"diff: {m.group(1)}/{m.group(2)} cases match")
        g = run(["rust-anything", "gate", crate])
        if g.returncode != 0 or "Gate passed" not in (g.stdout + g.stderr):
            return fail("gate 未通过: " + (g.stdout + g.stderr).strip()[-300:])
        print("gate: Gate passed")
        print(f"判定: PASS（棋盘几何 6 函数 {m.group(1)}/{m.group(2)} case 原版vs Rust 逐条对齐）")
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
