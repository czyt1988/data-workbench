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


# ---------------------------------------------------------------- 按块取数 provider（D1-a）


def _provider_from_array(x):
    """把一维数组包成 provider(start, count)，并记录请求区间。

    provider 只返回请求区间的切片（float32），模拟 C++ ``readRange`` 的语义。
    """
    requests = []

    def provider(start, count):
        requests.append((int(start), int(count)))
        return np.ascontiguousarray(x[int(start) : int(start) + int(count)], dtype=np.float32)

    provider.requests = requests
    return provider


def _deterministic_signal(n=24000, seed=2026):
    """确定性多正弦 + 噪声（固定 seed），带少量直流避免零均值巧合。"""
    rng = np.random.default_rng(seed)
    t = np.arange(n) / FS
    x = (
        0.4 * np.sin(2.0 * np.pi * 440.0 * t)
        + 0.2 * np.sin(2.0 * np.pi * 1500.0 * t + 0.3)
        + 0.05 * rng.normal(size=n)
        + 0.01
    )
    return x.astype(np.float32)


def test_provider_amplitude_matches_array_version():
    """D1-a：provider 版分块幅度谱与整段数组版数值一致（mean/median × detrend × 零填充）。"""
    n = 24000
    x = _deterministic_signal(n)
    nperseg = 1000
    for average in ("mean", "median"):
        for detrend in ("none", "constant", "linear"):
            for zero_padding in (0, 512):
                nfft = (nperseg + zero_padding) if zero_padding else None
                y = spectral.detrend_signal(x, detrend)
                f_ref, a_ref = spectral.amplitude_spectrum_segmented(
                    y,
                    FS,
                    nperseg=nperseg,
                    overlap=0.5,
                    window="hann",
                    nfft=nfft,
                    average=average,
                )
                f_p, a_p = spectral.amplitude_spectrum_segmented_provider(
                    _provider_from_array(x),
                    n,
                    FS,
                    nperseg=nperseg,
                    overlap=0.5,
                    window="hann",
                    nfft=nfft,
                    average=average,
                    detrend=detrend,
                    block_samples=4096,
                )
                # 频率轴必须完全一致
                assert np.array_equal(f_ref, f_p), (average, detrend, zero_padding)
                assert np.allclose(a_p, a_ref, rtol=1e-5, atol=1e-9), (
                    "amplitude mismatch: average=%s detrend=%s zeroPadding=%s" % (average, detrend, zero_padding)
                )


def test_provider_welch_matches_array_version():
    """D1-a：provider 版韦尔奇 PSD 与整段数组版数值一致（mean/median × detrend）。"""
    n = 24000
    x = _deterministic_signal(n)
    nperseg = 1000
    for average in ("mean", "median"):
        for detrend in ("none", "constant", "linear"):
            y = spectral.detrend_signal(x, detrend)
            f_ref, p_ref = spectral.welch_psd(
                y, FS, nperseg, overlap=0.5, window="hann", average=average
            )
            f_p, p_p = spectral.welch_psd_provider(
                _provider_from_array(x),
                n,
                FS,
                nperseg,
                overlap=0.5,
                window="hann",
                average=average,
                detrend=detrend,
                block_samples=4096,
            )
            assert np.array_equal(f_ref, f_p), (average, detrend)
            assert np.allclose(p_p, p_ref, rtol=1e-5, atol=1e-12), (
                "welch mismatch: average=%s detrend=%s" % (average, detrend)
            )


