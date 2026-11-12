#!/bin/bash
# rebuild_rs_apk.sh — src_rs（64 位正向开发）换 so 一条龙
# 固化：每轮手写 10+ 次的 "cp so → zip 重打(STORE) → zipalign → sign → install → 对账"。
# 源 APK: 从 dec_rs 构建树 apktool b 生成（com.qwsaeda.junqirs，★ 2026-11-11 包名演进修复：
#   旧 rs168_wav.apk 是 com.cnvcs.junqirs 旧包名，与 so Java_com_qwsaeda_App_* 错配 →
#   UnsatisfiedLinkError 启动崩溃。dec_rs 已演进 qwsaeda（smali/manifest/FindClass 对齐）。
#   ★ 诚实边界（对抗审计 P3-A6）：spak_res 推送段已改为从 rev64/dist/spak_res/
#   63 张（spak_render_refs.txt 清单）取，不再依赖源 APK 的 spak_res。
# 用法:
#   bash rev64/tools/rebuild_rs_apk.sh              # 换 so → 打包 → 签名 → 装 → 对账（不启动）
#   bash rev64/tools/rebuild_rs_apk.sh --start      # 追加启动 app
#   bash rev64/tools/rebuild_rs_apk.sh --no-install # 只打包签名不装机
#   bash rev64/tools/rebuild_rs_apk.sh --dev IP:PORT
# 判据: so 哈希设备=本地 一致（md5 对账），v1+v2 签名，STORE 未压缩
set -u
WS=/disk/workspaces/jq
RS=/disk/workspaces/jq/rev64/forward/src_rs
TOOLS=/disk/workspaces/jq/rev64/tools
WORK=/tmp/opencode/jq_apk/rs_a64
SRCAPK=$WORK/rs_dec.apk
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
[ -d "$WORK/dec_rs" ] || { log "★ dec_rs 构建树缺失: $WORK/dec_rs（需 apktool d 解包，包名 com.qwsaeda.junqirs）"; exit 1; }
[ -f "$WORK/dec_rs/AndroidManifest.xml" ] || { log "★ dec_rs/AndroidManifest.xml 缺失"; exit 1; }
[ -f "$RS/dist/libjunqi_rs.so" ] || { log "★ so 不存在，先 make so"; exit 1; }
[ -f "$TOOLS/spak_render_refs.txt" ] || { log "★ spak_render_refs.txt 缺失"; exit 1; }
[ -d "$WS/rev64/dist/spak_res" ] || { log "★ rev64/dist/spak_res 全量源缺失"; exit 1; }
[ -f "$TOOLS/so_link_check.py" ] || { log "★ so_link_check.py 缺失（工具不在，链接审计无法执行）"; exit 1; }

log "=== [0/5] 构建源 APK（dec_rs → com.qwsaeda.junqirs）==="
rm -f "$SRCAPK"
cd "$WORK/dec_rs"
apktool b . -o "$SRCAPK" 2>&1 | tail -2
[ -f "$SRCAPK" ] || { log "★ apktool b dec_rs 失败"; exit 1; }
PKG=$(aapt dump badging "$SRCAPK" 2>/dev/null | grep "^package:" | grep -o "name='[^']*'" | head -1 | cut -d"'" -f2)
[ "$PKG" = "com.qwsaeda.junqirs" ] || { log "★ 源 APK 包名错误: $PKG（需 com.qwsaeda.junqirs）"; exit 1; }
log "源 APK 包名 = $PKG（✓ qwsaeda 对齐 so 符号）"

log "=== [1/5] 链接安全审计 ==="
# AUD-1000 D2-#5 P0 修：缓存缺失（rc=1）必须 abort，禁止静默当 PASS
# （旧：只查 rc=2，缓存缺失时 64 位门禁完全失效，设备跑旧 so）
so_link_check_rc=0
python3 $TOOLS/so_link_check.py $RS/dist/libjunqi_rs.so >/tmp/opencode/rebuild_link.log 2>&1 || so_link_check_rc=$?
if [ $so_link_check_rc -eq 2 ]; then
  log "★ 链接审计 FAIL（UND MuMu 无法解析）"; tail -3 /tmp/opencode/rebuild_link.log; exit 1
elif [ $so_link_check_rc -eq 1 ]; then
  log "★ 链接审计缓存缺失（so_link_check rc=1，/tmp 缓存被清或首次构建）"; exit 1
fi
log "链接审计 PASS"

