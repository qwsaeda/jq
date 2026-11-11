#!/usr/bin/env python3
"""regen_vtbl.py — jqt_vtbl.c 机械化再生成（防手工重复劳动）
用法:
  python3 rev64/tools/regen_vtbl.py            # chain 基线版（默认）
  python3 rev64/tools/regen_vtbl.py sp27       # sp27 版（13 访问器 + 27 闭包，§sp27 稳定态）
  python3 rev64/tools/regen_vtbl.py sp27 build # sp27 + 自动编译 + 打包 + 装 MuMu
作用:
  1. 从源文件恢复基线（chain: /tmp/opencode/chain/jqt_vtbl.c；sp27: jqt_vtbl_sp27.c）
  2. 自动注入幂等补丁：
     - i2f 存根批量修（tex_pipeline.py i2f --fix 等价物）
     - jqt_blx_dispatch 绝对归一化（reg_val>=0x80000000 → off=reg_val-shadow）
     - F_23eec L2400c 强制 glClear（仅 chain 版，F_23eec 完整体；sp27 版 F_23eec 存根则跳过）
     - F_9d3d4 渲染器单例访问器完整体（sp27 版：_Znwj(136)+F_9d988 构造 + vtable ABS(0x1595c0) + 存 0x162400 槽）
     - 重复定义自动去重（sp27 regen 的 F_b5eec 类，max_funcs BFS 截断产生）
  3. brace 完整性检查（全函数 depth 归 0）
  4. 写 rev64/reverse/src_jni_translated/jqt_vtbl.c
  5. build: 编译 so + 打包 APK + 安装 MuMu + 启动
背景（§sp27 沉淀）：13→14 参数批量迁移 regex 误伤 52 处断裂行 → 编译失败。
根因 = gen 工具链 13/14 参数不统一。手工 sed 式迁移不可靠，回归单点注入（本工具固化）。
"""
import re, sys, os, subprocess, glob, json

CHAIN = '/tmp/opencode/chain/jqt_vtbl.c'
SP27 = '/tmp/opencode/chain/jqt_vtbl_sp27.c'
OUT = 'rev64/reverse/src_jni_translated/jqt_vtbl.c'
VARIANT = sys.argv[1] if len(sys.argv) > 1 else 'chain'
DO_BUILD = 'build' in sys.argv

# ============ 补丁 1: i2f 存根 → 真实实现 ============
I2F_STUB = r'(L[0-9a-f]+: )r0=0; /\* blx 0x232c8 存根 \*/'
I2F_REAL = r'\1{ float _f=(float)(int32_t)r0; r0=*(uint32_t*)&_f; } /* blx 0x232c8 __aeabi_i2f 真实 */'

# ============ 补丁 2: jqt_blx_dispatch 绝对归一化 ============
DISPATCH_OLD = """    uint32_t off;
    if (reg_val >= (uint32_t)g_jqt_shadow && reg_val < (uint32_t)g_jqt_shadow + 0x200000u)
        off = reg_val - (uint32_t)g_jqt_shadow;
    else
        off = reg_val;"""
DISPATCH_NEW = """    uint32_t off;
    /* §124zz-9：reg_val >= 0x80000000（RELATIVE 重定位后 vtable 槽值 = 真实绝对地址）
     * → 减 shadow 基址得 .text 内偏移；旧版只处理 shadow 区内指针 → 全 miss */
    if (reg_val >= (uint32_t)g_jqt_shadow && reg_val < (uint32_t)g_jqt_shadow + 0x200000u)
        off = reg_val - (uint32_t)g_jqt_shadow;
    else if (reg_val >= 0x80000000u)
        off = reg_val - (uint32_t)g_jqt_shadow;   /* 绝对地址 → .text 偏移 */
    else
        off = reg_val;"""

# ============ 补丁 3: L2400c 强制 glClear（仅 chain 版 F_23eec 完整体）============
L24008_OLD = 'L24008: r0=F_32f8c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x32f8c 完整 */'
L24008_NEW = '''L24008: r0=F_32f8c(r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); /* bl 0x32f8c 完整 */
/* §124zz-9 强制 glClear：渲染器单例 0x162400 的 vtable[13]（= F_9d458） */
{ uint32_t _rs = MEM(0x162400);
  if (_rs) {
    uint32_t _vt = MEM(_rs);
    r3 = JQT_ABS_PTR(MEM(_vt+52));
    r0 = _rs;
    r1 = r1 ? r1 : 0xff9c6630;
    { r0=jqt_blx_dispatch(r3, r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,sp); }
    __android_log_print(6,"junqi","F_23eec L2400c 强制glClear this=0x%08x vt=0x%08x method=0x%08x", _rs, _vt, r3);
  } else {
    __android_log_print(6,"junqi","F_23eec L2400c 渲染器单例0x162400=0 跳过 glClear");
  } }'''

