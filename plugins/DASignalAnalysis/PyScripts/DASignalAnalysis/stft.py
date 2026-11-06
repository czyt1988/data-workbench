"""短时傅里叶变换：显示用栅格的计算与缓存。

内存策略（方案 §6.6 / §11）：
- 不保留整张 STFT 幅值矩阵；
- 按时间分块（每块 256 帧）计算，逐块把结果池化进「显示栅格」后丢弃块矩阵；
- 时间方向池化到列数（≈ 绘图区宽度），频率方向最大池化到行数（≈ 绘图区高度）；
- 频率方向用最大值池化（不取平均，避免窄峰被抹平）。

返回的栅格是线性幅值；dB 换算与色标下限由显示层处理。
"""

import threading
import traceback

import numpy as np

from DAWorkbench.DAPyBase import spectral
from DAWorkbench.DAPyBase import thread_status_manager as tsm

from . import importer
from . import spectrum

_RESULT_STORE = {}

# 每块帧数
_BLOCK_FRAMES = 256


def _load_channel(source):
    """按来源描述读取单通道整段数据（与 spectrum.py 同口径）。"""
    return spectrum._load_channel(source)


def _stft_layout(fs, sample_count, params):
    """由参数推导 nperseg / hop / nfft / 帧数（§6.3）。"""
    window_ms = float(params.get("stftWindowMs", 50.0) or 50.0)
    overlap = float(params.get("stftOverlap", 50.0) or 0.0) / 100.0
    zero_padding = int(params.get("stftZeroPadding", 0) or 0)
    nperseg = int(max(2, round(float(fs) * window_ms / 1000.0)))
    nperseg = min(nperseg, int(sample_count))
    hop = spectral.hop_from_overlap(nperseg, overlap)
    nfft = max(nperseg, nperseg + zero_padding if zero_padding > 0 else nperseg)
    n_frames = spectral.frame_count(sample_count, nperseg, hop)
    return nperseg, hop, nfft, n_frames


def _freq_pool_index(n_freqs, rows):
    """频率方向池化索引：每行对应的频点区间 [start, end)。"""
    rows = int(max(1, min(rows, n_freqs)))
    edges = np.linspace(0, n_freqs, rows + 1, dtype=np.int64)
    out = []
    for i in range(rows):
        s = int(edges[i])
        e = int(edges[i + 1])
        if e <= s:
            e = min(n_freqs, s + 1)
        out.append((s, e))
    return out


def compute_heatmap_sync(source, params, columns, rows, progress_cb=None, task_id=None):
    """计算显示用 STFT 栅格（线性幅值）。

    :param source: 来源描述（type/path 或 dataframeName/column）
    :param params: 参数（stftWindowMs/stftOverlap/stftZeroPadding/window/removeDc/detrend）
    :param columns: 时间方向目标列数
    :param rows: 频率方向目标行数
    :param progress_cb: 可选进度回调（0~100）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :return: dict{times, freqs, values(flat row-major, 行=频率), shape=(rows,cols), nperseg, hop, n_frames}
    """
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    x = _load_channel(source).astype(np.float64)
    y = spectrum._preprocess(x, params)
    nperseg, hop, nfft, n_frames = _stft_layout(fs, y.size, params)
    if n_frames <= 0:
        raise RuntimeError("signal is shorter than one STFT window")

    columns = int(max(1, min(columns, n_frames)))
    window = spectral.window_values(str(params.get("window", "hann")), nperseg)
    u = float(np.sum(window))
    if u <= 0.0:
        u = 1.0
    n_freqs = nfft // 2 + 1
    rows = int(max(1, min(rows, n_freqs)))
    pool = _freq_pool_index(n_freqs, rows)

    grid = np.full((rows, columns), -np.inf, dtype=np.float32)
    # 时间方向：每列覆盖的帧区间
    col_edges = np.linspace(0, n_frames, columns + 1, dtype=np.int64)

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
        spec = np.fft.rfft(segs * window, n=nfft, axis=1)
        mag = np.abs(spec) / u
        if mag.shape[1] > 1:
            mag[:, 1:] *= 2.0
            if nfft % 2 == 0 and mag.shape[1] > 1:
                mag[:, -1] /= 2.0
        mag = mag.astype(np.float32, copy=False)
        # 块内逐帧归入列（同一列可能跨块：取最大值累积）
        for k in range(f0, f1):
            col = int(np.searchsorted(col_edges, k, side="right") - 1)
            col = min(max(col, 0), columns - 1)
            frame = mag[k - f0]
            pooled = np.empty(rows, dtype=np.float32)
            for r, (s, e) in enumerate(pool):
                pooled[r] = np.max(frame[s:e])
            np.fmax(grid[:, col], pooled, out=grid[:, col])
        if progress_cb:
            progress_cb(100.0 * (block_idx + 1) / total_blocks)
        del mag, spec, segs

    # 行中心频率 / 列中心时间
    freqs_all = np.fft.rfftfreq(nfft, d=1.0 / fs)
    freqs = np.array([0.5 * (freqs_all[s] + freqs_all[max(s, e - 1)]) for (s, e) in pool], dtype=np.float64)
    times_all = (np.arange(n_frames, dtype=np.float64) * hop + nperseg / 2.0) / fs
    times = np.empty(columns, dtype=np.float64)
    for c in range(columns):
        s = int(col_edges[c])
        e = max(s + 1, int(col_edges[c + 1]))
        times[c] = float(np.mean(times_all[s:min(e, n_frames)]))
    # -inf 替换为 NaN（该单元无数据）
    grid[~np.isfinite(grid)] = np.nan
    return {
        "times": times,
        "freqs": freqs,
        "values": np.ascontiguousarray(grid.reshape(-1)),
        "rows": rows,
        "columns": columns,
        "nperseg": int(nperseg),
        "hop": int(hop),
        "n_frames": int(n_frames),
        "window_seconds": float(nperseg) / fs,
    }


def compute_heatmap_async(source, params, columns, rows):
    """在守护线程中计算 STFT 显示栅格，返回任务 id。"""
    task_id, status = tsm.create_task_with_status("signal stft")
    status.start("signal stft")

    def progress(p):
        status.update_progress(max(1.0, min(99.0, p)), "stft blocks")

    def worker():
        try:
            result = compute_heatmap_sync(source, params, columns, rows, progress_cb=progress, task_id=task_id)
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


def take_result(task_id):
    return _RESULT_STORE.pop(task_id, None)


def cancel_task(task_id):
    return tsm.cancel_task(task_id, "cancelled by user")
