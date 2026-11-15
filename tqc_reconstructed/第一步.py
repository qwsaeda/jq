# 第一步.py — 登录平台并进入游戏（重建版：忠实 100% 语义转储 analysis_biz.txt）
# 职责: Steam 活跃用户校验 + 杀进程守卫 + 游戏窗口/桌面识别
# 边界: 只查进程存在性/注册表, 绝不结束/注入/读游戏内存

import os, re, sys, time, threading

STEP_NAME = '第一步'


_CAPTURE_GUARD = {'ts': 0.0}


def _capture_guard_on(seconds) -> None:
    """记录保护截止时间(从此刻起 seconds 秒内禁止杀进程)。"""
    _CAPTURE_GUARD['ts'] = max(_CAPTURE_GUARD['ts'], time.time() + float(seconds))


def _capture_guard_off() -> None:
    """登录成功/窗口消失时解除保护,允许后续清理。"""
    _CAPTURE_GUARD['ts'] = 0.0


def _capture_guard_active() -> bool:
    """当前是否处于捕获保护期(禁止杀进程)。"""
    return time.time() < _CAPTURE_GUARD['ts']


def _guard_skip_kill(ctx, action):
    """保护期内的杀进程动作统一走这里:记日志并拒绝执行。"""
    if not _capture_guard_active():
        return False
    ctx.log(STEP_NAME, f'捕获保护中(Steam登录橙框):禁止杀进程,已跳过 {action}', 'WARN')
    return True
def _wrap_kill(ctx, orig):
    """把杀进程函数包成\"保护期内一律拒绝\"。"""
    def guarded(*args, **kwargs):
        if _guard_skip_kill(ctx, f"terminate_processes {str(args[0] if args else '')[:60]}"):
            return 0
        else:
            return orig(*args, **kwargs)
    guarded._tqc_capture_guard = True
    return guarded
_skip_kill = False


def _install_kill_guard(ctx, cls):
    """把平台层 terminate_processes 套上捕获保护:保护期内直接拒绝执行。

    两处都要换,否则拦不住:
      ① 模块属性 pc_tqc.platform.windows.terminate_processes(以模块名调用);
      ② 已被 `from ... import` 绑到 runner 模块全局的那个名字(以裸名调用)。
    runner 模块直接从 cls.__module__ 取(一定已加载),避免 import 失败漏挂。
    """

    def _guard(owner_name, target):
        hit = 0
        for key, val in list(vars(target).items()):
            if key != 'terminate_processes' or not callable(val):
                continue
            if getattr(val, '_tqc_capture_guard', False):
                continue
            try:
                setattr(target, key, _wrap_kill(ctx, val))
                hit += 1
            except Exception as e:
                ctx.log(STEP_NAME, f'挂载捕获保护失败 {owner_name}.{key}: {e}', 'WARN')
                continue
        ctx.log(STEP_NAME, f'捕获保护挂载 {owner_name}: {hit} 处', 'INFO' if hit else 'WARN')
        return None

    targets = []
    seen = set()
    if cls is not None:
        mod_name = getattr(cls, '__module__', '')
        mod = sys.modules.get(mod_name)
        if mod is not None:
            targets.append((mod_name, mod))
            seen.add(mod_name)
    for mod_name in ('pc_tqc.platform.windows',):
        if mod_name in seen:
            continue
        mod = sys.modules.get(mod_name)
        if mod is None:
            try:
                import importlib
                mod = importlib.import_module(mod_name)
            except Exception as e:
                ctx.log(STEP_NAME, f'导入 {mod_name} 失败(该处不挂载): {type(e).__name__}: {e}', 'WARN')
                continue
        targets.append((mod_name, mod))
        seen.add(mod_name)
    for owner_name, mod in targets:
        _guard(owner_name, mod)

    try:
        import psutil
        for meth in ('kill', 'terminate'):
            orig = getattr(psutil.Process, meth, None)
            if orig is None or getattr(orig, '_tqc_capture_guard', False):
                continue

            def guarded(self, *a, _orig=orig, _meth=meth, **k):
                if _guard_skip_kill(ctx, f'psutil.Process.{_meth} pid={getattr(self, "pid", "?")}'):
                    return None
                return _orig(self, *a, **k)

            guarded._tqc_capture_guard = True
            setattr(psutil.Process, meth, guarded)
        ctx.log(STEP_NAME, '捕获保护挂载 psutil.Process.kill/terminate: 2 处')
        return None
    except Exception as e:
        ctx.log(STEP_NAME, f'挂载 psutil 捕获保护失败: {e}', 'WARN')
        return None
def _read_steam_active_user() -> int:
    """读当前 Steam 运行态登录账号的 SteamID3(未登录或读不到返回 0)。"""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam\ActiveProcess') as key:
            return int(winreg.QueryValueEx(key, 'ActiveUser')[0])
    except Exception:
        return 0


