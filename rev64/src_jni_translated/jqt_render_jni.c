/* jqt_render_jni.c — P0 so→Java 接线适配层
 *
 * 把渲染链里的 so→Java 调用（43 条表中的 A/C/E/G 组）接到 jqt_jni_call。
 * 当前 43 条表 + 桥已编译链入（build_arm32_aligned.sh），OnCreate 已走桥（jq_jni_make_glview 替代）。
 * 本文件补渲染链 / 对局链的 JNI 边界：
 *   - 在 F_b6790（OnRender 入口）的 vtable 调度点改调 jqt_jni_call
 *   - 在 OnEvent（对局事件）里改调 SiteView.create / TextDlg.<init>
 *
 * 不重生成 jqt_vtbl.c（避免覆盖 wire_blx_dispatch，§124z 红线）。
 * 接线方式：覆盖 jqt_jni_call 的调用点（适配层函数 + 在 F_xxx 的 JNI 边界 override）。
 */
#include <stdint.h>
#include <string.h>
#include "jqt_jnibr.h"
#include "jqt_jni_calls.h"

/* extern jqt_jni_call 已在 jqt_jnibr.h 声明 */

/* ★ 接线点 1：渲染链 View 布局（A 组 9 条）
 *   原版渲染器在 surface 创建/尺寸变化时调 View 布局方法（getParent/removeView/
 *   setVisibility/getLayoutParams/setMargins/requestLayout/addContentView/setEGLConfigChooser/
 *   getHolder）。这些在 jqt_vtbl.c 的 F_24108/F_23eec 里是 bl 存根 r0=0。
 *   这里提供包装函数供 F_xxx override 调用。receiver=this（渲染对象句柄），
 *   args 按签名填。 */

/* 调用约定：this = 渲染对象（jobject 句柄，从 jqt_vtbl 的 r0 来）
 *   A 组 9 条的 jni_id（对应 jqt_jni_calls.json）：
 *   0 getParent / 1 removeView / 2 setVisibility / 3 getLayoutParams
 *   4 setMargins / 5 requestLayout / 6 LinearLayout.<init> / 7 addContentView
 *   8 LayoutParams.<init> / 9 setEGLConfigChooser / 10 setEGLContextClientVersion / 11 getHolder */

/* requestLayout()V：receiver=this，无参 */
uint32_t jqt_render_requestLayout(uint32_t this_handle){
    uint32_t a[1] = { this_handle };
    return jqt_jni_call(5, 1, a);   /* id5 requestLayout */
}

/* setVisibility(int)V：receiver=this，1 个 int 参 */
uint32_t jqt_render_setVisibility(uint32_t this_handle, int32_t vis){
    uint32_t a[2] = { this_handle, (uint32_t)vis };
    return jqt_jni_call(2, 2, a);   /* id2 setVisibility */
}

/* getLayoutParams()Landroid/view/ViewGroup$LayoutParams; receiver=this，无参 */
uint32_t jqt_render_getLayoutParams(uint32_t this_handle){
    uint32_t a[1] = { this_handle };
    return jqt_jni_call(3, 1, a);   /* id3 getLayoutParams */
}

/* setMargins(int,int,int,int)V：receiver=lp 句柄，4 个 int 参 */
uint32_t jqt_render_setMargins(uint32_t lp_handle, int32_t l, int32_t t, int32_t r, int32_t b){
    uint32_t a[5] = { lp_handle, (uint32_t)l, (uint32_t)t, (uint32_t)r, (uint32_t)b };
    return jqt_jni_call(4, 5, a);   /* id4 setMargins */
}

/* removeView(View)V：receiver=parent，1 个 view 句柄 */
uint32_t jqt_render_removeView(uint32_t parent_handle, uint32_t view_handle){
    uint32_t a[2] = { parent_handle, view_handle };
    return jqt_jni_call(1, 2, a);   /* id1 removeView */
}

/* ★ 接线点 2：对局 UI（C 组 3 条：SiteView.create / TextDlg.<init> / Intent.getParcelableExtra）
 *   OnEvent("resign"/"draw"/"layout") 时 so 调这些 Java 对话框。 */

/* SiteView.create(Activity,String,String,int,int,bool)V：静态，6 参 */
uint32_t jqt_render_siteViewCreate(uint32_t activity, uint32_t url, uint32_t title,
                                   int32_t backColor, int32_t textColor, int32_t back){
    uint32_t a[6] = { activity, url, title, (uint32_t)backColor, (uint32_t)textColor, (uint32_t)back };
    return jqt_jni_call(20, 6, a);  /* id20 SiteView.create */
}

