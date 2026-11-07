# 自动化对抗审计探针（机器穷举）

> 生成器：`audit/probe_audit.py`　条目数：**1610**
> 姊妹文档：子代理深度审计（rules / jni-render / ui-geom / build-pkg / doc-truth / rev32）

## 统计

| 严重级 | 条数 |
|---|---|
| P0 | 0 |
| P1 | 0 |
| P2 | 1065 |
| P3 | 545 |

## 按探针分组

| 探针 | 条数 |
|---|---|
| array-index | 776 |
| todo | 480 |
| coverage | 154 |
| text-delta | 77 |
| unsafe-func | 48 |
| geom-delta | 42 |
| ui-geom | 14 |
| lock-discipline | 5 |
| int-overflow | 5 |
| artifact | 4 |
| test-gap | 3 |
| claim | 1 |
| git | 1 |

---

## AUD-A-0001 [P2] 数组 EV2WAV[i] 下标在 jq_audio_init() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/audio.c:49
- 问题：EV2WAV[i] 索引，EV2WAV 声明大小 [0]
- 依据：函数 jq_audio_init()（src_rs/audio.c:42-56）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0002 [P2] 数组 EV2WAV[i] 下标在 jq_audio_play() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/audio.c:64
- 问题：EV2WAV[i] 索引，EV2WAV 声明大小 [0]
- 依据：函数 jq_audio_play()（src_rs/audio.c:60-72）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0003 [P2] 数组 cells[i] 下标在 mk_flag() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:30
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 mk_flag()（src_rs/evverify.c:26-34）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0004 [P2] 数组 cells[i] 下标在 mk_mine() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:41
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 mk_mine()（src_rs/evverify.c:37-46）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0005 [P2] 数组 cells[i] 下标在 run_undo() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:67
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_undo()（src_rs/evverify.c:63-100）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0006 [P2] 数组 cells[f] 下标在 run_undo() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:78
- 问题：cells[f] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_undo()（src_rs/evverify.c:63-100）体内未对 f 做范围检查
- 修法：在索引前加 if (f < 0 || f >= 59) 拒绝并记日志

## AUD-A-0007 [P2] 数组 cells[f] 下标在 run_undo() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:79
- 问题：cells[f] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_undo()（src_rs/evverify.c:63-100）体内未对 f 做范围检查
- 修法：在索引前加 if (f < 0 || f >= 59) 拒绝并记日志

## AUD-A-0008 [P2] 数组 cells[i] 下标在 run_draw_resign() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:108
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_draw_resign()（src_rs/evverify.c:104-120）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0009 [P2] 数组 cells[i] 下标在 run_draw_resign() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:115
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_draw_resign()（src_rs/evverify.c:104-120）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0010 [P2] 数组 cells[i] 下标在 run_stalemate() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:128
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_stalemate()（src_rs/evverify.c:124-133）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0011 [P2] 数组 cells[i] 下标在 run_layout_save() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:205
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_layout_save()（src_rs/evverify.c:198-242）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0012 [P2] 数组 lay[i] 下标在 run_layout_save() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:210
- 问题：lay[i] 索引，lay 声明大小 [30]
- 依据：函数 run_layout_save()（src_rs/evverify.c:198-242）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 30) 拒绝并记日志

## AUD-A-0013 [P2] 数组 cells[i] 下标在 run_layout_save() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:231
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_layout_save()（src_rs/evverify.c:198-242）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0014 [P2] 数组 cells[i] 下标在 run_layout_save() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:232
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_layout_save()（src_rs/evverify.c:198-242）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0015 [P2] 数组 seq[N_REPLAY] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:259
- 问题：seq[N_REPLAY] 索引，seq 声明大小 [6]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 N_REPLAY 做范围检查
- 修法：在索引前加 if (N_REPLAY < 0 || N_REPLAY >= 6) 拒绝并记日志

## AUD-A-0016 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:262
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0017 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:294
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0018 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:302
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0019 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:312
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0020 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:313
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0021 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:324
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0022 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:325
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0023 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:331
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0024 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:340
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0025 [P2] 数组 cells[i] 下标在 run_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:341
- 问题：cells[i] 索引，cells 声明大小 [25, 30, 49, 53, 54, 58, 59]
- 依据：函数 run_replay()（src_rs/evverify.c:250-361）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0026 [P2] 数组 rb[i] 下标在 run_rules() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:454
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_rules()（src_rs/evverify.c:443-503）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0027 [P2] 数组 rb[i] 下标在 run_rules() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:455
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_rules()（src_rs/evverify.c:443-503）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0028 [P2] 数组 rb[i] 下标在 run_rules() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:460
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_rules()（src_rs/evverify.c:443-503）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0029 [P2] 数组 rb[i] 下标在 run_rules() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:466
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_rules()（src_rs/evverify.c:443-503）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0030 [P2] 数组 jb[i] 下标在 run_join() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:602
- 问题：jb[i] 索引，jb 声明大小 [0, 1, 2, 8, 9]
- 依据：函数 run_join()（src_rs/evverify.c:591-651）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 9) 拒绝并记日志

## AUD-A-0031 [P2] 数组 jb[i] 下标在 run_join() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:603
- 问题：jb[i] 索引，jb 声明大小 [0, 1, 2, 8, 9]
- 依据：函数 run_join()（src_rs/evverify.c:591-651）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 9) 拒绝并记日志

## AUD-A-0032 [P2] 数组 jb[i] 下标在 run_join() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:608
- 问题：jb[i] 索引，jb 声明大小 [0, 1, 2, 8, 9]
- 依据：函数 run_join()（src_rs/evverify.c:591-651）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 9) 拒绝并记日志

## AUD-A-0033 [P2] 数组 jb[i] 下标在 run_join() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:614
- 问题：jb[i] 索引，jb 声明大小 [0, 1, 2, 8, 9]
- 依据：函数 run_join()（src_rs/evverify.c:591-651）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 9) 拒绝并记日志

