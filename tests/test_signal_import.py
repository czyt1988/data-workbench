"""DASignalAnalysis 插件导入与采样率推断测试（方案 §13.1）。"""

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

from DASignalAnalysis import importer  # noqa: E402
from DAWorkbench.DAPyBase import thread_status_manager as tsm  # noqa: E402


pytestmark = pytest.mark.signal_import


def _params(**overrides):
    """默认参数字典（键名与 DASignalAnalysisParams::toJson 一致）。"""
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
    }
    params.update(overrides)
    return params


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


def test_read_range_missing_file_raises():
    """N-I1c：读失败不再静默返回空数组，而是抛出带原因的 RuntimeError（C++ error 已具备）。"""
    with pytest.raises(RuntimeError, match="cannot read audio file"):
        importer.read_range("no-such-file.wav", 0, 0, 100)


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


# ---------------------------------------------------------------- 峰值曲线（单趟扫描）


def test_peak_curves_frequency_sweep(tmp_path):
    from DASignalAnalysis import peaks

    fs = 48000
    seconds = 1.0
    t = np.arange(int(fs * seconds)) / fs
    # 线性上扫：频率从 500 赫兹到 2500 赫兹
    f0, f1 = 500.0, 2500.0
    phase = 2 * np.pi * (f0 * t + 0.5 * (f1 - f0) / seconds * t * t)
    x = 0.5 * np.sin(phase)
    wav = tmp_path / "sweep.wav"
    import soundfile as sf

    sf.write(str(wav), x.astype(np.float32), fs)

    src = {"type": "file", "path": str(wav), "sampleRate": fs}
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
    r = peaks.compute_peak_curves_sync(src, params)
    assert r["top_n"] == 1
    curve = r["freq_by_rank"][0]
    valid = curve[np.isfinite(curve)]
    assert valid.size > 10
    # 第 1 名频率随时间单调上升（允许个别帧抖动：整体趋势用分位比较）
    n = valid.size
    first_quarter = np.median(valid[: n // 4])
    last_quarter = np.median(valid[-n // 4 :])
    assert last_quarter > first_quarter + 1000.0
    # 严格单调比例足够高
    diffs = np.diff(valid)
    assert np.mean(diffs >= 0) > 0.9


# ---------------------------------------------------------------- 数据帧播放导出


def test_export_column_to_wav(tmp_path):
    pd = pytest.importorskip("pandas")

    df = pd.DataFrame({"v": [0.1, -0.2, 0.3, float("nan"), 0.0]})
    out = tmp_path / "export.wav"
    ok = importer.export_column_to_wav(df, "v", str(out), 48000)
    assert ok and out.exists()
    probe = importer.probe_audio(out)
    assert probe["ok"]
    assert probe["sample_rate"] == pytest.approx(48000.0)
    assert probe["channels"] == 1
    assert probe["frames"] == 5
    # NaN 按 0 处理
    data = importer.read_channel_all(out, 0)
    assert data[3] == pytest.approx(0.0)


# ---------------------------------------------------------------- 导出（M10）


def test_exporter_peaks_and_csv(tmp_path):
    from DASignalAnalysis import exporter

    freqs = [1000.0, 2000.0]
    mags = [0.5, 0.2]
    rel = [0.0, -8.0]
    share = [80.0, 20.0]
    df = exporter.peaks_to_dataframe(freqs, mags, rel, share)
    assert list(df.columns) == exporter.PEAK_COLUMNS
    assert len(df) == 2

    csv_path = tmp_path / "peaks.csv"
    assert exporter.write_peaks_csv(str(csv_path), freqs, mags, rel, share)
    # CSV 用 utf-8-sig 写盘：带 BOM，便于表格软件直接打开中文内容
    assert csv_path.read_bytes()[:3] == b"\xef\xbb\xbf"
    lines = csv_path.read_text(encoding="utf-8-sig").strip().splitlines()
    assert len(lines) == 3  # 表头 + 2 行

    spec_path = tmp_path / "spectrum.csv"
    assert exporter.write_spectrum_csv(str(spec_path), [0.0, 1000.0], [0.0, 0.5], "Amplitude")
    spec_lines = spec_path.read_text(encoding="utf-8-sig").strip().splitlines()
    assert spec_lines[0].startswith("Frequency (Hz)")

    heat_path = tmp_path / "heat.csv"
    times = [0.01, 0.02]
    freqs_h = [0.0, 1000.0]
    values = list(range(4))  # 2 行 × 2 列
    assert exporter.write_heatmap_csv(str(heat_path), times, freqs_h, values, 2, 2)
    heat_lines = heat_path.read_text(encoding="utf-8-sig").strip().splitlines()
    # 表头 + 2 行；首列是 Frequency (Hz)
    assert len(heat_lines) == 3
    assert heat_lines[0].startswith("Frequency (Hz)")


# ---------------------------------------------------------------- 数据帧来源（fake da_app）


def test_dataframe_source_uses_da_app(monkeypatch):
    """数据帧来源经宿主注入的顶层 da_app 访问数据管理器（回归：曾误用 from DAWorkbench import da_app）。"""
    import sys
    import types

    pd = pytest.importorskip("pandas")
    from DASignalAnalysis import envelope, spectrum

    fs = 1000.0
    t = np.arange(1000) / fs
    df = pd.DataFrame({"t": t, "y": np.sin(2 * np.pi * 100.0 * t)})

    class FakeDM:
        def getAllDataframes(self):
            return {"df1": df}

    class FakeCore:
        def getDataManagerInterface(self):
            return FakeDM()

    fake = types.ModuleType("da_app")
    fake.getCore = lambda: FakeCore()
    monkeypatch.setitem(sys.modules, "da_app", fake)

    src = {"type": "dataframe", "dataframeName": "df1", "column": "y", "sampleRate": fs}
    r = spectrum.compute_spectrum_sync_from_source(src, {"window": "hann", "resolutionHz": 10.0})
    idx = int(np.argmax(r["amplitudes"]))
    assert abs(r["freqs"][idx] - 100.0) <= 10.0

    env = envelope.compute_envelope_sync(src, 0, 0, 1000, 100)
    assert env["mins"].size == 100

    # 缺数据帧时给出明确错误（而不是 ImportError）
    with pytest.raises(RuntimeError):
        spectrum.compute_spectrum_sync_from_source(
            {"type": "dataframe", "dataframeName": "nope", "column": "y", "sampleRate": fs}, {}
        )


# ---------------------------------------------------------------- 频谱编排口径（C2/C3/C4/C6/C7）


def test_spectrum_zero_padding_is_extra_points():
    """C3：频谱侧 zeroPadding 为「额外点数」（与 STFT 侧一致）。"""
    from DASignalAnalysis import spectrum

    fs = 48000.0
    t = np.arange(int(fs * 0.5)) / fs
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    r0 = spectrum.compute_spectrum_sync(x, fs, _params())
    r1 = spectrum.compute_spectrum_sync(x, fs, _params(zeroPadding=4800))

    # 段长与实际分辨率不因零填充改变
    assert r0["seg_len"] == r1["seg_len"] == 4800
    assert r0["resolution_hz"] == pytest.approx(10.0)
    assert r1["resolution_hz"] == pytest.approx(10.0)
    # 段长 4800 + 额外 4800 → nfft 9600 → 4801 个频点；默认 nfft 为 4801 个
    assert r0["freqs"].size == 4800 // 2 + 1
    assert r1["freqs"].size == 9600 // 2 + 1
    assert r1["freqs"][1] - r1["freqs"][0] == pytest.approx(48000.0 / 9600)
    # 峰值频率不变，只是频点变密
    assert r0["freqs"][int(np.argmax(r0["amplitudes"]))] == pytest.approx(1000.0)
    assert r1["freqs"][int(np.argmax(r1["amplitudes"]))] == pytest.approx(1000.0)


def test_spectrum_resolution_controls_segment_and_bins():
    """C4：频率分辨率决定段长与 Δf，进而改变频点与峰值 bin。"""
    from DASignalAnalysis import spectrum

    fs = 48000.0
    t = np.arange(int(fs * 0.5)) / fs
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    coarse = spectrum.compute_spectrum_sync(x, fs, _params(resolutionHz=100.0))
    fine = spectrum.compute_spectrum_sync(x, fs, _params(resolutionHz=10.0))

    assert coarse["seg_len"] == 480
    assert fine["seg_len"] == 4800
    assert coarse["resolution_hz"] == pytest.approx(100.0)
    assert fine["resolution_hz"] == pytest.approx(10.0)
    assert coarse["freqs"][1] - coarse["freqs"][0] == pytest.approx(100.0)
    assert fine["freqs"][1] - fine["freqs"][0] == pytest.approx(10.0)
    # 1 千赫分别落在第 10 / 第 100 个 bin
    assert int(np.argmax(coarse["amplitudes"])) == 10
    assert int(np.argmax(fine["amplitudes"])) == 100
    # 分段平均不改变单音峰值幅度
    assert coarse["amplitudes"][10] == pytest.approx(0.5, rel=0.05)
    assert fine["amplitudes"][100] == pytest.approx(0.5, rel=0.05)


def test_spectrum_amplitude_mode_is_display_only():
    """C2：amplitudeMode 不参与计算层，始终返回峰值幅度（rms 字段固定 False）。"""
    from DASignalAnalysis import spectrum

    fs = 48000.0
    t = np.arange(int(fs * 0.5)) / fs
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    peak = spectrum.compute_spectrum_sync(x, fs, _params(amplitudeMode="peak"))
    rms = spectrum.compute_spectrum_sync(x, fs, _params(amplitudeMode="rms"))

    assert peak["rms"] is False
    assert rms["rms"] is False
    assert np.allclose(peak["amplitudes"], rms["amplitudes"])
    # 峰值口径：正弦峰值幅度 ≈ 0.5（而不是均方根 0.354）
    idx = int(np.argmax(rms["amplitudes"]))
    assert rms["amplitudes"][idx] == pytest.approx(0.5, rel=0.05)


def test_spectrum_nan_inf_sanitized(monkeypatch):
    """C6：数据帧列里的 NaN/Inf 在读取时归零，不污染 FFT 与包络。"""
    import sys
    import types

    pd = pytest.importorskip("pandas")
    from DASignalAnalysis import envelope, spectrum

    fs = 1000.0
    t = np.arange(1000) / fs
    y = np.sin(2 * np.pi * 100.0 * t).copy()
    y[10] = np.nan
    y[11] = np.inf
    y[12] = -np.inf
    df = pd.DataFrame({"t": t, "y": y})

    class FakeDM:
        def getAllDataframes(self):
            return {"df1": df}

    class FakeCore:
        def getDataManagerInterface(self):
            return FakeDM()

    fake = types.ModuleType("da_app")
    fake.getCore = lambda: FakeCore()
    monkeypatch.setitem(sys.modules, "da_app", fake)

    src = {"type": "dataframe", "dataframeName": "df1", "column": "y", "sampleRate": fs}
    values = spectrum._load_channel(src)
    assert np.all(np.isfinite(values))
    assert values[10] == pytest.approx(0.0)
    assert values[11] == pytest.approx(0.0)
    assert values[12] == pytest.approx(0.0)

    r = spectrum.compute_spectrum_sync_from_source(src, _params(resolutionHz=10.0))
    assert np.all(np.isfinite(r["amplitudes"]))
    assert np.all(np.isfinite(r["psd_values"]))

    env = envelope.compute_envelope_sync(src, 0, 0, 1000, 100)
    assert np.all(np.isfinite(env["mins"]))
    assert np.all(np.isfinite(env["maxs"]))


def test_spectrum_cancellation_aborts():
    """C7：频谱分块循环命中取消标志时抛 TaskCanceledError。"""
    from DASignalAnalysis import spectrum

    task_id, _status = tsm.create_task_with_status("cancel spectrum")
    assert tsm.cancel_task(task_id)
    assert tsm.is_task_canceled(task_id)
    fs = 48000.0
    t = np.arange(int(fs * 0.5)) / fs
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    with pytest.raises(tsm.TaskCanceledError):
        spectrum.compute_spectrum_sync(x, fs, _params(), task_id=task_id)


def test_stft_cancellation_aborts(tmp_path):
    """C7：STFT 分块循环命中取消标志时抛 TaskCanceledError。"""
    from DASignalAnalysis import stft

    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.5)
    task_id, _status = tsm.create_task_with_status("cancel stft")
    assert tsm.cancel_task(task_id)
    src = {"type": "file", "path": str(wav), "sampleRate": 48000}
    with pytest.raises(tsm.TaskCanceledError):
        stft.compute_heatmap_sync(src, {"stftWindowMs": 20.0}, 100, 50, task_id=task_id)


# ---------------------------------------------------------------- 导入降级与 dtype 分派（I1/I5）


def test_soundfile_available_selfcheck():
    """D3/N-test1：依赖可用性自检入口（无 soundfile 的部署环境自动 skip，不误报失败）。"""
    pytest.importorskip("soundfile")
    assert importer.soundfile_available() is True


def test_column_as_array_datetime64():
    """I5：datetime64 时间列转秒后可直接推断采样率。"""
    pd = pytest.importorskip("pandas")

    stamps = pd.date_range("2024-01-01 00:00:00", periods=200, freq="1ms")
    df = pd.DataFrame({"t": stamps, "v": np.arange(200, dtype=float)})
    arr = importer.column_as_array(df, "t")
    assert arr.dtype == np.float64
    # float64 秒（绝对时间戳）在 2024 年附近分辨率约 0.24 微秒，毫秒间隔的相对误差 ~1e-4
    assert np.allclose(np.diff(arr), 0.001, rtol=1e-3)
    r = importer.infer_sample_rate_from_column(df, "t")
    assert r["ok"]
    assert r["sample_rate"] == pytest.approx(1000.0, rel=1e-3)


def test_soundfile_missing_wav_scipy_fallback(tmp_path, monkeypatch):
    """I1：soundfile 缺失时 WAV 走 scipy.io.wavfile 兜底（读取路径）。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.5, freq=1000.0, amp=0.5, channels=1)
    # 写盘后再模拟运行时缺 soundfile
    monkeypatch.setitem(sys.modules, "soundfile", None)
    assert importer.soundfile_available() is False

    probe = importer.probe_audio(wav)
    assert probe["ok"]
    assert probe["sample_rate"] == pytest.approx(48000.0)
    assert probe["channels"] == 1
    assert probe["frames"] == 24000
    assert probe["duration"] == pytest.approx(0.5, rel=1e-9)

    chunk = importer.read_range(wav, 0, 100, 500)
    assert chunk.dtype == np.float32
    assert chunk.size == 500
    full = importer.read_channel_all(wav, 0)
    assert full.size == 24000
    assert np.allclose(chunk, full[100:600], atol=1e-7)
    # 与写入的正弦一致（PCM16 量化误差以内）
    t = np.arange(24000) / 48000.0
    expected = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    assert np.max(np.abs(full - expected)) < 1e-2
    # 越界与通道越界仍返回空数组
    assert importer.read_range(wav, 0, 10**9, 100).size == 0
    assert importer.read_range(wav, 3, 0, 100).size == 0


def test_soundfile_missing_other_format_reports_error(tmp_path, monkeypatch):
    """I1：soundfile 缺失且非 WAV 时给出明确错误信息，不静默失败。"""
    monkeypatch.setitem(sys.modules, "soundfile", None)
    fake = tmp_path / "tone.flac"
    fake.write_bytes(b"not a flac")

    probe = importer.probe_audio(fake)
    assert not probe["ok"]
    assert "soundfile is not available" in probe["error"]

    with pytest.raises(RuntimeError, match="soundfile is not available"):
        importer.read_range(fake, 0, 0, 10)
    with pytest.raises(RuntimeError, match="soundfile is not available"):
        importer.read_channel_all(fake, 0)


def test_soundfile_missing_export_wav_fallback(tmp_path, monkeypatch):
    """I1：soundfile 缺失时导出数据帧列走 scipy.io.wavfile（float32 WAV）。"""
    pd = pytest.importorskip("pandas")
    from scipy.io import wavfile

    df = pd.DataFrame({"v": [0.1, -0.2, 0.3, float("nan"), 0.0]})
    out = tmp_path / "export.wav"
    monkeypatch.setitem(sys.modules, "soundfile", None)
    assert importer.export_column_to_wav(df, "v", str(out), 48000)
    assert out.exists()

    rate, data = wavfile.read(str(out))
    assert rate == 48000
    assert data.dtype == np.float32
    assert data.size == 5
    assert data[3] == pytest.approx(0.0)
    assert data[0] == pytest.approx(0.1, abs=1e-6)


# ---------------------------------------------------------------- 包导出清单（D7）


def test_package_all_lists_all_modules():
    import importlib

    import DASignalAnalysis

    expected = {"importer", "spectrum", "stft", "peaks", "envelope", "exporter"}
    assert set(DASignalAnalysis.__all__) == expected
    for name in expected:
        assert importlib.import_module("DASignalAnalysis." + name) is not None


# ---------------------------------------------------------------- N-C1b 单通道 float32 取数

def _wait_task(task_id, timeout=10.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        info = tsm.get_task_status(task_id)
        if info and not info["is_running"]:
            return info
        time.sleep(0.02)
    raise AssertionError("task did not finish in %.1f s" % timeout)


def test_read_channel_all_reads_single_channel_in_blocks(monkeypatch):
    """N-C1b：soundfile 路径按块读取，且只保留目标通道（float32）。"""
    import types

    total = 2500
    base = np.arange(total, dtype=np.float32)
    two_ch = np.stack([base, -base], axis=1)
    requests = []

    class FakeSoundFile:
        def __init__(self, path):
            self.path = path
            self.frames = total
            self.channels = 2

        def __enter__(self):
            return self

        def __exit__(self, *args):
            return False

        def read(self, frames=-1, dtype="float64", always_2d=False):
            requests.append(int(frames))
            start = sum(requests[:-1])
            return np.ascontiguousarray(two_ch[start : start + frames])

    fake_sf = types.SimpleNamespace(SoundFile=FakeSoundFile)
    monkeypatch.setattr(importer, "_IO_BLOCK_FRAMES", 1000)
    monkeypatch.setattr(importer, "_load_soundfile", lambda: fake_sf)

    out = importer.read_channel_all("unused.wav", 1)
    assert out.dtype == np.float32
    assert out.size == total
    assert np.array_equal(out, -base)
    assert len(requests) == 3
    assert max(requests) <= 1000


def test_load_channel_returns_float32(tmp_path, monkeypatch):
    """N-C1b：频谱编排取数（文件与数据帧两路）都返回 float32，不再整段升 float64。"""
    from DASignalAnalysis import spectrum

    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.2)
    src = {"type": "file", "path": str(wav), "sampleRate": 48000}
    values = spectrum._load_channel(src)
    assert values.dtype == np.float32

    pd = pytest.importorskip("pandas")

    class FakeDM:
        def getAllDataframes(self):
            return {"df1": pd.DataFrame({"y": np.arange(100, dtype=np.float64)})}

    class FakeCore:
        def getDataManagerInterface(self):
            return FakeDM()

    import types

    fake_app = types.ModuleType("da_app")
    fake_app.getCore = lambda: FakeCore()
    monkeypatch.setitem(sys.modules, "da_app", fake_app)
    df_src = {"type": "dataframe", "dataframeName": "df1", "column": "y", "sampleRate": 100.0}
    assert spectrum._load_channel(df_src).dtype == np.float32


def test_spectrum_sync_float32_and_float64_inputs_match():
    """N-C1b：float32 取数路径与 float64 输入的计算结果一致。"""
    from DASignalAnalysis import spectrum

    fs = 48000.0
    t = np.arange(int(fs * 0.5)) / fs
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    r32 = spectrum.compute_spectrum_sync(x.astype(np.float32), fs, _params())
    r64 = spectrum.compute_spectrum_sync(x, fs, _params())
    assert np.allclose(r32["freqs"], r64["freqs"])
    # 绝对门限覆盖 float32 量化在 FFT 噪声底引入的 ~1e-8 级差异，相对门限约束主瓣/峰值
    assert np.allclose(r32["amplitudes"], r64["amplitudes"], rtol=1e-4, atol=1e-7)
    assert np.allclose(r32["psd_values"], r64["psd_values"], rtol=1e-4, atol=1e-8)


# ---------------------------------------------------------------- N-I1a WAV 兜底按帧读取

def test_probe_wav_reads_header_only(tmp_path, monkeypatch):
    """N-I1a：缺 soundfile 时 probe_audio 只解析 WAV 头，不整文件读入。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.5, channels=2)
    monkeypatch.setitem(sys.modules, "soundfile", None)

    def _no_full_read(*args, **kwargs):
        raise AssertionError("probe_audio must not read sample data")

    monkeypatch.setattr(importer, "_read_wav_channel_all", _no_full_read)
    monkeypatch.setattr(importer, "_read_wav_channel_range", _no_full_read)
    probe = importer.probe_audio(wav)
    assert probe["ok"]
    assert probe["sample_rate"] == pytest.approx(48000.0)
    assert probe["channels"] == 2
    assert probe["frames"] == 24000


def test_wav_fallback_reading_matches_soundfile(tmp_path, monkeypatch):
    """N-I1a：标准库 RIFF 按帧 seek 与 soundfile 读取结果一致。"""
    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.2, channels=2)
    full_sf = importer.read_channel_all(wav, 1)
    chunk_sf = importer.read_range(wav, 1, 300, 400)

    monkeypatch.setitem(sys.modules, "soundfile", None)
    full_fb = importer.read_channel_all(wav, 1)
    chunk_fb = importer.read_range(wav, 1, 300, 400)

    assert full_fb.dtype == chunk_fb.dtype == np.float32
    assert np.array_equal(full_sf, full_fb)
    assert np.array_equal(chunk_sf, chunk_fb)


def test_wav_fallback_reads_ieee_float_wav(tmp_path, monkeypatch):
    """N-I1a：兜底解析支持 IEEE float WAV（FFmpeg pcm_f32le 产物），且可 seek。"""
    from scipy.io import wavfile

    data = (np.arange(100, dtype=np.float32) / 100.0).astype(np.float32)
    path = tmp_path / "f32.wav"
    wavfile.write(str(path), 8000, data)
    monkeypatch.setitem(sys.modules, "soundfile", None)

    probe = importer.probe_audio(path)
    assert probe["ok"]
    assert probe["sample_rate"] == pytest.approx(8000.0)
    assert probe["frames"] == 100
    full = importer.read_channel_all(path, 0)
    assert full.dtype == np.float32
    assert np.allclose(full, data)
    assert np.allclose(importer.read_range(path, 0, 10, 20), data[10:30])


def test_wav_read_failure_raises_not_empty(tmp_path, monkeypatch):
    """N-I1c：坏 WAV 读取失败要抛错，不静默返回空数组。"""
    bad = tmp_path / "bad.wav"
    bad.write_bytes(b"this is not a wave file")
    monkeypatch.setitem(sys.modules, "soundfile", None)
    probe = importer.probe_audio(bad)
    assert not probe["ok"]
    assert probe["error"]
    with pytest.raises(RuntimeError):
        importer.read_range(bad, 0, 0, 10)
    with pytest.raises(RuntimeError):
        importer.read_channel_all(bad, 0)


def test_load_soundfile_catches_missing_dll_oserror(monkeypatch):
    """N-I1b：Windows 缺 libsndfile DLL 抛 OSError 时也要走降级并说明原因。"""
    import builtins

    real_import = builtins.__import__

    def fake_import(name, globals=None, locals=None, fromlist=(), level=0):
        if name == "soundfile" or name.startswith("soundfile."):
            raise OSError("Could not find module 'libsndfile-1.dll' (or one of its dependencies)")
        return real_import(name, globals, locals, fromlist, level)

    monkeypatch.setattr(importer, "_SOUNDFILE_ERROR", "")
    monkeypatch.setattr(builtins, "__import__", fake_import)
    assert importer.soundfile_available() is False
    message = importer._soundfile_missing_message()
    assert "soundfile is not available" in message
    assert "libsndfile-1.dll" in message


# ---------------------------------------------------------------- U9 数据帧播放 WAV 后台导出

def _fake_dataframe_app(monkeypatch, df):
    import types

    class FakeDM:
        def getAllDataframes(self):
            return {"df1": df}

    class FakeCore:
        def getDataManagerInterface(self):
            return FakeDM()

    fake_app = types.ModuleType("da_app")
    fake_app.getCore = lambda: FakeCore()
    monkeypatch.setitem(sys.modules, "da_app", fake_app)


def test_export_playback_wav_async_writes_file(monkeypatch, tmp_path):
    """U9：数据帧列在守护线程导出临时 WAV，结果经 take_result 取走。"""
    pd = pytest.importorskip("pandas")
    fs = 8000.0
    df = pd.DataFrame({"v": np.sin(2 * np.pi * 100.0 * np.arange(800) / fs)})
    _fake_dataframe_app(monkeypatch, df)
    out = tmp_path / "playback.wav"
    src = {"type": "dataframe", "dataframeName": "df1", "column": "v", "sampleRate": fs}

    task_id = importer.export_playback_wav_async(src, str(out))
    info = _wait_task(task_id)
    assert info["is_success"] is True
    result = importer.take_result(task_id)
    assert result is not None
    assert result["path"] == str(out)
    assert result["sample_rate"] == pytest.approx(fs)
    assert out.exists()
    probe = importer.probe_audio(out)
    assert probe["ok"]
    assert probe["sample_rate"] == pytest.approx(fs)
    assert probe["frames"] == 800
    assert importer.take_result(task_id) is None


def test_export_playback_wav_async_cancel_removes_partial(monkeypatch, tmp_path):
    """U9/C7：导出被取消时不写结果表，并删除半成品 WAV。"""
    pd = pytest.importorskip("pandas")
    df = pd.DataFrame({"v": np.arange(64, dtype=np.float64)})
    _fake_dataframe_app(monkeypatch, df)
    out = tmp_path / "playback.wav"
    started = threading.Event()
    proceed = threading.Event()

    def fake_export(dataframe, column, path, sample_rate, cancel_check=None):
        started.set()
        assert proceed.wait(5.0)
        with open(path, "wb") as f:
            f.write(b"partial")
        if cancel_check is not None:
            cancel_check()
        return True

    monkeypatch.setattr(importer, "export_column_to_wav", fake_export)
    src = {"type": "dataframe", "dataframeName": "df1", "column": "v", "sampleRate": 8000.0}
    task_id = importer.export_playback_wav_async(src, str(out))
    assert started.wait(5.0)
    assert importer.cancel_task(task_id)
    proceed.set()
    info = _wait_task(task_id)
    assert info["is_canceled"] is True
    time.sleep(0.1)
    assert importer.take_result(task_id) is None
    assert not out.exists()


# ---------------------------------------------------------------- 返回字典契约（§11.5 第 9 项）

def test_spectrum_sync_return_contract():
    """spectrum 返回键与 C++ DASpectrumResult 读取键一致，矩阵长度自洽。"""
    from DASignalAnalysis import spectrum

    fs = 48000.0
    t = np.arange(int(fs * 0.5)) / fs
    x = 0.5 * np.sin(2 * np.pi * 1000.0 * t)
    r = spectrum.compute_spectrum_sync(x, fs, _params())
    assert set(r.keys()) == {
        "freqs",
        "amplitudes",
        "psd_freqs",
        "psd_values",
        "seg_len",
        "resolution_hz",
        "window",
        "rms",
    }
    assert r["freqs"].size == r["amplitudes"].size
    assert r["psd_freqs"].size == r["psd_values"].size
    assert isinstance(r["seg_len"], int)
    assert isinstance(r["resolution_hz"], float)
    assert r["rms"] is False
    assert r["window"] == "hann"


def test_heatmap_sync_return_contract(tmp_path):
    """stft 返回键与 C++ DAStftResult 读取键一致，栅格长度 = rows × columns。"""
    from DASignalAnalysis import stft

    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.5)
    src = {"type": "file", "path": str(wav), "sampleRate": 48000}
    r = stft.compute_heatmap_sync(src, {"stftWindowMs": 20.0, "stftOverlap": 50.0}, 40, 20)
    assert set(r.keys()) == {
        "times",
        "freqs",
        "values",
        "rows",
        "columns",
        "nperseg",
        "hop",
        "n_frames",
        "window_seconds",
    }
    # C++ 读取的 8 个键必须全部存在
    assert {"times", "freqs", "values", "rows", "columns", "nperseg", "hop", "window_seconds"} <= set(r.keys())
    assert r["values"].size == r["rows"] * r["columns"]
    assert r["times"].size == r["columns"]
    assert r["freqs"].size == r["rows"]


