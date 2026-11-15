# client_ui.py — 执行端主窗口（重建版：忠实 100% 语义转储 analysis_biz.txt + pyc 字节码钉死）
# 本文件 = 顶层工具函数 / 状态持久化 / 设备指纹 / 云端握手 / GUI 类（ConfigDialog/ClickableLabel/
# PasswordDialog/UpdateReadyDialog/UpdateDownloader/CloudWorker/UpsertWorker/RestoreWorker/RunnerWorkerThread）
# GUI 类按原版 pyc 字节码逐指令重建(不依赖 PySide6 运行时,可 ast.parse + compile)。

import hashlib, json, logging, os, re, shutil, subprocess, sys, tempfile, time
from datetime import datetime
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Optional

from client_config import ClientConfig, load_config, save_config
from cloud_client import (CloudClient, CloudAccount, CloudClientError,
                          CloudVerificationError, test_connection)
from sig_verify import USER_MESSAGE

# PySide6 导入(原版 client_ui.py L25-28)
from PySide6.QtCore import Qt, QTimer, Signal, Slot, QThread, QUrl, QTime
from PySide6.QtGui import (QColor, QFont, QIcon, QPalette, QPainter, QPixmap,
                           QDesktopServices)
from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                               QHBoxLayout, QLabel, QPushButton, QSpinBox,
                               QTableWidget, QTableWidgetItem, QTextEdit,
                               QHeaderView, QAbstractItemView, QSplitter,
                               QMessageBox, QDialog, QFormLayout, QLineEdit,
                               QDialogButtonBox, QProgressBar, QMenu,
                               QProgressDialog, QCheckBox, QTimeEdit,
                               QInputDialog)

logger = logging.getLogger('client_ui')

APP_VERSION = '1.9.20261115'
CLOUD_URL = 'https://teqinsi.cn'   # 模块级常量(原版 client_ui.py L44 直接赋值)
RELEASE_PUB_N = int('b8a2cb7a8b71d6b8e79a3bd0466bda1c9ead351fd2a9a280433475cfca2c497d001cea261415a3822ecc38350165e5a040a92e3c8eb4d375a5be7002b5ececaf8ca96c316b3371d93183e7c856396a0cb3091a8c51c00e715486012e6b3c9192ada8909d6b4bf568d26f463636d080cbcd6bc11ef185b5c63f70e66e6e273265f336a489b4b16a271f339694b99a1055483d62f0dcb441af93e90eb8f111cfd0e677cdeab0e5fa33dfa04317cf2eb38db396d7e8a3203470a49ba8d45b72d79fb50078dc2a6a026432829ed666516fc58c306a0f96429b5b70256f841ce1f377ba9f3bdfde55134377936278c0e073c740735cd4a327c8178aa8bac2b91688a17002eb7bf239eaca5f257519d47afbbd54c3ae2264f71a3a2ca01b01f44b43025720d16edad4baedc92ead5dc1ca6cc9dfeab01c47e86c2c54e7413ebb61b835f2b0aa94120db0426eb74a5d48cb96007c64f416c2928a9122b11f2787b7f1c002595bb1d0c82c2d61590bf15538cd24a019657467806b85d1926896ac54b56d', 16)   # 安装包发布者 RSA-2048 模数
RELEASE_PUB_E = 65537
_DIGESTINFO_SHA256 = bytes.fromhex('3031300d060960864801650304020105000420')
DEBUG_LOG = os.environ.get('PC_TQC_DEBUG_LOG', '').strip().lower() in ('1', 'true', 'yes', 'on')
_RE_POINT = re.compile(r'point=\(\s*-?\d{1,4}\s*,\s*-?\d{1,4}\s*\)')
_RE_CLICK = re.compile(r'点击\(\s*\d{1,4}\s*,\s*\d{1,4}\s*\)')
_RE_CLICK_LABEL = re.compile(r'(点击「[^」]*」)\(\s*\d{1,4}\s*,\s*\d{1,4}\s*\)')
_RE_CLICK_POS = re.compile(r'点击\(\s*\d{1,4}\s*,\s*\d{1,4}\s*\)')
_RE_KEY = re.compile(r'\b0x[0-9A-Fa-f]{2}\b')
_RE_SCORE = re.compile(r'score=\d+(?:\.\d+)?')
_RE_SCROLL = re.compile(r'scroll=[-\w.]+')
_RE_Y = re.compile(r'\by=-?\d+')
_RE_ROW = re.compile(r'\brow=-?\d+')
_RE_COL = re.compile(r'\bcol=-?\d+')
_RE_LABEL = re.compile(r'label=[^\s,)]+')
_RE_ITEM = re.compile(r'item_id=[^\s,)]+')
_LOG_DROP_KEYS = ('固定模板匹配成功', '制造列表候选确认')
_FACILITY_LABELS = (('tech', '技术中心'), ('workbench', '工作台'), ('pharmacy', '制药台'), ('armor', '防具台'))
STATUS_COLORS = {
    'PENDING': '#9e9e9e', 'WAITING': '#ff9800', 'READY': '#4fc3f7',
    'RUNNING': '#4caf50', 'SUCCESS': '#8bc34a', 'BANNED': '#f44336',
    'OBSERVATION': '#ff9800', 'FAILED': '#e91e63', 'STOPPED': '#795548',
    'SKIPPED': '#607d8b', 'PAUSED': '#607d8b',
}
STATUS_NAMES = {
    'PENDING': '等待', 'WAITING': '等待时间', 'READY': '待执行',
    'RUNNING': '运行中', 'SUCCESS': '成功', 'BANNED': '封禁',
    'OBSERVATION': '观察期', 'FAILED': '失败', 'STOPPED': '已停止',
    'SKIPPED': '已跳过',
}
STATUS_CN_TO_CODE = {v: k for k, v in STATUS_NAMES.items()}
TERMINAL_STATUSES = frozenset({'FAILED', 'SUCCESS', 'OBSERVATION', 'BANNED', 'SKIPPED', 'STOPPED'})
RUN_LIMIT_TIMES = 3
RUN_LIMIT_WINDOW = 3600
DARK_STYLE = """\nQMainWindow {\n    background-color: transparent;\n}\nQDialog {\n    background-color: #23201a;\n    color: #e8e2d4;\n}\nQWidget {\n    background-color: transparent;\n    color: #e8e2d4;\n    font-family: "Microsoft YaHei", "SimHei", sans-serif;\n    font-size: 12px;\n}\nQTableWidget {\n    background-color: rgba(14, 18, 24, 205);\n    gridline-color: rgba(120, 130, 145, 60);\n    border: 1px solid rgba(140, 150, 165, 80);\n    border-radius: 4px;\n}\nQTableWidget::item {\n    padding: 4px;\n    border-bottom: 1px solid rgba(120, 130, 145, 45);\n}\nQTableWidget::item:selected {\n    background-color: rgba(88, 130, 180, 110);\n    color: #f2ede2;\n}\nQHeaderView::section {\n    background-color: rgba(26, 32, 42, 225);\n    color: #d5c9ae;\n    padding: 6px;\n    border: none;\n    font-weight: bold;\n}\nQTextEdit {\n    background-color: rgba(8, 10, 14, 228);\n    color: #e8dfc8;\n    border: 1px solid rgba(140, 150, 165, 80);\n    border-radius: 4px;\n    font-family: "Consolas", "Courier New", monospace;\n    font-size: 11px;\n}\nQPushButton {\n    background-color: rgba(52, 60, 74, 215);\n    color: #e8e2d4;\n    border: 1px solid rgba(150, 160, 175, 90);\n    border-radius: 4px;\n    padding: 6px 16px;\n    font-weight: bold;\n}\nQPushButton:hover {\n    background-color: rgba(66, 76, 92, 225);\n}\nQPushButton:pressed {\n    background-color: rgba(40, 46, 58, 235);\n}\nQPushButton:disabled {\n    background-color: rgba(40, 44, 52, 160);\n    color: rgba(160, 158, 150, 130);\n}\nQPushButton#stopBtn {\n    background-color: rgba(160, 62, 44, 230);\n    color: white;\n    font-size: 14px;\n    padding: 8px 24px;\n}\nQPushButton#stopBtn:hover {\n    background-color: rgba(186, 78, 56, 240);\n}\nQPushButton#stopBtn:disabled {\n    background-color: rgba(90, 60, 54, 160);\n    color: rgba(180, 160, 150, 120);\n}\nQPushButton#startBtn {\n    background-color: rgba(88, 122, 58, 230);\n    color: white;\n    font-size: 14px;\n    padding: 8px 24px;\n}\nQPushButton#startBtn:hover {\n    background-color: rgba(102, 140, 68, 240);\n}\nQPushButton#restoreBtn {\n    background-color: rgba(180, 122, 32, 230);\n    color: white;\n    font-weight: bold;\n}\nQPushButton#restoreBtn:hover {\n    background-color: rgba(198, 140, 44, 240);\n}\nQPushButton#restoreBtn:disabled {\n    background-color: rgba(90, 76, 52, 160);\n    color: rgba(170, 160, 140, 120);\n}\nQMenu {\n    background-color: #262c36;\n    color: #e8e2d4;\n    border: 1px solid rgba(140, 150, 165, 90);\n}\nQMenu::item:selected {\n    background-color: rgba(88, 130, 180, 120);\n}\nQSpinBox {\n    background-color: rgba(20, 25, 32, 215);\n    color: #e8e2d4;\n    border: 1px solid rgba(140, 150, 165, 90);\n    border-radius: 4px;\n    padding: 4px;\n}\nQLineEdit {\n    background-color: rgba(20, 25, 32, 215);\n    color: #e8e2d4;\n    border: 1px solid rgba(140, 150, 165, 90);\n    border-radius: 4px;\n    padding: 6px;\n}\nQLabel#statLabel {\n    color: #c9b892;\n    font-weight: bold;\n    padding: 4px 8px;\n}\nQLabel#statValue {\n    color: #e8a83e;\n    font-weight: bold;\n    padding: 4px 8px;\n}\nQLabel#serverInfo {\n    color: #9ec48a;\n    font-size: 11px;\n}\nQProgressBar {\n    background-color: rgba(20, 25, 32, 215);\n    border: 1px solid rgba(140, 150, 165, 90);\n    border-radius: 4px;\n    height: 16px;\n    text-align: center;\n}\nQProgressBar::chunk {\n    background-color: #c98f2d;\n    border-radius: 4px;\n}\nQPushButton#cellBtn {\n    padding: 2px 8px;\n    font-size: 11px;\n    border-radius: 3px;\n}\nQStatusBar {\n    background-color: rgba(12, 15, 20, 200);\n    color: #b8b0a0;\n}\nQToolTip {\n    background-color: #262c36;\n    color: #e8e2d4;\n    border: 1px solid rgba(140, 150, 165, 90);\n}\nQLabel#updateTip {\n    color: #ffd166;\n    font-weight: bold;\n    padding: 2px 4px;\n}\nQPushButton#updateBtn {\n    background-color: rgba(196, 124, 36, 235);\n    color: #fff5e0;\n    border: 1px solid rgba(255, 196, 110, 150);\n}\nQPushButton#updateBtn:hover {\n    background-color: rgba(222, 148, 52, 245);\n}\n"""


def _mask_sensitive_log(text: str) -> Optional[str]:
    """隐藏坐标/按键码/模板分数/滚动位置/模板名/候选明细;返回 None 表示整行丢弃。"""
    if not text:
        return text
    if any(k in text for k in _LOG_DROP_KEYS):
        return None
    text = _RE_POINT.sub('point=(***)', text)
    text = _RE_CLICK.sub('已点击(***)', text)
    text = _RE_CLICK_LABEL.sub('\\1(***)', text)
    text = _RE_CLICK_POS.sub('点击(***)', text)
    text = _RE_KEY.sub('***', text)
    text = _RE_SCORE.sub('score=***', text)
    text = _RE_SCROLL.sub('scroll=***', text)
    text = _RE_Y.sub('y=***', text)
    text = _RE_ROW.sub('row=***', text)
    text = _RE_COL.sub('col=***', text)
    text = _RE_LABEL.sub('label=***', text)
    text = _RE_ITEM.sub('item_id=***', text)
    return text


def _get_config_dir() -> Path:
    """执行端配置目录: %USERPROFILE%/.pc_tqc_client/。"""
    up = os.environ.get('USERPROFILE', str(Path.home()))
    d = Path(up) / '.pc_tqc_client'
    d.mkdir(parents=True, exist_ok=True)
    return d


def _get_config_path() -> Path:
    return _get_config_dir() / 'config.json'


def _cleanup_stale_runner_cfg(max_age_sec: int = 600) -> None:
    """清理旧版本遗留的明文配置(新版本已改为 stdin 传参,不再产生该文件)。"""
    try:
        _tmp = tempfile.gettempdir()
        for _name in os.listdir(_tmp):
            if _name.startswith('pc_tqc_runner_cfg_') and _name.endswith('.json'):
                _p = os.path.join(_tmp, _name)
                try:
                    if time.time() - os.path.getmtime(_p) > max_age_sec:
                        os.remove(_p)
                except Exception:
                    continue
    except Exception:
        pass


def verify_release_signature(sha256_hex: str, sig_hex: str) -> bool:
    """校验安装包发布者签名;签名只覆盖 SHA256 摘要,防止包被替换。"""
    try:
        sig = bytes.fromhex(str(sig_hex or '').strip())
        digest = bytes.fromhex(str(sha256_hex or '').strip())
    except ValueError:
        return False
    size = (RELEASE_PUB_N.bit_length() + 7) // 8
    if len(sig) != size or len(digest) != 32:
        return False
    try:
        em = pow(int.from_bytes(sig, 'big'), RELEASE_PUB_E, RELEASE_PUB_N).to_bytes(size, 'big')
    except Exception:
        return False
    tail = _DIGESTINFO_SHA256 + digest
    pad = len(em) - len(tail) - 3
    if pad < 8:
        return False
    return em == b'\x00\x01' + b'\xff' * pad + b'\x00' + tail


def _format_hafu_w(value) -> str:
    """哈夫币 → 万(w)显示串,仅用于界面展示,例如 40998000 → '4099.8w'。
    不足1w或无法解析时原样返回;底层数据与云端回传一律使用原始值。"""
    if value is None or value == '':
        return '-'
    s = str(value).strip().replace(',', '')
    try:
        n = int(s)
    except ValueError:
        return str(value)
    if abs(n) < 10000:
        return str(n)
    return f'{n / 10000:.1f}w'


def _earliest_facility_time(timers: dict, future_only: bool = False):
    """从4设施到期时间取最早一个(引擎到最早设施到期即醒来开跑)。
    入参 {tech/workbench/pharmacy/armor: 'YYYY-MM-DD HH:MM:SS'},返回最早 datetime;无则 None。"""
    if not isinstance(timers, dict):
        return None
    times = []
    now = datetime.now()
    for raw in timers.values():
        if not raw:
            continue
        try:
            dt = datetime.fromisoformat(str(raw))
        except ValueError:
            continue
        if future_only and dt <= now:
            continue
        times.append(dt)
    return min(times) if times else None


def _fmt_hms(seconds: int) -> str:
    """秒 → H:MM:SS(小时不封顶,可超过24)。"""
    p = lambda n: str(n).zfill(2)
    return f'{seconds // 3600}:{p((seconds % 3600) // 60)}:{p(seconds % 60)}'


def _facility_countdown_text(timers: dict) -> str:
    """4设施实时倒计时多行文本(每秒重算):
        技术中心 02:44:08
        ...
        制药台 可执行
    无任何有效时间返回 ''。"""
    if not isinstance(timers, dict):
        return ''
    now = datetime.now()
    lines = []
    any_time = False
    for key, label in _FACILITY_LABELS:
        raw = timers.get(key)
        if not raw:
            lines.append(f'{label} —')
            continue
        try:
            dt = datetime.fromisoformat(str(raw))
        except ValueError:
            lines.append(f'{label} —')
            continue
        any_time = True
        remain = int((dt - now).total_seconds())
        lines.append(f'{label} {_fmt_hms(remain) if remain > 0 else "可执行"}')
    if any_time:
        return '\n'.join(lines)
    return ''


def _runner_db_path() -> str:
    """runner(原版EXE核心)的本地调度库路径: 桌面/PC特勤处/pc_tqc.db"""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,
                            r'Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders') as key:
            desktop = os.path.expandvars(str(winreg.QueryValueEx(key, 'Desktop')[0]).strip())
            if desktop:
                return os.path.join(desktop, 'PC特勤处', 'pc_tqc.db')
    except (ImportError, OSError):
        pass
    return os.path.join(os.path.expanduser('~'), 'Desktop', 'PC特勤处', 'pc_tqc.db')


def _load_facility_schedules() -> dict:
    """读 runner 本地调度库: {username: 最晚设施到期时间(datetime),无有效时间则为 None}。
    库不存在/读取失败时返回空表。"""
    import sqlite3
    db = _runner_db_path()
    result = {}
    if not os.path.exists(db):
        return result
    try:
        conn = sqlite3.connect(db, timeout=10)
        try:
            cur = conn.cursor()
            for name, tech, wb, ph, ar in cur.execute(
                    'SELECT username, tech_next_at, workbench_next_at, pharmacy_next_at, armor_next_at FROM account_state'):
                times = []
                for raw in (tech, wb, ph, ar):
                    if not raw:
                        continue
                    try:
                        times.append(datetime.fromisoformat(str(raw)))
                    except ValueError:
                        continue
                result[name] = max(times) if times else None
        finally:
            conn.close()
    except Exception:
        return result
    return result


def _last_run_store_path() -> str:
    """最近运行时间持久化文件(与客户端配置同目录)。"""
    base = os.environ.get('USERPROFILE', os.path.expanduser('~'))
    d = os.path.join(base, '.pc_tqc_client')
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, 'last_run.json')


def _atomic_write_json(path: str, data) -> None:
    """原子写 JSON:先写同目录临时文件再 os.replace,避免断电/崩溃留下半截文件。"""
    tmp = path + '.tmp'
    with open(tmp, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        f.flush()
        os.fsync(f.fileno())
    os.replace(tmp, path)


def _load_last_run_store() -> dict:
    try:
        with open(_last_run_store_path(), 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception:
        return {}


def _save_last_run_store(store: dict) -> None:
    try:
        _atomic_write_json(_last_run_store_path(), store)
    except Exception as e:
        logger.warning('保存 last_run.json 失败: %s: %s', type(e).__name__, e)


def _known_accounts_path() -> str:
    base = os.environ.get('USERPROFILE', os.path.expanduser('~'))
    d = os.path.join(base, '.pc_tqc_client')
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, 'known_accounts.json')


def _load_known_accounts() -> dict:
    try:
        with open(_known_accounts_path(), 'r', encoding='utf-8') as f:
            return {str(k): int(v) for k, v in json.load(f).items()}
    except Exception:
        return {}


def _save_known_accounts(mapping: dict) -> None:
    try:
        with open(_known_accounts_path(), 'w', encoding='utf-8') as f:
            json.dump(mapping, f, ensure_ascii=False, indent=2)
    except Exception:
        pass


def _manual_state_path() -> str:
    """手动操作状态文件(EXE 同目录,跨重启保留暂停名单/锁定名单/密码哈希)"""
    if getattr(sys, 'frozen', False):
        base = os.path.dirname(sys.executable)
    else:
        base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base, 'manual_state.json')


def _load_manual_state() -> dict:
    try:
        with open(_manual_state_path(), 'r', encoding='utf-8') as f:
            data = json.load(f)
        if isinstance(data, dict):
            return data
        return {}
    except Exception:
        return {}


def _save_manual_state(data: dict) -> None:
    try:
        _atomic_write_json(_manual_state_path(), data)
    except Exception as e:
        logger.warning('保存 manual_state.json 失败: %s: %s', type(e).__name__, e)


def _hash_password(pwd: str, salt: str) -> str:
    """PBKDF2 口令哈希(本地只存哈希,绝不存明文)。"""
    return hashlib.pbkdf2_hmac('sha256', pwd.encode('utf-8'), salt, 100000).hex()


def device_fingerprint() -> str:
    """本机设备指纹:MachineGuid + 系统盘卷序列号 + 网卡MAC 的 SHA256。

    只读取系统标识,不读取/修改任何进程内存,也不注入或结束任何进程。"""
    parts = []
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            r'SOFTWARE\Microsoft\Cryptography',
                            0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY) as k:
            parts.append(str(winreg.QueryValueEx(k, 'MachineGuid')[0]))
    except Exception:
        parts.append('')
    try:
        import ctypes
        serial = ctypes.c_ulong(0)
        ok = ctypes.windll.kernel32.GetVolumeInformationW(
            ctypes.c_wchar_p(os.environ.get('SystemDrive', 'C:') + '\\'),
            None, 0, ctypes.byref(serial), None, None, None, 0)
        parts.append(str(serial.value) if ok else '')
    except Exception:
        parts.append('')
    try:
        import uuid
        parts.append('%012x' % uuid.getnode())
    except Exception:
        parts.append('')
    return hashlib.sha256('|'.join(parts).encode('utf-8')).hexdigest()