## AUD-A-0034 [P2] 数组 cb[i] 下标在 run_create() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:668
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_create()（src_rs/evverify.c:657-717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0035 [P2] 数组 cb[i] 下标在 run_create() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:669
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_create()（src_rs/evverify.c:657-717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0036 [P2] 数组 cb[i] 下标在 run_create() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:674
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_create()（src_rs/evverify.c:657-717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0037 [P2] 数组 cb[i] 下标在 run_create() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:680
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_create()（src_rs/evverify.c:657-717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0038 [P2] 数组 gb[i] 下标在 run_gameinfo() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:840
- 问题：gb[i] 索引，gb 声明大小 [0, 1, 2]
- 依据：函数 run_gameinfo()（src_rs/evverify.c:829-889）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0039 [P2] 数组 gb[i] 下标在 run_gameinfo() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:841
- 问题：gb[i] 索引，gb 声明大小 [0, 1, 2]
- 依据：函数 run_gameinfo()（src_rs/evverify.c:829-889）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0040 [P2] 数组 gb[i] 下标在 run_gameinfo() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:846
- 问题：gb[i] 索引，gb 声明大小 [0, 1, 2]
- 依据：函数 run_gameinfo()（src_rs/evverify.c:829-889）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0041 [P2] 数组 gb[i] 下标在 run_gameinfo() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:852
- 问题：gb[i] 索引，gb 声明大小 [0, 1, 2]
- 依据：函数 run_gameinfo()（src_rs/evverify.c:829-889）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0042 [P2] 数组 bb[i] 下标在 run_bright() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:906
- 问题：bb[i] 索引，bb 声明大小 [0, 1, 2]
- 依据：函数 run_bright()（src_rs/evverify.c:895-959）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0043 [P2] 数组 bb[i] 下标在 run_bright() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:907
- 问题：bb[i] 索引，bb 声明大小 [0, 1, 2]
- 依据：函数 run_bright()（src_rs/evverify.c:895-959）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0044 [P2] 数组 bb[i] 下标在 run_bright() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:912
- 问题：bb[i] 索引，bb 声明大小 [0, 1, 2]
- 依据：函数 run_bright()（src_rs/evverify.c:895-959）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0045 [P2] 数组 bb[i] 下标在 run_bright() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:918
- 问题：bb[i] 索引，bb 声明大小 [0, 1, 2]
- 依据：函数 run_bright()（src_rs/evverify.c:895-959）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0046 [P2] 数组 cb[i] 下标在 run_chatlog() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:975
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chatlog()（src_rs/evverify.c:965-1017）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0047 [P2] 数组 cb[i] 下标在 run_chatlog() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:976
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chatlog()（src_rs/evverify.c:965-1017）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0048 [P2] 数组 cb[i] 下标在 run_chatlog() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:981
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chatlog()（src_rs/evverify.c:965-1017）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0049 [P2] 数组 cb[i] 下标在 run_chatlog() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:987
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chatlog()（src_rs/evverify.c:965-1017）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0050 [P2] 数组 pb[i] 下标在 run_profile() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1034
- 问题：pb[i] 索引，pb 声明大小 [0, 1, 2]
- 依据：函数 run_profile()（src_rs/evverify.c:1023-1083）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0051 [P2] 数组 pb[i] 下标在 run_profile() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1035
- 问题：pb[i] 索引，pb 声明大小 [0, 1, 2]
- 依据：函数 run_profile()（src_rs/evverify.c:1023-1083）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0052 [P2] 数组 pb[i] 下标在 run_profile() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1040
- 问题：pb[i] 索引，pb 声明大小 [0, 1, 2]
- 依据：函数 run_profile()（src_rs/evverify.c:1023-1083）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0053 [P2] 数组 pb[i] 下标在 run_profile() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1046
- 问题：pb[i] 索引，pb 声明大小 [0, 1, 2]
- 依据：函数 run_profile()（src_rs/evverify.c:1023-1083）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0054 [P2] 数组 sb[i] 下标在 run_setentry() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1211
- 问题：sb[i] 索引，sb 声明大小 [0]
- 依据：函数 run_setentry()（src_rs/evverify.c:1200-1265）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0055 [P2] 数组 sb[i] 下标在 run_setentry() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1212
- 问题：sb[i] 索引，sb 声明大小 [0]
- 依据：函数 run_setentry()（src_rs/evverify.c:1200-1265）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0056 [P2] 数组 sb[i] 下标在 run_setentry() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1217
- 问题：sb[i] 索引，sb 声明大小 [0]
- 依据：函数 run_setentry()（src_rs/evverify.c:1200-1265）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0057 [P2] 数组 sb[i] 下标在 run_setentry() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1223
- 问题：sb[i] 索引，sb 声明大小 [0]
- 依据：函数 run_setentry()（src_rs/evverify.c:1200-1265）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0058 [P2] 数组 s4[i] 下标在 run_setentry4() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1284
- 问题：s4[i] 索引，s4 声明大小 [0]
- 依据：函数 run_setentry4()（src_rs/evverify.c:1274-1331）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0059 [P2] 数组 s4[i] 下标在 run_setentry4() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1285
- 问题：s4[i] 索引，s4 声明大小 [0]
- 依据：函数 run_setentry4()（src_rs/evverify.c:1274-1331）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0060 [P2] 数组 s4[i] 下标在 run_setentry4() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1290
- 问题：s4[i] 索引，s4 声明大小 [0]
- 依据：函数 run_setentry4()（src_rs/evverify.c:1274-1331）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0061 [P2] 数组 s4[i] 下标在 run_setentry4() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1296
- 问题：s4[i] 索引，s4 声明大小 [0]
- 依据：函数 run_setentry4()（src_rs/evverify.c:1274-1331）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0062 [P2] 数组 rb[i] 下标在 run_room060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1813
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_room060()（src_rs/evverify.c:1802-1868）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0063 [P2] 数组 rb[i] 下标在 run_room060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1814
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_room060()（src_rs/evverify.c:1802-1868）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0064 [P2] 数组 rb[i] 下标在 run_room060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1819
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_room060()（src_rs/evverify.c:1802-1868）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0065 [P2] 数组 rb[i] 下标在 run_room060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1825
- 问题：rb[i] 索引，rb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_room060()（src_rs/evverify.c:1802-1868）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0066 [P2] 数组 cb[i] 下标在 run_chat063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1888
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chat063()（src_rs/evverify.c:1877-1947）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0067 [P2] 数组 cb[i] 下标在 run_chat063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1889
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chat063()（src_rs/evverify.c:1877-1947）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0068 [P2] 数组 cb[i] 下标在 run_chat063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1894
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chat063()（src_rs/evverify.c:1877-1947）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0069 [P2] 数组 cb[i] 下标在 run_chat063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1900
- 问题：cb[i] 索引，cb 声明大小 [0, 1, 2, 3, 4, 5]
- 依据：函数 run_chat063()（src_rs/evverify.c:1877-1947）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0070 [P2] 数组 ab[i] 下标在 run_about() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1966
- 问题：ab[i] 索引，ab 声明大小 [0, 1]
- 依据：函数 run_about()（src_rs/evverify.c:1956-2014）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0071 [P2] 数组 ab[i] 下标在 run_about() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1967
- 问题：ab[i] 索引，ab 声明大小 [0, 1]
- 依据：函数 run_about()（src_rs/evverify.c:1956-2014）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0072 [P2] 数组 ab[i] 下标在 run_about() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1972
- 问题：ab[i] 索引，ab 声明大小 [0, 1]
- 依据：函数 run_about()（src_rs/evverify.c:1956-2014）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0073 [P2] 数组 ab[i] 下标在 run_about() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:1978
- 问题：ab[i] 索引，ab 声明大小 [0, 1]
- 依据：函数 run_about()（src_rs/evverify.c:1956-2014）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0074 [P2] 数组 b[i] 下标在 run_ui44() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2377
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui44()（src_rs/evverify.c:2368-2403）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0075 [P2] 数组 b[i] 下标在 run_ui44() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2378
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui44()（src_rs/evverify.c:2368-2403）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0076 [P2] 数组 b[i] 下标在 run_ui44() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2382
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui44()（src_rs/evverify.c:2368-2403）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0077 [P2] 数组 b[i] 下标在 run_ui44() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2386
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui44()（src_rs/evverify.c:2368-2403）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0078 [P2] 数组 b[i] 下标在 run_ui45() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2414
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui45()（src_rs/evverify.c:2406-2440）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0079 [P2] 数组 b[i] 下标在 run_ui45() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2415
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui45()（src_rs/evverify.c:2406-2440）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0080 [P2] 数组 b[i] 下标在 run_ui45() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2419
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui45()（src_rs/evverify.c:2406-2440）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0081 [P2] 数组 b[i] 下标在 run_ui45() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2423
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui45()（src_rs/evverify.c:2406-2440）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0082 [P2] 数组 b[i] 下标在 run_ui46() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2451
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui46()（src_rs/evverify.c:2443-2483）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0083 [P2] 数组 b[i] 下标在 run_ui46() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2452
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui46()（src_rs/evverify.c:2443-2483）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0084 [P2] 数组 b[i] 下标在 run_ui46() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2456
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui46()（src_rs/evverify.c:2443-2483）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0085 [P2] 数组 b[i] 下标在 run_ui46() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2460
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui46()（src_rs/evverify.c:2443-2483）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0086 [P2] 数组 b[i] 下标在 run_ui47() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2494
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui47()（src_rs/evverify.c:2486-2526）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0087 [P2] 数组 b[i] 下标在 run_ui47() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2495
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui47()（src_rs/evverify.c:2486-2526）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0088 [P2] 数组 b[i] 下标在 run_ui47() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2499
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui47()（src_rs/evverify.c:2486-2526）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0089 [P2] 数组 b[i] 下标在 run_ui47() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2503
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui47()（src_rs/evverify.c:2486-2526）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0090 [P2] 数组 b[i] 下标在 run_ui49() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2537
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui49()（src_rs/evverify.c:2529-2569）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0091 [P2] 数组 b[i] 下标在 run_ui49() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2538
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui49()（src_rs/evverify.c:2529-2569）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0092 [P2] 数组 b[i] 下标在 run_ui49() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2542
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui49()（src_rs/evverify.c:2529-2569）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0093 [P2] 数组 b[i] 下标在 run_ui49() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2546
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui49()（src_rs/evverify.c:2529-2569）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0094 [P2] 数组 b[i] 下标在 run_ui51() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2581
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui51()（src_rs/evverify.c:2572-2618）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0095 [P2] 数组 b[i] 下标在 run_ui51() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2582
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui51()（src_rs/evverify.c:2572-2618）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0096 [P2] 数组 b[i] 下标在 run_ui51() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2586
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui51()（src_rs/evverify.c:2572-2618）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0097 [P2] 数组 b[i] 下标在 run_ui51() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2590
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui51()（src_rs/evverify.c:2572-2618）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0098 [P2] 数组 b[i] 下标在 run_ui54() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2630
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui54()（src_rs/evverify.c:2621-2666）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0099 [P2] 数组 b[i] 下标在 run_ui54() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2631
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui54()（src_rs/evverify.c:2621-2666）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0100 [P2] 数组 b[i] 下标在 run_ui54() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2635
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui54()（src_rs/evverify.c:2621-2666）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0101 [P2] 数组 b[i] 下标在 run_ui54() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2639
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui54()（src_rs/evverify.c:2621-2666）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0102 [P2] 数组 b[i] 下标在 run_ui73() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2678
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui73()（src_rs/evverify.c:2669-2717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0103 [P2] 数组 b[i] 下标在 run_ui73() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2679
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui73()（src_rs/evverify.c:2669-2717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0104 [P2] 数组 b[i] 下标在 run_ui73() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2683
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui73()（src_rs/evverify.c:2669-2717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0105 [P2] 数组 b[i] 下标在 run_ui73() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2687
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui73()（src_rs/evverify.c:2669-2717）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0106 [P2] 数组 b[i] 下标在 run_ui90() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2729
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui90()（src_rs/evverify.c:2720-2768）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0107 [P2] 数组 b[i] 下标在 run_ui90() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2730
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui90()（src_rs/evverify.c:2720-2768）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0108 [P2] 数组 b[i] 下标在 run_ui90() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2734
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui90()（src_rs/evverify.c:2720-2768）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0109 [P2] 数组 b[i] 下标在 run_ui90() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2738
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui90()（src_rs/evverify.c:2720-2768）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0110 [P2] 数组 b[i] 下标在 run_ui100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2779
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui100()（src_rs/evverify.c:2771-2811）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0111 [P2] 数组 b[i] 下标在 run_ui100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2780
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui100()（src_rs/evverify.c:2771-2811）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0112 [P2] 数组 b[i] 下标在 run_ui100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2784
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui100()（src_rs/evverify.c:2771-2811）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0113 [P2] 数组 b[i] 下标在 run_ui100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2788
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui100()（src_rs/evverify.c:2771-2811）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0114 [P2] 数组 b[i] 下标在 run_ui110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2823
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui110()（src_rs/evverify.c:2814-2859）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0115 [P2] 数组 b[i] 下标在 run_ui110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2824
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui110()（src_rs/evverify.c:2814-2859）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0116 [P2] 数组 b[i] 下标在 run_ui110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2828
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui110()（src_rs/evverify.c:2814-2859）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0117 [P2] 数组 b[i] 下标在 run_ui110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2832
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui110()（src_rs/evverify.c:2814-2859）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0118 [P2] 数组 b[i] 下标在 run_ui118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2871
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui118()（src_rs/evverify.c:2862-2908）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0119 [P2] 数组 b[i] 下标在 run_ui118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2872
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui118()（src_rs/evverify.c:2862-2908）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0120 [P2] 数组 b[i] 下标在 run_ui118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2876
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui118()（src_rs/evverify.c:2862-2908）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0121 [P2] 数组 b[i] 下标在 run_ui118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2880
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_ui118()（src_rs/evverify.c:2862-2908）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0122 [P2] 数组 b[i] 下标在 run_exitconfirm() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2919
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_exitconfirm()（src_rs/evverify.c:2912-2929）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0123 [P2] 数组 b[i] 下标在 run_exitconfirm() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2920
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_exitconfirm()（src_rs/evverify.c:2912-2929）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0124 [P2] 数组 b[i] 下标在 run_d000() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2967
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d000()（src_rs/evverify.c:2958-2981）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0125 [P2] 数组 b[i] 下标在 run_d000() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2968
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d000()（src_rs/evverify.c:2958-2981）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0126 [P2] 数组 b[i] 下标在 run_d000() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2974
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d000()（src_rs/evverify.c:2958-2981）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0127 [P2] 数组 b[i] 下标在 run_d001() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2994
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d001()（src_rs/evverify.c:2983-3008）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0128 [P2] 数组 b[i] 下标在 run_d001() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:2995
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d001()（src_rs/evverify.c:2983-3008）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0129 [P2] 数组 b[i] 下标在 run_d001() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3001
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d001()（src_rs/evverify.c:2983-3008）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0130 [P2] 数组 b[i] 下标在 run_d002() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3019
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d002()（src_rs/evverify.c:3010-3033）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0131 [P2] 数组 b[i] 下标在 run_d002() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3020
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d002()（src_rs/evverify.c:3010-3033）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0132 [P2] 数组 b[i] 下标在 run_d002() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3026
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d002()（src_rs/evverify.c:3010-3033）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0133 [P2] 数组 b[i] 下标在 run_d003() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3045
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d003()（src_rs/evverify.c:3035-3059）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0134 [P2] 数组 b[i] 下标在 run_d003() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3046
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d003()（src_rs/evverify.c:3035-3059）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0135 [P2] 数组 b[i] 下标在 run_d003() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3052
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d003()（src_rs/evverify.c:3035-3059）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0136 [P2] 数组 b[i] 下标在 run_d004() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3070
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d004()（src_rs/evverify.c:3061-3084）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0137 [P2] 数组 b[i] 下标在 run_d004() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3071
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d004()（src_rs/evverify.c:3061-3084）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0138 [P2] 数组 b[i] 下标在 run_d004() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3077
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d004()（src_rs/evverify.c:3061-3084）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0139 [P2] 数组 b[i] 下标在 run_d005() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3095
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d005()（src_rs/evverify.c:3086-3109）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0140 [P2] 数组 b[i] 下标在 run_d005() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3096
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d005()（src_rs/evverify.c:3086-3109）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0141 [P2] 数组 b[i] 下标在 run_d005() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3102
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d005()（src_rs/evverify.c:3086-3109）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0142 [P2] 数组 b[i] 下标在 run_d006() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3126
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d006()（src_rs/evverify.c:3111-3140）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0143 [P2] 数组 b[i] 下标在 run_d006() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3127
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d006()（src_rs/evverify.c:3111-3140）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0144 [P2] 数组 b[i] 下标在 run_d006() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3133
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d006()（src_rs/evverify.c:3111-3140）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0145 [P2] 数组 b[i] 下标在 run_d007() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3155
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d007()（src_rs/evverify.c:3142-3169）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0146 [P2] 数组 b[i] 下标在 run_d007() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3156
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d007()（src_rs/evverify.c:3142-3169）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0147 [P2] 数组 b[i] 下标在 run_d007() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3162
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d007()（src_rs/evverify.c:3142-3169）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0148 [P2] 数组 b[i] 下标在 run_d008() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3183
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d008()（src_rs/evverify.c:3171-3197）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0149 [P2] 数组 b[i] 下标在 run_d008() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3184
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d008()（src_rs/evverify.c:3171-3197）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0150 [P2] 数组 b[i] 下标在 run_d008() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3190
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d008()（src_rs/evverify.c:3171-3197）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0151 [P2] 数组 b[i] 下标在 run_d010() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3208
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d010()（src_rs/evverify.c:3199-3222）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0152 [P2] 数组 b[i] 下标在 run_d010() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3209
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d010()（src_rs/evverify.c:3199-3222）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0153 [P2] 数组 b[i] 下标在 run_d010() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3215
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d010()（src_rs/evverify.c:3199-3222）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0154 [P2] 数组 b[i] 下标在 run_d012() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3237
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d012()（src_rs/evverify.c:3224-3251）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0155 [P2] 数组 b[i] 下标在 run_d012() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3238
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d012()（src_rs/evverify.c:3224-3251）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0156 [P2] 数组 b[i] 下标在 run_d012() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3244
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d012()（src_rs/evverify.c:3224-3251）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0157 [P2] 数组 b[i] 下标在 run_d013() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3263
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d013()（src_rs/evverify.c:3253-3277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0158 [P2] 数组 b[i] 下标在 run_d013() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3264
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d013()（src_rs/evverify.c:3253-3277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0159 [P2] 数组 b[i] 下标在 run_d013() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3270
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d013()（src_rs/evverify.c:3253-3277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0160 [P2] 数组 b[i] 下标在 run_d015() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3287
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d015()（src_rs/evverify.c:3279-3301）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0161 [P2] 数组 b[i] 下标在 run_d015() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3288
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d015()（src_rs/evverify.c:3279-3301）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0162 [P2] 数组 b[i] 下标在 run_d015() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3294
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d015()（src_rs/evverify.c:3279-3301）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0163 [P2] 数组 b[i] 下标在 run_d016() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3314
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d016()（src_rs/evverify.c:3303-3328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0164 [P2] 数组 b[i] 下标在 run_d016() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3315
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d016()（src_rs/evverify.c:3303-3328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0165 [P2] 数组 b[i] 下标在 run_d016() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3321
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d016()（src_rs/evverify.c:3303-3328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0166 [P2] 数组 b[i] 下标在 run_d017() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3338
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d017()（src_rs/evverify.c:3330-3352）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0167 [P2] 数组 b[i] 下标在 run_d017() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3339
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d017()（src_rs/evverify.c:3330-3352）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0168 [P2] 数组 b[i] 下标在 run_d017() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3345
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d017()（src_rs/evverify.c:3330-3352）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0169 [P2] 数组 b[i] 下标在 run_d019() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3365
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d019()（src_rs/evverify.c:3354-3379）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0170 [P2] 数组 b[i] 下标在 run_d019() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3366
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d019()（src_rs/evverify.c:3354-3379）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0171 [P2] 数组 b[i] 下标在 run_d019() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3372
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d019()（src_rs/evverify.c:3354-3379）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0172 [P2] 数组 b[i] 下标在 run_d020() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3392
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d020()（src_rs/evverify.c:3381-3406）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0173 [P2] 数组 b[i] 下标在 run_d020() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3393
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d020()（src_rs/evverify.c:3381-3406）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0174 [P2] 数组 b[i] 下标在 run_d020() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3399
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d020()（src_rs/evverify.c:3381-3406）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0175 [P2] 数组 b[i] 下标在 run_d021() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3418
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d021()（src_rs/evverify.c:3408-3432）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0176 [P2] 数组 b[i] 下标在 run_d021() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3419
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d021()（src_rs/evverify.c:3408-3432）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0177 [P2] 数组 b[i] 下标在 run_d021() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3425
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d021()（src_rs/evverify.c:3408-3432）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0178 [P2] 数组 b[i] 下标在 run_d022() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3446
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d022()（src_rs/evverify.c:3434-3460）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0179 [P2] 数组 b[i] 下标在 run_d022() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3447
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d022()（src_rs/evverify.c:3434-3460）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0180 [P2] 数组 b[i] 下标在 run_d022() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3453
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d022()（src_rs/evverify.c:3434-3460）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0181 [P2] 数组 b[i] 下标在 run_d023() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3473
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d023()（src_rs/evverify.c:3462-3487）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0182 [P2] 数组 b[i] 下标在 run_d023() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3474
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d023()（src_rs/evverify.c:3462-3487）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0183 [P2] 数组 b[i] 下标在 run_d023() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3480
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d023()（src_rs/evverify.c:3462-3487）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0184 [P2] 数组 b[i] 下标在 run_d024() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3497
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d024()（src_rs/evverify.c:3489-3511）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0185 [P2] 数组 b[i] 下标在 run_d024() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3498
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d024()（src_rs/evverify.c:3489-3511）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0186 [P2] 数组 b[i] 下标在 run_d024() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3504
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d024()（src_rs/evverify.c:3489-3511）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0187 [P2] 数组 b[i] 下标在 run_d025() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3521
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d025()（src_rs/evverify.c:3513-3535）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0188 [P2] 数组 b[i] 下标在 run_d025() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3522
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d025()（src_rs/evverify.c:3513-3535）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0189 [P2] 数组 b[i] 下标在 run_d025() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3528
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d025()（src_rs/evverify.c:3513-3535）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0190 [P2] 数组 b[i] 下标在 run_d026() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3547
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d026()（src_rs/evverify.c:3537-3561）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0191 [P2] 数组 b[i] 下标在 run_d026() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3548
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d026()（src_rs/evverify.c:3537-3561）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0192 [P2] 数组 b[i] 下标在 run_d026() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3554
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d026()（src_rs/evverify.c:3537-3561）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0193 [P2] 数组 b[i] 下标在 run_d027() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3573
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d027()（src_rs/evverify.c:3563-3587）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0194 [P2] 数组 b[i] 下标在 run_d027() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3574
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d027()（src_rs/evverify.c:3563-3587）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0195 [P2] 数组 b[i] 下标在 run_d027() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3580
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d027()（src_rs/evverify.c:3563-3587）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0196 [P2] 数组 b[i] 下标在 run_d028() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3599
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d028()（src_rs/evverify.c:3589-3613）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0197 [P2] 数组 b[i] 下标在 run_d028() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3600
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d028()（src_rs/evverify.c:3589-3613）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0198 [P2] 数组 b[i] 下标在 run_d028() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3606
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d028()（src_rs/evverify.c:3589-3613）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0199 [P2] 数组 b[i] 下标在 run_d029() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3624
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d029()（src_rs/evverify.c:3615-3638）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0200 [P2] 数组 b[i] 下标在 run_d029() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3625
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d029()（src_rs/evverify.c:3615-3638）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0201 [P2] 数组 b[i] 下标在 run_d029() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3631
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d029()（src_rs/evverify.c:3615-3638）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0202 [P2] 数组 b[i] 下标在 run_d030() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3649
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d030()（src_rs/evverify.c:3640-3663）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0203 [P2] 数组 b[i] 下标在 run_d030() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3650
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d030()（src_rs/evverify.c:3640-3663）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0204 [P2] 数组 b[i] 下标在 run_d030() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3656
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d030()（src_rs/evverify.c:3640-3663）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0205 [P2] 数组 b[i] 下标在 run_d031() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3675
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d031()（src_rs/evverify.c:3665-3689）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0206 [P2] 数组 b[i] 下标在 run_d031() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3676
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d031()（src_rs/evverify.c:3665-3689）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0207 [P2] 数组 b[i] 下标在 run_d031() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3682
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d031()（src_rs/evverify.c:3665-3689）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0208 [P2] 数组 b[i] 下标在 run_d032() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3702
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d032()（src_rs/evverify.c:3691-3716）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0209 [P2] 数组 b[i] 下标在 run_d032() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3703
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d032()（src_rs/evverify.c:3691-3716）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0210 [P2] 数组 b[i] 下标在 run_d032() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3709
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d032()（src_rs/evverify.c:3691-3716）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0211 [P2] 数组 b[i] 下标在 run_d033() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3727
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d033()（src_rs/evverify.c:3718-3741）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0212 [P2] 数组 b[i] 下标在 run_d033() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3728
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d033()（src_rs/evverify.c:3718-3741）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0213 [P2] 数组 b[i] 下标在 run_d033() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3734
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d033()（src_rs/evverify.c:3718-3741）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0214 [P2] 数组 b[i] 下标在 run_d034() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3752
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d034()（src_rs/evverify.c:3743-3766）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0215 [P2] 数组 b[i] 下标在 run_d034() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3753
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d034()（src_rs/evverify.c:3743-3766）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0216 [P2] 数组 b[i] 下标在 run_d034() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3759
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d034()（src_rs/evverify.c:3743-3766）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0217 [P2] 数组 b[i] 下标在 run_d035() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3777
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d035()（src_rs/evverify.c:3768-3791）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0218 [P2] 数组 b[i] 下标在 run_d035() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3778
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d035()（src_rs/evverify.c:3768-3791）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0219 [P2] 数组 b[i] 下标在 run_d035() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3784
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d035()（src_rs/evverify.c:3768-3791）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0220 [P2] 数组 b[i] 下标在 run_d036() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3804
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d036()（src_rs/evverify.c:3793-3818）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0221 [P2] 数组 b[i] 下标在 run_d036() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3805
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d036()（src_rs/evverify.c:3793-3818）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0222 [P2] 数组 b[i] 下标在 run_d036() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3811
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d036()（src_rs/evverify.c:3793-3818）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0223 [P2] 数组 b[i] 下标在 run_d037() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3830
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d037()（src_rs/evverify.c:3820-3844）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0224 [P2] 数组 b[i] 下标在 run_d037() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3831
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d037()（src_rs/evverify.c:3820-3844）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0225 [P2] 数组 b[i] 下标在 run_d037() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3837
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d037()（src_rs/evverify.c:3820-3844）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0226 [P2] 数组 b[i] 下标在 run_d038() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3857
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d038()（src_rs/evverify.c:3846-3871）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0227 [P2] 数组 b[i] 下标在 run_d038() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3858
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d038()（src_rs/evverify.c:3846-3871）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0228 [P2] 数组 b[i] 下标在 run_d038() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3864
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d038()（src_rs/evverify.c:3846-3871）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0229 [P2] 数组 b[i] 下标在 run_d039() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3881
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d039()（src_rs/evverify.c:3873-3895）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0230 [P2] 数组 b[i] 下标在 run_d039() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3882
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d039()（src_rs/evverify.c:3873-3895）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0231 [P2] 数组 b[i] 下标在 run_d039() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3888
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d039()（src_rs/evverify.c:3873-3895）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0232 [P2] 数组 b[i] 下标在 run_d041() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3912
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d041()（src_rs/evverify.c:3897-3926）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0233 [P2] 数组 b[i] 下标在 run_d041() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3913
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d041()（src_rs/evverify.c:3897-3926）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0234 [P2] 数组 b[i] 下标在 run_d041() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3919
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d041()（src_rs/evverify.c:3897-3926）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0235 [P2] 数组 b[i] 下标在 run_d042() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3942
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d042()（src_rs/evverify.c:3928-3956）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0236 [P2] 数组 b[i] 下标在 run_d042() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3943
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d042()（src_rs/evverify.c:3928-3956）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0237 [P2] 数组 b[i] 下标在 run_d042() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3949
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d042()（src_rs/evverify.c:3928-3956）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0238 [P2] 数组 b[i] 下标在 run_d043() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3967
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d043()（src_rs/evverify.c:3958-3981）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0239 [P2] 数组 b[i] 下标在 run_d043() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3968
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d043()（src_rs/evverify.c:3958-3981）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0240 [P2] 数组 b[i] 下标在 run_d043() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3974
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d043()（src_rs/evverify.c:3958-3981）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0241 [P2] 数组 b[i] 下标在 run_d044() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3992
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d044()（src_rs/evverify.c:3983-4006）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0242 [P2] 数组 b[i] 下标在 run_d044() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3993
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d044()（src_rs/evverify.c:3983-4006）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0243 [P2] 数组 b[i] 下标在 run_d044() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:3999
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d044()（src_rs/evverify.c:3983-4006）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0244 [P2] 数组 b[i] 下标在 run_d045() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4016
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d045()（src_rs/evverify.c:4008-4030）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0245 [P2] 数组 b[i] 下标在 run_d045() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4017
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d045()（src_rs/evverify.c:4008-4030）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0246 [P2] 数组 b[i] 下标在 run_d045() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4023
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d045()（src_rs/evverify.c:4008-4030）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0247 [P2] 数组 b[i] 下标在 run_d046() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4040
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d046()（src_rs/evverify.c:4032-4054）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0248 [P2] 数组 b[i] 下标在 run_d046() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4041
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d046()（src_rs/evverify.c:4032-4054）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0249 [P2] 数组 b[i] 下标在 run_d046() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4047
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d046()（src_rs/evverify.c:4032-4054）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0250 [P2] 数组 b[i] 下标在 run_d047() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4064
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d047()（src_rs/evverify.c:4056-4078）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0251 [P2] 数组 b[i] 下标在 run_d047() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4065
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d047()（src_rs/evverify.c:4056-4078）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0252 [P2] 数组 b[i] 下标在 run_d047() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4071
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d047()（src_rs/evverify.c:4056-4078）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0253 [P2] 数组 b[i] 下标在 run_d049() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4088
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d049()（src_rs/evverify.c:4080-4102）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0254 [P2] 数组 b[i] 下标在 run_d049() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4089
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d049()（src_rs/evverify.c:4080-4102）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0255 [P2] 数组 b[i] 下标在 run_d049() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4095
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d049()（src_rs/evverify.c:4080-4102）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0256 [P2] 数组 b[i] 下标在 run_d050() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4114
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d050()（src_rs/evverify.c:4104-4128）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0257 [P2] 数组 b[i] 下标在 run_d050() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4115
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d050()（src_rs/evverify.c:4104-4128）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0258 [P2] 数组 b[i] 下标在 run_d050() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4121
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d050()（src_rs/evverify.c:4104-4128）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0259 [P2] 数组 b[i] 下标在 run_d051() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4139
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d051()（src_rs/evverify.c:4130-4153）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0260 [P2] 数组 b[i] 下标在 run_d051() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4140
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d051()（src_rs/evverify.c:4130-4153）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0261 [P2] 数组 b[i] 下标在 run_d051() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4146
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d051()（src_rs/evverify.c:4130-4153）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0262 [P2] 数组 b[i] 下标在 run_d052() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4164
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d052()（src_rs/evverify.c:4155-4178）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0263 [P2] 数组 b[i] 下标在 run_d052() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4165
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d052()（src_rs/evverify.c:4155-4178）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0264 [P2] 数组 b[i] 下标在 run_d052() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4171
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d052()（src_rs/evverify.c:4155-4178）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0265 [P2] 数组 b[i] 下标在 run_d053() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4189
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d053()（src_rs/evverify.c:4180-4203）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0266 [P2] 数组 b[i] 下标在 run_d053() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4190
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d053()（src_rs/evverify.c:4180-4203）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0267 [P2] 数组 b[i] 下标在 run_d053() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4196
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d053()（src_rs/evverify.c:4180-4203）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0268 [P2] 数组 b[i] 下标在 run_d054() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4214
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d054()（src_rs/evverify.c:4205-4228）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0269 [P2] 数组 b[i] 下标在 run_d054() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4215
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d054()（src_rs/evverify.c:4205-4228）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0270 [P2] 数组 b[i] 下标在 run_d054() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4221
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d054()（src_rs/evverify.c:4205-4228）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0271 [P2] 数组 b[i] 下标在 run_d055() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4238
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d055()（src_rs/evverify.c:4230-4252）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0272 [P2] 数组 b[i] 下标在 run_d055() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4239
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d055()（src_rs/evverify.c:4230-4252）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0273 [P2] 数组 b[i] 下标在 run_d055() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4245
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d055()（src_rs/evverify.c:4230-4252）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0274 [P2] 数组 b[i] 下标在 run_d057() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4263
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d057()（src_rs/evverify.c:4254-4277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0275 [P2] 数组 b[i] 下标在 run_d057() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4264
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d057()（src_rs/evverify.c:4254-4277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0276 [P2] 数组 b[i] 下标在 run_d057() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4270
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d057()（src_rs/evverify.c:4254-4277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0277 [P2] 数组 b[i] 下标在 run_d058() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4288
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d058()（src_rs/evverify.c:4279-4302）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0278 [P2] 数组 b[i] 下标在 run_d058() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4289
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d058()（src_rs/evverify.c:4279-4302）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0279 [P2] 数组 b[i] 下标在 run_d058() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4295
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d058()（src_rs/evverify.c:4279-4302）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0280 [P2] 数组 b[i] 下标在 run_d059() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4314
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d059()（src_rs/evverify.c:4304-4328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0281 [P2] 数组 b[i] 下标在 run_d059() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4315
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d059()（src_rs/evverify.c:4304-4328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0282 [P2] 数组 b[i] 下标在 run_d059() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4321
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d059()（src_rs/evverify.c:4304-4328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0283 [P2] 数组 b[i] 下标在 run_d060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4342
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d060()（src_rs/evverify.c:4330-4356）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0284 [P2] 数组 b[i] 下标在 run_d060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4343
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d060()（src_rs/evverify.c:4330-4356）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0285 [P2] 数组 b[i] 下标在 run_d060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4349
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d060()（src_rs/evverify.c:4330-4356）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0286 [P2] 数组 b[i] 下标在 run_d061() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4371
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d061()（src_rs/evverify.c:4358-4385）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0287 [P2] 数组 b[i] 下标在 run_d061() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4372
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d061()（src_rs/evverify.c:4358-4385）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0288 [P2] 数组 b[i] 下标在 run_d061() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4378
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d061()（src_rs/evverify.c:4358-4385）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0289 [P2] 数组 b[i] 下标在 run_d062() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4398
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d062()（src_rs/evverify.c:4387-4412）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0290 [P2] 数组 b[i] 下标在 run_d062() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4399
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d062()（src_rs/evverify.c:4387-4412）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0291 [P2] 数组 b[i] 下标在 run_d062() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4405
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d062()（src_rs/evverify.c:4387-4412）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0292 [P2] 数组 b[i] 下标在 run_d063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4424
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d063()（src_rs/evverify.c:4414-4438）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0293 [P2] 数组 b[i] 下标在 run_d063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4425
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d063()（src_rs/evverify.c:4414-4438）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0294 [P2] 数组 b[i] 下标在 run_d063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4431
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d063()（src_rs/evverify.c:4414-4438）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0295 [P2] 数组 b[i] 下标在 run_d064() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4450
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d064()（src_rs/evverify.c:4440-4464）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0296 [P2] 数组 b[i] 下标在 run_d064() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4451
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d064()（src_rs/evverify.c:4440-4464）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0297 [P2] 数组 b[i] 下标在 run_d064() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4457
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d064()（src_rs/evverify.c:4440-4464）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0298 [P2] 数组 b[i] 下标在 run_d065() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4475
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d065()（src_rs/evverify.c:4466-4489）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0299 [P2] 数组 b[i] 下标在 run_d065() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4476
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d065()（src_rs/evverify.c:4466-4489）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0300 [P2] 数组 b[i] 下标在 run_d065() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4482
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d065()（src_rs/evverify.c:4466-4489）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0301 [P2] 数组 b[i] 下标在 run_d066() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4501
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d066()（src_rs/evverify.c:4491-4515）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0302 [P2] 数组 b[i] 下标在 run_d066() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4502
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d066()（src_rs/evverify.c:4491-4515）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0303 [P2] 数组 b[i] 下标在 run_d066() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4508
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d066()（src_rs/evverify.c:4491-4515）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0304 [P2] 数组 b[i] 下标在 run_d067() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4532
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d067()（src_rs/evverify.c:4517-4546）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0305 [P2] 数组 b[i] 下标在 run_d067() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4533
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d067()（src_rs/evverify.c:4517-4546）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0306 [P2] 数组 b[i] 下标在 run_d067() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4539
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d067()（src_rs/evverify.c:4517-4546）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0307 [P2] 数组 b[i] 下标在 run_d068() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4558
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d068()（src_rs/evverify.c:4548-4572）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0308 [P2] 数组 b[i] 下标在 run_d068() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4559
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d068()（src_rs/evverify.c:4548-4572）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0309 [P2] 数组 b[i] 下标在 run_d068() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4565
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d068()（src_rs/evverify.c:4548-4572）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0310 [P2] 数组 b[i] 下标在 run_d069() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4587
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d069()（src_rs/evverify.c:4574-4601）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0311 [P2] 数组 b[i] 下标在 run_d069() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4588
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d069()（src_rs/evverify.c:4574-4601）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0312 [P2] 数组 b[i] 下标在 run_d069() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4594
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d069()（src_rs/evverify.c:4574-4601）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0313 [P2] 数组 b[i] 下标在 run_d070() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4616
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d070()（src_rs/evverify.c:4603-4630）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0314 [P2] 数组 b[i] 下标在 run_d070() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4617
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d070()（src_rs/evverify.c:4603-4630）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0315 [P2] 数组 b[i] 下标在 run_d070() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4623
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d070()（src_rs/evverify.c:4603-4630）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0316 [P2] 数组 b[i] 下标在 run_d071() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4641
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d071()（src_rs/evverify.c:4632-4655）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0317 [P2] 数组 b[i] 下标在 run_d071() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4642
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d071()（src_rs/evverify.c:4632-4655）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0318 [P2] 数组 b[i] 下标在 run_d071() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4648
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d071()（src_rs/evverify.c:4632-4655）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0319 [P2] 数组 b[i] 下标在 run_d072() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4668
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d072()（src_rs/evverify.c:4657-4682）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0320 [P2] 数组 b[i] 下标在 run_d072() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4669
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d072()（src_rs/evverify.c:4657-4682）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0321 [P2] 数组 b[i] 下标在 run_d072() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4675
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d072()（src_rs/evverify.c:4657-4682）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0322 [P2] 数组 b[i] 下标在 run_d073() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4693
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d073()（src_rs/evverify.c:4684-4707）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0323 [P2] 数组 b[i] 下标在 run_d073() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4694
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d073()（src_rs/evverify.c:4684-4707）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0324 [P2] 数组 b[i] 下标在 run_d073() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4700
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d073()（src_rs/evverify.c:4684-4707）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0325 [P2] 数组 b[i] 下标在 run_d075() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4720
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d075()（src_rs/evverify.c:4709-4734）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0326 [P2] 数组 b[i] 下标在 run_d075() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4721
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d075()（src_rs/evverify.c:4709-4734）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0327 [P2] 数组 b[i] 下标在 run_d075() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4727
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d075()（src_rs/evverify.c:4709-4734）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0328 [P2] 数组 b[i] 下标在 run_d077() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4749
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d077()（src_rs/evverify.c:4736-4763）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0329 [P2] 数组 b[i] 下标在 run_d077() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4750
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d077()（src_rs/evverify.c:4736-4763）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0330 [P2] 数组 b[i] 下标在 run_d077() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4756
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d077()（src_rs/evverify.c:4736-4763）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0331 [P2] 数组 b[i] 下标在 run_d078() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4775
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d078()（src_rs/evverify.c:4765-4789）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0332 [P2] 数组 b[i] 下标在 run_d078() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4776
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d078()（src_rs/evverify.c:4765-4789）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0333 [P2] 数组 b[i] 下标在 run_d078() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4782
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d078()（src_rs/evverify.c:4765-4789）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0334 [P2] 数组 b[i] 下标在 run_d079() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4799
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d079()（src_rs/evverify.c:4791-4813）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0335 [P2] 数组 b[i] 下标在 run_d079() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4800
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d079()（src_rs/evverify.c:4791-4813）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0336 [P2] 数组 b[i] 下标在 run_d079() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4806
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d079()（src_rs/evverify.c:4791-4813）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0337 [P2] 数组 b[i] 下标在 run_d082() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4827
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d082()（src_rs/evverify.c:4815-4841）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0338 [P2] 数组 b[i] 下标在 run_d082() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4828
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d082()（src_rs/evverify.c:4815-4841）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0339 [P2] 数组 b[i] 下标在 run_d082() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4834
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d082()（src_rs/evverify.c:4815-4841）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0340 [P2] 数组 b[i] 下标在 run_d083() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4854
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d083()（src_rs/evverify.c:4843-4868）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0341 [P2] 数组 b[i] 下标在 run_d083() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4855
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d083()（src_rs/evverify.c:4843-4868）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0342 [P2] 数组 b[i] 下标在 run_d083() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4861
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d083()（src_rs/evverify.c:4843-4868）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0343 [P2] 数组 b[i] 下标在 run_d084() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4881
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d084()（src_rs/evverify.c:4870-4895）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0344 [P2] 数组 b[i] 下标在 run_d084() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4882
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d084()（src_rs/evverify.c:4870-4895）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0345 [P2] 数组 b[i] 下标在 run_d084() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4888
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d084()（src_rs/evverify.c:4870-4895）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0346 [P2] 数组 b[i] 下标在 run_d085() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4909
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d085()（src_rs/evverify.c:4897-4923）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0347 [P2] 数组 b[i] 下标在 run_d085() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4910
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d085()（src_rs/evverify.c:4897-4923）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0348 [P2] 数组 b[i] 下标在 run_d085() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4916
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d085()（src_rs/evverify.c:4897-4923）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0349 [P2] 数组 b[i] 下标在 run_d086() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4936
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d086()（src_rs/evverify.c:4925-4950）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0350 [P2] 数组 b[i] 下标在 run_d086() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4937
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d086()（src_rs/evverify.c:4925-4950）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0351 [P2] 数组 b[i] 下标在 run_d086() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4943
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d086()（src_rs/evverify.c:4925-4950）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0352 [P2] 数组 b[i] 下标在 run_d089() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4962
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d089()（src_rs/evverify.c:4952-4976）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0353 [P2] 数组 b[i] 下标在 run_d089() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4963
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d089()（src_rs/evverify.c:4952-4976）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0354 [P2] 数组 b[i] 下标在 run_d089() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4969
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d089()（src_rs/evverify.c:4952-4976）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0355 [P2] 数组 b[i] 下标在 run_d090() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4987
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d090()（src_rs/evverify.c:4978-5001）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0356 [P2] 数组 b[i] 下标在 run_d090() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4988
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d090()（src_rs/evverify.c:4978-5001）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0357 [P2] 数组 b[i] 下标在 run_d090() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:4994
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d090()（src_rs/evverify.c:4978-5001）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0358 [P2] 数组 b[i] 下标在 run_d094() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5015
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d094()（src_rs/evverify.c:5003-5029）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0359 [P2] 数组 b[i] 下标在 run_d094() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5016
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d094()（src_rs/evverify.c:5003-5029）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0360 [P2] 数组 b[i] 下标在 run_d094() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5022
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d094()（src_rs/evverify.c:5003-5029）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0361 [P2] 数组 b[i] 下标在 run_d095() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5043
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d095()（src_rs/evverify.c:5031-5057）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0362 [P2] 数组 b[i] 下标在 run_d095() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5044
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d095()（src_rs/evverify.c:5031-5057）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0363 [P2] 数组 b[i] 下标在 run_d095() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5050
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d095()（src_rs/evverify.c:5031-5057）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0364 [P2] 数组 b[i] 下标在 run_d096() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5069
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d096()（src_rs/evverify.c:5059-5083）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0365 [P2] 数组 b[i] 下标在 run_d096() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5070
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d096()（src_rs/evverify.c:5059-5083）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0366 [P2] 数组 b[i] 下标在 run_d096() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5076
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d096()（src_rs/evverify.c:5059-5083）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0367 [P2] 数组 b[i] 下标在 run_d097() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5096
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d097()（src_rs/evverify.c:5085-5110）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0368 [P2] 数组 b[i] 下标在 run_d097() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5097
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d097()（src_rs/evverify.c:5085-5110）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0369 [P2] 数组 b[i] 下标在 run_d097() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5103
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d097()（src_rs/evverify.c:5085-5110）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0370 [P2] 数组 b[i] 下标在 run_d098() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5125
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d098()（src_rs/evverify.c:5112-5139）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0371 [P2] 数组 b[i] 下标在 run_d098() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5126
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d098()（src_rs/evverify.c:5112-5139）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0372 [P2] 数组 b[i] 下标在 run_d098() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5132
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d098()（src_rs/evverify.c:5112-5139）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0373 [P2] 数组 b[i] 下标在 run_d099() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5152
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d099()（src_rs/evverify.c:5141-5166）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0374 [P2] 数组 b[i] 下标在 run_d099() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5153
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d099()（src_rs/evverify.c:5141-5166）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0375 [P2] 数组 b[i] 下标在 run_d099() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5159
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d099()（src_rs/evverify.c:5141-5166）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0376 [P2] 数组 b[i] 下标在 run_d100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5176
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d100()（src_rs/evverify.c:5168-5190）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0377 [P2] 数组 b[i] 下标在 run_d100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5177
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d100()（src_rs/evverify.c:5168-5190）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0378 [P2] 数组 b[i] 下标在 run_d100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5183
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d100()（src_rs/evverify.c:5168-5190）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0379 [P2] 数组 b[i] 下标在 run_d101() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5203
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d101()（src_rs/evverify.c:5192-5217）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0380 [P2] 数组 b[i] 下标在 run_d101() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5204
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d101()（src_rs/evverify.c:5192-5217）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0381 [P2] 数组 b[i] 下标在 run_d101() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5210
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d101()（src_rs/evverify.c:5192-5217）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0382 [P2] 数组 b[i] 下标在 run_d102() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5227
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d102()（src_rs/evverify.c:5219-5241）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0383 [P2] 数组 b[i] 下标在 run_d102() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5228
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d102()（src_rs/evverify.c:5219-5241）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0384 [P2] 数组 b[i] 下标在 run_d102() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5234
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d102()（src_rs/evverify.c:5219-5241）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0385 [P2] 数组 b[i] 下标在 run_d104() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5255
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d104()（src_rs/evverify.c:5243-5269）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0386 [P2] 数组 b[i] 下标在 run_d104() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5256
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d104()（src_rs/evverify.c:5243-5269）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0387 [P2] 数组 b[i] 下标在 run_d104() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5262
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d104()（src_rs/evverify.c:5243-5269）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0388 [P2] 数组 b[i] 下标在 run_d107() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5283
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d107()（src_rs/evverify.c:5271-5297）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0389 [P2] 数组 b[i] 下标在 run_d107() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5284
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d107()（src_rs/evverify.c:5271-5297）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0390 [P2] 数组 b[i] 下标在 run_d107() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5290
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d107()（src_rs/evverify.c:5271-5297）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0391 [P2] 数组 b[i] 下标在 run_d108() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5309
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d108()（src_rs/evverify.c:5299-5323）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0392 [P2] 数组 b[i] 下标在 run_d108() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5310
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d108()（src_rs/evverify.c:5299-5323）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0393 [P2] 数组 b[i] 下标在 run_d108() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5316
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d108()（src_rs/evverify.c:5299-5323）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0394 [P2] 数组 b[i] 下标在 run_d109() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5335
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d109()（src_rs/evverify.c:5325-5349）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0395 [P2] 数组 b[i] 下标在 run_d109() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5336
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d109()（src_rs/evverify.c:5325-5349）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0396 [P2] 数组 b[i] 下标在 run_d109() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5342
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d109()（src_rs/evverify.c:5325-5349）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0397 [P2] 数组 b[i] 下标在 run_d110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5360
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d110()（src_rs/evverify.c:5351-5374）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0398 [P2] 数组 b[i] 下标在 run_d110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5361
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d110()（src_rs/evverify.c:5351-5374）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0399 [P2] 数组 b[i] 下标在 run_d110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5367
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d110()（src_rs/evverify.c:5351-5374）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0400 [P2] 数组 b[i] 下标在 run_d112() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5388
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d112()（src_rs/evverify.c:5376-5402）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0401 [P2] 数组 b[i] 下标在 run_d112() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5389
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d112()（src_rs/evverify.c:5376-5402）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0402 [P2] 数组 b[i] 下标在 run_d112() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5395
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d112()（src_rs/evverify.c:5376-5402）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0403 [P2] 数组 b[i] 下标在 run_d113() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5415
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d113()（src_rs/evverify.c:5404-5429）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0404 [P2] 数组 b[i] 下标在 run_d113() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5416
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d113()（src_rs/evverify.c:5404-5429）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0405 [P2] 数组 b[i] 下标在 run_d113() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5422
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d113()（src_rs/evverify.c:5404-5429）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0406 [P2] 数组 b[i] 下标在 run_d114() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5439
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d114()（src_rs/evverify.c:5431-5453）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0407 [P2] 数组 b[i] 下标在 run_d114() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5440
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d114()（src_rs/evverify.c:5431-5453）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0408 [P2] 数组 b[i] 下标在 run_d114() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5446
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d114()（src_rs/evverify.c:5431-5453）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0409 [P2] 数组 b[i] 下标在 run_d118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5464
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d118()（src_rs/evverify.c:5455-5478）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0410 [P2] 数组 b[i] 下标在 run_d118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5465
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d118()（src_rs/evverify.c:5455-5478）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0411 [P2] 数组 b[i] 下标在 run_d118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5471
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d118()（src_rs/evverify.c:5455-5478）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0412 [P2] 数组 b[i] 下标在 run_d119() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5489
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d119()（src_rs/evverify.c:5480-5503）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0413 [P2] 数组 b[i] 下标在 run_d119() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5490
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d119()（src_rs/evverify.c:5480-5503）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0414 [P2] 数组 b[i] 下标在 run_d119() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5496
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d119()（src_rs/evverify.c:5480-5503）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0415 [P2] 数组 b[i] 下标在 run_d120() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5515
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d120()（src_rs/evverify.c:5505-5529）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0416 [P2] 数组 b[i] 下标在 run_d120() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5516
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d120()（src_rs/evverify.c:5505-5529）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0417 [P2] 数组 b[i] 下标在 run_d120() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5522
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d120()（src_rs/evverify.c:5505-5529）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0418 [P2] 数组 b[i] 下标在 run_d121() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5540
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d121()（src_rs/evverify.c:5531-5554）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0419 [P2] 数组 b[i] 下标在 run_d121() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5541
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d121()（src_rs/evverify.c:5531-5554）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0420 [P2] 数组 b[i] 下标在 run_d121() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5547
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d121()（src_rs/evverify.c:5531-5554）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0421 [P2] 数组 b[i] 下标在 run_d122() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5565
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d122()（src_rs/evverify.c:5556-5579）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0422 [P2] 数组 b[i] 下标在 run_d122() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5566
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d122()（src_rs/evverify.c:5556-5579）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0423 [P2] 数组 b[i] 下标在 run_d122() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5572
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d122()（src_rs/evverify.c:5556-5579）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0424 [P2] 数组 b[i] 下标在 run_d123() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5589
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d123()（src_rs/evverify.c:5581-5603）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0425 [P2] 数组 b[i] 下标在 run_d123() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5590
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d123()（src_rs/evverify.c:5581-5603）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0426 [P2] 数组 b[i] 下标在 run_d123() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5596
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d123()（src_rs/evverify.c:5581-5603）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0427 [P2] 数组 b[i] 下标在 run_d246() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5614
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d246()（src_rs/evverify.c:5605-5628）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0428 [P2] 数组 b[i] 下标在 run_d246() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5615
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d246()（src_rs/evverify.c:5605-5628）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0429 [P2] 数组 b[i] 下标在 run_d246() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5621
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d246()（src_rs/evverify.c:5605-5628）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0430 [P2] 数组 b[i] 下标在 run_d247() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5638
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d247()（src_rs/evverify.c:5630-5652）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0431 [P2] 数组 b[i] 下标在 run_d247() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5639
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d247()（src_rs/evverify.c:5630-5652）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0432 [P2] 数组 b[i] 下标在 run_d247() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5645
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d247()（src_rs/evverify.c:5630-5652）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0433 [P2] 数组 b[i] 下标在 run_d248() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5665
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d248()（src_rs/evverify.c:5654-5679）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0434 [P2] 数组 b[i] 下标在 run_d248() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5666
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d248()（src_rs/evverify.c:5654-5679）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0435 [P2] 数组 b[i] 下标在 run_d248() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5672
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d248()（src_rs/evverify.c:5654-5679）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0436 [P2] 数组 b[i] 下标在 run_d249() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5689
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d249()（src_rs/evverify.c:5681-5703）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0437 [P2] 数组 b[i] 下标在 run_d249() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5690
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d249()（src_rs/evverify.c:5681-5703）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0438 [P2] 数组 b[i] 下标在 run_d249() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5696
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d249()（src_rs/evverify.c:5681-5703）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0439 [P2] 数组 b[i] 下标在 run_d250() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5714
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d250()（src_rs/evverify.c:5705-5728）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0440 [P2] 数组 b[i] 下标在 run_d250() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5715
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d250()（src_rs/evverify.c:5705-5728）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0441 [P2] 数组 b[i] 下标在 run_d250() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5721
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d250()（src_rs/evverify.c:5705-5728）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0442 [P2] 数组 b[i] 下标在 run_d251() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5738
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d251()（src_rs/evverify.c:5730-5752）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0443 [P2] 数组 b[i] 下标在 run_d251() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5739
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d251()（src_rs/evverify.c:5730-5752）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0444 [P2] 数组 b[i] 下标在 run_d251() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5745
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d251()（src_rs/evverify.c:5730-5752）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0445 [P2] 数组 b[i] 下标在 run_d253() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5762
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d253()（src_rs/evverify.c:5754-5776）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0446 [P2] 数组 b[i] 下标在 run_d253() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5763
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d253()（src_rs/evverify.c:5754-5776）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0447 [P2] 数组 b[i] 下标在 run_d253() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5769
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d253()（src_rs/evverify.c:5754-5776）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0448 [P2] 数组 b[i] 下标在 run_d255() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5788
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d255()（src_rs/evverify.c:5778-5802）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0449 [P2] 数组 b[i] 下标在 run_d255() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5789
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d255()（src_rs/evverify.c:5778-5802）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0450 [P2] 数组 b[i] 下标在 run_d255() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5795
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d255()（src_rs/evverify.c:5778-5802）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0451 [P2] 数组 b[i] 下标在 run_d256() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5815
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d256()（src_rs/evverify.c:5804-5829）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0452 [P2] 数组 b[i] 下标在 run_d256() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5816
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d256()（src_rs/evverify.c:5804-5829）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0453 [P2] 数组 b[i] 下标在 run_d256() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5822
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d256()（src_rs/evverify.c:5804-5829）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0454 [P2] 数组 b[i] 下标在 run_d257() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5843
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d257()（src_rs/evverify.c:5831-5857）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0455 [P2] 数组 b[i] 下标在 run_d257() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5844
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d257()（src_rs/evverify.c:5831-5857）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0456 [P2] 数组 b[i] 下标在 run_d257() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5850
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d257()（src_rs/evverify.c:5831-5857）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0457 [P2] 数组 b[i] 下标在 run_d258() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5871
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d258()（src_rs/evverify.c:5859-5885）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0458 [P2] 数组 b[i] 下标在 run_d258() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5872
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d258()（src_rs/evverify.c:5859-5885）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0459 [P2] 数组 b[i] 下标在 run_d258() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5878
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d258()（src_rs/evverify.c:5859-5885）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0460 [P2] 数组 b[i] 下标在 run_d259() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5896
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d259()（src_rs/evverify.c:5887-5910）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0461 [P2] 数组 b[i] 下标在 run_d259() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5897
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d259()（src_rs/evverify.c:5887-5910）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0462 [P2] 数组 b[i] 下标在 run_d259() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5903
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d259()（src_rs/evverify.c:5887-5910）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0463 [P2] 数组 b[i] 下标在 run_d260() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5924
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d260()（src_rs/evverify.c:5912-5938）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0464 [P2] 数组 b[i] 下标在 run_d260() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5925
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d260()（src_rs/evverify.c:5912-5938）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0465 [P2] 数组 b[i] 下标在 run_d260() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5931
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d260()（src_rs/evverify.c:5912-5938）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0466 [P2] 数组 b[i] 下标在 run_d261() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5948
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d261()（src_rs/evverify.c:5940-5962）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0467 [P2] 数组 b[i] 下标在 run_d261() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5949
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d261()（src_rs/evverify.c:5940-5962）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0468 [P2] 数组 b[i] 下标在 run_d261() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5955
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d261()（src_rs/evverify.c:5940-5962）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0469 [P2] 数组 b[i] 下标在 run_d262() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5975
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d262()（src_rs/evverify.c:5964-5989）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0470 [P2] 数组 b[i] 下标在 run_d262() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5976
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d262()（src_rs/evverify.c:5964-5989）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0471 [P2] 数组 b[i] 下标在 run_d262() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:5982
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d262()（src_rs/evverify.c:5964-5989）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0472 [P2] 数组 b[i] 下标在 run_d263() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:6003
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d263()（src_rs/evverify.c:5991-6017）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0473 [P2] 数组 b[i] 下标在 run_d263() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:6004
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d263()（src_rs/evverify.c:5991-6017）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0474 [P2] 数组 b[i] 下标在 run_d263() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:6010
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d263()（src_rs/evverify.c:5991-6017）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0475 [P2] 数组 b[i] 下标在 run_d264() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:6027
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d264()（src_rs/evverify.c:6019-6041）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0476 [P2] 数组 b[i] 下标在 run_d264() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:6028
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d264()（src_rs/evverify.c:6019-6041）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0477 [P2] 数组 b[i] 下标在 run_d264() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/evverify.c:6034
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 run_d264()（src_rs/evverify.c:6019-6041）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0478 [P2] 数组 g_dlg_batch_hooks[hi] 下标在 jq_dlg_batch_hook() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:148
- 问题：g_dlg_batch_hooks[hi] 索引，g_dlg_batch_hooks 声明大小 [0]
- 依据：函数 jq_dlg_batch_hook()（src_rs/jni_bridge.c:141-158）体内未对 hi 做范围检查
- 修法：在索引前加 if (hi < 0 || hi >= N) 拒绝并记日志