def test_envelope_sync_return_contract(tmp_path):
    """envelope 返回键与 C++ envelopeReady 读取键一致。"""
    from DASignalAnalysis import envelope

    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=48000, seconds=0.5)
    src = {"type": "file", "path": str(wav), "sampleRate": 48000}
    r = envelope.compute_envelope_sync(src, 0, 0, 24000, 200)
    assert set(r.keys()) == {"x", "mins", "maxs"}
    assert r["x"].size == r["mins"].size == r["maxs"].size
    assert r["x"].size <= 200


def test_envelope_cancel_helpers():
    """C7：包络模块补齐与 spectrum/stft/peaks 一致的取消入口（worker 取消链路按 kind 分发）。"""
    from DASignalAnalysis import envelope

    task_id, _status = tsm.create_task_with_status("cancel envelope")
    assert envelope.is_cancelled(task_id) is False
    assert envelope.cancel_task(task_id) is True
    assert envelope.is_cancelled(task_id) is True


def test_export_column_to_wav_accepts_cancel_check(tmp_path):
    """U9：同步导出的可选取消检查会被调用（后台异步编排复用同一实现）。"""
    pd = pytest.importorskip("pandas")
    df = pd.DataFrame({"v": [0.1, -0.2, 0.3]})
    out = tmp_path / "cancel.wav"
    calls = []

    def cancel_check():
        calls.append(1)
        raise RuntimeError("cancelled")

    with pytest.raises(RuntimeError, match="cancelled"):
        importer.export_column_to_wav(df, "v", str(out), 48000, cancel_check=cancel_check)
    assert len(calls) == 1

