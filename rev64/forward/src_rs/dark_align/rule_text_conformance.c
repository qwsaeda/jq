/* rule_text_conformance.c — 以原版自带规则文本为判据的一致性测试
 * 依据（原版 SPAK 资源原文）：
 *   rev64/dist/spak_res/data_junqi_rulelay.txt（明棋）
 *   rev64/dist/spak_res/data_junqi_ruleflip.txt（暗棋）
 * 逐条机判，输出 PASS/FAIL。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int pass = 0, fail = 0;

static void put(jq_state *st, int row, int col, int rank, int side, int rev)
{
    jq_cell *c = &st->cells[cell_index(row, col)];
    memset(c, 0, sizeof(*c));
    c->rank = (uint8_t)rank; c->side = (uint8_t)side;
    c->revealed = (uint8_t)(rev ? 1 : 0);
    if (rank == RANK_FLAG) c->flags = JQF_FLAG;
    if (rank == RANK_MINE) c->flags = JQF_MINE;
    if (rank == RANK_BOMB) c->flags = JQF_BOMB;
}

static void clr(jq_state *st)
{
    memset(&st->cells, 0, sizeof(st->cells));
    st->stage = STG_PLAY; st->winner = JQ_WIN_NONE; st->turn = SIDE_BLUE;
    st->moves_since_eat = 0; st->total_moves = 0;
    st->stale_dirty = 1;
}

static void chk(const char *what, int cond)
{
    printf("%s %s\n", cond ? "PASS" : "FAIL", what);
    if (cond) pass++; else fail++;
}

int main(void)
{
    jq_state st;
    jq_state_init(&st);

    /* ---------- 明棋：公路线一步 ---------- */
    st.mode = JQ_MODE_LAY;
    clr(&st); put(&st, 4, 1, 5, SIDE_BLUE, 1);      /* 蓝 rank5 @(4,1) */
    chk("[明棋] 公路相邻一步合法 (4,1)->(4,2)",
        jq_can_move(&st, cell_index(4,1), cell_index(4,2)));
    chk("[明棋] 公路跨两格非法 (4,1)->(4,3)",
        !jq_can_move(&st, cell_index(4,1), cell_index(4,3)));

    /* ---------- 明棋：铁路直走（col2 纵线）----------
       夹具修正：col2 纵线被中心行营 (3,2)/(8,2) 阻断，取 row4-row7 段。 */
    clr(&st); put(&st, 4, 2, 5, SIDE_BLUE, 1);      /* 蓝 @(4,2) 铁路上 */
    chk("[明棋] 非工兵铁路纵向直走远格 (4,2)->(7,2)",
        jq_can_move(&st, cell_index(4,2), cell_index(7,2)));
    put(&st, 5, 4, 5, SIDE_BLUE, 1);                /* 蓝 @(5,4) 横线端 */
    chk("[明棋] 非工兵铁路不能拐弯 (5,4)->(6,2)",
        !jq_can_move(&st, cell_index(5,4), cell_index(6,2)));

    /* ---------- 明棋：工兵铁路任意拐弯 ---------- */
    clr(&st); put(&st, 5, 4, RANK_WORKER, SIDE_BLUE, 1);   /* 工兵 @(5,4) */
    chk("[明棋] 工兵铁路拐弯 (5,4)->(6,2)",
        jq_can_move(&st, cell_index(5,4), cell_index(6,2)));

    /* ---------- 明棋：铁路中间有子不可穿越 ---------- */
    clr(&st);
    put(&st, 2, 2, 5, SIDE_BLUE, 1);
    put(&st, 5, 2, 7, SIDE_BLUE, 1);                /* 己方子挡在 (5,2) */
    chk("[明棋] 铁路滑行被中间子阻断 (2,2)->(9,2) 非法",
        !jq_can_move(&st, cell_index(2,2), cell_index(9,2)));

    /* ---------- 明棋：行营安全岛（敌方不能吃行营中的子）---------- */
    clr(&st);
    put(&st, 3, 2, 1, SIDE_RED, 1);                 /* 红司令在行营 (3,2) */
    put(&st, 3, 1, 3, SIDE_BLUE, 1);                /* 蓝师长相邻 */
    chk("[明棋] 行营安全岛：敌方不能吃行营中的棋子",
        !jq_can_move(&st, cell_index(3,1), cell_index(3,2)));

    /* ---------- 明棋：进入大本营的棋子不能再移动 ---------- */
    clr(&st);
    put(&st, 0, 1, 5, SIDE_BLUE, 1);                /* 蓝子已在蓝大本营 (0,1) */
    chk("[明棋] 大本营内棋子不能再移动",
        !jq_can_move(&st, cell_index(0,1), cell_index(1,1)));

    /* ---------- 明棋：吃子大小（司令 > 军长，小 rank 强）---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, 1, SIDE_BLUE, 1);             /* 蓝司令 rank1 */
        put(&a, 4, 2, 2, SIDE_RED, 1);              /* 红军长 rank2 */
        int ev = jq_try_move(&a, cell_index(4,1), cell_index(4,2));
        chk("[明棋] 司令吃军长：攻方留 EV_EAT",
            ev == EV_EAT && a.cells[cell_index(4,2)].rank == 1 &&
            a.cells[cell_index(4,1)].rank == RANK_EMPTY);
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, 2, SIDE_BLUE, 1);             /* 蓝军长 rank2 */
        put(&a, 4, 2, 1, SIDE_RED, 1);              /* 红司令 rank1 */
        int ev = jq_try_move(&a, cell_index(4,1), cell_index(4,2));
        chk("[明棋] 军长撞司令：攻方死 EV_DEAD，司令留",
            ev == EV_DEAD && a.cells[cell_index(4,2)].rank == 1 &&
            a.cells[cell_index(4,1)].rank == RANK_EMPTY);
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, 3, SIDE_BLUE, 1);
        put(&a, 4, 2, 3, SIDE_RED, 1);
        int ev = jq_try_move(&a, cell_index(4,1), cell_index(4,2));
        /* 2026-11-14 原版 0x31de0 实证：同级同归事件 = EV_EAT（Junqi.Eat） */
        chk("[明棋] 同级同归于尽 EV_EAT 两格空（原版 0x31de0 实证）",
            ev == EV_EAT && a.cells[cell_index(4,1)].rank == RANK_EMPTY &&
            a.cells[cell_index(4,2)].rank == RANK_EMPTY);
    }

    /* ---------- 明棋：炸弹与任何棋子相遇双方消失 ---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, RANK_BOMB, SIDE_BLUE, 1);
        put(&a, 4, 2, 1, SIDE_RED, 1);
        int ev = jq_try_move(&a, cell_index(4,1), cell_index(4,2));
        /* 夹具修正：炸弹同归后双方均无子 → stalemate_after 判 Draw 并置 OVER
           （规则原文"棋子被吃光…判负"，双方都吃光 = 无胜负 → 原版 winner=3 和棋）。
           本用例只断言"双方消失 + EV_BOMB"，终局归胜由专门用例覆盖。 */
        chk("[明棋] 炸弹撞司令：双方消失 EV_BOMB",
            ev == EV_BOMB && a.cells[cell_index(4,1)].rank == RANK_EMPTY &&
            a.cells[cell_index(4,2)].rank == RANK_EMPTY);
    }
    /* 炸弹炸军旗 → 双方消失 + 攻方胜 */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 10, 1, RANK_BOMB, SIDE_BLUE, 1);    /* 蓝炸弹 @(10,1) */
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);     /* 红军旗 @(11,1) 大本营 */
        int ev = jq_try_move(&a, cell_index(10,1), cell_index(11,1));
        chk("[明棋] 炸弹炸军旗：双方消失且攻方(蓝)胜",
            ev == EV_BOMB && a.stage == STG_OVER && a.winner == JQ_WIN_BLUE);
    }

    /* ---------- 明棋：地雷小于工兵、大于其他 ---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, RANK_WORKER, SIDE_BLUE, 1);
        put(&a, 4, 2, RANK_MINE, SIDE_RED, 1);
        int ev = jq_try_move(&a, cell_index(4,1), cell_index(4,2));
        /* 夹具修正：工兵挖雷后**工兵位移到目标格**（*dst=*src，原版 0x3a15c 交换语义），
           源格清空——旧断言写"工兵留原格"错误。 */
        chk("[明棋] 工兵挖雷：工兵进目标格、雷消失 EV_MINE",
            ev == EV_MINE && a.cells[cell_index(4,2)].rank == RANK_WORKER &&
            a.cells[cell_index(4,1)].rank == RANK_EMPTY);
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, 1, SIDE_BLUE, 1);             /* 司令 */
        put(&a, 4, 2, RANK_MINE, SIDE_RED, 1);
        int ev = jq_try_move(&a, cell_index(4,1), cell_index(4,2));
        chk("[明棋] 非工兵撞雷：攻方死、雷留 EV_DEAD",
            ev == EV_DEAD && a.cells[cell_index(4,2)].rank == RANK_MINE &&
            a.cells[cell_index(4,1)].rank == RANK_EMPTY);
    }

    /* ---------- 明棋：任何棋子都能扛军旗 ---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 10, 1, 9, SIDE_BLUE, 1);            /* 工兵也好、任意子 */
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        int ev = jq_try_move(&a, cell_index(10,1), cell_index(11,1));
        chk("[明棋] 任何棋子扛军旗即胜 EV_FLAG",
            ev == EV_FLAG && a.stage == STG_OVER && a.winner == JQ_WIN_BLUE);
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 10, 1, 5, SIDE_BLUE, 1);            /* 普通子（非工兵） */
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        int ev = jq_try_move(&a, cell_index(10,1), cell_index(11,1));
        chk("[明棋] 普通子也能扛军旗",
            ev == EV_FLAG && a.winner == JQ_WIN_BLUE);
    }

    /* ---------- 明棋：70 步无吃子判和 ---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, 5, SIDE_BLUE, 1);
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        put(&a, 10, 0, RANK_MINE, SIDE_RED, 1);
        a.moves_since_eat = 69;
        jq_try_move(&a, cell_index(4,1), cell_index(4,2));   /* 第 70 步无吃子 */
        chk("[明棋] 连续 70 步无吃子判和", a.stage == STG_OVER && a.winner == JQ_WIN_DRAW);
    }

    /* ---------- 明棋：1000 步判和 ---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 4, 1, 5, SIDE_BLUE, 1);
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        put(&a, 10, 0, RANK_MINE, SIDE_RED, 1);
        a.total_moves = 999; a.moves_since_eat = 0;
        jq_try_move(&a, cell_index(4,1), cell_index(4,2));
        chk("[明棋] 总步数达 1000 判和", a.stage == STG_OVER && a.winner == JQ_WIN_DRAW);
    }

    /* ---------- 明棋：无棋可走判负（困毙）---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        /* 红方只剩被冻在自家大本营的军旗 + 明棋雷，蓝方有可动子 */
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        put(&a, 5, 0, RANK_MINE, SIDE_RED, 1);
        put(&a, 4, 1, 5, SIDE_BLUE, 1);     /* 蓝有可动子 */
        put(&a, 0, 1, RANK_FLAG, SIDE_BLUE, 1);
        a.turn = SIDE_BLUE;
        jq_try_move(&a, cell_index(4,1), cell_index(4,2));   /* 蓝走子 → 轮到红 */
        chk("[明棋] 轮红无棋可走 → 红判负（蓝胜）",
            a.stage == STG_OVER && a.winner == JQ_WIN_BLUE);
    }

    /* ---------- 暗棋：未翻子不可走 ---------- */
    st.mode = JQ_MODE_FLIP;
    clr(&st); put(&st, 4, 1, 5, SIDE_BLUE, 0);
    chk("[暗棋] 未翻开的己方子不可走",
        !jq_can_move(&st, cell_index(4,1), cell_index(4,2)));

    /* ---------- 暗棋：地雷和军旗只有工兵能吃 ---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 4, 1, 1, SIDE_BLUE, 1);          /* 蓝司令 */
        put(&a, 4, 2, RANK_MINE, SIDE_RED, 1);   /* 红地雷（已翻） */
        chk("[暗棋] 非工兵不能吃地雷",
            !jq_can_move(&a, cell_index(4,1), cell_index(4,2)));
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 4, 1, RANK_WORKER, SIDE_BLUE, 1);
        put(&a, 4, 2, RANK_MINE, SIDE_RED, 1);
        chk("[暗棋] 工兵能吃地雷",
            jq_can_move(&a, cell_index(4,1), cell_index(4,2)));
    }
    /* 工兵挖满 3 雷前不能扛旗 */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 10, 1, RANK_WORKER, SIDE_BLUE, 1);
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        a.mines_cleared[SIDE_BLUE] = 2;
        chk("[暗棋] 工兵未挖满 3 雷不能扛旗",
            !jq_can_move(&a, cell_index(10,1), cell_index(11,1)));
        a.mines_cleared[SIDE_BLUE] = 3;
        chk("[暗棋] 工兵挖满 3 雷可扛旗",
            jq_can_move(&a, cell_index(10,1), cell_index(11,1)));
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 10, 1, 1, SIDE_BLUE, 1);         /* 司令 */
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        a.mines_cleared[SIDE_BLUE] = 3;
        chk("[暗棋] 非工兵不能扛军旗",
            !jq_can_move(&a, cell_index(10,1), cell_index(11,1)));
    }

    /* ---------- 暗棋：未翻子可翻（含地雷）→ 有棋可走不困毙 ---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 4, 1, RANK_MINE, SIDE_BLUE, 0);   /* 蓝仅剩未翻地雷 */
        put(&a, 0, 1, RANK_FLAG, SIDE_BLUE, 1);
        put(&a, 11, 1, RANK_FLAG, SIDE_RED, 1);
        put(&a, 6, 1, 3, SIDE_RED, 1);
        a.turn = SIDE_BLUE;
        chk("[暗棋] 未翻地雷可翻（jq_can_flip：翻雷是合法翻子动作）",
            jq_can_flip(&a, cell_index(4,1)));
        chk("[暗棋] 仅剩未翻地雷 → 困毙（原版 0x30320 可翻扫描排地雷，翻雷不算可行动）",
            jq_side_stalemated(&a, SIDE_BLUE));
    }

    /* ---------- 暗棋：大本营安全岛（ruleflip：仅雷/旗可吃）---------- */
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 0, 1, 5, SIDE_BLUE, 1);              /* 蓝普通子在蓝大本营 (0,1) */
        put(&a, 1, 1, 3, SIDE_RED, 1);               /* 红旅长相邻 */
        a.turn = SIDE_RED;
        chk("[暗棋] 大本营安全岛：敌方不能吃大本营中的普通子",
            !jq_can_move(&a, cell_index(1,1), cell_index(0,1)));
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 0, 1, RANK_MINE, SIDE_BLUE, 1);      /* 蓝地雷在蓝大本营 */
        put(&a, 1, 1, RANK_WORKER, SIDE_RED, 1);     /* 红工兵相邻 */
        a.turn = SIDE_RED;
        chk("[暗棋] 大本营安全岛例外：工兵可吃大本营中的地雷",
            jq_can_move(&a, cell_index(1,1), cell_index(0,1)));
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_FLIP; clr(&a);
        put(&a, 0, 1, RANK_FLAG, SIDE_BLUE, 1);      /* 蓝军旗在蓝大本营 */
        put(&a, 1, 1, RANK_WORKER, SIDE_RED, 1);
        a.turn = SIDE_RED;
        chk("[暗棋] 大本营安全岛例外：工兵挖满 3 雷可扛大本营军旗",
            !jq_can_move(&a, cell_index(1,1), cell_index(0,1)));
        a.mines_cleared[SIDE_RED] = 3;
        chk("[暗棋] 大本营安全岛例外：挖满 3 雷后工兵可扛大本营军旗",
            jq_can_move(&a, cell_index(1,1), cell_index(0,1)));
    }
    {
        jq_state a; jq_state_init(&a); a.mode = JQ_MODE_LAY; clr(&a);
        put(&a, 0, 1, 5, SIDE_BLUE, 1);              /* 明棋：大本营非安全岛 */
        put(&a, 1, 1, 3, SIDE_RED, 1);
        a.turn = SIDE_RED;
        chk("[明棋] 大本营非安全岛（rulelay 未禁，军旗须任何子可扛）",
            jq_can_move(&a, cell_index(1,1), cell_index(0,1)));
    }

    printf("\n=== 合计 PASS=%d FAIL=%d ===\n", pass, fail);
    return fail ? 1 : 0;
}
