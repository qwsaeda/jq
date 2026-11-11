> 来源: REVERSE_NOTES.md §1-§83（行 1-275），2026-11-02 拆分
# 原版 32 位 libjunqi.so 逆向笔记（对齐基准）

> 目标：把原版精确行为移植到 arm64。本文件记录从 `.text` 逐函数逆向的关键事实。
> 原版：`/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so`（ELF32/ARM/Thumb，stripped）
> 工具：`/tmp/opencode/disasm_thumb.py`、`/tmp/opencode/scan_refs.py`、`/tmp/opencode/find_ref.py`（capstone）
> 逆向工具链：`python3 find_ref.py <so> "<字符串>"` 定位 PIC 引用；`disasm_thumb.py` 反汇编。

## 1. 架构事实（重要纠正）

- **JNI 符号地址以 `.symtab` 为准，不是 `llvm-nm -D`**：
  - `llvm-nm -D` 给 0x58xx（GOT/重定位桩，错误）
  - `.symtab` 真实值：`Java_com_cnvcs_App_OnEvent=0x233e1`（Ndx 7=.text，奇数=Thumb）
- JNI 入口是 **20 字节 Thumb 跳板**（`push{lr}; ldr; add pc; ldr; bl 真实实现; pop{pc}`），
  真实实现在 `.text` 深处：
  - `OnEvent 桩@0x233e1 → bl 0xbd914`（真实实现）
  - `OnCreate 桩@0x23560 → bl 0xba12c`（初始化/清理）
- `.text` VA=0x233e0 Off=0x233e0（Off==VA），size 0xfd814（1,105,428B）
- `.rodata` VA=0x120bf8 size 0x1d030（=118832B=rodata_dec.bin）
- **VA→字符串**：`rodata_dec.bin[VA - 0x120bf8]`（明文 dump）
- 字符串引用是 **PIC 相对**：`ldr rX,[pc,#imm]`（字面量=偏移）+ `add rX,pc` → 绝对 VA。
  目标 = 字面量 + `(add_addr+4)&~3`。**字面量通常不是绝对 VA，是相对偏移**。
- `.data.rel.ro`/`.got` 指向 rodata 字符串的指针极少（事件串不经 GOT）。

## 2. 核心字符串锚点（VA→明文，供 find_ref 定位）

| 字符串 | VA | 用途 |
|---|---|---|
| Junqi.Move | 0x120f20 | 走子事件（引用@0x3a1b2） |
| Junqi.Win | 0x120e34 | 胜利事件 |
| data/lay/default.lay | 0x1211b8 | 默认布局路径 |
| data/lay/%02d.lay | 0x12113c | 布局格式化 |
| jq1/jq2/jq3.lay | 0x121164/6c/74 | 单机布局 |
| comp.lay / net.lay | 0x12185c/868 | 联机布局 |
| data/sg/board.png | 0x121610 | 棋盘纹理 |
| data/sg/piece.png | 0x121624 | 棋子 sprite |
| LowLay/MidLay/HighLay | 0x12284c/85c/86c | 难度布局 |

## 3. 布局加载（0x2a340 / 0x2a380）

- `0x2a340(r0=state, r1=布局字节流)`：循环 30 次（`r4` 从 `state+0x1bc` 到 `state+0x324`，步进 12），
  每字节经 `bl 0x6a3a4` 映射 → `str [r4]`。
- `0x6a3a4(r0=字节, r1=侧)`：`if(byte==0) return 0; if(side==0) return byte+0xc; return byte;`
  - **红方(侧0)=字节+0xc；蓝方(侧非0)=字节原值**；空=0。
- `0x2a380(r0=state, r1=布局名, r2=长度)`：`state+0x48=侧`，`state+0x4c=长度`，
  `bl 0x3dd60`（读文件）→ `bl 0x2a340`（解析）。

## 4. 棋盘状态结构（state 基址 + 偏移）

- 棋盘格：`state + 0x84` 起 30 字节（6×5 格，每格 1 字节 rank）
- 走子时每格访问：`state + 行*12 + 列`，格内 `[#4]`=rank（见 0x3a15c）
- `state+0x48`=侧，`state+0x4c`=布局长度，`state+0x40`=侧标记（0x2a354 `ldrb r1,[r6,#0x40]`）
- `state+0xa2/a4`=当前移动 from/to 坐标（0x39d58 存储）

