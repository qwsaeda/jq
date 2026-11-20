#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""rust-anything 可复跑判据：原版 32 位 so 行为 → Rust 移植逐 case 对齐

固化 RUST_ANYTHING_POC.md 的 PoC 为门禁脚本。核心原则：
  1. 对齐基线 = 逆向翻译体（逐字节反汇编自原版 32 位 so VA），**不是正向 C 重写版**。
  2. 行为源**动态抽取**（不是硬编码文本）：从 jq_rs_rules.c 抽 ai_score 函数体、
     从 jq_rs_rules.h 抽 RANK_* 常量 + rank_norm。翻译体改了 → 基线重录 → 不会假绿。
  3. 负向测试防恒真：注入 Rust 独有的 -2 空源哨兵（R-057/R-092 已知分叉），
     diff **必须检出**分叉；检不出 = 判据恒真 = FAIL。
  4. 全程在隔离临时目录构建，不改动工作区任何 C/Rust 源码。

链路：extract → analyze → bridge → record → port → build → diff → gate → 负向 diff

用法：python3 rev64/tools/rust_anything_check.py [--keep-tmp]
rc：0=PASS  2=FAIL  3=PENDING（rust-anything 未装 / cargo 未装）
"""
import argparse, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RULES_C = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_rules.c")
RULES_H = os.path.join(ROOT, "rev64/reverse/src_jni_translated/jq_rs_rules.h")
# Rust 正向 SSOT（独立来源）：Rust 侧常量从这里抽，绝不能复用原版抽取值——
# 否则篡改原版时两边一起变 = 自证恒真假绿。
STATE_RS = os.path.join(ROOT, "rev64/forward/src_rs_rust/src/state.rs")


def fail(msg):
    print("FAIL:", msg)
    return 2


def run(cmd, cwd=None):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)


def extract_c(src):
    """从 jq_rs_rules.c 抽 ai_score 完整函数体（大括号配平）。"""
    m = re.search(r"^int ai_score\s*\([^)]*\)\s*\{", src, re.M)
    if not m:
        return None
    i = src.index("{", m.start())
    depth = 0
    for j in range(i, len(src)):
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
            if depth == 0:
                return src[m.start():j + 1]
    return None


def extract_const(h, name):
    """原版侧：从 jq_rs_rules.h 的 #define RANK_* 抽。"""
    m = re.search(r"#define\s+" + name + r"\s+(\d+)", h)
    return m.group(1) if m else None


def extract_rust_const(rs, name):
    """Rust 侧：从 state.rs 抽 RANK_* 数值（独立 SSOT）。
    兼容两种格式：
      旧 `pub const RANK_BOMB: u8 = 10;`
      新 `pub const RANK_BOMB: Rank = Rank::BOMB;` + `pub const BOMB: Rank = Rank(10);`
    """
    m = re.search(r"pub const\s+" + name + r"\s*:\s*u8\s*=\s*(\d+)", rs)
    if m:
        return m.group(1)
    # 新 newtype：别名 RANK_X = Rank::X → 查 impl 里的 X = Rank(N)
    alias = re.search(r"pub const\s+" + name + r"\s*:\s*Rank\s*=\s*Rank::(\w+)", rs)
    if alias:
        assoc = re.search(r"pub const\s+" + alias.group(1) + r"\s*:\s*Rank\s*=\s*Rank\((\d+)\)", rs)
        return assoc.group(1) if assoc else None
    return None


def extract_rank_norm(h):
    m = re.search(r"static inline int rank_norm\s*\([^)]*\)\s*\{[^}]*\}", h, re.S)
    return m.group(0) if m else None


