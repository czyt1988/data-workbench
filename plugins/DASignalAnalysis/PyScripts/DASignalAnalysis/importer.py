"""音频/视频导入与分块取数。

职责边界（方案 §5）：
- soundfile 直读 WAV / AIFF / FLAC / OGG；
- MP3 / M4A / AAC 与视频文件先经系统 FFmpeg 转临时 WAV（pcm_f32le），再按文件来源处理；
- 所有读取返回 float32 单通道数据，按块读取/按帧 seek，供包络/频谱/STFT 编排调用。

运行时依赖降级（方案 D3 / 评审 I1）：
- 若运行时没有 soundfile（含 libsndfile DLL 缺失抛 OSError，评审 N-I1b），
  WAV 走标准库实现的 RIFF 解析 + 按帧 seek（只读所需帧，不整文件读入，评审 N-I1a），
  整数 PCM 归一化到 [-1, 1) 的 float32，IEEE float WAV 直接转 float32；
- 其它格式返回明确错误信息；soundfile_available() 供 C++ 侧做依赖自检；
- 读取失败抛 RuntimeError（不静默返回空数组，评审 N-I1c），只有「越界/通道不存在」
  这类合法空结果才返回空数组。

本模块不导入 GUI、不读宿主配置，保持纯函数以便 pytest 与直接调用。
"""

import os
import struct
import subprocess
import threading
import time
import traceback
from pathlib import Path

import numpy as np

from DAWorkbench.DAPyBase import thread_status_manager as tsm

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

# 每次读取的帧数上限（只限制临时内存，不影响结果；评审 N-C1b）
_IO_BLOCK_FRAMES = 1 << 20

_WAV_FORMAT_PCM = 1
_WAV_FORMAT_FLOAT = 3
_WAV_FORMAT_EXTENSIBLE = 0xFFFE

# 最近一次 soundfile 导入失败的原因（libsndfile DLL 缺失时是 OSError，评审 N-I1b）
_SOUNDFILE_ERROR = ""

# task_id -> 结果 dict（异步导出任务用，大数组不进 status，避免轮询时反复拷贝）
_RESULT_STORE = {}


def _ext(path):
    return Path(str(path)).suffix.lower()


def _soundfile_missing_message():
    """缺 soundfile / libsndfile DLL 时的统一错误信息（说明可能是 DLL 缺失，N-I1b）。"""
    if _SOUNDFILE_ERROR:
        return "%s (%s)" % (SOUNDFILE_MISSING_MESSAGE, _SOUNDFILE_ERROR)
    return SOUNDFILE_MISSING_MESSAGE


def _load_soundfile():
    """惰性导入 soundfile；缺失或 libsndfile DLL 缺失时返回 None。

    Windows 上 DLL 缺失抛的是 OSError 而不是 ImportError，两者都要降级（N-I1b）。
    """
    global _SOUNDFILE_ERROR
    try:
        import soundfile as sf

        _SOUNDFILE_ERROR = ""
        return sf
    except (ImportError, OSError) as e:  # noqa: BLE001 - 缺包/缺 DLL 都走降级
        _SOUNDFILE_ERROR = str(e) or e.__class__.__name__
        return None


def soundfile_available():
    """运行时是否可用 soundfile（供 C++ 侧依赖自检 / 界面提示）"""
    return _load_soundfile() is not None


# ---------------------------------------------------------------- WAV 兜底（无 soundfile）


