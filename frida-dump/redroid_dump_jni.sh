#!/bin/bash
# redroid_dump_jni.sh — 红roid 容器精准 dump 14 JNI 入口运行时 18 地址值
# 前置：红roid 容器已起（docker run -d --privileged -p 5555:5555 <android镜像>），
#       frida-server(32位) 已 push 到容器 /data/local/ 并 su 启动，frida 在宿主可用
# 用法：bash redroid_dump_jni.sh
set -e
ADB="adb"
PKG="com.cnvcs.junqi"
FRIDA_DUMP="/disk/workspaces/jq/frida-dump/redroid_jni_dump.py"
OUT="/disk/workspaces/jq/frida-dump"

echo "[1/5] adb 连红roid（localhost:5555）"
$ADB connect localhost:5555 2>/dev/null || true
$ADB wait-for-device

echo "[2/5] 启动军棋让壳 dlopen libjunqi.so 跑完 init_array（自解密完成）"
# 让壳跑起来；OnCreate 会 NPE crash（redroid 无 GPU），但 native 自解密在 crash 前已完成
$ADB shell am start -n "$PKG/.App" 2>&1 | head -2 || true
echo "  等壳 dlopen + 自解密（~3s，在 NPE crash 前 dump）"
sleep 3

echo "[3/5] 确认 frida-server 在跑（32位 app 需 32位 frida-server）"
$ADB shell "su -c 'ps -A | grep -q frida-server || (/data/local/frida-server-32 &; sleep 1)'" 2>/dev/null || \
$ADB shell "ls /data/local/frida-server* 2>/dev/null" || echo "  [!] frida-server 未找到，先 push 32位 frida-server 到 /data/local/"

echo "[4/5] frida attach 精准读 18 地址运行时值"
cd "$OUT"
python3 "$FRIDA_DUMP" 2>&1 | tee "$OUT/jni_dump.log"

echo "[5/5] 产物检查"
if [ -f "$OUT/jni_runtime_values.json" ]; then
  echo "  OK: $OUT/jni_runtime_values.json"
  python3 -c "
import json
v=json.load(open('$OUT/jni_runtime_values.json'))
ok=sum(1 for x in v if x.get('runtime_value') is not None)
print('  %d/%d 地址读到运行时值'%(ok,len(v)))
for x in v:
    rv=x.get('runtime_value')
    print('    0x%06x -> %s'%(x['off'], '0x%08x'%rv if rv is not None else 'FAIL'))
"
  echo "  → 喂给 rev64/reverse/src_jni_translated/jni_aligned.c 注入层，14 入口可运行"
else
  echo "  [!] 未生成 jni_runtime_values.json（frida 未 attach 成功？）"
fi