def test_provider_frame_alignment_non_integer_blocks():
    """帧边界：总长不是块的整数倍、段长/跳步也不能整除时，provider 分块不影响结果。"""
    block_samples = 4096
    n = int(3.7 * block_samples)  # 15155：卡在非整块位置
    nperseg = 1003  # hop = round(1003*0.5) = 502，不能整除 N-段长
    hop = spectral.hop_from_overlap(nperseg, 0.5)
    assert hop == 502
    x = _deterministic_signal(n)
    for average in ("mean", "median"):
        y = spectral.detrend_signal(x, "none")
        f_ref, a_ref = spectral.amplitude_spectrum_segmented(
            y, FS, nperseg=nperseg, overlap=0.5, window="hann", average=average
        )
        f_p, a_p = spectral.amplitude_spectrum_segmented_provider(
            _provider_from_array(x),
            n,
            FS,
            nperseg=nperseg,
            overlap=0.5,
            window="hann",
            average=average,
            detrend="none",
            block_samples=block_samples,
        )
        assert np.array_equal(f_ref, f_p)
        assert np.allclose(a_p, a_ref, rtol=1e-5, atol=1e-9)
    y = spectral.detrend_signal(x, "none")
    f_ref, p_ref = spectral.welch_psd(y, FS, nperseg, overlap=0.5, window="hann")
    f_p, p_p = spectral.welch_psd_provider(
        _provider_from_array(x), n, FS, nperseg, overlap=0.5, window="hann", block_samples=block_samples
    )
    assert np.array_equal(f_ref, f_p)
    assert np.allclose(p_p, p_ref, rtol=1e-5, atol=1e-12)


def test_provider_nperseg_ge_total_falls_back_to_whole_read():
    """nperseg >= total_samples 时回退「整段读 + 现有函数」，且只读一次整段。"""
    x = _tone(seconds=0.02)  # 960 点
    n = x.size
    nperseg = n

    provider = _provider_from_array(x)
    f_ref, a_ref = spectral.amplitude_spectrum_segmented(
        x.astype(np.float32), FS, nperseg=nperseg, overlap=0.5, window="hann"
    )
    f_p, a_p = spectral.amplitude_spectrum_segmented_provider(
        provider, n, FS, nperseg=nperseg, overlap=0.5, window="hann"
    )
    assert np.array_equal(f_ref, f_p)
    assert np.array_equal(a_ref, a_p)
    assert provider.requests == [(0, n)]

    # nperseg 大于数据长度：同样退化为整段单次 FFT
    provider2 = _provider_from_array(x)
    f_ref2, a_ref2 = spectral.amplitude_spectrum_segmented(
        x.astype(np.float32), FS, nperseg=n + 512, overlap=0.5, window="hann"
    )
    f_p2, a_p2 = spectral.amplitude_spectrum_segmented_provider(
        provider2, n, FS, nperseg=n + 512, overlap=0.5, window="hann"
    )
    assert np.array_equal(f_ref2, f_p2)
    assert np.array_equal(a_ref2, a_p2)

    # PSD 同样回退，数值与整段版一致
    provider3 = _provider_from_array(x)
    f_ref3, p_ref3 = spectral.welch_psd(x.astype(np.float32), FS, nperseg=n)
    f_p3, p_p3 = spectral.welch_psd_provider(provider3, n, FS, nperseg=n)
    assert np.array_equal(f_ref3, f_p3)
    assert np.array_equal(p_ref3, p_p3)
    assert provider3.requests == [(0, n)]


def test_provider_cancel_hook_aborts_on_first_block():
    """C7：cancel_hook 在第一次按块取数时就中止（幅度/PSD、含统计遍与频谱遍）。"""
    x = _tone(seconds=0.5)
    for detrend in ("none", "constant"):
        calls = []

        def cancel_hook():
            calls.append(1)
            raise RuntimeError("cancelled")

        with pytest.raises(RuntimeError, match="cancelled"):
            spectral.amplitude_spectrum_segmented_provider(
                _provider_from_array(x),
                x.size,
                FS,
                nperseg=1024,
                overlap=0.5,
                window="hann",
                detrend=detrend,
                cancel_hook=cancel_hook,
                block_samples=4096,
            )
        assert len(calls) == 1, "cancel_hook should abort on the first block (detrend=%s)" % detrend

        calls.clear()
        with pytest.raises(RuntimeError, match="cancelled"):
            spectral.welch_psd_provider(
                _provider_from_array(x),
                x.size,
                FS,
                nperseg=1024,
                overlap=0.5,
                window="hann",
                detrend=detrend,
                cancel_hook=cancel_hook,
                block_samples=4096,
            )
        assert len(calls) == 1


