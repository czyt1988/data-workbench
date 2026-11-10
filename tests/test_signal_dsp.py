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


def _peak_near(freqs, mags, f0, tol=1.0):
    idx = int(np.argmax(mags))
    assert abs(freqs[idx] - f0) < tol, f"peak at {freqs[idx]} Hz, expected {f0} Hz"
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
    # C8：未知名称回退矩形窗（与文档一致，不再回退汉宁窗）
    unknown = spectral.window_values("unknown", 8)
    assert np.all(unknown == 1.0)
    assert not np.allclose(spectral.window_values("hann", 8), 1.0)


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


# ---------------------------------------------------------------- RMS 直流口径（C2/C9）


def test_amplitude_spectrum_rms_dc_not_divided():
    """常量信号的均方根等于其幅值：DC 频点不应除以 √2。"""
    x = np.full(4096, 2.0)
    f, peak = spectral.amplitude_spectrum(x, FS, window="rectangular", rms=False)
    f2, rms = spectral.amplitude_spectrum(x, FS, window="rectangular", rms=True)
    assert f.size == f2.size
    assert peak[0] == pytest.approx(2.0, rel=1e-9)
    assert rms[0] == pytest.approx(2.0, rel=1e-9)  # 直流不除 √2
    # k >= 1 的频点仍按 RMS 换算
    assert np.allclose(rms[1:], peak[1:] / np.sqrt(2.0))


# ------------------------------------------------ 分块平均幅度谱（C4 / 零填充语义）


def test_amplitude_spectrum_segmented_short_signal_matches_whole():
    """nperseg >= 数据长度时退化为整段单次 FFT，与 amplitude_spectrum 完全一致。"""
    x = _tone(seconds=0.1)
    f1, a1 = spectral.amplitude_spectrum(x, FS, window="hann")
    f2, a2 = spectral.amplitude_spectrum_segmented(x, FS, nperseg=x.size, overlap=0.5, window="hann")
    assert np.allclose(f1, f2)
    assert np.allclose(a1, a2)


def test_amplitude_spectrum_segmented_mean_tone():
    """分段平均后 Δf = fs/nperseg，单音峰值幅度仍在 ±5% 内。"""
    x = _tone()
    nperseg = 4800  # 0.1 秒：1000 赫兹刚好 100 个整周期，避免扇贝损失
    freqs, amps = spectral.amplitude_spectrum_segmented(
        x, FS, nperseg=nperseg, overlap=0.5, window="hann", average="mean"
    )
    assert freqs.size == nperseg // 2 + 1
    assert freqs[1] - freqs[0] == pytest.approx(FS / nperseg, rel=1e-9)
    peak = _peak_near(freqs, amps, FREQ, tol=FS / nperseg)
    assert peak == pytest.approx(AMP, rel=0.05)


def test_amplitude_spectrum_segmented_median_tone():
    x = _tone()
    nperseg = 4800
    f_mean, a_mean = spectral.amplitude_spectrum_segmented(
        x, FS, nperseg=nperseg, overlap=0.5, window="hann", average="mean"
    )
    f_med, a_med = spectral.amplitude_spectrum_segmented(
        x, FS, nperseg=nperseg, overlap=0.5, window="hann", average="median"
    )
    assert np.allclose(f_mean, f_med)
    assert a_mean.shape == a_med.shape
    assert _peak_near(f_med, a_med, FREQ, tol=FS / nperseg) == pytest.approx(AMP, rel=0.05)


def test_amplitude_spectrum_segmented_zero_padding_is_extra_points():
    """分块路径同样按「额外点数」理解 nfft（C3）：频点变密但段长/分辨率不变。"""
    x = _tone()
    nperseg = 4800
    extra = 4800
    f0, a0 = spectral.amplitude_spectrum_segmented(x, FS, nperseg=nperseg, window="hann")
    f1, a1 = spectral.amplitude_spectrum_segmented(
        x, FS, nperseg=nperseg, window="hann", nfft=nperseg + extra
    )
    assert f1.size == (nperseg + extra) // 2 + 1
    assert f1[1] - f1[0] == pytest.approx(FS / (nperseg + extra), rel=1e-9)
    assert f0.size == nperseg // 2 + 1
    assert _peak_near(f1, a1, FREQ, tol=FS / (nperseg + extra)) == pytest.approx(
        _peak_near(f0, a0, FREQ, tol=FS / nperseg), rel=0.05
    )


