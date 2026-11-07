# 审计修复台账（100+ 问题全量清单，2026-11-07 4 批子代理审计）

审计批次：
- 批 1：资源对齐链路（build_phases/rebuild_rs_apk/mumu_swap_so/render.c rs_read_png/verify_asset_chain）
- 批 2：32 位分叉标记 + 审计清单闭环（jq_rs_rules.c/jq_rs_walkbridge.c/AUD_*.md）
- 批 3：构建流水线 + 蓝牙 + 音效（bt_smali/bt_session/jni_bridge sfx/mumu_regress/sign_apk）
- 批 4：渲染 UI + JNI 桥 + 工具链 + 文档（render.c/ui_menu/jni_bridge/工具脚本/SPEC 文档）

状态标记：[ ] 待修 / [x] 已修 / [~] 诚实标注 / [N/A] 撤回误报

## P0 真 bug（最高优先）

### 渲染/UI 批
- [x] P0-R1（已修：rs_render_free 删 EGL 三行（所有权归 glview_gl）+ jq_glview_destroy 先 free 再销毁 EGL） render.c:10388-10402 + glview_gl.c:316-330 rs_render_free 与 EGL 所有权矛盾（glview_gl 先 eglDestroyContext 再 rs_render_free，free 内又 eglDestroyContext/Terminate → 双销毁 + 无 current context 下删 GL 对象）
- [x] P0-R2（已修：rs_read_png ftell<0 + fread 短读/0 字节守卫（LOGI 诊断不绕过）） render.c:345-351 rs_read_png 末段 fs 二次探测：负 ftell → malloc(1)+*out_len=负值 → png_decode 越界；绕过 LOGI 诊断
- [x] P0-R3（已修：slots_up 补全 27 文字图集 + opt_text 槽（16→44 槽）） render.c:9703-9723 slots_up 统计漏 27+ 文字图集纹理槽（MISS 告警严重低估）
- [ ] P0-R4 render.c:9280+ rs_gl_rebuild 同 slot 二次 rebuild 不删旧纹理/VBO/shader → 泄漏
- [x] P0-R5（已修：on_event move g_event_total 双加 → 删重复 total++） jni_bridge.c:809-810 on_event move 分支 g_event_total 双加
- [x] P0-R6（已修：OnBluetooth 真实帧路径 g_mutex 包裹（jq_bt_session_on_frame + 懒初始化）） jni_bridge.c:7982-8000 OnBluetooth 真实帧路径无 g_mutex 改 g_state（数据竞争，CODE_STYLE §5 漂移）
- [~] P0-R7 jni_bridge.c:5694（撤回误报：UI_Dxxx 嵌套在 UI_MENU 块内，:5694 return 是 MENU fall-through 出口，:5695 } 闭合 UI_MENU 块，UI_DIFFICULTY 等是独立顶层屏幕态。括号平衡实证：行 5695 前 depth=0。原结构正确，不改。） return JNI_FALSE; 截胡所有非 D 弹窗态触摸 → UI_DIFFICULTY/NEWGAME/LOGIN/GAMESET 等弹窗按钮全失效（最高危，一行删除）
- [ ] P0-R8 jni_bridge.c:2749+ g_vs_ai 无锁写、持锁读（数据竞争，AI 介入决策）
- [x] P0-R9（已修：OnKey 返回键 pop 音效重复 → 删锁外全局 sfx_play（各分支自播）） jni_bridge.c:2712-2741 OnKey 返回键 pop 音效重复（:2733 锁内 + :2741 锁外）
- [x] P0-R10（已修：sfx_sound_save 补调用点（UI_SETTINGS 确定）+ g_sound_on 接入 sfx_play 门控 + 静音模式切换同步） jni_bridge.c:690 sfx_sound_save 全仓 0 调用点 → 音效持久化写侧死代码（g_sound_on 死变量）
- [x] P0-R1（已修：rs_render_free 删 EGL 三行（所有权归 glview_gl）+ jq_glview_destroy 先 free 再销毁 EGL）1 ui_state.c:364-440 ui_layout 系列裸读 g_lay_names/g_lay_count（OnTouch 锁内 vs 其他线程写）