# ---------------------------------------------------------------- 跨语言参数键契约（D2 / 测试缺口）

# C++ DASignalAnalysisParams::toPythonDict() 的键集合，与 DASignalAnalysisTest::paramsPythonDictContract
# 用例保持同源；Python 编排只允许读取这里列出的键，防止 C++ 侧改名后 Python 静默取默认值。
_CPP_PYTHON_PARAM_KEYS = {
    "removeDc", "detrend", "window", "amplitudeMode", "dbReference", "resolutionHz",
    "zeroPadding", "average", "overlap", "stftWindowMs", "stftOverlap", "stftZeroPadding",
    "peakMode", "peakTopN", "peakRelDb", "peakProminence", "peakMinSpacingHz",
    "peakMinHeight", "calibration", "unit",
}
# 仅显示/导出层使用、Python 计算编排不读取的键
_DISPLAY_ONLY_PARAM_KEYS = {"amplitudeMode", "dbReference", "calibration", "unit"}


def _module_param_keys(module):
    """扫描模块内函数源码，收集 params.get("...") / params["..."] 读取的键名。"""
    import inspect
    import re

    keys = set()
    for _name, obj in vars(module).items():
        if not inspect.isfunction(obj) or obj.__module__ != module.__name__:
            continue
        try:
            src = inspect.getsource(obj)
        except (OSError, TypeError):
            continue
        keys |= set(re.findall(r"params\.get\(\s*[\"']([A-Za-z_][A-Za-z0-9_]*)[\"']", src))
        keys |= set(re.findall(r"params\[\s*[\"']([A-Za-z_][A-Za-z0-9_]*)[\"']\s*\]", src))
    return keys


