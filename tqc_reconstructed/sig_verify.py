# Decompiled with PyLingual (https://pylingual.io)
# Internal filename: 'sig_verify.py'
# Bytecode version: 3.12.0rc2 (3531)
# Source timestamp: 1970-01-01 00:00:00 UTC (0)

"""sig_verify.py — 云端响应签名校验(Ed25519)\n\n服务端对 HTTP 2xx 响应返回:\n  X-TQC-Ts      服务端 unix 秒\n  X-TQC-Sig     base64(Ed25519 签名,64 字节)\n  X-TQC-Sig-Alg ed25519\n\n待签字符串(UTF-8,\n 连接,末段后无换行):\n  <ts>\n<METHOD>\n<PATH>\n<base64(sha256(响应体原始字节))>\n\nPATH 只取路径(不含域名/query)。签名不通过即视为连到伪造云端,必须中断,\n不允许回落到本地默认规则。\n\n环境变量 PC_TQC_SIG_OFF=1 仅供本地排查临时关闭,正式包不在界面暴露。\n"""
global _public_key
import base64
import hashlib
import os
import threading
import time
from urllib.parse import urlsplit
PUBLIC_KEY_PEM = b'-----BEGIN PUBLIC KEY-----\nMCowBQYDK2VwAyEAM4yVqnLMMN/ORD6T8F5I3GKzrXJlevFv6ulO0UuFgCU=\n-----END PUBLIC KEY-----\n'
SIG_MAX_SKEW = 300
_SIG_EXEMPT_PREFIXES = ('/api/public/',)
SIG_OFF = os.environ.get('PC_TQC_SIG_OFF', '').strip().lower() in ['1', 'true', 'yes', 'on']
USER_MESSAGE = '云端响应校验失败，连接可能被劫持，已停止运行'
_key_lock = threading.Lock()
_public_key = None
class CloudSignatureError(Exception):
    """云端响应签名校验失败。"""
    def __init__(self, detail: str=''):
        self.detail = detail or ''
        super().__init__(USER_MESSAGE + (f'（{detail}）' if detail else ''))
def _get_public_key():
    global _public_key
    # ***<module>._get_public_key: Failure: Different control flow
    if _public_key is None:
        with _key_lock:
            if _public_key is None:
                from cryptography.hazmat.primitives.serialization import load_pem_public_key
                _public_key = load_pem_public_key(PUBLIC_KEY_PEM)
        return _public_key
def _header(headers, name: str) -> str:
    """大小写不敏感取响应头(兼容 requests 与 urllib)。"""
    if headers is None:
        return ''
    v = headers.get(name)
    if v is not None:
        return str(v).strip()
    try:
        for k, v in headers.items():
            if str(k).lower() == name:
                return str(v).strip()
    except Exception:
        return ''
    return ''

def _path_of(url_or_path: str) -> str:
    s = str(url_or_path or '')
    if '://' in s:
        path = urlsplit(s).path
    else:
        path = s.split('?', 1)[0].split('#', 1)[0]
    return path or '/'
def verify_response_signature(method: str, url_or_path: str, body: bytes, headers, now: float=None) -> None:
    """校验一次 2xx 响应的签名;失败抛 CloudSignatureError。\n\n    4xx/5xx 服务端不签名,调用方应在 raise_for_status 之后再调用本函数。\n    """
    if SIG_OFF:
        return
    else:
        path = _path_of(url_or_path)
        if path.startswith(_SIG_EXEMPT_PREFIXES):
            return
        else:
            ts_raw = _header(headers, 'x-tqc-ts')
            sig_b64 = _header(headers, 'x-tqc-sig')
            alg = _header(headers, 'x-tqc-sig-alg').lower()
            if not ts_raw or not sig_b64:
                raise CloudSignatureError('缺少签名响应头')
            else:
                if alg and alg != 'ed25519':
                    raise CloudSignatureError(f'不支持的签名算法 {alg}')
                else:
                    try:
                        ts_val = int(ts_raw)
                    except Exception:
                        raise CloudSignatureError('时间戳格式非法')
                    if abs((time.time() if now is None else now) - ts_val) > SIG_MAX_SKEW:
                        raise CloudSignatureError('响应时间戳超出允许范围,疑似重放')
                    else:
                        body_hash = base64.b64encode(hashlib.sha256(body or b'').digest()).decode('ascii')
                        message = f'{ts_raw}\n{method.upper()}\n{path}\n{body_hash}'.encode('utf-8')
                        try:
                            signature = base64.b64decode(sig_b64, validate=True)
                        except Exception:
                            raise CloudSignatureError('签名编码非法')
                        from cryptography.exceptions import InvalidSignature
                        try:
                            _get_public_key().verify(signature, message)
                        except InvalidSignature:
                            raise CloudSignatureError('签名不匹配')
                        except CloudSignatureError:
                            raise
                        except Exception as e:
                            raise CloudSignatureError(f'{type(e).__name__}: {e}')