def cloud_handshake(client):
    """与云端握手(设备绑定 + 会话签名)。

    返回 (blocked, detail):
      blocked=True  → 授权/设备校验不通过,必须锁住"开始",detail 为提示文案;
      blocked=False → 握手通过,或仅网络异常(交给原有刷新逻辑判断)。
    """
    try:
        res = client._post('/api/client/handshake', {
            'deviceId': device_fingerprint(),
            'appVersion': APP_VERSION,
            'platform': 'windows',
            'os': f'{sys.platform} {os.environ.get("PROCESSOR_ARCHITECTURE", "")}',
        })
    except CloudVerificationError as e:
        return (True, str(e))
    except CloudClientError as e:
        detail = MainWindow._license_detail(str(e))
        if detail:
            return (True, detail)
        return (False, '')
    except Exception:
        return (False, '')
    if res.get('ok', True):
        try:
            client.set_session(res.get('session', ''), res.get('expiresIn', 0))
        except Exception:
            return (False, '')
        return (False, '')
    return (True, res.get('error') or '设备校验失败,请联系管理员')


# ================================================================== GUI 类
# 以下 GUI 类按原版 pyc 字节码逐指令重建(结构/常量/调用序列忠实)。
# PySide6 未安装时本文件仍可 ast.parse/compile(类定义惰性,import 在 try/except 内)。
try:
    from PySide6.QtCore import Qt, QTimer, Signal, Slot, QThread, QUrl, QTime
    from PySide6.QtGui import QColor, QFont, QIcon, QPalette, QPainter, QPixmap, QDesktopServices
    from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                                   QLabel, QPushButton, QSpinBox, QDialog, QFormLayout, QLineEdit,
                                   QDialogButtonBox, QInputDialog, QMessageBox)
    _PYSIDE_OK = True
except Exception:
    _PYSIDE_OK = False
    class _Stub:
        def __init__(self, *a, **k): pass
        def __getattr__(self, n):
            class _M:
                def __call__(self_, *a, **k): pass
            return _M()
    class _SignalStub:
        def __init__(self, *a, **k): self._t = a
        def connect(self, *a, **k): pass
        def emit(self, *a, **k): pass
        def __call__(self, *a, **k): return self
        def __getattr__(self, n): return (lambda *a, **k: None)
    Qt = QTimer = QThread = QUrl = QTime = _Stub
    Slot = _Stub
    Signal = _SignalStub
    QColor = QFont = QIcon = QPalette = QPainter = QPixmap = QDesktopServices = _Stub
    QApplication = QMainWindow = QWidget = QVBoxLayout = QHBoxLayout = QLabel = QPushButton = _Stub
    QSpinBox = QDialog = QFormLayout = QLineEdit = QDialogButtonBox = QInputDialog = QMessageBox = _Stub


class ConfigDialog:
    '''授权配置对话框(服务器地址可编辑,留空用内置默认值)'''
    def __init__(self, parent, config: ClientConfig = None):
        super().__init__(parent)
        self.setWindowTitle('授权配置')
        self.setModal(True)
        self.setMinimumWidth(450)
        self.config = config or ClientConfig()
        layout = QFormLayout(self)
        self.server_edit = QLineEdit(self.config.server_url or CLOUD_URL)
        self.server_edit.setPlaceholderText(CLOUD_URL)
        layout.addRow('服务器地址:', self.server_edit)
        self.token_edit = QLineEdit(self.config.token)
        self.token_edit.setPlaceholderText('tqc_xxxxxxxxxxxxxxxx')
        self.token_edit.setEchoMode(QLineEdit.Password)
        layout.addRow('访问令牌:', self.token_edit)
        self.test_btn = QPushButton('测试连接')
        self.test_btn.clicked.connect(self._test_connection)
        layout.addRow('', self.test_btn)
        self.test_result = QLabel('')
        layout.addRow('', self.test_result)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addRow(buttons)

    def _test_connection(self):
        server = self.server_edit.text().strip() or CLOUD_URL
        token = self.token_edit.text().strip()
        if not token:
            self.test_result.setText('请填写访问令牌')
            self.test_result.setStyleSheet('color: #f44336;')
            return None
        self.test_btn.setEnabled(False)
        self.test_result.setText('正在测试...')
        self.test_result.setStyleSheet('color: #ff9800;')
        QApplication.processEvents()
        ok, msg = test_connection(server, token)
        try:
            if server:
                msg = msg.replace(server, '***')
                _h = server.split('://', 1)[-1].split('/')[0].strip()
                if _h:
                    msg = msg.replace(_h, '***')
        except Exception:
            pass
        self.test_result.setText(msg)
        self.test_result.setStyleSheet('color: #4caf50;' if ok else 'color: #f44336;')
        self.test_btn.setEnabled(True)
        return None

    def get_config(self) -> ClientConfig:
        return ClientConfig(
            server_url=self.server_edit.text().strip() or CLOUD_URL,
            token=self.token_edit.text().strip(),
            platform_mode=getattr(self.config, 'platform_mode', 'steam'),
            trade_price_adjust=getattr(self.config, 'trade_price_adjust', 4),
            auto_start_queue=getattr(self.config, 'auto_start_queue', False),
            ammo_exchange_enabled=bool(getattr(self.config, 'ammo_exchange_enabled', False)),
            aw_enabled=bool(getattr(self.config, 'aw_enabled', False)),
            sell_window_enabled=bool(getattr(self.config, 'sell_window_enabled', False)),
            sell_window_start=str(getattr(self.config, 'sell_window_start', '14:00')),
            sell_window_end=str(getattr(self.config, 'sell_window_end', '20:00')),
        )


class ClickableLabel:
    '''可点击的统计数值标签(V1.8:点击过滤主表格)'''
    clicked = Signal()

    def __init__(self, text='', parent=None):
        super().__init__(text, parent)
        self.setCursor(Qt.PointingHandCursor)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.clicked.emit()
        super().mousePressEvent(event)


class PasswordDialog:
    """登录密码对话框:mode='verify' 验证密码(解锁/上锁账号登录);mode='set' 设置/修改密码"""
    def __init__(self, parent, mode='verify', has_old=False):
        super().__init__(parent)
        self.setModal(True)
        self.setMinimumWidth(380)
        self.mode = mode
        self.setWindowTitle('验证登录密码' if mode == 'verify' else '设置登录密码')
        form = QFormLayout(self)
        self.old_edit = None
        if mode == 'set' and has_old:
            self.old_edit = QLineEdit()
            self.old_edit.setEchoMode(QLineEdit.Password)
            form.addRow('当前密码:', self.old_edit)
        self.pwd_edit = QLineEdit()
        self.pwd_edit.setEchoMode(QLineEdit.Password)
        if mode == 'verify':
            self.pwd_edit.setPlaceholderText('请输入登录密码')
            form.addRow('登录密码:', self.pwd_edit)
        else:
            self.pwd_edit.setPlaceholderText('用于上锁账号的手动登录与解锁')
            form.addRow('新密码:', self.pwd_edit)
        self.pwd2_edit = None
        if mode == 'set':
            self.pwd2_edit = QLineEdit()
            self.pwd2_edit.setEchoMode(QLineEdit.Password)
            form.addRow('确认新密码:', self.pwd2_edit)
        self.err_label = QLabel('')
        self.err_label.setStyleSheet('color: #b0533c;')
        form.addRow('', self.err_label)
        self.reset_requested = False
        if mode == 'set':
            self.forgot_btn = QPushButton('忘记密码')
            self.forgot_btn.setToolTip('输入配置中的访问令牌(授权码)后重置登录密码')
            self.forgot_btn.setStyleSheet('color: #c98f2d;')
            self.forgot_btn.clicked.connect(self._on_forgot)
            form.addRow('', self.forgot_btn)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(self._on_accept)
        buttons.rejected.connect(self.reject)
        form.addRow(buttons)

    def _on_accept(self):
        pwd = self.pwd_edit.text()
        if self.mode == 'verify':
            if not pwd:
                self.err_label.setText('请输入密码')
                return None
            self.accept()
            return None
        if self.old_edit is not None and not self.old_edit.text():
            self.err_label.setText('请输入当前密码')
            return None
        if not pwd:
            self.err_label.setText('请输入新密码')
            return None
        if self.pwd2_edit is not None and pwd != self.pwd2_edit.text():
            self.err_label.setText('两次输入的密码不一致')
            return None
        self.accept()

    def _on_forgot(self):
        code, ok = QInputDialog.getText(self, '忘记密码', '请输入授权码(配置中的访问令牌):', QLineEdit.Password)
        if not ok:
            return None
        token = (ClientConfig.load().token or '').strip()
        if token and (code or '').strip() != token:
            self.err_label.setText('授权码不正确')
            return None
        self.reset_requested = True
        self.accept()

    @property
    def password(self) -> str:
        return self.pwd_edit.text()

    @property
    def old_password(self) -> str:
        return self.old_edit.text() if self.old_edit is not None else ''


class UpdateReadyDialog:
    '''更新确认框:沿用原生提示框字体与样式,「Yes」需阅读 5 秒后才可点击。'''
    READ_SECONDS = 5

    def __init__(self, parent):
        super().__init__(parent)
        self.setWindowTitle('更新已就绪')
        self.setIcon(QMessageBox.Question)
        self.setText('安装包已下载完成,并通过发布者签名校验。\n点击「Yes」后本程序会自动退出,安装程序随后自动弹出。\n\n请注意:\n1. 解压与安装需要数分钟,请耐心等待,不要关闭程序;\n2. 解压界面会自动弹出,请勿重复点击、关闭或取消;\n3. 安装完成前请勿启动游戏或重复打开本程序。\n\n请阅读以上提示 5 秒后再点击「Yes」。')
        self._left = self.READ_SECONDS
        self.yes_btn = self.addButton('Yes(%d)' % self._left, QMessageBox.YesRole)
        self.no_btn = self.addButton('No', QMessageBox.NoRole)
        self.yes_btn.setEnabled(False)
        self.setDefaultButton(self.no_btn)
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._tick)
        self._timer.start(1000)

    def _tick(self):
        self._left -= 1
        if self._left > 0:
            self.yes_btn.setText('Yes(%d)' % self._left)
            return None
        self._timer.stop()
        self.yes_btn.setText('Yes')
        self.yes_btn.setEnabled(True)
        self.setDefaultButton(self.yes_btn)

    def done(self, result):
        self._timer.stop()
        super().done(result)