## AUD-A-0479 [P2] 数组 g_dlg_batch_hooks[hi] 下标在 jq_dlg_batch_hook() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:150
- 问题：g_dlg_batch_hooks[hi] 索引，g_dlg_batch_hooks 声明大小 [0]
- 依据：函数 jq_dlg_batch_hook()（src_rs/jni_bridge.c:141-158）体内未对 hi 做范围检查
- 修法：在索引前加 if (hi < 0 || hi >= N) 拒绝并记日志

## AUD-A-0480 [P2] 数组 g_dlg_batch_hooks[hi] 下标在 jq_dlg_batch_hook() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:153
- 问题：g_dlg_batch_hooks[hi] 索引，g_dlg_batch_hooks 声明大小 [0]
- 依据：函数 jq_dlg_batch_hook()（src_rs/jni_bridge.c:141-158）体内未对 hi 做范围检查
- 修法：在索引前加 if (hi < 0 || hi >= N) 拒绝并记日志

## AUD-A-0481 [P2] 数组 g_dlg_batch_hooks[hi] 下标在 jq_dlg_batch_hook() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:154
- 问题：g_dlg_batch_hooks[hi] 索引，g_dlg_batch_hooks 声明大小 [0]
- 依据：函数 jq_dlg_batch_hook()（src_rs/jni_bridge.c:141-158）体内未对 hi 做范围检查
- 修法：在索引前加 if (hi < 0 || hi >= N) 拒绝并记日志

## AUD-A-0482 [P2] 数组 cells[i] 下标在 jq_restart_game() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:222
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 jq_restart_game()（src_rs/jni_bridge.c:212-232）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0483 [P2] 数组 cells[i] 下标在 jq_restart_game() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:223
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 jq_restart_game()（src_rs/jni_bridge.c:212-232）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0484 [P2] 数组 cells[i] 下标在 ai_pick() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:244
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 ai_pick()（src_rs/jni_bridge.c:238-277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0485 [P2] 数组 cells[i] 下标在 ai_pick() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:270
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 ai_pick()（src_rs/jni_bridge.c:238-277）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0486 [P2] 数组 tb[BT_BUF] 下标在 jq_bt_ctrl_send() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:386
- 问题：tb[BT_BUF] 索引，tb 声明大小 [128]
- 依据：函数 jq_bt_ctrl_send()（src_rs/jni_bridge.c:383-393）体内未对 BT_BUF 做范围检查
- 修法：在索引前加 if (BT_BUF < 0 || BT_BUF >= 128) 拒绝并记日志

## AUD-A-0487 [P2] 数组 dirs[d] 下标在 on_event() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:834
- 问题：dirs[d] 索引，dirs 声明大小 [1]
- 依据：函数 on_event()（src_rs/jni_bridge.c:768-1003）体内未对 d 做范围检查
- 修法：在索引前加 if (d < 0 || d >= 1) 拒绝并记日志

## AUD-A-0488 [P2] 数组 cells[i] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1045
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0489 [P2] 数组 cells[i] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1081
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0490 [P2] 数组 cells[i] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1082
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0491 [P2] 数组 cells[i] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1113
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0492 [P2] 数组 g_event_cnt[ftev] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1120
- 问题：g_event_cnt[ftev] 索引，g_event_cnt 声明大小 [0, 1, 6]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 ftev 做范围检查
- 修法：在索引前加 if (ftev < 0 || ftev >= 6) 拒绝并记日志

## AUD-A-0493 [P2] 数组 cells[i] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1181
- 问题：cells[i] 索引，cells 声明大小 [49, 53, 54, 58, 59]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 59) 拒绝并记日志

## AUD-A-0494 [P2] 数组 g_event_cnt[mtev] 下标在 jq_bt_test_run() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/jni_bridge.c:1189
- 问题：g_event_cnt[mtev] 索引，g_event_cnt 声明大小 [0, 1, 6]
- 依据：函数 jq_bt_test_run()（src_rs/jni_bridge.c:1041-1061）体内未对 mtev 做范围检查
- 修法：在索引前加 if (mtev < 0 || mtev >= 6) 拒绝并记日志

## AUD-A-0495 [P2] 数组 cnt[RANK_COUNT] 下标在 jq_standard_army_count() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:23
- 问题：cnt[RANK_COUNT] 索引，cnt 声明大小 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
- 依据：函数 jq_standard_army_count()（src_rs/layout.c:23-28）体内未对 RANK_COUNT 做范围检查
- 修法：在索引前加 if (RANK_COUNT < 0 || RANK_COUNT >= 12) 拒绝并记日志

## AUD-A-0496 [P2] 数组 layout[i] 下标在 jq_load_layout() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:47
- 问题：layout[i] 索引，layout 声明大小 [30]
- 依据：函数 jq_load_layout()（src_rs/layout.c:43-68）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 30) 拒绝并记日志

## AUD-A-0497 [P2] 数组 cnt[RANK_COUNT] 下标在 jq_validate_layout() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:94
- 问题：cnt[RANK_COUNT] 索引，cnt 声明大小 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
- 依据：函数 jq_validate_layout()（src_rs/layout.c:92-108）体内未对 RANK_COUNT 做范围检查
- 修法：在索引前加 if (RANK_COUNT < 0 || RANK_COUNT >= 12) 拒绝并记日志

## AUD-A-0498 [P2] 数组 out[i] 下标在 jq_extract_layout_30() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/layout.c:115
- 问题：out[i] 索引，out 声明大小 [30]
- 依据：函数 jq_extract_layout_30()（src_rs/layout.c:113-124）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 30) 拒绝并记日志

## AUD-A-0499 [P2] 数组 T_W[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1226
- 问题：T_W[i] 索引，T_W 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1182-1183）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0500 [P2] 数组 T_X[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1229
- 问题：T_X[i] 索引，T_X 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1182-1183）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0501 [P2] 数组 T_Y[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:1230
- 问题：T_Y[i] 索引，T_Y 声明大小 [5]
- 依据：函数 if()（src_rs/render.c:1182-1183）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 5) 拒绝并记日志

## AUD-A-0502 [P2] 数组 lb[i] 下标在 rs_layres_list_rows() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:2642
- 问题：lb[i] 索引，lb 声明大小 [6]
- 依据：函数 rs_layres_list_rows()（src_rs/render.c:2636-2664）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 6) 拒绝并记日志

## AUD-A-0503 [P2] 数组 lb[i] 下标在 rs_layres_list_rows() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:2643
- 问题：lb[i] 索引，lb 声明大小 [6]
- 依据：函数 rs_layres_list_rows()（src_rs/render.c:2636-2664）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 6) 拒绝并记日志

## AUD-A-0504 [P2] 数组 BTN_COL[UI_SAVELAYOUT_COUNT] 下标在 rs_draw_savelay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:2793
- 问题：BTN_COL[UI_SAVELAYOUT_COUNT] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_savelay()（src_rs/render.c:2783-2853）体内未对 UI_SAVELAYOUT_COUNT 做范围检查
- 修法：在索引前加 if (UI_SAVELAYOUT_COUNT < 0 || UI_SAVELAYOUT_COUNT >= 6) 拒绝并记日志

## AUD-A-0505 [P2] 数组 BTN_COL[UI_LAYMGR_COUNT] 下标在 rs_draw_laymgr() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:2868
- 问题：BTN_COL[UI_LAYMGR_COUNT] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_laymgr()（src_rs/render.c:2858-2927）体内未对 UI_LAYMGR_COUNT 做范围检查
- 修法：在索引前加 if (UI_LAYMGR_COUNT < 0 || UI_LAYMGR_COUNT >= 6) 拒绝并记日志

