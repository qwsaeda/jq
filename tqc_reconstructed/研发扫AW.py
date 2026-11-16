"""研发扫AW · 研发部门扫货插件(外挂式)

挂载点:包装「第五步」模块级函数 _ensure_in_lobby ——
军需兑换结束、交易行出售开始前,游戏已确认回到主大厅时触发本步。

流程:大厅 → 特勤处 → 部门页签 → 军需处子页签 → 研发部门卡片 → 等6秒校验
「研发成果刷新」→ 整区 OCR 找 .338 Lap Mag AP → 命中读卡片右上角 ×N 发数
→ 一键补齐 → 购买 → 兑换 → 确认 → 识别到成功提示才回传云端(云端累计次数)
→ ESC 回大厅(大厅特征校验)→ 交还交易行原流程。

档位:12点档=12:00~19:59;20点档=20:00~次日11:59(跨零点,标记键=刷新日期+档位)。
扫完(命中/未命中)写档位标记;失败(没钱/异常/超时)不写,下趟重试;顺路执行不补跑。

安全:不改任何原方法内部字节码;dry_run=True 时只识别+日志,不真实购买/不上传/不写标记。
"""
import importlib
import json
import os
import re
import sqlite3
import sys
import time
import urllib.request

from datetime import date, timedelta

STEP_NAME = '研发扫AW'

STEP_KEY = 'step_aw'

_ITEM_NAME = '.338 Lap Mag AP'


_默认配置 = {
    'enable': True, 'dry_run': True, 'ref_w': 1027,
    'ref_h': 768, 'total_timeout': 120, 'esc_vk': '0x1B',
    'poll_ms': 200, 'lobby_page_kw': ['无目标', '行前备战'], 'lobby_page_kw_need': 1,
    'tqc_page_kw': ['技术中心', '工作台', '制药台', '防具台'], 'tqc_page_kw_need': 2, 'dept_page_kw': ['震荡危情', '跨越生命线', '阿萨拉的枪匠', '全副武装', '矩阵意志'],
    'dept_page_kw_need': 2, 'qxs_page_kw': ['战斗部门', '医疗部门', '后勤部门', '战术部门', '研发部门'], 'qxs_page_kw_need': 2,
    'rd_page_kw': '研发成果刷新', 'dept_tab_x': 311, 'dept_tab_y': 31,
    'dept_tab_wait': 1.5, 'qxs_tab_x': 243, 'qxs_tab_y': 56,
    'qxs_tab_wait': 1.5, 'rd_card_x': 857, 'rd_card_y': 428,
    'rd_card_wait': 6.0, 'nav_max_rounds': 8, 'target_strong': '338lapmag',
    'target_weak': '338', 'target_confirm': ['lap', 'mag', 'ap'], 'qty_regex': '[x×]\\s*(\\d{1,3})',
    'qty_max_dx': 170, 'qty_above_dy': 200, 'default_qty': 1,
    'fill_btn_x': 870, 'fill_btn_y': 686, 'pay_x': 518,
    'pay_y': 493, 'exchange_x': 867, 'exchange_y': 684,
    'confirm_x': 585, 'confirm_y': 459, 'detail_wait': 4.0,
    'popup_wait': 3.0, 'btn_become_wait': 8.0, 'success_wait': 4.0,
    'btn_roi_box': [0.6, 0.78, 1.0, 1.0], 'popup_roi_box': [0.18, 0.27, 0.88, 0.92], 'fill_kw': '补齐',
    'exchange_kw': '兑换', 'success_kw': ['成功', '获得'], 'error_kw': ['不足', '失败', '无法兑换', '已达上限', '次数已达'],
    'confirm_kw': ['确认', '确定'], 'return_lobby_max_esc': 1, 'return_lobby_esc_wait': 0.8,
    'cloud_hit_path': '/api/client/account/rd-aw-hit', 'cloud_timeout': 10,
}
def _log(ctx, msg, level='INFO'):
    try:
        ctx.log(STEP_NAME, msg, level)
    except Exception:
        pass

