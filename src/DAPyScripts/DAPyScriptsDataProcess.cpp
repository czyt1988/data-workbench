#include "DAPyScriptsDataProcess.h"
#include "DAPybind11QtCaster.hpp"
#include "DALogCategory.h"

/**
 * @def DATAPROCESS_CALL_DF
 * @brief 生成签名形如 (wave, fs, args, err) -> DAPyDataFrame 的数据处理方法
 *
 * 适用于 spectrum_analysis / peak_analysis / wavelet_dwt 三个签名一致的方法。
 * butterworth_filter / stft_analysis / wavelet_cwt 因签名不同，保持显式实现。
 */
#define DATAPROCESS_CALL_DF(functionName, pyFunctionName)                                                       \
    DAPyDataFrame DAPyScriptsDataProcess::functionName(                                                         \
        const DAPySeries& wave, double fs, const QVariantMap& args, QString* err)                               \
    {                                                                                                           \
        try {                                                                                                   \
            pybind11::object fn = attr(#pyFunctionName);                                                        \
            if (fn.is_none()) {                                                                                 \
                qDebug() << "DAWorkbench.data_processing.py have no attr " #pyFunctionName;                     \
                return DAPyDataFrame();                                                                         \
            }                                                                                                   \
            pybind11::object v = fn(wave.object(), fs, pybind11::cast(args));                                   \
            return DAPyDataFrame(std::move(v));                                                                 \
        } catch (const std::exception& e) {                                                                     \
            if (err) {                                                                                          \
                *err = e.what();                                                                                \
            }                                                                                                   \
            qDebug() << e.what();                                                                               \
        }                                                                                                       \
        return DAPyDataFrame();                                                                                 \
    }

namespace DA
{

/** @brief 构造DAPyScriptsDataProcess @param autoImport 是否自动导入模块 */
DAPyScriptsDataProcess::DAPyScriptsDataProcess(bool autoImport) : DAPyModule()
{
    if (autoImport) {
        if (!import()) {
            daCritical << QObject::tr("cannot import da_data_processing module");  // cn:无法导入 da_data_processing 模块
        }
    }
}

/** @brief 构造DAPyScriptsDataProcess @param obj Python模块对象 */
DAPyScriptsDataProcess::DAPyScriptsDataProcess(const pybind11::object& obj) : DAPyModule(obj)
{
    if (!isModule()) {
        daCritical << QObject::tr(
            "cannot import DAWorkbench.data_processing");  // cn:无法导入 DAWorkbench.data_processing 模块
    }
}

/** @brief 析构DAPyScriptsDataProcess */
DAPyScriptsDataProcess::~DAPyScriptsDataProcess()
{
}

/**
 * @brief 映射 da_data_processing.da_spectrum_analysis
 * @param wave
 * @param fs
 * @param args
 * @param err
 * @return
 */
DATAPROCESS_CALL_DF(spectrum_analysis, da_spectrum_analysis)

/** @brief 巴特沃斯滤波 @param wave 输入信号 @param fs 采样率 @param fo 滤波器阶数 @param args 滤波参数 @param err 错误信息输出 @return 滤波后的数据 */
DAPyDataFrame
DAPyScriptsDataProcess::butterworth_filter(const DAPySeries& wave, double fs, int fo, const QVariantMap& args, QString* err)
{
    try {
        pybind11::object fn = attr("da_butterworth_filter");
        if (fn.is_none()) {
            qDebug() << "DAWorkbench.data_processing.py have no attr da_butterworth_filter";
            return DAPyDataFrame();
        }
        pybind11::object v = fn(wave.object(), fs, fo, pybind11::cast(args));
        return DAPyDataFrame(std::move(v));
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return DAPyDataFrame();
}

DATAPROCESS_CALL_DF(peak_analysis, da_peak_analysis)

/** @brief 短时傅里叶变换分析 @param wave 输入信号 @param fs 采样率 @param args 分析参数 @param err 错误信息输出 @return 包含频率、时间、幅值信息的字典 */
pybind11::dict DAPyScriptsDataProcess::stft_analysis(const DAPySeries& wave, double fs, const QVariantMap& args, QString* err)
{
    try {
        pybind11::object fn = attr("da_stft_analysis");
        if (fn.is_none()) {
            qDebug() << "DAWorkbench.data_processing.py have no attr da_stft_analysis";
            return pybind11::dict();
        }
        pybind11::object v = fn(wave.object(), fs, pybind11::cast(args));
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

/** @brief 连续小波变换 @param wave 输入信号 @param fs 采样率 @param scales 尺度序列 @param args 变换参数 @param err 错误信息输出 @return 包含系数、频率等信息的小波变换结果字典 */
pybind11::dict DAPyScriptsDataProcess::wavelet_cwt(const DAPySeries& wave,
                                                   double fs,
                                                   const DA::DAPySeries& scales,
                                                   const QVariantMap& args,
                                                   QString* err)
{
    try {
        pybind11::object fn = attr("da_wavelet_cwt");
        if (fn.is_none()) {
            qDebug() << "DAWorkbench.data_processing.py have no attr da_wavelet_cwt";
            return pybind11::dict();
        }
        pybind11::object v = fn(wave.object(), fs, scales.object(), pybind11::cast(args));
        return v.cast< pybind11::dict >();
    } catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        qDebug() << e.what();
    }
    return pybind11::dict();
}

DATAPROCESS_CALL_DF(wavelet_dwt, da_wavelet_dwt)

/** @brief 导入DAWorkbench.data_processing模块 @return 导入成功返回true */
bool DAPyScriptsDataProcess::import()
{
    try {
        pybind11::module m = pybind11::module::import("DAWorkbench");
        object()           = m.attr("data_processing");
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return false;
    }
    return true;
}

}  // end DA
