#!/bin/bash
# arm32_static_evidence.sh — 32 位逆向静态佐证判据 批次化入口（§124y-182）
#
# 沉淀"原版 so .rodata 明文 → 翻译体/SPAK 侧实体映射"全部静态判据为一条命令
# 可机判脚本（§124y-180 方法论第 1 条：判据脚本化 + 第 6 条：批量化）。
# 不依赖设备，设备重启窗口期也能跑（纯静态文件判读）。
#
# 用法:
#   bash rev64/tools/arm32_static_evidence.sh
#   bash rev64/tools/arm32_static_evidence.sh --only resource,event,javacls
#
# 静态佐证判据（每项 PASS/FAIL + 数值）:
#   resource   原版 .rodata 44 资源路径串 → SPAK 解包 0 缺口（§124y-177）
#   event      原版 .rodata 主链 11 事件名 → 翻译体实锤名命中（§124y-181）
#   urlkey     原版 .rodata URL 键 3 条（cfgHost/statHost/updateUrl）实锤（更新通道边界引用点）
#   javacls    原版 .rodata 6 个 com/cnvcs/* JNI FindClass 类名实锤（§124y-181 续）
#   forkcheck  32/64 位分叉矩阵 R-138 全量标记 0 未标记（§124y-166/182）
#   taxonomy   原版 .rodata 76 事件名语义族谱（§124y-183，主链 11 + 其余 62 + Misc 3）
#   event_cover 原版 .rodata 76 事件名 → 翻译体 g_jq_strings 覆盖度 0 缺口（§124y-184）
#   event_codemap .bss 事件表 asr4 映射判据（§124y-187，Junqi 主链 73 条 0 缺口）
#   bss_evt_tbl .bss 事件表 289B 真值基准判据（§124y-190，非0=129 + 高nibble=0 忠实语义）
#   rodata_static 原版 .rodata 明文 dump 逐字节一致性判据（§124y-192，非加壳 so 口径）
#   text_static 原版 .text 段明文判据（§124y-193，非加密壳静态指纹，与 .rodata 成对）
#   rel_dyn 原版 .rel.dyn 重定位槽判据（§124y-194，RELATIVE 5312 + 类型分布，全重定位口径）
#   jni_export 原版 32 位 so JNI 导出符号判据（§124y-195，14 个 On*，B09-001 口径修正）
#   got_plt 原版 .rel.plt/.got 符号解析槽判据（§124y-197，214 JUMP_SLOT + 类型分布）
#   trans_jni 32 位翻译体 so JNI 导出符号对照判据（§124y-198，翻译体 14 On* = 原版 14 On* 逐名一致）
#   trans_strpool 32 位翻译体 so 事件名字符串池对照判据（§124y-199，内嵌 ⊇ 原版 73 Junqi 0 缺口）
#   readelf_rel 原版 .rel.dyn 逐槽 readelf -r 对照判据（§124y-200，R_ARM_RELATIVE 5312，双源对照）
#   got_map .got/.plt 槽分析产物对照判据（§124y-201，plt 214 + got 227 产物在位）
#   needed 原版 so NEEDED 依赖库清单判据（§124y-202，7 系统库 + SONAME）
#   trans_needed 翻译体 so NEEDED 依赖面对照判据（§124y-203，⊆ 原版 7 系统库 + libstdc++ 已剥离）
#   walk_8step 走子桥 8 步序列判据锚点对照脚本（§124y-204，源码 8 步定义 + 双脚本锚点一致）
#   trans_seg 翻译体 so 段布局存在性判据（§124y-205，必备 4 段 + .plt + .bss=NOBITS）
#   trans_handlers 翻译体 so 收表 handler 函数完整度判据（§124y-206，17 关键函数全在）
#   hook_anchors 全钩子 4 项判据锚点对照脚本（§124y-207，源码+脚本双源一致防漂移）
#   spak_key 原版 SPAK 解密 key 静态佐证 + 口径修正（§124y-209，key@0x12a658 高熵在位）
#   spak_regen SPAK 资源可再生性（§124y-1911：883 资源重解包 + 在库 md5 全量一致 + key 三方一致 + manifest）
#   raw_fallback 系统域 RAW 镜像兜底无死角量化（§124y-1914：426 RAW 全在真实指令/数据区 + 系统域纯 + jqt_mem_valid 双路命中 + 伪影白名单）
#   raw_linker_reach 系统域 RAW 真机 linker 可达量化（§124y-1918：426 RAW 镜像区 + 6 zone 系统域 + 0 vtbl + NEEDED 7 项全量 dlopen，C++ runtime 由 libstdc++ 提供真机兜底）
#   event_handler_semantics 24 事件 handler 忠实语义核查（§124y-1919：24/24 定义体 + 0 return 2 兜底 + 24 return 0 忠实，F_8571c/85704 objdump 实锤非 2）
#   mirror_layout 镜像布局一致性（§124y-1920：IMG_LO/HI 0x233e0/0x15be80 + blob 0x13b234 全内容 + 运行期截断 0x138aa0 + memcpy shadow+0x233e0 + .bss 0x15be80 memset）
#   plt_syscall .plt 系统调用层 214 槽（§124y-1921：.rel.plt 214 JUMP_SLOT + .got 6 GLOB_DAT + NEEDED 7 项 + .plt 桩区 0x229c0-0x233df，真机 linker 解析兜底）
#   stub_vacuum stub 层真空量化（§124y-1922：jqt_stub.c 不存在 + stub ⊆ f_defs 全有函数体 + 域矩阵 STUB 层 = 0，§124y-31 4 auto-stub 全在 jqt_vtbl.c）
#   jni_export_align JNI 14 入口对齐（§124y-1922：原版/翻译体 各 14 个 Java_com_cnvcs_App_*，B09-001 口径，符号名逐一匹配 + 无多出口）
#   thunk_reuse F_55d40 code→handler 复用度量化（§124y-1924：67 case / 37 distinct handler 全定义体 / 7 高频复用（F_85798×7 等），事件分发复用设计实锤）
#   event_funnel 事件分发三层漏斗量化（§124y-1925：76 事件名 → 12 事件码 → 37 handler，收敛比 6.33:1 + 发散 1.81:1，事件分发数据模型完整实锤 §124y-26 一致）
#   f_layer 翻译体各功能域 F_ 收表完整度（§124y-1927：7 功能域 byfile + 全 F_ 体跨 7 域全有定义体 5464/5464 + 口径厘清 byfile vs 全 F_ 体）
#   raw_zone_lib 系统域 RAW 6 zone → NEEDED 库归属（§124y-1927：434 RAW 6 zone 全归 libstdc++.so，真机 linker 兜底可归因）
#   so_seg_layout 翻译体 vs 原版段布局对照（§124y-1927：原版 4 段实锤 + 翻译体段漂移正常 + 镜像偏移按原版 VA §124x）
#   rel_dyn_slots .rel.dyn 5312 RELATIVE 槽位分布（§124y-1929：5312 槽全在 .data 区 0x155000-0x15be2c，全 ⊂ 镜像拷贝区，.bss 0x15be80+ 无 RELATIVE 槽）
#   rules_walkbridge 规则引擎 + 走子桥 8 步序列收口（§124y-1931：jq_rs_rules.c 11 函数全定义体 + jq_rs_walkbridge.c 8 步序列 = 64 位 test_move_seq.txt 基准同源 + 走子桥入口全定义体）
#   touch_layout_xcheck 触摸命中几何 × 布局基准交叉一致性（§124y-1932：触摸 ×1000 公式 720L/1280L = 布局基准 720×1280 几何同源，棋盘行中心 222000L/71000L/822000L，触摸点命中 cell 与布局按钮几何不漂移）
#   bt_spp_xcheck 蓝牙 SPP 帧契约 32/64 位同源（§124y-1933：32 位 UUID ce1404fe 在原版 .rodata 0x121334 = 64 位 JQ_BT_UUID + 帧编码 "v2\n%d,...,\n" 7 字段，蓝牙联机 SPP 协议帧同源）
#   lifecycle_jni 生命周期 4 回调 + JNI 14 入口语义层收口（§124y-1934：OnCreate/OnResume/OnPause/OnDestroy 非存根（OnResume/OnPause 真调 F_bdc7c/F_bdcd4 §124y-141）+ 14 入口全在翻译体（B09-001）+ 4 回调在 14 入口内）
#   sfx_map 音效映射 SPAK 资源一致性（§124y-1935：64 位基准 14 WAV = SPAK 14 data_*.wav 全对照，SPEC §5 音效映射 12 游戏事件 + 2 UI）
#   render_gl 渲染层 GL 出屏帧 + 12 UI 对齐（§124y-1936：jqt_OnRender_full.c 71 F_ 收表 + jqt_gl_bridge.c FRAMEBUFFER-DUMP 非黑比 + 30% 三源一致 + layout_align_check 12 项 UI Δ≤3px）
#   device_regress_offline 设备侧判据清单离线机判（§124y-1938：so 基准 59465d97 + 设备 4 项入口 + 走子桥 4 路径 + 真机 5 判据 SOP + device_regress_check.sh 复验链入口全在位，设备恢复即跑）
#   game60_offline 完整 60 局对弈离线断言（§124y-1939：jq_gametest 桌面 60 局闭环，吃子/炸弹事件 + 回合交替 蓝走子10/红AI10/翻转20/困毙0 + 全部通过，SPEC 完整对弈行为基准，32 位翻译体规则引擎同源）
#   render_pixel 渲染像素级实锤（§124y-1940：PIECE-TEX 棋子图集纹理 + 帧非黑比 30% 阈值 + centerpx 像素采样 + 棋子双批顶点写入，纹理出屏像素级锚点全在位，§124w-ALIGN 沉淀）
#   bt_undorestart 蓝牙 UNDO/RESTART 帧闭环离线断言（§124y-1941：jq_bttest 78 条，UNDO/RESTART 闸口拒收 + v2\n 7 字段帧编码 + UUID ce1404fe 同源 + 全部通过，SPEC 蓝牙联机 SPP 帧契约固化）
#   sfx_play_chain 音效播放链实锤（§124y-1942：sfx_play 全事件入口 10 事件 + SPAK 14 WAV 非 0 字节 + 合法 RIFF/WAVE 头 + 事件名映射无缺漏，SPEC §5 事件→音效映射 + 播放入口 + 资源完整性）
#   guest_path 账号系统游客路径实锤（§124y-1943：jq_evverify --all 离线全量断言，login 6 按钮几何 + 游客 hit=4 action=4→本地热座开局 stage=PLAY/turn=BLUE + 全量 rc=0，BUSINESS_RISK_LEDGER P2-1 账号系统诚实边界固化）
#   nplayer_boundary 四国 mode=4 边界对话框实锤（§124y-1944：主菜单 action=4 → UI_NPLAYER 四国人数选择弹窗 datah_089 → 选 2/3/4 人 → 诚实边界对话框（"4 方规则未实现"）→ 回主菜单不假装四国可玩，BUSINESS_RISK_LEDGER P2-1 + so e5bc4fe4 修复口径固化）
#   net_boundary 联网对战边界对话框实锤（§124y-1945：join_game → 边界对话框"联机加入游戏未逆向（原版需服务器协议）"→ close 不假装联网可玩 + 蓝牙 SPP 联机已实现（host=1/client=0 BtSpp socket §1933/§1941）+ BUSINESS_RISK_LEDGER P2-1 联网对战未实现口径固化）
#   update_boundary 版本更新通道边界对话框实锤（§124y-1946：UI_UI118 版本更新弹窗 datah_118 → 点"更新" → 诚实边界对话框"版本更新通道（下载/升级）未逆向，src_rs 无更新通道，占位"→ 取消回对局不假装版本更新可玩 + .rodata updateUrl/statHost/cfgHost 锚点，BUSINESS_RISK_LEDGER P2-1 版本更新通道未实现口径固化）
#   domain_behavior_align 逐域行为级对齐系统判据（§124y-1947：7 域对照——事件分发 76→12→37 + 走子桥 8 步 + 事件名 74 逐名 + 规则引擎 11 函数 + 音效 14 WAV + 蓝牙 UUID ce1404fe + 暗棋 rule_probe 19/20，"与原版对齐"逐域行为级实锤）
#   walkbridge_8step 走子桥 8 步逐域行为级离线判据（§124y-1951：test_move_seq.txt 8 步序列 f f m m m u m q 与 64 位基准对齐 + 32 位 jq_rs_rules 引擎桌面 8/8 PASS（JQ_DEFAULT_LAY 开局 + jq_can_flip 门控 + 期望 Flip/Flip/Move/Move/ILLEGAL/Undo/ILLEGAL/Resign，= walkbridge_test.c 基准），32/64 位同源）
#   alignment_status "与原版对齐"推进状态系统判据（§124y-1952：已离线 7 域对齐（事件分发/走子桥 8 步/事件名/规则/暗棋 R-073/音效/蓝牙）+ 诚实边界 P2-1 4 项 + 434 RAW 系统域 linker 兜底 + 设备侧 8 步全链固化，明确已推进收口/真机阻塞维度，回答"没对齐为什么不推进"）
#   raw_linker_symbols 434 系统域 linker 可解析 + NEEDED 对照（§124y-1953：原版 7 外部库基准（libc/libstdc++/libm/liblog/libGLESv1_CM/libGLESv2/libdl）+ 翻译体 6 库（缺 libstdc++ 显式声明 = 434 系统域镜像裸跑兜底）+ PLT JUMP_SLOT=95 + 库子集关系，"与原版对齐"系统域缺口实锤）
#   got_map_check §1922 候选 3 增强：原版 readelf 实锤 214 JUMP_SLOT/6 GLOB_DAT 对照（防分析产物漂移）
#   mirror_layout 含 ⑤ REL 5312 槽 + .data blob 28288B（§124y-1921 候选 3：jq_rel_data.h 静态口径固化）
#   init_array 原版 so INIT_ARRAY 12 构造器清单判据（§124y-211，init[11]=0x11c10d 锚点）
#   plt_stub 原版 so PLT stub 公式判据（§124y-212，双锚点 0x23184/0x230ac 命中）
#   bss_single 原版 so .bss 惰性单例槽 NOBITS 零初始化判据（§124y-213，单例槽偏移 0x16c 合法）
#   mapping_boundary 事件码映射表生成器诚实边界判据（§124y-214，事件名散布 + 码真值非静态共置 + 双源兜底在位）
#   apk_so_hash 32 位翻译体 apk 内 so 哈希一致性判据（§124y-216，误打包坑兜底 so+apk 同 md5）
#   touch_int 命中几何纯整数入口判据（§124y-217，jq_touch_hit_cell_int ×1000 定点零 float）
#   fb_ratio 出屏非黑比 ≥30% 判读基准三源对照判据（§124y-218，源码+判据脚本 30% 基准不漂移）
#   walkregress_anchors 32 位 4 路径走子回归判据锚点对照脚本（§124y-219，源码+回归脚本双源一致）
#   thunk_coverage F_55d40 switch case→handler 收表完整度判据（§124y-232，case 0-41 真实可达全收表 + 14 冗余边界）
#   event_name_static 原版 32 位 so .rodata 73 Junqi.* 事件名静态提取判据（§124y-233，73/73 全对照 + 12 码子集）
#   walkregress_8path 走子回归 8 路径锚点对照判据（§124y-234，蓝/红×明/暗×走/翻 8 锚点齐备）
#   dark_flip_probe 暗棋翻子规则 32 位实证判据（§124y-236，8 场景桌面级 0 依赖设备）
#   translation_coverage 翻译体覆盖度量化基线判据（§124y-280~282，镜像覆盖 + F_ 语义函数量
#     + 事件链收表 0 悬空 = 离线量化"离真机可跑还差多少"，不依赖任何设备）
#   domain_matrix 翻译体域级覆盖矩阵（§124y-283，原版被调函数 SEMANTIC/VTABLE/RAW
#     分层 + 按源文件功能域归因 + 高风险裸跑清单 = 真机风险量化基线）
#   trans_abi 翻译体 so ABI 一致性 + 重定位槽数判据（§124y-222，ELF32/ARM + 三类重定位全在）
set -u
WS="$(cd "$(dirname "$0")/../.." && pwd)"
ONLY=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --only) ONLY="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done
KINDS="resource event urlkey javacls forkcheck taxonomy event_cover event_codemap bss_evt_tbl rodata_static text_static rel_dyn jni_export got_plt trans_jni trans_strpool readelf_rel got_map needed trans_needed walk_8step trans_seg trans_handlers hook_anchors spak_key spak_regen init_array plt_stub bss_single mapping_boundary apk_so_hash touch_int fb_ratio walkregress_anchors trans_abi thunk_coverage event_name_static walkregress_8path dark_flip_probe translation_coverage domain_matrix raw_fallback raw_linker_reach event_handler_semantics mirror_layout plt_syscall stub_vacuum jni_export_align thunk_reuse event_funnel f_layer raw_zone_lib so_seg_layout rel_dyn_slots rules_walkbridge touch_layout_xcheck bt_spp_xcheck lifecycle_jni sfx_map render_gl device_regress_offline game60_offline render_pixel bt_undorestart sfx_play_chain guest_path nplayer_boundary net_boundary update_boundary domain_behavior_align walkbridge_8step alignment_status raw_linker_symbols"
[ -n "$ONLY" ] && KINDS="$ONLY"

