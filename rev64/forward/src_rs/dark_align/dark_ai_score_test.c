/* dark_ai_score_test.c — AI 贪心评分（ai_score）缺口分值行为级实证
 * 补全 ai_selftest.c 的诚实标注缺口（§1144/§1148 记录"无炸弹撞雷/炸弹撞旗
 * 用例"）：
 *   · B01-030（P1 已修）：炸弹撞雷 = 800（旧评 1 导致 AI 避免炸弹撞雷，修后
 *     800 对齐 EV_BOMB 同归语义）；
 *   · B01-032（P1 标注）：炸弹撞旗（r=10<dr=12）命中 1000（'吃子最高分'，
 *     语义'同归胜局'非'净吃'——原版 0x55d40 单步贪心无价值维度，保持 1000）；
 *   · AUD-R-060：同级炸弹（炸弹撞炸弹）= 900（同级分，覆盖"两炸弹相撞"与
 *     "同级普通子互杀"双语义，事件侧 EV_BOMB 同码同源）。
 * 对照 64 位 ai_score 实现（rules.c 纯函数，3 参数 src_rank_raw/dst_rank_raw/
 * dst_revealed，无状态，桌面可独立实证）+ ai_selftest 既有主链分值
 * （吃子 1000 / 同级 900 / 炸弹 800 / 挖雷 700 / 空移 2 / 送死 1 / -1 / -2）。
 * 64 位行为级实证（走已实现的 ai_score 各分支，0 分叉）。
 *
 * rank 编码口径：蓝方 raw=rank(1-12)，红方 raw=rank+0xc；RANK_BOMB=10
 * RANK_MINE=11 RANK_FLAG=12 RANK_WORKER=9（junqi.h）。
 */
#include <stdio.h>
#include "junqi.h"
extern int ai_score(int src_raw, int dst_raw, int dst_revealed);

static int chk(char *name, int got, int want)
{
    int ok = (got == want);
    printf("%s: got=%d want=%d → %s\n", name, got, want, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    int ok = 1;
    int blueB = 10, redB = 10 + 0xc;          /* 炸弹（蓝 raw / 红 raw+0xc） */
    int blueM = 11, redM = 11 + 0xc;          /* 地雷 */
    int blueF = 12, redF = 12 + 0xc;          /* 军旗 */
    int blueW = 9;                             /* 工兵 */
    int blue6 = 6, red8 = 8 + 0xc;            /* 普通子 */

    /* B01-030 炸弹撞雷 = 800（蓝/红视角对照） */
    ok &= chk("B01-030 蓝炸弹撞雷=800", ai_score(blueB, blueM, 1), 800);
    ok &= chk("B01-030 红炸弹撞雷=800", ai_score(redB, redM, 1), 800);
    /* B01-032 炸弹撞旗 = 1000（吃子最高分，非 800） */
    ok &= chk("B01-032 蓝炸弹撞旗=1000", ai_score(blueB, blueF, 1), 1000);
    ok &= chk("B01-032 红炸弹撞旗=1000", ai_score(redB, redF, 1), 1000);
    /* AUD-R-060 同级炸弹（炸弹撞炸弹）= 900（同级分） */
    ok &= chk("AUD-R-060 蓝炸弹撞红炸弹同级=900", ai_score(blueB, redB, 1), 900);
    /* 既有主链分值回归（对照 ai_selftest，防 0 退化） */
    ok &= chk("工兵挖雷=700", ai_score(blueW, blueM, 1), 700);
    ok &= chk("普通子撞雷送死=1", ai_score(blue6, blueM, 1), 1);
    ok &= chk("吃子(蓝3吃红8)=1000", ai_score(3, red8, 1), 1000);
    ok &= chk("暗棋未翻不透视=2", ai_score(blue6, redB, 0), 2);
    ok &= chk("旗不可走=-1", ai_score(blueF, 0, 1), -1);
    ok &= chk("空源哨兵=-2", ai_score(0, blue6, 1), -2);
    printf("dark_ai_score_test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
