# Decompiled with PyLingual (https://pylingual.io)
# Internal filename: 'cloud_client.py'
# Bytecode version: 3.12.0rc2 (3531)
# Source timestamp: 1970-01-01 00:00:00 UTC (0)

"""cloud_client.py — 与自建云端通信的客户端模块\n\n对应原版 pc_tqc.cloud_accounts 的四个接口,去掉卡密签名,改固定令牌认证:\n  POST /api/client/accounts/list              拉取本令牌名下全部账号\n  POST /api/client/account/manufacture-config 按账号拉制造配置\n  POST /api/client/account/upsert             回传账号执行结果\n  POST /api/client/account/observation        只推送封禁观察期时间\n\nv1100: 2026-11-02 移除旧 IP 强制改写逻辑(_normalize_url),\n用户在界面「服务器配置」里填的地址(域名/IP/HTTP/HTTPS)原样使用。\n"""
import ctypes
import hashlib
import json
import logging
import os
import time
import uuid
import winreg
import requests
from dataclasses import dataclass, field
from datetime import datetime
from typing import Any, Callable, Optional
from sig_verify import CloudSignatureError, verify_response_signature, USER_MESSAGE
logger = logging.getLogger('pc_tqc.cloud_client')
APP_VERSION = '1.9.20261120'
def direct_get_json(base_url: str, path: str, timeout: int=10) -> Any:
    """GET base_url+path 并返回 JSON(用户配置的地址原样使用,不再强制改写)。"""
    url = base_url.rstrip('/') + '/' + path.lstrip('/')
    r = requests.get(url, timeout=timeout)
    r.raise_for_status()
    verify_response_signature('GET', url, r.content, r.headers)
    return r.json()
def device_fingerprint() -> str:
    """本机设备指纹:MachineGuid + 系统盘卷序列号 + 网卡MAC 的 SHA256。\n\n    只读取系统标识,不读取/修改任何进程内存,也不注入或结束任何进程。\n    """
    parts = []
    try:
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Microsoft\\Cryptography', 0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY)
        parts.append(str(winreg.QueryValueEx(key, 'MachineGuid')[0]))
    except Exception:
        pass
    try:
        vol = ctypes.c_ulong(0)
        root = os.environ.get('SystemDrive', 'C:') + '\\'
        ctypes.windll.kernel32.GetVolumeInformationW(ctypes.c_wchar_p(root), None, 0, ctypes.byref(vol), None, None, None, 0)
        parts.append('%012x' % vol.value)
    except Exception:
        pass
    try:
        parts.append('%012x' % uuid.getnode())
    except Exception:
        pass
    return hashlib.sha256('|'.join(parts).encode('utf-8')).hexdigest()
class CloudClientError(Exception):
    """云端通信异常"""
class CloudVerificationError(CloudClientError):
    """云端响应验签失败:判定连到伪造云端,必须中断,不得回落本地默认。"""
@dataclass
class CloudAccount:
    """从云端拉取的账号信息"""
    account_id: int
    username: str
    password: str
    level: str = ''
    haf_coins: str = ''
    status: str = 'PENDING'
    status_time: str = ''
    paused: bool = None
    remark: str = ''
    platform: str = 'steam'
    facility_next_run_at: dict = field(default_factory=dict)
    last_run_at: str = ''
    last_run_stage: str = ''
    last_run_message: str = ''
@dataclass
class ManufacturePlan:
    """制造配置计划"""
    catalog_version: int = 2
    config_version: int = 0
    tech: dict = field(default_factory=lambda: {'mode': 'pinned'})
    workbench: dict = field(default_factory=lambda: {'mode': 'pinned'})
    pharmacy: dict = field(default_factory=lambda: {'mode': 'pinned'})
    armor: dict = field(default_factory=lambda: {'mode': 'pinned'})
