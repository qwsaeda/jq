#!/usr/bin/env python3
"""src_rs_check.py — src_rs 源码重写路线防回归审计
用法: python3 src_rs_check.py
检查 rev64/src_rs/ 下所有已知红线（本轮踩坑沉淀），一条命令出 PASS/FAIL：
  [1] Makefile 链接行含 -lEGL -lGLESv1_CM -lGLESv2 -landroid
  [2] glview_gl.c 用 PBUFFER_BIT config（WINDOW_BIT 配 PBuffer → 3002）
  [3] glview_gl.c 不预 eglMakeCurrent（跨线程 makeCurrent → 3002）
  [4] render.c rs_render_init 不自建 EGL（复用 disp/surf/ctx，双套 → 3002）
  [5] render.c 调 glVertexAttribPointer（GLES2 几何出帧核心，GL2 context 下安全）
  [5b] render.c 含 GLSL shader 编译链
  [5c] glview_gl.c 建 GL2 context（EGL_CONTEXT_CLIENT_VERSION=2）
  [5d] glview_gl.c 有独立渲染线程（绕开 Java GLThread GL1 EglHelper 冲突）
  [6] jni_bridge.c OnCreate 有 setContentView（GLThread 出帧链启动关键）
  [7] jni_bridge.c OnUpdate 有 SIGSEGV setjmp/siglongjmp 兜底
  [8] jni_bridge.c 含 signal.h
  [9] so_link_check.py 链接审计 PASS（UND 符号 MuMu 可解析）
"""
import os, sys, re, subprocess

RS = '/disk/workspaces/jq/rev64/src_rs'
TOOLS = '/disk/workspaces/jq/rev64/tools'
results = []

def check(name, ok, hint=''):
    results.append(ok)
    mark = "PASS" if ok else "FAIL"
    print(f"  [{mark}] {name}" + (f" —— {hint}" if hint and not ok else ""))

def read(p):
    fp = os.path.join(RS, p)
    if not os.path.exists(fp):
        return None
    with open(fp, encoding='utf-8', errors='replace') as f:
        return f.read()

print("=== src_rs 防回归审计 ===")

# [1] Makefile 链接行
mk = read('Makefile') or ''
ok1 = all(x in mk for x in ['-lEGL', '-lGLESv1_CM', '-lGLESv2', '-landroid'])
check("[1] Makefile 链接行 -lEGL -lGLESv1_CM -lGLESv2 -landroid", ok1, "缺库 → MuMu dlopen failed to link")

# [2] glview_gl.c PBUFFER_BIT
glview = read('glview_gl.c') or ''
ok2 = 'EGL_PBUFFER_BIT' in glview
check("[2] glview_gl.c 用 PBUFFER_BIT config", ok2, "WINDOW_BIT 配 PBuffer → 3002 EGL_BAD_ACCESS")

# [3] glview_gl.c jq_glview_init 不预 eglMakeCurrent
m = re.search(r'int jq_glview_init.*?\n\}', glview, re.DOTALL)
init_body = m.group(0) if m else glview
ok3 = 'eglMakeCurrent' not in init_body
check("[3] glview_gl.c jq_glview_init 不预 eglMakeCurrent", ok3, "主线程 makeCurrent → GLThread 再 make → 3002")

# [4] render.c rs_render_init 不自建 EGL（复用传入 disp/surf/ctx）
render = read('render.c') or ''
m4 = re.search(r'int rs_render_init\([^)]*\)', render)
sig4 = m4.group(0) if m4 else ''
ok4 = 'EGLDisplay' in sig4 or 'disp' in sig4.lower()
check("[4] render.c rs_render_init 接受传入的 EGL 上下文（不自建）", ok4, "双套 EGL → Mesa 3002")

# [5] render.c 必须调 glVertexAttribPointer（GLES2 + GLSL + VBO 几何出帧核心；
#    MuMu 64 位 GL2 context（EGL_CONTEXT_CLIENT_VERSION=2）实证出几何像素，
#    崩溃根因 = GL1 context 上跑 GL2 调用（Java EglHelper 默认 GL1），
#    修法 = 渲染线程自建 PBuffer GL2 context + 首帧 makeCurrent，GL2 调用在 GL2 context 上跑）
ok5 = any(
    'glVertexAttribPointer(' in l
    and not l.lstrip().startswith(('*', '//', '/*'))
    for l in render.splitlines()
)
check("[5] render.c 调 glVertexAttribPointer（GLES2 几何出帧核心）", ok5,
      "GLES2 几何绘制必需；缺失则退回清屏色不出帧")

