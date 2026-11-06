"""面向信号的频谱编排（调用核心层纯函数）。

线程约定（方案 §6.7）：本模块在 Python 侧起守护线程执行长任务，
进度经 DAWorkbench.DAPyBase.thread_status_manager 上报，C++ 侧定时轮询。
本模块不做界面、不弹窗；结果暂存在模块级结果表，由 C++ 侧 take_result 取走。

口径（方案 §6.2 / C2 / C3 / C4 / C6 / C7）：
- 计算层始终产出「峰值幅度谱」：amplitudeMode（peak/rms）不参与计算，均方根与 dB
  换算统一由显示/导出层做（C2），返回字典的 rms 字段固定 False；
- 段长由目标频率分辨率反推：nperseg = round(fs / resolutionHz)，clamp 到 [2, N]；
  实际分辨率 Δf = fs/nperseg。N > nperseg 时按 overlap 分块求单边峰值幅度谱再按
  average（mean/median）平均；N <= nperseg 时退化为整段单次 FFT；
- zeroPadding 语义为「额外点数」：nfft = 段长 + zeroPadding（与 stft.py 一致）；
- 读入的 NaN/Inf 统一归零（nan_to_num），避免污染 FFT 与包络；
- 分块循环每个块检查取消标志，命中抛 TaskCanceledError，且不写结果表。
"""

import threading
import traceback

import numpy as np

from DAWorkbench.DAPyBase import spectral
from DAWorkbench.DAPyBase import thread_status_manager as tsm

from . import importer

# task_id -> 结果 dict（大数组不进 status，避免轮询时反复拷贝）
_RESULT_STORE = {}


def _load_channel(source):
    """按来源描述读取单通道整段数据（文件路径读 float32，统一转 float64）。

    NaN/Inf 统一用 np.nan_to_num 归零（C6）：数据帧列里的缺失值不能污染
    FFT/包络（否则峰值被吞成 0、dBFS 参考失真）。
    """
    stype = str(source.get("type", "file"))
    channel = int(source.get("channel", 0))
    if stype == "dataframe":
        # 惰性导入：da_app 是宿主注入的顶层模块（与 DataAnalysis 插件同一约定）
        import da_app

        datamanager = da_app.getCore().getDataManagerInterface()
        frames = datamanager.getAllDataframes()
        df = frames.get(source.get("dataframeName", ""))
        if df is None:
            raise RuntimeError("dataframe not found: %s" % source.get("dataframeName", ""))
        column = source.get("column", "")
        values = np.asarray(df[column].to_numpy(), dtype=np.float64)
    else:
        values = importer.read_channel_all(source.get("path", ""), channel).astype(np.float64)
    # 两路统一归零 NaN/Inf（C6）
    values = np.nan_to_num(values, nan=0.0, posinf=0.0, neginf=0.0)
    if values.size == 0:
        raise RuntimeError("signal contains no samples")
    return values


def _preprocess(x, params):
    """预处理：去直流 + 去趋势（方案 §8.6 信号预处理分组）。"""
    mode = str(params.get("detrend", "none")).lower()
    if bool(params.get("removeDc", False)) and mode in ("none", ""):
        mode = "constant"
    return spectral.detrend_signal(x, mode)


def compute_spectrum_sync(x, fs, params, task_id=None):
    """同步计算频谱与功率谱（纯函数编排，供数据帧路径与测试复用）。

    计算层始终返回峰值幅度谱（C2）：amplitudeMode 只影响显示/导出层的换算，
    不参与这里的计算，返回的 rms 字段固定为 False。

    :param x: 单通道数据
    :param fs: 采样率
    :param params: 参数字典（键名见 DASignalAnalysisParams::toJson）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :return: dict{freqs, amplitudes, psd_freqs, psd_values, seg_len, resolution_hz, window, rms}
    """
    y = _preprocess(np.asarray(x, dtype=np.float64), params)
    window = str(params.get("window", "hann"))
    # C2：口径统一在显示/导出层换算，计算层永远输出峰值幅度
    rms = False
    # 零填充语义为「额外点数」：nfft = 段长 + zeroPadding（C3，与 stft.py 一致）
    zero_padding = int(params.get("zeroPadding", 0) or 0)
    resolution = float(params.get("resolutionHz", 10.0) or 10.0)
    # 重叠率按百分比传入（C++ 侧 50 表示 50%）
    overlap = float(params.get("overlap", 50.0) or 0.0) / 100.0

    # 段长由目标分辨率反推（C4）；clamp 到 [2, N]，N 很小（<2）时直接用数据长度
    if y.size >= 2:
        nperseg = int(max(2, min(spectral.seg_len_from_resolution(fs, resolution), y.size)))
    else:
        nperseg = int(y.size)
    nfft = (nperseg + zero_padding) if zero_padding > 0 else None

    cancel_hook = None
    if task_id is not None:
        tsm.raise_if_canceled(task_id)

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
        rms=rms,
        cancel_hook=cancel_hook,
    )
    psd_freqs, psd_values = spectral.welch_psd(
        y,
        fs,
        nperseg=nperseg,
        overlap=overlap,
        window=window,
        average=str(params.get("average", "mean")),
    )
    return {
        "freqs": freqs,
        "amplitudes": amps,
        "psd_freqs": psd_freqs,
        "psd_values": psd_values,
        "seg_len": int(nperseg),
        "resolution_hz": float(fs) / float(nperseg) if nperseg > 0 else 0.0,
        "window": window,
        "rms": rms,
    }


def compute_spectrum_sync_from_source(source, params, task_id=None):
    """同步计算（从来源描述读取数据；后台线程内部使用）。"""
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    x = _load_channel(source)
    return compute_spectrum_sync(x, fs, params, task_id=task_id)


def compute_spectrum_async(source, params):
    """在守护线程中计算频谱，返回任务 id。"""
    task_name = "signal spectrum"
    task_id, status = tsm.create_task_with_status(task_name)
    status.start(task_name)

    def worker():
        try:
            status.update_progress(5.0, "reading samples")
            result = compute_spectrum_sync_from_source(source, params, task_id=task_id)
            if tsm.is_task_canceled(task_id):
                # 取消后不写结果表，避免大数组滞留（C7）
                return
            status.update_progress(95.0, "finalizing")
            _RESULT_STORE[task_id] = result
            status.finish(True, "")
        except tsm.TaskCanceledError:
            status.finish(False, "cancelled")
        except Exception as e:  # noqa: BLE001 - 失败原因必须回传给界面
            if tsm.is_task_canceled(task_id):
                status.finish(False, "cancelled")
                return
            status.finish(False, "%s\n%s" % (str(e), traceback.format_exc()))

    threading.Thread(target=worker, daemon=True).start()
    return task_id


def take_result(task_id):
    """取走某任务的频谱结果（取走后从结果表删除）。"""
    return _RESULT_STORE.pop(task_id, None)


def cancel_task(task_id):
    """请求取消任务（线程在下一个检查点退出）。"""
    return tsm.cancel_task(task_id, "cancelled by user")


def is_cancelled(task_id):
    """任务是否已被取消。"""
    return tsm.is_task_canceled(task_id)
