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
    """D3：依赖可用性自检入口（当前测试环境已安装 soundfile）。"""
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
