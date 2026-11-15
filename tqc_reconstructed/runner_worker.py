# Decompiled with PyLingual (https://pylingual.io)
# Internal filename: 'runner_worker.py'
# Bytecode version: 3.12.0rc2 (3531)
# Source timestamp: 1970-01-01 00:00:00 UTC (0)

"""runner_worker.py — 运行 EXE 自动化工作流的【独立子进程】\n\n为什么要单独进程:\n    EXE 编译模块(pc_tqc.workflow.runner)在收到停止信号、或流程结束清理时,\n    内部会调用 os._exit() 直接终止整个 Python 进程。如果和 Flask 跑在同一进程,\n    中控网页会一起挂掉。把 runner 放进这个独立子进程后,os._exit() 只杀本子进程,\n    Flask 主进程毫发无损,停止按钮也只影响本子进程。\n\n协议(子进程 → 主进程,stdout,每行一个 JSON):\n    {\"type\": \"log\",      \"text\": \"...\", \"level\": \"INFO|WARN|ERROR\"}\n    {\"type\": \"status\",   \"status\": \"...\", \"stage\": \"...\", \"message\": \"...\", \"account\": \"...\"}\n    {\"type\": \"facility\", \"account\": \"...\", \"timers\": {\"tech\": \"2026-09-30 20:00:00\", ...}}\n    {\"type\": \"done\",     \"success\": true|false}\n\n停止信号(主进程 → 子进程):\n    主进程创建停止标志文件,子进程的 stop_event 定期检查该文件是否存在,\n    存在则视为停止。这样 EXE 内部能走自己的清理流程(即使最后 os._exit 也只杀子进程)。\n\n用法:\n    python runner_worker.py <config_json_path>\nconfig JSON 字段:\n    account, password, platform, launch_only, login_only, account_id, accounts_list\n    (login_only=True: V1.8 手动登录,只执行第一步登录平台并进入游戏)\n"""
global _HAF_RE
import json
import os
import sys
import time
import tempfile
import threading
import logging
import contextlib
if getattr(sys, 'frozen', False):
    _HERE = os.path.dirname(sys.executable)
else:
    _HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)
for _s in [sys.stdout, sys.stderr]:
    try:
        _s.reconfigure(encoding='utf-8', errors='replace')
    except Exception:
        pass
def _resolve_engine_dir():
    _cands = (os.path.join(_HERE, '_internal', 'engine'), os.path.join(_HERE, 'PC特勤处V1.0.66.exe_extracted'), os.path.join(os.path.dirname(_HERE), '_internal', 'engine'), os.path.join(os.path.dirname(_HERE), 'PC特勤处V1.0.66.exe_extracted'))
    for _p in _cands:
        if os.path.isdir(_p):
            return _p
    return _cands[0]
EXTRACTED = _resolve_engine_dir()
if EXTRACTED not in sys.path:
    sys.path.insert(0, EXTRACTED)
for _sub in ['win32', os.path.join('win32', 'lib'), 'pythonwin']:
    _p = os.path.join(EXTRACTED, _sub)
    if os.path.isdir(_p) and _p not in sys.path:
            sys.path.append(_p)
for _d in [EXTRACTED, os.path.join(EXTRACTED, 'numpy.libs'), os.path.join(EXTRACTED, 'Shapely.libs'), os.path.join(EXTRACTED, 'cv2'), os.path.join(EXTRACTED, 'onnxruntime', 'capi'), os.path.join(EXTRACTED, 'PySide6'), os.path.join(EXTRACTED, 'pywin32_system32'), os.path.join(EXTRACTED, 'pythonwin')]:
    if os.path.isdir(_d):
        os.environ['PATH'] = _d + os.pathsep + os.environ.get('PATH', '')
        try:
            os.add_dll_directory(_d)
        except (AttributeError, OSError):
            pass
try:
    import driver_check as _driver_check
    _driver_check._prime_dll_path()
except Exception as _de:
    print(f'[runner_worker] driver_check 预加载失败: {_de}', file=sys.stderr)
_STOP_FLAG = os.path.join(tempfile.gettempdir(), 'pc_tqc_client_stop.flag')
_EMIT_LOCK = threading.Lock()
def _emit(obj):
    """向 stdout 输出一行 JSON(主进程的读取线程会解析)。"""
    with _EMIT_LOCK:
        try:
            sys.stdout.write(json.dumps(obj, ensure_ascii=False) + '\n')
            sys.stdout.flush()
        except Exception:
            pass