class CloudClient:
    """云端 API 客户端"""
    def __init__(self, server_url: str, token: str, timeout: int=30, device_id: str=''):
        self.server_url = server_url.rstrip('/')
        self.token = token
        self.timeout = timeout
        self.device_id = device_id
        self._session = ''
        self._session_expires_at = 0.0
        self.session = requests.Session()
        self.session.headers.update({'Authorization': f'Bearer {token}', 'Content-Type': 'application/json', 'User-Agent': 'PC-TQC-Client/1.0'})
    def set_session(self, session: str, expires_in=0):
        """保存握手会话;之后请求自动带 X-TQC-Session / X-TQC-Device 头。"""
        self._session = session or ''
        try:
            self._session_expires_at = time.time() + float(expires_in)
        except Exception:
            self._session_expires_at = 0.0
    def _session_headers(self) -> dict:
        """会话头;未握手或已过期时为空。"""
        if self._session and time.time() < self._session_expires_at:
            return {'X-TQC-Session': self._session, 'X-TQC-Device': self.device_id}
        else:
            return {}
    def _post(self, path: str, data: dict) -> dict:
        """发送 POST 请求,返回解析后的 JSON"""
        url = f'{self.server_url}{path}'
        try:
            resp = self.session.post(url, json=data, timeout=self.timeout, headers=self._session_headers())
            resp.raise_for_status()
            try:
                verify_response_signature('POST', url, resp.content, resp.headers)
            except CloudSignatureError as e:
                raise CloudVerificationError(str(e)) from e
            return resp.json()
        except requests.exceptions.ConnectionError as e:
            raise CloudClientError(f'无法连接服务器: {url}') from e
        except requests.exceptions.Timeout as e:
            raise CloudClientError(f'请求超时: {url}') from e
        except requests.exceptions.HTTPError as e:
            raise CloudClientError(f'HTTP {resp.status_code}: {resp.text[:200]}') from e
        except json.JSONDecodeError as e:
            raise CloudClientError(f'响应解析失败: {resp.text[:200]}') from e
    def list_accounts(self) -> list[CloudAccount]:
        """拉取本令牌名下全部账号"""
        data = self._post('/api/client/accounts/list', {})
        items = data.get('items', [])
        accounts = []
        for item in items:
            accounts.append(CloudAccount(account_id=item.get('id', 0), username=item.get('username', ''), password=item.get('password', ''), level=item.get('level', ''), haf_coins=item.get('hafuCoin', ''), status=item.get('status', 'PENDING'), status_time=item.get('statusTime', ''), paused=item.get('paused'), remark=item.get('remark', ''), platform=item.get('platform', 'steam'), facility_next_run_at=item.get('facilityNextRunAt') if isinstance(item.get('facilityNextRunAt'), dict) else {}))
        return accounts
    def get_manufacture_config(self, account_id: int) -> ManufacturePlan:
        """按账号拉制造配置"""
        data = self._post('/api/client/account/manufacture-config', {'accountId': account_id})
        if not data.get('ok', True):
            raise CloudClientError(data.get('error', '获取制造配置失败'))
        else:
            cfg = data.get('manufactureConfig', {})
            return ManufacturePlan(catalog_version=data.get('manufactureCatalogVersion', 2), config_version=data.get('manufactureConfigVersion', 0), tech=cfg.get('tech', {'mode': 'pinned'}), workbench=cfg.get('workbench', {'mode': 'pinned'}), pharmacy=cfg.get('pharmacy', {'mode': 'pinned'}), armor=cfg.get('armor', {'mode': 'pinned'}))
    def upsert_account(self, account_id: int, level: str=None, hafu_coin: str=None, status: str=None, status_time: str=None, remark: str=None, stage: str=None, message: str=None, observation_until: str=None, unban_at: str=None, facility_next_run_at: dict=None, paused: bool=None) -> bool:
        """回传账号执行结果"""
        data = {'accountId': account_id}
        if level is not None:
            data['level'] = level
        if hafu_coin is not None:
            data['hafuCoin'] = hafu_coin
        if status is not None:
            data['status'] = status
        if status_time is not None:
            data['statusTime'] = status_time
        if remark is not None:
            data['remark'] = remark
        if stage is not None:
            data['stage'] = stage
        if message is not None:
            data['message'] = message
        if observation_until is not None:
            data['observationUntil'] = observation_until
        if unban_at is not None:
            data['unbanAt'] = unban_at
        if facility_next_run_at is not None:
            data['facilityNextRunAt'] = facility_next_run_at
        if paused is not None:
            data['paused'] = bool(paused)
        result = self._post('/api/client/account/upsert', data)
        return result.get('ok', False)
    def report_observation(self, account_id: int, observation_until: str) -> bool:
        """推送观察期时间,同时将账号状态置为 OBSERVATION。\n\n        观察期账号仍可执行制造任务,但需在云端持久化该状态,\n        避免刷新后被覆盖为\"等待\"。\n        """
        return self.upsert_account(account_id=account_id, status='OBSERVATION', observation_until=observation_until, status_time=datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
def create_client(server_url: str, token: str) -> CloudClient:
    """创建云端客户端实例"""
    return CloudClient(server_url, token)
def test_connection(server_url: str, token: str) -> tuple[bool, str]:
    """测试服务器连接和令牌有效性"""
    try:
        client = CloudClient(server_url, token, timeout=10)
        accounts = client.list_accounts()
        return (True, f'连接成功,共 {len(accounts)} 个账号')
    except CloudClientError as e:
        return (False, str(e))
    except Exception as e:
        return (False, f'未知错误: {type(e).__name__}: {e}')