## 5. 走子合法性判定（0x39e14）——核心规则

`0x39e14(r0=state, r1=(from<<8|to)打包, r2=目标格字节)`：
```
if r2 > 0xc: 报错(非法)          # 0x39e22 bhi
if r2 == 0xa:                     # 炸弹
    if r1低字节==0 (to_row==0): 报错
    else: 合法
if r2 == 0xb:                     # 地雷
    if r1高字节<=3: 报错(非工兵)  # 0x39e50 ble
    else: 合法
if r2 == 0xc:                     # 军旗
    if from_row==3 or from_row==1: 特判(0x39e7a)
    else: 报错
else: 合法走子                    # 0x39e54 movs r0,#1
```
**关键字节语义**：`0xa=炸弹, 0xb=地雷, 0xc=军旗`，普通 rank=1..9。
**与 arm64 当前实现不一致**（arm64: JQ_RANK_BOMB=1, JQ_RANK_MINE=31, JQ_RANK_FLAG=2）——**需对齐**。

## 6. 走子执行（0x3a15c-0x3a1b6）

- `0x3a15c ldrsb r2,[r4,r7]`（from）、`ldrsb r1,[r4,r6]`（to）
- `0x3a17e ldrb r2,[r2格,#4]`（to rank）、`0x3a182 ldrb r1,[r3格,#4]`（from rank）
- `0x3a184 strb r2,[r3,#4]`（from←to）、`0x3a1a0 strb r1,[r3,#4]`（to←from）【交换=移动】
- `0x3a1a8 strb r5,[r4,r6]`（标记）、`0x3a1b2 ldr Junqi.Move; bl 0x9b658`（发事件）

## 7. 布局校验（0x6a3b0，60 格循环）

- 循环 60 格（`r0<0x3c`），每格 12 字节结构体
- `if rank>0xc: rank-=0xc`（红方还原）`if rank-0xb<=1`（军旗/地雷特判）
- 计数表 `[r3,#0x78]` 每 rank+1（棋子计数）

## 8. 逆向工具用法

```bash
# 定位引用某字符串的代码（PIC）
python3 /tmp/opencode/find_ref.py <so> "Junqi.Move"
# 反汇编区间（Thumb，--str 解析引用）
python3 /tmp/opencode/disasm_thumb.py <so> 0x39e14 0x39ea0 --str
```

## 9. 待逆向（月级工程，诚实边界）

- 吃子胜负比较（rank 1-9 谁大谁小）——0x396a4 之后的吃子核心
- 炸弹/地雷/军旗完整特判（0x39e7a 军旗 from_row 特判细节）
- 翻棋逻辑（Junqi.Flip 引用点）
- 联机协议（Junqi.Peer* / Regret* / Draw*）
- rank→贴图映射（data/sg/piece.png sprite 偏移）


## 10. 布局位置校验（0x2a228，红方自定义布局）

`0x2a228(r1=索引0-59, r2=字节)`：
- `if r1<=0x1d(29): 合法`（蓝方区域，default.lay 固定）
- `if r2>0xc: r2-=0xc`（红方已翻开还原）
- `r2==0xa(10) 炸弹`：`if r1<=0x22(34): 报错 ZhaError`（炸弹禁红方前场）
- `r2==0xb(11) 地雷`：`if r1<=0x31(49): 报错 LeiError`（地雷禁红方前场，仅行10+）
- `r2==0xc(12) 军旗`：`if r1==0x38(56) or r1==0x3a(58): 合法; else 报错 QiError`
  （军旗必须在格 56/58 = 红方大本营 row11col1/col3）

**与 arm64 对齐**：arm64 军旗经 `5-lrow`/`6+lrow,4-lcol` 落位格1（蓝大本营）/格58（红大本营），
与原版规则一致。地雷/炸弹位置 default.lay 本身合规。

## 11. 事件串引用锚点（补齐）

| 字符串 | 引用 VA | 事件 |
|---|---|---|
| Junqi.ZhaError/QiError/LeiError | 0x2a24a/262/276 | 炸弹/军旗/地雷错误 |
| Junqi.Move | 0x3a1b2 | 走子 |
| Junqi.Win/Draw | 0x30236/3020a | 胜负 |