class _Heartbeat(threading.Thread):
    """心跳线程:每30秒向主进程报一次存活,与引擎状态无关。\n\n    正常的设施滚动等待可能几小时不输出业务日志,主进程靠心跳区分\n    \"正常等待\"和\"真卡死/已退出\"。"""
    INTERVAL = 30
    def __init__(self):
        super().__init__(daemon=True)
    def run(self):
        while True:
            time.sleep(self.INTERVAL)
            _emit({'type': 'heartbeat', 'ts': time.time()})
def _log(text, level='INFO'):
    _emit({'type': 'log', 'text': text, 'level': level})
def _local_db_path():
    """runner 本地调度库路径: 桌面/PC特勤处/pc_tqc.db(与主进程口径一致)。"""
    import winreg
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders') as key:
            desktop = os.path.expandvars(str(winreg.QueryValueEx(key, 'Desktop')[0]).strip())
            if desktop:
                return os.path.join(desktop, 'PC特勤处', 'pc_tqc.db')
    except Exception:
        pass
    return os.path.join(os.path.expanduser('~'), 'Desktop', 'PC特勤处', 'pc_tqc.db')

_HAF_RE = None
def _parse_haf_coins(raw):
    """本地库 haf_coins(如 \'50,653K\' / \'1.2M\' / \'50653000\') → 整数。失败返回 None。"""
    global _HAF_RE
    if raw is None:
        return
    else:
        if _HAF_RE is None:
            import re
            _HAF_RE = re.compile('([0-9]{1,3}(?:,[0-9]{3})*|[0-9]+)\\s*([KMkm]?)')
        m = _HAF_RE.search(str(raw).replace(' ', ''))
        if not m:
            return
        else:
            val = int(m.group(1).replace(',', ''))
            suf = (m.group(2) or '').upper()
            if suf == 'K':
                val *= 1000
                return val
            else:
                if suf == 'M':
                    val *= 1000000
                return val
def _read_local_final(username):
    """从本地调度库读该账号最新 (level, haf_coins_int)。读不到返回 (None, None)。"""
    if not username:
        return (None, None)
    else:
        import sqlite3
        db = _local_db_path()
        if not os.path.exists(db):
            return (None, None)
        else:
            try:
                conn = sqlite3.connect(db, timeout=10)
                try:
                    cur = conn.cursor()
                    row = cur.execute('SELECT level, haf_coins FROM account_state WHERE username=? ORDER BY rowid DESC LIMIT 1', (username,)).fetchone()
                finally:
                    conn.close()
            except Exception:
                return (None, None)
            if not row:
                return (None, None)
            else:
                lvl, raw = row
                coins = _parse_haf_coins(raw)
                if lvl:
                    return (str(lvl).strip(), coins)
                else:
                    return ('', coins)
def _backup_account_state(username):
    """快照该账号的 account_state 行。返回 (cols, row);row=None 表示跑前无记录。"""
    if not username:
        return
    else:
        import sqlite3
        db = _local_db_path()
        if not os.path.exists(db):
            return
        else:
            try:
                conn = sqlite3.connect(db, timeout=10)
                try:
                    cols = [c[1] for c in conn.execute('PRAGMA table_info(account_state)')]
                    row = conn.execute('SELECT * FROM account_state WHERE username=?', (username,)).fetchone()
                finally:
                    conn.close()
            except Exception:
                return None
            return (cols, row)
def _restore_account_state(username, bak):
    """把手动登录期间引擎对 account_state 的写入原样盖回快照。"""
    if username and bak is None:
        return None
    else:
        cols, row = bak
        import sqlite3
        db = _local_db_path()
        if not os.path.exists(db):
            return
        else:
            try:
                conn = sqlite3.connect(db, timeout=10)
                try:
                    exists = conn.execute('SELECT COUNT(*) FROM account_state WHERE username=?', (username,)).fetchone()[0]
                    if row is None:
                        if exists:
                            conn.execute('DELETE FROM account_state WHERE username=?', (username,))
                    else:
                        if exists:
                            sets = ', '.join((f'{c}=?' for c in cols if c != 'account_key'))
                            vals = [row[cols.index(c)] for c in cols if c != 'account_key']
                            conn.execute(f'UPDATE account_state SET {sets} WHERE username=?', (*vals, username))
                        else:
                            ph = ', '.join('?' * len(cols))
                            conn.execute(f"INSERT INTO account_state ({', '.join(cols)}) VALUES ({ph})", tuple(row))
                    conn.commit()
                finally:
                    conn.close()
            except Exception:
                return