def _解析vk(param):
    """把 '0x1B' / 27 / '27' 统一成 int VK code。"""
    if param is None:
        return None
    if isinstance(param, int):
        return param
    s = str(param).strip()
    if s.lower().startswith('0x'):
        return int(s, 16)
    try:
        return int(s)
    except Exception:
        return None

def _本地配置文件() -> str:
    """本地测试配置路径(TQC_AW_LOCAL 环境变量,相对路径按 __file__ 解析)。"""
    name = os.getenv('TQC_AW_LOCAL') or 'aw_scan_local.json'
    if os.path.isabs(name):
        p = name
    else:
        p = os.path.join(os.path.dirname(os.path.abspath(__file__)), name)
    return p if os.path.isfile(p) else ''

_QTY_RE = None

def _加载配置(ctx) -> dict:
    """本地测试配置优先;否则云端 step_aw;都没有用内置默认。"""
    global _QTY_RE
    _QTY_RE = None
    cfg = dict(_默认配置)
    lp = _本地配置文件()
    if lp:
        try:
            with open(lp, 'r', encoding='utf-8-sig') as f:
                data = json.load(f)
            if not isinstance(data, dict):
                raise ValueError('顶层必须是 JSON 对象')
            for k, v in data.items():
                if k in cfg and v is not None:
                    cfg[k] = v
            _log(ctx, '⚠ 本地测试配置生效:%s(%d项),已忽略云端 %s' %
                 (os.path.basename(lp), len(data), STEP_KEY), 'WARN')
            return cfg
        except Exception as e:
            _log(ctx, '本地测试配置读取失败(%s),改走云端 %s' % (e, STEP_KEY), 'WARN')
    try:
        from workflow_steps import fetch_step_config
        data = fetch_step_config(ctx, STEP_KEY, STEP_NAME)
    except Exception:
        data = None
    if data:
        for k, v in data.items():
            if k in cfg and v is not None:
                cfg[k] = v
        _log(ctx, '已从云端拉取步骤参数(%d项)' % len(data))
    else:
        _log(ctx, '使用内置默认步骤参数(云端不可用或未配置)')
    try:
        _QTY_RE = re.compile(str(cfg.get('qty_regex') or _默认配置['qty_regex']))
    except Exception:
        pass
    return cfg

def _mod_step5():
    """动态取「第五步」模块(挂载时由 importlib 拿,避免循环依赖)。"""
    global _mod5
    if _mod5 is None:
        _mod5 = importlib.import_module('第五步')
    return _mod5

_mod5 = None

def _local_db():
    try:
        return _mod_step5()._local_db()
    except Exception:
        return None

def _云端凭据():
    """(server_url, token) —— 复用第五步的云端配置口径。"""
    try:
        return _mod_step5()._云端凭据()
    except Exception:
        return None

def _ui_aw_on() -> bool:
    """界面「研发兑换」开关(唯一判定入口)。

    优先读 worker 写的 PC_TQC_RUNNER_CFG;读不到/未配置一律按关闭处理,
    保证「不勾选/无配置 = 一次点击都不发」。
    """
    _w = {}
    try:
        _raw = os.environ.get('PC_TQC_RUNNER_CFG')
        _w = json.loads(_raw) if _raw else None
    except Exception:
        _w = None
    if not isinstance(_w, dict):
        try:
            _w = _mod_step5()._read_worker_cfg()
        except Exception:
            _w = {}
    if not isinstance(_w, dict):
        _w = {}
    try:
        return _mod_step5()._cfg_bool(_w.get('aw_enabled'), False)
    except Exception:
        v = _w.get('aw_enabled')
        if isinstance(v, str):
            return v.strip().lower() in ('1', 'true', 'on', 'yes', 'y', '开', '开启', '是')
        if v is not None:
            return bool(v)
        return False

