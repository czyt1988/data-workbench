#include "DAAgentToolWriteFile.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace DA
{
// isPathSafe is defined in DAAgentToolReadFile.cpp (static linkage);
// duplicate it here to keep the translation unit self-contained.
static bool isPathSafe_write(const QString& path)
{
    QString normalized = QDir::cleanPath(path).toLower();
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
QJsonObject DAAgentToolWriteFile::getToolSpec() const
{
    return QJsonObject{
        {"name", "write_file"},
        {"description", "Write text content to a file. System directories are blocked for safety."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"file_path", QJsonObject{{"type", "string"}, {"description", "Path to the file to write"}}},
                {"content", QJsonObject{{"type", "string"}, {"description", "Text content to write"}}}
            }},
            {"required", QJsonArray{"file_path", "content"}}
        }}
    };
}

/**
 * @copydoc DAAbstractAgentTool::execute
 */
QJsonObject DAAgentToolWriteFile::execute(const QJsonObject& params)
{
    QString filePath = params["file_path"].toString();
    QString content  = params["content"].toString();
    if (filePath.isEmpty()) {
        return errorResponse("file_path is required");
    }
    if (!isPathSafe_write(filePath)) {
        return errorResponse("Access denied: path is in a system-protected directory");
    }

    // Ensure parent directory exists
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return errorResponse(QString("Cannot open file for writing: %1").arg(f.errorString()));
    }
    qint64 written = f.write(content.toUtf8());
    f.close();

    if (written < 0) {
        return errorResponse("Failed to write file");
    }

    return successResponse(QString("Wrote %1 bytes to %2").arg(written).arg(filePath));
}
}  // namespace DA