@contextlib.contextmanager
def _chdir_to_extracted():
    prev = os.getcwd()
    os.chdir(EXTRACTED)
    try:
        yield
    finally:
        os.chdir(prev)
class _FileStopEvent:
    """跨进程停止信号:主进程创建标志文件 → 子进程检测到即视为停止。\n\n    同时保留 threading.Event 接口(EXE 内部调用 .is_set()/.wait()),\n    这样 AccountRunner 收到的 stop_event 行为和原来一致。\n    """
    def __init__(self, flag_path):
        self._flag = flag_path
        self._evt = threading.Event()
        self._t = threading.Thread(target=self._watch, daemon=True)
        self._t.start()
    def _watch(self):
        # irreducible cflow, using cdg fallback
        while not self._evt.is_set():
            try:
                if os.path.exists(self._flag):
                    self._evt.set()
                    return
                time.sleep(0.5)
            except Exception:
                pass

    def is_set(self):
        return self._evt.is_set()
    def set(self):
        self._evt.set()
    def wait(self, timeout=None):
        return self._evt.wait(timeout)
_FACILITY_MAP = {'tech': 'tech', '技术': 'tech', '技术中心': 'tech', 'workbench': 'workbench', '工作台': 'workbench', 'pharmacy': 'pharmacy', '制药': 'pharmacy', '制药台': 'pharmacy', 'armor': 'armor', '防具': 'armor', '防具台': 'armor'}
def _stage_to_facility(stage):
    s = str(stage).lower()
    for k, v in _FACILITY_MAP.items():
        if k in s:
            return v
