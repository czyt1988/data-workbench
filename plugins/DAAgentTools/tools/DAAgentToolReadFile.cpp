#include "DAAgentToolReadFile.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace DA
{
/// Check if a file path is safe to access (not in system-critical directories).
static bool isPathSafe(const QString& path)
{
    QString normalized = QDir::cleanPath(path).toLower();
    // Block Windows system directories
    static const QStringList blocked = {
        "c:/windows",
        "c:/windows/system32",
        "c:/program files",
        "c:/program files (x86)",
        "c:/programdata",
    };
    for (const QString& b : blocked) {
        if (normalized.startsWith(b)) {
            return false;
        }
    }
    return true;
}

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
QJsonObject DAAgentToolReadFile::getToolSpec() const
{
    return QJsonObject{
        {"name", "read_file"},
        {"description", "Read the content of a text file. System directories are blocked for safety."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"file_path", QJsonObject{{"type", "string"}, {"description", "Path to the file to read"}}}
            }},
            {"required", QJsonArray{"file_path"}}
        }}
    };
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolReadFile::execute(const QJsonObject& params)
{
    QString filePath = params["file_path"].toString();
    if (filePath.isEmpty()) {
        return errorResponse("file_path is required");
    }
    if (!isPathSafe(filePath)) {
        return errorResponse("Access denied: path is in a system-protected directory");
    }

    QFile f(filePath);
    if (!f.exists()) {
        return errorResponse(QString("File not found: %1").arg(filePath));
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return errorResponse(QString("Cannot open file: %1").arg(f.errorString()));
    }
    QByteArray data = f.readAll();
    f.close();

    QJsonObject result;
    result["content"]  = QString::fromUtf8(data);
    result["size"]     = static_cast<qint64>(data.size());
    result["file_path"] = filePath;
    return result;
}
}  // namespace DA
