# workflow_steps.py — 工作流总控（重建版：忠实 100% 语义转储 analysis_biz.txt）
# 职责: 本地调度库读写 / 哈夫币解析 / 验证码守卫 / 弹药交换安装 / 游戏窗口定位
# 边界: 视觉层经 ctx.grab_frame/ocr_scan 抽象(Nuitka .pyd 收口), 不读游戏内存

import contextlib, importlib, json, logging, os, sys, threading, time
import urllib.request

STEP_NAME = 'workflow_steps'
_HAF_RE = None
_GAME_PROCS = ('deltalaction.exe', 'deltalaction.exe')
_AMMO_EXCHANGE_TABLE = 'ammo_exchange'


def _local_db_path() -> str:
    """runner 本地调度库路径: 桌面/PC特勤处/pc_tqc.db。"""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,
                            r'Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders') as key:
            desktop = os.path.expandvars(str(winreg.QueryValueEx(key, 'Desktop')[0]).strip())
            if desktop:
                return os.path.join(desktop, 'PC特勤处', 'pc_tqc.db')
    except Exception:
        pass
    return os.path.join(os.path.expanduser('~'), 'Desktop', 'PC特勤处', 'pc_tqc.db')


def _local_db() -> str:
    """本地调度库路径(与 _local_db_path 同口径)。"""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,
                            r'Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders') as key:
            desktop = os.path.expandvars(str(winreg.QueryValueEx(key, 'Desktop')[0]).strip())
            if desktop:
                return os.path.join(desktop, 'PC特勤处', 'pc_tqc.db')
    except Exception:
        pass
    return os.path.join(os.path.expanduser('~'), 'Desktop', 'PC特勤处', 'pc_tqc.db')


def popup_event_log(username: str, event: str) -> None:
    """回归弹窗识别/按键事件落盘(桌面/PC特勤处/运行日志/回归弹窗.log)。
    UI控制台最多缓存1000行会被刷掉,文件留底供事后核查。任何失败静默忽略(日志绝不能影响主流程)。"""
    try:
        d = os.path.join(os.path.dirname(_local_db_path()), '运行日志')
        os.makedirs(d, exist_ok=True)
        line = f'[{time.strftime("%m-%d %H:%M:%S")}] {username or "?"} {event}\n'
        with open(os.path.join(d, '回归弹窗.log'), 'a', encoding='utf-8') as f:
            f.write(line)
    except Exception:
        pass


def _parse_haf_coins(raw):
    """本地库 haf_coins(如 '50,653K' / '1.2M' / '50653000') → 整数。失败返回 None。"""
    global _HAF_RE
    if raw is None:
        return None
    if _HAF_RE is None:
        import re
        _HAF_RE = re.compile(r'([0-9][0-9,]*(?:\.[0-9]+)?)\s*([KMkm]?)')
    m = _HAF_RE.search(str(raw).replace(' ', ''))
    if not m:
        return None
    try:
        num = float(m.group(1).replace(',', ''))
    except ValueError:
        return None
    suf = (m.group(2) or '').upper()
    if suf == 'K':
        num *= 1000
    elif suf == 'M':
        num *= 1000000
    return int(round(num))


def _read_local_final(username: str):
    """从本地调度库读该账号最新 (level, haf_coins_int, updated_at);读不到返回 (None,None,None)。
    带 3 次轻量重试,抵御 SQLite 瞬时锁/核心写入尚未提交,避免偶发读空。"""
    if not username:
        return (None, None, None)
    import sqlite3
    db = _local_db_path()
    if not os.path.exists(db):
        return (None, None, None)
    for _ in range(3):
        try:
            conn = sqlite3.connect(db, timeout=15)
            try:
                row = conn.cursor().execute(
                    'SELECT level, haf_coins, updated_at FROM account_state '
                    'WHERE username=? ORDER BY rowid DESC LIMIT 1',
                    (username,)).fetchone()
            finally:
                conn.close()
            if not row:
                return (None, None, None)
            lvl, raw, uat = row
            return (str(lvl).strip() if lvl else '', _parse_haf_coins(raw), uat or None)
        except Exception:
            time.sleep(0.6)
            continue
    return (None, None, None)


def _is_fresh(updated_at, within: float = 180) -> bool:
    """updated_at 是否在 within 秒内——即本轮刚写入的新值,而非历史旧值。"""
    if not updated_at:
        return False
    try:
        from datetime import datetime
        ts = updated_at
        if isinstance(ts, str):
            ts = datetime.fromisoformat(ts.replace('Z', ''))
        age = (datetime.now() - ts).total_seconds()
        return 0 <= age < within
    except Exception:
        return False


def _read_final_with_freshness(username: str, max_wait: float = 10.0):
    """读本地库最终币,并确认是本轮新写的(updated_at 新鲜);否则等待重试。

    抵御 on_account_completed 早于核心写库的时序风险:核心若在回调之后才提交,
    这里会等到拿到新鲜值再回传,避免把上一轮旧值覆盖到显示/云端。
    超时时回退到最后一次读到的值(回调时点本地库通常已是最新,新鲜度只是双保险)。
    """
    deadline = time.monotonic() + max_wait
    last_lvl, last_coins = (None, None)
    while True:
        lvl, coins, uat = _read_local_final(username)
        if coins is not None:
            last_lvl, last_coins = lvl, coins
        if _is_fresh(uat):
            return (lvl, coins)
        if time.monotonic() >= deadline:
            return (last_lvl, last_coins)
        time.sleep(1.0)


def _aid_read_cfg_map() -> dict:
    """读 worker 配置 JSON,返回 {账号名: 云端中控ID} 映射;读不到返回 {}。

    优先读进程环境变量 PC_TQC_RUNNER_CFG(新链路配置经 stdin 传入,由 worker
    解析后写入环境变量,不再落临时文件);兼容旧链路 sys.argv[1] 与 %TEMP% 下
    历史遗留的 pc_tqc_runner_cfg_*.json。
    """

    def _map_of(cfg):
        m = {}
        lst = cfg.get('accounts_list')
        if isinstance(lst, list):
            for a in lst:
                if not isinstance(a, dict):
                    continue
                if a.get('account') is None:
                    continue
                if a.get('id') is None:
                    continue
                m[str(a['account'])] = a['id']
        if cfg.get('account') and cfg.get('account_id') is not None:
            m.setdefault(str(cfg['account']), cfg['account_id'])
        return m

    try:
        _raw = os.environ.get('PC_TQC_RUNNER_CFG')
        if _raw:
            _m = _map_of(json.loads(_raw))
            if _m:
                return _m
    except Exception:
        pass

    paths = []
    try:
        if len(sys.argv) >= 2 and str(sys.argv[1]).lower().endswith('.json') and os.path.exists(sys.argv[1]):
            paths.append(sys.argv[1])
    except Exception:
        pass

    try:
        import glob
        import tempfile
        tmp = tempfile.gettempdir()
        cands = [p for p in glob.glob(os.path.join(tmp, 'pc_tqc_runner_cfg_*.json')) if p not in paths]
        cands.sort(key=lambda p: os.path.getmtime(p), reverse=True)
        if cands:
            paths.append(cands[0])
    except Exception:
        pass

    import json as _json
    for p in paths:
        try:
            with open(p, 'r', encoding='utf-8-sig') as f:
                cfg = _json.load(f)
            m = _map_of(cfg)
            if m:
                return m
        except Exception:
            continue

    return {}


def _install_captcha_guard(ctx, push_log):
    """补丁引擎视觉OCR模块级函数:任何环节(种菜购买/军需兑换/交易行出售)
    OCR扫到验证码特征 → ERROR日志「该账号出现了验证码弹窗」+ 抛 CaptchaError
    判失败切号。未命中时原返回值原样返回,对现有逻辑零改动。"""
    try:
        from pc_tqc.vision import ocr as _vocr
    except Exception as e:
        push_log(f'[验证码] 视觉模块加载失败,守卫未生效: {type(e).__name__}: {e}', 'WARN')
        return None

    patched = []
    for fname in ('recognize_texts', 'recognize_texts_rec_only'):
        fn = getattr(_vocr, fname, None)
        if not callable(fn):
            continue
        if getattr(fn, '_tqc_captcha', False):
            continue

        def _mk(_orig):
            def _guarded(*args, **kwargs):
                result = _orig(*args, **kwargs)
                texts = _提取OCR文本(result)
                if texts and _是验证码弹窗(' '.join(texts)):
                    try:
                        ctx._captcha_hit = 'ocr'
                        push_log('[验证码] 该账号出现了验证码弹窗,判定失败,切下一个账号', 'ERROR')
                        raise CaptchaError('该账号出现了验证码弹窗')
                    except CaptchaError:
                        raise
                    except Exception:
                        pass
                return result
            _guarded._tqc_captcha = True
            return _guarded

        setattr(_vocr, fname, _mk(fn))
        patched.append(fname)
    if patched:
        push_log(f'[验证码] 弹窗守卫已安装({", ".join(patched)})', 'INFO')
    else:
        push_log('[验证码] 未找到可补丁的OCR函数,守卫未生效', 'WARN')
    return None


