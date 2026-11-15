# Decompiled with PyLingual (https://pylingual.io)
# Internal filename: '第二步.py'
# Bytecode version: 3.12.0rc2 (3531)
# Source timestamp: 1970-01-01 00:00:00 UTC (0)

import json
import os
import re
import sys
import time
import urllib.request
try:
    from sig_verify import CloudSignatureError, verify_response_signature
except ImportError:
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '_internal', 'client'))
    from sig_verify import CloudSignatureError, verify_response_signature
STEP_NAME = '第二步 · 进入游戏大厅'
def _弹窗日志(username, event):
    """回归弹窗识别/按键事件落盘(运行日志/回归弹窗.log),事后可查\"认没认到、按了没\"。"""
    try:
        from workflow_steps import popup_event_log
        popup_event_log(username, '[第二步] ' + event)
    except Exception:
        return None
TIMEOUT = 300
VK_空格 = 32
VK_TAB = 9
VK_ESC = 27
空轮询间隔 = 2
_DEFAULT_RULES = [{'name': '回归配装页', 'priority': 10, 'keywords': ['确认配装', '制式套装使用中', '战前准备'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': [], 'action_type': 'key_press', 'action_param': '0x1B', 'cooldown_sec': 4, 'max_count': 2, 'wait_after': 3}, {'name': '选地图页', 'priority': 11, 'keywords': ['任务列表', '常规行动', '机密行动', '准入价值'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': [], 'action_type': 'key_press', 'action_param': '0x1B', 'cooldown_sec': 4, 'max_count': 6, 'wait_after': 3}, {'name': '主大厅', 'priority': 20, 'keywords': ['仓库', '特战干员', '交易行', '改枪台', '特勤处'], 'match_mode': 'count', 'match_count': 2, 'exclude_keywords': ['确认配装', '制式套装', '战前准备', '任务列表', '常规行动', '机密行动'], 'action_type': 'success', 'action_param': '', 'cooldown_sec': 0, 'max_count': 0, 'wait_after': 0}, {'name': '执勤报道-点领取', 'priority': 30, 'keywords': ['执勤报道', '领取'], 'match_mode': 'all', 'match_count': 2, 'exclude_keywords': [], 'action_type': 'click', 'action_param': 'ocr_coord', 'cooldown_sec': 4, 'max_count': 3, 'wait_after': 3}, {'name': '执勤报道-空格兜底', 'priority': 31, 'keywords': ['执勤报道'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': ['领取'], 'action_type': 'key_press', 'action_param': '0x20', 'cooldown_sec': 4, 'max_count': 5, 'wait_after': 3}, {'name': '特别行动', 'priority': 32, 'keywords': ['特别行动', '开始行动'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': [], 'action_type': 'key_press', 'action_param': '0x1B', 'cooldown_sec': 4, 'max_count': 8, 'wait_after': 3}, {'name': '重返行动', 'priority': 33, 'keywords': ['重返行动'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': [], 'action_type': 'key_press', 'action_param': '0x1B', 'cooldown_sec': 4, 'max_count': 5, 'wait_after': 3}, {'name': '活动页-继续', 'priority': 40, 'keywords': ['继续'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': [], 'action_type': 'key_press', 'action_param': '0x20', 'cooldown_sec': 4, 'max_count': 5, 'wait_after': 3}, {'name': '模式入口-前往游玩', 'priority': 50, 'keywords': ['前往游玩'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': [], 'action_type': 'click', 'action_param': 'ocr_coord', 'cooldown_sec': 6, 'max_count': 3, 'wait_after': 5}, {'name': '3D特勤处', 'priority': 60, 'keywords': ['开始游戏', '医疗部门', '研发部门'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': ['确认配装', '制式套装', '战前准备', '任务列表', '常规行动'], 'action_type': 'key_press', 'action_param': '0x09', 'cooldown_sec': 10, 'max_count': 3, 'wait_after': 8}, {'name': '大厅半边菜单', 'priority': 90, 'keywords': ['退出'], 'match_mode': 'any', 'match_count': 1, 'exclude_keywords': [], 'action_type': 'key_press', 'action_param': '0x1B', 'cooldown_sec': 4, 'max_count': 2, 'wait_after': 3}]
def _读取云端配置():
    """从 ~/.pc_tqc_client/config.json 读取 server_url 和 token。"""
    cfg_path = os.path.join(os.path.expanduser('~'), '.pc_tqc_client', 'config.json')
    if not os.path.exists(cfg_path):
        return ('', '')
    else:
        try:
            with open(cfg_path, 'r', encoding='utf-8-sig') as f:
                cfg = json.load(f)
            return (cfg.get('server_url', '') or cfg.get('cloud_url', ''), cfg.get('token', ''))
        except Exception:
            return ('', '')
def _客户端版本():
    """上报给云端的客户端版本:云端据此给新客户端发 fixed 规则,老客户端(无此头)继续发老格式。"""
    try:
        from cloud_client import APP_VERSION
        return APP_VERSION
    except Exception:
        return '1.9.20261115'
def _拉取云端规则(ctx):
    """从云端拉取动作规则;签名校验失败直接中断(不回落到内置规则)。"""
    server_url, token = _读取云端配置()
    if not server_url or not token:
        return None
    url = f"{server_url.rstrip('/')}/api/client/action-rules"
    req = urllib.request.Request(
        url,
        headers={
            'Authorization': f'Bearer {token}',
            'User-Agent': 'Mozilla/5.0 Chrome/130',
            'X-Client-Version': _客户端版本(),
        })
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            raw = r.read()
            verify_response_signature('GET', url, raw, r.headers)
            data = json.loads(raw.decode('utf-8'))
        if data.get('ok') and data.get('rules'):
            return data['rules']
        return None
    except CloudSignatureError as e:
        ctx.log(STEP_NAME, str(e), 'ERROR')
        raise RuntimeError(str(e)) from e
    except Exception as e:
        ctx.log(STEP_NAME, f'云端规则拉取失败,使用内置默认规则: {type(e).__name__}: {e}', 'WARN')
        return None

def _匹配规则(items, rule):
    """判断 OCR 结果是否命中某条规则。返回 (命中词, 坐标) 或 None。\n\n    match_mode: any(任一命中) / all(全部命中) / count(命中数>=match_count)\n    exclude_keywords: 任一命中则不匹配\n    """
    keywords = rule.get('keywords') or []
    exclude = rule.get('exclude_keywords') or []
    if not keywords:
        return
    else:
        for txt, _ in items:
            for ek in exclude:
                if ek in txt:
                    return
        hits = []
        for txt, coord in items:
            for kw in keywords:
                if kw in txt:
                    hits.append((kw, coord))
                    break
        if not hits:
            return
        else:
            mode = rule.get('match_mode', 'any')
            if mode == 'all':
                hit_kws = {h[0] for h in hits}
                if not all((kw in hit_kws for kw in keywords)):
                    return
                else:
                    return hits[0]
            else:
                if mode == 'count':
                    need = int(rule.get('match_count', 1))
                    if len({h[0] for h in hits}) < need:
                        return
                return hits[0]
_RE_FIXED = re.compile('^fixed\\s*:\\s*([-+]?\\d+(?:\\.\\d+)?)\\s*,\\s*([-+]?\\d+(?:\\.\\d+)?)$', re.I)
def _解析比例(seg):
    """解析 \"fixed:x,y\" 里的 0~1 比例;格式不对返回 None。"""
    m = _RE_FIXED.match(str(seg).strip())
    if not m:
        return
    else:
        try:
            return (float(m.group(1)), float(m.group(2)))
        except (TypeError, ValueError):
            return None
def _解析点击参数(param):
    """解析 click 规则的 action_param,返回 (OCR关键词 or None, fixed比例 or None)。\n\n    \"ocr_coord\"                          → (None, None)   老逻辑:点规则命中词坐标\n    \"ocr_coord:进入游戏\"                  → (\'进入游戏\', None)\n    \"ocr_coord:进入游戏|fixed:0.48,0.58\"  → (\'进入游戏\', (0.48, 0.58))\n    \"fixed:0.48,0.58\"                     → (None, (0.48, 0.58))  纯固定坐标,不做 OCR 找字\n    任何解析失败都当作「没有 fixed」,老规则行为完全不变。\n    """
    # ***<module>._解析点击参数: Failure: Different bytecode
    if not isinstance(param, str):
        return (None, None)
    else:
        text = param.strip()
        if text.startswith('fixed'):
            return (None, _解析比例(text))
        else:
            if not text.startswith('ocr_coord') or ':' not in text:
                return (None, None)
            else:
                parts = text.split(':', 1)[1].split('|')
                kw = parts[0].strip() or None
                fixed = None
                for seg in parts[1:]:
                    fixed = _解析比例(seg)
                    if fixed is not None:
                        return (kw, fixed)
                return (kw, fixed)
def _找关键词坐标(items, kw):
    """在当前这一屏 OCR 结果里找关键词的文字框中心点。"""
    if not kw:
        return
    else:
        for txt, coord in items:
            if kw in txt:
                return coord
        return
def _比例坐标(ctx, frame, fixed):
    """把 0~1 比例换算成客户区像素;比例或结果越界返回 None(调用方不点击)。"""
    # ***<module>._比例坐标: Failure: Compilation Error
    size = getattr(ctx, '_client_size', None)
    if not size and frame is not None:
            try:
                h, w = frame.shape[:2]
                size = (w, h)
            except Exception:
                size = None
    if not size:
        return
    else:
        w, h = (int(size[0]), int(size[1]))
        if w <= 0 or h <= 0:
            return None
        else:
            rx, ry = fixed
            if not (0.0 <= rx <= 1.0 and 0.0 <= ry <= 1.0):
                return
            else:
                px, py = (int(round(rx * w)), int(round(ry * h)))
                if not (0 <= px < w and 0 <= py < h):
                    return (px, py)
                return None

class _OkResult:
    """EXE runner 期望的 _pre_lobby 返回值接口(.ok/.details 等)。"""
    ok = True
    error = None
    stage = 'pre_lobby'
    def __init__(self, message=''):
        self.message = message
        self.details = {}
    def __getattr__(self, name):
        return {}
def _flow(self, ctx, started_at, username=''):
    """规则引擎:每轮 OCR 后按优先级匹配规则,执行对应动作,直到到达主大厅。"""
    hwnd = ctx.resolve_hwnd(self, STEP_NAME)
    if not hwnd:
        raise RuntimeError('第二步: 找不到游戏窗口')
    else:
        rules = _拉取云端规则(ctx)
        if rules is None:
            rules = _DEFAULT_RULES
            ctx.log(STEP_NAME, f'使用内置默认规则({len(rules)}条)')
        else:
            ctx.log(STEP_NAME, f'已从云端拉取规则({len(rules)}条)')
        deadline = started_at + TIMEOUT
        last_act = {}
        counts = {}
        fixed_warned = set()
        for r in rules:
            key = r.get('id') or r.get('name')
            last_act[key] = 0
            counts[key] = 0
        ctx.log(STEP_NAME, '目标图3主大厅 · 规则引擎驱动(按优先级匹配页面动作)')
        while time.time() < deadline:
            if ctx.stopped():
                raise RuntimeError('第二步: 已停止')
            frame = ctx.grab_frame(hwnd, STEP_NAME)
            if frame is None:
                new_hwnd = ctx.recover_game_hwnd(hwnd, STEP_NAME)
                if not new_hwnd:
                    raise RuntimeError('第二步: 游戏窗口已丢失(可能被启动器强制关闭),切换账号')
                else:
                    hwnd = new_hwnd
                    time.sleep(空轮询间隔)
                    continue
            items = ctx.ocr_scan(frame, STEP_NAME)
            if not items:
                new_hwnd = ctx.recover_game_hwnd(hwnd, STEP_NAME)
                if not new_hwnd:
                    raise RuntimeError('第二步: 游戏窗口已丢失且画面无内容,切换账号')
                else:
                    hwnd = new_hwnd
            ctx.handle_observation(items, hwnd, username, STEP_NAME)
            ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
            acted = False
            for rule in rules:
                key = rule.get('id') or rule.get('name')
                cooldown = int(rule.get('cooldown_sec', 4))
                max_cnt = int(rule.get('max_count', 5))
                if time.time() - last_act[key] < cooldown:
                    continue
                else:
                    if max_cnt > 0 and counts[key] >= max_cnt:
                            continue
                    hit = _匹配规则(items, rule)
                    if hit is None:
                        continue
                    else:
                        action = rule.get('action_type')
                        param = rule.get('action_param', '')
                        name = rule.get('name', key)
                        if action == 'success':
                            hit_kw = hit[0] if isinstance(hit, (list, tuple)) else hit
                            ctx.log(STEP_NAME, f'规则「{name}」命中「{hit_kw}」,已到达主大厅 ✓')
                            return _OkResult(f'规则引擎完成:已到主大厅({name})')
                        else:
                            if action == 'key_press':
                                try:
                                    vk = int(param, 16) if isinstance(param, str) and param.startswith('0x') else int(param)
                                except (ValueError, TypeError):
                                    ctx.log(STEP_NAME, f'规则「{name}」action_param无效: {param}', 'WARN')
                                else:
                                    if not ctx.key_foreground(hwnd, vk, STEP_NAME):
                                        continue
                                    else:
                                        last_act[key] = time.time()
                                        counts[key] += 1
                                        hit_kw = hit[0] if isinstance(hit, (list, tuple)) else hit
                                        ctx.log(STEP_NAME, f'规则「{name}」命中「{hit_kw}」,已按 0x{vk:02X}(第{counts[key]}次)')
                                        _弹窗日志(username, f'{name} 命中「{hit_kw}」,按 0x{vk:02X}(第{counts[key]}次)')
                                        time.sleep(int(rule.get('wait_after', 3)))
                                        acted = True
                                        break
                            else:
                                if action == 'click':
                                    ocr_kw, fixed = _解析点击参数(param)
                                    coord = _找关键词坐标(items, ocr_kw) if ocr_kw else None
                                    used_fixed = False
                                    if coord is None and fixed is not None:
                                            coord = _比例坐标(ctx, frame, fixed)
                                            used_fixed = True
                                            if coord is None:
                                                warn_key = (name, fixed)
                                                if warn_key not in fixed_warned:
                                                    fixed_warned.add(warn_key)
                                                    ctx.log(STEP_NAME, f'规则「{name}」' + (f'OCR 未命中「{ocr_kw}」,' if ocr_kw else '') + f'固定坐标越界(比例={fixed[0]:g},{fixed[1]:g}),不点击', 'WARN')
                                                continue
                                            else:
                                                if ocr_kw:
                                                    ctx.log(STEP_NAME, f'规则「{name}」OCR 未命中「{ocr_kw}」,使用固定坐标兜底,点击({coord[0]},{coord[1]})', 'WARN')
                                                else:
                                                    ctx.log(STEP_NAME, f'规则「{name}」命中页面,按固定坐标点击({coord[0]},{coord[1]})')
                                    if coord is None:
                                        coord = hit[1] if isinstance(hit, (list, tuple)) and len(hit) > 1 else None
                                    if coord is None:
                                        continue
                                    else:
                                        px, py = coord
                                        if not ctx.click(hwnd, px, py, STEP_NAME):
                                            continue
                                        else:
                                            last_act[key] = time.time()
                                            counts[key] += 1
                                            hit_kw = hit[0]
                                            ctx.log(STEP_NAME, f'规则「{name}」命中「{hit_kw}」,已点击({px},{py})(第{counts[key]}次)' + ('(固定坐标)' if used_fixed else ''))
                                            _弹窗日志(username, f'{name} 命中「{hit_kw}」,点击@({px},{py})(第{counts[key]}次)')
                                            time.sleep(int(rule.get('wait_after', 3)))
                                            acted = True
                                            break
            if not acted:
                time.sleep(空轮询间隔)
        raise RuntimeError('第二步: 超时仍未进入主大厅(规则引擎未收敛)')
def install(cls, ctx):
    """总管调用:替换 EXE 的 _pre_lobby。"""
    def _make(original):
        def wrapped(self, *args, **kwargs):
            username = ''
            state = kwargs.get('state')
            if state is None and args:
                    state = args[0]
            if state is not None:
                try:
                    username = state.account.username or ''
                except Exception:
                    username = ''
            if not username:
                username = getattr(ctx, 'current_username', '') or ''
            ctx.log(STEP_NAME, f'>>> 开始(预算超时 {TIMEOUT}s)')
            started = time.time()
            try:
                result = _flow(self, ctx, started, username=username)
                ctx.log(STEP_NAME, f'<<< 完成({time.time() - started:.1f}s)')
                return result
            except Exception as e:
                ctx.log(STEP_NAME, f'!!! 异常({time.time() - started:.1f}s): {type(e).__name__}: {e}', 'ERROR')
                raise
        return wrapped
    ctx.patch_with(cls, '_pre_lobby', _make)