.class public Lcom/qwsaeda/App;
.super Landroid/app/Activity;
.source "App.java"

# interfaces
.implements Landroid/view/View$OnTouchListener;
.implements Landroid/os/Handler$Callback;
.implements Lcom/qwsaeda/GLView$Renderer;


# instance fields
.field protected m_handler:Landroid/os/Handler;

# ★ hp 翻译桥库缺失容错标志（loadLibrary 失败 s_native_ok=0，native 调用全跳过）
.field private static s_native_ok:I

# static fields
.field static s_ctx:Landroid/app/Activity;    # 全面屏+对话框：Activity 引用缓存（showInfo 用）


# direct methods
.method static constructor <clinit>()V
    .locals 1

    .prologue
    .line 20
    const-string v0, "junqi"

    :try_start
    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
    const/4 v0, 0x1
    sput v0, Lcom/qwsaeda/App;->s_native_ok:I
    :try_end
    .catch Ljava/lang/UnsatisfiedLinkError; {:try_start .. :try_end} :cond_hp_fail

    .line 21
    return-void

    :cond_hp_fail
    move-exception v0
    const/4 v0, 0x0
    sput v0, Lcom/qwsaeda/App;->s_native_ok:I
    return-void
.end method

.method public constructor <init>()V
    .locals 0

    .prologue
    .line 16
    invoke-direct {p0}, Landroid/app/Activity;-><init>()V

    return-void
.end method

.method public static native OnBluetooth(Landroid/content/Intent;)V
.end method

.method public static native OnCreate(Landroid/app/Activity;)Lcom/qwsaeda/GLView;
.end method

.method public static native OnDestroy()V
.end method

.method public static native OnEvent(Ljava/lang/String;Ljava/lang/String;)V
.end method

.method public static native OnGraphChange(II)V
.end method

.method public static native OnGraphCreate()V
.end method

.method public static native OnInput(Ljava/lang/String;)V
.end method

.method public static native OnKey(Landroid/view/KeyEvent;)Z
.end method

.method public static native OnPause()V
.end method

.method public static native OnResult(IILandroid/content/Intent;)V
.end method

.method public static native OnResume()V
.end method

.method public static native OnTimer()V
.end method

.method public static native OnTouch(Landroid/view/MotionEvent;)Z
.end method

.method public static native OnUpdate()Z
.end method


# ★ 按键功能对齐：Java 对话框（游戏规则/战况统计/软件信息/联网/联机 显示文字）
.method public static showInfo(Ljava/lang/String;Ljava/lang/String;)V
    .locals 5
    .param p0, "title"    # Ljava/lang/String;
    .param p1, "text"     # Ljava/lang/String;

    .prologue
    sget-object v0, Lcom/qwsaeda/App;->s_ctx:Landroid/app/Activity;

    if-eqz v0, :cond_0

    new-instance v1, Landroid/app/AlertDialog$Builder;

    invoke-direct {v1, v0}, Landroid/app/AlertDialog$Builder;-><init>(Landroid/content/Context;)V

    invoke-virtual {v1, p0}, Landroid/app/AlertDialog$Builder;->setTitle(Ljava/lang/CharSequence;)Landroid/app/AlertDialog$Builder;

    move-result-object v1

    invoke-virtual {v1, p1}, Landroid/app/AlertDialog$Builder;->setMessage(Ljava/lang/CharSequence;)Landroid/app/AlertDialog$Builder;

    move-result-object v1

    const-string v2, "确定"

    const/4 v3, 0x0

    invoke-virtual {v1, v2, v3}, Landroid/app/AlertDialog$Builder;->setPositiveButton(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)Landroid/app/AlertDialog$Builder;

    move-result-object v1

    invoke-virtual {v1}, Landroid/app/AlertDialog$Builder;->show()Landroid/app/AlertDialog;

    :cond_0
    return-void
.end method


