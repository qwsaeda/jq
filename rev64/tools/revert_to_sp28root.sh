#!/bin/bash
# §sp28-ROOT 回退：填字段版（F_9d3d4 填 this[0x1c/0x20/0x24]=jq_board_px/737/751 + this[0x4c]=0
# texID=0 创建路径 + F_9e204 放行）若 MuMu 崩 → 回退到本基线。
# 与 revert_to_sp27.sh（回 sp27 无填字段稳定态）区别：sp28-ROOT 是"填字段+放行创建函数"的根治版，
# 比 sp27 更靠前（字段填对不崩、纹理创建函数已就位），回退目标 = 当前已验证稳定态。
set -e
echo "[1/4] 恢复 jqt_vtbl.c = sp28-ROOT 已验证版（含填字段 + F_9e204）"
cp /tmp/opencode/chain/jqt_vtbl_sp28root_verified.c rev64/src_jni_translated/jqt_vtbl.c
cp /tmp/opencode/chain/jqt_OnRender_sp28root.c rev64/src_jni_translated/jqt_OnRender_full.c
echo "[2/4] Lbf51c 豁免（幂等）"
python3 rev64/tools/render_patch.py
echo "[3/4] 编译 + 打包"
bash rev64/build_arm32_aligned.sh 2>&1 | grep -E "OK:|JNI 符号"
bash rev64/pack_arm32.sh rev64/dist/libjunqi_arm32_aligned.so 2>&1 | grep -E "verified"
cp /tmp/opencode/phase32_aligned.apk.signed rev64/dist/phase_chain_arm32.apk
echo "[4/4] 装 MuMu + 启动"
adb -s 10.0.182.146:5555 shell "am force-stop com.cnvcs.junqi; logcat -c" >/dev/null 2>&1 || true
adb -s 10.0.182.146:5555 push /tmp/opencode/phase32_aligned.apk.signed /data/local/tmp/p32root.apk >/dev/null 2>&1 || true
adb -s 10.0.182.146:5555 shell "su 0 pm install -r /data/local/tmp/p32root.apk" 2>&1 | tr -d '\r' | tail -1
adb -s 10.0.182.146:5555 shell "am start -n com.cnvcs.junqi/com.cnvcs.App" >/dev/null 2>&1
echo "完成（45s 后: adb logcat -d | grep -E 'SIGSEGV|F_9d3d4 填字段|this\[76\]'; shot_check.py classify <png>）"
