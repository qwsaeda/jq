# 自动化对抗审计探针 第二批（补齐子代理因 Token 限额失败的全部维度）

> 生成器：`audit/probe_audit2.py`　条目数：**218**
> 覆盖维度：A 规则引擎 / B 文档诚实性 / C 打包链路 / D 32位产物 / E 业务映射 / F GL与规范

## 统计

| 严重级 | 条数 |
|---|---|
| P0 | 0 |
| P1 | 0 |
| P2 | 76 |
| P3 | 142 |

## 按探针分组

| 探针 | 条数 |
|---|---|
| F-naming | 139 |
| B-doc-num | 52 |
| E-sem | 23 |
| C-apk | 2 |
| A-board | 1 |
| F-gl | 1 |

---

## AUD-A2-0001 [P2] 未找到 JQ_CELLS 定义
- 探针：`A-board`
- 位置：src_rs/junqi.h
- 问题：无法核对棋盘常量
- 依据：rules/board 用 cells[] 索引
- 修法：补 JQ_CELLS 宏

## AUD-A2-0002 [P2] 文档 evverify 21/21 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 21/21
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0003 [P2] 文档 evverify 16/16 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 16/16
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0004 [P2] 文档 evverify 22/22 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 22/22
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0005 [P2] 文档 evverify 16/16 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 16/16
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0006 [P2] 文档 evverify 21/21 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 21/21
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0007 [P2] 文档 evverify 21/21 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 21/21
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0008 [P2] 文档 evverify 9/9 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 9/9
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0009 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0010 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0011 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0012 [P2] 文档 evverify 11/11 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 11/11
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0013 [P2] 文档 evverify 21/21 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 21/21
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0014 [P2] 文档 evverify 21/21 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 21/21
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0015 [P2] 文档 evverify 12/12 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 12/12
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0016 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0017 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0018 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0019 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0020 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0021 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0022 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0023 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0024 [P2] 文档 evverify 31/31 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 31/31
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0025 [P2] 文档 evverify 32/32 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 32/32
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0026 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0027 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0028 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0029 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0030 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0031 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0032 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0033 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0034 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0035 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0036 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0037 [P2] 文档 evverify 44/44 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 44/44
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0038 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0039 [P2] 文档 evverify 11/11 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 11/11
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0040 [P2] 文档 evverify 12/12 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 12/12
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0041 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0042 [P2] 文档 evverify 11/11 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 11/11
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0043 [P2] 文档 evverify 9/9 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 9/9
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0044 [P2] 文档 evverify 9/9 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 9/9
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0045 [P2] 文档 evverify 9/9 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 9/9
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0046 [P2] 文档 evverify 9/9 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 9/9
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0047 [P2] 文档 evverify 9/9 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 9/9
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0048 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0049 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0050 [P2] 文档 evverify 10/10 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 10/10
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0051 [P2] 文档 evverify 11/11 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 11/11
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0052 [P2] 文档 evverify 11/11 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 11/11
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0053 [P2] 文档 evverify 12/12 与实际 118/118 不符
- 探针：`B-doc-num`
- 位置：docs_rs/AUDIT_LAYOUT_ALIGN.md
- 问题：文档 12/12
- 依据：实际 118/118
- 修法：更新数字

## AUD-A2-0054 [P2] UI_D005 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0055 [P2] UI_D005 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0056 [P2] UI_D012 的 Draw 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Draw（期望含 和棋/提和），我方注释「/* Draw → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「和棋」语义的实现

## AUD-A2-0057 [P2] UI_D015 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0058 [P2] UI_D030 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0059 [P2] UI_D030 的 Chat 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Chat（期望含 聊天），我方注释「/* Chat → 业务动作 */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「聊天」语义的实现

## AUD-A2-0060 [P2] UI_D064 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0061 [P2] UI_D064 的 Chat 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Chat（期望含 聊天），我方注释「/* Chat → 业务动作 */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「聊天」语义的实现

## AUD-A2-0062 [P2] UI_D064 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0063 [P2] UI_D070 的 Draw 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Draw（期望含 和棋/提和），我方注释「/* Draw → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「和棋」语义的实现

