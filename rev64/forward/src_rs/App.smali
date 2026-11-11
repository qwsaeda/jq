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

    # ★ 全面屏（现代游戏设计，用户指正"顶部栏黑边"）：隐藏状态栏+导航栏，沉浸式
    # getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN)  # 0x400
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;

    move-result-object v0

    const v1, 0x400

    invoke-virtual {v0, v1}, Landroid/view/Window;->addFlags(I)V

    # 沉浸式 sticky：SYSTEM_UI_FLAG_IMMERSIVE_STICKY(0x1000)|FULLSCREEN(0x4)|
    # HIDE_NAVIGATION(0x2)|LAYOUT_STABLE(0x100)|LAYOUT_HIDE_NAVIGATION(0x200)|
    # LAYOUT_FULLSCREEN(0x400) = 0x1706 → content 延展到系统栏下 + 系统栏隐藏
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;

    move-result-object v0

    invoke-virtual {v0}, Landroid/view/Window;->getDecorView()Landroid/view/View;

    move-result-object v0

    const v1, 0x1706

    invoke-virtual {v0, v1}, Landroid/view/View;->setSystemUiVisibility(I)V

    # ★ 2026-11-10 用户指正"MuMu 玩不了"修复：MuMu 顶部系统"Viewing full screen / GOT IT"
    # 提示栏压在 app 画面之上拦截触摸（用户进不了对局）。API 30+ 主动
    # WindowInsetsController.hide(statusBars|navigationBars) dismiss 系统全屏提示栏；
    # API <30 维持 setSystemUiVisibility 旧路径（行为不变）。
    # API 30 门槛（const/16：0x1e 超 const/4 的 4bit 常量域）
    const/16 v2, 0x1e
    sget v3, Landroid/os/Build$VERSION;->SDK_INT:I
    if-lt v3, v2, :cond_immersive_skip
    # API 30+：window.insetsController().hide(statusBars | navigationBars)
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v0
    invoke-virtual {v0}, Landroid/view/Window;->getInsetsController()Landroid/view/WindowInsetsController;
    move-result-object v0
    if-eqz v0, :cond_immersive_skip
    # WindowInsets.Type statusBars=0x1 | navigationBars=0x2 = 0x3（API30+ WindowInsetsController.hide(int)）
    const/4 v1, 0x3
    invoke-interface {v0, v1}, Landroid/view/WindowInsetsController;->hide(I)V
    :cond_immersive_skip

    # ★ 2026-11-11 现代全面屏适配（用户"安卓设计都是全面屏"）：刘海/挖孔避让。
    # API 28+：layoutInDisplayCutoutMode = SHORT_EDGES（内容延伸到刘海短边区域，
    # 挖孔屏上游戏内容铺满全屏不被状态栏 inset 压缩；游戏中央内容不受顶部挖孔遮挡）。
    const/16 v2, 0x1c
    sget v3, Landroid/os/Build$VERSION;->SDK_INT:I
    if-lt v3, v2, :cond_cutout_skip
    invoke-virtual {p0}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v0
    invoke-virtual {v0}, Landroid/view/Window;->getAttributes()Landroid/view/WindowManager$LayoutParams;
    move-result-object v0
    if-eqz v0, :cond_cutout_skip
    # LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES = 1（API 28+ 常量，兼容 30+ 的 ALWAYS/NEVER）
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
