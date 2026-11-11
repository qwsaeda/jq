# rev64/tools 工具清单（机械化工具，别再手写内联脚本）

## src_rs 源码重写路线（本轮新增 5 个 + 原有工具）

| 工具 | 用途 | 用法 |
|---|---|---|
| `mumu_swap_so.sh` | 换 so 一条龙：编译→打包(对齐+签名)→链接审计→装 MuMu→推 SPAK→启动→截图+像素判定 | `bash mumu_swap_so.sh` |
| `so_link_check.py` | so UND GL/EGL 符号能否被 MuMu 系统库解析（`dlopen failed to link` 防回归） | `python3 so_link_check.py <so>`（首跑加 `--update-syms` 建缓存） |
| `src_rs_check.py` | src_rs 8+2 项红线防回归审计（PBUFFER_BIT/不预 makeCurrent/无 glVertexAttribPointer/setContentView/SIGSEGV 兜底/链接行…） | `python3 src_rs_check.py` |
| `shot_check.py` | 截图出帧像素判定（非黑比例 + 5 点采样 + 结论） | `python3 shot_check.py <verify.png>` |
| `shot_check.py classify` | §sp27 截图分类判读（warm_brown_ui / white_anr / black / board_candidate，替代手写 PNG 解析+暖棕占比脚本） | `python3 shot_check.py classify <png>` |
| `glview_smali_check.py` | smali 出帧驱动链完整性（GLView 继承/GLThread.start/EglHelper EGL 链/onDrawFrame/setRenderer） | `python3 glview_smali_check.py <smali目录>` |
| `mumu_bt_test.sh` | 蓝牙链路 MuMu 实机验证（bttest 桌面 14 断言 → so 编译 → bt_session 符号链入 → 装 MuMu → 进程存活 → OnBluetooth JNI 导出） | `bash mumu_bt_test.sh`（5/5 PASS） |
| `lay_extract.py` | 从 SPAK 解包产物提取 12 预设布局（data 组 idx 47-58，00.lay=default 已逐字节证） | `python3 lay_extract.py` → `rev64/dist/layouts/00-11.lay` |
| `so_strings.py` | 原版 32 位 so 字符串提取（带文件偏移，替代手写 `bytes.find` 循环） | `python3 so_strings.py <so> [关键词]`；`--rodata` 只看 .rodata 明文区（0x120bf8-0x13dc28）；`<so> 0xADDR` 按偏移读 |
| `gl_probe.c` | MuMu 64 位 GL 出帧能力探测（清屏/client-state 几何/glDrawTexfOES 三路） | **注意：独立 64 位二进制跑不了 EGL（MuMu `eglInitialize` 只在 app 进程上下文可用）**。用途 = 参考实现 / 真机独立跑。编译：`aarch64 NDK clang -O2 gl_probe.c -o /tmp/opencode/gl_probe -lEGL -lGLESv1_CM -lGLESv2 -llog -lm` |
| `src_rs/bt_selftest.c`（`make bttest`） | 蓝牙帧协议 + 状态机桌面自测（v2 帧往返/非法帧拒绝/FLIP/MOVE 应用/回合闸口/全盘同步），无真机蓝牙环境也能验逻辑 | `cd rev64/forward/src_rs && make bttest` |

## 原有工具（翻译体/影子路线）

- `link_audit.py` — 翻译体 so 链接安全审计（UND 解析/悬空内部符号/dispatch 表一致性/APK 完整性）
- `tex_pipeline.py` — 纹理/GL 分析（plt/vtable/closure/glcall/texseq + vtabswrite/dispatchfix/i2f/vtwrite 机械化修复）
- `mumu_verify.sh` / `mumu_proven_und.json` — 翻译体 MuMu 实机验证 + 实证基线 UND 集
- `regen_vtbl.py` — jqt_vtbl.c 机械化再生成（§sp27 固化，防手工重复）。用法（build 子命令自动前置 render_patch.py，Lbf51c 豁免必打）：
  - `python3 rev64/tools/regen_vtbl.py`（chain 基线版，默认）
  - `python3 rev64/tools/regen_vtbl.py sp27`（sp27 版：13 访问器+27 闭包 + F_9d3d4 完整体，§sp27 稳定态）
  - `python3 rev64/tools/regen_vtbl.py sp27 build`（sp27 + 自动编译 + 打包 + 装 MuMu + 启动）
  - 幂等注入 4 类补丁：i2f 存根批量修 / jqt_blx_dispatch 绝对归一化 / L2400c 强制 glClear / F_9d3d4 渲染器单例完整体；
    自动去重（sp27 regen 的 F_b5eec 类重复定义）+ brace 完整性检查。与已验证 dist 逐字节一致（diff 0）
- `render_patch.py` — jqt_OnRender_full.c 渲染链补丁机械化（§sp27）。用法：`python3 rev64/tools/render_patch.py`。
  幂等注入 Lbf51c vtable 检查豁免（`r5==MEM(0x162400)` 时不判"无效"——旧 `MEM(r5)!=0x1595c0` 字面量误杀单例自身）+ brace 检查
- `revert_to_sp27.sh` — sp28 崩溃一键回退到 sp27 稳定态（§sp28 沉淀，防回归误判）。用法：`bash rev64/tools/revert_to_sp27.sh`
  流程：恢复 jqt_vtbl.c = /tmp/opencode/chain/jqt_vtbl_sp27_verified.c + render_patch.py（Lbf51c 豁免）+ 编译 + 打包 + 装 MuMu + 启动。sp28（纹理上传闭包 F_9e2b4/F_9e0f0）GLThread SIGSEGV → 回 sp27
- `gen_shadow.py` / `gen_arm_thunks.py` / `wire_render_jni.py` 等 — 影子运行时生成工具链

## 回归命令（一条全跑）

```bash
cd /disk/workspaces/jq/rev64/forward/src_rs && make bttest && make desk && python3 regress.py --games 20
python3 /disk/workspaces/jq/rev64/tools/src_rs_check.py
```

## 工具链依赖

- MuMu 符号缓存：`/tmp/opencode/mumu_und_syms/all_syms.json`（`so_link_check.py --update-syms` 生成/刷新，
  含 libEGL/libGLESv1_CM/libGLESv2/libandroid 导出符号）
- MuMu GL 库本地缓存：`/tmp/opencode/mumu_glesv1_cm.so`（so_link_check 直接复用，免 pull）
