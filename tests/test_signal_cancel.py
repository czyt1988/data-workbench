"""DASignalAnalysis 取消链路压测（阶段 D2 / 方案 §8.2「取消」行）。

覆盖：
- 连续取消 5 次：频谱 / STFT / 峰值表三条异步链路，取消后无残留任务、无结果表滞留；
- 取消幂等：同一任务重复取消、未知任务取消；
- 内存不增长：连续 5 次取消后进程 RSS 增量在阈值内（实测值打印在输出里）。

为什么不用「起任务立刻取消」：任务可能瞬间跑完，测试会抖动。这里用 monkeypatch 给
各模块的 thread_status_manager 代理注入每块 20ms 延迟，让取消点确定落在分块循环中间。
"""

import gc
import os
import sys
import threading
import time
import wave

import numpy as np
import pytest

# 把插件 Python 包加入 sys.path（conftest 只加了 src/PyScripts）
_PLUGIN_PY = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "plugins", "DASignalAnalysis", "PyScripts",
)
if _PLUGIN_PY not in sys.path:
    sys.path.insert(0, _PLUGIN_PY)

from DASignalAnalysis import peaks, spectrum, stft  # noqa: E402
from DAWorkbench.DAPyBase import thread_status_manager as tsm  # noqa: E402


pytestmark = pytest.mark.signal_cancel

# 连续取消次数（方案 §8.2/§6 D2 的要求）
CANCEL_ROUNDS = 5
# 每块注入延迟（秒），保证取消请求落在分块循环中间而不是任务结束后
BLOCK_DELAY_S = 0.05
# 单次取消后等待任务线程收敛的上限
WAIT_TIMEOUT_S = 20.0
# 连续 5 次取消后的 RSS 增量上限（正常应在个位数 MB；泄漏整段信号时每次 ≥ 信号大小）
RSS_GROWTH_LIMIT_MB = 64.0


