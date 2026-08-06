#include "DAAgentToolListData.h"

namespace DA
{
QJsonObject DAAgentToolListData::getToolSpec() const
{
    return QJsonObject{
        {"name", "list_data"},
        {"description", "List all currently loaded datasets with their names, types, row counts, and column counts."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{}},
            {"required", QJsonArray{}}
        }}
    };
}

QJsonObject DAAgentToolListData::execute(const QJsonObject& params)
{
    Q_UNUSED(params);
    QJsonArray datasets;
    for (const DAData& data : allDatas()) {
        QJsonObject ds;
        ds["name"] = data.getName();
        ds["type"] = data.typeToString();
        auto shape = data.shape();
        ds["rows"] = static_cast<qint64>(shape.first);
        ds["cols"] = static_cast<qint64>(shape.second);
        datasets.append(ds);
    }
    QJsonObject result;
    result["datasets"] = datasets;
    result["count"]    = datasets.size();
    return result;
}
}  // namespace DA
