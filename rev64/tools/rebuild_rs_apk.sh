#!/bin/bash
# rebuild_rs_apk.sh — src_rs（64 位正向开发）换 so 一条龙
# 固化：每轮手写 10+ 次的 "cp so → zip 重打(STORE) → zipalign → sign → install → 对账"。
# 源 APK: /tmp/opencode/jq_apk/rs_a64/rs168_wav.apk（含 14 WAV assets + 正确包名 com.cnvcs.junqirs）。
#   ★ 诚实边界（对抗审计 P3-A6）：spak_res 推送段已改为从 rev64/dist/spak_res/
#   44 张（spak_render_refs.txt 清单）取，不再依赖源 APK 的 spak_res（rs168 仅 16 张旧裁剪）。
# 用法:
#   bash rev64/tools/rebuild_rs_apk.sh              # 换 so → 打包 → 签名 → 装 → 对账（不启动）
#   bash rev64/tools/rebuild_rs_apk.sh --start      # 追加启动 app
#   bash rev64/tools/rebuild_rs_apk.sh --no-install # 只打包签名不装机
#   bash rev64/tools/rebuild_rs_apk.sh --dev IP:PORT
# 判据: so 哈希设备=本地 一致（md5 对账），v1+v2 签名，STORE 未压缩
set -u
WS=/disk/workspaces/jq
RS=/disk/workspaces/jq/rev64/src_rs
TOOLS=/disk/workspaces/jq/rev64/tools
WORK=/tmp/opencode/jq_apk/rs_a64
SRCAPK=$WORK/rs168_wav.apk
OUT=$RS/dist/phase_rs_a64.apk
DEV=${DEV:-10.0.182.146:5555}
START=0
INSTALL=1
while [[ $# -gt 0 ]]; do
  case "$1" in
    --start) START=1; shift;;
    --no-install) INSTALL=0; shift;;
    --dev) DEV="$2"; shift 2;;
    *) echo "未知参数: $1"; exit 2;;
  esac
done

log() { echo "[rebuild_rs] $*"; }

# 前置检查
[ -f "$SRCAPK" ] || { log "★ 源 APK 不存在: $SRCAPK（需 rs168_wav.apk，含 14 WAV）"; exit 1; }
[ -f "$RS/dist/libjunqi_rs.so" ] || { log "★ so 不存在，先 make so"; exit 1; }
[ -f "$TOOLS/spak_render_refs.txt" ] || { log "★ spak_render_refs.txt 缺失"; exit 1; }
[ -d "$WS/rev64/dist/spak_res" ] || { log "★ rev64/dist/spak_res 全量源缺失"; exit 1; }
[ -f "$TOOLS/so_link_check.py" ] || { log "★ so_link_check.py 缺失（工具不在，链接审计无法执行）"; exit 1; }

log "=== [1/5] 链接安全审计 ==="
python3 $TOOLS/so_link_check.py $RS/dist/libjunqi_rs.so >/tmp/opencode/rebuild_link.log 2>&1
if [ $? -eq 2 ]; then
  log "★ 链接审计 FAIL（UND MuMu 无法解析）"; tail -3 /tmp/opencode/rebuild_link.log; exit 1
fi
log "链接审计 PASS"

log "=== [2/5] 换 so 进 APK（STORE 未压缩）==="
cp $RS/dist/libjunqi_rs.so $WORK/lib/arm64-v8a/libjunqi.so
cd $WORK
python3 - "$SRCAPK" <<'PYEOF'
import zipfile, sys, hashlib
src = sys.argv[1]
zin = zipfile.ZipFile(src)
new = open('lib/arm64-v8a/libjunqi.so','rb').read()
STORE = ('.so','.arsc','.png','.pak','.jpg','.mp3','.ogg')
with zipfile.ZipFile('rs_new.apk','w') as z:
    for i in zin.infolist():
        d = new if i.filename=='lib/arm64-v8a/libjunqi.so' else zin.read(i.filename)
        z.writestr(i, d, compress_type=zipfile.ZIP_STORED if i.filename.lower().endswith(STORE) else zipfile.ZIP_DEFLATED)
print(f'so md5={hashlib.md5(new).hexdigest()} ({len(new)}B)')
PYEOF
[ $? -ne 0 ] && { log "★ zip 重打失败"; exit 1; }

log "=== [3/5] zipalign（STORE + 4 字节对齐自检）==="
python3 $RS/zipalign_apk.py rs_new.apk rs_new_a.apk >/tmp/opencode/rebuild_align.log 2>&1
[ $? -ne 0 ] && { log "★ zipalign 失败"; cat /tmp/opencode/rebuild_align.log; exit 1; }
tail -1 /tmp/opencode/rebuild_align.log