def _install_final_coins_report(ar, ctx, push_log):
    """包装核心 callbacks.on_account_completed:账号本轮跑完后读本地库最终币回传。

    为什么放这:runner_worker 是 exe 内嵌入口(改外部副本不生效),但它会
    `from workflow_steps import` 加载本文件(外部文件,改了生效)。在安装步骤时
    拿到 ar.callbacks 包一层即可,无需动 runner_worker。
    """
    cb_obj = getattr(ar, 'callbacks', None)
    if cb_obj is None:
        push_log('[总管] 未找到 ar.callbacks,最终币回传未安装', 'WARN')
        return None
    orig = getattr(cb_obj, 'on_account_completed', None)
    if not callable(orig):
        push_log('[总管] callbacks.on_account_completed 不可调用,最终币回传未安装', 'WARN')
        return None

    def _wrapper(*args, _orig=orig, _ctx=ctx, **kwargs):
        try:
            _orig(*args, **kwargs)
        except Exception as _e:
            _ctx.log('数据回传', f'原 on_account_completed 异常(已忽略): {_e}', 'WARN')
        try:
            state = args[0] if args else None
            acct = getattr(state, 'account', None) if state is not None else None
            uname = (getattr(acct, 'username', '') if acct is not None else '') or ''
            lvl, coins = _read_final_with_freshness(uname)
            if not lvl:
                lvl = None
            if coins is not None:
                _ctx.report_account_data(level=lvl, coins=coins, username=uname)
                _ctx.log('数据回传', f'账号 {uname} 本轮完成,本地库最终哈夫币 {coins} 已回传', 'INFO')
            else:
                _ctx.log('数据回传', f'账号 {uname} 本轮完成,但本地库未读到哈夫币,暂不回传', 'WARN')
        except Exception as _e:
            _ctx.log('数据回传', f'本地库最终币回传失败: {type(_e).__name__}: {_e}', 'ERROR')

    try:
        cb_obj.on_account_completed = _wrapper
        push_log('[总管] 已安装:账号完成时读本地库最终哈夫币回传', 'INFO')
        return None
    except Exception as _e:
        push_log(f'[总管] on_account_completed 不可替换({_e}),最终币回传未安装', 'WARN')
        return None


def _install_aid_inject(ar, push_log):
    """包装 ar.run:执行前给每个账号补齐 account_id(中控ID)。

    为什么放这:引擎(runner.pyd)在接入制造计划 loader 后要求每个 Account
    带中控 ID,否则判定"制造配置暂不可用:账号缺少有效的中控 ID",只重查
    配置不登录游戏,3 分钟一轮死循环。exe 内嵌 worker 构建 Account 时只有
    username/password(改内嵌入口不生效),而本文件是 exe 每次都加载的外部
    覆盖层——在 ar.run 入口把中控ID补上即可,不改动任何原版逻辑。
    """
    orig_run = getattr(ar, 'run', None)
    if not callable(orig_run):
        push_log('[总管] ar.run 不可调用,中控ID注入未安装', 'WARN')
        return None

    def _run_with_aid(*args, _orig=orig_run, **kwargs):
        idmap = {}
        try:
            idmap = _aid_read_cfg_map()
        except Exception as _e:
            push_log(f'[总管] 读中控ID映射异常({type(_e).__name__}: {_e}),按原样执行', 'WARN')
        if idmap:
            fixed = 0
            for arg in list(args) + list(kwargs.values()):
                if not isinstance(arg, (list, tuple)):
                    continue
                for acct in arg:
                    try:
                        name = str(getattr(acct, 'username', '') or '')
                        if not name:
                            continue
                        aid = idmap.get(name)
                        cur = getattr(acct, 'account_id', None)
                        if aid is not None and (cur is None or cur == ''):
                            acct.account_id = aid
                            fixed += 1
                    except Exception:
                        pass
            try:
                existing = getattr(ar, '_manufacture_account_ids', None)
                if isinstance(existing, dict):
                    for _k, _v in idmap.items():
                        existing.setdefault(_k, _v)
                else:
                    ar._manufacture_account_ids = dict(idmap)
            except Exception:
                pass
            if fixed:
                push_log(f'[总管] 已为 {fixed} 个账号补齐中控ID(映射共{len(idmap)}条)', 'INFO')
        return _orig(*args, **kwargs)

    try:
        ar.run = _run_with_aid
        push_log('[总管] 已安装:执行前注入账号中控ID', 'INFO')
        return None
    except Exception as _e:
        push_log(f'[总管] ar.run 不可替换({_e}),中控ID注入未安装', 'WARN')
        return None


class CaptchaError(Exception):
    """识别到安全验证码弹窗(购买/出售触发),该账号判失败,人工处理。"""
    pass


def _find_game_hwnd():
    """按进程名找游戏窗口 hwnd。"""
    import ctypes
    from ctypes import wintypes
    u32 = ctypes.windll.user32
    hwnd = None
    for proc in GAME_PROCS:
        try:
            import psutil
            for p in psutil.process_iter(['name']):
                if p.info['name'] == proc:
                    pid = p.pid
                    def _cb(h, _):
                        nonlocal hwnd
                        if hwnd:
                            return
                        else:
                            pid_out = wintypes.DWORD()
                            u32.GetWindowThreadProcessId(h, ctypes.byref(pid_out))
                            if pid_out.value == pid and u32.IsWindowVisible(h):
                                    hwnd = h
                    u32.EnumWindows(ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)(_cb), 0)
                    if hwnd:
                        return hwnd
        except Exception:
            pass
    return hwnd
def _run_ammo_exchange(ctx, push_log):
    """军需处子弹兑换主流程。"""
    uname = ctx.current_username
    if not uname:
        push_log('[军需兑换] 无当前账号名,跳过', 'WARN')
        return None
    if _is_ammo_exchanged_today(uname):
        push_log(f'[军需兑换] 账号 {uname} 今日已兑换,跳过', 'INFO')
        return None
    top5 = _fetch_ammo_exchange_top5(ctx, push_log)
    if not top5:
        push_log('[军需兑换] 拉取云端排行失败,跳过', 'WARN')
        return None
    push_log(f'[军需兑换] 云端Top5: {[r["name"] for r in top5]}', 'INFO')
    hwnd = _find_game_hwnd()
    if not hwnd:
        push_log('[军需兑换] 找不到游戏窗口,跳过', 'WARN')
        return None
    try:
        _do_ammo_exchange(ctx, hwnd, top5, push_log)
        _mark_ammo_exchanged_today(uname)
        push_log(f'[军需兑换] 账号 {uname} 兑换完成', 'INFO')
    except Exception as e:
        push_log(f'[军需兑换] 兑换失败: {type(e).__name__}: {e}', 'WARN')
        _mark_ammo_exchanged_today(uname)
    return None


def _fetch_ammo_exchange_top5(ctx, push_log):
    """从云端拉取战术部门子弹利润 Top5。"""
    cfg_path = os.path.join(os.path.expanduser('~'), '.pc_tqc_client', 'config.json')
    if not os.path.exists(cfg_path):
        push_log('[军需兑换] 无云端配置文件,跳过', 'WARN')
        return None
    with open(cfg_path, 'r', encoding='utf-8-sig') as f:
        cfg = json.load(f)
    server_url = cfg.get('server_url') or cfg.get('cloud_url') or ''
    token = cfg.get('token') or ''
    if not server_url or not token:
        push_log('[军需兑换] 云端配置缺少 server_url/token,跳过', 'WARN')
        return None
    url = f'{server_url.rstrip("/")}/api/admin/ammo-exchange-rec'
    req = urllib.request.Request(url, headers={
        'Authorization': f'Bearer {token}',
        'User-Agent': 'Mozilla/5.0 Chrome/130',
    })
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            d = json.loads(r.read().decode('utf-8'))
        if not d.get('ok'):
            push_log('[军需兑换] 云端返回错误', 'WARN')
            return None
        data = d.get('data') or {}
        rows = data.get('战术部门') or []
        if not rows:
            push_log('[军需兑换] 云端无战术部门数据', 'WARN')
            return None
        return rows[:5]
    except Exception as e:
        push_log(f'[军需兑换] 拉云端排行失败: {type(e).__name__}: {e}', 'WARN')
        return None


def _is_ammo_exchanged_today(username: str) -> bool:
    """检查该账号今天是否已兑换过子弹。"""
    if not username:
        return True
    db = _local_db()
    if not os.path.exists(db):
        return False
    try:
        import sqlite3
        from datetime import date
        today = date.today().isoformat()
        conn = sqlite3.connect(db, timeout=10)
        try:
            cur = conn.cursor()
            cur.execute(f'CREATE TABLE IF NOT EXISTS {_AMMO_EXCHANGE_TABLE} (username TEXT PRIMARY KEY, exchange_date TEXT)')
            row = cur.execute(f'SELECT exchange_date FROM {_AMMO_EXCHANGE_TABLE} WHERE username=?',
                              (username,)).fetchone()
            if row and row[0] == today:
                return True
            return False
        finally:
            conn.close()
    except Exception:
        return False


