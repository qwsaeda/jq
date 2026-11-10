#!/bin/bash
# patch_hp_guard.sh — 给 App.smali 加 hp 翻译桥库缺失容错（64 位 so dlopen 失败不崩进程）
# 背景：MuMu x86 宿主跑 arm64 so 需 hp 桥（libhp12_x86_64.so）。hp 库缺失时
#   System.loadLibrary 抛 UnsatisfiedLinkError，App.smali <clinit> 无 catch → 崩进程。
# 修法：
#   1. <clinit> loadLibrary 段加 try/catch → 置 s_native_ok 标志（0=失败 1=成功）
#   2. onCreate 段查 s_native_ok=0 → Log.e + Toast 标注 + return（不 setContentView，不崩）
#   3. 字段 s_native_err + s_native_ok
# 用法：bash rev64/tools/patch_hp_guard.sh [dec树路径]
# 幂等：已含 s_native_ok 的树跳过。

set -u
RA=/tmp/opencode/jq_apk/rs_a64
for tree in "$RA/dec_rs" "$RA/dec_dual" "${1:-}"; do
  [ -z "$tree" ] && continue
  f="$tree/smali/com/cnvcs/App.smali"
  [ -f "$f" ] || { echo "  ✗ $f 不存在"; continue; }
  if grep -q "s_native_ok" "$f"; then
    echo "  ✓ $tree App.smali 已含 hp 容错（跳过）"
    continue
  fi
  python3 - "$f" << 'PYEOF'
import sys
f = sys.argv[1]
s = open(f).read()

# 1. <clinit> loadLibrary 段加 try/catch + s_native_ok 标志
old_clinit = '''.method static constructor <clinit>()V
    .locals 1

    .prologue
    .line 20
    const-string v0, "junqi"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V

    .line 21
    return-void
.end method'''
new_clinit = '''.method static constructor <clinit>()V
    .locals 2

    .prologue
    .line 20
    const-string v0, "junqi"

    :try_start
    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
    const/4 v0, 0x1
    sput v0, Lcom/cnvcs/App;->s_native_ok:I
    :try_end
    .catch Ljava/lang/UnsatisfiedLinkError; {:try_start .. :try_end} :cond_hp_dlg

    .line 21
    return-void

    :cond_hp_dlg
    move-exception v1
    # ★ hp 翻译桥库缺失（MuMu x86 宿主跑 arm64 so）→ dlopen 失败不崩：s_native_ok=0，
    #   onCreate 检测后 Log.e + Toast 标注（不 setContentView GLView）。
    sput-object v1, Lcom/cnvcs/App;->s_native_err:Ljava/lang/UnsatisfiedLinkError;
    const/4 v0, 0x0
    sput v0, Lcom/cnvcs/App;->s_native_ok:I
    return-void
.end method'''
if s.count(old_clinit) == 1:
    s = s.replace(old_clinit, new_clinit)
    print(f"  ✓ {f.split('/')[-3]} <clinit> 加 try/catch")
else:
    print(f"  ⚠ {f.split('/')[-3]} <clinit> 未匹配（已改？跳过）")

# 2. 字段声明（加在 m_handler 字段后）
old_field = '.field protected m_handler:Landroid/os/Handler;'
new_field = '''.field protected m_handler:Landroid/os/Handler;

# ★ hp 翻译桥库缺失容错标志（loadLibrary 失败时 s_native_ok=0，onCreate 弹边界标注）
.field private static s_native_err:Ljava/lang/UnsatisfiedLinkError;

.field private static s_native_ok:I'''
if s.count(old_field) == 1 and 's_native_ok' not in s:
    s = s.replace(old_field, new_field)
    print(f"  ✓ {f.split('/')[-3]} 字段声明")

# 3. onCreate 段 s_native_ok 守卫（native 不可用 → Toast + Log.e + return）
old_onc = '''    .line 46
    invoke-static {p0}, Lcom/cnvcs/App;->OnCreate(Landroid/app/Activity;)Lcom/cnvcs/GLView;

    move-result-object v0'''
new_onc = '''    .line 46
    # ★ hp 翻译桥库缺失容错：s_native_ok=0（loadLibrary 失败）→ Log.e + Toast，不崩
    sget v1, Lcom/cnvcs/App;->s_native_ok:I

    if-nez v1, :cond_native_ok

    sget-object v1, Lcom/cnvcs/App;->s_native_err:Ljava/lang/UnsatisfiedLinkError;

    const-string v0, "junqi_rs"
    const-string v2, "native so 加载失败（hp 翻译桥库缺失或设备不支持），无法渲染对局，请重启模拟器"
    invoke-static {v0, v2}, Landroid/util/Log;->e(Ljava/lang/String;Ljava/lang/String;)I

    new-instance v1, Landroid/widget/Toast;
    invoke-direct {v1, p0}, Landroid/widget/Toast;-><init>(Landroid/content/Context;)V
    invoke-virtual {v1, v2}, Landroid/widget/Toast;->setText(Ljava/lang/CharSequence;)V
    invoke-virtual {v1}, Landroid/widget/Toast;->show()V

    return-void

    :cond_native_ok
    invoke-static {p0}, Lcom/cnvcs/App;->OnCreate(Landroid/app/Activity;)Lcom/cnvcs/GLView;

    move-result-object v0'''
if s.count(old_onc) == 1:
    s = s.replace(old_onc, new_onc)
    # .locals 2 → 3（Toast 段用 v2）
    s = s.replace('.method protected onCreate(Landroid/os/Bundle;)V\n    .locals 2',
                  '.method protected onCreate(Landroid/os/Bundle;)V\n    .locals 3')
    print(f"  ✓ {f.split('/')[-3]} onCreate 守卫 + .locals 3")
