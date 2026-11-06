"""峰值查找与每帧前 N 峰值曲线（方案 §6.4 / §6.5）。

两个入口：
- compute_peaks_async：整段频谱的前 N 个峰（峰值表数据）；
- compute_peak_curves_async：单趟扫描每帧前 N（第 k 条曲线 = 每帧第 k 名的频率随时间）。

内存策略：峰值曲线不保留整张 STFT 矩阵——分块（512 帧）计算、逐帧取前 N、
丢弃块矩阵，最终只留 N × 帧数 的排名序列。

返回字典的键（C++ 侧 DASignalAnalysisWorker.cpp 按这些键读取，不得增删改名）：
- compute_peaks_sync: {frequencies, magnitudes, rel_db, share, resolution_hz}
- compute_peak_curves_sync: {times, freq_by_rank, mag_by_rank, top_n, n_frames, nperseg, hop}
  其中 freq_by_rank / mag_by_rank 均为 top_n × n_frames 的行主序矩阵（rank * n_frames + frame），
  某帧不足 N 个峰时对应位置为 NaN。

口径：
- 峰值表与峰值曲线的段长都由各自时间窗口参数决定，最小间距默认值统一按「实际频率分辨率
  fs/nperseg」× 5 计算，不用参数里的目标分辨率（C5）；
- 峰值表段长由 resolutionHz 反推（v2 C2/C4），改分辨率后峰值表与频谱同步变化；
- 频谱侧零填充语义为「额外点数」：nfft = 段长 + zeroPadding（C3）。
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
    """把参数结构体量纲转成 find_peaks_topn 的关键字参数（§6.4）。

    :param resolution_hz: 实际频率分辨率（赫兹），未指定最小间距时按 5 倍取默认值
    """
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


def compute_peaks_sync(source, params, task_id=None):
    """整段频谱的前 N 个峰（峰值表，v2 C2/C4 口径）。

    段长由 resolutionHz 反推：nperseg = clamp(round(fs/Δf), 2, N)，段间按 overlap
    （百分比）重叠；按 ``amplitude_spectrum_segmented`` 对单边峰值幅度谱做
    mean/median 平均，使实际频率分辨率 Δf = fs/nperseg；N <= nperseg 时退化为
    整段单次 FFT。默认最小间距按 5 × 实际 Δf（C5），并以 resolution_hz 返回，
    从而「改分辨率 → 峰值表同步变化」（v2 C2）。

    :param source: 来源描述（type/path 或 dataframeName/column）
    :param params: 参数字典（键名见 DASignalAnalysisParams::toJson）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :return: dict{frequencies, magnitudes, rel_db, share, resolution_hz}
    """
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    x = spectrum._load_channel(source).astype(np.float64)
    y = spectrum._preprocess(x, params)
    if task_id is not None:
        tsm.raise_if_canceled(task_id)
    window = str(params.get("window", "hann"))
    zero_padding = int(params.get("zeroPadding", 0) or 0)
    resolution = float(params.get("resolutionHz", 10.0) or 10.0)
    # 重叠率按百分比传入（C++ 侧 50 表示 50%），与 spectrum.py 同口径
    overlap = float(params.get("overlap", 50.0) or 0.0) / 100.0
    # 段长由目标分辨率反推并 clamp 到 [2, N]；N 很小（<2）时直接用数据长度
    if y.size >= 2:
        nperseg = int(max(2, min(spectral.seg_len_from_resolution(fs, resolution), y.size)))
    else:
        nperseg = int(y.size)
    # 零填充语义统一为「额外点数」（C3）
    nfft = (nperseg + zero_padding) if zero_padding > 0 else None

    cancel_hook = None
    if task_id is not None:

        def _cancel_hook():
            """每个分块调用一次，命中取消即抛 TaskCanceledError。"""
            tsm.raise_if_canceled(task_id)

        cancel_hook = _cancel_hook

    freqs, amps = spectral.amplitude_spectrum_segmented(
        y,
        fs,
        nperseg=nperseg,
        overlap=overlap,
        window=window,
        nfft=nfft,
        average=str(params.get("average", "mean")),
        rms=False,
        cancel_hook=cancel_hook,
    )
    # 实际分辨率由所用段长决定，而不是参数里的目标分辨率（C5）
    actual_resolution = float(fs) / float(nperseg) if nperseg > 0 else 0.0
    kw = _peak_kwargs(params, actual_resolution)
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
        "resolution_hz": float(actual_resolution),
    }


def compute_peak_curves_sync(source, params, progress_cb=None, task_id=None):
    """单趟扫描：每帧前 N 峰，返回排名曲线（§6.5）。

    :param source: 来源描述（type/path 或 dataframeName/column）
    :param params: 参数字典（键名见 DASignalAnalysisParams::toJson）
    :param progress_cb: 可选进度回调（0~100）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :return: dict{times, freq_by_rank(N×frames, 缺失为 NaN), mag_by_rank(N×frames, 缺失为 NaN),
                  top_n, n_frames, nperseg, hop}
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
    # 峰值曲线的实际频率分辨率由段长决定（Δf = fs/nperseg），两处默认间距口径一致（C5）
    resolution_hz = float(fs) / float(max(1, nperseg))
    kw = _peak_kwargs(params, resolution_hz)
    w = spectral.window_values(str(params.get("window", "hann")), nperseg)
    u = float(np.sum(w))
    if u <= 0.0:
        u = 1.0
    freqs = np.fft.rfftfreq(nperseg, d=1.0 / fs)

    freq_by_rank = np.full((top_n, n_frames), np.nan, dtype=np.float64)
    mag_by_rank = np.full((top_n, n_frames), np.nan, dtype=np.float64)
    total_blocks = (n_frames + _BLOCK_FRAMES - 1) // _BLOCK_FRAMES
    for block_idx in range(total_blocks):
        if task_id is not None:
            tsm.raise_if_canceled(task_id)
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
                mag_by_rank[:m, k] = res["magnitudes"]
        if progress_cb:
            progress_cb(100.0 * (block_idx + 1) / total_blocks)
        del mag, spec, segs

    times = (np.arange(n_frames, dtype=np.float64) * hop + nperseg / 2.0) / fs
    return {
        "times": times,
        "freq_by_rank": freq_by_rank,
        "mag_by_rank": mag_by_rank,
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
            result = compute(progress, task_id)
            if tsm.is_task_canceled(task_id):
                # 取消后不写结果表，避免大数组滞留（C7）
                return
            _RESULT_STORE[task_id] = result
            status.finish(True, "")
        except tsm.TaskCanceledError:
            status.finish(False, "cancelled")
        except Exception as e:  # noqa: BLE001
            if tsm.is_task_canceled(task_id):
                status.finish(False, "cancelled")
                return
            status.finish(False, "%s\n%s" % (str(e), traceback.format_exc()))

    threading.Thread(target=worker, daemon=True).start()
    return task_id


def compute_peaks_async(source, params):
    return _async(
        "signal peaks",
        lambda progress, task_id: compute_peaks_sync(source, params, task_id=task_id),
        "peaks",
    )


def compute_peak_curves_async(source, params):
    return _async(
        "signal peak curves",
        lambda progress, task_id: compute_peak_curves_sync(
            source, params, progress_cb=progress, task_id=task_id
        ),
        "peak curves",
    )


def take_result(task_id):
    return _RESULT_STORE.pop(task_id, None)


def cancel_task(task_id):
    """请求取消任务（线程在下一个分块检查点抛出 TaskCanceledError）。"""
    return tsm.cancel_task(task_id, "cancelled by user")


def is_cancelled(task_id):
    """任务是否已被取消。"""
    return tsm.is_task_canceled(task_id)
