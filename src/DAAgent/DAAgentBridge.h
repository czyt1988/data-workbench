// DAAgentBridge.h
#pragma once
#include <QObject>
namespace DA
{
class DAAgentBridge : public QObject
{
    Q_OBJECT
public:
    explicit DAAgentBridge(QObject* parent = nullptr);
};
} // namespace DA