def test_plugin_python_param_key_contract():
    """跨语言输入契约：C++ 传的 20 个参数键中，16 个必须被 spectrum/stft/peaks 读取。"""
    from DASignalAnalysis import peaks, spectrum, stft

    used = _module_param_keys(spectrum) | _module_param_keys(stft) | _module_param_keys(peaks)
    # C++ 侧多传不被读取的键可以容忍；但 Python 读取的键必须都在 C++ 契约集合里（否则取默认值静默跑偏）
    assert used <= _CPP_PYTHON_PARAM_KEYS, "python reads undeclared param keys: %s" % sorted(
        used - _CPP_PYTHON_PARAM_KEYS
    )
    missing = (_CPP_PYTHON_PARAM_KEYS - _DISPLAY_ONLY_PARAM_KEYS) - used
    assert not missing, "declared compute params not read by python orchestration: %s" % sorted(missing)


# ---------------------------------------------------------------- D1-a / D1-b 按块取数（readRange）

def test_read_priority_spectrum_uses_reader_callable():
    """D1-a：source 带 read callable 时频谱走 reader（不碰 path / da_app）。"""
    from DASignalAnalysis import spectrum

    fs = 1000.0
    n = 1000
    t = np.arange(n) / fs
    x = (0.5 * np.sin(2.0 * np.pi * 100.0 * t)).astype(np.float32)
    calls = []

    def read(channel, start, count):
        calls.append((int(channel), int(start), int(count)))
        assert int(channel) == 0
        assert int(start) >= 0 and int(count) > 0 and int(start) + int(count) <= n
        return x[int(start) : int(start) + int(count)]

    # path 指向不存在的文件：若走 _load_channel 会直接抛错，从而证明 reader 优先
    src = {
        "type": "file",
        "path": "no-such-file.wav",
        "sampleRate": fs,
        "channel": 0,
        "sampleCount": n,
        "read": read,
    }
    assert spectrum._source_provider(src) is not None
    assert spectrum._source_provider({"type": "file", "path": "x.wav"}) is None

    r = spectrum.compute_spectrum_sync_from_source(src, _params(resolutionHz=10.0))
    assert calls, "reader was not used"
    assert all(c[0] == 0 for c in calls)
    assert r["seg_len"] == 100
    idx = int(np.argmax(r["amplitudes"]))
    assert abs(r["freqs"][idx] - 100.0) <= 10.0
    assert np.all(np.isfinite(r["amplitudes"]))
    assert np.all(np.isfinite(r["psd_values"]))

    # 与整段数组版结果一致（同一批样本）
    ref = spectrum.compute_spectrum_sync(x, fs, _params(resolutionHz=10.0))
    assert np.array_equal(ref["freqs"], r["freqs"])
    assert np.allclose(ref["amplitudes"], r["amplitudes"], rtol=1e-5, atol=1e-9)
    assert np.allclose(ref["psd_values"], r["psd_values"], rtol=1e-5, atol=0.0)


