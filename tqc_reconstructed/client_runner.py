# client_runner.py — 游戏自动化执行器（重建版：PyLingual 干净体 + 语义转储 analysis_biz.txt 拼合）
# 文件头（docstring/路径设置/DLL 目录循环）与 _emit_* 系列为 PyLingual 原始输出逐字保留；
# run_accounts/_run_single_account（含 5 个回调 + _manufacture_plan_loader + _norm_setting）
# 为 cflow 破碎区，按语义转储重写；_FACILITY_MAP 对照 runner_worker.py 干净版同源表。

'''client_runner.py — 游戏自动化执行器

基于原版 EXE 的 pc_tqc.workflow.runner.AccountRunner,通过 monkey-patch
接入现有四步脚本(第一步.py~第四步.py),并对接云端通信。

架构说明:
  - 本模块在独立子进程中运行(与 runner_worker.py 类似)
  - 使用原版 EXE 解包后的 pc_tqc 模块作为游戏自动化引擎
  - 通过 install_step_wrappers 注入自定义步骤逻辑
  - 云端通信通过 CloudClient 完成
'''
import json
import logging
import os
import sys
import tempfile
import threading
import time
from datetime import datetime
from typing import Optional, Callable

if getattr(sys, 'frozen', False):
    _HERE = os.path.dirname(sys.executable)
else:
    _HERE = os.path.dirname(os.path.abspath(__file__))
EXTRACTED = os.path.join(_HERE, 'PC特勤处V1.0.66.exe_extracted')
if not os.path.isdir(EXTRACTED):
    EXTRACTED = os.path.join(os.path.dirname(_HERE), 'PC特勤处V1.0.66.exe_extracted')
if EXTRACTED not in sys.path:
    sys.path.insert(0, EXTRACTED)
for _sub in ('win32', os.path.join('win32', 'lib'), 'pythonwin'):
    _p = os.path.join(EXTRACTED, _sub)
    if not os.path.isdir(_p):
        continue
    if _p not in sys.path:
        sys.path.append(_p)
for _d in (EXTRACTED, os.path.join(EXTRACTED, 'numpy.libs'), os.path.join(EXTRACTED, 'Shapely.libs'),
           os.path.join(EXTRACTED, 'cv2'), os.path.join(EXTRACTED, 'onnxruntime', 'capi'),
           os.path.join(EXTRACTED, 'PySide6'), os.path.join(EXTRACTED, 'pywin32_system32'),
           os.path.join(EXTRACTED, 'pythonwin')):
    if not os.path.isdir(_d):
        continue
    os.environ['PATH'] = _d + os.pathsep + os.environ.get('PATH', '')
    os.add_dll_directory(_d)

logger = logging.getLogger('pc_tqc.runner_client')


class RunnerSignals:
    '''回调信号集合(用于与 UI 通信)'''

    def __init__(self):
        self.on_log = None
        self.on_status = None
        self.on_account_data = None
        self.on_facility = None
        self.on_done = None


class FileStopEvent:
    '''跨进程停止信号(文件标志)'''

    def __init__(self, flag_path: str):
        self._flag = flag_path
        self._evt = threading.Event()
        self._t = threading.Thread(target=self._watch, daemon=True)
        self._t.start()

    def _watch(self):
        while not self._evt.is_set():
            try:
                if os.path.exists(self._flag):
                    self._evt.set()
                    return
            except Exception:
                pass
            time.sleep(0.5)

    def is_set(self):
        return self._evt.is_set()

    def set(self):
        self._evt.set()

    def wait(self, timeout=None):
        return self._evt.wait(timeout)


