.class public Lcom/qwsaeda/App$ImmersiveRunnable;
.super Ljava/lang/Object;
.implements Ljava/lang/Runnable;

.field final a:Lcom/qwsaeda/App;

.method constructor <init>(Lcom/qwsaeda/App;)V
    .registers 2
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V
    iput-object p1, p0, Lcom/qwsaeda/App$ImmersiveRunnable;->a:Lcom/qwsaeda/App;
    return-void
.end method

.method public run()V
    .registers 6
    # 主线程（window attach 后）：API30+ 官方 WindowInsetsController.hide + edge-to-edge
    # ★ 2026-11-12 官方 Android 15 标准（第 6 轮）：系统栏行为策略（setSystemBarsBehavior）。
    #   旧：hide(statusBars|navBars) 后系统栏永久隐藏，用户无法滑出（非官方推荐）。
    #   官方 Android 15（developer.android.com/develop/behavior-changes/15）：
    #   沉浸 = hide + SYSTEM_BAR_BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE（滑动手势临时显示，
    #   松手不动即自动隐藏，官方沉浸式游戏标准行为）。
    const/16 v0, 0x1e
    sget v1, Landroid/os/Build$VERSION;->SDK_INT:I
    if-lt v1, v0, :imm_done
    iget-object v1, p0, Lcom/qwsaeda/App$ImmersiveRunnable;->a:Lcom/qwsaeda/App;
    invoke-virtual {v1}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v1
    invoke-virtual {v1}, Landroid/view/Window;->getInsetsController()Landroid/view/WindowInsetsController;
    move-result-object v1
    if-eqz v1, :imm_done
    # WindowInsets.Type statusBars=0x1 | navigationBars=0x2 = 0x3
    const/4 v2, 0x3
    invoke-interface {v1, v2}, Landroid/view/WindowInsetsController;->hide(I)V
    # ★ 官方 Android 15 系统栏行为：BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE=1
    #   （滑出临时显示，松手不点即自动隐藏；API32+ framework 是 interface，invoke-interface）
    const/4 v2, 0x1
    invoke-interface {v1, v2}, Landroid/view/WindowInsetsController;->setSystemBarsBehavior(I)V
    # ★ 官方 Android 15 系统栏外观（第 7 轮）：isAppearanceLightStatusBars=false
    #   游戏深色 UI → 状态栏图标浅色（官方按内容动态设；API30+ invoke-virtual，
    #   返回 WindowInsetsController 自身用于链式）。appearance=0(LIGHT_BARS_NONE) flags=0
    const/4 v2, 0x0
    invoke-interface {v1, v2, v2}, Landroid/view/WindowInsetsController;->setSystemBarsAppearance(II)V
    # ② 官方 edge-to-edge 第一原则（setDecorFitsSystemWindows(false)，官方内容延伸到系统栏下）
    iget-object v1, p0, Lcom/qwsaeda/App$ImmersiveRunnable;->a:Lcom/qwsaeda/App;
    invoke-virtual {v1}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v1
    if-eqz v1, :imm_done
    const/4 v2, 0x0
    invoke-virtual {v1, v2}, Landroid/view/Window;->setDecorFitsSystemWindows(Z)V
    # ★ 官方 Android 15 edge-to-edge 最后动作（第 8 轮）：insets 消费。
    #   setOnApplyWindowInsetsListener：系统栏 TRANSIENT 滑出可见时，内容避让系统栏
    #   （非裁剪）。GL 全屏场景渲染层按 view_w/h 铺满，此处消费 insets 防默认 inset 裁剪。
    #   官方正解 = View.setOnApplyWindowInsetsListener（API29+，消费 insets 返回 CONSUMED）
    iget-object v1, p0, Lcom/qwsaeda/App$ImmersiveRunnable;->a:Lcom/qwsaeda/App;
    invoke-virtual {v1}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v1
    invoke-virtual {v1}, Landroid/view/Window;->getDecorView()Landroid/view/View;
    move-result-object v1
    if-eqz v1, :imm_done
    # 监听 insets 变化（onApplyWindowInsets 返回 CONSUMED 消费，防系统栏裁剪 content）
    new-instance v2, Lcom/qwsaeda/App$ImmersiveRunnable$InsetsListener;
    invoke-direct {v2}, Lcom/qwsaeda/App$ImmersiveRunnable$InsetsListener;-><init>()V
    invoke-virtual {v1, v2}, Landroid/view/View;->setOnApplyWindowInsetsListener(Landroid/view/View$OnApplyWindowInsetsListener;)V
    :imm_done
    return-void
.end method
