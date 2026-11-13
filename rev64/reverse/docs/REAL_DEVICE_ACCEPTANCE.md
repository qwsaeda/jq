# 32 位翻译体 · 真机验收 SOP（§124y-279，2026-11-13）

> **背景**：用户 2026-11-13 指令「不准在模拟器上运行」——此前多轮把 **模拟器（MuMu
> 10.0.182.146，x86_64 宿主 + houdini 转译 ARM32）** 的环境故障当成逆向主线排查，
> 产出大量「转译层 flaky」笔记，而**逆向本身的量化进度没有推进**。本 SOP 固化
> **真机（32 位 ARM 原生 Android 手机）** 的验收流程，作为 32 位侧唯一验收基准。
>
> **模拟器结论不可外推真机**：§124y-248~278 的全部「转译层故障」（ART 反射崩溃周期
> fault 0x9916a728 / `__system_property_get` 运行期不可见 / houdini PC 相对字面量
> 偏移失真 / 前台被抢导致 GL 线程暂停）在**真机上不存在**（原生 ARM 执行，无 houdini
> 转译层、无 32 位 libc 属性区差异）。这些条目仅作模拟器环境史，**不作真机缺陷依据**。

## 0. 前置条件（真机）

| 项 | 要求 | 校验命令 |
|---|---|---|
| 架构 | 32 位 ARM（armeabi-v7a / armeabi）**真机**（非模拟器） | `adb shell getprop ro.product.cpu.abi` → `armeabi-v7a` / `armeabi` |
| 系统 | Android 5.0–13（minSdk 21，见 AGENTS 阶段 2） | `adb shell getprop ro.build.version.release` |
| 授权 | adb 已授权（真机必弹窗授权） | `adb devices` → `device`（非 `unauthorized`） |
| 安装 | `adb install -r rev64/dist/phase_chain_arm32.apk` → `Success` | 见 §2 判据① |
| so 一致性 | 设备端 so = dist so = apk 内 so（三方哈希核对，§124y-248 教训） | §3 |
| 存储 | 允许写 `/sdcard/Download`（出屏 dump 落地） | `adb shell ls /sdcard/Download` |

**判定基准**：32 位翻译体「能玩」= 下列 5 判据在**真机**同轮全 PASS + 全链 0 崩溃。

## 1. 一次性准备（真机）

```bash
DEV=<真机 adb 序列号>            # 例：192.168.1.20:5555 或 USB serial
adb -s $DEV get-state            # → device
adb -s $DEV shell getprop ro.product.cpu.abi   # → armeabi-v7a（必须是真 32 位 ARM）
adb -s $DEV shell getprop ro.build.version.release
```

## 2. 5 判据逐步验收（真机）

### 判据① 装得上
```bash
adb -s $DEV install -r rev64/dist/phase_chain_arm32.apk      # → Success
adb -s $DEV shell "pm list packages | grep junqi32"          # → package:com.cnvcs.junqi32
adb -s $DEV shell "am start -n com.cnvcs.junqi32/com.cnvcs.App"
sleep 3
adb -s $DEV shell "pidof com.cnvcs.junqi32"                  # → pid 存活
adb -s $DEV shell "dumpsys package com.cnvcs.junqi32 | grep versionName"   # → 1.78
```

### 判据② 进主界面（出屏）
> **真机差异**：真机是真实 GPU，`screencap` **可信**（模拟器上 screencap 8% 是 GL 软驱动
> 采样假象 → 真机改用 screencap 判定，翻译体 dump 仅作备查）。

```bash
adb -s $DEV shell "am force-stop com.cnvcs.junqi32; logcat -c"
adb -s $DEV shell "am start -n com.cnvcs.junqi32/com.cnvcs.App"
sleep 15
adb -s $DEV shell "screencap -p /sdcard/rd.png" && adb -s $DEV pull /sdcard/rd.png /tmp/rd.png
python3 - <<'PY'
from PIL import Image
im = Image.open('/tmp/rd.png').convert('RGB'); w,h = im.size; px = im.load()
nb = sum(1 for y in range(0,h,8) for x in range(0,w,8) if sum(px[x,y]) > 60)
print('真机截图 %dx%d 非黑比 = %d%%' % (w,h,100*nb//((w//8)*(h//8))))
PY
# 判据：非黑比 ≥30%（§124y-179 阈值口径）
```

