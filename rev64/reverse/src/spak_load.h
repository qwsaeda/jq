/* spak_load.h — 军棋 SPAK 资源加载器接口（R5） */
#ifndef SPAK_LOAD_H
#define SPAK_LOAD_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 加载 assets 目录下的全部 SPAK 组，返回 0 成功 */
int spak_load(const char *assets_dir);
/* 从 Android AAssetManager 加载（APK assets 无文件路径，用此变体）。
   mgr 来自 Activity.getAssets()。返回 0 成功。 */
int spak_load_assetmanager(void *mgr);
/* 按路径查找资源（MD5(路径) 匹配），返回解压数据或 NULL */
const uint8_t *spak_get(const char *path, uint32_t *out_len);
/* 按 16 字节 hash 查找资源（部分资源路径需运行时拼接，hash 已知但路径未逆出） */
const uint8_t *spak_get_by_hash(const uint8_t hash[16], uint32_t *out_len);
/* 释放全部资源 */
void spak_free(void);

#ifdef __cplusplus
}
#endif
#endif /* SPAK_LOAD_H */
