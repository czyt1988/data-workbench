"""音频/视频导入与分块取数。

职责边界（方案 §5）：
- soundfile 直读 WAV / AIFF / FLAC / OGG；
- MP3 / M4A / AAC 与视频文件先经系统 FFmpeg 转临时 WAV（pcm_f32le），再按文件来源处理；
- 所有读取返回 float32 单精度，按通道取数，供 C++ 侧 DASignalSource::readRange 调用。

运行时依赖降级（方案 D3 / 评审 I1）：若运行时没有 soundfile，WAV 用 scipy.io.wavfile 兜底
（整数 PCM 归一化到 [-1, 1) 的 float32），其它格式返回明确错误信息；
soundfile_available() 供 C++ 侧做依赖自检，避免「部署缺包」再次表现为静默失败。

本模块不导入 GUI、不读宿主配置，保持纯函数以便 pytest 与直接调用。
"""

import os
import subprocess
from pathlib import Path

import numpy as np

# 直读格式（libsndfile 支持）
AUDIO_EXT_DIRECT = {".wav", ".aiff", ".aif", ".flac", ".ogg", ".oga"}
# 需要 FFmpeg 转码的音频格式
AUDIO_EXT_FFMPEG = {".mp3", ".m4a", ".aac", ".wma", ".opus"}
# 视频格式（抽取音频轨）
VIDEO_EXT = {".mp4", ".mov", ".m4v", ".avi", ".mkv", ".webm", ".flv", ".wmv", ".ts", ".mts"}

AUDIO_DIRECT_FILTER = "*.wav *.aiff *.aif *.flac *.ogg *.oga"
AUDIO_FFMPEG_FILTER = "*.mp3 *.m4a *.aac *.wma *.opus"
VIDEO_FILTER = "*.mp4 *.mov *.m4v *.avi *.mkv *.webm *.flv *.wmv *.ts *.mts"

# soundfile 缺失时的统一错误信息（C++ 侧可直接展示）
SOUNDFILE_MISSING_MESSAGE = "soundfile is not available; install it or use FFmpeg"


def _ext(path):
    return Path(str(path)).suffix.lower()


def _load_soundfile():
    """惰性导入 soundfile；缺失时返回 None（不回退，调用方决定降级）"""
    try:
        import soundfile as sf

        return sf
    except ImportError:
        return None


def soundfile_available():
    """运行时是否可用 soundfile（供 C++ 侧依赖自检 / 界面提示）"""
    return _load_soundfile() is not None


def _read_wav_scipy(path):
    """scipy.io.wavfile 兜底读取 WAV。

    :return: (float32 二维数组 [frames, channels], sample_rate)
    :raises: 读取失败或采样格式不支持时抛异常（由调用方转成明确错误）
    """
    from scipy.io import wavfile

    rate, data = wavfile.read(str(path))
    arr = np.asarray(data)
    if arr.ndim == 1:
        arr = arr[:, None]
    if arr.dtype.kind == "u":
        # 无符号整型 PCM：8 bit 以 128 为零点，其它位宽按满量程归一化
        info = np.iinfo(arr.dtype)
        if info.bits == 8:
            out = (arr.astype(np.float32) - 128.0) / 128.0
        else:
            out = arr.astype(np.float32) / float(1 << (info.bits - 1))
    elif arr.dtype.kind == "i":
        info = np.iinfo(arr.dtype)
        out = arr.astype(np.float32) / float(1 << (info.bits - 1))
    elif arr.dtype.kind == "f":
        out = arr.astype(np.float32, copy=False)
    else:
        raise ValueError("unsupported WAV sample format: %s" % arr.dtype)
    return np.ascontiguousarray(out), int(rate)


def is_direct_audio(path):
    """是否为 soundfile 可直接读取的音频文件"""
    return _ext(path) in AUDIO_EXT_DIRECT


def needs_ffmpeg(path):
    """是否需要 FFmpeg（压缩音频格式或视频）"""
    ext = _ext(path)
    return ext in AUDIO_EXT_FFMPEG or ext in VIDEO_EXT


def is_video(path):
    """是否为视频文件"""
    return _ext(path) in VIDEO_EXT