def _mark_ammo_exchanged_today(username: str) -> None:
    """标记该账号今天已兑换。"""
    if not username:
        return None
    db = _local_db()
    if not os.path.exists(db):
        return None
    try:
        import sqlite3
        from datetime import date
        today = date.today().isoformat()
        conn = sqlite3.connect(db, timeout=10)
        try:
            cur = conn.cursor()
            cur.execute(f'CREATE TABLE IF NOT EXISTS {_AMMO_EXCHANGE_TABLE} (username TEXT PRIMARY KEY, exchange_date TEXT)')
            cur.execute(f'INSERT OR REPLACE INTO {_AMMO_EXCHANGE_TABLE}(username,exchange_date) VALUES(?,?)',
                        (username, today))
            conn.commit()
        finally:
            conn.close()
    except Exception:
        return None


def _do_ammo_exchange(ctx, hwnd, top5, push_log):
    """游戏内军需处兑换操作。

    ★本模块自带一套独立实现(legacy 版),与 第五步._do_ammo_exchange(新版状态感知版)
    是两套不同代码:本版用 ocr_rect/wait_kw/press_key,按固定坐标盲点执行。
    """
    import time

    def click(x, y, tag):
        """点击(参考分辨率1027x768自动缩放)。"""
        return ctx.click_ref(hwnd, x, y, 1027, 768, tag)

    def ocr_rect(x1, y1, x2, y2, tag):
        """OCR识别指定区域文字。"""
        try:
            frame = ctx.grab_frame(hwnd, tag)
            if frame is None:
                return ''
            from pc_tqc.vision.ocr_rules import parse_text
            return parse_text(frame, x1, y1, x2, y2)
        except Exception:
            return ''

    def wait(ms):
        time.sleep(ms / 1000)

    def wait_kw(kws, max_ms, region=None, exact=False, all_of=False, gone=False):
        """轮询截图OCR等关键词:命中即提前返回(加速);超时返回False,调用方照原固定
        坐标盲点——行为与改前完全一致,只快不慢、不会出叉子。

        region: 比例区域 (x1,y1,x2,y2)(0~1),限定识别范围防误识;gone=True 表示等消失。
        """
        deadline = time.time() + max_ms / 1000
        while time.time() < deadline:
            if ctx.stopped():
                return False
            frame = ctx.grab_frame(hwnd, '军需等待')
            if frame is not None:
                try:
                    items = ctx.ocr_scan(frame, '军需等待')
                except Exception:
                    items = []
                if region is not None:
                    size = ctx._client_size or (1027, 768)
                    W, H = size
                    x1, y1, x2, y2 = region
                    items = [it for it in items
                             if (x1 * W <= it[1][0] <= x2 * W) and (y1 * H <= it[1][1] <= y2 * H)]
                texts = [t.strip() for t, _ in items]
                if exact:
                    hit = any(t in kws for t in texts)
                elif all_of:
                    hit = all(any(k in t for t in texts) for k in kws)
                else:
                    hit = any(k in t for k in kws for t in texts)
                if gone:
                    if not hit:
                        return True
                elif hit:
                    return True
            wait(250)
        return False

    VK_ESCAPE = 27
    push_log('[军需兑换] 步骤1: 点部门', 'INFO')
    click(321, 34, '部门')
    wait(1500)

    push_log('[军需兑换] 步骤2: 点军需处', 'INFO')
    click(243, 61, '军需处')
    wait(1500)

    push_log('[军需兑换] 步骤3: 点战术部门', 'INFO')
    click(690, 378, '战术部门')
    wait(2000)

    exchanged_count = 0
    for idx, row in enumerate(top5, 1):
        name = row['name']
        push_log(f'[军需兑换] [{idx}/5] 尝试兑换: {name}', 'INFO')

        found = False
        click_pos = None
        for page in range(3):
            frame = ctx.grab_frame(hwnd, f'列表OCR-{idx}')
            if frame is not None:
                items = ctx.ocr_scan(frame, f'列表OCR-{idx}')
                for txt, (cx, cy) in items:
                    if name in txt:
                        found = True
                        click_pos = (cx, cy)
                        break
            if found:
                break
            click(500, 700, '滚动列表')
            wait(400)
        if not found:
            push_log(f'[军需兑换] [{idx}/5] 列表未找到 {name},跳过', 'WARN')
            continue

        push_log(f'[军需兑换] [{idx}/5] 点击 {name}', 'INFO')
        ctx.click(hwnd, click_pos[0], click_pos[1], f'点击{name}')
        wait(600)

        frame = ctx.grab_frame(hwnd, f'详情OCR-{idx}')
        if frame is not None:
            items = ctx.ocr_scan(frame, f'详情OCR-{idx}')
            panel_text = ' '.join(t for t, _ in items)
            if '已兑换' in panel_text or '限购0' in panel_text or '明日再来' in panel_text:
                push_log(f'[军需兑换] [{idx}/5] {name} 今日已兑换,跳过', 'INFO')
                continue

        push_log(f'[军需兑换] [{idx}/5] 点一键补齐', 'INFO')
        click(886, 695, '一键补齐')
        wait_kw(('一键补齐清单', '总价'), 1500)

        push_log(f'[军需兑换] [{idx}/5] 点支付确认', 'INFO')
        click(518, 496, '支付确认')
        wait_kw(('兑换',), 3000, region=(0.72, 0.82, 1.0, 1.0), exact=True)

        push_log(f'[军需兑换] [{idx}/5] 点兑换', 'INFO')
        click(886, 695, '兑换')
        wait_kw(('确认', '取消'), 1000, all_of=True)

        push_log(f'[军需兑换] [{idx}/5] 点确认', 'INFO')
        click(597, 461, '确认兑换')
        wait_kw(('确认', '取消'), 1500, all_of=True, gone=True)

        exchanged_count += 1
        push_log(f'[军需兑换] [{idx}/5] {name} 兑换完成', 'INFO')

    if exchanged_count == 0:
        push_log('[军需兑换] 5款子弹均未兑换成功', 'WARN')
    else:
        push_log(f'[军需兑换] 共兑换 {exchanged_count}/5 款子弹', 'INFO')

    push_log('[军需兑换] 步骤9: ESC回大厅', 'INFO')
    ctx.press_key(hwnd, VK_ESCAPE, 'ESC回大厅-1')
    wait(500)
    ctx.press_key(hwnd, VK_ESCAPE, 'ESC回大厅-2')
    wait(1000)


def uninstall_step_wrappers():
    """卸载所有 patch,恢复 EXE 原始方法(调试/回退用)。"""
    if not _originals:
        return None
    from pc_tqc.workflow import runner as R
    cls = R.AccountRunner
    for method_name, original in _originals.items():
        setattr(cls, method_name, original)
    _originals.clear()
    return None


def _是验证码弹窗(joined_text):
    """OCR拼接文本判定:双词同框 或 红字命中。未命中返回 False(零影响)。"""
    if not joined_text:
        return False
    else:
        if _验证码红字 in joined_text:
            return True
        else:
            return all((w in joined_text for w in _验证码双词))


def _提取OCR文本(result):
    """从引擎OCR返回值提取纯文本列表(兼容对象.txts/dict/列表多种形态)。"""
    txts = getattr(result, 'txts', None)
    if txts:
        return [str(t) for t in txts]
    else:
        if isinstance(result, dict):
            v = result.get('txts') or result.get('texts')
            if v:
                return [str(t) for t in v]
        if isinstance(result, (list, tuple)):
            out = []
            for it in result:
                if isinstance(it, str):
                    out.append(it)
                else:
                    if isinstance(it, (list, tuple)) and it and isinstance(it[0], str):
                                out.append(it[0])
            return out
        else:
            return []


def _install_ammo_exchange(ar, ctx, push_log):
    """在账号完成时执行军需处子弹兑换(每天一次)。"""
    cb = getattr(ar, 'callbacks', None)
    if cb is None:
        return
    else:
        orig = getattr(cb, 'on_account_completed', None)
        if not callable(orig):
            return
        else:
            def _wrapper(*args, **kwargs):
                try:
                    _run_ammo_exchange(ctx, push_log)
                except Exception as e:
                    push_log(f'[军需兑换] 流程异常: {type(e).__name__}: {e}', 'WARN')
                return orig(*args, **kwargs)
            try:
                cb.on_account_completed = _wrapper
                push_log('[总管] 已安装:账号完成时军需处子弹兑换', 'INFO')
            except Exception as e:
                push_log(f'[总管] 军需兑换安装失败: {e}', 'WARN')


