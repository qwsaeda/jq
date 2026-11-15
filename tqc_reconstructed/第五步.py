# 第五步.py — 军需处兑换（重建版：忠实 100% 语义转储 analysis_biz.txt）

# 本文件 = 从语义转储重建的关键函数（可执行、可 ast.parse）+ pycdc 体对照
import json, os, re, sqlite3, sys, time, urllib.request, datetime, unicodedata
from datetime import date

STEP_NAME = '第五步'
_AMMO_EXCHANGE_TABLE = 'ammo_exchange'
VK_ESCAPE = 0x1B

# 主菜单专属词(判定是否在主大厅,与第二步同口径)
_LOBBY_MENU_KW = ('仓库', '特战干员', '交易行', '改枪台', '特勤处')

def _is_ammo_exchanged_today(username: str) -> bool:
    """检查该账号今天是否已兑换过子弹。"""
    if not username:
        return True
    db = _local_db()
    if not os.path.exists(db):
        return False
    today = date.today().isoformat()
    try:
        conn = sqlite3.connect(db, timeout=10)
        try:
            cur = conn.cursor()
            cur.execute(f'CREATE TABLE IF NOT EXISTS {_AMMO_EXCHANGE_TABLE} (username TEXT PRIMARY KEY, exchange_date TEXT)')
            row = cur.execute(f'SELECT exchange_date FROM {_AMMO_EXCHANGE_TABLE} WHERE username=?',
                              (username,)).fetchone()
            if row and row[0] == today:
                return True
            return False
        finally:
            conn.close()
    except Exception:
        return False


def _mark_ammo_exchanged_today(username: str) -> None:
    """标记该账号今天已兑换。"""
    if not username:
        return None
    db = _local_db()
    if not os.path.exists(db):
        return None
    today = date.today().isoformat()
    try:
        conn = sqlite3.connect(db, timeout=10)
        try:
            cur = conn.cursor()
            cur.execute(f'CREATE TABLE IF NOT EXISTS {_AMMO_EXCHANGE_TABLE} (username TEXT PRIMARY KEY, exchange_date TEXT)')
            cur.execute(f'INSERT OR REPLACE INTO {_AMMO_EXCHANGE_TABLE} (username, exchange_date) VALUES (?, ?)',
                        (username, today))
            conn.commit()
        finally:
            conn.close()
    except Exception:
        return None


def _norm_txt(s) -> str:
    """名称归一化:全角转半角、去全部空白、统一小写、×→x(防OCR空格/全角差异)。"""
    import unicodedata
    s = unicodedata.normalize('NFKC', str(s) if s else '')
    s = s.replace('×', 'x')
    return ''.join(s.split()).lower()


def _是子弹名(t: str) -> bool:
    """子弹名口径锚点:归一化后以.数字开头(.45 ACP/.50 AE/.300 BLK)
    或含 mm(9x19mm等)/auge(12 Gauge)/acp/blk。
    曾只认 mm+auge 导致 .45 ACP RIP 普查漏显示(10-16 ctbnesc7798实发)。"""
    nt = _norm_txt(t)
    if not nt:
        return False
    if nt[0] == '.' and len(nt) > 1 and nt[1].isdigit():
        return True
    return any(k in nt for k in ('mm', 'auge', 'acp', 'blk'))


def _名匹配(nname: str, t: str) -> bool:
    """Top5名(已归一化)匹配OCR文本:双向包含 + 点前缀容忍。
    10-16实发:OCR把.45 ACP RIP的点读丢成"45 ACP RIP"(同一行4个.45
    两个读出点两个没读出),归一化后 .45acprip 不包含于 45acprip 误判不在列表。"""
    nt = _norm_txt(t)
    if not nname or not nt:
        return False
    if nname in nt or nt in nname:
        return True
    alt = nname.lstrip('.')
    return bool(alt) and (alt in nt or nt in alt)


def _is_in_time_window(start_str: str, end_str: str) -> bool:
    """检查本机当前时间是否在 [start_str, end_str] 范围内(支持跨零点)。
    start_str/end_str: 'HH:MM' 格式;start > end 视为跨零点窗口(如 22:00-02:00)。"""
    try:
        from datetime import datetime, time
        now = datetime.now().time()
        sh, sm = [int(x) for x in start_str.split(':')]
        eh, em = [int(x) for x in end_str.split(':')]
        start_t = time(sh, sm)
        end_t = time(eh, em)
        if start_t <= end_t:
            return start_t <= now <= end_t
        return now >= start_t or now <= end_t
    except Exception:
        return True


def _cfg_bool(value, default=False) -> bool:
    """把界面/JSON 传进来的开关值归一化成 bool(未配置时按 default 处理)。
    兼容 bool、0/1、'true'/'false'、'on'/'off'、'yes'/'no'、'1'/'0' 等写法;
    无法识别时返回 default(默认 False=关闭),避免 bool('false')==True 的坑。"""
    if value is None:
        return default
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return bool(value)
    s = str(value).strip().lower()
    if s in ('1', 'true', 'on', 'yes', 'y', '开', '开启', '是'):
        return True
    if s in ('0', 'false', 'off', 'no', 'n', '关', '关闭', '否', ''):
        return False
    return default


def _read_worker_cfg() -> dict:
    """读 worker 子进程的界面配置(开关/时段)。

    优先进程环境变量 PC_TQC_RUNNER_CFG(worker 解析完配置后写入的 JSON 字符串,
    新链路配置经 stdin 传入、不再落地临时文件);兼容旧链路 sys.argv[1] 与
    %TEMP% 下历史遗留的 pc_tqc_runner_cfg_*.json;都找不到返回 {}。
    """
    try:
        _raw = os.environ.get('PC_TQC_RUNNER_CFG')
        if _raw:
            _cfg = json.loads(_raw)
            if isinstance(_cfg, dict):
                return _cfg
    except Exception:
        pass

    paths = []
    try:
        if len(sys.argv) >= 2 and str(sys.argv[1]).lower().endswith('.json') and os.path.exists(sys.argv[1]):
            paths.append(sys.argv[1])
    except Exception:
        pass

    try:
        import glob
        import tempfile
        tmp = tempfile.gettempdir()
        cands = [p for p in glob.glob(os.path.join(tmp, 'pc_tqc_runner_cfg_*.json')) if p not in paths]
        cands.sort(key=lambda p: os.path.getmtime(p), reverse=True)
        if cands:
            paths.append(cands[0])
    except Exception:
        pass

    for p in paths:
        try:
            with open(p, 'r', encoding='utf-8-sig') as f:
                return json.load(f)
        except Exception:
            continue

    return {}
