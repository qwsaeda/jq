# 阶段 R4 · 渲染层（GLES2 接入）

## 目标
- 用 GLES2 渲染棋盘（原版 GLES1 立即模式在新 NDK 已移除符号）。
- JNI `OnGraphCreate/OnGraphChange/OnUpdate` 驱动渲染，真实棋盘纹理可注入。
- 视觉：棋盘色块 + 边线 + 真实棋盘 PNG 纹理（可选）。

## 关键决策
- **GLES1 立即模式不可用**：原版用 `libGLESv1_CM.so`（glBegin/glEnd）。但新 NDK 的
  `GLES/gl.h` 与 stub **已移除立即模式符号**（glBegin/glEnd 不在导出表）。
  arm64 下无法编译链接 → **改用 GLES2**（shader + 顶点缓冲，视觉等价）。
- **纹理注入**：`junqi_render_set_board_texture(rgba,w,h)` 接收真实 PNG 解码 RGBA，
  无则回退色块网格（可编译骨架）。
- **线程安全**：JNI 回调与 GL 线程经 `pthread_mutex` 保护（g_mutex）。

## 接口规范
```c
void junqi_render_init(int w, int h);          /* OnGraphChange 时调用，建 shader */
void junqi_render_frame(const jq_state *st);   /* OnUpdate 每帧，画 60 格 */
void junqi_render_set_board_texture(const unsigned char *rgba, int w, int h); /* 注入棋盘纹理 */
```
- shader：VS `attribute vec2 a_pos; attribute vec3 a_color`；FS 输出 `gl_FragColor`。
- 顶点布局：每顶点 5 float（x,y,r,g,b），`glVertexAttribPointer` stride 5*sizeof(float)。
- 格色：空=绿（行营/大本营特殊色），未翻=灰，蓝=蓝，红=红。

## JNI 接入
- `OnGraphCreate`：`junqi_render_init(g_view_w, g_view_h)`。
- `OnGraphChange(w,h)`：更新 `g_view_w/h` + `junqi_render_init(w,h)`。
- `OnUpdate`：`junqi_render_frame(&g_state)`，返回 JNI_TRUE。

## 经验（→ LESSONS）
- L23：走子规则自检清单（渲染依赖状态正确）。
- L24：OnTouch 坐标→格映射 + 选中→走子状态机。
- L25：单机对弈（玩家触屏 + AI OnUpdate 自动回应）。
- GLES1 立即模式符号在新 NDK 移除，改用 GLES2 是唯一可编译路径。

## 复用指引
- 棋盘/棋子 PNG 素材：`dist/spak_res/`（datah#188 737×751、datax#187 720×736、datax#297 736×736）。
- 解码：`png_decode(data,len,&rgba,&w,&h)`（仅 zlib，无 libpng；L26）。
- 注入：`junqi_render_set_board_texture(rgba,w,h)`；渲染层画全屏纹理四边形（采样 u_tex）。
- 选中格高亮：`junqi_render_set_selected(idx)`（渲染时颜色亮化）。
- 棋盘资源：`spak_get_by_hash(datah#188 hash)`（路径需运行时拼接，L27）。

## 已验证
- `render_gles2.c` 编译无 error（GLES2 头，`-lGLESv2`）。
- JNI OnGraph* 驱动渲染接入，arm64 库 14 JNI 导出。
- 布局状态驱动渲染（蓝红镜像、军旗/地雷、暗棋背面、rank 数字）。
- OnTouch 内 `bl jq_cell_index/jq_try_move/jq_flip/junqi_render_set_selected` 反汇编确认。
- PNG 解码：board_datah(737×751)/board_datax(736×736)/piece(72×72) 全部成功（L26）。
- 棋盘纹理注入：OnGraphCreate 经 `spak_get_by_hash` + `png_decode` → `set_board_texture`。
- 棋盘居中 + 格方形（5:12）+ 纹理映射棋盘主体（L35）；touch 同步（L35）。
- 棋子 rank 数字渲染（L38）；空格不画色块（纹理露出，L42）。
- 特殊子符号（军旗/炸弹/地雷，L46）；可走格提示（L46）。

## 诚实边界
- 色块渲染已由**真实棋盘 PNG + rank 数字**替代（注入接口 + 加载全接通）。
- 棋子 sprite sheet（`data/sg/piece.png`）**不在 SPAK**（883 资源无 sg），rank 数字是诚实渲染。
- 字体渲染未接入（原版 TTF，像素字模替代）。