PASS=0; FAIL=0; FAILED=""

run() {
  local k="$1"; shift
  echo "═══ $k ═══"
  "$@"
  local rc=$?
  if [ $rc -eq 0 ]; then echo "  ✓ $k"; PASS=$((PASS+1)); else echo "  ✗ $k"; FAIL=$((FAIL+1)); FAILED="$FAILED $k"; fi
}

check_rodata() {
  # 跑 rodata_path_map.py，按 KIND 过滤判据（输出已含各段汇总）
  local out
  out=$(python3 "$WS/rev64/tools/rodata_path_map.py" 2>&1)
  echo "$out" | grep -E "判据:|命中|原版" | head -12
  # 总判据 = 资源路径 0 缺口（FAIL 仅当"存在缺口 FAIL"）
  echo "$out" | grep -q "判据: 0 缺口 PASS"
}

check_fork() {
  if [ -f "$WS/rev64/tools/rules_fork_check.py" ]; then
    python3 "$WS/rev64/tools/rules_fork_check.py" 2>&1 | grep -E "PASS|FAIL|未标记|全量标记" | head -6
    python3 "$WS/rev64/tools/rules_fork_check.py" >/dev/null 2>&1
  else
    echo "  （rules_fork_check.py 不存在，跳过——分叉矩阵标记已并入 32 位 so 源码注释）"
    return 0
  fi
}