def _do_ammo_exchange(ctx, hwnd, top5, log):
    '''游戏内军需处兑换操作(固定滚动序列 + 状态感知返回版)。

    设计要点(对应实测踩过的坑):
      1. 每次页面跳转都 OCR 确认到位,不到位重试一次,不盲点;
      2. 滚动只用鼠标悬停滚轮,绝不在列表中央点击(会误开子弹卡片);
      3. 滚动使用固定序列: 7 -> 7 -> 7 -> 3,每段滚完等1500ms稳定后扫描;
         序列滚完即到底,不依赖签名判底;
      4. 每款兑换结束(成功/失败/已兑换)都通过 back_to_list() 状态感知返回:
         已在列表则不按ESC,在详情页才按,误退到部门页会自动重进战术部门
         并补滚恢复位置;
      5. 任何异常都走 finally 回大厅(逐次ESC,检测到大厅特征即停,
         避免在大厅多按ESC把暂停菜单打开,挡死后续交易行入口);
      6. 关键节点检测错误弹窗(哈夫币不足/缺货/涨价等),出错关窗跳过该款。
    '''
    import win32api
    import win32con
    import win32gui

    VK_ESC = 27
    REF_W, REF_H = 1027, 768
    TOTAL_BUDGET = 300
    # ★ 字节码实锤 = (7, 6, 7, 3);docstring 里写的 "7 -> 7 -> 7 -> 3" 是笔误,以字节码为准
    SCROLL_SEQUENCE = (7, 6, 7, 3)
    deadline = time.time() + TOTAL_BUDGET

    def wait(ms):
        time.sleep(ms / 1000)

    def click(rx, ry, tag):
        return ctx.click_ref(hwnd, rx, ry, REF_W, REF_H, tag)

    def scaled(rx, ry):
        '''参考分辨率坐标 -> 当前客户区像素坐标。'''
        size = getattr(ctx, '_client_size', None)
        if size:
            return (int(rx * size[0] / REF_W), int(ry * size[1] / REF_H))
        return (rx, ry)

    def _列表区内(cx):
        size = getattr(ctx, '_client_size', None)
        w = size[0] if size else REF_W
        return cx < int(w * 0.67)

    def ocr_items(tag):
        frame = ctx.grab_frame(hwnd, tag)
        if frame is None:
            return None
        return ctx.ocr_scan(frame, tag)

    def wait_for_kws(kws, timeout, tag):
        '''循环OCR等关键词出现;返回 (items, 命中词) 或 (items, None)。'''
        end = time.time() + timeout
        items = None
        while time.time() < end:
            items = ocr_items(tag)
            if items:
                joined = ' '.join(t for t, _ in items)
                for kw in kws:
                    if kw in joined:
                        return (items, kw)
            if ctx.stopped():
                break
            wait(400)
        return (items, None)

    def in_list_page(items):
        '''是否子弹列表页:无详情页特征(一键补齐),且有多个口径文本块。'''
        if not items:
            return False
        joined = ' '.join(t for t, _ in items)
        if '一键补齐' in joined:
            return False
        cnt = sum(1 for t, _ in items if _是子弹名(t))
        return cnt >= 2

    def in_detail_page(items):
        '''是否详情页:有「一键补齐」或按钮区有「兑换/售罄/已兑换」等特征。'''
        if not items:
            return False
        joined = ' '.join(t for t, _ in items)
        if '一键补齐' in joined:
            return True
        txt = _norm_txt(btn_area_text(items, 886, 695))
        return any(w in txt for w in ('兑换', '售罄', '已兑换', '明日再来', '限购'))

    def in_department_page(items):
        '''是否部门选择页:命中≥2个部门名称(战斗/医疗/后勤/战术/研发部门)。'''
        if not items:
            return False
        joined = ' '.join(t for t, _ in items)
        cnt = sum(1 for w in ('战斗部门', '医疗部门', '后勤部门', '战术部门', '研发部门') if w in joined)
        return cnt >= 2

    def card_sold_out(items, cx, cy):
        '''列表页:子弹名点(cx,cy 为OCR实际像素)所在卡片今天是否已兑换。

        已兑子弹卡片变灰,盒图上盖黄色「已售罄」+「X时X分后更新上架」,
        位置在子弹名上方(盒图中央)。按窗口缩放比例给矩形容差,
        只在本卡片范围内找章字,不会误吃相邻卡片。
        返回 (是否售罄, 命中的原始OCR文本)。
        '''
        if not items:
            return (False, '')
        if cx is None or cy is None:
            return (False, '')
        size = getattr(ctx, '_client_size', None)
        sx = size[0] / REF_W if size else 1.0
        sy = size[1] / REF_H if size else 1.0
        rx = 150 * sx
        y_top = cy - 125 * sy
        y_bot = cy + 12 * sy
        for t, (tx, ty) in items:
            if abs(tx - cx) <= rx:
                if y_top <= ty <= y_bot:
                    if any(w in t for w in ('售罄', '售馨', '罄', '更新上架', '更新上', '后更新')):
                        return (True, t)
        return (False, '')

    def in_lobby_page(items):
        """是否主大厅:与第二步同一判定——主菜单专属词(仓库/特战干员/
        交易行/改枪台/特勤处)同屏命中≥2,且没有军需处内部的'限购'字样。
        不看部门名(军需处内部也挂着5个部门标签,曾因此误判停在子弹详情页)。"""
        if not items:
            return False
        joined = ' '.join(t for t, _ in items)
        hits = sum(1 for w in _LOBBY_MENU_KW if w in joined)
        return hits >= 2 and '限购' not in joined

    def hover_scroll(clicks, tag):
        '''鼠标移到列表左侧上方(不点击,防误开卡片)后滚轮。clicks<0向下,>0向上。
        ★2026-10-17 修复:移动必须走驱动通道 move_client(与所有点击同一通道)。
        游戏光标只听驱动/原始输入,SetCursorPos只动系统光标——补点兑换路径
        最后一下点击停在详情页「兑换」按钮上(按设计第二轮不再盲点确认),
        之后滚轮全落在详情页,列表永远不滚,剩余目标款永远扫不到
        (用户实测:滚13/20/23格弹名一字不差,照片光标停在兑换按钮)。
        滚动前一律用驱动把光标归位到列表区,任何路径留下的光标位置都能纠正;
        驱动取不到/函数不存在时回落原SetCursorPos行为,不会更差。'''
        ax, ay = scaled(257, 384)
        _moved = False
        try:
            _inp = ctx._get_input(tag)
            if _inp is not None and hasattr(_inp, 'move_client'):
                _inp.move_client(hwnd, ax, ay)
                _moved = True
        except Exception:
            pass
        try:
            sx, sy = win32gui.ClientToScreen(hwnd, (ax, ay))
            win32api.SetCursorPos(sx, sy)
        except Exception:
            pass
        if not _moved:
            log('[军需兑换] 驱动移动不可用,已回落系统级移动(滚动可能落空)', 'WARN')
        time.sleep(0.5)
        for _ in range(abs(clicks)):
            delta = -120 if clicks < 0 else 120
            win32api.mouse_event(win32con.MOUSEEVENTF_WHEEL, 0, 0, delta, 0)
            time.sleep(0.15)
        wait(1500)

    def back_to_list(scrolled_clicks):
        '''兑换结束后状态感知返回列表页。
        返回 (ok, scrolled_clicks): ok=True表示已在列表页;
        scrolled_clicks 为恢复后的已滚格数(误退部门页重进后补滚)。
        '''
        for i in range(3):
            items = ocr_items('回列表确认')
            if not items:
                wait(400)
                continue
            if in_list_page(items):
                return (True, scrolled_clicks)
            if in_department_page(items):
                log('[军需兑换] 检测到误退到部门页,自动重进战术部门并恢复位置', 'WARN')
                click(690, 378, '战术部门')
                end = time.time() + 8
                ok = False
                while time.time() < end:
                    items2 = ocr_items('战术部门列表')
                    if in_list_page(items2):
                        ok = True
                        break
                    wait(500)
                if not ok:
                    log('[军需兑换] 重进战术部门失败', 'WARN')
                    return (False, scrolled_clicks)
                if scrolled_clicks > 0:
                    hover_scroll(-scrolled_clicks, '补滚恢复位置')
                return (True, scrolled_clicks)
            if in_detail_page(items):
                if i < 2:
                    ctx.key_foreground(hwnd, VK_ESC, f'回列表-{i + 1}')
                    wait(800)
                    continue
                log('[军需兑换] 详情页按2次ESC仍未回列表', 'WARN')
                return (False, scrolled_clicks)
            if i < 2:
                ctx.key_foreground(hwnd, VK_ESC, f'回列表-{i + 1}')
                wait(800)
        log('[军需兑换] 回列表失败', 'WARN')
        return (False, scrolled_clicks)

    _已兑章词 = ('已售罄', '售馨', '罄', '更新上架', '更新上', '后更新')

    def exchange_one(name, cx, cy, scrolled_clicks):
        '''点开当前屏子弹卡片并完成兑换;返回 (是否成功, scrolled_clicks)。
        任何结局都经 back_to_list() 状态感知返回列表页。
        ★命中后必须先点击该款子弹卡片:不点击只看列表画面就判定是误判根因。'''
        if cx is None or cy is None:
            log(f'[军需兑换] {name} OCR坐标异常({cx},{cy}),跳过该款', 'WARN')
            return (False, scrolled_clicks)
        ok_list = False
        state = ''
        for attempt in range(2):
            ctx.click(hwnd, cx, cy, f'点击{name}' if not attempt else f'重试点{name}')
            end = time.time() + (6 if not attempt else 4)
            while time.time() < end and not state:
                items = ocr_items('详情页判定')
                if items:
                    joined = ' '.join(t for t, _ in items)
                    if '一键补齐' in joined:
                        state = 'exchange'
                        break
                    if any(w in joined for w in _已兑章词):
                        if in_list_page(items):
                            sold, _st = card_sold_out(items, cx, cy)
                            if sold:
                                state = 'soldout'
                                break
                        else:
                            state = 'soldout'
                            break
                wait(400)
            if state:
                break
            if attempt:
                continue
            log(f'[军需兑换] {name} 详情页未打开,重试点一次', 'WARN')
        if not state:
            log(f'[军需兑换] {name} 详情页打不开,跳过', 'WARN')
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (False, scrolled_clicks)
        if state == 'soldout':
            log(f'[军需兑换] {name} 今日已兑换(售罄章),跳过', 'INFO')
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (False, scrolled_clicks)
        owner_verdict = None
        _nname = _norm_txt(name)
        _oend = time.time() + 2
        while time.time() < _oend:
            _oit = ocr_items('详情页归属')
            _pbullets = [t for t, (_px, _py) in (_oit or []) if not _列表区内(_px) and _是子弹名(t) and '口径' not in t]
            if any(_名匹配(_nname, t) for t in _pbullets):
                owner_verdict = True
                break
            elif _pbullets:
                owner_verdict = False
            wait(300)
        if owner_verdict is False:
            log(f'[军需兑换] {name} 详情页面板显示其他子弹名(点错卡片),放弃兑换 ✗', 'WARN')
            ctx.key_foreground(hwnd, VK_ESC, '点错卡片ESC返回')
            wait(800)
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (False, scrolled_clicks)
        if owner_verdict is None:
            log(f'[军需兑换] {name} 详情页面板名未认出,按原流程继续', 'WARN')
        ctx.raise_if_banned(items, hwnd, ctx.current_username, '军需兑换')
        popup_seen = False
        loss_abort = False
        for _bq in range(2):
            click(886, 695, '一键补齐' if not _bq else '重点一键补齐')
            _pend = time.time() + 4
            while time.time() < _pend:
                wait(300)
                _it = ocr_items('等支付弹窗')
                _joined = ' '.join(t for t, _ in (_it or []))
                if any(w in _joined for w in ('价格更', '参考价格')):
                    loss_abort = True
                    break
                if any(w in _joined for w in ('确认', '支付', '购买', '总价', '单价', '补齐清单')):
                    popup_seen = True
                    break
            if loss_abort or popup_seen:
                break
            if _bq:
                continue
            log(f'[军需兑换] {name} 补齐清单弹窗未认出,重点一键补齐一次', 'WARN')
        if loss_abort:
            log(f'[军需兑换] {name} 检测到亏本警告(交易行参考价格更低),放弃兑换 ✗', 'WARN')
            ctx.key_foreground(hwnd, VK_ESC, '关闭亏本补齐弹窗')
            wait(800)
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (False, scrolled_clicks)
        if check_error_and_dismiss():
            log(f'[军需兑换] {name} 一键补齐失败,跳过', 'WARN')
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (False, scrolled_clicks)
        if not popup_seen:
            _it = ocr_items('盲点支付前亏本复查')
            _joined = ' '.join(t for t, _ in (_it or []))
            if any(w in _joined for w in ('价格更', '参考价格')):
                log(f'[军需兑换] {name} 盲点支付前查出亏本警告,放弃兑换 ✗', 'WARN')
                ctx.key_foreground(hwnd, VK_ESC, '关闭亏本补齐弹窗')
                wait(800)
                ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
                return (False, scrolled_clicks)
            log(f'[军需兑换] {name} 没看清支付弹窗,仍按坐标补点一次', 'WARN')
        click(518, 496, '支付确认')
        wait(600)
        if check_error_and_dismiss():
            log(f'[军需兑换] {name} 材料购买失败(可能哈夫币不足),跳过', 'WARN')
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (False, scrolled_clicks)
        if not wait_exchange_btn(6):
            log(f'[军需兑换] {name} 按钮未变为兑换,跳过', 'WARN')
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (False, scrolled_clicks)
        for _ex in range(2):
            click(886, 695, '兑换' if not _ex else '补点兑换')
            _, kw = wait_for_kws(('确认', '确定'), 5 if not _ex else 4, '兑换确认弹窗')
            if kw:
                click(597, 461, '确认兑换' if not _ex else '补点确认兑换')
            elif not _ex:
                log(f'[军需兑换] {name} 确认弹窗未识别到,盲点确认坐标一次', 'WARN')
                click(597, 461, '盲点确认')
                wait(600)
            success = False
            _pend = time.time() + 5
            while time.time() < _pend:
                wait(500)
                res_items = ocr_items('兑换结果')
                res_joined = ' '.join(t for t, _ in (res_items or []))
                if any(w in res_joined for w in ('售罄', '已兑换', '已达上限', '次数已达', '成功', '获得', '更新上架')):
                    success = True
                    break
            if success:
                log(f'[军需兑换] {name} 兑换成功 ✓' + ('(补点后)' if _ex else ''), 'INFO')
                ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
                return (True, scrolled_clicks)
            if check_error_and_dismiss():
                log(f'[军需兑换] {name} 兑换失败(缺货/涨价等)' + ('(补点后仍被拒)' if _ex else '') + ',跳过', 'WARN')
                ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
                return (False, scrolled_clicks)
            if not _ex:
                log(f'[军需兑换] {name} 兑换结果不明(判点击落空),补点兑换按钮一次', 'WARN')
                continue
            log(f'[军需兑换] {name} 兑换完成 ✓(无错误提示)', 'INFO')
            ok_list, scrolled_clicks = back_to_list(scrolled_clicks)
            return (True, scrolled_clicks)
        return None

    def btn_area_text(items, rx, ry):
        '''读参考坐标按钮区域附近的OCR文本。'''
        if not items:
            return ''
        bx, by = scaled(rx, ry)
        return ' '.join(t for t, (cx, cy) in items if abs(cx - bx) <= 90 and abs(cy - by) <= 45)

    def wait_exchange_btn(timeout=8):
        '''等右下角按钮从「一键补齐」变成「兑换」(=材料购买完成)。True=已可变。
        防线3b(参考第四步种菜流程:一键补齐支付后可能弹「价格变动确认」,
        且可能二次弹):轮询中认到涨价确认弹窗就点确认(认不到按钮盲点597,461),
        点完继续等——涨价弹窗是模态,不处理会挡死按钮永远不变兑换。
        防线3a:补齐清单弹窗仍开着=支付点击落空,补点一次支付(只补一次;
        清单不在时绝不盲点518,496——那里是列表区,空点会误开其他卡片)。
        防线1兜底:清单弹窗若带「参考价格更低」亏本警告,绝不补点直接放弃。'''
        end = time.time() + timeout
        price_confirmed = 0
        repay_done = False
        while time.time() < end:
            items = ocr_items('等兑换按钮')
            joined = ' '.join(t for t, _ in (items or []))
            if any(w in joined for w in ('价格更', '参考价格')):
                log('[军需兑换] 清单弹窗带亏本警告(参考价格更低),放弃该款', 'WARN')
                return False
            if price_confirmed < 2 and any(w in joined for w in ('价格变动', '价格上涨', '涨价', '价格波动')):
                hit = ctx.find_kw(items, ('确认', '确定')) if items else None
                if hit:
                    ctx.click(hwnd, hit[1][0], hit[1][1], '价格变动确认')
                else:
                    click(597, 461, '盲点价格变动确认')
                price_confirmed += 1
                log(f'[军需兑换] 材料价格变动弹窗,已确认({price_confirmed}/2)', 'WARN')
                wait(800)
                continue
            if not repay_done and any(w in joined for w in ('总价', '单价', '补齐清单')):
                repay_done = True
                log('[军需兑换] 支付后清单弹窗仍开着(支付落空),补点支付一次', 'WARN')
                click(518, 496, '补点支付')
                wait(800)
                continue
            txt = _norm_txt(btn_area_text(items, 886, 695))
            if '兑换' in txt and '补齐' not in txt:
                return True
            if ctx.stopped():
                return False
            wait(400)
        return False

    def check_error_and_dismiss():
        items = ocr_items('错误检测')
        if not items:
            return False
        joined = ' '.join(t for t, _ in items)
        err_hit = None
        for kw in ('不足', '失败', '售罄', '缺货', '无法兑换', '已达上限', '次数已达'):
            if kw in joined:
                err_hit = kw
                break
        if not err_hit:
            return False
        if err_hit in ('售罄', '缺货') and '更新上架' in joined:
            log(f'[军需兑换] 「{err_hit}」伴随「更新上架」,识别为成功售罄章,不判错误', 'INFO')
            return False
        hit = ctx.find_kw(items, ('确定', '确认'))
        if hit:
            ctx.click(hwnd, hit[1][0], hit[1][1], '关掉错误提示')
        else:
            ctx.key_foreground(hwnd, VK_ESC, 'ESC关错误提示')
        wait(1200)
        log(f'[军需兑换] 检测到错误提示「{err_hit}」,已关闭', 'WARN')
        return True

    exchanged = 0
    try:
        log('[军需兑换] 步骤1: 进部门', 'INFO')
        click(321, 34, '部门')
        wait(3000)

        log('[军需兑换] 步骤2: 进军需处', 'INFO')
        click(243, 61, '军需处')
        wait(3000)

        log('[军需兑换] 步骤3: 进战术部门', 'INFO')
        ok = False
        for _ in range(2):
            click(690, 378, '战术部门')
            end = time.time() + 10
            while time.time() < end:
                items = ocr_items('战术部门列表')
                if in_list_page(items):
                    ok = True
                    break
                wait(1000)
            if ok:
                break
            log('[军需兑换] 列表未加载,重试点一次', 'WARN')
        if not ok:
            raise RuntimeError('无法进入战术部门列表')

        def _列表弹名数(items):
            return sum(1 for t, _ in (items or []) if _是子弹名(t))

        _稳定最长秒 = 8
        _稳定间隔秒 = 0.8
        _稳定款数门槛 = 10
        _稳定连续次数 = 2
        _cnt = 0
        _last_cnt, _same = (-1, 0)
        _t_end = time.time() + _稳定最长秒
        while time.time() < _t_end:
            _items_st = ocr_items('等列表稳定')
            _cnt = _列表弹名数(_items_st)
            if _cnt >= _稳定款数门槛 and _cnt == _last_cnt:
                _same += 1
                if _same >= _稳定连续次数 - 1:
                    break
            else:
                _same = 0
            _last_cnt = _cnt
            wait(int(_稳定间隔秒 * 1000))
        log(f'[军需兑换] 战术部门列表已稳定(第1屏认出{_cnt}款),开始普查', 'INFO')

        log('[军需兑换] 步骤4: 扫描列表,命中Top5即兑换', 'INFO')
        targets = []
        for row in top5[:5]:
            nm = str(row.get('name') or '').strip()
            if not nm:
                continue
            targets.append((_norm_txt(nm), nm))

        log('[军需兑换] 步骤4a: 全量普查列表(识别所有子弹+Top5落点)', 'INFO')
        survey_names = []
        top5_pos = {}
        survey_scrolled = 0
        _survey_lines = []

        def _survey_screen(screen_tag):
            items = ocr_items(f'普查{screen_tag}')
            screen_names = []
            for t, (cx, cy) in (items or []):
                if not _是子弹名(t):
                    continue
                if t not in screen_names:
                    screen_names.append(t)
                if t not in survey_names:
                    survey_names.append(t)
            for nname, raw in targets:
                if nname in top5_pos:
                    continue
                for t, (cx, cy) in (items or []):
                    if not _名匹配(nname, t):
                        continue
                    if not _列表区内(cx):
                        continue
                    top5_pos[nname] = (screen_tag, cx, cy)
                    break
            _line = f'普查{screen_tag}: {len(screen_names)}款 -> ' + ' | '.join(screen_names[:24])
            _survey_lines.append(_line)
            log(f'[军需兑换] {_line}', 'INFO')
            return len(screen_names)

        first_screen_count = _survey_screen('第1屏')
        for seg in SCROLL_SEQUENCE:
            if ctx.stopped() or time.time() > deadline:
                break
            hover_scroll(-seg, f'普查向下滚{seg}格')
            survey_scrolled += seg
            _survey_screen(f'累计{survey_scrolled}格')
        log(f'[军需兑换] 普查完成: 列表共识别{len(survey_names)}款子弹', 'INFO')
        _survey_lines.append(f'普查完成: 共识别{len(survey_names)}款')
        if survey_scrolled > 0:
            hover_scroll(survey_scrolled + 2, f'普查后回顶(上滚{survey_scrolled + 2}格)')

        if first_screen_count == 0:
            log('[军需兑换] 普查第1屏为0款,回顶后启动第1屏兜底补扫...', 'WARN')
            _last_cnt, _same = (-1, 0)
            _items_b = None
            _t_end = time.time() + _稳定最长秒
            while time.time() < _t_end:
                _items_b = ocr_items('第1屏兜底补扫')
                _cnt = _列表弹名数(_items_b)
                if _cnt >= _稳定款数门槛 and _cnt == _last_cnt:
                    _same += 1
                    if _same >= _稳定连续次数 - 1:
                        break
                else:
                    _same = 0
                _last_cnt = _cnt
                wait(int(_稳定间隔秒 * 1000))
            _extra = []
            if _items_b:
                for t, (cx, cy) in _items_b:
                    if not _是子弹名(t):
                        continue
                    if t not in survey_names:
                        survey_names.append(t)
                        _extra.append(t)
            for nname, raw in targets:
                if nname in top5_pos:
                    continue
                for t, (cx, cy) in _items_b:
                    if not _名匹配(nname, t):
                        continue
                    if not _列表区内(cx):
                        continue
                    top5_pos[nname] = ('第1屏兜底补扫', cx, cy)
                    break
            log(f'[军需兑换] 兜底补扫完成: 第1屏认出{_列表弹名数(_items_b)}款,新增{len(_extra)}款',
                'WARN' if _extra else 'INFO')
            _survey_lines.append(f'第1屏兜底补扫: 认出{_列表弹名数(_items_b)}款,新增{len(_extra)}款 -> '
                                 + ' | '.join(_extra[:24]))

        for nname, raw in targets:
            if nname in top5_pos:
                _st, _cx, _cy = top5_pos[nname]
                _line = f'  {raw} ✓ 在列表(普查{_st}) @({_cx},{_cy})'
                log(f'[军需兑换] {_line}', 'INFO')
            else:
                _line = f'  {raw} ✗ 不在列表,本轮跳过该款'
                log(f'[军需兑换] {_line}', 'WARN')
            _survey_lines.append(_line)

        _logdir = os.path.join(os.path.dirname(_local_db()), '运行日志')
        os.makedirs(_logdir, exist_ok=True)
        try:
            with open(os.path.join(_logdir, '军需普查.log'), 'a', encoding='utf-8') as _f:
                _f.write(f'\n===== {time.strftime("%m-%d %H:%M:%S")} 账号={ctx.current_username} =====\n')
                for _line in _survey_lines:
                    _f.write(_line + '\n')
        except Exception:
            pass

        _before = len(targets)
        targets = [(n, r) for n, r in targets if n in top5_pos]
        if len(targets) < _before:
            log(f'[军需兑换] {_before - len(targets)}款不在列表已剔除,剩余{len(targets)}款待处理', 'WARN')
        if not targets:
            log('[军需兑换] Top5全部不在列表,本轮无款可换', 'WARN')

        processed = set()
        scrolled_clicks = 0

        def process_current_screen(screen_tag=''):
            '''处理当前屏:循环找未处理的Top5子弹并兑换,直到本屏没有可处理的。
            返回本屏处理款数。已售罄卡片在列表上直接认出、不点入。'''
            nonlocal scrolled_clicks, exchanged
            n = 0
            debug_logged = False
            while len(processed) < len(targets):
                if ctx.stopped() or time.time() > deadline:
                    break
                items = ocr_items('扫描列表')
                if not items:
                    break
                try:
                    if not debug_logged:
                        debug_logged = True
                        names = [t for t, _ in items if _是子弹名(t)]
                        log(f'[军需兑换] {screen_tag}本屏识别到{len(names)}个弹名: '
                            + ' | '.join(names[:24]), 'INFO')
                except Exception:
                    pass
                hit = None
                for nname, raw in targets:
                    if nname in processed:
                        continue
                    for t, (cx, cy) in items:
                        if not _名匹配(nname, t):
                            continue
                        if not _列表区内(cx):
                            continue
                        hit = (nname, raw, cx, cy)
                        break
                    if hit:
                        break
                if not hit:
                    break
                nname, name, cx, cy = hit
                log(f'[军需兑换] 命中[{len(processed) + 1}/{len(targets)}] {name} @({cx},{cy})', 'INFO')
                ok_x, scrolled_clicks = exchange_one(name, cx, cy, scrolled_clicks)
                if ok_x:
                    exchanged += 1
                processed.add(nname)
                n += 1
            return n

        process_current_screen('第1屏 ')
        for seg in SCROLL_SEQUENCE:
            if len(processed) >= len(targets):
                log('[军需兑换] 五款已全部处理完,提前结束扫描', 'INFO')
                break
            if ctx.stopped():
                log('[军需兑换] 收到停止,中断兑换', 'WARN')
                break
            if time.time() > deadline:
                log('[军需兑换] 超出时间预算,停止扫描', 'WARN')
                break
            hover_scroll(-seg, f'向下滚{seg}格')
            scrolled_clicks += seg
            process_current_screen(f'累计滚{scrolled_clicks}格后 ')
        total = len(targets)
        log(f'[军需兑换] 扫描完成: 处理 {len(processed)}/{total} 款,成功 {exchanged} 款',
            'INFO' if exchanged else 'WARN')
    finally:
        try:
            log('[军需兑换] 收尾: 退回大厅', 'INFO')
            reached = False
            for i in range(6):
                items = ocr_items('回大厅确认')
                if items and in_lobby_page(items):
                    log('[军需兑换] 已确认回到主大厅 ✓', 'INFO')
                    reached = True
                    break
                elif i >= 5:
                    break
                else:
                    ctx.key_foreground(hwnd, VK_ESC, f'ESC回大厅-{i + 1}')
                    wait(1200)
            if not reached:
                log('[军需兑换] 收尾未能确认大厅,交易行前还有兜底确认', 'WARN')
        except Exception:
            pass