def install_step_wrappers(ar, push_log, stop_event, account_id=None, on_account_data=None, on_account_banned=None, on_observation_detected=None, login_only=False):
    """顺序加载 第一步~第五步,把各自的 patch 装到 AccountRunner 类上。\n\n    V1.8 login_only=True(手动登录):只装载第一步,编排入口由 runner_worker\n    走引擎 run_launch_only(第一步完成后自然停止,后续步骤不执行)。"""
    ctx = StepContext(push_log, stop_event, account_id=account_id, on_account_data=on_account_data, on_account_banned=on_account_banned, on_observation_detected=on_observation_detected)
    _install_capture_guard(push_log)
    cls = ar.__class__
    if login_only:
        _install_login_only(cls, ctx, ar, push_log)
        return
    else:
        names = []
        for mod_name in STEP_MODULES:
            mod = importlib.import_module(mod_name)
            mod.install(cls, ctx)
            names.append(getattr(mod, 'STEP_NAME', mod_name))
        push_log('[总管] 已按顺序安装 ' + ' → '.join(names), 'INFO')
        _install_market_budget_guard(push_log)
        _install_captcha_guard(ctx, push_log)
        ctx.start_captcha_watcher()
        globals()['_ACTIVE_CTX'] = ctx
        _install_final_coins_report(ar, ctx, push_log)
        _install_aid_inject(ar, push_log)
        _install_login_diag(push_log)


def _install_login_only(cls, ctx, ar, push_log):
    """V1.8 手动登录模式安装:只装载第一步补丁,编排入口由 runner_worker 走\n    引擎自带 run_launch_only(内部 _run_account(launch_only=True))。\n\n    引擎探针实测:run_launch_only 的调用序列为\n    清理环境→登录Steam→启动游戏→处理启动器,之后自然返回,\n    不会进入第二步(_pre_lobby)/读数/制造/交易行 —— 天然就是\"只跑第一步\"。\n    这里只需装上第一步的日志壳与 Steam 登录兼容补丁;\n    (注:不能用 _run_account_once 当入口——它是队列续跑语义,\n    会直接跳到设施批量校验,登录步骤根本不会执行。)"""
    mod = importlib.import_module('第一步')
    mod.install(cls, ctx)
    push_log(f"[总管] 手动登录模式: 仅装载 {getattr(mod, 'STEP_NAME', '第一步')},编排走引擎 run_launch_only(第一步完成后自然停止)", 'INFO')
    ctx.start_captcha_watcher()
    globals()['_ACTIVE_CTX'] = ctx
    _install_aid_inject(ar, push_log)


def _install_market_budget_guard(push_log):
    """给 MarketSeller.run 套一层\"减价点击预算\":同一减价按钮位置被点击\n    超过 config.market_price_decrements(UI 设置)次后,后续点击直接拦截。\n\n    原版 pyd 的减价循环理论上读该配置,实测仍会多下调;这里在输入层兜底,\n    不改变任何交易逻辑。不同物品/其他按钮的点击坐标不同,天然重置计数。\n    """
    try:
        from pc_tqc.workflow import market as _market
    except Exception as e:
        push_log(f'[交易行] 模块加载失败,减价上限未生效: {type(e).__name__}: {e}', 'WARN')
        return
    seller_cls = _market.MarketSeller
    if getattr(seller_cls.run, '_tqc_budget', False):
        return
    else:
        _orig_run = seller_cls.run
        def run_with_budget(self, account, hwnd_supplier):
            try:
                budget = int(getattr(self.config, 'market_price_decrements', 5) or 0)
            except Exception:
                budget = 5
            push_log(f'[交易行] 价格下调硬上限 {budget} 次/物品(以UI设置为准)', 'INFO')
            inp = None
            for _attr in ['input', 'input_controller', '_input']:
                try:
                    inp = getattr(self, _attr)
                    if inp is not None:
                        break
                except AttributeError:
                    pass
            if inp is None:
                push_log('[交易行] 找不到输入控制器,减价上限未能生效', 'WARN')
                return _orig_run(self, account, hwnd_supplier)
            else:
                _orig_click = inp.click_client
                cluster = {'x': None, 'y': None, 'n': 0}
                def guarded_click(hwnd, x, y, *args, **kwargs):
                    cx, cy = (cluster['x'], cluster['y'])
                    if cx is not None and abs(x - cx) <= 18 and (abs(y - cy) <= 18):
                        cluster['n'] += 1
                    else:
                        cluster['x'], cluster['y'], cluster['n'] = (x, y, 1)
                    if cluster['n'] > budget:
                        push_log(f"[交易行] 第 {cluster['n']} 次减价超过 UI 上限 {budget},拦截该次点击({x},{y})", 'WARN')
                        return
                    else:
                        return _orig_click(hwnd, x, y, *args, **kwargs)
            try:
                inp.click_client = guarded_click
                return _orig_run(self, account, hwnd_supplier)
            finally:
                try:
                    inp.click_client = _orig_click
                except Exception:
                    pass
        run_with_budget._tqc_budget = True
        seller_cls.run = run_with_budget
        push_log('[交易行] 减价次数硬上限补丁已安装', 'INFO')


def _diag_fmt(v, n=100):
    try:
        s = repr(v)
    except Exception:
        s = '<repr失败>'
    if len(s) <= n:
        return s
    else:
        return s[:n] + '…'


def _diag_wi(wi):
    """把一个 WindowInfo 打印成一行:hwnd/标题/类名/进程/可见/客户区尺寸。"""
    if wi is None:
        return 'None'
    else:
        return f"hwnd={getattr(wi, 'hwnd', '?')} title={_diag_fmt(getattr(wi, 'title', ''), 40)} class={_diag_fmt(getattr(wi, 'class_name', ''), 30)} proc={_diag_fmt(getattr(wi, 'process_name', ''), 30)} visible={getattr(wi, 'visible', '?')} client={getattr(wi, 'client_width', '?')}x{getattr(wi, 'client_height', '?')}"


def _hwnd_stable(hwnd):
    """连续两次(间隔 100ms)采样都有效且客户区尺寸一致,才认为窗口稳定。"""
    ok, why, size = _hwnd_state(hwnd)
    if not ok:
        return (False, why)
    else:
        time.sleep(0.1)
        ok2, why2, size2 = _hwnd_state(hwnd)
        if not ok2:
            return (False, why2)
        else:
            if size != size2:
                return (False, f'客户区尺寸不稳定 {size[0]}x{size[1]}→{size2[0]}x{size2[1]}')
            else:
                return (True, '')


def _install_capture_guard(push_log):
    """安装 WGC 截屏防护(纯 Python 层,自身出错不影响主流程)。"""
    if _CAP_GUARD['installed']:
        return None
    _CAP_GUARD['installed'] = True

    def glog(msg, level='WARN'):
        if _CAP_GUARD['logs'] > 120:
            return None
        _CAP_GUARD['logs'] += 1
        try:
            push_log('[截屏防护] ' + msg, level)
        except Exception:
            return None

    try:
        from pc_tqc.platform import capture as _C
    except Exception as e:
        glog(f'capture 模块不可用,跳过防护: {type(e).__name__}: {e}', 'ERROR')
        return None

    if str(os.environ.get('PC_TQC_FORCE_GDI') or '') == '1':
        try:
            _orig_init = _C.CaptureManager.__init__
            def _init_force_gdi(self, preferred='wgc', *a, **k):
                return _orig_init(self, 'gdi', *a, **k)
            _init_force_gdi._tqc_guard = True
            _C.CaptureManager.__init__ = _init_force_gdi
            glog('已按执行端要求整轮改用 GDI 截屏(兼容模式)', 'WARN')
        except Exception as e:
            glog(f'切换 GDI 失败: {type(e).__name__}: {e}', 'ERROR')

    try:
        _orig_cap = _C.CaptureManager.capture
        def _capture_guarded(self, hwnd, timeout=3.0):
            now = time.time()
            cur = int(hwnd or 0)
            switched = cur != _CAP_GUARD['last_hwnd']
            gap = now - _CAP_GUARD['last_switch'] if _CAP_GUARD['last_switch'] else 1000000000.0
            too_fast = switched and gap < 1.5
            if switched:
                _CAP_GUARD['last_hwnd'] = cur
                _CAP_GUARD['last_switch'] = now
            if cur != _CAP_GUARD['ok_hwnd'] or now - _CAP_GUARD['ok_at'] > 2.0:
                ok, why = _hwnd_stable(hwnd)
                if not ok:
                    glog(f'窗口校验未通过({why}) hwnd={hwnd},本次改用 GDI 截图')
                    return self.capture_gdi(hwnd, timeout)
                _CAP_GUARD['ok_hwnd'] = cur
                _CAP_GUARD['ok_at'] = now
            if too_fast:
                glog(f'窗口切换过快(距上次 {gap:.1f}s),本次改用 GDI 截图 hwnd={hwnd}')
                return self.capture_gdi(hwnd, timeout)
            with _capture_session() as (locked, waited_ms):
                if not locked:
                    glog(f'等锁超时 {waited_ms:.0f}ms,本次改用 GDI 截图 hwnd={hwnd}', 'WARN')
                    return self.capture_gdi(hwnd, timeout)
                return _orig_cap(self, hwnd, timeout)
        _capture_guarded._tqc_guard = True
        _C.CaptureManager.capture = _capture_guarded
    except Exception as e:
        glog(f'挂截屏校验失败: {type(e).__name__}: {e}', 'ERROR')

    try:
        _orig_start = _C.WindowsGraphicsCaptureBackend._start
        def _start_guarded(self, hwnd):
            with _capture_session() as (locked, waited_ms):
                if not locked:
                    glog(f'等锁超时 {waited_ms:.0f}ms,WGC 会话创建取消,本次改用 GDI', 'WARN')
                    raise _C.CaptureError(f'截屏会话锁等待超时({waited_ms:.0f}ms),改用 GDI')
                prev = _CAP_GUARD.get('backend')
                prev_hwnd = _CAP_GUARD.get('backend_hwnd') or 0
                need_wait = False
                if prev is not None and (prev is not self or (prev_hwnd and prev_hwnd != int(hwnd or 0))):
                    ctrl = getattr(prev, '_control', None)
                    if ctrl is not None:
                        try:
                            ctrl.stop()
                            need_wait = True
                        except Exception:
                            pass
                if need_wait:
                    glog(f'WGC 切换窗口: 已停止旧会话 hwnd={prev_hwnd},等待采集线程退出 400ms 后重建')
                    time.sleep(0.4)
                _CAP_GUARD['backend'] = self
                _CAP_GUARD['backend_hwnd'] = int(hwnd or 0)
                return _orig_start(self, hwnd)
        _start_guarded._tqc_guard = True
        _C.WindowsGraphicsCaptureBackend._start = _start_guarded
    except Exception as e:
        glog(f'挂 WGC 会话串行失败: {type(e).__name__}: {e}', 'ERROR')
    return None


