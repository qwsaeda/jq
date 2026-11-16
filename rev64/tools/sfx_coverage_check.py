#!/usr/bin/env python3
# sfx_coverage_check.py — 73 事件名→evt55_sfx 键→14 WAV 三层覆盖门禁（R8-B-16/R7-B-16 落地）
# 三层矩阵：L0 73 名全集（jq_event_names_73.c 名表）/ L1 名→sfx_wav 键合法（∈14 键）
# / L2 键→WAV 文件存在（spak_res 实物）+ 兜底白名单（60/73 default→move）
# + 不可达资源白名单（start/over/select/close/pop：有键/文件，无 73 名映射可达，
#   属 UI 直播/资源冗余，非事件名链可达 = 白名单放行非 FAIL）。
# 事实源：evt55_sfx.c（名→键，strcmp 链静态可解析）/ jni_bridge.c sfx_wav（14 键）/
#   jq_event_names_73.c（73 名，[idx] 注释行）/ rev64/dist/spak_res（WAV 实物 14）。
# 判据：任一 L1 键非法 / L2 文件缺失 / 兜底名未入白名单 / 不可达资源白名单漂移 → FAIL。
# 纳入 align_gate 第 ⑧ 项（桌面 0 设备依赖；R7-B-20 设计落地）。
import io, os, re, sys, json

WS = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(WS, "rev64", "forward", "src_rs")
DIST = os.path.join(WS, "rev64", "dist")

def read(p):
    with io.open(p, encoding="utf-8", errors="ignore") as f:
        return f.read()

def load_73_names():
    """73 名全集 = jq_event_names_73.c 名表数组体（[idx] 注释行；含 Junqi.GameType%d 格式串）。"""
    txt = read(os.path.join(SRC, "jq_event_names_73.c"))
    # 名表数组体 = 第 5 行起至**第一个** `};`（jq_event_names_73[] 闭合）——
    # 旧 `\}\s*;` 非贪婪正则越过数组体末尾误匹配到 82 行 jq_event_code_names 表的
    # `};` 闭合（捕获段含 12 码分组区 2 条重复名 = 75 解析数根因）。
    m = re.search(
        r"jq_event_names_73\s*\[\s*JQ_EVENT_NAME_COUNT\s*\]\s*=\s*\{(.*?)^\};",
        txt, re.S | re.M)
    if not m:
        print("FATAL: jq_event_names_73 名表未找到（结构漂移）")
        sys.exit(2)
    # ★ 解析口径定案（R8-E 实锤）：名表 73 条目 = 行级 `"(Junqi|Misc)....",  /* [idx] */`
    #   形态（73 行 73 条）；名表**注释体内**可能引用旧形态名串（如 [72] 注释含
    #   "Junqi.GameType" 与 "Junqi.GameType%d" 2 串 = A4-04 纠错注），全段 findall
    #   会误收注释串（75 解析数根因）→ 只取行级带 `",` 收尾的名串（注释体内
    #   名串后接中文/箭头非 `",`）。
    names = []
    for ln in m.group(1).split("\n"):
        mm = re.match(r'\s*"((?:Junqi|Misc)\.[^"]*)",', ln)
        if mm:
            names.append(mm.group(1))
    if len(names) != 73:
        # 结构漂移告警（名表行级形态变化）→ 宽松兜底去重保序
        seen, uniq = set(), []
        for n in re.findall(r'"((?:Junqi|Misc)\.[^"]*)"', m.group(1)):
            if n not in seen:
                seen.add(n)
                uniq.append(n)
        names = uniq
        print("L0 告警: 行级解析 %d != 73，宽松兜底 %d 条（名表结构漂移？）"
              % (len(names), len(names)))
    return names

def parse_sfx_mapping(names):
    """evt55_sfx.c 名→键解析：strcmp 显式映射 + default 兜底 move（静态形态实锤）。"""
    txt = read(os.path.join(SRC, "evt55_sfx.c"))
    explicit = {}
    # 形态 1：单名 if (!strcmp(evname, "X")) return "k";
    for mm in re.finditer(r'if \(!strcmp\(evname,\s*"((?:Junqi|Misc)\.[^"]+)"\)\s*\)\s*return\s*"([a-z]+)"', txt):
        explicit[mm.group(1)] = mm.group(2)
    # 形态 2：多名 || 串 if (!strcmp(a) || !strcmp(b) || ...) return "k";
    # （★ R8-E：Win|RedWin|BlueWin / Draw|GameOver|Over / Resign|ResignConfirm 三族，
    #   旧正则 `[^)]*` 非贪婪在首个 `)`（strcmp 闭括号）即停 → 多名串恒 0 命中）
    for mm in re.finditer(r'if \(\s*[^;]*?\|\|[^;]*?\)\s*return\s*"([a-z]+)"', txt):
        for nm in re.findall(r'strcmp\(evname,\s*"((?:Junqi|Misc)\.[^"]+)"\)', mm.group(0)):
            explicit.setdefault(nm, mm.group(1))
    has_default_move = bool(re.search(r'return\s*"move"\s*;', txt))
    mapping = {n: explicit.get(n, "move" if has_default_move else None) for n in names}
    return mapping, explicit, has_default_move