## AUD-A-0506 [P2] 数组 bb[i] 下标在 rs_draw_d000() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:4933
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d000()（src_rs/render.c:4927-4949）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0507 [P2] 数组 bb[i] 下标在 rs_draw_d000() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:4942
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d000()（src_rs/render.c:4927-4949）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0508 [P2] 数组 bb[i] 下标在 rs_draw_d001() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:4964
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d001()（src_rs/render.c:4958-4980）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0509 [P2] 数组 bb[i] 下标在 rs_draw_d001() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:4973
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d001()（src_rs/render.c:4958-4980）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0510 [P2] 数组 bb[i] 下标在 rs_draw_d002() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:4995
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d002()（src_rs/render.c:4989-5011）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0511 [P2] 数组 bb[i] 下标在 rs_draw_d002() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5004
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d002()（src_rs/render.c:4989-5011）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0512 [P2] 数组 bb[i] 下标在 rs_draw_d003() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5026
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d003()（src_rs/render.c:5020-5042）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0513 [P2] 数组 bb[i] 下标在 rs_draw_d003() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5035
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d003()（src_rs/render.c:5020-5042）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0514 [P2] 数组 bb[i] 下标在 rs_draw_d004() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5057
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d004()（src_rs/render.c:5051-5073）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0515 [P2] 数组 bb[i] 下标在 rs_draw_d004() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5066
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d004()（src_rs/render.c:5051-5073）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0516 [P2] 数组 bb[i] 下标在 rs_draw_d005() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5088
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d005()（src_rs/render.c:5082-5104）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0517 [P2] 数组 bb[i] 下标在 rs_draw_d005() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5097
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d005()（src_rs/render.c:5082-5104）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0518 [P2] 数组 bb[i] 下标在 rs_draw_d006() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5119
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d006()（src_rs/render.c:5113-5135）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0519 [P2] 数组 bb[i] 下标在 rs_draw_d006() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5128
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d006()（src_rs/render.c:5113-5135）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0520 [P2] 数组 bb[i] 下标在 rs_draw_d007() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5150
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d007()（src_rs/render.c:5144-5166）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0521 [P2] 数组 bb[i] 下标在 rs_draw_d007() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5159
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d007()（src_rs/render.c:5144-5166）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0522 [P2] 数组 bb[i] 下标在 rs_draw_d008() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5181
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d008()（src_rs/render.c:5175-5197）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0523 [P2] 数组 bb[i] 下标在 rs_draw_d008() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5190
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d008()（src_rs/render.c:5175-5197）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0524 [P2] 数组 bb[i] 下标在 rs_draw_d010() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5212
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d010()（src_rs/render.c:5206-5228）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0525 [P2] 数组 bb[i] 下标在 rs_draw_d010() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5221
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d010()（src_rs/render.c:5206-5228）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0526 [P2] 数组 bb[i] 下标在 rs_draw_d012() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5243
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d012()（src_rs/render.c:5237-5259）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0527 [P2] 数组 bb[i] 下标在 rs_draw_d012() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5252
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d012()（src_rs/render.c:5237-5259）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0528 [P2] 数组 bb[i] 下标在 rs_draw_d013() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5274
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d013()（src_rs/render.c:5268-5290）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0529 [P2] 数组 bb[i] 下标在 rs_draw_d013() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5283
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d013()（src_rs/render.c:5268-5290）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0530 [P2] 数组 bb[i] 下标在 rs_draw_d015() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5305
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d015()（src_rs/render.c:5299-5321）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0531 [P2] 数组 bb[i] 下标在 rs_draw_d015() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5314
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d015()（src_rs/render.c:5299-5321）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0532 [P2] 数组 bb[i] 下标在 rs_draw_d016() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5336
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d016()（src_rs/render.c:5330-5352）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0533 [P2] 数组 bb[i] 下标在 rs_draw_d016() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5345
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d016()（src_rs/render.c:5330-5352）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0534 [P2] 数组 bb[i] 下标在 rs_draw_d017() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5367
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d017()（src_rs/render.c:5361-5383）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0535 [P2] 数组 bb[i] 下标在 rs_draw_d017() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5376
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d017()（src_rs/render.c:5361-5383）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0536 [P2] 数组 bb[i] 下标在 rs_draw_d019() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5398
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d019()（src_rs/render.c:5392-5414）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0537 [P2] 数组 bb[i] 下标在 rs_draw_d019() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5407
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d019()（src_rs/render.c:5392-5414）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0538 [P2] 数组 bb[i] 下标在 rs_draw_d020() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5429
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d020()（src_rs/render.c:5423-5445）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0539 [P2] 数组 bb[i] 下标在 rs_draw_d020() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5438
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d020()（src_rs/render.c:5423-5445）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0540 [P2] 数组 bb[i] 下标在 rs_draw_d021() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5460
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d021()（src_rs/render.c:5454-5476）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0541 [P2] 数组 bb[i] 下标在 rs_draw_d021() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5469
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d021()（src_rs/render.c:5454-5476）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0542 [P2] 数组 bb[i] 下标在 rs_draw_d022() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5491
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d022()（src_rs/render.c:5485-5507）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0543 [P2] 数组 bb[i] 下标在 rs_draw_d022() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5500
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d022()（src_rs/render.c:5485-5507）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0544 [P2] 数组 bb[i] 下标在 rs_draw_d023() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5522
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d023()（src_rs/render.c:5516-5538）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0545 [P2] 数组 bb[i] 下标在 rs_draw_d023() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5531
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d023()（src_rs/render.c:5516-5538）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0546 [P2] 数组 bb[i] 下标在 rs_draw_d024() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5553
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d024()（src_rs/render.c:5547-5569）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0547 [P2] 数组 bb[i] 下标在 rs_draw_d024() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5562
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d024()（src_rs/render.c:5547-5569）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0548 [P2] 数组 bb[i] 下标在 rs_draw_d025() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5584
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d025()（src_rs/render.c:5578-5600）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0549 [P2] 数组 bb[i] 下标在 rs_draw_d025() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5593
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d025()（src_rs/render.c:5578-5600）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0550 [P2] 数组 bb[i] 下标在 rs_draw_d026() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5615
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d026()（src_rs/render.c:5609-5631）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0551 [P2] 数组 bb[i] 下标在 rs_draw_d026() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5624
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d026()（src_rs/render.c:5609-5631）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0552 [P2] 数组 bb[i] 下标在 rs_draw_d027() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5646
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d027()（src_rs/render.c:5640-5662）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0553 [P2] 数组 bb[i] 下标在 rs_draw_d027() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5655
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d027()（src_rs/render.c:5640-5662）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0554 [P2] 数组 bb[i] 下标在 rs_draw_d028() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5677
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d028()（src_rs/render.c:5671-5693）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0555 [P2] 数组 bb[i] 下标在 rs_draw_d028() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5686
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d028()（src_rs/render.c:5671-5693）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0556 [P2] 数组 bb[i] 下标在 rs_draw_d029() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5708
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d029()（src_rs/render.c:5702-5724）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0557 [P2] 数组 bb[i] 下标在 rs_draw_d029() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5717
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d029()（src_rs/render.c:5702-5724）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0558 [P2] 数组 bb[i] 下标在 rs_draw_d030() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5739
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d030()（src_rs/render.c:5733-5755）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0559 [P2] 数组 bb[i] 下标在 rs_draw_d030() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5748
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d030()（src_rs/render.c:5733-5755）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0560 [P2] 数组 bb[i] 下标在 rs_draw_d031() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5770
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d031()（src_rs/render.c:5764-5786）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0561 [P2] 数组 bb[i] 下标在 rs_draw_d031() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5779
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d031()（src_rs/render.c:5764-5786）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0562 [P2] 数组 bb[i] 下标在 rs_draw_d032() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5801
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d032()（src_rs/render.c:5795-5817）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0563 [P2] 数组 bb[i] 下标在 rs_draw_d032() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5810
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d032()（src_rs/render.c:5795-5817）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0564 [P2] 数组 bb[i] 下标在 rs_draw_d033() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5832
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d033()（src_rs/render.c:5826-5848）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0565 [P2] 数组 bb[i] 下标在 rs_draw_d033() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5841
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d033()（src_rs/render.c:5826-5848）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0566 [P2] 数组 bb[i] 下标在 rs_draw_d034() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5863
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d034()（src_rs/render.c:5857-5879）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0567 [P2] 数组 bb[i] 下标在 rs_draw_d034() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5872
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d034()（src_rs/render.c:5857-5879）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0568 [P2] 数组 bb[i] 下标在 rs_draw_d035() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5894
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d035()（src_rs/render.c:5888-5910）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0569 [P2] 数组 bb[i] 下标在 rs_draw_d035() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5903
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d035()（src_rs/render.c:5888-5910）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0570 [P2] 数组 bb[i] 下标在 rs_draw_d036() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5925
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d036()（src_rs/render.c:5919-5941）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0571 [P2] 数组 bb[i] 下标在 rs_draw_d036() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5934
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d036()（src_rs/render.c:5919-5941）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0572 [P2] 数组 bb[i] 下标在 rs_draw_d037() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5956
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d037()（src_rs/render.c:5950-5972）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0573 [P2] 数组 bb[i] 下标在 rs_draw_d037() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5965
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d037()（src_rs/render.c:5950-5972）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0574 [P2] 数组 bb[i] 下标在 rs_draw_d038() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5987
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d038()（src_rs/render.c:5981-6003）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0575 [P2] 数组 bb[i] 下标在 rs_draw_d038() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:5996
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d038()（src_rs/render.c:5981-6003）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0576 [P2] 数组 bb[i] 下标在 rs_draw_d039() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6018
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d039()（src_rs/render.c:6012-6034）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0577 [P2] 数组 bb[i] 下标在 rs_draw_d039() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6027
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d039()（src_rs/render.c:6012-6034）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0578 [P2] 数组 bb[i] 下标在 rs_draw_d041() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6049
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d041()（src_rs/render.c:6043-6065）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0579 [P2] 数组 bb[i] 下标在 rs_draw_d041() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6058
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d041()（src_rs/render.c:6043-6065）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0580 [P2] 数组 bb[i] 下标在 rs_draw_d042() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6080
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d042()（src_rs/render.c:6074-6096）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0581 [P2] 数组 bb[i] 下标在 rs_draw_d042() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6089
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d042()（src_rs/render.c:6074-6096）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0582 [P2] 数组 bb[i] 下标在 rs_draw_d043() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6111
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d043()（src_rs/render.c:6105-6127）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0583 [P2] 数组 bb[i] 下标在 rs_draw_d043() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6120
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d043()（src_rs/render.c:6105-6127）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0584 [P2] 数组 bb[i] 下标在 rs_draw_d044() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6142
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d044()（src_rs/render.c:6136-6158）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0585 [P2] 数组 bb[i] 下标在 rs_draw_d044() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6151
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d044()（src_rs/render.c:6136-6158）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0586 [P2] 数组 bb[i] 下标在 rs_draw_d045() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6173
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d045()（src_rs/render.c:6167-6189）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0587 [P2] 数组 bb[i] 下标在 rs_draw_d045() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6182
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d045()（src_rs/render.c:6167-6189）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0588 [P2] 数组 bb[i] 下标在 rs_draw_d046() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6204
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d046()（src_rs/render.c:6198-6220）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0589 [P2] 数组 bb[i] 下标在 rs_draw_d046() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6213
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d046()（src_rs/render.c:6198-6220）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0590 [P2] 数组 bb[i] 下标在 rs_draw_d047() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6235
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d047()（src_rs/render.c:6229-6251）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0591 [P2] 数组 bb[i] 下标在 rs_draw_d047() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6244
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d047()（src_rs/render.c:6229-6251）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0592 [P2] 数组 bb[i] 下标在 rs_draw_d049() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6266
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d049()（src_rs/render.c:6260-6282）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0593 [P2] 数组 bb[i] 下标在 rs_draw_d049() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6275
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d049()（src_rs/render.c:6260-6282）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0594 [P2] 数组 bb[i] 下标在 rs_draw_d050() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6297
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d050()（src_rs/render.c:6291-6313）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0595 [P2] 数组 bb[i] 下标在 rs_draw_d050() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6306
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d050()（src_rs/render.c:6291-6313）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0596 [P2] 数组 bb[i] 下标在 rs_draw_d051() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6328
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d051()（src_rs/render.c:6322-6344）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0597 [P2] 数组 bb[i] 下标在 rs_draw_d051() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6337
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d051()（src_rs/render.c:6322-6344）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0598 [P2] 数组 bb[i] 下标在 rs_draw_d052() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6359
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d052()（src_rs/render.c:6353-6375）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0599 [P2] 数组 bb[i] 下标在 rs_draw_d052() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6368
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d052()（src_rs/render.c:6353-6375）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0600 [P2] 数组 bb[i] 下标在 rs_draw_d053() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6390
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d053()（src_rs/render.c:6384-6406）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0601 [P2] 数组 bb[i] 下标在 rs_draw_d053() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6399
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d053()（src_rs/render.c:6384-6406）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0602 [P2] 数组 bb[i] 下标在 rs_draw_d054() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6421
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d054()（src_rs/render.c:6415-6437）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0603 [P2] 数组 bb[i] 下标在 rs_draw_d054() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6430
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d054()（src_rs/render.c:6415-6437）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0604 [P2] 数组 bb[i] 下标在 rs_draw_d055() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6452
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d055()（src_rs/render.c:6446-6468）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0605 [P2] 数组 bb[i] 下标在 rs_draw_d055() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6461
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d055()（src_rs/render.c:6446-6468）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0606 [P2] 数组 bb[i] 下标在 rs_draw_d057() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6483
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d057()（src_rs/render.c:6477-6499）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0607 [P2] 数组 bb[i] 下标在 rs_draw_d057() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6492
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d057()（src_rs/render.c:6477-6499）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0608 [P2] 数组 bb[i] 下标在 rs_draw_d058() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6514
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d058()（src_rs/render.c:6508-6530）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0609 [P2] 数组 bb[i] 下标在 rs_draw_d058() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6523
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d058()（src_rs/render.c:6508-6530）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0610 [P2] 数组 bb[i] 下标在 rs_draw_d059() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6545
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d059()（src_rs/render.c:6539-6561）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0611 [P2] 数组 bb[i] 下标在 rs_draw_d059() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6554
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d059()（src_rs/render.c:6539-6561）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0612 [P2] 数组 bb[i] 下标在 rs_draw_d060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6576
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d060()（src_rs/render.c:6570-6592）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0613 [P2] 数组 bb[i] 下标在 rs_draw_d060() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6585
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d060()（src_rs/render.c:6570-6592）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0614 [P2] 数组 bb[i] 下标在 rs_draw_d061() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6607
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d061()（src_rs/render.c:6601-6623）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0615 [P2] 数组 bb[i] 下标在 rs_draw_d061() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6616
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d061()（src_rs/render.c:6601-6623）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0616 [P2] 数组 bb[i] 下标在 rs_draw_d062() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6638
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d062()（src_rs/render.c:6632-6654）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0617 [P2] 数组 bb[i] 下标在 rs_draw_d062() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6647
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d062()（src_rs/render.c:6632-6654）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0618 [P2] 数组 bb[i] 下标在 rs_draw_d063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6669
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d063()（src_rs/render.c:6663-6685）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0619 [P2] 数组 bb[i] 下标在 rs_draw_d063() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6678
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d063()（src_rs/render.c:6663-6685）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0620 [P2] 数组 bb[i] 下标在 rs_draw_d064() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6700
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d064()（src_rs/render.c:6694-6716）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0621 [P2] 数组 bb[i] 下标在 rs_draw_d064() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6709
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d064()（src_rs/render.c:6694-6716）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0622 [P2] 数组 bb[i] 下标在 rs_draw_d065() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6731
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d065()（src_rs/render.c:6725-6747）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0623 [P2] 数组 bb[i] 下标在 rs_draw_d065() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6740
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d065()（src_rs/render.c:6725-6747）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0624 [P2] 数组 bb[i] 下标在 rs_draw_d066() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6762
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d066()（src_rs/render.c:6756-6778）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0625 [P2] 数组 bb[i] 下标在 rs_draw_d066() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6771
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d066()（src_rs/render.c:6756-6778）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0626 [P2] 数组 bb[i] 下标在 rs_draw_d067() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6793
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d067()（src_rs/render.c:6787-6809）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0627 [P2] 数组 bb[i] 下标在 rs_draw_d067() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6802
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d067()（src_rs/render.c:6787-6809）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0628 [P2] 数组 bb[i] 下标在 rs_draw_d068() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6824
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d068()（src_rs/render.c:6818-6840）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0629 [P2] 数组 bb[i] 下标在 rs_draw_d068() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6833
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d068()（src_rs/render.c:6818-6840）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0630 [P2] 数组 bb[i] 下标在 rs_draw_d069() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6855
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d069()（src_rs/render.c:6849-6871）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0631 [P2] 数组 bb[i] 下标在 rs_draw_d069() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6864
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d069()（src_rs/render.c:6849-6871）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0632 [P2] 数组 bb[i] 下标在 rs_draw_d070() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6886
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d070()（src_rs/render.c:6880-6902）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0633 [P2] 数组 bb[i] 下标在 rs_draw_d070() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6895
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d070()（src_rs/render.c:6880-6902）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0634 [P2] 数组 bb[i] 下标在 rs_draw_d071() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6917
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d071()（src_rs/render.c:6911-6933）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0635 [P2] 数组 bb[i] 下标在 rs_draw_d071() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6926
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d071()（src_rs/render.c:6911-6933）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0636 [P2] 数组 bb[i] 下标在 rs_draw_d072() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6948
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d072()（src_rs/render.c:6942-6964）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0637 [P2] 数组 bb[i] 下标在 rs_draw_d072() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6957
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d072()（src_rs/render.c:6942-6964）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0638 [P2] 数组 bb[i] 下标在 rs_draw_d073() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6979
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d073()（src_rs/render.c:6973-6995）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0639 [P2] 数组 bb[i] 下标在 rs_draw_d073() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:6988
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d073()（src_rs/render.c:6973-6995）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0640 [P2] 数组 bb[i] 下标在 rs_draw_d075() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7010
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d075()（src_rs/render.c:7004-7026）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0641 [P2] 数组 bb[i] 下标在 rs_draw_d075() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7019
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d075()（src_rs/render.c:7004-7026）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0642 [P2] 数组 bb[i] 下标在 rs_draw_d077() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7041
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d077()（src_rs/render.c:7035-7057）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0643 [P2] 数组 bb[i] 下标在 rs_draw_d077() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7050
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d077()（src_rs/render.c:7035-7057）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0644 [P2] 数组 bb[i] 下标在 rs_draw_d078() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7072
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d078()（src_rs/render.c:7066-7088）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0645 [P2] 数组 bb[i] 下标在 rs_draw_d078() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7081
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d078()（src_rs/render.c:7066-7088）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0646 [P2] 数组 bb[i] 下标在 rs_draw_d079() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7103
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d079()（src_rs/render.c:7097-7119）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0647 [P2] 数组 bb[i] 下标在 rs_draw_d079() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7112
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d079()（src_rs/render.c:7097-7119）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0648 [P2] 数组 bb[i] 下标在 rs_draw_d082() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7134
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d082()（src_rs/render.c:7128-7150）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0649 [P2] 数组 bb[i] 下标在 rs_draw_d082() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7143
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d082()（src_rs/render.c:7128-7150）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0650 [P2] 数组 bb[i] 下标在 rs_draw_d083() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7165
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d083()（src_rs/render.c:7159-7181）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0651 [P2] 数组 bb[i] 下标在 rs_draw_d083() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7174
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d083()（src_rs/render.c:7159-7181）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0652 [P2] 数组 bb[i] 下标在 rs_draw_d084() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7196
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d084()（src_rs/render.c:7190-7212）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0653 [P2] 数组 bb[i] 下标在 rs_draw_d084() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7205
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d084()（src_rs/render.c:7190-7212）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0654 [P2] 数组 bb[i] 下标在 rs_draw_d085() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7227
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d085()（src_rs/render.c:7221-7243）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0655 [P2] 数组 bb[i] 下标在 rs_draw_d085() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7236
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d085()（src_rs/render.c:7221-7243）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0656 [P2] 数组 bb[i] 下标在 rs_draw_d086() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7258
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d086()（src_rs/render.c:7252-7274）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0657 [P2] 数组 bb[i] 下标在 rs_draw_d086() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7267
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d086()（src_rs/render.c:7252-7274）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0658 [P2] 数组 bb[i] 下标在 rs_draw_d089() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7289
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d089()（src_rs/render.c:7283-7305）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0659 [P2] 数组 bb[i] 下标在 rs_draw_d089() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7298
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d089()（src_rs/render.c:7283-7305）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0660 [P2] 数组 bb[i] 下标在 rs_draw_d090() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7320
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d090()（src_rs/render.c:7314-7336）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0661 [P2] 数组 bb[i] 下标在 rs_draw_d090() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7329
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d090()（src_rs/render.c:7314-7336）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0662 [P2] 数组 bb[i] 下标在 rs_draw_d094() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7351
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d094()（src_rs/render.c:7345-7367）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0663 [P2] 数组 bb[i] 下标在 rs_draw_d094() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7360
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d094()（src_rs/render.c:7345-7367）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0664 [P2] 数组 bb[i] 下标在 rs_draw_d095() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7382
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d095()（src_rs/render.c:7376-7398）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0665 [P2] 数组 bb[i] 下标在 rs_draw_d095() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7391
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d095()（src_rs/render.c:7376-7398）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0666 [P2] 数组 bb[i] 下标在 rs_draw_d096() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7413
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d096()（src_rs/render.c:7407-7429）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0667 [P2] 数组 bb[i] 下标在 rs_draw_d096() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7422
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d096()（src_rs/render.c:7407-7429）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0668 [P2] 数组 bb[i] 下标在 rs_draw_d097() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7444
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d097()（src_rs/render.c:7438-7460）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0669 [P2] 数组 bb[i] 下标在 rs_draw_d097() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7453
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d097()（src_rs/render.c:7438-7460）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0670 [P2] 数组 bb[i] 下标在 rs_draw_d098() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7475
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d098()（src_rs/render.c:7469-7491）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0671 [P2] 数组 bb[i] 下标在 rs_draw_d098() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7484
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d098()（src_rs/render.c:7469-7491）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0672 [P2] 数组 bb[i] 下标在 rs_draw_d099() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7506
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d099()（src_rs/render.c:7500-7522）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0673 [P2] 数组 bb[i] 下标在 rs_draw_d099() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7515
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d099()（src_rs/render.c:7500-7522）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0674 [P2] 数组 bb[i] 下标在 rs_draw_d100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7537
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d100()（src_rs/render.c:7531-7553）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0675 [P2] 数组 bb[i] 下标在 rs_draw_d100() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7546
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d100()（src_rs/render.c:7531-7553）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0676 [P2] 数组 bb[i] 下标在 rs_draw_d101() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7568
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d101()（src_rs/render.c:7562-7584）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0677 [P2] 数组 bb[i] 下标在 rs_draw_d101() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7577
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d101()（src_rs/render.c:7562-7584）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0678 [P2] 数组 bb[i] 下标在 rs_draw_d102() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7599
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d102()（src_rs/render.c:7593-7615）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0679 [P2] 数组 bb[i] 下标在 rs_draw_d102() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7608
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d102()（src_rs/render.c:7593-7615）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0680 [P2] 数组 bb[i] 下标在 rs_draw_d104() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7630
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d104()（src_rs/render.c:7624-7646）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0681 [P2] 数组 bb[i] 下标在 rs_draw_d104() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7639
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d104()（src_rs/render.c:7624-7646）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0682 [P2] 数组 bb[i] 下标在 rs_draw_d107() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7661
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d107()（src_rs/render.c:7655-7677）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0683 [P2] 数组 bb[i] 下标在 rs_draw_d107() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7670
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d107()（src_rs/render.c:7655-7677）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0684 [P2] 数组 bb[i] 下标在 rs_draw_d108() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7692
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d108()（src_rs/render.c:7686-7708）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0685 [P2] 数组 bb[i] 下标在 rs_draw_d108() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7701
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d108()（src_rs/render.c:7686-7708）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0686 [P2] 数组 bb[i] 下标在 rs_draw_d109() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7723
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d109()（src_rs/render.c:7717-7739）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0687 [P2] 数组 bb[i] 下标在 rs_draw_d109() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7732
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d109()（src_rs/render.c:7717-7739）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0688 [P2] 数组 bb[i] 下标在 rs_draw_d110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7754
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d110()（src_rs/render.c:7748-7770）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0689 [P2] 数组 bb[i] 下标在 rs_draw_d110() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7763
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d110()（src_rs/render.c:7748-7770）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0690 [P2] 数组 bb[i] 下标在 rs_draw_d112() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7785
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d112()（src_rs/render.c:7779-7801）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0691 [P2] 数组 bb[i] 下标在 rs_draw_d112() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7794
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d112()（src_rs/render.c:7779-7801）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0692 [P2] 数组 bb[i] 下标在 rs_draw_d113() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7816
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d113()（src_rs/render.c:7810-7832）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0693 [P2] 数组 bb[i] 下标在 rs_draw_d113() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7825
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d113()（src_rs/render.c:7810-7832）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0694 [P2] 数组 bb[i] 下标在 rs_draw_d114() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7847
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d114()（src_rs/render.c:7841-7863）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0695 [P2] 数组 bb[i] 下标在 rs_draw_d114() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7856
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d114()（src_rs/render.c:7841-7863）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0696 [P2] 数组 bb[i] 下标在 rs_draw_d118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7878
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d118()（src_rs/render.c:7872-7894）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0697 [P2] 数组 bb[i] 下标在 rs_draw_d118() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7887
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d118()（src_rs/render.c:7872-7894）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0698 [P2] 数组 bb[i] 下标在 rs_draw_d119() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7909
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d119()（src_rs/render.c:7903-7925）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0699 [P2] 数组 bb[i] 下标在 rs_draw_d119() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7918
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d119()（src_rs/render.c:7903-7925）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0700 [P2] 数组 bb[i] 下标在 rs_draw_d120() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7940
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d120()（src_rs/render.c:7934-7956）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0701 [P2] 数组 bb[i] 下标在 rs_draw_d120() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7949
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d120()（src_rs/render.c:7934-7956）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0702 [P2] 数组 bb[i] 下标在 rs_draw_d121() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7971
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d121()（src_rs/render.c:7965-7987）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0703 [P2] 数组 bb[i] 下标在 rs_draw_d121() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:7980
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d121()（src_rs/render.c:7965-7987）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0704 [P2] 数组 bb[i] 下标在 rs_draw_d122() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8002
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d122()（src_rs/render.c:7996-8018）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0705 [P2] 数组 bb[i] 下标在 rs_draw_d122() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8011
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d122()（src_rs/render.c:7996-8018）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0706 [P2] 数组 bb[i] 下标在 rs_draw_d123() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8033
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d123()（src_rs/render.c:8027-8049）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0707 [P2] 数组 bb[i] 下标在 rs_draw_d123() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8042
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d123()（src_rs/render.c:8027-8049）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0708 [P2] 数组 bb[i] 下标在 rs_draw_d246() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8064
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d246()（src_rs/render.c:8058-8080）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0709 [P2] 数组 bb[i] 下标在 rs_draw_d246() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8073
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d246()（src_rs/render.c:8058-8080）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0710 [P2] 数组 bb[i] 下标在 rs_draw_d247() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8095
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d247()（src_rs/render.c:8089-8111）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0711 [P2] 数组 bb[i] 下标在 rs_draw_d247() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8104
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d247()（src_rs/render.c:8089-8111）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0712 [P2] 数组 bb[i] 下标在 rs_draw_d248() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8126
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d248()（src_rs/render.c:8120-8142）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0713 [P2] 数组 bb[i] 下标在 rs_draw_d248() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8135
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d248()（src_rs/render.c:8120-8142）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0714 [P2] 数组 bb[i] 下标在 rs_draw_d249() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8157
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d249()（src_rs/render.c:8151-8173）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0715 [P2] 数组 bb[i] 下标在 rs_draw_d249() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8166
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d249()（src_rs/render.c:8151-8173）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0716 [P2] 数组 bb[i] 下标在 rs_draw_d250() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8188
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d250()（src_rs/render.c:8182-8204）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0717 [P2] 数组 bb[i] 下标在 rs_draw_d250() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8197
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d250()（src_rs/render.c:8182-8204）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0718 [P2] 数组 bb[i] 下标在 rs_draw_d251() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8219
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d251()（src_rs/render.c:8213-8235）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0719 [P2] 数组 bb[i] 下标在 rs_draw_d251() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8228
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d251()（src_rs/render.c:8213-8235）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0720 [P2] 数组 bb[i] 下标在 rs_draw_d253() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8250
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d253()（src_rs/render.c:8244-8266）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0721 [P2] 数组 bb[i] 下标在 rs_draw_d253() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8259
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d253()（src_rs/render.c:8244-8266）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0722 [P2] 数组 bb[i] 下标在 rs_draw_d255() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8281
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d255()（src_rs/render.c:8275-8297）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0723 [P2] 数组 bb[i] 下标在 rs_draw_d255() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8290
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d255()（src_rs/render.c:8275-8297）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0724 [P2] 数组 bb[i] 下标在 rs_draw_d256() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8312
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d256()（src_rs/render.c:8306-8328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0725 [P2] 数组 bb[i] 下标在 rs_draw_d256() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8321
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d256()（src_rs/render.c:8306-8328）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0726 [P2] 数组 bb[i] 下标在 rs_draw_d257() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8343
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d257()（src_rs/render.c:8337-8359）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0727 [P2] 数组 bb[i] 下标在 rs_draw_d257() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8352
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d257()（src_rs/render.c:8337-8359）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0728 [P2] 数组 bb[i] 下标在 rs_draw_d258() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8374
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d258()（src_rs/render.c:8368-8390）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0729 [P2] 数组 bb[i] 下标在 rs_draw_d258() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8383
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d258()（src_rs/render.c:8368-8390）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0730 [P2] 数组 bb[i] 下标在 rs_draw_d259() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8405
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d259()（src_rs/render.c:8399-8421）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0731 [P2] 数组 bb[i] 下标在 rs_draw_d259() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8414
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d259()（src_rs/render.c:8399-8421）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0732 [P2] 数组 bb[i] 下标在 rs_draw_d260() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8436
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d260()（src_rs/render.c:8430-8452）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0733 [P2] 数组 bb[i] 下标在 rs_draw_d260() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8445
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d260()（src_rs/render.c:8430-8452）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0734 [P2] 数组 bb[i] 下标在 rs_draw_d261() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8467
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d261()（src_rs/render.c:8461-8483）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0735 [P2] 数组 bb[i] 下标在 rs_draw_d261() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8476
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d261()（src_rs/render.c:8461-8483）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0736 [P2] 数组 bb[i] 下标在 rs_draw_d262() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8498
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d262()（src_rs/render.c:8492-8514）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0737 [P2] 数组 bb[i] 下标在 rs_draw_d262() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8507
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d262()（src_rs/render.c:8492-8514）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0738 [P2] 数组 bb[i] 下标在 rs_draw_d263() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8529
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d263()（src_rs/render.c:8523-8545）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0739 [P2] 数组 bb[i] 下标在 rs_draw_d263() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8538
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d263()（src_rs/render.c:8523-8545）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0740 [P2] 数组 bb[i] 下标在 rs_draw_d264() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8560
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d264()（src_rs/render.c:8554-8576）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0741 [P2] 数组 bb[i] 下标在 rs_draw_d264() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8569
- 问题：bb[i] 索引，bb 声明大小 [0]
- 依据：函数 rs_draw_d264()（src_rs/render.c:8554-8576）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= N) 拒绝并记日志

