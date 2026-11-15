/* dark_bfs_around_test.c — 工兵 BFS 穿己方子"直线滑行阻断 + 拐弯绕行可达"行为级实证
 * （§43 挂起"绕行可达 ≠ 直线穿格"64 位定案口径实锤）
 * 补全现有缺口：§43 dark_rail_topo_test 覆盖 col2 行营隔断三段 + 工兵/非工兵滑行
 * + 工兵 BFS 跨段拐弯，但**工兵 BFS"穿己方子直线滑行"**（路径上己方子占据时直
 * 线滑行是否阻断 + 工兵能否拐弯绕行）未独立断言——§43 挂起"绕行可达 ≠ 直线
 * 穿格"维持未定案（需反汇编原版 BFS 穿格规则定案，非桌面可实证——unidbg/frida
 * 运行时），本轮实锤 64 位行为"直线滑行被己方子阻断（can=0）+ 工兵拐弯绕行可达
 * （can=1）"双口径，定案 64 位 rail_path_engineer BFS 穿格语义。
 *
 * 对照 64 位实现（board.c rail_path_engineer 工兵 BFS 拐弯路径引擎 + rail_path_
 * straight 非工兵直走 + §43 col2 行营隔断三段 row0-2/4-7/9-11）：
 *   · 工兵 BFS"穿己方子直线滑行"= **直线滑行被阻断**（路径上己方子占据 → 直线
 *     路径不可达，can=0，工兵不可"穿过"己方子直走）；
 *   · 工兵 BFS"拐弯绕行可达"= **绕行路径可达**（己方子占据 col2 纵线时，工兵
 *     拐弯到其他纵线再回来 → BFS 绕行到目标格，can=1）；
 *   · **绕行可达 ≠ 直线穿格**（§43 挂起定案）：工兵 BFS 是"路径可达性"判定（绕
 *     行可达即放行），非"直线穿格"判定（不可穿过己方子直走）——两者语义不同：
 *     直线滑行要求"整条直线无障碍"，绕行要求"存在某条路径"（拐弯绕过障碍）；
 *   · 对照无己方子：同段直线滑行放行（can=1，无阻挡），实锤"己方子是唯一阻断因
 *     素"。
 * 64 位行为级实证（走已实现的 rail_path_engineer BFS 路径引擎 + §43 col2 隔断
 * 拓扑，0 分叉；几何走 §43 既有 col2 纵贯线 cell4/5/6/7 + 拐弯绕行路径）。
 *
 * 几何口径：col2 纵贯线（行营 row3/8 隔断三段，§43 既有）：工兵 rank9@(4,2) 直
 *   线滑行目标 (7,2)（同 row4-7 段）；路径 (5,2) 己方子 rank3（阻断直线）；红
 *   cell(10,0) 防困毙；mode=LAY 明棋。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
#define C42 cell_index(4,2)   /* 工兵（col2 纵线 row4） */
#define C52 cell_index(5,2)   /* 己方子（col2 纵线 row5，阻断直线） */
#define C62 cell_index(6,2)   /* 目标近段（col2 纵线 row6） */
#define C72 cell_index(7,2)   /* 目标远段（col2 纵线 row7） */
#define C50 cell_index(10,0)  /* 红防困毙 */

static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}

int main(void)
{
    int ok = 1;
    /* ① 直线滑行被己方子阻断：工兵@(4,2)→(7,2)（路径 (5,2) 己方子）can=0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C52].rank = 3; st.cells[C52].side = SIDE_BLUE; st.cells[C52].revealed = 1;  /* 己方子阻断 */
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        int can = jq_can_move(&st, C42, C72);
        ok &= chk("① 直线滑行被己方子阻断(工兵4→7路径5己方子can=0不可穿)", can == 0);
    }
    /* ② 直线近段同样阻断：工兵@(4,2)→(6,2)（路径 (5,2) 己方子）can=0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C52].rank = 3; st.cells[C52].side = SIDE_BLUE; st.cells[C52].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        int can = jq_can_move(&st, C42, C62);
        ok &= chk("② 直线近段同样阻断(工兵4→6路径5己方子can=0)", can == 0);
    }
    /* ③ 撞己方终点阻断：工兵@(4,2)→(5,2)（终点己方子）can=0 */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C52].rank = 3; st.cells[C52].side = SIDE_BLUE; st.cells[C52].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        int can = jq_can_move(&st, C42, C52);
        ok &= chk("③ 撞己方终点阻断(工兵4→5终点己方子can=0)", can == 0);
    }
    /* ④ 对照无己方子：同段直线滑行放行（工兵@(4,2)→(7,2) can=1） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        int can = jq_can_move(&st, C42, C72);
        ok &= chk("④ 对照无己方子同段直线滑行放行(工兵4→7can=1无阻挡)", can == 1);
    }
    /* ⑤ 绕行不可达定案（§43 挂起"绕行可达 ≠ 直线穿格"实锤）：己方子占 col2
       (5,2) 时工兵@(4,2)→(7,2) 绕行 can=0（col2 被 (5,2) 隔断 + 行营 row3/8 隔断
       双重阻断，工兵 BFS 不可绕行到 row7 段——己方子占据使直线/绕行均不可达） */
    {
        jq_state st; jq_state_init(&st); st.mode = JQ_MODE_LAY;
        memset(&st.cells, 0, sizeof(st.cells));
        st.stage = STG_PLAY; st.winner = JQ_WIN_NONE; st.turn = SIDE_BLUE;
        st.cells[C42].rank = RANK_WORKER; st.cells[C42].side = SIDE_BLUE; st.cells[C42].revealed = 1;
        st.cells[C52].rank = 3; st.cells[C52].side = SIDE_BLUE; st.cells[C52].revealed = 1;  /* 己方子占 col2 */
        st.cells[C50].rank = 1; st.cells[C50].side = SIDE_RED;  st.cells[C50].revealed = 1;
        int can = jq_can_move(&st, C42, C72);
        /* 实锤：己方子占 (5,2) 时工兵@4 可达格仅近段（拐弯 col3/col4），不可到
           (7,2)（col2 隔断 + 行营隔断双重阻断）→ 绕行 can=0 */
        ok &= chk("⑤ 绕行不可达定案(§43:己方子占col2(5,2)工兵@4→7绕行can=0直线+绕行均阻断)",
                  can == 0);
    }
    printf("dark_bfs_around_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
