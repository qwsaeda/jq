# 第三步.py — 读取行动等级（重建版：PyLingual 干净体 + 语义转储 analysis_biz.txt 拼合）
# 本体 90% 为 PyLingual 原始输出（前半段干净），仅 _flow 主循环 / _patch_engine_haf_coin_m
# 两处 3.12 嵌套异常错位段按语义转储重写，其余逐字保留。

import re
import time

STEP_NAME = '第三步 · 读取行动等级'


def _弹窗日志(username, event):
    """回归弹窗识别/按键/复核事件落盘(运行日志/回归弹窗.log),事后可查:
    特别行动等弹窗认没认到、空格按了几次、每次复核页面响应没。"""
    try:
        from workflow_steps import popup_event_log
        popup_event_log(username, '[第三步] ' + event)
    except Exception:
        return None


TIMEOUT = 240
PROFILE_入口参考坐标 = (1213, 700)
PROFILE_参考分辨率 = (1280, 720)
点击入口后等待秒 = 4
等级面板最长等待 = 12
入口最多尝试次数 = 3
KW_行动等级 = '行动等级'
等级正则 = r'行动等级\s*[:：]?\s*([0-9]+)'
KW_收藏室 = ('收藏室', '收藏', '藏室')
收藏室判定等级 = 60
VK_ESCAPE = 27
Esc后等待秒 = 1
空轮询间隔 = 2
KW_执勤报道 = ('执勤报道',)
KW_领取 = ('领取',)
KW_特别行动 = ('特别行动',)
KW_继续 = ('继续',)
KW_配装页 = ('确认配装', '制式套装使用中', '战前准备')
VK_空格 = 32
弹窗按键复核等待 = 3
弹窗按键上限 = 4
配装页ESC上限 = 2
回大厅最长等待 = 35
领取后等待 = 3
弹窗链最多轮数 = 10


class _OkResult:
    """EXE runner 期望的返回值接口。"""
    ok = True
    error = None
    stage = 'read_action_level'

    def __init__(self, message=''):
        self.message = message
        self.details = {}

    def __getattr__(self, name):
        return {}


def _win32_click(hwnd, x, y, ctx):
    """用 EXE 的 click_client_foreground 点击(activate_window+SetCursorPos+SendInput)。
    这是 EXE 原生的前台点击方式,游戏能正常响应。"""
    try:
        from pc_tqc.platform.input import InputController
        inp = ctx._get_input(STEP_NAME) if hasattr(ctx, '_get_input') else None
        if inp is None:
            import logging
            inp = InputController(logger=logging.getLogger('pc_tqc.step3'))
        inp.click_client_foreground(hwnd, x, y)
        ctx.log(STEP_NAME, f'前台点击: 客户区({x},{y})')
    except Exception as e:
        ctx.log(STEP_NAME, f'点击失败: {type(e).__name__}: {e}', 'ERROR')
        return False
    return True


def _读等级(items):
    for txt, _ in items:
        if KW_行动等级 in txt:
            m = re.search(等级正则, txt)
            if m:
                return int(m.group(1))
            else:
                digits = re.findall(r'\d+', txt)
                if digits:
                    return int(digits[0])
    return None


def _是收藏室界面(items):
    """个人信息页被收藏室占位:有「收藏室」且没有「行动等级」。"""
    has_room = False
    has_level = False
    for txt, _ in items:
        t = str(txt)
        if KW_行动等级 in t:
            has_level = True
        if any((k in t for k in KW_收藏室)):
            has_room = True
    return has_room and (not has_level)


def _左下放大裁剪(frame):
    import cv2
    h, w = frame.shape[:2]
    crop = frame[int(h * 0.55):int(h * 0.95), int(w * 0.02):int(w * 0.45)]
    return cv2.resize(crop, None, fx=2.0, fy=2.0, interpolation=cv2.INTER_CUBIC)


