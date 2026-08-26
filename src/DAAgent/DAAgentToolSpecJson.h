#pragma once
#include "DAAgentAPI.h"
#include "DAAgentToolSpec.h"
#include <QJsonObject>

namespace DA
{
// 把工具规格序列化为 OpenAI function schema（{"name", "description", "parameters"}），
// 供 DAAgentModule::assembleToolSpecs() 组装后经 stdio 下发给 Python agent 运行时
DAAgent_API QJsonObject toJson(const DAAgentToolSpec& spec);
} // namespace DA
