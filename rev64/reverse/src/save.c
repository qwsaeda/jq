/*
 * save.c — 存档 one.sjq 读写（R6 手写路线，L74 对齐原版）
 *
 * 原版 one.sjq（0x37664 读 / 0x379ec 写）字段（0x38=time/0x3c=difficulty/
 * 0x40=level/0x44=mark/0x48=resultEx[4]），resultEx 用 "%d;%d;%d;%d"（0x121774）。
 *
 * 诚实边界：字段语义/顺序/resultEx 格式确证（L74）；键值文本格式为 arm64 重建
 * （原版写逻辑 0x379ec 是 C++ 层长链，未逐字节确证分隔符/顺序）。
 */
#include "junqi_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 键名（对齐原版 rodata：time 0x121744 / difficulty 0x12174c / mark 0x121760 /
   resultEx 0x121768；level 亦在字段表内，0x40）。 */
static const char K_TIME[]       = "time";
static const char K_DIFFICULTY[] = "difficulty";
static const char K_LEVEL[]      = "level";
static const char K_MARK[]       = "mark";
static const char K_RESULTEX[]   = "resultEx";

/* 写 one.sjq：key=value\n 文本（resultEx 用 "%d;%d;%d;%d"）。 */
int jq_save_write(const char *path, const jq_save_data *sd) {
    if (!path || !sd) return 0;
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "%s=%u\n", K_TIME, sd->time);
    fprintf(f, "%s=%d\n", K_DIFFICULTY, sd->difficulty);
    fprintf(f, "%s=%d\n", K_LEVEL, sd->level);
    fprintf(f, "%s=%d\n", K_MARK, sd->mark);
    fprintf(f, "%s=%d;%d;%d;%d\n", K_RESULTEX,
            sd->resultEx[0], sd->resultEx[1], sd->resultEx[2], sd->resultEx[3]);
    fclose(f);
    return 1;
}

/* 读 one.sjq：逐行 key=value 解析（字段缺失保持默认）。 */
int jq_save_read(const char *path, jq_save_data *sd) {
    if (!path || !sd) return 0;
    memset(sd, 0, sizeof(*sd));
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* 去换行 */
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line, *val = eq + 1;
        if      (strcmp(key, K_TIME) == 0)       sd->time = (uint32_t)atoi(val);
        else if (strcmp(key, K_DIFFICULTY) == 0) sd->difficulty = atoi(val);
        else if (strcmp(key, K_LEVEL) == 0)      sd->level = atoi(val);
        else if (strcmp(key, K_MARK) == 0)       sd->mark = atoi(val);
        else if (strcmp(key, K_RESULTEX) == 0)
            sscanf(val, "%d;%d;%d;%d",
                   &sd->resultEx[0], &sd->resultEx[1], &sd->resultEx[2], &sd->resultEx[3]);
    }
    fclose(f);
    return 1;
}

/* ================= 对局进度存档 one.sjg（L89 原版 0x390ac 写确证） =================
   arm64 文本格式（诚实边界：保存完整棋盘状态供"退出重进续局"）。
   格式：
     board=r0s0v0,r1s1v1,...,r59s59v59   (每格 rank,side,revealed)
     turn=N / stage=N / winner=N
     flags=f0,f1 / mines=m0,m1 / pieces=p0,p1
     moves=N / moves_since_eat=N */

int jq_save_game_write(const char *path, const jq_state *st) {
    if (!path || !st) return 0;
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "board=");
    for (int i = 0; i < JQ_CELLS; i++) {
        fprintf(f, "%d,%d,%d%c", st->board[i].rank, st->board[i].side,
                st->board[i].revealed, (i + 1 < JQ_CELLS) ? ',' : '\n');
    }
    fprintf(f, "turn=%d\n", st->turn);
    fprintf(f, "stage=%d\n", st->stage);
    fprintf(f, "winner=%d\n", st->winner);
    fprintf(f, "flags=%d,%d\n", st->flags_captured[0], st->flags_captured[1]);
    fprintf(f, "mines=%d,%d\n", st->mines_cleared[0], st->mines_cleared[1]);
    fprintf(f, "pieces=%d,%d\n", st->pieces_left[0], st->pieces_left[1]);
    fprintf(f, "moves=%u\n", st->moves);
    fprintf(f, "moves_since_eat=%u\n", st->moves_since_eat);
    fclose(f);
    return 1;
}

int jq_save_game_read(const char *path, jq_state *st) {
    if (!path || !st) return 0;
    memset(st, 0, sizeof(*st));
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line, *val = eq + 1;
        if (strcmp(key, "board") == 0) {
            int i = 0;
            const char *p = val;
            while (i < JQ_CELLS && *p) {
                unsigned rank, side, rev;
                if (sscanf(p, "%u,%u,%u", &rank, &side, &rev) == 3) {
                    st->board[i].rank = (uint8_t)rank;
                    st->board[i].side = (uint8_t)side;
                    st->board[i].revealed = (uint8_t)rev;
                    st->board[i].is_flag  = (rank == JQ_RANK_FLAG);
                    st->board[i].is_mine  = (rank == JQ_RANK_MINE);
                    st->board[i].is_bomb  = (rank == JQ_RANK_BOMB);
                }
                i++;
                const char *c = strchr(p, ',');
                if (!c) break;
                p = c + 1;
                /* 跳过后两字段 */
                for (int k = 0; k < 2; k++) { c = strchr(p, ','); if (!c) { p = ""; break; } p = c + 1; }
            }
        }
        else if (strcmp(key, "turn") == 0)   st->turn = (uint8_t)atoi(val);
        else if (strcmp(key, "stage") == 0)  st->stage = (jq_stage)atoi(val);
        else if (strcmp(key, "winner") == 0) st->winner = (uint8_t)atoi(val);
        else if (strcmp(key, "flags") == 0)
            sscanf(val, "%hhu,%hhu", &st->flags_captured[0], &st->flags_captured[1]);
        else if (strcmp(key, "mines") == 0)
            sscanf(val, "%hhu,%hhu", &st->mines_cleared[0], &st->mines_cleared[1]);
        else if (strcmp(key, "pieces") == 0)
            sscanf(val, "%hhu,%hhu", &st->pieces_left[0], &st->pieces_left[1]);
        else if (strcmp(key, "moves") == 0)  st->moves = (uint32_t)strtoul(val, NULL, 10);
        else if (strcmp(key, "moves_since_eat") == 0)
            st->moves_since_eat = (uint32_t)strtoul(val, NULL, 10);
    }
    fclose(f);
    return 1;
}