# [5b] render.c 必须含 GLSL shader 编译链
ok5b = all(x in render for x in ['glCreateShader', 'glCompileShader', 'glLinkProgram', 'glUseProgram'])
check("[5b] render.c 含 GLSL shader 编译链（glCreateShader/glLinkProgram/glUseProgram）", ok5b,
      "GLES2 走 GLSL 管线，无 shader 则几何 no-op")

# [5c] glview_gl.c 必须建 GL2 context（EGL_CONTEXT_CLIENT_VERSION=2）
ok5c = 'EGL_CONTEXT_CLIENT_VERSION' in glview and 'ca[] = { EGL_CONTEXT_CLIENT_VERSION, 2' in glview
check("[5c] glview_gl.c 建 GL2 context（EGL_CONTEXT_CLIENT_VERSION=2）", ok5c,
      "GL2 context 是 GLES2 几何出帧前提（GL1 context 上 GL2 调用 SIGSEGV）")

# [5d] glview_gl.c 必须有独立渲染线程（pthread，绕开 Java GLThread GL1 EglHelper）
ok5d = 'pthread_create' in glview and 'rs_render_thread_main' in glview
check("[5d] glview_gl.c 有独立渲染线程（pthread，绕开 Java GLThread GL1 EglHelper）", ok5d,
      "Java EglHelper 建 GL1 context，驱动 GL2 会崩；native 独立线程独占 PBuffer GL2 context")

# [6] jni_bridge.c OnCreate 有 setContentView
jni = read('jni_bridge.c') or ''
ok6 = 'setContentView' in jni
check("[6] jni_bridge.c 有 setContentView(GLView)", ok6, "App.smali 无 setContentView → GLThread 永不启动")

# [7] jni_bridge.c OnUpdate 有 SIGSEGV 兜底
m7 = re.search(r'Java_com_cnvcs_App_OnUpdate.*?\n\}', jni, re.DOTALL)
upd = m7.group(0) if m7 else ''
ok7 = ('setjmp' in jni or 'siglongjmp' in jni) and 'g_gl_crash' in jni
check("[7] jni_bridge.c 有 SIGSEGV setjmp/siglongjmp 兜底", ok7, "GL 渲染 SIGSEGV 未兜底 → 闪退")

# [8] jni_bridge.c 含 signal.h
ok8 = '#include <signal.h>' in jni
check("[8] jni_bridge.c 含 #include <signal.h>", ok8, "sigaction/siglongjmp 需要")

# [9] 编译产物 + 链接审计
so = os.path.join(RS, 'dist', 'libjunqi_rs.so')
if os.path.exists(so):
    check("[9a] dist/libjunqi_rs.so 编译产物存在", True)
    r = subprocess.run(['python3', os.path.join(TOOLS, 'so_link_check.py'), so],
                      capture_output=True, text=True)
    ok9 = r.returncode == 0
    last = r.stdout.strip().splitlines()[-1] if r.stdout.strip() else ''
    check("[9b] so_link_check.py 链接审计 PASS", ok9, last)
else:
    check("[9] dist/libjunqi_rs.so 存在", False, "先跑 make so")

# [10] jni_method_audit.py：GetMethodID 异常清理（坑 2 防复发）
# 扫 jni_bridge.c 所有 GetMethodID 调用，检查 8 行内有无 ExceptionCheck/ExceptionClear
r = subprocess.run(['python3', os.path.join(TOOLS, 'jni_method_audit.py'),
                    os.path.join(RS, 'jni_bridge.c')],
                   capture_output=True, text=True)
ok10 = r.returncode == 0
last = r.stdout.strip().splitlines()[-1] if r.stdout.strip() else ''
check("[10] jni_method_audit.py GetMethodID 异常清理", ok10, last)

# [11] 对抗审计 #1 防复发：OnEvent 指针截断（(char)(*env)->GetStringUTFChars 截断指针）
# 旧版 `char *n = (char)(*env)->GetStringUTFChars(...)` 把指针截断到 1 字节 → 垃圾指针。
# 扫描 jni_bridge.c，该模式必须 0 处（修复后为 const char* 赋值）。
jni_src = open(os.path.join(RS, 'jni_bridge.c')).read()
trunc = re.findall(r'\(char\)\s*\(\s*\*env\s*\)\s*->\s*GetStringUTFChars', jni_src)
check("[11] 无 GetStringUTFChars 指针截断", len(trunc) == 0,
      f"发现 {len(trunc)} 处 (char) 截断（对抗审计 #1，应 0 处）")

print("=== 结论 ===")
if all(results):
    print("★ src_rs 全部红线 PASS（无回归）")
    sys.exit(0)
else:
    bad = [i for i, r in enumerate(results) if not r]
    print(f"★ src_rs 有 {len(bad)} 项 FAIL（结果序号 {bad}）")
    sys.exit(2)
