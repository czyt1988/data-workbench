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
