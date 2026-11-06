#!/usr/bin/env python3
"""spak_strings.py — 原版界面字符串表索引（ch02 N45 门牌号→房间真值）

原版所有界面文案真值在 SPAK 的 data_config_string.xml（StringConfig/StringSet/String）。
我方此前部分文案是「自编」（如返回键确认弹窗的"退出/继续"），与原版不一致 —— 本工具提供权威查询。

用法:
  python3 tools/spak_strings.py --list            # 全部字符串（编号）
  python3 tools/spak_strings.py --set Common       # 按组列出
  python3 tools/spak_strings.py --get ExitConfirm  # 查单条
  python3 tools/spak_strings.py --grep 退出          # 按关键词搜
"""
import os,re,sys,json

BASE=os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","dist","spak_res")
SRC=os.path.join(BASE,"data_config_string.xml")
IDX=os.path.join(BASE,"ui_string_index.json")

def parse():
    raw=open(SRC,"rb").read()
    try: txt=raw.decode("gbk")
    except UnicodeDecodeError: txt=raw.decode("utf-8","ignore")
    out={}
    group=None
    for gm in re.finditer(r'<StringSet\s+name="([^"]+)"\s*>(.*?)</StringSet>', txt, re.S):
        group=gm.group(1)
        for sm in re.finditer(r'<String\s+name="([^"]+)"\s+value="([^"]*)"', gm.group(2)):
            out[sm.group(1)]={"value":sm.group(2),"group":group}
    return out

def load():
    idx=parse()
    if os.path.exists(IDX):
        try:
            saved=json.load(open(IDX))
            for k,v in saved.items(): idx.setdefault(k,v)
        except Exception: pass
    return idx

def save(idx):
    json.dump(idx,open(IDX,"w"),ensure_ascii=False,indent=1)

def main():
    a=sys.argv[1:]
    if not a: print(__doc__); return 1
    idx=load()
    if a[0]=="--list":
        for i,(k,v) in enumerate(sorted(idx.items()),1):
            print(f"{i:3d} [{v['group']}] {k} = {v['value']}")
        print(f"共 {len(idx)} 条（已写索引 {IDX}）")
    elif a[0]=="--set":
        for k,v in sorted(idx.items()):
            if v["group"]==a[1]: print(f"  {k} = {v['value']}")
    elif a[0]=="--get":
        v=idx.get(a[1])
        print(f"{a[1]} = {v['value'] if v else '(未找到)'}" + (f"  [{v['group']}]" if v else ""))
    elif a[0]=="--grep":
        for k,v in sorted(idx.items()):
            if a[1] in v["value"] or a[1] in k:
                print(f"  [{v['group']}] {k} = {v['value']}")
    save(idx)
    return 0

if __name__=="__main__":
    sys.exit(main())
