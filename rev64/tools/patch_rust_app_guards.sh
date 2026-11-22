#!/bin/bash
# patch_rust_app_guards.sh — Rust 正向 app（com.qwsaeda）Java 层防御补丁（幂等）
#
# 修复两处 happy-path 测不到、真实使用必中招的缺陷：
#   1. GLView.surfaceDestroyed 空指针崩溃（P0）：setRenderer 已移除 → mGLThread 永不创建，
#      surfaceCreated/Changed 有 if-eqz 守卫，唯独 surfaceDestroyed 漏了 → 切后台/锁屏时 NPE。
#      修：补 if-eqz v0, :cond_no_thread_destroyed（与另两个回调一致）。
#   2. manifest predictive back 致返回键失效（P1）：enableOnBackInvokedCallback=true +
#      targetSdk 35 → Android 13+ 不把 BACK 派发到 onKeyDown → native OnKey 退出确认链失效。
#      修：删除 enableOnBackInvokedCallback 属性（恢复 KEYCODE_BACK 派发）。
#
# 用法：bash rev64/tools/patch_rust_app_guards.sh [dec树路径]
# 幂等：已修过的树跳过。rebuild_rust_apk.sh 打包前调用即可。
set -u
DECRS=/tmp/opencode/jq_apk/dec_rs
TREE=${1:-$DECRS}

GLVIEW="$TREE/smali/com/qwsaeda/GLView.smali"
MANIFEST="$TREE/AndroidManifest.xml"

# 1. GLView.surfaceDestroyed 空守卫
if [ -f "$GLVIEW" ]; then
  if grep -q "cond_no_thread_destroyed" "$GLVIEW"; then
    echo "  ✓ $GLVIEW surfaceDestroyed 已有空守卫（跳过）"
  else
    python3 - "$GLVIEW" << 'PYEOF'
import sys
f = sys.argv[1]; s = open(f).read()
old = '''    .line 113
    iget-object v0, p0, Lcom/qwsaeda/GLView;->mGLThread:Lcom/qwsaeda/GLView$GLThread;

    invoke-virtual {v0}, Lcom/qwsaeda/GLView$GLThread;->surfaceDestroyed()V

    .line 114
    return-void'''
new = '''    .line 113
    iget-object v0, p0, Lcom/qwsaeda/GLView;->mGLThread:Lcom/qwsaeda/GLView$GLThread;

    if-eqz v0, :cond_no_thread_destroyed
    invoke-virtual {v0}, Lcom/qwsaeda/GLView$GLThread;->surfaceDestroyed()V

    :cond_no_thread_destroyed
    .line 114
    return-void'''
assert old in s, "surfaceDestroyed 锚点未找到（GLView.smali 结构漂移？）"
open(f, 'w').write(s.replace(old, new, 1))
print("  ✓ surfaceDestroyed 空守卫已补")
PYEOF
  fi
else
  echo "  ✗ $GLVIEW 不存在"
fi

# 2. manifest 删除 enableOnBackInvokedCallback
if [ -f "$MANIFEST" ]; then
  if grep -q "enableOnBackInvokedCallback" "$MANIFEST"; then
    python3 - "$MANIFEST" << 'PYEOF'
import sys, re
f = sys.argv[1]; s = open(f).read()
s2 = re.sub(r'\s*android:enableOnBackInvokedCallback="true"', '', s)
assert s2 != s, "enableOnBackInvokedCallback 删除失败"
open(f, 'w').write(s2)
print("  ✓ enableOnBackInvokedCallback 已删除（恢复 BACK 派发）")
PYEOF
  else
    echo "  ✓ $MANIFEST 无 predictive back（跳过）"
  fi
else
  echo "  ✗ $MANIFEST 不存在"
fi

echo "✓ patch_rust_app_guards 完成"
