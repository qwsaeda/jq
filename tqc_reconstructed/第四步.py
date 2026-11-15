# 第四步.py — 设施/交易行准备（重建版：忠实 100% 语义转储 analysis_biz.txt）
# 职责: 阶段4(设施准备/交易行前置) + 模式B 自定义跳过流程(OCR 判定锁标,全锁则跳过本轮)
# 关键: _SkipResult 是与引擎 StepResult 同构的"跳过"兜底对象(引擎 runner.pyd 读 .ok/.skipped/.reason)

import json, os, re, time

STEP_NAME = '第四步'


class _SkipResult:
    """模式B 跳过设施时返回给 runner 的兜底结果对象。"""
    ok = True
    skipped = True
    reason = 'facilities_locked'

    def __init__(self, message=''):
        self.message = message
        self.results = []

    def __getattr__(self, name):
        return {}




def install(cls, ctx):
    """总管调用:包装 EXE 的 _facility_batch 与 _facility(两个设施阶段都挂验证码兜底)。"""

    def _压制重试(self, state, username):
        _far = time.time() + 21600
        _far_str = time.strftime('%H:%M:%S', time.localtime(_far))
        ok = []

        def _set_dict(d):
            try:
                d['failed_retry_after'] = _far
                return True
            except Exception:
                return False

        def _set_attr(o):
            try:
                o.failed_retry_after = _far
                return True
            except Exception:
                return False

        if state is not None:
            if _set_attr(state):
                ok.append('state.attr')
            try:
                if _set_dict(state.__dict__):
                    ok.append('state.__dict__')
            except Exception:
                pass
            if isinstance(state, dict):
                if _set_dict(state):
                    ok.append('state[dict]')

        try:
            states = getattr(self, 'states', None)
        except Exception:
            states = None

        if states is not None:
            try:
                if isinstance(states, dict):
                    e = states.get(username) if username else None
                    if e is not None and e is not state:
                        if _set_attr(e) or _set_dict(e):
                            ok.append(f'states[{username}]')
                else:
                    for e in states:
                        if e is state:
                            continue
                        try:
                            nm = getattr(getattr(e, 'account', None), 'username', None)
                        except Exception:
                            continue
                        if not nm:
                            continue
                        if nm == username:
                            try:
                                hit = _set_attr(e)
                                hit = _set_dict(e.__dict__) or hit
                            except Exception:
                                continue
                            if not hit:
                                continue
                            ok.append(f'states条目[{username}]')
            except Exception:
                pass

        if ok:
            ctx.log(STEP_NAME, f'验证码压制:该账号重试时间已推至 {_far_str}(6小时外),写入位置={",".join(ok)}', 'WARN')
        else:
            ctx.log(STEP_NAME, '验证码压制失败:未找到任何可写入的重试字段位置!!', 'ERROR')

    def _取state(args, kwargs):
        st = kwargs.get('state')
        if st is not None and hasattr(st, 'account'):
            return st
        for a in args:
            if hasattr(a, 'account'):
                return a
        return None

    def _make(original):
        def wrapped(self, *args, **kwargs):
            username = ''
            state = _取state(args, kwargs)
            if state is not None:
                try:
                    username = state.account.username or ''
                except Exception:
                    username = ''
                try:
                    state.has_saved_action_level = False
                except Exception:
                    pass
            ctx.log(STEP_NAME, f'>>> 开始(预算超时 {TIMEOUT}s)模式={"B-锁标自定义跳过" if 全设施锁标自定义跳过 else "A-EXE原流程"}')
            started = time.time()
            if 全设施锁标自定义跳过:
                try:
                    skip = _自定义跳过流程(self, ctx, started, username=username)
                    if skip is not None:
                        ctx.log(STEP_NAME, f'<<< 完成({time.time() - started:.1f}s)')
                        return skip
                    ctx.log(STEP_NAME, '放行 → 继续执行 EXE 原设施流程')
                except Exception as e:
                    ctx.log(STEP_NAME, f'锁标自检异常,放行 EXE: {type(e).__name__}: {e}', 'WARN')
            try:
                result = original(self, *args, **kwargs)
                if getattr(ctx, '_captcha_hit', None):
                    _压制重试(self, state, username)
                    ctx.log(STEP_NAME, '该账号出现了验证码弹窗(阶段返回兜底),判定失败,切下一个账号', 'ERROR')
                    raise CaptchaError('该账号出现了验证码弹窗')
                ctx.log(STEP_NAME, f'<<< 完成({time.time() - started:.1f}s)')
                return result
            except CaptchaError:
                _压制重试(self, state, username)
                raise
            except Exception as e:
                if getattr(ctx, '_captcha_hit', None):
                    _压制重试(self, state, username)
                    ctx.log(STEP_NAME, '该账号出现了验证码弹窗,判定失败,切下一个账号', 'ERROR')
                    raise CaptchaError('该账号出现了验证码弹窗') from e
                msg = f'{type(e).__name__}: {e}'
                elapsed = time.time() - started
                if 'TIMEOUT' in msg.upper() or 'CARD_STATE' in msg.upper() or '未解锁' in msg:
                    ctx.log(STEP_NAME, f'!!! 设施流程异常,很可能是设施未解锁/卡片卡住({elapsed:.1f}s): {msg}', 'ERROR')
                    ctx.log(STEP_NAME, '若是全锁低等级账号,可把本文件「全设施锁标自定义跳过」改为 True', 'WARN')
                    raise
                ctx.log(STEP_NAME, f'!!! 异常({elapsed:.1f}s): {msg}', 'ERROR')
                raise
        return wrapped

    ctx.patch_with(cls, '_facility_batch', _make)
    if hasattr(cls, '_facility'):
        ctx.patch_with(cls, '_facility', _make)
        ctx.log(STEP_NAME, '_facility(进入特勤处)验证码兜底+压制已挂载 ✓')

    def _是卡片状态超时(e):
        text = f'{type(e).__name__}: {e} {getattr(e, "code", "")}'.upper()
        return 'CARD_STATE' in text or '未识别完整' in str(e)

    def _移开鼠标(self):
        try:
            hwnd = ctx.resolve_hwnd(self, STEP_NAME)
            if not hwnd:
                return None
            import ctypes
            import win32gui
            ox, oy = win32gui.ClientToScreen(hwnd, (0, 0))
            size = getattr(ctx, '_client_size', None)
            rw, rh = 参考分辨率
            sx = size[0] / rw if size else 1.0
            sy = size[1] / rh if size else 1.0
            hx, hy = 悬停清除点
            ctypes.windll.user32.SetCursorPos(int(ox + hx * sx), int(oy + hy * sy))
            return None
        except Exception:
            return None

    def _取证(self):
        for attr in ('_facility_overview_last_raw_states', 'facility_overview_last_raw_states', '_facility_overview_last_states', 'facility_overview_last_states', '_facility_overview_card_errors', 'facility_overview_card_errors'):
            try:
                v = getattr(self, attr, None)
                if v:
                    ctx.log(STEP_NAME, f'取证[{attr}]: {v}', 'WARN')
            except Exception:
                continue
        try:
            hwnd = ctx.resolve_hwnd(self, STEP_NAME)
            if not hwnd:
                return None
            frame = ctx.grab_frame(hwnd, STEP_NAME)
            if frame is None:
                return None
            items = ctx.ocr_scan(frame, STEP_NAME) or []
            size = getattr(ctx, '_client_size', None)
            rw, rh = 参考分辨率
            sx = size[0] / rw if size else 1.0
            sy = size[1] / rh if size else 1.0
            x1, y1, x2, y2 = 面板取证区
            parts = [f'{t}@({int(cx / sx)},{int(cy / sy)})' for t, (cx, cy) in items if x1 * sx <= cx <= x2 * sx and y1 * sy <= cy <= y2 * sy]
            ctx.log(STEP_NAME, '总览面板OCR取证: ' + (" | ".join(parts) if parts else "(无文本)"), 'WARN')
            return None
        except Exception as _oe:
            ctx.log(STEP_NAME, f'总览OCR取证异常(忽略): {_oe}', 'WARN')
            return None

    if 总览卡片重试 and hasattr(cls, '_batch_wait_for_overview'):
        def _make_overview_retry(original):
            def wrapped_overview(self, *args, **kwargs):
                try:
                    return original(self, *args, **kwargs)
                except Exception as e:
                    if not _是卡片状态超时(e):
                        raise
                    last = e
                    for rd in range(1, 总览重试轮数 + 1):
                        if ctx.stopped():
                            raise last
                        ctx.log(STEP_NAME, f'总览卡片状态识别超时,追加第{rd}/{总览重试轮数}轮重试(移开鼠标+等{总览重试间隔}s稳定)...', 'WARN')
                        _移开鼠标(self)
                        time.sleep(总览重试间隔)
                        try:
                            result = original(self, *args, **kwargs)
                            ctx.log(STEP_NAME, f'第{rd}轮重试成功,四设施状态已齐全 ✓')
                            return result
                        except Exception as e2:
                            if not _是卡片状态超时(e2):
                                raise
                            last = e2
                    ctx.log(STEP_NAME, f'追加{总览重试轮数}轮后仍未识别完整,按原逻辑判失败', 'ERROR')
                    _取证(self)
                    raise last
            return wrapped_overview
        ctx.patch_with(cls, '_batch_wait_for_overview', _make_overview_retry)
        ctx.log(STEP_NAME, f'总览卡片识别重试守卫已安装(最多追加{总览重试轮数}轮,仅原逻辑判负前介入)')

    if hasattr(cls, '_batch_select_custom_item'):
        _设施词 = ('工作台', '制药台', '防具台', '技术中心', 'workbench', 'pharmacy', 'armor', 'armory', 'tech')

        def _取物品名(args, kwargs):
            cands = ([kwargs['item']] if 'item' in kwargs else []) + list(args)
            for it in cands:
                if it is None or isinstance(it, str):
                    continue
                for attr in ('name', 'item_name', 'display_name', 'title'):
                    v = getattr(it, attr, None)
                    if isinstance(v, str) and v.strip():
                        return v.strip()
            for it in cands:
                if isinstance(it, str) and it.strip() and it.strip() not in _设施词:
                    return it.strip()
            return ''

        def _归一(s):
            import unicodedata
            s = unicodedata.normalize('NFKC', str(s or ''))
            s = s.replace('×', 'x')
            return ''.join(s.split()).lower()

        def _认置顶行(self):
            hwnd = ctx.resolve_hwnd(self, STEP_NAME)
            if not hwnd:
                return ''
            frame = ctx.grab_frame(hwnd, STEP_NAME)
            if frame is None:
                return ''
            items = ctx.ocr_scan(frame, STEP_NAME) or []
            rw, rh = 列表参考分辨率
            size = getattr(ctx, '_client_size', None)
            sx = size[0] / rw if size else 1.0
            sy = size[1] / rh if size else 1.0
            x_max, y_lo, y_hi = 置顶行文本区
            x_max, y_lo, y_hi = x_max * sx, y_lo * sy, y_hi * sy
            parts = []
            for t, (cx, cy) in items:
                if cx <= x_max and y_lo <= cy <= y_hi and len(str(t).strip()) >= 2:
                    parts.append(str(t).strip())
            return ' '.join(parts)

        def _取物品对象(args, kwargs):
            cands = ([kwargs['item']] if 'item' in kwargs else []) + list(args)
            for it in cands:
                if it is None or isinstance(it, str):
                    continue
                if not getattr(it, 'id', None):
                    continue
                return it
            return None

        def _换物品(args, kwargs, alt):
            a = list(args)
            for i, v in enumerate(a):
                if v is None or isinstance(v, str):
                    continue
                if not getattr(v, 'id', None):
                    continue
                a[i] = alt
                return (tuple(a), kwargs)
            if 'item' in kwargs:
                k = dict(kwargs)
                k['item'] = alt
                return (tuple(a), k)
            a.append(alt)
            return (tuple(a), kwargs)

        def _make_select(original):
            def _原查找(self, args, kwargs):
                result = original(self, *args, **kwargs)
                if result is None or 'FACILITY_CUSTOM_NOT_FOUND' in str(result):
                    return result
                it = _取物品对象(args, kwargs)
                if it is None:
                    return result
                key = (str(getattr(ctx, 'account_id', '') or ''), str(getattr(it, 'id', '')))
                seq = _候选表.get(key) or []
                if len(seq) < 2:
                    return result
                ctx.log(STEP_NAME, f'自选物品「{getattr(it, "name", it.id)}」未找到({result}),按候选表改试 {seq[1:]}', 'WARN')
                for cid in seq[1:]:
                    alt = _查本地物品(cid)
                    if alt is None:
                        ctx.log(STEP_NAME, f'候选 {cid} 不在本地制造目录,跳过', 'WARN')
                        continue
                    ctx.log(STEP_NAME, f'改试候选 {cid}「{getattr(alt, "name", cid)}」', 'WARN')
                    try:
                        a2, k2 = _换物品(args, kwargs, alt)
                        r2 = original(self, *a2, **k2)
                        if r2 is None:
                            ctx.log(STEP_NAME, f'候选 {cid}「{getattr(alt, "name", cid)}」命中 ✓', 'INFO')
                            return None
                        result = r2
                        if 'FACILITY_CUSTOM_NOT_FOUND' not in str(r2):
                            return r2
                    except Exception as e:
                        ctx.log(STEP_NAME, f'候选 {cid} 查找异常: {type(e).__name__}: {e}', 'WARN')
                ctx.log(STEP_NAME, f'候选表已全部试完仍未找到,按原错误上交 {result}', 'ERROR')
                return result

            def wrapped_select(self, *args, **kwargs):
                if not 置顶前置识别:
                    return _原查找(self, args, kwargs)
                nm = _取物品名(args, kwargs)
                if not nm:
                    ctx.log(STEP_NAME, '置顶前置识别:入参无物品名,走原查找', 'WARN')
                    return _原查找(self, args, kwargs)
                try:
                    row_text = _认置顶行(self)
                except Exception as e:
                    ctx.log(STEP_NAME, f'置顶前置识别异常,走原查找: {type(e).__name__}: {e}', 'WARN')
                    return _原查找(self, args, kwargs)
                nn, nr = _归一(nm), _归一(row_text)
                if nn and nr and (nn in nr or nr in nn):
                    hwnd = ctx.resolve_hwnd(self, STEP_NAME)
                    rw, rh = 列表参考分辨率
                    px, py = 置顶行点击坐标
                    ctx.click_ref(hwnd, px, py, rw, rh, STEP_NAME)
                    time.sleep(置顶识别动作间隔)
                    ctx.log(STEP_NAME, f'云端物品=当前置顶项「{row_text}」,直接选用置顶 ✓')
                    return None
                ctx.log(STEP_NAME, f'置顶项「{row_text or "未认出"}」非云端物品「{nm}」,走原查找')
                return _原查找(self, args, kwargs)
            return wrapped_select
        ctx.patch_with(cls, '_batch_select_custom_item', _make_select)
        ctx.log(STEP_NAME, '自选物品候选兜底已挂载(查不到时按 candidates 顺序换下一个)')
    elif 置顶前置识别:
        ctx.log(STEP_NAME, '引擎无 _batch_select_custom_item 方法,置顶前置识别未挂载', 'WARN')

    try:
        from pc_tqc.vision import ocr_rules as _ocr_rules
        import re as _re

        def _wrap_timer_parse(original):
            def wrapped_timer(*args, **kwargs):
                result = original(*args, **kwargs)
                if result is not None:
                    return result
                new_args = list(args)
                fixed = False
                for i, a in enumerate(new_args):
                    if not isinstance(a, str):
                        continue
                    if not _re.search(r'\d\s*[-–—]\s*\d{1,2}:', a):
                        continue
                    new_args[i] = _re.sub(r'(\d)\s*[-–—]\s*(\d{1,2}:)', r'\1:\2', a)
                    fixed = True
                if not fixed:
                    return result
                try:
                    retry = original(*new_args, **kwargs)
                    if retry is not None:
                        return retry
                    return result
                except Exception:
                    return result
            return wrapped_timer

        _timer_patched = []
        for _mod_name in ('pc_tqc.vision.ocr_rules', 'pc_tqc.vision.ocr'):
            try:
                _mod = __import__(_mod_name, fromlist=['*'])
                for _fn in ('parse_facility_detail_remaining_time', 'parse_facility_card_remaining_time', 'parse_remaining_time'):
                    if not hasattr(_mod, _fn):
                        continue
                    ctx.patch_with(_mod, _fn, _wrap_timer_parse)
                    _timer_patched.append(f'{_mod_name.rsplit(".", 1)[-1]}.{_fn}')
            except Exception:
                continue
        ctx.log(STEP_NAME, f'已安装倒计时短横线OCR兼容补丁: {", ".join(_timer_patched)}')
    except Exception as e:
        ctx.log(STEP_NAME, f'倒计时兼容补丁未安装(忽略): {type(e).__name__}: {e}', 'WARN')

    if hasattr(cls, '_batch_manufacture_facility'):
        def _fill_rescue(self):
            hwnd = ctx.resolve_hwnd(self, STEP_NAME)

            def _read(tag):
                if not hwnd:
                    return None
                try:
                    frame = ctx.grab_frame(hwnd, tag)
                    if frame is None:
                        return None
                    return ctx.ocr_scan(frame, tag)
                except CaptchaError:
                    raise
                except Exception:
                    return None

            def _joined(items):
                return ''.join(t for t, _ in (items or []))

            j = _joined(_read('fill_rescue'))
            if '一键补齐' in j or '补齐清单' in j:
                ctx.log(STEP_NAME, '弹窗救援:补齐清单仍开着,补点支付按钮(510,497@1024x768)', 'WARN')
                ctx.click_ref(hwnd, 510, 497, 1024, 768, STEP_NAME)
                end = time.time() + 8
                closed = False
                while time.time() < end:
                    time.sleep(1.0)
                    j = _joined(_read('fill_rescue_wait'))
                    if '一键补齐' not in j and '补齐清单' not in j:
                        closed = True
                        break
                if not closed:
                    ctx.log(STEP_NAME, '弹窗救援:补点后弹窗仍未关闭,放弃', 'WARN')
                    return False
                ctx.log(STEP_NAME, '弹窗救援:支付完成,弹窗已关闭 ✓', 'INFO')
            else:
                ctx.log(STEP_NAME, '弹窗救援:弹窗已不在(支付或已成功),直接重试', 'INFO')
            if hwnd:
                ctx.key_foreground(hwnd, 27, '弹窗救援ESC回总览')
            time.sleep(1.0)
            return True

        def _make_manu(original):
            def wrapped_manu(self, *args, **kwargs):
                result = original(self, *args, **kwargs)
                if not result:
                    return result
                r = str(result)
                if 'FILL_RETURN' in r and '补齐后未返回' in r and '返回制造详情' in r:
                    return result
                ctx.log(STEP_NAME, f'检测到补齐返回超时({r}),启动弹窗救援...', 'WARN')
                try:
                    if not _fill_rescue(self):
                        ctx.log(STEP_NAME, '弹窗救援失败,等待 3s 后检测是否为验证码弹窗...', 'WARN')
                        time.sleep(3.0)
                        ctx.detect_captcha_window('fill_rescue_captcha_check')
                        return result
                    ctx.log(STEP_NAME, '弹窗救援完成,引擎重试该设施制造一次', 'WARN')
                    return original(self, *args, **kwargs)
                except CaptchaError:
                    raise
                except Exception as e:
                    ctx.log(STEP_NAME, f'弹窗救援异常: {type(e).__name__}: {e}', 'WARN')
                    ctx.log(STEP_NAME, '等待 3s 后检测是否为验证码弹窗...', 'WARN')
                    time.sleep(3.0)
                    ctx.detect_captcha_window('fill_rescue_captcha_check')
                    return result
            return wrapped_manu
        ctx.patch_with(cls, '_batch_manufacture_facility', _make_manu)
        ctx.log(STEP_NAME, '已挂载一键补齐弹窗救援(FILL_RETURN超时→补点支付+重试)', 'INFO')
        return None
    return None