### 蓝牙批
- [x] P0-B1（已修：bt_smali.sh set -o pipefail + 清 BtSpp 家族旧 smali + --verify 跳过编译段） bt_smali.sh:18,39-45 无 set -e/pipefail，javac/d8 失败被 | head 吞 → 编译失败静默合入旧 smali
- [x] P0-B2（已修：jq_bt_send 回传连接态（g_bt_connected 镜像 + 未连接 LOGI 诚实诊断）） jni_bridge.c:7826 蓝牙发送侧本地走子成功误发帧 + 假成功日志（未检查 sendFrame 连接态）
- [ ] P0-B3 jni_bridge.c:7988-7996 OnBluetooth 无锁改 g_state（= P0-R6 同一条）

### 资源批
- [x] P0-A1（已修：rebuild_rs_apk.sh v2 签名诚实边界注释 + verify 失败 abort） rebuild_rs_apk.sh:65 v2 签名 MuMu 装不上（应纯 v1 或标注仅真机 64 位）
- [x] P0-A2（已修：build_phases.sh 注释纠正（APK 资产保全非运行时路径，g_assets=NULL）） verify_asset_chain.py:84-97 "APK 自包含"措辞误导（g_assets=NULL → APK assets 是死资源，运行时单点依赖设备端 push）
- [x] P0-A3（已修：= P1-A4 同修（rebuild 推送 44 张）） rebuild_rs_apk.sh:98 推 16 张（rs168_wav 旧）非 44 张 → 新设备首跑 UI 全退化

### 分叉标记批
- [x] P0-F1（已修：R-009 改判对齐（炸弹撞旗 1000 两侧保留）+ R-057/092 改判分叉（-2 哨兵 64 位独有）） jq_rs_rules.c:485 R-009 误判（"同 bug 已修"错，炸弹撞旗 1000 两侧同源保留）+ R-057/092 误判（64 位有 -2 哨兵 32 位无，非"同源"）
- [x] P0-F2（已修：R-033/086 改判对齐（jq_flip 行 377-378 实际累加 total_moves）） jq_rs_rules.c:226-227 R-033 误判（jq_flip 行 377-378 实际累加 total_moves，非"翻棋不累加"）
- [x] P0-F3（已修：R-006 补标 0xFF 目标拦截缺失（jq_can_move 行 182 只拦己方）） jq_rs_rules.c:176-198 R-006 漏标（32 位 jq_can_move 无 dst->side==0xFF 拦截，64 位已修）
- [x] P0-F4（已修：R-004 改判分叉（行 272 pieces_left[dst->side] 无 SIDE_VALID 守卫）） jq_rs_rules.c:214-215 R-004 误判（32 位 EV_BOMB 行 271 无 SIDE_VALID 守卫，0xFF 越界写未修）
- [x] P0-F5（已修：R-035 方向纠正 + R-114 改判分叉（64 位已修 NULL 防护 32 位未同步）） jq_rs_rules.c:231 R-035 方向写反（无 stage 守卫是 32 位特性非 64 位）+ 开关缺失漏标

## P1 行为缺陷

### 资源批
- [x] P1-A1（已修：build_phases refs_n 局部化 + grep -c 不再 || echo 0（inject_spak_res 内 local refs_n）） build_phases.sh:82 refs_n=$(grep -c . || echo 0) 空文件双 0 换行污染
- [x] P1-A2（已修：build_phases verify_one 增 spak_res 条目数抽查 + 注入失败 return 1） build_phases.sh:86-92 apktool b 吞日志 + verify_one 不验 spak_res 44 张
- [x] P1-A4（已修：rebuild_rs_apk.sh 推送源改 spak_render_refs.txt 44 张（不再从 rs168 16 张推）） mumu_swap_so.sh:170 推 965 全量 vs rebuild 推 16 口径分裂
- [x] P1-A5 render.c:339-351（= P0-R2 已修：ftell<0 守卫 + fread 短读/0 字节 LOGI）
- [x] P1-A6（已修：verify_asset_chain pidof 2>/dev/null + isdigit 判定） verify_asset_chain.py:108 pidof 旧 Android not found 污染
- [x] P1-A7（已修：verify_asset_chain logcat -T 500 限窗 + 注释口径 44） verify_asset_chain.py:140-141 33 vs 44 口径矛盾 + n_ok 正则命中 2 .bin