# 忠实移植模板（对齐原版 ai_score：无 -2 哨兵；值来自 C 常量，运行时填）
PORT_TMPL = """#![forbid(unsafe_code)]
const RANK_BOMB: i32 = {BOMB};
const RANK_MINE: i32 = {MINE};
const RANK_FLAG: i32 = {FLAG};
const RANK_TOP: i32 = {TOP};
#[inline]
fn rank_norm(v: i32) -> i32 {{ if v > RANK_TOP {{ v - 0xc }} else {{ v }} }}
/// 忠实移植原版 32 位翻译体 ai_score（无 -2 哨兵，对齐录制基线）
pub fn ai_score(src: std::ffi::c_int, dst: std::ffi::c_int) -> std::ffi::c_int {{
    let r = rank_norm(src);
    if r == RANK_FLAG || r == RANK_MINE {{ return -1; }}
    let dr = if dst != 0 {{ rank_norm(dst) }} else {{ 0 }};
    if dst != 0 {{
        if dr == RANK_MINE {{ return if r == 9 {{ 700 }} else {{ 1 }}; }}
        if r < dr {{ return 1000; }}
        else if r == dr {{ return 900; }}
        else if r == RANK_BOMB {{ return 800; }}
        else {{ return 1; }}
    }}
    2
}}
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--keep-tmp", action="store_true", help="保留临时目录（调试用）")
    a = ap.parse_args()

    # 0. 前置判据
    if shutil.which("rust-anything") is None:
        print("PENDING: rust-anything 未安装（npm install -g rust-anything）")
        return 3
    if shutil.which("cargo") is None:
        cargo = os.path.expanduser("~/.cargo/bin")
        if not os.path.isdir(cargo):
            print("PENDING: cargo 未安装")
            return 3
        os.environ["PATH"] = cargo + os.pathsep + os.environ["PATH"]

    # 1. 动态抽取原版行为源
    csrc = open(RULES_C, encoding="utf-8").read()
    hsrc = open(RULES_H, encoding="utf-8").read()
    fn = extract_c(csrc)
    rn = extract_rank_norm(hsrc)
    # 双源独立：原版侧 from 翻译体（编译 original.so 录制行为），
    #           Rust 侧 from src_rs_rust/src/state.rs（port.rs 用）——自证恒真假绿根治。
    c_consts = {k: extract_const(hsrc, "RANK_" + k) for k in ("BOMB", "MINE", "FLAG", "TOP")}
    rs_src = open(STATE_RS, encoding="utf-8").read()
    rs_consts = {k: extract_rust_const(rs_src, "RANK_" + k) for k in ("BOMB", "MINE", "FLAG", "TOP")}
    if not fn or not rn or any(v is None for v in c_consts.values()) or any(v is None for v in rs_consts.values()):
        return fail("抽取 ai_score/rank_norm/RANK_* 失败（翻译体或 state.rs 结构漂移？）")
    print("原版常量: BOMB=%s MINE=%s FLAG=%s TOP=%s (jq_rs_rules.h)" %
          (c_consts["BOMB"], c_consts["MINE"], c_consts["FLAG"], c_consts["TOP"]))
    print("Rust 常量: BOMB=%s MINE=%s FLAG=%s TOP=%s (state.rs)" %
          (rs_consts["BOMB"], rs_consts["MINE"], rs_consts["FLAG"], rs_consts["TOP"]))
    # 显式静态断言：随机 case 采样可能碰巧不覆盖某个常量分支（覆盖率假绿），
    # 故常量一致性必须直接断言，不能只靠 diff 采样。
    drift = [k for k in ("BOMB", "MINE", "FLAG", "TOP") if c_consts[k] != rs_consts[k]]
    if drift:
        return fail("RANK_* 常量双源漂移 %s（原版 %s vs Rust %s）" %
                    (drift, [c_consts[k] for k in drift], [rs_consts[k] for k in drift]))

    tmp = tempfile.mkdtemp(prefix="rust-anything-check-")
    try:
        unit = os.path.join(tmp, "unit")
        os.makedirs(unit)
        # 最小自包含 C：常量 + rank_norm + ai_score（逐字节来自翻译体）
        defs = "\n".join("#define RANK_%s %s" % (k, c_consts[k]) for k in ("BOMB", "MINE", "FLAG", "TOP"))
        min_c = ("/* 原版32位逆向翻译体 ai_score 最小自包含抽取（动态生成，勿手改） */\n"
                 + defs + "\n" + rn + "\n" + fn + "\n")
        open(os.path.join(unit, "min_rules.c"), "w", encoding="utf-8").write(min_c)

        # 2. analyze + bridge
        if run(["rust-anything", "analyze", ".", "--lang", "c", "--json"], unit).returncode != 0:
            return fail("analyze 失败")
        br = run(["rust-anything", "bridge", ".", "min_rules.c", "-o", "rust/min-rs", "--name", "min-rs"], unit)
        if br.returncode != 0:
            return fail("bridge 失败: " + br.stderr.strip()[:200])

        crate = os.path.join(unit, "rust/min-rs")
        port = os.path.join(crate, "src/port.rs")
        # 3. record（原版行为基线）
        rec = run(["rust-anything", "record", crate], unit)
        if rec.returncode != 0:
            return fail("record 失败: " + rec.stderr.strip()[:200])
        cases = os.path.join(crate, "cases.jsonl")
        ncase = sum(1 for _ in open(cases)) if os.path.exists(cases) else 0
        print("record: %d cases" % ncase)
        if ncase < 50:
            return fail("录制的 case 数过少（%d < 50，防空集假绿）" % ncase)

        # 4. port（忠实移植）→ cargo fmt → build → diff
        open(port, "w", encoding="utf-8").write(PORT_TMPL.format(**rs_consts))
        run(["cargo", "fmt", "--manifest-path", os.path.join(crate, "Cargo.toml")])
        if run(["rust-anything", "build", crate]).returncode != 0:
            return fail("build 失败（翻译体行为已变，port 需同步？）")
        d = run(["rust-anything", "diff", crate])
        m = re.search(r"(\d+)/(\d+) cases match", d.stdout + d.stderr)
        if d.returncode != 0 or not m or m.group(1) != m.group(2):
            return fail("diff 未全匹配: " + (d.stdout + d.stderr).strip()[-300:])
        print("diff: %s/%s cases match" % (m.group(1), m.group(2)))

        # 5. gate
        g = run(["rust-anything", "gate", crate])
        if g.returncode != 0 or "Gate passed" not in (g.stdout + g.stderr):
            return fail("gate 未通过: " + (g.stdout + g.stderr).strip()[-300:])
        print("gate: Gate passed")

        # 6. 负向测试：注入 -2 空源哨兵，diff 必须检出（防判据恒真）
        good = open(port, encoding="utf-8").read()
        bad = good.replace("    let r = rank_norm(src);",
                           "    if src == 0 { return -2; }  // 注入已知分叉 R-057/R-092\n    let r = rank_norm(src);", 1)
        if bad == good:
            return fail("负向测试注入失败（port 结构漂移，锚点未命中）")
        open(port, "w", encoding="utf-8").write(bad)
        run(["rust-anything", "build", crate])
        nd = run(["rust-anything", "diff", crate])
        nd_out = nd.stdout + nd.stderr
        detected = re.findall(r"^\s*-\s*ai_score\(0,", nd_out, re.M)
        open(port, "w", encoding="utf-8").write(good)  # 恢复
        if nd.returncode == 0 or len(detected) == 0:
            return fail("负向测试未检出 -2 哨兵分叉（判据恒真，非假绿防线失效）")
        print("负向测试: 检出 %d 处分叉（ai_score(0,*) expected vs -2），恢复后基线有效" % len(detected))
        print("判定: PASS（原版32位行为录制 → Rust 移植 %s/%s match + gate + 负向可检出）" % (m.group(1), m.group(2)))
        return 0
    finally:
        if a.keep_tmp:
            print("临时目录保留:", tmp)
        else:
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
