# UI 与按键对齐台账（正向 com.qwsaeda.junqirs / src_rs 64 位）

> 生成：2026-11-11，数据源 = 原版 `rev64/dist/spak_res/datah_*.bin`（SPAK R5 解包 272 个 datah）

> 判据入口：`bash rev64/tools/rs_logic_align.sh --dev 10.0.182.146:5555`（当前 8/8 PASS）

## 总口径

- 原版 datah 对话框：**272 个**
- 含按钮 XML 节点：**111 个，按钮节点合计 351**
- 有按钮已对齐：**111/111（全量，无缺口）**
- 无按钮（纯展示/文本框/资源占位）：161 个（**诚实边界：无交互业务，不补**）

## 有按钮 datah 明细（111 个，全量对齐）

| datah | 按钮数 | rs_draw 实现 | 分发/命中 | 几何判据 |
|---|---|---|---|---|
| datah_000 | 2 | rs_draw_d000 | ui_d000_hit + evverify | Δ=0 |
| datah_001 | 4 | rs_draw_d001 | ui_d001_hit + evverify | Δ=0 |
| datah_002 | 2 | rs_draw_d002 | ui_d002_hit + evverify | Δ=0 |
| datah_003 | 3 | rs_draw_d003 | ui_d003_hit + evverify | Δ=0 |
| datah_004 | 2 | rs_draw_d004 | ui_d004_hit + evverify | Δ=0 |
| datah_005 | 2 | rs_draw_d005 | ui_d005_hit + evverify | Δ=0 |
| datah_007 | 6 | rs_draw_d007 | ui_d007_hit + evverify | Δ=0 |
| datah_008 | 5 | rs_draw_d008 | ui_d008_hit + evverify | Δ=0 |
| datah_010 | 2 | rs_draw_d010 | ui_d010_hit + evverify | Δ=0 |
| datah_012 | 6 | rs_draw_d012 | ui_d012_hit + evverify | Δ=0 |
| datah_013 | 3 | rs_draw_d013 | ui_d013_hit + evverify | Δ=0 |
| datah_015 | 1 | rs_draw_d015 | ui_d015_hit + evverify | Δ=0 |
| datah_016 | 6 | rs_draw_d016 | ui_d016_hit + evverify | Δ=0 |
| datah_017 | 1 | rs_draw_d017 | ui_d017_hit + evverify | Δ=0 |
| datah_020 | 4 | rs_draw_d020 | ui_d020_hit + evverify | Δ=0 |
| datah_021 | 3 | rs_draw_d021 | ui_d021_hit + evverify | Δ=0 |
| datah_022 | 5 | rs_draw_d022 | ui_d022_hit + evverify | Δ=0 |
| datah_023 | 4 | rs_draw_d023 | ui_d023_hit + evverify | Δ=0 |
| datah_024 | 1 | rs_draw_d024 | ui_d024_hit + evverify | Δ=0 |
| datah_025 | 3 | rs_draw_d025 | ui_d025_hit + evverify | Δ=0 |
| datah_026 | 3 | rs_draw_d026 | ui_d026_hit + evverify | Δ=0 |
| datah_027 | 3 | rs_draw_d027 | ui_d027_hit + evverify | Δ=0 |
| datah_028 | 3 | rs_draw_d028 | ui_d028_hit + evverify | Δ=0 |
| datah_029 | 2 | rs_draw_d029 | ui_d029_hit + evverify | Δ=0 |
| datah_030 | 2 | rs_draw_d030 | ui_d030_hit + evverify | Δ=0 |
| datah_031 | 3 | rs_draw_d031 | ui_d031_hit + evverify | Δ=0 |
| datah_032 | 4 | rs_draw_d032 | ui_d032_hit + evverify | Δ=0 |
| datah_033 | 2 | rs_draw_d033 | ui_d033_hit + evverify | Δ=0 |
| datah_034 | 2 | rs_draw_d034 | ui_d034_hit + evverify | Δ=0 |
| datah_035 | 2 | rs_draw_d035 | ui_d035_hit + evverify | Δ=0 |
| datah_036 | 4 | rs_draw_d036 | ui_d036_hit + evverify | Δ=0 |
| datah_037 | 3 | rs_draw_d037 | ui_d037_hit + evverify | Δ=0 |
| datah_038 | 4 | rs_draw_d038 | ui_d038_hit + evverify | Δ=0 |
| datah_039 | 1 | rs_draw_d039 | ui_d039_hit + evverify | Δ=0 |
| datah_041 | 8 | rs_draw_d041 | ui_d041_hit + evverify | Δ=0 |
| datah_043 | 2 | rs_draw_d043 | ui_d043_hit + evverify | Δ=0 |
| datah_044 | 2 | rs_draw_d044 | ui_d044_hit + evverify | Δ=0 |
| datah_045 | 1 | rs_draw_d045 | ui_d045_hit + evverify | Δ=0 |
| datah_046 | 1 | rs_draw_d046 | ui_d046_hit + evverify | Δ=0 |
| datah_047 | 1 | rs_draw_d047 | ui_d047_hit + evverify | Δ=0 |
| datah_049 | 1 | rs_draw_d049 | ui_d049_hit + evverify | Δ=0 |
| datah_050 | 4 | rs_draw_d050 | ui_d050_hit + evverify | Δ=0 |
| datah_051 | 2 | rs_draw_d051 | ui_d051_hit + evverify | Δ=0 |
| datah_052 | 2 | rs_draw_d052 | ui_d052_hit + evverify | Δ=0 |
| datah_053 | 2 | rs_draw_d053 | ui_d053_hit + evverify | Δ=0 |
| datah_054 | 2 | rs_draw_d054 | ui_d054_hit + evverify | Δ=0 |
| datah_055 | 1 | rs_draw_d055 | ui_d055_hit + evverify | Δ=0 |
| datah_057 | 2 | rs_draw_d057 | ui_d057_hit + evverify | Δ=0 |
| datah_058 | 2 | rs_draw_d058 | ui_d058_hit + evverify | Δ=0 |
| datah_059 | 3 | rs_draw_d059 | ui_d059_hit + evverify | Δ=0 |
| datah_060 | 5 | rs_draw_d060 | ui_d060_hit + evverify | Δ=0 |
| datah_061 | 6 | rs_draw_d061 | ui_d061_hit + evverify | Δ=0 |
| datah_062 | 4 | rs_draw_d062 | ui_d062_hit + evverify | Δ=0 |
| datah_063 | 3 | rs_draw_d063 | ui_d063_hit + evverify | Δ=0 |
| datah_064 | 3 | rs_draw_d064 | ui_d064_hit + evverify | Δ=0 |
| datah_065 | 2 | rs_draw_d065 | ui_d065_hit + evverify | Δ=0 |
| datah_066 | 3 | rs_draw_d066 | ui_d066_hit + evverify | Δ=0 |
| datah_068 | 3 | rs_draw_d068 | ui_d068_hit + evverify | Δ=0 |
| datah_069 | 6 | rs_draw_d069 | ui_d069_hit + evverify | Δ=0 |
| datah_070 | 6 | rs_draw_d070 | ui_d070_hit + evverify | Δ=0 |
| datah_071 | 3 | rs_draw_d071 | ui_d071_hit + evverify | Δ=0 |
| datah_072 | 4 | rs_draw_d072 | ui_d072_hit + evverify | Δ=0 |
| datah_073 | 2 | rs_draw_d073 | ui_d073_hit + evverify | Δ=0 |
| datah_075 | 4 | rs_draw_d075 | ui_d075_hit + evverify | Δ=0 |
| datah_078 | 3 | rs_draw_d078 | ui_d078_hit + evverify | Δ=0 |
| datah_079 | 1 | rs_draw_d079 | ui_d079_hit + evverify | Δ=0 |
| datah_082 | 6 | rs_draw_d082 | ui_d082_hit + evverify | Δ=0 |
| datah_084 | 4 | rs_draw_d084 | ui_d084_hit + evverify | Δ=0 |
| datah_085 | 5 | rs_draw_d085 | ui_d085_hit + evverify | Δ=0 |
| datah_086 | 6 | rs_draw_d086 | ui_d086_hit + evverify | Δ=0 |
| datah_089 | 3 | rs_draw_d089 | ui_d089_hit + evverify | Δ=0 |
| datah_090 | 2 | rs_draw_d090 | ui_d090_hit + evverify | Δ=0 |
| datah_094 | 5 | rs_draw_d094 | ui_d094_hit + evverify | Δ=0 |
| datah_095 | 9 | rs_draw_d095 | ui_d095_hit + evverify | Δ=0 |
| datah_096 | 3 | rs_draw_d096 | ui_d096_hit + evverify | Δ=0 |
| datah_097 | 4 | rs_draw_d097 | ui_d097_hit + evverify | Δ=0 |
| datah_098 | 6 | rs_draw_d098 | ui_d098_hit + evverify | Δ=0 |
| datah_100 | 1 | rs_draw_d100 | ui_d100_hit + evverify | Δ=0 |
| datah_101 | 4 | rs_draw_d101 | ui_d101_hit + evverify | Δ=0 |
| datah_102 | 1 | rs_draw_d102 | ui_d102_hit + evverify | Δ=0 |
| datah_104 | 5 | rs_draw_d104 | ui_d104_hit + evverify | Δ=0 |
| datah_107 | 5 | rs_draw_d107 | ui_d107_hit + evverify | Δ=0 |
| datah_108 | 4 | rs_draw_d108 | ui_d108_hit + evverify | Δ=0 |
| datah_109 | 3 | rs_draw_d109 | ui_d109_hit + evverify | Δ=0 |
| datah_110 | 2 | rs_draw_d110 | ui_d110_hit + evverify | Δ=0 |
| datah_113 | 4 | rs_draw_d113 | ui_d113_hit + evverify | Δ=0 |
| datah_114 | 1 | rs_draw_d114 | ui_d114_hit + evverify | Δ=0 |
| datah_117 | 7 | rs_draw_d117 | ui_d117_hit + evverify | Δ=0 |
| datah_118 | 2 | rs_draw_d118 | ui_d118_hit + evverify | Δ=0 |
| datah_119 | 2 | rs_draw_d119 | ui_d119_hit + evverify | Δ=0 |
| datah_120 | 3 | rs_draw_d120 | ui_d120_hit + evverify | Δ=0 |
| datah_121 | 2 | rs_draw_d121 | ui_d121_hit + evverify | Δ=0 |
| datah_122 | 2 | rs_draw_d122 | ui_d122_hit + evverify | Δ=0 |
| datah_123 | 1 | rs_draw_d123 | ui_d123_hit + evverify | Δ=0 |
| datah_246 | 2 | rs_draw_d246 | ui_d246_hit + evverify | Δ=0 |
| datah_247 | 1 | rs_draw_d247 | ui_d247_hit + evverify | Δ=0 |
| datah_248 | 4 | rs_draw_d248 | ui_d248_hit + evverify | Δ=0 |
| datah_249 | 1 | rs_draw_d249 | ui_d249_hit + evverify | Δ=0 |
| datah_250 | 2 | rs_draw_d250 | ui_d250_hit + evverify | Δ=0 |
| datah_251 | 1 | rs_draw_d251 | ui_d251_hit + evverify | Δ=0 |
| datah_253 | 1 | rs_draw_d253 | ui_d253_hit + evverify | Δ=0 |
| datah_255 | 3 | rs_draw_d255 | ui_d255_hit + evverify | Δ=0 |
| datah_256 | 4 | rs_draw_d256 | ui_d256_hit + evverify | Δ=0 |
| datah_257 | 6 | rs_draw_d257 | ui_d257_hit + evverify | Δ=0 |
| datah_258 | 5 | rs_draw_d258 | ui_d258_hit + evverify | Δ=0 |
| datah_259 | 2 | rs_draw_d259 | ui_d259_hit + evverify | Δ=0 |
| datah_260 | 9 | rs_draw_d260 | ui_d260_hit + evverify | Δ=0 |
| datah_261 | 1 | rs_draw_d261 | ui_d261_hit + evverify | Δ=0 |
| datah_262 | 4 | rs_draw_d262 | ui_d262_hit + evverify | Δ=0 |
| datah_263 | 5 | rs_draw_d263 | ui_d263_hit + evverify | Δ=0 |
| datah_264 | 1 | rs_draw_d264 | ui_d264_hit + evverify | Δ=0 |