## AUD-A-0742 [P2] 数组 BTN_COL[i] 下标在 rs_draw_d264() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8613
- 问题：BTN_COL[i] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_d264()（src_rs/render.c:8554-8576）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 6) 拒绝并记日志

## AUD-A-0743 [P2] 数组 b[i] 下标在 rs_draw_ui44() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8657
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 rs_draw_ui44()（src_rs/render.c:8646-8670）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0744 [P2] 数组 BTN_COL[i] 下标在 rs_draw_ui44() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8658
- 问题：BTN_COL[i] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_ui44()（src_rs/render.c:8646-8670）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 6) 拒绝并记日志

## AUD-A-0745 [P2] 数组 b[i] 下标在 rs_draw_ui44() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8661
- 问题：b[i] 索引，b 声明大小 [0, 1]
- 依据：函数 rs_draw_ui44()（src_rs/render.c:8646-8670）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 1) 拒绝并记日志

## AUD-A-0746 [P2] 数组 bx[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8801
- 问题：bx[i] 索引，bx 声明大小 [0, 1, 2]
- 依据：函数 if()（src_rs/render.c:8796-8797）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0747 [P2] 数组 BTN_COL[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8802
- 问题：BTN_COL[i] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 if()（src_rs/render.c:8796-8797）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 6) 拒绝并记日志

## AUD-A-0748 [P2] 数组 bx[i] 下标在 if() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8805
- 问题：bx[i] 索引，bx 声明大小 [0, 1, 2]
- 依据：函数 if()（src_rs/render.c:8796-8797）体内未对 i 做范围检查
- 修法：在索引前加 if (i < 0 || i >= 2) 拒绝并记日志

## AUD-A-0749 [P2] 数组 BTN_COL[UI_REPLAY_CTRL_COUNT] 下标在 rs_draw_replay() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:8995
- 问题：BTN_COL[UI_REPLAY_CTRL_COUNT] 索引，BTN_COL 声明大小 [2, 3, 4, 5, 6]
- 依据：函数 rs_draw_replay()（src_rs/render.c:8946-9040）体内未对 UI_REPLAY_CTRL_COUNT 做范围检查
- 修法：在索引前加 if (UI_REPLAY_CTRL_COUNT < 0 || UI_REPLAY_CTRL_COUNT >= 6) 拒绝并记日志

## AUD-A-0750 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9055
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0751 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9061
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0752 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9066
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0753 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9076
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0754 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9085
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0755 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9094
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0756 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9103
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0757 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9112
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0758 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9121
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0759 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9130
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0760 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9139
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0761 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9147
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0762 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9156
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0763 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9164
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0764 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9172
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0765 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9180
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0766 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9188
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0767 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9196
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0768 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9204
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0769 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9212
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0770 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9220
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0771 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9228
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0772 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9236
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0773 [P2] 数组 gl2_ok[slot] 下标在 rs_gl_rebuild() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/render.c:9244
- 问题：gl2_ok[slot] 索引，gl2_ok 声明大小 [0, 1]
- 依据：函数 rs_gl_rebuild()（src_rs/render.c:9050-9449）体内未对 slot 做范围检查
- 修法：在索引前加 if (slot < 0 || slot >= 1) 拒绝并记日志

## AUD-A-0774 [P2] 数组 qr[tail] 下标在 rail_path_engineer() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/rules.c:54
- 问题：qr[tail] 索引，qr 声明大小 [64]
- 依据：函数 rail_path_engineer()（src_rs/rules.c:47-71）体内未对 tail 做范围检查
- 修法：在索引前加 if (tail < 0 || tail >= 64) 拒绝并记日志

## AUD-A-0775 [P2] 数组 dr[k] 下标在 rail_path_engineer() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/rules.c:59
- 问题：dr[k] 索引，dr 声明大小 [4]
- 依据：函数 rail_path_engineer()（src_rs/rules.c:47-71）体内未对 k 做范围检查
- 修法：在索引前加 if (k < 0 || k >= 4) 拒绝并记日志

## AUD-A-0776 [P2] 数组 qr[tail] 下标在 rail_path_engineer() 内无范围校验
- 探针：`array-index`
- 位置：src_rs/rules.c:67
- 问题：qr[tail] 索引，qr 声明大小 [64]
- 依据：函数 rail_path_engineer()（src_rs/rules.c:47-71）体内未对 tail 做范围检查
- 修法：在索引前加 if (tail < 0 || tail >= 64) 拒绝并记日志

## AUD-A-0777 [P2] 声明「蓝牙 v2 7 字段」的实现符号 bt_send_frame 不存在
- 探针：`claim`
- 位置：src_rs/jni_bridge.c
- 问题：找不到 bt_send_frame
- 依据：声明与实现不符
- 修法：补实现或修正文档声明

## AUD-A-0778 [P2] 原版 datah_009 未生成对应 UI_D009
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_009.bin 但无 UI_D009
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0779 [P2] 原版 datah_011 未生成对应 UI_D011
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_011.bin 但无 UI_D011
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0780 [P2] 原版 datah_014 未生成对应 UI_D014
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_014.bin 但无 UI_D014
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0781 [P2] 原版 datah_018 未生成对应 UI_D018
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_018.bin 但无 UI_D018
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0782 [P2] 原版 datah_040 未生成对应 UI_D040
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_040.bin 但无 UI_D040
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0783 [P2] 原版 datah_048 未生成对应 UI_D048
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_048.bin 但无 UI_D048
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0784 [P2] 原版 datah_056 未生成对应 UI_D056
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_056.bin 但无 UI_D056
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0785 [P2] 原版 datah_074 未生成对应 UI_D074
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_074.bin 但无 UI_D074
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0786 [P2] 原版 datah_076 未生成对应 UI_D076
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_076.bin 但无 UI_D076
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0787 [P2] 原版 datah_080 未生成对应 UI_D080
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_080.bin 但无 UI_D080
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0788 [P2] 原版 datah_081 未生成对应 UI_D081
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_081.bin 但无 UI_D081
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0789 [P2] 原版 datah_087 未生成对应 UI_D087
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_087.bin 但无 UI_D087
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0790 [P2] 原版 datah_088 未生成对应 UI_D088
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_088.bin 但无 UI_D088
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0791 [P2] 原版 datah_091 未生成对应 UI_D091
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_091.bin 但无 UI_D091
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0792 [P2] 原版 datah_092 未生成对应 UI_D092
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_092.bin 但无 UI_D092
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0793 [P2] 原版 datah_093 未生成对应 UI_D093
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_093.bin 但无 UI_D093
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0794 [P2] 原版 datah_103 未生成对应 UI_D103
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_103.bin 但无 UI_D103
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0795 [P2] 原版 datah_105 未生成对应 UI_D105
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_105.bin 但无 UI_D105
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0796 [P2] 原版 datah_106 未生成对应 UI_D106
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_106.bin 但无 UI_D106
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0797 [P2] 原版 datah_111 未生成对应 UI_D111
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_111.bin 但无 UI_D111
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0798 [P2] 原版 datah_115 未生成对应 UI_D115
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_115.bin 但无 UI_D115
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0799 [P2] 原版 datah_116 未生成对应 UI_D116
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_116.bin 但无 UI_D116
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0800 [P2] 原版 datah_117 未生成对应 UI_D117
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_117.bin 但无 UI_D117
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0801 [P2] 原版 datah_124 未生成对应 UI_D124
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_124.bin 但无 UI_D124
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0802 [P2] 原版 datah_125 未生成对应 UI_D125
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_125.bin 但无 UI_D125
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0803 [P2] 原版 datah_126 未生成对应 UI_D126
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_126.bin 但无 UI_D126
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0804 [P2] 原版 datah_127 未生成对应 UI_D127
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_127.bin 但无 UI_D127
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0805 [P2] 原版 datah_128 未生成对应 UI_D128
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_128.bin 但无 UI_D128
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0806 [P2] 原版 datah_129 未生成对应 UI_D129
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_129.bin 但无 UI_D129
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0807 [P2] 原版 datah_130 未生成对应 UI_D130
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_130.bin 但无 UI_D130
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0808 [P2] 原版 datah_131 未生成对应 UI_D131
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_131.bin 但无 UI_D131
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0809 [P2] 原版 datah_132 未生成对应 UI_D132
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_132.bin 但无 UI_D132
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0810 [P2] 原版 datah_133 未生成对应 UI_D133
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_133.bin 但无 UI_D133
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0811 [P2] 原版 datah_134 未生成对应 UI_D134
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_134.bin 但无 UI_D134
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0812 [P2] 原版 datah_135 未生成对应 UI_D135
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_135.bin 但无 UI_D135
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0813 [P2] 原版 datah_136 未生成对应 UI_D136
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_136.bin 但无 UI_D136
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0814 [P2] 原版 datah_137 未生成对应 UI_D137
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_137.bin 但无 UI_D137
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0815 [P2] 原版 datah_138 未生成对应 UI_D138
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_138.bin 但无 UI_D138
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0816 [P2] 原版 datah_139 未生成对应 UI_D139
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_139.bin 但无 UI_D139
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0817 [P2] 原版 datah_140 未生成对应 UI_D140
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_140.bin 但无 UI_D140
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0818 [P2] 原版 datah_141 未生成对应 UI_D141
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_141.bin 但无 UI_D141
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0819 [P2] 原版 datah_142 未生成对应 UI_D142
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_142.bin 但无 UI_D142
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0820 [P2] 原版 datah_143 未生成对应 UI_D143
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_143.bin 但无 UI_D143
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0821 [P2] 原版 datah_144 未生成对应 UI_D144
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_144.bin 但无 UI_D144
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0822 [P2] 原版 datah_145 未生成对应 UI_D145
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_145.bin 但无 UI_D145
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0823 [P2] 原版 datah_146 未生成对应 UI_D146
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_146.bin 但无 UI_D146
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0824 [P2] 原版 datah_147 未生成对应 UI_D147
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_147.bin 但无 UI_D147
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0825 [P2] 原版 datah_148 未生成对应 UI_D148
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_148.bin 但无 UI_D148
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0826 [P2] 原版 datah_149 未生成对应 UI_D149
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_149.bin 但无 UI_D149
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0827 [P2] 原版 datah_150 未生成对应 UI_D150
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_150.bin 但无 UI_D150
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0828 [P2] 原版 datah_151 未生成对应 UI_D151
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_151.bin 但无 UI_D151
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0829 [P2] 原版 datah_152 未生成对应 UI_D152
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_152.bin 但无 UI_D152
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0830 [P2] 原版 datah_153 未生成对应 UI_D153
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_153.bin 但无 UI_D153
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0831 [P2] 原版 datah_154 未生成对应 UI_D154
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_154.bin 但无 UI_D154
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0832 [P2] 原版 datah_155 未生成对应 UI_D155
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_155.bin 但无 UI_D155
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0833 [P2] 原版 datah_156 未生成对应 UI_D156
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_156.bin 但无 UI_D156
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0834 [P2] 原版 datah_157 未生成对应 UI_D157
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_157.bin 但无 UI_D157
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0835 [P2] 原版 datah_158 未生成对应 UI_D158
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_158.bin 但无 UI_D158
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0836 [P2] 原版 datah_159 未生成对应 UI_D159
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_159.bin 但无 UI_D159
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0837 [P2] 原版 datah_160 未生成对应 UI_D160
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_160.bin 但无 UI_D160
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0838 [P2] 原版 datah_161 未生成对应 UI_D161
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_161.bin 但无 UI_D161
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0839 [P2] 原版 datah_162 未生成对应 UI_D162
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_162.bin 但无 UI_D162
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0840 [P2] 原版 datah_163 未生成对应 UI_D163
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_163.bin 但无 UI_D163
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0841 [P2] 原版 datah_164 未生成对应 UI_D164
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_164.bin 但无 UI_D164
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0842 [P2] 原版 datah_165 未生成对应 UI_D165
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_165.bin 但无 UI_D165
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0843 [P2] 原版 datah_166 未生成对应 UI_D166
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_166.bin 但无 UI_D166
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0844 [P2] 原版 datah_167 未生成对应 UI_D167
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_167.bin 但无 UI_D167
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0845 [P2] 原版 datah_168 未生成对应 UI_D168
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_168.bin 但无 UI_D168
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0846 [P2] 原版 datah_169 未生成对应 UI_D169
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_169.bin 但无 UI_D169
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0847 [P2] 原版 datah_170 未生成对应 UI_D170
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_170.bin 但无 UI_D170
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0848 [P2] 原版 datah_171 未生成对应 UI_D171
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_171.bin 但无 UI_D171
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0849 [P2] 原版 datah_172 未生成对应 UI_D172
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_172.bin 但无 UI_D172
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0850 [P2] 原版 datah_173 未生成对应 UI_D173
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_173.bin 但无 UI_D173
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0851 [P2] 原版 datah_174 未生成对应 UI_D174
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_174.bin 但无 UI_D174
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0852 [P2] 原版 datah_175 未生成对应 UI_D175
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_175.bin 但无 UI_D175
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0853 [P2] 原版 datah_176 未生成对应 UI_D176
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_176.bin 但无 UI_D176
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0854 [P2] 原版 datah_177 未生成对应 UI_D177
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_177.bin 但无 UI_D177
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0855 [P2] 原版 datah_178 未生成对应 UI_D178
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_178.bin 但无 UI_D178
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0856 [P2] 原版 datah_179 未生成对应 UI_D179
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_179.bin 但无 UI_D179
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0857 [P2] 原版 datah_180 未生成对应 UI_D180
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_180.bin 但无 UI_D180
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0858 [P2] 原版 datah_181 未生成对应 UI_D181
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_181.bin 但无 UI_D181
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0859 [P2] 原版 datah_182 未生成对应 UI_D182
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_182.bin 但无 UI_D182
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0860 [P2] 原版 datah_183 未生成对应 UI_D183
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_183.bin 但无 UI_D183
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0861 [P2] 原版 datah_184 未生成对应 UI_D184
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_184.bin 但无 UI_D184
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0862 [P2] 原版 datah_185 未生成对应 UI_D185
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_185.bin 但无 UI_D185
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0863 [P2] 原版 datah_186 未生成对应 UI_D186
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_186.bin 但无 UI_D186
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0864 [P2] 原版 datah_187 未生成对应 UI_D187
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_187.bin 但无 UI_D187
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0865 [P2] 原版 datah_188 未生成对应 UI_D188
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_188.bin 但无 UI_D188
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0866 [P2] 原版 datah_189 未生成对应 UI_D189
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_189.bin 但无 UI_D189
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0867 [P2] 原版 datah_190 未生成对应 UI_D190
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_190.bin 但无 UI_D190
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0868 [P2] 原版 datah_191 未生成对应 UI_D191
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_191.bin 但无 UI_D191
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0869 [P2] 原版 datah_192 未生成对应 UI_D192
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_192.bin 但无 UI_D192
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0870 [P2] 原版 datah_193 未生成对应 UI_D193
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_193.bin 但无 UI_D193
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0871 [P2] 原版 datah_194 未生成对应 UI_D194
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_194.bin 但无 UI_D194
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0872 [P2] 原版 datah_195 未生成对应 UI_D195
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_195.bin 但无 UI_D195
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0873 [P2] 原版 datah_196 未生成对应 UI_D196
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_196.bin 但无 UI_D196
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0874 [P2] 原版 datah_197 未生成对应 UI_D197
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_197.bin 但无 UI_D197
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0875 [P2] 原版 datah_198 未生成对应 UI_D198
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_198.bin 但无 UI_D198
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0876 [P2] 原版 datah_199 未生成对应 UI_D199
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_199.bin 但无 UI_D199
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0877 [P2] 原版 datah_200 未生成对应 UI_D200
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_200.bin 但无 UI_D200
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0878 [P2] 原版 datah_201 未生成对应 UI_D201
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_201.bin 但无 UI_D201
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0879 [P2] 原版 datah_202 未生成对应 UI_D202
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_202.bin 但无 UI_D202
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0880 [P2] 原版 datah_203 未生成对应 UI_D203
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_203.bin 但无 UI_D203
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0881 [P2] 原版 datah_204 未生成对应 UI_D204
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_204.bin 但无 UI_D204
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0882 [P2] 原版 datah_205 未生成对应 UI_D205
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_205.bin 但无 UI_D205
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0883 [P2] 原版 datah_206 未生成对应 UI_D206
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_206.bin 但无 UI_D206
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0884 [P2] 原版 datah_207 未生成对应 UI_D207
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_207.bin 但无 UI_D207
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0885 [P2] 原版 datah_208 未生成对应 UI_D208
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_208.bin 但无 UI_D208
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0886 [P2] 原版 datah_209 未生成对应 UI_D209
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_209.bin 但无 UI_D209
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0887 [P2] 原版 datah_210 未生成对应 UI_D210
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_210.bin 但无 UI_D210
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0888 [P2] 原版 datah_211 未生成对应 UI_D211
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_211.bin 但无 UI_D211
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0889 [P2] 原版 datah_212 未生成对应 UI_D212
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_212.bin 但无 UI_D212
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0890 [P2] 原版 datah_213 未生成对应 UI_D213
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_213.bin 但无 UI_D213
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0891 [P2] 原版 datah_214 未生成对应 UI_D214
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_214.bin 但无 UI_D214
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0892 [P2] 原版 datah_215 未生成对应 UI_D215
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_215.bin 但无 UI_D215
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0893 [P2] 原版 datah_216 未生成对应 UI_D216
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_216.bin 但无 UI_D216
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0894 [P2] 原版 datah_217 未生成对应 UI_D217
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_217.bin 但无 UI_D217
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0895 [P2] 原版 datah_218 未生成对应 UI_D218
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_218.bin 但无 UI_D218
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0896 [P2] 原版 datah_219 未生成对应 UI_D219
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_219.bin 但无 UI_D219
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0897 [P2] 原版 datah_220 未生成对应 UI_D220
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_220.bin 但无 UI_D220
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0898 [P2] 原版 datah_221 未生成对应 UI_D221
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_221.bin 但无 UI_D221
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0899 [P2] 原版 datah_222 未生成对应 UI_D222
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_222.bin 但无 UI_D222
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0900 [P2] 原版 datah_223 未生成对应 UI_D223
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_223.bin 但无 UI_D223
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0901 [P2] 原版 datah_224 未生成对应 UI_D224
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_224.bin 但无 UI_D224
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0902 [P2] 原版 datah_225 未生成对应 UI_D225
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_225.bin 但无 UI_D225
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0903 [P2] 原版 datah_226 未生成对应 UI_D226
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_226.bin 但无 UI_D226
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0904 [P2] 原版 datah_227 未生成对应 UI_D227
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_227.bin 但无 UI_D227
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0905 [P2] 原版 datah_228 未生成对应 UI_D228
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_228.bin 但无 UI_D228
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0906 [P2] 原版 datah_229 未生成对应 UI_D229
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_229.bin 但无 UI_D229
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0907 [P2] 原版 datah_230 未生成对应 UI_D230
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_230.bin 但无 UI_D230
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0908 [P2] 原版 datah_231 未生成对应 UI_D231
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_231.bin 但无 UI_D231
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0909 [P2] 原版 datah_232 未生成对应 UI_D232
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_232.bin 但无 UI_D232
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0910 [P2] 原版 datah_233 未生成对应 UI_D233
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_233.bin 但无 UI_D233
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0911 [P2] 原版 datah_234 未生成对应 UI_D234
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_234.bin 但无 UI_D234
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0912 [P2] 原版 datah_235 未生成对应 UI_D235
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_235.bin 但无 UI_D235
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0913 [P2] 原版 datah_236 未生成对应 UI_D236
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_236.bin 但无 UI_D236
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0914 [P2] 原版 datah_237 未生成对应 UI_D237
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_237.bin 但无 UI_D237
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0915 [P2] 原版 datah_238 未生成对应 UI_D238
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_238.bin 但无 UI_D238
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0916 [P2] 原版 datah_239 未生成对应 UI_D239
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_239.bin 但无 UI_D239
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0917 [P2] 原版 datah_240 未生成对应 UI_D240
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_240.bin 但无 UI_D240
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0918 [P2] 原版 datah_241 未生成对应 UI_D241
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_241.bin 但无 UI_D241
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0919 [P2] 原版 datah_242 未生成对应 UI_D242
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_242.bin 但无 UI_D242
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0920 [P2] 原版 datah_243 未生成对应 UI_D243
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_243.bin 但无 UI_D243
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0921 [P2] 原版 datah_244 未生成对应 UI_D244
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_244.bin 但无 UI_D244
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0922 [P2] 原版 datah_245 未生成对应 UI_D245
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_245.bin 但无 UI_D245
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0923 [P2] 原版 datah_252 未生成对应 UI_D252
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_252.bin 但无 UI_D252
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0924 [P2] 原版 datah_254 未生成对应 UI_D254
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_254.bin 但无 UI_D254
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0925 [P2] 原版 datah_265 未生成对应 UI_D265
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_265.bin 但无 UI_D265
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0926 [P2] 原版 datah_266 未生成对应 UI_D266
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_266.bin 但无 UI_D266
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0927 [P2] 原版 datah_267 未生成对应 UI_D267
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_267.bin 但无 UI_D267
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0928 [P2] 原版 datah_268 未生成对应 UI_D268
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_268.bin 但无 UI_D268
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0929 [P2] 原版 datah_269 未生成对应 UI_D269
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_269.bin 但无 UI_D269
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0930 [P2] 原版 datah_270 未生成对应 UI_D270
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_270.bin 但无 UI_D270
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0931 [P2] 原版 datah_271 未生成对应 UI_D271
- 探针：`coverage`
- 位置：rev64/src_rs/ui_menu.h
- 问题：spak_out 有 datah_271.bin 但无 UI_D271
- 依据：原版该对话框未对齐
- 修法：用 gen_dialog_batch.py 生成（若为特殊界面则显式标诚实边界）