# ============ 补丁 4: F_9d3d4 渲染器单例访问器完整体（§sp27）============
F9D3D4_STUB = 'uint32_t F_9d3d4(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){ uint32_t r0=_r0; g_cur_fn=0x9d3d4; return r0; }'
F9D3D4_BODY = '''uint32_t F_9d3d4(uint32_t _r0,uint32_t _r1,uint32_t _r2,uint32_t _r3,uint32_t _r4,uint32_t _r5,uint32_t _r6,uint32_t _r7,uint32_t _r8,uint32_t _r9,uint32_t _r10,uint32_t _r11,uint32_t _sp){
    /* §124zz-10：渲染器单例 0x162400 访问器（原版 0x9d3dc 语义：读单例槽 → 非空返回；
     * 否则 _Znwj(136)+F_9d988 构造（vtable=ABS(0x1595c0)）+ 存单例槽。幂等。
     * 直接构造（不调 F_9d3dc——其 literal pool MEM(644064+28) 在 .text 偏移语义下
     * 翻译错 → 写坏单例槽崩。0x162400 槽地址来自 jqt_OnRender_full.c 的渲染回退约定）。 */
    extern void* _Znwj(unsigned int);
    extern uint32_t F_9d988(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
    uint32_t r0=_r0;
    g_cur_fn=0x9d3d4;
    { uint32_t _csp; __asm__ volatile("mov %0,sp" : "=&r"(_csp)); _sp=_csp; }
    uint32_t sp=_sp;
    uint32_t r4=0, r5=0;
    r4 = 0x162400;                          /* 单例槽（.bss） */
    r5 = MEM(r4);
    if (r5 != 0) { __android_log_print(4,"junqi","F_9d3d4 单例已构造 0x162400=0x%08x", r5); return r5; }
    r5 = (uint32_t)_Znwj(136);             /* 渲染器类大小 0x88=136 */
    if (r5 == 0) { __android_log_print(6,"junqi","F_9d3d4 _Znwj(136)=0，无法构造单例"); return 0; }
    /* F_9d988(this=r5)：基类 ctor F_9f88c + this[0]=vtable ABS(0x1595c0) + 字段清零 */
    F_9d988(r5,0,0,0,0,0,0,0,0,0,0,0,sp);
    /* §sp28-ROOT：手工填场景数据链字段（攻 sp28 崩溃根因——先填字段再放行纹理上传）。
     * 原版 ctor 只清零 this[0x1c..0x48]，this[0x4c](texID)/this[0x20](宽)/this[0x24](高)/this[0x1c](像素指针)
     * 全 .bss 残留=0 → F_9e0f0/F_9e204 读 0 → memcpy 读 0x16 SEGV。
     * 填 board_datah.png 737x751 RGB 像素（jq_board_px.h，SPAK 解包产物，static const 内部链接，
     * 文件头 #include 提供；勿加 extern——与 static 矛盾会造悬空 UND → MuMu failed to link §124zz）：
     *   this[0x1c]=像素指针(jq_board_px)  this[0x20]=宽(jq_board_w=737)
     *   this[0x24]=高(jq_board_h=751)     this[0x4c]=0（texID=0 → F_9e2b4 走创建路径 F_9e204
     *                                        glGen+glTexImage2D，真实创建棋盘纹理；
     *                                        非 0 会误走上传路径查空脏区，不创建） */
    MEM(r5+0x1c) = (uint32_t)jq_board_px;
    MEM(r5+0x20) = (uint32_t)jq_board_w;
    MEM(r5+0x24) = (uint32_t)jq_board_h;
    MEM(r5+0x4c) = 0;                      /* texID=0（创建路径，F_9e2b4→F_9e204 glGen+glTexImage） */
    MEM(r4) = r5;                          /* 存单例槽 */
    __android_log_print(4,"junqi","F_9d3d4 渲染器单例构造 0x162400=0x%08x vtable=0x%08x 填字段 px=jq_board_px w=%d h=%d texID=0(创建)", r5, MEM(r5), jq_board_w, jq_board_h);
    return r5;
}'''