def _fetch_ammo_exchange_top5(log):
    """从云端拉取战术部门子弹利润 Top5。"""
    try:
        cfg_path = os.path.join(os.path.expanduser('~'), '.pc_tqc_client', 'config.json')
        if not os.path.exists(cfg_path):
            log('[军需兑换] 无云端配置文件,跳过', 'WARN')
            return
        else:
            with open(cfg_path, 'r', encoding='utf-8-sig') as f:
                cfg = json.load(f)
            server_url = cfg.get('server_url') or cfg.get('cloud_url') or ''
            token = cfg.get('token') or ''
            if not server_url or not token:
                log('[军需兑换] 云端配置缺少 server_url/token,跳过', 'WARN')
                return
            else:
                url = f"{server_url.rstrip('/')}/api/admin/ammo-exchange-rec"
                req = urllib.request.Request(url, headers={'Authorization': f'Bearer {token}', 'User-Agent': 'Mozilla/5.0 Chrome/130'})
                with urllib.request.urlopen(req, timeout=15) as r:
                    d = json.loads(r.read().decode('utf-8'))
                if not d.get('ok'):
                    log('[军需兑换] 云端返回错误', 'WARN')
                    return
                else:
                    data = d.get('data') or {}
                    rows = data.get('战术部门') or []
                    if not rows:
                        log('[军需兑换] 云端无战术部门数据', 'WARN')
                        return
                    else:
                        return rows[:5]
    except Exception as e:
        log(f'[军需兑换] 拉云端排行失败: {type(e).__name__}: {e}', 'WARN')
        return None


