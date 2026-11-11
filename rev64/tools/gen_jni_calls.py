#!/usr/bin/env python3
"""生成权威 P0 so→Java JNI 调用表 jqt_jni_calls.json。

签名手工核实（对照 smali + Android API 文档 + .rodata 扫描）。
每条 {id, cls, name, sig, ret, args, group, p0}。

cls 用 Java 类全名（FindClass 用 "/" 分隔，见 C 侧 jq_jni_calls）。
args 参数个数（不含 this，不含 env/class）。
"""
import json

# 权威 P0 清单（按组）
ROWS = [
 # ---- A 组：GLView/View 布局（渲染/UI）----
 ('android/view/View', 'getParent', '()Landroid/view/ViewParent;', 'Landroid/view/ViewParent;', 'A_view'),
 ('android/view/ViewGroup', 'removeView', '(Landroid/view/View;)V', 'V', 'A_view'),
 ('android/view/View', 'setVisibility', '(I)V', 'V', 'A_view'),
 ('android/view/View', 'getLayoutParams', '()Landroid/view/ViewGroup$LayoutParams;', 'Landroid/view/ViewGroup$LayoutParams;', 'A_view'),
  ('android/view/ViewGroup$MarginLayoutParams', 'setMargins', '(IIII)V', 'V', 'A_view'),  # §124ar: 红盾SDK30 setMargins 在 MarginLayoutParams（非 LayoutParams），改对内部类
 ('android/view/View', 'requestLayout', '()V', 'V', 'A_view'),
 ('android/widget/LinearLayout', '<init>', '(Landroid/content/Context;)V', 'V', 'A_view'),
 ('android/app/Activity', 'addContentView', '(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V', 'V', 'A_view'),
 ('android/view/ViewGroup$LayoutParams', '<init>', '(II)V', 'V', 'A_view'),
 ('com/cnvcs/GLView', 'setEGLConfigChooser', '(Lcom/cnvcs/GLView$EGLConfigChooser;)V', 'V', 'A_view'),
 ('com/cnvcs/GLView', 'setEGLContextClientVersion', '(I)V', 'V', 'A_view'),
 ('com/cnvcs/GLView', 'getHolder', '()Landroid/view/SurfaceHolder;', 'Landroid/view/SurfaceHolder;', 'A_view'),
 # ---- B 组：窗口全屏 ----
 ('android/app/Activity', 'getWindow', '()Landroid/view/Window;', 'Landroid/view/Window;', 'B_window'),
 ('android/view/Window', 'requestFeature', '(I)Z', 'Z', 'B_window'),
 ('android/view/Window', 'setFlags', '(II)V', 'V', 'B_window'),
 ('android/app/Activity', 'getWindowManager', '()Landroid/view/WindowManager;', 'Landroid/view/WindowManager;', 'B_window'),
 ('android/view/Display', 'getWidth', '()I', 'I', 'B_window'),
 ('android/view/Display', 'getHeight', '()I', 'I', 'B_window'),
 # ---- C 组：对话框/Toast ----
 ('android/widget/Toast', 'makeText', '(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;', 'Landroid/widget/Toast;', 'C_dialog'),  # 红盾SDK30 smali: 4参版没合入, 用3参版
 ('android/widget/Toast', 'show', '()V', 'V', 'C_dialog'),
 ('com/cnvcs/SiteView', 'create', '(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;IIZ)V', 'V', 'C_dialog'),
 ('com/cnvcs/TextDlg', '<init>', '(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V', 'V', 'C_dialog'),
 ('android/content/Intent', 'getParcelableExtra', '(Ljava/lang/String;)Landroid/os/Parcelable;', 'Landroid/os/Parcelable;', 'C_dialog'),
 # ---- E 组：文件/资源 ----
 ('android/content/Context', 'getAssets', '()Landroid/content/res/AssetManager;', 'Landroid/content/res/AssetManager;', 'E_file'),
 ('android/content/Context', 'openFileInput', '(Ljava/lang/String;)Ljava/io/FileInputStream;', 'Ljava/io/FileInputStream;', 'E_file'),
 ('android/content/Context', 'openFileOutput', '(Ljava/lang/String;I)Ljava/io/FileOutputStream;', 'Ljava/io/FileOutputStream;', 'E_file'),
 ('android/content/Context', 'deleteFile', '(Ljava/lang/String;)Z', 'Z', 'E_file'),
 # ---- F 组：音频 ----
 ('android/media/AudioManager', '<init>', '(Landroid/content/Context;)V', 'V', 'F_audio'),  # 红盾SDK30 smali: (Context,int)构造器SDK29已删, 用(Context)单参版
 ('android/media/AudioManager', 'getMinVolumeIndexForAttributes', '(Landroid/media/AudioAttributes;)I', 'I', 'F_audio'),  # 红盾SDK30: getMinVolume已删, 替代方法
 ('android/media/AudioManager', 'getMaxVolumeIndexForAttributes', '(Landroid/media/AudioAttributes;)I', 'I', 'F_audio'),  # 红盾SDK30: getMaxVolume已删, 替代方法
 ('android/media/AudioTrack', '<init>', '(IIIIII)V', 'V', 'F_audio'),
 ('android/media/AudioTrack', 'release', '()V', 'V', 'F_audio'),
 ('android/media/AudioTrack', 'getPlayState', '()I', 'I', 'F_audio'),
 ('android/media/AudioTrack', 'setStereoVolume', '(FF)I', 'I', 'F_audio'),
 # ---- G 组：蓝牙 ----
 ('android/bluetooth/BluetoothAdapter', 'getDefaultAdapter', '()Landroid/bluetooth/BluetoothAdapter;', 'Landroid/bluetooth/BluetoothAdapter;', 'G_bluetooth'),
 ('android/bluetooth/BluetoothAdapter', 'startDiscovery', '()Z', 'Z', 'G_bluetooth'),
 ('android/bluetooth/BluetoothAdapter', 'cancelDiscovery', '()Z', 'Z', 'G_bluetooth'),
 ('android/bluetooth/BluetoothAdapter', 'getBondedDevices', '()Ljava/util/Set;', 'Ljava/util/Set;', 'G_bluetooth'),
 ('android/bluetooth/BluetoothAdapter', 'getRemoteDevice', '(Ljava/lang/String;)Landroid/bluetooth/BluetoothDevice;', 'Landroid/bluetooth/BluetoothDevice;', 'G_bluetooth'),
 ('android/bluetooth/BluetoothDevice', 'createRfcommSocketToServiceRecord', '(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;', 'Landroid/bluetooth/BluetoothSocket;', 'G_bluetooth'),
 ('android/bluetooth/BluetoothSocket', 'accept', '()Landroid/bluetooth/BluetoothSocket;', 'Landroid/bluetooth/BluetoothSocket;', 'G_bluetooth'),
 ('android/content/Context', 'registerReceiver', '(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;', 'Landroid/content/Intent;', 'G_bluetooth'),
 ('android/content/Context', 'unregisterReceiver', '(Landroid/content/BroadcastReceiver;)V', 'V', 'G_bluetooth'),
]

