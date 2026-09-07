#ifndef DAPLUGINFILEINFO_H
#define DAPLUGINFILEINFO_H
#include <QString>
#include "DAPluginSupportGlobal.h"
namespace DA
{
/**
 * @brief 插件目录下插件文件及其运行状态信息
 *
 * 供插件管理界面展示全部插件（含未加载的禁用项），
 * 由 DAPluginManager::scanPluginFiles 生成
 */
struct DAPLUGINSUPPORT_API DAPluginFileInfo
{
    enum State
    {
        Loaded,                 ///< 已加载
        NotLoaded,              ///< 未加载（目录中新发现的插件文件，可立即加载）
        Disabled,               ///< 被 .pluginignore 禁用，未加载
        LoadFailed,             ///< 加载失败
        InactivePendingRestart  ///< 已停用：资源已清理注销，但库释放失败，重启后完全释放
    };

    State state { NotLoaded };
    QString baseName;      ///< 文件基本名（不含后缀），与 .pluginignore 条目对应
    QString filePath;      ///< 插件文件绝对路径
    QString name;          ///< 插件显示名（已加载时为插件 getName()，否则为 baseName）
    QString version;       ///< 插件版本（未加载时为空）
    QString description;   ///< 插件描述（未加载时为空）
    QString errorString;   ///< 加载失败时的错误信息
};
}  // namespace DA
#endif  // DAPLUGINFILEINFO_H