def _find_game_hwnd():
    """查找游戏窗口句柄。"""
    try:
        import ctypes
        import ctypes.wintypes as wintypes
        user32 = ctypes.windll.user32
        kernel32 = ctypes.windll.kernel32
        result = []
        def enum_callback(hwnd, _):
            if not user32.IsWindowVisible(hwnd):
                return True
            else:
                pid = wintypes.DWORD()
                user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                try:
                    hproc = kernel32.OpenProcess(1040, False, pid.value)
                    if hproc:
                        buf = ctypes.create_unicode_buffer(260)
                        size = wintypes.DWORD(260)
                        kernel32.QueryFullProcessImageNameW(hproc, 0, buf, ctypes.byref(size))
                        kernel32.CloseHandle(hproc)
                        path = buf.value
                        for proc in ['DeltaForceClient-Win64-Shipping.exe', 'DeltaForceClient.exe']:
                            if proc.lower() in path.lower():
                                result.append(hwnd)
                                return False
                except Exception:
                    return True
                return True
        enum_proc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)(enum_callback)
        user32.EnumWindows(enum_proc, 0)
        return result[0] if result else None
    except Exception:
        return None


def _is_main_lobby(items):
    """是否主大厅:5个主菜单专属词同屏命中≥2,且没有军需处内部\'限购\'字样。"""
    if not items:
        return False
    else:
        joined = ' '.join((t for t, _ in items))
        hits = sum((1 for w in _LOBBY_MENU_KW if w in joined))
        return hits >= 2 and '限购' not in joined