def _capture_session(timeout):
    """WGC 会话互斥。产出 (是否拿到锁, 等待毫秒);异常路径也会释放。"""
    t0 = time.monotonic()
    got = _CAPTURE_SESSION_LOCK.acquire(timeout=timeout)
    waited_ms = (time.monotonic() - t0) * 1000.0
    try:
        yield (got, waited_ms)
    finally:
        if got:
            _CAPTURE_SESSION_LOCK.release()


def _hwnd_state(hwnd):
    """窗口是否真实可用 → (ok, 原因, 客户区宽高)。只读校验,不碰采集。"""
    try:
        import ctypes
        from ctypes import wintypes
        if not hwnd:
            return (False, '句柄为空', (0, 0))
        u32 = ctypes.WinDLL('user32', use_last_error=True)
        k32 = ctypes.WinDLL('kernel32', use_last_error=True)
        if not u32.IsWindow(wintypes.HWND(int(hwnd))):
            return (False, '句柄已失效', (0, 0))
        if not u32.IsWindowVisible(wintypes.HWND(int(hwnd))):
            return (False, '窗口不可见', (0, 0))
        rect = wintypes.RECT()
        if not u32.GetClientRect(wintypes.HWND(int(hwnd)), ctypes.byref(rect)):
            return (False, '客户区读取失败', (0, 0))
        w, h = int(rect.right - rect.left), int(rect.bottom - rect.top)
        if w < 50 or h < 50:
            return (False, f'客户区尺寸异常 {w}x{h}', (w, h))
        pid = wintypes.DWORD(0)
        u32.GetWindowThreadProcessId(wintypes.HWND(int(hwnd)), ctypes.byref(pid))
        if not pid.value:
            return (False, '进程ID无效', (w, h))
        hproc = k32.OpenProcess(4096, False, pid.value)
        if not hproc:
            return (False, '所属进程已退出', (w, h))
        try:
            code = wintypes.DWORD(0)
            if not k32.GetExitCodeProcess(hproc, ctypes.byref(code)):
                return (False, '进程状态读取失败', (w, h))
            if code.value != 259:
                return (False, '所属进程已退出', (w, h))
        finally:
            k32.CloseHandle(hproc)
        return (True, '', (w, h))
    except Exception as e:
        return (False, f'校验异常 {type(e).__name__}', (0, 0))


def _install_login_diag(push_log):
    """安装诊断探针(只读、只打印,不改动执行端任何动作)。"""
    if _DIAG['installed']:
        return None
    _DIAG['installed'] = True

    def dlog(msg, level='INFO'):
        if _DIAG['count'] > 400:
            return None
        _DIAG['count'] += 1
        try:
            push_log('[诊断] ' + msg, level)
        except Exception:
            return None

    try:
        import windows_capture as _WC
        _cls = getattr(_WC, 'WindowsCapture', None)
        if _cls is not None and (not getattr(_cls.__init__, '_tqc_diag', False)):
            _orig_init = _cls.__init__
            def _init(self, *a, **k):
                dlog(f'WGC会话: 新建 WindowsCapture args={_diag_fmt(a, 80)} kwargs={_diag_fmt(k, 160)}', 'WARN')
                return _orig_init(self, *a, **k)
            _init._tqc_diag = True
            _cls.__init__ = _init
            for _m in ['start', 'start_free_threaded', 'stop']:
                _om = getattr(_cls, _m, None)
                if _om is None or getattr(_om, '_tqc_diag', False):
                    continue
                def _mk(orig=_om, nm=_m):
                    def _w(self, *a, **k):
                        dlog(f'WGC会话: {nm}() 调用', 'WARN')
                        try:
                            r = orig(self, *a, **k)
                            dlog(f'WGC会话: {nm}() 成功 → {_diag_fmt(r, 60)}', 'INFO')
                            return r
                        except Exception as e:
                            dlog(f'WGC会话: {nm}() 失败 {type(e).__name__}: {e}', 'ERROR')
                            raise
                    _w._tqc_diag = True
                    return _w
                setattr(_cls, _m, _mk())
            dlog('已挂 WGC 会话探针', 'WARN')
    except Exception as e:
        dlog(f'windows_capture 探针未挂上: {type(e).__name__}: {e}', 'WARN')

    try:
        from pc_tqc.platform import windows as _W
        _of = getattr(_W, 'find_window', None)
        if _of is not None and (not getattr(_of, '_tqc_diag', False)):
            _last = {'k': None}
            def find_window_diag(*a, **k):
                wi = _of(*a, **k)
                key = (_diag_fmt(a, 60), _diag_fmt(sorted(((str(kk), str(vv)) for kk, vv in k.items())), 160))
                if key != _last['k']:
                    _last['k'] = key
                    dlog(f'找窗口 find_window(args={_diag_fmt(a, 60)}, kwargs={_diag_fmt(k, 160)}) → {_diag_wi(wi)}', 'WARN' if wi is None else 'INFO')
                return wi
            find_window_diag._tqc_diag = True
            _W.find_window = find_window_diag
            dlog('已挂 find_window 探针', 'WARN')
    except Exception as e:
        dlog(f'find_window 探针未挂上: {type(e).__name__}: {e}', 'WARN')

    try:
        from pc_tqc.platform import capture as _C
        _oc = getattr(_C, 'capture_visible_client', None)
        if _oc is not None and (not getattr(_oc, '_tqc_diag', False)):
            _lastc = {'k': None}
            def capture_visible_client_diag(hwnd, *a, **k):
                try:
                    fr = _oc(hwnd, *a, **k)
                except Exception as e:
                    dlog(f'捕获 capture_visible_client(hwnd={hwnd}) 异常 {type(e).__name__}: {e}', 'ERROR')
                    raise
                shp = getattr(fr, 'shape', None)
                key = (hwnd, str(shp))
                if key != _lastc['k']:
                    _lastc['k'] = key
                    if fr is None:
                        dlog(f'捕获 capture_visible_client(hwnd={hwnd}) → None(捕获失败)', 'WARN')
                        return fr
                    else:
                        dlog(f'捕获 capture_visible_client(hwnd={hwnd}) → OK shape={shp}', 'INFO')
                return fr
            capture_visible_client_diag._tqc_diag = True
            _C.capture_visible_client = capture_visible_client_diag
            dlog('已挂 capture_visible_client 探针', 'WARN')
    except Exception as e:
        dlog(f'capture 探针未挂上: {type(e).__name__}: {e}', 'WARN')

    try:
        from pc_tqc.platform import windows as _W2
        _ol = getattr(_W2, 'list_windows', None)
        if _ol is not None and (not getattr(_ol, '_tqc_diag', False)):
            _lastl = {'k': None, 't': 0.0}
            def list_windows_diag(*a, **k):
                wins = _ol(*a, **k)
                try:
                    steam = [w for w in wins or [] if 'steam' in str(getattr(w, 'process_name', '') or '').lower() or 'steam' in str(getattr(w, 'title', '') or '').lower()]
                    key = tuple(sorted((_diag_wi(w) for w in steam)))
                    now = time.time()
                    if key != _lastl['k'] and now - _lastl['t'] >= 3.0:
                        _lastl['k'], _lastl['t'] = (key, now)
                        dlog(f'窗口普查: 可见窗口共 {len(wins or [])} 个,Steam 相关 {len(steam)} 个', 'WARN')
                        for w in steam:
                            dlog('    · ' + _diag_wi(w), 'WARN')
                except Exception:
                    pass
                return wins
            list_windows_diag._tqc_diag = True
            _W2.list_windows = list_windows_diag
            dlog('已挂 list_windows 探针', 'WARN')
    except Exception as e:
        dlog(f'list_windows 探针未挂上: {type(e).__name__}: {e}', 'WARN')