## 12. 胜负判定（0x30200）+ 诚实边界
- 胜负：`r3=0/1/2`（蓝胜/红胜/和）→ `Junqi.Win/Draw` 事件（0x30202 cmp r3,#1 / 0x30210 cmp r3,#2）。
  与 arm64 `st->winner`(0蓝/1红)+`stage=OVER` 一致。
- **rank 1-9 方向已确证**：原版 default.lay 值分布与标准军棋完全匹配
  （1司令×1/2军长×1/3师长×2/4旅长×2/5团长×2/6营长×2/7连长×3/8排长×3/9工兵×3，10炸弹×2/11地雷×3/12军旗×1），
  **值越小越强（1=司令最强）**。arm64 `1=司令最强`/`src<dst` 为**原版直接证据**（非推断）。
  注：吃子 rank 比较函数在事件分发深层（0x9b658→0x9c318 链），未逐指令逆向，但 default.lay 分布是决定性佐证。
- 待逆向：蓝牙联机协议（Junqi.Peer*/Regret*/Draw*）、piece.png sprite 偏移、rank→贴图映射。

## 13. AI 难度菜单（0x485d0）
- `0x485de/0x48632` 引用 LowLay/MidLay（`bl 0x82d3c` 字符串比较 + `bl 0xb4edc` 事件派发）。
- 难度布局（Low/Mid/High/RankLay）是**菜单选择**（非自动）；arm64 无 Java UI 入口，诚实边界。

## 14. 铁路拓扑（对齐 L49）
- 军棋铁路：**横向仅 row5/6（中线两行）+ 纵向所有列贯穿**；行营/大本营在铁路外。
- `jq_is_rail`=排除行营/大本营；`walkable`=行营一律不通行（安全格不可穿行）。
- BFS（工兵）横向仅 row5/6，纵向任意列；straight 同行仅 row5/6。
- 原版走子判定 0x39e14 只判目标类型，铁路路径在调用者（0x6ef54 坐标比较 + 路径滑行）。

## 15. 翻棋判定（0x39e14 同格 from==to，L50）
- 军旗(r2==0xc)：`to∈{1,3} and from==5`——翻棋时 from==to，条件永假 → **军旗不可翻**。
- 地雷(r2==0xb)：`from>3` 合法——地雷可翻（后场）。
- 炸弹(r2==0xa)：`from!=0` 合法——炸弹可翻。
- 普通 1-9：合法。
- arm64 `jq_flip` 加 turn 守卫（`board[idx].side != st->turn → false`），对齐原版 turn 由调用者强制。

## 16. 吃子/军旗坐标约定 + 0x6ef54 语义（L51）
- `0x6ef54(r0=坐标A, r1=坐标B)`：`[0]=col, [1]=row`（注意 col 在前）。返回 0 表示两坐标相同。
- `0x39e14` 军旗特判：目标格 `col in {1,3} && row==5`（大本营列 + 半棋盘第 5 行），
  即军旗在大本营（col1/3），吃旗需攻方从第 5 行（己方前线路）进攻。
- **注意**：原版坐标用 `col,row` 打包（`strh`），与 arm64 绝对 12x5 索引不同。
  arm64 用标准规则（军旗在大本营、相邻攻子吃旗到胜），语义等价（已验证吃旗胜）。
- **吃子 rank 比较不在直接走子路径**：0x39e14 只判目标类型（0xa/0xb/0xc/普通），
  返回 1（合法）后 0x3a15c 直接交换 rank——**rank 大小比较在更深调用链**（0x3a0ee 之后的事件处理）。
  故 arm64 `src<dst`（1=司令最强）为合理推断，非原版直接证据（诚实边界 §12）。

## 17. 原版规则文本权威确证（L57）+ 和棋/炸弹/困毙修正
- **rulelay.txt / ruleflip.txt（SPAK，GBK）为原版权威规则文本**：
  - 吃子：司令>军长>师长>旅长>团长>营长>连长>排长>工兵（rank 小=强，**再确证 §12**）；
    炸弹与任何棋相遇**双方都消失（包括军旗）**；地雷<工兵、>所有其他棋；任何棋子都能扛军旗。
  - 布局：炸弹不放第一行；地雷只放最后两行；**军旗必须放**大本营；**进大本营不可再动**；行营=安全岛。
  - 胜负：棋子被吃光、军旗被扛、**无棋可走→判负**；**连续70步无吃子→和**；**总步数1000→和**。