def _ensure_in_lobby(ctx, log, max_esc=5):
    """交易行开始前的双保险:不在主大厅就逐次ESC,认到大厅立刻停。\n    返回 True=已确认大厅;False=未能确认(调用方仍按原流程走,不抛异常)。"""
    try:
        hwnd = _find_game_hwnd()
        if not hwnd:
            log('[军需兑换] 交易行前找不到游戏窗口,大厅确认跳过', 'WARN')
            return False
        else:
            for i in range(max_esc + 1):
                frame = ctx.grab_frame(hwnd, '交易行前大厅确认')
                items = ctx.ocr_scan(frame, '交易行前大厅确认') if frame is not None else None
                if items and _is_main_lobby(items):
                    if i > 0:
                        log(f'[军需兑换] 第{i}次ESC后已确认回到主大厅 ✓', 'INFO')
                    return True
                else:
                    if i >= max_esc:
                        break
                    else:
                        ctx.key_foreground(hwnd, VK_ESCAPE, f'交易行前ESC回大厅-{i + 1}')
                        time.sleep(1.2)
            log('[军需兑换] 交易行前未能确认回到大厅(仍放行原流程)', 'WARN')
    except Exception as e:
        log(f'[军需兑换] 交易行前大厅确认异常(忽略): {type(e).__name__}: {e}', 'WARN')
        return False
    return False


