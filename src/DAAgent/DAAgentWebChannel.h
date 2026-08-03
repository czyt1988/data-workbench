// DAAgentWebChannel.h —— QObject 桥对象，由 DAAgentDockWidget 通过 QWebChannel::registerObject 暴露给 JS
// 注意：本类是 QObject（不是 QWebChannel）。QWebChannel 实例本身由 setupWebChannel() 中
// `new QWebChannel(this)` 单独创建，并通过 registerObject("chatBridge", m_channel) 注册本对象
// （见 plan-03 §2.3 setupWebChannel）。继承 QObject 即可拥有 Q_OBJECT/槽，供 JS 调用。
#pragma once
#include <QObject>
namespace DA
{
class DAAgentWebChannel : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentWebChannel(QObject* parent = nullptr);
};
} // namespace DA
