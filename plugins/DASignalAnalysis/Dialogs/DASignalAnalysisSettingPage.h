#ifndef DASIGNALANALYSISSETTINGPAGE_H
#define DASIGNALANALYSISSETTINGPAGE_H
#include <QPointer>
#include <QVariant>
#include "Common/Settings/DAAbstractSettingPage.h"

class QLabel;
class QJsonObject;

namespace DA
{
class DAPropertyFormWidget;
class DASignalAnalysisUI;

/**
 * @brief 信号分析插件全局设置页（主页 → 设置 → 信号分析）
 *
 * 只放与具体信号/数据无关的全局配置（当前为 FFmpeg 可执行文件路径）：
 * 这样没有导入任何数据时也能完成配置，不必先有信号再打开分析设置。
 * 频率分辨率、窗函数、峰值门限、标定等按信号生效的分析参数仍放在
 * 信号分析标签页的「分析设置」对话框里（DASignalAnalysisSettingDialog）。
 */
class DASignalAnalysisSettingPage : public DAAbstractSettingPage
{
    Q_OBJECT
public:
    explicit DASignalAnalysisSettingPage(QWidget* parent = nullptr);
    ~DASignalAnalysisSettingPage() override;

    // DAAbstractSettingPage 接口：应用设置 / 页面标题 / 页面图标
    void apply() override;
    QString getSettingPageTitle() const override;
    QIcon getSettingPageIcon() const override;

    // 注入插件界面层：应用后刷新视频导入入口可用性；插件卸载后 QPointer 自动置空
    void setSignalAnalysisUI(DASignalAnalysisUI* ui);

protected:
    void changeEvent(QEvent* e) override;

private Q_SLOTS:
    void onFieldValueChanged(const QString& fieldName, const QVariant& value);

private:
    void buildUI();
    void retranslateUi();
    // 语言变更后按新语言重建表单规格（字段标签/描述），保留用户当前输入
    void rebuildFormForLanguage();
    // 用配置里的 FFmpeg 路径回填表单（空表示使用 PATH 中的 ffmpeg）
    void loadConfig();
    // 内联状态：留空（显示 PATH 探测结果）/ 路径有效 / 文件不存在
    void refreshStatus();
    // 应用 FFmpeg 路径：留空清除配置；无效路径不落盘并返回 false
    bool applyFFmpegPath();
    // 全局设置表单规格（「外部工具」分组 + FFmpeg 可执行文件字段）
    static QJsonObject buildFormSpec();

private:
    QPointer< DASignalAnalysisUI > mSignalUI;
    DAPropertyFormWidget* mForm { nullptr };
    QLabel* mStatusLabel { nullptr };
    QLabel* mHintLabel { nullptr };
    bool mFormReady { false };  ///< 表单规格是否已就绪（retranslateUi 在构造早期也会被调用）
};
}  // namespace DA

#endif  // DASIGNALANALYSISSETTINGPAGE_H