def test_amplitude_spectrum_segmented_cancel_hook_aborts():
    """分块循环每块调用 cancel_hook，回调抛异常即中止计算（C7）。"""
    x = _tone(seconds=1.0)
    calls = []

    def cancel_hook():
        calls.append(1)
        raise RuntimeError("cancelled")

    with pytest.raises(RuntimeError, match="cancelled"):
        spectral.amplitude_spectrum_segmented(
            x, FS, nperseg=4096, overlap=0.5, window="hann", cancel_hook=cancel_hook
        )
    assert len(calls) == 1


# ---------------------------------------------------------------- §11.5 测试缺口（2026-11-09 整改）

def test_amplitude_spectrum_nyquist_not_doubled():
    """偶数 nfft 时最后一个频点（Nyquist）不乘 2（单边修正）。"""
    n = 2048
    # cos(pi*n) = 交替 ±1：能量正好落在 Nyquist 频点
    x = 0.5 * np.cos(np.pi * np.arange(n))
    f, a = spectral.amplitude_spectrum(x, FS, window="rectangular")
    assert f[-1] == pytest.approx(FS / 2.0)
    # 若 Nyquist 被误乘 2 会得到 1.0
    assert a[-1] == pytest.approx(0.5, rel=1e-9)


def test_window_flattop_amplitude_normalization():
    """平顶窗的幅值归一化：非整 bin 单音也应给出接近真实的峰值幅度。"""
    t = np.arange(24000) / FS
    x = 0.5 * np.sin(2 * np.pi * 1000.3 * t)  # 故意不在 bin 中心（1000.3 Hz）
    f_flat, a_flat = spectral.amplitude_spectrum(x, FS, window="flattop")
    assert a_flat[int(np.argmax(a_flat))] == pytest.approx(0.5, rel=0.01)
    # 对照：汉宁窗存在扇贝损失，峰值明显偏低（说明上面的断言确实覆盖了平顶窗归一化）
    f_hann, a_hann = spectral.amplitude_spectrum(x, FS, window="hann")
    assert a_hann[int(np.argmax(a_hann))] < 0.5 * 0.995


def test_to_db_log10_zero_protected():
    """log10(0) 保护：0 幅值/0 参考不得产生 -inf/NaN。"""
    v = spectral.to_db(0.0)
    assert np.all(np.isfinite(v))
    assert v[0] == pytest.approx(20.0 * np.log10(1e-30))  # 幅值下限 _EPS
    assert np.all(np.isfinite(spectral.to_db([0.0, 0.0], ref=0.0)))
    assert np.all(np.isfinite(spectral.to_db(np.zeros(4), kind="power")))


def test_all_zero_signal_spectrum_psd_and_peaks():
    """全零信号：频谱/PSD 全 0 且无 NaN，峰值列表为空。"""
    x = np.zeros(4096)
    f, a = spectral.amplitude_spectrum(x, FS, window="hann")
    assert a.size == f.size
    assert np.all(a == 0.0)
    assert np.all(np.isfinite(a))
    psd_f, psd = spectral.welch_psd(x, FS, nperseg=1024, window="hann")
    assert psd.size == psd_f.size
    assert np.all(psd == 0.0)
    assert np.all(np.isfinite(psd))
    res = spectral.find_peaks_topn(a, f, n=5, rel_db=80.0)
    assert res["frequencies"].size == res["magnitudes"].size == 0


