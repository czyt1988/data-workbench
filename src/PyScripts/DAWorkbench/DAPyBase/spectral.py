"""通用频谱计算纯函数（信号分析插件与工作流共用）。

本模块只依赖 numpy / scipy，不导入 da_app、不做界面、不读配置，
便于 pytest 直接测试，也便于后续工作流节点与智能体工具复用（方案 D6 / D22）。

口径（方案 §6.2）：
- 单边峰值幅度谱：A[0] = |X[0]|/U，A[k] = 2|X[k]|/U（k ≥ 1），U = Σw[n]
- 均方根谱：A_rms[k] = A[k]/√2（k ≥ 1）；直流分量的均方根等于其幅值，不除 √2
- 分块平均幅度谱：段长由目标分辨率反推（Δf = fs/nperseg），逐段单边峰值幅度谱再按
  mean/median 平均（方案 C4）
- 韦尔奇功率谱密度：P[f] = 2/(fs·Σw²)·mean(|X_f|²)（单边，scipy.signal.welch 的 density 口径）
- dB：幅值类 20·log10(A/ref)，功率类 10·log10(P/ref)，ref=1.0 即 dBFS
"""

import numpy as np

# 支持的窗函数名 → scipy.signal.windows 名称
WINDOW_NAMES = {
    "rectangular": None,  # 矩形窗（全 1）
    "boxcar": None,
    "hann": "hann",
    "hanning": "hann",
    "hamming": "hamming",
    "blackman": "blackman",
    "flattop": "flattop",
}

# 支持的窗函数显示名（供界面枚举使用）
WINDOW_LIST = ["rectangular", "hann", "hamming", "blackman", "flattop"]

_EPS = 1e-30

# 分块平均幅度谱每块帧数（只限制临时内存，不影响结果）
_SEG_BLOCK_FRAMES = 256


def window_values(name, length):
    """返回窗函数数组。

    :param name: 窗函数名（rectangular/hann/hamming/blackman/flattop，大小写不敏感）
    :param length: 窗长（采样点数）
    :return: float64 一维数组，长度 length；未知名称回退矩形窗（全 1）
    """
    length = int(max(1, length))
    key = str(name or "hann").strip().lower()
    scipy_name = WINDOW_NAMES.get(key)  # 未知名称得到 None，即回退矩形窗
    if scipy_name is None or length == 1:
        return np.ones(length, dtype=np.float64)
    from scipy.signal import windows

    return np.asarray(windows.get_window(scipy_name, length, fftbins=True), dtype=np.float64)


def detrend_signal(x, mode="none"):
    """去趋势。

    :param x: 一维序列
    :param mode: none / constant（去直流）/ linear（线性去趋势）
    :return: float64 数组
    """
    arr = np.asarray(x, dtype=np.float64)
    m = str(mode or "none").strip().lower()
    if m in ("none", ""):
        return arr
    if m == "constant":
        if arr.size == 0:
            return arr
        return arr - np.mean(arr)
    if m == "linear":
        from scipy.signal import detrend

        return detrend(arr, type="linear")
    return arr


def to_db(a, ref=1.0, kind="amplitude"):
    """转分贝。

    :param a: 幅值或功率数组
    :param ref: 参考值（1.0 即 dBFS）
    :param kind: amplitude（20·log10）或 power（10·log10）
    :return: dB 数组（float64）
    """
    arr = np.atleast_1d(np.asarray(a, dtype=np.float64))
    scale = 20.0 if str(kind).lower() != "power" else 10.0
    return scale * np.log10(np.maximum(np.abs(arr), _EPS) / max(abs(float(ref)), _EPS))


def _amplitude_from_spec(spec, window_sum, n_fft, rms):
    """FFT 结果 → 单边峰值幅度谱（一维/批量共用）。

    - DC 不乘 2；偶数点数时最后一个频点（Nyquist）不乘 2；
    - rms=True 时仅 k ≥ 1 的频点除以 √2：直流分量的均方根等于其幅值（C2/C9）。

    :param spec: np.fft.rfft 的结果（一维或二维，最后一维为频点）
    :param window_sum: 窗函数之和 U = Σw[n]
    :param n_fft: FFT 点数（用于判断 Nyquist 频点）
    :param rms: 是否输出均方根谱
    :return: 幅度数组（与 spec 同形状）
    """
    u = float(window_sum)
    if u <= 0.0:
        u = 1.0
    mag = np.abs(spec) / u
    if mag.shape[-1] > 1:
        mag[..., 1:] *= 2.0
        if n_fft % 2 == 0:
            # 偶数点时最后一个频点是 Nyquist，不应乘 2
            mag[..., -1] /= 2.0
    if rms:
        mag[..., 1:] /= np.sqrt(2.0)
    return mag