def _db_conn():
    """建表(aw_scan_log + aw_upload_queue),返回连接。"""
    path = _local_db()
    if not path:
        return None
    os.makedirs(os.path.dirname(path), exist_ok=True)
    conn = sqlite3.connect(path)
    conn.execute('CREATE TABLE IF NOT EXISTS aw_scan_log(\n'
                 '        username TEXT, slot_key TEXT, done_at TEXT, result TEXT, qty INTEGER,\n'
                 '        PRIMARY KEY(username, slot_key))')
    try:
        conn.execute('ALTER TABLE aw_scan_log ADD COLUMN items TEXT')
    except Exception:
        pass
    conn.execute('CREATE TABLE IF NOT EXISTS aw_upload_queue(\n'
                 '        id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT, account_id INTEGER,\n'
                 '        qty INTEGER, slot TEXT, hit_at TEXT, tries INTEGER DEFAULT 0,\n'
                 '        last_err TEXT, created_at TEXT)')
    return conn

def _slot_done(username: str, slot_key: str) -> bool:
    if not username:
        return True
    try:
        with _db_conn() as conn:
            row = conn.execute('SELECT 1 FROM aw_scan_log WHERE username=? AND slot_key=?',
                               (username, slot_key)).fetchone()
        return row is None
    except Exception:
        return False

def _mark_slot(username: str, slot_key: str, result: str, qty, items=None) -> None:
    if not username:
        return None
    try:
        with _db_conn() as conn:
            conn.execute('INSERT OR REPLACE INTO aw_scan_log(username,slot_key,done_at,result,qty,items)'
                         ' VALUES(?,?,?,?,?,?)',
                         (username, slot_key, time.strftime('%Y-%m-%d %H:%M:%S'), result,
                          int(qty or 0), str(items or '')))
    except Exception:
        pass

def _queue_upload(username: str, account_id, qty, slot_label: str, err) -> None:
    try:
        with _db_conn() as conn:
            conn.execute('INSERT INTO aw_upload_queue(username,account_id,qty,slot,hit_at,tries,last_err,created_at)'
                         ' VALUES(?,?,?,?,?,0,?,?)',
                         (username, int(account_id or 0), int(qty or 1), slot_label,
                          time.strftime('%Y-%m-%dT%H:%M:%S'), str(err)[:200],
                          time.strftime('%Y-%m-%d %H:%M:%S')))
    except Exception:
        pass

def _current_slot():
    """返回 (slot_key, slot_label);label 即回传云端的 slot 字段。
    档位:12点档=12:00~19:59;20点档=20:00~次日11:59(跨零点,标记键=刷新日期+档位)。"""
    now = time.localtime()
    today = date.today()
    if 12 <= now.tm_hour <= 19:
        return ('%s#12' % today.isoformat(), '12:00')
    if now.tm_hour >= 20:
        return ('%s#20' % today.isoformat(), '20:00')
    return ('%s#20' % (today - timedelta(days=1)).isoformat(), '20:00')

def _grab_ocr(ctx, hwnd, tag):
    frame = ctx.grab_frame(hwnd, tag)
    if frame is None:
        return None
    return ctx.ocr_scan(frame, tag)

def _joined(items) -> str:
    if items:
        return ' '.join(t for t, _ in items)
    return ''

def _hits(joined: str, kws) -> int:
    return sum(1 for kw in kws if kw in joined)

def _ocr_roi(ctx, hwnd, box, tag):
    """按比例裁剪区域 OCR,坐标换算回客户区像素。box 为分数坐标 [x0,y0,x1,y1]∈[0,1]。"""
    try:
        frame = ctx.grab_frame(hwnd, tag)
        if frame is None:
            return None
        h, w = frame.shape[:2]
        x1 = max(0, min(w - 2, int(float(box[0]) * w)))
        y1 = max(0, min(h - 2, int(float(box[1]) * h)))
        x2 = max(x1 + 2, min(w, int(float(box[2]) * w)))
        y2 = max(y1 + 2, min(h, int(float(box[3]) * h)))
        crop = frame[y1:y2, x1:x2]
        items = ctx.ocr_scan(crop, tag)
        return [(t, (cx + x1, cy + y1)) for t, (cx, cy) in (items or [])
                if cx is not None and cy is not None]
    except Exception:
        return None

def _norm(s) -> str:
    return re.sub(r'[^0-9a-z]', '', str(s).lower())