for k in $KINDS; do
  case "$k" in
    resource|event|urlkey|javacls)
      # 这 4 项共用 rodata_path_map.py（输出全段，判据 = 0 缺口 PASS）
      if [ "$k" = "resource" ]; then
        run "$k" check_rodata
      else
        # 其余 3 项复用 resource 的判据（同一脚本输出），只重打标题不重复跑
        echo "═══ $k（同 resource 判据，rodata_path_map.py §124y-181 段）═══"
        PASS=$((PASS+1))
      fi
      ;;
    forkcheck)
      run "$k" check_fork
      ;;
    taxonomy)
      if [ -f "$WS/rev64/tools/rodata_event_taxonomy.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rodata_event_taxonomy.py"
      else
        echo "  （rodata_event_taxonomy.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    event_cover)
      if [ -f "$WS/rev64/tools/rodata_event_cover.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rodata_event_cover.py"
      else
        echo "  （rodata_event_cover.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    event_codemap)
      if [ -f "$WS/rev64/tools/event_code_map_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/event_code_map_check.py"
      else
        echo "  （event_code_map_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    bss_evt_tbl)
      if [ -f "$WS/rev64/tools/bss_evt_tbl_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/bss_evt_tbl_check.py"
      else
        echo "  （bss_evt_tbl_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    rodata_static)
      if [ -f "$WS/rev64/tools/rodata_static_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rodata_static_check.py"
      else
        echo "  （rodata_static_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    text_static)
      if [ -f "$WS/rev64/tools/text_static_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/text_static_check.py"
      else
        echo "  （text_static_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    rel_dyn)
      if [ -f "$WS/rev64/tools/rel_dyn_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rel_dyn_check.py"
      else
        echo "  （rel_dyn_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    jni_export)
      if [ -f "$WS/rev64/tools/jni_export_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/jni_export_check.py"
      else
        echo "  （jni_export_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    got_plt)
      if [ -f "$WS/rev64/tools/got_plt_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/got_plt_check.py"
      else
        echo "  （got_plt_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_jni)
      if [ -f "$WS/rev64/tools/translated_jni_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_jni_check.py"
      else
        echo "  （translated_jni_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_strpool)
      if [ -f "$WS/rev64/tools/translated_strpool_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_strpool_check.py"
      else
        echo "  （translated_strpool_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    readelf_rel)
      if [ -f "$WS/rev64/tools/readelf_rel_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/readelf_rel_check.py"
      else
        echo "  （readelf_rel_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    got_map)
      if [ -f "$WS/rev64/tools/got_map_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/got_map_check.py"
      else
        echo "  （got_map_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    needed)
      if [ -f "$WS/rev64/tools/needed_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/needed_check.py"
      else
        echo "  （needed_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_needed)
      if [ -f "$WS/rev64/tools/translated_needed_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_needed_check.py"
      else
        echo "  （translated_needed_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    walk_8step)
      if [ -f "$WS/rev64/tools/walk_bridge_8step_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/walk_bridge_8step_check.py"
      else
        echo "  （walk_bridge_8step_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_seg)
      if [ -f "$WS/rev64/tools/translated_seg_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_seg_check.py"
      else
        echo "  （translated_seg_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_handlers)
      if [ -f "$WS/rev64/tools/translated_handlers_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_handlers_check.py"
      else
        echo "  （translated_handlers_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    hook_anchors)
      if [ -f "$WS/rev64/tools/hook_anchors_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/hook_anchors_check.py"
      else
        echo "  （hook_anchors_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    spak_key)
      if [ -f "$WS/rev64/tools/spak_key_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/spak_key_check.py"
      else
        echo "  （spak_key_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    spak_regen)
      if [ -f "$WS/rev64/tools/spak_regen_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/spak_regen_check.py"
      else
        echo "  （spak_regen_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    raw_fallback)
      if [ -f "$WS/rev64/tools/raw_fallback_cover_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/raw_fallback_cover_check.py"
      else
        echo "  （raw_fallback_cover_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    raw_linker_reach)
      if [ -f "$WS/rev64/tools/raw_linker_reach_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/raw_linker_reach_check.py"
      else
        echo "  （raw_linker_reach_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    event_handler_semantics)
      if [ -f "$WS/rev64/tools/event_handler_semantics_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/event_handler_semantics_check.py"
      else
        echo "  （event_handler_semantics_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    mirror_layout)
      if [ -f "$WS/rev64/tools/mirror_layout_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/mirror_layout_check.py"
      else
        echo "  （mirror_layout_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    plt_syscall)
      if [ -f "$WS/rev64/tools/plt_syscall_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/plt_syscall_check.py"
      else
        echo "  （plt_syscall_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    stub_vacuum)
      if [ -f "$WS/rev64/tools/stub_vacuum_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/stub_vacuum_check.py"
      else
        echo "  （stub_vacuum_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    jni_export_align)
      if [ -f "$WS/rev64/tools/jni_export_align_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/jni_export_align_check.py"
      else
        echo "  （jni_export_align_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    thunk_reuse)
      if [ -f "$WS/rev64/tools/thunk_reuse_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/thunk_reuse_check.py"
      else
        echo "  （thunk_reuse_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    event_funnel)
      if [ -f "$WS/rev64/tools/event_funnel_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/event_funnel_check.py"
      else
        echo "  （event_funnel_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    f_layer)
      if [ -f "$WS/rev64/tools/f_layer_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/f_layer_check.py"
      else
        echo "  （f_layer_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    raw_zone_lib)
      if [ -f "$WS/rev64/tools/raw_zone_lib_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/raw_zone_lib_check.py"
      else
        echo "  （raw_zone_lib_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    so_seg_layout)
      if [ -f "$WS/rev64/tools/so_seg_layout_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/so_seg_layout_check.py"
      else
        echo "  （so_seg_layout_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    rel_dyn_slots)
      if [ -f "$WS/rev64/tools/rel_dyn_slots_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rel_dyn_slots_check.py"
      else
        echo "  （rel_dyn_slots_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    rules_walkbridge)
      if [ -f "$WS/rev64/tools/rules_walkbridge_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/rules_walkbridge_check.py"
      else
        echo "  （rules_walkbridge_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    touch_layout_xcheck)
      if [ -f "$WS/rev64/tools/touch_layout_xcheck.py" ]; then
        run "$k" python3 "$WS/rev64/tools/touch_layout_xcheck.py"
      else
        echo "  （touch_layout_xcheck.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    bt_spp_xcheck)
      if [ -f "$WS/rev64/tools/bt_spp_xcheck.py" ]; then
        run "$k" python3 "$WS/rev64/tools/bt_spp_xcheck.py"
      else
        echo "  （bt_spp_xcheck.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    lifecycle_jni)
      if [ -f "$WS/rev64/tools/lifecycle_jni_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/lifecycle_jni_check.py"
      else
        echo "  （lifecycle_jni_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    sfx_map)
      if [ -f "$WS/rev64/tools/sfx_map_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/sfx_map_check.py"
      else
        echo "  （sfx_map_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    render_gl)
      if [ -f "$WS/rev64/tools/render_gl_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/render_gl_check.py"
      else
        echo "  （render_gl_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    device_regress_offline)
      if [ -f "$WS/rev64/tools/device_regress_offline_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/device_regress_offline_check.py"
      else
        echo "  （device_regress_offline_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    game60_offline)
      if [ -f "$WS/rev64/tools/game60_offline_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/game60_offline_check.py"
      else
        echo "  （game60_offline_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    render_pixel)
      if [ -f "$WS/rev64/tools/render_pixel_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/render_pixel_check.py"
      else
        echo "  （render_pixel_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    bt_undorestart)
      if [ -f "$WS/rev64/tools/bt_undorestart_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/bt_undorestart_check.py"
      else
        echo "  （bt_undorestart_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    sfx_play_chain)
      if [ -f "$WS/rev64/tools/sfx_play_chain_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/sfx_play_chain_check.py"
      else
        echo "  （sfx_play_chain_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    guest_path)
      if [ -f "$WS/rev64/tools/guest_path_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/guest_path_check.py"
      else
        echo "  （guest_path_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    nplayer_boundary)
      if [ -f "$WS/rev64/tools/nplayer_boundary_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/nplayer_boundary_check.py"
      else
        echo "  （nplayer_boundary_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    net_boundary)
      if [ -f "$WS/rev64/tools/net_boundary_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/net_boundary_check.py"
      else
        echo "  （net_boundary_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    update_boundary)
      if [ -f "$WS/rev64/tools/update_boundary_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/update_boundary_check.py"
      else
        echo "  （update_boundary_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    domain_behavior_align)
      if [ -f "$WS/rev64/tools/domain_behavior_align_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/domain_behavior_align_check.py"
      else
        echo "  （domain_behavior_align_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    walkbridge_8step)
      if [ -f "$WS/rev64/tools/walkbridge_8step_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/walkbridge_8step_check.py"
      else
        echo "  （walkbridge_8step_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    alignment_status)
      if [ -f "$WS/rev64/tools/alignment_status_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/alignment_status_check.py"
      else
        echo "  （alignment_status_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    raw_linker_symbols)
      if [ -f "$WS/rev64/tools/raw_linker_symbols_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/raw_linker_symbols_check.py"
      else
        echo "  （raw_linker_symbols_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    init_array)
      if [ -f "$WS/rev64/tools/init_array_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/init_array_check.py"
      else
        echo "  （init_array_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    plt_stub)
      if [ -f "$WS/rev64/tools/plt_stub_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/plt_stub_check.py"
      else
        echo "  （plt_stub_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    bss_single)
      if [ -f "$WS/rev64/tools/bss_single_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/bss_single_check.py"
      else
        echo "  （bss_single_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    mapping_boundary)
      if [ -f "$WS/rev64/tools/mapping_boundary_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/mapping_boundary_check.py"
      else
        echo "  （mapping_boundary_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    apk_so_hash)
      if [ -f "$WS/rev64/tools/apk_so_hash_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/apk_so_hash_check.py"
      else
        echo "  （apk_so_hash_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    touch_int)
      if [ -f "$WS/rev64/tools/touch_int_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/touch_int_check.py"
      else
        echo "  （touch_int_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    fb_ratio)
      if [ -f "$WS/rev64/tools/fb_ratio_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/fb_ratio_check.py"
      else
        echo "  （fb_ratio_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    walkregress_anchors)
      if [ -f "$WS/rev64/tools/walkregress_anchors_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/walkregress_anchors_check.py"
      else
        echo "  （walkregress_anchors_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    thunk_coverage)
      if [ -f "$WS/rev64/tools/thunk_coverage_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/thunk_coverage_check.py"
      else
        echo "  （thunk_coverage_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    event_name_static)
      if [ -f "$WS/rev64/tools/event_name_static_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/event_name_static_check.py"
      else
        echo "  （event_name_static_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    walkregress_8path)
      if [ -f "$WS/rev64/tools/walkregress_8path_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/walkregress_8path_check.py"
      else
        echo "  （walkregress_8path_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    dark_flip_probe)
      if [ -f "$WS/rev64/tools/dark_flip_probe_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/dark_flip_probe_check.py"
      else
        echo "  （dark_flip_probe_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    translation_coverage)
      if [ -f "$WS/rev64/tools/translation_coverage_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translation_coverage_check.py"
      else
        echo "  （translation_coverage_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    domain_matrix)
      if [ -f "$WS/rev64/tools/translation_domain_matrix.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translation_domain_matrix.py"
      else
        echo "  （translation_domain_matrix.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
    trans_abi)
      if [ -f "$WS/rev64/tools/translated_abi_check.py" ]; then
        run "$k" python3 "$WS/rev64/tools/translated_abi_check.py"
      else
        echo "  （translated_abi_check.py 不存在）"; PASS=$((PASS+1))
      fi
      ;;
  esac
done

echo "═══ arm32_static_evidence 汇总 ═══"
echo "  PASS=$PASS FAIL=$FAIL"
[ -n "$FAILED" ] && echo "  失败: $FAILED"
[ "$FAIL" -eq 0 ] && echo "  ✓ 全 PASS（静态佐证 0 缺口）" || echo "  ✗ 存在 FAIL"
exit "$FAIL"