/* TextDlg.<init>(Context,String,String,String,int)V：构造，5 参（NewObject）*/
uint32_t jqt_render_newTextDlg(uint32_t context, uint32_t s1, uint32_t s2, uint32_t s3, int32_t n){
    uint32_t a[5] = { context, s1, s2, s3, (uint32_t)n };
    return jqt_jni_call(21, 5, a);  /* id21 TextDlg.<init> */
}

/* ★ 接线点 3：存档（E 组 3 条：openFileInput / openFileOutput / getAssets）
 *   OnPause 存 one.sjg，OnCreate 续局读 one.sjg。 */

/* openFileInput(String)Ljava/io/FileInputStream; receiver=context，1 个 String 句柄 */
uint32_t jqt_render_openFileInput(uint32_t context, uint32_t name){
    uint32_t a[2] = { context, name };
    return jqt_jni_call(24, 2, a);  /* id24 openFileInput（查表）*/
}

/* openFileOutput(String,int)Ljava/io/FileOutputStream; receiver=context，2 参 */
uint32_t jqt_render_openFileOutput(uint32_t context, uint32_t name, int32_t mode){
    uint32_t a[3] = { context, name, (uint32_t)mode };
    return jqt_jni_call(25, 3, a);  /* id25 openFileOutput */
}

/* getAssets()Landroid/content/res/AssetManager; receiver=context，无参 */
uint32_t jqt_render_getAssets(uint32_t context){
    uint32_t a[1] = { context };
    return jqt_jni_call(23, 1, a);  /* id23 getAssets */
}

/* ★ 接线点 4：A 组剩余 7 条（12 条里前 5 条已有，补 7 条）
 *   getParent / addContentView / LinearLayout.<init> / LayoutParams.<init> /
 *   setEGLConfigChooser / setEGLContextClientVersion / getHolder */

/* getParent()Landroid/view/ViewParent; receiver=this，无参 */
uint32_t jqt_render_getParent(uint32_t this_handle){
    uint32_t a[1] = { this_handle };
    return jqt_jni_call(0, 1, a);   /* id0 getParent */
}

/* addContentView(View,LayoutParams)V：receiver=activity（Context），2 参 */
uint32_t jqt_render_addContentView(uint32_t context, uint32_t view, uint32_t lp){
    uint32_t a[3] = { context, view, lp };
    return jqt_jni_call(7, 3, a);   /* id7 addContentView */
}

/* LinearLayout.<init>(Context)V：构造，1 参（NewObject）*/
uint32_t jqt_render_newLinearLayout(uint32_t context){
    uint32_t a[1] = { context };
    return jqt_jni_call(6, 1, a);   /* id6 LinearLayout.<init> */
}

/* ViewGroup$LayoutParams.<init>(int,int)V：构造，2 参（NewObject）*/
uint32_t jqt_render_newLayoutParams(uint32_t w, uint32_t h){
    uint32_t a[2] = { w, h };
    return jqt_jni_call(8, 2, a);   /* id8 LayoutParams.<init> */
}

/* GLView.setEGLConfigChooser(EGLConfigChooser)V：receiver=glview（GLView 实例），1 参 */
uint32_t jqt_render_setEGLConfigChooser(uint32_t glview, uint32_t chooser){
    uint32_t a[2] = { glview, chooser };
    return jqt_jni_call(9, 2, a);   /* id9 setEGLConfigChooser */
}

/* GLView.setEGLContextClientVersion(int)V：receiver=glview，1 参 */
uint32_t jqt_render_setEGLContextClientVersion(uint32_t glview, uint32_t ver){
    uint32_t a[2] = { glview, ver };
    return jqt_jni_call(10, 2, a);  /* id10 setEGLContextClientVersion */
}

/* GLView.getHolder()Landroid/view/SurfaceHolder; receiver=glview，无参 */
uint32_t jqt_render_getHolder(uint32_t glview){
    uint32_t a[1] = { glview };
    return jqt_jni_call(11, 1, a);  /* id11 getHolder */
}

/* §124aj 渲染链接线 receiver 源：OnCreate NewObject(GLView) 的句柄。
 * jni_aligned.c 的 g_glview_handle 是全局，这里加 accessor 供 jqt_vtbl.c 插桩。 */
extern uint32_t g_glview_handle;
uint32_t jqt_render_get_glview_handle(void){ return g_glview_handle; }