log "=== [2/5] 换 so + 全量资源进 APK（STORE 未压缩）==="
# ★ 2026-11-12 荣耀 Magic9 真机修复（P0）：apk assets 补全 spak_res 全量 969 张。
#   旧实现只从 dec_rs 源 APK 取（assets/spak_res/ 仅 2 张 datah）→ 真机 fs 路径
#   /data/local/tmp/spak_res/ 不存在时走 apk assets 兜底 → 48 张纹理全 MISS → 纯色
#   退化 + 真机（无 fs 推）全黑。现把 rev64/dist/spak_res/ 全量 969 张入 assets。
cp $RS/dist/libjunqi_rs.so $WORK/lib/arm64-v8a/libjunqi.so
cd $WORK
# 先清 dec_rs 源 apk 里旧的 spak_res（仅 2 张），再注入全量 969 张
rm -rf assets/spak_res
mkdir -p assets/spak_res
cp -f $WS/rev64/dist/spak_res/* assets/spak_res/ 2>/dev/null || true
NSPAK=$(ls assets/spak_res/ 2>/dev/null | wc -l)
log "assets/spak_res 注入 $NSPAK 张（全量源 rev64/dist/spak_res/）"
python3 - "$SRCAPK" <<'PYEOF'
import zipfile, sys, hashlib, os
src = sys.argv[1]
zin = zipfile.ZipFile(src)
new_so = open('lib/arm64-v8a/libjunqi.so','rb').read()
STORE = ('.so','.arsc','.png','.pak','.jpg','.mp3','.ogg')
# 收集 apk 里已有 entries + 新增 spak_res
existing = {}
for i in zin.infolist():
    existing[i.filename] = i
# 新增/覆盖 assets/spak_res/*
new_assets = {}
if os.path.isdir('assets/spak_res'):
    for root, dirs, files in os.walk('assets/spak_res'):
        for f in files:
            fp = os.path.join(root, f)
            rel = fp.replace('\\', '/')   # 相对 apk 根路径
            new_assets[rel] = fp
# 写新 apk
with zipfile.ZipFile('rs_new.apk','w') as z:
    for i in zin.infolist():
        if i.filename == 'lib/arm64-v8a/libjunqi.so':
            d = new_so
        elif i.filename in new_assets:
            d = open(new_assets[i.filename],'rb').read()
        else:
            d = zin.read(i)
        ct = zipfile.ZIP_STORED if i.filename.lower().endswith(STORE) else zipfile.ZIP_DEFLATED
        z.writestr(i, d, compress_type=ct)
    # 新增（不在原 apk 的 assets/spak_res/*）
    for fn, fp in new_assets.items():
        if fn not in existing:
            inf = zipfile.ZipInfo(fn)
            inf.compress_type = zipfile.ZIP_STORED
            with open(fp,'rb') as fsrc:
                z.writestr(inf, fsrc.read(), compress_type=zipfile.ZIP_STORED)
print(f'so md5={hashlib.md5(new_so).hexdigest()} ({len(new_so)}B), assets/spak_res 新增/覆盖 {len(new_assets)} 张')
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
# AUD-1000 D2-#4 P0 修：管道吞码（旧 `| tail -1` 后 `$?` 查 tail 退出码，
# 签名失败静默继续，装机全挂且 so md5 对账仍通过）→ 直接查 apksigner 退出码
apksigner sign --v1-signing-enabled true --v2-signing-enabled true \
  --ks /disk/workspaces/jq/junqi.keystore --ks-pass pass:junqi2026 \
  --ks-key-alias junqi --key-pass pass:junqi2026 rs_new_a.apk 2>&1 | tail -1
sign_rc=${PIPESTATUS[0]}
[ $sign_rc -ne 0 ] && { log "★ 签名失败（apksigner rc=$sign_rc）"; exit 1; }
cp rs_new_a.apk $OUT
log "产物: $OUT ($(stat -c%s $OUT)B)"
# 签名自检（★ 对抗审计 P2-A9：verify 失败必须 abort，原 grep 输出判据假绿）
# AUD-1000 D6-#20 P0 修：grep "Verified" 命中 "Verified: false" 也通过（判据恒真）
# → 改 grep -qE "Verified using v[0-9] scheme .*: true"（须至少一个 v-scheme true）
set -o pipefail
if ! apksigner verify --verbose $OUT 2>&1 | grep -qE "Verified using v[0-9] scheme .*: true"; then
  log "★ 验签失败（v1/v2 任一无效）"; apksigner verify --verbose $OUT 2>&1 | head -8; exit 1
fi
apksigner verify --verbose $OUT 2>&1 | grep -E "v1|v2" | head -2

if [[ $INSTALL -eq 1 ]]; then
  log "=== [5/5] 装机 + so 哈希对账 ==="
  # ★ AUD-1000 B07-027（P1 假绿三型-管道吞码）：旧版 `adb install … | tail -1` 只看末行，
  #   install 失败（INSTALL_FAILED_UPDATE_INCOMPATIBLE 签名不符/磁盘满）不判 → 继续
  #   so 哈希对账（pm path 拿到旧 apk，旧 so 哈希恰好同本地时假过）。现先显式判
  #   install 退出码（对账兜底前加主判据），失败 abort。
  adb -s $DEV install -r $OUT
  inst_rc=$?
  [ $inst_rc -eq 0 ] || { log "★ install 失败 rc=$inst_rc（pm path 可能对账到旧 apk，中止防假过）"; exit 1; }
  APKPATH=$(adb -s $DEV shell pm path com.qwsaeda.junqirs 2>/dev/null | tr -d '\r' | sed 's/package://')
  DEVHASH=$(adb -s $DEV shell "unzip -p $APKPATH lib/arm64-v8a/libjunqi.so | md5sum" 2>/dev/null | tr -d '\r' | cut -d' ' -f1)
  LOCHASH=$(md5sum $RS/dist/libjunqi_rs.so | cut -d' ' -f1)
  if [ "$DEVHASH" = "$LOCHASH" ]; then
    log "✓ so 哈希对账一致（$LOCHASH）"
  else
    log "★ so 哈希不一致（设备=$DEVHASH 本地=$LOCHASH）"; exit 1
  fi
  if [[ $START -eq 1 ]]; then
    log "启动 app"
    adb -s $DEV shell am force-stop com.qwsaeda.junqirs >/dev/null 2>&1; sleep 2
    adb -s $DEV shell am start -n com.qwsaeda.junqirs/com.qwsaeda.App >/dev/null 2>&1
    sleep 8
    log "进程: $(adb -s $DEV shell pidof com.qwsaeda.junqirs || echo 无)"
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
  # ★ 对抗审计 P0（批4 修 P0-A3 时引入的回归）：本段在脚本顶层（非函数），
  #   用 local 会报 "local: can only be used in a function" → set -u 下
  #   refs unbound → 整段推送失败（设备端 44 张推不上 → UI 全退化）。
  #   改为普通变量赋值（顶层无 local 语义）。
  refs=$WS/rev64/tools/spak_render_refs.txt
  srcdir=$WS/rev64/dist/spak_res
  if [ -f "$refs" ]; then
    # ★ AUD-1000-ALL A5-F5-002（P1 已修，根因补正）：旧 `cp "$srcdir"/$(cat "$refs")` 把
    #   61 个文件名当多路径参数展开——"$srcdir"/前缀只对首项生效，其余 60 项裸文件名
    #   在当前目录找 → 全部静默失败（2>/dev/null 吞错误），实际只推 /tmp 残留的 5 张。
    #   修：逐文件 cp 循环（srcdir 前缀逐项生效）。
    rm -rf /tmp/opencode/spak_res_push; mkdir -p /tmp/opencode/spak_res_push
    ncpc=0
    while IFS= read -r refline; do
      [ -z "$refline" ] && continue
      if cp "$srcdir/$refline" /tmp/opencode/spak_res_push/ 2>/dev/null; then
        ncpc=$((ncpc+1))
      else
        log "★ refs 清单 $refline 在 $srcdir 缺失（cp 失败，跳过）"
      fi
    done < "$refs"
    log "spak_res cp：$ncpc/$(grep -c . "$refs") 张入 /tmp/opencode/spak_res_push/"
    adb -s $DEV shell mkdir -p /data/local/tmp/spak_res >/dev/null 2>&1
    # ★ 单文件循环推送（目录 push 在部分设备只推部分文件，PNG 纹理漏推 → UI 全退化）。
    #   逐个 push 幂等，含 .bin + .png（菜单背景/按钮/☰ 图标/弹窗 纹理全量）。
    npushed=0
    for f in /tmp/opencode/spak_res_push/*; do
      adb -s $DEV push "$f" /data/local/tmp/spak_res/ >/dev/null 2>&1 && npushed=$((npushed+1))
    done
    NBIN=$(adb -s $DEV shell ls /data/local/tmp/spak_res/*.bin 2>/dev/null | wc -l)
    NPNG=$(adb -s $DEV shell ls /data/local/tmp/spak_res/*.png 2>/dev/null | wc -l)
    # ★ AUD-1000-ALL A5-F5-002（P1 已修）：cp/push 部分失败检测——旧版 npushed=0（全部
    #   push 失败，如设备离线）仍报"✓ 推送完成"假绿。修：npushed < refs 清单数 → 显式
    #   告警（非静默过），调用方按输出判。设备离线时 npushed=0 且 refs 数 >0 → 不 PASS。
    NREFS=$(grep -c . "$refs")
    if [ "$npushed" -lt "$NREFS" ]; then
      log "★ spak_res 推送不完整（$npushed/$NREFS 张）——部分/全部 push 失败（设备离线？），设备端存量 $NBIN .bin / $NPNG .png（非全量，判读时注意）"
    else
      log "✓ spak_res 推送完成（本次推 $npushed 张 / refs 清单 $NREFS，设备端存量 $NBIN .bin / $NPNG .png）"
    fi
  else
    log "⚠ spak_render_refs.txt 缺失（设备端存量复用：$(adb -s $DEV shell ls /data/local/tmp/spak_res/*.bin 2>/dev/null | wc -l) 个 .bin）"
  fi
fi
log "✓ rebuild_rs_apk 完成"
exit 0
