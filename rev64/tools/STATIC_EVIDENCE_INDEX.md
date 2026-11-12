# 32 位静态佐证判据脚本索引（§124y-190~233 批次化，36 项全 PASS）

批次化入口：`bash rev64/tools/arm32_static_evidence.sh`（设备无关，离线可跑）

| 项 | 判据脚本 | 固化段 | 判据口径 |
|---|---|---|---|
| resource | rodata_path_map.py | §124y-177/181 | 原版 .rodata 44 资源路径串 → SPAK 解包 0 缺口 |
| event/urlkey/javacls | rodata_path_map.py | §124y-181 | 事件名 76 + URL 键 3（cfgHost/statHost/updateUrl）+ Java 类 静态佐证 |
| forkcheck | rules_fork_check.py | §124y-182 | 分叉矩阵 R-138 0 未标记（已标记 87 + 行为实证 16 + 分歧 1） |
| taxonomy | rodata_event_taxonomy.py | §124y-183 | 76 事件名族谱（73 Junqi + 3 Misc，%d 模板 token 口径） |
| event_cover | rodata_event_cover.py | §124y-184 | 76 事件名 → 翻译体 g_jq_strings 5345 条覆盖度 0 缺口 |
| event_codemap | event_code_map_check.py | §124y-187/196 | 73 事件名 → 12 事件码映射表 + 可再生对照 0 缺口 |
| bss_evt_tbl | bss_evt_tbl_check.py | §124y-190 | .bss 事件表 289B 真值基准（非0=129 + 高nibble=0 忠实语义） |
| rodata_static | rodata_static_check.py | §124y-192 | 原版 .rodata 明文 dump 逐字节一致（非加壳 so） |
| text_static | text_static_check.py | §124y-193 | 原版 .text 明文熵 7.10（非加密壳，与 .rodata 成对指纹） |
| rel_dyn | rel_dyn_check.py | §124y-194 | .rel.dyn RELATIVE 5312 + 类型分布 {23:5312,21:6,2:7} |
| jni_export | jni_export_check.py | §124y-195 | 原版 14 个 On*（B09-001 口径修正 15→14） |
| got_plt | got_plt_check.py | §124y-197 | 214 JUMP_SLOT + .got 符号解析槽 |
| trans_jni | translated_jni_check.py | §124y-198 | 翻译体 14 On* = 原版 14 On* 逐名一致 |
| trans_strpool | translated_strpool_check.py | §124y-199 | 翻译体内嵌事件名 ⊇ 原版 73 Junqi 0 缺口 |
| readelf_rel | readelf_rel_check.py | §124y-200 | readelf -r 5312 RELATIVE（与 §124y-194 自解析双源对照） |
| got_map | got_map_check.py | §124y-201 | plt_map.txt 214 + got_map.txt 227 产物在位 |
| needed | needed_check.py | §124y-202 | 原版 7 NEEDED 系统库 + SONAME=libjunqi.so |
| trans_needed | translated_needed_check.py | §124y-203 | 翻译体 NEEDED ⊆ 原版 7 库 + libstdc++ 已剥离 |
| walk_8step | walk_bridge_8step_check.py | §124y-204 | 走子桥 8 步序列判据锚点三源对照（源码+双脚本） |
| trans_seg | translated_seg_check.py | §124y-205 | 翻译体 so 段布局存在性（必备 4 段 + .plt + .bss=NOBITS） |
| trans_handlers | translated_handlers_check.py | §124y-206 | 翻译体 17 关键函数全在（收表 handler 13 + 主链 4） |
| hook_anchors | hook_anchors_check.py | §124y-207 | 全钩子 4 项判据锚点双源对照（源码+脚本一致防漂移） |
| spak_key | spak_key_check.py | §124y-209/210 | SPAK 解密 key 高熵在位（@VA=文件偏移 0x12a658，旧口径 0x2e5268 误值已纠正 + 双源对照 spak_unpack.py） |
| init_array | init_array_check.py | §124y-211 | 原版 so INIT_ARRAY 12 构造器清单（INIT_ARRAYSZ=48 + init[11]=0x11c10d 锚点） |
| plt_stub | plt_stub_check.py | §124y-212 | 原版 so PLT stub 公式双锚点（0x229d4+12B/条，0x23184/0x230ac 命中） |
| bss_single | bss_single_check.py | §124y-213 | 原版 so .bss 惰性单例槽 NOBITS 零初始化（单例槽偏移 0x16c 合法） |
| mapping_boundary | mapping_boundary_check.py | §124y-214 | 事件码映射表生成器不可静态实现边界（事件名散布 + 码真值非静态共置 + 双源兜底） |
| apk_so_hash | apk_so_hash_check.py | §124y-216 | 32 位翻译体 apk 内 so 哈希一致性（误打包坑 §124y-176 兜底，so+apk 同 md5） |
| touch_int | touch_int_check.py | §124y-217 | 命中几何纯整数入口（jq_touch_hit_cell_int ×1000 定点，零 float 防回退） |
| fb_ratio | fb_ratio_check.py | §124y-218 | 出屏非黑比 ≥30% 判读基准三源对照（源码 <30 跳过槽 + 判据脚本 -ge 30 不漂移） |
| walkregress_anchors | walkregress_anchors_check.py | §124y-219 | 4 路径走子回归判据锚点双源对照（源码 + arm32_walkregress.sh PATTERN 不漂移） |
| trans_abi | translated_abi_check.py | §124y-222 | 翻译体 so ABI 一致性 + 重定位槽数（ELF32/ARM + RELATIVE 2270/JUMP_SLOT 95/GLOB_DAT 6） |
| thunk_coverage | thunk_coverage_check.py | §124y-232 | F_55d40 switch case→handler 收表完整度（case 0-41 真实可达全收表 + case 42-67 扩展区 18 个 + 14 冗余 .rodata 边界忠实） |
| event_name_static | event_name_static_check.py | §124y-233 | 原版 32 位 so .rodata 73 Junqi.* 事件名静态提取（strings 直提 73 + frida-dump 二手机 73/73 对照 + event_code_map.json 12 码子集 + 模板串 %d 归一化口径） |