def _reset_steam_active_user() -> None:
    """启动 Steam 前清空运行态 ActiveUser,使其只在本次登录成功后才被写入。"""
    try:
        import winreg
        with winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, 'Software\Valve\Steam\ActiveProcess', 0, winreg.KEY_SET_VALUE) as key:
            winreg.SetValueEx(key, 'ActiveUser', 0, winreg.REG_DWORD, 0)
    except Exception:
        pass


def _steam_root():
    """定位 Steam 根目录:注册表 SteamPath(优先)/ SteamExe 所在目录。"""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam') as key:
            try:
                p = str(winreg.QueryValueEx(key, 'SteamPath')[0]).strip()
                if p:
                    return os.path.normpath(os.path.expandvars(p))
            except Exception:
                pass
            exe = str(winreg.QueryValueEx(key, 'SteamExe')[0]).strip()
            if exe:
                return os.path.dirname(os.path.expandvars(exe))
    except Exception:
        return None
    return None


def _steam_id3_of_name(root, name):
    """从 config.vdf 的 Accounts 块取账号名对应 SteamID3;找不到返回 None。"""
    if not root or not name:
        return None
    path = os.path.join(root, 'config', 'config.vdf')
    try:
        with open(path, 'r', encoding='utf-8-sig', errors='replace') as f:
            text = f.read()
    except Exception:
        return None
    m = re.search(r'"Accounts"\s*\{', text)
    if not m:
        return None
    body = text[m.end():]
    for entry in re.finditer(r'"((?:[^"\\]|\\.)+)"\s*\{\s*"SteamID"\s*"(\d{15,20})"', body):
        if entry.group(1) == name:
            try:
                return int(entry.group(2)) - _STEAM_ID64_BASE
            except Exception:
                return None
    return None


def _steam_registry_login_ok(self, username: str) -> bool:
    """运行态注册表判定:ActiveUser == 该账号 SteamID3 即视为已登录。"""
    active = _read_steam_active_user()
    if active <= 0:
        return False
    root = getattr(self, 'root', None)
    if not (root and os.path.isdir(str(root))):
        root = _steam_root()
    sid3 = _steam_id3_of_name(root, username)
    return sid3 is not None and active == sid3


def _steam_process_running() -> bool:
    """steam.exe 是否还在(只查进程存在性,绝不结束/注入/读内存)。"""
    try:
        from pc_tqc.platform.windows import process_exists
        return bool(process_exists('steam.exe') or process_exists('steam'))
    except Exception:
        return True


def _game_process_running():
    """三角洲游戏进程是否在跑(只查存在性)。"""
    try:
        from pc_tqc.platform import windows
        return any(windows.process_exists(n) for n in STEAM_守卫_游戏进程名)
    except Exception:
        return False


def _game_or_launcher_running() -> bool:
    """三角洲游戏或启动器进程是否在跑(画面判据兜底,只查存在性)。"""
    try:
        from pc_tqc.platform import windows
        for n in STEAM_守卫_游戏进程名 + STEAM_守卫_启动器进程名:
            if windows.process_exists(n):
                return True
        return False
    except Exception:
        return False


def _virtual_screen_rect():
    """虚拟桌面(多显示器)的屏幕坐标范围 (left, top, width, height)。"""
    try:
        import win32api, win32con
        left = win32api.GetSystemMetrics(win32con.SM_XVIRTUALSCREEN)
        top = win32api.GetSystemMetrics(win32con.SM_YVIRTUALSCREEN)
        width = win32api.GetSystemMetrics(win32con.SM_CXVIRTUALSCREEN)
        height = win32api.GetSystemMetrics(win32con.SM_CYVIRTUALSCREEN)
        return (left, top, width, height)
    except Exception:
        return (0, 0, 1920, 1080)


def _own_window_rects():
    """执行端自身窗口的屏幕矩形(用于 OCR 前涂黑)。"""
    now = time.time()
    try:
        import psutil
        pids = set()
        pids.add(os.getpid())
        try:
            p = psutil.Process()
            parent = p.parent()
            if parent:
                pids.add(parent.pid)
        except Exception:
            pass
        if now - _OWN_WIN['ts'] < 10.0 and _OWN_WIN['rects'] is not None:
            return _OWN_WIN['rects']
        _OWN_WIN['pids'] = pids
    except Exception:
        pids = _OWN_WIN['pids']

    rects = []
    try:
        import win32process
        from win32gui import EnumWindows, IsWindowVisible, GetWindowText, GetWindowRect

        def _cb(hwnd):
            if not IsWindowVisible(hwnd):
                return None
            try:
                title = GetWindowText(hwnd) or ''
                _, pid = win32process.GetWindowThreadProcessId(hwnd)
                if pid in pids or '特勤处' in title or '执行端' in title:
                    l, t, r, b = GetWindowRect(hwnd)
                    rects.append((l, t, r, b))
                    return None
                return None
            except Exception:
                return None

        EnumWindows(_cb, None)
    except Exception:
        pass
    _OWN_WIN['rects'] = rects
    _OWN_WIN['ts'] = now
    return rects
