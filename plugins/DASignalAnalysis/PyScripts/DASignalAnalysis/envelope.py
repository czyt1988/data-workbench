"""波形显示包络降采样。

目的（方案 §7.1）：1.7 亿点的波形无法整体塞进 QwtPlotCurve，
按「视图区间 + 像素宽」算每列的 min/max，返回至多 2×像素宽的点。

本模块不做界面；由 C++ 侧在后台任务中调用。
"""

import threading
import traceback

import numpy as np

from DAWorkbench.DAPyBase import thread_status_manager as tsm

from . import importer

_RESULT_STORE = {}


def envelope_of_window(x, start, end, width):
    """把一段数据按列池化成 min/max 包络。

    :param x: 一维数据（float，可为 float32/float64）
    :param start: 区间起点（采样点，含）
    :param end: 区间终点（采样点，不含）
    :param width: 目标列数（≈ 绘图区像素宽）
    :return: dict{x, mins, maxs}，x 为采样点下标，长度为实际列数
    """
    arr = np.asarray(x)
    start = int(max(0, start))
    end = int(min(arr.size, end))
    width = int(max(1, width))
    if end <= start:
        return {"x": np.zeros(0), "mins": np.zeros(0), "maxs": np.zeros(0)}
    seg = arr[start:end].astype(np.float32, copy=False)
    n = seg.size
    # 把 n 个点分到 width 列
    cols = min(width, n)
    edges = np.linspace(0, n, cols + 1, dtype=np.int64)
    mins = np.empty(cols, dtype=np.float32)
    maxs = np.empty(cols, dtype=np.float32)
    for i in range(cols):
        s = int(edges[i])
        e = int(edges[i + 1])
        if e <= s:
            e = min(n, s + 1)
        chunk = seg[s:e]
        mins[i] = np.min(chunk)
        maxs[i] = np.max(chunk)
    xs = (edges[:-1] + edges[1:]) / 2.0 + start
    return {"x": xs.astype(np.float64), "mins": mins.astype(np.float64), "maxs": maxs.astype(np.float64)}


def _load_range(source, start, end):
    """读取来源的某段单通道数据。"""
    stype = str(source.get("type", "file"))
    channel = int(source.get("channel", 0))
    if stype == "dataframe":
        import da_app

        datamanager = da_app.getCore().getDataManagerInterface()
        frames = datamanager.getAllDataframes()
        df = frames.get(source.get("dataframeName", ""))
        if df is None:
            raise RuntimeError("dataframe not found: %s" % source.get("dataframeName", ""))
        values = np.asarray(df[source.get("column", "")].to_numpy())
        return np.asarray(values[int(start) : int(end)], dtype=np.float32)
    return importer.read_range(source.get("path", ""), channel, start, end - start)


def compute_envelope_sync(source, channel, start, end, width):
    """同步计算包络（后台线程内部使用）。"""
    src = dict(source)
    src["channel"] = channel
    seg = _load_range(src, start, end)
    return envelope_of_window(seg, 0, seg.size, width)


def compute_envelope_async(source, channel, start, end, width):
    """在守护线程中计算包络，返回任务 id。"""
    task_id, status = tsm.create_task_with_status("signal envelope")
    status.start("signal envelope")

    def worker():
        try:
            status.update_progress(10.0, "reading samples")
            result = compute_envelope_sync(source, channel, start, end, width)
            status.update_progress(95.0, "finalizing")
            _RESULT_STORE[task_id] = result
            status.finish(True, "")
        except Exception as e:  # noqa: BLE001 - 失败原因必须回传
            status.finish(False, "%s\n%s" % (str(e), traceback.format_exc()))

    threading.Thread(target=worker, daemon=True).start()
    return task_id


def take_result(task_id):
    """取走包络结果。"""
    return _RESULT_STORE.pop(task_id, None)
