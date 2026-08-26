#include "DAAgentToolColumnStats.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolColumnStats::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("get_column_stats"),
                         QStringLiteral("Get descriptive statistics for a column in a dataset.")};
    spec.addParam({QStringLiteral("data_name"), QStringLiteral("Dataset name"), {Type::String}, true});
    spec.addParam({QStringLiteral("column"), QStringLiteral("Column name"), {Type::String}, true});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolColumnStats::execute(const QJsonObject& params)
{
    QString dataName = params["data_name"].toString();
    QString column   = params["column"].toString();
    if (dataName.isEmpty()) {
        return errorResponse("data_name is required");
    }
    if (column.isEmpty()) {
        return errorResponse("column is required");
    }

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();

    // Check column exists
    QList<QString> cols = df.columns();
    if (!cols.contains(column)) {
        return errorResponse(QString("Column '%1' not found in dataset").arg(column));
    }

    // Use operator[] (df["col"]) NOT df.loc("col") which accesses rows by label
    DAPySeries col = df[ column ];

    // Missing value count: isNull().sum() two-step
    DAPySeries nullMask = col.isNull();
    qint64 missingCount = nullMask.sum();

    // Descriptive statistics
    DAPySeries stats = col.describe();
    QStringList statNames = stats.indexAsStringList();
    QJsonObject statsObj;
    for (int i = 0; i < statNames.size(); ++i) {
        QString name = statNames[i];
        QString valStr = stats.valueAsString(i);
        // Try numeric conversion
        bool ok;
        double dval = valStr.toDouble(&ok);
        if (ok) {
            statsObj[name] = dval;
        } else {
            statsObj[name] = valStr;
        }
    }
    statsObj["missing_count"] = static_cast<qint64>(missingCount);

    QJsonObject result;
    result["column"] = column;
    result["dtype"]  = col.dtypeString();
    result["stats"]  = statsObj;
    return result;
}
}  // namespace DA