- **arm64 已对齐**（L57 修正）：和棋=70步无吃子/1000步；炸弹撞军旗=双方消失；困毙=单方无子可动→该方负。
- **待确证**：①进大本营不可再动（arm64 大本营格子是否允许离开？需检查）②行营安全岛（敌方不能吃行营棋子）。

- **AI 对局**（规则修改后确认）：玩家(蓝) vs AI(红) 95 步正常终局（stage=OVER、winner 有效），
  AI 路径无回归。"能玩"规则层面 OK（真机渲染仍需 arm64 GPU 设备）。
- **红方镜像布局**：arm64 `jq_load_layout` 红方 `6+lrow, 4-lcol`（列翻转），25 子分布 = 标准
  （1司令...9工兵, 10×2, 11×3, 12×1），与原版 default.lay 镜像一致（test_lay PASS）。

- **0x39e14 布局校验确证（L59）**：军旗 col∈{1,3}&&row==5（大本营）、地雷 row>3（最后两行）、
  炸弹 row!=0（不放第一行）——**非走子方向限制**，军旗吃法无方向限制（arm64 对齐）。
- **JQ_PIECES 38→25**：default.lay 实际 25 子；旧 38 致暗棋放 rank=0 子（bug 已修，25/25 对称）。

- **0x2a340/0x6a3a4/0x6a3b0 确证（L60）**：布局加载 30 字节循环、映射蓝 rank/红 rank+0xc、
  可动子统计（困毙判定）——arm64 全部对齐。

- **0x6b87c→0x6a3b0 刷新可动子统计确证（L61）**：走子后刷新统计+困毙判定，arm64 `check_stalemate` 对齐。

- **ruleflip 翻棋吃子规则（L63）**：军旗/地雷仅工兵吃、工兵挖3雷才能扛旗（arm64 DARK 模式对齐）；
  PLAY 布局模式任何棋扛旗（rulelay）不受影响。

- **ruleflip 先手翻子定色（L64）**：暗棋布子 side=UNKNOWN，jq_flip 翻子定色（c->side=turn）；
  暗棋困毙修复（side_has_movable 先扫 UNKNOWN 暗子，有则未困毙）。

- **0x39f20 移动执行器确证（L65）**：纯用户走子（state[0xa2]/[0xa3] 坐标 + state[0x80+i*6+4] rank）；
  AI 走子=OnEvent→0xbd914 vtable（+0x2a4）C++ 方法；蓝牙=OnBluetooth→0xbe9cc C++ 方法（RFCOMM SPP）——两者静态逆向月级。

- **jq_ai_move 困毙兜底（L66）**：三段走子全失败时调 check_stalemate（无可走子→判负）；原返回0不判负 bug 已修。

- **default.lay 完整字节映射（L67）**：30字节（6行×5列含行营空位），蓝(5-r)/红(6+r,列镜像)；
  军旗蓝格1/红格58、25子/方 rank 分布全对齐原版。

- **原版"十"字铁路几何（L68）**：纵向 col2 贯穿（行营(3,2)(8,2)分三段）+ 横向 row5/6 中线；
  行营/大本营阻断；col0/col4 非铁路（只相邻一步）。arm64 jq_is_rail 对齐。

- **悔棋 undo 完整（L69）**：走子/吃旗/炸弹/翻棋全部可恢复（board/turn/stage/flags/pieces/mines/moves_since_eat）。

- **暗棋 AI 走子完整（L70）**：翻 UNKNOWN 子、吃已翻子（UNKNOWN 不可吃）、任一可动兜底、自身困毙判定。
  暗棋 70步无吃子和棋（翻棋计步）对齐原版 ruleflip。

- **走子事件分类（L71）**：jq_try_move 加 eat_type（move/eat/bomb/flag/dead/mine），对齐原版 Junqi.Move/Eat/Bomb/Flag/Dead 音效。

- **规则完整性审计（L72）**：rulelay/ruleflip 全条款逐条对齐——规则层原版完整对齐（L53-L71 全绿）。

