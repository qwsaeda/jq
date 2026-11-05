# 军棋 C 代码规范（src_rs 强制准则）

> 定位：src_rs C 源码的**强制编码规范**，从经验库 8 坑 + 坑清单沉淀成文。
> 每准则附"违反后果"（真机崩溃/链接失败/静默失败实证）。

## 1. 命名

- 前缀：游戏逻辑 `jq_` / 渲染 `rs_` / UI `ui_` / 蓝牙 `bt_` / 棋盘格几何 `bg_`（datax_177 背景棋盘格，render+input 共享：`bg_row_cy`/`bg_cell_img_center`，junqi.h）。
- 函数体完整 = 命名参数（`_r0` 类）；存根 = 匿名参数 `(uint32_t,uint32_t,...){return 0;}`（翻译体区分；src_rs 不用存根）。
- 静态函数（static）完整体；跨文件函数在 .h 声明。

## 2. 返回值

- 规则引擎返回 **int**（**禁用 enum 返回值**，符号性坑）。
- **0=成功契约显式 `==0`**（`if(fn())` 只用于布尔/真值函数）。
- `jq_try_move` 返回事件码/-1（非法）。

## 3. JNI 反射（8 坑头号来源）

- **GetMethodID/GetFieldID 后必须 `ExceptionCheck + ExceptionClear`**（NoSuchMethodError 累积 → abort）。
- **MediaPlayer 重载实测**：`setDataSource(String)` / `(FD,off,len)` 有，**无 File/(AFD,JJ) 重载**（勿猜）。
- **local ref 用完即删（DeleteLocalRef），删后不得再用**（悬空 → SIGSEGV 0x70）。
- **类/对象 ref 重新获取**（act_cls DeleteLocalRef 后再 GetMethodID → 必须拿 act_cls2）。
- **每个 GetMethodID 后 ExceptionClear**（jni_method_audit.py 自动化扫 8 行内有无清理）。

## 4. 链接安全（so_link_check.py / link_audit.py）

- **内部符号不得悬空**（`F_xxx` 无函数体 → NOTYPE UND → MuMu fail to link）。
- **每次重建 so 后必跑 UND 审计**（`readelf -sW so | awk '$7=="UND"'` vs 基线 diff）。
- **GL 符号直接链接**（-lEGL -lGLESv1_CM -lGLESv2 -landroid），**勿 dlsym 乱改**（引入新 UND）。
- **AAsset 符号 dlsym 运行时解析**（转译层不导出 AAssetManager_fromJava 等 → 直接 UND 会 fail to link）。

## 5. 内存/并发

- 全局状态（g_state/g_undo/g_ts）读写必须 g_mutex（**勿裸读写**，野指针坑）。
- 静态完整体避免大全局（g_undo[UNDO_MAX] 完整快照栈，UNDO_MAX 定界）。
- 循环防护：递归深度阀/调用预算/越界内存阀（翻译体；src_rs 用规则引擎自检）。

## 6. 错误处理

- 日志：`LOGI/LOGE`（android/log.h），走子/事件/音效关键路径必日志（真机诊断）。
- 失败路径：返回值契约 + LOGI（勿静默失败）。
- 音效降级：g_mp 未建 → sfx 降级（日志"实例未建"，勿崩）。

## 7. 编译

- Makefile desk（桌面 headless，-DRS_NO_GL）+ so（NDK arm64，-lEGL -lGLESv1_CM -lGLESv2 -landroid）。
- `make evverify/walkverify/laytest/touchtest/gametest/bttest` 各 target 判据 grep 同步（N25）。
- 回归 12/12 + 真机 9/9 全过才交付。

## 8. 防复发（坑清单）

1. 返回 enum → int（jq_try_move）。
2. 0=成功契约显式 `==0`（ai_pick 3 调用点）。
3. GetMethodID 后 ExceptionClear（jni_method_audit.py）。
4. local ref 悬空（DeleteLocalRef 后不得再用）。
5. 内部符号悬空（so_link_check.py UND 审计）。
6. 锁竞争（g_mutex 统一；AAsset 走文件系统优先）。
7. 触摸 ACTION_DOWN 过滤。
8. 音效/重开/悔棋统一入口（sfx_for_move/sfx_game_over/jq_restart_game/undo 连续 pop）。
9. rebuild 前先 make so（rs_cycle 一体）。
