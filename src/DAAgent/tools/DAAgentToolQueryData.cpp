#include "DAAgentToolQueryData.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"
#include <QJsonValue>

namespace DA
{
QJsonObject DAAgentToolQueryData::getToolSpec() const
{
    return QJsonObject{
        {"name", "query_data"},
        {"description", "Filter dataset rows using a pandas query expression and return a preview."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"expr", QJsonObject{{"type", "string"}, {"description", "pandas query expression, e.g. 'col > 100'"}}},
                {"preview_rows", QJsonObject{{"type", "integer"}, {"description", "Number of preview rows, default 10"}}}
            }},
            {"required", QJsonArray{"data_name", "expr"}}
        }}
    };
}

QJsonObject DAAgentToolQueryData::execute(const QJsonObject& params)
{
    QString dataName = params["data_name"].toString();
    QString expr     = params["expr"].toString();
    if (dataName.isEmpty()) {
        return errorResponse("data_name is required");
    }
    if (expr.isEmpty()) {
        return errorResponse("expr is required");
    }
    int previewRows = params.contains("preview_rows") ? params["preview_rows"].toInt() : 10;
    if (previewRows <= 0) previewRows = 10;

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();
    DAPyDataFrame queried;
    try {
        queried = df.query(expr);
    } catch (const std::exception& e) {
        return errorResponse(QString("Query failed: %1").arg(e.what()));
    }

    auto shape = queried.shape();
    int rowCnt = static_cast<int>(shape.first);
    int colCnt = static_cast<int>(shape.second);

    QList<QString> cols = queried.columns();
    QJsonArray columnsArr;
    for (const QString& c : cols) {
        columnsArr.append(c);
    }

    // Preview rows
    int n = qMin(previewRows, rowCnt);
    QJsonArray preview;
    if (n > 0) {
        DAPyDataFrame headDf = queried.head(n);
        for (int r = 0; r < n; ++r) {
            QJsonArray row;
            for (int c = 0; c < colCnt; ++c) {
                QVariant v = headDf.iat(r, c);
                row.append(QJsonValue::fromVariant(v));
            }
            preview.append(row);
        }
    }

    QJsonObject result;
    result["rows"]    = rowCnt;
    result["cols"]    = colCnt;
    result["columns"] = columnsArr;
    result["preview"] = preview;
    return result;
}
}  // namespace DA