def test_stft_provider_matches_whole_signal(tmp_path):
    """D1-b：带 read provider 的 STFT 栅格与整段路径一致，且按块读取不整段读入。"""
    from DASignalAnalysis import stft

    fs = 48000
    seconds = 10.0
    wav = tmp_path / "long_tone.wav"
    _write_wav(wav, fs=fs, seconds=seconds, freq=1000.0, amp=0.5)
    sample_count = int(fs * seconds)
    params = {
        "stftWindowMs": 50.0,
        "stftOverlap": 50.0,
        "stftZeroPadding": 0,
        "window": "hann",
        "removeDc": False,
        "detrend": "none",
    }
    plain = {"type": "file", "path": str(wav), "sampleRate": fs}
    calls = []

    def read(channel, start, count):
        calls.append((int(channel), int(start), int(count)))
        return importer.read_range(wav, channel, start, count)

    src = {
        "type": "file",
        "path": str(wav),
        "sampleRate": fs,
        "channel": 0,
        "sampleCount": sample_count,
        "read": read,
    }
    whole = stft.compute_heatmap_sync(plain, params, 120, 60)
    streamed = stft.compute_heatmap_sync(src, params, 120, 60)

    assert calls, "reader was not used"
    assert all(c[0] == 0 for c in calls)
    # 不整段读入：每次请求都短于总采样数，且 399 帧需要多块
    assert max(c[2] for c in calls) < sample_count
    assert len(calls) >= 2

    # 栅格形状/轴一致
    assert streamed["rows"] == whole["rows"] == 60
    assert streamed["columns"] == whole["columns"]
    assert streamed["nperseg"] == whole["nperseg"] == 2400
    assert streamed["hop"] == whole["hop"] == 1200
    assert streamed["n_frames"] == whole["n_frames"]
    np.testing.assert_array_equal(streamed["times"], whole["times"])
    np.testing.assert_array_equal(streamed["freqs"], whole["freqs"])

    ga = np.asarray(whole["values"]).reshape(whole["rows"], whole["columns"])
    gb = np.asarray(streamed["values"]).reshape(streamed["rows"], streamed["columns"])
    assert np.array_equal(np.isnan(ga), np.isnan(gb))
    valid = ~np.isnan(ga)
    assert np.allclose(gb[valid], ga[valid], rtol=1e-5)