### 分叉标记批
- [ ] P1-F1 jq_rs_rules.c 标记块 46 处行号过期（标记插入后全漂移）
- [ ] P1-F2 jq_rs_rules.c:397-418 R-005 误判（32 位 jq_side_stalemated 仍带回合门，API 契约与 64 位参数化版不等价）
- [ ] P1-F3 jq_rs_rules.c:369-382 B-3 jq_flip 无 stalemate_after 收口 + R-109 措辞失实（jq_flip 行 379 有和棋判定）
- [ ] P1-F4 jq_rs_rules.c:266-273 B-4 炸弹撞旗判胜未移植（32 位 EV_BOMB 不判胜）
- [ ] P1-F5 AUD_AUTO.md 0016-0039 无处置记录（闭环口径不一致）
- [ ] P1-F6 jq_rs_walkbridge.c:23-24 R-082 "行为等价" 把机制条当行为对齐（自相矛盾）

### 蓝牙批
- [x] P1-B5（已修：BT_T_SYNC 改 jq_state_init(peer_state)（原 peer_state=*st 语义错）） bt_session.c:94-97 BT_T_SYNC 死代码 + 语义错（peer_state=*st 拷本地）
- [x] P1-B6（已修：we_blue?OUR:OUR 死三元 → 直接 OUR_TURN） bt_session.c:53 we_blue?OUR:OUR 死三元
- [x] P1-B9（已修：UNDO 空 if → jq_bt_undo_available()==0 返 -3（诚实边界注释）） bt_session.c:71-75 UNDO 收端不回退（状态发散）
- [x] P1-B1（已修：= P0-B1 同修（bt_smali pipefail + 清 BtSpp 旧 smali + --verify 跳过编译））1 bt_smali.sh:47-48 残留旧 BtSpp$* 内部类 smali
- [x] P1-B1（已修：= P0-B1 同修（bt_smali pipefail + 清 BtSpp 旧 smali + --verify 跳过编译））4 sign_apk.sh:13-14 verify 失败仍 echo OK（假绿）
- [x] P1-B5（已修：BT_T_SYNC 改 jq_state_init(peer_state)（原 peer_state=*st 语义错））j jni_bridge.c:7826 sendFrame 假成功（= P0-B2 同一条）

### 渲染/UI 批
- [x] P1-R3（已修：rs_draw_menu UV 比例改用 rr->btn_small_w 实际宽度（原硬编码 485/666 耦合死）） render.c:1199-1212 UV 硬编码 485/666（应用 rr->btn_small_w）
- [x] P1-R4（已修：ui_menu_action_log case 3 日志纠正（联机=蓝牙 SPP 已实装，非四国占位）） ui_menu.c:106-116 action_log case 3 日志错（"联机"打"四国占位"日志）

### JNI 桥批
- [ ] P1-J1 build_phases.sh:82 refs_n 全局耦合（= P1-A1 同一条）
- [x] P1-J3（已修：verify_one phase3 adjar=0 断言） build_phases.sh:101 verify_one 不判 adjar=0（phase3 应 0）
- [x] P1-J4（已修：verify_one SRC_RS 缺失前置检查（防 grep -q 空 pattern 假绿）） build_phases.sh:112 so 哈希 SRC_RS 不存在时 grep -q "" 恒真（假绿）
- [x] P1-J5（已修：mumu_regress 重跑容错改 FLAKY 单独计数（非 PASS，exit 2 供人工复核）） mumu_regress.sh:27-30 重跑容错掩盖真回归
- [x] P1-J6（已修：redroid_test 业务 gate（TEST-GAME GS8/F_a4234 命中判据）） redroid_test.sh:77-82 判据只看崩溃/存活不查业务 miss
- [x] P1-J7（已修：= P1-A7 同修（logcat 限窗）） verify_asset_chain.py:137-147 logcat 全量历史判据（假绿）
- [x] P1-J11（已修：g_bt_sess_init 死字段删除） jni_bridge.c:785 g_bt_sess_init 死字段
- [x] P1-J12（已修：= P1-B9 同修（UNDO 栈空 rc=-3）） bt_session.c:72-74 UNDO 空 if 死代码（= P1-B9 同一条）

