"""峰值查找与每帧前 N 峰值曲线（方案 §6.4 / §6.5）。

两个入口：
- compute_peaks_async：整段频谱的前 N 个峰（峰值表数据）；
- compute_peak_curves_async：单趟扫描每帧前 N（第 k 条曲线 = 每帧第 k 名的频率随时间）。

内存策略：峰值曲线不保留整张 STFT 矩阵——分块（512 帧）计算、逐帧取前 N、
丢弃块矩阵，最终只留 N × 帧数 的排名序列。
"""

import threading
import traceback

import numpy as np

from DAWorkbench.DAPyBase import spectral
from DAWorkbench.DAPyBase import thread_status_manager as tsm

from . import spectrum

_RESULT_STORE = {}

_BLOCK_FRAMES = 512


def _peak_kwargs(params, resolution_hz):
    """把参数结构体量纲转成 find_peaks_topn 的关键字参数（§6.4）。"""
    advanced = str(params.get("peakMode", "simple")).lower() == "advanced"
    min_spacing = float(params.get("peakMinSpacingHz", -1.0) or -1.0)
    if min_spacing <= 0.0:
        min_spacing = 5.0 * float(resolution_hz) if resolution_hz > 0 else None
    prominence = float(params.get("peakProminence", 0.0) or 0.0)
    height = float(params.get("peakMinHeight", 0.0) or 0.0)
    return {
        "n": int(params.get("peakTopN", 10) or 10),
        "prominence": (prominence if (advanced and prominence > 0) else None),
        "min_spacing_hz": min_spacing,
        "height": (height if (advanced and height > 0) else None),
        "rel_db": float(params.get("peakRelDb", 40.0) or 40.0),
    }


def compute_peaks_sync(source, params):
    """整段频谱的前 N 个峰（峰值表）。"""
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    x = spectrum._load_channel(source).astype(np.float64)
    y = spectrum._preprocess(x, params)
    window = str(params.get("window", "hann"))
    nfft = int(params.get("zeroPadding", 0) or 0)
    freqs, amps = spectral.amplitude_spectrum(y, fs, window=window, nfft=(nfft or None), rms=False)
    resolution = float(params.get("resolutionHz", 10.0) or 10.0)
    kw = _peak_kwargs(params, resolution)
    res = spectral.find_peaks_topn(
        amps,
        freqs,
        n=kw["n"],
        prominence=kw["prominence"],
        min_spacing_hz=kw["min_spacing_hz"],
        height=kw["height"],
        rel_db=kw["rel_db"],
    )
    mags = res["magnitudes"]
    peak_freqs = res["frequencies"]
    if mags.size > 0:
        ref = float(mags[0])
        rel_db = 20.0 * np.log10(np.maximum(mags, 1e-30) / max(ref, 1e-30))
        power = mags.astype(np.float64) ** 2
        total = float(np.sum(power))
        share = (power / total * 100.0) if total > 0 else np.zeros_like(power)
    else:
        rel_db = np.zeros(0)
        share = np.zeros(0)
    return {
        "frequencies": np.asarray(peak_freqs, dtype=np.float64),
        "magnitudes": np.asarray(mags, dtype=np.float64),
        "rel_db": np.asarray(rel_db, dtype=np.float64),
        "share": np.asarray(share, dtype=np.float64),
        "resolution_hz": float(resolution),
    }


def compute_peak_curves_sync(source, params, progress_cb=None):
    """单趟扫描：每帧前 N 峰，返回排名曲线（§6.5）。

    :return: dict{times, freq_by_rank(N×frames, 缺失为 NaN), n_perseg, hop}
    """
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    x = spectrum._load_channel(source).astype(np.float64)
    y = spectrum._preprocess(x, params)
    window_ms = float(params.get("stftWindowMs", 50.0) or 50.0)
    overlap = float(params.get("stftOverlap", 50.0) or 0.0) / 100.0
    nperseg = int(max(2, round(fs * window_ms / 1000.0)))
    nperseg = min(nperseg, y.size)
    hop = spectral.hop_from_overlap(nperseg, overlap)
    n_frames = spectral.frame_count(y.size, nperseg, hop)
    if n_frames <= 0:
        raise RuntimeError("signal is shorter than one STFT window")

    top_n = int(params.get("peakTopN", 10) or 10)
    resolution_hz = fs / float(nperseg)
    kw = _peak_kwargs(params, resolution_hz)
    w = spectral.window_values(str(params.get("window", "hann")), nperseg)
    u = float(np.sum(w))
    if u <= 0.0:
        u = 1.0
    freqs = np.fft.rfftfreq(nperseg, d=1.0 / fs)

    freq_by_rank = np.full((top_n, n_frames), np.nan, dtype=np.float64)
    total_blocks = (n_frames + _BLOCK_FRAMES - 1) // _BLOCK_FRAMES
    for block_idx in range(total_blocks):
        f0 = block_idx * _BLOCK_FRAMES
        f1 = min(n_frames, f0 + _BLOCK_FRAMES)
        segs = np.empty((f1 - f0, nperseg), dtype=np.float64)
        for k in range(f0, f1):
            start = k * hop
            segs[k - f0, :] = y[start : start + nperseg]
        spec = np.fft.rfft(segs * w, n=nperseg, axis=1)
        mag = np.abs(spec) / u
        if mag.shape[1] > 1:
            mag[:, 1:] *= 2.0
            if nperseg % 2 == 0 and mag.shape[1] > 1:
                mag[:, -1] /= 2.0
        for k in range(f0, f1):
            res = spectral.find_peaks_topn(
                mag[k - f0],
                freqs,
                n=top_n,
                prominence=kw["prominence"],
                min_spacing_hz=kw["min_spacing_hz"],
                height=kw["height"],
                rel_db=kw["rel_db"],
            )
            m = res["frequencies"].size
            if m > 0:
                freq_by_rank[:m, k] = res["frequencies"]
        if progress_cb:
            progress_cb(100.0 * (block_idx + 1) / total_blocks)
        del mag, spec, segs

    times = (np.arange(n_frames, dtype=np.float64) * hop + nperseg / 2.0) / fs
    return {
        "times": times,
        "freq_by_rank": freq_by_rank,
        "top_n": top_n,
        "n_frames": int(n_frames),
        "nperseg": int(nperseg),
        "hop": int(hop),
    }


def _async(kind, compute, progress_stage):
    task_id, status = tsm.create_task_with_status(kind)
    status.start(kind)

    def progress(p):
        status.update_progress(max(1.0, min(99.0, p)), progress_stage)

    def worker():
        try:
            _RESULT_STORE[task_id] = compute(progress)
            status.finish(True, "")
        except Exception as e:  # noqa: BLE001
            status.finish(False, "%s\n%s" % (str(e), traceback.format_exc()))

    threading.Thread(target=worker, daemon=True).start()
    return task_id


def compute_peaks_async(source, params):
    return _async("signal peaks", lambda progress: compute_peaks_sync(source, params), "peaks")


def compute_peak_curves_async(source, params):
    return _async(
        "signal peak curves",
        lambda progress: compute_peak_curves_sync(source, params, progress_cb=progress),
        "peak curves",
    )


def take_result(task_id):
    return _RESULT_STORE.pop(task_id, None)