class StepContext:
    """共享工具上下文:日志、停止信号、OCR、游戏窗口、鼠标键盘。

    各步骤文件通过 install(cls, ctx) 拿到本对象,自己不维护这些单例。
    """

    def __init__(self, push_log, stop_event, account_id=None, on_account_data=None, on_account_banned=None, on_observation_detected=None):
        self._push_log = push_log
        self._stop_event = stop_event
        self._ocr = None
        self._inp = None
        self._client_size = None
        self.account_id = account_id
        self._on_account_data = on_account_data
        self._on_account_banned = on_account_banned
        self._on_observation_detected = on_observation_detected
        self._banned_reported = set()
        self._observation_reported = set()
        self.current_username = ''
        self.last_ban = None
        self._grab_fail_streak = 0
        self._error_dialog_last = 0.0
        self._last_dialog_n = 0

    def begin_account(self, username):
        """每开始执行一个账号时调用:记录账号名,并清空封禁去重集合。

        注意:封禁去重必须以账号名为键,且每个新账号开始时重置——
        否则上游拿不到用户名(空串)时,第一个账号把 '' 写进去后,
        后续所有账号都会被误判为"已上报过"而跳过回传。
        """
        self.current_username = username or ''
        self._banned_reported.clear()
        self._observation_reported.clear()
        self.last_ban = None
        self._captcha_hit = None

    def log(self, tag, msg, level='INFO'):
        self._push_log(f'[{tag}] {msg}', level)

    def report_account_data(self, level=None, coins=None, username=''):
        """把读到的行动等级/哈夫币回传给中控(写回 SQLite 并刷新页面)。"""
        cb = self._on_account_data
        if cb is None:
            self.log('数据回传', f'未注册回传回调,仅记录:level={level} coins={coins}', 'WARN')
            return None
        try:
            cb(level, coins, username)
            return None
        except Exception as e:
            self.log('数据回传', f'回传失败: {type(e).__name__}: {e}', 'ERROR')
            return None

    def detect_ban(self, items):
        """识别封禁页(2026-10-19 改版,覆盖十年封禁/冻结N天/临时封禁等全部变体)。

        判定条件(三者同时满足):
          ① 按钮强特征:「帮助/退出游戏/申诉/自助服务/返回登录」同页出现 >= 2 个;
          ② 封禁词:「封禁/冻结/作弊/第三方/模拟器/环境异常/行为数据/账号异常」命中 >= 1;
          ③ 必须解析出解封/解冻日期(解析不出日期一律不判封禁,防误报;
             OCR 写成「解冻」时先替换成「解封」再交给原版日期解析)。

        返回 {'unban_at': 'YYYY-MM-DD HH:MM:SS', 'source': 文案} 或 None。
        """
        if not items:
            return None
        try:
            from pc_tqc.vision.ocr_rules import parse_account_ban_date
        except Exception as e:
            self.log('封禁检测', f'ocr_rules 导入失败: {type(e).__name__}: {e}', 'WARN')
            return None
        joined = ' '.join(str(t) for t, _ in items)
        btn_hits = 0
        for w in ('帮助', '退出游戏', '申诉', '自助服务', '返回登录'):
            if w in joined:
                btn_hits += 1
        if btn_hits < 2:
            return None
        if not any(w in joined for w in ('封禁', '冻结', '作弊', '第三方', '模拟器', '环境异常', '行为数据', '账号异常')):
            return None
        unban = None
        try:
            unban = parse_account_ban_date(joined)
        except Exception:
            unban = None
        if unban is None:
            if '解冻' in joined:
                try:
                    unban = parse_account_ban_date(joined.replace('解冻', '解封'))
                except Exception:
                    unban = None
        if unban is None:
            return None
        return {'unban_at': unban.strftime('%Y-%m-%d %H:%M:%S'), 'source': joined[:200], 'items': items}

    def raise_if_banned(self, items, hwnd, username, tag):
        """检测到封禁页 → 回传云控 → 点"退出游戏" → 抛异常切下一个账号。"""
        ban = self.detect_ban(items)
        if not ban:
            return None
        username = username or self.current_username or ''
        unban_str = ban['unban_at']
        self.log(tag, f'★ 检测到封禁页面!账号 {username} 解封时间: {unban_str or "未识别"}', 'ERROR')
        self.last_ban = {'username': username, 'unban_at': unban_str}
        if username not in self._banned_reported:
            self._banned_reported.add(username)
            cb = self._on_account_banned
            if cb is not None:
                try:
                    cb(username, unban_str, ban['source'])
                    self.log(tag, '封禁状态已回传云控 ✓', 'WARN')
                except Exception as e:
                    self.log(tag, f'封禁状态回传失败: {type(e).__name__}: {e}', 'ERROR')
        hit_exit = self.find_kw(items, ('退出游戏',)) or self.find_kw(items, ('返回登录',))
        if hit_exit and hwnd:
            px, py = hit_exit[1]
            self.log(tag, f'点击「退出游戏」({px},{py}) 退出当前账号')
            self.click(hwnd, px, py, tag)
            import time as _t
            _t.sleep(3)
        raise RuntimeError(f'ACCOUNT_BANNED: 账号 {username} 已封禁,解封时间 {unban_str or "未知"}')

    def detect_observation(self, items):
        """识别观察期页面(GTI 安全风险提示)。

        页面特征:"观察期" / "GTI" / "安全风险" /
                "解除时间:2026年10月05日09时57分37秒"
        返回 {'until': 'YYYY-MM-DD HH:MM:SS'(可能为空), 'source': 文案} 或 None。

        OCR 经常把一行文字切成多段、且在冒号/数字之间塞空格,
        所以先把所有空白删掉再做正则(空白容忍),避免识别不到解除时间。
        """
        if not items:
            return None
        import re
        joined = ' '.join(str(t) for t, _ in items)
        flat = re.sub(r'\s+', '', joined)
        if '观察期' not in flat:
            return None
        until = ''
        m = re.search(r'解除时间[:：]?\s*(\d{4})\s*年\s*(\d{1,2})\s*月\s*(\d{1,2})\s*日\s*(\d{1,2})\s*时\s*(\d{1,2})\s*分\s*(\d{1,2})\s*秒', flat)
        if m:
            y, mo, d, h, mi, s = m.groups()
            until = f'{y}-{int(mo):02d}-{int(d):02d} {int(h):02d}:{int(mi):02d}:{int(s):02d}'
        return {'until': until, 'source': joined[:200]}

    def handle_observation(self, items, hwnd, username, tag):
        """检测到观察期页面 → 回传云控解除时间 → 点"确定"关闭 → 继续运行。

        观察期账号仍可执行制造等任务(仅限制部分玩法),点确定后继续流程。
        与封禁不同:封禁点"退出游戏"并切下一个账号,观察期只关页面继续跑。
        """
        obs = self.detect_observation(items)
        if not obs:
            return None
        username = username or self.current_username or ''
        until_str = obs['until']
        self.log(tag, f'★ 检测到观察期页面!账号 {username} 解除时间: {until_str or "未识别"}', 'WARN')
        if username not in self._observation_reported:
            self._observation_reported.add(username)
            cb = self._on_observation_detected
            if cb is not None:
                try:
                    cb(username, until_str)
                    self.log(tag, '观察期状态已回传云控 ✓', 'WARN')
                except Exception as e:
                    self.log(tag, f'观察期回传失败: {type(e).__name__}: {e}', 'ERROR')
        hit_ok = self.find_kw(items, ('确定',))
        clicked = False
        if hit_ok and hwnd:
            px, py = hit_ok[1]
            self.log(tag, f'点击「确定」({px},{py}) 关闭观察期页面,继续执行')
            clicked = self.click_foreground(hwnd, px, py, tag)
        if not clicked and hwnd:
            self.log(tag, '未点到「确定」按钮,兜底按回车关闭观察期页面', 'WARN')
            self.key_foreground(hwnd, 13, tag)
        import time as _t
        _t.sleep(2)
        return None

    def stopped(self):
        return self._stop_event.is_set()

    def get_original(self, cls, name):
        """取 EXE 原始方法(只备份一次,避免重复安装时套娃)。"""
        if name not in _originals:
            _originals[name] = getattr(cls, name)
        return _originals[name]

    def patch_with(self, cls, name, builder):
        """builder(original) -> 新方法;替换到类上。"""
        original = self.get_original(cls, name)
        setattr(cls, name, builder(original))
        return None

    def ocr_scan(self, frame, tag='OCR'):
        """对一帧只做一次 OCR,返回 [(text, (cx, cy)), ...](客户区像素坐标)。"""
        if frame is None:
            return []
        ocr = self._get_ocr(tag)
        if ocr is None:
            return []
        try:
            result = ocr(frame, return_word_box=True)
        except CaptchaError:
            raise
        except Exception as e:
            self.log(tag, f'OCR 调用异常: {type(e).__name__}: {e}', 'WARN')
            return []
        txts = getattr(result, 'txts', None)
        boxes = getattr(result, 'boxes', None)
        if txts is None or boxes is None:
            return []
        try:
            import numpy as np
        except Exception:
            return []
        items = []
        for txt, box in zip(list(txts), list(boxes)):
            if not txt or box is None:
                continue
            try:
                arr = np.array(box, dtype=float)
            except Exception:
                continue
            if arr.ndim == 2 and arr.shape[0] >= 2:
                cx, cy = int(arr[:, 0].mean()), int(arr[:, 1].mean())
            elif arr.ndim == 1 and arr.shape[0] >= 4:
                cx, cy = int((arr[0] + arr[2]) / 2), int((arr[1] + arr[3]) / 2)
            else:
                continue
            items.append((txt, (cx, cy)))
        if items and _是验证码弹窗(' '.join(t for t, _ in items)):
            self._captcha_hit = 'ocr'
            self.log(tag, '该账号出现了验证码弹窗,判定失败,切下一个账号', 'ERROR')
            raise CaptchaError('该账号出现了验证码弹窗')
        self._check_captcha_window_throttled(tag)
        return items

    def _get_ocr(self, tag):
        if self._ocr is not None:
            return self._ocr
        try:
            from pc_tqc.vision.ocr import RapidOCR
        except Exception as e:
            self.log(tag, f'RapidOCR 导入失败: {type(e).__name__}: {e}', 'ERROR')
            return None
        self.log(tag, '首次加载 OCR 模型(约 1-2s)…')
        try:
            self._ocr = RapidOCR()
        except Exception as e:
            self.log(tag, f'RapidOCR 初始化失败: {type(e).__name__}: {e}', 'ERROR')
            return None
        return self._ocr

    @staticmethod
    def find_kw(items, keywords):
        """OCR 结果里找第一个包含任一关键词的文字,返回 (text, (x,y)) 或 None。"""
        for txt, center in items:
            for kw in keywords:
                if kw in txt:
                    return (txt, center)
        return None

    def detect_captcha_window(self, tag='验证码检测'):
        """枚举所有可见顶层窗口,查找安全验证弹窗(独立窗口,游戏截图抓不到)。
        优先用窗口标题匹配(快、无需OCR);标题不中再对小尺寸候选窗OCR兜底。
        命中即抛 CaptchaError。返回是否找到。
        """
        try:
            from pc_tqc.platform.windows import list_windows
            from pc_tqc.platform.capture import capture_visible_client
        except Exception:
            return False
        try:
            wins = list_windows(visible_only=True)
        except Exception:
            return False
        _标题词 = ('验证码', '安全验证')
        _内容词 = ('安全验证', '安全挑战', '账号存在异常状态', '是人类吗')
        for wi in wins:
            try:
                title = getattr(wi, 'title', '') or ''
                if any(kw in title for kw in _标题词):
                    self._captcha_hit = title
                    self.log(tag, f'★ 检测到验证码弹窗(标题匹配): "{title}"', 'ERROR')
                    raise CaptchaError('该账号出现了验证码弹窗')
                cw = getattr(wi, 'client_width', 0) or 0
                ch = getattr(wi, 'client_height', 0) or 0
                if not (200 <= cw <= 700 and 150 <= ch <= 600):
                    continue
                frame = capture_visible_client(wi.hwnd)
                if frame is None:
                    continue
                items = self.ocr_scan(frame, tag)
                joined = ' '.join(t for t, _ in items)
                if any(kw in joined for kw in _内容词):
                    self._captcha_hit = 'ocr'
                    self.log(tag, f'★ 检测到验证码弹窗(OCR匹配): "{joined[:80]}"', 'ERROR')
                    raise CaptchaError('该账号出现了验证码弹窗')
            except CaptchaError:
                raise
            except Exception:
                continue
        return False

    def _check_captcha_window_throttled(self, tag):
        """节流版验证码窗口检测:默认 3 秒一次,避免每帧枚举拖慢流程。
        被 ocr_scan 调用时不会递归死锁——再次进入 detect_captcha_window 的 OCR 兜底
        会调 ocr_scan,但节流间隔未到直接 return。
        """
        now = time.time()
        if now - getattr(self, '_captcha_check_last', 0) < 3:
            return None
        self._captcha_check_last = now
        self.detect_captcha_window(tag)
        return None

    def start_captcha_watcher(self):
        """启动验证码弹窗后台巡检(幂等;守护线程随 worker 进程退出)。"""
        if getattr(self, '_captcha_watch_started', False):
            return None
        self._captcha_watch_started = True
        if not hasattr(self, '_captcha_hit'):
            self._captcha_hit = None
        import threading

        def _loop():
            while True:
                time.sleep(3)
                hit = None
                try:
                    from pc_tqc.platform.windows import list_windows
                    for wi in list_windows(visible_only=True):
                        title = getattr(wi, 'title', '') or ''
                        if '验证码' in title or '安全验证' in title:
                            hit = title
                            break
                except Exception:
                    continue
                if hit and not self._captcha_hit:
                    self._captcha_hit = hit
                    self.log('验证码巡检', f'★ 检测到验证码弹窗(后台巡检,标题匹配): "{hit}" —— 引擎阶段内无法实时中断,本轮该账号将判失败', 'ERROR')

        threading.Thread(target=_loop, daemon=True, name='pc-tqc-captcha-watch').start()
        return None

    def resolve_hwnd(self, runner_self, tag):
        """取游戏窗口句柄:优先 runner 的 _game_hwnd,失效则按进程名重查。"""
        raw = getattr(runner_self, '_game_hwnd', None)
        if callable(raw):
            try:
                raw = raw()
            except Exception as e:
                self.log(tag, f'_game_hwnd() 异常: {type(e).__name__}: {e}', 'WARN')
                raw = None
        if isinstance(raw, int) and raw:
            return raw
        try:
            from pc_tqc.platform.windows import find_window
        except Exception as e:
            self.log(tag, f'windows 模块导入失败: {type(e).__name__}: {e}', 'ERROR')
            return None
        for proc in GAME_PROCS:
            try:
                wi = find_window(process_name=proc, visible_only=True)
            except Exception:
                continue
            if wi is None:
                continue
            self.log(tag, f'已定位游戏窗口 {proc} hwnd={wi.hwnd}')
            return wi.hwnd
        return None

    def grab_frame(self, hwnd, tag):
        """截游戏客户区画面;顺便记录客户区尺寸供参考坐标缩放。失败返回 None。

        异常场景自愈(对应实测问题):
          · 启动器弹"登录组件报错(1067105),游戏被强制关闭"弹窗 → 自动点掉;
          · 游戏崩溃/被强制关闭,旧 hwnd 失效 → 重新按进程名找窗口;
          · 连续失败 3 次且进程或窗口确实没了 → 直接抛异常切下一个账号,
            绝不卡在"截图异常:窗口句柄无效"死循环里空转。
        """
        try:
            from pc_tqc.platform.capture import capture_visible_client
            frame = capture_visible_client(hwnd)
        except Exception as e:
            self._grab_fail_streak += 1
            self.log(tag, f'截图异常: {type(e).__name__}: {e} | hwnd={hwnd!r}', 'WARN')
            if self._grab_fail_streak >= 3:
                try:
                    self.dismiss_launcher_error(tag)
                except Exception:
                    pass
                if not self._is_game_alive():
                    self._grab_fail_streak = 0
                    raise RuntimeError('游戏进程已退出(可能被启动器强制关闭),切换账号')
                if not self._find_game_hwnd():
                    self._grab_fail_streak = 0
                    raise RuntimeError('游戏窗口已丢失(崩溃或被强制关闭),切换账号')
                self._grab_fail_streak = 0
            return None
        self._grab_fail_streak = 0
        if frame is not None:
            h, w = frame.shape[:2]
            self._client_size = (w, h)
        return frame

    def recover_game_hwnd(self, prev_hwnd, tag):
        """截图失败/句柄可疑时调用:点错误弹窗 → 返回当前可用游戏窗口句柄。

        返回:可用 hwnd(int);找不到返回 None(调用方应结束本账号流程)。
        """
        try:
            self.dismiss_launcher_error(tag)
        except Exception:
            pass
        if self._is_window(prev_hwnd):
            return prev_hwnd
        hw = self._find_game_hwnd()
        if hw and hw != prev_hwnd:
            self.log(tag, f'已重新定位游戏窗口 hwnd={hw}', 'WARN')
        return hw

    @staticmethod
    def _is_window(hwnd):
        """Win32 IsWindow:句柄是否仍对应一个真实窗口。"""
        try:
            import ctypes
            return bool(hwnd) and bool(ctypes.windll.user32.IsWindow(int(hwnd)))
        except Exception:
            return bool(hwnd)

    def _find_game_hwnd(self):
        """按游戏进程名重新查找可见窗口,找不到返回 None。"""
        try:
            from pc_tqc.platform.windows import find_window
        except Exception:
            return None
        for proc in GAME_PROCS:
            try:
                wi = find_window(process_name=proc, visible_only=True)
            except Exception:
                continue
            if wi is None:
                continue
            if self._is_window(wi.hwnd):
                return wi.hwnd
        return None

    def _is_game_alive(self):
        """检查游戏进程(DeltaForceClient)是否仍在运行。"""
        try:
            import psutil
        except Exception:
            try:
                import subprocess
                out = subprocess.run(['tasklist', '/FI', f'IMAGENAME eq {GAME_PROCS[0]}'], capture_output=True, text=True, timeout=5).stdout
                return GAME_PROCS[0].lower() in out.lower()
            except Exception:
                return True
        for proc in psutil.process_iter(['name']):
            try:
                name = proc.info.get('name', '')
                if name in GAME_PROCS:
                    return True
            except Exception:
                continue
        return False

    _ERROR_DIALOG_PROCS = ('df_launcher.exe', 'launcher.exe', 'DeltaForceClient.exe', 'DeltaForceClient-Win64-Shipping.exe')
    _ERROR_DIALOG_KEYWORDS = ('登录组件', '启动组件', '强制关闭', '1067105', '2007105', '需要重新启动')
    _ERROR_DIALOG_BUTTONS = ('重新启动', '启动', '确定', '关闭')

    def dismiss_launcher_error(self, tag, force=False):
        """检测并点掉启动器"登录组件报错(1067105)/游戏被强制关闭"弹窗。

        实测该弹窗属于启动器窗口(游戏窗口此时已失效),所以不能只截游戏窗口,
        要枚举启动器/游戏相关顶层窗口逐个 OCR,找到后用 Win32 前台点击按钮。
        3 秒节流,避免每帧枚举拖慢流程。返回本次关掉的弹窗数。
        """
        import time as _t
        now = _t.time()
        if not force and now - self._error_dialog_last < 3:
            return 0
        self._error_dialog_last = now
        try:
            from pc_tqc.platform.windows import list_windows
            from pc_tqc.platform.capture import capture_visible_client
        except Exception as e:
            self.log(tag, f'弹窗检测模块加载失败: {type(e).__name__}: {e}', 'WARN')
            return 0
        closed = 0
        try:
            wins = list_windows(visible_only=True)
        except Exception:
            return 0
        for wi in wins:
            try:
                pname = (getattr(wi, 'process_name', '') or '').lower()
                title = getattr(wi, 'title', '') or ''
                if pname not in self._ERROR_DIALOG_PROCS and not (('三角洲' in title) or ('Delta' in title) or ('启动器' in title)):
                    continue
                cw = getattr(wi, 'client_width', 0) or 0
                ch = getattr(wi, 'client_height', 0) or 0
                if cw < 200 or ch < 120:
                    continue
                dlg_hwnd = wi.hwnd
                frame = None
                try:
                    frame = capture_visible_client(dlg_hwnd)
                except Exception:
                    continue
                if frame is None:
                    continue
                items = self.ocr_scan(frame, tag)
                joined = ' '.join(str(t) for t, _ in items)
                if not any(kw in joined for kw in self._ERROR_DIALOG_KEYWORDS):
                    continue
                self.log(tag, f'★ 发现启动器错误弹窗({pname or title}):{joined[:80]}', 'WARN')
                hit = self.find_kw(items, self._ERROR_DIALOG_BUTTONS)
                if hit is None:
                    self.key_foreground(dlg_hwnd, 13, tag)
                else:
                    bx, by = hit[1]
                    self.log(tag, f'点击弹窗按钮「{hit[0]}」({bx},{by}) 关闭错误弹窗')
                    self.click_foreground(dlg_hwnd, bx, by, tag)
                closed += 1
                _t.sleep(1.5)
            except Exception:
                continue
        if closed:
            self._last_dialog_n = closed
            self.log(tag, f'已关闭 {closed} 个启动器错误弹窗,继续流程', 'WARN')
        return closed

    def click_foreground(self, hwnd, x, y, tag):
        """Win32 前台点击(激活窗口→SetCursorPos→SendInput)。

        与第三步同款,走 EXE 的 click_client_foreground;
        即使 Interception 驱动没装也能用,用于点观察期/封禁/启动器弹窗按钮。
        """
        inp = self._get_input(tag)
        if inp is not None:
            try:
                inp.click_client_foreground(hwnd, x, y)
                return True
            except Exception as e:
                self.log(tag, f'前台点击失败,尝试纯Win32: {type(e).__name__}: {e}', 'WARN')
        return self._win32_click_raw(hwnd, x, y, tag)

    def _win32_click_raw(self, hwnd, x, y, tag):
        """纯 ctypes Win32 兜底点击:ClientToScreen + SetCursorPos + 鼠标事件。"""
        try:
            import ctypes
            from ctypes import wintypes
            u32 = ctypes.windll.user32
            u32.SetForegroundWindow.argtypes = [wintypes.HWND]
            u32.SetForegroundWindow.restype = wintypes.BOOL
            try:
                u32.SetForegroundWindow(int(hwnd))
            except Exception:
                pass
            pt = wintypes.POINT(int(x), int(y))
            u32.ClientToScreen.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.POINT)]
            if not u32.ClientToScreen(int(hwnd), ctypes.byref(pt)):
                self.log(tag, f'Win32点击放弃:窗口句柄 {hwnd} 无效', 'WARN')
                return False
            u32.SetCursorPos(pt.x, pt.y)
            time.sleep(0.15)
            u32.mouse_event(2, 0, 0, 0, 0)
            u32.mouse_event(4, 0, 0, 0, 0)
            return True
        except Exception as e:
            self.log(tag, f'纯Win32点击失败: {type(e).__name__}: {e}', 'ERROR')
            return False

    def key_foreground(self, hwnd, vk, tag):
        """纯 Win32 按键(前台),不依赖 Interception 驱动(回车/Esc 兜底用)。"""
        try:
            import ctypes
            u32 = ctypes.windll.user32
            try:
                u32.SetForegroundWindow(int(hwnd))
            except Exception:
                pass
            u32.keybd_event(int(vk), 0, 0, 0)
            u32.keybd_event(int(vk), 0, 2, 0)
            return True
        except Exception as e:
            self.log(tag, f'Win32按键失败(vk={vk:#x}): {type(e).__name__}: {e}', 'ERROR')
            return False

    def _get_input(self, tag):
        if self._inp is None:
            try:
                from pc_tqc.platform.input import InputController
                self._inp = InputController(logger=logging.getLogger('pc_tqc.fallback'))
            except Exception as e:
                self.log(tag, f'InputController 初始化失败: {type(e).__name__}: {e}', 'ERROR')
                return None
        return self._inp

    def click(self, hwnd, x, y, tag):
        inp = self._get_input(tag)
        if inp is None:
            return False
        try:
            inp.click_client(hwnd, x, y)
            return True
        except Exception as e:
            self.log(tag, f'鼠标点击异常: {type(e).__name__}: {e}', 'ERROR')
            return False

    def key(self, hwnd, vk, tag):
        inp = self._get_input(tag)
        if inp is None:
            return False
        try:
            inp.key_click(hwnd, vk)
            return True
        except Exception as e:
            self.log(tag, f'按键异常(vk={vk:#x}): {type(e).__name__}: {e}', 'ERROR')
            return False

    def click_ref(self, hwnd, x, y, ref_w, ref_h, tag):
        """按参考分辨率坐标点击,自动缩放到当前游戏客户区实际尺寸。"""
        size = self._client_size
        if size is None:
            self.grab_frame(hwnd, tag)
            size = self._client_size
        if size:
            sx = int(x * size[0] / ref_w)
            sy = int(y * size[1] / ref_h)
        else:
            sx, sy = x, y
            self.log(tag, '拿不到客户区尺寸,参考坐标未缩放直接点击', 'WARN')
        return self.click(hwnd, sx, sy, tag)