## P2 健壮性

### 资源批
- [ ] P2-A1 build_phases.sh:57-66 inject_spak_res refs_n 局部化（= P1-A1 衍生）
- [x] P2-A2（已修：inject_spak_res 幂等清理（rm -rf 旧 assets/spak_res）） build_phases.sh:39,60 spak_res 幂等清理缺口（rm -rf 旧 assets/spak_res）
- [ ] P2-A3 build_phases.sh:81-83 verify_one ABI 判据不按 phase 区分期望集合
- [x] P2-A4（已修：verify_asset_chain refs splitlines（CRLF 安全）） verify_asset_chain.py:95 CRLF 误计（改 splitlines）
- [x] P2-A5（已修：rebuild_rs_apk 前置 so_link_check.py 缺失检查） rebuild_rs_apk.sh:36-39 so_link_check.py 缺失误判
- [x] P2-A7（已修：= P0-R2 同修（render.c ftell/fread 守卫）） render.c:312-317 ftell -1 边界（sz<0 未防护）
- [x] P2-A9（已修：= sign_apk verify abort 同修） rebuild_rs_apk.sh:72 apksigner verify 无 abort（= P1-B14 同一条）
- [ ] P2-A6 verify_asset_chain.py:159 14 JNI magic number 无注释
- [x] P2-A8（已修：verify_asset_chain 14 JNI magic 注释） mumu_swap_so.sh:177 am start 包名矛盾（junqi vs junqirs）

### 分叉标记批
- [ ] P2-F1 jq_rs_rules.c:401-407 R-073 困毙未排地雷（标记正确，32 位行为缺陷存留）
- [ ] P2-F2 jq_rs_rules.h:122-124 rank_norm 无上界钳（64 位已修 32 位未移植）
- [ ] P2-F3 jq_rs_rules.c:462-477 终局函数无 stage 守卫 + R-114 "同源"失实
- [ ] P2-F4 jq_rs_rules.c:500-514 ai_score 无空源哨兵（= P0-F1 衍生）
- [ ] P2-F5 AUD_AUTO2.md 218 条无处置记录
- [ ] P2-F6 AUD_BUILD_PKG.md 127 条无逐条处置行
- [ ] P2-F7 jq_rs_walkbridge_test.c:68-74 期望值与推导矛盾

### 渲染/UI 批
- [x] P2-R1（已修：rs_gl_first_init 诚实标注（无兜底，px 缺失走纯色退化 + LOGI 告警）） render.c:1068-1072 rs_gl_first_init 空壳 + gl_px_loaded 假完成
- [ ] P2-R2 render.c:407-675 45×512B 栈帧爆炸 + 30 空行
- [ ] P2-R3 junqi.h:177 rank_norm 蓝方域污染（AUD-R-047 未落地）
- [ ] P2-R4 board_geom.h + glview_gl.c:114 PBuffer 1024 vs BG 1280 越界

### 蓝牙批
- [ ] P2-B13 各 evtest sleep 12/8 硬编码时序
- [ ] P2-B14 空 md5 巧合判据（= P1-J4 衍生）

### JNI 桥批
- [x] P2-J2（已修：jni_bridge (void)ny 误标删除（ny 实际被用）） jni_bridge.c:7739-7744 (void)ny 误标（ny 实际被用）
- [ ] P2-J5 jni_bridge.c:1154 OnCreate 锁内 sfx_play（低风险）
- [ ] P2-J8 pipeline_run.sh:46 evverify grep 判据格式漂移
- [x] P2-J10（已修：check_consistency evverify 单次实跑复用（原 2 次 make 浪费）） check_consistency.sh:21,37 双重 make evverify

