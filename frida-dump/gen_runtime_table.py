#!/usr/bin/env python3
"""把 redroid dump 的 jni_runtime_values.json 生成 C 注入表 → 拼进 jni_aligned.c。
用法：python3 gen_runtime_table.py [jni_runtime_values.json]
生成后重新 build_aligned.sh 即可让 14 入口读运行时值。"""
import sys, json, os

J = sys.argv[1] if len(sys.argv)>1 else '/disk/workspaces/jq/frida-dump/jni_runtime_values.json'
C = '/disk/workspaces/jq/rev64/reverse/src_jni_translated/jni_aligned.c'

vals = json.load(open(J))
print('读 %d 个运行时地址值' % len(vals))
# 生成 C 表（替换 jni_aligned.c 里的占位 g_jq_runtime 表）
lines = ['    {0x%06x, 0x%08x}, /* off=0x%06x */' % (v['off'], v.get('runtime_value') or 0, v['off'])
         for v in vals]
table = '\n'.join(lines)

c = open(C).read()
# 替换占位注释 + 空表
import re
c2, n = re.subn(r'\{ *0x[0-9a-f]{4,6}, 0x[0-9a-f]{0,8x}\}.*?(?=\};)\n\};', '', c, count=1, flags=re.DOTALL)
# 简单：定位 "static const struct jq_runtime_off g_jq_runtime[JQ_GLOB_COUNT] = {" 后的内容替换
marker = 'static const struct jq_runtime_off g_jq_runtime[JQ_GLOB_COUNT] = {'
i = c2.find(marker)
if i<0: i = c.find(marker); c2=c
j = c2.find(marker)
end = c2.find('};', j)
c2 = c2[:j+len(marker)] + '\n' + table + '\n' + c2[end+2:]
open(C,'w').write(c2)
print('已注入 %d 项到 %s' % (len(vals), C))
print('下一步：bash rev64/build_aligned.sh 重新编译')
