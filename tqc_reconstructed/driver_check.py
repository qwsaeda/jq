"""driver_check — Interception 键盘驱动检测/安装

EXE 的游戏自动化用 Interception 内核驱动模拟键盘输入(绕过用户层检测)。
驱动没装 → InterceptionKeyboard 抛 InterceptionUnavailableError → 输入失败。

本模块:
  is_installed()       检测驱动是否已装(尝试实例化 InterceptionKeyboard)
  install_elevated()   弹 UAC 以管理员身份调 install-interception.exe /install
  install()            当前已是管理员时直接调 install-interception.exe /install
  ensure()             启动前调用:检测,未装则尝试安装并返回提示

驱动文件位置(提取目录内):
  assets/interception/install-interception.exe  安装器
  assets/interception/interception.dll          运行时 DLL
  pc_tqc/platform/interception.cp312-win_amd64.pyd  Python 绑定

★打包坑:PyInstaller 冻结后 sys.executable 指向执行端EXE,
interception.pyd 内部 interception_dll_candidates() 算出的候选路径
(如 <exe目录>/interception.dll、固定的 Desktop/PC特勤处/drivers/...)
都不指向解包目录 assets/interception/interception.dll,
会误报"没有找到 Interception x64 接口库"。
pyd 支持环境变量 TQC_INTERCEPTION_DLL 强制指定 DLL 路径(候选第一位),
本模块在导入 pc_tqc 之前就设置好,保证冻结/开发环境都能找到。
"""
import os
import sys
import subprocess
import ctypes

if getattr(sys, 'frozen', False):
    _DRV_BASE = os.path.dirname(sys.executable)
else:
    _DRV_BASE = os.path.dirname(os.path.abspath(__file__))

_CANDIDATE_EXTRACTED = [
    os.path.join(_DRV_BASE, '_internal', 'engine'),
    os.path.join(os.path.dirname(_DRV_BASE), '_internal', 'engine'),
    os.path.join(_DRV_BASE, 'PC特勤处V1.0.66.exe_extracted'),
    os.path.join(os.path.dirname(_DRV_BASE), 'PC特勤处V1.0.66.exe_extracted'),
]
EXTRACTED = next((p for p in _CANDIDATE_EXTRACTED if os.path.isdir(p)), _CANDIDATE_EXTRACTED[0])
if EXTRACTED not in sys.path:
    sys.path.insert(0, EXTRACTED)
for _pywin32_sub in ('pythonwin', 'win32', 'win32com', 'pywin32_system32'):
    _sub_path = os.path.join(EXTRACTED, _pywin32_sub)
    if not os.path.isdir(_sub_path):
        continue
    if _sub_path not in sys.path:
        sys.path.insert(0, _sub_path)
_ASSETS_DRV = os.path.join(EXTRACTED, 'assets', 'interception')
INSTALLER = os.path.join(_ASSETS_DRV, 'install-interception.exe')
DLL = os.path.join(_ASSETS_DRV, 'interception.dll')
_generator_patched = False


def _prime_dll_path():
    '''让 interception.pyd 在任何环境下都能找到解包目录里的运行时 DLL。

    pyd 内部 interception_dll_candidates() 只查两个固定位置:
      <app_data_dir>/drivers/interception.dll(固定路径,打包机器上不存在)
      <sys.executable 目录>/interception.dll(默认也不存在)
    且候选全失效时不会回退 PATH/cwd,直接抛
    "没有找到 Interception x64 接口库"(实测)。

    解决:导入模块后把候选生成器包一层,把解包目录内的
    assets/interception/interception.dll 插到第一位。
    InterceptionKeyboard 实例化时按模块全局名调用该生成器,
    所以进程内后续所有调用方(InputController 等)一并生效。
    另附带 add_dll_directory / PATH,处理 DLL 自身的依赖。
    '''
    global _generator_patched
    if not os.path.exists(DLL):
        return None
    try:
        os.add_dll_directory(_ASSETS_DRV)
    except (AttributeError, OSError):
        pass
    os.environ['PATH'] = _ASSETS_DRV + os.pathsep + os.environ.get('PATH', '')
    os.environ.setdefault('TQC_INTERCEPTION_DLL', DLL)
    if _generator_patched:
        return None
    try:
        from pathlib import Path
        from pc_tqc.platform import interception as _itc
        _orig_gen = _itc.interception_dll_candidates

        def _patched_gen():
            yield Path(DLL)
            for p in _orig_gen():
                yield p

        _itc.interception_dll_candidates = _patched_gen
        _generator_patched = True
    except Exception:
        # 补丁失败不影响后续流程(已通过环境变量指定 DLL 路径)
        return None