def _mask_own_windows(frame):
    """把执行端自身窗口区域涂黑,避免 OCR 把自己日志里的字当成游戏画面。"""
    rects = _own_window_rects()
    if not rects:
        return frame
    try:
        import win32gui
        dl, dt, _, _ = _virtual_screen_rect()
        h, w = frame.shape[:2]
        for l, t, r, b in rects:
            l -= dl
            r -= dl
            t -= dt
            b -= dt
            l = max(0, int(l))
            t = max(0, int(t))
            r = min(w, int(r))
            b = min(h, int(b))
            if l < r and t < b:
                frame[t:b, l:r] = 0
        return frame
    except Exception:
        return frame


def _grab_desktop_bgr():
    """全屏截图(BGR)。用 GDI 桌面 BitBlt。"""
    try:
        import numpy as np
        import win32con, win32gui, win32ui
        hwnd = win32gui.GetDesktopWindow()
        left, top, right, bottom = win32gui.GetWindowRect(hwnd)
        w = int(right - left)
        h = int(bottom - top)
        hwnd_dc = win32gui.GetWindowDC(hwnd)
        mfc_dc = win32ui.CreateDCFromHandle(hwnd_dc)
        save_dc = mfc_dc.CreateCompatibleDC()
        bmp = win32ui.CreateBitmap()
        bmp.CreateCompatibleBitmap(mfc_dc, w, h)
        save_dc.SelectObject(bmp)
        save_dc.BitBlt((0, 0), (w, h), mfc_dc, (0, 0), win32con.SRCCOPY)
        bmpinfo = bmp.GetInfo()
        bmpstr = bmp.GetBitmapBits(True)
        arr = np.frombuffer(bmpstr, dtype=np.uint8).reshape(
            (bmpinfo['bmHeight'], bmpinfo['bmWidth'], 4))[:, :, :3]
        arr = np.ascontiguousarray(arr)
        win32gui.DeleteObject(bmp.GetHandle())
        save_dc.DeleteDC()
        mfc_dc.DeleteDC()
        win32gui.ReleaseDC(hwnd, hwnd_dc)
        return arr
    except Exception:
        return None


def _shrink_for_ocr(frame, max_width):
    """整屏缩到 max_width 宽再 OCR,速度翻倍。"""
    try:
        import cv2
        h, w = frame.shape[:2]
        if w <= max_width:
            return frame
        s = float(max_width) / w
        return cv2.resize(frame, (int(max_width), int(h * s)),
                          interpolation=cv2.INTER_AREA)
    except Exception:
        return frame


def _steam_window_state():
    """Steam 窗口状态:'visible'/'minimized'/'none';同时缓存窗口矩形供黑屏判定。"""
    now = time.time()
    if now - _STEAM_WIN['ts'] < 1.5:
        return _STEAM_WIN['state']
    resolved = 'none'
    try:
        import win32process
        from win32gui import EnumWindows, IsWindowVisible, GetWindowRect

        def _cb(hwnd):
            nonlocal resolved
            try:
                _, pid = win32process.GetWindowThreadProcessId(hwnd)
                name = ''
                try:
                    p = __import__('psutil').Process(pid)
                    name = (p.name() or '').lower()
                except Exception:
                    pass
                if name in _STEAM_WINDOW_PROCS:
                    l, t, r, b = GetWindowRect(hwnd)
                    visible = IsWindowVisible(hwnd)
                    if visible and r - l > 0 and b - t > 0:
                        resolved = 'visible'
                        _STEAM_WIN['rect'] = (l, t, r, b)
                        return None
                    if resolved != 'visible':
                        resolved = 'minimized'
                        return None
                    return None
                return None
            except Exception:
                return None

        EnumWindows(_cb, None)
    except Exception:
        pass
    _STEAM_WIN['state'] = resolved
    _STEAM_WIN['ts'] = now
    return resolved
