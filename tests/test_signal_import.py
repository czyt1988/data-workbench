"""DASignalAnalysis 插件导入与采样率推断测试（方案 §13.1）。"""

import os
import sys

import numpy as np
import pytest

# 把插件 Python 包加入 sys.path（conftest 只加了 src/PyScripts）
_PLUGIN_PY = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "plugins", "DASignalAnalysis", "PyScripts",
)
if _PLUGIN_PY not in sys.path:
    sys.path.insert(0, _PLUGIN_PY)

from DASignalAnalysis import importer  # noqa: E402


pytestmark = pytest.mark.signal_import


# ---------------------------------------------------------------- 格式判断


def test_format_classification():
    assert importer.is_direct_audio("a.wav")
    assert importer.is_direct_audio("a.FLAC")
    assert not importer.is_direct_audio("a.mp3")
    assert importer.needs_ffmpeg("a.mp3")
    assert importer.needs_ffmpeg("a.m4a")
    assert importer.is_video("a.mp4")
    assert importer.needs_ffmpeg("a.mkv")
    assert not importer.is_video("a.wav")
    assert not importer.needs_ffmpeg("a.wav")


# ---------------------------------------------------------------- 采样率推断


def test_infer_sample_rate_regular_grid():
    fs_true = 48000.0
    t = np.arange(0, 1.0, 1.0 / fs_true)
    r = importer.infer_sample_rate(t)
    assert r["ok"]
    assert abs(r["sample_rate"] - fs_true) / fs_true < 1e-6
    assert r["duration"] == pytest.approx(t[-1] - t[0], rel=1e-9)


def test_infer_sample_rate_with_gaps_and_duplicates():
    # 等间隔 100 赫兹 + 一个缺失点 + 一个重复点：中位数仍应稳定
    t = np.arange(0, 2.0, 0.01)
    t = np.delete(t, 50)          # 缺失（产生一个 2 倍间隔）
    t = np.insert(t, 10, t[9])    # 重复（产生一个 0 间隔）
    r = importer.infer_sample_rate(t)
    assert r["ok"]
    assert abs(r["sample_rate"] - 100.0) / 100.0 < 0.01


def test_infer_sample_rate_failures():
    assert not importer.infer_sample_rate([1.0])["ok"]
    assert not importer.infer_sample_rate([])["ok"]
    assert not importer.infer_sample_rate([1.0, 1.0, 1.0])["ok"]  # 非严格递增


# ---------------------------------------------------------------- 文件读取


def _write_wav(path, fs=48000, seconds=0.5, freq=1000.0, amp=0.5, channels=1):
    t = np.arange(int(fs * seconds)) / fs
    sig = amp * np.sin(2 * np.pi * freq * t)
    if channels > 1:
        data = np.stack([sig] * channels, axis=1)
    else:
        data = sig
    import soundfile as sf
    sf.write(str(path), data.astype(np.float32), int(fs))
    return data


def test_probe_and_read_range(tmp_path):
    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.5, channels=2)

    probe = importer.probe_audio(wav)
    assert probe["ok"]
    assert probe["sample_rate"] == pytest.approx(48000.0)
    assert probe["channels"] == 2
    assert probe["frames"] == 24000
    assert probe["duration"] == pytest.approx(0.5, rel=1e-9)

    # 分块读取：第 2 通道中段 1000 点
    chunk = importer.read_range(wav, 1, 5000, 1000)
    assert chunk.dtype == np.float32
    assert chunk.size == 1000
    # 与全量读取一致
    full = importer.read_channel_all(wav, 1)
    assert np.allclose(chunk, full[5000:6000], atol=1e-6)

    # 越界读取返回空数组
    assert importer.read_range(wav, 0, 10**9, 100).size == 0
    # 通道越界返回空数组
    assert importer.read_range(wav, 5, 0, 100).size == 0


def test_read_range_missing_file_returns_empty():
    assert importer.read_range("no-such-file.wav", 0, 0, 100).size == 0


# ---------------------------------------------------------------- 数据帧取数


def test_read_dataframe_range():
    pd = pytest.importorskip("pandas")
    df = pd.DataFrame({"t": np.arange(10) * 0.01, "v": np.arange(10) * 0.5})
    chunk = importer.read_dataframe_range(df, "v", 2, 4)
    assert chunk.dtype == np.float32
    assert np.allclose(chunk, np.array([1.0, 1.5, 2.0, 2.5], dtype=np.float32))


def test_infer_sample_rate_from_dataframe():
    pd = pytest.importorskip("pandas")
    fs = 1000.0
    df = pd.DataFrame({"t": np.arange(100) / fs, "v": np.sin(np.arange(100) / 10.0)})
    r = importer.infer_sample_rate_from_column(df, "t")
    assert r["ok"]
    assert abs(r["sample_rate"] - fs) / fs < 1e-6


# ---------------------------------------------------------------- 编排层（频谱）


