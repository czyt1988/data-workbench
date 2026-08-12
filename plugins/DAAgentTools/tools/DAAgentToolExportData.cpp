#include "DAAgentToolExportData.h"
#include "DAPyGILGuard.h"
#include "pandas/DAPyDataFrame.h"

namespace DA
{

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolExportData::getToolSpec() const
{
    return QJsonObject{
        {"name", "export_data"},
        {"description", "Export a dataset to a CSV or Excel file."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"data_name", QJsonObject{{"type", "string"}, {"description", "Dataset name"}}},
                {"file_path", QJsonObject{{"type", "string"}, {"description", "Output file path"}}},
                {"format", QJsonObject{{"type", "string"}, {"description", "Output format: csv or excel (default csv)"}}}
            }},
            {"required", QJsonArray{"data_name", "file_path"}}
        }}
    };
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
