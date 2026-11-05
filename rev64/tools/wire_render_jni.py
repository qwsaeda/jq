#!/usr/bin/env python3
"""把 P0 A 组 12 条 so→Java 调用接进渲染链 F_24108（渲染入口，GLThread 内执行）。

接线原理：
- F_24108 是 OnGraphCreate→F_2349d→F_bd764→F_b6790 渲染入口链的一环（vtable 调度器）
- 原版 F_24108 内部用 JNI 调 GLView/View 布局方法（setEGLConfigChooser/getHolder/
  setVisibility/requestLayout 等 A 组 12 条）——但机械翻译体里这些是 bl 存根 r0=0
- 本脚本在 F_24108 的函数体末尾（RET 前）插一段"接线块"：
  遍历 A 组 12 条 jqt_render_xxx 包装函数，对"可安全调用"的（receiver 非 0 的实例方法）
  真调一次，验证 so→Java 通路在渲染链里可达。

安全边界（诚实）：
- GLView 实例 receiver 从 F_9d3d4 单例 + g_activity 取（主线程 OnCreate 设的全局）
- 静态/构造器 receiver 可 0
- 只调"纯查询/布局"方法（setEGLConfigChooser/getHolder/requestLayout），
  不调有副作用的（addContentView 会改 window，newTextDlg 会弹窗）
- 插桩在 RET 前（不影响原 F_24108 返回值），异常被 jqt_jnibr 的 ExceptionClear 兜住

不重生成 jqt_vtbl.c（§124z 红线：避免覆盖 wire_blx_dispatch）。
改的是 jni_aligned.c 的 F_24108 适配 override（在 F_24108 定义文件 jqt_vtbl.c 的
函数体外加 wrapper，或直接在 F_24108 末尾插代码）。
"""
import re

# 读 jqt_vtbl.c 找 F_24108 定义
P = '/disk/workspaces/jq/rev64/src_jni_translated/jqt_vtbl.c'
s = open(P).read()

# 找 F_24108 函数体（从 uint32_t F_24108( 到匹配的 })
m = re.search(r'uint32_t F_24108\(uint32_t _r0.*?\n\}', s, re.S)
if not m:
    print("[FAIL] 未找到 F_24108 函数体")
    raise SystemExit(1)

body = m.group(0)
# 在 RET: return r0; 前插接线块（F_24108 末尾的 RET 标签）
# 找 "RET: return r0;" 或 "L2412e_r: goto RET;"
ret_line = "RET: return r0;"
if ret_line not in body:
    print("[FAIL] F_24108 体里没找到 RET: return r0;")
    print(body[-400:])
    raise SystemExit(1)

# 接线块：调 A 组 12 条（receiver 用 g_activity/GLView 单例）
# GLView 单例：OnCreate 时 jq_jni_make_glview 返回的 view（jni_aligned.c 全局）
# 用 extern 引 g_activity（jni_aligned.c 的 static，需要暴露）
wiring = '''
    /* §124aj 渲染链 so→Java 接线：A 组 12 条 View 布局调用在 F_24108（渲染入口）
     *   末尾插桩验证通路。receiver：
     *   - GLView 实例方法（setEGLConfigChooser/setEGLContextClientVersion/getHolder）
     *     用 glview 单例（extern g_glview，jni_aligned.c 暴露）
     *   - View 实例方法（getParent/visibility/requestLayout/setMargins）用 glview
     *   - 静态/构造器 receiver 可 0
     * 只调纯查询（setEGLConfigChooser/setEGLContextClientVersion/getHolder/requestLayout），
     *   不调 addContentView/newTextDlg（副作用大，留对局链验）。 */
    extern uint32_t g_glview_handle;
    uint32_t _gv = g_glview_handle;
    if (_gv) {
        /* GLView 实例 3 条（receiver=glview） */
        (void)jqt_render_setEGLConfigChooser(_gv, 0);   /* id9 */
        (void)jqt_render_setEGLContextClientVersion(_gv, 0); /* id10 */
        (void)jqt_render_getHolder(_gv);                /* id11 */
        /* View 实例 3 条（receiver=glview，GLView 是 View） */
        (void)jqt_render_requestLayout(_gv);            /* id5 */
        (void)jqt_render_getParent(_gv);                /* id0 */
        (void)jqt_render_getLayoutParams(_gv);          /* id3 */
    }
'''

# jqt_render_xxx 原型（jqt_vtbl.c 没 include jqt_render_jni.c，要 extern）
protos = """
extern uint32_t jqt_render_setEGLConfigChooser(uint32_t, uint32_t);
extern uint32_t jqt_render_setEGLContextClientVersion(uint32_t, uint32_t);
extern uint32_t jqt_render_getHolder(uint32_t);
extern uint32_t jqt_render_requestLayout(uint32_t);
extern uint32_t jqt_render_getParent(uint32_t);
extern uint32_t jqt_render_getLayoutParams(uint32_t);
"""

# 把 extern + 接线块插到 F_24108 函数体开头（g_cur_fn 之后），接线调用插到 RET 前
# 1. extern 插到 F_24108 的 g_cur_fn 行后
anchor = "g_cur_fn = 0x24108;"
assert anchor in body, "F_24108 里没找到 g_cur_fn = 0x24108;"
body2 = body.replace(anchor, anchor + "\n" + protos, 1)
# 2. 接线调用插到 RET 前
body2 = body2.replace(ret_line, wiring + ret_line, 1)

s = s.replace(body, body2, 1)
open(P, 'w').write(s)
print("OK: F_24108 已插 A 组 6 条（GLView3+View3）so→Java 接线")
print("   （addContentView/newLinearLayout/newLayoutParams/setMargins/setVisibility 6 条副作用大，留对局链）")