## AUD-A-0932 [P2] UI_D019 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (570,590) > 面板右下 (585,580)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0933 [P2] UI_D079 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (450,960) > 面板右下 (360,705)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0934 [P2] UI_D083 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (570,724) > 面板右下 (585,717)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0935 [P2] UI_D099 按钮组终点超出面板
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.c
- 问题：按钮最大 (570,724) > 面板右下 (585,717)
- 依据：同上
- 修法：核对居中偏移或按钮坐标

## AUD-A-0936 [P2] g_state 写入疑似在 g_mutex 之外
- 探针：`lock-discipline`
- 位置：src_rs/jni_bridge.c:225
- 问题：锁外写 g_state：g_state.stage = STG_PLAY;
- 依据：最近一次 pthread_mutex_lock 在第 None 行，此处更早或之后未解锁
- 修法：把 g_state 读写统一收敛到 g_mutex 临界区

## AUD-A-0937 [P2] g_state 写入疑似在 g_mutex 之外
- 探针：`lock-discipline`
- 位置：src_rs/jni_bridge.c:226
- 问题：锁外写 g_state：g_state.turn = SIDE_BLUE;
- 依据：最近一次 pthread_mutex_lock 在第 None 行，此处更早或之后未解锁
- 修法：把 g_state 读写统一收敛到 g_mutex 临界区

## AUD-A-0938 [P2] g_state 写入疑似在 g_mutex 之外
- 探针：`lock-discipline`
- 位置：src_rs/jni_bridge.c:227
- 问题：锁外写 g_state：g_state.winner = -1;
- 依据：最近一次 pthread_mutex_lock 在第 None 行，此处更早或之后未解锁
- 修法：把 g_state 读写统一收敛到 g_mutex 临界区

## AUD-A-0939 [P2] g_state 写入疑似在 g_mutex 之外
- 探针：`lock-discipline`
- 位置：src_rs/jni_bridge.c:228
- 问题：锁外写 g_state：g_state.moves_since_eat = 0;
- 依据：最近一次 pthread_mutex_lock 在第 None 行，此处更早或之后未解锁
- 修法：把 g_state 读写统一收敛到 g_mutex 临界区

## AUD-A-0940 [P2] g_state 写入疑似在 g_mutex 之外
- 探针：`lock-discipline`
- 位置：src_rs/jni_bridge.c:229
- 问题：锁外写 g_state：g_state.total_moves = 0;
- 依据：最近一次 pthread_mutex_lock 在第 None 行，此处更早或之后未解锁
- 修法：把 g_state 读写统一收敛到 g_mutex 临界区

## AUD-A-0941 [P2] UI_D020 按钮（Rename）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2150
- 问题：我方注释「Rename」，原版 text=「改名」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「改名」

## AUD-A-0942 [P2] UI_D037 按钮（Lay）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1942
- 问题：我方注释「Lay」，原版 text=「规则 - 布局」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「规则 - 布局」

## AUD-A-0943 [P2] UI_D037 按钮（Lay）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1952
- 问题：我方注释「Lay」，原版 text=「规则 - 布局」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「规则 - 布局」

## AUD-A-0944 [P2] UI_D037 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1973
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0945 [P2] UI_D037 按钮（Lay）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2021
- 问题：我方注释「Lay」，原版 text=「规则 - 布局」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「规则 - 布局」

## AUD-A-0946 [P2] UI_D037 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2073
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0947 [P2] UI_D037 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0948 [P2] UI_D037 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0949 [P2] UI_D037 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0950 [P2] UI_D037 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0951 [P2] UI_D042 按钮（View）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1929
- 问题：我方注释「View1」，原版 text=「玩家资料」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「玩家资料」

## AUD-A-0952 [P2] UI_D042 按钮（View）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1930
- 问题：我方注释「View2」，原版 text=「玩家资料」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「玩家资料」

## AUD-A-0953 [P2] UI_D042 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1972
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0954 [P2] UI_D042 按钮（Draw）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2032
- 问题：我方注释「Draw」，原版 text=「提和」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「提和」

## AUD-A-0955 [P2] UI_D042 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2179
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0956 [P2] UI_D042 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2421
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0957 [P2] UI_D042 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2443
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0958 [P2] UI_D044 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1972
- 问题：我方注释「Chat」，原版 text=「消息」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「消息」

## AUD-A-0959 [P2] UI_D044 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2179
- 问题：我方注释「Chat」，原版 text=「消息」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「消息」

## AUD-A-0960 [P2] UI_D044 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2421
- 问题：我方注释「Chat」，原版 text=「消息」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「消息」

## AUD-A-0961 [P2] UI_D044 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2443
- 问题：我方注释「Chat」，原版 text=「消息」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「消息」

## AUD-A-0962 [P2] UI_D050 按钮（View）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1929
- 问题：我方注释「View1」，原版 text=「复盘」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「复盘」

## AUD-A-0963 [P2] UI_D050 按钮（View）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:1930
- 问题：我方注释「View2」，原版 text=「复盘」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「复盘」

## AUD-A-0964 [P2] UI_D066 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2073
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0965 [P2] UI_D066 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0966 [P2] UI_D066 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0967 [P2] UI_D066 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0968 [P2] UI_D066 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0969 [P2] UI_D067 按钮（Draw）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2032
- 问题：我方注释「Draw」，原版 text=「提和」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「提和」

## AUD-A-0970 [P2] UI_D067 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2179
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0971 [P2] UI_D067 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2421
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0972 [P2] UI_D067 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2443
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0973 [P2] UI_D072 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2073
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0974 [P2] UI_D072 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0975 [P2] UI_D072 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0976 [P2] UI_D072 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0977 [P2] UI_D072 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0978 [P2] UI_D082 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2388
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0979 [P2] UI_D082 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0980 [P2] UI_D082 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0981 [P2] UI_D082 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2420
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0982 [P2] UI_D082 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2442
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0983 [P2] UI_D082 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0984 [P2] UI_D082 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2453
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0985 [P2] UI_D082 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0986 [P2] UI_D085 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「取消」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「取消」

## AUD-A-0987 [P2] UI_D085 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「取消」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「取消」

## AUD-A-0988 [P2] UI_D085 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「取消」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「取消」

## AUD-A-0989 [P2] UI_D085 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「取消」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「取消」

## AUD-A-0990 [P2] UI_D095 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2388
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0991 [P2] UI_D095 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0992 [P2] UI_D095 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0993 [P2] UI_D095 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2420
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0994 [P2] UI_D095 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2421
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0995 [P2] UI_D095 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2442
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0996 [P2] UI_D095 按钮（Chat）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2443
- 问题：我方注释「Chat」，原版 text=「聊天」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「聊天」

## AUD-A-0997 [P2] UI_D095 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-0998 [P2] UI_D095 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2453
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-0999 [P2] UI_D095 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1000 [P2] UI_D101 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1001 [P2] UI_D101 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1002 [P2] UI_D101 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1003 [P2] UI_D101 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1004 [P2] UI_D107 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2388
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-1005 [P2] UI_D107 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1006 [P2] UI_D107 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1007 [P2] UI_D107 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2420
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-1008 [P2] UI_D107 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2442
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-1009 [P2] UI_D107 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1010 [P2] UI_D107 按钮（Prev）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2451
- 问题：我方注释「Prev」，原版 text=「回退」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「回退」

## AUD-A-1011 [P2] UI_D107 按钮（Next）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2452
- 问题：我方注释「Next」，原版 text=「前进」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「前进」

## AUD-A-1012 [P2] UI_D107 按钮（Menu）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2453
- 问题：我方注释「Menu」，原版 text=「菜单」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「菜单」

## AUD-A-1013 [P2] UI_D107 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1014 [P2] UI_D120 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2389
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1015 [P2] UI_D120 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2396
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1016 [P2] UI_D120 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2444
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1017 [P2] UI_D120 按钮（Back）文案与原版不一致
- 探针：`text-delta`
- 位置：src_rs/ui_menu.c:2454
- 问题：我方注释「Back」，原版 text=「返回」
- 依据：原版 XML 为权威文案
- 修法：改用原版文案「返回」