def _run_ammo_exchange(ctx, log):
    """军需处子弹兑换主流程。"""
    uname = ctx.current_username
    if not uname:
        log('[军需兑换] 无当前账号名,跳过', 'WARN')
        return
    else:
        if _is_ammo_exchanged_today(uname):
            log(f'[军需兑换] 账号 {uname} 今日已兑换,跳过', 'INFO')
            return
        else:
            top5 = _fetch_ammo_exchange_top5(log)
            if not top5:
                log('[军需兑换] 拉取云端排行失败,跳过', 'WARN')
                return
            else:
                log(f"[军需兑换] 云端Top5: {[r['name'] for r in top5]}", 'INFO')
                hwnd = _find_game_hwnd()
                if not hwnd:
                    log('[军需兑换] 找不到游戏窗口,跳过', 'WARN')
                    return
                else:
                    try:
                        _do_ammo_exchange(ctx, hwnd, top5, log)
                        _mark_ammo_exchanged_today(uname)
                        log(f'[军需兑换] 账号 {uname} 兑换完成', 'INFO')
                    except Exception as e:
                        log(f'[军需兑换] 兑换失败(不标记今日已兑,下轮自动重试): {type(e).__name__}: {e}', 'WARN')
                        return None


def _market_ok_result(msg, details=None):
    """构造与引擎 StepResult 同构的\"阶段正常完成\"结果。\n\n    调用方(引擎 runner.pyd)按 StepResult 契约消费 MarketSeller.run 的返回值:\n    读 .ok(=status==成功)/.details/.message/.elapsed_seconds 等字段\n    (runner.pyd 二进制内含全部 StepResult 属性名字符串,实证)。\n    跳过出售绝不能 return None——否则引擎读 None.ok 抛 AttributeError,\n    任务被判失败、流程走不到收尾(游戏停在特勤处不退出)。\n    首选引擎真实 StepResult(status=RunStatus.成功),构造失败退回\n    duck-typing 兜底对象(与第二步 _OkResult 同款,现网验证过的接口)。"""
    try:
        from pc_tqc.models import StepResult as _SR, RunStatus as _RS, WorkflowStage as _WS
        return _SR(status=_RS.SUCCESS, stage=_WS.MARKET, message=msg, details=details or {})
    except Exception:
        class _Ok:
            ok = True
            error = None
            status = '成功'
            stage = '交易行出售'
            elapsed_seconds = 0.0
            attempts = 0
            def __init__(self, message='', details=None):
                self.message = message
                self.details = details or {}
                self.results = []
            def __getattr__(self, name):
                return {}
        return _Ok(msg, details)


