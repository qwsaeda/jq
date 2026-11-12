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
    .registers 4
    # 主线程（window attach 后）：API30+ 官方 WindowInsetsController.hide + edge-to-edge
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
    # edge-to-edge 第一原则（WindowCompat.setDecorFitsSystemWindows(false) 等价）
    iget-object v1, p0, Lcom/qwsaeda/App$ImmersiveRunnable;->a:Lcom/qwsaeda/App;
    invoke-virtual {v1}, Landroid/app/Activity;->getWindow()Landroid/view/Window;
    move-result-object v1
    invoke-virtual {v1}, Landroid/view/Window;->getDecorView()Landroid/view/View;
    move-result-object v1
    if-eqz v1, :imm_done
    const/4 v2, 0x0
    invoke-virtual {v1, v2}, Landroid/view/View;->setFitsSystemWindows(Z)V
    :imm_done
    return-void
.end method
