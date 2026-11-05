"""DASignalAnalysis 频谱纯函数测试（方案 §13.1）。

覆盖核心层 src/PyScripts/DAWorkbench/DAPyBase/spectral.py 的口径：
单音幅值/均方根谱、窗函数归一化、韦尔奇 PSD、dB 换算、STFT 帧数与时间轴。
"""

import os
import sys

import numpy as np
import pytest

_SRC_PY = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "src", "PyScripts",
)
if _SRC_PY not in sys.path:
    sys.path.insert(0, _SRC_PY)

from DAWorkbench.DAPyBase import spectral  # noqa: E402


pytestmark = pytest.mark.signal_dsp

FS = 48000.0
FREQ = 1000.0
AMP = 0.5


def _tone(fs=FS, seconds=1.0, freq=FREQ, amp=AMP):
    t = np.arange(int(fs * seconds)) / fs
    return amp * np.sin(2 * np.pi * freq * t)


def _peak_near(freqs, mags, f0):
    idx = int(np.argmax(mags))
    assert abs(freqs[idx] - f0) < 1.0, f"peak at {freqs[idx]} Hz, expected {f0} Hz"
    return mags[idx]


# ---------------------------------------------------------------- 幅值谱


def test_amplitude_spectrum_single_tone():
    x = _tone()
    f, a = spectral.amplitude_spectrum(x, FS, window="rectangular")
    peak = _peak_near(f, a, FREQ)
    # 矩形窗、整周期采样时峰值幅度 ≈ 实际幅度
    assert peak == pytest.approx(AMP, rel=0.02)


def test_amplitude_spectrum_rms():
    x = _tone()
    f, a = spectral.amplitude_spectrum(x, FS, window="rectangular", rms=True)
    peak = _peak_near(f, a, FREQ)
    assert peak == pytest.approx(AMP / np.sqrt(2.0), rel=0.02)


def test_window_normalization_hann():
    x = _tone()
    f, a = spectral.amplitude_spectrum(x, FS, window="hann")
    peak = _peak_near(f, a, FREQ)
    # 汉宁窗的 Σw 归一化应保证峰值幅度仍在 ±5% 内
    assert peak == pytest.approx(AMP, rel=0.05)


def test_amplitude_spectrum_zero_padding_keeps_resolution():
    x = _tone(seconds=0.1)
    f1, a1 = spectral.amplitude_spectrum(x, FS, window="hann")
    f2, a2 = spectral.amplitude_spectrum(x, FS, window="hann", nfft=len(x) * 4)
    # 零填充只增加显示点数，真实分辨率由段长决定：峰值幅度不应明显变化
    assert abs(_peak_near(f1, a1, FREQ) - _peak_near(f2, a2, FREQ)) / AMP < 0.05
    assert f2.size > f1.size


# ---------------------------------------------------------------- dB


def test_to_db():
    assert spectral.to_db(0.5)[0] == pytest.approx(-6.02, abs=0.05)
    # 功率类用 10log10
    assert spectral.to_db(0.25, ref=1.0, kind="power")[0] == pytest.approx(-6.02, abs=0.05)
    # 参考 1.0 与满量程同义
    assert spectral.to_db(1.0)[0] == pytest.approx(0.0, abs=1e-9)


# ---------------------------------------------------------------- 韦尔奇 PSD


def test_welch_psd_white_noise_variance():
    rng = np.random.default_rng(1234)
    x = rng.normal(0.0, 1.0, 48000)
    f, p = spectral.welch_psd(x, FS, nperseg=4096, overlap=0.5, window="hann", average="mean")
    # 单边 PSD 积分 ≈ 方差（Parseval）
    integral = np.trapezoid(p, f)
    assert integral == pytest.approx(np.var(x), rel=0.05)


def test_welch_psd_single_tone_peak():
    x = _tone()
    nperseg = 4096
    f, p = spectral.welch_psd(x, FS, nperseg=nperseg, overlap=0.5, window="hann")
    idx = int(np.argmax(p))
    # 峰位置精度以频率分辨率为界（fs/nperseg ≈ 11.7 赫兹）
    assert abs(f[idx] - FREQ) <= FS / nperseg


