"""DASignalAnalysis 插件 Python 入口。

包内模块（C++ 侧按模块名调用）：

- importer：音频/视频探测与分块取数、采样率推断、数据帧列导出 WAV；
- spectrum：整段频谱 / 韦尔奇功率谱编排（分块平均 + 可取消）；
- stft：时频显示栅格（分块计算 + 双向池化）；
- peaks：峰值表与每帧前 N 峰值曲线；
- envelope：波形显示包络降采样；
- exporter：峰值/频谱/热图导出（CSV 统一 utf-8-sig）。
"""

__all__ = ["importer", "spectrum", "stft", "peaks", "envelope", "exporter"]