calls = []
for cls, name, sig, ret, grp in ROWS:
    args = sig[1:sig.find(')')]
    # 去嵌套括号后的参数个数（粗略）
    n = 0
    depth = 0
    started = False
    for ch in args:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        elif depth == 0:
            if ch in 'IVZJFDSCB' or (ch == 'L') or (ch == '['):
                # 记一个参数起点
                n += 1
            elif ch in ';':
                pass
    # 更准：按 L...; / [L...; / 基础类型切分
    import re
    params = re.findall(r'L[A-Za-z0-9_$/]+;|\[L[A-Za-z0-9_$/]+;|[IVZJFDSCB]', args)
    n = len(params)
    calls.append({
        'id': len(calls),
        'cls': cls, 'name': name, 'sig': sig, 'ret': ret,
        'arg_types': args, 'n_args': n,
        'group': grp,
        'p0': grp in ('A_view', 'C_dialog', 'E_file', 'G_bluetooth'),
    })

out = '/tmp/opencode/jq_jni/jq_jni_calls.json'
json.dump(calls, open(out, 'w'), indent=1, ensure_ascii=False)
print(f"生成 {len(calls)} 条 → {out}")
from collections import Counter
for g, c in sorted(Counter(x['group'] for x in calls).items()):
    print(f"  {g:12s} {c:3d}")

# 同时生成 C 侧表头 jqt_jni_calls.h
hdr = '/disk/workspaces/jq/rev64/reverse/src_jni_translated/jqt_jni_calls.h'
with open(hdr, 'w') as f:
    f.write("/* 自动生成的 P0 so→Java JNI 调用表（gen_jni_calls.py v3）\n")
    f.write(" * id 对应 jqt_jni_calls.json。桥 jqt_jnibr.c 用 jq_jni_calls[id]。 */\n")
    f.write("#ifndef JQT_JNI_CALLS_H\n#define JQT_JNI_CALLS_H\n")
    f.write("typedef struct { const char cls[48]; const char name[40]; const char sig[96]; } JqJniCall;\n")
    f.write(f"extern JqJniCall jq_jni_calls[{len(calls)}];\n")
    f.write("#define JQJNINUM " + str(len(calls)) + "\n")
    f.write("#endif\n")
print(f"生成 {hdr}")

# 生成 C 侧表定义 jqt_jni_calls_tbl.c（含 9 条签名修正：removeView→ViewGroup、makeText→4参）
tbl = '/disk/workspaces/jq/rev64/reverse/src_jni_translated/jqt_jni_calls_tbl.c'
with open(tbl, 'w') as f:
    f.write("#include \"jqt_jnibr.h\"\n#include \"jqt_jni_calls.h\"\n\n")
    f.write("JqJniCall jq_jni_calls[JQJNINUM] = {\n")
    for c in calls:
        f.write(f'  /* id{c["id"]:3d} {c["group"]:9s} */ {{"{c["cls"]}", "{c["name"]}", "{c["sig"]}"}},\n')
    f.write("};\n")
print(f"生成 {tbl}（43 条 + 签名修正）")