def _item_name_only(t):
    """日志用:只保留像物品名的文字,过滤页签/标题/UID/等级等杂项。"""
    s = str(t).strip()
    c = _clean(s)
    if len(c) < 2 or not re.search('[\\u4e00-\\u9fffA-Za-z]', c):
        return ''
    else:
        bad = ('限购', '部门', '刷新', '赛季', '任务', '军需', '开始游戏', '仓库', '交易行', '特勤处', '改枪台', 'LV', 'Lv', 'lv', 'UID', 'uid', '集市', '雷达站', '数据中心', '研发成果', '卡包', '藏品箱', '档案箱', '收集品', '背包胸挂', '枪械配件', '武器箱', '材料箱', '装备箱', '弹药箱', '任务道具', '医药箱', '物品箱', 'Esc', '处理')
        if any((k in s for k in bad)):
            return ''
        else:
            if not re.search('[\\u4e00-\\u9fff]', c) and (not re.search('[0-9]', c)):
                return ''
            else:
                if len(re.sub('[0-9]', '', c)) < 2:
                    return ''
                else:
                    if len(c) < 4 and (not re.search('[0-9A-Za-z]', c)):
                        return ''
                    else:
                        return s

def _find_target(items):
    """整屏 OCR 找 AW 卡片。返回 (cx, cy, matched_text) 或 None。"""
    if not items:
        return
    else:
        strong = [str(s) for s in _CFG.get('target_strong', []) if s]
        weak = [str(s) for s in _CFG.get('target_weak', []) if s]
        confirm = [str(s) for s in _CFG.get('target_confirm', []) if s]
        for t, (cx, cy) in items:
            n = _norm(t)
            if not any((s in n for s in strong)):
                continue
            else:
                return (cx, cy, t)
        for t, (cx, cy) in items:
            n = _norm(t)
            if not any((w in n for w in weak)):
                continue
            else:
                if any((c in n for c in confirm)):
                    return (cx, cy, t)
                else:
                    near = ''.join((str(t2) for t2, (x2, y2) in items
                                    if abs(x2 - cx) <= 170 and abs(y2 - cy) <= 90))
                    nn = _norm(near)
                    if not any((c in nn for c in confirm)):
                        continue
                    else:
                        return (cx, cy, t)

def _is_aw_text(t: str) -> bool:
    """命中文字是否为 AW(含 target_strong 或 338),决定要不要上报云端。"""
    n = _norm(t)
    if not n:
        return False
    if any(str(s) in n for s in _CFG.get('target_strong', []) if s):
        return True
    return '338' in n

def _read_qty(items, cx, cy):
    """读卡片右上角 ×N;读不出返回 default_qty。返回 (qty, qty_defaulted)。"""
    best = None
    for t, (x2, y2) in (items or []):
        m = _QTY_RE.search(str(t))
        if not m:
            continue
        if 0 <= cy - y2 <= _CFG['qty_above_dy']:
            if abs(x2 - cx) <= _CFG['qty_max_dx']:
                d = (cy - y2) + abs(x2 - cx)
                if best is None or d < best[0]:
                    best = (d, int(m.group(1)))
    if best is not None and best[1] > 0:
        return (best[1], False)
    return (int(_CFG.get('default_qty', 1)), True)

def _会话头():
    """云端会话头(main/__main__ 模块的 _CLOUD_SESSION 两元组)。"""
    try:
        m = sys.modules.get('__main__')
        sess = getattr(m, '_CLOUD_SESSION', None)
        if sess and sess[0]:
            return {'X-TQC-Session': str(sess[0]), 'X-TQC-Device': str(sess[1])}
    except Exception:
        pass
    return {}

def _post_hit(account_id, qty, slot_label: str) -> bool:
    """回传命中AW。成功返回 True;否则抛异常。"""
    server, token = _云端凭据()
    if not server or not token:
        raise RuntimeError('云端凭据缺失')
    url = server.rstrip('/') + _CFG['cloud_hit_path']
    body = json.dumps({'accountId': int(account_id), 'item': _ITEM_NAME,
                       'qty': int(qty), 'slot': slot_label,
                       'hitAt': time.strftime('%Y-%m-%dT%H:%M:%S')}).encode('utf-8')
    headers = {'Authorization': 'Bearer ' + token,
               'Content-Type': 'application/json',
               'User-Agent': 'Mozilla/5.0 Chrome/130'}
    headers.update(_会话头())
    req = urllib.request.Request(url, data=body, headers=headers)
    with urllib.request.urlopen(req, timeout=int(_CFG['cloud_timeout'])) as r:
        data = json.loads(r.read().decode('utf-8'))
    if not data.get('ok'):
        raise RuntimeError('云端返回失败: %s' % str(data)[:120])
    return True

