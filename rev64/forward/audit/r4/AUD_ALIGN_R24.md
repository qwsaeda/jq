# AUD_ALIGN_R24: 32 位 ExceptionClear 补齐 + R23/R24 推送收口
# 生成: 2026-11-17
# 承接: R23 台账 "R24 输入 = jni_aligned OnCreate GetMethodID ExceptionClear 3 处"

## 已修 (★ 实锤)

### P1-1 jni_aligned.c 9 处 GetMethodID/FindClass 无 ExceptionClear (64 位 R17 P0-3 同款 32 位同步)
- 实修 **9 调用点** (比 R23 登记的 "3 处" 多——全量扫描 OnCreate 双路径 + jqt_jni_make_glview):
  - **jq_jni_make_glview**: FindClass "com/cnvcs/GLView" + GetMethodID `<init>` 失败分支 → 清异常再 return 0
  - **OnCreate r!=0 路径**: setEGLContextClientVersion / setContentView / GetSuperclass 回退 3 处 GetMethodID → 补清
  - **OnCreate fallback 路径**: FindClass GLView / GetMethodID `<init>` / setEGLContextClientVersion / setContentView / GetSuperclass 回退 5 处 → 补清
- 模式统一: `if (!mid/cls) { if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env); ... }` + "★ R24-P1" 注释
- 对照: 64 位 jni_bridge.c 已有同款 (jni_method_audit.py 0 缺口门禁); 32 位 jqt_jnibr.c:78-91 风格一致
- **重编 + 全链实锤**:
  - so md5 623ead77 → **d8c98fd9** (9531160B, JNI 14 符号)
  - phase_chain_arm32.apk 重打 (16877079B, v1/v2/v3 scheme 全 true)
  - 4 处基准串同步 (apk_so_hash_check / device_regress_offline_check / device_regress_check / dist/MANIFEST.md)
  - 静态佐证 **74/74** + 32 位桌面 8 步 **12/12** + 三方 md5 一致 (so = apk 内 so = 基准 d8c98fd9)

## 推送收口 (★ 网络恢复实锤)
- R23 `78a2cf6` (vtable qsort + P1 批, 前轮网络断未推) + R24 `d8aaf45` **一并推上 origin/main**
  (一次通过, 第 1 次尝试成功)
- 工作区: 除 course_unidbg/* (其他会话文件, 不纳入) 外**无未提交改动**

## 台账状态
- 总条目: 1 (P1-1 含 9 调用点)
- 已修: 1 (9 点全补 + 重编 + 基准同步 + 推送)
- 待修: 0 (R24 输入清空)
- 撤回: 0

## 审计线终态 (R16→R24 全入库)
```
R16 逆向地址澄清 → R17 门禁击穿 P0×3 → R18 三轮对抗 P0×2+P1×4 → R19 工具链×5+判据×4
→ R20 边角×2 → R21 交付物+MANIFEST → R22 UNDO 核查 (不适用) → R23 vtable P0+P1×4
→ R24 ExceptionClear×9 + R23/R24 推送收口
```

## 全门禁基线 (R24 终态, 全实锤)
| 门禁 | 状态 |
|------|------|
| arm32_static_evidence | **74/74** (含 vtable_sorted 新门禁) |
| 32 位桌面 8 步 | **12/12** |
| align_gate (64 位) | **8/8** (R23/R24 只动 32 位, 无回归) |
| src_rs_regress | **12/12 + [2.5] + [2.6]** |
| so 三方一致 | **d8c98fd9** (dist so = apk 内 so = 4 基准串) |
| 双签 | phase_chain_arm32.apk v1+v2+v3 = true |
| git | R16-R24 全入库 origin/main |

## 唯一残留 PENDING (环境依赖, 非缺陷)
- **设备 8 步全链**: 10.0.182.146 离线 (No route to host)。离线态 arm32_hook_check /
  mumu_regress 均自动 rc=3 PENDING 提前终止 (R19/R23 加固, 不误标 so 回归);
  设备恢复后收口:
  ```bash
  bash rev64/tools/device_regress_check.sh    # 基准 d8c98fd9 已同步
  bash rev64/tools/mumu_regress.sh --dev 10.0.182.146:5555
  ```
