# 自动化对抗审计探针 第二批（补齐子代理因 Token 限额失败的全部维度）

> 生成器：`audit/probe_audit2.py`　条目数：**23**
> 覆盖维度：A 规则引擎 / B 文档诚实性 / C 打包链路 / D 32位产物 / E 业务映射 / F GL与规范

## 统计

| 严重级 | 条数 |
|---|---|
| P0 | 0 |
| P1 | 0 |
| P2 | 1 |
| P3 | 22 |

## 按探针分组

| 探针 | 条数 |
|---|---|
| F-naming | 20 |
| C-apk | 2 |
| A-board | 1 |

---

## AUD-A2-0001 [P2] 未找到 JQ_CELLS 定义
- 探针：`A-board`
- 位置：src_rs/junqi.h
- 问题：无法核对棋盘常量
- 依据：rules/board 用 cells[] 索引
- 修法：补 JQ_CELLS 宏

## AUD-A2-0002 [P3] 阶段 0 APK 含广告资源 assets/__xadsdk__remote__final__.jar（阶段 0/1/2 允许）
- 探针：`C-apk`
- 位置：rev64/dist/phase0.apk
- 问题：assets/__xadsdk__remote__final__.jar
- 依据：阶段 <3 允许保留
- 修法：仅阶段 3 需清理

## AUD-A2-0003 [P3] 阶段 1 APK 含广告资源 assets/__xadsdk__remote__final__.jar（阶段 0/1/2 允许）
- 探针：`C-apk`
- 位置：rev64/dist/phase1.apk
- 问题：assets/__xadsdk__remote__final__.jar
- 依据：阶段 <3 允许保留
- 修法：仅阶段 3 需清理

## AUD-A2-0004 [P3] 非 static 函数 is_camp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:24
- 问题：bool is_camp(int row, int col)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_camp 等

## AUD-A2-0005 [P3] 非 static 函数 is_base() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:41
- 问题：bool is_base(int row, int col)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_base 等

## AUD-A2-0006 [P3] 非 static 函数 is_rail() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:58
- 问题：bool is_rail(int row, int col)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_rail 等

## AUD-A2-0007 [P3] 非 static 函数 cell_index() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:83
- 问题：int cell_index(int row, int col) { return row * BOARD_COLS + col; }
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_index 等

## AUD-A2-0008 [P3] 非 static 函数 cell_row() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:89
- 问题：int cell_row(int idx) { return idx / BOARD_COLS; }
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_row 等

## AUD-A2-0009 [P3] 非 static 函数 cell_col() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:90
- 问题：int cell_col(int idx) { return idx % BOARD_COLS; }
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_col 等

## AUD-A2-0010 [P3] 非 static 函数 coord_cmp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/board.c:96
- 问题：int coord_cmp(const int8_t *a, const int8_t *b)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_coord_cmp 等

## AUD-A2-0011 [P3] 非 static 函数 __android_log_print() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/bt_selftest.c:22
- 问题：int __android_log_print(int prio, const char *tag, const char *fmt, ...);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq___android_log_print 等

## AUD-A2-0012 [P3] 非 static 函数 __android_log_print() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/bt_selftest.c:24
- 问题：int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq___android_log_print 等

## AUD-A2-0013 [P3] 非 static 函数 __android_log_print() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/bt_semtest.c:29
- 问题：int __android_log_print(int prio, const char *tag, const char *fmt, ...);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq___android_log_print 等

## AUD-A2-0014 [P3] 非 static 函数 __android_log_print() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/bt_semtest.c:31
- 问题：int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq___android_log_print 等

## AUD-A2-0015 [P3] 非 static 函数 cell_index() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:210
- 问题：int  cell_index(int row, int col);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_index 等

## AUD-A2-0016 [P3] 非 static 函数 cell_row() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:211
- 问题：int  cell_row(int idx);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_row 等

## AUD-A2-0017 [P3] 非 static 函数 cell_col() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:212
- 问题：int  cell_col(int idx);
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_cell_col 等

## AUD-A2-0018 [P3] 非 static 函数 is_camp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:213
- 问题：bool is_camp(int row, int col);       /* 行营（安全岛：敌方不能**吃**行营中棋子，空行营可入，非铁路；★ AUD-R-105 修正头注释；★ AUD-A2 F-naming 诚实标注：无模块前缀保留，见上方 AUD-A2 族注释） */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_camp 等

## AUD-A2-0019 [P3] 非 static 函数 is_base() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:214
- 问题：bool is_base(int row, int col);       /* 大本营（军旗所在，进后不可动 L17） */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_base 等

## AUD-A2-0020 [P3] 非 static 函数 is_rail() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:215
- 问题：bool is_rail(int row, int col);       /* 铁路（col2 纵线 + row5/6 横线） */
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_is_rail 等

## AUD-A2-0021 [P3] 非 static 函数 coord_cmp() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:216
- 问题：int  coord_cmp(const int8_t *a, const int8_t *b); /* 0x6ef54：[0]=col [1]=row，0=相同 1=不同
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_coord_cmp 等

## AUD-A2-0022 [P3] 非 static 函数 ai_score() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/junqi.h:300
- 问题：int ai_score(int src_rank_raw, int dst_rank_raw, int dst_revealed);  /* AI 贪心评分（对抗审计 #2，可单测；★ AUD-R-010 暗棋目标未翻=不透视真 rank；★ AUD-A2 F-naming 诚
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_ai_score 等

## AUD-A2-0023 [P3] 非 static 函数 ai_score() 缺模块前缀
- 探针：`F-naming`
- 位置：src_rs/rules.c:778
- 问题：int ai_score(int src_rank_raw, int dst_rank_raw, int dst_revealed)
- 依据：CODE_STYLE C5：命名前缀 jq_/rs_/ui_/bt_
- 修法：重命名为 jq_ai_score 等

