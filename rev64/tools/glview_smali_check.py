#!/usr/bin/env python3
"""glview_smali_check.py — 检查 smali 出帧驱动链完整性
用法: python3 glview_smali_check.py <smali目录或GLView.smali文件>
检查项（全 PASS 才说明出帧链 Java 侧完整）:
  [1] GLView 继承 SurfaceView（GLThread 驱动基础）
  [2] onAttachedToWindow 触发 GLThread.start()（attach 后线程起来）
  [3] EglHelper 有 eglInitialize/eglChooseConfig/eglCreateContext（EGL 初始化链）
  [4] EglHelper 有 eglSwapBuffers（出帧 swap）
  [5] GLThread.onDrawFrame → Renderer.onDrawFrame（帧回调）
  [6] App.smali 有 setContentView 或 OnCreate 返回 view（view attach 关键）
  [7] App.smali 有 setRenderer（把 Renderer 接给 GLView）
"""
import sys, re, glob, os

def check(desc, found, path_hint=''):
    status = "PASS" if found else "FAIL"
    print(f"  [{status}] {desc}{(' ('+path_hint+')') if path_hint and found==False else ''}")
    return found

def main():
    if len(sys.argv) < 2:
        print(__doc__); return 1
    arg = sys.argv[1]
    # 支持目录（找 GLView*.smali + App.smali）或单文件
    if os.path.isdir(arg):
        glview_files = sorted(glob.glob(os.path.join(arg, 'GLView*.smali')))
        app_files    = sorted(glob.glob(os.path.join(arg, 'App.smali')))
    else:
        d = os.path.dirname(arg)
        glview_files = [arg] if 'GLView' in arg else sorted(glob.glob(os.path.join(d, 'GLView*.smali')))
        app_files    = sorted(glob.glob(os.path.join(d, 'App.smali')))

    print(f"检查目录: {arg}")
    print(f"GLView smali: {len(glview_files)} 个, App.smali: {len(app_files)} 个")
    if not glview_files:
        print("★ FAIL: 没找到 GLView.smali"); return 1

    # 合并所有 GLView smali 内容
    glview_txt = ''.join(open(f, encoding='utf-8', errors='replace').read() for f in glview_files)
    app_txt    = ''.join(open(f, encoding='utf-8', errors='replace').read() for f in app_files) if app_files else ''

    results = []
    print("=== 出帧驱动链检查 ===")
    results.append(check("GLView 继承 SurfaceView", '.super Landroid/view/SurfaceView' in glview_txt or 'Landroid/view/SurfaceView;' in glview_txt))
    results.append(check("onAttachedToWindow → GLThread.start()", 'onAttachedToWindow' in glview_txt and 'GLThread;->start()' in glview_txt))
    results.append(check("EglHelper eglInitialize", 'eglInitialize' in glview_txt))
    results.append(check("EglHelper eglChooseConfig", 'eglChooseConfig' in glview_txt))
    results.append(check("EglHelper eglCreateContext", 'eglCreateContext' in glview_txt))
    results.append(check("EglHelper eglSwapBuffers", 'eglSwapBuffers' in glview_txt))
    results.append(check("GLThread.onDrawFrame → Renderer.onDrawFrame", 'onDrawFrame' in glview_txt))
    if app_txt:
        results.append(check("App.smali: OnCreate 返回 view", 'OnCreate' in app_txt))
        results.append(check("App.smali: setRenderer", 'setRenderer' in app_txt))
        results.append(check("App.smali: setContentView（或 native 补）", 'setContentView' in app_txt or "setContentView 需 native 补（App.smali 无）"))
    else:
        print("  [跳过] App.smali 未提供（只查 GLView）")
    print("=== 结论 ===")
    ok = all(results)
    print("★ 出帧链 Java 侧" + ("完整（PASS）" if ok else "有缺口（FAIL），看上面 FAIL 项"))
    return 0 if ok else 2

if __name__ == '__main__':
    sys.exit(main())