def probe_audio(path):
    """探测音频文件元数据。

    返回 dict：{ok, is_video, sample_rate, channels, frames, duration, error}
    直读格式用 soundfile 读文件头；需要 FFmpeg 的格式由调用方先转码再探测。
    运行时缺 soundfile 时：WAV 走 scipy.io.wavfile 兜底，其它格式返回明确错误（I1）。
    """
    result = {
        "ok": False,
        "is_video": is_video(path),
        "sample_rate": 0.0,
        "channels": 0,
        "frames": 0,
        "duration": 0.0,
        "error": "",
    }
    sf = _load_soundfile()
    if sf is not None:
        try:
            info = sf.info(str(path))
            result["ok"] = True
            result["sample_rate"] = float(info.samplerate)
            result["channels"] = int(info.channels)
            result["frames"] = int(info.frames)
            if info.samplerate > 0:
                result["duration"] = float(info.frames) / float(info.samplerate)
        except Exception as e:  # noqa: BLE001 - 探测失败要把原因带回界面
            result["error"] = str(e)
        return result
    # soundfile 缺失：WAV 用 scipy.io.wavfile 兜底，其它格式明确报错
    if _ext(path) != ".wav":
        result["error"] = SOUNDFILE_MISSING_MESSAGE
        return result
    try:
        data, rate = _read_wav_scipy(path)
        frames = int(data.shape[0])
        result["ok"] = True
        result["sample_rate"] = float(rate)
        result["channels"] = int(data.shape[1])
        result["frames"] = frames
        if rate > 0:
            result["duration"] = float(frames) / float(rate)
    except Exception as e:  # noqa: BLE001
        result["error"] = "%s (%s)" % (SOUNDFILE_MISSING_MESSAGE, str(e))
    return result


def read_range(path, channel, start, count):
    """按通道读取一段采样，返回 float32 一维数组。

    - start / count 以采样点为单位（每通道）；
    - 越界部分返回实际可读长度（可能短于 count），完全越界返回空数组；
    - 运行时缺 soundfile 时：WAV 用 scipy.io.wavfile 兜底，其它格式抛明确错误（I1）。
    """
    start = max(0, int(start))
    count = max(0, int(count))
    if count == 0:
        return np.zeros(0, dtype=np.float32)
    sf = _load_soundfile()
    if sf is None:
        if _ext(path) != ".wav":
            raise RuntimeError(SOUNDFILE_MISSING_MESSAGE)
        try:
            data, _rate = _read_wav_scipy(path)
        except Exception:  # noqa: BLE001 - 读失败由调用方按空数组处理
            return np.zeros(0, dtype=np.float32)
        if channel < 0 or channel >= data.shape[1]:
            return np.zeros(0, dtype=np.float32)
        if start >= data.shape[0]:
            return np.zeros(0, dtype=np.float32)
        return np.ascontiguousarray(data[start : start + count, channel])
    try:
        with sf.SoundFile(str(path)) as f:
            if channel < 0 or channel >= f.channels:
                return np.zeros(0, dtype=np.float32)
            if start >= f.frames:
                return np.zeros(0, dtype=np.float32)
            f.seek(start)
            data = f.read(frames=count, dtype="float32", always_2d=True)
        if data.size == 0:
            return np.zeros(0, dtype=np.float32)
        return np.ascontiguousarray(data[:, channel])
    except Exception:  # noqa: BLE001 - 读失败由调用方按空数组处理并给出明确错误
        return np.zeros(0, dtype=np.float32)


def read_channel_all(path, channel):
    """读取某通道的全部采样（float32）。

    仅供后台任务使用（包络/频谱/STFT 编排），界面线程禁止调用整段读取。
    运行时缺 soundfile 时：WAV 用 scipy.io.wavfile 兜底，其它格式抛明确错误（I1）。
    """
    sf = _load_soundfile()
    if sf is None:
        if _ext(path) != ".wav":
            raise RuntimeError(SOUNDFILE_MISSING_MESSAGE)
        data, _rate = _read_wav_scipy(path)
        if channel < 0 or channel >= data.shape[1]:
            return np.zeros(0, dtype=np.float32)
        return np.ascontiguousarray(data[:, channel])
    with sf.SoundFile(str(path)) as f:
        if channel < 0 or channel >= f.channels:
            return np.zeros(0, dtype=np.float32)
        data = f.read(dtype="float32", always_2d=True)
    if data.size == 0:
        return np.zeros(0, dtype=np.float32)
    return np.ascontiguousarray(data[:, channel])


# ---------------------------------------------------------------- FFmpeg


def run_ffmpeg(ffmpeg_path, input_path, output_path, extra_args=None):
    """运行 FFmpeg 把输入转成 32 位浮点 WAV（或执行额外参数）。

    返回 dict：{ok, returncode, message}
    message 含失败原因（取最后 10 行输出，方案 §5.3）。
    """
    args = [
        str(ffmpeg_path),
        "-hide_banner",
        "-y",
        "-i",
        str(input_path),
    ]
    if extra_args:
        args.extend([str(a) for a in extra_args])
    else:
        # 默认：丢弃视频轨，转 32 位浮点 PCM WAV
        args.extend(["-vn", "-acodec", "pcm_f32le", str(output_path)])
    try:
        proc = subprocess.run(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=1800,
        )
    except FileNotFoundError:
        return {"ok": False, "returncode": -1, "message": "ffmpeg executable not found"}
    except subprocess.TimeoutExpired:
        return {"ok": False, "returncode": -2, "message": "ffmpeg timed out"}
    if proc.returncode != 0:
        tail = "\n".join((proc.stderr or "").strip().splitlines()[-10:])
        return {"ok": False, "returncode": int(proc.returncode), "message": tail}
    return {"ok": True, "returncode": 0, "message": ""}