def test_stft_provider_detrend_matches_whole_signal(tmp_path):
    """D1-b：去直流/线性去趋势时流式 STFT 与整段路径仍在容差内一致。"""
    from DASignalAnalysis import stft

    fs = 48000
    wav = tmp_path / "drift_tone.wav"
    n = 48000
    t = np.arange(n) / fs
    # 带直流偏置 + 线性漂移的单音
    x = (0.4 * np.sin(2.0 * np.pi * 1000.0 * t) + 0.5 + 2.0e-4 * np.arange(n)).astype(np.float32)
    import soundfile as sf

    sf.write(str(wav), x, fs)

    base = {
        "stftWindowMs": 50.0,
        "stftOverlap": 50.0,
        "window": "hann",
        "removeDc": False,
        "detrend": "none",
    }
    plain = {"type": "file", "path": str(wav), "sampleRate": fs}
    stream_src = {
        "type": "file",
        "path": str(wav),
        "sampleRate": fs,
        "channel": 0,
        "sampleCount": n,
        "read": lambda ch, start, count: importer.read_range(wav, ch, start, count),
    }
    for mode in ("constant", "linear"):
        params = dict(base, detrend=mode, removeDc=False)
        whole = stft.compute_heatmap_sync(plain, params, 80, 40)
        streamed = stft.compute_heatmap_sync(stream_src, params, 80, 40)
        np.testing.assert_array_equal(streamed["times"], whole["times"])
        np.testing.assert_array_equal(streamed["freqs"], whole["freqs"])
        ga = np.asarray(whole["values"]).reshape(whole["rows"], whole["columns"])
        gb = np.asarray(streamed["values"]).reshape(streamed["rows"], streamed["columns"])
        assert np.array_equal(np.isnan(ga), np.isnan(gb))
        valid = ~np.isnan(ga)
        # constant：provider 用 float64 全局均值（与 detrend_signal 同口径），差异仅求和顺序；
        # linear：provider 用 float64 闭式最小二乘，整段路径用 scipy 的 float32 lstsq，
        # 两者拟合系数存在 float32 量化差（相对信号约 1e-5），因此放宽到 rtol=1e-4/atol=1e-6。
        if mode == "linear":
            assert np.allclose(gb[valid], ga[valid], rtol=1e-4, atol=1e-6), mode
        else:
            assert np.allclose(gb[valid], ga[valid], rtol=1e-5, atol=1e-9), mode


