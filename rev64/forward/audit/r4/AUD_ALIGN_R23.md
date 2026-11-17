# AUD_ALIGN_R23: 32 位翻译体源码本体审计 P0 修复 (vtable 分发表排序)
# 生成: 2026-11-17
# 承接: 32 位翻译体源码本体子代理审计 (P0×1 + P1×7 + P2×10, 其中 8 项 = 忠实兜底已标注合规)

## P0 (★ 已修, 实锤)

### P0-1 jqt_vtbl.c 分发表 4 处逆序 → 43% 表项二分静默 miss (无标注裸降级)
- **实锤 (机读)**: jqt_vtbl_offs[] 5463 项, 4 处逆序 (idx3406 0xe83ac→0xe8388 /
  idx3468 0x120ba0→0x2393c / idx3472 0x24a1c→0x249bc / idx5260 0x120b84→0xce020,
  §124y-165 扩展区批次低位偏移插进高位区尾部所致); 无序二分布局 **2375/5463 (43.5%) 值
  miss** → jqt_blx_dispatch 落 `return 0` (无兜底注释 = 无标注裸降级; 同文件特判区
  0xad248/0x32a48 均有 §/AUD 注释, 通用 miss 路径漏标)
- **判据盲区**: 4 路径回归锚 "已收表 24 handler 主链 + F_3ab64 ok=1", 主链 handler 走
  F_55d40 switch **直调** (不经 dispatch 表) → 表 43.5% 不可达当前无感; 后续 "降裸跑率"
  扩表新增 F_ 函数依赖 dispatch 可达性时会在 2375 漏网值上静默退化 0
- **修**:
  1. jqt_vtbl_offs[] + jqt_vtbl_fns[] 同步 qsort (fns 同序随行, 5463/5463 配对, 0 失配)
  2. 写回后验证: 逆序 0 / 二分 miss 0 / 表项 5463 不变
  3. 重编 so: `bash rev64/build_arm32_aligned.sh` → md5 2bf57a5b → **790fb19a** (9530152B, JNI 14 符号)
  4. **防复发门禁**: arm32_static_evidence.sh 新增 `vtable_sorted` 项 (KINDS +74) —
     纯文本解析 jqt_vtbl_offs[] 表体 (jqt_vtbl.c 为 .gitignore 排除文件, 无需编译):
     严格升序 + bisect 全值可达断言, 违序/miss → FAIL
- **实锤**: `--only vtable_sorted` ✓ (5463 项 0 逆序 / 0 miss); 全量静态佐证重跑
- **REVERSE_LOG 红线同源**: "jq_dispatch[] 必须按地址排序 (追加条目要重排, 否则
  合法 .text 目标查不到)" — 本缺陷 = 该红线在 §124y-165 扩展区批次插入时的执行遗漏

## P1 (子代理审计发现, 修复由 R23 子代理任务执行 — 进行中/已修, 见台账更新)

| 项 | 位置 | 问题 | 处置 |
|----|------|------|------|
| P1-3 | jq_rs_walkbridge.c:161-173 | jq_walk_bridge_flip 入口先读 cells[cell] 打日志再 jq_can_flip 界查, 越界 cell 先于界检查解引用 (OOB 读) | 入口界检查前置 + 拒日志 |
| P1-6 | jni_aligned.c:807 | _Znwj(0x200/0x100) 无 NULL 守卫 (OOME 返 0 → MEM(0) 写影子内存 0 区; 转译层 malloc 哨兵先例 §124w-ALIGN) | 判空 + 日志 + return |
| P1-7 | jni_aligned.c:104-137 | B05-019 注释 "每次 calloc 泄漏" 与已加 _eb_done 幂等漂移 (实为仅首次 400B) | 注释同步 |
| P2-3 | jq_rs_layout.c:123 | jq_refresh_movable_counts n 恒 0/1 非可动子数 (可走→n=1 跳出, 不可走→n++) | 改每可动子 n++ (对照 64 位) |