def _steam_content_ratio(frame):
    """Steam 窗口内的有效画面占比(非近黑且非近白像素);无窗口/无帧返回 None。

    黑屏(纯黑)与白屏(纯白)都会让占比接近 0,据此判定"画面闪退"。
    """
    try:
        import numpy as np
        rect = _STEAM_WIN.get('rect')
        if frame is None or not rect:
            return None
        dl, dt, _, _ = _virtual_screen_rect()
        h, w = frame.shape[:2]
        l = max(0, int(rect[0] - dl))
        t = max(0, int(rect[1] - dt))
        r = min(w, int(rect[2] - dl))
        b = min(h, int(rect[3] - dt))
        if r - l < 16 or b - t < 16:
            return None
        roi = frame[t:b, l:r]
        mx = roi.max(axis=2)
        mn = roi.min(axis=2)
        good = np.count_nonzero((mx >= 30) & (mn <= 235))
        return float(good) / float(roi.shape[0] * roi.shape[1])
    except Exception:
        return None


def _make_steam_login_check(original):
    """包装引擎 Steam 登录检查:加运行态注册表判定兜底。"""
    def wrapped(self, username, *args, **kwargs):
        try:
            if original(self, username, *args, **kwargs):
                return True
        except Exception:
            pass
        return _steam_registry_login_ok(self, username)
    return wrapped


def _make_steam_login_reset(original):
    """包装引擎 Steam 登录重置:启动前清 ActiveUser。"""
    def wrapped(self, *args, **kwargs):
        _reset_steam_active_user()
        return original(self, *args, **kwargs)
    return wrapped


def _install_steam_login_compat(ctx):
    """把兼容补丁装到 EXE 的 SteamClient 上。"""
    try:
        from pc_tqc.platform import steam as steam_mod
    except Exception as e:
        ctx.log(STEP_NAME, f'Steam登录兼容补丁未安装(导入失败): {type(e).__name__}: {e}', 'WARN')
        return None
    try:
        steam_cls = getattr(steam_mod, 'SteamClient', None)
        if steam_cls is None:
            ctx.log(STEP_NAME, 'Steam登录兼容补丁未安装:未找到 SteamClient', 'WARN')
            return None
        if hasattr(steam_cls, 'is_account_logged_in'):
            ctx.patch_with(steam_cls, 'is_account_logged_in', _make_steam_login_check)
        if hasattr(steam_cls, 'login'):
            ctx.patch_with(steam_cls, 'login', _make_steam_login_reset)
        ctx.log(STEP_NAME, '已安装 Steam 登录判定兼容补丁(新版Steam无 loginusers.vdf 时按运行态注册表判定)', 'INFO')
        return None
    except Exception as e:
        ctx.log(STEP_NAME, f'Steam登录兼容补丁安装异常(忽略): {type(e).__name__}: {e}', 'WARN')
        return None