def test_spectrum_provider_remove_dc_matches_whole_path(tmp_path):
    """D1-a：removeDc 合成 constant 去趋势，provider 路径与整段路径一致且只统计一遍趋势。"""
    from DASignalAnalysis import spectrum

    fs = 48000
    n = 24000
    wav = tmp_path / "tone_dc.wav"
    t = np.arange(n) / fs
    x = (0.5 * np.sin(2.0 * np.pi * 1000.0 * t) + 0.35).astype(np.float32)
    import soundfile as sf

    sf.write(str(wav), x, fs)

    params = _params(removeDc=True, detrend="none", resolutionHz=10.0)
    plain = {"type": "file", "path": str(wav), "sampleRate": fs}
    calls = []

    def read(channel, start, count):
        calls.append((int(start), int(count)))
        return importer.read_range(wav, channel, start, count)

    stream_src = {
        "type": "file",
        "path": str(wav),
        "sampleRate": fs,
        "channel": 0,
        "sampleCount": n,
        "read": read,
    }
    whole = spectrum.compute_spectrum_sync_from_source(plain, params)
    streamed = spectrum.compute_spectrum_sync_from_source(stream_src, params)

    assert calls, "reader was not used"
    assert np.array_equal(whole["freqs"], streamed["freqs"])
    assert np.allclose(streamed["amplitudes"], whole["amplitudes"], rtol=1e-5, atol=1e-9)
    assert np.allclose(streamed["psd_values"], whole["psd_values"], rtol=1e-5, atol=1e-12)
    # 去掉 DC 后 DC 频点幅值应接近 0，而 1 千赫单音仍保留（确认 removeDc 真的生效）
    assert abs(streamed["amplitudes"][0]) < 1e-6
    assert np.max(streamed["amplitudes"][1:]) == pytest.approx(0.5, rel=0.05)

    # 请求序列：1 遍趋势统计 + 幅度谱 1 遍 + PSD 1 遍（共用统计结果，不是 4 遍）
    passes = []
    prev_start = None
    for start, _count in calls:
        if prev_start is None or start <= prev_start:
            passes.append([])
        passes[-1].append(start)
        prev_start = start
    assert len(passes) == 3, "expected 1 stats pass + 2 spectrum passes, got %d" % len(passes)

