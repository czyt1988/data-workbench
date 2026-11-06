"""导出编排：峰值表 → DataFrame/CSV、频谱 CSV、热图矩阵 CSV。

C++ 侧只负责取文件名与状态提示；表头文本由参数传入（保持英文列名，便于送表格页）。

CSV 统一用 utf-8-sig 写盘（带 BOM），便于 Excel 等表格软件直接打开中文内容（C10/D2）。
"""

import numpy as np

# 峰值表列名（英文，§10.2）
PEAK_COLUMNS = ["Frequency (Hz)", "Amplitude", "Rel. peak (dB)", "Share (%)"]


def peaks_to_dataframe(frequencies, amplitudes, rel_db, share, amplitude_label="Amplitude"):
    """峰值表 → pandas.DataFrame（送表格页用）。"""
    import pandas as pd

    return pd.DataFrame(
        {
            PEAK_COLUMNS[0]: np.asarray(frequencies, dtype=np.float64),
            str(amplitude_label): np.asarray(amplitudes, dtype=np.float64),
            PEAK_COLUMNS[2]: np.asarray(rel_db, dtype=np.float64),
            PEAK_COLUMNS[3]: np.asarray(share, dtype=np.float64),
        }
    )


def write_peaks_csv(path, frequencies, amplitudes, rel_db, share, amplitude_label="Amplitude"):
    """峰值表 → CSV（列同上；amp 口径由调用方换算后传入，列名随之变化）。"""
    df = peaks_to_dataframe(frequencies, amplitudes, rel_db, share, amplitude_label)
    df.to_csv(path, index=False, encoding="utf-8-sig")
    return True


def write_spectrum_csv(path, frequencies, values, y_label="Amplitude"):
    """频谱曲线 → CSV（两列：频率、当前幅值模式下的幅值）。"""
    import pandas as pd

    df = pd.DataFrame(
        {
            "Frequency (Hz)": np.asarray(frequencies, dtype=np.float64),
            str(y_label): np.asarray(values, dtype=np.float64),
        }
    )
    df.to_csv(path, index=False, encoding="utf-8-sig")
    return True


def write_heatmap_csv(path, times, freqs, values, rows, columns):
    """热图矩阵 → CSV（行 = 频率、列 = 时间）。"""
    import pandas as pd

    matrix = np.asarray(values, dtype=np.float64).reshape(int(rows), int(columns))
    df = pd.DataFrame(matrix, columns=np.asarray(times, dtype=np.float64))
    df.insert(0, "Frequency (Hz)", np.asarray(freqs, dtype=np.float64))
    df.to_csv(path, index=False, encoding="utf-8-sig")
    return True
