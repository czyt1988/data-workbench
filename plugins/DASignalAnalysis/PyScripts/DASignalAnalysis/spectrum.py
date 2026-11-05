"""面向信号的频谱编排（调用核心层纯函数）。

线程约定（方案 §6.7）：本模块在 Python 侧起守护线程执行长任务，
进度经 DAWorkbench.DAPyBase.thread_status_manager 上报，C++ 侧定时轮询。
本模块不做界面、不弹窗；结果暂存在模块级结果表，由 C++ 侧 take_result 取走。
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
    """按来源描述读取单通道整段数据（float32 → float64）。"""
    stype = str(source.get("type", "file"))
    channel = int(source.get("channel", 0))
    if stype == "dataframe":
        # 惰性导入：仅数据帧来源需要访问宿主数据管理器
        from DAWorkbench import da_app  # noqa: F401  (宿主注入的全局对象)

        datamanager = da_app.getCore().getDataManagerInterface()
        df = datamanager.getDataframeByName(source.get("dataframeName", ""))
        if df is None:
            raise RuntimeError("dataframe not found: %s" % source.get("dataframeName", ""))
        column = source.get("column", "")
        values = np.asarray(df[column].to_numpy(), dtype=np.float64)
    else:
        values = importer.read_channel_all(source.get("path", ""), channel).astype(np.float64)
    if values.size == 0:
        raise RuntimeError("signal contains no samples")
    return values


def _preprocess(x, params):
    """预处理：去直流 + 去趋势（方案 §8.6 信号预处理分组）。"""
    mode = str(params.get("detrend", "none")).lower()
    if bool(params.get("removeDc", False)) and mode in ("none", ""):
        mode = "constant"
    return spectral.detrend_signal(x, mode)


def compute_spectrum_sync(x, fs, params):
    """同步计算频谱与功率谱（纯函数编排，供数据帧路径与测试复用）。

    :param x: 单通道数据
    :param fs: 采样率
    :param params: 参数字典（键名见 DASignalAnalysisParams::toJson）
    :return: dict{freqs, amplitudes, psd_freqs, psd_values, seg_len, resolution_hz}
    """
    y = _preprocess(np.asarray(x, dtype=np.float64), params)
    window = str(params.get("window", "hann"))
    rms = str(params.get("amplitudeMode", "peak")).lower() == "rms"
    nfft = int(params.get("zeroPadding", 0) or 0)
    resolution = float(params.get("resolutionHz", 10.0) or 10.0)
    # 重叠率按百分比传入（C++ 侧 50 表示 50%）
    overlap = float(params.get("overlap", 50.0) or 0.0) / 100.0

    freqs, amps = spectral.amplitude_spectrum(y, fs, window=window, nfft=(nfft or None), rms=rms)
    nperseg = spectral.seg_len_from_resolution(fs, resolution)
    nperseg = max(2, min(nperseg, y.size))
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
        "resolution_hz": float(fs) / float(nperseg),
        "window": window,
        "rms": rms,
    }


def compute_spectrum_sync_from_source(source, params):
    """同步计算（从来源描述读取数据；后台线程内部使用）。"""
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    x = _load_channel(source)
    return compute_spectrum_sync(x, fs, params)


def compute_spectrum_async(source, params):
    """在守护线程中计算频谱，返回任务 id。"""
    task_name = "signal spectrum"
    task_id, status = tsm.create_task_with_status(task_name)
    status.start(task_name)

    def worker():
        try:
            status.update_progress(5.0, "reading samples")
            result = compute_spectrum_sync_from_source(source, params)
            status.update_progress(95.0, "finalizing")
            _RESULT_STORE[task_id] = result
            status.finish(True, "")
        except Exception as e:  # noqa: BLE001 - 失败原因必须回传给界面
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
    info = tsm.get_task_status(task_id)
    return bool(info and info.get("is_canceled"))