## AUD-A2-0064 [P2] UI_D075 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0065 [P2] UI_D075 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0066 [P2] UI_D094 的 Chat 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Chat（期望含 聊天），我方注释「/* Chat → 业务动作 */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「聊天」语义的实现

## AUD-A2-0067 [P2] UI_D246 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0068 [P2] UI_D246 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0069 [P2] UI_D247 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0070 [P2] UI_D250 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0071 [P2] UI_D250 的 Chat 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Chat（期望含 聊天），我方注释「/* Chat → 业务动作 */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「聊天」语义的实现

## AUD-A2-0072 [P2] UI_D255 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0073 [P2] UI_D255 的 Chat 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Chat（期望含 聊天），我方注释「/* Chat → 业务动作 */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「聊天」语义的实现

## AUD-A2-0074 [P2] UI_D255 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0075 [P2] UI_D256 的 Menu 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Menu（期望含 菜单），我方注释「/* Menu → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「菜单」语义的实现

## AUD-A2-0076 [P2] UI_D256 的 Back 按钮语义与原版名不符
- 探针：`E-sem`
- 位置：src_rs/jni_bridge.c
- 问题：原版 name=Back（期望含 回/取消/关闭/退出/菜单），我方注释「/* Back → 边界对话框（诚实边界） */」
- 依据：原版按钮 name 是语义权威
- 修法：改为含「回」语义的实现

## AUD-A2-0077 [P3] 阶段 0 APK 含广告资源 assets/__xadsdk__remote__final__.jar（阶段 0/1/2 允许）
- 探针：`C-apk`
- 位置：rev64/dist/phase0.apk
- 问题：assets/__xadsdk__remote__final__.jar
- 依据：阶段 <3 允许保留
- 修法：仅阶段 3 需清理

## AUD-A2-0078 [P3] 阶段 1 APK 含广告资源 assets/__xadsdk__remote__final__.jar（阶段 0/1/2 允许）
- 探针：`C-apk`
- 位置：rev64/dist/phase1.apk
- 问题：assets/__xadsdk__remote__final__.jar
- 依据：阶段 <3 允许保留
- 修法：仅阶段 3 需清理

## AUD-A2-0079 [P3] 有 glDrawArrays 但无 glGetError 检查
- 探针：`F-gl`
- 位置：src_rs/render.h
- 问题：GL 错误静默
- 依据：排障困难
- 修法：关键路径加 glGetError 日志

## AUD-A2-0080 [P3] 非 static 函数 is_camp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:12
- 问题：bool is_camp(int row, int col)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_camp 等

## AUD-A2-0081 [P3] 非 static 函数 is_base() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:25
- 问题：bool is_base(int row, int col)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_base 等

## AUD-A2-0082 [P3] 非 static 函数 is_rail() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:35
- 问题：bool is_rail(int row, int col)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_rail 等

## AUD-A2-0083 [P3] 非 static 函数 cell_index() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:44
- 问题：int cell_index(int row, int col) { return row * BOARD_COLS + col; }
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_index 等

## AUD-A2-0084 [P3] 非 static 函数 cell_row() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:45
- 问题：int cell_row(int idx) { return idx / BOARD_COLS; }
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_row 等

## AUD-A2-0085 [P3] 非 static 函数 cell_col() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:46
- 问题：int cell_col(int idx) { return idx % BOARD_COLS; }
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_col 等

## AUD-A2-0086 [P3] 非 static 函数 coord_cmp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:48
- 问题：int coord_cmp(const int8_t *a, const int8_t *b)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_coord_cmp 等

## AUD-A2-0087 [P3] 非 static 函数 __android_log_print() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/bt_selftest.c:19
- 问题：int __android_log_print(int prio, const char *tag, const char *fmt, ...);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq___android_log_print 等

## AUD-A2-0088 [P3] 非 static 函数 __android_log_print() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/bt_selftest.c:21
- 问题：int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq___android_log_print 等