### P1 撤回/降级 (子代理审计内已自核)
- P1-1/P1-2 (OnCreate r!=0 路径局部引用/F_23560 返回值当 jobject): 依赖 F_23560 存根
  r0=0 语义 (314-320 行注释实锤存根返 0), r!=0 路径实际不可达 → **撤回**, 仅留注释
  "r!=0 理论路径, 存根恒 0 不可达" 标注
- P1-4 (F_3d9e8 位移除法近似): 原版 LUT 精确除 4 语义, 位移近似负数域分叉 — 登记为
  **已知近似**, 加 "★ R23 登记: 负数域位移近似 ≠ 原版 LUT 精确, 无消费点实锤 (F_3d9e8
  仅 1 调用点, 消费域 = 场景构造, 4 路径不覆盖)" 注释, 不静默
- P1-5 (F_55d40 双路径): P0-1 修后 dispatch 表全值可达 → 双路径一致性**根因消除**,
  剩 "switch 直调 + dispatch 可达" 双通路 = 设计正常 (直调快路径), 撤回缺陷判定
- P1 系列 GetMethodID 无 ExceptionClear (jni_aligned.c OnCreate 460/470/513): 对照
  64 位 R17 P0-3 同款 → **登记 P1 待修** (OnCreate 路径 FindClass 失败 pending 态,
  低风险但 64 位已修 32 位未同步), 子代理任务外登记 R24

## P2 (登记, 忠实兜底已标注合规项不计缺陷)
- P2-1 JQ_EVENT_NAMES 非 const + 6 项 vs 73: R-127 已标分叉, 合规
- P2-2 g_cur_fn 撕裂对: B05-023 已标, 诊断用途无行为影响, 合规
- P2-4 jqt_mem 满表告警 int 回绕: 不可达 (1345 万 rebuild), 低危
- P2-5 ai_score 空源无 -2 哨兵: R-057/R-092 已标 32/64 分叉, 合规
- P2-6 jqt_render_jni.c 14 包装函数死代码: 头注释自述, 体积/维护成本, 登记
- P2-7 jq_watchdog 每秒 logcat: B05-016 已标, 合规
- P2-8 jqt_OnBluetooth 存根 r0=0: R-013 已标 32 位无蓝牙帧发送, 合规 (R22 同源)
- P2-9 hit_log[64] 满后静默: 诊断限速合理
- P2-10 init_empty_containers 注释精确度: 行为正确, 注释待精修

## 台账状态
- 总条目: P0×1 + P1×4 (实修) + P1×2 (撤回) + P1×1 (登记 R24) + P2×10 (8 合规 + 2 登记)
- 已修: P0-1 (vtable 排序 + 门禁 + 重编) + P1-3/P1-6/P1-7/P2-3 (子代理任务, 见状态更新)
- 撤回: P1-1/P1-2 (存根恒 0 不可达), P1-5 (P0-1 根因消除后双通路正常)
- R24 输入: jni_aligned.c OnCreate GetMethodID ExceptionClear 3 处 (64 位 R17 P0-3 同款 32 位未同步)
- 基准级联: so md5 790fb19a → device_regress_offline_check / device_regress_check /
  apk_so_hash / dist/MANIFEST 同步 (子代理任务 1)

## 门禁基线 (R23 闭环, 2026-11-17 全实锤)
- arm32_static_evidence **74/74** (73 旧 + vtable_sorted 新增)
- so md5 **623ead77** (P1-6 OOME 守卫 + P1-3 flip 界检 + P2-3 n 语义 + P1-7 注释 4 项修复后重编, JNI 14 符号)
- phase_chain_arm32.apk 重打 → 三方一致 (dist so = apk 内 so = 基准 623ead77)
- 32 位桌面 8 步基准 **12/12** (jq_rs_walkbridge_test.c 官方测试本体实跑, P1/P2 修复后复验)
- align_gate **8/8** (64 位侧无回归, R23 只动 32 位)
- 设备 8 步全链: PENDING (10.0.182.146 离线; arm32_hook_check 离线 **rc=3** PENDING +
  首轮 ~20s 终止不逐项 4×30s 误标 so 回归)
