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
- 取数与编排层全程 float32（N-C1b）：文件路径按块只读所需通道，不整段升 float64，
  FFT 在核心层分块循环内部升 float64，避免「整段 float64 + nan_to_num 副本」；
- 按块取数（D1-a）：C++ 把 ``DASignalSource::readRange`` 作为 ``source["read"]`` 注入后，
  频谱/功率谱改走核心层 provider 流式路径（按块请求 + carry 成帧），完全不为整段信号分配数组；
  没有 reader 时保留原文件/数据帧整段回退路径，数值口径不变；
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
    """按来源描述读取单通道数据（float32，评审 N-C1b）。

    - 文件来源：按块只读目标通道（importer.read_channel_all），峰值内存是
      「块 × 通道数」而不是「整文件所有通道」；
    - 数据帧来源：取该列后转 float32（不整段升 float64）；
    - NaN/Inf 统一用 np.nan_to_num 原地归零（C6），不产生第二份整段副本；
    - FFT 的 float64 转换只发生在核心层分块循环内部。

    :return: float32 一维数组
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
        values = np.asarray(df[column].to_numpy(), dtype=np.float32)
    else:
        values = importer.read_channel_all(source.get("path", ""), channel)
        if values.dtype != np.float32:
            values = values.astype(np.float32, copy=False)
    # 两路统一原地归零 NaN/Inf（C6），不再复制整段数组
    values = np.nan_to_num(values, copy=False, nan=0.0, posinf=0.0, neginf=0.0)
    if values.size == 0:
        raise RuntimeError("signal contains no samples")
    return values


def _detrend_mode(params):
    """参数字典 → 去趋势模式（与 _preprocess 同语义：removeDc 映射到 constant）。

    :return: none / constant / linear
    """
    mode = str(params.get("detrend", "none")).lower()
    if bool(params.get("removeDc", False)) and mode in ("none", ""):
        mode = "constant"
    return mode


def _preprocess(x, params):
    """预处理：去直流 + 去趋势（方案 §8.6 信号预处理分组）。"""
    return spectral.detrend_signal(x, _detrend_mode(params))


def _source_provider(source):
    """来源描述 → 分块取数 callable（D1-a：C++ 把 ``readRange`` 作为 ``source["read"]`` 注入）。

    有 reader 时返回 ``provider(start, count)``：内部按源描述里的 channel 调用
    ``source["read"](channel, start, count)``，统一转 float32（不整段升 float64，N-C1b），
    并逐块把 NaN/Inf 归零（与 ``_load_channel`` 的整段 nan_to_num 口径一致，C6）。
    无 reader（旧来源 / 数据帧来源 / 未注入 readRange 的会话）返回 None，
    调用方沿用 ``_load_channel`` 的整段回退路径，行为与数值口径不变。

    :param source: 来源描述（含 type/path/dataframeName/column/channel/sampleRate/sampleCount/read）
    :return: callable(start, count) -> float32 一维数组；或 None
    """
    reader = source.get("read")
    if not callable(reader):
        return None
    channel = int(source.get("channel", 0))

    def _provider(start, count):
        """按块向 C++ readRange 取一段单通道样本（float32）。"""
        block = np.asarray(reader(channel, int(start), int(count)), dtype=np.float32)
        if block.ndim != 1:
            block = block.reshape(-1)
        if block.size and not np.all(np.isfinite(block)):
            block = np.nan_to_num(block, copy=True, nan=0.0, posinf=0.0, neginf=0.0)
        return block

    return _provider


def _spectrum_setup(fs, sample_count, params):
    """由参数字典推导段长/零填充/重叠率/平均方式（整段与 provider 两条路径共用）。

    段长由目标频率分辨率反推：nperseg = clamp(round(fs/resolutionHz), 2, N)（C4）；
    零填充语义为「额外点数」：nfft = 段长 + zeroPadding（C3）；重叠率为百分比（C++ 传 50）。

    :return: (window, overlap, nperseg, nfft, average)
    """
    window = str(params.get("window", "hann"))
    zero_padding = int(params.get("zeroPadding", 0) or 0)
    resolution = float(params.get("resolutionHz", 10.0) or 10.0)
    # 重叠率按百分比传入（C++ 侧 50 表示 50%）
    overlap = float(params.get("overlap", 50.0) or 0.0) / 100.0
    sample_count = int(sample_count)
    if sample_count >= 2:
        nperseg = int(max(2, min(spectral.seg_len_from_resolution(fs, resolution), sample_count)))
    else:
        nperseg = int(sample_count)
    nfft = (nperseg + zero_padding) if zero_padding > 0 else None
    return window, overlap, nperseg, nfft, str(params.get("average", "mean"))


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
    # N-C1b：编排层保持 float32，FFT 段内由核心层升 float64
    y = _preprocess(np.asarray(x), params)
    # 段长/零填充/重叠率/平均方式由参数统一推导（C3/C4，与 provider 路径共用）
    window, overlap, nperseg, nfft, average = _spectrum_setup(fs, y.size, params)
    # C2：口径统一在显示/导出层换算，计算层永远输出峰值幅度
    rms = False
    cancel_hook = _spectrum_cancel_hook(task_id)

    freqs, amps = spectral.amplitude_spectrum_segmented(
        y,
        fs,
        nperseg=nperseg,
        overlap=overlap,
        window=window,
        nfft=nfft,
        average=average,
        rms=rms,
        cancel_hook=cancel_hook,
    )
    psd_freqs, psd_values = spectral.welch_psd(
        y,
        fs,
        nperseg=nperseg,
        overlap=overlap,
        window=window,
        average=average,
        cancel_hook=cancel_hook,
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


def _spectrum_cancel_hook(task_id):
    """构造分块取消回调（C7）：每块调用一次，命中取消即抛 TaskCanceledError。

    :return: 无参回调；task_id 为 None 时返回 None
    """
    if task_id is None:
        return None
    tsm.raise_if_canceled(task_id)

    def _cancel_hook():
        """每个分块调用一次，命中取消即抛 TaskCanceledError。"""
        tsm.raise_if_canceled(task_id)

    return _cancel_hook


def _compute_spectrum_from_provider(provider, sample_count, fs, params, task_id=None):
    """provider 流式计算频谱与功率谱（D1-a：不整段读入、不整段成帧）。

    与 ``compute_spectrum_sync`` 数值口径一致（段长/零填充/重叠率/平均方式相同），
    取数改为按块向 ``source["read"]`` 请求：全程 float32，FFT 段内升 float64，
    常驻内存只与频点数（median 时再加「帧数 × 频点数」float32 段谱缓冲）有关。

    :param provider: 分块取数 callable provider(start, count) -> float32 一维数组
    :param sample_count: 信号总采样点数（来自 source["sampleCount"]）
    :param fs: 采样率
    :param params: 参数字典（键名见 DASignalAnalysisParams::toJson）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :return: 与 compute_spectrum_sync 完全相同的返回字典
    """
    window, overlap, nperseg, nfft, average = _spectrum_setup(fs, sample_count, params)
    detrend = _detrend_mode(params)
    rms = False
    cancel_hook = _spectrum_cancel_hook(task_id)
    # 全局趋势参数（constant/linear）只做一遍流式统计，幅度谱与 PSD 共用同一份结果，
    # 避免为两个函数各扫一遍全量数据（none 时不扫描，plan 为 None）
    detrend_plan = spectral._provider_detrend_plan(provider, sample_count, detrend, cancel_hook=cancel_hook)
    freqs, amps = spectral.amplitude_spectrum_segmented_provider(
        provider,
        sample_count,
        fs,
        nperseg=nperseg,
        overlap=overlap,
        window=window,
        nfft=nfft,
        average=average,
        rms=rms,
        detrend=detrend,
        cancel_hook=cancel_hook,
        detrend_plan=detrend_plan,
    )
    psd_freqs, psd_values = spectral.welch_psd_provider(
        provider,
        sample_count,
        fs,
        nperseg=nperseg,
        overlap=overlap,
        window=window,
        average=average,
        detrend=detrend,
        cancel_hook=cancel_hook,
        detrend_plan=detrend_plan,
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
    """同步计算（从来源描述读取数据；后台线程内部使用）。

    取数策略（D1-a）：

    - 来源带 ``read`` callable（C++ ``DASignalSource::readRange`` 注入）且带
      ``sampleCount`` 时，走核心层 provider 流式路径：按块取数、全程 float32，
      FFT 段内升 float64，不整段读入；detrend/去直流由 provider 版函数先做一遍
      流式统计再在块内减去；
    - 否则回退现有整段取数路径（文件 / 数据帧来源，行为与数值口径不变）。

    :param source: 来源描述（type/path 或 dataframeName/column，可含 channel/sampleCount/read）
    :param params: 参数字典（键名见 DASignalAnalysisParams::toJson）
    :param task_id: 可选任务 id；传入时每个分块检查取消标志（C7）
    :return: dict{freqs, amplitudes, psd_freqs, psd_values, seg_len, resolution_hz, window, rms}
    """
    fs = float(source.get("sampleRate", 0.0))
    if fs <= 0:
        raise RuntimeError("invalid sample rate")
    provider = _source_provider(source)
    sample_count = source.get("sampleCount")
    if provider is not None and sample_count is not None and int(sample_count) > 0:
        return _compute_spectrum_from_provider(provider, int(sample_count), fs, params, task_id=task_id)
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
            if tsm.is_task_canceled(task_id):
                # 写结果表与取消的竞态：取消后自行清理，避免结果滞留（C7）
                _RESULT_STORE.pop(task_id, None)
                return
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