def install(cls, ctx):
    """总管调用:把军需兑换挂到【交易行阶段入口】(MarketSeller.run)。"""
    try:
        from pc_tqc.workflow import market as _market_mod
        def _压制重试(self, args, kwargs):
            """验证码命中→把该账号引擎重试时间推到6小时外(同第四步实锤版)。\n            引擎滚动调度器读条目上的 failed_retry_after 动态属性(缺省≈180s\n            setdefault);state.attr/__dict__ 与 runner.states 条目全覆盖。\n            MarketSeller 无 states,从 self 常见属性名找 runner 引用。"""
            # ***<module>.install._压制重试: Failure: Different control flow
            _far = time.time() + 21600
            _far_str = time.strftime('%H:%M:%S', time.localtime(_far))
            ok = []
            def _set_dict(d):
                try:
                    d['failed_retry_after'] = _far
                except Exception:
                    return False
                return True
            def _set_attr(o):
                try:
                    o.failed_retry_after = _far
                except Exception:
                    return False
                return True
            state = kwargs.get('state')
            if state is None:
                for a in args:
                    if hasattr(a, 'account'):
                        state = a
                        break
            username = ''
            if state is not None:
                try:
                    username = state.account.username or ''
                except Exception:
                    username = ''
                if _set_attr(state):
                    ok.append('state.attr')
                try:
                    if _set_dict(state.__dict__):
                        ok.append('state.__dict__')
                except Exception:
                    pass
            runner = None
            for attr in ['runner', '_runner', 'account_runner', '_account_runner']:
                r = getattr(self, attr, None)
                if r is not None and hasattr(r, 'states'):
                        runner = r
                        break
            if runner is not None:
                states = getattr(runner, 'states', None)
                try:
                    if isinstance(states, dict):
                        e = states.get(username) if username else None
                        if e is not None and e is not state and (_set_attr(e) or _set_dict(e)):
                                    ok.append(f'states[{username}]')
                    else:
                        if states is not None:
                            for e in states:
                                if e is state:
                                    continue
                                else:
                                    try:
                                        nm = getattr(getattr(e, 'account', None), 'username', None)
                                    except Exception:
                                        pass
                                    else:
                                        if nm and nm == username:
                                                hit = _set_attr(e)
                                                try:
                                                    hit = _set_dict(e.__dict__) or hit
                                                except Exception:
                                                    pass
                                                if hit:
                                                    ok.append(f'states条目[{username}]')
                except Exception:
                    pass
            if ok:
                ctx.log(STEP_NAME, f"验证码压制:该账号重试时间已推至 {_far_str}(6小时外),写入位置={','.join(ok)}", 'WARN')
            else:
                ctx.log(STEP_NAME, '验证码压制:交易行阶段未找到可写位置,依赖客户端停止兜底', 'WARN')
        def _make_market(original):
            def wrapped(self, *args, **kwargs):
                _cfg = _read_worker_cfg()
                _ammo_on = _cfg_bool(_cfg.get('ammo_exchange_enabled'), False)
                if _ammo_on:
                    try:
                        _run_ammo_exchange(ctx, lambda text, level='INFO': ctx.log('军需兑换', text, level))
                    except Exception as _ae:
                        ctx.log(STEP_NAME, f'军需兑换异常(已忽略,不影响出售): {type(_ae).__name__}: {_ae}', 'WARN')
                    else:
                        pass
                else:
                    ctx.log(STEP_NAME, '军需处子弹兑换已关闭(界面开关),跳过', 'INFO')
                try:
                    _ensure_in_lobby(ctx, lambda text, level='INFO': ctx.log('军需兑换', text, level))
                except Exception as _le:
                    ctx.log(STEP_NAME, f'交易行前大厅确认异常(已忽略): {type(_le).__name__}: {_le}', 'WARN')
                _sell_win_on = _cfg_bool(_cfg.get('sell_window_enabled'), False)
                if _sell_win_on:
                    _sw_start = str(_cfg.get('sell_window_start', '14:00'))
                    _sw_end = str(_cfg.get('sell_window_end', '20:00'))
                    if not _is_in_time_window(_sw_start, _sw_end):
                        ctx.log(STEP_NAME, f'出售时段限制启用({_sw_start}-{_sw_end}),当前不在时段内,跳过出售', 'INFO')
                        return _market_ok_result(f'出售时段({_sw_start}-{_sw_end})外,按计划跳过出售(阶段正常完成)', {'skipped': 'sell_window', 'sell_window': f'{_sw_start}-{_sw_end}'})
                try:
                    _result = original(self, *args, **kwargs)
                except Exception as _se:
                    if getattr(ctx, '_captcha_hit', None):
                        from workflow_steps import CaptchaError as _CE
                        _压制重试(self, args, kwargs)
                        ctx.log(STEP_NAME, '该账号出现了验证码弹窗,判定失败,切下一个账号', 'ERROR')
                        raise _CE('该账号出现了验证码弹窗') from _se
                    else:
                        raise
                if getattr(ctx, '_captcha_hit', None):
                    from workflow_steps import CaptchaError as _CE
                    _压制重试(self, args, kwargs)
                    ctx.log(STEP_NAME, '该账号出现了验证码弹窗(阶段返回兜底),判定失败,切下一个账号', 'ERROR')
                    raise _CE('该账号出现了验证码弹窗')
                else:
                    return _result
            return wrapped
        _ui = _read_worker_cfg()
        _ammo_s = '开' if _cfg_bool(_ui.get('ammo_exchange_enabled'), False) else '关'
        if _cfg_bool(_ui.get('sell_window_enabled'), False):
            _sell_s = '开(%s-%s)' % (str(_ui.get('sell_window_start', '14:00')), str(_ui.get('sell_window_end', '20:00')))
        else:
            _sell_s = '关'
        ctx.log(STEP_NAME, f'界面开关: 子弹兑换={_ammo_s}, 出售时段={_sell_s}(未配置按关闭处理)')
        ctx.patch_with(_market_mod.MarketSeller, 'run', _make_market)
        ctx.log(STEP_NAME, '军需兑换已挂载:交易行阶段入口(收菜后、出售前)')
    except Exception as e:
        ctx.log(STEP_NAME, f'军需兑换挂载交易行失败: {type(e).__name__}: {e}', 'WARN')


def _local_db():
    """本地调度库路径。"""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, 'Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders') as key:
            desktop = os.path.expandvars(str(winreg.QueryValueEx(key, 'Desktop')[0]).strip())
            if desktop:
                return os.path.join(desktop, 'PC特勤处', 'pc_tqc.db')
    except Exception:
        pass
    return os.path.join(os.path.expanduser('~'), 'Desktop', 'PC特勤处', 'pc_tqc.db')