#!/bin/bash
# §sp28 回归回退：sp28（纹理上传闭包 F_9e2b4/F_9e0f0）在 MuMu GLThread SIGSEGV → 回 sp27 稳定态
set -e
echo "[1/4] 恢复 jqt_vtbl.c = sp27 已验证版"
cp /tmp/opencode/chain/jqt_vtbl_sp27_verified.c rev64/src_jni_translated/jqt_vtbl.c
echo "[2/4] Lbf51c 豁免（幂等）"
python3 rev64/tools/render_patch.py
echo "[3/4] 编译 + 打包"
bash rev64/build_arm32_aligned.sh 2>&1 | grep -E "OK:|JNI 符号"
bash rev64/pack_arm32.sh rev64/dist/libjunqi_arm32_aligned.so 2>&1 | grep -E "verified"
cp /tmp/opencode/phase32_aligned.apk.signed rev64/dist/phase_chain_arm32.apk
echo "[4/4] 装 MuMu + 启动"
adb -s 10.0.182.146:5555 shell "am force-stop com.cnvcs.junqi; logcat -c" >/dev/null 2>&1 || true
adb -s 10.0.182.146:5555 push /tmp/opencode/phase32_aligned.apk.signed /data/local/tmp/p32sp27.apk >/dev/null 2>&1 || true
adb -s 10.0.182.146:5555 shell "su 0 pm install -r /data/local/tmp/p32sp27.apk" 2>&1 | tr -d '\r' | tail -1
adb -s 10.0.182.146:5555 shell "am start -n com.cnvcs.junqi/com.cnvcs.App" >/dev/null 2>&1
echo "完成（45s 后: adb logcat -d | grep -E 'SIGSEGV|F_9d3d4|Lbf5c2'; shot_check.py classify <png>）"
