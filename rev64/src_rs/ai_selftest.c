/*
 * ai_selftest.c — AI 贪心评分单测（对抗审计 #2 盲区覆盖）
 *
 * 背景：设备端 ai_pick（jni_bridge.c）曾用原始编码 rank 比较（红方 rank+0xc 参与）
 * → 红方 AI 永不识别吃子/同级/炸弹/挖雷（sc=700-1000 全死，只空移）。
 * 修复 = ai_score 提取（rank_norm 内建），本单测直接验 ai_score 评分正确性。
 *
 * 关键：红方棋子存储 rank+0xc（layout.c:59），本测试用红方原始值（+12）喂入，
 *       验证 rank_norm 还原后评分正确——这正是原 bug 的输入空间。
 */
#include <stdio.h>
#include <string.h>
#include "junqi.h"

static int fails = 0;
#define CHECK(cond, msg) do { \
    if (cond) printf("  ✓ %s\n", msg); \
    else { printf("  ✗ FAIL: %s\n", msg); fails++; } \
} while (0)

int main(void)
{
    printf("=== AI 贪心评分单测（红方 rank+0xc 输入空间）===\n");

    /* 红方存储值 = 实际 rank + 0xc（rank_norm 还原） */
    int red3  = 3  + 0xc;   /* 红 rank3   → 15 */
    int red6  = 6  + 0xc;   /* 红 rank6   → 18 */
    int red2  = 2  + 0xc;   /* 红 rank2   → 14 */
    int redB  = 10 + 0xc;   /* 红炸弹    → 22 */
    int redE  = 9  + 0xc;   /* 红工兵    → 21 */
    int blue8 = 8;          /* 蓝 rank8  → 8  */
    int blue6 = 6;          /* 蓝 rank6  → 6  */
    int blue1 = 1;          /* 蓝 rank1  → 1  */
    int mine  = 11;         /* 地雷      → 11 */
    int empty = 0;          /* 空格      → 0  */

    /* 1. 吃子：红 rank3 吃 蓝 rank8（3<8 大压小）→ 1000 */
    CHECK(ai_score(red3, blue8) == 1000, "红 rank3 吃蓝 rank8 = 1000（吃子）");

    /* 2. 同级互杀：红 rank6 vs 蓝 rank6 → 900 */
    CHECK(ai_score(red6, blue6) == 900, "红 rank6 同级蓝 rank6 = 900（同级）");

    /* 3. 炸弹：红炸弹（存储22）撞任意 → 800 */
    CHECK(ai_score(redB, blue1) == 800, "红炸弹 = 800（炸弹）");

    /* 4. 工兵挖雷：红工兵（存储21）挖地雷 → 700 */
    CHECK(ai_score(redE, mine) == 700, "红工兵挖雷 = 700（工兵挖雷）");

    /* 5. 送死：红 rank2 撞蓝 rank1（2>1 吃不掉）→ 1（最低分，不主动送死） */
    CHECK(ai_score(red2, blue1) == 1, "红 rank2 撞蓝 rank1 = 1（送死最低分）");

    /* 5b. ★ 隐藏 bug 修复验证：普通子碰地雷 = 送死（非"能吃"1000），仅工兵可挖 */
    CHECK(ai_score(red2, mine) == 1, "红 rank2 碰地雷 = 1（送死，非工兵不能挖）");
    CHECK(ai_score(red6, mine) == 1, "红 rank6 碰地雷 = 1（送死，非工兵不能挖）");

    /* 6. 空移：目标空格 → 2（优先于送死） */
    CHECK(ai_score(red3, empty) == 2, "红 rank3 空格 = 2（空移）");

    /* 7. 旗/雷不可走：src 为军旗/地雷 → -1 */
    CHECK(ai_score(12, empty) == -1, "军旗不可走 = -1");
    CHECK(ai_score(11, empty) == -1, "地雷不可走 = -1");

    /* 8. 蓝方视角（无 +0xc）：蓝 rank3 吃 红 rank8（存储 8+12=20）→ 1000 */
    CHECK(ai_score(3, 20) == 1000, "蓝 rank3 吃红 rank8（存储20）= 1000（吃子）");

    printf("=== AI 评分单测: %s（%d FAIL）===\n", fails ? "FAIL" : "全部通过", fails);
    return fails ? 1 : 0;
}