log "=== [4/5] v1+v2 签名 ==="
# ★ 对抗审计 P0-A1（诚实边界标注）：v1+v2 双签。MuMu 解析器部分版本只认 v1
#   （v2 块致 INSTALL_PARSE_FAILED）——若目标设备是 MuMu 且装机失败，
#   临时改用 mumu_swap_so.sh 的 zip-surgery 路径（保留源 v1 条目）或
#   `--v2-signing-enabled false` 纯 v1 签。本脚本默认双签（真机 64 位 OK）。
apksigner sign --v1-signing-enabled true --v2-signing-enabled true \
  --ks /disk/workspaces/jq/junqi.keystore --ks-pass pass:junqi2026 \
  --ks-key-alias junqi --key-pass pass:junqi2026 rs_new_a.apk 2>&1 | tail -1
[ $? -ne 0 ] && { log "★ 签名失败"; exit 1; }
cp rs_new_a.apk $OUT
log "产物: $OUT ($(stat -c%s $OUT)B)"
# 签名自检（★ 对抗审计 P2-A9：verify 失败必须 abort，原 grep 输出判据假绿）
if ! apksigner verify --verbose $OUT 2>&1 | grep -q "Verified"; then
  log "★ 验签失败（v1/v2 任一无效）"; apksigner verify --verbose $OUT 2>&1 | head -8; exit 1
fi
apksigner verify --verbose $OUT 2>&1 | grep -E "v1|v2" | head -2

if [[ $INSTALL -eq 1 ]]; then
  log "=== [5/5] 装机 + so 哈希对账 ==="
  adb -s $DEV install -r $OUT 2>&1 | tail -1
  APKPATH=$(adb -s $DEV shell pm path com.cnvcs.junqirs 2>/dev/null | tr -d '\r' | sed 's/package://')
  DEVHASH=$(adb -s $DEV shell "unzip -p $APKPATH lib/arm64-v8a/libjunqi.so | md5sum" 2>/dev/null | tr -d '\r' | cut -d' ' -f1)
  LOCHASH=$(md5sum $RS/dist/libjunqi_rs.so | cut -d' ' -f1)
  if [ "$DEVHASH" = "$LOCHASH" ]; then
    log "✓ so 哈希对账一致（$LOCHASH）"
  else
    log "★ so 哈希不一致（设备=$DEVHASH 本地=$LOCHASH）"; exit 1
  fi
  if [[ $START -eq 1 ]]; then
    log "启动 app"
    adb -s $DEV shell am force-stop com.cnvcs.junqirs >/dev/null 2>&1; sleep 2
    adb -s $DEV shell am start -n com.cnvcs.junqirs/com.cnvcs.App >/dev/null 2>&1
    sleep 8
    log "进程: $(adb -s $DEV shell pidof com.cnvcs.junqirs || echo 无)"
  fi
  # 资产复用：推 spak_res（APK assets → 设备文件系统优先路径，幂等。
  #   渲染 rs_read_png 文件系统优先（免 AAsset 锁竞争），缺失才 fallback assets；
  #   每次装机后确保设备端优先路径存在，防 assets 兜底 dlsym 风险。）
  log "=== 资产复用：推 spak_res ==="
  rm -rf /tmp/opencode/spak_res_push
  mkdir -p /tmp/opencode/spak_res_push
  # ★ 对抗审计 P0-A3/P1-A4：推送源 = spak_render_refs.txt 44 张（render.c 实际引用清单），
  #   不从源 APK rs168_wav.apk 推（rs168 仅 16 张旧裁剪 → 新设备首跑缺 28 张 UI 全退化）。
  #   44 张取自 rev64/dist/spak_res/（965 全量源，与 build_phases.sh inject_spak_res 同源）。
  local refs=$WS/rev64/tools/spak_render_refs.txt
  local srcdir=$WS/rev64/dist/spak_res
  if [ -f "$refs" ]; then
    cp "$srcdir"/$(cat "$refs") /tmp/opencode/spak_res_push/ 2>/dev/null
    adb -s $DEV shell mkdir -p /data/local/tmp/spak_res >/dev/null 2>&1
    # ★ 单文件循环推送（目录 push 在部分设备只推部分文件，PNG 纹理漏推 → UI 全退化）。
    #   逐个 push 幂等，含 .bin + .png（菜单背景/按钮/☰ 图标/弹窗 纹理全量）。
    local npushed=0
    for f in /tmp/opencode/spak_res_push/*; do
      adb -s $DEV push "$f" /data/local/tmp/spak_res/ >/dev/null 2>&1 && npushed=$((npushed+1))
    done
    NBIN=$(adb -s $DEV shell ls /data/local/tmp/spak_res/*.bin 2>/dev/null | wc -l)
    NPNG=$(adb -s $DEV shell ls /data/local/tmp/spak_res/*.png 2>/dev/null | wc -l)
    log "✓ spak_res 推送完成（本次推 $npushed 张 / refs 清单 $(grep -c . "$refs")，设备端存量 $NBIN .bin / $NPNG .png）"
  else
    log "⚠ spak_render_refs.txt 缺失（设备端存量复用：$(adb -s $DEV shell ls /data/local/tmp/spak_res/*.bin 2>/dev/null | wc -l) 个 .bin）"
  fi
fi
log "✓ rebuild_rs_apk 完成"
exit 0