def _flush_upload_queue(ctx):
    """补传上次失败的命中记录(每次扫描前顺带尝试)。"""
    try:
        with _db_conn() as conn:
            rows = conn.execute('SELECT id, username, account_id, qty, slot, hit_at, tries'
                                ' FROM aw_upload_queue ORDER BY id LIMIT 5').fetchall()
        for rid, uname, aid, qty, slot_label, hit_at, tries in rows:
            if tries >= 10:
                continue
            try:
                _post_hit(aid, qty, slot_label)
                with _db_conn() as conn:
                    conn.execute('DELETE FROM aw_upload_queue WHERE id=?', (rid,))
                _log(ctx, f'补传命中AW成功: 账号{aid} ×{qty} ({slot_label})')
            except Exception as e:
                with _db_conn() as conn:
                    conn.execute('UPDATE aw_upload_queue SET tries=tries+1, last_err=? WHERE id=?',
                                 (str(e)[:200], rid))
    except Exception as e:
        pass

def _clean(s) -> str:
    """中文友好清洗:去掉空格/标点/符号,保留汉字字母数字。"""
    return re.sub(r'[\s\W_]+', '', str(s))

def _on_page(items, key, need=1):
    """云端给字符串=命中即算;给列表=命中数>=need(默认取*_need)才算。"""
    val = _CFG.get(key)
    if not val:
        return False
    else:
        joined = _clean(_joined(items))
        if isinstance(val, (list, tuple)):
            kws = [_clean(k) for k in val if _clean(k)]
            if not kws:
                return False
            else:
                n = sum((1 for k in kws if k in joined))
                return n >= int(_CFG.get(key + '_need', need))
        else:
            return _clean(val) in joined

def _press_esc(ctx, hwnd, tag):
    vk = _解析vk(_CFG.get('esc_vk')) or 27
    ctx.key_foreground(hwnd, vk, tag)

def _back_to_lobby(ctx, hwnd):
    """扫完后 ESC 回大厅,带大厅特征校验。"""
    for i in range(int(_CFG['return_lobby_max_esc'])):
        items = _grab_ocr(ctx, hwnd, '扫AW回大厅校验')
        if items and _on_page(items, 'lobby_page_kw'):
            return True
        _press_esc(ctx, hwnd, '扫AW回大厅ESC-%d' % (i + 1))
        time.sleep(float(_CFG['return_lobby_esc_wait']))
    # 终检
    items = _grab_ocr(ctx, hwnd, '扫AW回大厅终检')
    ok = bool(items and _on_page(items, 'lobby_page_kw'))
    if not ok:
        _log(ctx, '回大厅未能确认(仍交还交易行原流程)', 'WARN')
    return ok