class UpdateDownloader(QThread):
    '''多线程分段下载更新包:支持断点续传、下载链接过期自动刷新,校验 SHA256 与发布者签名。'''
    progress = Signal(int)
    finished_ok = Signal(str)
    failed = Signal(str)

    CHUNK = 1048576
    PART_SIZE = 25165824
    MIN_PARTS = 4
    MAX_PARTS = 8

    def __init__(self, url: str, sha256: str, dest: str, sig: str = '', url_refresh=None):
        super().__init__()
        self.url = url
        self.sha256 = sha256
        self.sig = sig
        self.dest = dest
        self.url_refresh = url_refresh
        self.parts_dir = dest + '.' + (self.sha256 or '')[:8] + '.parts'
        self._lock = threading.Lock()
        self._got = 0
        self._total = 0
        self._last_pct = -1

    def _add_progress(self, n):
        with self._lock:
            self._got += n
            total = self._total
            got = self._got
        if not total:
            return None
        pct = min(99, got * 100 // total)
        if pct != self._last_pct:
            self._last_pct = pct
            self.progress.emit(pct)
            return None
        return None

    def _refresh_url(self):
        '''下载链接过期(401/403)时向云端重新取一次签名链接。'''
        if not self.url_refresh:
            return False
        try:
            fresh = str(self.url_refresh() or '').strip()
            if not fresh:
                return False
            self.url = fresh
            return True
        except Exception:
            return False

    def _probe(self, requests):
        '''探测总大小与是否支持 Range 分段。'''
        for attempt in (0, 1):
            with requests.get(self.url, headers={'Range': 'bytes=0-0'}, stream=True, timeout=30) as resp:
                if resp.status_code in (401, 403) and attempt == 0 and self._refresh_url():
                    continue
                resp.raise_for_status()
                content_range = resp.headers.get('Content-Range') or ''
                if '/' in content_range:
                    return (int(content_range.rsplit('/', 1)[-1]), True)
                return (int(resp.headers.get('Content-Length') or 0), False)
        raise PermissionError('下载链接已过期')

    def _fetch_part(self, requests, index, start, end):
        '''下载 [start, end] 区间;断线后从该段已写位置继续。'''
        path = os.path.join(self.parts_dir, str(index))
        had = os.path.getsize(path) if os.path.exists(path) else 0
        pos = start + min(had, end - start + 1)
        if pos > end:
            return None
        retries = 0
        with open(path, 'ab' if had else 'wb') as f:
            while pos <= end:
                try:
                    with requests.get(self.url, stream=True, timeout=30,
                                      headers={'Range': 'bytes=%d-%d' % (pos, end)}) as resp:
                        if resp.status_code in (401, 403):
                            raise PermissionError('下载链接已过期')
                        resp.raise_for_status()
                        if resp.status_code != 206:
                            raise RuntimeError('服务器未按分段返回数据')
                        for chunk in resp.iter_content(self.CHUNK):
                            if not chunk:
                                continue
                            if pos + len(chunk) > end + 1:
                                chunk = chunk[:end + 1 - pos]
                            f.write(chunk)
                            f.flush()
                            pos += len(chunk)
                            self._add_progress(len(chunk))
                            if pos > end:
                                break
                    retries = 0
                except Exception as exc:
                    if isinstance(exc, PermissionError):
                        self._refresh_url()
                    retries += 1
                    if retries > self.MAX_RETRY:
                        raise
                    time.sleep(min(5, retries))

    def _download_ranged(self, requests):
        '''按段并发下载,全部完成后顺序合并为最终文件。'''
        from concurrent.futures import ThreadPoolExecutor, as_completed
        size = self._total
        count = max(self.MIN_PARTS, min(self.MAX_PARTS, (size + self.PART_SIZE - 1) // self.PART_SIZE))
        base = size // count
        bounds = []
        offset = 0
        for i in range(count):
            end = size - 1 if i == count - 1 else offset + base - 1
            bounds.append((offset, end))
            offset = end + 1
        os.makedirs(self.parts_dir, exist_ok=True)
        for i, (start, end) in enumerate(bounds):
            path = os.path.join(self.parts_dir, str(i))
            if not os.path.exists(path):
                continue
            self._got += min(os.path.getsize(path), end - start + 1)
        with ThreadPoolExecutor(max_workers=count) as pool:
            futures = [pool.submit(self._fetch_part, requests, i, start, end) for i, (start, end) in enumerate(bounds)]
            for future in as_completed(futures):
                future.result()
        with open(self.dest, 'wb') as out:
            for i in range(count):
                with open(os.path.join(self.parts_dir, str(i)), 'rb') as f:
                    shutil.copyfileobj(f, out, 4194304)
        shutil.rmtree(self.parts_dir, ignore_errors=True)

    def _download_single(self, requests):
        '''服务器不支持分段时的单线程兜底。'''
        tmp = self.dest + '.part'
        resp = requests.get(self.url, stream=True, timeout=30)
        if resp.status_code in (401, 403) and self._refresh_url():
            resp.close()
            return self._download_single(requests)
        try:
            resp.raise_for_status()
            total = int(resp.headers.get('Content-Length') or 0) or self._total
            self._total = total
            with open(tmp, 'wb') as f:
                for chunk in resp.iter_content(self.CHUNK):
                    if not chunk:
                        continue
                    f.write(chunk)
                    self._add_progress(len(chunk))
            resp.close()
            os.replace(tmp, self.dest)
        except:
            resp.close()
            raise

    @staticmethod
    def _sha256(path):
        h = hashlib.sha256()
        with open(path, 'rb') as f:
            for chunk in iter(lambda: f.read(4194304), b''):
                h.update(chunk)
        return h.hexdigest()

    def _cleanup(self, remove_dest=False):
        '''清理下载中间文件。'''
        shutil.rmtree(self.parts_dir, ignore_errors=True)
        for path in (self.dest + '.part', self.dest if remove_dest else ''):
            if path:
                try:
                    if os.path.exists(path):
                        os.remove(path)
                except OSError:
                    pass

    def run(self):
        import requests
        try:
            if os.path.exists(self.dest) and self._sha256(self.dest).lower() == self.sha256:
                self._finish()
                return None
            self._total, ranged = self._probe(requests)
            if ranged and self._total:
                self._download_ranged(requests)
            else:
                self._download_single(requests)
            if self._sha256(self.dest).lower() != self.sha256:
                self._cleanup(remove_dest=True)
                self.failed.emit('安装包校验失败(哈希不一致),文件已丢弃')
                return None
            self._finish()
        except Exception as e:
            self.failed.emit('%s: %s' % (type(e).__name__, e))

    def _finish(self):
        if not verify_release_signature(self.sha256, self.sig):
            self._cleanup(remove_dest=True)
            self.failed.emit('安装包发布者签名无效,文件已丢弃\n请稍后重试或联系管理员')
            return None
        self.progress.emit(100)
        self.finished_ok.emit(self.dest)


class CloudWorker(QThread):
    '''后台线程:从云端拉取账号列表'''
    accounts_loaded = Signal(list)
    error_occurred = Signal(str)
    license_blocked = Signal(str)

    def __init__(self, client: CloudClient):
        super().__init__()
        self.client = client

    def run(self):
        blocked, detail = cloud_handshake(self.client)
        if blocked:
            self.license_blocked.emit(detail)
            return None
        try:
            accounts = self.client.list_accounts()
            self.accounts_loaded.emit(accounts)
        except CloudClientError as e:
            self.error_occurred.emit(str(e))
        except Exception as e:
            self.error_occurred.emit('%s: %s' % (type(e).__name__, e))


class UpsertWorker(QThread):
    '''后台线程:把执行结果回传云端(不阻塞 UI)'''
    finished_ok = Signal(int, str)
    failed = Signal(int, str)

    MAX_ATTEMPTS = 3

    def __init__(self, client: CloudClient, account_id: int, payload: dict, desc: str, followup=None, followup_desc: str = ''):
        super().__init__()
        self.client = client
        self.account_id = account_id
        self.payload = payload
        self.desc = desc
        self.followup = followup
        self.followup_desc = followup_desc

    def _send_followup(self):
        if not self.followup:
            return None
        for attempt in range(1, self.MAX_ATTEMPTS + 1):
            try:
                self.client.upsert_account(self.account_id, **self.followup)
                self.followup = None
                return None
            except Exception:
                if attempt < self.MAX_ATTEMPTS:
                    time.sleep(1.5 * attempt)

    def run(self):
        last_err = ''
        try:
            for attempt in range(1, self.MAX_ATTEMPTS + 1):
                try:
                    self.client.upsert_account(self.account_id, **self.payload)
                except Exception as e:
                    last_err = f'{type(e).__name__}: {e}'
                    if attempt < self.MAX_ATTEMPTS:
                        time.sleep(1.5 * attempt)
                else:
                    if attempt > 1:
                        self.finished_ok.emit(self.account_id, f'{self.desc}(第{attempt}次重试成功)')
                    else:
                        self.finished_ok.emit(self.account_id, self.desc)
                    self._send_followup()
                    return
            self.failed.emit(self.account_id,
                             f'{last_err}(已重试{self.MAX_ATTEMPTS}次仍失败,本地状态已保留,下次刷新继续补传)')
            self._send_followup()
        except Exception:
            self._send_followup()
            raise


class RestoreWorker(QThread):
    '''后台线程:批量把账号状态回传云端(默认=手动恢复为"待执行")'''
    progress = Signal(int, int)
    one_ok = Signal(int)
    one_failed = Signal(int, str)
    all_done = Signal(int, int)

    MAX_ATTEMPTS = 3

    def __init__(self, client: CloudClient, account_ids: list, status: str = 'READY', stage: str = '手动恢复', message: str = '已手动重置为待执行', remark: str = '手动恢复,等待执行'):
        super().__init__()
        self.client = client
        self.account_ids = account_ids
        self.status = status
        self.stage = stage
        self.message = message
        self.remark = remark

    def _payload(self, item):
        if isinstance(item, (tuple, list)) and len(item) == 2:
            aid, extra = item
            payload = {'status': self.status, 'stage': self.stage, 'message': self.message, 'remark': self.remark}
            payload.update(dict(extra or {}))
            return (aid, payload)
        return (item, {'status': self.status, 'stage': self.stage, 'message': self.message, 'remark': self.remark})

    def run(self):
        ok_n = fail_n = 0
        total = len(self.account_ids)
        now_str = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        for i, item in enumerate(self.account_ids, 1):
            aid, payload = self._payload(item)
            last_err = ''
            for attempt in range(1, self.MAX_ATTEMPTS + 1):
                try:
                    body = dict(payload or {})
                    body.setdefault('status_time', now_str)
                    self.client.upsert_account(aid, **body)
                    ok_n += 1
                    self.one_ok.emit(aid)
                    break
                except Exception as e:
                    last_err = '%s: %s' % (type(e).__name__, e)
                    if attempt < self.MAX_ATTEMPTS:
                        time.sleep(1.5 * attempt)
            else:
                fail_n += 1
                self.one_failed.emit(aid, last_err)
            self.progress.emit(i, total)
        self.all_done.emit(ok_n, fail_n)


class RunnerWorkerThread(QThread):
    '''后台线程:运行游戏自动化子进程'''
    log_received = Signal(str, str)
    status_received = Signal(str, str, str, str, int)
    account_data_received = Signal(int, str, str)
    facility_received = Signal(str, dict)
    banned_received = Signal(int, str, str)
    observation_received = Signal(str, int, str)
    done_received = Signal(bool)
    error_occurred = Signal(str)
    heartbeat_received = Signal(float)

    def __init__(self, accounts: list, server_url: str, token: str, trade_price_adjust: int = 4, login_only: bool = False, extra_cfg: dict = None, force_gdi: bool = False):
        super().__init__()
        self.accounts = accounts
        self.server_url = server_url
        self.token = token
        self.trade_price_adjust = trade_price_adjust
        self.login_only = login_only
        self.extra_cfg = extra_cfg or {}
        self.force_gdi = force_gdi
        self.proc = None
        self._got_done = False
        self.stopped_by_request = False
        self.exit_code = None
        self._stop_flag = os.path.join(tempfile.gettempdir(), 'pc_tqc_client_stop.flag')

    def run(self):
        _cleanup_stale_runner_cfg()
        first = self.accounts[0] if self.accounts else {}
        cfg = {
            'account': first.get('account', ''),
            'password': first.get('password', ''),
            'platform': first.get('platform', 'Steam'),
            'account_id': first.get('id'),
            'accounts_list': self.accounts,
            'trade_price_adjust': self.trade_price_adjust,
            'server_url': self.server_url,
            'token': self.token,
            'login_only': self.login_only,
        }
        if self.extra_cfg:
            cfg.update(self.extra_cfg)
        try:
            try:
                if os.path.exists(self._stop_flag):
                    os.remove(self._stop_flag)
            except Exception:
                pass
            if getattr(sys, 'frozen', False):
                here = os.path.dirname(sys.executable)
            else:
                here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            extracted_dir = None
            for _cand in (
                os.path.join(here, '_internal', 'engine'),
                os.path.join(here, 'PC特勤处V1.0.66.exe_extracted'),
                os.path.join(os.path.dirname(here), '_internal', 'engine'),
                os.path.join(os.path.dirname(here), 'PC特勤处V1.0.66.exe_extracted'),
            ):
                if os.path.isdir(_cand):
                    extracted_dir = _cand
                    break
            if not extracted_dir:
                self.error_occurred.emit(f'找不到引擎目录: {os.path.join(here, "_internal", "engine")}(旧解包目录 junction 也不存在)')
                return None
            if getattr(sys, 'frozen', False):
                cmd = [sys.executable, '--stdin-config']
            else:
                main_py = os.path.join(here, 'client', 'main.py')
                if not os.path.exists(main_py):
                    main_py = os.path.join(os.path.dirname(here), 'client', 'main.py')
                cmd = [sys.executable, main_py, '--stdin-config']
            env = os.environ.copy()
            env['PC_TQC_WORKER'] = '1'
            env['PYTHONIOENCODING'] = 'utf-8'
            env['PYTHONUTF8'] = '1'
            if self.force_gdi:
                env['PC_TQC_FORCE_GDI'] = '1'
            self.proc = subprocess.Popen(
                cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                cwd=here, env=env,
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0,
                encoding='utf-8', errors='replace',
            )
            try:
                self.proc.stdin.write(json.dumps(cfg, ensure_ascii=False))
                self.proc.stdin.close()
            except Exception as e:
                try:
                    self.proc.kill()
                except Exception:
                    pass
                self.error_occurred.emit(f'向 worker 传递配置失败: {e}')
                return None
            for line in self.proc.stdout:
                line = line.strip()
                if not line:
                    continue
                try:
                    obj = json.loads(line)
                except json.JSONDecodeError:
                    self.log_received.emit(line, 'INFO')
                    continue
                t = obj.get('type')
                if t == 'log':
                    self.log_received.emit(obj.get('text', ''), obj.get('level', 'INFO'))
                elif t == 'status':
                    aid = obj.get('account_id')
                    self.status_received.emit(
                        obj.get('account', ''), obj.get('status', ''), obj.get('stage', ''),
                        obj.get('message', ''), int(aid) if aid else 0,
                    )
                elif t == 'account_data':
                    _lvl = obj.get('level')
                    _coins = obj.get('hafu_coins')
                    self.account_data_received.emit(
                        int(obj.get('account_id') or 0),
                        str(_lvl) if _lvl is not None else '',
                        str(_coins) if _coins is not None else '',
                    )
                elif t == 'banned':
                    self.banned_received.emit(
                        int(obj.get('account_id') or 0),
                        str(obj.get('unban_at', '')),
                        str(obj.get('source_text', '')),
                    )
                elif t == 'facility':
                    self.facility_received.emit(obj.get('account', ''), obj.get('timers', {}))
                elif t == 'observation':
                    self.observation_received.emit(
                        str(obj.get('account', '')),
                        int(obj.get('account_id') or 0),
                        str(obj.get('until', '')),
                    )
                elif t == 'done':
                    self._got_done = True
                    self.done_received.emit(obj.get('success', False))
                elif t == 'heartbeat':
                    self.heartbeat_received.emit(float(obj.get('ts') or time.time()))
            if not self._got_done and self.proc is not None:
                code = None
                try:
                    code = self.proc.wait(timeout=6)
                except Exception:
                    code = self.proc.poll()
                self.exit_code = code
                if self.stopped_by_request:
                    self.error_occurred.emit(f'运行进程已按停止请求退出(退出码 {code})')
                    return None
                self.error_occurred.emit(f'运行进程异常退出(退出码 {code},未收到完成信号)')
            return None
        except Exception as e:
            self.error_occurred.emit(f'{type(e).__name__}: {e}')

    def stop(self):
        '''发送停止信号:先让引擎走自己的清理流程(文件标志),8秒未退出才强杀。'''
        self.stopped_by_request = True
        try:
            with open(self._stop_flag, 'w') as f:
                f.write('stop')
        except Exception:
            pass

        def _graceful_kill():
            proc = self.proc
            if proc is None:
                return None
            for _ in range(16):
                if proc.poll() is not None:
                    return None
                time.sleep(0.5)
            if proc.poll() is None:
                try:
                    proc.terminate()
                except Exception:
                    pass

        threading.Thread(target=_graceful_kill, daemon=True).start()
class MainWindow(QMainWindow):
    """执行端主窗口"""
    IP刷新间隔秒 = 18000
    IP失败重试秒 = 600

    def __init__(self):
        super().__init__()
        self.setWindowTitle('PC特勤处 - 执行端 V1.9')
        self.setMinimumSize(900, 650)
        self.config = load_config()
        self.config.server_url = self.config.server_url or CLOUD_URL
        self.cloud_client = None
        self.accounts = []
        self.worker = None
        self.is_running = False
        self.runner_thread = None
        _cleanup_stale_runner_cfg(0)
        self._last_account_ids = {}
        self._license_blocked = False
        _man = _load_manual_state()
        self._paused = set(_man.get('paused', []))
        self._ban_paused = set(_man.get('ban_paused', []))
        self._locked = set(_man.get('locked', []))
        self._pwd_salt = bytes.fromhex(_man['pwd_salt']) if _man.get('pwd_salt') else None
        self._pwd_hash = _man.get('pwd_hash', '')
        self._manual_ok = set()
        self._reset_ready = set()
        self._run_times = {}
        for _n, _ts in (_man.get('run_times') or {}).items():
            if not isinstance(_ts, list):
                continue
            else:
                self._run_times[_n] = [float(t) for t in _ts if isinstance(t, (int, float))]
        self._fail_times = {}
        for _n, _ts in (_man.get('fail_times') or {}).items():
            if not isinstance(_ts, list):
                continue
            else:
                self._fail_times[_n] = [float(t) for t in _ts if isinstance(t, (int, float))]
        _stale_ban = self._ban_paused & self._paused
        if _stale_ban:
            self._paused -= _stale_ban
            self._save_manual()
        self._login_only_active = False
        self._terminal_upserted = {}
        self._pause_after_terminal = {}
        self._pause_reconciled = set()
        self._filter_mode = None
        self._view_rows = []
        self._bg_pixmap = QPixmap(_bg_image_path())
        self._init_ui()
        self._apply_style()
        self._start_public_ip_fetch()
        self._start_update_check()
        self._driver_ok = True
        self._check_driver()
        self.refresh_timer = QTimer(self)
        self.refresh_timer.timeout.connect(self._refresh_accounts)
        self.tick_timer = QTimer(self)
        self.tick_timer.timeout.connect(self._tick_countdowns)
        self.tick_timer.start(1000)
        self.watchdog_timer = QTimer(self)
        self.watchdog_timer.timeout.connect(self._watchdog_check)
        self._last_heartbeat = 0.0
        self._heartbeat_asked = False
        self._worker_lost_handled = False
        self._user_stopped = False
        self._auto_launch_times = []
        self._auto_blocked = False
        self._refresh_conflict_sig = None
        self._missed_active = set()
        self._missed_upsert_ts = {}
        self._waiting_upserted = {}
        self._auto_restart_pending = False
        self._current_run_ids = set()
        self._crash_round = 0
        self._crash_recovered_round = (-1)
        self._crash_running_ids = set()
        self._crash_times_by_acc = {}
        self._last_crash_at = 0.0
        self._crash_streak = 0
        self._force_gdi = False
        if self.config.is_valid():
            self._connect_cloud()
        else:
            self._show_config_dialog()

    def _check_driver(self):
        """启动时检测 Interception 键盘驱动。

        已装=放行;未装=锁死"开始"按钮,弹窗询问是否立即安装(点 YES 走
        ShellExecuteW runas 提权,装完需重启)。若失败原因是接口库/DLL 缺失,
        则提示安装包不完整,不再误导用户重复安装驱动。
        """
        try:
            if getattr(sys, 'frozen', False):
                drv_dir = os.path.dirname(sys.executable)
            else:
                drv_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            if drv_dir not in sys.path:
                sys.path.insert(0, drv_dir)
            import driver_check
        except Exception as e:
            self._log(f'驱动检测模块加载失败: {type(e).__name__}: {e}', 'WARN')
            return None
        try:
            ok, msg = driver_check.is_installed()
        except Exception as e:
            self._log(f'驱动检测异常: {type(e).__name__}: {e}', 'WARN')
            return None
        if ok:
            self._driver_ok = True
            self._log(f'键盘驱动: {msg}', 'INFO')
            self._refresh_start_btn()
            return None
        self._driver_ok = False
        self._refresh_start_btn()
        if not msg.startswith('驱动未安装'):
            self._log(f'键盘驱动不可用: {msg}', 'ERROR')
            QMessageBox.warning(self, '驱动组件异常',
                                f'{msg}\n\n这不是驱动未安装，通常是安装包内缺少接口库或被安全软件拦截。\n请重新下载完整安装包后再试；若仍不行请联系管理员。')
            return None
        self._log(f'键盘驱动未安装: {msg}', 'WARN')
        try:
            reply = QMessageBox.question(self, '键盘驱动未安装',
                                         f'{msg}\n\n安装包已内置在执行端里,点「立即安装」会自动弹出管理员授权并安装驱动，安装完成后需要重启电脑。\n\n是否现在安装？',
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if reply == QMessageBox.Yes:
                self._install_driver()
                return None
            return None
        except Exception:
            return None

    def _install_driver(self):
        """一键安装 Interception 键盘驱动(弹 UAC,装完提示重启)。"""
        try:
            if getattr(sys, 'frozen', False):
                drv_dir = os.path.dirname(sys.executable)
            else:
                drv_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            if drv_dir not in sys.path:
                sys.path.insert(0, drv_dir)
            import driver_check
        except Exception as e:
            QMessageBox.critical(self, '驱动安装', f'驱动模块加载失败: {e}')
            return None
        ok, msg = driver_check.is_installed()
        if ok:
            self._driver_ok = True
            self._log(f'键盘驱动: {msg}', 'INFO')
            self._refresh_start_btn()
            return None
        self._log('开始安装键盘驱动(需要管理员授权)...', 'WARN')
        ok, imsg = driver_check.install_elevated()
        if not ok:
            self._log(f'键盘驱动安装失败: {imsg}', 'ERROR')
            QMessageBox.critical(self, '驱动安装失败', f'{imsg}\n\n可以关闭此窗口后重试；若一直失败，请右键执行端选择「以管理员身份运行」后再点一次安装。')
            return None
        self._log(f'键盘驱动安装: {imsg}', 'INFO')
        reply = QMessageBox.question(self, '驱动已安装', f'{imsg}\n\n驱动必须重启电脑后才会生效。是否现在重启？',
                                     QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
        if reply == QMessageBox.Yes:
            try:
                os.system('shutdown /r /t 5 /c "键盘驱动已安装,5秒后重启"')
            except Exception as e:
                QMessageBox.warning(self, '重启失败', f'请手动重启电脑: {e}')

    def _start_public_ip_fetch(self):
        """启动后台线程循环查公网IP+归属地;主线程用500ms定时器轮询结果并更新标签。"""
        self._public_ip_result = None
        self._public_ip_shown = None
        threading.Thread(target=self._fetch_public_ip_job, daemon=True).start()
        self._ip_poll_timer = QTimer(self)
        self._ip_poll_timer.timeout.connect(self._poll_public_ip)
        self._ip_poll_timer.start(500)

    def _fetch_public_ip_once(self):
        import re as _re
        ipv4 = r'(\d{1,3}(?:\.\d{1,3}){3})'
        try:
            import requests
            text = (requests.get('https://myip.ipip.net', timeout=3).text or '').strip()
            m = _re.search(ipv4, text)
            if m:
                loc = ''
                lm = _re.search('来自于[:：]\s*(.+)', text)
                if lm:
                    parts = [p for p in _re.split(r'\s+', lm.group(1).strip()) if p and p != '中国']
                    loc = ' '.join(parts)
                return (m.group(1), loc)
        except Exception:
            pass
        try:
            import requests
            j = requests.get('http://ip-api.com/json/?lang=zh-CN&fields=status,query,regionName,city,isp', timeout=3).json()
            q = str(j.get('query', ''))
            if j.get('status') == 'success' and _re.match(r'^\d{1,3}(?:\.\d{1,3}){3}$', q):
                loc = ' '.join(p for p in (str(j.get('regionName', '')).strip(),
                                           str(j.get('city', '')).strip(),
                                           str(j.get('isp', '')).strip()) if p)
                return (q, loc)
        except Exception:
            pass
        for url in ('https://api.ipify.org', 'https://v4.ident.me'):
            try:
                import requests
                cand = (requests.get(url, timeout=3).text or '').strip()
                if _re.match(r'^\d{1,3}(?:\.\d{1,3}){3}$', cand):
                    return (cand, '')
            except Exception:
                pass
        return ('', '')

    def _fetch_public_ip_job(self):
        """循环:查一次 → 成功睡5小时 / 失败睡10分钟重试;结果交主线程定时器回显。"""
        import time as _t
        while True:
            try:
                ip, loc = self._fetch_public_ip_once()
                if ip:
                    self._public_ip_result = (ip, loc)
                    _t.sleep(self.IP刷新间隔秒)
                else:
                    self._public_ip_result = ('获取失败', '')
                    _t.sleep(self.IP失败重试秒)
            except Exception:
                _t.sleep(self.IP失败重试秒)

    def _poll_public_ip(self):
        res = getattr(self, '_public_ip_result', None)
        if not res or res == getattr(self, '_public_ip_shown', None):
            return None
        else:
            self._public_ip_shown = res
            ip, loc = res
            if loc and ip != '获取失败':
                self.ip_label.setText(f'本机IP: {ip} · {loc}')
            else:
                self.ip_label.setText(f'本机IP: {ip}')

    def _start_update_check(self):
        """启动即查一次,之后每 30 分钟查一次;版本号不一致则顶部提示。"""
        self._update_info = None
        self._update_shown = False
        threading.Thread(target=self._check_update_job, daemon=True).start()
        self._update_timer = QTimer(self)
        self._update_timer.timeout.connect(self._poll_update)
        self._update_timer.start(1000)

    def _check_update_job(self):
        sleep_s = 1800
        while True:
            try:
                srv = self.config.server_url or CLOUD_URL
                try:
                    import requests
                    j = requests.get(srv + '/api/public/version', timeout=5).json()
                except Exception:
                    from cloud_client import direct_get_json
                    j = direct_get_json(srv, '/api/public/version', timeout=5)
                ver = str((j or {}).get('version') or '').strip()
                if ver and ver != APP_VERSION:
                    self._update_info = j
                    return None
            except Exception:
                sleep_s = 300
            time.sleep(sleep_s)

    def _poll_update(self):
        info = getattr(self, '_update_info', None)
        if not info or self._update_shown:
            return None
        else:
            self._update_shown = True
            notes = str(info.get('notes') or '').strip()
            tip = f"发现新版本 {info.get('version')}（当前 {APP_VERSION}）"
            if notes:
                tip += f' · {notes}'
            self.update_label.setText(tip)
            self.update_label.show()
            self.update_btn.show()
            self._log(f"发现新版本 {info.get('version')}，点击顶部「立即更新」下载", 'INFO')

    def _fetch_update_info(self):
        """重新拉取版本信息:缓存的签名链接 10 分钟就过期,点更新时必须重新取一次。"""
        srv = self.config.server_url or CLOUD_URL
        try:
            import requests
            return requests.get(srv + '/api/public/version', timeout=5).json()
        except Exception:
            try:
                from cloud_client import direct_get_json
                return direct_get_json(srv, '/api/public/version', timeout=5)
            except Exception:
                return None

    def _on_update_clicked(self):
        srv = self.config.server_url or CLOUD_URL
        info = self._fetch_update_info() or getattr(self, '_update_info', None) or {}
        url = str(info.get('url') or '').strip()
        if not url:
            url = srv + '/static/download/PCTQC_V1.9.exe'
        elif url.startswith('/'):
            url = srv + url
        sha = str(info.get('sha256') or '').strip().lower()
        if len(sha) != 64:
            QMessageBox.warning(self, '无法校验更新包', '云端未提供安装包哈希,已停止自动更新。\n请稍后重试或联系管理员。')
            return None
        sig = str(info.get('sig') or '').strip().lower()
        if not sig:
            QMessageBox.warning(self, '无法校验更新包', '云端未提供安装包发布者签名,已停止自动更新。\n请稍后重试或联系管理员。')
            return None
        ver = str(info.get('version') or 'new').replace('/', '_').replace('\\', '_')
        dest = os.path.join(tempfile.gettempdir(), f'PCTQC_{ver}_{sha[:8]}.exe')
        dlg = QProgressDialog('正在下载更新包(约 176MB),下载完成后会自动解压并弹出安装界面。\n请耐心等待,不要重复点击或关闭窗口。', '', 0, 100, self)
        dlg.setWindowTitle('更新')
        dlg.setCancelButton(None)
        dlg.setAutoClose(False)
        dlg.setAutoReset(False)
        dlg.setMinimumDuration(0)
        dlg.show()

        def fresh_url():
            """链接中途过期时重新取一次签名地址。"""
            srv = self.config.server_url or CLOUD_URL
            item = self._fetch_update_info() or {}
            value = str(item.get('url') or '').strip()
            if value.startswith('/'):
                value = srv + value
            return value

        self._update_dl = UpdateDownloader(url, sha, dest, sig, url_refresh=fresh_url)
        self._update_dl.progress.connect(dlg.setValue)

        def ok(path):
            dlg.close()
            box = UpdateReadyDialog(self)
            box.exec()
            if box.clickedButton() is not box.yes_btn:
                return None
            try:
                subprocess.Popen('cmd.exe /c ping -n 5 127.0.0.1 >nul & start "" "%s"' % path,
                                 shell=True,
                                 creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
            except Exception as e:
                QMessageBox.warning(self, '启动失败', str(e))
            QTimer.singleShot(200, self.close)

        def fail(msg):
            dlg.close()
            QMessageBox.warning(self, '更新失败', msg)

        self._update_dl.finished_ok.connect(ok)
        self._update_dl.failed.connect(fail)
        self._update_dl.finished.connect(lambda: dlg.close())
        self._update_dl.start()

    def _refresh_start_btn(self):
        """按「驱动是否就绪 + 是否运行中 + 开始锁」决定\"开始\"按钮的可用状态与文案。"""
        if not hasattr(self, 'start_btn'):
            return
        else:
            if getattr(self, '_license_blocked', False):
                self.start_btn.setEnabled(False)
                self.start_btn.setText('云端校验失败' if getattr(self, '_block_is_verify', False) else '授权异常')
            else:
                if getattr(self, '_driver_ok', True) and (not getattr(self, 'is_running', False)):
                    self.start_btn.setEnabled(True)
                    if getattr(self, '_start_locked', False):
                        self.start_btn.setText('开始(锁定)')
                    else:
                        self.start_btn.setText('开始')
                else:
                    if not getattr(self, '_driver_ok', True):
                        self.start_btn.setEnabled(True)
                        self.start_btn.setText('请先安装键盘驱动')
                    else:
                        self.start_btn.setEnabled(False)
                        self.start_btn.setText('开始')
                if hasattr(self, '_start_lock_btn'):
                    self._start_lock_btn.setText('解锁开始' if getattr(self, '_start_locked', False) else '锁开始')

    def _init_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(8)
        main_layout.setContentsMargins(12, 12, 12, 12)
        top_bar = QHBoxLayout()
        self.server_label = QLabel('服务器: 未连接')
        self.server_label.setObjectName('serverInfo')
        top_bar.addWidget(self.server_label)
        self.ip_label = QLabel('本机IP: 获取中...')
        self.ip_label.setObjectName('serverInfo')
        top_bar.addWidget(self.ip_label)
        top_bar.addStretch()
        top_bar.addWidget(QLabel('交易行价格下调'))
        self.price_adjust_spin = QSpinBox()
        self.price_adjust_spin.setRange(0, 10)
        self.price_adjust_spin.setValue(self.config.trade_price_adjust)
        self.price_adjust_spin.setSuffix(' 次')
        self.price_adjust_spin.valueChanged.connect(self._on_price_adjust_changed)
        top_bar.addWidget(self.price_adjust_spin)
        self.config_btn = QPushButton('配置')
        self.config_btn.clicked.connect(self._show_config_dialog)
        top_bar.addWidget(self.config_btn)
        self.clean_log_btn = QPushButton('清理旧日志')
        self.clean_log_btn.setToolTip('清理运行日志目录下的历史日志文件(保留当天的日志)')
        self.clean_log_btn.clicked.connect(self._on_clean_old_logs)
        top_bar.addWidget(self.clean_log_btn)
        self.restore_btn = QPushButton('重置状态')
        self.restore_btn.setObjectName('restoreBtn')
        self.restore_btn.setToolTip('选中行:把选中账号重置为待执行(失败计数清零)\n未选中:重置全部失败/封禁/已停止账号\n不影响一键暂停/一键恢复,也不改调度开关')
        self.restore_btn.clicked.connect(self._on_restore)
        top_bar.addWidget(self.restore_btn)
        self.start_btn = QPushButton('开始')
        self.start_btn.setObjectName('startBtn')
        self.start_btn.clicked.connect(self._on_start)
        top_bar.addWidget(self.start_btn)
        self.stop_btn = QPushButton('停止')
        self.stop_btn.setObjectName('stopBtn')
        self.stop_btn.setEnabled(False)
        self.stop_btn.clicked.connect(self._on_stop)
        top_bar.addWidget(self.stop_btn)
        main_layout.addLayout(top_bar)
        self.update_bar = QHBoxLayout()
        self.update_label = QLabel('')
        self.update_label.setObjectName('updateTip')
        self.update_btn = QPushButton('立即更新')
        self.update_btn.setObjectName('updateBtn')
        self.update_btn.clicked.connect(self._on_update_clicked)
        self.update_bar.addWidget(self.update_label)
        self.update_bar.addWidget(self.update_btn)
        self.update_bar.addStretch()
        self.update_bar.addSpacing(8)
        self.ammo_chk = QCheckBox('子弹兑换')
        self.ammo_chk.setChecked(self.config.ammo_exchange_enabled)
        self.ammo_chk.setToolTip('关闭后军需处子弹兑换整段跳过')
        self.ammo_chk.stateChanged.connect(self._on_ammo_changed)
        self.update_bar.addWidget(self.ammo_chk)
        self.aw_chk = QCheckBox('研发兑换')
        self.aw_chk.setChecked(self.config.aw_enabled)
        self.aw_chk.setToolTip('关闭后研发部门 AW 兑换不发任何点击')
        self.aw_chk.stateChanged.connect(self._on_aw_changed)
        self.update_bar.addWidget(self.aw_chk)
        self.sell_window_chk = QCheckBox('出售时段')
        self.sell_window_chk.setChecked(self.config.sell_window_enabled)
        self.sell_window_chk.setToolTip('启用后仅在指定时段内执行交易行出售')
        self.sell_window_chk.stateChanged.connect(self._on_sell_window_changed)
        self.update_bar.addWidget(self.sell_window_chk)
        self.sell_start_edit = QTimeEdit()
        self.sell_start_edit.setDisplayFormat('HH:mm')
        self.sell_start_edit.setTime(QTime.fromString(self.config.sell_window_start, 'HH:mm') or QTime(14, 0))
        self.sell_start_edit.setToolTip('出售开始时间(本机时间)')
        self.sell_start_edit.timeChanged.connect(self._on_sell_time_changed)
        self.update_bar.addWidget(self.sell_start_edit)
        self.update_bar.addWidget(QLabel('-'))
        self.sell_end_edit = QTimeEdit()
        self.sell_end_edit.setDisplayFormat('HH:mm')
        self.sell_end_edit.setTime(QTime.fromString(self.config.sell_window_end, 'HH:mm') or QTime(20, 0))
        self.sell_end_edit.setToolTip('出售结束时间(本机时间,支持跨零点)')
        self.sell_end_edit.timeChanged.connect(self._on_sell_time_changed)
        self.update_bar.addWidget(self.sell_end_edit)
        main_layout.addLayout(self.update_bar)
        update_bar_label_style = 'color: #e8e8e8;'
        self.ammo_chk.setStyleSheet(update_bar_label_style)
        self.aw_chk.setStyleSheet(update_bar_label_style)
        self.sell_window_chk.setStyleSheet(update_bar_label_style)
        time_edit_style = 'QTimeEdit { color: #ffffff; background-color: #2a2d35; border: 1px solid #4a4f5c; border-radius: 3px; padding: 2px 6px; font-size: 13px; font-weight: bold; }QTimeEdit::up-button, QTimeEdit::down-button { width: 0px; }QTimeEdit::section { color: #ffffff; }'
        self.sell_start_edit.setStyleSheet(time_edit_style)
        self.sell_end_edit.setStyleSheet(time_edit_style)
        self.update_bar_label_hyphen = self.update_bar.itemAt(self.update_bar.count() - 2).widget()
        if self.update_bar_label_hyphen and isinstance(self.update_bar_label_hyphen, QLabel):
                self.update_bar_label_hyphen.setStyleSheet(update_bar_label_style)
        self.update_label.hide()
        self.update_btn.hide()
        stats_bar = QHBoxLayout()
        stats_bar.setSpacing(0)
        self.stat_total = self._create_stat_item('账号', '0', clickable=True)
        self.stat_ready = self._create_stat_item('待执行', '0')
        self.stat_waiting = self._create_stat_item('等待时间', '0')
        self.stat_failed = self._create_stat_item('失败', '0', clickable=True)
        self.stat_banned = self._create_stat_item('封禁', '0', clickable=True)
        for label, value in [self.stat_total, self.stat_ready, self.stat_waiting, self.stat_failed, self.stat_banned]:
            stats_bar.addWidget(label)
            stats_bar.addWidget(value)
        for value, mode in [(self.stat_total[1], None), (self.stat_failed[1], 'failed'), (self.stat_banned[1], 'banned')]:
            value.clicked.connect(lambda _=False, m=mode: self._set_filter(m))
            value.setToolTip('点击后主表格只显示对应账号;点\"账号\"恢复显示全部')
        stats_bar.addStretch()
        self.pause_all_btn = QPushButton('一键暂停')
        self.pause_all_btn.setObjectName('restoreBtn')
        self.pause_all_btn.setToolTip('一键暂停全部账号的调度任务(暂停后调度直接跳过;再点一次恢复)')
        self.pause_all_btn.clicked.connect(self._on_pause_all)
        stats_bar.addWidget(self.pause_all_btn)
        self.lock_all_btn = QPushButton('一键上锁')
        self.lock_all_btn.setToolTip('全部账号上登录验证锁:手动登录时需输入登录密码')
        self.lock_all_btn.clicked.connect(self._on_lock_all)
        stats_bar.addWidget(self.lock_all_btn)
        self.unlock_all_btn = QPushButton('一键解锁')
        self.unlock_all_btn.setToolTip('解除全部账号的登录验证锁(需输入登录密码)')
        self.unlock_all_btn.clicked.connect(self._on_unlock_all)
        stats_bar.addWidget(self.unlock_all_btn)
        self.pwd_set_btn = QPushButton('密码设置')
        self.pwd_set_btn.setToolTip('设置/修改登录密码(仅存哈希,用于锁定账号的手动登录与解锁)')
        self.pwd_set_btn.clicked.connect(self._on_set_password)
        stats_bar.addWidget(self.pwd_set_btn)
        main_layout.addLayout(stats_bar)
        splitter = QSplitter(Qt.Vertical)
        self.table = QTableWidget()
        self.table.setColumnCount(7)
        self.table.setHorizontalHeaderLabels(['#', '账号', '等级', '哈夫币', '状态', '日志', '操作'])
        self.table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(1, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(3, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(4, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(5, QHeaderView.Stretch)
        self.table.horizontalHeader().setSectionResizeMode(6, QHeaderView.ResizeToContents)
        self.table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.table.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.table.verticalHeader().setVisible(False)
        self.table.verticalHeader().setDefaultSectionSize(82)
        self.table.setContextMenuPolicy(Qt.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self._on_table_context_menu)
        splitter.addWidget(self.table)
        self.log_console = QTextEdit()
        self.log_console.setReadOnly(True)
        self.log_console.document().setMaximumBlockCount(1000)
        splitter.addWidget(self.log_console)
        splitter.setSizes([350, 250])
        main_layout.addWidget(splitter)
        self.statusBar().showMessage('就绪')

    def _create_stat_item(self, label_text: str, value_text: str, clickable: bool=False) -> tuple:
        """创建统计项标签对(clickable=True 时数值可点击,用于主表格过滤)"""
        label = QLabel(label_text)
        label.setObjectName('statLabel')
        if clickable:
            value = ClickableLabel(value_text)
        else:
            value = QLabel(value_text)
        value.setObjectName('statValue')
        return (label, value)

    def _apply_style(self):
        self.setStyleSheet(DARK_STYLE)

    def paintEvent(self, event):
        """绘制氛围背景:右侧对齐铺满窗口,再整体压暗,保证内容可读"""
        painter = QPainter(self)
        if not self._bg_pixmap.isNull():
            scaled = self._bg_pixmap.scaled(self.size(), Qt.KeepAspectRatioByExpanding, Qt.SmoothTransformation)
            painter.drawPixmap(self.width() - scaled.width(), 0, scaled)
            painter.fillRect(self.rect(), QColor(10, 13, 18, 155))
        else:
            painter.fillRect(self.rect(), QColor('#14181f'))
        super().paintEvent(event)

    def _show_config_dialog(self):
        dialog = ConfigDialog(self, self.config)
        if dialog.exec() != QDialog.Accepted:
            self._abort_startup('已关闭授权窗口,执行端退出')
            return
        else:
            self.config = dialog.get_config()
            if not str(self.config.token or '').strip():
                self._abort_startup('未填写访问令牌,执行端退出')
                return
            else:
                if save_config(self.config):
                    self._log('配置已保存', 'INFO')
                    self._connect_cloud()
                else:
                    self._log('配置保存失败', 'ERROR')

    def _abort_startup(self, reason):
        """授权配置未完成:记录原因后终止进程(main.py 不会 show 主窗口)。"""
        try:
            self._log(reason, 'WARN')
        except Exception:
            pass
        app = QApplication.instance()
        if app is not None:
            app.quit()
        raise SystemExit(0)

    def _connect_cloud(self):
        """连接到云端"""
        if not self.config.is_valid():
            self._log('配置无效,请先填写访问令牌', 'WARN')
            return
        else:
            self.cloud_client = CloudClient(self.config.server_url or CLOUD_URL, self.config.token, device_id=device_fingerprint())
            self.server_label.setText('服务器: 已连接')
            self._log('已连接服务器', 'INFO')
            self._refresh_accounts()
            self.refresh_timer.start(30000)

    def _refresh_accounts(self):
        """刷新账号列表"""
        if not self.cloud_client:
            return
        else:
            if self.worker and self.worker.isRunning():
                return
            else:
                self.worker = CloudWorker(self.cloud_client)
                self.worker.accounts_loaded.connect(self._on_accounts_loaded)
                self.worker.error_occurred.connect(self._on_cloud_error)
                self.worker.license_blocked.connect(self._on_license_blocked)
                self.worker.start()

    @Slot(list)
    def _on_accounts_loaded(self, accounts: list):
        """账号列表加载完成(30秒定时刷新/结束刷新)。\n\n        ★防状态跳动:云端回传可能失败或有延迟,刷新下来的旧状态绝不能把\n        本地刚更新的真实状态冲回去。\n          · 任何时候:BANNED/OBSERVATION(终态事实)本地优先;\n          · 任务运行期间:RUNNING/WAITING(等待时间)/SUCCESS 也本地优先,\n            避免\"已执行→等待时间→又变回待执行/等待\"的跳动;\n          · 未运行时其他状态以云端为准(允许后台改状态)。\n        """
        # ***<module>.MainWindow._on_accounts_loaded: Failure: Different control flow
        protected = {'BANNED', 'STOPPED', 'OBSERVATION'}
        if getattr(self, '_license_blocked', False):
            self._license_blocked = False
            self.server_label.setText('服务器: 已连接')
            self._log('授权已恢复,可继续执行', 'INFO')
            self._refresh_start_btn()
        if getattr(self, 'is_running', False):
            protected |= {'WAITING', 'SUCCESS', 'RUNNING'}
        cloud_status = {a.username: a.status or '' for a in accounts if a.username}
        local_facts = {}
        for acc in getattr(self, 'accounts', []) or []:
            if acc.status in protected:
                local_facts[acc.account_id] = (acc.status, acc.status_time)
        merged_ids = dict(_load_known_accounts())
        merged_ids.update(self._last_account_ids)
        cloud_usernames = {a.username for a in accounts if a.username}
        reimported = []
        for acc in accounts:
            old_id = merged_ids.get(acc.username)
            if old_id is not None and acc.account_id and (old_id != acc.account_id):
                        reimported.append(acc.username)
        deleted = [u for u in _load_local_record_usernames() if u not in cloud_usernames]
        try:
            if deleted:
                info = _purge_local_accounts(deleted)
                self._log(f"账号已从云端删除: {','.join(deleted)} → 本机记录已全部清除(调度{info['state']}条/挂售{info['market']}条/军需{info['ammo']}条/运行日志{info['lastrun']}条)", 'WARN')
            if reimported:
                info = _purge_local_accounts(reimported)
                self._log(f"检测到删号重导: {','.join(reimported)} → 旧记录已彻底清除(调度{info['state']}条/挂售{info['market']}条/军需{info['ammo']}条/运行日志{info['lastrun']}条),按新账号重新执行全流程", 'WARN')
        except Exception as e:
            self._log(f'清除账号本地记录失败: {e}', 'WARN')
        self._last_account_ids = {a.username: a.account_id for a in accounts}
        _save_known_accounts(self._last_account_ids)
        last_run = _load_last_run_store()
        for acc in accounts:
            info = last_run.get(acc.username)
            if info:
                acc.last_run_at = info.get('at', '')
                acc.last_run_stage = info.get('stage', '')
                acc.last_run_message = info.get('message', '')
            acc.fail_count = int((info or {}).get('fail_count', 0))
        db_timers = _load_all_facility_timers()
        prev_timers = {}
        if getattr(self, 'is_running', False):
            prev_timers = {a.account_id: dict(getattr(a, 'facility_next_run_at', None) or {}) for a in getattr(self, 'accounts', []) or []}
        reset_ready = getattr(self, '_reset_ready', set())
        for acc in accounts:
            merged = dict(getattr(acc, 'facility_next_run_at', None) or {})
            merged.update(db_timers.get(acc.username, {}))
            if acc.username not in reimported:
                merged.update(prev_timers.get(acc.account_id, {}))
            if acc.username in reset_ready:
                _now_iso = datetime.now().isoformat(sep=' ', timespec='seconds')
                merged = {k: _now_iso for k, _l in _FACILITY_LABELS}
            acc.facility_next_run_at = merged
        self.accounts = accounts
        if not hasattr(self, '_observed_ids'):
            self._observed_ids = set()
        if not hasattr(self, '_observed_this_run'):
            self._observed_this_run = set()
        for acc in self.accounts:
            if acc.status == 'OBSERVATION':
                self._observed_ids.add(acc.account_id)
                acc.status = 'SUCCESS'
                acc.status_time = ''
                acc.remark = ''
        conflicts = []
        for acc in self.accounts:
            if acc.account_id in local_facts:
                st, stt = local_facts[acc.account_id]
                if acc.status != st:
                    conflicts.append(f'{acc.username}(本地{STATUS_NAMES.get(st, st)}/云端{STATUS_NAMES.get(acc.status, acc.status)})')
                acc.status, acc.status_time = (st, stt)
        if getattr(self, '_crash_running_ids', None):
            for acc in self.accounts:
                if acc.status == 'RUNNING':
                    if acc.account_id in self._crash_running_ids:
                        acc.status = 'FAILED'
                        acc.remark = '执行进程异常退出,已自动重新排队'
                else:
                    self._crash_running_ids.discard(acc.account_id)
        if conflicts:
            sig = tuple(sorted(conflicts))
            if sig != getattr(self, '_refresh_conflict_sig', None):
                self._log(f"刷新保护: 状态不一致 → {', '.join(conflicts)},已保留本地值(云端回传成功后自动一致)", 'WARN')
            self._refresh_conflict_sig = sig
        else:
            self._refresh_conflict_sig = None
        if not getattr(self, 'is_running', False):
            missed = self._find_missed_accounts()
            if missed:
                self._reset_missed_to_ready(missed)
        self._sync_ban_pause(cloud_status)
        if self.cloud_client:
            for acc in self.accounts:
                if acc.username in getattr(self, '_paused', ()) and getattr(acc, 'paused', None) is False and (acc.username not in getattr(self, '_pause_reconciled', ())):
                            payload = self._pause_payload(acc)
                            if payload:
                                self._pause_reconciled.add(acc.username)
                                self._cloud_upsert(acc.account_id, payload, desc=f'{acc.username} 暂停对账(paused=true)')
        self._update_table()
        self._update_stats()
        if hasattr(self, 'pause_all_btn'):
            names = {a.username for a in self.accounts if a.username and a.username not in getattr(self, '_ban_paused', ())}
            self.pause_all_btn.setText('一键恢复' if names and names <= self._paused else '一键暂停')
        self.statusBar().showMessage(f'已加载 {len(accounts)} 个账号', 3000)
        self._auto_start_if_needed()

    @staticmethod
    def _license_detail(err: str):
        """从云端错误里识别授权类错误(401/403),返回提示文案;非授权错误返回 None。"""
        if 'HTTP 403' not in err and 'HTTP 401' not in err:
            return
        else:
            m = re.search('\"detail\"\\s*:\\s*\"([^\"]+)\"', err)
            return m.group(1) if m else '授权已失效,请联系管理员'

    def _apply_license_block(self, detail: str, stop_running: bool=True):
        """授权到期/冻结/注销,或云端验签失败:立即停止调度,锁住\"开始\"按钮。"""
        self._license_blocked = True
        self._block_is_verify = detail.startswith(USER_MESSAGE)
        if self._block_is_verify:
            self.server_label.setText('云端校验失败')
            self._log(f'云端校验失败,已停止执行: {detail}', 'ERROR')
        else:
            self.server_label.setText('授权异常: ' + detail[:24])
            self._log(f'授权异常,已停止执行: {detail}', 'ERROR')
        if stop_running and getattr(self, 'is_running', False):
                self._on_stop()
        self._refresh_start_btn()

    def _verify_license(self) -> bool:
        """点"开始"前主动校验一次;网络异常不拦(交给定时刷新判断)。"""
        if not self.cloud_client:
            QMessageBox.warning(self, '未连接', '尚未连接云端,请先填写令牌。')
            return False
        blocked, detail = cloud_handshake(self.cloud_client)
        if blocked:
            self._apply_license_block(detail, stop_running=False)
            _title = '云端校验失败' if detail.startswith(USER_MESSAGE) else '授权异常'
            QMessageBox.warning(self, _title, detail)
            return False
        try:
            self.cloud_client.list_accounts()
        except CloudVerificationError as e:
            self._apply_license_block(str(e), stop_running=False)
            QMessageBox.warning(self, '云端校验失败', str(e))
            return False
        except CloudClientError as e:
            detail = self._license_detail(str(e))
            if detail:
                self._apply_license_block(detail, stop_running=False)
                QMessageBox.warning(self, '授权异常', detail)
                return False
            return True
        except Exception:
            return True
        return True

    @Slot(str)
    def _on_license_blocked(self, detail: str):
        """握手/设备校验失败,或云端验签失败:立即锁住调度。"""
        if getattr(self, '_license_blocked', False):
            return
        else:
            self._apply_license_block(detail, stop_running=False)
            _title = '云端校验失败' if detail.startswith(USER_MESSAGE) else '授权异常'
            QMessageBox.warning(self, _title, detail)

    @Slot(str)
    def _on_cloud_error(self, error: str):
        """云端通信错误"""
        if error.startswith(USER_MESSAGE):
            if not getattr(self, '_license_blocked', False):
                self._apply_license_block(error)
                QMessageBox.warning(self, '云端校验失败', error)
            return None
        else:
            detail = self._license_detail(error)
            if detail:
                if not getattr(self, '_license_blocked', False):
                    self._log(f'授权校验失败: {error}', 'ERROR')
                    self._apply_license_block(detail)
                    QMessageBox.warning(self, '授权异常', detail)
                return None
            else:
                self._log(f'云端通信错误: {error}', 'ERROR')
                self.server_label.setText('服务器: 连接失败')

    def _update_table(self):
        """更新账号表格(V1.9:支持 待执行/等待/失败/封禁/暂停 过滤,行内容=_view_rows)"""
        # ***<module>.MainWindow._update_table: Failure: Different control flow
        fm = self._filter_mode
        if fm == 'failed':
            rows = [a for a in self.accounts if self._display_status(a)[0] == '失败']
        else:
            if fm == 'banned':
                rows = [a for a in self.accounts if self._display_status(a)[0] == '封禁']
            else:
                if fm == 'ready':
                    rows = [a for a in self.accounts if self._display_status(a)[0] == '待执行']
                else:
                    if fm == 'waiting':
                        rows = [a for a in self.accounts if self._display_status(a)[0] == '等待时间']
                    else:
                        if fm == 'paused':
                            paused_set = getattr(self, '_paused', ())
                            ban_set = getattr(self, '_ban_paused', ())
                            rows = [a for a in self.accounts if a.username in paused_set and a.username not in ban_set and (getattr(a, 'status', '') != 'STOPPED')]
                        else:
                            rows = list(self.accounts)
        self._view_rows = rows
        self.table.setRowCount(len(rows))
        for row, acc in enumerate(rows):
            num_item = QTableWidgetItem(str(row + 1))
            num_item.setTextAlignment(Qt.AlignCenter)
            self.table.setItem(row, 0, num_item)
            user_item = QTableWidgetItem(acc.username or '-')
            self.table.setItem(row, 1, user_item)
            level_item = QTableWidgetItem(acc.level or '-')
            level_item.setTextAlignment(Qt.AlignCenter)
            self.table.setItem(row, 2, level_item)
            coins_text = _format_hafu_w(acc.haf_coins)
            coins_item = QTableWidgetItem(coins_text)
            coins_item.setTextAlignment(Qt.AlignCenter)
            if coins_text != (acc.haf_coins or '-'):
                coins_item.setToolTip(f'哈夫币原值: {acc.haf_coins}')
            self.table.setItem(row, 3, coins_item)
            status_name, color_key = self._display_status(acc)
            status_item = QTableWidgetItem(status_name)
            status_item.setTextAlignment(Qt.AlignCenter)
            color = STATUS_COLORS.get(color_key, '#e0e0e0')
            status_item.setForeground(QColor(color))
            self.table.setItem(row, 4, status_item)
            log_text = ''
            if acc.status == 'BANNED':
                log_text = f'封禁,解封时间 {acc.status_time}' if acc.status_time else '账号封禁中'
            else:
                if acc.status == 'STOPPED':
                    detail = acc.last_run_stage or ''
                    if acc.last_run_message:
                        detail = f'{detail}: {acc.last_run_message}' if detail else acc.last_run_message
                    log_text = detail or acc.remark or '连续失败3次,待人工处理'
                else:
                    log_text = _facility_countdown_text(getattr(acc, 'facility_next_run_at', None))
                    if not log_text:
                        detail = acc.last_run_stage or ''
                        if acc.last_run_message:
                            detail = f'{detail}: {acc.last_run_message}' if detail else acc.last_run_message
                        log_text = detail or acc.remark or acc.status_time or '-'
            log_item = QTableWidgetItem(log_text)
            self.table.setItem(row, 5, log_item)
            self.table.setCellWidget(row, 6, self._build_row_controls(acc))

    def _display_status(self, acc) -> tuple:
        """表格状态显示(V1.8):封禁 > 失败(连败3次/验证码) > 手动暂停 > 手动已登录 > 常规推导"""
        if acc.username in getattr(self, '_ban_paused', ()) or getattr(acc, 'status', '') == 'BANNED':
            return ('封禁', 'BANNED')
        else:
            if getattr(acc, 'status', '') == 'STOPPED':
                return ('失败', 'FAILED')
            else:
                if acc.username in getattr(self, '_paused', ()):
                    return ('已暂停', 'PAUSED')
                else:
                    if acc.username in getattr(self, '_manual_ok', ()):
                        return ('已登录', 'SUCCESS')
                    else:
                        return _derive_display_status(acc)

    def _build_row_controls(self, acc) -> QWidget:
        """操作列控件:暂停(调度跳过) / 登录(只跑第一步) / 登录验证(账号锁)"""
        w = QWidget()
        lay = QHBoxLayout(w)
        lay.setContentsMargins(4, 0, 4, 0)
        lay.setSpacing(6)
        paused = acc.username in self._paused
        banned = acc.username in getattr(self, '_ban_paused', ()) or getattr(acc, 'status', '') in ('BANNED', 'STOPPED')
        if banned:
            unban_btn = QPushButton('解封')
            unban_btn.setObjectName('cellBtn')
            unban_btn.setMinimumWidth(58)
            unban_btn.setStyleSheet('color: #b0533c; font-weight: bold;')
            unban_btn.setToolTip('云端重置该账号状态为待执行,解除封禁/停用后重新参与调度')
            unban_btn.setEnabled(not self.is_running)
            unban_btn.clicked.connect(lambda _=False, a=acc: self._on_unban_row(a))
            lay.addWidget(unban_btn)
        else:
            pause_btn = QPushButton('恢复' if paused else '暂停')
            pause_btn.setObjectName('cellBtn')
            pause_btn.setMinimumWidth(58)
            if paused:
                pause_btn.setStyleSheet('color: #c98f2d; font-weight: bold;')
            pause_btn.setToolTip('暂停后该账号不参与调度任务(不影响手动登录)')
            pause_btn.clicked.connect(lambda _=False, a=acc: self._on_toggle_pause(a))
            lay.addWidget(pause_btn)
        locked = acc.username in self._locked
        login_btn = QPushButton('登录')
        login_btn.setObjectName('cellBtn')
        login_btn.setMinimumWidth(58)
        login_btn.setEnabled(not locked)
        login_btn.setToolTip('只执行第一步(登录平台并进入游戏),后续步骤不运行' + (';该账号已上锁,需先点「解锁」验证密码后才能登录' if locked else ''))
        login_btn.clicked.connect(lambda _=False, a=acc: self._on_manual_login(a))
        lay.addWidget(login_btn)
        lock_btn = QPushButton('解锁' if locked else '已解锁')
        lock_btn.setObjectName('cellBtn')
        lock_btn.setMinimumWidth(58)
        if locked:
            lock_btn.setStyleSheet('color: #b0533c; font-weight: bold;')
        lock_btn.setToolTip('登录验证锁:锁定后手动登录该账号需输入登录密码')
        lock_btn.clicked.connect(lambda _=False, a=acc: self._on_toggle_lock(a))
        lay.addWidget(lock_btn)
        return w

    def _tick_countdowns(self):
        """每秒一次:原地更新日志列倒计时 + 状态列跟随倒计时跳变。\n\n        状态只认4设施倒计时:最晚台子归零瞬间\"等待时间\"→\"待执行\";\n        不重建表格,不影响选中状态;状态有跳变时顺带刷新统计条。"""
        if not self.accounts:
            return
        else:
            acc_by_name = {acc.username: acc for acc in self.accounts}
            stats_dirty = False
            for row in range(self.table.rowCount()):
                name_item = self.table.item(row, 1)
                if not name_item:
                    continue
                else:
                    acc = acc_by_name.get(name_item.text())
                    if not acc:
                        continue
                    else:
                        if acc.status in ['BANNED', 'STOPPED']:
                            continue
                        else:
                            disp_name, color_key = self._display_status(acc)
                            status_item = self.table.item(row, 4)
                            if status_item is not None and status_item.text() != disp_name:
                                    status_item.setText(disp_name)
                                    status_item.setForeground(QColor(STATUS_COLORS.get(color_key, '#e0e0e0')))
                                    stats_dirty = True
                            item = self.table.item(row, 5)
                            if item is not None:
                                text = _facility_countdown_text(getattr(acc, 'facility_next_run_at', None))
                                if text and item.text() != text and ('\n' in item.text()):
                                            item.setText(text)
            if stats_dirty:
                self._update_stats()

    def _update_stats(self):
        """更新统计条(口径=状态列显示,只认4设施倒计时推导;V1.8 暂停/已登录不进分类计数)"""
        try:
            if not getattr(self, '_start_locked', False):
                self._start_locked = bool(_load_manual_state().get('start_locked', False))
        except Exception:
            self._start_locked = False
        if not hasattr(self, 'stat_paused'):
            self._inject_paused_stat_and_lock_btn()
        total = len(self.accounts)
        ready = waiting = failed = banned = 0
        for a in self.accounts:
            disp, _ = self._display_status(a)
            if disp == '待执行':
                ready += 1
            else:
                if disp == '等待时间':
                    waiting += 1
                else:
                    if disp == '封禁':
                        banned += 1
                    else:
                        if disp == '失败':
                            failed += 1
        names = {a.username for a in self.accounts}
        paused = len(getattr(self, '_paused', set()) - getattr(self, '_ban_paused', set()) & names)
        self.stat_total[1].setText(str(total))
        self.stat_ready[1].setText(str(ready))
        self.stat_waiting[1].setText(str(waiting))
        self.stat_failed[1].setText(str(failed))
        self.stat_banned[1].setText(str(banned))
        if hasattr(self, 'stat_paused'):
            self.stat_paused[1].setText(str(paused))

    def _save_manual(self):
        """持久化手动操作状态(暂停名单/锁定名单/密码哈希/近期运行时间/开始锁)"""
        _save_manual_state({'paused': sorted(self._paused), 'ban_paused': sorted(self._ban_paused), 'locked': sorted(self._locked), 'pwd_salt': self._pwd_salt.hex() if self._pwd_salt else '', 'pwd_hash': self._pwd_hash, 'run_times': {k: v for k, v in self._run_times.items() if v}, 'fail_times': {k: v for k, v in self._fail_times.items() if v}, 'start_locked': bool(getattr(self, '_start_locked', False))})

    def _prune_run_times(self, now=None):
        """丢弃窗口外的运行时间戳,只留最近 RUN_LIMIT_WINDOW 秒内的记录。"""
        # ***<module>.MainWindow._prune_run_times: Failure: Different control flow
        now = now or time.time()
        for name in list(self._run_times.keys()):
            kept = [t for t in self._run_times.get(name) or [] if now - float(t) < RUN_LIMIT_WINDOW]
            if kept:
                self._run_times[name] = kept
            else:
                self._run_times.pop(name, None)

    def _recent_run_count(self, uname, now=None) -> int:
        """该账号在最近 RUN_LIMIT_WINDOW 秒内的运行次数。"""
        # ***<module>.MainWindow._recent_run_count: Failure: Different control flow
        now = now or time.time()
        return len([t for t in self._run_times.get(uname) or [] if now - float(t) < RUN_LIMIT_WINDOW])

    def _record_run(self, uname, now=None):
        """记录一次实际派发(运行)的时间戳。"""
        if not uname:
            return
        else:
            self._run_times.setdefault(uname, []).append(now or time.time())

    def _clear_run_times(self, uname):
        """清零该账号的运行时间记录(人工恢复/暂停解除时调用)。"""
        if uname and uname in self._run_times:
                self._run_times.pop(uname, None)

    def _prune_fail_times(self, now=None):
        """丢弃 1 小时外的失败时间戳。"""
        # ***<module>.MainWindow._prune_fail_times: Failure: Different control flow
        now = now or time.time()
        for name in list(self._fail_times.keys()):
            kept = [t for t in self._fail_times.get(name) or [] if now - float(t) < RUN_LIMIT_WINDOW]
            if kept:
                self._fail_times[name] = kept
            else:
                self._fail_times.pop(name, None)

    def _recent_fail_count(self, uname, now=None) -> int:
        """该账号在最近 1 小时内的失败次数。"""
        # ***<module>.MainWindow._recent_fail_count: Failure: Different control flow
        now = now or time.time()
        return len([t for t in self._fail_times.get(uname) or [] if now - float(t) < RUN_LIMIT_WINDOW])

    def _record_fail(self, uname, now=None):
        """记录一次失败时间戳(成功后清零)。"""
        if not uname:
            return
        else:
            self._fail_times.setdefault(uname, []).append(now or time.time())

    def _clear_fail_times(self, uname):
        """清零失败时间记录(成功/人工重置时调用)。"""
        if uname:
            self._fail_times.pop(uname, None)

    def _has_password(self) -> bool:
        return bool(self._pwd_hash and self._pwd_salt)

    def _verify_password(self, pwd: str) -> bool:
        return self._has_password() and _hash_password(pwd, self._pwd_salt) == self._pwd_hash

    def _refresh_manual_rows(self):
        """手动状态变更后刷新表格与统计"""
        self._update_table()
        self._update_stats()

    def _on_unban_row(self, acc):
        """封禁/停用行「解封」:走云端重置路径,解除后重新参与调度。"""
        if getattr(self, 'is_running', False):
            return
        else:
            if not self.cloud_client:
                QMessageBox.warning(self, '未连接', '尚未连接云端,无法解封')
                return
            else:
                self._do_restore([(acc.account_id, acc.username)])

    @staticmethod
    def _status_now() -> str:
        return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    def _pause_payload(self, acc):
        """暂停上报载荷:暂停一律上报(观察期/封禁只是记录,不挡暂停)\n\n        用独立的 paused 布尔标记,不写 status,避免抢状态列。\n        """
        if not self.cloud_client or not getattr(acc, 'account_id', 0):
            return None
        else:
            return {'paused': True, 'status_time': self._status_now(), 'stage': '手动暂停', 'message': '已手动暂停调度'}

    def _resume_payload(self, acc):
        """取消暂停上报载荷:发 paused=false,状态列仍按真实显示状态报 WAITING/READY"""
        if not self.cloud_client or not getattr(acc, 'account_id', 0):
            return None
        else:
            if getattr(acc, 'status', '') in ['RUNNING', 'EXECUTING', 'LAUNCHING']:
                return
            else:
                disp, _ = _derive_display_status(acc)
                status = 'WAITING' if disp == '等待时间' else 'READY'
                return {'paused': False, 'status': status, 'status_time': getattr(acc, 'status_time', '') or '' if status == 'WAITING' else '', 'stage': '手动恢复', 'message': '已恢复调度'}

    def _batch_cloud_status(self, items: list, label: str):
        """一次批量提交多号状态(RestoreWorker 单线程顺序提交,不逐号起线程)"""
        if not self.cloud_client or not items:
            return None
        else:
            worker = RestoreWorker(self.cloud_client, items, status=None)
            worker.setObjectName('batch_status')
            if not hasattr(self, '_batch_workers'):
                self._batch_workers = []
            self._batch_workers.append(worker)
            def _on_fail(aid, err):
                self._log(f'{label}云端回传失败: ID={aid} {err}', 'ERROR')
            def _on_done(ok_n, fail_n):
                self._log(f'{label}云端回传完成: 成功 {ok_n} 个' + (f',失败 {fail_n} 个' if fail_n else ''), 'SUCCESS' if not fail_n else 'WARN')
            worker.one_failed.connect(_on_fail)
            worker.all_done.connect(_on_done)
            worker.start()

    def _on_toggle_pause(self, acc):
        """暂停/恢复单个账号的调度(随时可点,不影响手动登录)"""
        name = acc.username
        if name in self._paused:
            self._paused.discard(name)
            self._ban_paused.discard(name)
            self._clear_run_times(name)
            self._clear_fail_times(name)
            if hasattr(self, '_auto_pause_logged'):
                self._auto_pause_logged.discard(name)
            self._pause_after_terminal.pop(getattr(acc, 'account_id', 0), None)
            self._pause_reconciled.discard(name)
            self._log(f'账号 {name} 已恢复调度', 'SUCCESS')
            payload = self._resume_payload(acc)
            if payload:
                self._cloud_upsert(acc.account_id, payload, desc=f"{name} 状态={payload['status']}")
        else:
            self._paused.add(name)
            self._pause_reconciled.discard(name)
            self._log(f'账号 {name} 已暂停,调度任务将跳过该账号', 'WARN')
            payload = self._pause_payload(acc)
            if payload:
                if getattr(acc, 'status', '') in ['RUNNING', 'EXECUTING', 'LAUNCHING']:
                    self._pause_after_terminal[acc.account_id] = payload
                    self._log(f'账号 {name} 正在执行,本轮结束(终态上报)后补报已暂停', 'INFO')
                else:
                    self._cloud_upsert(acc.account_id, payload, desc=f'{name} 暂停标记(paused=true)')
        self._save_manual()
        self._refresh_manual_rows()

    def _on_pause_all(self):
        """一键暂停全部/一键恢复全部(自动切换)"""
        # ***<module>.MainWindow._on_pause_all: Failure: Different control flow
        names = {a.username for a in self.accounts if a.username and a.username not in self._ban_paused}
        if not names:
            QMessageBox.information(self, '提示', '当前没有可暂停的账号')
            return
        else:
            if any((n not in self._paused for n in names)):
                self._paused |= names
                for _n in names:
                    self._pause_reconciled.discard(_n)
                self.pause_all_btn.setText('一键恢复')
                batch = []
                deferred = 0
                for a in self.accounts:
                    if a.username not in names:
                        continue
                    else:
                        payload = self._pause_payload(a)
                        if not payload:
                            continue
                        else:
                            if getattr(a, 'status', '') in ['RUNNING', 'EXECUTING', 'LAUNCHING']:
                                self._pause_after_terminal[a.account_id] = payload
                                deferred += 1
                            else:
                                batch.append((a.account_id, payload))
                self._batch_cloud_status(batch, '一键暂停')
                self._log(f'已一键暂停 {len(names)} 个账号,调度任务将全部跳过' + (f'(其中 {deferred} 个运行中,结束后补报)' if deferred else ''), 'WARN')
            else:
                self._paused -= names
                self._prune_run_times()
                batch = []
                for _n in names:
                    self._clear_run_times(_n)
                    self._clear_fail_times(_n)
                    self._pause_reconciled.discard(_n)
                    if hasattr(self, '_auto_pause_logged'):
                        self._auto_pause_logged.discard(_n)
                for a in self.accounts:
                    if a.username not in names:
                        continue
                    else:
                        self._pause_after_terminal.pop(getattr(a, 'account_id', 0), None)
                        payload = self._resume_payload(a)
                        if payload:
                            batch.append((a.account_id, payload))
                self._batch_cloud_status(batch, '一键恢复')
                self.pause_all_btn.setText('一键暂停')
                self._log(f'已一键恢复 {len(names)} 个账号的调度(已清零失败计数)', 'SUCCESS')
            self._save_manual()
            self._refresh_manual_rows()

    def _on_toggle_lock(self, acc):
        """单个账号上锁/解锁(解锁需验证登录密码;上锁免密但需先有密码)"""
        name = acc.username
        if name in self._locked:
            if self._has_password():
                dlg = PasswordDialog(self, mode='verify')
                dlg.setWindowTitle(f'解锁账号 {name}')
                if dlg.exec() != QDialog.Accepted:
                    return
                else:
                    if not self._verify_password(dlg.password):
                        QMessageBox.warning(self, '密码错误', '登录密码不正确,该账号保持锁定')
                        self._log(f'账号 {name} 解锁失败:密码错误', 'WARN')
                        return
            self._locked.discard(name)
            self._log(f'账号 {name} 已解除登录验证锁,可直接手动登录', 'SUCCESS')
        else:
            if not self._has_password():
                reply = QMessageBox.question(self, '请先设置登录密码', f'锁定账号 {name} 前需要先设置登录密码\n(锁定后手动登录该账号必须输入密码)\n\n现在打开密码设置吗?', QMessageBox.Yes | QMessageBox.No)
                if reply == QMessageBox.Yes:
                    self._on_set_password()
                return None
            else:
                self._locked.add(name)
                self._log(f'账号 {name} 已上登录验证锁,手动登录时需输入密码', 'WARN')
        self._save_manual()
        self._refresh_manual_rows()

    def _on_lock_all(self):
        """一键上锁全部账号(免密,但需已设置登录密码)"""
        names = {a.username for a in self.accounts if a.username}
        if not names:
            QMessageBox.information(self, '提示', '当前没有账号')
            return
        else:
            if not self._has_password():
                QMessageBox.warning(self, '请先设置登录密码', '一键上锁前需要先设置登录密码\n(锁定后手动登录必须输入密码)')
                self._on_set_password()
                return
            else:
                self._locked |= names
                self._save_manual()
                self._refresh_manual_rows()
                self._log(f'已一键上锁 {len(names)} 个账号,手动登录均需输入登录密码', 'WARN')

    def _on_unlock_all(self):
        """一键解锁全部账号(需验证登录密码)"""
        names = {a.username for a in self.accounts if a.username}
        if not names:
            QMessageBox.information(self, '提示', '当前没有账号')
            return
        else:
            locked_now = names & self._locked
            if not locked_now:
                QMessageBox.information(self, '提示', '当前没有已上锁的账号')
                return
            else:
                if self._has_password():
                    dlg = PasswordDialog(self, mode='verify')
                    dlg.setWindowTitle('一键解锁全部账号')
                    if dlg.exec() != QDialog.Accepted:
                        return
                    else:
                        if not self._verify_password(dlg.password):
                            QMessageBox.warning(self, '密码错误', '登录密码不正确,账号保持锁定')
                            self._log('一键解锁失败:密码错误', 'WARN')
                            return
                self._locked -= names
                self._save_manual()
                self._refresh_manual_rows()
                self._log(f'已一键解锁 {len(locked_now)} 个账号', 'SUCCESS')

    def _on_set_password(self):
        """设置/修改登录密码(只存 PBKDF2 哈希)"""
        dlg = PasswordDialog(self, mode='set', has_old=self._has_password())
        if dlg.exec() != QDialog.Accepted:
            return
        else:
            if getattr(dlg, 'reset_requested', False):
                self._pwd_hash = ''
                self._pwd_salt = None
                self._start_locked = False
                self._save_manual()
                self._refresh_start_btn()
                self._log('已通过授权码重置登录密码(锁定账号名单保留),请设置新密码', 'WARN')
                self._on_set_password()
                return
            else:
                if dlg.old_password and (not self._verify_password(dlg.old_password)):
                    QMessageBox.warning(self, '密码错误', '当前密码不正确,未做修改')
                    self._log('登录密码修改失败:当前密码验证不通过', 'WARN')
                    return
                else:
                    self._pwd_salt = os.urandom(16)
                    self._pwd_hash = _hash_password(dlg.password, self._pwd_salt)
                    self._save_manual()
                    self._log('登录密码已设置,锁定账号的手动登录/解锁需输入该密码', 'SUCCESS')

    def _inject_paused_stat_and_lock_btn(self):
        """运行时注入 暂停 统计项 + 把 ready/waiting 变成可点击 + 独立锁按钮
        (对齐 v4 ui_patch_src.py 逻辑)
        """
        try:
            main_layout = self.centralWidget().layout()
            stats_bar = None
            banned_val = getattr(self, 'stat_banned', None)
            for i in range(main_layout.count()):
                item = main_layout.itemAt(i)
                lay = item.layout() if hasattr(item, 'layout') and item.layout() else None
                if lay is not None and type(lay).__name__ == 'QHBoxLayout' and banned_val is not None:
                    for j in range(lay.count()):
                        w = lay.itemAt(j).widget()
                        if w is banned_val[1]:
                            stats_bar = lay
                            break
                    if stats_bar is not None:
                        break
            if stats_bar is None:
                return None

            paused_label = QLabel('暂停')
            paused_label.setObjectName('statLabel')
            paused_value = ClickableLabel('0')
            paused_value.setObjectName('statValue')
            paused_value.clicked.connect(lambda _=False: self._set_filter('paused'))
            paused_value.setToolTip('点击后主表格只显示暂停账号;点"账号"恢复显示全部')
            self.stat_paused = (paused_label, paused_value)

            target_idx = -1
            for i in range(stats_bar.count()):
                w = stats_bar.itemAt(i).widget()
                if w is banned_val[1]:
                    target_idx = i
                    break
            if target_idx >= 0:
                stats_bar.insertWidget(target_idx + 1, paused_label)
                stats_bar.insertWidget(target_idx + 2, paused_value)

            for attr, mode, tip in (('stat_ready', 'ready', '点击后主表格只显示待执行账号;点"账号"恢复显示全部'),
                                    ('stat_waiting', 'waiting', '点击后主表格只显示等待时间的账号;点"账号"恢复显示全部')):
                pair = getattr(self, attr, None)
                if not pair:
                    continue
                label, value = pair
                if type(value).__name__ == 'ClickableLabel':
                    continue
                new_value = ClickableLabel(value.text())
                new_value.setObjectName(value.objectName() or 'statValue')
                if value.styleSheet():
                    new_value.setStyleSheet(value.styleSheet())
                new_value.setAlignment(value.alignment())
                if value.minimumWidth() > 0:
                    new_value.setMinimumWidth(value.minimumWidth())
                stats_bar.replaceWidget(value, new_value)
                value.hide()
                value.deleteLater()
                setattr(self, attr, (label, new_value))
                new_value.clicked.connect(lambda _=False, m=mode: self._set_filter(m))
                new_value.setCursor(Qt.PointingHandCursor)
                new_value.setToolTip(tip)

            lock_btn = QPushButton('锁开始')
            lock_btn.setToolTip('锁定/解锁"开始"按钮(上锁后点开始需输入登录密码)')
            lock_btn.clicked.connect(self._on_toggle_start_lock)
            self._start_lock_btn = lock_btn

            target_idx2 = -1
            for i in range(stats_bar.count()):
                w = stats_bar.itemAt(i).widget()
                if w is getattr(self, 'pwd_set_btn', None):
                    target_idx2 = i
                    break
            if target_idx2 >= 0:
                stats_bar.insertWidget(target_idx2 + 1, lock_btn)
            self._refresh_start_btn()
        except Exception as e:
            return None

    def _on_toggle_start_lock(self):
        """独立切换\"开始\"按钮锁(上锁免密,解锁需验证登录密码)"""
        if not getattr(self, '_has_password', lambda: False)():
            QMessageBox.warning(self, '请先设置登录密码', '锁开始前需要先设置登录密码\n(解锁开始时需输入该密码)')
            return
        else:
            if getattr(self, '_start_locked', False):
                dlg = PasswordDialog(self, mode='verify')
                dlg.setWindowTitle('解锁\"开始\"按钮')
                if dlg.exec() != QDialog.Accepted:
                    return
                else:
                    if not self._verify_password(dlg.password):
                        QMessageBox.warning(self, '密码错误', '登录密码不正确,开始按钮保持锁定')
                        self._log('解锁开始按钮失败:密码错误', 'WARN')
                        return
                    else:
                        self._start_locked = False
                        self._save_manual()
                        self._log('\"开始\"按钮已解锁', 'SUCCESS')
            else:
                self._start_locked = True
                self._save_manual()
                self._log('\"开始\"按钮已锁定', 'SUCCESS')
            self._refresh_start_btn()

    def _on_manual_login(self, acc):
        """手动登录:只执行第一步(登录平台并进入游戏),后续步骤不运行"""
        name = acc.username
        if getattr(self, 'is_running', False):
            QMessageBox.warning(self, '无法登录', '正在执行自动调度任务,请先停止后再手动登录账号')
            return
        else:
            if getattr(self, '_login_only_active', False):
                return
            else:
                fresh = None
                for a in self.accounts:
                    if a.username == name:
                        fresh = a
                        break
                if fresh is None:
                    QMessageBox.warning(self, '提示', f'账号 {name} 已不在列表中')
                    return
                else:
                    if fresh.username in self._locked:
                        if not self._has_password():
                            QMessageBox.warning(self, '请先设置登录密码', f'账号 {fresh.username} 已上锁,但尚未设置登录密码\n请先点「登录密码设置」')
                            return
                        else:
                            dlg = PasswordDialog(self, mode='verify')
                            dlg.setWindowTitle(f'登录验证 - {fresh.username}')
                            if dlg.exec() != QDialog.Accepted:
                                return
                            else:
                                if not self._verify_password(dlg.password):
                                    QMessageBox.warning(self, '密码错误', '登录密码不正确,无法登录该账号')
                                    self._log(f'账号 {fresh.username} 手动登录拒绝:密码错误', 'WARN')
                                    return
                    reply = QMessageBox.question(self, '确认手动登录', f'将只执行第一步:登录平台并进入游戏({fresh.username})。\n后续步骤(大厅/制造/交易行)不会运行,完成后可手动操作该账号。\n\n是否继续?', QMessageBox.Yes | QMessageBox.No)
                    if reply != QMessageBox.Yes:
                        return
                    else:
                        self._login_only_active = True
                        self._log(f'手动登录开始: {fresh.username}(仅第一步,登录平台并进入游戏)', 'SUCCESS')
                        self._launch_runner([fresh], 1, 0, automatic=False)

    def _set_filter(self, mode):
        """统计条过滤主表格:\n        None=全部 / \'ready\'=待执行 / \'waiting\'=等待时间 / \'paused\'=暂停 /\n        \'failed\'=失败 / \'banned\'=封禁 (互斥)\n        """
        self._filter_mode = mode
        pairs = [(self.stat_total[1], None)]
        for attr, m in [('stat_ready', 'ready'), ('stat_waiting', 'waiting'), ('stat_failed', 'failed'), ('stat_banned', 'banned')]:
            if hasattr(self, attr):
                pairs.append((getattr(self, attr)[1], m))
        if hasattr(self, 'stat_paused'):
            pairs.append((self.stat_paused[1], 'paused'))
        for value_label, m in pairs:
            if m == mode:
                value_label.setStyleSheet('color: #b0533c; font-weight: bold;')
            else:
                value_label.setStyleSheet('')
        self._update_table()
        n = len(self._view_rows)
        label = {None: '全部账号', 'ready': '待执行', 'waiting': '等待时间', 'paused': '暂停', 'failed': '失败', 'banned': '封禁'}.get(mode, '')
        if mode is None:
            msg = '正在显示:全部账号'
        else:
            msg = f'正在显示:{label} {n} 个'
        self.statusBar().showMessage(msg, 5000)

    def _find_missed_accounts(self) -> list:
        """找出\"成功但调度已到期/无调度\"的账号:调度库无记录或最晚设施时间已过。\n        V1.6:每轮刷新都可重复触发(不再按开机去重)——云端回传失败导致本地\n        重置被云端旧值冲回\"成功\"时,下一轮会再次纠正,彻底堵死\"显示可执行\n        却永远不跑\"的漏洞。正常跑完引擎会立刻写入下一轮未来时间,不会误判。"""
        schedules = _load_facility_schedules()
        now = datetime.now()
        missed = []
        for acc in self.accounts:
            if acc.status != 'SUCCESS':
                continue
            else:
                if acc.username in getattr(self, '_paused', ()):
                    continue
                else:
                    if acc.username in getattr(self, '_manual_ok', ()):
                        continue
                    else:
                        if acc.account_id in getattr(self, '_observed_ids', ()):
                            continue
                        else:
                            next_at = schedules.get(acc.username)
                            if next_at is not None and next_at > now:
                                    continue
                            missed.append(acc)
        self._missed_active &= {acc.account_id for acc in missed}
        return missed

    def _reset_missed_to_ready(self, missed: list):
        """把错过调度的账号重置为待执行(本地+云端)。\n        V1.6 细节:\n        · 本地重置每轮无条件生效(云端旧值\"成功\"冲进来也会被再次纠正);\n        · 日志只在账号\"进入错过状态\"时打一次(_missed_active 去重),防刷屏;\n        · 云端回传按账号限流(2分钟一次),云端持续失败也不会轰炸服务器。"""
        now_ts = time.time()
        now_str = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        transitions = []
        for acc in missed:
            if acc.status == 'SUCCESS':
                acc.status = 'READY'
                acc.status_time = ''
                acc.remark = '错过调度,已自动恢复待执行'
            if acc.account_id not in self._missed_active:
                self._missed_active.add(acc.account_id)
                transitions.append(acc.username)
            if acc.account_id and self.cloud_client and (now_ts - self._missed_upsert_ts.get(acc.account_id, 0) >= 120):
                        self._missed_upsert_ts[acc.account_id] = now_ts
                        self._cloud_upsert(acc.account_id, {'status': 'READY', 'status_time': now_str, 'stage': '自动恢复', 'message': '错过调度时间,自动重置为待执行', 'remark': '错过调度,已自动恢复待执行'}, desc=f'{acc.username} 错过调度自动恢复')
        if transitions:
            self._log(f"检测到 {len(transitions)} 个账号调度已到期({', '.join(transitions)}),已自动重置为待执行并补跑", 'WARN')

    def _log(self, text: str, level: str='INFO'):
        """添加日志到控制台"""
        try:
            _srv = getattr(getattr(self, 'config', None), 'server_url', '') or ''
            if _srv:
                text = text.replace(_srv, '***')
                _host = _srv.split('://', 1)[-1].split('/')[0].strip()
                if _host:
                    text = text.replace(_host, '***')
        except Exception:
            pass
        if not DEBUG_LOG:
            _masked = _mask_sensitive_log(text)
            if _masked is None:
                return None
            text = _masked
        timestamp = datetime.now().strftime('%H:%M:%S')
        color = {'INFO': '#c9d1d9', 'WARN': '#ff9800', 'ERROR': '#f44336', 'SUCCESS': '#4caf50'}.get(level, '#c9d1d9')
        self.log_console.append(f'<span style="color:#666">[{timestamp}]</span> <span style="color:{color}">{text}</span>')
        getattr(logger, level.lower(), logger.info)(text)
        try:
            _ldir = os.path.join(os.path.expanduser('~'), 'Desktop', 'PC特勤处', '运行日志')
            os.makedirs(_ldir, exist_ok=True)
            _lfile = os.path.join(_ldir, f'执行端_{datetime.now():%Y%m%d}.log')
            with open(_lfile, 'a', encoding='utf-8') as _lf:
                _lf.write(f'[{datetime.now():%Y-%m-%d %H:%M:%S}] [{level}] {text}\n')
        except Exception:
            return None

    def _on_price_adjust_changed(self, value: int):
        """交易行价格下调次数变更"""
        self.config.trade_price_adjust = value
        save_config(self.config)

    def _on_clean_old_logs(self):
        """手动清理历史运行日志:保留当天文件,不动账号数据/数据库。"""
        _ldir = os.path.join(os.path.expanduser('~'), 'Desktop', 'PC特勤处', '运行日志')
        if not os.path.isdir(_ldir):
            QMessageBox.information(self, '提示', '运行日志目录不存在,无需清理')
            return
        else:
            today_name = f'执行端_{datetime.now():%Y%m%d}.log'
            targets, total = ([], 0)
            for name in os.listdir(_ldir):
                p = os.path.join(_ldir, name)
                if not os.path.isfile(p) or not name.lower().endswith('.log'):
                    continue
                else:
                    if name == today_name:
                        continue
                    else:
                        targets.append(p)
                        try:
                            total += os.path.getsize(p)
                        except Exception:
                            pass
            if not targets:
                QMessageBox.information(self, '提示', '没有需要清理的历史日志(当天日志已保留)')
                return
            else:
                reply = QMessageBox.question(self, '清理旧日志', f'将删除 {len(targets)} 个历史日志文件(共 {total / 1024 / 1024:.1f} MB),\n当天日志「{today_name}」保留。\n是否继续？', QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
                if reply != QMessageBox.Yes:
                    return
                else:
                    ok = 0
                    for p in targets:
                        try:
                            os.remove(p)
                            ok += 1
                        except Exception:
                            pass
                    self._log(f'已清理 {ok} 个历史日志文件(保留当天日志)', 'SUCCESS' if ok == len(targets) else 'WARN')
                    QMessageBox.information(self, '清理完成', f'已删除 {ok} 个历史日志文件\n当天日志已保留')

    @staticmethod
    def _state_on(state) -> bool:
        """复选框状态归一化:int(0/2) 或 Qt.CheckState 枚举都按数值判定。\n\n        PySide6 6.11 的 Qt.CheckState.Unchecked 是普通枚举成员,直接 bool()\n        恒为 True;必须取 .value 再判,否则「取消勾选」会被存成开启。"""
        return bool(int(getattr(state, 'value', state)))

    def _on_ammo_changed(self, state: int):
        self.config.ammo_exchange_enabled = self._state_on(state)
        save_config(self.config)

    def _on_aw_changed(self, state: int):
        self.config.aw_enabled = self._state_on(state)
        save_config(self.config)

    def _on_sell_window_changed(self, state: int):
        self.config.sell_window_enabled = self._state_on(state)
        save_config(self.config)

    def _on_sell_time_changed(self, *_args):
        if hasattr(self, 'sell_start_edit') and hasattr(self, 'sell_end_edit'):
                self.config.sell_window_start = self.sell_start_edit.time().toString('HH:mm')
                self.config.sell_window_end = self.sell_end_edit.time().toString('HH:mm')
                save_config(self.config)

    def _sync_ban_pause(self, cloud_status: dict):
        """云端状态同步:只有真封禁(BANNED)才归入 _ban_paused(不混入 _paused)。\n\n        · 云端 status=BANNED → 记入 _ban_paused,不再调度;\n        · 云端 status=STOPPED(连败3次/验证码)不是封禁:不进封禁名单、\n          也不动人工暂停名单,状态列按原设计显示\"失败\",调度由 status=STOPPED 排除;\n        · 云端恢复正常 → 只移除 _ban_paused 记录,人工暂停/失败熔断写入的账号不受影响。\n        """
        changed = False
        for uname, st in (cloud_status or {}).items():
            if not uname:
                continue
            else:
                if st == 'BANNED':
                    if uname not in self._ban_paused:
                        self._ban_paused.add(uname)
                        changed = True
                        self._log(f'⛔ 账号 {uname} 已被封禁,加入封禁名单,不再调度', 'ERROR')
                else:
                    if uname in self._ban_paused:
                        self._ban_paused.discard(uname)
                        changed = True
                        self._log(f'✅ 账号 {uname} 封禁已解除,自动恢复调度', 'SUCCESS')
                        for acc in getattr(self, 'accounts', []) or []:
                            if acc.username == uname and acc.status == 'BANNED':
                                    acc.status = st or 'READY'
                                    acc.status_time = ''
        if changed:
            self._save_manual()

    def _collect_runnable(self):
        """按状态列显示(4设施倒计时推导)收集两类账号 → (ready, waiting):\n          ① 待执行(四台全归零)/失败(无倒计时,含新账号):启动后立即跑;\n          ② 等待时间(四台任一到点未到):交给引擎恢复调度、到点自动执行。\n        封禁/已停止(1小时内失败3次)不跑;执行中的账号由 worker 持有,不在此列。\n\n        防护:失败满 3 次时已在 _on_runner_status 里写入暂停名单,这里只需跳过。"""
        ready_accounts, waiting_accounts = ([], [])
        for a in self.accounts:
            if a.username in getattr(self, '_paused', ()):
                continue
            else:
                if a.username in getattr(self, '_ban_paused', ()):
                    continue
                else:
                    if a.status in ['BANNED', 'STOPPED']:
                        continue
                    else:
                        disp, _ = _derive_display_status(a)
                        if disp == '等待时间':
                            waiting_accounts.append(a)
                        else:
                            if disp in ['待执行', '失败']:
                                ready_accounts.append(a)
        return (ready_accounts, waiting_accounts)

    def _auto_pause_log(self, uname):
        """运行次数熔断日志(同一账号只报一次,避免刷屏)"""
        if not hasattr(self, '_auto_pause_logged'):
            self._auto_pause_logged = set()
        if uname in self._auto_pause_logged:
            return
        else:
            self._auto_pause_logged.add(uname)
            self._log(f'★ 账号 {uname} 在 {RUN_LIMIT_WINDOW // 60} 分钟内已运行 {RUN_LIMIT_TIMES} 次,已自动暂停该账号调度(仅此号,其他号不受影响),等人工放行', 'ERROR')
            self._save_manual()

    def _on_start(self):
        """开始按钮点击(v4:先过开始锁密码验证)"""
        if getattr(self, '_start_locked', False):
            dlg = PasswordDialog(self, mode='verify')
            dlg.setWindowTitle('\"开始\"按钮已锁定')
            if dlg.exec() != QDialog.Accepted:
                return
            else:
                if not self._verify_password(dlg.password):
                    QMessageBox.warning(self, '密码错误', '登录密码不正确,无法开始执行')
                    return
        if not self._verify_license():
            return
        else:
            if not getattr(self, '_driver_ok', True):
                self._install_driver()
                return
            else:
                if not self.accounts:
                    QMessageBox.warning(self, '警告', '没有可执行的账号')
                    return
                else:
                    ready_accounts, waiting_accounts = self._collect_runnable()
                    run_accounts = ready_accounts + waiting_accounts
                    if not run_accounts:
                        QMessageBox.warning(self, '警告', '没有待执行的账号,也没有带未来调度时间的账号可供等待')
                        return
                    else:
                        if ready_accounts and waiting_accounts:
                            confirm_text = f'将立即执行 {len(ready_accounts)} 个账号,并等待 {len(waiting_accounts)} 个账号到调度时间,是否继续?'
                        else:
                            if ready_accounts:
                                confirm_text = f'将启动 {len(ready_accounts)} 个账号的自动化流程,是否继续?'
                            else:
                                confirm_text = f'没有立即执行的账号,将启动并等待 {len(waiting_accounts)} 个账号到调度时间后自动执行,是否继续?'
                        reply = QMessageBox.question(self, '确认', confirm_text, QMessageBox.Yes | QMessageBox.No)
                        if reply != QMessageBox.Yes:
                            return
                        else:
                            self._user_stopped = False
                            self._launch_runner(run_accounts, len(ready_accounts), len(waiting_accounts), automatic=False)

    def _auto_start_if_needed(self, force=False):
        """自动模式(方案A):连上云端、账号加载完后自动接管——\n        有待执行立即跑、有倒计时就等、归零由引擎自动跑。无需任何人点按钮。\n        防护:10分钟内自动拉起 ≥4 次=疑似崩溃循环,停止自动拉起并提示。\n\n        force=True:仅用于「程序崩溃后自动续跑」——该场景不受 auto_start_queue\n        开关限制(那个开关只管\"打开执行端要不要自动开跑\"),但同样受下面的熔断约束。"""
        # ***<module>.MainWindow._auto_start_if_needed: Failure: Different control flow
        if not force and (not getattr(self.config, 'auto_start_queue', False)):
            if not getattr(self, '_auto_off_logged', False):
                self._auto_off_logged = True
                self._log('自动启动已关闭(auto_start_queue=false),请点击\"开始\"按钮手动执行', 'INFO')
            return None
        else:
            if not getattr(self, '_driver_ok', True):
                return
            else:
                if getattr(self, '_license_blocked', False):
                    return
                else:
                    if getattr(self, '_ban_restart_pending', False):
                        return
                    else:
                        if getattr(self, 'is_running', False):
                            return
                        else:
                            if not getattr(self, 'accounts', None):
                                return
                            else:
                                if not self.cloud_client or not self.config.is_valid():
                                    return None
                                else:
                                    if getattr(self, '_user_stopped', False):
                                        return
                                    else:
                                        if getattr(self, '_auto_blocked', False):
                                            return
                                        else:
                                            now = time.time()
                                            launches = [t for t in getattr(self, '_auto_launch_times', []) if now - t < 600]
                                            if len(launches) >= 4:
                                                self._auto_blocked = True
                                                self._log('自动模式:worker 短时间内反复退出,已停止自动拉起,请检查后手动点\"开始\"', 'ERROR')
                                                return
                                            else:
                                                self._auto_launch_times = launches
                                                ready_accounts, waiting_accounts = self._collect_runnable()
                                                run_accounts = ready_accounts + waiting_accounts
                                                if not run_accounts:
                                                    return
                                                else:
                                                    self._launch_runner(run_accounts, len(ready_accounts), len(waiting_accounts), automatic=True)

    def _launch_runner(self, run_accounts, n_ready, n_waiting, automatic=False, is_retry=False):
        """启动 runner 子进程(手动开始/自动模式共用)。\n        is_retry=True 表示本次为失败补跑,不计入自动拉起熔断计数。"""
        # ***<module>.MainWindow._launch_runner: Failure: Different control flow
        if not getattr(self, '_driver_ok', True):
            self._log('键盘驱动未安装,已阻止启动执行', 'ERROR')
            return
        else:
            login_only = getattr(self, '_login_only_active', False)
            if not login_only:
                _blocked = [acc.username for acc in run_accounts if acc.username in getattr(self, '_paused', ()) or acc.username in getattr(self, '_ban_paused', ()) or getattr(acc, 'status', '') in ['BANNED', 'STOPPED']]
                if _blocked:
                    run_accounts = [acc for acc in run_accounts if acc.username not in _blocked]
                    n_ready = len(run_accounts)
                    n_waiting = 0
                    if not run_accounts:
                        self._log('本轮账号均已在暂停名单,已全部跳过,等人工处理', 'ERROR')
                        return
            accounts_data = []
            for acc in run_accounts:
                accounts_data.append({'account': acc.username, 'password': acc.password, 'platform': acc.platform or 'Steam', 'id': acc.account_id})
            if not login_only:
                self._manual_ok -= {acc.username for acc in run_accounts}
                self._reset_ready -= {acc.username for acc in run_accounts}
            self.is_running = True
            self.start_btn.setEnabled(False)
            self.stop_btn.setEnabled(True)
            self.restore_btn.setEnabled(False)
            prefix = '自动补跑(失败重试): ' if is_retry else '自动模式: ' if automatic else ''
            log_text = f'{prefix}开始执行 {n_ready} 个账号'
            if n_waiting:
                log_text += f',同时等待 {n_waiting} 个账号的调度时间'
            self._log(log_text, 'SUCCESS')
            if automatic and (not is_retry):
                    self._auto_launch_times.append(time.time())
            self._last_heartbeat = time.time()
            self._heartbeat_asked = False
            self._worker_lost_handled = False
            self._crash_round += 1
            self._current_run_ids = {acc.account_id for acc in run_accounts}
            if not login_only:
                _t = time.time()
                for acc in run_accounts:
                    self._record_run(acc.username, _t)
                self._save_manual()
            _extra_cfg = {'ammo_exchange_enabled': self.config.ammo_exchange_enabled, 'aw_enabled': self.config.aw_enabled, 'sell_window_enabled': self.config.sell_window_enabled, 'sell_window_start': self.config.sell_window_start, 'sell_window_end': self.config.sell_window_end}
            self.runner_thread = RunnerWorkerThread(accounts_data, self.config.server_url, self.config.token, self.config.trade_price_adjust, login_only=login_only, extra_cfg=_extra_cfg, force_gdi=getattr(self, '_force_gdi', False))
            self.runner_thread.log_received.connect(self._on_runner_log)
            self.runner_thread.status_received.connect(self._on_runner_status)
            self.runner_thread.account_data_received.connect(self._on_runner_account_data)
            self.runner_thread.facility_received.connect(self._on_runner_facility)
            self.runner_thread.banned_received.connect(self._on_runner_banned)
            self.runner_thread.observation_received.connect(self._on_runner_observation)
            self.runner_thread.done_received.connect(self._on_runner_done)
            self.runner_thread.error_occurred.connect(self._on_runner_error)
            self.runner_thread.heartbeat_received.connect(self._on_runner_heartbeat)
            self.runner_thread.start()
            self.watchdog_timer.start(5000)

    def _on_stop(self):
        """停止按钮点击(用户主动操作:这是唯一允许全体停止的入口)"""
        self._user_stopped = True
        self._log('【用户操作】用户点击了\"停止\":本轮全部账号停止调度,程序不会自动再拉起', 'WARN')
        if self.runner_thread:
            self.runner_thread.stop()
            self._log('已发送停止信号', 'WARN')
        self.is_running = False
        self._refresh_start_btn()
        self.stop_btn.setEnabled(False)
        self.restore_btn.setEnabled(True)
        self.watchdog_timer.stop()

    def _restorable_codes(self):
        """可手动恢复的状态码(仅 RUNNING/EXECUTING 这类\"正在执行中\"不可恢复)"""
        return {'BANNED', 'FAILED', 'WAITING', 'SUCCESS', 'STOPPED', 'SKIPPED'}

    def _find_account(self, account_id: int):
        for acc in self.accounts:
            if acc.account_id == account_id:
                return acc
        return

    def _selected_account_ids(self):
        """取表格中选中行的账号ID列表(行号=当前显示的 _view_rows 下标,过滤后仍正确)"""
        ids = []
        for row in sorted({idx.row() for idx in self.table.selectedIndexes()}):
            if 0 <= row < len(getattr(self, '_view_rows', []) or []):
                    acc = self._view_rows[row]
                    ids.append((acc.account_id, acc.username))
        return ids

    def _on_table_context_menu(self, pos):
        """表格右键菜单"""
        menu = QMenu(self.table)
        act_restore = menu.addAction('恢复选中账号为待执行')
        act_refresh = menu.addAction('刷新列表')
        action = menu.exec(self.table.viewport().mapToGlobal(pos))
        if action == act_restore:
            self._on_restore()
        else:
            if action == act_refresh:
                self._refresh_accounts()

    def _on_restore(self):
        """恢复按钮/右键:选中则恢复选中,未选中则询问恢复全部终态账号"""
        if not self.cloud_client:
            QMessageBox.warning(self, '警告', '请先配置服务器')
            return
        else:
            if self.is_running:
                QMessageBox.warning(self, '警告', '正在执行中,不能恢复账号')
                return
            else:
                running_codes = {'LAUNCHING', 'EXECUTING', 'RUNNING'}
                selected = self._selected_account_ids()
                targets = []
                if selected:
                    for aid, uname in selected:
                        acc = self._find_account(aid)
                        code = STATUS_CN_TO_CODE.get(acc.status, acc.status) if acc else ''
                        if code not in running_codes:
                            targets.append((aid, uname))
                    if not targets:
                        QMessageBox.information(self, '提示', f'选中的 {len(selected)} 个账号都无需恢复\n(仅\"正在执行中\"不可恢复)')
                        return
                    else:
                        tip = f'将把选中的 {len(targets)} 个账号重置为\"待执行\"。\n是否继续？'
                else:
                    for acc in self.accounts:
                        code = STATUS_CN_TO_CODE.get(acc.status, acc.status)
                        if code not in running_codes:
                            targets.append((acc.account_id, acc.username))
                    if not targets:
                        QMessageBox.information(self, '提示', '没有需要恢复的账号\n(所有账号都在执行中)')
                        return
                    else:
                        tip = f'将把全部 {len(targets)} 个账号重置为\"待执行\"。\n是否继续？'
                reply = QMessageBox.question(self, '确认恢复', tip, QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)
                if reply != QMessageBox.Yes:
                    return
                else:
                    self._do_restore(targets)

    def _do_restore(self, targets: list):
        """异步批量恢复;targets=[(id, username), ...]"""
        ids = [t[0] for t in targets]
        self.restore_btn.setEnabled(False)
        self.restore_btn.setText(f'重置中 0/{len(ids)}')
        self._log(f'开始重置 {len(ids)} 个账号为待执行...', 'INFO')
        self._restore_worker = RestoreWorker(self.cloud_client, ids)
        self._restore_targets = dict(targets)
        self._restore_worker.progress.connect(self._on_restore_progress)
        self._restore_worker.one_ok.connect(self._on_restore_one_ok)
        self._restore_worker.one_failed.connect(self._on_restore_one_failed)
        self._restore_worker.all_done.connect(self._on_restore_all_done)
        self._restore_worker.start()

    @Slot(int, int)
    def _on_restore_progress(self, done: int, total: int):
        self.restore_btn.setText(f'重置中 {done}/{total}')

    @Slot(int)
    def _on_restore_one_ok(self, account_id: int):
        uname = getattr(self, '_restore_targets', {}).get(account_id, str(account_id))
        self._paused.discard(uname)
        self._ban_paused.discard(uname)
        if hasattr(self, '_auto_pause_logged'):
            self._auto_pause_logged.discard(uname)
        acc = self._find_account(account_id)
        if acc:
            acc.status = 'READY'
            acc.status_time = ''
            acc.remark = '手动恢复,等待执行'
            _now_iso = datetime.now().isoformat(sep=' ', timespec='seconds')
            acc.facility_next_run_at = {k: _now_iso for k, _l in _FACILITY_LABELS}
            if hasattr(self, '_reset_ready'):
                self._reset_ready.add(uname)
            acc.fail_count = 0
            store = _load_last_run_store()
            if acc.username in store:
                store[acc.username]['fail_count'] = 0
                _save_last_run_store(store)
        self._clear_run_times(uname)
        self._clear_fail_times(uname)
        self._save_manual()
        self._refresh_manual_rows()
        self._log(f'  ✓ 账号 {uname} 已重置为待执行', 'SUCCESS')

    @Slot(int, str)
    def _on_restore_one_failed(self, account_id: int, err: str):
        uname = getattr(self, '_restore_targets', {}).get(account_id, str(account_id))
        self._log(f'  ✗ 账号 {uname} 重置失败: {err}', 'ERROR')

    @Slot(int, int)
    def _on_restore_all_done(self, ok_n: int, fail_n: int):
        self.restore_btn.setEnabled(True)
        self.restore_btn.setText('重置状态')
        if fail_n == 0:
            self._log(f'重置完成: {ok_n} 个账号已重置为待执行,可点\"开始\"执行', 'SUCCESS')
            QMessageBox.information(self, '重置完成', f'{ok_n} 个账号已重置为待执行\n现在可以点\"开始\"执行')
        else:
            self._log(f'重置结束:成功 {ok_n} 个,失败 {fail_n} 个', 'WARN')
            QMessageBox.warning(self, '重置结束', f'成功 {ok_n} 个,失败 {fail_n} 个\n详见日志控制台')
        self._refresh_accounts()

    @Slot(str, str)
    def _on_runner_log(self, text: str, level: str):
        """接收 runner 日志"""
        self._log(text, level)

    @Slot(str, str, str, str, int)
    def _on_runner_status(self, account: str, status: str, stage: str, message: str, account_id: int):
        """接收 runner 状态更新(status 可能是英文码或中文枚举名)"""
        code = status if status in STATUS_NAMES else STATUS_CN_TO_CODE.get(status, status)
        if getattr(self, '_login_only_active', False):
            for acc in self.accounts:
                if account_id and acc.account_id == account_id or (account and acc.username == account):
                    if code == 'SUCCESS':
                        acc.manual_login_ok = True
                        self._manual_ok.add(acc.username)
                        self._log(f'手动登录完成: {acc.username}(纯手动操作,不影响调度倒计时)', 'SUCCESS')
                    else:
                        if code == 'FAILED':
                            self._log(f'手动登录失败: {acc.username} - {stage} {message}', 'ERROR')
                    self._update_table()
                    self._update_stats()
                    self.statusBar().showMessage(f'{acc.username}: 手动登录{STATUS_NAMES.get(code, code)}', 3000)
                    return
            return
        already_terminal = False
        matched = None
        for acc in self.accounts:
            if account_id and acc.account_id == account_id or (account and acc.username == account):
                matched = acc
                if acc.status == 'BANNED' and code != 'BANNED':
                    already_terminal = True
                    self._log(f'{account}: 已标记封禁,忽略后续状态 {STATUS_NAMES.get(code, code)}({stage})', 'INFO')
                else:
                    if acc.status == 'OBSERVATION' and code not in ('BANNED', 'OBSERVATION'):
                        already_terminal = True
                        self._log(f'{account}: 已标记观察期,忽略后续状态 {STATUS_NAMES.get(code, code)}({stage})', 'INFO')
                    else:
                        acc.status = code
                        if code == 'SUCCESS':
                            acc.last_run_at = datetime.now().strftime('%m-%d %H:%M:%S')
                            acc.last_run_stage = ''
                            acc.last_run_message = ''
                            store = _load_last_run_store()
                            store[acc.username] = {'at': acc.last_run_at, 'stage': '', 'message': '', 'fail_count': 0}
                            _save_last_run_store(store)
                            acc.fail_count = 0
                            self._clear_fail_times(acc.username)
                        else:
                            if code == 'FAILED':
                                acc.last_run_stage = stage or ''
                                acc.last_run_message = message or ''
                                store = _load_last_run_store()
                                if '验证码' in (message or '') or '验证码' in (stage or ''):
                                    acc.status = 'STOPPED'
                                    acc.remark = '出现验证码弹窗,需人工处理后恢复'
                                    acc.fail_count = 3
                                    info = dict(store.get(acc.username, {}) or {})
                                    info.update({'at': acc.last_run_at, 'stage': acc.last_run_stage, 'message': acc.last_run_message, 'fail_count': 3})
                                    store[acc.username] = info
                                    _save_last_run_store(store)
                                    self._paused.add(acc.username)
                                    self._save_manual()
                                    self._log(f'★ 账号 {acc.username} 出现验证码弹窗,已停止该账号自动调度(已写入暂停名单,后续不再拉起)——请人工去掉验证码后点「恢复」', 'ERROR')
                                else:
                                    self._record_fail(acc.username)
                                    self._prune_fail_times()
                                    fc = self._recent_fail_count(acc.username)
                                    store[acc.username] = {'at': acc.last_run_at, 'stage': acc.last_run_stage, 'message': acc.last_run_message, 'fail_count': fc}
                                    _save_last_run_store(store)
                                    acc.fail_count = fc
                                    if fc >= RUN_LIMIT_TIMES:
                                        acc.status = 'STOPPED'
                                        acc.remark = '1小时内失败3次,待人工处理'
                                        self._paused.add(acc.username)
                                        self._save_manual()
                                        self._log(f'★ 账号 {acc.username} 1小时内失败 {fc} 次,已暂停该账号(写入暂停名单,当轮不再重登),请人工处理后再点"重置状态"', 'ERROR')
                            else:
                                if code == 'WAITING' and message:
                                    t = message.replace('下次执行时间', '').lstrip(':：').strip()
                                    acc.status_time = t or message
                                    if self.cloud_client and account_id and (account_id not in getattr(self, '_observed_ids', ())) and (self._waiting_upserted.get(account_id) != acc.status_time):
                                        self._waiting_upserted[account_id] = acc.status_time
                                        self._cloud_upsert(account_id, {'status': 'WAITING', 'status_time': acc.status_time}, desc=f'{account} 状态=等待时间')
                break
        if not already_terminal:
            self._update_table()
            self._update_stats()
            self.statusBar().showMessage(f'{account}: {STATUS_NAMES.get(code, code)} - {stage}', 3000)
        if code == 'RUNNING' and matched is not None:
            self._manual_ok.discard(matched.username)
            matched.manual_login_ok = False
        if code == 'RUNNING' and account_id:
            getattr(self, '_observed_this_run', set()).discard(account_id)
            self._terminal_upserted.pop(account_id, None)
            getattr(self, '_crash_running_ids', set()).discard(account_id)
        if code in TERMINAL_STATUSES and self.cloud_client and account_id and (not already_terminal):
            observed = account_id in getattr(self, '_observed_ids', ())
            obs_this_run = account_id in getattr(self, '_observed_this_run', ())
            if code != 'BANNED' and observed and obs_this_run:
                pause_followup = self._pause_after_terminal.pop(account_id, None)
                if pause_followup:
                    self._cloud_upsert(account_id, pause_followup, desc=f'{account} 暂停标记(paused=true)')
            else:
                if code != 'BANNED' and observed and (not obs_this_run):
                    self._observed_ids.discard(account_id)
                    self._cloud_upsert(account_id, {'status': 'SUCCESS', 'stage': stage, 'message': message}, desc=f'{account} 观察期已过,清除观察期标记', followup=self._pause_after_terminal.pop(account_id, None), followup_desc=f'{account} 暂停标记(paused=true)')
                    return
                else:
                    up_status = matched.status if matched else code
                    pause_followup = self._pause_after_terminal.pop(account_id, None)
                    _sig = (up_status, stage, message)
                    _last = self._terminal_upserted.get(account_id)
                    if _last and _last[0] == _sig and (time.time() - _last[1] < 600):
                        if pause_followup:
                            self._cloud_upsert(account_id, pause_followup, desc=f'{account} 暂停标记(paused=true)')
                        return None
                    else:
                        self._terminal_upserted[account_id] = (_sig, time.time())
                        self._cloud_upsert(account_id, {'status': up_status, 'stage': stage, 'message': message, 'remark': (getattr(matched, 'remark', '') or None) if matched else None}, desc=f'{account} 状态={STATUS_NAMES.get(up_status, up_status)}', followup=pause_followup, followup_desc=f'{account} 暂停标记(paused=true)')

    @Slot(int, str, str)
    def _on_runner_account_data(self, account_id: int, level: str, coins: str):
        """接收账号数据(等级/哈夫币)→ 更新本地并回传云端。\n\n        空值/\"???\"(本地库等级未知占位)一律不覆盖本地、不回传云端,\n        避免把未知值写进云端冲掉真实等级。\n        """
        if getattr(self, '_login_only_active', False):
            return
        else:
            level = (level or '').strip()
            coins = (coins or '').strip()
            self._log(f"账号数据更新: ID={account_id} 等级={level or '-'} 哈夫币={_format_hafu_w(coins) or '-'}", 'INFO')
            for acc in self.accounts:
                if acc.account_id == account_id:
                    if level and level != '???':
                            acc.level = level
                    if coins:
                        acc.haf_coins = coins
                    break
            self._update_table()
            if self.cloud_client and account_id:
                    payload = {}
                    if level and level != '???':
                            payload['level'] = level
                    if coins:
                        payload['hafu_coin'] = coins
                    if payload:
                        self._cloud_upsert(account_id, payload, desc=f'ID={account_id} 等级/哈夫币')

    @Slot(int, str, str)
    def _on_runner_banned(self, account_id: int, unban_at: str, source_text: str):
        """接收封禁事件 → 本地标记 + 回传云端(BANNED + 解封时间)"""
        if getattr(self, '_login_only_active', False):
            return
        else:
            self._log(f"★ 账号 ID={account_id} 被封禁,解封时间: {unban_at or '未识别'}", 'ERROR')
            banned_name = ''
            for acc in self.accounts:
                if acc.account_id == account_id:
                    acc.status = 'BANNED'
                    if unban_at:
                        acc.status_time = f'解封时间: {unban_at}'
                    banned_name = acc.username
                    break
            if banned_name:
                self._ban_paused.add(banned_name)
                self._paused.discard(banned_name)
                self._save_manual()
            self._update_table()
            self._update_stats()
            if self.cloud_client and account_id:
                    payload = {'status': 'BANNED', 'unban_at': unban_at or None, 'remark': '封禁,解封时间 ' + unban_at if unban_at else '封禁,解封时间未识别'}
                    self._cloud_upsert(account_id, payload, desc=f'ID={account_id} 封禁回传')
            self._restart_after_ban(account_id, banned_name)

    def _restart_after_ban(self, account_id: int, name: str=''):
        """运行中被封 → 立即把该号踢出本轮并重启 worker 跑其余账号。\n\n        · 手动登录过程不打断;\n        · 10 秒防抖 + 独立计数(10 分钟最多 6 次),不计入自动拉起的 4 次熔断;\n        · 不设置 _user_stopped(这不是用户主动停止)。\n        """
        if getattr(self, '_login_only_active', False):
            return
        else:
            if not getattr(self, 'is_running', False):
                return
            else:
                if account_id and account_id not in getattr(self, '_current_run_ids', set()):
                    return
                else:
                    now = time.time()
                    if now - getattr(self, '_ban_restart_at', 0.0) < 10:
                        return
                    else:
                        times = [t for t in getattr(self, '_ban_restart_times', []) if now - t < 600]
                        self._ban_restart_times = times
                        if len(times) >= 6:
                            self._log('封禁踢人重启过于频繁,本轮不再自动重启,请手动检查', 'ERROR')
                            return
                        else:
                            times.append(now)
                            self._ban_restart_at = now
                            self._ban_restart_pending = True
                            self._log(f'账号 {name or account_id} 运行中被封,立即踢出本轮并重启 worker,其余账号继续执行', 'WARN')
                            th = getattr(self, 'runner_thread', None)
                            if th:
                                th.stop()
                            QTimer.singleShot(800, self._do_ban_restart)

    def _do_ban_restart(self):
        """等被踢的 worker 退出后,用剩余可跑账号重启一轮(封禁号不再进入队列)。"""
        if not getattr(self, '_ban_restart_pending', False):
            return
        else:
            th = getattr(self, 'runner_thread', None)
            proc = getattr(th, 'proc', None)
            if proc is not None and proc.poll() is None:
                QTimer.singleShot(800, self._do_ban_restart)
                return
            else:
                self._ban_restart_pending = False
                self.is_running = False
                self._refresh_start_btn()
                self.stop_btn.setEnabled(False)
                self.restore_btn.setEnabled(True)
                self.watchdog_timer.stop()
                ready, waiting = self._collect_runnable()
                accts = ready + waiting
                if not accts:
                    self._log('封禁踢出后已无待执行账号,本轮结束', 'INFO')
                    self._refresh_accounts()
                    return
                else:
                    self._log(f'被封账号已踢出,继续执行其余 {len(accts)} 个账号', 'SUCCESS')
                    self._launch_runner(accts, len(ready), len(waiting), automatic=True, is_retry=True)

    def _cloud_upsert(self, account_id: int, payload: dict, desc: str='', followup: dict=None, followup_desc: str=''):
        """异步回传云端(payload 中 None 值不发送);followup=主回传完成后接着补报的载荷"""
        clean = {k: v for k, v in payload.items() if v is not None}
        follow = None
        if followup:
            follow = {k: v for k, v in followup.items() if v is not None}
        worker = UpsertWorker(self.cloud_client, account_id, clean, desc, followup=follow, followup_desc=followup_desc)
        worker.setObjectName(f'upsert_{account_id}')
        if not hasattr(self, '_upsert_workers'):
            self._upsert_workers = []
        self._upsert_workers.append(worker)
        worker.finished_ok.connect(self._on_upsert_ok)
        worker.failed.connect(self._on_upsert_failed)
        worker.start()

    @Slot(int, str)
    def _on_upsert_ok(self, account_id: int, desc: str):
        # ***<module>.MainWindow._on_upsert_ok: Failure: Different control flow
        self._log(f'云端回传成功: {desc}', 'INFO')
        if hasattr(self, '_upsert_workers'):
            self._upsert_workers = [w for w in self._upsert_workers if not (w.account_id == account_id and w.desc == desc and (not w.isRunning()))]

    @Slot(int, str)
    def _on_upsert_failed(self, account_id: int, err: str):
        self._log(f'云端回传失败: ID={account_id} {err}', 'ERROR')

    @Slot(str, dict)
    def _on_runner_facility(self, account: str, timers: dict):
        """接收设施倒计时 → 本地记日志 + 回传云端持久化"""
        if getattr(self, '_login_only_active', False):
            return
        else:
            timer_str = ', '.join((f'{k}={v}' for k, v in timers.items()))
            self._log(f'{account} 设施倒计时: {timer_str}', 'INFO')
            account_id = 0
            for acc in self.accounts:
                if acc.username == account:
                    account_id = acc.account_id
                    acc.facility_next_run_at = dict(timers)
                    break
            if self.cloud_client and account_id and timers:
                        self._cloud_upsert(account_id, {'facility_next_run_at': timers}, desc=f'{account} 4设施到期时间')

    @Slot(str, int, str)
    def _on_runner_observation(self, account: str, account_id: int, until: str):
        """接收观察期事件 → 只回传云端持久化解除时间,执行端本地不标不显示。\n\n        与封禁不同:观察期账号点掉提示后继续执行(不切账号),这里只做云端记录。\n        """
        if getattr(self, '_login_only_active', False):
            return
        else:
            if not hasattr(self, '_observed_ids'):
                self._observed_ids = set()
            if not hasattr(self, '_observed_this_run'):
                self._observed_this_run = set()
            if account_id:
                self._observed_ids.add(account_id)
                self._observed_this_run.add(account_id)
            if self.cloud_client and account_id:
                    self._cloud_upsert(account_id, {'status': 'OBSERVATION', 'observation_until': until or None, 'remark': '观察期,解除时间 ' + until if until else '观察期,解除时间未识别'}, desc=f'{account or account_id} 观察期回传')

    @Slot(bool)
    def _on_runner_done(self, success: bool):
        """runner 完成:检查本轮失败账号,未达3次上限则自动补跑。\n\n        顺序:本轮 worker 跑完(含等待倒计时的账号到点收菜)→ done →\n        在本轮跑过的账号里找仍 FAILED 且 fail_count<3 的,自动补跑一次;\n        补跑完会再次 done,届时无 FAILED 候选则刷新账号列表。\n        fail_count>=3 的账号已在 _on_runner_status 标记为 STOPPED,不会被补跑。"""
        # ***<module>.MainWindow._on_runner_done: Failure: Different control flow
        if getattr(self, '_ban_restart_pending', False):
            self._login_only_active = False
            self.is_running = False
            self._refresh_start_btn()
            self.stop_btn.setEnabled(False)
            self.restore_btn.setEnabled(True)
            self.watchdog_timer.stop()
            self._worker_lost_handled = True
        else:
            was_login_only = getattr(self, '_login_only_active', False)
            self._login_only_active = False
            self.is_running = False
            self._crash_streak = 0
            self._crash_running_ids.clear()
            self._refresh_start_btn()
            self.stop_btn.setEnabled(False)
            self.restore_btn.setEnabled(True)
            self.watchdog_timer.stop()
            self._log(f"任务执行结束 ({('成功' if success else '失败')})", 'SUCCESS' if success else 'ERROR')
            if not was_login_only and (not getattr(self, '_user_stopped', False)) and (not getattr(self, '_auto_blocked', False)):
                run_ids = getattr(self, '_current_run_ids', set())
                retry = [a for a in getattr(self, 'accounts', []) if a.account_id in run_ids and a.status == 'FAILED' and (a.username not in self._paused) and (a.username not in self._manual_ok) and (int(getattr(a, 'fail_count', 0)) < 3)]
                if retry:
                    names = ', '.join((a.username for a in retry))
                    self._log(f'{len(retry)} 个账号失败未达3次上限({names}),自动补跑', 'WARN')
                    self._launch_runner(retry, len(retry), 0, automatic=True, is_retry=True)
                    return None
            self._refresh_accounts()

    @Slot(str)
    def _on_runner_error(self, error: str):
        """runner 错误(含异常退出补发)。\n\n        程序自己崩的会纠正状态并自动接着跑;用户主动停止的只提示用户操作结果,\n        不自动拉起,也不再要求用户手动重开。"""
        if getattr(self, '_user_stopped', False):
            self._log(f'用户主动停止:运行进程已退出({error})', 'WARN')
        else:
            self._log(f'程序自动处理:运行进程异常({error}),正在纠正状态并自动接着跑', 'ERROR')
        if getattr(self, '_ban_restart_pending', False):
            self._login_only_active = False
            self.is_running = False
            self._refresh_start_btn()
            self.stop_btn.setEnabled(False)
            self.restore_btn.setEnabled(True)
            self.watchdog_timer.stop()
            self._worker_lost_handled = True
        else:
            if getattr(self, '_auto_restart_pending', False):
                self._auto_restart_pending = False
                self._log('卡死进程已停止(程序自动操作,非用户),准备重新拉起', 'WARN')
            self._login_only_active = False
            self.is_running = False
            self._refresh_start_btn()
            self.stop_btn.setEnabled(False)
            self.restore_btn.setEnabled(True)
            self.watchdog_timer.stop()
            self._recover_after_crash(error)
            self._refresh_accounts()

    @Slot(float)
    def _on_runner_heartbeat(self, ts: float):
        """收到 worker 心跳:记录时间,清除\"已询问\"标记"""
        self._last_heartbeat = ts or time.time()
        self._heartbeat_asked = False

    def _watchdog_check(self):
        """每5秒:检查 worker 进程存活;2分钟无心跳则询问是否停止。"""
        if not self.is_running or not self.runner_thread:
            return None
        else:
            proc = self.runner_thread.proc
            if proc is not None and proc.poll() is not None:
                if not self._worker_lost_handled:
                    self._handle_worker_lost(proc.poll())
                return None
            else:
                silent_for = time.time() - self._last_heartbeat
                if silent_for >= 120 and (not self._heartbeat_asked):
                        self._heartbeat_asked = True
                        self._log(f'【程序自动】运行进程 {int(silent_for)} 秒无心跳,判定卡死,已自动终止该进程(不是用户操作);退出后将自动接着跑', 'ERROR')
                        self._auto_restart_pending = True
                        if self.runner_thread:
                            self.runner_thread.stop()

    def _recover_after_crash(self, reason: str):
        """程序自身异常退出后的收尾 + 自动续跑(这不是用户点停止)。

        · 用户按过停止 / 封禁踢人重启中 / 手动登录:一律不自动拉起;
        · 崩溃时停在"执行中"的账号 → 本地+云端纠正为失败,重新进队列,不再卡死;
        · 同一账号 10 分钟内崩 2 次 → 只暂停该号并告警,其他账号照常跑;
        · 连续两次崩溃间隔 <60 秒 → 判定截屏环境问题,自动降级 GDI 再试一次;
        · 降级后仍崩 / 10 分钟内自动拉起 ≥4 次 → 复用现有熔断,停止自动拉起并提示。
        """
        if getattr(self, '_user_stopped', False):
            self._log('程序检测到运行进程退出,但本轮是用户主动停止,不自动拉起', 'INFO')
            return None
        if getattr(self, '_ban_restart_pending', False):
            return None
        if getattr(self, '_login_only_active', False):
            return None
        if self._crash_recovered_round == self._crash_round:
            return None
        self._crash_recovered_round = self._crash_round

        if not getattr(self.config, 'auto_resume_after_crash', True):
            self._log('程序已关闭崩溃自动恢复(auto_resume_after_crash=false),本次不自动接着跑,请手动点"开始"', 'WARN')
            return None

        now = time.time()

        crashed = [a for a in (getattr(self, 'accounts', None) or []) if getattr(a, 'status', '') == 'RUNNING']
        for acc in crashed:
            self._crash_running_ids.add(acc.account_id)
            acc.status = 'FAILED'
            acc.remark = '执行进程异常退出,已自动重新排队'
            acc.last_run_stage = '程序异常退出'
            acc.last_run_message = (reason or '')[:200]
            if acc.account_id and self.cloud_client:
                self._cloud_upsert(acc.account_id,
                                   {'status': 'FAILED', 'stage': '程序异常退出',
                                    'message': (reason or '')[:200], 'remark': acc.remark},
                                   desc=f'{acc.username} 崩溃后状态纠正')
            times = [t for t in self._crash_times_by_acc.get(acc.username, []) if now - t < 600]
            times.append(now)
            self._crash_times_by_acc[acc.username] = times
            if len(times) >= 2:
                self._paused.add(acc.username)
                self._save_manual()
                self._log(f'★ 账号 {acc.username} 10 分钟内崩溃 {len(times)} 次,已单独暂停该号,其他账号继续执行', 'ERROR')

        if crashed:
            self._log(f'程序异常退出,已把 {len(crashed)} 个"执行中"账号纠正为失败并重新排队: {", ".join(a.username for a in crashed)}', 'WARN')

        gap = (now - self._last_crash_at) if self._last_crash_at else 1000000000.0
        self._last_crash_at = now
        self._crash_streak = (self._crash_streak + 1) if gap < 60 else 1
        if self._crash_streak >= 2:
            if not getattr(self, '_force_gdi', False):
                self._force_gdi = True
                self._log('程序连续崩溃且间隔不足 60 秒,判定为截屏环境问题,已自动改用 GDI 截屏重试一次', 'WARN')
            else:
                self._auto_blocked = True
                self._log('切换到 GDI 后仍然崩溃,已停止自动拉起,请检查游戏/驱动环境后手动点"开始"', 'ERROR')
                return None

        self._log('程序自动恢复:3 秒后自动接着跑剩余账号(不是用户操作)', 'SUCCESS')
        QTimer.singleShot(3000, lambda: self._auto_start_if_needed(force=True))

    def _handle_worker_lost(self, code):
        """worker 进程消失的统一收尾(只执行一次)。\n\n        崩溃时停在\"执行中\"的账号由 _recover_after_crash 纠正并重新排队;\n        用户主动停止的情况下不会自动拉起。"""
        if self._worker_lost_handled:
            return
        else:
            self._worker_lost_handled = True
            self._login_only_active = False
            if getattr(self, '_ban_restart_pending', False):
                self._log('被封账号已踢出,worker 已提前退出,准备重启', 'WARN')
                self.is_running = False
                self._refresh_start_btn()
                self.stop_btn.setEnabled(False)
                self.restore_btn.setEnabled(True)
                self.watchdog_timer.stop()
            else:
                self._log(f'程序检测到运行进程已退出(退出码 {code}),正在纠正状态并自动接着跑', 'ERROR')
                self.is_running = False
                self._refresh_start_btn()
                self.stop_btn.setEnabled(False)
                self.restore_btn.setEnabled(True)
                self.watchdog_timer.stop()
                self._recover_after_crash(f'运行进程已退出(退出码 {code})')
                self._refresh_accounts()

    def closeEvent(self, event):
        """窗口关闭事件"""
        if self.is_running:
            reply = QMessageBox.question(self, '确认退出', '任务正在运行中,确定要退出吗?', QMessageBox.Yes | QMessageBox.No)
            if reply != QMessageBox.Yes:
                event.ignore()
                return
            else:
                self._on_stop()
        try:
            _proc = getattr(self.runner_thread, 'proc', None) if self.runner_thread else None
            if _proc is not None and _proc.poll() is None:
                    try:
                        _proc.wait(timeout=2)
                    except Exception:
                        try:
                            _proc.kill()
                            _proc.wait(timeout=1)
                        except Exception:
                            pass
                        else:
                            pass
        except Exception:
            pass
        event.accept()



def _derive_display_status(acc) -> tuple:
    """显示状态由4设施倒计时推导(状态列只认倒计时,不信任云端状态码)。\n\n    返回 (显示文本, 颜色key):\n      · 封禁(内部BANNED):不跑,显示\'封禁\',日志列显示封禁信息;\n      · 已停止(内部STOPPED,连败3次):显示\'失败\',调度排除,等人工恢复;\n      · 运行中(内部RUNNING):显示\'执行中\';\n      · 工作台/制药台/防具台 三项全部到点 → \'待执行\'(技术中心不参与判定);\n      · 上述三项有一项未到           → \'等待时间\';\n      · 否则(无数据/新账号)          → \'失败\'(可重跑)。\n    """
    status = getattr(acc, 'status', '')
    if status == 'BANNED':
        return ('封禁', 'BANNED')
    else:
        if status == 'STOPPED':
            return ('失败', 'FAILED')
        else:
            if status == 'RUNNING':
                return ('执行中', 'RUNNING')
            else:
                timers = getattr(acc, 'facility_next_run_at', None)
                if isinstance(timers, dict) and timers:
                    now = datetime.now()
                    remains = []
                    for key, _label in _FACILITY_LABELS:
                        if key == 'tech':
                            continue
                        else:
                            raw = timers.get(key)
                            if raw:
                                try:
                                    dt = datetime.fromisoformat(str(raw))
                                except ValueError:
                                    pass
                                else:
                                    remains.append(int((dt - now).total_seconds()))
                    if remains:
                        if all((r <= 0 for r in remains)):
                            return ('待执行', 'READY')
                        else:
                            return ('等待时间', 'WAITING')
                return ('失败', 'FAILED')


def _load_all_facility_timers() -> dict:
    """读 runner 本地调度库的完整4设施时间:\n    {username: {tech/workbench/pharmacy/armor: \'ISO时间串\'}}。\n    无有效时间的账号不放入结果;库不存在/失败返回空表。"""
    import sqlite3
    db = _runner_db_path()
    result = {}
    if not os.path.exists(db):
        return result
    else:
        try:
            conn = sqlite3.connect(db, timeout=10)
            try:
                cur = conn.cursor()
                for name, tech, wb, ph, ar in cur.execute('SELECT username, tech_next_at, workbench_next_at, pharmacy_next_at, armor_next_at FROM account_state'):
                    timers = {}
                    for key, raw in [('tech', tech), ('workbench', wb), ('pharmacy', ph), ('armor', ar)]:
                        if raw:
                            try:
                                datetime.fromisoformat(str(raw))
                                timers[key] = str(raw)
                            except ValueError:
                                pass
                            else:
                                pass
                    if timers:
                        result[name] = timers
            finally:
                conn.close()
        except Exception:
            pass
        return result


def _load_local_record_usernames() -> set:
    """本地存在任何痕迹的用户名集合:调度库 account_state/market_listing 两表
    + last_run.json。库不存在/失败时只返回 last_run 中的名字。"""
    import sqlite3
    names = {u for u in _load_last_run_store().keys() if u}
    db = _runner_db_path()
    if os.path.exists(db):
        try:
            conn = sqlite3.connect(db, timeout=10)
            try:
                cur = conn.cursor()
                for table in ('account_state', 'market_listing'):
                    try:
                        cur.execute(f'SELECT DISTINCT username FROM {table} WHERE username IS NOT NULL AND username<>\'\'')
                        names.update((u for u, in cur.fetchall() if u))
                    except sqlite3.Error:
                        continue
            finally:
                conn.close()
        except Exception:
            pass
    return names


def _purge_local_accounts(usernames) -> dict:
    """彻底清除指定账号在本机的全部记录(云端删除/删号重导后调用):\n    account_state 整行、market_listing 挂售记录、ammo_exchange_log 军需兑换标记、\n    last_run.json 条目。\n    返回 {\'state\': n, \'market\': n, \'ammo\': n, \'lastrun\': n}。"""
    import sqlite3
    info = {'state': 0, 'market': 0, 'ammo': 0, 'lastrun': 0}
    names, seen = ([], set())
    for u in usernames or []:
        if u and u not in seen:
                seen.add(u)
                names.append(u)
    if not names:
        return info
    else:
        db = _runner_db_path()
        if os.path.exists(db):
            conn = sqlite3.connect(db, timeout=10)
            try:
                cur = conn.cursor()
                for name in names:
                    cur.execute('DELETE FROM account_state WHERE username=?', (name,))
                    info['state'] += cur.rowcount
                    cur.execute('DELETE FROM market_listing WHERE username=?', (name,))
                    info['market'] += cur.rowcount
                    try:
                        cur.execute('DELETE FROM ammo_exchange_log WHERE username=?', (name,))
                        info['ammo'] += cur.rowcount
                    except sqlite3.Error:
                        pass
                    else:
                        pass
                conn.commit()
            finally:
                conn.close()
        store = _load_last_run_store()
        removed = 0
        for name in names:
            if name in store:
                del store[name]
                removed += 1
        if removed:
            _save_last_run_store(store)
            info['lastrun'] = removed
        return info


def _bg_image_path() -> str:
    """窗口氛围背景图(EXE 同目录 bg.jpg,随包分发)"""
    if getattr(sys, 'frozen', False):
        base = os.path.dirname(sys.executable)
    else:
        base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base, 'bg.jpg')


def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
