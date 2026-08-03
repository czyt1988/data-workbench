#pragma once
#include "DAAgentAPI.h"
#include <QJsonObject>
#include <QString>

namespace DA
{
class DAAgent_API DAAbstractAgentTool
{
public:
    virtual ~DAAbstractAgentTool() = default;
    virtual QJsonObject getToolSpec() const = 0;
    virtual QJsonObject execute(const QJsonObject& params) = 0;
    virtual QString getOwnerModule() const = 0;
};
} // namespace DA
