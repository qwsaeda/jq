#!/usr/bin/env bash
# build_phase23_so.sh — 重编 phase2/3 的 arm64 so，使 JNI 符号与 com.cnvcs 包名对齐
#
# 定位：phase2/3 APK 包名/Java 类 = com.cnvcs（MANIFEST 旧口径起点），但注入的
#   src_rs so 导出 Java_com_qwsaeda_App_* → JNI 绑定断裂 = UnsatisfiedLinkError（实锤）。
#   本脚本：源码临时副本 sed com.qwsaeda→com.cnvcs（不动原树，守正向会话红线）→ NDK 重编
#   → 构建树注入 → apktool b → STORE → zipalign → 双签 → 校验 dex/so 前缀一致。
#
# 用法：bash rev64/tools/build_phase23_so.sh
set -uo pipefail
WS=/disk/workspaces/jq
RA=/tmp/opencode/jq_apk/rs_a64
SRC=$WS/rev64/forward/src_rs
NDK=/tmp/opencode/android-ndk-r25b
TMP=/tmp/opencode/phase23_so_build
ZO=/tmp/opencode/phase23_cnvcs.so

echo "════════ phase2/3 arm64 so 重编（com.cnvcs JNI 前缀）════════"
rm -rf "$TMP" && mkdir -p "$TMP"
cp -a "$SRC"/. "$TMP"/ || exit 1
n=0
for f in "$TMP"/*.c "$TMP"/*.h; do
  [ -f "$f" ] || continue
  if grep -q "com/qwsaeda\|com_qwsaeda\|com\.qwsaeda" "$f" 2>/dev/null; then
    sed -i 's|com/qwsaeda|com/cnvcs|g; s|com_qwsaeda|com_cnvcs|g; s|com\.qwsaeda|com.cnvcs|g' "$f"
    n=$((n+1))
  fi
done
echo "  源码改前缀文件数: $n"
cd "$TMP" || exit 1
make clean >/dev/null 2>&1 || true
NDK="$NDK" make so >/tmp/opencode/phase23_so_build.log 2>&1
[ $? -eq 0 ] || { echo "✗ 编译失败"; grep -E "error:" /tmp/opencode/phase23_so_build.log | head -5; exit 1; }
cp "$TMP/dist/libjunqi_rs.so" "$ZO"
CN=$(nm -D "$ZO" 2>/dev/null | grep -c "Java_com_cnvcs_App_" || true)
QW=$(nm -D "$ZO" 2>/dev/null | grep -c "Java_com_qwsaeda" || true)
echo "  so: $(stat -c%s "$ZO")B md5=$(md5sum "$ZO"|cut -d' ' -f1) | JNI cnvcs=$CN qwsaeda=$QW"
[ "$CN" = "14" ] && [ "$QW" = "0" ] || { echo "✗ JNI 前缀不符"; exit 1; }

# 构建树注入 cnvcs so（两棵树）
for tree in dec_dual dec_full; do
  T=$RA/$tree
  [ -d "$T" ] || { echo "✗ 构建树缺失 $T"; exit 1; }
  cp "$ZO" "$T/lib/arm64-v8a/libjunqi.so"
  echo "  注入 $tree/lib/arm64-v8a/libjunqi.so"
done

# 打包 phase2/phase3（apktool b → STORE → zipalign → sign，复用 build_phases 的独立步骤）
for phase in phase2 phase3; do
  case $phase in phase2) T=$RA/dec_dual;; phase3) T=$RA/dec_full;; esac
  echo "  ── 打包 $phase ($T) ──"
  apktool b "$T" -o "$RA/${phase}_raw.apk" >/tmp/opencode/phase23_${phase}_apktool.log 2>&1 || { echo "✗ apktool b 失败"; tail -5 /tmp/opencode/phase23_${phase}_apktool.log; exit 1; }
  python3 "$WS/rev64/tools/store_compressed.py" "$RA/${phase}_raw.apk" "$RA/${phase}_stored.apk" >/dev/null 2>&1 || exit 1
  zipalign -f -p 4 "$RA/${phase}_stored.apk" "$RA/${phase}_aligned.apk" || exit 1
  bash "$WS/rev64/sign_apk.sh" "$RA/${phase}_aligned.apk" "$RA/${phase}_signed.apk" >/tmp/opencode/phase23_${phase}_sign.log 2>&1 || { echo "✗ 签名失败"; tail -5 /tmp/opencode/phase23_${phase}_sign.log; exit 1; }
  cp "$RA/${phase}_signed.apk" "$WS/dist/$phase.apk"
  echo "  ✓ $phase.apk $(stat -c%s "$WS/dist/$phase.apk")B md5=$(md5sum "$WS/dist/$phase.apk"|cut -d' ' -f1)"
done

# 终校验：dex 前缀 = so 前缀 = com.cnvcs
cd "$WS" || exit 1
python3 - <<'PY'
import zipfile, subprocess
for phase in ('phase2','phase3'):
    z=zipfile.ZipFile('dist/%s.apk'%phase)
    dex=z.read('classes.dex')
    so=z.read('lib/arm64-v8a/libjunqi.so')
    open('/tmp/opencode/_v.so','wb').write(so)
    qw=nm=subprocess.run(['nm','-D','/tmp/opencode/_v.so'],capture_output=True,text=True).stdout
    print('%s dex cnvcs=%d qwsaeda=%d | so JNI cnvcs=%d qwsaeda=%d' % (
        phase, dex.count(b'com/cnvcs/App'), dex.count(b'com/qwsaeda/App'),
        qw.count('Java_com_cnvcs_App_'), qw.count('Java_com_qwsaeda')))
PY
