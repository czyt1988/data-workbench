#ifndef DASIGNALANALYSISPLUGIN_H
#define DASIGNALANALYSISPLUGIN_H
#include <QObject>
#include "DAAbstractPlugin.h"
#include "DASignalAnalysisGlobal.h"
namespace DA
{
class DAAbstractArchiveTask;
}

namespace DA
{
class DASignalAnalysisUI;
/**
 * @brief 通用时序信号分析插件
 *
 * 定位（方案 D1）：信号对象只认「采样率、通道、时间原点」，音频是一等导入格式但不是唯一格式。
 * 插件在主工作区注册「信号分析」页面（与绘图/表格/工作流并列），并提供独立的丝带标签页。
 */
class DASIGNALANALYSIS_API DASignalAnalysisPlugin : public QObject, public DAAbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DAABSTRACTPLUGIN_IID)
    Q_INTERFACES(DA::DAAbstractPlugin)
public:
    DASignalAnalysisPlugin();
    ~DASignalAnalysisPlugin() override;

    // 初始化：创建 UI（页面 + 丝带标签页）
    bool initialize() override;
    // 卸载清理：移除丝带标签页、动作与页面停靠窗口
    bool finalize() override;

    QString getIID() const override;
    QString getName() const override;
    QString getVersion() const override;
    QString getDescription() const override;

    // 语言变更：由宿主 AppMainWindow::retranslateUi 遍历插件调用
    void retranslate() override;

    // 工程存档任务（只存来源引用与参数，方案 §10.1）
    std::shared_ptr< DA::DAAbstractArchiveTask > createArchiveTask(bool isSave) override;

private:
    DASignalAnalysisUI* mUI { nullptr };
};
}  // namespace DA

#endif  // DASIGNALANALYSISPLUGIN_H
