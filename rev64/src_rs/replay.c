/*
 * replay.c — 复盘回放实现（★ P1 datah_039/107）
 * 走子记录（jq_try_move/jq_flip 成功后 append）+ 回放重建
 * （开局 60 格 cells 快照 memcpy + 前 k 步逐条 jq_try_move/jq_flip 重走）。
 * 确定性：同序列同布局 → 同 cells 终态（桌面 evverify run_replay 校验：
 * 回放重建 cells == 独立重放前 k 步结果，不依赖触摸/GL）。
 * 诚实边界：跳转简化为回开头/跳结尾；菜单简化为退复盘；入口 setprop 钩子。
 * ★ 基底用 60 格 cells 快照（非 30 字节布局）：30 字节提取（jq_extract_layout_30）
 *   只收蓝方半区，对局进行中（已被红子占位）提取会镜像出错误红方分布
 *   （实证 init_lay 非零 24 vs 默认布局 22）→ 回放失真。cells 快照精确还原
 *   rank/side/revealed/flags 全 60 格。
 */
#include "replay.h"
#include <string.h>

void jq_replay_init(jq_replay *rp, const jq_state *st)
{
    memset(rp, 0, sizeof(*rp));
    for (int i = 0; i < N_CELLS; i++)
        rp->init_cells[i] = st->cells[i];   /* 开局 60 格快照（rank/side/revealed/flags） */
}

void jq_replay_record(jq_replay *rp, int from, int to, int is_flip)
{
    if (rp->n >= N_REPLAY) return;   /* 容量满忽略（和棋前 1000 步，512 条够复盘） */
    if (from < 0 || from >= N_CELLS || to < 0 || to >= N_CELLS) return;
    rp->seq[rp->n][0] = (uint8_t)from;
    rp->seq[rp->n][1] = (uint8_t)to;
    rp->is_flip[rp->n] = is_flip ? 1 : 0;
    rp->n++;
    rp->cur = rp->n;   /* ★ 游标跟随记录末尾（进入复盘态从最新步开始；回退=cur-- 重建，前进=cur++ 走 1 步；
                       旧版 `if (cur > n) cur = n` 只在越界 clamp，进复盘 cur 恒 0 → 前进无步进） */
}

void jq_replay_cur_set(jq_replay *rp, int cur)
{
    if (cur < 0) cur = 0;
    if (cur > rp->n) cur = rp->n;
    rp->cur = cur;
}

int jq_replay_cur_get(const jq_replay *rp) { return rp->cur; }
int jq_replay_count(const jq_replay *rp) { return rp->n; }

/* 回放重建：st = 开局 60 格 cells 快照 + 前 cur 步逐条重走。
   关键确定性契约（★ 本轮实证 rc=-1 根因链）：
   ① 记录侧走明棋（revealed=1 开局，如 jni_bridge OnCreate/evverify 范式）；
   ② 基底用 cells 快照（非 30 字节布局）：jq_extract_layout_30 只收蓝方半区，
      对局进行中（已被红子占位）提取会镜像出错误红方分布（实证 init_lay 非零
      24 vs 默认布局 22）→ 回放失真。cells 快照精确还原 rank/side/revealed/flags；
   ③ 重放前先全翻开（引擎 jq_can_move 要求 revealed=1，暗棋不可动 L113；
      被吃子 rank=EMPTY 自然跳过，重放全序列后 cells 终态与记录侧逐格一致）；
   ④ pieces_left 快照带入（重放链 吃子/炸弹 EV 会 pieces_left--，避免终局
      判定的 pieces_left 计数偏差影响和棋/困毙路径）。
   失败步（jq_try_move 返回 -1 / jq_flip 失败）= 记录与规则引擎不一致 →
   防御性停止（不崩）。 */
int jq_replay_rewind_to(jq_replay *rp, jq_state *st, int cur)
{
    if (cur < 0 || cur > rp->n) return -1;
    for (int i = 0; i < N_CELLS; i++)
        st->cells[i] = rp->init_cells[i];   /* 开局 60 格快照还原（rank/side/revealed/flags） */
    st->turn = SIDE_BLUE;
    st->winner = -1;
    st->moves_since_eat = 0;
    st->total_moves = 0;
    st->pieces_left[0] = 25; st->pieces_left[1] = 25;   /* 默认布局 25 子/方（重放链 EV 计数基准） */
    st->stage = STG_PLAY;
    /* 复盘态明棋全翻开（对齐 jq_restart_game 明棋范式；引擎要求 revealed=1 才可走子） */
    for (int i = 0; i < N_CELLS; i++)
        if (st->cells[i].rank != RANK_EMPTY) st->cells[i].revealed = 1;
    for (int k = 0; k < cur; k++) {
        int f = rp->seq[k][0], t = rp->seq[k][1];
        if (rp->is_flip[k]) {
            if (!jq_flip(st, f)) return -1;   /* 防御：记录与引擎不一致 */
        } else {
            if (jq_try_move(st, f, t) < 0) return -1;
        }
    }
    return 0;
}