## P3 文档/整洁

### 资源批
- [ ] P3-A1 build_phases.sh:48 "APK 自包含"注释误导（= P0-A2 衍生）
- [ ] P3-A2 render.c:324-327 "APK 兜底"注释不可达（g_assets=NULL）
- [ ] P3-A3 mumu_swap_so.sh:167-172 "全量"口径（965 vs 44）
- [ ] P3-A4 verify_asset_chain.py:140 注释 33 过期
- [ ] P3-A5 build_phases.sh:49 "962+3"口径（实际 965 含 3 张同名覆盖）
- [ ] P3-A6 rebuild_rs_apk.sh:4 注释漏 spak_res 数量
- [ ] P3-A7 build_phases.sh:29 refs_n=0 死代码

### 分叉标记批
- [ ] P3-F2 jq_rs_rules.c:15-17 R-021 "同源"为假（64 位已改措辞 32 位未改）
- [ ] P3-F3 rules_fork_check.py "1 实证"口径虚高（R-017 实为不适用）
- [ ] P3-F4 AUD_RULES.md 137 条无闭环标记行（修复痕迹在代码注释非文档）
- [ ] P3-F5 AUD_AUTO.md 头部 41/42 失配
- [ ] P3-F6 jq_rs_walkbridge.c:76 R-085 跨文件行号（行 305-307 → 实际 358）

### 蓝牙批
- [ ] P3-B17 jni_bridge.c:336 g_sound_on 死变量双轨持久化（= P0-R10 衍生）
- [ ] P3-B18 jni_bridge.c:319/540 SFX_DIR/priv_dir 双轨命名
- [x] P3-B19（已修：g_undo_snap 死代码删除） bt_session.c:11 g_undo_snap 死代码
- [ ] P3-B20 build_phases.sh:29 refs_n 全局脆弱（= P2-A1 同一条）
- [ ] P3-B21 bt_smali.sh:27-34 stub 非幂等叠加（= P0-B1 衍生）

### JNI 桥批
- [ ] P3-J2 SPEC/GUIDELINE "0-5" 措辞易误读（代码实际 0-6 含 EV_FLIP）
- [ ] P3-J3 CODE_STYLE "必须 g_mutex" vs OnBluetooth 裸读（= P0-R6 衍生）
- [ ] P3-J5 SPEC §5 漏内部存储 WAV 路径（commit 044964b1 新增）
- [ ] P3-J6 AGENTS 宣称音效持久化 vs P0-R10 死代码
- [ ] P3-J7 jni_bridge.c:2892-2912 20 行空行
- [ ] P3-J9 rs_cycle.sh:50 grep 过滤吞错误
- [x] P3-J1（已修：render.h board_gl_loaded 死字段删除） render.h:225 board_gl_loaded 死字段
- [x] P3-J2j（已修：rs_render_free 补全 50 个 *_px free（二次 create/destroy 循环防泄漏）） render.c:10396-10398 rs_render_free 漏 50 个 *_px free

## 撤回误报（子代理自纠）
- [N/A] build_phases.sh:39 dec_p0 spak_res 污染（不同树隔离，不成立）
- [N/A] build_phases.sh:82 refs_n set -e 中断（|| echo 0 兜底，不成立）
- [N/A] jni_bridge.c:7828 flip 传 to 参数（flip 不用 to，正确）
- [N/A] bt_smali.sh:47-48 cp 覆盖删除其他类（cp 非 rsync --delete，不成立）
- [N/A] jni_bridge.c:2892-2912 菜单态点空白穿透（return JNI_FALSE 符合意图，OK）
- [N/A] SPEC 棋盘 12×5 与代码一致（无漂移）
- [N/A] CODE_STYLE §3 GetMethodID 后 ExceptionCheck（合规）
- [N/A] jni_bridge.c OnTimer g_mutex 嵌套 g_ui_lock（锁序一致，无死锁）
- [N/A] OnTouch 末段 :5694（初判 P2 后升 P0-R7）
