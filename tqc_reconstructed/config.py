# config.py — 程序配置模型及桌面数据目录定位（重建版：pyc 字节码直接还原，全函数反汇编对照）
# 来源：x/engine_pyz/pc_tqc__config.pyc（Python 3.12 原生字节码），dis 反汇编逐行还原，
# 非 decompiler 猜测；stage_timeouts 默认 8 阶段超时表由 BUILD_CONST_KEY_MAP 实锤。

'''程序配置模型及桌面数据目录定位。

无论源码运行还是 PyInstaller 成品运行，数据库、统计和卡密配置都统一写入
Windows 实际桌面的 ``PC特勤处`` 文件夹；不可恢复异常截图再归档到它下面的
``异常处理图片`` 子目录。设备私钥则单独保存在当前 Windows 用户的本地应用
数据目录，避免桌面同步、清理或替换 EXE 时丢失设备身份。

职责：承担“config.py”对应的单一功能。
边界：本模块属于应用基础层；修改公开接口时需同步调用方、交接文档与对应测试。
'''
from __future__ import annotations
import json
import os
import shutil
import sys
from dataclasses import asdict, dataclass, field
from pathlib import Path

APP_DATA_FOLDER_NAME = 'PC特勤处'
DIAGNOSTIC_IMAGE_FOLDER_NAME = '异常处理图片'
DEVICE_CREDENTIAL_FILE_NAME = 'device_credentials.dpapi'
LEGACY_DATA_FILES = ('config.json', 'pc_tqc.db', 'pc_tqc.db-wal', 'pc_tqc.db-shm', 'stats.json')
STEAM_APP_ID = '2507950'
VISION_POLL_INTERVAL_MS = 280
ACTION_STABLE_FRAMES = 2
ACTION_COOLDOWN_MS = 900
CAPTURE_BACKEND = 'wgc'
INPUT_MODE = 'foreground'
RESTORE_CURSOR = False
DELETE_LOGINUSERS = True


def windows_desktop_dir():
    """读取 Windows 实际桌面目录，兼容 OneDrive 等桌面重定向。"""
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_CURRENT_USER,
                             r'Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders')
        (value, _) = winreg.QueryValueEx(key, 'Desktop')
        resolved = os.path.expandvars(str(value).strip())
        if resolved:
            return Path(resolved)
        return Path.home() / 'Desktop'
    except (ImportError, OSError):
        return Path.home() / 'Desktop'


def _legacy_app_data_dir():
    """返回旧版本的数据位置，仅用于首次迁移已有配置。"""
    if getattr(sys, 'frozen', False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parents[2]


def _migrate_legacy_data(destination):
    """首次启用桌面目录时安全复制旧数据，保留旧文件作为回退。"""
    legacy = _legacy_app_data_dir()
    if legacy == destination:
        return
    for filename in LEGACY_DATA_FILES:
        source = legacy / filename
        target = destination / filename
        if source.is_file() or target.exists():
            continue
        temporary = destination / f'.{filename}.migrating'
        try:
            shutil.copy2(source, temporary)
            os.replace(temporary, target)
        except OSError:
            temporary.unlink(missing_ok=True)


def app_data_dir():
    """返回桌面专用数据目录，并保证数据与异常图片目录都存在。"""
    path = windows_desktop_dir() / APP_DATA_FOLDER_NAME
    path.mkdir(parents=True, exist_ok=True)
    (path / DIAGNOSTIC_IMAGE_FOLDER_NAME).mkdir(parents=True, exist_ok=True)
    _migrate_legacy_data(path)
    return path


def legacy_device_credential_path():
    """返回旧版桌面设备凭据路径，不删除或改写旧文件。"""
    return app_data_dir() / DEVICE_CREDENTIAL_FILE_NAME


def device_credential_path():
    """返回稳定的设备凭据路径，并一次性迁移旧桌面凭据。

    DPAPI 数据只能由同一 Windows 用户解密；将原文件复制到 LocalAppData
    不改变其加密上下文，同时避免升级或桌面同步导致每次启动生成新密钥。
    """
    local_app_data = os.environ.get('LOCALAPPDATA', '').strip()
    base = Path(local_app_data) if local_app_data else Path.home() / 'AppData' / 'Local'
    destination_dir = base / APP_DATA_FOLDER_NAME
    destination_dir.mkdir(parents=True, exist_ok=True)
    destination = destination_dir / DEVICE_CREDENTIAL_FILE_NAME
    legacy = legacy_device_credential_path()
    if destination.is_file() and legacy.is_file():
        temporary = destination.with_suffix(destination.suffix + '.migrating')
        try:
            shutil.copy2(legacy, temporary)
            os.replace(temporary, destination)
        except OSError:
            temporary.unlink(missing_ok=True)
    return destination


def diagnostic_image_dir():
    """返回异常截图专用目录。"""
    path = app_data_dir() / DIAGNOSTIC_IMAGE_FOLDER_NAME
    path.mkdir(parents=True, exist_ok=True)
    return path


@dataclass(slots=True)
class AppConfig:
    """可持久化配置；坐标和业务常量不放进用户配置，避免误改。"""
    license_card: str = ''
    platform_mode: str = 'steam'
    steam_path: str = ''
    wegame_path: str = ''
    market_price_decrements: int = 5
    stage_timeouts: dict = field(default_factory=lambda: {
        'cleanup': 20,
        'steam_login': 90,
        'launch_game': 90,
        'launcher': 120,
        'pre_lobby': 300,
        'profile': 30,
        'facility': 180,
        'market': 600,
    })

    @classmethod
    def load(cls, path=None):
        """读取已知字段；文件缺失或损坏时安全回退默认配置。"""
        config_path = path or app_data_dir() / 'config.json'
        if not config_path.is_file():
            return cls()
        try:
            raw = json.loads(config_path.read_text(encoding='utf-8'))
        except (OSError, json.JSONDecodeError):
            return cls()
        defaults = asdict(cls())
        defaults.update({key: value for key, value in raw.items() if key in defaults})
        return cls(**defaults)

    def save(self, path=None):
        """通过临时文件原子写入配置，避免异常退出留下半个 JSON。"""
        config_path = path or app_data_dir() / 'config.json'
        config_path.parent.mkdir(parents=True, exist_ok=True)
        temp_path = config_path.with_suffix('.json.tmp')
        temp_path.write_text(json.dumps(asdict(self), ensure_ascii=False, indent=2),
                             encoding='utf-8')
        os.replace(temp_path, config_path)
        return config_path