def _nav_to_rd_page(ctx, hwnd, deadline):
    """只认四种页面:大厅/特勤处/部门页/军需处。认出哪个就按那个的坐标点;认不出放弃。"""
    if time.time() >= deadline or ctx.stopped():
        return None
    items = _grab_ocr(ctx, hwnd, '当前位置识别')
    if items is None:
        _log(ctx, '当前位置识别失败,本档放弃', 'WARN')
        return None
    in_lobby = _on_page(items, 'lobby_page_kw')
    in_tqc = _on_page(items, 'tqc_page_kw')
    in_dept = _on_page(items, 'dept_page_kw')
    in_qxs = _on_page(items, 'qxs_page_kw') and not in_dept
    if in_qxs:
        _log(ctx, '当前位置: 军需处 → 点「研发部门」(等%g秒)' % float(_CFG['rd_card_wait']))
    elif in_lobby or in_tqc:
        _log(ctx, '当前位置: %s → 点「部门」→ 点「军需处」→ 点「研发部门」(等%g秒)' %
             ('特勤处' if in_tqc else '大厅', float(_CFG['rd_card_wait'])))
        ctx.click_ref(hwnd, _CFG['dept_tab_x'], _CFG['dept_tab_y'], _CFG['ref_w'], _CFG['ref_h'], '顶部门页签')
        time.sleep(float(_CFG['dept_tab_wait']))
        if time.time() >= deadline or ctx.stopped():
            return None
        ctx.click_ref(hwnd, _CFG['qxs_tab_x'], _CFG['qxs_tab_y'], _CFG['ref_w'], _CFG['ref_h'], '军需处子页签')
        time.sleep(float(_CFG['qxs_tab_wait']))
        if time.time() >= deadline or ctx.stopped():
            return None
    elif in_dept:
        _log(ctx, '当前位置: 部门页 → 点「军需处」→ 点「研发部门」(等%g秒)' % float(_CFG['rd_card_wait']))
        ctx.click_ref(hwnd, _CFG['qxs_tab_x'], _CFG['qxs_tab_y'], _CFG['ref_w'], _CFG['ref_h'], '军需处子页签')
        time.sleep(float(_CFG['qxs_tab_wait']))
        if time.time() >= deadline or ctx.stopped():
            return None
    else:
        _log(ctx, '当前位置识别失败,本档放弃(OCR读到: %s)' % _joined(items)[:300], 'WARN')
        return None
    ctx.click_ref(hwnd, _CFG['rd_card_x'], _CFG['rd_card_y'], _CFG['ref_w'], _CFG['ref_h'], '研发部门卡片')
    time.sleep(float(_CFG['rd_card_wait']))
    if time.time() >= deadline or ctx.stopped():
        return None
    items2 = _grab_ocr(ctx, hwnd, '研发部门确认')
    if not items2:
        _log(ctx, '未到研发部门(截图/识别为空),本档放弃', 'WARN')
        return None
    if not _on_page(items2, 'rd_page_kw'):
        _log(ctx, '未到研发部门,本档放弃(OCR读到: %s)' % _joined(items2)[:300], 'WARN')
        return None
    _log(ctx, '已到达研发部门 ✓')
    return True

def _wait_detail(ctx, hwnd, deadline):
    """等详情页打开:右下按钮区能读出 补齐/兑换 即视为已开。"""
    end = min(deadline, time.time() + float(_CFG['detail_wait']))
    while time.time() < end and (not ctx.stopped()):
        items = _ocr_roi(ctx, hwnd, _CFG['btn_roi_box'], '扫AW详情按钮')
        if items:
            j = _joined(items)
            if any((k in j for k in _CFG['fill_kw'] + _CFG['exchange_kw'])):
                return items
        time.sleep(float(_CFG['poll_ms']) / 1000.0)
    return None

def _wait_success_or_error(ctx, hwnd, deadline):
    """兑换后等结果提示。返回 'success' / 'error' / 'unknown'。"""
    end = min(deadline, time.time() + float(_CFG['success_wait']))
    while time.time() < end and not ctx.stopped():
        items = _grab_ocr(ctx, hwnd, '扫AW结果判定')
        if items:
            j = _joined(items)
            if any(k in j for k in _CFG['error_kw']):
                return 'error'
            if any(k in j for k in _CFG['success_kw']):
                return 'success'
        time.sleep(float(_CFG['poll_ms']) / 1000.0)
    return 'unknown'