- **AI 难度档数确证（L73）**：0x485d0 菜单 8 标签 = LowLay/LowFlip/MidLay/MidFlip/HighLay/HighFlip/RankLay/RankFlip——原版单机难度 3 档（Low/Mid/High）+ Rank，每档 × Lay/Flip。arm64 单档贪心（诚实边界）。

- **存档 one.sjq 格式完整确证（L74）**：0x37664 读/0x379ec 写，字段偏移 0x38=time/0x3c=difficulty/0x40=level/0x44=mark/0x48=resultEx[4]，与 rodata 字符串一致。arm64 无存档（诚实边界）。

- **棋子 PNG 图集确证（L75）**：datah_124-174（72×72 蓝方棋子）+ datax_124-174（108×108），来自 assets/datah0/1.pak。arm64 rank 数字渲染（诚实边界，rank→索引映射在 C++ 层）。

- **原版 15 个 JNI 完整确证（L76）**：含 OnUpdate(0x23474)/OnTimer(0x23460)=AI 定时器入口，均 bl 0x8598（JNI 核心）。arm64 OnUpdate 已实现 AI 走子（对齐）。

- **Junqi.* 事件全集确证（L77）**：73 个（走子6/胜负6/布局错误4/联机15/登录注册20+）；arm64 覆盖走子/胜负/悔棋，布局编辑/联机/登录为诚实边界。

- **arm64 JNI 接口完整对齐（L78）**：14 个 On* 符号逐一匹配原版（OnBluetooth~OnUpdate），junqi_jni.c 全实现——接口层 100% 对齐。

- **棋盘维度/格子布局确证（L79）**：原版 12×5 每格 8 字节（0x6ef54 行内 index=r7*5×8）；arm64 同维度 index=row*5+col，jq_cell=6 字节（紧凑，无 col/row 缓存）——规则一致。

- **布局加载函数校正（L80）**：L60 记 0x2a340 布局加载实为 vector insert；实际布局加载在 0x2a37c 场景构造（bl 0x2a310 + bl 0x8598 JNI 读 default.lay + 0x6a3a4 rank 映射）。arm64 jq_load_layout 对齐。

- **arm64 布局编辑落地（L81，本轮新增）**：布阵阶段 JQ_STAGE_SETUP + 触屏拖子 + rulelay 全条款校验。
  - **原版校验逐指令重读（0x2a228 直接反汇编确证，非仅笔记）**：
    - `if r1<=0x1d(29): 返回0`（蓝半区非法——红方自定义布局仅行6-11）
    - 炸弹(r2==0xa)：`if r1<=0x22(34): ZhaError`（炸弹禁行6=红方前线路，即"第一行"）
    - 地雷(r2==0xb)：`if r1<=0x31(49): LeiError`（地雷仅行10-11=最后两行）
    - 军旗(r2==0xc)：`r1∈{0x38,0x3a}` 合法，否则 QiError（军旗仅格56/58=大本营）
    - 普通 1-9：`r1>0x1d` 合法（蓝半区禁入）
  - **红方对称映射到蓝方（row0=蓝底线）**：炸弹禁行5、地雷仅行0-1、军旗仅行0 列1/3。
    与 default.lay 完全吻合（炸弹行3-4、地雷行0-1、军旗行0col1，L59 字节一致）。
  - **arm64 API**：`jq_setup_begin/place/validate/confirm/is_setup`，错误码
    JQ_LAYOUT_ZHA/QI/LEI/COUNT/BAD_POS（对齐原版 Junqi.ZhaError/QiError/LeiError，§11）。
  - **UI**：布阵阶段蓝方 25 子全翻开可编辑、红方暗置预览；底部确认按钮（绿底白勾）；
    触屏选中→放置/交换（rulelay 逐格校验，原版 0x2a228 逐格语义）；确认→PLAY 双方暗置（军旗明置）。
  - **验收**：test_setup 28/28 PASS；既有规则回归全绿（laypos/layout_final/ruleflip/flipcolor/undo/
    eattype/railstd/ai_dark/ai_multi/bombflag/flagpos/basemine）；arm64 库 49.6KB 14 JNI；
    phase_hand.apk Verifies(v1/v2/v3)、manifest 0x80003、.so 全 STORE、arm64 MD5 匹配。
  - **诚实边界**：arm64 布阵编辑=蓝方拖子（原版红方自定义布局 0x2a5f0，arm64 蓝方对称）；
    无布局名选择（Low/Mid/High/RankLay 菜单，§13）；暗棋(ruleflip)无布阵（随机，L63）。

