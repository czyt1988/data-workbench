"""通用频谱计算纯函数（信号分析插件与工作流共用）。

本模块只依赖 numpy / scipy，不导入 da_app、不做界面、不读配置，
便于 pytest 直接测试，也便于后续工作流节点与智能体工具复用（方案 D6 / D22）。

口径（方案 §6.2）：
- 单边峰值幅度谱：A[0] = |X[0]|/U，A[k] = 2|X[k]|/U（k ≥ 1），U = Σw[n]
- 均方根谱：A_rms[k] = A[k]/√2（k ≥ 1）；直流分量的均方根等于其幅值，不除 √2
- 分块平均幅度谱：段长由目标分辨率反推（Δf = fs/nperseg），逐段单边峰值幅度谱再按
  mean/median 平均（方案 C4）
- 韦尔奇功率谱密度：P[f] = 2/(fs·Σw²)·mean(|X_f|²)（单边，scipy.signal.welch 的 density 口径）；
  自实现分块流式累加（每块 256 帧，块内成帧后立即丢弃），mean 口径常驻内存只与频点数有关，
  不会为整段信号一次性分配「帧数 × 段长」矩阵（评审 N-C1a）
- 按块取数 provider（方案 D1-a）：``amplitude_spectrum_segmented_provider`` /
  ``welch_psd_provider`` 用 callable ``provider(start, count)`` 代替整段数组，跨块 carry 成帧，
  不整段读入、不整段成帧；C++ ``DASignalSource::readRange`` 注入的 Python 可调用对象即 provider
- dB：幅值类 20·log10(A/ref)，功率类 10·log10(P/ref)，ref=1.0 即 dBFS

浮点口径（评审 N-C1b）：长信号读取/编排层保持 float32，FFT 只在段内（分块循环里）
升到 float64；本模块不假设输入是 float64，也不做整段 dtype 复制。
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

# 韦尔奇 PSD 每块帧数（只限制临时内存，不影响结果；评审 N-C1a）
_PSD_BLOCK_FRAMES = 256


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

    浮点口径（N-C1b）：保持输入 dtype，不再整段升到 float64。
    - 输入 float32 时用 float32 累加/相减（结果仍 float32），FFT 段内再升 float64；
    - 输入 float64 时行为与旧版一致。

    :param x: 一维序列
    :param mode: none / constant（去直流）/ linear（线性去趋势）
    :return: 与输入同 dtype 的数组（none 时原样返回引用）
    """
    arr = np.asarray(x)
    m = str(mode or "none").strip().lower()
    if m in ("none", ""):
        return arr
    if arr.size == 0:
        return arr
    if m == "constant":
        # 均值用 float64 累加（精度），相减结果按输入 dtype 落地（不整段升精度）
        mean = float(np.mean(arr, dtype=np.float64))
        if arr.dtype == np.float64:
            return arr - mean
        # float32 标量参与运算，避免 arr - float64 产生整段 float64 临时量
        return (arr - np.float32(mean)).astype(arr.dtype, copy=False)
    if m == "linear":
        from scipy.signal import detrend

        # scipy.signal.detrend 对 float32 输入原地相减，结果保持 float32
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

    浮点口径（N-C1b）：入参可为 float32，不在函数入口整段升 float64；
    每块成帧时由 ``segs``（float64）承接，FFT 只在块内用 float64。
    """
    arr = np.asarray(x)
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


def _median_bias(n):
    """中位数相对均值的偏差修正系数（scipy.signal.welch 的 median 口径同款）。

    公式见 B. Allen et al., "FINDCHIRP", Phys. Rev. D 85 (2012)；
    scipy 内部同样用 ``np.median(periodograms) / _median_bias(n_frames)`` 保持无偏。
    """
    if n <= 1:
        return 1.0
    ii_2 = 2.0 * np.arange(1.0, (int(n) - 1) // 2 + 1)
    return float(1.0 + np.sum(1.0 / (ii_2 + 1.0) - 1.0 / ii_2))


def _single_sided_power(spec, n_fft):
    """复数 FFT 结果 → 单边功率 |X|²（DC 不乘 2；偶数点数时 Nyquist 不乘 2）。

    与 scipy.signal.welch 的 one-sided 处理一致：偶数 n_fft 时只把 1..-2 的频点乘 2，
    最后一个频点（Nyquist）保持 1 倍。
    """
    power = np.abs(spec) ** 2
    if power.shape[-1] > 1:
        power[..., 1:] *= 2.0
        if n_fft % 2 == 0:
            power[..., -1] *= 0.5
    return power


def welch_psd(
    x,
    fs,
    nperseg,
    overlap=0.5,
    window="hann",
    average="mean",
    cancel_hook=None,
    block_frames=_PSD_BLOCK_FRAMES,
):
    """韦尔奇法功率谱密度（单边，单位「幅值²/赫兹」）。

    自实现分块流式累加（评审 N-C1a），不复用 ``scipy.signal.welch`` 的整段成帧：

    - 每块最多 ``block_frames`` 帧：块内 ``rfft(seg * win)`` → 单边功率 → 立即归并/暂存，
      块矩阵随即释放；mean 口径常驻内存只有频点数级别，不会为 1 小时信号分配
      「帧数 × 段长」的整段加窗矩阵；
    - 数值口径与 scipy.signal.welch(scaling="density", detrend=False, return_onesided=True)
      完全一致：P[f] = 1/(fs·Σw²) · mean_frames(|X_f|²)（DC/Nyquist 不乘 2）；
    - median 口径需要保留全部帧功率（float32，内存 O(帧数×频点数)），但不再有整段
      加窗矩阵与复数中间量；逐频点求中位数（np.partition 单列）不再复制整张矩阵；
      默认 mean 口径下无此开销；
    - 每个分块检查 ``cancel_hook``（命中抛异常即中止）。

    :param x: 一维序列（可用 float32，块内升 float64）
    :param fs: 采样率（赫兹）
    :param nperseg: 段长（采样点数）
    :param overlap: 重叠率 0~1（段间重叠比例）
    :param window: 窗函数名
    :param average: mean / median
    :param cancel_hook: 可选无参回调，每个分块调用一次（供编排层取消）
    :param block_frames: 每块帧数（只影响临时内存，不影响结果）
    :return: (freqs, psd)
    """
    arr = np.asarray(x)
    nperseg = int(max(2, nperseg))
    if arr.size < 2:
        return np.zeros(0), np.zeros(0)
    nperseg = min(nperseg, int(arr.size))
    fs = float(fs)
    if fs <= 0.0:
        return np.zeros(0), np.zeros(0)
    hop = int(max(1, round(nperseg * (1.0 - float(overlap)))))
    w = window_values(window, nperseg)
    u2 = float(np.sum(w * w))
    if u2 <= 0.0:
        u2 = 1.0
    # density 标定：1/(fs·Σw²)；先累加功率再统一乘，避免逐帧乘法
    scale = 1.0 / (fs * u2)
    n_frames = (int(arr.size) - nperseg) // hop + 1
    n_freqs = nperseg // 2 + 1
    use_median = str(average or "mean").strip().lower() == "median"
    block = int(max(1, block_frames))
    accum = None if use_median else np.zeros(n_freqs, dtype=np.float64)
    # median 口径预分配单块缓冲区（float32），避免 list + concatenate 再复制一份整矩阵
    frames_buf = np.empty((n_frames, n_freqs), dtype=np.float32) if use_median else None
    for f0 in range(0, n_frames, block):
        if cancel_hook is not None:
            cancel_hook()
        f1 = min(n_frames, f0 + block)
        segs = np.empty((f1 - f0, nperseg), dtype=np.float64)
        for k in range(f0, f1):
            start = k * hop
            segs[k - f0, :] = arr[start : start + nperseg]
        spec = np.fft.rfft(segs * w, axis=1)
        power = _single_sided_power(spec, nperseg)
        if use_median:
            frames_buf[f0:f1, :] = power  # float32 暂存，控制中位数路径的内存占用
        else:
            accum += power.sum(axis=0)
        del power, spec, segs
    if use_median:
        # 与 scipy.signal.welch(average="median") 同款中位数偏差修正；逐频点求中位数，
        # 只对单列做 partition，不再整矩阵复制（N-C1a 内存约束）
        bias = _median_bias(n_frames)
        psd = np.empty(n_freqs, dtype=np.float64)
        for i in range(n_freqs):
            psd[i] = np.median(frames_buf[:, i]) / bias
        del frames_buf
    else:
        psd = accum / float(n_frames)
    psd = psd * scale
    freqs = np.fft.rfftfreq(nperseg, d=1.0 / fs)
    return freqs, psd


def stft_matrix(x, fs, nperseg, hop, window="hann", nfft=None):
    """短时傅里叶变换幅值矩阵（未取 dB）。

    帧数 = floor((N - nperseg) / hop) + 1；
    时间轴用帧中心：t_i = (i*hop + nperseg/2) / fs（方案 §6.3）。

    :return: (times, freqs, mag)，mag 形状 (n_frames, n_freqs)
    """
    arr = np.asarray(x)
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


# ---------------------------------------------------------------- 按块取数 provider（D1-a）


# 默认每次向 provider 请求的样本数上限（只限制临时内存，不影响结果）
_PROVIDER_BLOCK_SAMPLES = 1 << 20

# detrend_plan 参数的「未提供」哨兵：区分「没传」与「显式传 None（不做去趋势）」
_DETREND_PLAN_UNSET = object()


def _provider_read(provider, start, count):
    """从 provider 读取一段样本并做防御性处理（float32 一维 + NaN/Inf 归零）。

    ``provider(start, count)`` 约定返回一维 float32（长度 count、C 连续，供 C++
    ``DASignalSource::readRange`` 注入）。这里不假设调用方守约：

    - 非一维结果按行优先展平；
    - 超长截断到 count；长度不足抛 RuntimeError（避免静默错位成帧）；
    - 统一转 float32（不整段升 float64，N-C1b）；
    - NaN/Inf 归零（C6 口径，与 ``spectrum._load_channel`` 的整段 nan_to_num 等价）；
      只在确实存在非有限值时复制，不污染 provider 返回的数组。

    :param provider: 分块取数 callable provider(start, count) -> 一维数组
    :param start: 起始采样点
    :param count: 采样点数
    :return: float32 一维 C 连续数组（长度 count）
    """
    count = int(count)
    block = np.asarray(provider(int(start), count), dtype=np.float32)
    if block.ndim != 1:
        block = block.reshape(-1)
    if block.size > count:
        block = block[:count]
    if block.size < count:
        raise RuntimeError(
            "sample provider returned %d samples, expected %d (start=%d)" % (block.size, count, int(start))
        )
    if block.size and not np.all(np.isfinite(block)):
        block = np.nan_to_num(block, copy=True, nan=0.0, posinf=0.0, neginf=0.0)
    return np.ascontiguousarray(block)


def _provider_blocks(provider, total_samples, block_samples=_PROVIDER_BLOCK_SAMPLES, cancel_hook=None):
    """单遍按块读取整段样本，产出 (start, float32 块)。

    用于去趋势参数的流式统计（第一遍只读扫描）；每块调用一次 ``cancel_hook``。

    :param provider: 分块取数 callable
    :param total_samples: 信号总采样点数
    :param block_samples: 每块样本数上限
    :param cancel_hook: 可选无参回调，每块调用一次
    :return: 生成器，逐块产出 (起始采样点, float32 一维数组)
    """
    total = int(max(0, total_samples))
    block = int(max(1, block_samples))
    pos = 0
    while pos < total:
        if cancel_hook is not None:
            cancel_hook()
        count = min(block, total - pos)
        yield pos, _provider_read(provider, pos, count)
        pos += count


def _provider_detrend_plan(provider, total_samples, detrend, block_samples=_PROVIDER_BLOCK_SAMPLES, cancel_hook=None):
    """第一遍流式统计全局去趋势参数（provider 版频谱 / STFT 共用）。

    语义与 ``detrend_signal`` 一致：

    - none / "" ：返回 None，第二遍原样成帧；
    - constant：返回 ``{"mode": "constant", "mean": 全局均值}``，均值用 float64 流式累加；
    - linear：返回 ``{"mode": "linear", "slope": a, "mean": 全局均值, "index_mean": (N-1)/2}``，
      拟合直线 y ≈ mean + a·(i − index_mean)；用中心化索引累计 ``Σ(i−ī)·x``，
      避免大 i 求和时的数值抵消，等价于 scipy.signal.detrend(type="linear") 的最小二乘直线；
    - 其它模式按 none 处理（与 detrend_signal 的未知模式回退一致）。

    统计前每块样本已经过 NaN/Inf 归零（见 ``_provider_read``），与整段 nan_to_num 后再
    detrend_signal 的口径一致。

    :param provider: 分块取数 callable
    :param total_samples: 信号总采样点数
    :param detrend: none / constant / linear
    :param block_samples: 统计扫描的每块样本数上限
    :param cancel_hook: 可选无参回调，每块调用一次
    :return: None 或 dict（供 ``_apply_provider_detrend`` 使用）
    """
    mode = str(detrend or "none").strip().lower()
    total = int(max(0, total_samples))
    if mode not in ("constant", "linear") or total <= 0:
        return None
    if mode == "constant":
        sum_x = 0.0
        for _start, block in _provider_blocks(provider, total, block_samples, cancel_hook):
            sum_x += float(np.sum(block, dtype=np.float64))
        return {"mode": "constant", "mean": sum_x / float(total)}
    # linear：y = mean + a·(i − index_mean)
    index_mean = 0.5 * float(total - 1)
    sum_x = 0.0
    sum_cx = 0.0
    for start, block in _provider_blocks(provider, total, block_samples, cancel_hook):
        idx = np.arange(int(start), int(start) + block.size, dtype=np.float64)
        sum_x += float(np.sum(block, dtype=np.float64))
        sum_cx += float(np.dot(idx - index_mean, block.astype(np.float64)))
    # Σ(i−ī)² = N(N²−1)/12 的闭式；N=1 时为 0，无法拟合斜率
    sum_cc = float(total) * (float(total) ** 2 - 1.0) / 12.0
    slope = (sum_cx / sum_cc) if sum_cc > 0.0 else 0.0
    return {"mode": "linear", "slope": slope, "mean": sum_x / float(total), "index_mean": index_mean}


def _apply_provider_detrend(block, start, plan):
    """按去趋势参数对块内样本落地去趋势（float32 运算，与整段 detrend_signal 口径一致）。

    - constant：减 ``np.float32(全局均值)``（与 detrend_signal 的 constant 分支同款）；
    - linear：减 ``np.float32(mean + a·(i−ī))``，即全局最小二乘直线；
    - plan 为 None（none/未知模式）时原样返回。

    :param block: 一段 float32 样本（一维）
    :param start: 该段首样本的全局索引（用于 linear 的 i）
    :param plan: ``_provider_detrend_plan`` 的返回值
    :return: float32 一维数组（去趋势后）
    """
    if plan is None:
        return block
    if plan["mode"] == "constant":
        return block - np.float32(plan["mean"])
    idx = np.arange(int(start), int(start) + block.size, dtype=np.float64)
    trend = plan["mean"] + plan["slope"] * (idx - plan["index_mean"])
    return block - trend.astype(np.float32)


def _iter_provider_frames(
    provider,
    total_samples,
    nperseg,
    hop,
    block_samples=_PROVIDER_BLOCK_SAMPLES,
    cancel_hook=None,
    chunk_frames=_SEG_BLOCK_FRAMES,
    detrend_plan=None,
):
    """流式按帧产出 (k0, segs[float64])：segs 形状 (n, nperseg)，覆盖第 k0..k0+n-1 帧。

    帧位置与 ``frame_count`` 完全一致（第 k 帧 = [k*hop, k*hop+nperseg)），跨块用 carry
    保留 ``nperseg-hop`` 个重叠样本，不会漏帧/重帧；每块成帧后立即交付调用方 FFT，
    调用方用完即丢弃，因此不会为整段信号或「帧数 × 段长」矩阵分配内存。

    每次只请求「尚缺且本块需要」的样本（``min(block_samples, 需要量, 剩余量)``），
    请求区间单调不重叠；每读取一个 provider 块调用一次 ``cancel_hook``。
    单块帧数受 ``chunk_frames`` 与 ``block_samples`` 共同限制，块样本跨度不超过
    ``block_samples``（nperseg 本身更大时至少 1 帧）。

    :param provider: 分块取数 callable
    :param total_samples: 信号总采样点数（决定帧数）
    :param nperseg: 段长
    :param hop: 帧跳步
    :param block_samples: 单次请求样本数上限
    :param cancel_hook: 可选无参回调，每块调用一次
    :param chunk_frames: 每次交付的最大帧数
    :param detrend_plan: 可选去趋势参数（``_provider_detrend_plan`` 返回值），逐帧落地
    :return: 生成器，逐块产出 (首帧号, float64 二维数组)
    """
    total = int(max(0, total_samples))
    nperseg = int(nperseg)
    hop = int(hop)
    n_frames = frame_count(total, nperseg, hop)
    block = int(max(1, block_samples))
    chunk = int(max(1, chunk_frames))
    if hop > 0 and block > nperseg:
        # 让单块 256 帧的样本跨度也不超过 block_samples
        chunk = max(1, min(chunk, (block - nperseg) // hop + 1))
    buf = np.empty(0, dtype=np.float32)
    buf_start = 0
    pos = 0
    k = 0
    while k < n_frames:
        n = min(chunk, n_frames - k)
        need_end = (k + n - 1) * hop + nperseg
        while pos < need_end:
            if cancel_hook is not None:
                cancel_hook()
            want = min(block, need_end - pos, total - pos)
            if want <= 0:
                raise RuntimeError("sample provider ended before the last frame")
            part = _provider_read(provider, pos, want)
            buf = part if buf.size == 0 else np.concatenate((buf, part))
            pos += want
        segs = np.empty((n, nperseg), dtype=np.float64)
        base = k * hop - buf_start
        for j in range(n):
            off = base + j * hop
            piece = buf[off : off + nperseg]
            if detrend_plan is not None:
                piece = _apply_provider_detrend(piece, k * hop + j * hop, detrend_plan)
            segs[j, :] = piece
        yield k, segs
        k += n
        if k < n_frames:
            # carry：只保留下一帧起点之后的样本（跨块重叠部分）
            cut = min(k * hop - buf_start, buf.size)
            if cut > 0:
                buf = np.ascontiguousarray(buf[cut:])
                buf_start += cut
        else:
            buf = np.empty(0, dtype=np.float32)
            buf_start = pos
        del segs


def amplitude_spectrum_segmented_provider(
    provider,
    total_samples,
    fs,
    nperseg,
    overlap=0.5,
    window="hann",
    nfft=None,
    average="mean",
    rms=False,
    detrend="none",
    cancel_hook=None,
    block_samples=_PROVIDER_BLOCK_SAMPLES,
    detrend_plan=_DETREND_PLAN_UNSET,
):
    """按块取数的分块平均单边峰值幅度谱（D1-a：不整段读入、不整段成帧）。

    数值口径与 ``amplitude_spectrum_segmented`` 完全一致（帧位置 k*hop、帧数
    ``frame_count``、单边峰值幅度谱归一化、mean/median 平均、零填充语义都相同），
    区别只在取数与成帧方式：数据经 ``provider(start, count)`` 按块拉取，跨块用 carry
    保留重叠样本，块内成帧 → FFT → 立即归并后丢弃块矩阵。

    内存口径：

    - mean 口径常驻只有「频点数」量级的 float64 累加数组，不分配整段信号，也不分配
      「帧数 × 段长」成帧矩阵；
    - median 口径只分配「帧数 × 频点数」float32 段谱缓冲（与 ``welch_psd`` 的 median
      同思路），同样没有整段信号/整段成帧矩阵；幅度路径**不**做 welch 的 median bias
      修正（与 ``amplitude_spectrum_segmented`` 保持一致）；
    - ``nperseg >= total_samples``（短信号）时允许回退「整段读 + 现有函数」，此时
      内存与数值都与数组版完全相同。

    浮点与预处理（N-C1b / C6）：

    - 数据块保持 float32，FFT 只在段内升 float64；
    - NaN/Inf 逐块归零（nan_to_num 口径，见 ``_provider_read``）；
    - ``detrend`` 语义与 ``detrend_signal`` 相同：none / constant（去全局均值）/
      linear（去全局最小二乘直线）。constant/linear 会先做一遍流式统计（多一次只读扫描），
      再在块内减去趋势；未知模式按 none 处理。

    :param provider: 分块取数 callable provider(start, count) -> 一维 float32（长度 count）
    :param total_samples: 信号总采样点数（帧数由它决定）
    :param fs: 采样率（赫兹）
    :param nperseg: 段长（采样点数）
    :param overlap: 段间重叠率 0~1（0.5 表示 50%）
    :param window: 窗函数名（未知名称回退矩形窗）
    :param nfft: FFT 总点数（大于段长即零填充；None 表示等于段长）
    :param average: mean / median
    :param rms: True 返回均方根谱（k ≥ 1 除 √2，直流分量不除）
    :param detrend: none / constant / linear
    :param cancel_hook: 可选无参回调，每次按块读取调用一次（供编排层取消）
    :param block_samples: 每次向 provider 请求的样本数上限（只影响临时内存，不影响结果）
    :param detrend_plan: 内部复用参数（编排层可先调 ``_provider_detrend_plan`` 算一次，
        让幅度谱与 PSD 共用同一遍统计扫描）；不传则按 detrend 自行统计
    :return: (freqs, amplitudes)
    """
    total = int(max(0, total_samples))
    nperseg = int(max(1, nperseg))
    if total == 0 or nperseg >= total:
        x = _provider_read(provider, 0, total) if total > 0 else np.zeros(0, dtype=np.float32)
        y = detrend_signal(x, detrend)
        return amplitude_spectrum_segmented(
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
    hop = hop_from_overlap(nperseg, max(0.0, min(0.99, float(overlap))))
    n_frames = frame_count(total, nperseg, hop)
    if n_frames <= 0:
        x = _provider_read(provider, 0, total)
        y = detrend_signal(x, detrend)
        return amplitude_spectrum(arr=y, fs=fs, window=window, nfft=nfft, rms=rms)
    n = int(nfft) if nfft else nperseg
    n = max(n, nperseg)
    w = window_values(window, nperseg)
    u = float(np.sum(w))
    use_median = str(average or "mean").strip().lower() == "median"
    n_freqs = n // 2 + 1
    if detrend_plan is _DETREND_PLAN_UNSET:
        plan = _provider_detrend_plan(provider, total, detrend, block_samples, cancel_hook)
    else:
        plan = detrend_plan
    median_buf = np.empty((n_frames, n_freqs), dtype=np.float32) if use_median else None
    accum = None if use_median else np.zeros(n_freqs, dtype=np.float64)
    for k0, segs in _iter_provider_frames(
        provider,
        total,
        nperseg,
        hop,
        block_samples=block_samples,
        cancel_hook=cancel_hook,
        detrend_plan=plan,
    ):
        spec = np.fft.rfft(segs * w, n=n, axis=1)
        mag = _amplitude_from_spec(spec, u, n, rms)
        count = mag.shape[0]
        if use_median:
            median_buf[k0 : k0 + count, :] = mag
        else:
            accum += mag.sum(axis=0)
        del mag, spec, segs
    if use_median:
        # 逐频点中位数（幅度路径不做 welch 的 median bias 修正）；逐列 partition，
        # 不再复制整张「帧数 × 频点数」矩阵（与 welch_psd 的 median 同思路）
        amps = np.empty(n_freqs, dtype=np.float64)
        for i in range(n_freqs):
            amps[i] = np.median(median_buf[:, i])
        del median_buf
    else:
        amps = accum / float(n_frames)
    freqs = np.fft.rfftfreq(n, d=1.0 / float(fs))
    return freqs, amps


def welch_psd_provider(
    provider,
    total_samples,
    fs,
    nperseg,
    overlap=0.5,
    window="hann",
    average="mean",
    detrend="none",
    cancel_hook=None,
    block_samples=_PROVIDER_BLOCK_SAMPLES,
    detrend_plan=_DETREND_PLAN_UNSET,
):
    """按块取数的韦尔奇功率谱密度（D1-a：不整段读入、不整段成帧）。

    数值口径与 ``welch_psd(x, ...)`` 完全一致：帧位置、单边功率、density 标定
    1/(fs·Σw²)、median 口径的偏差修正都相同；区别只在取数与成帧方式——数据经
    ``provider(start, count)`` 按块拉取，跨块 carry 保留重叠样本，块内成帧后立即
    FFT 并归并/暂存，块矩阵随即释放。

    内存口径与 ``welch_psd`` 相同：mean 口径常驻只有频点数级别；median 口径只分配
    「帧数 × 频点数」float32 段谱缓冲，不分配整段信号或「帧数 × 段长」成帧矩阵。
    ``nperseg >= total_samples`` 时回退「整段读 + 现有函数」。

    NaN/Inf 归零与 detrend 语义见 ``amplitude_spectrum_segmented_provider``。

    :param provider: 分块取数 callable provider(start, count) -> 一维 float32（长度 count）
    :param total_samples: 信号总采样点数
    :param fs: 采样率（赫兹）
    :param nperseg: 段长（采样点数）
    :param overlap: 段间重叠率 0~1
    :param window: 窗函数名
    :param average: mean / median
    :param detrend: none / constant / linear
    :param cancel_hook: 可选无参回调，每次按块读取调用一次（供编排层取消）
    :param block_samples: 每次向 provider 请求的样本数上限（只影响临时内存，不影响结果）
    :param detrend_plan: 内部复用参数（与 amplitude_spectrum_segmented_provider 共用一遍
        ``_provider_detrend_plan`` 统计结果）；不传则按 detrend 自行统计
    :return: (freqs, psd)
    """
    total = int(max(0, total_samples))
    fs = float(fs)
    nperseg = int(max(2, nperseg))
    if total < 2 or fs <= 0.0:
        return np.zeros(0), np.zeros(0)
    nperseg = min(nperseg, total)
    if nperseg >= total:
        x = _provider_read(provider, 0, total)
        y = detrend_signal(x, detrend)
        return welch_psd(
            y,
            fs,
            nperseg,
            overlap=overlap,
            window=window,
            average=average,
            cancel_hook=cancel_hook,
        )
    hop = int(max(1, round(nperseg * (1.0 - float(overlap)))))
    n_frames = frame_count(total, nperseg, hop)
    if n_frames <= 0:
        return np.zeros(0), np.zeros(0)
    w = window_values(window, nperseg)
    u2 = float(np.sum(w * w))
    if u2 <= 0.0:
        u2 = 1.0
    # density 标定：1/(fs·Σw²)；先累加功率再统一乘，避免逐帧乘法
    scale = 1.0 / (fs * u2)
    n_freqs = nperseg // 2 + 1
    use_median = str(average or "mean").strip().lower() == "median"
    if detrend_plan is _DETREND_PLAN_UNSET:
        plan = _provider_detrend_plan(provider, total, detrend, block_samples, cancel_hook)
    else:
        plan = detrend_plan
    frames_buf = np.empty((n_frames, n_freqs), dtype=np.float32) if use_median else None
    accum = None if use_median else np.zeros(n_freqs, dtype=np.float64)
    for k0, segs in _iter_provider_frames(
        provider,
        total,
        nperseg,
        hop,
        block_samples=block_samples,
        cancel_hook=cancel_hook,
        detrend_plan=plan,
    ):
        spec = np.fft.rfft(segs * w, axis=1)
        power = _single_sided_power(spec, nperseg)
        count = power.shape[0]
        if use_median:
            frames_buf[k0 : k0 + count, :] = power
        else:
            accum += power.sum(axis=0)
        del power, spec, segs
    if use_median:
        # 与 scipy.signal.welch(average="median") 同款中位数偏差修正
        bias = _median_bias(n_frames)
        psd = np.empty(n_freqs, dtype=np.float64)
        for i in range(n_freqs):
            psd[i] = np.median(frames_buf[:, i]) / bias
        del frames_buf
    else:
        psd = accum / float(n_frames)
    psd = psd * scale
    freqs = np.fft.rfftfreq(nperseg, d=1.0 / fs)
    return freqs, psd