def _request_passes(requests):
    """把请求序列按「起点回退」切成多遍扫描（detrend 统计遍 + 频谱遍）。

    每遍内部请求区间必须单调不重叠（allow carry 由实现内部完成，不重复请求）。
    """
    passes = []
    prev_start = None
    for start, count in requests:
        if prev_start is None or start <= prev_start:
            passes.append([])
        passes[-1].append((start, count))
        prev_start = start
    return passes


def test_provider_requests_bounded_and_never_whole_read():
    """N-C1a/D1-a：N=2e6 时不整段读取，单次请求不超过 block_samples，请求区间单调不重叠。"""
    n = 2_000_000
    block_samples = 1 << 20
    requests = []

    def provider(start, count):
        requests.append((int(start), int(count)))
        idx = np.arange(int(start), int(start) + int(count), dtype=np.float64)
        return (0.5 * np.sin(2.0 * np.pi * 1000.0 * idx / FS)).astype(np.float32)

    f, a = spectral.amplitude_spectrum_segmented_provider(
        provider, n, FS, nperseg=4096, overlap=0.5, window="hann", detrend="none", block_samples=block_samples
    )
    assert f.size == 4096 // 2 + 1 and a.size == f.size
    assert requests, "provider was never called"
    assert max(c for _s, c in requests) <= block_samples
    assert all(c < n for _s, c in requests), "provider must not read the whole signal at once"
    passes = _request_passes(requests)
    assert len(passes) == 1  # detrend=none：只有一遍频谱扫描
    _assert_pass_monotonic(passes[0], n)

    # detrend=constant：先一遍统计均值、再一遍频谱；每遍都不整段、区间单调
    requests.clear()
    spectral.amplitude_spectrum_segmented_provider(
        provider,
        n,
        FS,
        nperseg=4096,
        overlap=0.5,
        window="hann",
        detrend="constant",
        block_samples=block_samples,
    )
    passes = _request_passes(requests)
    assert len(passes) == 2, "constant detrend should do a stats pass + a spectrum pass"
    for reqs in passes:
        assert max(c for _s, c in reqs) <= block_samples
        assert all(c < n for _s, c in reqs)
        _assert_pass_monotonic(reqs, n)


def _assert_pass_monotonic(reqs, n):
    """单遍请求：从 0 开始、起点严格递增、区间首尾相接且不越界（不重复请求）。"""
    pos = 0
    for start, count in reqs:
        assert start == pos, "request %r not contiguous after %d" % ((start, count), pos)
        assert count > 0
        pos += count
    assert pos <= n
    # 频谱遍可能因最后一帧不覆盖末尾少量样本而提前结束，但误差不超过一帧跨度
    assert n - pos < 4096 + 2048


def test_provider_defensive_handling_and_nan_sanitize():
    """防御性：provider 返回 float64/含 NaN/Inf 时按 float32 + nan_to_num 口径处理。"""
    n = 4800
    x = _deterministic_signal(n).astype(np.float64)
    x[10] = np.nan
    x[11] = np.inf
    x[12] = -np.inf
    sanitized = np.nan_to_num(x, copy=True, nan=0.0, posinf=0.0, neginf=0.0).astype(np.float32)

    def provider(start, count):
        # 故意返回 float64（core 层必须防御性转 float32）
        return x[int(start) : int(start) + int(count)]

    f_ref, a_ref = spectral.amplitude_spectrum_segmented(
        sanitized, FS, nperseg=512, overlap=0.5, window="hann"
    )
    f_p, a_p = spectral.amplitude_spectrum_segmented_provider(
        provider, n, FS, nperseg=512, overlap=0.5, window="hann"
    )
    assert np.array_equal(f_ref, f_p)
    assert np.all(np.isfinite(a_p))
    assert np.array_equal(a_ref, a_p)

    # provider 长度不足 → 明确报错（不静默错位成帧）
    def short_provider(start, count):
        return x[: min(n, 100)]

    with pytest.raises(RuntimeError, match="sample provider returned"):
        spectral.amplitude_spectrum_segmented_provider(
            short_provider, n, FS, nperseg=100, overlap=0.5, window="hann"
        )