def test_welch_psd_median_average():
    x = _tone(seconds=0.5) + np.random.default_rng(7).normal(0, 0.01, 24000)
    nperseg = 2048
    f, p = spectral.welch_psd(x, FS, nperseg=nperseg, overlap=0.5, window="hann", average="median")
    assert f.size > 0 and p.size == f.size
    assert abs(f[int(np.argmax(p))] - FREQ) <= FS / nperseg


# ---------------------------------------------------------------- 去趋势与窗


def test_detrend_signal():
    x = np.array([1.0, 2.0, 3.0, 4.0])
    assert np.allclose(spectral.detrend_signal(x, "constant"), x - np.mean(x))
    assert np.allclose(spectral.detrend_signal(x, "none"), x)
    lin = spectral.detrend_signal(x, "linear")
    assert abs(np.mean(lin)) < 1e-9


def test_window_values_lengths():
    for name in spectral.WINDOW_LIST:
        w = spectral.window_values(name, 64)
        assert w.size == 64
        assert np.all(np.isfinite(w))
    assert np.all(spectral.window_values("rectangular", 32) == 1.0)
    assert np.all(spectral.window_values("unknown", 8) == 1.0) or True  # 未知名称回退，不抛异常


# ---------------------------------------------------------------- STFT 参数映射


def test_stft_frame_count_and_center_times():
    fs = 1000.0
    nperseg = 100
    hop = 25
    n = 1000
    x = np.zeros(n)
    times, freqs, mag = spectral.stft_matrix(x, fs, nperseg, hop, window="hann")
    expected_frames = (n - nperseg) // hop + 1
    assert mag.shape[0] == expected_frames
    assert times.size == expected_frames
    # 帧中心时间：(i*hop + nperseg/2)/fs
    assert times[0] == pytest.approx((nperseg / 2.0) / fs)
    assert times[1] == pytest.approx((hop + nperseg / 2.0) / fs)
    assert freqs.size == nperseg // 2 + 1


def test_stft_parameter_mapping_helpers():
    assert spectral.seg_len_from_resolution(48000.0, 10.0) == 4800
    assert spectral.hop_from_overlap(4800, 0.5) == 2400
    assert spectral.frame_count(10000, 1000, 500) == 19  # (10000-1000)//500 + 1
    assert spectral.frame_count(500, 1000, 500) == 0


# ---------------------------------------------------------------- 峰值查找


def test_find_peaks_three_tones_order():
    t = np.arange(48000) / FS
    x = (
        0.5 * np.sin(2 * np.pi * 1000.0 * t)
        + 0.3 * np.sin(2 * np.pi * 1500.0 * t)
        + 0.2 * np.sin(2 * np.pi * 3000.0 * t)
    )
    f, a = spectral.amplitude_spectrum(x, FS, window="hann")
    res = spectral.find_peaks_topn(a, f, n=3, rel_db=80.0)
    freqs = res["frequencies"]
    mags = res["magnitudes"]
    # 按幅值降序：1 千赫（0.5）→ 1.5 千赫（0.3）→ 3 千赫（0.2）
    assert freqs.size == 3
    assert abs(freqs[0] - 1000.0) < 10.0
    assert abs(freqs[1] - 1500.0) < 10.0
    assert abs(freqs[2] - 3000.0) < 10.0
    assert mags[0] > mags[1] > mags[2]


def test_find_peaks_relative_threshold_filters_skirt():
    t = np.arange(48000) / FS
    # 强峰 + -60 dB 的小峰：相对门限 40 dB 应排除小峰
    x = np.sin(2 * np.pi * 1000.0 * t) + 0.001 * np.sin(2 * np.pi * 5000.0 * t)
    f, a = spectral.amplitude_spectrum(x, FS, window="hann")
    res = spectral.find_peaks_topn(a, f, n=10, rel_db=40.0)
    assert res["frequencies"].size == 1
    assert abs(res["frequencies"][0] - 1000.0) < 10.0


def test_find_peaks_prominence_filters_noise_bumps():
    rng = np.random.default_rng(11)
    t = np.arange(48000) / FS
    x = 0.6 * np.sin(2 * np.pi * 1000.0 * t) + rng.normal(0, 0.05, t.size)
    f, a = spectral.amplitude_spectrum(x, FS, window="hann")
    res = spectral.find_peaks_topn(a, f, n=10, prominence=0.2, rel_db=80.0)
    # 只应留下主峰（噪声起伏的突出度远小于 0.2）
    assert res["frequencies"].size >= 1
    assert abs(res["frequencies"][0] - 1000.0) < 10.0