else:
    print(f"  ⚠ {f.split('/')[-3]} onCreate 段未匹配")

open(f, 'w').write(s)
PYEOF
  echo "  ✓ $tree App.smali hp 容错已打补丁"
done
echo "✓ patch_hp_guard 完成"

# ★ 2026-11-10 追加：dec_full 树 App.smali 损坏行修复（move-result 行尾粘连 :skip_native
#   + 6 方法缺 :skip_native 标签定义 → smali 编译 "label hasn't been placed yet" →
#   dex 头全 0 → MuMu dexopt Invalid or truncated dex file FATAL）。
#   幂等：仅当检测到损坏行/缺标签时修。
for tree in "$RA/dec_full"; do
  f="$tree/smali/com/cnvcs/App.smali"
  [ -f "$f" ] || continue
  python3 - "$f" << 'PYEOF'
import sys
f = sys.argv[1]
lines = open(f).readlines()
out = []
fixed_merge = 0
for l in lines:
    s = l.rstrip('\n')
    if s == '    move-result-object v0:skip_native':
        s = '    move-result-object v0'
        fixed_merge += 1
    elif s == '    move-result v0:skip_native':
        s = '    move-result v0'
        fixed_merge += 1
    out.append(s + '\n')
# 插标签：if-eqz v0, :skip_native 所在方法内无 :skip_native 定义 → 行后插
final = []
i = 0
fixed_label = 0
while i < len(out):
    final.append(out[i])
    if out[i].strip() == 'if-eqz v0, :skip_native':
        j = i + 1
        has_label = False
        while j < len(out) and not out[j].startswith('.end method'):
            if out[j].strip().startswith(':skip_native'):
                has_label = True
                break
            j += 1
        if not has_label:
            final.insert(len(final), ':skip_native\n')
            fixed_label += 1
    i += 1
open(f, 'w').writelines(final)
print(f'  ✓ {f}: 粘连行修 {fixed_merge} + 缺标签插 {fixed_label}')
PYEOF
done

# ============================================================================
# ★ 2026-11-10 用户指正"MuMu 玩不了"修复固化：顶部系统"Viewing full screen / GOT IT"
#   提示栏压在 app 画面之上拦截主菜单触摸 → 进不了对局。
#   修复：App.smali onCreate 在 setSystemUiVisibility(0x1706) 后追加
#   API30+ WindowInsetsController.hide(statusBars|navigationBars=0x3) 主动 dismiss。
#   幂等：已含 getInsetsController 则跳过。
# ============================================================================
patch_immersive() {
  local f="$1"
  if grep -q "getInsetsController" "$f" 2>/dev/null; then
    echo "  ✓ $f: immersive 段已在（跳过）"
    return 0
  fi
  python3 - "$f" << 'PYEOF'
import sys
f = sys.argv[1]
s = open(f).read()
anchor = '    invoke-virtual {v0, v1}, Landroid/view/View;->setSystemUiVisibility(I)V\n'
assert s.count(anchor) == 1, f"anchor count={s.count(anchor)}"
immersive = '''    invoke-virtual {v0, v1}, Landroid/view/View;->setSystemUiVisibility(I)V

    # ★ 2026-11-10 用户指正"MuMu 玩不了"修复：MuMu 顶部系统"Viewing full screen / GOT IT"
    # 提示栏压在 app 画面之上拦截触摸（用户进不了对局）。API 30+ 主动
    # WindowInsetsController.hide(statusBars|navigationBars) dismiss 系统全屏提示栏；
    # API <30 维持 setSystemUiVisibility 旧路径（行为不变）。
    # ★ 教训固化：MuMu framework.jar!classes3.dex 里 WindowInsetsController 是 interface
    #   （非 class）→ hide 调用必须 invoke-interface（invoke-virtual 触发 IncompatibleClassChangeError）。
    # API 30 门槛（const/16：0x1e 超 const/4 的 4bit 常量域）
    const/16 v2, 0x1e
    sget v3, Landroid/os/Build$VERSION;->SDK_INT:I
    if-lt v3, v2, :cond_immersive_skip
    # API 30+：window.insetsController().hide(statusBars | navigationBars = 0x3)
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v0
    invoke-virtual {v0}, Landroid/view/Window;->getInsetsController()Landroid/view/WindowInsetsController;
    move-result-object v0
    if-eqz v0, :cond_immersive_skip
    # WindowInsets.Type statusBars=0x1 | navigationBars=0x2 = 0x3
    const/4 v1, 0x3
    invoke-interface {v0, v1}, Landroid/view/WindowInsetsController;->hide(I)V
    :cond_immersive_skip
'''
s = s.replace(anchor, immersive, 1)
# .locals 扩展（v2/v3 需 .locals >= 4）
import re
m = re.search(r'\.method protected onCreate\(Landroid/os/Bundle;\)V\n    \.locals (\d+)', s)
if m:
    n = int(m.group(1))
    if n < 4:
        s = s[:m.start(1)] + '4' + s[m.end(1):]
open(f, 'w').write(s)
print(f"  ✓ {f}: immersive 段已插入（onCreate + .locals 4）")
PYEOF
}

# 对所有存在的 smali 树应用 immersive 补丁（与 hp guard 同款幂等循环）
for tree in /tmp/opencode/jq_apk/rs_a64/dec_rs/smali/com/cnvcs/App.smali; do
  [ -f "$tree" ] && patch_immersive "$tree"
done