## AUD-A2-0089 [P3] 非 static 函数 run_exitconfirm() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:2912
- 问题：int run_exitconfirm(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_exitconfirm 等

## AUD-A2-0090 [P3] 非 static 函数 run_resignconfirm() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:2931
- 问题：int run_resignconfirm(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_resignconfirm 等

## AUD-A2-0091 [P3] 非 static 函数 run_regretconfirm() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:2944
- 问题：int run_regretconfirm(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_regretconfirm 等

## AUD-A2-0092 [P3] 非 static 函数 run_d000() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:2958
- 问题：int run_d000(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d000 等

## AUD-A2-0093 [P3] 非 static 函数 run_d001() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:2983
- 问题：int run_d001(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d001 等

## AUD-A2-0094 [P3] 非 static 函数 run_d002() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3010
- 问题：int run_d002(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d002 等

## AUD-A2-0095 [P3] 非 static 函数 run_d003() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3035
- 问题：int run_d003(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d003 等

## AUD-A2-0096 [P3] 非 static 函数 run_d004() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3061
- 问题：int run_d004(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d004 等

## AUD-A2-0097 [P3] 非 static 函数 run_d005() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3086
- 问题：int run_d005(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d005 等

## AUD-A2-0098 [P3] 非 static 函数 run_d006() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3111
- 问题：int run_d006(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d006 等

## AUD-A2-0099 [P3] 非 static 函数 run_d007() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3142
- 问题：int run_d007(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d007 等

## AUD-A2-0100 [P3] 非 static 函数 run_d008() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3171
- 问题：int run_d008(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d008 等

## AUD-A2-0101 [P3] 非 static 函数 run_d010() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3199
- 问题：int run_d010(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d010 等

## AUD-A2-0102 [P3] 非 static 函数 run_d012() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3224
- 问题：int run_d012(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d012 等

## AUD-A2-0103 [P3] 非 static 函数 run_d013() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3253
- 问题：int run_d013(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d013 等

## AUD-A2-0104 [P3] 非 static 函数 run_d015() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3279
- 问题：int run_d015(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d015 等

## AUD-A2-0105 [P3] 非 static 函数 run_d016() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3303
- 问题：int run_d016(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d016 等

## AUD-A2-0106 [P3] 非 static 函数 run_d017() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3330
- 问题：int run_d017(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d017 等

## AUD-A2-0107 [P3] 非 static 函数 run_d019() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3354
- 问题：int run_d019(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d019 等

## AUD-A2-0108 [P3] 非 static 函数 run_d020() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3381
- 问题：int run_d020(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d020 等

## AUD-A2-0109 [P3] 非 static 函数 run_d021() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3408
- 问题：int run_d021(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d021 等

## AUD-A2-0110 [P3] 非 static 函数 run_d022() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3434
- 问题：int run_d022(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d022 等

## AUD-A2-0111 [P3] 非 static 函数 run_d023() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3462
- 问题：int run_d023(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d023 等

## AUD-A2-0112 [P3] 非 static 函数 run_d024() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3489
- 问题：int run_d024(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d024 等

## AUD-A2-0113 [P3] 非 static 函数 run_d025() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3513
- 问题：int run_d025(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d025 等

## AUD-A2-0114 [P3] 非 static 函数 run_d026() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3537
- 问题：int run_d026(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d026 等

## AUD-A2-0115 [P3] 非 static 函数 run_d027() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3563
- 问题：int run_d027(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d027 等

## AUD-A2-0116 [P3] 非 static 函数 run_d028() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3589
- 问题：int run_d028(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d028 等

## AUD-A2-0117 [P3] 非 static 函数 run_d029() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3615
- 问题：int run_d029(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d029 等

## AUD-A2-0118 [P3] 非 static 函数 run_d030() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3640
- 问题：int run_d030(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d030 等

## AUD-A2-0119 [P3] 非 static 函数 run_d031() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3665
- 问题：int run_d031(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d031 等

## AUD-A2-0120 [P3] 非 static 函数 run_d032() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3691
- 问题：int run_d032(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d032 等

## AUD-A2-0121 [P3] 非 static 函数 run_d033() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3718
- 问题：int run_d033(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d033 等

## AUD-A2-0122 [P3] 非 static 函数 run_d034() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3743
- 问题：int run_d034(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d034 等

## AUD-A2-0123 [P3] 非 static 函数 run_d035() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3768
- 问题：int run_d035(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d035 等

## AUD-A2-0124 [P3] 非 static 函数 run_d036() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3793
- 问题：int run_d036(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d036 等

## AUD-A2-0125 [P3] 非 static 函数 run_d037() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3820
- 问题：int run_d037(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d037 等

## AUD-A2-0126 [P3] 非 static 函数 run_d038() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3846
- 问题：int run_d038(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d038 等

## AUD-A2-0127 [P3] 非 static 函数 run_d039() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3873
- 问题：int run_d039(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d039 等

## AUD-A2-0128 [P3] 非 static 函数 run_d041() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3897
- 问题：int run_d041(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d041 等

## AUD-A2-0129 [P3] 非 static 函数 run_d042() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3928
- 问题：int run_d042(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d042 等

## AUD-A2-0130 [P3] 非 static 函数 run_d043() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3958
- 问题：int run_d043(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d043 等

## AUD-A2-0131 [P3] 非 static 函数 run_d044() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:3983
- 问题：int run_d044(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d044 等

## AUD-A2-0132 [P3] 非 static 函数 run_d045() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4008
- 问题：int run_d045(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d045 等

## AUD-A2-0133 [P3] 非 static 函数 run_d046() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4032
- 问题：int run_d046(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d046 等

## AUD-A2-0134 [P3] 非 static 函数 run_d047() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4056
- 问题：int run_d047(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d047 等

## AUD-A2-0135 [P3] 非 static 函数 run_d049() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4080
- 问题：int run_d049(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d049 等

## AUD-A2-0136 [P3] 非 static 函数 run_d050() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4104
- 问题：int run_d050(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d050 等

## AUD-A2-0137 [P3] 非 static 函数 run_d051() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4130
- 问题：int run_d051(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d051 等

## AUD-A2-0138 [P3] 非 static 函数 run_d052() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4155
- 问题：int run_d052(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d052 等

## AUD-A2-0139 [P3] 非 static 函数 run_d053() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4180
- 问题：int run_d053(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d053 等

## AUD-A2-0140 [P3] 非 static 函数 run_d054() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4205
- 问题：int run_d054(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d054 等

## AUD-A2-0141 [P3] 非 static 函数 run_d055() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4230
- 问题：int run_d055(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d055 等

## AUD-A2-0142 [P3] 非 static 函数 run_d057() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4254
- 问题：int run_d057(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d057 等

## AUD-A2-0143 [P3] 非 static 函数 run_d058() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4279
- 问题：int run_d058(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d058 等

## AUD-A2-0144 [P3] 非 static 函数 run_d059() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4304
- 问题：int run_d059(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d059 等

## AUD-A2-0145 [P3] 非 static 函数 run_d060() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4330
- 问题：int run_d060(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d060 等

## AUD-A2-0146 [P3] 非 static 函数 run_d061() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4358
- 问题：int run_d061(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d061 等

## AUD-A2-0147 [P3] 非 static 函数 run_d062() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4387
- 问题：int run_d062(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d062 等

## AUD-A2-0148 [P3] 非 static 函数 run_d063() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4414
- 问题：int run_d063(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d063 等

## AUD-A2-0149 [P3] 非 static 函数 run_d064() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4440
- 问题：int run_d064(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d064 等

## AUD-A2-0150 [P3] 非 static 函数 run_d065() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4466
- 问题：int run_d065(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d065 等

## AUD-A2-0151 [P3] 非 static 函数 run_d066() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4491
- 问题：int run_d066(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d066 等

## AUD-A2-0152 [P3] 非 static 函数 run_d067() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4517
- 问题：int run_d067(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d067 等

## AUD-A2-0153 [P3] 非 static 函数 run_d068() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4548
- 问题：int run_d068(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d068 等

## AUD-A2-0154 [P3] 非 static 函数 run_d069() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4574
- 问题：int run_d069(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d069 等

## AUD-A2-0155 [P3] 非 static 函数 run_d070() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4603
- 问题：int run_d070(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d070 等

## AUD-A2-0156 [P3] 非 static 函数 run_d071() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4632
- 问题：int run_d071(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d071 等

## AUD-A2-0157 [P3] 非 static 函数 run_d072() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4657
- 问题：int run_d072(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d072 等

## AUD-A2-0158 [P3] 非 static 函数 run_d073() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4684
- 问题：int run_d073(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d073 等

## AUD-A2-0159 [P3] 非 static 函数 run_d075() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4709
- 问题：int run_d075(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d075 等

## AUD-A2-0160 [P3] 非 static 函数 run_d077() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4736
- 问题：int run_d077(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d077 等

## AUD-A2-0161 [P3] 非 static 函数 run_d078() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4765
- 问题：int run_d078(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d078 等

## AUD-A2-0162 [P3] 非 static 函数 run_d079() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4791
- 问题：int run_d079(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d079 等

## AUD-A2-0163 [P3] 非 static 函数 run_d082() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4815
- 问题：int run_d082(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d082 等

## AUD-A2-0164 [P3] 非 static 函数 run_d083() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4843
- 问题：int run_d083(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d083 等

## AUD-A2-0165 [P3] 非 static 函数 run_d084() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4870
- 问题：int run_d084(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d084 等

## AUD-A2-0166 [P3] 非 static 函数 run_d085() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4897
- 问题：int run_d085(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d085 等

## AUD-A2-0167 [P3] 非 static 函数 run_d086() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4925
- 问题：int run_d086(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d086 等

## AUD-A2-0168 [P3] 非 static 函数 run_d089() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4952
- 问题：int run_d089(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d089 等

## AUD-A2-0169 [P3] 非 static 函数 run_d090() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:4978
- 问题：int run_d090(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d090 等

## AUD-A2-0170 [P3] 非 static 函数 run_d094() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5003
- 问题：int run_d094(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d094 等

## AUD-A2-0171 [P3] 非 static 函数 run_d095() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5031
- 问题：int run_d095(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d095 等

## AUD-A2-0172 [P3] 非 static 函数 run_d096() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5059
- 问题：int run_d096(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d096 等

## AUD-A2-0173 [P3] 非 static 函数 run_d097() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5085
- 问题：int run_d097(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d097 等

## AUD-A2-0174 [P3] 非 static 函数 run_d098() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5112
- 问题：int run_d098(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d098 等

## AUD-A2-0175 [P3] 非 static 函数 run_d099() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5141
- 问题：int run_d099(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d099 等

## AUD-A2-0176 [P3] 非 static 函数 run_d100() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5168
- 问题：int run_d100(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d100 等

## AUD-A2-0177 [P3] 非 static 函数 run_d101() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5192
- 问题：int run_d101(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d101 等

## AUD-A2-0178 [P3] 非 static 函数 run_d102() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5219
- 问题：int run_d102(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d102 等

## AUD-A2-0179 [P3] 非 static 函数 run_d104() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5243
- 问题：int run_d104(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d104 等

## AUD-A2-0180 [P3] 非 static 函数 run_d107() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5271
- 问题：int run_d107(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d107 等

## AUD-A2-0181 [P3] 非 static 函数 run_d108() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5299
- 问题：int run_d108(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d108 等

## AUD-A2-0182 [P3] 非 static 函数 run_d109() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5325
- 问题：int run_d109(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d109 等

## AUD-A2-0183 [P3] 非 static 函数 run_d110() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5351
- 问题：int run_d110(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d110 等

## AUD-A2-0184 [P3] 非 static 函数 run_d112() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5376
- 问题：int run_d112(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d112 等

## AUD-A2-0185 [P3] 非 static 函数 run_d113() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5404
- 问题：int run_d113(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d113 等

## AUD-A2-0186 [P3] 非 static 函数 run_d114() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5431
- 问题：int run_d114(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d114 等

## AUD-A2-0187 [P3] 非 static 函数 run_d118() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5455
- 问题：int run_d118(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d118 等

## AUD-A2-0188 [P3] 非 static 函数 run_d119() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5480
- 问题：int run_d119(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d119 等

## AUD-A2-0189 [P3] 非 static 函数 run_d120() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5505
- 问题：int run_d120(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d120 等

## AUD-A2-0190 [P3] 非 static 函数 run_d121() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5531
- 问题：int run_d121(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d121 等

## AUD-A2-0191 [P3] 非 static 函数 run_d122() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5556
- 问题：int run_d122(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d122 等

## AUD-A2-0192 [P3] 非 static 函数 run_d123() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5581
- 问题：int run_d123(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d123 等

## AUD-A2-0193 [P3] 非 static 函数 run_d246() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5605
- 问题：int run_d246(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d246 等

## AUD-A2-0194 [P3] 非 static 函数 run_d247() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5630
- 问题：int run_d247(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d247 等

## AUD-A2-0195 [P3] 非 static 函数 run_d248() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5654
- 问题：int run_d248(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d248 等

## AUD-A2-0196 [P3] 非 static 函数 run_d249() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5681
- 问题：int run_d249(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d249 等

## AUD-A2-0197 [P3] 非 static 函数 run_d250() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5705
- 问题：int run_d250(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d250 等

## AUD-A2-0198 [P3] 非 static 函数 run_d251() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5730
- 问题：int run_d251(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d251 等

## AUD-A2-0199 [P3] 非 static 函数 run_d253() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5754
- 问题：int run_d253(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d253 等

## AUD-A2-0200 [P3] 非 static 函数 run_d255() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5778
- 问题：int run_d255(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d255 等

## AUD-A2-0201 [P3] 非 static 函数 run_d256() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5804
- 问题：int run_d256(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d256 等

## AUD-A2-0202 [P3] 非 static 函数 run_d257() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5831
- 问题：int run_d257(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d257 等

## AUD-A2-0203 [P3] 非 static 函数 run_d258() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5859
- 问题：int run_d258(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d258 等

## AUD-A2-0204 [P3] 非 static 函数 run_d259() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5887
- 问题：int run_d259(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d259 等

## AUD-A2-0205 [P3] 非 static 函数 run_d260() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5912
- 问题：int run_d260(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d260 等

## AUD-A2-0206 [P3] 非 static 函数 run_d261() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5940
- 问题：int run_d261(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d261 等

## AUD-A2-0207 [P3] 非 static 函数 run_d262() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5964
- 问题：int run_d262(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d262 等

## AUD-A2-0208 [P3] 非 static 函数 run_d263() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:5991
- 问题：int run_d263(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d263 等

## AUD-A2-0209 [P3] 非 static 函数 run_d264() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/evverify.c:6019
- 问题：int run_d264(void)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_run_d264 等

## AUD-A2-0210 [P3] 非 static 函数 cell_index() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:127
- 问题：int  cell_index(int row, int col);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_index 等

## AUD-A2-0211 [P3] 非 static 函数 cell_row() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:128
- 问题：int  cell_row(int idx);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_row 等

## AUD-A2-0212 [P3] 非 static 函数 cell_col() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:129
- 问题：int  cell_col(int idx);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_col 等

## AUD-A2-0213 [P3] 非 static 函数 is_camp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:130
- 问题：bool is_camp(int row, int col);       /* 行营（安全岛，敌方不可入，非铁路） */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_camp 等

## AUD-A2-0214 [P3] 非 static 函数 is_base() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:131
- 问题：bool is_base(int row, int col);       /* 大本营（军旗所在，进后不可动 L17） */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_base 等

## AUD-A2-0215 [P3] 非 static 函数 is_rail() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:132
- 问题：bool is_rail(int row, int col);       /* 铁路（col2 纵线 + row5/6 横线） */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_rail 等

## AUD-A2-0216 [P3] 非 static 函数 coord_cmp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:133
- 问题：int  coord_cmp(const int8_t *a, const int8_t *b); /* 0x6ef54：[0]=col [1]=row，0=相同 */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_coord_cmp 等

## AUD-A2-0217 [P3] 非 static 函数 ai_score() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:166
- 问题：int ai_score(int src_rank_raw, int dst_rank_raw);  /* AI 贪心评分（对抗审计 #2，可单测） */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_ai_score 等

## AUD-A2-0218 [P3] 非 static 函数 ai_score() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/rules.c:330
- 问题：int ai_score(int src_rank_raw, int dst_rank_raw)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_ai_score 等

