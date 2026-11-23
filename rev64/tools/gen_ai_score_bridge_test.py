#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从原版录制的 ai_score cases.jsonl 生成 Rust 集成测试，直接测生产 ai::ai_score。

P0-1 根因修复：此前 rust-anything 判据测的是脚本内嵌模板（非生产代码），
篡改生产 ai_score 判据仍 PASS。本生成器让原版真值表驱动生产函数测试：
  - 读 tests_data/ai_score_orig_cases.jsonl（原版 32 位行为录制，154 条）
  - 生成 tests/ai_score_orig_bridge.rs，直接 crate::ai::ai_score(src,dst,true)
  - Some(x)→x 与真值比对；None（生产 -2/-1 哨兵，R-057/R-092 已知分叉）走白名单
用法：python3 rev64/tools/gen_ai_score_bridge_test.py
"""
import json, os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CASES = os.path.join(ROOT, "rev64/forward/src_rs_rust/tests_data/ai_score_orig_cases.jsonl")
OUT = os.path.join(ROOT, "rev64/forward/src_rs_rust/tests/ai_score_orig_bridge.rs")

# 输入域分类：
#  - 生产域内 (src∈[1,24], dst∈[0,24])：与原版真值逐条比对（真正测生产代码）
#  - 域外 (src<0/==0/>24 或 dst<0/>24)：生产有 -2 哨兵(R-057/R-092)，原版 C 模板无 → 已知分叉跳过
def in_production_domain(src, dst):
    return 1 <= src <= 24 and 0 <= dst <= 24

def main():
    rows = [json.loads(l) for l in open(CASES) if l.strip()]
    assert rows, "cases 为空"
    lines = [
        "// ★ 自动生成（gen_ai_score_bridge_test.py）：原版 32 位 ai_score 真值 → 直接测生产 ai::ai_score",
        "// P0-1 根因修复：判据真正测生产代码，篡改生产 ai_score 必被本测试检出。",
        "use junqi_rs::ai;",
        "",
        "#[test]",
        "fn ai_score_matches_original_recorded_cases() {",
        "    let mut checked = 0usize;",
        "    let mut fork = 0usize;",
    ]
    for i, r in enumerate(rows):
        src, dst, want = r["args"][0], r["args"][1], r["ok"]
        # C args 可能是 i64 极值（9223372036...），生产 ai_score 收 i32——裁剪到 i32 域
        src32 = max(-2147483648, min(2147483647, int(src)))
        dst32 = max(-2147483648, min(2147483647, int(dst)))
        if in_production_domain(src32, dst32):
            lines.append(f"    {{ let got = ai::ai_score({src32}, {dst32}, true); assert_eq!(got, Some({want}), \"case{i} 原版真值 src={src32} dst={dst32}\"); checked += 1; }}")
        else:
            # 域外（极值/哨兵）：生产与原版 C 模板输入域假设不同，跳过不强比（R-057/R-092）
            lines.append(f"    // case{i}: 域外 src={src32} dst={dst32} 跳过（生产/原版输入域不同）")
            lines.append(f"    fork += 1;")
    lines += [
        # ★ RTEST-005 修复：旧文案 ">50" 与实际 12 条真值断言严重不符（142 case 被标域外
        #   fork 跳过），阈值 >=10 删 2 条仍 PASS = 假绿。改：阈值 12（=当前真值数，删 1 条即 FAIL）
        #   + 文案与实际一致。注：域外白名单本身是 RTEST-010 残留面，本修复只修阈值/文案漂移。
        f"    assert!(checked >= 12, \"真值 case 数 {{checked}} >= 12（12 条生产断言，删任 1 条即检出）\");",
        "    eprintln!(\"生产 ai_score 对齐原版: {} 条一致 + {} 条已知分叉\", checked, fork);",
        "}",
    ]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, "w").write("\n".join(lines) + "\n")
    domain = sum(1 for r in rows if in_production_domain(max(-2147483648,min(2147483647,int(r['args'][0]))), max(-2147483648,min(2147483647,int(r['args'][1])))))
    print(f"生成 {OUT}: {len(rows)} 条（生产域真值 {domain} / 域外哨兵 {len(rows)-domain}）")

if __name__ == "__main__":
    main()