def _do_purchase(ctx, hwnd, cx, cy, deadline):
    """点卡片 → 补齐/购买/兑换/确认。成功返回 True,失败抛异常(不写档位标记)。"""
    ctx.click(hwnd, cx, cy, 'AW卡片')
    btn_items = _wait_detail(ctx, hwnd, deadline)
    if btn_items is None:
        ctx.click(hwnd, cx, cy, 'AW卡片-补点')
        btn_items = _wait_detail(ctx, hwnd, deadline)
    if btn_items is None:
        raise RuntimeError('详情页打不开(按钮区读不出)')
    else:
        j = _joined(btn_items)
        if any((k in j for k in _CFG['fill_kw'])):
            ctx.click_ref(hwnd, _CFG['fill_btn_x'], _CFG['fill_btn_y'], _CFG['ref_w'], _CFG['ref_h'], '一键补齐')
            end = min(deadline, time.time() + float(_CFG['popup_wait']))
            popup = None
            while time.time() < end and (not ctx.stopped()):
                    popup = _ocr_roi(ctx, hwnd, _CFG['popup_roi_box'], '扫AW补齐弹窗')
                    if popup and any((k in _joined(popup) for k in ['购买', '支付', '总价', '补齐'])):
                            break
                    popup = None
                    time.sleep(float(_CFG['poll_ms']) / 1000.0)
            if popup is None:
                raise RuntimeError('补齐弹窗未出现')
            else:
                if time.time() >= deadline:
                    raise RuntimeError('超时(补齐弹窗后)')
                else:
                    ctx.click_ref(hwnd, _CFG['pay_x'], _CFG['pay_y'], _CFG['ref_w'], _CFG['ref_h'], '购买')
                    end = min(deadline, time.time() + float(_CFG['btn_become_wait']))
                    ok = False
                    while time.time() < end and (not ctx.stopped()):
                            items = _ocr_roi(ctx, hwnd, _CFG['btn_roi_box'], '扫AW等兑换按钮')
                            if items and any((k in _joined(items) for k in _CFG['exchange_kw'])):
                                    ok = True
                                    break
                            time.sleep(float(_CFG['poll_ms']) / 1000.0)
                    if not ok:
                        raise RuntimeError('购买后按钮未变兑换(可能钱不够/材料异常)')
        if time.time() >= deadline:
            raise RuntimeError('超时(兑换前)')
        else:
            ctx.click_ref(hwnd, _CFG['exchange_x'], _CFG['exchange_y'], _CFG['ref_w'], _CFG['ref_h'], '兑换')
            end = min(deadline, time.time() + float(_CFG['popup_wait']))
            conf = False
            while time.time() < end and (not ctx.stopped()):
                    items = _ocr_roi(ctx, hwnd, _CFG['popup_roi_box'], '扫AW确认弹窗')
                    if items and any((k in _joined(items) for k in _CFG['confirm_kw'])):
                            conf = True
                            break
                    time.sleep(float(_CFG['poll_ms']) / 1000.0)
            if not conf:
                raise RuntimeError('确认弹窗未出现')
            else:
                ctx.click_ref(hwnd, _CFG['confirm_x'], _CFG['confirm_y'], _CFG['ref_w'], _CFG['ref_h'], '确认')
                result = _wait_success_or_error(ctx, hwnd, deadline)
                if result == 'success':
                    return True
                else:
                    if result == 'error':
                        raise RuntimeError('兑换失败提示(钱不足/上限等)')
                    else:
                        raise RuntimeError('兑换结果不明(未识别到成功提示)')