# ---------------------------------------------------------------- I2：导入后台准备

def test_prepare_import_direct_wav_no_transcode(tmp_path):
    """I2：直读 WAV 不转码，read_path 就是源文件；结果携带采样率供界面接线。"""
    from DASignalAnalysis import importer

    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=8000, seconds=1.0)
    out = tmp_path / "out.wav"
    task_id = importer.prepare_import_async(str(wav), "audio", "", str(out))
    status = None
    for _ in range(1000):
        info = tsm.get_task_status(task_id)
        if info is not None and not info["is_running"]:
            status = info
            break
        time.sleep(0.01)
    assert status is not None and status["is_success"] is True, status
    r = importer.take_result(task_id)
    assert r["ok"] is True
    assert r["read_path"] == str(wav)
    assert r["is_temporary"] is False
    assert r["sample_rate"] == pytest.approx(8000.0)
    assert not out.exists()  # 没有转码就不应产生临时文件


def test_prepare_import_video_without_ffmpeg_fails_clearly(tmp_path):
    """I2：视频导入需要 FFmpeg，缺失时任务失败且带明确原因（不静默空结果）。"""
    from DASignalAnalysis import importer

    fake = tmp_path / "fake.mp4"
    fake.write_bytes(b"not a video")
    task_id = importer.prepare_import_async(str(fake), "video", "", str(tmp_path / "out.wav"))
    info = None
    for _ in range(1000):
        info = tsm.get_task_status(task_id)
        if info is not None and not info["is_running"]:
            break
        time.sleep(0.01)
    assert info is not None and info["is_success"] is False
    assert "ffmpeg is not available" in info["message"]
    assert importer.take_result(task_id) is None


def test_prepare_import_cancel_removes_partial_temp(tmp_path):
    """I2：取消后台导入后无结果残留，预分配临时 WAV 被清理。"""
    from DASignalAnalysis import importer

    wav = tmp_path / "tone.wav"
    _write_wav(wav, fs=8000, seconds=1.0)
    out = tmp_path / "out.wav"
    task_id = importer.prepare_import_async(str(wav), "audio", "", str(out))
    assert tsm.cancel_task(task_id) is True
    info = None
    for _ in range(1000):
        info = tsm.get_task_status(task_id)
        if info is not None and not info["is_running"]:
            break
        time.sleep(0.01)
    assert info is not None and info["is_canceled"] is True
    assert importer.take_result(task_id) is None
    assert not out.exists()

def test_stft_local_range_and_block_frames(tmp_path):
    """D5：热图「只扫可见区段」时间轴落在请求范围内；块帧数只影响临时内存、不改变栅格。"""
    from DASignalAnalysis import stft

    wav = tmp_path / "range.wav"
    _write_wav(wav, fs=8000, seconds=10.0)
    n = 80000

    def _read(channel, start, count):
        return importer.read_range(wav, channel, start, count)

    src = {"type": "file", "path": str(wav), "channel": 0, "sampleRate": 8000, "sampleCount": n, "read": _read}
    params = {"stftWindowMs": 20.0, "stftOverlap": 50.0, "stftZeroPadding": 0, "window": "hann"}
    full = stft.compute_heatmap_sync(src, params, 300, 100)
    ranged = stft.compute_heatmap_sync(src, params, 300, 100, start_sample=16000, end_sample=48000)
    assert ranged["n_frames"] < full["n_frames"]
    assert ranged["times"][0] >= 16000 / 8000 - 0.05
    assert ranged["times"][-1] <= 48000 / 8000 + 0.05
    assert ranged["rows"] == full["rows"] and ranged["columns"] == 300
    # 块帧数只限制临时内存：8 帧/块与默认 256 帧/块的结果应完全一致
    coarse = stft.compute_heatmap_sync(src, params, 300, 100, block_frames=8)
    assert np.allclose(coarse["values"], full["values"], equal_nan=True)
