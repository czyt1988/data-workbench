#include "DAAgentToolExportData.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolExportData::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("export_data"),
                         QStringLiteral("Export a dataset to a CSV or Excel file.")};
    spec.addParam({QStringLiteral("data_name"), QStringLiteral("Dataset name"), {Type::String}, true});
    spec.addParam({QStringLiteral("file_path"), QStringLiteral("Output file path"), {Type::String}, true});
    spec.addParam({QStringLiteral("format"),
                   QStringLiteral("Output format: csv or excel (default csv)"),
                   {Type::String}});
    return spec;
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolExportData::execute(const QJsonObject& params)
{
    QString dataName = params["data_name"].toString();
    QString filePath = params["file_path"].toString();
    QString format  = params.contains("format") ? params["format"].toString().toLower() : "csv";

    if (dataName.isEmpty()) {
        return errorResponse("data_name is required");
    }
    if (filePath.isEmpty()) {
        return errorResponse("file_path is required");
    }

    DAData data = findData(dataName);
    if (data.isNull()) {
        return errorResponse(QString("Dataset '%1' not found").arg(dataName));
    }

    DAPyGILGuard gil;
    DAPyDataFrame df = data.toDataFrame();

    bool ok = false;
    if (format == "excel" || format == "xlsx") {
        ok = df.to_excel(filePath);
    } else {
        // default: csv
        ok = df.to_csv(filePath);
    }

    if (!ok) {
        return errorResponse(QString("Failed to export data to %1").arg(filePath));
    }

    return successResponse(QString("Data exported to %1").arg(filePath));
}
}  // namespace DA
