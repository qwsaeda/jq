/*
 * save.c — 存档（逆源码重写）
 * 依据原版（L74/L83/L89 确证）：
 *   one.sjq（0x37664 读 / 0x379ec 写）：进度存档，字段
 *     +0x38=time / +0x3c=difficulty / +0x40=level / +0x44=mark / +0x48=resultEx[4]，
 *     resultEx 序列化 "%d;%d;%d;%d"（rodata 0x121774）。键名 rodata 0x121744(time)/
 *     0x12174c(difficulty)/0x121760(mark)/0x121768(resultEx)。
 *   one.sjg（0x390ac 写，L89）：对局进度二进制存档（board 60 格 + 对局状态），
 *     OnCreate 续局 / OnPause 保存 / 对局结束清理。
 * 诚实边界：one.sjq 键值文本格式（key=value\n 分隔/顺序）为行为级重建
 *   （原版 0x379ec 是 C++ 层长链，未逐字节确证分隔符/顺序）；
 *   one.sjg 本重写用紧凑二进制（字段顺序对齐原版 0x390ac stm 语义，非逐字节）。
 */
#include "junqi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ★ AUD-R-049/041（P2）：jq_state 删掉 undo_depth / count_tbl 死字段后结构体变小。
   one.sjg 是自有二进制格式（无外部兼容旧档需求，"非逐字节"自述），
   写读两侧同步用 sizeof(jq_state) 序列化——写旧读新/写新读旧互读时
   读侧 fread 按新 sizeof 读，字段顺序不变则逐字段可解析；诚实边界：
   混合版本互读未回归测试（设备存档每次重装 so 后覆盖写，实际不会发生）。
   ★ AUD-R-128（P3 诚实标注）：jq_cell 4 字节紧凑布局（无 padding）与 save.c
   裸字节序列化（sizeof(jq_state) 直接 fwrite/fread）耦合——无版本号/无
   magic 校验，jq_state 字段顺序或 jq_cell 大小一旦变化，读旧档即错位。
   现有 SJG_MAGIC 仅校验文件头（1 个 uint32），未校验 sizeof(jq_state)。
   修法（存档加 sizeof(jq_state) 字段到 magic 区，读侧校验 sizeof 一致，
   不一致则返回 0）留待下一轮。 */
#define SJG_STATE_BYTES sizeof(jq_state)



int jq_save_write(const char *path, const jq_save_data *sd)
{
    if (!path || !sd) return 0;
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "time=%u\n", sd->time);
    fprintf(f, "difficulty=%d\n", sd->difficulty);
    fprintf(f, "level=%d\n", sd->level);
    fprintf(f, "mark=%d\n", sd->mark);
    fprintf(f, "resultEx=%d;%d;%d;%d\n",
            sd->resultEx[0], sd->resultEx[1], sd->resultEx[2], sd->resultEx[3]);
    fclose(f);
    return 1;
}

int jq_save_read(const char *path, jq_save_data *sd)
{
    if (!path || !sd) return 0;
    memset(sd, 0, sizeof(*sd));
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        int a, b, c, d;
        uint32_t t;
        if (sscanf(line, "time=%u", &t) == 1) sd->time = t;
        else if (sscanf(line, "difficulty=%d", &a) == 1) sd->difficulty = a;
        else if (sscanf(line, "level=%d", &a) == 1) sd->level = a;
        else if (sscanf(line, "mark=%d", &a) == 1) sd->mark = a;
        else if (sscanf(line, "resultEx=%d;%d;%d;%d", &a, &b, &c, &d) == 4)
            sd->resultEx[0]=a; sd->resultEx[1]=b; sd->resultEx[2]=c; sd->resultEx[3]=d;
    }
    fclose(f);
    return 1;
}

/* ---- one.sjg 对局进度（二进制）：board 60 格 + 对局状态 ---- */
#define SJG_MAGIC 0x534a4731  /* "SJG1" */
int jq_save_game_write(const char *path, const jq_state *st)
{
    if (!path || !st) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    uint32_t m = SJG_MAGIC;
    fwrite(&m, 4, 1, f);
    fwrite(st, 1, SJG_STATE_BYTES, f);   /* ★ AUD-R-049/041（P2）：整结构序列化 */
    fclose(f);
    return 1;
}

int jq_save_game_read(const char *path, jq_state *st)
{
    if (!path || !st) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint32_t m = 0;
    if (fread(&m, 4, 1, f) != 1 || m != SJG_MAGIC) {
        fclose(f);
        return 0;
    }
    /* ★ AUD-R-049/041（P2）：整结构读取（与写侧同版 sizeof） */
    if (fread(st, 1, SJG_STATE_BYTES, f) != SJG_STATE_BYTES) { fclose(f); return 0; }
    fclose(f);
    /* AUD-1000 B01-037：读档后 pieces_left 可为负（旧版游戏存档序列化了
       增量递减未钳制的脏值），下界钳 0 防止负值进规则引擎/AI 评分。 */
    for (int i = 0; i < 2; i++)
        if (st->pieces_left[i] < 0) st->pieces_left[i] = 0;
    for (int i = 0; i < 2; i++)
        if (st->mines_cleared[i] < 0) st->mines_cleared[i] = 0;
    return 1;
}
