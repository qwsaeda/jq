.class public Lcom/qwsaeda/App$ImmersiveRunnable$InsetsListener;
.super Ljava/lang/Object;
.implements Landroid/view/View$OnApplyWindowInsetsListener;

.method constructor <init>()V
    .registers 1
    invoke-direct {p0}, Ljava/lang/Object;-><init>()V
    return-void
.end method

.method public onApplyWindowInsets(Landroid/view/View; Landroid/view/WindowInsets;)Landroid/view/WindowInsets;
    .registers 4
    # 官方 edge-to-edge（第 8 轮）：insets 消费监听器。
    # GL 全屏场景 = 渲染层按 view_w/h 铺满全屏（EGL surface 不受系统栏 inset 影响），
    # 系统栏 TRANSIENT 滑出可见时内容自然延伸到系统栏下（edge-to-edge），无需手动裁剪。
    # 直接 return insets（不消费）：insets 自然传播到子 view，GL surface 不受 inset 约束。
    # ★ 诚实边界：不返回 CONSUMED/NONE（红果 framework 字段缺失导致 NoSuchFieldError，
    #   GL 全屏场景 insets 消费非必需，return insets 官方沉浸式标准行为）
    return-object p2
.end method