def test_compute_spectrum_sync_orchestration():
    from DASignalAnalysis import spectrum

    fs = 48000.0
    t = np.arange(int(fs * 0.5)) / fs
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    params = {
        "removeDc": False,
        "detrend": "none",
        "window": "hann",
        "amplitudeMode": "peak",
        "resolutionHz": 10.0,
        "overlap": 50.0,
        "average": "mean",
    }
    r = spectrum.compute_spectrum_sync(x, fs, params)
    # 幅值谱峰值在 1 千赫、幅度约 0.5
    idx = int(np.argmax(r["amplitudes"]))
    assert abs(r["freqs"][idx] - 1000.0) < 2.0
    assert r["amplitudes"][idx] == pytest.approx(0.5, rel=0.05)
    # 段长由分辨率反推：fs/10 = 4800 点
    assert r["seg_len"] == 4800
    assert r["resolution_hz"] == pytest.approx(10.0)
    # 功率谱峰值位置一致
    pidx = int(np.argmax(r["psd_values"]))
    assert abs(r["psd_freqs"][pidx] - 1000.0) <= 10.0


def test_compute_spectrum_sync_remove_dc():
    from DASignalAnalysis import spectrum

    x = np.ones(48000) * 3.0
    params = {"removeDc": True, "detrend": "none", "window": "hann", "resolutionHz": 10.0}
    r = spectrum.compute_spectrum_sync(x, 48000.0, params)
    # 去直流后幅值谱在非 DC 处应接近 0
    assert np.max(r["amplitudes"][1:]) < 1e-6


# ---------------------------------------------------------------- 波形包络


def test_envelope_downsampling_keeps_extremes():
    from DASignalAnalysis import envelope

    rng = np.random.default_rng(3)
    x = rng.normal(size=100000)
    width = 500
    r = envelope.envelope_of_window(x, 0, x.size, width)
    assert r["mins"].size <= width
    assert r["mins"].size == r["maxs"].size == r["x"].size
    # 与原始数据同规则分段比对，确认每段极值不丢失
    edges = np.linspace(0, x.size, width + 1, dtype=np.int64)
    for i in (0, 1, width // 2, width - 1):
        s, e = edges[i], edges[i + 1]
        assert r["mins"][i] == pytest.approx(np.min(x[s:e]), abs=1e-5)
        assert r["maxs"][i] == pytest.approx(np.max(x[s:e]), abs=1e-5)


def test_envelope_empty_and_short_ranges():
    from DASignalAnalysis import envelope

    r = envelope.envelope_of_window(np.zeros(0), 0, 0, 100)
    assert r["mins"].size == 0
    r2 = envelope.envelope_of_window(np.arange(10.0), 3, 6, 100)
    assert r2["mins"].size == 3
    assert r2["x"][0] == pytest.approx(3.5)


# ---------------------------------------------------------------- STFT 显示栅格


def test_stft_heatmap_grid_and_pooling(tmp_path):
    from DASignalAnalysis import stft

    wav = tmp_path / "tone.wav"
    fs = 48000
    _write_wav(wav, fs=fs, seconds=0.5, freq=1000.0, amp=0.5)
    params = {
        "stftWindowMs": 50.0,
        "stftOverlap": 50.0,
        "stftZeroPadding": 0,
        "window": "hann",
        "removeDc": False,
        "detrend": "none",
    }
    src = {"type": "file", "path": str(wav), "sampleRate": fs}
    r = stft.compute_heatmap_sync(src, params, 120, 60)
    # 列数受实际帧数限制（0.5 秒 / 25 毫秒跳步 → 19 帧）
    assert r["rows"] == 60
    assert r["columns"] == r["n_frames"] == 19
    assert len(r["values"]) == 60 * r["columns"]
    # 段长 = 50 毫秒 × 48 千赫 = 2400 点
    assert r["nperseg"] == 2400
    # 峰值行频率接近 1 千赫
    grid = np.asarray(r["values"]).reshape(60, r["columns"])
    row_of_peak = int(np.nanargmax(np.nanmax(grid, axis=1)))
    assert abs(r["freqs"][row_of_peak] - 1000.0) < 60.0
    # 时间轴单调递增，首列中心时间 = nperseg/2/fs
    assert np.all(np.diff(r["times"]) > 0)
    assert r["times"][0] == pytest.approx(2400 / 2.0 / fs, rel=1e-6)
    assert r["window_seconds"] == pytest.approx(0.05, rel=1e-6)


def test_stft_window_truncated_to_signal_length(tmp_path):
    from DASignalAnalysis import stft

    wav = tmp_path / "short.wav"
    _write_wav(wav, fs=48000, seconds=0.01, freq=1000.0)
    src = {"type": "file", "path": str(wav), "sampleRate": 48000}
    # 窗长超过信号时被截断到信号长度，仍得到至少 1 帧
    r = stft.compute_heatmap_sync(src, {"stftWindowMs": 500.0}, 50, 30)
    assert r["nperseg"] == 480
    assert r["n_frames"] == 1
    assert r["columns"] == 1