class _SteamWatchdog(threading.Thread):
    """Steam 启动兜底守卫:OCR 扫桌面 + 进程/窗口监测 + 自动重拉。"""

    def __init__(self, ctx, account):
        super().__init__(daemon=True, name='pc-tqc-steam-watch')
        self.ctx = ctx
        self.account = account
        self.tag = f'{STEP_NAME} · Steam守卫'
        self.stop_event = threading.Event()
        self.lost = False
        self.hit = False
        self.relaunch_count = 0
        self._seen = False
        self._missing = 0
        self._hit_count = 0
        self._window_seen = False
        self._no_window = 0
        self._proc_announced = False
        self._bad_screen = 0
        self._last_ratio = None
        self.screen_check = True
        self._t0 = time.time()
        self._next_ocr = self._t0
        self._next_log = self._t0 + 15.0
        self._next_err = 0.0

    def set_screen_check(self, enabled: bool, reason: str = ''):
        """启停"黑屏/白屏"画面判据;窗口/进程两条判据不受影响。

        启动游戏阶段画面会被启动器/全屏切换短暂变黑,此时若继续按占比判定
        会误判"画面闪退"并重拉 Steam(重拉必然打断启动,实测救不回来)。
        """
        enabled = bool(enabled)
        if enabled == getattr(self, 'screen_check', True):
            return None
        self.screen_check = enabled
        self._bad_screen = 0
        if enabled:
            self.ctx.log(self.tag, '画面判据已恢复:黑屏/白屏重新参与判定', 'INFO')
            return None
        self.ctx.log(self.tag, f'画面判据已暂停({reason or "启动游戏阶段"}):黑屏/白屏不再触发重拉,窗口/进程判据照常', 'INFO')
        return None

    def _ocr_joined(self, frame):
        """抓一帧全屏、涂黑自身窗口、OCR、拼成一行文本。frame 可复用已抓的帧。"""
        try:
            if frame is None:
                frame = _grab_desktop_bgr()
            if frame is None:
                return ''
            rects = _own_window_rects()
            if rects:
                _mask_own_windows(frame)
            items = self.ctx.ocr_scan(_shrink_for_ocr(frame), STEP_NAME)
            return ' '.join(str(t) for t, _ in items)
        except Exception:
            return ''

    def _scan_once(self, frame):
        """全屏识别一次;连续命中启动关键词+锚点才返回 True。frame 可复用已抓的帧。"""
        try:
            joined = self._ocr_joined(frame)
            if not joined:
                return False
            hit_kw = any(k in joined for k in STEAM_守卫_启动关键词)
            hit_anchor = any(k in joined for k in STEAM_守卫_启动锚点)
            if hit_kw and hit_anchor:
                self._hit_count = getattr(self, '_hit_count', 0) + 1
                if self._hit_count >= STEAM_守卫_命中确认次数:
                    self.hit = True
                    self.ctx.log(STEP_NAME, '★ 已识别到三角洲启动器「开始游戏」界面,停止画面扫描(继续监测 Steam 进程)', 'INFO')
                    return True
            self._hit_count = 0
            return False
        except Exception:
            return False

    def _screen_is_steam(self):
        """当前桌面是否还是 Steam 已知画面。"""
        try:
            joined = self._ocr_joined()
            return any(k in joined for k in STEAM_守卫_白名单关键词)
        except Exception:
            return False

    def _relaunch(self, reason):
        if self.relaunch_count >= STEAM_守卫_最多重启:
            self.lost = True
            self.ctx.log(self.tag, f'⚠ {reason};已重新拉起 {STEAM_守卫_最多重启} 次仍不稳定,停止扫描,本账号转入重跑', 'ERROR')
            self.stop_event.set()
            return None
        self.relaunch_count += 1
        self.lost = True
        uname = getattr(self.account, 'username', '?') if self.account else '?'
        self.ctx.log(self.tag, f'⚠ {reason};第 {self.relaunch_count}/{STEAM_守卫_最多重启} 次重新拉起 Steam(账号 {uname})', 'WARN')
        self._seen = False
        self._missing = 0
        self._bad_screen = 0
        self._no_window = 0
        self._last_ratio = None
        try:
            if self.account is None:
                raise RuntimeError('账号信息缺失,无法重新拉起')
            from pc_tqc.platform.steam import SteamClient, discover_steam_executable
            exe = discover_steam_executable()
            if not exe:
                raise RuntimeError('未找到 steam.exe')
            SteamClient(exe).login(self.account.username, self.account.password)
            self.ctx.log(self.tag, f'已重新拉起 Steam(账号 {uname}),等待自动登录', 'WARN')
            return None
        except Exception as e:
            self.lost = True
            self.ctx.log(self.tag, f'重新拉起 Steam 失败: {type(e).__name__}: {e}', 'ERROR')
            self.stop_event.set()
            return None

    def run(self):
        own = _own_window_rects()
        if own:
            self.ctx.log(self.tag, f'OCR 已屏蔽执行端自身窗口 {len(own)} 块(避免把自己的日志文字当成游戏画面)', 'INFO')
        else:
            self.ctx.log(self.tag, '⚠ 未能定位执行端窗口,OCR 仍可能读到自身日志文字', 'WARN')
        self.ctx.log(self.tag, '兜底扫描已启动:以桌面为准(窗口存在+画面正常=在);每 5 秒采样一次画面,进程状态只写日志、不作重拉依据(只观察,不杀进程)', 'INFO')
        grace_until = self._t0 + STEAM_守卫_初始宽限秒
        while not self.stop_event.is_set():
            try:
                now = time.time()
                if now >= self._next_ocr:
                    self._next_ocr = now + STEAM_守卫_扫描间隔秒
                    win_now = _steam_window_state()
                    frame = None
                    if win_now == 'visible':
                        frame = _grab_desktop_bgr()
                    ratio = None
                    if frame is not None and self.screen_check:
                        ratio = _steam_content_ratio(frame)
                    if ratio is not None:
                        self._last_ratio = ratio
                        if ratio < STEAM_守卫_画面正常占比:
                            self._bad_screen += 1
                            if self._bad_screen == 1:
                                self.ctx.log(self.tag, f'画面异常采样 {self._bad_screen}/{STEAM_守卫_画面异常确认次数}(窗口内有效像素 {ratio*100:.1f}%,进程{"在" if _steam_process_running() else "不在"})', 'WARN')
                        else:
                            if self._bad_screen:
                                self.ctx.log(self.tag, '画面恢复正常,异常计数清零', 'INFO')
                            self._bad_screen = 0
                    elif not self.screen_check:
                        self._bad_screen = 0
                    if self._scan_once(frame):
                        self._next_ocr = now + 86400.0
                win_state = _steam_window_state()
                alive_proc = _steam_process_running()
                now = time.time()
                proc_text = 'steam.exe 在' if alive_proc else 'steam.exe 不在'
                if win_state in ('visible', 'minimized'):
                    self._seen = True
                    self._missing = 0
                    self._no_window = 0
                    if not self._window_seen:
                        self._window_seen = True
                        self.ctx.log(self.tag, f'Steam 窗口已出现在桌面({win_state},{proc_text})', 'INFO')
                    if self.screen_check and win_state == 'visible' and now >= grace_until and self._bad_screen >= STEAM_守卫_画面异常确认次数:
                        pct = '未知' if self._last_ratio is None else f'{self._last_ratio*100:.1f}%'
                        if _game_or_launcher_running():
                            self._bad_screen = 0
                            self.ctx.log(self.tag, f'画面占比偏低({pct})但三角洲/启动器进程在运行,跳过画面闪退判定(不重拉)', 'INFO')
                        else:
                            self._bad_screen = 0
                            self._relaunch(f'Steam 窗口在但画面异常(窗口内有效像素 {pct},{proc_text}),判定画面闪退')
                            grace_until = time.time() + STEAM_守卫_重启宽限秒
                else:
                    self._bad_screen = 0
                    self._no_window += 1
                    if self._no_window >= STEAM_守卫_窗口缺失确认次数 and now >= grace_until:
                        self._no_window = 0
                        if _game_process_running():
                            self._missing = 0
                            self.ctx.log(self.tag, '桌面无 Steam 窗口但游戏已在运行,视为正常(不重拉)', 'INFO')
                        else:
                            self._relaunch(f'桌面枚举不到 Steam 窗口(含登录页,{proc_text}),判定已掉线')
                            grace_until = time.time() + STEAM_守卫_重启宽限秒
                if now >= self._next_log and not self.stop_event.is_set():
                    self._next_log = now + 15.0
                    win_text = {'visible': '可见', 'minimized': '最小化'}.get(win_state, '无')
                    ratio_text = '未知' if self._last_ratio is None else f'{self._last_ratio*100:.1f}%'
                    self.ctx.log(self.tag, f'扫描中... 窗口:{win_text} 画面:{ratio_text} {proc_text} 已重新拉起 {self.relaunch_count} 次', 'INFO')
                self.stop_event.wait(STEAM_守卫_进程轮询秒)
            except Exception as e:
                if time.time() >= self._next_err:
                    self._next_err = time.time() + 10.0
                    self.ctx.log(self.tag, f'扫描异常(继续): {type(e).__name__}: {e}', 'WARN')
                time.sleep(1.0)
        return None