def _write_wav(path, fs=48000, seconds=30.0, freq=1000.0, channels=1):
    """用标准库写 16 位 WAV，避免测试依赖 soundfile。"""
    n = int(fs * seconds)
    t = np.arange(n) / float(fs)
    x = 0.5 * np.sin(2.0 * np.pi * freq * t)
    pcm = np.clip(x * 32767.0, -32768, 32767).astype("<i2")
    with wave.open(str(path), "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(2)
        w.setframerate(fs)
        w.writeframes(pcm.tobytes())


def _params(**overrides):
    params = {
        "removeDc": False,
        "detrend": "none",
        "window": "hann",
        "amplitudeMode": "peak",
        "dbReference": "fullScale",
        "resolutionHz": 10.0,
        "zeroPadding": 0,
        "average": "mean",
        "overlap": 50.0,
        "stftWindowMs": 20.0,
        "stftOverlap": 50.0,
        "stftZeroPadding": 0,
        "peakMode": "topN",
        "peakTopN": 10,
        "peakRelDb": 40.0,
        "peakProminence": 0.0,
        "peakMinSpacingHz": 50.0,
        "peakMinHeight": 0.0,
    }
    params.update(overrides)
    return params


class _SlowTsm:
    """给模块的 tsm 代理注入延迟：每块先睡一下再检查取消标志。

    只覆盖 raise_if_canceled（分块循环唯一检查点），其余接口转发给真实模块。
    """

    def __init__(self, delay_s):
        self._delay_s = delay_s

    def raise_if_canceled(self, task_id):
        time.sleep(self._delay_s)
        tsm.raise_if_canceled(task_id)

    def __getattr__(self, name):
        return getattr(tsm, name)


def _wait_finished(task_id, timeout=WAIT_TIMEOUT_S):
    deadline = time.time() + timeout
    while time.time() < deadline:
        info = tsm.get_task_status(task_id)
        if info is not None and not info["is_running"]:
            return info
        time.sleep(0.01)
    raise AssertionError("task %s did not finish within %.0fs" % (task_id, timeout))


def _wait_python_threads_back(baseline, timeout=10.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if threading.active_count() <= baseline:
            return
        time.sleep(0.01)
    raise AssertionError(
        "daemon worker threads still alive: baseline=%d current=%d" % (baseline, threading.active_count())
    )


def _cancel_once(async_start, module, monkeypatch, delay_s=BLOCK_DELAY_S):
    """起一个异步任务 → 100ms 后取消 → 断言取消态且无结果残留。"""
    baseline_threads = threading.active_count()
    monkeypatch.setattr(module, "tsm", _SlowTsm(delay_s))
    task_id = async_start()
    assert task_id
    time.sleep(0.1)
    assert tsm.cancel_task(task_id, "stress") is True
    info = _wait_finished(task_id)
    assert info["is_canceled"] is True, info
    assert info["is_success"] is False, info
    # 取消后结果表必须没有该任务（大数组不滞留）
    assert module.take_result(task_id) is None
    _wait_python_threads_back(baseline_threads)
    return task_id


def test_cancel_spectrum_five_times_no_residue(tmp_path, monkeypatch):
    """连续取消 5 次频谱任务：任务表/结果表/线程都收敛，无残留。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav)
    source = {"type": "file", "path": str(wav), "channel": 0, "sampleRate": 48000, "sampleCount": 1440000}

    assert tsm.get_task_count()[1] == 0  # 干净起点（active==0）
    for i in range(CANCEL_ROUNDS):
        _cancel_once(lambda: spectrum.compute_spectrum_async(source, _params()), spectrum, monkeypatch)
        # 每轮后结果表都应为空、没有活跃任务
        assert spectrum._RESULT_STORE == {}, "spectrum result store leaked after round %d" % i
        assert tsm.get_task_count()[1] == 0, "active background tasks after round %d" % i
    # 压测结束后整体仍是干净状态
    assert tsm.get_task_count()[1] == 0


def test_cancel_stft_five_times_no_residue(tmp_path, monkeypatch):
    """连续取消 5 次 STFT 任务：无结果残留、无线程残留。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav)
    source = {"type": "file", "path": str(wav), "channel": 0, "sampleRate": 48000, "sampleCount": 1440000}

    for i in range(CANCEL_ROUNDS):
        _cancel_once(lambda: stft.compute_heatmap_async(source, _params(), 400, 200), stft, monkeypatch)
        assert stft._RESULT_STORE == {}, "stft result store leaked after round %d" % i
        assert tsm.get_task_count()[1] == 0, "active background tasks after round %d" % i


def test_cancel_peaks_five_times_no_residue(tmp_path, monkeypatch):
    """连续取消 5 次峰值表任务：无结果残留、无线程残留。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav)
    source = {"type": "file", "path": str(wav), "channel": 0, "sampleRate": 48000, "sampleCount": 1440000}

    for i in range(CANCEL_ROUNDS):
        _cancel_once(lambda: peaks.compute_peaks_async(source, _params(resolutionHz=100.0)), peaks, monkeypatch)
        assert peaks._RESULT_STORE == {}, "peaks result store leaked after round %d" % i
        assert tsm.get_task_count()[1] == 0, "active background tasks after round %d" % i


def test_cancel_all_task_kinds_no_memory_growth(tmp_path, monkeypatch):
    """三种任务各跑 5 轮取消后，进程 RSS 增量受控（无整段信号/结果矩阵滞留）。"""
    psutil = pytest.importorskip("psutil")
    wav = tmp_path / "tone.wav"
    _write_wav(wav)
    source = {"type": "file", "path": str(wav), "channel": 0, "sampleRate": 48000, "sampleCount": 1440000}
    proc = psutil.Process(os.getpid())

    def _one_round():
        _cancel_once(lambda: spectrum.compute_spectrum_async(source, _params()), spectrum, monkeypatch)
        monkeypatch.setattr(stft, "tsm", _SlowTsm(BLOCK_DELAY_S))
        _cancel_once(lambda: stft.compute_heatmap_async(source, _params(), 400, 200), stft, monkeypatch)
        monkeypatch.setattr(peaks, "tsm", _SlowTsm(BLOCK_DELAY_S))
        _cancel_once(lambda: peaks.compute_peaks_async(source, _params(resolutionHz=100.0)), peaks, monkeypatch)

    _one_round()  # 预热：分配器/线程池等一次性开销不计入增量
    gc.collect()
    rss_before = proc.memory_info().rss
    for _ in range(CANCEL_ROUNDS):
        _one_round()
    gc.collect()
    rss_after = proc.memory_info().rss
    growth_mb = (rss_after - rss_before) / (1024.0 * 1024.0)
    print("\n[RSS] before=%.1f MB after=%.1f MB growth=%.1f MB" % (
        rss_before / 1048576.0, rss_after / 1048576.0, growth_mb))
    assert growth_mb < RSS_GROWTH_LIMIT_MB, (
        "process RSS grew %.1f MB after %d cancel rounds (limit %.1f MB)" % (
            growth_mb, CANCEL_ROUNDS, RSS_GROWTH_LIMIT_MB)
    )
    assert spectrum._RESULT_STORE == {} and stft._RESULT_STORE == {} and peaks._RESULT_STORE == {}
    assert tsm.get_task_count()[1] == 0


def test_cancel_task_idempotent():
    """取消幂等：重复取消返回 True；未知任务返回 False；取消后仍保持取消态。"""
    task_id, status = tsm.create_task_with_status("idempotent cancel")
    status.start("idempotent cancel")
    assert tsm.cancel_task(task_id, "first") is True
    assert tsm.cancel_task(task_id, "second") is True  # 重复取消不报错
    assert tsm.is_task_canceled(task_id) is True
    assert tsm.cancel_task("no-such-task-id") is False
    tsm.finish_task(task_id, False, "cancelled")