# ★ 蓝牙联机接线（BtSpp 宿主 socket；原生 OnTouch 经 btHost/btClient 启，发送经 btSend）
.method public static btState(Ljava/lang/String;)V
    .locals 3
    .param p0, "s"    # Ljava/lang/String;

    .prologue
    const-string v0, "junqi_bt"

    invoke-static {v0, p0}, Landroid/util/Log;->i(Ljava/lang/String;Ljava/lang/String;)I

    # ★ 蓝牙连接成功 → 主线程自动进入两人对局（BT 会话保留，蓝先手）
    const-string v1, "host_connected"

    invoke-virtual {p0, v1}, Ljava/lang/String;->equals(Ljava/lang/Object;)Z

    move-result v1

    if-nez v1, :cond_start

    const-string v1, "client_connected"

    invoke-virtual {p0, v1}, Ljava/lang/String;->equals(Ljava/lang/Object;)Z

    move-result v1

    if-nez v1, :cond_start

    return-void

    :cond_start
    sget-object v1, Lcom/qwsaeda/App;->s_ctx:Landroid/app/Activity;

    if-eqz v1, :cond_0

    new-instance v2, Lcom/qwsaeda/App$BtStart;

    invoke-direct {v2}, Lcom/qwsaeda/App$BtStart;-><init>()V

    invoke-virtual {v1, v2}, Landroid/app/Activity;->runOnUiThread(Ljava/lang/Runnable;)V

    :cond_0
    return-void
.end method

.method public static btSend(Ljava/lang/String;)V
    .locals 0
    .param p0, "frame"    # Ljava/lang/String;

    .prologue
    invoke-static {p0}, Lcom/qwsaeda/BtSpp;->sendFrame(Ljava/lang/String;)V

    return-void
.end method

.method public static btHost()V
    .locals 0

    .prologue
    invoke-static {}, Lcom/qwsaeda/BtSpp;->startHost()V

    return-void
.end method

.method public static btClient(Ljava/lang/String;)V
    .locals 0
    .param p0, "addr"    # Ljava/lang/String;

    .prologue
    invoke-static {p0}, Lcom/qwsaeda/BtSpp;->startClient(Ljava/lang/String;)V

    return-void
.end method


# virtual methods
.method public handleMessage(Landroid/os/Message;)Z
    .locals 5
    .param p1, "msg"    # Landroid/os/Message;

    .prologue
    const/4 v4, 0x0

    .line 96
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {}, Lcom/qwsaeda/App;->OnTimer()V
    :after_native

    .line 97
    iget-object v0, p0, Lcom/qwsaeda/App;->m_handler:Landroid/os/Handler;

    invoke-virtual {v0, v4}, Landroid/os/Handler;->removeMessages(I)V

    .line 98
    iget-object v0, p0, Lcom/qwsaeda/App;->m_handler:Landroid/os/Handler;

    iget-object v1, p0, Lcom/qwsaeda/App;->m_handler:Landroid/os/Handler;

    invoke-virtual {v1, v4}, Landroid/os/Handler;->obtainMessage(I)Landroid/os/Message;

    move-result-object v1

    const-wide/16 v2, 0x64

    invoke-virtual {v0, v1, v2, v3}, Landroid/os/Handler;->sendMessageDelayed(Landroid/os/Message;J)Z

    .line 99
    return v4
.end method

.method public onActivityResult(IILandroid/content/Intent;)V
    .locals 1
    .param p1, "requestCode"    # I
    .param p2, "resultCode"    # I
    .param p3, "data"    # Landroid/content/Intent;

    .prologue
    .line 119
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {p1, p2, p3}, Lcom/qwsaeda/App;->OnResult(IILandroid/content/Intent;)V
    :after_native

    .line 120
    return-void
.end method

