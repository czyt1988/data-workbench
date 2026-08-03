// DAAgentDockWidget.h —— QWidget 子类（不是 QDockWidget，平台使用 ADS 管理停靠）
#pragma once
#include <QWidget>
namespace DA
{
class DAAgentWebChannel;
class DAAgentDockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DAAgentDockWidget(QWidget* parent = nullptr);
private:
    DAAgentWebChannel* m_channel;
};
} // namespace DA