def _start_steam_watchdog(ctx, account):
    """账号流程开始时启动兜底扫描(重复启动会先停掉上一轮)。\n\n    ★ 守卫只是兜底:启动失败只记 WARN,绝不阻断 Steam 登录主流程。\n    """
    _stop_steam_watchdog(ctx, '重新开始本账号流程')
    _STEAM_WATCH['lost'] = False
    _STEAM_WATCH['hit'] = False
    try:
        w = _SteamWatchdog(ctx, account)
        _STEAM_WATCH['thread'] = w
        w.start()
    except Exception as e:
        _STEAM_WATCH['thread'] = None
        ctx.log(f'{STEP_NAME} · Steam守卫', f'守卫启动失败(不影响登录主流程): {type(e).__name__}: {e}', 'WARN')


def _stop_steam_watchdog(ctx, reason=None):
    """停止兜底扫描,并把 lost/hit 状态留给 _run_account_once 判定。"""
    w = _STEAM_WATCH.get('thread')
    if w is None:
        return
    else:
        w.stop_event.set()
        _STEAM_WATCH['thread'] = None
        _STEAM_WATCH['lost'] = bool(w.lost)
        _STEAM_WATCH['hit'] = bool(w.hit)
        _STEAM_WATCH['relaunch'] = w.relaunch_count
        if ctx and reason:
                ctx.log(f'{STEP_NAME} · Steam守卫', f'兜底扫描结束({reason})', 'INFO')