.method protected onCreate(Landroid/os/Bundle;)V
    .locals 4
    .param p1, "savedInstanceState"    # Landroid/os/Bundle;

    .prologue
    .line 44
    invoke-super {p0, p1}, Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V

    # ★ 全面屏+对话框：缓存 Activity 引用（showInfo 静态方法用）
    sput-object p0, Lcom/qwsaeda/App;->s_ctx:Landroid/app/Activity;

    # ★ 2026-11-12 全面屏（安卓官方最新标准，用户"全部用最新安卓标准"指正）：
    # 旧实现用 FLAG_FULLSCREEN(0x400) + setSystemUiVisibility(0x1706) —— 两者 API 30+
    # 已废弃（Deprecation），官方推荐 = edge-to-edge + WindowInsetsController。
    # 本段只做 API<30 兜底（旧设备）；API30+ 走下方官方 insetsController + edge-to-edge。
    # 门槛（const/16：0x1e 超 const/4 4bit 域）：SDK_INT < 30 → 旧 setSystemUiVisibility
    const/16 v2, 0x1e
    sget v3, Landroid/os/Build$VERSION;->SDK_INT:I
    if-ge v3, v2, :cond_legacy_skip
    # ---- API <30 兜底（旧设备，官方已废弃但旧系统仍认）----
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v0
    invoke-virtual {v0}, Landroid/view/Window;->getDecorView()Landroid/view/View;
    move-result-object v0
    const v1, 0x1706
    invoke-virtual {v0, v1}, Landroid/view/View;->setSystemUiVisibility(I)V
    :cond_legacy_skip

    # ★ API 30+（官方最新标准）：WindowInsetsController.hide + edge-to-edge。
    # 官方推荐（developer.android.com/training/decorate/navigation）：
    # ① setDecorFitsSystemWindows(false) = edge-to-edge（内容延伸到系统栏下，自己处理 insets）
    # ② insetsController.hide(statusBars|navigationBars) 隐藏系统栏
    # ③ 刘海/挖孔：layoutInDisplayCutoutMode=SHORT_EDGES（API28+，内容延伸短边）
    # ★ 时序修正（本轮崩溃根因）：WindowInsetsController / DecorView 必须在 window attach
    #   后才能取，onCreate 早期 getInsetsController() 对未 attach DecorView NPE。
    #   官方正解 = 整段 post 到主线程（view 树就绪后）执行。
    const/16 v2, 0x1e
    sget v3, Landroid/os/Build$VERSION;->SDK_INT:I
    if-lt v3, v2, :cond_immersive_skip
    # 构造 Runnable：run() 里做 ②②b（API30+），主线程保证 window 已 attach
    new-instance v0, Lcom/qwsaeda/App$ImmersiveRunnable;
    invoke-direct {v0, p0}, Lcom/qwsaeda/App$ImmersiveRunnable;-><init>(Lcom/qwsaeda/App;)V
    # 主线程 handler post（m_handler 在 App 里，主线程）
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v1
    invoke-virtual {v1}, Landroid/view/Window;->getDecorView()Landroid/view/View;
    move-result-object v1
    if-eqz v1, :cond_immersive_skip
    invoke-virtual {v1, v0}, Landroid/view/View;->post(Ljava/lang/Runnable;)Z
    :cond_immersive_skip

    # ③ 刘海/挖孔避让（官方 API 28+：SHORT_EDGES 内容延伸短边，游戏中央不被挖孔挡）
    const/16 v2, 0x1c
    sget v3, Landroid/os/Build$VERSION;->SDK_INT:I
    if-lt v3, v2, :cond_cutout_skip
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v0
    invoke-virtual {v0}, Landroid/view/Window;->getAttributes()Landroid/view/WindowManager$LayoutParams;
    move-result-object v0
    if-eqz v0, :cond_cutout_skip
    # LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES = 1
    const/4 v1, 0x1
    iput v1, v0, Landroid/view/WindowManager$LayoutParams;->layoutInDisplayCutoutMode:I
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v2
    invoke-virtual {v2, v0}, Landroid/view/Window;->setAttributes(Landroid/view/WindowManager$LayoutParams;)V
    :cond_cutout_skip

    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {p0}, Lcom/qwsaeda/App;->OnCreate(Landroid/app/Activity;)Lcom/qwsaeda/GLView;

    move-result-object v0
    :after_native

    .line 47
    .local v0, "view":Lcom/qwsaeda/GLView;
    if-eqz v0, :cond_0

    invoke-virtual {v0, p0}, Lcom/qwsaeda/GLView;->setOnTouchListener(Landroid/view/View$OnTouchListener;)V

    .line 48
    invoke-virtual {v0, p0}, Lcom/qwsaeda/GLView;->setRenderer(Lcom/qwsaeda/GLView$Renderer;)V

    .line 50
    :cond_0
    new-instance v1, Landroid/os/Handler;

    invoke-direct {v1, p0}, Landroid/os/Handler;-><init>(Landroid/os/Handler$Callback;)V

    iput-object v1, p0, Lcom/qwsaeda/App;->m_handler:Landroid/os/Handler;

    .line 51
    const/4 v1, 0x0

    invoke-virtual {p0, v1}, Lcom/qwsaeda/App;->handleMessage(Landroid/os/Message;)Z

    .line 52
    return-void
.end method