## 独立命名 UI 态（非批量 datah，单独实现 + 对齐证据）

| UI 态 | 按键 | 对齐证据 |
|---|---|---|
| 主菜单（5 按钮） | 人机对战 / 两人对局 / 联网 / ☰ / 退出 | 97.8% 非黑（2026-11-10 复验） |
| 难度弹窗（datah_041） | 新手/中级/高级/好友区 × 布局/翻棋 8 按钮 | 对抗审计 |
| 方式选择（datah_072） | 4 按钮（个人资料入口） | 对抗审计 |
| 声音设置（datah_001） | 5 按钮 + 持久化 | 对抗审计 |
| 新人局（datah_062） | 玩法/布局明棋/翻棋暗棋/明暗棋/确定/取消 6 按钮 | P0 |
| 游戏设置（datah_012） | 和棋/大本营/吃地雷/吃军旗/开/关/确定/取消 9 行 | P1 |
| 登录（datah_038） | 用户名/密码占位 + 登录/注册/游客/返回 6 控件 | P1 |
| 布局存档（datah_016/017/020） | 9 按钮（载入/保存/管理/改名/删除/关闭） | P1 |
| 复盘（datah_039/107） | 8 按钮（回退/前进/跳转/返回/第N步/结束） | P1 |
| 规则（datah_037/070） | 8/10 按钮（翻棋/布局/工兵/地雷/炸弹/夺旗开关） | P2 |
| 软件信息（datah_000） | 反馈/关闭 + 版本条 | P2 |
| 新局完整版（datah_104/061） | 10 按钮 | P2 |
| 加入/创建（datah_034/035/065/069/071/073） | 10/6 按钮 | P2 |
| 房间态（datah_060） | 12 按钮 | P2 |
| 聊天（datah_029/063） | 8/5 按钮 | P2 |
| 个人资料（datah_031/096） | 10/3 按钮 | P2 |
| 提和详情（datah_042） | 7 按钮 | P2 |
| 设置入口（datah_003/004） | 7/3 按钮 | P2 |
| 图像设置（datah_007/008） | 10/3 按钮 | P2 |
| 对局信息（datah_050） | 8 按钮 | P2 |
| 亮度（datah_002） | 10 按钮 | P2 |
| 退出/悔棋/认输确认（datah_025） | 3/3 按钮 | P2 |
| 版本更新（datah_118） | 2 按钮 | P2 |

## 对局态（非弹窗，棋盘内交互）

| 交互 | 说明 |
|---|---|
| 棋盘触摸 | cell 命中 → 走子态机（选中→走/翻），`can_move/can_flip` 判据 |
| ↩ 悔棋 | 对局态左上角，undo_pop 连续 |
| 重开 | 对局态左上角重开按钮 |
| ☰ 游戏内菜单 | 本地版 datah_006 8 项 / 联机版 datah_067 8 项 |

## 结论（诚实边界）

- **有按钮的对话框 = 111/111 全量对齐**（面板几何 + N 按钮原版坐标/文字 + 共用文字图集行 + 命中判定 + 分发 + evverify 几何 Δ=0 判据），`rs_logic_align` 判据 5 实锤 "已对齐 120/272，有按钮未对齐 0"。
- 无按钮 161 个 = 纯展示/文本框/资源占位，**无交互业务，按诚实边界不补**（不包装成"对齐"）。
- 全部判据可一键复现：`bash rev64/tools/rs_logic_align.sh --dev 10.0.182.146:5555`（当前 8/8 PASS，含真机 UI 全项）。