def test_zero_padding_smaller_than_segment_is_ignored():
    """零填充小于段长时 nfft 被 clamp 到段长（不缩短分辨率、不报错）。"""
    x = _tone(seconds=0.1)
    f0, a0 = spectral.amplitude_spectrum(x, FS, window="hann")
    f1, a1 = spectral.amplitude_spectrum(x, FS, window="hann", nfft=len(x) // 4)
    assert np.array_equal(f0, f1)
    assert np.allclose(a0, a1)
    # 分块路径同样 clamp
    nperseg = len(x) // 2
    f2, a2 = spectral.amplitude_spectrum_segmented(x, FS, nperseg=nperseg, nfft=10)
    assert f2.size == nperseg // 2 + 1


def test_find_peaks_equal_magnitude_sorted_by_frequency():
    """同幅值峰按频率升序（排序稳定性）。"""
    mag = np.zeros(64)
    mag[8] = 2.0
    mag[24] = 2.0
    freqs = np.arange(64, dtype=np.float64) * 50.0
    res = spectral.find_peaks_topn(mag, freqs, n=2, rel_db=80.0)
    assert res["frequencies"].size == 2
    assert res["frequencies"][0] == pytest.approx(400.0)
    assert res["frequencies"][1] == pytest.approx(1200.0)
    assert res["magnitudes"][0] == res["magnitudes"][1]


# ---------------------------------------------------------------- 分块流式韦尔奇 PSD（N-C1a）

def test_welch_psd_matches_scipy_density():
    """分块流式 welch 与 scipy.signal.welch(density, detrend=False) 数值一致。"""
    import scipy.signal as sps

    rng = np.random.default_rng(2026)
    x = rng.normal(0.0, 1.0, 48000)
    cases = [(4096, 0.5, "hann"), (4001, 0.25, "hamming"), (2048, 0.5, "flattop")]
    for nperseg, overlap, window in cases:
        f1, p1 = spectral.welch_psd(x, FS, nperseg, overlap=overlap, window=window, average="mean")
        hop = int(max(1, round(nperseg * (1.0 - overlap))))
        f2, p2 = sps.welch(
            x,
            fs=FS,
            window=spectral.window_values(window, nperseg),
            nperseg=nperseg,
            noverlap=max(0, nperseg - hop),
            detrend=False,
            return_onesided=True,
            scaling="density",
            average="mean",
        )
        assert np.allclose(f1, f2)
        assert np.allclose(p1, p2, rtol=1e-10, atol=0.0)


def test_welch_psd_median_matches_scipy():
    """median 口径同样保持 scipy 的中位数偏差修正（分块暂存 float32，允许 1e-5 相对误差）。"""
    import scipy.signal as sps

    rng = np.random.default_rng(7)
    x = rng.normal(0.0, 1.0, 24000)
    nperseg, overlap = 2048, 0.5
    f1, p1 = spectral.welch_psd(x, FS, nperseg, overlap=overlap, window="hann", average="median")
    hop = int(max(1, round(nperseg * (1.0 - overlap))))
    f2, p2 = sps.welch(
        x,
        fs=FS,
        window=spectral.window_values("hann", nperseg),
        nperseg=nperseg,
        noverlap=max(0, nperseg - hop),
        detrend=False,
        return_onesided=True,
        scaling="density",
        average="median",
    )
    assert np.allclose(f1, f2)
    assert np.allclose(p1, p2, rtol=1e-5, atol=0.0)


def test_welch_psd_streaming_blocks_bounded(monkeypatch):
    """N-C1a：PSD 分块计算，FFT 输入永远不超过 block_frames 行（不整段成帧）。"""
    x = _tone(seconds=1.0)
    shapes = []
    real_rfft = np.fft.rfft

    def spy_rfft(a, *args, **kwargs):
        shapes.append(np.asarray(a).shape)
        return real_rfft(a, *args, **kwargs)

    monkeypatch.setattr(np.fft, "rfft", spy_rfft)
    f, p = spectral.welch_psd(
        x, FS, nperseg=1024, overlap=0.5, window="hann", average="mean", block_frames=64
    )
    # 1 秒 / 512 跳步 → 92 帧，应切成多块
    assert f.size == 1024 // 2 + 1
    assert len(shapes) >= 2
    assert max(s[0] for s in shapes) <= 64


def test_welch_psd_cancel_hook_aborts():
    """N-C1a/C7：每个分块检查 cancel_hook，回调抛异常即中止。"""
    x = _tone(seconds=1.0)
    calls = []

    def cancel_hook():
        calls.append(1)
        raise RuntimeError("cancelled")

    with pytest.raises(RuntimeError, match="cancelled"):
        spectral.welch_psd(x, FS, nperseg=1024, overlap=0.5, window="hann", cancel_hook=cancel_hook)
    assert len(calls) == 1


def test_spectrum_float32_input_matches_float64():
    """N-C1b：float32 输入（块内升 float64）与 float64 输入结果一致。"""
    x = _tone(seconds=1.0, freq=1000.7)
    f32, a32 = spectral.amplitude_spectrum_segmented(
        x.astype(np.float32), FS, nperseg=4800, overlap=0.5, window="hann"
    )
    f64, a64 = spectral.amplitude_spectrum_segmented(x, FS, nperseg=4800, overlap=0.5, window="hann")
    assert np.allclose(f32, f64)
    assert np.allclose(a32, a64, rtol=1e-5, atol=1e-9)
    p32 = spectral.welch_psd(x.astype(np.float32), FS, nperseg=4800, overlap=0.5)
    p64 = spectral.welch_psd(x, FS, nperseg=4800, overlap=0.5)
    assert np.allclose(p32[0], p64[0])
    assert np.allclose(p32[1], p64[1], rtol=1e-5, atol=1e-12)