class ClientRunner:
    '''执行端自动化运行器'''

    def __init__(self, cloud_client=None, signals: RunnerSignals = None):
        self.cloud_client = cloud_client
        self.signals = signals or RunnerSignals()
        self.stop_flag_path = os.path.join(tempfile.gettempdir(), 'pc_tqc_client_stop.flag')
        self.stop_event = None
        self._runner = None
        self._accounts = []

    def _emit_log(self, text: str, level: str = 'INFO'):
        if self.signals.on_log:
            self.signals.on_log(text, level)
        getattr(logger, level.lower(), logger.info)(text)

    def _emit_status(self, account: str, status: str, stage: str, message: str = ''):
        if self.signals.on_status:
            self.signals.on_status(account, status, stage, message)
            return

    def _emit_account_data(self, account_id: int, level: str, coins: str):
        if self.signals.on_account_data:
            self.signals.on_account_data(account_id, level, coins)
            return

    def _emit_facility(self, account: str, timers: dict):
        if self.signals.on_facility:
            self.signals.on_facility(account, timers)
            return

    def _emit_done(self, success: bool):
        if self.signals.on_done:
            self.signals.on_done(success)
            return

    def _clear_stop_flag(self):
        try:
            if os.path.exists(self.stop_flag_path):
                os.remove(self.stop_flag_path)
        except Exception:
            pass

    def run_accounts(self, accounts: list):
        """执行账号队列

        accounts: [{'account': str, 'password': str, 'platform': str, 'id': int}, ...]
        """
        self._clear_stop_flag()
        self.stop_event = FileStopEvent(self.stop_flag_path)
        self._accounts = accounts
        success_count = 0
        fail_count = 0
        for idx, acc_data in enumerate(accounts):
            if self.stop_event.is_set():
                self._emit_log('用户停止,中断队列', 'WARN')
                break
            account = acc_data.get('account', '')
            password = acc_data.get('password', '')
            platform = acc_data.get('platform', 'Steam')
            account_id = acc_data.get('id', 0)
            self._emit_log(f'开始账号 {idx + 1}/{len(accounts)}: {account}', 'INFO')
            self._emit_status(account, '运行中', '初始化', '')
            try:
                ok = self._run_single_account(account, password, platform, account_id)
                if ok:
                    success_count += 1
                    self._emit_status(account, '成功', '完成', '')
                else:
                    fail_count += 1
                    self._emit_status(account, '失败', '完成', '')
            except Exception as e:
                fail_count += 1
                self._emit_log(f'账号 {account} 执行异常: {type(e).__name__}: {e}', 'ERROR')
                self._emit_status(account, '失败', '异常', str(e))
                continue
        self._emit_log(f'队列完成: 成功 {success_count} · 失败 {fail_count}',
                       'SUCCESS' if fail_count == 0 else 'WARN')
        self._emit_done(fail_count == 0)

    def _run_single_account(self, account: str, password: str, platform: str, account_id: int) -> bool:
        """执行单个账号的四步流程"""
        from pc_tqc.workflow import runner as runner_mod
        from pc_tqc import models as models_mod
        from pc_tqc import config as cfg_mod
        from workflow_steps import install_step_wrappers
        platform_mode = 'steam' if platform == 'Steam' else 'wgc'
        config = cfg_mod.AppConfig(license_card='', platform_mode=platform_mode)
        tp = getattr(self, '_trade_price_adjust', None)
        if tp is not None:
            try:
                config.market_price_decrements = int(tp)
            except Exception:
                pass
        steam_path = runner_mod.discover_steam_executable(config.steam_path)
        wegame_path = runner_mod.discover_wegame_executable(config.wegame_path)
        self._emit_log(f'平台路径: Steam={steam_path} WeGame={wegame_path}', 'INFO')

        def _on_log_cb(msg):
            self._emit_log(msg, 'INFO')

        def _on_account_update_cb(state):
            uname = state.account.username if state.account else account
            self._emit_status(uname, str(state.status), str(state.stage), state.message or '')
            fr = getattr(state, 'facility_next_run_at', {}) or {}
            if fr:
                timers = {}
                for stage, dt in fr.items():
                    if not dt:
                        continue
                    fac = self._stage_to_facility(stage)
                    if not fac:
                        continue
                    timers[fac] = dt.strftime('%Y-%m-%d %H:%M:%S')
                if timers:
                    self._emit_facility(uname, timers)

        def _on_account_completed_cb(state):
            self._emit_log(f'账号 {state.account.username} 完成,阶段 {state.stage}', 'INFO')

        def _on_observation_detected_cb(until):
            if self.cloud_client and account_id:
                try:
                    self.cloud_client.report_observation(account_id, until)
                except Exception as e:
                    self._emit_log(f'观察期回传失败: {e}', 'ERROR')

        def _on_finished_cb(success):
            self._emit_log(f'任务执行结束 ({"成功" if success else "失败"})', 'INFO' if success else 'ERROR')

        def _on_game_window_ready_cb():
            self._emit_log('游戏窗口已就绪', 'INFO')

        def _on_account_data_cb(level, coins):
            """第三步读到等级/哈夫币 → 回传"""
            self._emit_account_data(account_id, level, coins)
            if self.cloud_client and account_id:
                try:
                    self.cloud_client.upsert_account(account_id=account_id, level=level,
                                                     hafu_coin=coins, status='RUNNING',
                                                     status_time=datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
                    self._emit_log(f'已回传云端: 等级={level} 哈夫币={coins}', 'INFO')
                except Exception as e:
                    self._emit_log(f'云端回传失败: {e}', 'ERROR')

        callbacks = runner_mod.RunnerCallbacks(on_log=_on_log_cb,
                                               on_account_update=_on_account_update_cb,
                                               on_account_completed=_on_account_completed_cb,
                                               on_observation_detected=_on_observation_detected_cb,
                                               on_finished=_on_finished_cb,
                                               on_game_window_ready=_on_game_window_ready_cb)
        _exe_manufacture = None

        def _manufacture_plan_loader(aid):
            """云端制造配置 → EXE parse_manufacture_plan 计划"""
            nonlocal _exe_manufacture
            if not self.cloud_client or not aid:
                return None
            try:
                plan = self.cloud_client.get_manufacture_config(int(aid))
            except Exception as _e:
                self._emit_log(f'拉取云端制造配置失败 aid={aid}: {type(_e).__name__}: {_e}', 'WARN')
                return None
            _CUSTOM_ALIASES = {'top1', 'custom', 'manual', 'workshop'}

            def _norm_setting(s):
                d = dict(s or {})
                m = (d.get('mode') or 'pinned').lower()
                if m in _CUSTOM_ALIASES:
                    d['mode'] = 'custom'
                elif m == 'pinned' or not m:
                    d['mode'] = 'pinned'
                if 'itemId' in d and 'item_id' not in d:
                    d['item_id'] = d['itemId']
                return d

            settings = {
                'tech': _norm_setting(getattr(plan, 'tech', {})),
                'workbench': _norm_setting(getattr(plan, 'workbench', {})),
                'pharmacy': _norm_setting(getattr(plan, 'pharmacy', {})),
                'armor': _norm_setting(getattr(plan, 'armor', {})),
            }
            try:
                import 第四步 as _step4
                _step4.登记候选(aid, settings)
            except Exception as _se:
                self._emit_log(f'候选表登记失败 aid={aid}: {type(_se).__name__}: {_se}', 'WARN')
            has_any = any(s.get('mode') and s.get('mode') != 'pinned' or s.get('pinned') for s in settings.values())
            if not has_any:
                return None
            payload = {
                'catalogVersion': getattr(plan, 'catalog_version', 2) or 2,
                'manufactureCatalogVersion': getattr(plan, 'catalog_version', 2) or 2,
                'manufactureConfigVersion': getattr(plan, 'config_version', 0) or 0,
                'manufactureConfig': settings,
            }
            try:
                if _exe_manufacture is None:
                    from pc_tqc import manufacture as _exe_manufacture
                exe_plan = _exe_manufacture.parse_manufacture_plan(payload)
                modes_hint = ' '.join((f"{k}={v.get('mode', 'pinned')}" for k, v in settings.items()))
                self._emit_log(f'✓ 已装载制造计划 aid={aid} [{modes_hint}]', 'INFO')
                return exe_plan
            except Exception as _pe:
                self._emit_log(f'解析 EXE 制造计划失败 aid={aid}: {type(_pe).__name__}: {_pe}', 'WARN')
                return None

        ar = runner_mod.AccountRunner(config=config, stop_event=self.stop_event,
                                      logger=logging.getLogger('pc_tqc.runner'), callbacks=callbacks,
                                      state_store=None, manufacture_plan_loader=_manufacture_plan_loader,
                                      manufacture_account_ids=None, online_guard=None)
        install_step_wrappers(ar, self._emit_log, self.stop_event, account_id=account_id,
                              on_account_data=_on_account_data_cb,
                              on_observation_detected=_on_observation_detected_cb)
        acct = models_mod.Account(username=account, password=password)
        state = models_mod.AccountRunState(account=acct, index=0)
        try:
            ar.run([acct])
            return True
        except SystemExit:
            self._emit_log('工作流进程已退出(SystemExit)', 'WARN')
            return True
        except Exception as e:
            self._emit_log(f'任务异常: {type(e).__name__}: {e}', 'ERROR')
            return False

    @staticmethod
    def _stage_to_facility(stage: str) -> Optional[str]:
        mapping = {
            'tech': 'tech',
            '技术': 'tech',
            '技术中心': 'tech',
            'workbench': 'workbench',
            '工作台': 'workbench',
            'pharmacy': 'pharmacy',
            '制药': 'pharmacy',
            '制药台': 'pharmacy',
            'armor': 'armor',
            '防具': 'armor',
            '防具台': 'armor',
        }
        s = str(stage).lower()
        for k, v in mapping.items():
            if k in s:
                return v
        return None

    def stop(self):
        """停止执行"""
        try:
            with open(self.stop_flag_path, 'w') as f:
                f.write('stop')
            self._emit_log('已发送停止信号', 'WARN')
        except Exception as e:
            self._emit_log(f'写停止标志失败: {e}', 'ERROR')


def run_worker(config_path):
    """子进程工作函数(供命令行调用)

    config JSON 字段:
        server_url, token, accounts: [{account, password, platform, id}, ...]
    """
    with open(config_path, 'r', encoding='utf-8') as f:
        cfg = json.load(f)
    server_url = cfg.get('server_url', '')
    token = cfg.get('token', '')
    accounts = cfg.get('accounts_list') or cfg.get('accounts', [])
    cloud_client = None
    if server_url and token:
        from cloud_client import CloudClient
        cloud_client = CloudClient(server_url, token)

    class JsonSignals(RunnerSignals):
        def __init__(self):
            super().__init__()
            self.on_log = lambda t, l: print(json.dumps({'type': 'log', 'text': t, 'level': l},
                                                        ensure_ascii=False))
            self.on_status = lambda a, s, st, m: print(json.dumps({'type': 'status', 'account': a,
                                                                   'status': s, 'stage': st, 'message': m},
                                                                  ensure_ascii=False))
            self.on_account_data = lambda aid, l, c: print(json.dumps({'type': 'account_data',
                                                                       'account_id': aid, 'level': l,
                                                                       'coins': c}, ensure_ascii=False))
            self.on_facility = lambda a, t: print(json.dumps({'type': 'facility', 'account': a,
                                                              'timers': t}, ensure_ascii=False))
            self.on_done = lambda s: print(json.dumps({'type': 'done', 'success': s},
                                                      ensure_ascii=False))

    runner = ClientRunner(cloud_client=cloud_client, signals=JsonSignals())
    tp = cfg.get('trade_price_adjust')
    if tp is not None:
        runner._trade_price_adjust = tp
    runner.run_accounts(accounts)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python client_runner.py <config.json>')
        sys.exit(2)
    run_worker(sys.argv[1])