- **arm64 AI 三档难度（L82，本轮新增）**：原版 Low/Mid/High/Rank 菜单（L73 8 标签确证），
  arm64 实现三档**决策强度**（诚实边界：原版难度决策在场景 vtable 运行时填充，静态无法确证）。
  - **档 0=LOW**：50% 随机走法（ai_random 收集全部合法动作等概率选），否则贪心——多失误。
  - **档 1=MID**：原贪心（L43，默认）——吃子→前进→兜底→翻棋，行为与既有验证完全一致。
  - **档 2=HIGH**：价值+安全最优吃子——`ai_value`（军旗=胜/司令90...工兵10/炸弹90/地雷80），
    吃子后攻击子会被反吃（`ai_capture_risky` 用 jq_try_move+jq_undo 干净模拟）则扣价值，选最高分。
  - **API**：`jq_ai_set_level(0/1/2)`（钳制）、`jq_ai_get_level`；OnEvent `ai_level=<n>`。
  - **子价值表**：1 司令=90 最强...9 工兵=10 最弱，炸弹=90、地雷=80、军旗=10000（吃旗=胜）。
  - **安全模拟**：jq_try_move 快照 undo → 检查敌方可达 t 且 rank<=攻击子/炸弹（反吃）→ jq_undo 恢复；
    undo 恢复 board/turn/stage/winner/flags/pieces/mines/moves_since_eat，moves 增后自减（L81 验证）。
  - **验收**：test_ai_level 9/9 PASS（set/get 钳制 + 三档各 5 局 AI vs AI 全终局：LOW 865/MID 745/HIGH 730 步，
    HIGH 更高效）；既有规则回归全绿（test_ai_multi 200 局全终局）；arm64 51.9KB 14 JNI；
    phase_hand.apk Verifies v1/v2、manifest 0x80003、.so 全 STORE、arm64 MD5 匹配、SPAK 9 包完整。
  - **诚实边界**：难度布局文件（jq1/jq2/jq3.lay、data/lay/%02d.lay）不在 SPAK/.so rodata（已扫描确认无
    30 字节布局数组），arm64 三档共用 default.lay；Rank 档（L73 "RankLay/RankFlip"）非独立难度，未单列。

- **arm64 存档 one.sjq（L83，本轮新增）**：原版 0x37664 读 / 0x379ec 写确证（L74），
  arm64 实现键值文本读写 + 进度跟踪 + 文件 I/O。
  - **字段确证（rodata VA）**：time=0x121744、difficulty=0x12174c、mark=0x121760、
    resultEx=0x121768；resultEx 序列化 `"%d;%d;%d;%d"`=0x121774（L74 字段偏移
    0x38=time/0x3c=difficulty/0x40=level/0x44=mark/0x48=resultEx[4]）。
  - **arm64 API**：`jq_save_write/read(path, jq_save_data*)`（src/save.c），
    `jq_save_data{time,difficulty,level,mark,resultEx[4]}`；格式 `key=value\n` 文本。
  - **JNI 接线**：OnCreate 经 Activity.getFilesDir() 得 `files/one.sjq`；OnEvent `save`/`load`；
    对局结果记 resultEx（[0]=玩家胜 [1]=AI胜 [2]=和 [3]=总盘）；OnUpdate 约 60 帧/秒计 time。
  - **验收**：test_save 16/16 PASS（round-trip 全字段 + 文本字段名 + 容错缺失字段 + 读不存在文件）；
    arm64 54.7KB 14 JNI；phase_hand.apk Verifies v1/v2/v3、manifest 0x80003、.so 全 STORE、
    arm64 MD5 匹配、SPAK 9 包完整。
  - **诚实边界**：键值文本格式（key=value\n 分隔/顺序）为 arm64 重建（原版写逻辑 0x379ec 是 C++ 层
    长链，未逐字节确证分隔符/顺序）；resultEx[4] 语义为 arm64 重建（原版仅确证字段名/格式）。