# ============ 工具函数 ============
def dedup_defs(src):
    """删除同名函数定义（第 2 个起，按行范围 depth 归 0 定位）"""
    lines = src.split('\n')
    heads = []  # (line_idx, f_name)
    for i, l in enumerate(lines):
        m = re.match(r'^uint32_t F_([0-9a-f]+)\(uint32_t _r0,', l)
        if m:
            heads.append((i, m.group(1)))
    seen = set()
    dup_ranges = []
    for idx, name in heads:
        if name in seen:
            d = 0; end = idx
            for i in range(idx, min(idx + 500, len(lines))):
                d += lines[i].count('{') - lines[i].count('}')
                if d == 0 and i > idx:
                    end = i; break
            dup_ranges.append((idx, end))
        else:
            seen.add(name)
    if dup_ranges:
        names = []
        for s, e in dup_ranges:
            m2 = re.match(r'^uint32_t F_([0-9a-f]+)', lines[s])
            if m2: names.append(m2.group(1))
        for s, e in sorted(dup_ranges, reverse=True):
            del lines[s:e+1]
        print("去重: 删 %d 个重复定义 (%s)" % (len(dup_ranges), ", ".join(sorted(set(names)))))
    return '\n'.join(lines), len(dup_ranges)

def main():
    if VARIANT == 'sp27':
        base = SP27
    else:
        base = CHAIN
    if not os.path.exists(base):
        print("错误: 源文件 %s 不存在（sp27 需先跑 gen_vtbl.py）" % base)
        sys.exit(1)
    src = open(base).read()
    print("%s 基线: %d bytes" % (VARIANT, len(src)))

    # 补丁 1: i2f
    if 'blx 0x232c8 存根' in src:
        n = len(re.findall(I2F_STUB, src))
        src = re.sub(I2F_STUB, I2F_REAL, src)
        print("补丁1 i2f 存根→真实: %d 处" % n)
    else:
        print("补丁1 i2f: 已修复（跳过）")

    # 补丁 2: 绝对归一化
    if '0x80000000u' not in src:
        if DISPATCH_OLD in src:
            src = src.replace(DISPATCH_OLD, DISPATCH_NEW, 1)
            print("补丁2 jqt_blx_dispatch 绝对归一化: 注入成功")
        else:
            print("补丁2 警告: jqt_blx_dispatch 块未找到（可能已改过）")
    else:
        print("补丁2 绝对归一化: 已存在（跳过）")

    # 补丁 3: L2400c 强制 glClear（仅 F_23eec 完整体有 L24008 行时）
    if 'L2400c 强制glClear' in src:
        print("补丁3 L2400c: 已存在（跳过）")
    elif L24008_OLD in src:
        src = src.replace(L24008_OLD, L24008_NEW, 1)
        print("补丁3 L2400c 强制 glClear: 注入成功")
    else:
        print("补丁3 L2400c: 未注入（%s 版 F_23eec 无 L24008 行 / 已是完整体）" % VARIANT)

    # 补丁 4: F_9d3d4 完整体（sp27 版；chain 版 F_9d3d4 保持 bx lr 存根）
    if VARIANT == 'sp27':
        if '§sp28-ROOT：手工填场景数据链字段' in src:
            print("补丁4 F_9d3d4 完整体+填字段: 已注入（跳过）")
        elif 'F_9d3d4 渲染器单例构造' in src:
            # 已注入旧版 F_9d3d4（无填字段），需升级到含填字段的版本
            src = src.replace('F9D3D4_BODY_PLACEHOLDER', F9D3D4_BODY)  # no-op safety
            # 实际：删除旧 F_9d3d4 定义 + 注入新模板
            import re as _re
            # 定位旧 F_9d3d4 定义行号
            lines = src.split('\n')
            old_idx = None
            for i, l in enumerate(lines):
                if l.startswith('uint32_t F_9d3d4(uint32_t _r0,'):
                    old_idx = i
                    break
            if old_idx is not None:
                # 找函数结束（depth 归 0）
                d = 0; end = old_idx
                for i in range(old_idx, min(old_idx+60, len(lines))):
                    d += lines[i].count('{') - lines[i].count('}')
                    if d == 0 and i > old_idx:
                        end = i
                        break
                del lines[old_idx:end+1]
                new_lines = F9D3D4_BODY.split('\n')
                # 在旧位置（原行号）插入——用原 F_9d988 之前
                ins_at = None
                for i, l in enumerate(lines):
                    if l.startswith('uint32_t F_ab3d4(uint32_t _r0,'):
                        ins_at = i
                        break
                if ins_at is not None:
                    lines[ins_at:ins_at] = new_lines
                else:
                    lines.extend(new_lines)
                src = '\n'.join(lines)
                print("补丁4 F_9d3d4 完整体+填字段: 升级成功")
            else:
                print("补丁4 警告: 旧 F_9d3d4 定义未找到")
        elif F9D3D4_STUB in src:
            src = src.replace(F9D3D4_STUB, F9D3D4_BODY, 1)
            print("补丁4 F_9d3d4 完整体+填字段: 注入成功")
        else:
            print("补丁4 警告: F_9d3d4 stub 未找到（可能已改过）")

    # 补丁 4.5: jq_board_px.h 像素数组 include（§sp28-ROOT 手工填场景数据链，仅 sp27）
    # 注意：必须放在补丁 4 之后（先确保 F_9d3d4 注入成功，再 include 像素头）
    # 判据用完整 #include 行（模板注释里含 "jq_board_px.h" 字样会误判）
    if VARIANT == 'sp27' and '#include "jq_board_px.h"' not in src:
        inc_line = '#include <android/log.h>'
        if inc_line in src:
            src = src.replace(inc_line, inc_line + '\n#include "jq_board_px.h"', 1)
            print("补丁4.5 jq_board_px.h include: 注入成功")
        else:
            print("补丁4.5 警告: #include <android/log.h> 未找到")

    # 补丁 4.6: F_a4e80 38 回跳环步数阀（§sp28-SCALE-9，对照全量场景闭包放大后 GLThread spin 根因）
    # 原版 F_a4e80 的 38 个回跳环（0xa4eb2/0xa4f06 链表环 + 0xa4f44 memcmp 环等）：
    #   场景对象图未填充（std::list 节点 next 指针 / memcmp 比较对象未构造）→ 自环
    #   （12264/13333 实证 WATCHDOG g_cur_fn=0x000a4e80 + 出帧=0，旧补丁只覆盖 2 环无效）。
    # 全 38 环回跳指令前插 static 计数阀：超 0x100 步 → r3=0 + goto La4f4e（函数中段安全出口），
    # 防 GLThread 卡死。守卫清单 = /tmp/opencode/la4e80_guards_38.json（tex_pipeline.py 机械化算出）。
    A4E80_GUARDS_38 = json.load(open('/tmp/opencode/la4e80_guards_38.json'))['guards'] if \
        os.path.exists('/tmp/opencode/la4e80_guards_38.json') else []
    if VARIANT == 'sp27':
        n_inj = 0
        for g38 in A4E80_GUARDS_38:
            tag = g38['guard_tag']
            gcode = g38['guard_code']
            if gcode + '\n' in src:
                continue  # 已注入
            # 锚点 = 回跳指令标签行（gen 翻译体里该标签所在行），在它前面插守卫行
            # 标签行格式：La4eb8: if(!Z) goto La4eb2; 或 La4f4c: goto La4ef4;（无条件）
            # 用 "L%s:" 前缀 + 行首匹配（只命中 F_a4e80 内的回跳标签）
            anchor_pat = re.compile(r'^La4e[0-9a-f]{2}: (if\(!?Z\) goto L[0-9a-f]+;|goto L[0-9a-f]+;)$', re.M)
            m_list = [m for m in anchor_pat.finditer(src) if m.group(0).startswith(tag + ':')]
            if not m_list:
                print("补丁4.6 警告: 锚点 %s 未找到（跳过）" % tag)
                continue
            pos = m_list[0].start()
            indent = ''
            nl = src.rfind('\n', 0, pos) + 1
            indent = src[nl:pos]
            ins = indent + gcode + '\n'
            src = src[:pos] + ins + src[pos:]
            n_inj += 1
        print("补丁4.6 F_a4e80 38 环步数阀: 注入 %d 个" % n_inj)

    # 去重（sp27 regen 的 F_b5eec 类）
    src, ndup = dedup_defs(src)

    # §sp28-SCALE-2 跨文件符号冲突去重（gen_vtbl.py real_fns 扩张后，jqt_vtbl.c 新增
    #   完整体可能与 jqt_On*_full.c / jqt_Render_full.c 既有同名 F_xxx 完整体重名
    #   → 链接 duplicate symbol。策略：保留 jqt_vtbl.c 版（gen 翻译体，最新），
    #   删其他文件里的同名 F_xxx 完整体（depth 归 0 定位函数尾）。
    import glob as _glob
    other_files = sorted(_glob.glob('rev64/reverse/src_jni_translated/jqt_*.c'))
    other_files = [f for f in other_files if f.split('/')[-1] not in ('jqt_vtbl.c', 'jqt_mem.c', 'jqt_jnibr.c')]
    vtbl_fns = set(re.findall(r'uint32_t F_([0-9a-f]+)\(uint32_t _r0,', src))
    removed_total = 0
    for of in other_files:
        oflines = open(of).read().split('\n')
        of_names = set(re.findall(r'uint32_t F_([0-9a-f]+)\(uint32_t _r0,', '\n'.join(oflines)))
        # §sp28-SCALE-3 链接安全闸门：仅删 jqt_vtbl.c 自身有【完整体】的同名函数。
        #   vtbl_fns 只收集完整体（_r0 命名参数签名）；jqt_vtbl.c 无完整体（存根/无定义）的同名
        #   不删对方文件定义（删了 = 悬空 UND → MuMu failed to link）。
        #   对方文件（jqt_On*_full.c）的同名也必须是完整体才删（声明行不算冲突）。
        of_bodies = set(re.findall(r'uint32_t F_([0-9a-f]+)\(uint32_t _r0,', '\n'.join(oflines)))
        conflict = vtbl_fns & of_bodies
        if not conflict:
            continue
        # 逐个删冲突函数完整体（从后往前删，防行号漂移）
        to_del = []
        for fn in sorted(conflict, key=lambda x: int(x,16)):
            for i, l in enumerate(oflines):
                if l.startswith('uint32_t F_%s(uint32_t _r0,' % fn):
                    d = 0; end = i
                    for j in range(i, min(i+4000, len(oflines))):
                        d += oflines[j].count('{') - oflines[j].count('}')
                        if d == 0 and j > i:
                            end = j
                            break
                    to_del.append((i, end, fn))
        for i, end, fn in sorted(to_del, reverse=True):
            del oflines[i:end+1]
            removed_total += 1
        open(of, 'w').write('\n'.join(oflines))
        print("跨文件去重 %s: 删 %d 个（%s）" % (of.split('/')[-1], len(to_del), ", ".join("F_0x"+fn for fn in sorted(conflict, key=lambda x: int(x,16)))))

    # brace 完整性检查
    depth = 0; neg = 0
    for l in src.split('\n'):
        depth += l.count('{') - l.count('}')
        if depth < 0:
            neg += 1; depth = 0
    if neg:
        print("brace 检查: 负深度 %d 处（需人工检查）" % neg)
        sys.exit(1)
    print("brace 检查: 全部通过")

    open(OUT, 'w').write(src)
    print("已写 %s (%d bytes)" % (OUT, len(src)))

    if DO_BUILD:
        print("=== 渲染链补丁（render_patch.py：Lbf51c 豁免，幂等）===")
        subprocess.run(['python3', 'rev64/tools/render_patch.py'], capture_output=True, text=True)
        print("=== 编译 ===")
        r = subprocess.run(['bash', 'rev64/build_arm32_aligned.sh'], capture_output=True, text=True)
        for l in r.stdout.splitlines():
            if 'error' in l or 'OK:' in l or 'JNI' in l:
                print(l)
        if r.returncode != 0:
            for l in (r.stdout + r.stderr).splitlines()[-10:]:
                print(l)
            sys.exit(1)
        print("=== 打包 + 安装 + 启动 ===")
        subprocess.run(['bash', 'rev64/pack_arm32.sh', 'rev64/dist/libjunqi_arm32_aligned.so'], capture_output=True, text=True)
        subprocess.run(['cp', '/tmp/opencode/phase32_aligned.apk.signed', 'rev64/dist/phase_chain_arm32.apk'], capture_output=True)
        adb = 'adb -s 10.0.182.146:5555'
        subprocess.run(['timeout','30',adb,'shell',"am force-stop com.cnvcs.junqi; logcat -c"], capture_output=True)
        subprocess.run(['timeout','30',adb,'push','/tmp/opencode/phase32_aligned.apk.signed','/data/local/tmp/p32sp.apk'], capture_output=True)
        subprocess.run(['timeout','30',adb,'shell','su 0 pm install -r /data/local/tmp/p32sp.apk'], capture_output=True, text=True)
        subprocess.run(['timeout','30',adb,'shell','am start -n com.cnvcs.junqi/com.cnvcs.App'], capture_output=True)
        print("=== 验证（一条命令，45s 后跑）===")
        print("  adb -s 10.0.182.146:5555 shell 'screencap -p /data/local/tmp/sp.png' && \\")
        print("    adb -s 10.0.182.146:5555 pull /data/local/tmp/sp.png /tmp/opencode/mumu_verify/sp.png && \\")
        print("    python3 rev64/tools/shot_check.py classify /tmp/opencode/mumu_verify/sp.png")
        print("  adb -s 10.0.182.146:5555 logcat -d | grep -E \'SIGSEGV|Fatal|F_9d3d4 渲染器单例构造|Lbf5c2 dispatch\'")

if __name__ == '__main__':
    main()