def test_provider_linear_detrend_matches_scipy():
    """linear 去趋势的全局最小二乘直线与 scipy.signal.detrend(type="linear") 一致。"""
    from scipy.signal import detrend as sp_detrend

    rng = np.random.default_rng(31)
    n = 20000
    t = np.arange(n) / FS
    # 带直流偏置 + 线性漂移 + 多正弦，确保最小二乘拟合有非零斜率
    x = (
        0.3 * np.sin(2.0 * np.pi * 700.0 * t)
        + 2.0
        + 3.0e-4 * np.arange(n)
        + 0.01 * rng.normal(size=n)
    ).astype(np.float32)
    provider = _provider_from_array(x)
    plan = spectral._provider_detrend_plan(provider, n, "linear")
    ours = spectral._apply_provider_detrend(x, 0, plan)
    ref = sp_detrend(x.astype(np.float64), type="linear")
    assert ours.dtype == np.float32
    # 与 scipy float64 结果比较：残差只来自 float32 落地（相对 ~1e-7）
    assert np.allclose(ours, ref, rtol=1e-5, atol=1e-6)


# ---------------------------------------------------------------- 跟随播放的当前帧谱（D13 修复）

# 插件 Python 包路径（conftest 只加了 src/PyScripts；与 test_signal_peaks.py 同口径）
_PLUGIN_PY = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "plugins", "DASignalAnalysis", "PyScripts",
)
if _PLUGIN_PY not in sys.path:
    sys.path.insert(0, _PLUGIN_PY)


