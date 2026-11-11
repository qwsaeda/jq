/*
 * replay.h — 复盘回放（★ P1 datah_039/107：走子记录 + 逐步回放）
 * 业务：记录整局走子/翻棋序列 → 复盘态（UI_REPLAY）下 回退/前进/跳转/返回 控制条
 *      （datah_107 5 按钮）逐步重放。回放 = 初始布局（jq_extract_layout_30 提取的
 *      30 字节）+ 前 k 步逐条 jq_try_move/jq_flip 重走（确定性，桌面 evverify 可验证）。
 * 诚实边界（勿包装成完整版）：
 *   - 跳转 Jump 简化为"回开头(cur=0)/跳结尾(cur=n)"二选一循环（无步进输入框）
 *   - 菜单 Menu 简化为"退复盘回对局"（无复盘内子菜单）
 *   - datah_039 入口 BackDlg 浮标命中 flaky → 用 setprop jq.replay 测试钩子触发
 *   - 复盘态不画真实棋子图（渲染层复用棋盘现有占位范式）
 */
#ifndef RS_REPLAY_H
#define RS_REPLAY_H
#include "junqi.h"

/* ★ AUD-1000 B02-064（P1 已修）：记录容量 512 → 1100——旧 512 按"和棋前 1000 步"
   注释但 1000 步 = 500 蓝 + 500 红（双方各 1000 总步阈值），单侧 1000 步走子
   + 翻棋可超 512（长局复盘截断）。扩 1100（1000 + 100 余量），seq 内存
   1100*2 + is_flip 1100*4 ≈ 7.6KB（jq_replay 栈变量由调用方持有，无压力）。 */
#define N_REPLAY 1100

typedef struct {
    uint8_t seq[N_REPLAY][2];  /* 每步 from_cell / to_cell（cell 索引 0-59；翻棋 from==to） */
    int     is_flip[N_REPLAY];  /* 0=走子 1=翻棋 */
    int     n;                  /* 已记录步数 */
    int     cur;                /* 回放游标（重放到前 cur 步的位置） */
    /* ★ AUD-1000 B02-066/067（P1 已修）：开局快照字段——旧版 rewind 硬编码
       turn=SIDE_BLUE（ui_first=1 红先手时重放第 1 步 turn 判定错误）+
       硬编码 pieces_left=25/25（非 25 子开局偏差）。init 时快照，
       rewind 用快照值。 */
    uint8_t turn_init;          /* 开局 turn（ui_first=1 → RED 先手） */
    uint8_t mode_init;          /* 开局 mode（LAY/FLIP 复盘语义） */
    jq_cell init_cells[N_CELLS];  /* 开局 60 格快照（rank/side/revealed/flags，重放基底） */
} jq_replay;

/* 开局/重开/布局载入时调用：清记录 + 存开局 30 字节布局 */
void jq_replay_init(jq_replay *rp, const jq_state *st);
/* 每次走子/翻棋成功后调用：追加 1 步（容量满则忽略，诚实边界 512 条） */
void jq_replay_record(jq_replay *rp, int from, int to, int is_flip);
void jq_replay_cur_set(jq_replay *rp, int cur);    /* 越界 clamp [0,n] */
int  jq_replay_cur_get(const jq_replay *rp);
int  jq_replay_count(const jq_replay *rp);
/* 回放重建：st = 初始布局 + 前 cur 步逐条重走（回退/前进/跳转 共用此入口）。
   返回 0 成功 / -1 越界（cur>n 不重建）。cur=0 = 纯初始布局。 */
int  jq_replay_rewind_to(jq_replay *rp, jq_state *st, int cur);
#endif
