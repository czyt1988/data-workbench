#include "DAAgentToolDataInfo.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"
#include "numpy/DAPyDType.h"
#include <QJsonValue>

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolDataInfo::getToolSpec() const
{
    return QJsonObject{
        {"name", "get_data_info"},
        {"description", "Get dataset schema and a preview of the first N rows."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"preview_rows", QJsonObject{{"type", "integer"}, {"description", "Number of preview rows, default 10"}}}
            }},
            {"required", QJsonArray{"data_name"}}
        }}
    };
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolDataInfo::execute(const QJsonObject& params)
{
    QString dataName = params["data_name"].toString();
    if (dataName.isEmpty()) {
        return errorResponse("data_name is required");
    }
    int previewRows = params.contains("preview_rows") ? params["preview_rows"].toInt() : 10;
    if (previewRows <= 0) previewRows = 10;

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();

    auto shape  = df.shape();
    int rowCnt  = static_cast<int>(shape.first);
    int colCnt  = static_cast<int>(shape.second);

    QList<QString> cols = df.columns();
    QJsonArray columnsArr;
    QJsonArray dtypesArr;
    for (int c = 0; c < colCnt; ++c) {
        columnsArr.append(cols.value(c));
        DAPyDType dt = df.dtypeObject(c);
        dtypesArr.append(dt.name());
    }

    // Preview rows
    int n = qMin(previewRows, rowCnt);
    QJsonArray preview;
    if (n > 0) {
        DAPyDataFrame headDf = df.head(n);
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
    result["columns"] = columnsArr;
    result["dtypes"]  = dtypesArr;
    result["rows"]    = rowCnt;
    result["cols"]    = colCnt;
    result["preview"] = preview;
    return result;
}
}  // namespace DA