def has_audio_stream(ffmpeg_path, input_path):
    """用 FFmpeg 探测文件是否含音频轨。

    返回 dict：{ok, has_audio, message}
    """
    args = [str(ffmpeg_path), "-hide_banner", "-i", str(input_path)]
    try:
        proc = subprocess.run(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=120,
        )
    except FileNotFoundError:
        return {"ok": False, "has_audio": False, "message": "ffmpeg executable not found"}
    except subprocess.TimeoutExpired:
        return {"ok": False, "has_audio": False, "message": "ffmpeg probe timed out"}
    # ffmpeg -i 无输出文件时返回非零码，但 stderr 里有流信息
    text = (proc.stderr or "")
    has_audio = "Audio:" in text
    return {"ok": True, "has_audio": has_audio, "message": "" if has_audio else text.strip()[-500:]}


# ---------------------------------------------------------------- 采样率推断（数据帧）


def infer_sample_rate(time_values):
    """根据时间列推断采样率：fs = 1 / median(diff(t))。

    time_values 为一维时间序列（秒或 datetime64）。返回 dict：{ok, sample_rate, duration, error}。
    - 用中位数抵抗缺失/重复（方案 §4.4）；
    - 少于 2 个有效点或差分全为 0 时返回失败，由界面要求用户手填。
    """
    result = {"ok": False, "sample_rate": 0.0, "duration": 0.0, "error": ""}
    try:
        t = np.asarray(time_values)
        if t.size < 2:
            result["error"] = "time column has fewer than 2 samples"
            return result
        # datetime64 → 秒；数值列按秒处理
        if np.issubdtype(t.dtype, np.datetime64):
            t = t.astype("datetime64[ns]").astype(np.int64) / 1e9
        else:
            t = t.astype(np.float64)
        t = t[np.isfinite(t)]
        if t.size < 2:
            result["error"] = "time column has fewer than 2 finite samples"
            return result
        diffs = np.diff(t)
        diffs = diffs[diffs > 0]
        if diffs.size == 0:
            result["error"] = "time column is not strictly increasing"
            return result
        dt = float(np.median(diffs))
        if dt <= 0:
            result["error"] = "invalid median time step"
            return result
        result["ok"] = True
        result["sample_rate"] = 1.0 / dt
        result["duration"] = float(t[-1] - t[0])
    except Exception as e:  # noqa: BLE001
        result["error"] = str(e)
    return result


def column_as_array(dataframe, column_name):
    """从 DataFrame 取一列为一维 numpy 数组（按 dtype 分派，I5）。

    - datetime64：先转纳秒再转秒（float64），使 infer_sample_rate 能对时间列推断采样率；
    - 数值/布尔：转 float64，保留 NaN 给调用方处理；
    - 其它（含时区感知 datetime、字符串）：转 float64 失败时抛出明确异常。
    """
    values = np.asarray(dataframe[column_name].to_numpy())
    if np.issubdtype(values.dtype, np.datetime64):
        return values.astype("datetime64[ns]").astype(np.int64).astype(np.float64) / 1e9
    return values.astype(np.float64)


def read_dataframe_range(dataframe, column_name, start, count):
    """从 DataFrame 某列读取一段，返回 float32 一维数组（按块转换，不整列复制到 float32）"""
    start = max(0, int(start))
    count = max(0, int(count))
    if count == 0:
        return np.zeros(0, dtype=np.float32)
    series = dataframe[column_name]
    values = np.asarray(series.to_numpy())[start : start + count]
    return np.ascontiguousarray(values, dtype=np.float32)


def infer_sample_rate_from_column(dataframe, column_name):
    """由 DataFrame 的时间列推断采样率（供 C++ 对话框调用）"""
    return infer_sample_rate(column_as_array(dataframe, column_name))


def export_column_to_wav(dataframe, column_name, path, sample_rate):
    """把数据帧某列导出为 32 位浮点 WAV（数据帧来源播放前调用，T8）。

    NaN 按 0 处理；返回 True 表示写盘成功。
    运行时缺 soundfile 时用 scipy.io.wavfile 写 float32 WAV（I1）。
    """
    values = np.asarray(dataframe[column_name].to_numpy(), dtype=np.float32)
    values = np.nan_to_num(values, nan=0.0, posinf=0.0, neginf=0.0)
    rate = int(round(float(sample_rate)))
    sf = _load_soundfile()
    if sf is not None:
        sf.write(str(path), values, rate, subtype="FLOAT")
    else:
        from scipy.io import wavfile

        wavfile.write(str(path), rate, values.astype(np.float32))
    return True


def dataframe_columns(dataframe):
    """返回 DataFrame 的列名列表（字符串）"""
    return [str(c) for c in dataframe.columns]
