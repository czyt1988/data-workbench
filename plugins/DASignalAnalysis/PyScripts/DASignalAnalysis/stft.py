"""短时傅里叶变换：显示用栅格的计算与缓存。

内存策略（方案 §6.6 / §11 / D1-b）：
- 不保留整张 STFT 幅值矩阵；
- 按时间分块（每块 256 帧）计算，逐块把结果池化进「显示栅格」后丢弃块矩阵；
- 来源带 C++ 注入的 ``read`` provider 时按块取样本（不整段读入信号），
  无 provider 时沿用整段取数回退路径；两条路径的栅格数值口径一致；
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


def _stft_frame_mag(segs, window, u, nfft):
    """成帧块（float64）→ 单边峰值幅度矩阵（float32）。

    与 ``spectral.stft_matrix`` 的单边幅度口径一致：DC 不乘 2，偶数 nfft 时
    Nyquist 频点不乘 2（块内先算 float64，落地 float32 控制栅格/临时内存）。

    :param segs: (n_frames, nperseg) float64 成帧矩阵
    :param window: 窗函数数组（长度 nperseg）
    :param u: 窗函数之和 Σw
    :param nfft: FFT 点数
    :return: (n_frames, nfft//2+1) float32 幅度矩阵
    """
    spec = np.fft.rfft(segs * window, n=nfft, axis=1)
    mag = np.abs(spec) / u
    if mag.shape[1] > 1:
        mag[:, 1:] *= 2.0
        if nfft % 2 == 0 and mag.shape[1] > 1:
            mag[:, -1] /= 2.0
    return mag.astype(np.float32, copy=False)


def _accumulate_grid(grid, mag, f0, f1, col_edges, pool):
    """把块内逐帧段谱按列归并进显示栅格（同一列可能跨块：取最大值累积）。

    :param grid: (rows, columns) 显示栅格（-inf 初值）
    :param mag: 块内 (n_frames, n_freqs) float32 幅度矩阵
    :param f0: 块内首帧号
    :param f1: 块内末帧号（不含）
    :param col_edges: 时间方向池化的列边界
    :param pool: 频率方向池化区间列表
    """
    columns = grid.shape[1]
    for k in range(f0, f1):
        col = int(np.searchsorted(col_edges, k, side="right") - 1)
        col = min(max(col, 0), columns - 1)
        frame = mag[k - f0]
        pooled = np.empty(grid.shape[0], dtype=np.float32)
        for r, (s, e) in enumerate(pool):
            pooled[r] = np.max(frame[s:e])
        np.fmax(grid[:, col], pooled, out=grid[:, col])


def _heatmap_axes(fs, nperseg, hop, nfft, n_frames, columns, pool, frame_offset=0):
    """行中心频率 / 列中心时间（整段与流式两条路径共用，§6.3）。

    :param frame_offset: 本段首帧的绝对帧号（D5 局部区段扫描时时间轴仍按全局帧号换算）
    :return: (times[columns], freqs[rows])
    """
    freqs_all = np.fft.rfftfreq(nfft, d=1.0 / fs)
    freqs = np.array([0.5 * (freqs_all[s] + freqs_all[max(s, e - 1)]) for (s, e) in pool], dtype=np.float64)
    times_all = ((np.arange(n_frames, dtype=np.float64) + float(frame_offset)) * hop + nperseg / 2.0) / fs
    col_edges = np.linspace(0, n_frames, columns + 1, dtype=np.int64)
    times = np.empty(columns, dtype=np.float64)
    for c in range(columns):
        s = int(col_edges[c])
        e = max(s + 1, int(col_edges[c + 1]))
        times[c] = float(np.mean(times_all[s:min(e, n_frames)]))
    return times, freqs


def _heatmap_result(grid, fs, nperseg, hop, nfft, n_frames, columns, pool, frame_offset=0):
    """累积栅格 → 返回字典（键与 C++ ``DAStftResult`` 读取的键一致）。"""
    times, freqs = _heatmap_axes(fs, nperseg, hop, nfft, n_frames, columns, pool, frame_offset)
    # -inf 替换为 NaN（该单元无数据）
    grid[~np.isfinite(grid)] = np.nan
    return {
        "times": times,
        "freqs": freqs,
        "values": np.ascontiguousarray(grid.reshape(-1)),
        "rows": int(grid.shape[0]),
        "columns": int(columns),
        "nperseg": int(nperseg),
        "hop": int(hop),
        "n_frames": int(n_frames),
        "window_seconds": float(nperseg) / float(fs),
    }


def _compute_heatmap_from_provider(
    provider,
    sample_count,
    fs,
    params,
    columns,
    rows,
    progress_cb,
    task_id,
    frame_start=0,
    frame_stop=None,
    block_frames=None,
):
    """D1-b：有 readRange provider 时按 256 帧块流式计算 STFT 显示栅格。

    每块只请求该块帧范围所需的样本区间（``[(f0)*hop, (f1-1)*hop+nperseg)``），
    跨块由核心层 carry 保留重叠样本；块内成帧 → rfft → 频率/时间双向池化进 grid
    后立即丢弃块矩阵。全程不整段读入信号，也不保留整张 STFT 矩阵。

    D5：支持只扫 [frame_start, frame_stop) 帧区间（局部区段降级）与 ``block_frames``
    降低单块帧数（内存超限时「降低热图精度」）；时间轴仍按全局帧号换算。

    :param provider: 分块取数 callable provider(start, count) -> float32 一维数组
    :param sample_count: 信号总采样点数（来自 source["sampleCount"]）
    :param fs: 采样率
    :param params: 参数（stftWindowMs/stftOverlap/stftZeroPadding/window/removeDc/detrend）
    :param columns: 时间方向目标列数
    :param rows: 频率方向目标行数
    :param progress_cb: 可选进度回调（0~100）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :param frame_start: 本段首帧绝对帧号（局部扫描）
    :param frame_stop: 本段末帧绝对帧号（不含；None 表示到信号末尾）
    :param block_frames: 每块帧数（None 用默认 256；内存降级时可调小）
    :return: 与 compute_heatmap_sync 完全相同的返回字典
    """
    nperseg, hop, nfft, n_frames_total = _stft_layout(fs, sample_count, params)
    if n_frames_total <= 0:
        raise RuntimeError("signal is shorter than one STFT window")
    frame_start = int(max(0, min(int(frame_start), n_frames_total - 1)))
    frame_stop = n_frames_total if frame_stop is None else int(max(frame_start + 1, min(int(frame_stop), n_frames_total)))
    span = frame_stop - frame_start
    columns = int(max(1, min(columns, span)))
    window = spectral.window_values(str(params.get("window", "hann")), nperseg)
    u = float(np.sum(window))
    if u <= 0.0:
        u = 1.0
    n_freqs = nfft // 2 + 1
    rows = int(max(1, min(rows, n_freqs)))
    pool = _freq_pool_index(n_freqs, rows)
    grid = np.full((rows, columns), -np.inf, dtype=np.float32)
    col_edges = np.linspace(0, span, columns + 1, dtype=np.int64)

    def _cancel():
        """每块检查一次取消标志（C7）。"""
        if task_id is not None:
            tsm.raise_if_canceled(task_id)

    block_samples = int(spectral._PROVIDER_BLOCK_SAMPLES)
    # 去趋势统计必须基于**整段**信号（与整段路径口径一致），局部扫描也不例外
    plan = spectral._provider_detrend_plan(
        provider, sample_count, spectrum._detrend_mode(params), block_samples, _cancel
    )
    chunk = int(block_frames) if block_frames else _BLOCK_FRAMES
    chunk = int(max(1, chunk))
    # 每块样本跨度不超过 provider 块上限（nperseg 很大时至少 1 帧）
    if hop > 0 and block_samples > nperseg:
        chunk = max(1, min(chunk, (block_samples - nperseg) // hop + 1))
    total_blocks = (span + chunk - 1) // chunk
    for block_idx in range(total_blocks):
        _cancel()
        a = frame_start + block_idx * chunk
        b = min(frame_stop, a + chunk)
        start_abs = a * hop
        count = (b - 1) * hop + nperseg - start_abs
        block = spectral._provider_read(provider, start_abs, count)
        if plan is not None:
            block = spectral._apply_provider_detrend(block, start_abs, plan)
        segs = np.empty((b - a, nperseg), dtype=np.float64)
        for k in range(a, b):
            off = (k - a) * hop
            segs[k - a, :] = block[off : off + nperseg]
        mag = _stft_frame_mag(segs, window, u, nfft)
        _accumulate_grid(grid, mag, a - frame_start, b - frame_start, col_edges, pool)
        if progress_cb:
            progress_cb(100.0 * (block_idx + 1) / total_blocks)
        del mag, segs, block
    return _heatmap_result(grid, fs, nperseg, hop, nfft, span, columns, pool, frame_start)


def compute_heatmap_sync(
    source, params, columns, rows, progress_cb=None, task_id=None, start_sample=None, end_sample=None, block_frames=None
):
    """计算显示用 STFT 栅格（线性幅值）。

    取数策略（D1-b）：

    - 来源带 ``read`` callable（C++ ``DASignalSource::readRange`` 注入）且带
      ``sampleCount`` 时走流式路径：按「每块 256 帧」请求所需样本区间，块内成帧 →
      rfft → 池化进显示栅格后丢弃块矩阵，**不整段读入**；
    - 否则回退现有整段取数路径（文件 / 数据帧来源，行为与数值口径不变）。

    :param source: 来源描述（type/path 或 dataframeName/column，可含 channel/sampleCount/read）
    :param params: 参数（stftWindowMs/stftOverlap/stftZeroPadding/window/removeDc/detrend）
    :param columns: 时间方向目标列数
    :param rows: 频率方向目标行数
    :param progress_cb: 可选进度回调（0~100）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :param start_sample: 可选，局部区段起点（采样点，D5「只扫可见区段」）；None 表示整段
    :param end_sample: 可选，局部区段终点（采样点，不含）
    :param block_frames: 可选，每块帧数（D5「降低热图精度」时调小以降低峰值内存）
    :return: dict{times, freqs, values(flat row-major, 行=频率), shape=(rows,cols), nperseg, hop, n_frames}
    """
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    provider = spectrum._source_provider(source)
    sample_count = source.get("sampleCount")
    if provider is not None and sample_count is not None and int(sample_count) > 0:
        # 局部区段：把采样点范围映射成帧区间（帧窗口起点落在 [start, end) 内）
        layout_nperseg, layout_hop, _layout_nfft, layout_frames = _stft_layout(fs, int(sample_count), params)
        frame_start = 0
        frame_stop = None
        if start_sample is not None or end_sample is not None:
            total_frames = layout_frames
            if total_frames <= 0:
                raise RuntimeError("signal is shorter than one STFT window")
            if start_sample is not None:
                frame_start = int(max(0, int(start_sample) // max(1, layout_hop)))
            if end_sample is not None:
                frame_stop = int(max(frame_start + 1, (int(end_sample) + layout_hop - 1) // max(1, layout_hop)))
                frame_stop = min(total_frames, frame_stop)
            frame_start = min(frame_start, max(0, total_frames - 1))
        return _compute_heatmap_from_provider(
            provider,
            int(sample_count),
            fs,
            params,
            columns,
            rows,
            progress_cb,
            task_id,
            frame_start=frame_start,
            frame_stop=frame_stop,
            block_frames=block_frames,
        )
    # N-C1b：编排层保持 float32，块内成帧时由 float64 的 segs 承接（不在入口整段升精度）
    x = _load_channel(source)
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
        mag = _stft_frame_mag(segs, window, u, nfft)
        # 块内逐帧归入列（同一列可能跨块：取最大值累积）
        _accumulate_grid(grid, mag, f0, f1, col_edges, pool)
        if progress_cb:
            progress_cb(100.0 * (block_idx + 1) / total_blocks)
        del mag, segs

    return _heatmap_result(grid, fs, nperseg, hop, nfft, n_frames, columns, pool)


def compute_heatmap_async(source, params, columns, rows, start_sample=None, end_sample=None, block_frames=None):
    """在守护线程中计算 STFT 显示栅格，返回任务 id（支持 D5 局部区段/块帧数降级）。"""
    task_id, status = tsm.create_task_with_status("signal stft")
    status.start("signal stft")

    def progress(p):
        status.update_progress(max(1.0, min(99.0, p)), "stft blocks")

    def worker():
        try:
            result = compute_heatmap_sync(
                source,
                params,
                columns,
                rows,
                progress_cb=progress,
                task_id=task_id,
                start_sample=start_sample,
                end_sample=end_sample,
                block_frames=block_frames,
            )
            if tsm.is_task_canceled(task_id):
                # 取消后不写结果表，避免大数组滞留（C7）
                return
            _RESULT_STORE[task_id] = result
            if tsm.is_task_canceled(task_id):
                # 写结果表与取消的竞态：取消后自行清理，避免结果滞留（C7）
                _RESULT_STORE.pop(task_id, None)
                return
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
