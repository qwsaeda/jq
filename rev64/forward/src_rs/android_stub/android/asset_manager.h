/*
 * asset_manager.h — 桌面构建 stub（src_rs/android_stub/android/）
 * 最小 AAsset API（jni_bridge.c 资源加载用：open / read / getLength / close）
 */
#ifndef ANDROID_STUB_ASSET_MANAGER_H
#define ANDROID_STUB_ASSET_MANAGER_H
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
typedef struct AAssetManager AAssetManager;
typedef struct AAsset AAsset;
#ifdef __cplusplus
extern "C" {
#endif
AAsset *AAsset_open(const AAssetManager *mgr, const char *filename, int mode);
void    AAsset_close(AAsset *asset);
int     AAsset_read(AAsset *asset, void *buf, size_t count);
off_t   AAsset_getLength(AAsset *asset);
int     AAsset_seek(AAsset *asset, off_t offset, int whence);
#ifdef __cplusplus
}
#endif
#endif