def amplitude_spectrum(x, fs, window="hann", nfft=None, rms=False):
    """单边峰值幅度谱（方案 §6.2）。

    :param x: 一维序列
    :param fs: 采样率（赫兹）
    :param window: 窗函数名（未知名称回退矩形窗）
    :param nfft: FFT 总点数（大于段长即零填充；None 表示等于段长）
    :param rms: True 返回均方根谱（k ≥ 1 除 √2，直流分量不除）
    :return: (freqs, amplitudes)
    """
    arr = np.asarray(x, dtype=np.float64)
    if arr.size == 0:
        return np.zeros(0), np.zeros(0)
    seg_len = arr.size
    n = int(nfft) if nfft else seg_len
    n = max(n, seg_len)
    w = window_values(window, seg_len)
    spec = np.fft.rfft(arr * w, n=n)
    mag = _amplitude_from_spec(spec, float(np.sum(w)), n, rms)
    freqs = np.fft.rfftfreq(n, d=1.0 / float(fs))
    return freqs, mag


def amplitude_spectrum_segmented(
    x, fs, nperseg, overlap=0.5, window="hann", nfft=None, average="mean", rms=False, cancel_hook=None
):
    """分块平均的单边峰值幅度谱（方案 C4）。

    段长 nperseg 由目标频率分辨率反推（Δf = fs/nperseg），段间按 overlap 重叠；
    逐段按 amplitude_spectrum 的口径算单边峰值幅度谱，再逐频点按 average 平均。
    当 nperseg >= 数据长度时退化为整段单次 FFT（与 amplitude_spectrum 完全一致）。

    :param x: 一维序列
    :param fs: 采样率（赫兹）
    :param nperseg: 段长（采样点数）
    :param overlap: 段间重叠率 0~1（0.5 表示 50%）
    :param window: 窗函数名（未知名称回退矩形窗）
    :param nfft: FFT 总点数（段长 + 零填充额外点数；None 表示等于段长）
    :param average: mean / median（中位数需要保存全部段谱，内存 O(帧数 × 频点数)）
    :param rms: True 返回均方根谱（k ≥ 1 除 √2，直流分量不除）
    :param cancel_hook: 可选无参回调，每个分块调用一次；回调抛异常即中止计算（供编排层取消）
    :return: (freqs, amplitudes)
    """
    arr = np.asarray(x, dtype=np.float64)
    nperseg = int(max(1, nperseg))
    if arr.size == 0 or nperseg >= arr.size:
        return amplitude_spectrum(arr, fs, window=window, nfft=nfft, rms=rms)
    hop = hop_from_overlap(nperseg, max(0.0, min(0.99, float(overlap))))
    n_frames = frame_count(arr.size, nperseg, hop)
    if n_frames <= 0:
        return amplitude_spectrum(arr, fs, window=window, nfft=nfft, rms=rms)
    n = int(nfft) if nfft else nperseg
    n = max(n, nperseg)
    w = window_values(window, nperseg)
    u = float(np.sum(w))
    use_median = str(average or "mean").strip().lower() == "median"
    accum = None
    median_frames = []
    for f0 in range(0, n_frames, _SEG_BLOCK_FRAMES):
        if cancel_hook is not None:
            cancel_hook()
        f1 = min(n_frames, f0 + _SEG_BLOCK_FRAMES)
        segs = np.empty((f1 - f0, nperseg), dtype=np.float64)
        for k in range(f0, f1):
            start = k * hop
            segs[k - f0, :] = arr[start : start + nperseg]
        spec = np.fft.rfft(segs * w, n=n, axis=1)
        mag = _amplitude_from_spec(spec, u, n, rms)
        if use_median:
            # float32 保存段谱，控制中位数路径的内存占用
            median_frames.append(mag.astype(np.float32, copy=False))
        else:
            block_sum = mag.sum(axis=0)
            accum = block_sum if accum is None else accum + block_sum
        del mag, spec, segs
    if use_median:
        stacked = np.concatenate(median_frames, axis=0) if len(median_frames) > 1 else median_frames[0]
        amps = np.median(stacked, axis=0).astype(np.float64, copy=False)
        del median_frames, stacked
    else:
        amps = accum / float(n_frames)
    freqs = np.fft.rfftfreq(n, d=1.0 / float(fs))
    return freqs, amps


def welch_psd(x, fs, nperseg, overlap=0.5, window="hann", average="mean"):
    """韦尔奇法功率谱密度（单边，单位「幅值²/赫兹」）。

    :param x: 一维序列
    :param fs: 采样率（赫兹）
    :param nperseg: 段长（采样点数）
    :param overlap: 重叠率 0~1（段间重叠比例）
    :param window: 窗函数名
    :param average: mean / median
    :return: (freqs, psd)
    """
    arr = np.asarray(x, dtype=np.float64)
    nperseg = int(max(2, nperseg))
    if arr.size < 2:
        return np.zeros(0), np.zeros(0)
    nperseg = min(nperseg, arr.size)
    hop = int(max(1, round(nperseg * (1.0 - float(overlap)))))
    noverlap = max(0, nperseg - hop)
    from scipy.signal import welch

    avg = "median" if str(average).strip().lower() == "median" else "mean"
    f, p = welch(
        arr,
        fs=float(fs),
        window=window_values(window, nperseg),
        nperseg=nperseg,
        noverlap=noverlap,
        detrend=False,
        return_onesided=True,
        scaling="density",
        average=avg,
    )
    return f, p


