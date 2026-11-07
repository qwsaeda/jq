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
    uint32_t turn = st->turn, stage = st->stage, winner = st->winner;
    uint32_t since_eat = st->moves_since_eat, total = st->total_moves;
    uint32_t mc[2] = {st->mines_cleared[0], st->mines_cleared[1]};
    uint32_t pl[2] = {st->pieces_left[0], st->pieces_left[1]};
    fwrite(&m, 4, 1, f);
    fwrite(&turn, 4, 1, f);
    fwrite(&stage, 4, 1, f);
    fwrite(&winner, 4, 1, f);
    fwrite(&since_eat, 4, 1, f);
    fwrite(&total, 4, 1, f);
    fwrite(mc, 4, 2, f);
    fwrite(pl, 4, 2, f);
    { uint32_t mode = st->mode; fwrite(&mode, 4, 1, f); }   /* ★ AUD-R-002/003（P0） */
    for (int i = 0; i < N_CELLS; i++)
        fwrite(&st->cells[i], sizeof(jq_cell), 1, f);
    fclose(f);
    return 1;
}

int jq_save_game_read(const char *path, jq_state *st)
{
    if (!path || !st) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint32_t m = 0;
    fread(&m, 4, 1, f);
    if (m != SJG_MAGIC) { fclose(f); return 0; }
    uint32_t turn, stage, winner, since_eat, total;
    uint32_t mc[2], pl[2];
    fread(&turn, 4, 1, f);
    fread(&stage, 4, 1, f);
    fread(&winner, 4, 1, f);
    fread(&since_eat, 4, 1, f);
    fread(&total, 4, 1, f);
    fread(mc, 4, 2, f);
    fread(pl, 4, 2, f);
    { uint32_t mode; if (fread(&mode, 4, 1, f) == 1) st->mode = (uint8_t)mode; }   /* ★ AUD-R-002/003（P0） */
    st->turn = turn; st->stage = stage; st->winner = winner;
    st->moves_since_eat = since_eat; st->total_moves = total;
    st->mines_cleared[0] = mc[0]; st->mines_cleared[1] = mc[1];
    st->pieces_left[0] = pl[0]; st->pieces_left[1] = pl[1];
    for (int i = 0; i < N_CELLS; i++)
        fread(&st->cells[i], sizeof(jq_cell), 1, f);
    fclose(f);
    return 1;
}