def _make_wrapper(original, label, budget, ctx, pre_delay=0, steam_guard=False, skip_when_guarded=False, steam_watch=False, stop_steam_watch=False, screen_check_off=False, screen_check_on=False):
    """给 EXE 原方法套日志壳,可选挂载捕获保护和 Steam 守卫。\n\n    pre_delay > 0 时,在调用原方法前先等待 pre_delay 秒(用于杀完进程后\n    给 Steam 留出退出缓冲,避免旧进程未退干净导致登录卡在加载态)。\n    steam_guard=True:登录开始时开启捕获保护(橙框出现即禁止杀进程 180 秒),\n                     登录成功自动解除;异常/超时则等保护期自然结束。\n    skip_when_guarded=True:清理环境等杀进程步骤,在保护期内整段跳过。\n    steam_watch=True:登录开始即启动 Steam 兜底扫描(每 5 秒全屏识别启动器画面,\n                     进程消失自动重新拉起),一直扫到 _handle_launcher 结束。\n    stop_steam_watch=True:本步结束时停止兜底扫描(启动器阶段结束)。\n    screen_check_off=True:本步开始时暂停\"黑屏/白屏\"画面判据(启动游戏阶段),\n                          窗口/进程判据照常。\n    screen_check_on=True:本步结束时恢复画面判据(启动器画面识别完成/已进游戏)。\n    """
    def wrapped(self, *args, **kwargs):
        if skip_when_guarded and _guard_skip_kill(ctx, f'{label}(本步会杀进程)'):
            return
        else:
            if steam_watch:
                account = None
                if args:
                    account = args[0]
                else:
                    account = kwargs.get('account')
                _start_steam_watchdog(ctx, account)
            if steam_guard:
                _capture_guard_on(budget)
                ctx.log(STEP_NAME, f'捕获保护已开启: {budget}s 内禁止杀进程(Steam登录橙框)', 'INFO')
            if pre_delay > 0:
                ctx.log(STEP_NAME, f'{label} 等待 {pre_delay}s(清理进程退出缓冲)', 'INFO')
                time.sleep(pre_delay)
            if screen_check_off:
                _set_steam_screen_check(ctx, False, reason=label)
            ctx.log(STEP_NAME, f'{label} >>> 开始(参考预算 {budget}s)')
            started = time.time()
            result = original(self, *args, **kwargs)
            elapsed = time.time() - started
            level = 'WARN' if elapsed > budget * 0.85 else 'INFO'
            ctx.log(STEP_NAME, f'{label} <<< 完成({elapsed:.1f}s)', level)
            if steam_guard:
                _capture_guard_off()
                ctx.log(STEP_NAME, '登录完成,捕获保护解除(允许后续清理)', 'INFO')
            if screen_check_on:
                _set_steam_screen_check(ctx, True, reason=label)
            if stop_steam_watch:
                _stop_steam_watchdog(ctx, reason=f'{label}结束')
            return result
    return wrapped


def _is_launcher_component_error(exc):
    """判断异常是否为启动器登录组件报错(如错误码 1067105,非封号)。"""
    msg = f'{type(exc).__name__}: {exc}'
    return any((k in msg for k in 启动组件错误关键词))


def _set_steam_screen_check(ctx, enabled, reason):
    """启停守卫的"黑屏/白屏"画面判据(启动游戏阶段暂停,识别完启动器再恢复)。"""
    w = _STEAM_WATCH.get('thread')
    if w is None:
        return None
    try:
        w.set_screen_check(enabled, reason=reason)
        return None
    except Exception as e:
        try:
            ctx.log(f'{STEP_NAME} · Steam守卫', f'切换画面判据失败(忽略): {type(e).__name__}: {e}', 'WARN')
        except Exception:
            pass
        return None


def _dismiss_launcher_error(self, ctx):
    """重跑前尝试点掉启动器错误页的按钮。"""
    try:
        hwnd = ctx.resolve_hwnd(self, STEP_NAME)
        if not hwnd:
            return False
        frame = ctx.grab_frame(hwnd, STEP_NAME)
        if frame is None:
            return False
        items = ctx.ocr_scan(frame, STEP_NAME)
        joined = ' '.join(str(t) for t, _ in items)
        is_error_page = (any(k in joined for k in ('登录组件报错', '登录组件异常', '强制关闭', '需要重新启动', '1067105', '2007105'))
                         or '错误码' in joined or '启动器' in joined or '登录组件' in joined)
        if not is_error_page:
            return False
        for kw in ('启动', '确定'):
            from pc_tqc.platform import windows
            hit = ctx.find_kw(items, (kw,))
            if not hit:
                continue
            px, py = hit[1]
            if not ctx.click(hwnd, px, py, STEP_NAME):
                continue
            ctx.log(STEP_NAME, f'已点击启动器错误页「{kw}」按钮,准备重新登录', 'WARN')
            time.sleep(2)
            return True
        ctx.log(STEP_NAME, '识别到启动器错误页但未找到按钮,直接重跑', 'WARN')
        return True
    except Exception as e:
        ctx.log(STEP_NAME, f'点掉错误页异常(忽略): {type(e).__name__}: {e}', 'WARN')
        return False


