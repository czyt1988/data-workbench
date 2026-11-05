#include "DASignalAnalysisPlugin.h"
#include <QDebug>
#include "DACoreInterface.h"
#include "DASignalAnalysisUI.h"

namespace DA
{
DASignalAnalysisPlugin::DASignalAnalysisPlugin()
{
}

DASignalAnalysisPlugin::~DASignalAnalysisPlugin()
{
    if (mUI) {
        mUI->finalize();
        delete mUI;
        mUI = nullptr;
    }
}

/**
 * @brief 初始化插件：创建界面（中央页面 + 丝带标签页）
 * @return 初始化是否成功
 */
bool DASignalAnalysisPlugin::initialize()
{
    if (nullptr == core()) {
        qWarning() << "DASignalAnalysisPlugin::initialize: core interface is null";
        return false;
    }
    mUI = new DASignalAnalysisUI(this);
    if (!mUI->initialize(core())) {
        qWarning() << "DASignalAnalysisPlugin::initialize: UI initialization failed";
        delete mUI;
        mUI = nullptr;
        return false;
    }
    return DAAbstractPlugin::initialize();
}

/**
 * @brief 卸载清理：移除界面资源（插件热卸载前调用）
 * @return 卸载是否成功
 */
bool DASignalAnalysisPlugin::finalize()
{
    if (mUI) {
        mUI->finalize();
        delete mUI;
        mUI = nullptr;
    }
    return DAAbstractPlugin::finalize();
}

/**
 * @brief 插件id
 */
QString DASignalAnalysisPlugin::getIID() const
{
    return DAABSTRACTPLUGIN_IID;
}

/**
 * @brief 插件名
 */
QString DASignalAnalysisPlugin::getName() const
{
    return QStringLiteral("DASignalAnalysis");
}

/**
 * @brief 插件版本
 */
QString DASignalAnalysisPlugin::getVersion() const
{
    return QStringLiteral("0.1.0");
}

/**
 * @brief 插件描述
 */
QString DASignalAnalysisPlugin::getDescription() const
{
    return tr("General purpose time-series signal analysis (spectrum, spectrogram, peaks, playback)");
    // cn:通用时序信号分析（频谱、时频图、峰值、播放）
}

/**
 * @brief 工程存档任务：保存来源引用/参数/视图状态，加载时重建信号
 */
std::shared_ptr< DA::DAAbstractArchiveTask > DASignalAnalysisPlugin::createArchiveTask(bool isSave)
{
    if (mUI) {
        return mUI->createArchiveTask(isSave);
    }
    return DAAbstractPlugin::createArchiveTask(isSave);
}

/**
 * @brief 语言变更：由宿主 AppMainWindow::retranslateUi 遍历插件调用
 */
void DASignalAnalysisPlugin::retranslate()
{
    if (mUI) {
        mUI->retranslateUi();
    }
}
}  // namespace DA
