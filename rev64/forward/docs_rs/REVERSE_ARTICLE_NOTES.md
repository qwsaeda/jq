# 看雪 so 逆向文章方法论沉淀（2026-11-20）

> 用户分享：看雪学苑《记录第一次的 so 文件逆向分析》
> https://mp.weixin.qq.com/s/rO80pkDap7ZptLNK-cd5cg
> **价值不在具体算法**（MD5/DES 是那个 app 的），在**方法论纪律**——多条直接命中我前几轮犯的错。

## 一、文章核心纪律（6 条，可复用）

1. **Java bridge 不可用/被检测 → 纯 native 路径**：不调 `Java.perform`，hook libart 的
   `RegisterNatives`，从 JNINativeMethod 注册表直接拿 native 函数真实 fnPtr，再
   `Interceptor.attach`。**绕开"模块枚举找不到符号"**——正是我 guest so 不可见问题的正解方向。
2. **注册时机**：RegisterNatives hook 要在目标方法注册前挂（spawn 或 app 未注册时 attach）；
   已注册完再 attach 抓不到。
3. **JNINativeMethod 布局**：`{name_ptr, sig_ptr, fnPtr}`，每项 3 个指针宽
   （ARM32=12B / ARM64=24B）。
4. **Thumb 位**：ARM32 注册地址含 Thumb 位（0x23bd），`fn.and(0xfffffffe)` 才是代码地址（0x23bc）；
   **不能用于 ARM64**（会截断高位）。
5. **jstring 必须走 JNI 函数表**：不能 `readUtf8String()` 直接读对象引用 → 用
   `GetStringUTFChars`（表索引 169）+ `ReleaseStringUTFChars`（170）配对。
   函数表索引 167/169/170，**表头 4 个 reserved 槽要计数**。
   偏移：ARM32 `169×4=676` / ARM64 `169×8=1352`（不能照搬）。
6. **不靠函数名猜，靠注册表 + 运行日志确认映射**（JNINativeMethod 的 name/sig/fnPtr 三者对应）。

## 二、算法分析的纪律（后半篇，同样命中我的假绿三型）

- **不看到常量就下结论**：MD5 常量 `67452301 efcdab89...` 只是线索 → 还要验证分块/填充/长度/输出 +
  Python `hashlib` 实际对照，才能确认。
- **认出算法族≠标准实现**：识别 DES 后还要**逐项比表**（PC2[35] 样本 46 vs 标准 47 的差异）+ 位序 +
  已知测试向量验证——"开头几个值一致"不能证明全表标准。
- **PC 相对寻址**：`0x5772 + 0x23ee = 0x7b60`（Thumb ADD Rd,PC 的 PC=(addr+4)&~3）——与我
  `jqt_mem_fuzz32` 踩过的 `off &= ~1u` 坑同源。
- **立即数还原字符串**：`MOVW #0x2638` + `MOVT #0x6425` → 0x64252638 → 小端字节 `38 26 25 64 2A` →
  ASCII `8&%d*`（按字节排列，不是十六进制数字倒写）。
- **纯算法还原链**：Java 调用链 → JNI 入口 → 数据流 → 算法特征 → 中间值对照 → 独立实现；
  最终 Python 结果与原始 so 抓包/Unidbg 结果三方对照 PASS。

## 三、对照我们的工作（哪些已做 / 哪些犯错 / 哪些可深化）

| 文章纪律 | 我们的状态 |
|---|---|
| Java bridge 不可用→纯 native hook | ⚠️ **我下"guest so 不可见所以不行"太早**——已打通 host JNI 桥梁 hook（FindClass），但**没试 RegisterNatives 直接拿 fnPtr**（文章正解方向）→ 本轮补脚本 |
| 注册时机 | ⚠️ 我 spawn 模式在 arm64 转译下进程销毁 → PENDING；文章印证"时机"是关键 |
| 不靠函数名猜，靠注册表 | ✅ 我 jqt_mem 的 dispatch 表 fns/offs 名值对应判据（5463/5463）= 同款纪律 |
| 不看到常量就下结论 | ✅ 已内化为假绿三型；本轮还修了域矩阵口径漂移（7.8%→8.1%）|
| 逐项比表 + 测试向量 | ✅ 我用 3 seed 重跑 + 多源对照（fuzz_all A/B/C 三段）|
| PC 相对寻址 Thumb 位 | ✅ jqt_mem_fuzz32 已实锤 `off &= ~1u` 语义 + 6 奇数 off 诚实边界 |
| 纯算法还原三方对照 | ✅ 正向 C 行为级重写 + rule_probe 双端 + 154 case 对照 = 同款思路 |

## 四、可复用产出

- `rev64/tools/frida_registernatives_hook.py` —— 按文章方法写的 RegisterNatives hook
  （纯 native + JNINativeMethod 3 指针布局 + JNI 函数表偏移按位宽计算 + Thumb 去位）。
  **本环境 frida-server 反复挂 → 未实测 PASS，标注为换稳定环境可跑的入口**（不伪装成已验证）。

## 五、诚实边界

- 文章样本是 **ARM32 + 有 Java bridge 检测**；我们是 **arm64 so 经 x86_64 houdini 转译 +
  Java bridge 不可用**——场景不同，方法论可迁移，具体偏移/位宽要按我们的环境算。
- 文章的具体 signA/signB/DES 变体算法**不照搬**（那是另一个 app），只吸收分析纪律。