def main():
    if len(sys.argv) < 2:
        _log('runner_worker: 缺少配置文件参数', 'ERROR')
        sys.exit(2)
    cfg_path = sys.argv[1]
    try:
        if cfg_path == '--stdin-config':
            _stdin = getattr(sys.stdin, 'buffer', None)
            if _stdin is not None:
                _raw = _stdin.read()
            else:
                with os.fdopen(0, 'rb') as _f:
                    _raw = _f.read()
            cfg = json.loads(_raw.decode('utf-8-sig'))
        else:
            with open(cfg_path, 'r', encoding='utf-8-sig') as f:
                cfg = json.load(f)
    except Exception as e:
        _log(f'runner_worker: 读取配置失败 {e}', 'ERROR')
        sys.exit(2)
    try:
        _env_cfg = {k: v for k, v in cfg.items() if k not in ['password', 'token', 'server_url', 'cloud_url']}
        _lst = _env_cfg.get('accounts_list')
        if isinstance(_lst, list):
            _env_cfg['accounts_list'] = [{k: v for k, v in a.items() if k != 'password'} if isinstance(a, dict) else a for a in _lst]
        os.environ['PC_TQC_RUNNER_CFG'] = json.dumps(_env_cfg, ensure_ascii=False)
        def _b(v):
            if isinstance(v, str):
                return v.strip().lower() in ['1', 'true', 'on', 'yes', 'y', '开', '开启', '是']
            else:
                return bool(v)
        _log('界面开关: 子弹兑换=%s, 研发兑换=%s, 出售时段=%s' % ('开' if _b(_env_cfg.get('ammo_exchange_enabled')) else '关', '开' if _b(_env_cfg.get('aw_enabled')) else '关', '开(%s-%s)' % (_env_cfg.get('sell_window_start'), _env_cfg.get('sell_window_end')) if _b(_env_cfg.get('sell_window_enabled')) else '关'), 'INFO')
    except Exception:
        pass
    account = cfg.get('account', '')
    password = cfg.get('password', '')
    platform = cfg.get('platform', 'Steam')
    launch_only = cfg.get('launch_only', False)
    login_only = cfg.get('login_only', False)
    account_id = cfg.get('account_id')
    accounts_list = cfg.get('accounts_list')
    server_url = cfg.get('server_url') or cfg.get('cloud_url', '')
    token = cfg.get('token', '')
    if not server_url or not token:
        try:
            _cc = os.path.join(os.path.expanduser('~'), '.pc_tqc_client', 'config.json')
            with open(_cc, 'r', encoding='utf-8-sig') as _f:
                _cj = json.load(_f)
            server_url = server_url or _cj.get('server_url') or _cj.get('cloud_url') or ''
            token = token or _cj.get('token') or ''
            if server_url and token:
                    _log(f'runner_worker: 云端凭证取自主程序配置 {_cc}', 'INFO')
        except Exception:
            pass
    acct_id_map = {}
    if isinstance(accounts_list, list):
        for a in accounts_list:
            if isinstance(a, dict) and a.get('account') and (a.get('id') is not None):
                        acct_id_map[str(a['account'])] = a['id']
    def _id_of(name):
        return acct_id_map.get(str(name)) if name else account_id
    try:
        if os.path.exists(_STOP_FLAG):
            os.remove(_STOP_FLAG)
    except Exception:
        pass
    _Heartbeat().start()
    with _chdir_to_extracted():
        from pc_tqc.workflow import runner as runner_mod
        from pc_tqc import models as models_mod
        from pc_tqc import config as cfg_mod
        from workflow_steps import install_step_wrappers
        try:
            from pc_tqc.workflow import rules as _rules_mod
            _rules_mod.has_saved_action_level = lambda _level: False
            runner_mod.has_saved_action_level = lambda _level: False
            _log('行动等级: 已启用每轮必读(has_saved_action_level 恒 False)', 'INFO')
        except Exception as _lv_err:
            _log(f'行动等级每轮必读补丁未生效(沿用引擎默认): {type(_lv_err).__name__}: {_lv_err}', 'WARN')
        try:
            _drv = _driver_check.ensure()
            _log(f"键盘驱动: {_drv['message']}", 'INFO' if _drv['installed'] else 'WARN')
            if not _drv['installed']:
                if _drv.get('need_reboot'):
                    _log('键盘驱动已安装,请重启 Windows 后重新打开执行端', 'WARN')
                else:
                    _log('键盘驱动未安装,游戏内输入可能失败。请用管理员身份运行执行端自动安装,或手动安装 install-interception.exe', 'WARN')
        except Exception as _e:
            _log(f'键盘驱动检测失败: {_e}', 'WARN')

        stop_event = _FileStopEvent(_STOP_FLAG)
        platform_mode = 'steam' if platform == 'Steam' else 'wgc'
        config = cfg_mod.AppConfig(license_card='', platform_mode=platform_mode)
        tp = cfg.get('trade_price_adjust')
        if tp is not None:
            try:
                config.market_price_decrements = int(tp)
                _log(f'交易行价格下调次数: {tp}', 'INFO')
            except Exception:
                pass
        try:
            config.stage_timeouts['steam_login'] = 300
            _log('Steam 登录超时放宽: 90s → 300s', 'INFO')
        except Exception as _te:
            _log(f'Steam 登录超时配置失败(沿用默认): {_te}', 'WARN')

        steam_path = runner_mod.discover_steam_executable(config.steam_path)
        wegame_path = runner_mod.discover_wegame_executable(config.wegame_path)
        _log(f'启动: {account} | 平台 {platform} | Steam={steam_path} WeGame={wegame_path}', 'INFO')
        def _on_log_cb(msg):
            _log(msg, 'INFO')
        def _captcha_note(uname):
            try:
                import workflow_steps as _ws
                _ctx = getattr(_ws, '_ACTIVE_CTX', None)
                if _ctx is not None and getattr(_ctx, '_captcha_hit', None):
                    _cur = getattr(_ctx, 'current_username', '') or ''
                    if _cur and uname and _cur == uname:
                        return '(该账号出现了验证码弹窗,需人工处理后恢复)'
                return ''
            except Exception:
                return ''

        def _on_account_update_cb(state):
            uname = state.account.username if state.account else None
            _st = str(state.status)
            _msg = state.message or ''
            if '失败' in _st or 'FAIL' in _st.upper() or '异常' in _msg:
                _msg = _msg + _captcha_note(uname)
            _emit({'type': 'status', 'status': _st, 'stage': str(state.stage), 'message': _msg, 'account': uname or '', 'account_id': _id_of(uname)})
            fr = getattr(state, 'facility_next_run_at', {}) or {}
            if fr:
                timers = {}
                for stage, dt in fr.items():
                    if dt:
                        fac = _stage_to_facility(stage)
                        if fac:
                            timers[fac] = dt.strftime('%Y-%m-%d %H:%M:%S')
                if timers:
                    _emit({'type': 'facility', 'account': uname or '', 'account_id': _id_of(uname), 'timers': timers})
        def _on_account_completed_cb(state):
            uname = state.account.username if state.account else ''
            _log(f'账号 {uname} 完成,阶段 {state.stage}', 'INFO')
            try:
                lvl, coins = _read_local_final(uname)
                if coins is not None:
                    _emit({'type': 'account_data', 'account': uname, 'account_id': _id_of(uname) or account_id, 'level': lvl or '', 'hafu_coins': coins})
                    _log(f'已从本地库读取最终哈夫币 {coins} 回传云端', 'INFO')
                else:
                    _log(f'本地库未读到 {uname} 的哈夫币,暂不回传', 'WARN')
            except Exception as _e:
                _log(f'本地库回传哈夫币失败: {type(_e).__name__}: {_e}', 'WARN')
        def _on_observation_detected_cb(account_name, until):
            _emit({'type': 'observation', 'account': account_name or '', 'account_id': _id_of(account_name), 'until': until or ''})
        def _on_finished_cb(success):
            if login_only:
                return
            else:
                _log(f"任务执行结束 ({('成功' if success else '失败')})", 'INFO' if success else 'ERROR')
                _emit({'type': 'done', 'success': bool(success)})
        def _on_game_window_ready_cb():
            _log('游戏窗口已就绪', 'INFO')
        def _on_account_data_cb(level, coins, username=''):
            """第三步读到行动等级/哈夫币 → 回传中控写库。"""
            _emit({'type': 'account_data', 'account': username or account, 'account_id': _id_of(username) or account_id, 'level': level, 'hafu_coins': coins})
        def _on_account_banned_cb(username, unban_at, source_text):
            """第二/三/四步 OCR 识别到封禁页 → 通知主进程回传云控。"""
            _log(f"★ 账号 {username} 封禁,解封时间 {unban_at or '未知'}", 'ERROR')
            _emit({'type': 'banned', 'account': username or '', 'account_id': _id_of(username) or account_id, 'unban_at': unban_at or '', 'source_text': source_text or ''})
        callbacks = runner_mod.RunnerCallbacks(on_log=_on_log_cb, on_account_update=_on_account_update_cb, on_account_completed=_on_account_completed_cb, on_observation_detected=_on_observation_detected_cb, on_finished=_on_finished_cb, on_game_window_ready=_on_game_window_ready_cb)
        _cloud_client = None
        if server_url and token:
            try:
                _cc_path = os.path.join(_HERE, '_internal', 'client')
                if _cc_path not in sys.path:
                    sys.path.insert(0, _cc_path)
                from cloud_client import CloudClient
                _cloud_client = CloudClient(server_url.rstrip('/'), token)
                _log(f'已接入云端制造配置接口 {server_url}', 'INFO')
            except Exception as _ce:
                _log(f'云端客户端初始化失败(无 cloud_client 模块?): {_ce}', 'WARN')

        _exe_manufacture = None
        def _manufacture_plan_loader(aid):
            """EXE AccountRunner 回调:按 account_id 返回 ManufacturePlan 或 None。"""
            nonlocal _exe_manufacture
            if not _cloud_client or not aid:
                return None
            else:
                try:
                    plan = _cloud_client.get_manufacture_config(int(aid))
                    _CUSTOM_ALIASES = {'top1', 'custom', 'workshop', 'manual'}
                except Exception as _e:
                    if '云端响应校验失败' in str(_e):
                        _log(f'拉取云端制造配置失败 aid={aid}: {_e}', 'ERROR')
                        raise RuntimeError(str(_e)) from _e
                    else:
                        _log(f'拉取云端制造配置失败 aid={aid}: {type(_e).__name__}: {_e}', 'WARN')
                        return None
                def _norm_setting(s):
                    d = dict(s or {})
                    m = (d.get('mode') or 'pinned').lower()
                    if m in _CUSTOM_ALIASES:
                        d['mode'] = 'custom'
                    else:
                        if m == 'pinned' or not m:
                            d['mode'] = 'pinned'
                    if 'itemId' in d and 'item_id' not in d:
                            d['item_id'] = d['itemId']
                    return d
                settings = {'tech': _norm_setting(getattr(plan, 'tech', {})), 'workbench': _norm_setting(getattr(plan, 'workbench', {})), 'pharmacy': _norm_setting(getattr(plan, 'pharmacy', {})), 'armor': _norm_setting(getattr(plan, 'armor', {}))}
                try:
                    import 第四步 as _step4
                    _step4.登记候选(aid, settings)
                except Exception as _se:
                    _log(f'候选表登记失败 aid={aid}: {type(_se).__name__}: {_se}', 'WARN')
                has_any = any((s.get('mode') and s.get('mode') != 'pinned' or s.get('pinned') for s in settings.values()))
                if not has_any:
                    return
                else:
                    payload = {'catalogVersion': getattr(plan, 'catalog_version', 2) or 2, 'manufactureCatalogVersion': getattr(plan, 'catalog_version', 2) or 2, 'manufactureConfigVersion': getattr(plan, 'config_version', 0) or 0, 'manufactureConfig': settings}
                    if _exe_manufacture is None:
                        from pc_tqc import manufacture as _exe_manufacture
                    try:
                        exe_plan = _exe_manufacture.parse_manufacture_plan(payload)
                        modes_hint = ' '.join((f"{k}={v.get('mode', 'pinned')}" for k, v in settings.items()))
                        _log(f'✓ 已装载制造计划 aid={aid} [{modes_hint}]', 'INFO')
                        return exe_plan
                    except Exception as _pe:
                        _log(f'解析 EXE 制造计划失败 aid={aid}: {type(_pe).__name__}: {_pe}', 'WARN')
        ar = runner_mod.AccountRunner(config=config, stop_event=stop_event, logger=logging.getLogger('pc_tqc.runner'), callbacks=callbacks, state_store=None, manufacture_plan_loader=_manufacture_plan_loader, manufacture_account_ids=None, online_guard=None)
        install_step_wrappers(ar, _log, stop_event, account_id=account_id, on_account_data=_on_account_data_cb, on_account_banned=_on_account_banned_cb, on_observation_detected=_on_observation_detected_cb, login_only=login_only)
        _queue_mode = bool(not launch_only and (not login_only) and accounts_list and isinstance(accounts_list[0], dict))
        _log('任务开始', 'INFO')
        if not _queue_mode:
            _emit({'type': 'status', 'status': '运行中', 'stage': '等待开始', 'message': '', 'account': account, 'account_id': account_id})
        success = False
        try:
            def _mk_account(name, pwd, aid):
                acct = models_mod.Account(username=name, password=pwd)
                try:
                    acct.account_id = aid
                except Exception:
                    pass
                return acct
            if launch_only:
                acct = models_mod.Account(username=account, password=password)
                try:
                    acct.account_id = account_id or _id_of(account)
                except Exception:
                    pass
                state = models_mod.AccountRunState(account=acct, index=0)
                os.environ['PC_TQC_MANUAL_EXEC'] = '1'
                try:
                    ar.run_launch_only(state)
                finally:
                    os.environ.pop('PC_TQC_MANUAL_EXEC', None)
            else:
                if login_only:
                    acct = models_mod.Account(username=account, password=password)
                    try:
                        acct.account_id = account_id or _id_of(account)
                    except Exception:
                        pass
                    state = models_mod.AccountRunState(account=acct, index=0)
                    _log('手动登录模式: 仅执行第一步,登录平台并进入游戏后结束', 'INFO')
                    _state_bak = _backup_account_state(account)
                    try:
                        os.environ['PC_TQC_MANUAL_EXEC'] = '1'
                        ar.run_launch_only(state)
                    except:
                        pass
                    os.environ.pop('PC_TQC_MANUAL_EXEC', None)
                    _restore_account_state(account, _state_bak)
                    _aid = account_id or _id_of(account)
                    if getattr(state, 'status', None) == models_mod.RunStatus.SUCCESS:
                        _emit({'type': 'status', 'status': '成功', 'stage': '手动登录', 'message': '登录完成,可手动操作该账号', 'account': account, 'account_id': _aid})
                    else:
                        _fail_msg = getattr(state, 'message', '') or '第一步执行失败'
                        _log(f'手动登录失败: {_fail_msg}', 'ERROR')
                        _emit({'type': 'status', 'status': '失败', 'stage': '手动登录', 'message': _fail_msg, 'account': account, 'account_id': _aid})
                else:
                    if accounts_list and isinstance(accounts_list[0], dict):
                        accts = [_mk_account(a['account'], a['password'], a.get('id', a.get('account_id'))) for a in accounts_list]
                    else:
                        if accounts_list:
                            accts = accounts_list
                        else:
                            accts = [_mk_account(account, password, account_id)]
                    _log(f'队列调度: 共 {len(accts)} 个账号,按工作台倒计时自动接力', 'INFO')
                    ar.run(accts)
            success = True
        except SystemExit:
            _log('工作流进程已退出(SystemExit)', 'WARN')
        except Exception as e:
            _log(f'任务异常: {type(e).__name__}: {e}', 'ERROR')
        finally:
            _emit({'type': 'done', 'success': success})


if __name__ == '__main__':
    main()
