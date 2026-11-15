# Source Generated with Decompyle++
# File: main_pyz__client_config.pyc (Python 3.12)

'''client_config.py — 执行端配置管理

负责保存/加载服务器地址和访问令牌到本地配置文件。
配置文件位置:用户目录下的 .pc_tqc_client/config.json
'''
import json
import os
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Optional
from urllib.parse import urlparse
DEFAULT_SERVER_URL = 'https://teqinsi.cn'
_LEGACY_SERVER_HOSTS = {
    '43.138.186.91'}

def _get_config_dir():
    '''获取配置目录(Windows 用户目录下的 .pc_tqc_client)'''
    if os.name == 'nt':
        base = Path(os.environ.get('USERPROFILE', Path.home()))
    else:
        base = Path.home()
    config_dir = base / '.pc_tqc_client'
    config_dir.mkdir(exist_ok=True)
    return config_dir


def _get_config_path():
    '''获取配置文件路径'''
    return _get_config_dir() / 'config.json'


@dataclass
class ClientConfig:
    '''执行端配置'''
    server_url: str = DEFAULT_SERVER_URL
    token: str = ''
    platform_mode: str = 'steam'
    trade_price_adjust: int = 4
    auto_start_queue: bool = False
    auto_resume_after_crash: bool = True
    ammo_exchange_enabled: bool = False
    aw_enabled: bool = False
    sell_window_enabled: bool = False
    sell_window_start: str = '14:00'
    sell_window_end: str = '20:00'

    def save(self) -> bool:
        '''保存配置到本地文件'''
        try:
            path = _get_config_path()
            with open(path, 'w', encoding='utf-8') as f:
                json.dump(asdict(self), f, ensure_ascii=False, indent=2)
            return True
        except Exception:
            return False

    @classmethod
    def load(cls) -> ClientConfig:
        '''从本地文件加载配置,不存在则返回默认配置。

        旧版本内置地址是裸 IP:已装用户配置里若仍是该地址,这里平滑迁移到域名,
        避免继续走明文 IP(也避免证书/SNI 问题);用户自填的其他地址原样保留。
        '''
        path = _get_config_path()
        if not path.exists():
            return cls()
        try:
            with open(path, 'r', encoding='utf-8-sig') as f:
                data = json.load(f)
            cfg = cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})
        except Exception:
            return cls()
        try:
            host = (urlparse(cfg.server_url or '').hostname or '').lower()
        except Exception:
            host = ''
        if (cfg.server_url or '').strip() and host in _LEGACY_SERVER_HOSTS:
            cfg.server_url = DEFAULT_SERVER_URL
            cfg.save()
        return cfg

    def is_valid(self):
        '''检查配置是否有效(服务器地址和令牌非空)'''
        return bool(self.server_url.strip() and self.token.strip())

    def get_display_url(self):
        '''获取显示用的服务器地址(隐藏令牌)'''
        return self.server_url



def save_config(config=None):
    '''保存配置'''
    return config.save()


def load_config():
    '''加载配置'''
    return ClientConfig.load()


def delete_config():
    '''删除配置文件(重置)'''
    path = _get_config_path()
    if path.exists():
        path.unlink()
        return None