def stft_matrix(x, fs, nperseg, hop, window="hann", nfft=None):
    """短时傅里叶变换幅值矩阵（未取 dB）。

    帧数 = floor((N - nperseg) / hop) + 1；
    时间轴用帧中心：t_i = (i*hop + nperseg/2) / fs（方案 §6.3）。

    :return: (times, freqs, mag)，mag 形状 (n_frames, n_freqs)
    """
    arr = np.asarray(x, dtype=np.float64)
    nperseg = int(max(2, nperseg))
    hop = int(max(1, hop))
    if arr.size < nperseg:
        return np.zeros(0), np.zeros(0), np.zeros((0, 0))
    n = int(nfft) if nfft else nperseg
    n = max(n, nperseg)
    w = window_values(window, nperseg)
    u = float(np.sum(w))
    if u <= 0.0:
        u = 1.0
    n_frames = (arr.size - nperseg) // hop + 1
    freqs = np.fft.rfftfreq(n, d=1.0 / float(fs))
    mag = np.empty((n_frames, freqs.size), dtype=np.float64)
    for i in range(n_frames):
        seg = arr[i * hop : i * hop + nperseg]
        spec = np.fft.rfft(seg * w, n=n)
        m = np.abs(spec) / u
        if m.size > 1:
            m[1:] *= 2.0
            if n % 2 == 0 and m.size > 1:
                m[-1] /= 2.0
        mag[i, :] = m
    times = (np.arange(n_frames, dtype=np.float64) * hop + nperseg / 2.0) / float(fs)
    return times, freqs, mag


def find_peaks_topn(mag, freqs, n=10, prominence=None, min_spacing_hz=None, height=None, rel_db=40.0):
    """查找前 N 个峰（等价 scipy.signal.find_peaks，方案 §6.4）。

    过滤顺序：
    1. scipy.signal.find_peaks(height, prominence, distance)；
    2. 相对门限：mag >= max_mag * 10**(-rel_db/20)；
    3. 按幅值降序取前 N，同幅值按频率升序。

    :param mag: 幅值数组
    :param freqs: 对应频率轴
    :param n: 前 N 个
    :param prominence: 突出度（None 表示不限制）
    :param min_spacing_hz: 峰间最小间距（赫兹），按频率分辨率换算成 bin 距离
    :param height: 绝对高度门限（None 表示不限制）
    :param rel_db: 相对最强峰的允许下探量（分贝）
    :return: dict {indices, frequencies, magnitudes}
    """
    from scipy.signal import find_peaks

    mag_arr = np.asarray(mag, dtype=np.float64)
    freqs_arr = np.asarray(freqs, dtype=np.float64)
    empty = {"indices": np.zeros(0, dtype=np.int64), "frequencies": np.zeros(0), "magnitudes": np.zeros(0)}
    if mag_arr.size < 3 or freqs_arr.size != mag_arr.size:
        return empty
    # 频率分辨率从频率轴推断
    df = float(freqs_arr[1] - freqs_arr[0]) if freqs_arr.size > 1 else 0.0
    distance_bins = 1
    if min_spacing_hz is not None and df > 0:
        distance_bins = int(max(1, round(float(min_spacing_hz) / df)))
    idx, _ = find_peaks(
        mag_arr,
        height=(float(height) if height is not None else None),
        prominence=(float(prominence) if prominence is not None else None),
        distance=distance_bins,
    )
    if idx.size == 0:
        return empty
    # 相对门限过滤
    peak_max = float(np.max(mag_arr))
    if peak_max > 0 and rel_db is not None:
        threshold = peak_max * (10.0 ** (-float(rel_db) / 20.0))
        idx = idx[mag_arr[idx] >= threshold]
    if idx.size == 0:
        return empty
    # 排序：幅值降序，同幅值频率升序
    order = np.lexsort((freqs_arr[idx], -mag_arr[idx]))
    idx = idx[order][: max(1, int(n))]
    return {"indices": idx.astype(np.int64), "frequencies": freqs_arr[idx], "magnitudes": mag_arr[idx]}


# ---------------------------------------------------------------- 参数映射（§6.3）


def seg_len_from_resolution(fs, resolution_hz):
    """由目标频率分辨率反推段长：nperseg = round(fs / Δf)"""
    res = max(float(resolution_hz), 1e-9)
    return int(max(8, round(float(fs) / res)))


def hop_from_overlap(nperseg, overlap):
    """由重叠率推跳步：hop = max(1, round(nperseg * (1 - overlap)))"""
    return int(max(1, round(int(nperseg) * (1.0 - float(overlap)))))


def frame_count(sample_count, nperseg, hop):
    """帧数 = floor((N - nperseg) / hop) + 1"""
    if sample_count < nperseg:
        return 0
    return int((int(sample_count) - int(nperseg)) // int(hop) + 1)