## AUD-A-1018 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:44
- 问题：调用 memset：memset(au, 0, sizeof(*au));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1019 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:78
- 问题：调用 memset：memset(au, 0, sizeof(*au));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1020 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:103
- 问题：调用 memcpy：memcpy(tag, p, 4); tag[4] = 0;
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1021 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/audio.c:126
- 问题：调用 memcpy：memcpy(pcm, buf + data_pos + 8, data_len);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1022 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/bt_protocol.c:74
- 问题：调用 memcpy：memcpy(buf + n * cap_per_frame, fb, len + 1);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1023 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/bt_session.c:31
- 问题：调用 memset：memset(s, 0, sizeof(*s));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1024 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/containers.c:27
- 问题：调用 memset：memset(cc->count_tbl, 0, sizeof(cc->count_tbl));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1025 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/containers.c:53
- 问题：调用 memset：memset(cc, 0, sizeof(*cc));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1026 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:28
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1027 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/evverify.c:39
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1028 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/glview_gl.c:87
- 问题：调用 memset：memset(gv, 0, sizeof(*gv));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1029 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:816
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1030 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:1111
- 问题：调用 memset：memset(&g_state, 0, sizeof(g_state));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1031 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:1176
- 问题：调用 memset：memset(&g_state, 0, sizeof(g_state));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1032 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/jni_bridge.c:2000
- 问题：调用 memset：memset(&sa, 0, sizeof(sa));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1033 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/layout.c:25
- 问题：调用 memset：memset(cnt, 0, sizeof(int[RANK_COUNT]));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1034 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/layout.c:32
- 问题：调用 memset：memset(st, 0, sizeof(*st));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1035 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/layout.c:97
- 问题：调用 memset：memset(seen, 0, sizeof(seen));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1036 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:148
- 问题：调用 memcpy：if (idx == 0) memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1037 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:156
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1038 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:160
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1039 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:166
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1040 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:169
- 问题：调用 memcpy：memcpy(lay, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1041 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/main.c:199
- 问题：调用 memcpy：memcpy(l2, JQ_DEFAULT_LAY, 30);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1042 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/png_decode.c:80
- 问题：调用 memcpy：memcpy(idat + idat_len, cdata, clen);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1043 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/png_decode.c:109
- 问题：调用 memset：memset(&zs, 0, sizeof(zs));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1044 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render.c:377
- 问题：调用 memset：memset(rr, 0, sizeof(*rr));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1045 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_obj.c:11
- 问题：调用 memset：memset(l, 0, sizeof(*l));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1046 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_obj.c:43
- 问题：调用 memset：memset(v, 0, sizeof(*v));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1047 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_scene.c:82
- 问题：调用 memset：memset(sc, 0, sizeof(*sc));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1048 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/render_scene.c:107
- 问题：调用 memset：memset(sc, 0, sizeof(*sc));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1049 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/replay.c:18
- 问题：调用 memset：memset(rp, 0, sizeof(*rp));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1050 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:52
- 问题：调用 memset：memset(visited, 0, sizeof(visited));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1051 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:158
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1052 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:163
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1053 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:164
- 问题：调用 memset：memset(dst, 0, sizeof(*dst));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1054 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:174
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1055 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:189
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1056 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:195
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1057 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:204
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1058 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:208
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1059 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:212
- 问题：调用 memset：memset(src, 0, sizeof(*src));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1060 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/rules.c:213
- 问题：调用 memset：memset(dst, 0, sizeof(*dst));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1061 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/save.c:39
- 问题：调用 memset：memset(sd, 0, sizeof(*sd));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1062 [P2] memset 使用需复核（需核对长度）
- 探针：`unsafe-func`
- 位置：src_rs/touch_state.c:27
- 问题：调用 memset：memset(ts, 0, sizeof(*ts));
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1063 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:352
- 问题：调用 memcpy：memcpy(tmp, g_lay_names[i], UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1064 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:353
- 问题：调用 memcpy：memcpy(g_lay_names[i], g_lay_names[j], UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1065 [P2] memcpy 使用需复核（需核对长度来源）
- 探针：`unsafe-func`
- 位置：src_rs/ui_state.c:354
- 问题：调用 memcpy：memcpy(g_lay_names[j], tmp, UI_LAY_NAME);
- 依据：该行未见边界参数约束（同行无 snprintf/strncpy/memcpy/memmove 等受控替代）
- 修法：改用 snprintf/strlcpy 语义并显式校验长度；或记录已证明安全的边界

## AUD-A-1066 [P3] 阶段 0 交付物存在
- 探针：`artifact`
- 位置：dist/phase0.apk
- 问题：大小 8205847 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-1067 [P3] 阶段 1 交付物存在
- 探针：`artifact`
- 位置：dist/phase1.apk
- 问题：大小 8214110 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-1068 [P3] 阶段 2 交付物存在
- 探针：`artifact`
- 位置：dist/phase2.apk
- 问题：大小 10952491 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-1069 [P3] 阶段 3 交付物存在
- 探针：`artifact`
- 位置：dist/phase3.apk
- 问题：大小 9338594 字节
- 依据：需校验 STORE/签名/ABI
- 修法：跑 build_phases.sh --verify-only

## AUD-A-1070 [P3] UI_D006 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1071 [P3] UI_D007 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1072 [P3] UI_D010 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1073 [P3] UI_D012 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1074 [P3] UI_D016 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1075 [P3] UI_D017 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1076 [P3] UI_D020 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1077 [P3] UI_D033 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1078 [P3] UI_D034 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1079 [P3] UI_D035 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1080 [P3] UI_D038 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1081 [P3] UI_D041 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1082 [P3] UI_D042 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1083 [P3] UI_D043 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1084 [P3] UI_D044 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1085 [P3] UI_D047 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1086 [P3] UI_D055 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=-0.50, Y0 Δ=+0.00, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1087 [P3] UI_D061 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1088 [P3] UI_D062 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1089 [P3] UI_D063 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1090 [P3] UI_D065 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1091 [P3] UI_D067 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1092 [P3] UI_D068 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1093 [P3] UI_D069 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1094 [P3] UI_D070 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1095 [P3] UI_D071 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1096 [P3] UI_D072 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1097 [P3] UI_D077 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1098 [P3] UI_D083 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1099 [P3] UI_D084 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1100 [P3] UI_D086 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1101 [P3] UI_D096 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1102 [P3] UI_D099 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1103 [P3] UI_D102 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=-0.50, Y0 Δ=+0.00, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1104 [P3] UI_D112 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=+0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1105 [P3] UI_D119 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1106 [P3] UI_D121 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1107 [P3] UI_D123 面板几何存在亚像素偏差
- 探针：`geom-delta`
- 位置：src_rs/ui_menu.h
- 问题：X0 Δ=+0.00, Y0 Δ=-0.50, W Δ=+0.00, H Δ=+0.00
- 依据：浮点取整差，视觉可接受
- 修法：可保留（记录为亚像素偏差）

## AUD-A-1108 [P3] 本地提交未推送到 origin/main
- 探针：`git`
- 位置：仓库
- 问题：## main...origin/main [ahead 2]
- 依据：远端缺最新提交
- 修法：git push origin main

## AUD-A-1109 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/evverify.c:2042
- 问题：窄类型赋值含乘法：int i = r * 5 + c;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-1110 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/png_decode.c:16
- 问题：窄类型赋值含乘法：int stride = w * bpp;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-1111 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/png_decode.c:101
- 问题：窄类型赋值含乘法：int stride = (int)w * channels;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-1112 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/render.c:2690
- 问题：窄类型赋值含乘法：int filled = have && lay[r * 5 + c] != 0;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-1113 [P3] 窄类型乘法可能溢出
- 探针：`int-overflow`
- 位置：src_rs/render.c:9761
- 问题：窄类型赋值含乘法：int sig = imgset_trans * 8 + imgset_piece * 4 + imgset_night * 2 + imgset_opp;
- 依据：未见显式溢出检查
- 修法：中间用 int64_t 或加范围断言

## AUD-A-1114 [P3] 一致性基准未覆盖 UI_MENU 路径
- 探针：`test-gap`
- 位置：src_rs/consistency_desktop.c
- 问题：未提及 UI_MENU
- 依据：基准序列覆盖不足
- 修法：补对应场景

## AUD-A-1115 [P3] 一致性基准未覆盖 UI_GAME 路径
- 探针：`test-gap`
- 位置：src_rs/consistency_desktop.c
- 问题：未提及 UI_GAME
- 依据：基准序列覆盖不足
- 修法：补对应场景

## AUD-A-1116 [P3] 一致性基准未覆盖 replay 路径
- 探针：`test-gap`
- 位置：src_rs/consistency_desktop.c
- 问题：未提及 replay
- 依据：基准序列覆盖不足
- 修法：补对应场景

## AUD-A-1117 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/bt_session.c:102
- 问题：/* 翻棋帧：type=FLIP, f2=col, f3=row，其余占位 0 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1118 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/consistency_desktop.c:50
- 问题：char tmpl[] = "/tmp/jq_bench_XXXXXX";
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1119 [P3] 未完成标记（XXX）
- 探针：`todo`
- 位置：src_rs/evverify.c:18
- 问题：#include <stdlib.h>   /* abs()（SPAK2UI 批量 run_uiXXX 几何 Δ 判定） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1120 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:588
- 问题：+ 几何 Δ=0 + 业务分支（桌号占位/确定边界对话框/取消，确定性不依赖触摸/GL）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1121 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:627
- 问题：/* 桌号（action=0）→ 占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1122 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:632
- 问题：printf("join table: hit=%d action=%d → 桌号占位标签（★ 诚实边界：不画真输入框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1123 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:654
- 问题：+ 几何 Δ=0 + 业务分支（密码占位/确定边界对话框/取消，确定性不依赖触摸/GL）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1124 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:693
- 问题：/* 密码（action=0）→ 占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1125 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:698
- 问题：printf("create pwd: hit=%d action=%d → 密码占位标签（★ 诚实边界：不画真输入框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1126 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:964
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1127 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1000
- 问题：/* 清除（action=0）→ 清本地占位记录（诚实边界：联机聊天文本未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1128 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1005
- 问题：printf("chatlog clear: hit=%d action=%d → 清本地占位记录（★ 诚实边界：联机聊天文本未逆向，仅蓝牙占位帧）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1129 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1022
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1130 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1059
- 问题：/* 修改（action=0）→ 诚实边界对话框（修改资料需账号系统，未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1131 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1064
- 问题：printf("profile modify: hit=%d action=%d → 诚实边界对话框（修改资料需账号系统，未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1132 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1080
- 问题：printf("profile: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：账号系统未逆向，Info=占位）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1133 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1090
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1134 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1132
- 问题：/* 提和（action=0）→ BT_T_DRAW 帧 + 诚实边界对话框（本地提和判定未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1135 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1137
- 问题：printf("drawdetail draw: hit=%d action=%d → BT_T_DRAW 帧 + 诚实边界对话框（★ 本地提和判定未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1136 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1146
- 问题：/* 聊天（action=2）→ BT_T_CHAT 占位帧 + 边界对话框（联机聊天文本未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1137 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1151
- 问题：printf("drawdetail chat: hit=%d action=%d → BT_T_CHAT 占位帧（★ 联机聊天文本未逆向）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1138 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1188
- 问题：printf("drawdetail: 7 图标几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：本地提和判定未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1139 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1199
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1140 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1236
- 问题：/* 图像（action=0）→ UI_IMGSET7（图像设置简化版 datah_008，★ 诚实边界：渲染参数未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1141 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1245
- 问题：printf("setentry img: hit=%d action=%d → UI_IMGSET7 图像设置弹窗（datah_008，★ 诚实边界：渲染参数未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1142 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1262
- 问题：printf("setentry: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1143 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1273
- 问题：图像设置渲染参数未实现（src_rs 无对应渲染参数），仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1144 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1309
- 问题：/* 图像（action=0）→ UI_IMGSET8（图像设置简化版 datah_008，★ 诚实边界：渲染参数未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1145 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1318
- 问题：printf("setentry4 img: hit=%d action=%d → UI_IMGSET8 图像设置（简化版 datah_008，★ 诚实边界：渲染参数未实现）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1146 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1328
- 问题：printf("setentry4: 2 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1147 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1338
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1148 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1389
- 问题：printf("imgset7 trans: hit=%d action=%d → 开/关 循环 ui_imgset_trans_get()=%d（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1149 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1397
- 问题：printf("imgset7 piece: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1150 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1404
- 问题：printf("imgset7 night: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1151 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1411
- 问题：printf("imgset7 opp: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1152 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1433
- 问题：printf("imgset7: 6 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1153 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1443
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1154 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1490
- 问题：printf("imgset8 trans: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1155 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1498
- 问题：printf("imgset8 piece: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1156 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1505
- 问题：printf("imgset8 night: hit=%d action=%d → 开/关 循环（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1157 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:1527
- 问题：printf("imgset8: 5 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1158 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1533
- 问题：命中判定 + 几何 Δ=0 + 业务分支（8 显示占位标签 + 开始→边界对话框/取消→回主菜单，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1159 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1538
- 问题：显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1160 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1547
- 问题：{275.0f, 473.0f, 555.0f, 508.0f},   /* 0=先手 Player1（140,85,420,120）→ 占位标签"蓝方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1161 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1548
- 问题：{275.0f, 513.0f, 555.0f, 548.0f},   /* 1=后手 Player2（140,125,420,160）→ 占位标签"红方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1162 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1549
- 问题：{275.0f, 553.0f, 555.0f, 588.0f},   /* 2=玩法 Lay（140,165,420,200）→ 占位标签"布局" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1163 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1550
- 问题：{275.0f, 593.0f, 555.0f, 628.0f},   /* 3=明暗棋 Show（140,205,420,240）→ 占位标签"暗棋" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1164 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1551
- 问题：{275.0f, 633.0f, 555.0f, 668.0f},   /* 4=和棋 Draw（140,245,420,280）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1165 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1552
- 问题：{275.0f, 673.0f, 555.0f, 708.0f},   /* 5=大本营 CmdSafe（140,285,420,320）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1166 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1553
- 问题：{275.0f, 713.0f, 555.0f, 748.0f},   /* 6=吃地雷 BingLei（140,325,420,360）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1167 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1554
- 问题：{275.0f, 753.0f, 555.0f, 788.0f},   /* 7=吃军旗 BingQi（140,365,420,400）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1168 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1604
- 问题：printf("join065: 10 控件几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：联机开局配置=占位标签，开始→边界对话框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1169 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1801
- 问题：开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1170 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1865
- 问题：printf("room060: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：联机房间=边界对话框，2 玩家显示区=占位标签）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1171 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1871
- 问题：命中判定 + 几何 Δ=0 + 业务分支（发送→BT_T_CHAT 占位帧+边界对话框/记录→UI_CHATLOG/
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1172 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1875
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1173 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1876
- 问题：Msg 输入框简化为占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1174 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1883
- 问题：{175.0f, 723.0f, 285.0f, 783.0f},   /* 0=发送 Send（25,245,135,305）→ BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1175 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1913
- 问题：/* 发送（action=0）→ BT_T_CHAT 占位帧 + 边界对话框（联机聊天文本未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1176 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1918
- 问题：printf("chat063 send: hit=%d action=%d → BT_T_CHAT 占位帧 + 边界对话框（★ 诚实边界：联机聊天文本未逆向）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1177 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:1944
- 问题：printf("chat063: 3 按钮几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：联机聊天文本未逆向，发送=占位帧+边界对话框）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1178 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2115
- 问题：（★ 诚实边界：账号系统未逆向，Info=占位，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1179 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2118
- 问题：（★ 诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1180 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2121
- 问题：（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1181 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2124
- 问题：（★ 诚实边界：图像设置渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1182 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2127
- 问题：（★ 诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1183 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/evverify.c:2130
- 问题：（★ 诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1184 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2133
- 问题：（★ 诚实边界：联机开局配置=占位标签，开始→边界对话框，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1185 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2142
- 问题：（★ 诚实边界：联机房间=边界对话框，2 玩家显示区=占位标签，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1186 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2145
- 问题：（★ 诚实边界：联机聊天文本未逆向，发送=占位帧+边界对话框，不依赖触摸/GL） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1187 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2151
- 问题：几何 Δ=0 + 命中判定 + 业务分支（★ 诚实边界：未逆向数据=占位标签+边界对话框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1188 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2365
- 问题：占位标签 + 边界对话框，桌面验证只证几何+命中+状态切换，不证数据真实现 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1189 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2475
- 问题：printf("ui46 close: hit=%d action=%d → 回对局（★ 诚实边界：玩家资料内容未逆向，占位标签）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1190 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2518
- 问题：printf("ui47 close: hit=%d action=%d → 回对局（★ 诚实边界：玩家列表未逆向，占位）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1191 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/evverify.c:2561
- 问题：printf("ui49 close: hit=%d action=%d → 回对局（★ 诚实边界：对局详情内容未逆向，占位）%s\n",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1192 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1288
- 问题：OnCreate 弹 UI_JOIN 加入游戏弹窗：桌号占位标签 + 确定/取消）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1193 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1297
- 问题：LOGI("jq.join 钩子：→ UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1194 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1303
- 问题：OnCreate 弹 UI_CREATE 创建游戏弹窗：密码占位标签 + 确定/取消）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1195 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1312
- 问题：LOGI("jq.create 钩子：→ UI_CREATE 创建游戏弹窗（datah_035 密码占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1196 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1361
- 问题：OnCreate 弹 UI_CHATLOG 聊天记录弹窗：Msg 文本框（诚实边界占位） + 清除/关闭 2 按钮）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1197 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1363
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1198 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1369
- 问题：LOGI("jq.chatlog 钩子：→ UI_CHATLOG 聊天记录弹窗（datah_029 Msg 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1199 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1375
- 问题：OnCreate 弹 UI_PROFILE 个人资料弹窗：Info 文本框（诚实边界占位） + 修改/帮助/返回 3 按钮）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1200 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1378
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1201 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1384
- 问题：LOGI("jq.profile 钩子：→ UI_PROFILE 个人资料弹窗（datah_031 Info 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1202 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1392
- 问题：一次性（清 0）。★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1203 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1400
- 问题："诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1204 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1407
- 问题：一次性（清 0）。★ 诚实边界：本地提和判定未实现（原版需对端应答），联机提和/认输走
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1205 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1415
- 问题："提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，诚实边界：本地提和判定未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1206 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1423
- 问题：占位标签 + 边界对话框，不包装成"已实现" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1207 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1450
- 问题："1 按钮，★ 诚实边界：玩家资料内容未逆向，占位标签）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1208 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1460
- 问题："1 按钮，★ 诚实边界：玩家列表未逆向，占位）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1209 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1470
- 问题："1 按钮，★ 诚实边界：对局详情内容未逆向，占位）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1210 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1547
- 问题：★ 诚实边界（已部分解除）：夜间模式/对方棋子接渲染层（暗色遮罩/红方隐藏），仅界面/棋子动画过渡帧未实现，入口指向 UI_IMGSET7；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1211 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1556
- 问题："★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1212 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1563
- 问题：★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现，入口指向 UI_IMGSET8；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1213 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1571
- 问题："图像→UI_IMGSET8；声音→UI_SETTINGS（datah_001），★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅动画过渡帧未实现）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1214 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1578
- 问题：★ 诚实边界（已部分解除）：夜间模式接渲染层暗色遮罩 / 对方棋子接渲染层隐藏红方 / 棋子动画关=不画，仅界面动画过渡帧未实现；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1215 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1586
- 问题："★ 诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅界面动画过渡帧未实现；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1216 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1595
- 问题：★ 诚实边界：图像/夜间渲染参数未实现，开关仅 UI 状态记录；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1217 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1603
- 问题："★ 诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1218 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1612
- 问题：★ 诚实边界：联机开局配置=占位标签，开始→边界对话框（src_rs 仅蓝牙已实现）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1219 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1619
- 问题："4 规则显示 + 开始/取消，★ 诚实边界：联机开局配置=占位标签，开始→边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1220 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1654
- 问题：OnCreate 弹 UI_ROOM060 房间态弹窗：2 玩家房间列表（占位标签）+ 开始/换桌/离开。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1221 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1656
- 问题：★ 诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1222 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1662
- 问题：LOGI("jq.room060 钩子：→ UI_ROOM060 房间态弹窗（datah_060 2 玩家房间列表（占位标签）+ 开始/换桌/离开，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1223 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1663
- 问题："★ 诚实边界：联机房间=边界对话框，2 玩家显示区=占位标签）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1224 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1668
- 问题：OnCreate 弹 UI_CHAT063 聊天完整版弹窗：Msg 输入框（占位标签）+ 发送/记录/关闭。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1225 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1670
- 问题：★ 诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1226 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1676
- 问题：LOGI("jq.chat063 钩子：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1227 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1677
- 问题："★ 诚实边界：联机聊天文本未逆向，发送=占位帧+边界对话框；记录→UI_CHATLOG（datah_029））");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1228 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:1982
- 问题：未实现而在 GLThread 上崩 → 信号 handler 设标志位，OnUpdate 检测到后跳过渲染
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1229 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2180
- 问题：LOGI("OnUpdate 加入游戏入口：join=1 → UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1230 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2197
- 问题：LOGI("OnUpdate 创建游戏入口：create=1 → UI_CREATE 创建游戏弹窗（datah_035 密码占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1231 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2259
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1232 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2265
- 问题：LOGI("OnUpdate 聊天记录入口：chatlog=1 → UI_CHATLOG 聊天记录弹窗（datah_029 Msg 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1233 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2277
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1234 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2283
- 问题：LOGI("OnUpdate 个人资料入口：profile=1 → UI_PROFILE 个人资料弹窗（datah_031 Info 文本框（诚实边界占位） + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1235 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2294
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1236 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2302
- 问题："提和/认输/聊天/系统设置/玩家资料/战况统计/软件信息，诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1237 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2314
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1238 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2323
- 问题："图像→UI_IMGSET7（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）；声音→UI_SETTINGS（datah_001）；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1239 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2335
- 问题：★ 诚实边界：图像设置渲染参数未实现，图像设置入口指向 UI_IMGSET8（datah_008 简化版，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1240 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2343
- 问题："无游戏设置项，联机态/蓝牙态；图像→UI_IMGSET8（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1241 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2355
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1242 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2363
- 问题："诚实边界：图像/夜间/对方棋子渲染参数未实现，仅 UI 状态记录；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1243 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2377
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1244 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2385
- 问题："诚实边界：图像/夜间渲染参数未实现，仅 UI 状态记录；"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1245 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2400
- 问题：显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1246 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2409
- 问题："4 规则显示 + 开始/取消，诚实边界：联机开局配置=占位标签，开始→边界对话框（src_rs 仅蓝牙已实现））");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1247 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2463
- 问题：OnCreate 弹 UI_ROOM060 房间态弹窗：2 玩家房间列表（占位标签）+ 开始/换桌/离开。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1248 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2466
- 问题：开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据，触摸 flaky 下房间列表无意义）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1249 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2472
- 问题：LOGI("jq.room060 钩子：→ UI_ROOM060 房间态弹窗（datah_060 2 玩家房间列表（占位标签）+ 开始/换桌/离开，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1250 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2473
- 问题："诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1251 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2482
- 问题：OnCreate 弹 UI_CHAT063 聊天完整版弹窗：Msg 输入框（占位标签）+ 发送/记录/关闭。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1252 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2484
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1253 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2485
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1254 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2486
- 问题：发送→BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029 已实现）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1255 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2493
- 问题：LOGI("jq.chat063 钩子：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1256 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2494
- 问题："诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029））");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1257 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2563
- 问题：范式与 jq.login 同；不重启 app）。★ 诚实边界：未逆向数据=占位标签+边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1258 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2584
- 问题：LOGI("OnUpdate SPAK2UI 批量入口：%s=1 → UI_UI%d 弹窗（诚实边界：未逆向数据=占位）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1259 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2773
- 问题：/* UI 分流：菜单态点按钮 → 进对局（人机/重开），其它按钮占位日志。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1260 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:2817
- 问题：显示边界对话框（4 方规则未实现，勿静默开 2 人局误导为四国可玩）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1261 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:3805
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1262 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:4319
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1263 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:4803
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1264 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5563
- 问题：jq_show_dialog(env, "提和（诚实边界）", "提和：原版需对端应答，src_rs 本地未实现提和判定。"); ui_screen_set(UI_GAME); sfx_play(env, "pop");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1265 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5704
- 问题：LOGI("OnTouch 方式选择：Wi-Fi 联机 → UI_JOIN 加入游戏弹窗（datah_034 桌号占位标签 + 确定/取消，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1266 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5774
- 问题：点 2/3/4 人 → 诚实边界对话框（4 方规则未实现）→ 回主菜单；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1267 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5786
- 问题："未实现。原版四国棋盘/规则需另项逆向移植。\n"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1268 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5847
- 问题：点 用户名/密码（占位标签，无输入框）→ 无动作（LOGI）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1269 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5852
- 问题：Junqi.* 登录/注册 20+ 事件未实现）；游客=本地单机唯一真生效路径。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1270 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5861
- 问题：if (lact == 0 || lact == 1) {   /* 用户名/密码 占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1271 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5862
- 问题：LOGI("OnTouch 登录弹窗：用户名/密码 占位标签（★ 诚实边界：输入框简化，不画真输入框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1272 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5867
- 问题："未实现，当前为离线单机模式。请选择游客进入。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1273 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5874
- 问题："未实现，当前为离线单机模式。请选择游客进入。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1274 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:5957
- 问题：点 取消 / 面板外 → 回对局（不载入）。★ 诚实边界：ListBox 滚动未实现（6 条截断）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1275 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6088
- 问题：点 取消 / 面板外 → 回对局。30 格预览区 = 小方块占位（★ 诚实边界）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1276 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6314
- 问题：点 桌号（占位标签）→ 无动作（LOGI）；点 确定 → 诚实边界对话框 + 回主菜单；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1277 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6325
- 问题：if (jact == 0) {   /* 桌号占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1278 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6326
- 问题：LOGI("OnTouch 加入游戏弹窗：桌号占位标签（★ 诚实边界：输入框简化，不画真输入框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1279 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6351
- 问题：点 密码（占位标签）→ 无动作（LOGI）；点 确定 → 诚实边界对话框 + 回主菜单；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1280 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6362
- 问题：if (cact == 0) {   /* 密码占位标签（触摸 flaky 无输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1281 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6363
- 问题：LOGI("OnTouch 创建游戏弹窗：密码占位标签（★ 诚实边界：输入框简化，不画真输入框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1282 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6453
- 问题：Info 文本=☰ 战况统计复用（渲染层占位标签 + LOGI 实际值）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1283 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6533
- 问题：点 清除 → 清本地占位记录（诚实边界：联机聊天文本未逆向，仅蓝牙占位帧）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1284 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6535
- 问题：Msg 文本框=诚实边界占位（渲染层占位标签 + LOGI）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1285 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6544
- 问题：if (cact == 0) {   /* 清除：清本地占位记录（诚实边界） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1286 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6545
- 问题：static char chat_log_clear = 1;   /* 占位记录标志（无真实聊天文本存储） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1287 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6548
- 问题：LOGI("OnTouch 聊天记录弹窗：清除 → 清本地占位记录（★ 诚实边界：联机聊天文本未逆向，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1288 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6549
- 问题："src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本存储，清除=清占位标志）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1289 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6567
- 问题：点 修改 → 诚实边界对话框（修改资料需账号系统，未实现）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1290 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6570
- 问题：Info 文本框=诚实边界占位（渲染层占位标签 + LOGI）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1291 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6580
- 问题：if (pact == 0) {   /* 修改 → 诚实边界对话框（修改资料需账号系统，未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1292 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6593
- 问题："这些字段均为诚实边界占位，未实现账号功能。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1293 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6614
- 问题：提和 → jq_bt_ctrl_send(BT_T_DRAW) + 诚实边界对话框（本地提和判定未实现，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1294 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6622
- 问题：★ 诚实边界：本地提和判定未实现（src_rs 离线单机无对端），联机提和/认输走
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1295 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6632
- 问题：if (dact == 0) {   /* 提和 → 发 BT_T_DRAW 帧 + 诚实边界对话框（本地提和判定未实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1296 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6636
- 问题："本地提和判定未实现（原版需对端应答，\n"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1297 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6639
- 问题：LOGI("OnTouch 提和详情：提和 → 发送 DRAW 帧 + 诚实边界对话框（本地提和判定未实现）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1298 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6648
- 问题：} else if (dact == 2) {   /* 聊天 → 发 BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1299 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6653
- 问题："当前发送聊天占位帧（对端无文本渲染）。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1300 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6655
- 问题：LOGI("OnTouch 提和详情：聊天 → 发送 CHAT 占位帧（联机聊天文本未逆向）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1301 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6704
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1302 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6718
- 问题："★ 诚实边界：渲染参数未实现，仅 UI 状态记录）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1303 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6744
- 问题：★ 诚实边界：图像设置渲染参数未实现，开关只是 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1304 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6757
- 问题："★ 诚实边界：渲染参数未实现，仅 UI 状态记录）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1305 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6779
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1306 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6792
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：界面动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1307 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6797
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：棋子动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1308 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6802
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：夜间模式切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1309 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6807
- 问题：LOGI("OnTouch 图像设置弹窗（完整版）：对方棋子切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1310 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6839
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1311 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6852
- 问题：LOGI("OnTouch 图像设置弹窗（简化版）：界面动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1312 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6857
- 问题：LOGI("OnTouch 图像设置弹窗（简化版）：棋子动画切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1313 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6862
- 问题：LOGI("OnTouch 图像设置弹窗（简化版）：夜间模式切换 → %s（★ 诚实边界：渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1314 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6889
- 问题：8 显示项 = 占位标签（先手/后手/玩法/明暗棋/4 规则，非按钮，显示当前值占位
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1315 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6903
- 问题：if (jact >= 0 && jact <= 7) {   /* 8 显示项：占位标签（不实际处理，仅日志） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1316 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:6904
- 问题：LOGI("OnTouch 加入游戏完整版弹窗：显示项%d（占位标签，不实际处理，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1317 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7054
- 问题：（非按钮，简化为占位标签"蓝方/红方"，触摸 flaky 下房间列表无意义）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1318 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7061
- 问题：2 玩家显示区 = 占位标签（不画真数据） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1319 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7101
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1320 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7102
- 问题：点 发送 → jq_bt_ctrl_send(BT_T_CHAT) 占位帧 + 诚实边界对话框
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1321 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7103
- 问题：（"聊天（诚实边界）：联机聊天文本未逆向，src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1322 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7107
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1323 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7116
- 问题：if (c63act == 0) {   /* 发送 → BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1324 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7121
- 问题："src_rs 蓝牙聊天是占位帧 BT_T_CHAT，\n"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1325 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7124
- 问题：LOGI("OnTouch 聊天完整版弹窗：发送 → BT_T_CHAT 占位帧 + 边界对话框（联机聊天文本未逆向）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1326 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7254
- 问题：点面板外 → 回对局。★ 诚实边界：未逆向数据=占位标签+边界对话框，不包装成"已实现" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1327 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7265
- 问题："src_rs 无排行榜数据通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1328 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7289
- 问题："src_rs 无详细战绩数据通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1329 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7468
- 问题："src_rs 无详细战况数据通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1330 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7494
- 问题："src_rs 无更新通道，占位。");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1331 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7596
- 问题："诚实边界：本地提和判定未实现，提和/认输走 BT 帧 + 边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1332 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7610
- 问题：datah_063 聊天完整版弹窗（Msg 输入框占位标签 + 发送/记录/关闭 3 按钮），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1333 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7611
- 问题：发帧逻辑保留在弹窗内（发送→BT_T_CHAT 占位帧 + 边界对话框）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1334 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7614
- 问题：LOGI("☰菜单 聊天（联机版）：→ UI_CHAT063 聊天完整版弹窗（datah_063 Msg 输入框占位 + "
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1335 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/jni_bridge.c:7615
- 问题："发送/记录/关闭 3 按钮，诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1336 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:1515
- 问题：/* 图集 160 宽里字形按字数自然占位（4 字占 ~150、2 字占 ~75，居中）：
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1337 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:2018
- 问题：按钮几何 ui_nplayer_buttons()（720×1280 基准）。★ 诚实边界：四国=未实现 4 方规则。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1338 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2635
- 问题：简化：行底色 + 序号纯色块占位（★ 诚实边界：ListBox 文本渲染未接动态名，占位块）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1339 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2654
- 问题：else glUniform4f(uc, 0.15f, 0.15f, 0.18f, 0.60f);        /* 无条目灰暗占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1340 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2668
- 问题：★ 诚实边界：30 格 PiecePic 真棋子图未画，小方块占位（原版 Piece0-29 为棋子图）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1341 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2707
- 问题：if (!pdiag) { pdiag = 1; LOGI("rs_layres_preview：30 格棋子预览（小方块占位，选中=%s）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1342 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:2713
- 问题：取消/面板外=回对局。★ 诚实边界：ListBox 滚动未实现，6 条截断 + 选中金色高亮。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1343 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2857
- 问题：删除=删选中项；取消=回对局。★ 诚实边界：PiecePic 真棋子图未画（小方块占位）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1344 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2865
- 问题：/* 30 格棋子预览（小方块 6×5 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1345 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2926
- 问题：if (!mdiag) { mdiag = 1; LOGI("rs_draw_laymgr：布局管理弹窗（datah_016 30 格预览（小方块占位）+ 载入/删除/取消，面板 400×475）"); }
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1346 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2929
- 问题：/* ★ P1（datah_107 证据）：复盘回放渲染——棋盘（当前回放位置，棋子占位方块范式）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1347 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2934
- 问题：复盘态不画真实棋子图（复用棋盘现有占位方块范式，棋子图集缺失则退化纯色格）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1348 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:2969
- 问题：/* 复盘态棋子占位方块（60 格有子 → 蓝/红小方块，居中格区，不复用 rs_draw_board
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1349 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3006
- 问题：+ 用户名/密码占位标签（EditLabel 不画真输入框）+ 登录/注册/游客/返回 4 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1350 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3009
- 问题：游客=本地单机（无账号，唯一真生效路径）；输入框简化为占位标签（触摸 flaky）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1351 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3105
- 问题：/* ④ 用户名/密码 占位 EditLabel（矩形浅色底 + 占位文字，★ 诚实边界：不画真输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1352 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3125
- 问题：/* 占位标签文字（row1=用户名 row2=密码，输入框内左对齐） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1353 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3178
- 问题：登录按钮文字用 row6"账户"占位——★ 诚实边界：登录/注册 未逆向服务器协议） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1354 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3209
- 问题：LOGI("rs_draw_login：登录弹窗（datah_038 用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1355 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3339
- 问题：/* 按钮文字（row1=翻棋 row2=布局 row3=工兵 占位，按钮内居中；★ 诚实边界：
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1356 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3340
- 问题：规则文本简化版，row3 工兵标签占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1357 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3585
- 问题：/* 确定/取消（row9=确定 row4=蓝方 占位，★ 诚实边界：取消文字用 row4 蓝方占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1358 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3587
- 问题：int row_idx = (i == 3) ? 9 : 4;   /* 确定=row9，取消=row4 蓝方占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1359 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3623
- 问题：+ 桌号/密码占位标签 + 确定/取消 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1360 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3723
- 问题：/* ④ 桌号/密码 占位 EditLabel（矩形浅色底 + 占位文字，★ 诚实边界：不画真输入框） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1361 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3750
- 问题：/* 占位标签文字（row2=桌号 row3=密码，输入框内左对齐） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1362 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:3832
- 问题：LOGI("rs_draw_join：加入/创建游戏弹窗（datah_%s 桌号/密码占位标签 + 确定/取消，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1363 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4097
- 问题：/* 确定/取消 按钮文字（row10=确定 row6=蓝方 占位，★ 诚实边界：取消用 row6 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1364 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4113
- 问题：+ Info 文本框（战况统计复用占位） + 复盘/删除/返回 3 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1365 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4125
- 问题：/* ③ Info 文本框（浅色矩形占位，datah_050 Info 30,75,390,285 420 系 → 180,525-540,735 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1366 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4131
- 问题：/* ⑤ Info 文本框内 4 行占位标签（row1=蓝方 row2=红方 row3=步数 row4=回合，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1367 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4132
- 问题：战况统计语义复用；动态数字未烘焙 → 诚实边界占位标签 + LOGI 实际值） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1368 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4153
- 问题：LOGI("rs_draw_gameinfo：对局信息 Info 文本（蓝方=%d 红方=%d 步数=%d cur=%d/%d，占位标签未烘焙数字）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1369 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4182
- 问题：+ 滑条占位（不画真滑条，亮度 低/中/高 3 档循环切换区） + 确定/取消 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1370 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4193
- 问题：/* ③ 滑条占位条（Bright 区 152,601-568,649 720 基准，画 3 档占位色块 低/中/高，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1371 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4241
- 问题：+ Msg 文本框（诚实边界占位文本） + 清除/关闭 2 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1372 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4243
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1373 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4252
- 问题：/* ③ Msg 文本框（浅色矩形占位，datah_029 Msg 30,75,390,430 420 系 → 180,450-540,805 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1374 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4258
- 问题：/* ⑤ Msg 文本框内占位标签（row1=聊天 row2=占位 row3=帧 row4=边界 row5=未逆向，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1375 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4259
- 问题：诚实边界占位文本；动态聊天文本未烘焙 → 占位标签） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1376 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4264
- 问题：CHATLOG_ROW, 2, 360.0f, 540.0f, 150.0f, uc);   /* 占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1377 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4290
- 问题："诚实边界：联机聊天文本未逆向，src_rs 蓝牙聊天是占位帧 BT_T_CHAT）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1378 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4296
- 问题：+ Info 文本框（诚实边界占位文本） + 修改/帮助/返回 3 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1379 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4298
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1380 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4307
- 问题：/* ③ Info 文本框（浅色矩形占位，datah_031 Info 30,75,390,420 420 系 → 180,455-540,800 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1381 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4313
- 问题：/* ⑤ Info 文本框内占位标签（row1=账号 row2=昵称 row3=头像 row4=胜率 row5=边界 row6=未逆向，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1382 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4314
- 问题：诚实边界占位文本；动态账号信息未烘焙 → 占位标签） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1383 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4355
- 问题：图标图集 tex_imen_icons 8 槽占位映射） + 7 标签文字图集（drawdetail_text_atlas）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1384 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4357
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1385 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4359
- 问题：玩家资料/软件信息 图标=☰ 图标图集占位映射（原图集无专用图标）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1386 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4362
- 问题：玩家资料=0/软件信息=1 占位（原图集无专用图标，★ 诚实边界）） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1387 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4417
- 问题："战况统计/软件信息，面板 %.0f×%.0f，图标=☰ 图标图集占位映射，诚实边界：本地提和判定未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1388 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4425
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1389 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4470
- 问题："诚实边界：图像设置渲染参数未实现，仅 UI 状态记录）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1390 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4481
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1391 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4534
- 问题：/* 取消按钮：不画标签（原图集无"取消"标签，诚实边界占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1392 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:4573
- 问题："诚实边界（已部分解除）：夜间/对方棋子接渲染层，仅界面动画过渡帧未实现，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1393 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4585
- 问题：+ 8 显示占位标签（先手/后手/玩法/明暗棋/4 规则） + 开始/取消 2 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1394 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4588
- 问题：显示占位标签"蓝方/红方/布局/暗棋/开/开/开/开"，不画真数据）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1395 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4601
- 问题：/* ④ 8 显示占位标签（row1=蓝方 row2=红方 row3=布局 row4=暗棋 row5=开 row6=开 row7=开 row8=开） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1396 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4604
- 问题：/* 浅色占位标签区（非按钮，仅显示） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1397 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4620
- 问题：int row_idx = (i == 8) ? 9 : 8;   /* row9=开始 row8=开（占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1398 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4628
- 问题：LOGI("rs_draw_join065：加入游戏完整版弹窗（datah_065 8 显示占位标签 + 开始/取消，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1399 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4629
- 问题："诚实边界：联机开局配置=占位标签（蓝方/红方/布局/暗棋/开/开/开/开），"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1400 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4666
- 问题：else if (i == 2) vrow = ui_show_get() ? 8 : 7;   /* 明暗棋：明/暗（简化用 布局/翻棋 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1401 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4667
- 问题：else vrow = 4;   /* 游戏规则：text="设置"（row4 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1402 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4761
- 问题：+ 2 玩家占位标签（蓝方/红方） + 开始/换桌/离开 3 按钮 + 文字图集。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1403 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4764
- 问题：开始/换桌→边界对话框；2 玩家显示区=占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1404 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4776
- 问题：/* ④ 2 玩家占位标签（row1=蓝方 row2=红方，浅色矩形占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1405 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4802
- 问题：LOGI("rs_draw_room060：房间态弹窗（datah_060 2 玩家占位标签 + 开始/换桌/离开，面板 %.0f×%.0f，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1406 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4803
- 问题："诚实边界：联机房间=边界对话框（src_rs 仅蓝牙已实现），2 玩家显示区=占位标签）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1407 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4809
- 问题：+ Msg 输入框占位标签（"聊天输入（诚实边界：联机聊天文本未逆向）"）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1408 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4812
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，无真实文本渲染），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1409 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4813
- 问题：Msg 输入框简化为占位标签；发送→BT_T_CHAT 占位帧+边界对话框；记录→UI_CHATLOG（datah_029 已实现） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1410 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4822
- 问题：/* ③ Msg 输入框占位（浅色矩形，datah_063 Msg 区简化为占位标签） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1411 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4829
- 问题：/* ⑤ Msg 输入框内占位标签（row1=输入 row2=占位 row3=边界 row4=未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1412 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4834
- 问题：CHAT063_ROW, 2, 360.0f, UI_CHAT063_POPUP_Y0 + 130.0f, 150.0f, uc);  /* 占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1413 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4858
- 问题：LOGI("rs_draw_chat063：聊天完整版弹窗（datah_063 Msg 输入框（占位标签）+ 发送/记录/关闭 3 按钮，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1414 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:4859
- 问题："面板 %.0f×%.0f，诚实边界：联机聊天文本未逆向，发送=BT_T_CHAT 占位帧+边界对话框，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1415 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8584
- 问题：/* ③ Version 文本框（浅色矩形占位，datah_000 Version 140,90,390,120 400 系 → 300,590-550,620 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1416 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8587
- 问题：/* ④ 官网文本框（浅色矩形占位，datah_000 官网 145,135,390,165 400 系 → 305,635-550,665 720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1417 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8602
- 问题：LOGI("rs_draw_about：软件信息 Version=1.6.8 + 官网 www.cnvcs.com（占位标签，未烘焙动态版本字段）");
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1418 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8645
- 问题：★ 诚实边界：排行榜/消息数据未逆向→边界对话框，按钮标签=图集 row12/row13 占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1419 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8679
- 问题：/* Info 文本框占位（22,15,367,120 局部 + 45,620 → 67,635-412,740，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1420 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8697
- 问题：/* ③ datah_046 玩家资料（420×420 居中 +150,+430；标题 + Info 占位 + 返回 1 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1421 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8704
- 问题：/* Info 文本框占位（30,75,390,325 局部 + 150,430 → 180,505-540,755，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1422 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8717
- 问题："1 按钮，★ 诚实边界：玩家资料内容未逆向，占位标签）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1423 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8722
- 问题：/* ④ datah_047 玩家列表（420×305 居中 +150,+487.5；标题 + ListBox 占位 + 关闭 1 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1424 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8729
- 问题：/* ListBox 占位（20,75,400,200 局部 + 150,487.5 → 170,563-550,688，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1425 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8742
- 问题："1 按钮，★ 诚实边界：玩家列表未逆向，ListBox 占位）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1426 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8747
- 问题：/* ⑤ datah_049 对局详情（420×610 居中 +150,+335；标题 + ListBox 占位 + 关闭 1 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1427 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8754
- 问题：/* ListBox 占位（20,75,400,505 局部 + 150,335 → 170,410-550,915，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1428 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8767
- 问题："1 按钮，★ 诚实边界：对局详情内容未逆向，ListBox 占位）",
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1429 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8784
- 问题：/* ListBox 占位（20,75,430,505 局部 + 135,335 → 155,410-585,915，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1430 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8810
- 问题：LOGI("%s：450×610 对局列表弹窗出帧（ListBox 占位 + 清空/关闭 2 按钮，"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1431 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8831
- 问题：/* ⑧ datah_073 创建游戏（450×270 居中 +135,+505；标题 + 输入框占位 + 开始/取消 2 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1432 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8838
- 问题：/* Player1 输入框占位（30,85,420,120 局部 + 135,505 → 165,590-555,625，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1433 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8841
- 问题：/* Player2 输入框占位（30,130,420,165 局部 + 135,505 → 165,635-555,670，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1434 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8851
- 问题：/* 按钮文字：row8=人机对战/row10=联网对战 图集占位（★ 诚实边界：标签非精确"开始/取消"） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1435 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8915
- 问题：/* ⑫ datah_118 版本更新（450×360 居中 +135,+460；标题 + Info 占位 + 更新/取消 2 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1436 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8922
- 问题：/* Info 文本框占位（35,80,420,253 局部 + 135,460 → 170,540-555,713，720 基准） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1437 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8932
- 问题：/* 按钮文字：row11/row13 占位（★ 诚实边界：标签非精确"更新/取消"） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1438 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:8972
- 问题：/* ① 棋子占位方块（当前回放位置，诚实边界不画真棋子图） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1439 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9026
- 问题：/* ③ 步数标签（"复盘" 标题 row0 顶部 + 位置行 row6"第cur/n步"，文字图集占位标签；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1440 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9027
- 问题：动态 cur/n 数字未烘焙 → 诚实边界：只画"第步/结束"占位标签 + LOGI 实际步数） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1441 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9031
- 问题：rs_replay_text(rr, slot, 280.0f, 780.0f, 150.0f, 46.0f, 6, uc);     /* "第步" 位置行占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1442 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9037
- 问题：LOGI("rs_draw_replay：复盘回放渲染（datah_107 控制条 5 按钮 460×60@10,705 + 棋盘占位 + 步数标签 cur=%d/%d，jump 简化回开头/跳结尾，菜单简化退复盘）", cur, n);
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1443 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9751
- 问题：关闭=强制棋子立即到位，诚实边界：动画帧未实现，开关仅门控"是否画"）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1444 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9753
- 问题：- 棋子动画关（ui_imgset_piece=0）：棋子不画（诚实边界：过渡帧未实现，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1445 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9754
- 问题：关闭=棋子占位不画，对齐原版"关动画棋子静止"语义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1446 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9755
- 问题：int imgset_trans  = ui_imgset_cfg_get(0);   /* 界面动画（过渡帧未实现，门控用） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1447 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9757
- 问题：int imgset_piece  = ui_imgset_cfg_get(1);   /* 棋子动画：=0 时棋子不画（静止占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1448 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9787
- 问题：/* 棋子动画关 → 不画棋子（占位静止，诚实边界）；界面动画关 → 同上（无过渡帧可省） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1449 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9797
- 问题：- 棋子动画开关（ui_imgset_cfg_get(1)，默认 1=开）：=0 时棋子不画（静止占位）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1450 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/render.c:9798
- 问题：诚实边界：界面动画过渡帧未实现（开关仅门控）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1451 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9936
- 问题：/* ★ P1（datah_038 证据）：登录弹窗（用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1452 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9948
- 问题：/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗（桌号占位标签 + 确定/取消，面板 420×235 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1453 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9951
- 问题：/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗（密码占位标签 + 确定/取消，面板 420×235 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1454 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9985
- 问题：/* ★ P2（datah_065 证据）：加入游戏完整版弹窗（8 显示占位标签 + 开始/取消，面板 450×505 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1455 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9994
- 问题：/* ★ P2（datah_060 证据）：房间态弹窗（2 玩家占位标签 + 开始/换桌/离开，面板 450×352 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1456 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.c:9997
- 问题：/* ★ P2（datah_063 证据）：聊天完整版弹窗（Msg 输入框占位 + 发送/记录/关闭，面板 420×325 居中） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1457 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.h:87
- 问题：记录/聊天/清除/关闭/占位/帧/边界/未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1458 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render.h:111
- 问题：占位/边界/未逆向） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1459 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/render_obj.h:23
- 问题：int _vtable;   /* 对齐原版节点 [0]（重写填 0，仅占位） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1460 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/replay.c:9
- 问题：*   只收蓝方半区，对局进行中（已被红子占位）提取会镜像出错误红方分布
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1461 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/replay.c:49
- 问题：对局进行中（已被红子占位）提取会镜像出错误红方分布（实证 init_lay 非零
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1462 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/replay.h:10
- 问题：*   - 复盘态不画真实棋子图（渲染层复用棋盘现有占位范式）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1463 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:5
- 问题：*   人机对战（开局 STG_PLAY 蓝先手）/ 两人对战（占位）/ 联网对战（占位）/
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1464 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:6
- 问题：*   四国军棋（占位）/ 悔棋重开（restart）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1465 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:8
- 问题：* 诚实边界：两人/联网/四国 = 占位（日志记录，不实现联机 UI）；人机 = 直接开局。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1466 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:111
- 问题：case 2: LOGI("UI菜单：联网对战（占位，SPP socket 由 Java 宿主承担）"); break;
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1467 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:112
- 问题：case 3: LOGI("UI菜单：四国军棋（占位，未实现四国规则）"); break;
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1468 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:256
- 问题：4 人 15,155,215,215 → 260,680-460,740）。★ 诚实边界：四国=未实现 4 方规则。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1469 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:347
- 问题：★ 诚实边界：ListBox 滚动未实现（触摸 flaky），文本列表 6 条截断 + 选中高亮。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1470 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:409
- 问题：简化小方块 6×5 占位（有子=蓝/红方块，真棋子图未画）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1471 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:410
- 问题：面板 400×475 居中（偏移+160,+402.5）。★ 诚实边界：原版 Ok2/Cancel2 重叠备用按钮未实现。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1472 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:467
- 问题：/* ★ P1（datah_038 证据）：登录弹窗 6 控件（用户名/密码占位标签 + 登录/注册/游客/返回）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1473 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:475
- 问题：★ 诚实边界：原版登录=账号系统（服务器协议未逆向）；输入框简化为占位标签
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1474 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:479
- 问题：{ 274.0f, 578.0f, 519.0f, 623.0f, 0 },   /* 用户名 EditLabel 占位标签（130,80,375,125） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1475 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:480
- 问题：{ 274.0f, 638.0f, 519.0f, 683.0f, 1 },   /* 密码 EditLabel 占位标签（130,140,375,185） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1476 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:574
- 问题：{ 270.0f, 602.5f, 510.0f, 647.5f, 0 },   /* 桌号 Table（120,80,360,125）→ 占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1477 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:607
- 问题：{ 250.0f, 602.5f, 470.0f, 647.5f, 0 },   /* 密码 Pwd（100,80,320,125）→ 占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1478 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:729
- 问题：★ 诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1479 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:732
- 问题：{ 187.0f, 825.0f, 343.0f, 885.0f, 0 },   /* 清除 Clear（37,450,193,510）→ 清本地占位记录 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1480 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:758
- 问题：★ 诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1481 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:790
- 问题：业务：提和/认输/聊天 = 发 BT 帧 + 边界对话框（本地提和判定未实现，诚实边界）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1482 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:792
- 问题：★ 诚实边界：原版提和需对端应答（判定未实现），src_rs 离线单机无对端，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1483 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:829
- 问题：★ 诚实边界：图像设置渲染参数未实现（src_rs 无对应渲染参数），开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1484 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:858
- 问题：★ 诚实边界：图像设置渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1485 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:888
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1486 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:922
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1487 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:956
- 问题：业务：联机"加入游戏"完整版（8 项显示用 TransLabel 占位标签
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1488 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:959
- 问题：显示占位标签，不画真数据，触摸 flaky 下输入框无意义）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1489 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:964
- 问题：{ 275.0f, 473.0f, 555.0f, 508.0f, 0 },   /* 先手 Player1（140,85,420,120）→ 占位标签"蓝方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1490 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:965
- 问题：{ 275.0f, 513.0f, 555.0f, 548.0f, 1 },   /* 后手 Player2（140,125,420,160）→ 占位标签"红方" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1491 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:966
- 问题：{ 275.0f, 553.0f, 555.0f, 588.0f, 2 },   /* 玩法 Lay（140,165,420,200）→ 占位标签"布局" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1492 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:967
- 问题：{ 275.0f, 593.0f, 555.0f, 628.0f, 3 },   /* 明暗棋 Show（140,205,420,240）→ 占位标签"暗棋" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1493 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:968
- 问题：{ 275.0f, 633.0f, 555.0f, 668.0f, 4 },   /* 和棋 Draw（140,245,420,280）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1494 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:969
- 问题：{ 275.0f, 673.0f, 555.0f, 708.0f, 5 },   /* 大本营 CmdSafe（140,285,420,320）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1495 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:970
- 问题：{ 275.0f, 713.0f, 555.0f, 748.0f, 6 },   /* 吃地雷 BingLei（140,325,420,360）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1496 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:971
- 问题：{ 275.0f, 753.0f, 555.0f, 788.0f, 7 },   /* 吃军旗 BingQi（140,365,420,400）→ 占位标签"开" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1497 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1071
- 问题：简化为占位标签"蓝方/红方"）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1498 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1074
- 问题：★ 诚实边界：2 玩家显示区 = 占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1499 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1103
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1500 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1104
- 问题：业务：联机"聊天"完整版，发送→BT_T_CHAT 占位帧+边界对话框，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1501 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1106
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1502 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1110
- 问题：{ 175.0f, 723.0f, 285.0f, 783.0f, 0 },   /* 发送 Send（25,245,135,305）→ BT_T_CHAT 占位帧 + 边界对话框 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1503 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1165
- 问题：★ 诚实边界：排行榜/消息数据未逆向，按钮→边界对话框占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1504 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1209
- 问题：★ 诚实边界：玩家资料内容未逆向（账号系统未实现），Info 占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1505 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1230
- 问题：★ 诚实边界：玩家列表未逆向，ListBox 占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1506 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.c:1251
- 问题：★ 诚实边界：对局详情内容未逆向，ListBox 占位标签。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1507 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:20
- 问题：诚实边界：选人数 → 边界对话框（4 方规则未实现）→ 回主菜单 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1508 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:36
- 问题：30 格棋子预览（简化小方块占位）+ 载入/删除/取消 3 按钮（原版 datah_016 400×475 面板居中）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1509 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:38
- 问题：诚实边界：30 格 PiecePic 真棋子图未画，小方块占位（有子=蓝/红） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1510 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:40
- 问题：用户名/密码占位标签（EditLabel，不画真输入框）+ 登录/注册/游客/返回 4 按钮
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1511 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:43
- 问题：Junqi.* 登录/注册 20+ 事件未实现）；游客=本地单机（无账号，唯一真生效路径）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1512 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:71
- 问题：桌号=占位标签（不画真输入框）。入口：datah_072 方式弹窗"Wi-Fi 联机"
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1513 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:78
- 问题：密码=占位标签（不画真输入框）。入口：setprop jq.create 测试钩子。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1514 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:105
- 问题：占位帧（BT_T_CHAT，对端无文本渲染）；Msg 文本框显示诚实边界占位文本 +
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1515 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:106
- 问题：清除 → 清本地占位记录 + 关闭 → 回对局。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1516 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:110
- 问题：Info 文本=诚实边界占位（账号系统未逆向，离线单机无账号）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1517 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:111
- 问题：修改 → 诚实边界对话框（"修改资料需账号系统，未实现"）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1518 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:122
- 问题：值 1=进 / 0=退）。★ 诚实边界：图像设置=渲染参数未实现，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1519 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:140
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1520 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:150
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1521 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:155
- 问题：8 项显示用 TransLabel（非按钮，显示当前值占位标签"蓝方/红方/
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1522 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:163
- 问题：无联机开局逻辑，显示占位标签，不画真数据） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1523 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:188
- 问题：是 TransText 显示区（非按钮），简化为占位标签；开始/换桌/离开
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1524 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:195
- 问题：★ 诚实边界：2 玩家显示区 = 占位标签（不画真数据，触摸 flaky
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1525 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:200
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1526 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:201
- 问题：未逆向）"；发送 → BT_T_CHAT 占位帧 + 边界对话框；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1527 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:207
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1528 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:602
- 问题：★ 诚实边界：本地提和判定未实现（src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1529 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:612
- 问题：★ 诚实边界：排行榜/消息数据未逆向，按钮→边界对话框占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1530 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:621
- 问题：（标题"玩家资料" + Info 文本框占位 + 返回 1 按钮，原版 420×420
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1531 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:625
- 问题：★ 诚实边界：玩家资料内容未逆向（账号系统未实现），占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1532 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:627
- 问题：（标题"玩家列表" + ListBox 占位 + 关闭 1 按钮，原版 420×305
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1533 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:631
- 问题：★ 诚实边界：玩家列表未逆向，ListBox=占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1534 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:633
- 问题：（标题 + ListBox 占位 + 关闭 1 按钮，原版 420×610 面板居中
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1535 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:637
- 问题：★ 诚实边界：对局详情内容未逆向，ListBox=占位标签 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1536 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:639
- 问题：（标题"我的对局" + ListBox 占位 + 清空/关闭 2 按钮，原版 450×610
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1537 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:647
- 问题：（标题"历史对局" + ListBox 占位 + 清空/关闭 2 按钮，原版 450×610
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1538 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:654
- 问题：（标题"创建游戏" + Player1/Player2 输入框占位 + 开始/取消 2 按钮，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1539 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:678
- 问题：（标题"历史对局" + ListBox 占位 + 清空/关闭 2 按钮，原版 450×610
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1540 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:685
- 问题：（标题"版本更新" + Info 文本框占位 + 更新/取消 2 按钮，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1541 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:774
- 问题：void ui_menu_action_log(int action);  /* 按钮动作日志（占位语义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1542 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:790
- 问题：action：0=2 人 1=3 人 2=4 人（默认 4）。★ 诚实边界：四国 4 方规则未实现（src_rs=10×9 两人军棋）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1543 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:853
- 问题：★ 诚实边界： ListBox 滚动未实现（触摸 flaky），文本列表 6 条截断 + 选中高亮。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1544 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:888
- 问题：20,75,380,360 区占位）+ 载入/删除/取消 3 按钮（原版 Ok 25,390,129,450 → 185,793-289,853；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1545 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:892
- 问题：★ 诚实边界： 30 格 PiecePic 真棋子图未画（小方块占位，有子=蓝/红方块）；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1546 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:893
- 问题：原版 Ok2/Cancel2 重叠备用显示按钮未实现。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1547 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:939
- 问题：/* ★ P1（datah_038 证据）：登录弹窗——用户名/密码占位标签 + 登录/注册/游客/返回 4 按钮。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1548 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:950
- 问题：Junqi.* 登录/注册 20+ 事件未实现）；用户名/密码 输入框简化为占位标签（触摸 flaky
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1549 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1008
- 问题：/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗——桌号输入（占位标签）+ 确定/取消。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1550 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1012
- 问题：action：0=桌号（占位标签）1=确定（诚实边界对话框）2=取消（回主菜单）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1551 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1029
- 问题：/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗——密码输入（占位标签）+ 确定/取消。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1552 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1033
- 问题：action：0=密码（占位标签）1=确定（诚实边界对话框）2=取消（回主菜单）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1553 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1130
- 问题：action：0=清除（清本地占位记录）1=关闭（回对局）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1554 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1132
- 问题：src_rs 蓝牙聊天是占位帧（BT_T_CHAT，对端无文本渲染）；Msg 文本框显示诚实边界占位文本。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1555 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1152
- 问题：action：0=修改（诚实边界对话框：修改资料需账号系统未实现）1=帮助（显示帮助文本）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1556 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1157
- 问题：边界占位文本；修改=边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1557 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1186
- 问题：★ 诚实边界：本地提和判定未实现（原版需对端应答，src_rs 离线单机无对端），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1558 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1210
- 问题：★ 诚实边界：图像设置入口指向 UI_IMGSET7（图像设置渲染参数未实现，仅 UI 状态记录）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1559 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1233
- 问题：★ 诚实边界：图像设置入口指向 UI_IMGSET8（图像设置渲染参数未实现，仅 UI 状态记录）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1560 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1262
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1561 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1291
- 问题：★ 诚实边界：图像/夜间模式 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1562 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1320
- 问题：action：0-7=8 显示项（先手/后手/玩法/明暗棋/和棋/大本营/吃地雷/吃军旗，TransLabel 占位标签
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1563 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1321
- 问题：非按钮，显示当前值占位"蓝方/红方/布局/暗棋/开/开/开/开"）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1564 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1326
- 问题：显示占位标签，不画真数据；触摸 flaky 下输入框无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1565 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1408
- 问题：简化为占位标签"蓝方/红方"）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1566 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1413
- 问题：★ 诚实边界：2 玩家显示区 = 占位标签（不画真数据，触摸 flaky 下房间列表无意义） */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1567 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1434
- 问题：Msg 输入框简化为占位标签"聊天输入（诚实边界：联机聊天文本未逆向）"。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1568 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1435
- 问题：action：0=发送（→ BT_T_CHAT 占位帧 + 边界对话框）
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1569 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:1439
- 问题：★ 诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1570 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu.h:2589
- 问题：占位标签 + 边界对话框，不包装成"已实现" */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1571 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:20
- 问题：/* ★ P1（datah_016/017/020）：布局存档 3 弹窗按钮桩（桌面 headless 无渲染，几何表返回空占位
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1572 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:65
- 问题：/* ★ P1（datah_039/107）：复盘回放控制条 5 按钮桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1573 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:78
- 问题：/* ★ P1（datah_038）：登录弹窗 6 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1574 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:92
- 问题：/* ★ P2（datah_037 证据）：规则文本弹窗 3 按钮桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1575 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:105
- 问题：/* ★ P2（datah_104 证据）：新局完整版弹窗 5 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1576 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:119
- 问题：/* ★ P2（datah_034 证据）：联机"加入游戏"弹窗 3 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1577 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:132
- 问题：/* ★ P2（datah_035 证据）：联机"创建游戏"弹窗 3 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1578 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:145
- 问题：/* ★ P2（datah_061 证据）：新局完整版（完整版补充）弹窗 6 控件桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1579 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:184
- 问题：诚实边界：原版联机聊天文本（Junqi.Chat*）未逆向，src_rs 蓝牙聊天是占位帧（BT_T_CHAT）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1580 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:195
- 问题：诚实边界：原版个人资料（昵称/头像/胜率）+ 账号系统未逆向，Info 文本框=诚实边界占位。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1581 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:206
- 问题：/* ★ P2（datah_042 证据）：提和详情弹窗 7 图标桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1582 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:208
- 问题：诚实边界：本地提和判定未实现（原版需对端应答），提和/认输走 BT 帧 + 边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1583 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:222
- 问题：/* ★ P2（datah_003/004 证据）：设置入口 3/2 按钮桩（几何表返回 720 基准占位供
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1584 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:224
- 问题：诚实边界：图像设置渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1585 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:246
- 问题：诚实边界：图像/夜间模式渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1586 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:271
- 问题：诚实边界：联机开局配置（src_rs 无联机开局逻辑），8 项显示=占位标签，
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1587 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:331
- 问题：诚实边界：联机聊天文本未逆向（src_rs 蓝牙聊天是占位帧 BT_T_CHAT），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1588 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:332
- 问题：发送→占位帧+边界对话框。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1589 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:346
- 问题：反馈按钮→诚实边界对话框（几何表返回 720 基准占位供一致性/evverify 链接；
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1590 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_menu_stub.c:359
- 问题：桌面桩与 ui_menu.c 真实几何表同步（诚实边界占位不包装成"已实现"）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1591 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state.c:7
- 问题：* 两人/联网/四国 = 占位（日志记录，未实现联机 UI/四国规则，诚实边界）。
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1592 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state.c:186
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1593 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_state.c:298
- 问题：文本列表 6 条截断 + 小方块占位预览，诚实边界见 AUDIT_LAYOUT_ALIGN §十六）。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1594 [P3] 未完成标记（占位）
- 探针：`todo`
- 位置：src_rs/ui_state.h:7
- 问题：void ui_on_menu_action(int action);   /* 菜单按钮点击：0人机→选项弹窗 4重开 其它占位 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1595 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state.h:63
- 问题：★ 诚实边界：图像/夜间模式/对方棋子 = 渲染层未实现（src_rs 无对应渲染参数），
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1596 [P3] 未完成标记（未实现）
- 探针：`todo`
- 位置：src_rs/ui_state_stub.c:52
- 问题：诚实边界：图像/夜间模式渲染参数未实现，开关仅 UI 状态记录。 */
- 依据：源码内显式未完成标记，可能被误当完成
- 修法：补实现或在交付说明中列为已知边界

## AUD-A-1597 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (150,455)-(255,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1598 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (255,455)-(360,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1599 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (360,455)-(465,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1600 [P3] UI_D019 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (465,455)-(570,590) 面板 (135,430)-(585,580)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1601 [P3] UI_D079 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (270,900)-(450,960) 面板 (140,365)-(360,705)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1602 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (150,588)-(255,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1603 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (255,588)-(360,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1604 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (360,588)-(465,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1605 [P3] UI_D083 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (465,588)-(570,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1606 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (150,588)-(255,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1607 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (255,588)-(360,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1608 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (360,588)-(465,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1609 [P3] UI_D099 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (465,588)-(570,724) 面板 (135,564)-(585,717)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

## AUD-A-1610 [P3] UI_D123 按钮超出面板（原版排版如此，已如实还原）
- 探针：`ui-geom`
- 位置：src_rs/ui_menu.c
- 问题：按钮 (307,610)-(382,678) 面板 (300,602)-(420,677)
- 依据：与原版 XML 绝对坐标一致 → 非我方缺陷
- 修法：无需修（原版 UI 本身如此）