def _自定义跳过流程(self, ctx, started_at, username=''):
    """模式B:自己进图5面板 OCR 判定锁标,全锁则跳过本轮。"""
    hwnd = ctx.resolve_hwnd(self, STEP_NAME)
    if not hwnd:
        raise RuntimeError('第四步: 找不到游戏窗口')
    else:
        deadline = started_at + TIMEOUT
        tx, ty = 特勤处TAB参考坐标
        rw, rh = 参考分辨率
        ctx.click_ref(hwnd, tx, ty, rw, rh, STEP_NAME)
        ctx.log(STEP_NAME, '已点击顶部「特勤处」菜单')
        time.sleep(点击TAB后等待)
        panel = False
        while time.time() < min(time.time() + 面板最长等待, deadline):
            if ctx.stopped():
                raise RuntimeError('第四步: 已停止')
            frame = ctx.grab_frame(hwnd, STEP_NAME)
            if frame is None:
                time.sleep(空轮询间隔)
                continue
            items = ctx.ocr_scan(frame, STEP_NAME)
            ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
            if ctx.find_kw(items, KW_特勤处面板):
                panel = True
                break
            time.sleep(空轮询间隔)
        if not panel:
            ctx.log(STEP_NAME, '未进入特勤处面板,放行给 EXE 原流程尝试', 'WARN')
            return
        else:
            frame = ctx.grab_frame(hwnd, STEP_NAME)
            items = ctx.ocr_scan(frame, STEP_NAME)
            ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
            all_text = ' '.join((txt for txt, _ in items))
            found_fac = [name for name in 设施列表 if name in all_text]
            locked = ctx.find_kw(items, KW_未解锁) is not None
            ctx.log(STEP_NAME, f"面板识别:设施名命中 {found_fac or '无'} · 锁标「未解锁」={('有' if locked else '无')}")
            ctx.key_foreground(hwnd, VK_ESCAPE, STEP_NAME)
            if locked and len(found_fac) <= 2:
                ctx.log(STEP_NAME, '4 设施均未解锁(行动等级不足),本轮跳过,不制造 ✓', 'WARN')
                return _SkipResult('facilities_locked: 行动等级不足,4设施未解锁')
            else:
                if locked:
                    ctx.log(STEP_NAME, '存在未解锁设施且部分设施可见,自定义模式不制造,交回 EXE', 'WARN')
                else:
                    ctx.log(STEP_NAME, '未发现锁标,设施可能已解锁,自定义模式不制造,交回 EXE', 'WARN')


def 登记候选(account_id, settings):
    """runner_worker 拉到云端制造配置后调用,登记「物品ID -> 候选序列」。"""
    try:
        for _fac, s in (settings or {}).items():
            if not isinstance(s, dict):
                continue
            else:
                ids = s.get('candidates')
                if not isinstance(ids, (list, tuple)):
                    continue
                else:
                    seq = [str(x) for x in ids if isinstance(x, str) and str(x).strip()]
                    head = s.get('item_id') or s.get('itemId')
                    if head and (not seq or seq[0] != str(head)):
                            seq = [str(head)] + seq
                    seq = list(dict.fromkeys(seq))
                    if len(seq) < 2:
                        continue
                    else:
                        for i in seq:
                            _候选表[str(account_id), i] = seq
    except Exception:
        return None


def _查本地物品(item_id):
    """从本地制造目录按 ID 取 ManufactureItem;取不到返回 None。"""
    try:
        from pc_tqc.vision.manufacture_list import load_manufacture_catalog
        for items in (load_manufacture_catalog()[1] or {}).values():
            it = items.get(item_id)
            if it is not None:
                return it
    except Exception:
        return None
    return None