def test_frame_spectrum_matches_peak_curves(tmp_path):
    """跟随播放的当前帧谱与峰值曲线单帧同口径（D13 缺陷：显示栅格 vs 峰值细网格）。

    缺陷：跟随模式原先画 STFT 显示栅格的当前列——频率方向 max 池化到画布行数
    （Δf_row ≈ fs/2/rows），与峰值曲线（每帧前 N、细网格）频率网格不同，峰值标记
    落在曲线下降沿。修复后频谱取「当前帧真实幅值谱」，必须与 peaks 单帧完全同口径。

    覆盖：
    - freqs 与 peaks 的 rfftfreq(nperseg) 网格一致（同段长/跳步）；
    - 同一帧 |rfft| 幅值与 peaks.compute_peak_curves_sync 的 top1 结果对拍（1e-6）；
    - 取数 O(nperseg)：带 provider 时只请求一帧（起点 = frame_index*hop，不整段读）；
    - 越界 frame_index 被 clamp 后不抛异常；
    - frame_index_for_time 与帧中心时间往返一致、超界时间被 clamp。
    """
    from DASignalAnalysis import peaks, stft

    fs = 48000.0
    nperseg_expected = 960  # stftWindowMs=20 毫秒 × 48 千赫
    hop_expected = 480  # stftOverlap=50%
    n = 24000
    x = _deterministic_signal(n)  # 多正弦 + 噪声（float32，已含少量直流）
    wav = tmp_path / "frame_spectrum.wav"
    import soundfile as sf

    # FLOAT 子类型保证读出值与合成数组逐位一致（PCM_16 会量化，超出 1e-6 容差）
    sf.write(str(wav), x, int(fs), subtype="FLOAT")

    params = {
        "stftWindowMs": 20.0,
        "stftOverlap": 50.0,
        "window": "hann",
        "removeDc": False,
        "detrend": "none",
        "peakTopN": 1,
        "peakRelDb": 40.0,
        "peakMode": "simple",
    }

    requests = []

    def read(channel, start, count):
        """假 provider：模拟 C++ readRange(channel, start, count) -> float32 一维。"""
        requests.append((int(channel), int(start), int(count)))
        assert int(channel) == 0
        assert 0 <= start and count > 0 and start + count <= n
        return np.ascontiguousarray(x[int(start) : int(start) + int(count)], dtype=np.float32)

    provider_src = {
        "type": "file",
        "path": str(wav),
        "sampleRate": fs,
        "channel": 0,
        "sampleCount": n,
        "read": read,
    }
    plain_src = {"type": "file", "path": str(wav), "sampleRate": fs, "channel": 0}

    # 峰值曲线（每帧前 N）作为对拍基准
    curves = peaks.compute_peak_curves_sync(plain_src, params)
    assert curves["nperseg"] == nperseg_expected
    assert curves["hop"] == hop_expected
    frame_count = (n - nperseg_expected) // hop_expected + 1
    assert curves["n_frames"] == frame_count
    expected_freqs = np.fft.rfftfreq(nperseg_expected, d=1.0 / fs)

    frames = [0, 1, frame_count // 2, frame_count - 1]
    for k in frames:
        del requests[:]
        r = stft.frame_spectrum_sync(provider_src, params, k)
        # 1) 频率网格与峰值曲线完全一致（同段长/跳步的 rfftfreq 网格）
        np.testing.assert_array_equal(r["freqs"], expected_freqs)
        assert r["nperseg"] == nperseg_expected
        assert r["hop"] == hop_expected
        assert r["amplitudes"].shape == expected_freqs.shape
        # 2) 取数 O(nperseg)：只读一帧，帧窗口起点 = k*hop
        assert requests == [(0, k * hop_expected, nperseg_expected)]
        # 3) 同一帧的 |rfft| 口径与 peaks 单帧 top1 幅值一致（同窗、同 Σw、同单边倍增）
        peak_f = curves["freq_by_rank"][0, k]
        peak_m = curves["mag_by_rank"][0, k]
        assert np.isfinite(peak_f) and np.isfinite(peak_m)
        idx = int(np.argmin(np.abs(expected_freqs - peak_f)))
        assert expected_freqs[idx] == pytest.approx(peak_f, abs=1e-9)
        assert r["amplitudes"][idx] == pytest.approx(peak_m, abs=1e-6)

    # 无 provider 的整段回退路径与 provider 路径同一帧数值一致
    del requests[:]
    r_provider = stft.frame_spectrum_sync(provider_src, params, 3)
    r_plain = stft.frame_spectrum_sync(plain_src, params, 3)
    np.testing.assert_allclose(r_plain["amplitudes"], r_provider["amplitudes"], rtol=1e-12, atol=0.0)
    assert requests == [(0, 3 * hop_expected, nperseg_expected)]

    # 越界 frame_index：clamp 到 [0, frame_count-1]，不抛异常
    r_first = stft.frame_spectrum_sync(provider_src, params, -5)
    np.testing.assert_array_equal(
        r_first["amplitudes"], stft.frame_spectrum_sync(provider_src, params, 0)["amplitudes"]
    )
    r_last = stft.frame_spectrum_sync(provider_src, params, 10**9)
    np.testing.assert_array_equal(
        r_last["amplitudes"], stft.frame_spectrum_sync(provider_src, params, frame_count - 1)["amplitudes"]
    )
    # 末帧起点 = (frame_count-1)*hop，不会越过 size-nperseg
    assert requests[-1] == (0, (frame_count - 1) * hop_expected, nperseg_expected)

    # 时间 → 帧号：帧中心时间往返一致；超界时间被 clamp
    for k in frames:
        center = (k * hop_expected + nperseg_expected / 2.0) / fs
        assert stft.frame_index_for_time(provider_src, params, center) == k
    assert stft.frame_index_for_time(provider_src, params, -10.0) == 0
    assert stft.frame_index_for_time(provider_src, params, 10.0) == frame_count - 1

    # 去直流/去趋势：需要整段统计量（走整段预处理后取帧的慢路径），口径仍需与 peaks 单帧一致
    k_detrend = frame_count // 3
    for mode in ("constant", "linear"):
        detrend_params = dict(params, detrend=mode)
        detrend_curves = peaks.compute_peak_curves_sync(plain_src, detrend_params)
        r_detrend = stft.frame_spectrum_sync(provider_src, detrend_params, k_detrend)
        peak_f = detrend_curves["freq_by_rank"][0, k_detrend]
        peak_m = detrend_curves["mag_by_rank"][0, k_detrend]
        idx = int(np.argmin(np.abs(expected_freqs - peak_f)))
        assert r_detrend["amplitudes"][idx] == pytest.approx(peak_m, abs=1e-6)
