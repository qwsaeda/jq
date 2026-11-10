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