### 判据③ 下一局（走子出图）
```bash
adb -s $DEV shell "setprop jq.game_test 1; am force-stop com.cnvcs.junqi32; logcat -c"
adb -s $DEV shell "am start -n com.cnvcs.junqi32/com.cnvcs.App"
sleep 20
adb -s $DEV logcat -d | grep -E "GAME-TEST: 端到端完成 走子=OK|touch_int: tap1k|EVT-TBL-DUMP.*非0字节=129"
# 判据：3 条锚点全出（端到端 + 命中几何 cell6 + 事件表 289B 非0=129）
```
> 真机备注：`jq.game_test` 属性**必须在 `am start` 之前设**（§124y-272 实锤：32 位
> 转译层读不到运行期新设属性；原生真机虽无此限制，但统一前置更稳）。

### 判据④ 正常退出
```bash
adb -s $DEV shell "logcat -c"
adb -s $DEV shell "am force-stop com.cnvcs.junqi32"; sleep 3
adb -s $DEV shell "pidof com.cnvcs.junqi32"          # → 空（已退出）
adb -s $DEV logcat -d | grep -c "Fatal signal"      # → 0
```

### 判据⑤ 重进不崩
```bash
adb -s $DEV shell "am start -n com.cnvcs.junqi32/com.cnvcs.App"; sleep 15
adb -s $DEV shell "pidof com.cnvcs.junqi32"          # → pid 存活
adb -s $DEV shell "screencap -p /sdcard/rd2.png"     # → 非黑比 ≥30%
adb -s $DEV logcat -d | grep -c "Fatal signal"      # → 0
```

## 3. so 三方哈希核对（§124y-248 教训：防设备跑旧 so）

```bash
md5sum rev64/dist/libjunqi_arm32_aligned.so                      # dist so
python3 -c "
import hashlib,zipfile
z=zipfile.ZipFile('rev64/dist/phase_chain_arm32.apk')
for n in z.namelist():
    if n.endswith('libjunqi.so') and 'armeabi' in n:
        print(hashlib.md5(z.read(n)).hexdigest())"               # apk 内 so
# 设备端 so（需 root 或从 /data/app 路径取）：
adb -s $DEV shell "md5sum /data/app/*/lib/arm/libjunqi.so"       # 设备端 so
# 三者必须一致，否则判据跑的是旧 so（结果无效）
```

## 4. 批量脚本（真机复用现有判据脚本）

```bash
# 设备 4 项全钩子（判据逻辑与模拟器共用；真机应更稳——无 houdini/ART 转译故障面）
bash rev64/tools/arm32_hook_check.sh --dev $DEV --no-install --rounds 3
# 双端总入口（静态 38 项 + 设备 4 项；设备不可达自动 PENDING 降级）
bash rev64/tools/arm32_verify_all.sh --dev $DEV
```

## 5. 模拟器 ↔ 真机差异清单（勿把模拟器故障当真机缺陷）

| 项 | 模拟器（MuMu x86_64 + houdini） | 真机（ARM32 原生） |
|---|---|---|
| ART 反射崩溃周期 | **有**（fault 0x9916a728，`GetMethodID→strlen`） | 无（原生 ARM 执行） |
| 属性运行期可见性 | 32 位 libc 受限（需启动前设） | 原生可见（仍统一前置） |
| houdini PC 相对偏移 | 失真（§124y-268 已用有界拷贝兜底） | 无此问题 |
| 前台抢占 → GL 暂停 | 有（跨会话 app 抢前台） | 同机多 app 同样可能，但无转译层叠加 |
| 出屏判读 | 翻译体 dump 权威（screencap 8% 假象） | **screencap 权威** |
| 进程复用 | 有（force-stop 后 am start 可能复用） | 同样需确认 pid 消失（脚本已含） |

## 6. 验收记录格式（§124y-238/240/247 快照口径）

```
[真机验收] 设备=<序列号> ABI=armeabi-v7a Android=<版本>
  so: dist=apk=设备端=<md5>（三方一致）
  ① 装得上: Success + pid=<pid> + versionName=1.78
  ② 进主界面: screencap <w>x<h> 非黑比=<N>%（≥30%）
  ③ 下一局: GAME-TEST 走子=OK + touch_int cell6 + EVT-TBL-DUMP 289B=129
  ④ 正常退出: pid 退出 + 崩溃=0
  ⑤ 重进不崩: pid=<pid> + screencap 非黑比=<N>% + 崩溃=0
  全链崩溃计数: 0
```