def is_admin():
    '''当前进程是否有管理员权限'''
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        return False


def is_installed():
    '''检测 Interception 驱动是否已安装并可用

    返回 (ok, message)。message 用固定前缀区分三类原因,便于界面按原因给不同提示:
      「驱动未安装: ...」  驱动真没装,可提权安装修复
      「接口库缺失: ...」  interception.dll / pyd 绑定缺失或加载失败,装驱动修不好
      「驱动不可用: ...」  其他异常(权限/设备被占用等)
    '''
    try:
        _prime_dll_path()
        os.chdir(EXTRACTED)
        from pc_tqc.platform.interception import InterceptionKeyboard
        kb = InterceptionKeyboard()
        del kb
        return (True, 'Interception 驱动已安装,可用')
    except Exception as e:
        name = type(e).__name__
        text = str(e)
        low = text.lower()
        if (isinstance(e, (ImportError, ModuleNotFoundError))
                or '接口库' in text or 'interception x64' in low
                or 'dll' in low or 'loadlibrary' in low
                or 'module' in low or 'pyd' in low
                or ('找不到' in text and '驱动' not in text)):
            return (False, f'接口库缺失: {name}: {text}')
        elif ('Unavailable' in name or 'not found' in low
                or '找不到' in text or 'driver' in low or '驱动' in text):
            return (False, f'驱动未安装: {e}')
        else:
            return (False, f'驱动不可用: {name}: {e}')


def install_elevated():
    '''弹 UAC,以管理员身份运行 install-interception.exe /install

    执行端本身是普通权限,直接 subprocess 起安装器会被驱动签名/写 system32 拒绝,
    所以必须走 ShellExecuteW 的 runas 动词由系统提权。

    返回 (ok, message):
      ok=True  安装器已以管理员身份启动(用户已在 UAC 点"是"),需重启生效
      ok=False 安装器不存在 / 用户取消 UAC / 启动失败(带可读原因)
    '''
    if not os.path.exists(INSTALLER):
        return (False, f'安装器不存在: {INSTALLER}')
    try:
        rc = ctypes.windll.shell32.ShellExecuteW(
            None, 'runas', INSTALLER, '/install',
            os.path.dirname(INSTALLER), 1)
        if rc is not None and (rc <= 32 or rc == 1223):
            if rc in (5, 1223):
                return (False, f'已取消管理员授权,驱动未安装(ShellExecuteW={rc})')
            return (False, f'启动安装器失败(ShellExecuteW={rc}),请手动以管理员身份运行 {INSTALLER} /install')
        return (True, '驱动安装程序已以管理员身份启动,安装完成后需重启 Windows')
    except Exception as e:
        return (False, f'启动安装器异常: {type(e).__name__}: {e}')


def install():
    '''运行 install-interception.exe /install(需管理员权限)

    返回 (ok, message):
      ok=True  安装命令已执行(需重启生效)
      ok=False 需要提权或安装器不存在
    '''
    if not os.path.exists(INSTALLER):
        return (False, f'安装器不存在: {INSTALLER}')
    if not is_admin():
        return (False, '需要管理员权限,请以管理员身份运行执行端或手动安装')
    try:
        r = subprocess.run([INSTALLER, '/install'],
                           capture_output=True, timeout=30,
                           cwd=os.path.dirname(INSTALLER))
        if r.returncode == 0:
            return (True, 'Interception 驱动安装命令已执行,请重启 Windows 后重新打开执行端')
        r2 = subprocess.run([INSTALLER],
                            capture_output=True, timeout=30,
                            cwd=os.path.dirname(INSTALLER))
        if r2.returncode == 0:
            return (True, 'Interception 驱动安装命令已执行,请重启 Windows 后重新打开执行端')
        return (False, f'安装失败 returncode={r.returncode}')
    except Exception as e:
        return (False, f'安装异常: {e}')


def ensure():
    '''启动前确保驱动就绪:检测 → 未装则返回安装提示

    返回 dict: {installed, message, need_reboot}
    '''
    ok, msg = is_installed()
    if ok:
        return {'installed': True, 'message': msg, 'need_reboot': False}
    inst_ok, inst_msg = install()
    return {'installed': False, 'message': f'{msg} | {inst_msg}', 'need_reboot': inst_ok}


if __name__ == '__main__':
    ok, msg = is_installed()
    print(f'已安装: {ok}')
    print(f'信息: {msg}')
    if not ok:
        print(f'安装器: {INSTALLER} (存在: {os.path.exists(INSTALLER)})')
        print(f'运行时 DLL: {DLL} (存在: {os.path.exists(DLL)})')
        print('管理员: ', is_admin())