def _parse_wav_header(path):
    """只读 WAV 文件头，返回元信息（不读音频数据，N-I1a）。

    :return: dict{ok, sample_rate, channels, frames, format_tag, bits, block_align, data_offset, error}
    """
    result = {
        "ok": False,
        "sample_rate": 0.0,
        "channels": 0,
        "frames": 0,
        "format_tag": 0,
        "bits": 0,
        "block_align": 0,
        "data_offset": 0,
        "error": "",
    }
    fmt = None
    data_offset = -1
    data_size = 0
    try:
        with open(str(path), "rb") as f:
            header = f.read(12)
            if len(header) < 12 or header[0:4] != b"RIFF" or header[8:12] != b"WAVE":
                result["error"] = "not a RIFF/WAVE file"
                return result
            while fmt is None or data_offset < 0:
                chunk = f.read(8)
                if len(chunk) < 8:
                    break
                chunk_id = chunk[0:4]
                chunk_size = struct.unpack("<I", chunk[4:8])[0]
                padded = chunk_size + (chunk_size & 1)
                if chunk_id == b"fmt ":
                    body = f.read(chunk_size)
                    if len(body) < 16:
                        result["error"] = "WAV fmt chunk is truncated"
                        return result
                    tag, channels, rate, _byte_rate, block_align, bits = struct.unpack("<HHIIHH", body[:16])
                    if tag == _WAV_FORMAT_EXTENSIBLE and len(body) >= 40:
                        # WAVE_FORMAT_EXTENSIBLE：真实格式在 SubFormat GUID 前 2 字节
                        tag = struct.unpack("<H", body[24:26])[0]
                    fmt = (tag, int(channels), int(rate), int(block_align), int(bits))
                    if padded > chunk_size:
                        f.read(1)  # 奇数长度 chunk 的填充字节
                elif chunk_id == b"data":
                    data_offset = f.tell()
                    data_size = chunk_size
                    f.seek(padded, os.SEEK_CUR)
                else:
                    f.seek(padded, os.SEEK_CUR)
    except OSError as e:
        result["error"] = str(e)
        return result
    if fmt is None:
        result["error"] = "WAV file has no fmt chunk"
        return result
    if data_offset < 0:
        result["error"] = "WAV file has no data chunk"
        return result
    tag, channels, rate, block_align, bits = fmt
    if channels <= 0 or rate <= 0:
        result["error"] = "WAV header has invalid channel count or sample rate"
        return result
    if block_align <= 0:
        block_align = channels * ((bits + 7) // 8)
    frames = int(data_size) // int(block_align) if block_align > 0 else 0
    result.update(
        {
            "ok": True,
            "sample_rate": float(rate),
            "channels": channels,
            "frames": frames,
            "format_tag": tag,
            "bits": bits,
            "block_align": block_align,
            "data_offset": data_offset,
        }
    )
    return result


def _wav_bytes_to_float32(raw, format_tag, bits):
    """WAV 原始交错字节 → float32 数组（归一化口径与 scipy.io.wavfile 兜底一致）。"""
    if format_tag == _WAV_FORMAT_FLOAT:
        if bits == 32:
            return np.frombuffer(raw, dtype="<f4").astype(np.float32, copy=True)
        if bits == 64:
            return np.frombuffer(raw, dtype="<f8").astype(np.float32)
        raise ValueError("unsupported float WAV bit depth: %s" % bits)
    if format_tag == _WAV_FORMAT_PCM:
        if bits == 8:
            # 无符号 8 bit 以 128 为零点
            return (np.frombuffer(raw, dtype=np.uint8).astype(np.float32) - 128.0) / 128.0
        if bits == 16:
            return np.frombuffer(raw, dtype="<i2").astype(np.float32) / 32768.0
        if bits == 24:
            b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
            v = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
            v = np.where(v >= (1 << 23), v - (1 << 24), v)
            return v.astype(np.float32) / float(1 << 23)
        if bits == 32:
            return np.frombuffer(raw, dtype="<i4").astype(np.float32) / 2147483648.0
        raise ValueError("unsupported PCM WAV bit depth: %s" % bits)
    raise ValueError("unsupported WAV format tag: %s" % format_tag)


def _read_wav_channel_range(path, channel, start, count):
    """无 soundfile 时按帧 seek 读取 WAV 单通道一段（N-I1a）。"""
    info = _parse_wav_header(path)
    if not info["ok"]:
        raise RuntimeError("cannot read WAV file: %s" % info["error"])
    channels = info["channels"]
    if channel < 0 or channel >= channels:
        return np.zeros(0, dtype=np.float32)
    total = int(info["frames"])
    if start >= total or count <= 0:
        return np.zeros(0, dtype=np.float32)
    n = min(int(count), total - start)
    block = int(info["block_align"])
    with open(str(path), "rb") as f:
        f.seek(int(info["data_offset"]) + int(start) * block)
        raw = f.read(n * block)
    if len(raw) != n * block:
        raise RuntimeError("WAV file ended unexpectedly while reading samples")
    frames = _wav_bytes_to_float32(raw, info["format_tag"], info["bits"]).reshape(-1, channels)
    return np.ascontiguousarray(frames[:, channel])


def _read_wav_channel_all(path, channel):
    """无 soundfile 时按块读取 WAV 单通道全部采样（float32，N-I1a/N-C1b）。"""
    info = _parse_wav_header(path)
    if not info["ok"]:
        raise RuntimeError("cannot read WAV file: %s" % info["error"])
    channels = info["channels"]
    if channel < 0 or channel >= channels:
        return np.zeros(0, dtype=np.float32)
    total = int(info["frames"])
    if total <= 0:
        return np.zeros(0, dtype=np.float32)
    block = int(info["block_align"])
    out = np.empty(total, dtype=np.float32)
    pos = 0
    with open(str(path), "rb") as f:
        f.seek(int(info["data_offset"]))
        while pos < total:
            want = min(_IO_BLOCK_FRAMES, total - pos)
            raw = f.read(want * block)
            if len(raw) != want * block:
                raise RuntimeError("WAV file ended unexpectedly while reading samples")
            frames = _wav_bytes_to_float32(raw, info["format_tag"], info["bits"]).reshape(-1, channels)
            out[pos : pos + want] = frames[:, channel]
            pos += want
    return out


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
    直读格式用 soundfile 只读文件头；需要 FFmpeg 的格式由调用方先转码再探测。
    运行时缺 soundfile 时：WAV 用标准库解析 RIFF 头（不读音频数据，N-I1a），
    其它格式返回明确错误（I1）。
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
            return result
        except Exception as e:  # noqa: BLE001 - 探测失败要把原因带回界面
            if _ext(path) != ".wav":
                result["error"] = str(e)
                return result
            # WAV：soundfile 读头失败时仍可用标准库解析头（不读音频数据）
            header = _parse_wav_header(path)
            if not header["ok"]:
                result["error"] = "%s (%s)" % (str(e), header["error"])
                return result
            return _probe_result_from_wav_header(result, header)
    # soundfile 缺失：WAV 用标准库解析头，其它格式明确报错（N-I1b 附带缺失原因）
    if _ext(path) != ".wav":
        result["error"] = _soundfile_missing_message()
        return result
    header = _parse_wav_header(path)
    if not header["ok"]:
        result["error"] = "%s (%s)" % (_soundfile_missing_message(), header["error"])
        return result
    return _probe_result_from_wav_header(result, header)


def _probe_result_from_wav_header(result, header):
    """把标准库解析出的 WAV 头信息填进 probe_audio 的结果字典。"""
    result["ok"] = True
    result["sample_rate"] = float(header["sample_rate"])
    result["channels"] = int(header["channels"])
    result["frames"] = int(header["frames"])
    if header["sample_rate"] > 0:
        result["duration"] = float(header["frames"]) / float(header["sample_rate"])
    return result


def _read_range_soundfile(sf, path, channel, start, count):
    """soundfile 路径：按块读取一段并只保留目标通道（N-C1b/N-I1a）。

    返回 float32 一维数组；突发内存是「块 × 通道数 + 目标段单通道」，
    而不是「整段 × 通道数」。
    """
    with sf.SoundFile(str(path)) as f:
        if channel < 0 or channel >= f.channels:
            return np.zeros(0, dtype=np.float32)
        if start >= f.frames:
            return np.zeros(0, dtype=np.float32)
        n = min(int(count), int(f.frames) - int(start))
        if n <= 0:
            return np.zeros(0, dtype=np.float32)
        out = np.empty(n, dtype=np.float32)
        f.seek(int(start))
        pos = 0
        while pos < n:
            want = min(_IO_BLOCK_FRAMES, n - pos)
            block = f.read(frames=want, dtype="float32", always_2d=True)
            got = int(block.shape[0])
            if got <= 0:
                raise RuntimeError("audio file ended unexpectedly while reading samples")
            out[pos : pos + got] = block[:, channel]
            pos += got
        return out


def read_range(path, channel, start, count):
    """按通道读取一段采样，返回 float32 一维数组。

    - start / count 以采样点为单位（每通道）；
    - 越界部分返回实际可读长度（可能短于 count），完全越界/通道不存在返回空数组；
    - soundfile 路径按块读取（N-I1a/N-C1b）；运行时缺 soundfile 时 WAV 用标准库
      按帧 seek（只读所需帧），其它格式抛明确错误（I1）；
    - 文件打不开/数据损坏抛 RuntimeError，不再静默返回空数组（N-I1c）。
    """
    start = max(0, int(start))
    count = max(0, int(count))
    if count == 0:
        return np.zeros(0, dtype=np.float32)
    sf = _load_soundfile()
    if sf is None:
        if _ext(path) != ".wav":
            raise RuntimeError(_soundfile_missing_message())
        return _read_wav_channel_range(path, channel, start, count)
    try:
        return _read_range_soundfile(sf, path, channel, start, count)
    except Exception as e:  # noqa: BLE001 - 读失败必须上报（N-I1c）
        raise RuntimeError("cannot read audio file: %s" % e)


def _read_channel_all_soundfile(sf, path, channel):
    """soundfile 路径：按块读取并只保留目标通道，返回 float32 一维数组（N-C1b）。

    逐块读取（每块最多 _IO_BLOCK_FRAMES 帧、所有通道）后立即取单列写入预分配数组，
    避免「读入全部通道的整段数据再取一列」的峰值内存。
    """
    with sf.SoundFile(str(path)) as f:
        if channel < 0 or channel >= f.channels:
            return np.zeros(0, dtype=np.float32)
        total = int(f.frames)
        if total <= 0:
            return np.zeros(0, dtype=np.float32)
        out = np.empty(total, dtype=np.float32)
        pos = 0
        while pos < total:
            want = min(_IO_BLOCK_FRAMES, total - pos)
            block = f.read(frames=want, dtype="float32", always_2d=True)
            got = int(block.shape[0])
            if got <= 0:
                raise RuntimeError("audio file ended unexpectedly while reading samples")
            out[pos : pos + got] = block[:, channel]
            pos += got
        if pos != total:
            raise RuntimeError("audio file ended unexpectedly while reading samples")
    return out


def read_channel_all(path, channel):
    """读取某通道的全部采样（float32 单通道，按块读取）。

    仅供后台任务使用（包络/频谱/STFT 编排），界面线程禁止调用整段读取。
    运行时缺 soundfile 时：WAV 用标准库按块解析，其它格式抛明确错误（I1）；
    读取失败抛 RuntimeError（N-I1c）。
    """
    sf = _load_soundfile()
    if sf is None:
        if _ext(path) != ".wav":
            raise RuntimeError(_soundfile_missing_message())
        return _read_wav_channel_all(path, channel)
    try:
        return _read_channel_all_soundfile(sf, path, channel)
    except RuntimeError:
        raise
    except Exception as e:  # noqa: BLE001 - 读失败必须上报（N-I1c）
        raise RuntimeError("cannot read audio file: %s" % e)


# ---------------------------------------------------------------- FFmpeg


def run_ffmpeg(ffmpeg_path, input_path, output_path, extra_args=None, cancel_check=None):
    """运行 FFmpeg 把输入转成 32 位浮点 WAV（或执行额外参数）。

    :param cancel_check: 可选无参回调；返回 True 表示请求取消，会 kill 子进程并返回
                         returncode=-3（I2：导入异步化后转码必须可取消）
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
        proc = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except FileNotFoundError:
        return {"ok": False, "returncode": -1, "message": "ffmpeg executable not found"}

    # 用轮询代替 subprocess.run：转码是长任务，必须在等待期间响应取消（I2）。
    deadline = time.time() + 1800
    stderr_text = ""
    while True:
        try:
            _out, stderr_text = proc.communicate(timeout=0.2)
            break
        except subprocess.TimeoutExpired:
            if cancel_check is not None and cancel_check():
                proc.kill()
                proc.communicate()
                return {"ok": False, "returncode": -3, "message": "cancelled"}
            if time.time() > deadline:
                proc.kill()
                proc.communicate()
                return {"ok": False, "returncode": -2, "message": "ffmpeg timed out"}
    if proc.returncode != 0:
        tail = "\n".join((stderr_text or "").strip().splitlines()[-10:])
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


def export_column_to_wav(dataframe, column_name, path, sample_rate, cancel_check=None):
    """把数据帧某列导出为 32 位浮点 WAV（数据帧来源播放前调用，T8 / U9）。

    NaN 按 0 处理；返回 True 表示写盘成功。
    运行时缺 soundfile 时用 scipy.io.wavfile 写 float32 WAV（I1）。

    :param cancel_check: 可选无参回调；回调抛异常即中止导出（供后台任务取消）
    """
    if cancel_check is not None:
        cancel_check()
    values = np.asarray(dataframe[column_name].to_numpy(), dtype=np.float32)
    values = np.nan_to_num(values, nan=0.0, posinf=0.0, neginf=0.0)
    rate = int(round(float(sample_rate)))
    sf = _load_soundfile()
    if sf is not None:
        if cancel_check is not None:
            cancel_check()
        sf.write(str(path), values, rate, subtype="FLOAT")
    else:
        from scipy.io import wavfile

        if cancel_check is not None:
            cancel_check()
        wavfile.write(str(path), rate, values.astype(np.float32))
    return True


def dataframe_columns(dataframe):
    """返回 DataFrame 的列名列表（字符串）"""
    return [str(c) for c in dataframe.columns]


# ---------------------------------------------------------------- 数据帧 WAV 后台导出（U9）


def _lookup_dataframe(source):
    """按来源描述取宿主数据帧（da_app 是宿主注入的顶层模块）。"""
    import da_app

    datamanager = da_app.getCore().getDataManagerInterface()
    frames = datamanager.getAllDataframes()
    df = frames.get(source.get("dataframeName", ""))
    if df is None:
        raise RuntimeError("dataframe not found: %s" % source.get("dataframeName", ""))
    return df


def _safe_remove(path):
    """删除临时文件（失败忽略，清理路径用）。"""
    try:
        if path and os.path.exists(str(path)):
            os.remove(str(path))
    except OSError:
        pass


def prepare_import_async(path, mode, ffmpeg_path, temp_wav_path):
    """后台准备导入（I2：视频/压缩音频导入异步化）。

    界面线程只负责选文件与结果接线；本函数在 Python 守护线程里做耗时的
    「探测音频轨 / FFmpeg 转码 / 结果文件可读性校验」，进度与取消走
    thread_status_manager（C++ 侧 QTimer 轮询，不新起 QThread）。

    :param path: 用户选择的源文件
    :param mode: "audio" 或 "video"
    :param ffmpeg_path: FFmpeg 可执行文件路径（空串表示不可用）
    :param temp_wav_path: C++ 会话临时目录里预分配的目标 WAV（仅转码时使用）
    :return: 任务 id
    结果 dict：{ok, read_path, is_temporary, sample_rate, error}
    - 直读格式（WAV/AIFF/FLAC/OGG 且 soundfile 可用）：read_path 仍是源文件；
    - 视频 / 压缩音频 / 无 soundfile 的非 WAV：read_path 是转码出的临时 WAV。
    """
    task_id, status = tsm.create_task_with_status("signal import")
    status.start("signal import")
    mode = str(mode or "audio").lower()
    temp_wav_path = str(temp_wav_path or "")

    def _check():
        tsm.raise_if_canceled(task_id)

    def _cleanup_temp():
        # 只在「本次生成的临时产物」上删；源文件绝不删
        if temp_wav_path:
            _safe_remove(temp_wav_path)

    def worker():
        is_temporary = False
        try:
            _check()
            if not os.path.isfile(str(path)):
                raise RuntimeError("file does not exist: %s" % path)
            ext = _ext(path)
            needs_ffmpeg_convert = False
            if mode == "video":
                if not ffmpeg_path:
                    raise RuntimeError("ffmpeg is not available for video import")
                status.update_progress(5.0, "probing audio track")
                probe = has_audio_stream(ffmpeg_path, path)
                if not probe.get("ok"):
                    raise RuntimeError(probe.get("message") or "failed to probe the video file")
                if not probe.get("has_audio"):
                    raise RuntimeError("the file does not contain an audio track")
                needs_ffmpeg_convert = True
            elif needs_ffmpeg(path):
                if not ffmpeg_path:
                    raise RuntimeError("ffmpeg is not available for this format")
                needs_ffmpeg_convert = True
            elif is_direct_audio(path) and not soundfile_available() and ext != ".wav":
                # 运行时缺 soundfile：非 WAV 直读格式退化为 FFmpeg（与同步路径口径一致，评审 I1）
                if not ffmpeg_path:
                    raise RuntimeError(SOUNDFILE_MISSING_MESSAGE)
                needs_ffmpeg_convert = True
            if needs_ffmpeg_convert:
                if not temp_wav_path:
                    raise RuntimeError("temporary wav path is empty")
                _check()
                status.update_progress(15.0, "converting audio")

                def _cancel_check():
                    if tsm.is_task_canceled(task_id):
                        return True
                    status.update_progress(50.0, "converting audio")
                    return False

                res = run_ffmpeg(ffmpeg_path, path, temp_wav_path, cancel_check=_cancel_check)
                if not res.get("ok"):
                    if res.get("returncode") == -3:
                        raise tsm.TaskCanceledError("cancelled")
                    raise RuntimeError(res.get("message") or "ffmpeg failed")
                read_path = temp_wav_path
                is_temporary = True
            else:
                read_path = str(path)
            _check()
            status.update_progress(90.0, "verifying")
            probe = probe_audio(read_path)
            if not probe.get("ok"):
                raise RuntimeError(probe.get("error") or "cannot read the imported audio file")
            _check()
            _RESULT_STORE[task_id] = {
                "ok": True,
                "read_path": read_path,
                "is_temporary": bool(is_temporary),
                "sample_rate": float(probe.get("sample_rate", 0.0) or 0.0),
            }
            if tsm.is_task_canceled(task_id):
                _RESULT_STORE.pop(task_id, None)
                _cleanup_temp()
                return
            status.update_progress(100.0, "done")
            status.finish(True, "")
        except tsm.TaskCanceledError:
            _cleanup_temp()
            status.finish(False, "cancelled")
        except Exception as e:  # noqa: BLE001 - 失败原因必须回传界面
            _cleanup_temp()
            if tsm.is_task_canceled(task_id):
                status.finish(False, "cancelled")
                return
            status.finish(False, "%s\n%s" % (str(e), traceback.format_exc()))

    threading.Thread(target=worker, daemon=True).start()
    return task_id


def export_playback_wav_async(source, out_path):
    """后台把数据帧某列导出为临时 WAV（U9：数据帧来源播放前准备）。

    走 Python 守护线程 + thread_status_manager（与其它分析任务同一并发约定，
    不新起 QThread），C++ 侧用现有 QTimer 轮询状态并 take_result 取结果。

    :param source: 来源描述（type=dataframe/dataframeName/column/sampleRate）
    :param out_path: 目标临时 WAV 绝对路径（由 C++ 会话临时目录生成）
    :return: 任务 id（字符串）
    """
    task_id, status = tsm.create_task_with_status("signal playback wav")
    status.start("signal playback wav")

    def worker():
        try:
            if str(source.get("type", "file")) != "dataframe":
                raise RuntimeError("playback WAV export requires a dataframe source")
            sample_rate = float(source.get("sampleRate", 0.0) or 0.0)
            if sample_rate <= 0.0:
                raise RuntimeError("invalid sample rate")
            status.update_progress(10.0, "reading samples")
            tsm.raise_if_canceled(task_id)
            df = _lookup_dataframe(source)
            column = str(source.get("column", ""))
            status.update_progress(40.0, "writing wav")

            def _check():
                tsm.raise_if_canceled(task_id)

            export_column_to_wav(df, column, out_path, sample_rate, cancel_check=_check)
            if tsm.is_task_canceled(task_id):
                # 取消后删半成品、不写结果表（C7）
                _safe_remove(out_path)
                return
            _RESULT_STORE[task_id] = {
                "path": str(out_path),
                "sample_rate": sample_rate,
            }
            if tsm.is_task_canceled(task_id):
                # 写结果表与取消的竞态：取消后自行清理，避免结果滞留
                _RESULT_STORE.pop(task_id, None)
                _safe_remove(out_path)
                return
            status.update_progress(95.0, "finalizing")
            status.finish(True, "")
        except tsm.TaskCanceledError:
            _safe_remove(out_path)
            status.finish(False, "cancelled")
        except Exception as e:  # noqa: BLE001 - 失败原因必须回传界面
            _safe_remove(out_path)
            if tsm.is_task_canceled(task_id):
                status.finish(False, "cancelled")
                return
            status.finish(False, "%s\n%s" % (str(e), traceback.format_exc()))

    threading.Thread(target=worker, daemon=True).start()
    return task_id


def take_result(task_id):
    """取走某异步导出任务的结果（取走后从结果表删除）。"""
    return _RESULT_STORE.pop(task_id, None)


def cancel_task(task_id):
    """请求取消异步导出任务（线程在下一个检查点退出并删除半成品）。"""
    return tsm.cancel_task(task_id, "cancelled by user")


def is_cancelled(task_id):
    """任务是否已被取消。"""
    return tsm.is_task_canceled(task_id)
