#include "DAAgentToolReadFile.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

namespace DA
{
// 路径安全策略已移交权限门（permission-layer P1）：DAAgentBridge::executeTool 前置
// DAAgentPermissionManager::decide()——硬 deny（系统目录）全模式生效，工具内不再重复实现。

/**
 * @copydoc DAAbstractAgentTool::getToolSpec
 */
DAAgentToolSpec DAAgentToolReadFile::getToolSpec() const
{
    using Type = DAAgentToolParam::Type;
    DAAgentToolSpec spec{QStringLiteral("read_file"),
                         QStringLiteral("Read the content of a text file. System directories are blocked for safety.")};
    spec.addParam({QStringLiteral("file_path"), QStringLiteral("Path to the file to read"), {Type::String}, true});
    return spec;
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
    // 系统目录硬 deny 已由权限门统一拦截（permission-layer P1），此处不再重复检查

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