def _写回state(args, kwargs, level, ctx):
    state = kwargs.get('state', None)
    if state is None:
        for a in args:
            if hasattr(a, 'account') or hasattr(a, 'level') or hasattr(a, 'haf_coins'):
                state = a
                break
    if state is None:
        return
    for attr, val in [('level', level), ('has_saved_action_level', False), ('pending_level', None)]:
        if val is None and attr == 'level':
            continue
        if hasattr(state, attr):
            try:
                setattr(state, attr, val)
            except Exception:
                pass


def _flow(self, args, kwargs, ctx, started_at):
    hwnd = ctx.resolve_hwnd(self, STEP_NAME)
    if not hwnd:
        raise RuntimeError('第三步: 找不到游戏窗口')
    deadline = started_at + TIMEOUT
    ex, ey = PROFILE_入口参考坐标
    ref_w, ref_h = PROFILE_参考分辨率
    username = ''
    _st = kwargs.get('state')
    if _st is None:
        for a in args:
            if hasattr(a, 'account'):
                _st = a
                break
    if _st is not None:
        try:
            username = _st.account.username or ''
        except Exception:
            username = ''

    def _ocr():
        frame = ctx.grab_frame(hwnd, STEP_NAME)
        if frame is None:
            return None
        return ctx.ocr_scan(frame, STEP_NAME)

    def _still_blocked(items):
        """画面是否被回归弹窗/战备页挡着(判定口径与第二步一致)。"""
        if ctx.find_kw(items, KW_配装页):
            return True
        if ctx.find_kw(items, KW_执勤报道):
            return True
        if ctx.find_kw(items, KW_特别行动):
            return True
        if ctx.find_kw(items, KW_继续):
            return True
        return False

    def _按键复核(vk, desc, max_times, blocked):
        """按键→等→复核页面响应:目标词消失才算按有效;没动重点;按满上限WARN。
        用户实证:偶发"显示已按但页面没动"(焦点/时机问题),必须复核+重点。"""
        for i in range(1, max_times + 1):
            if ctx.stopped():
                raise RuntimeError('第三步: 已停止')
            ctx.key_foreground(hwnd, vk, STEP_NAME)
            ctx.log(STEP_NAME, f'{desc}: 已按键(第 {i}/{max_times} 次),等{弹窗按键复核等待}s复核页面响应')
            _弹窗日志(username, f'{desc} 已按键(第{i}/{max_times}次,vk=0x{vk:02X})')
            time.sleep(弹窗按键复核等待)
            items = _ocr()
            if items is None:
                _弹窗日志(username, f'{desc} 第{i}次复核时OCR无返回(空帧/黑帧),重点')
                continue
            ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
            if not blocked(items):
                ctx.log(STEP_NAME, f'{desc}: 页面已响应 ✓')
                _弹窗日志(username, f'{desc} 第{i}次复核 页面已响应✓')
                return True
            _弹窗日志(username, f'{desc} 第{i}次复核 页面未响应(目标词仍在),重点')
        ctx.log(STEP_NAME, f'{desc}: 按键已发{max_times}次页面仍未响应(疑似游戏焦点异常,可人工按一次验证)', 'WARN')
        _弹窗日志(username, f'{desc} 已按满{max_times}次页面仍未响应✗(疑似焦点异常)')
        return False

    def _等回大厅():
        """战备页按ESC后等回大厅(用户实测20-30秒);配装页词消失即认为已回。"""
        end = time.time() + 回大厅最长等待
        while time.time() < end and time.time() < deadline:
            if ctx.stopped():
                raise RuntimeError('第三步: 已停止')
            items = _ocr()
            if items is None:
                time.sleep(空轮询间隔)
                break
            ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
            if not ctx.find_kw(items, KW_配装页):
                ctx.log(STEP_NAME, '战备页已退出,回到大厅 ✓')
                return True
            time.sleep(空轮询间隔)
        ctx.log(STEP_NAME, '等回大厅超时,画面仍有配装页词', 'WARN')
        return False

    def _消弹窗链():
        """回归弹窗链自消+战备页退出。检测驱动:没弹窗只花一次OCR时间,零影响。"""
        for _round in range(1, 弹窗链最多轮数 + 1):
            if ctx.stopped():
                raise RuntimeError('第三步: 已停止')
            if time.time() >= deadline:
                return
            items = _ocr()
            if items is None:
                return
            ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
            hit_loadout = ctx.find_kw(items, KW_配装页)
            if hit_loadout:
                ctx.log(STEP_NAME, f'识别到战备页「{hit_loadout[0]}」→ 按ESC回大厅(绝不点「确认配装」/「开始行动」)')
                _弹窗日志(username, f'战备页 识别到「{hit_loadout[0]}」,准备按ESC')
                _按键复核(VK_ESCAPE, '战备页ESC', 配装页ESC上限, lambda it: ctx.find_kw(it, KW_配装页) is not None)
                _等回大厅()
                continue
            hit_duty = ctx.find_kw(items, KW_执勤报道)
            if hit_duty:
                _弹窗日志(username, f'执勤报道 识别到弹窗(原文「{hit_duty[0]}」)')
                hit_claim = ctx.find_kw(items, KW_领取)
                if hit_claim:
                    px, py = hit_claim[1]
                    if ctx.click(hwnd, px, py, STEP_NAME):
                        ctx.log(STEP_NAME, f'回归弹窗「执勤报道」已点「领取」({px},{py})')
                        _弹窗日志(username, f'执勤报道 已点「领取」@({px},{py})')
                        time.sleep(领取后等待)
                        continue
                ctx.log(STEP_NAME, '回归弹窗「执勤报道」未认到领取按钮,改按空格', 'WARN')
                _弹窗日志(username, '执勤报道 未认到领取按钮,改按空格')
                _按键复核(VK_空格, '执勤报道空格', 弹窗按键上限, lambda it: ctx.find_kw(it, KW_执勤报道) is not None)
                continue
            hit_s = ctx.find_kw(items, KW_特别行动)
            if hit_s:
                _弹窗日志(username, f'特别行动 识别到弹窗(原文「{hit_s[0]}」),坐标@({hit_s[1][0]},{hit_s[1][1]}),准备按ESC')
                _按键复核(VK_ESCAPE, '特别行动ESC', 弹窗按键上限, lambda it: ctx.find_kw(it, KW_特别行动) is not None)
                continue
            hit_cont = ctx.find_kw(items, KW_继续)
            if hit_cont:
                _弹窗日志(username, f'活动页 识别到「{hit_cont[0]}」,准备按空格')
                _按键复核(VK_空格, f'活动页「{hit_cont[0]}」空格', 弹窗按键上限, lambda it: ctx.find_kw(it, KW_继续) is not None)
            else:
                break

    level = None
    frame = ctx.grab_frame(hwnd, STEP_NAME)
    if frame is not None:
        items = ctx.ocr_scan(frame, STEP_NAME)
        ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
        h, w = frame.shape[:2]
    else:
        h, w = ref_h, ref_w
    click_x = int(ex * w / ref_w)
    click_y = int(ey * h / ref_h)
    ctx.log(STEP_NAME, f'入口坐标缩放: 参考({ex},{ey}@{ref_w}x{ref_h}) → 实际({click_x},{click_y}@{w}x{h})')
    for attempt in range(1, 入口最多尝试次数 + 1):
        if ctx.stopped():
            raise RuntimeError('第三步: 已停止')
        _消弹窗链()
        ok = _win32_click(hwnd, click_x, click_y, ctx)
        if not ok:
            raise RuntimeError('第三步: 点击个人信息入口失败')
        ctx.log(STEP_NAME, f'已点击个人信息入口 ({click_x},{click_y})(第 {attempt} 次)')
        time.sleep(点击入口后等待秒)
        panel_deadline = time.time() + 等级面板最长等待
        while time.time() < panel_deadline and time.time() < deadline:
            if ctx.stopped():
                raise RuntimeError('第三步: 已停止')
            frame = ctx.grab_frame(hwnd, STEP_NAME)
            if frame is None:
                time.sleep(空轮询间隔)
                break
            items = ctx.ocr_scan(frame, STEP_NAME)
            ctx.raise_if_banned(items, hwnd, username, STEP_NAME)
            if _still_blocked(items):
                _hits = []
                _h1 = ctx.find_kw(items, KW_配装页)
                _h2 = ctx.find_kw(items, KW_执勤报道)
                _h3 = ctx.find_kw(items, KW_特别行动)
                _h5 = ctx.find_kw(items, KW_继续)
                if _h1:
                    _hits.append(f'战备页「{_h1[0]}」')
                if _h2:
                    _hits.append(f'执勤报道「{_h2[0]}」')
                if _h3:
                    _hits.append(f'特别行动「{_h3[0]}」')
                if _h5:
                    _hits.append(f'继续「{_h5[0]}」')
                ctx.log(STEP_NAME, '读等级时识别到回归弹窗/战备页,转弹窗处理后重点入口', 'WARN')
                _弹窗日志(username, '读等级面板期间弹窗中断,命中: ' + ('、'.join(_hits) or '判定词'))
                level = None
                break
            level = _读等级(items)
            if level is None and _是收藏室界面(items):
                level = 收藏室判定等级
            if level is not None:
                break
            zoom = _左下放大裁剪(frame)
            zoom_items = ctx.ocr_scan(zoom, STEP_NAME)
            ctx.raise_if_banned(zoom_items, hwnd, username, STEP_NAME)
            level = _读等级(zoom_items)
            if level is None and _是收藏室界面(zoom_items):
                level = 收藏室判定等级
            if level is not None:
                break
            time.sleep(空轮询间隔)
            ctx.key_foreground(hwnd, VK_ESCAPE, STEP_NAME)
            time.sleep(Esc后等待秒)
        if level is not None:
            ctx.log(STEP_NAME, f'读取完成 行动等级={level}')
            _写回state(args, kwargs, level, ctx)
            return _OkResult(f'level={level}')
        ctx.log(STEP_NAME, f'第 {attempt} 次未读到行动等级,{"重开面板再试" if attempt < 入口最多尝试次数 else "放弃"}', 'WARN')
    raise RuntimeError('PROFILE_LEVEL_UNREADABLE: 打开个人信息后未识别到行动等级')


