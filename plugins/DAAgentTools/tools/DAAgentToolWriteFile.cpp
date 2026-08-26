#include "DAAgentToolWriteFile.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace DA
{
// 路径安全策略已移交权限门（permission-layer P1）：工作区内放行、区外询问、
// 系统目录硬 deny，统一由 DAAgentPermissionManager::decide() 执法。

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolWriteFile::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("write_file"),
                         QStringLiteral("Write text content to a file. System directories are blocked for safety.")};
    spec.addParam({QStringLiteral("file_path"), QStringLiteral("Path to the file to write"), {Type::String}, true});
    spec.addParam({QStringLiteral("content"), QStringLiteral("Text content to write"), {Type::String}, true});
    return spec;
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
    // 系统目录硬 deny 已由权限门统一拦截（permission-layer P1），此处不再重复检查

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
