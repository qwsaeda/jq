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
    /* ★ B02-074（P2 已修）：原子写——写临时文件 + rename 替换。旧实现直接 fopen(path,"w")
       写到一半崩溃 → 读到残缺 key=value 文件（部分字段有值，部分缺失），无完整性校验。
       临时文件写成功后 rename 原子替换，崩溃时目标文件要么是旧完整版本、要么不存在
       （fopen 临时文件失败），不会留半写残缺文件。 */
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return 0;
    fprintf(f, "time=%u\n", sd->time);
    fprintf(f, "difficulty=%d\n", sd->difficulty);
    fprintf(f, "level=%d\n", sd->level);
    fprintf(f, "mark=%d\n", sd->mark);
    fprintf(f, "resultEx=%d;%d;%d;%d\n",
            sd->resultEx[0], sd->resultEx[1], sd->resultEx[2], sd->resultEx[3]);
    fclose(f);
    if (rename(tmp, path) != 0) {
        remove(tmp);
        return 0;
    }
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
        else if (sscanf(line, "resultEx=%d;%d;%d;%d", &a, &b, &c, &d) == 4) {
            sd->resultEx[0]=a; sd->resultEx[1]=b; sd->resultEx[2]=c; sd->resultEx[3]=d;
        }
    }
    fclose(f);
    return 1;
}

/* ---- one.sjg 对局进度（二进制）：board 60 格 + 对局状态 ---- */
#define SJG_MAGIC 0x534a4731  /* "SJG1" */
/* ★ AUD-1000 B02-071/072/B10-016（P1 已修）：magic 区加 sizeof(jq_state) 双字段
   校验（写侧 fwrite 两值均检返回值，盘满半写 remove 清档；读侧 magic+sizeof
   双校验，不一致返 0 拒绝错位旧档）。one.sjg 为自有二进制格式无外部兼容
   需求（AUD-R-049/041），旧档（无 sizeof 字段）读侧自然 magic 后短读/值不
   匹配 → 拒读，行为安全。 */
int jq_save_game_write(const char *path, const jq_state *st)
{
    if (!path || !st) return 0;
    /* ★ AUD-1000-ALL A2-024（P1 已修）：原子写——one.sjg 旧实现 fopen(path,"wb") 直写目标，
       盘满/崩溃半写 → 残留 magic+sizeof 无 body 的残缺文件（读侧虽 magic 校验拒读，
       但目标文件被破坏，下次正常写前读旧档会拿到损坏态）。改 tmp+rename：写 path.tmp
       成功才 rename 原子替换，崩溃时目标文件要么是旧完整版、要么不存在。 */
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return 0;
    uint32_t m = SJG_MAGIC;
    uint32_t sz = (uint32_t)SJG_STATE_BYTES;
    /* ★ B10-016（P1 已修）：两 fwrite 返回值均检（盘满半写 magic 无 body 落盘
       会被读侧 magic OK 但短读掩盖为"文件损坏"；修：写失败 remove 清档）。 */
    if (fwrite(&m, 4, 1, f) != 1 || fwrite(&sz, 4, 1, f) != 1 ||
        fwrite(st, 1, SJG_STATE_BYTES, f) != SJG_STATE_BYTES) {
        fclose(f);
        remove(tmp);   /* ★ A2-024：清临时文件（目标文件保持旧完整版） */
        return 0;
    }
    fclose(f);
    if (rename(tmp, path) != 0) {   /* 原子替换 */
        remove(tmp);
        return 0;
    }
    return 1;
}

int jq_save_game_read(const char *path, jq_state *st)
{
    if (!path || !st) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint32_t m = 0, sz = 0;
    /* ★ B02-071/072（P1 已修）：magic + sizeof 双字段校验（旧版单 magic 读新档
       版本升级字段错位无感知）。旧档（无 sz 字段）fread sz 读到 st body 首 4 字节
       必 != sizeof → 拒读（自有格式无兼容需求，安全）。 */
    if (fread(&m, 4, 1, f) != 1 || m != SJG_MAGIC ||
        fread(&sz, 4, 1, f) != 1 || sz != (uint32_t)SJG_STATE_BYTES) {
        fclose(f);
        return 0;
    }
    /* ★ AUD-R-049/041（P2）：整结构读取（与写侧同版 sizeof，已按 sz 校验） */
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
