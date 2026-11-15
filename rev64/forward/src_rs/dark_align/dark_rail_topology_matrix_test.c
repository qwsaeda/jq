/* dark_rail_topology_matrix_test.c — is_rail 全格分布"横贯线全宽 vs 纵贯线非
 * 全高"完整拓扑矩阵行为级实证 + 横贯线全宽口径纠正
 * 补全现有缺口：§91 dark_rail_r0r11_ends_test（实锤 row0/row11 横贯线两端
 * col0/col4 is_rail=1 横贯线两端铁路属性，纠正 §80/§85/§88 "两端非铁路"误判），
 * 但**横贯线全宽 vs 纵贯线非全高完整拓扑矩阵**（is_rail 全格分布 row0-11×col0-
 * 4 完整拓扑 + 横贯线全宽仅 row5/6 10 格全宽 + row0/11 非全宽仅 3 格铁路
 * col0/2/4 + 纵贯线全高仅 col2 10 格全高 + col0/1/3/4 非全高 4/2/2/4 格 +
 * row3/row8 全非铁路行营隔断）无独立用例——§91 既有口径"row0/11 横贯线全宽
 * 10 格"系误判（实锤横贯线全宽仅 row5/6，row0/11 非全宽仅 3 格铁路 col0/2/4），
 * 完整拓扑矩阵"横贯线全宽 ≠ 纵贯线全高"口径未独立断言。
 *
 * 对照 64 位实现（board.c is_rail 十字铁路拓扑（§L68 + §83/§84/§85/§87/§88/§8
 * 9/§90/§91 全格分布实锤：col2 纵贯 10 格 + row5/6 横贯 10 格全宽 + row0/11
 * 横贯非全宽仅 3 格铁路 col0/2/4 + col0=4 格 / col1=2 格 / col3=2 格 / col4=4
 * 格非纵贯全高 + row3/row8 全非铁路行营隔断））：
 *   · 横贯线全宽仅 row5/6（col0-4 全 10 格 is_rail=1）；
 *   · row0/11 横贯线非全宽（仅 col0/2/4 三格铁路，col1/3 非铁路——row0: 1 0
 *     1 0 1 / row11: 1 0 1 0 1）；
 *   · 纵贯线全高仅 col2（row0/1/2/4/5/6/7/9/10/11 共 10 格 is_rail=1，row3/8
 *     行营隔断）；
 *   · col0/1/3/4 纵贯线非全高（col0=4 格 row0/5/6/11 / col1=2 格 row5/6 / col3
 *     =2 格 row5/6 / col4=4 格 row0/5/6/11）；
 *   · row3/row8 全非铁路（行营隔断，is_rail 全 0）；
 *   · 完整拓扑矩阵 is_rail 全格分布（row0-11×col0-4 12×5=60 格逐格实锤）：
 *     row0:  1 0 1 0 1  (col0/2/4 铁路)
 *     row1:  0 0 1 0 0  (仅 col2)
 *     row2:  0 0 1 0 0  (仅 col2)
 *     row3:  0 0 0 0 0  (全非铁路, 行营隔断)
 *     row4:  0 0 1 0 0  (仅 col2)
 *     row5:  1 1 1 1 1  (全宽 10 格)
 *     row6:  1 1 1 1 1  (全宽 10 格)
 *     row7:  0 0 1 0 0  (仅 col2)
 *     row8:  0 0 0 0 0  (全非铁路, 行营隔断)
 *     row9:  0 0 1 0 0  (仅 col2)
 *     row10: 0 0 1 0 0  (仅 col2)
 *     row11: 1 0 1 0 1  (col0/2/4 铁路)
 * 64 位行为级实证（走已实现的 is_rail 十字铁路拓扑，0 分叉；几何走 §83/§84/
 * §85/§87/§88/§89/§90/§91 既有 is_rail 全格分布口径 + 横贯线全宽仅 row5/6 +
 * 纵贯线全高仅 col2 + row3/8 行营隔断完整拓扑矩阵纠正）。
 *
 * 几何口径：横贯线全宽仅 row5/6（col0-4 全 10 格）；row0/11 横贯非全宽（仅
 *   col0/2/4 三格铁路）；纵贯线全高仅 col2（row0/1/2/4/5/6/7/9/10/11 共 10
 *   格，row3/8 行营隔断）；col0/1/3/4 纵贯非全高（4/2/2/4 格）；is_rail 全格
 *   分布 60 格逐格实锤。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"
static int chk(char *name, int cond)
{
    printf("%s: %s\n", name, cond ? "PASS" : "FAIL");
    return cond;
}
int main(void)
{
    int ok = 1;
    /* ① 横贯线全宽仅 row5/6（col0-4 全 10 格 is_rail=1），row0/11 非全宽 */
    {
        int r5_full = 1, r6_full = 1, r0_full = 0, r11_full = 0;
        for (int c = 0; c < 5; c++) {
            if (!is_rail(5, c)) r5_full = 0;
            if (!is_rail(6, c)) r6_full = 0;
        }
        for (int c = 0; c < 5; c++) {
            if (is_rail(0, c)) r0_full++;
            if (is_rail(11, c)) r11_full++;
        }
        ok &= chk("① 横贯线全宽仅row5/6(row5/6col0-4全10格is_rail=1),row0/11非全宽(row0/11仅col0/2/4=3格铁路)",
                  r5_full && r6_full && r0_full == 3 && r11_full == 3);
    }
    /* ② 纵贯线全高仅 col2（row0/1/2/4/5/6/7/9/10/11 共 10 格 is_rail=1，row3/8
       行营隔断） */
    {
        int c2 = 0;
        for (int r = 0; r < 12; r++)
            if (is_rail(r, 2)) c2++;
        /* col2 纵贯全高 10 格（row3/8 行营隔断非铁路） */
        int c2_row3_0 = !is_rail(3, 2), c2_row8_0 = !is_rail(8, 2);
        ok &= chk("② 纵贯线全高仅col2(col2=row0/1/2/4/5/6/7/9/10/11共10格is_rail=1,row3/8行营隔断非铁路)",
                  c2 == 10 && c2_row3_0 && c2_row8_0);
    }
    /* ③ col0/1/3/4 纵贯线非全高（col0=4 格 row0/5/6/11 / col1=2 格 row5/6 / col3
       =2 格 row5/6 / col4=4 格 row0/5/6/11） */
    {
        int c0 = 0, c1 = 0, c3 = 0, c4 = 0;
        for (int r = 0; r < 12; r++) {
            if (is_rail(r, 0)) c0++;
            if (is_rail(r, 1)) c1++;
            if (is_rail(r, 3)) c3++;
            if (is_rail(r, 4)) c4++;
        }
        ok &= chk("③ col0/1/3/4纵贯线非全高(col0=4格row0/5/6/11/col1=2格row5/6/col3=2格row5/6/col4=4格row0/5/6/11)",
                  c0 == 4 && c1 == 2 && c3 == 2 && c4 == 4);
    }
    /* ④ row3/row8 全非铁路（行营隔断，is_rail 全 0） */
    {
        int r3 = 0, r8 = 0;
        for (int c = 0; c < 5; c++) {
            if (is_rail(3, c)) r3++;
            if (is_rail(8, c)) r8++;
        }
        ok &= chk("④ row3/row8全非铁路(行营隔断is_rail全0,row3=row8=0格铁路)",
                  r3 == 0 && r8 == 0);
    }
    /* ⑤ is_rail 全格分布矩阵实锤（row0-11×col0-4 60 格逐格，对照既有拓扑） */
    {
        int ok_matrix = 1;
        /* row0: col0/2/4 铁路 (1 0 1 0 1) */
        if (!(is_rail(0,0)&&!is_rail(0,1)&&is_rail(0,2)&&!is_rail(0,3)&&is_rail(0,4))) ok_matrix=0;
        /* row1/2/4/7/9/10: 仅 col2 铁路 (0 0 1 0 0) */
        for (int r = 1; r <= 2; r++)
            for (int c = 0; c < 5; c++)
                if ((c==2)!=is_rail(r,c)) ok_matrix=0;
        for (int r = 4; r <= 4; r++)
            for (int c = 0; c < 5; c++)
                if ((c==2)!=is_rail(r,c)) ok_matrix=0;
        for (int r = 7; r <= 7; r++)
            for (int c = 0; c < 5; c++)
                if ((c==2)!=is_rail(r,c)) ok_matrix=0;
        for (int r = 9; r <= 10; r++)
            for (int c = 0; c < 5; c++)
                if ((c==2)!=is_rail(r,c)) ok_matrix=0;
        /* row5/6: 全宽 10 格 (1 1 1 1 1) */
        for (int r = 5; r <= 6; r++)
            for (int c = 0; c < 5; c++)
                if (!is_rail(r,c)) ok_matrix=0;
        /* row11: col0/2/4 铁路 (1 0 1 0 1) */
        if (!(is_rail(11,0)&&!is_rail(11,1)&&is_rail(11,2)&&!is_rail(11,3)&&is_rail(11,4))) ok_matrix=0;
        ok &= chk("⑤ is_rail全格分布矩阵实锤(row0/11=col0/2/4三格,row1/2/4/7/9/10=仅col2,row5/6=全宽10格,row3/8=全非铁路行营隔断)",
                  ok_matrix);
    }
    /* ⑥ 横贯线全宽 ≠ 纵贯线全高完整拓扑口径实锤（横贯线全宽仅 row5/6 10 格，
       纵贯线全高仅 col2 10 格，col0/1/3/4 非全高 4/2/2/4 格） */
    {
        int r5r6_full = 1;
        for (int c = 0; c < 5; c++) {
            if (!is_rail(5, c) || !is_rail(6, c)) r5r6_full = 0;
        }
        /* col2 纵贯全高 10 格（row3/8 行营隔断非铁路，非 12 格全高） */
        int c2_cnt = 0;
        for (int r = 0; r < 12; r++) if (is_rail(r, 2)) c2_cnt++;
        ok &= chk("⑥ 横贯线全宽≠纵贯线全高完整拓扑(横贯线全宽仅row5/6 10格[row5/6col0-4全宽],纵贯线全高仅col2 10格[row0-11除row3/8],col0/1/3/4非全高4/2/2/4格)",
                  r5r6_full && c2_cnt == 10);
    }
    printf("dark_rail_topology_matrix_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
