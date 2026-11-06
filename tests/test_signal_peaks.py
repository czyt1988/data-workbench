"""DASignalAnalysis 峰值查找 / 峰值曲线测试（方案 §6.4 / §6.5，评审 B1 / C5 / C7）。

覆盖：
- compute_peak_curves_sync 的返回键契约（含 mag_by_rank，B1）与矩阵形状；
- 频率上扫时第 1 名频率单调上升、幅值非 NaN；
- 某帧不足 N 个峰时对应名次为 NaN；
- 默认最小间距按实际频率分辨率（C5）；
- 零填充按「额外点数」理解（C3）；
- 分块循环取消（C7）与被取消任务不写结果表。
"""

import os
import sys
import threading
import time

import numpy as np
import pytest

# 把插件 Python 包加入 sys.path（conftest 只加了 src/PyScripts）
_PLUGIN_PY = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "plugins", "DASignalAnalysis", "PyScripts",
)
if _PLUGIN_PY not in sys.path:
    sys.path.insert(0, _PLUGIN_PY)

from DASignalAnalysis import peaks  # noqa: E402
from DAWorkbench.DAPyBase import thread_status_manager as tsm  # noqa: E402


pytestmark = pytest.mark.signal_peaks

FS = 48000.0


def _write_wav(path, data, fs=FS):
    import soundfile as sf

    sf.write(str(path), np.asarray(data, dtype=np.float32), int(fs))


def _tone(seconds=1.0, freq=1000.0, amp=0.5):
    t = np.arange(int(FS * seconds)) / FS
    return amp * np.sin(2 * np.pi * freq * t)


def _sweep(seconds=1.0, f0=500.0, f1=2500.0, amp=0.5):
    t = np.arange(int(FS * seconds)) / FS
    phase = 2 * np.pi * (f0 * t + 0.5 * (f1 - f0) / seconds * t * t)
    return amp * np.sin(phase)


def _curve_params(**overrides):
    params = {
        "stftWindowMs": 20.0,
        "stftOverlap": 50.0,
        "window": "hann",
        "peakTopN": 1,
        "peakRelDb": 40.0,
        "peakMode": "simple",
        "removeDc": False,
        "detrend": "none",
    }
    params.update(overrides)
    return params


def _table_params(**overrides):
    params = {
        "window": "hann",
        "amplitudeMode": "peak",
        "resolutionHz": 10.0,
        "zeroPadding": 0,
        "overlap": 50.0,
        "average": "mean",
        "peakMode": "simple",
        "peakTopN": 5,
        "peakRelDb": 80.0,
        "removeDc": False,
        "detrend": "none",
    }
    params.update(overrides)
    return params


# ---------------------------------------------------------------- 返回契约（B1）


def test_peak_curves_return_contract(tmp_path):
    wav = tmp_path / "sweep.wav"
    _write_wav(wav, _sweep())
    src = {"type": "file", "path": str(wav), "sampleRate": FS}
    r = peaks.compute_peak_curves_sync(src, _curve_params(peakTopN=3))

    # B1：C++ 侧按这七个键读取（DASignalAnalysisWorker.cpp）
    assert set(r.keys()) == {
        "times",
        "freq_by_rank",
        "mag_by_rank",
        "top_n",
        "n_frames",
        "nperseg",
        "hop",
    }
    assert r["top_n"] == 3
    assert r["n_frames"] > 10
    assert r["nperseg"] == 960  # 20 毫秒 × 48 千赫
    assert r["hop"] == 480
    assert r["times"].shape == (r["n_frames"],)
    # 两个排名矩阵形状一致，长度都等于 top_n × n_frames
    assert r["freq_by_rank"].shape == (3, r["n_frames"])
    assert r["mag_by_rank"].shape == (3, r["n_frames"])
    assert r["freq_by_rank"].size == r["mag_by_rank"].size == 3 * r["n_frames"]
    # 频率与幅值的缺失位置必须一一对应
    assert np.array_equal(np.isfinite(r["freq_by_rank"]), np.isfinite(r["mag_by_rank"]))


