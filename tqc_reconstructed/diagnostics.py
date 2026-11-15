# diagnostics.py — 不可恢复异常的截图留证（重建版：pyc 字节码直接还原）
# 来源：x/engine_pyz/pc_tqc__diagnostics.pyc（Python 3.12 原生字节码），
# safe_filename_part / save_frame / save_failure_frame 全部 dis 反汇编逐行还原。

'''不可恢复异常的截图留证。

正常流程不落盘；只有检查点回退与有限重试都失败时，才把客户区画面保存到
桌面 ``PC特勤处\\异常处理图片`` 目录。

职责：承担“diagnostics.py”对应的单一功能。
边界：本模块属于应用基础层；修改公开接口时需同步调用方、交接文档与对应测试。
'''
from __future__ import annotations
import re
from datetime import datetime
from pathlib import Path
import cv2
import numpy as np
from pc_tqc.config import diagnostic_image_dir


def safe_filename_part(value):
    cleaned = re.sub(r'[<>:"/\\|?*\x00-\x1f]+', '_', value.strip())
    return cleaned.strip(' ._')[:80] or 'unknown'


class DiagnosticWriter:
    def __init__(self, root=None):
        self.root = root

    def save_frame(self, image, prefix):
        del image
        del prefix

    def save_failure_frame(self, image, prefix):
        """仅为需要人工排查的关键错误保存一张桌面截图。"""
        if image is None or image.ndim < 2 or image.size == 0:
            return None
        try:
            destination = self.root or diagnostic_image_dir()
            destination.mkdir(parents=True, exist_ok=True)
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S_%f')[:-3]
            filename = f'PC特勤处_异常_{safe_filename_part(prefix)}_{timestamp}.png'
            path = destination / filename
            frame = np.ascontiguousarray(image[:, :, :3] if image.ndim == 3 else image)
            (ok, encoded) = cv2.imencode('.png', frame)
            if not ok:
                return None
            path.write_bytes(encoded.tobytes())
            return path
        except (OSError, cv2.error, ValueError):
            return None