def _patch_engine_haf_coin_m(ctx):
    """V1.15: 交易行出售完读取哈夫币时,引擎 parse_haf_coin_text 只认 K;
    这里只补 M(亿级)单位,坐标/裁切/调用链全部沿用引擎原版。"""
    try:
        from pc_tqc.vision import market as _m
        if getattr(_m, '_haf_m_patched', False):
            return
        _orig = _m.parse_haf_coin_text
        _re_m = re.compile(r'(?<![0-9A-Za-z])([0-9][0-9,]*(?:\.[0-9]+)?)\s*[Mm](?![A-Za-z])')

        def _parse(text):
            try:
                mm = _re_m.search(str(text) if text else '')
            except Exception:
                mm = None
            if mm:
                try:
                    val = float(mm.group(1).replace(',', '')) * 1000000
                    return f'{int(val):,}'
                except ValueError:
                    return None
            return _orig(text)

        _m.parse_haf_coin_text = _parse
        _m._haf_m_patched = True
        ctx.log(STEP_NAME, '已为交易行哈夫币识别补充 M(亿级)单位', 'INFO')
    except Exception as e:
        ctx.log(STEP_NAME, f'补 M 单位失败(不影响主流程): {type(e).__name__}: {e}', 'WARN')


def install(cls, ctx):
    _patch_engine_haf_coin_m(ctx)
    cls.has_saved_action_level = False
    try:
        from pc_tqc import models as _models
        _models.AccountRunState.has_saved_action_level = False
    except Exception:
        pass

    def _make(original):
        def wrapped(self, *args, **kwargs):
            ctx.log(STEP_NAME, f'>>> 开始(预算超时 {TIMEOUT}s)')
            started = time.time()
            try:
                result = _flow(self, args, kwargs, ctx, started)
                ctx.log(STEP_NAME, f'<<< 完成({time.time() - started:.1f}s)')
                return result
            except Exception as e:
                ctx.log(STEP_NAME, f'!!! 异常({time.time() - started:.1f}s): {type(e).__name__}: {e}', 'ERROR')
                raise
        return wrapped

    try:
        ctx.patch_with(cls, '_read_action_level', _make)
    except Exception:
        pass