def test_peak_curves_sweep_first_rank_increases(tmp_path):
    wav = tmp_path / "sweep.wav"
    _write_wav(wav, _sweep())
    src = {"type": "file", "path": str(wav), "sampleRate": FS}
    r = peaks.compute_peak_curves_sync(src, _curve_params(peakTopN=1))

    curve = r["freq_by_rank"][0]
    valid_idx = np.isfinite(curve)
    valid = curve[valid_idx]
    assert valid.size > 10
    n = valid.size
    # 第 1 名频率随时间上升（用分位比较，容忍个别帧抖动）
    assert np.median(valid[-n // 4 :]) > np.median(valid[: n // 4]) + 1000.0
    assert np.mean(np.diff(valid) >= 0) > 0.9
    # 对应幅值非 NaN，且接近合成信号幅度 0.5
    mags = r["mag_by_rank"][0][valid_idx]
    assert np.all(np.isfinite(mags))
    assert np.median(mags) == pytest.approx(0.5, rel=0.2)


def test_peak_curves_missing_ranks_are_nan(tmp_path):
    """某帧不足 N 个峰时对应名次为 NaN（单音信号只会命中第 1 名）。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav, _tone())
    src = {"type": "file", "path": str(wav), "sampleRate": FS}
    r = peaks.compute_peak_curves_sync(src, _curve_params(peakTopN=10, peakRelDb=40.0))

    assert np.all(np.isfinite(r["freq_by_rank"][0]))
    assert np.all(np.isfinite(r["mag_by_rank"][0]))
    # 最后一个名次在所有帧都没有峰
    assert np.all(np.isnan(r["freq_by_rank"][-1]))
    assert np.all(np.isnan(r["mag_by_rank"][-1]))


# ---------------------------------------------------------------- 峰值表口径（C5 / C3 / C4）


def test_compute_peaks_default_spacing_uses_actual_resolution(tmp_path):
    """C5：默认最小间距按实际 Δf×5，20 赫兹双音可分辨。

    resolutionHz=2 时 nperseg=24000（实际 Δf=2 赫兹），默认间距 10 赫兹。
    """
    wav = tmp_path / "two_tones.wav"
    t = np.arange(int(FS)) / FS
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t) + 0.5 * np.sin(2 * np.pi * 1020.0 * t)
    _write_wav(wav, x)
    src = {"type": "file", "path": str(wav), "sampleRate": FS}

    r = peaks.compute_peaks_sync(src, _table_params(resolutionHz=2.0))
    assert r["resolution_hz"] == pytest.approx(2.0)  # 实际 Δf = fs / nperseg
    freqs = r["frequencies"]
    assert freqs.size >= 2
    top2 = np.sort(freqs[:2])
    assert top2 == pytest.approx([1000.0, 1020.0], abs=5.0)


def test_compute_peaks_actual_resolution_is_clamped_by_signal_length(tmp_path):
    """C5：目标分辨率比 fs/N 更细时，实际 Δf 以段长（=N）为准并如实返回。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav, _tone())
    src = {"type": "file", "path": str(wav), "sampleRate": FS}

    # 0.5 赫兹目标 → 段长会超过 1 秒数据，clamp 到 N=48000，实际 Δf=1 赫兹
    r = peaks.compute_peaks_sync(src, _table_params(resolutionHz=0.5))
    assert r["resolution_hz"] == pytest.approx(1.0)


def test_compute_peaks_respects_explicit_min_spacing(tmp_path):
    """显式给出的最小间距仍然生效（用于确认默认值不是硬编码）。"""
    wav = tmp_path / "two_tones.wav"
    t = np.arange(int(FS)) / FS
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t) + 0.5 * np.sin(2 * np.pi * 1020.0 * t)
    _write_wav(wav, x)
    src = {"type": "file", "path": str(wav), "sampleRate": FS}

    r = peaks.compute_peaks_sync(src, _table_params(resolutionHz=2.0, peakMinSpacingHz=50.0))
    near_group = r["frequencies"][np.abs(r["frequencies"] - 1010.0) < 20.0]
    assert near_group.size == 1  # 50 赫兹间距下双音被合并