.method protected onDestroy()V
    .locals 2

    .prologue
    const/4 v1, 0x0

    .line 68
    iget-object v0, p0, Lcom/qwsaeda/App;->m_handler:Landroid/os/Handler;

    invoke-virtual {v0, v1}, Landroid/os/Handler;->removeMessages(I)V

    .line 69
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {}, Lcom/qwsaeda/App;->OnDestroy()V
    :after_native

    .line 70
    invoke-super {p0}, Landroid/app/Activity;->onDestroy()V

    .line 71
    invoke-static {v1}, Ljava/lang/System;->exit(I)V

    .line 72
    return-void
.end method

.method public onDrawFrame(Ljavax/microedition/khronos/opengles/GL10;)Z
    .locals 1
    .param p1, "gl"    # Ljavax/microedition/khronos/opengles/GL10;

    .prologue
    .line 104
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {}, Lcom/qwsaeda/App;->OnUpdate()Z

    move-result v0
    :after_native

    return v0
.end method

.method public onKeyDown(ILandroid/view/KeyEvent;)Z
    .locals 1
    .param p1, "keyCode"    # I
    .param p2, "event"    # Landroid/view/KeyEvent;

    .prologue
    .line 76
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {p2}, Lcom/qwsaeda/App;->OnKey(Landroid/view/KeyEvent;)Z

    move-result v0
    :after_native

    if-eqz v0, :cond_0

    .line 77
    const/4 v0, 0x1

    .line 78
    :goto_0
    return v0

    :cond_0
    invoke-super {p0, p1, p2}, Landroid/app/Activity;->onKeyDown(ILandroid/view/KeyEvent;)Z

    move-result v0

    goto :goto_0
.end method

.method public onKeyUp(ILandroid/view/KeyEvent;)Z
    .locals 1
    .param p1, "keyCode"    # I
    .param p2, "event"    # Landroid/view/KeyEvent;

    .prologue
    .line 83
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {p2}, Lcom/qwsaeda/App;->OnKey(Landroid/view/KeyEvent;)Z

    move-result v0
    :after_native

    if-eqz v0, :cond_0

    .line 84
    const/4 v0, 0x1

    .line 85
    :goto_0
    return v0

    :cond_0
    invoke-super {p0, p1, p2}, Landroid/app/Activity;->onKeyDown(ILandroid/view/KeyEvent;)Z

    move-result v0

    goto :goto_0
.end method

.method protected onPause()V
    .locals 1

    .prologue
    .line 56
    invoke-super {p0}, Landroid/app/Activity;->onPause()V

    .line 57
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {}, Lcom/qwsaeda/App;->OnPause()V
    :after_native

    .line 58
    return-void
.end method

.method protected onResume()V
    .locals 1

    .prologue
    .line 62
    invoke-super {p0}, Landroid/app/Activity;->onResume()V

    .line 63
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {}, Lcom/qwsaeda/App;->OnResume()V
    :after_native

    .line 64
    return-void
.end method

.method public onSurfaceChanged(Ljavax/microedition/khronos/opengles/GL10;II)V
    .locals 1
    .param p1, "gl"    # Ljavax/microedition/khronos/opengles/GL10;
    .param p2, "width"    # I
    .param p3, "height"    # I

    .prologue
    .line 114
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {p2, p3}, Lcom/qwsaeda/App;->OnGraphChange(II)V
    :after_native

    .line 115
    return-void
.end method

.method public onSurfaceCreated(Ljavax/microedition/khronos/opengles/GL10;Ljavax/microedition/khronos/egl/EGLConfig;)V
    .locals 1
    .param p1, "gl"    # Ljavax/microedition/khronos/opengles/GL10;
    .param p2, "config"    # Ljavax/microedition/khronos/egl/EGLConfig;

    .prologue
    .line 109
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {}, Lcom/qwsaeda/App;->OnGraphCreate()V
    :after_native

    .line 110
    return-void
.end method

.method public onTouch(Landroid/view/View;Landroid/view/MotionEvent;)Z
    .locals 1
    .param p1, "v"    # Landroid/view/View;
    .param p2, "event"    # Landroid/view/MotionEvent;

    .prologue
    .line 90
    sget v0, Lcom/qwsaeda/App;->s_native_ok:I
    if-gtz v0, :do_native
    :do_native
    invoke-static {p2}, Lcom/qwsaeda/App;->OnTouch(Landroid/view/MotionEvent;)Z
    :after_native

    .line 91
    const/4 v0, 0x1

    return v0
.end method