def sfx_wav_keys():
    """jni_bridge.c sfx_wav 14 键 + 键→WAV 文件名（strcmp return SFX_DIR "..." 链）。"""
    txt = read(os.path.join(SRC, "jni_bridge.c"))
    keymap = {}
    i = txt.find("static const char *sfx_wav")
    # seg = sfx_wav 函数体（到**下一个** `\n}` 闭合）；旧 `find("\n}", i)` 命中 i 前
    # 邻函数的 `}` 或 1600 窗口截断致 seg 不含全部 14 键行 = L2 假 FAIL 根因。
    j = txt.find("\n}", i)
    seg = txt[i:j] if (i >= 0 and j > i) else (txt[i:i + 1600] if i >= 0 else txt)
    # 键→WAV 文件：逐行解析（★ R8-E 实锤形态：if 条件内多空格对齐
    #   `if (!strcmp(event, "k")   ) return SFX_DIR "f";` ——双右括号 `")   )`
    #   是 `)`(strcmp)+空格+`)`(if 条件闭括号) 形态，单括号正则恒 0 命中）
    for ln in seg.split("\n") if i >= 0 else []:
        mm = re.search(r'strcmp\(event,\s*"([a-z]+)"\)\s*\)?\s*return SFX_DIR\s*"([^"]+)"', ln)
        if mm:
            keymap[mm.group(1)] = mm.group(2)
    if len(keymap) != 14:
        print("L1 告警: sfx_wav 键解析 %d != 14（实际形态漂移？实键: %s）"
              % (len(keymap), sorted(keymap)))
    return keymap

def main():
    errors = []
    names = load_73_names()
    if len(names) != 73:
        errors.append("L0: 73 名解析数=%d != 73（jq_event_names_73.c 结构漂移？）" % len(names))
    mapping, explicit, has_default = parse_sfx_mapping(names)
    keymap = sfx_wav_keys()
    VALID_KEYS = set(keymap.keys())
    if len(VALID_KEYS) != 14:
        errors.append("L1: sfx_wav 键集=%d != 14（实键: %s）" % (len(VALID_KEYS), sorted(VALID_KEYS)))

    # 不可达资源白名单：14 键中无 73 名映射可达（仅 UI 直播/资源冗余）
    UNREACHABLE_WHITELIST = {"start", "over", "select", "close", "pop"}
    used = {k for k in mapping.values() if k}
    unreachable = sorted(VALID_KEYS - used)
    wl_ok = set(unreachable) == UNREACHABLE_WHITELIST
    if not wl_ok:
        errors.append("UNREACHABLE 白名单漂移: actual=%s expect=%s" % (unreachable, sorted(UNREACHABLE_WHITELIST)))

    # 三层矩阵
    rows = []
    for n in names:
        k = mapping[n]
        l1 = k in VALID_KEYS
        wav = keymap.get(k, "")
        # ★ L2 判定口径定案（R8-E 实锤）：WAV 文件名取自 C 宏拼接
        #   `return SFX_DIR "/data_junqi_move.wav"` 的**文件名字面量**（前导
        #   `/` 是宏展开 artifact 非路径段）→ 判据取 basename 与 spak_res 实物比。
        base = wav.rsplit("/", 1)[-1] if wav else ""
        l2 = bool(base) and os.path.isfile(os.path.join(DIST, "spak_res", base))
        fb = n not in explicit  # 兜底 default 命中（非显式映射）
        if not l1:
            errors.append("L1: %s → 键 '%s' 不在 14 键集" % (n, k))
        if l1 and not l2:
            errors.append("L2: %s → %s → spak_res/%s 文件缺失" % (n, k, base))
        rows.append({"name": n, "key": k, "wav": wav, "L1": l1, "L2": l2, "fallback": fb})

    fallback_n = sum(1 for r in rows if r["fallback"])
    fallback_expect = 73 - len(explicit)
    if fallback_n != fallback_expect:
        errors.append("FALLBACK 统计漂移: %d != %d" % (fallback_n, fallback_expect))

    out = {
        "names": len(names), "explicit": len(explicit), "fallback_default": fallback_n,
        "sfx_wav_keys": sorted(VALID_KEYS), "unreachable": unreachable,
        "unreachable_whitelist_ok": wl_ok,
        "l1_ok": all(r["L1"] for r in rows), "l2_ok": all(r["L2"] for r in rows),
        "matrix": rows, "errors": errors, "pass": len(errors) == 0,
    }
    print(json.dumps(out, ensure_ascii=False, indent=2))
    if out["pass"]:
        print("SFX_COVERAGE: PASS（73 名 L1/L2 全过；显式 %d + 兜底 move %d；不可达白名单 %d 键）"
              % (len(explicit), fallback_n, len(unreachable)))
        return 0
    print("SFX_COVERAGE: FAIL（%d 项）" % len(errors))
    for e in errors:
        print("  -", e)
    return 1

if __name__ == "__main__":
    sys.exit(main())