def test_compute_peaks_resolution_controls_table(tmp_path):
    """v2 C2/C4：改 resolutionHz 后峰值表的实际 Δf 与峰值估计同步变化。

    用非整数 bin 的 1007 赫兹单音：Δf=10 时峰值落在 1010 赫兹附近，
    Δf=50 时落在 1000 赫兹附近；两者都接近真实频率。
    """
    wav = tmp_path / "tone1007.wav"
    _write_wav(wav, _tone(seconds=1.0, freq=1007.0))
    src = {"type": "file", "path": str(wav), "sampleRate": FS}

    fine = peaks.compute_peaks_sync(src, _table_params(resolutionHz=10.0))
    coarse = peaks.compute_peaks_sync(src, _table_params(resolutionHz=50.0))

    # 实际 Δf 分别约为 10 / 50 赫兹（频点一密一疏）
    assert fine["resolution_hz"] == pytest.approx(10.0)
    assert coarse["resolution_hz"] == pytest.approx(50.0)

    fine_peak = float(fine["frequencies"][0])
    coarse_peak = float(coarse["frequencies"][0])
    # 两种分辨率下峰值表都至少有一个峰接近真实频率 1007 赫兹
    assert abs(fine_peak - 1007.0) <= 10.0
    assert abs(coarse_peak - 1007.0) <= 50.0
    # 分辨率不同 → 峰值频率估计不同（不是恒等结果）
    assert abs(fine_peak - coarse_peak) >= 5.0
    # 幅值仍在峰值口径 0.5 附近
    assert fine["magnitudes"][0] == pytest.approx(0.5, rel=0.1)


def test_compute_peaks_zero_padding_is_extra_points(tmp_path):
    """C3：峰值表侧零填充也是「额外点数」，分辨率仍由段长决定。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav, _tone())
    src = {"type": "file", "path": str(wav), "sampleRate": FS}

    base = peaks.compute_peaks_sync(src, _table_params())
    r = peaks.compute_peaks_sync(src, _table_params(zeroPadding=48000))
    # 默认 resolutionHz=10 → nperseg=4800，实际 Δf=10 赫兹；零填充不改变分辨率
    assert base["resolution_hz"] == pytest.approx(10.0)
    assert r["resolution_hz"] == pytest.approx(10.0)
    assert r["frequencies"][0] == pytest.approx(1000.0, abs=1.0)


# ---------------------------------------------------------------- 取消（C7）


def test_peaks_sync_cancellation_aborts(tmp_path):
    wav = tmp_path / "sweep.wav"
    _write_wav(wav, _sweep())
    src = {"type": "file", "path": str(wav), "sampleRate": FS}

    task_id, _status = tsm.create_task_with_status("cancel peaks")
    assert tsm.cancel_task(task_id)
    with pytest.raises(tsm.TaskCanceledError):
        peaks.compute_peaks_sync(src, _table_params(), task_id=task_id)
    with pytest.raises(tsm.TaskCanceledError):
        peaks.compute_peak_curves_sync(src, _curve_params(), task_id=task_id)


def test_peaks_cancel_task_helpers():
    """C++ 侧可直接调用模块级 cancel_task / is_cancelled（与 spectrum/stft 一致）。"""
    task_id, _status = tsm.create_task_with_status("cancel helper")
    assert peaks.is_cancelled(task_id) is False
    assert peaks.cancel_task(task_id)
    assert peaks.is_cancelled(task_id) is True


def test_async_cancel_does_not_store_result(monkeypatch):
    """C7：计算完成时若任务已被取消，不写结果表（避免大数组滞留）。"""
    started = threading.Event()
    proceed = threading.Event()
    returned = threading.Event()

    def fake_compute(source, params, progress_cb=None, task_id=None):
        started.set()
        assert proceed.wait(5.0), "test did not release the fake compute"
        returned.set()
        return {"would": "be result"}

    monkeypatch.setattr(peaks, "compute_peak_curves_sync", fake_compute)
    task_id = peaks.compute_peak_curves_async({"type": "file", "path": "unused", "sampleRate": FS}, {})
    assert started.wait(5.0)
    assert tsm.cancel_task(task_id)
    proceed.set()
    assert returned.wait(5.0)
    # 给 worker 线程收尾的时间（取消后应直接 return，不写 _RESULT_STORE）
    time.sleep(0.2)
    assert peaks.take_result(task_id) is None
    assert tsm.is_task_canceled(task_id)


def test_async_success_stores_contract_result(tmp_path):
    """正常完成后结果可取走，且包含 mag_by_rank（B1 的异步链路）。"""
    wav = tmp_path / "sweep.wav"
    _write_wav(wav, _sweep())
    src = {"type": "file", "path": str(wav), "sampleRate": FS}
    task_id = peaks.compute_peak_curves_async(src, _curve_params(peakTopN=2))
    try:
        deadline = time.time() + 10.0
        while time.time() < deadline:
            info = tsm.get_task_status(task_id)
            if info and not info["is_running"]:
                break
            time.sleep(0.02)
        result = peaks.take_result(task_id)
        assert result is not None
        assert result["mag_by_rank"].shape == (2, result["n_frames"])
        assert np.isfinite(result["freq_by_rank"][0]).any()
    finally:
        peaks.take_result(task_id)