**设备侧判据入口**（区分静态/设备）：
- 全钩子 4 项：`bash rev64/tools/arm32_hook_check.sh --dev 10.0.182.146:5555 --rounds 3`
  （game_test/walk_bridge/native_event/framebuffer，§124y-180/188/189 flaky 根治 + 掉线容错）
- 4 路径走子回归：`bash rev64/tools/arm32_walkregress.sh`（旧入口，仍可用）

**坑清单（防复发）**：
- 32 位 ARM R_ARM_RELATIVE type=23 非 2（§124y-194 初版误用 type=2 算出 0 → 口径漂移）
- 翻译体 so 重编后须重跑 trans_jni/trans_strpool/trans_seg/trans_handlers + 重打包 apk 装机（误打包坑）
- 设备刚 reboot 转译冷启动 2-3 倍慢，判据窗口按正常 2-3 倍放宽（§124y-188）
- 转译层 JNI 反射崩溃（libart GetMethodID→strlen SEGV_MAPERR）= 环境非 so 回归（§124y-191）
- SPAK key 旧口径"文件偏移 0x2e5268"系误值（该处全 0）；实锤 32 位 so load base 0 时
  VA=文件偏移，key @ 0x12a658（§124y-209）
- .bss 单例槽偏移手算 0xc 误，实为 0x16c（§124y-213）；判据按实际相减防手算漂移
- 0x15af08 静态模板区 = Thumb 指针表（非 asr4 纯字节区），"asr4 全 0"假设不成立
  → 改"无 0..11 小常量共置"判读（§124y-214/166）
- python print 字符串含 "30%" 字面量与 % 格式化冲突 → 判据脚本含 % 须用字符串拼接（§124y-218）
- .got 段 0xaa4 = 2724 字节 = 681 槽（旧 429 误值）；原版 32 位 so **无独立 .got.plt 段**
  （.got 合并含 PLT 槽），翻译体 so 有独立 .got.plt（NDK 惯例）——对照须分段核（§124y-221）
- 翻译体 RELATIVE 2270 槽 < 原版 5312（代码量差异属预期），判核 ABI 一致 + 类型全在
  非逐值对齐（§124y-222）