def install(cls, ctx):
    """把启动阶段的 EXE 方法逐个套上日志壳 + 守卫 hook。"""
    _install_steam_login_compat(ctx)
    _install_kill_guard(ctx, cls)
    for method_name, label, budget in _SUB_STEPS:
        if not hasattr(cls, method_name):
            continue
        pre_delay = 0
        steam_guard = False
        steam_watch = False
        _delay = STEAM_登录前等待秒 if method_name == '_steam_login' else 0.0
        _steam_guard = method_name in ('_steam_login', '_wegame_login')
        _skip_cleanup = method_name == '_cleanup'
        _steam_watch = method_name == '_steam_login'
        _stop_watch = method_name == '_handle_launcher'
        _screen_off = method_name == '_launch_game'
        _screen_on = method_name == '_handle_launcher'
        ctx.patch_with(
            cls,
            method_name,
            lambda original, lb=label, bd=budget, dl=_delay, sg=_steam_guard,
                   sk=_skip_cleanup, sw=_steam_watch, stw=_stop_watch,
                   sdo=_screen_off, son=_screen_on:
                _make_wrapper(original, lb, bd, ctx,
                              pre_delay=dl, steam_guard=sg, skip_when_guarded=sk,
                              steam_watch=sw, stop_steam_watch=stw,
                              screen_check_off=sdo, screen_check_on=son),
        )
    if not hasattr(cls, '_run_account_once'):
        return
    original_once = ctx.get_original(cls, '_run_account_once')

    def _manual_state_path():
        try:
            base = os.path.dirname(os.path.abspath(__file__))
            base = os.path.dirname(os.path.dirname(base))
            return os.path.join(base, 'manual_state.json')
        except Exception:
            return ''

    def _load_paused_set():
        try:
            import json
            p = _manual_state_path()
            if not p or not os.path.exists(p):
                return set()
            data = json.load(open(p, 'r', encoding='utf-8'))
            return set(data.get('paused', []) or [])
        except Exception:
            return set()

    def wrapped_once(self, *args, **kwargs):
        state = kwargs.get('state')
        if state is None and args:
            state = args[0]
        try:
            uname = state.account.username if state else ''
        except Exception:
            uname = ''
        ctx.begin_account(uname)
        _STEAM_WATCH['account'] = state
        if uname and not os.environ.get('PC_TQC_MANUAL_EXEC'):
            paused = _load_paused_set()
            if uname in paused:
                ctx.log(STEP_NAME, f'⏸ 账号 {uname} 在手动暂停名单,跳过(不进游戏、不上传、不改计数)', 'INFO')
                from pc_tqc.models import RunStatus, StepResult
                from pc_tqc.workflow.runner import WorkflowStage
                return StepResult(RunStatus.SKIPPED, WorkflowStage.IDLE, message='账号在手动暂停名单,已跳过')
        if state is not None:
            try:
                state.has_saved_action_level = False
            except Exception:
                pass
        last_exc = None
        result = None
        for attempt in range(1, 启动组件错误_最多完整重跑 + 1):
            if ctx.stopped():
                _stop_steam_watchdog(ctx, reason='第一步: 已停止')
                raise RuntimeError('第一步: 已停止')
            try:
                result = original_once(self, *args, **kwargs)
                _stop_steam_watchdog(ctx, reason='第一步流程完成')
                return result
            except Exception as e:
                last_exc = e
                if 'ACCOUNT_BANNED' in str(e) and state is not None:
                    try:
                        from pc_tqc.models import RunStatus
                        state.status = RunStatus.BANNED
                        ban = getattr(ctx, 'last_ban', None) or {}
                        if ban.get('unban_at'):
                            from datetime import datetime as _dt
                            state.unban_at = _dt.strptime(ban['unban_at'], '%Y-%m-%d %H:%M:%S')
                        state.message = f'账号已封禁,解封时间 {ban.get("unban_at") or "未知"}'
                    except Exception:
                        pass
                    _stop_steam_watchdog(ctx, reason='ACCOUNT_BANNED')
                    raise
                steam_lost = bool(_STEAM_WATCH.get('lost'))
                if steam_lost and attempt < 启动组件错误_最多完整重跑:
                    _STEAM_WATCH['lost'] = False
                    ctx.log(STEP_NAME, f'⚠ Steam 中途消失且自动拉起未能恢复 第 {attempt}/{启动组件错误_最多完整重跑} 次,重新拉起 Steam 并重跑本账号', 'WARN')
                    _stop_steam_watchdog(ctx, reason='Steam lost, retrying')
                    continue
                can_retry = _is_launcher_component_error(e) and attempt < 启动组件错误_最多完整重跑 and not ctx.stopped()
                if not can_retry:
                    _stop_steam_watchdog(ctx, reason=f'异常结束: {type(e).__name__}')
                    raise
                ctx.log(STEP_NAME, f'⚠ 启动器登录组件报错(错误码1067105类,非封号) 第 {attempt}/{启动组件错误_最多完整重跑} 次,点确定后从清理环境重新登录重跑', 'WARN')
                _dismiss_launcher_error(self, ctx)
                continue
        _stop_steam_watchdog(ctx, reason='全部重跑失败')
        raise last_exc

    ctx.patch_with(cls, '_run_account_once', lambda original: wrapped_once)