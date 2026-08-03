// DAAgentSettingsWidget.h —— QWidget 子类（plan-06 将改为 DAAbstractSettingPage）
#pragma once
#include <QWidget>
namespace DA
{
class DAAgentSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DAAgentSettingsWidget(QWidget* parent = nullptr);
};
} // namespace DA
