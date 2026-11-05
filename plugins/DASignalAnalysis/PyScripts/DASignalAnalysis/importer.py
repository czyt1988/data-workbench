"""音频/视频导入与分块取数。

职责边界（方案 §5）：
- soundfile 直读 WAV / AIFF / FLAC / OGG；
- MP3 / M4A / AAC 与视频文件先经系统 FFmpeg 转临时 WAV（pcm_f32le），再按文件来源处理；
- 所有读取返回 float32 单精度，按通道取数，供 C++ 侧 DASignalSource::readRange 调用。

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


def _ext(path):
    return Path(str(path)).suffix.lower()


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
    try:
        import soundfile as sf

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


def read_range(path, channel, start, count):
    """按通道读取一段采样，返回 float32 一维数组。

    - start / count 以采样点为单位（每通道）；
    - 越界部分返回实际可读长度（可能短于 count），完全越界返回空数组。
    """
    import soundfile as sf

    start = max(0, int(start))
    count = max(0, int(count))
    if count == 0:
        return np.zeros(0, dtype=np.float32)
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
    """
    import soundfile as sf

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
    """从 DataFrame 取一列为一维 numpy 数组（保留 NaN，由调用方处理）"""
    series = dataframe[column_name]
    return np.asarray(series.to_numpy(), dtype=np.float64)


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
    """
    import soundfile as sf

    values = np.asarray(dataframe[column_name].to_numpy(), dtype=np.float32)
    values = np.nan_to_num(values, nan=0.0, posinf=0.0, neginf=0.0)
    sf.write(str(path), values, int(round(float(sample_rate))), subtype="FLOAT")
    return True


def dataframe_columns(dataframe):
    """返回 DataFrame 的列名列表（字符串）"""
    return [str(c) for c in dataframe.columns]