def _run_aw_scan(ctx):
    """研发部门扫AW主流程。"""
    cfg = _加载配置(ctx)
    global _CFG
    _CFG = cfg
    if not _ui_aw_on():
        _log(ctx, '研发部门兑换已关闭(界面开关),跳过')
        return None
    if not cfg.get('enable', True):
        _log(ctx, '功能已关闭(云端 enable=False),跳过')
        return None
    uname = getattr(ctx, 'current_username', '') or ''
    if not uname:
        _log(ctx, '无当前账号名,跳过', 'WARN')
        return None
    slot_key, slot_label = _current_slot()
    if _slot_done(uname, slot_key):
        return None
    while True:
        try:
            _flush_upload_queue(ctx)
            hwnd = _mod_step5()._find_game_hwnd()
        except Exception:
            continue
        break
    if not hwnd:
        _log(ctx, '找不到游戏窗口,跳过(不写档位标记,下趟重试)', 'WARN')
        return None
    deadline = time.time() + float(cfg['total_timeout'])
    try:
        _log(ctx, f'进入研发扫AW(档位 {slot_label}){" [DRY_RUN]" if cfg.get("dry_run") else ""}')
        nav = _nav_to_rd_page(ctx, hwnd, deadline)
        if not nav:
            return None
        items = _grab_ocr(ctx, hwnd, '扫AW整区识别') or []
        names = [_item_name_only(t) for t, _ in items]
        names = [n for n in names if n]
        shelf = '、'.join(names)
        _log(ctx, '本档扫到货: %s' % (shelf or '(未识别到物品名)'))
        hit = _find_target(items)
        if not hit:
            _log(ctx, '本档无 %s,扫货完成(未命中)' % _ITEM_NAME)
            if not cfg.get('dry_run'):
                _mark_slot(uname, slot_key, 'miss', 0, shelf)
            _back_to_lobby(ctx, hwnd)
            return None
        cx, cy, mtext = hit
        qty, qty_defaulted = _read_qty(items, cx, cy)
        _log(ctx, '命中 %s ×%d%s(卡片文字: %s)' % (_ITEM_NAME, qty,
             ' [×N未读出,用兜底]' if qty_defaulted else '', mtext))
        if cfg.get('dry_run'):
            _log(ctx, 'DRY_RUN:不真实购买、不上传、不写档位标记', 'WARN')
            _back_to_lobby(ctx, hwnd)
            return None
        _do_purchase(ctx, hwnd, cx, cy, deadline)
        if _is_aw_text(mtext):
            aid = getattr(ctx, 'account_id', 0) or 0
            try:
                _post_hit(aid, qty, slot_label)
                _log(ctx, '已回传云端:命中AW ×%d(账号%s,%s)' % (qty, aid, slot_label))
            except Exception as e:
                _queue_upload(uname, aid, qty, slot_label, e)
                _log(ctx, '回传失败已入本地队列,下次补传: %s' % e, 'WARN')
        else:
            _log(ctx, '非AW物品,已兑换,不上报云端')
        _mark_slot(uname, slot_key, 'hit', qty, shelf)
        _back_to_lobby(ctx, hwnd)
        return None
    except Exception as e:
        _log(ctx, '扫AW失败(不写档位标记,下趟重试): %s: %s' % (type(e).__name__, e), 'WARN')
        try:
            _back_to_lobby(ctx, hwnd)
        except Exception:
            pass
        return None

def _静音弹窗日志():
    """停写 桌面/PC特勤处/运行日志/回归弹窗.log(第二步/第三步落盘用)。"""
    try:
        import workflow_steps as ws
        fn = getattr(ws, 'popup_event_log', None)
        if fn is None or getattr(fn, '_aw_muted', False):
            return None

        def _muted(username, event):
            return None

        _muted._aw_muted = True
        ws.popup_event_log = _muted
        return None
    except Exception:
        return None

def install(cls, ctx):
    """包装「第五步」_ensure_in_lobby:回大厅后顺路扫AW,再交还交易行。"""
    _静音弹窗日志()
    try:
        mod5 = _mod_step5()
    except Exception as e:
        _log(ctx, '未找到第五步模块,扫AW未挂载: %s' % e, 'WARN')
        return None
    prev = getattr(mod5, '_ensure_in_lobby', None)
    if prev is None:
        _log(ctx, '第五步无 _ensure_in_lobby,扫AW未挂载', 'WARN')
        return None
    if getattr(prev, '_aw_wrapped', False):
        _log(ctx, '扫AW已挂载过,跳过重复安装')
        return None
    if not _ui_aw_on():
        _log(ctx, '研发兑换=关闭(界面未勾选/未配置),不挂载扫AW(本轮一次都不执行)')
        return None

    def wrapped(ctx2, log=None, max_esc=None):
        result = prev(ctx2, log, max_esc)
        try:
            _run_aw_scan(ctx2)
        except Exception as e:
            _log(ctx2, '扫AW异常(已忽略,不影响交易行): %s: %s' % (type(e).__name__, e), 'WARN')
        return result
    wrapped._aw_wrapped = True
    mod5._ensure_in_lobby = wrapped
    _log(ctx, '研发扫AW已挂载(界面开关=开启):军需兑换后、交易行出售前(回大厅顺路)')
    return None
