#include "DAPyPythonCompletion.h"
#include "DAPyGILGuard.h"
#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include "DAPythonSyntax.h"
// Qt
#include <QDebug>
#include <QHash>
#include <QStringList>

namespace DA
{
namespace
{
/**
 * @brief 取对象的公共成员名（过滤下划线开头项）
 * @param obj Python 对象
 * @return 成员名列表（去重，按 dir() 顺序）
 */
QStringList pythonDir(const pybind11::object& obj)
{
    pybind11::module_ builtins = pybind11::module_::import("builtins");
    pybind11::list names       = builtins.attr("dir")(obj).cast< pybind11::list >();
    QStringList res;
    for (auto item : names) {
        const std::string name = item.cast< std::string >();
        if (name.empty() || name.front() == '_') {
            continue;
        }
        res.append(QString::fromUtf8(name.c_str()));
    }
    res.removeDuplicates();
    return res;
}
}  // namespace

/**
 * @brief Python 关键字（与高亮器共用 DAPythonSyntax 词表）
 */
QStringList DAPyPythonCompletion::keywords()
{
    return DAPythonSyntax::keywords();
}

/**
 * @brief Python 内置名（与高亮器共用 DAPythonSyntax 词表）
 */
QStringList DAPyPythonCompletion::builtins()
{
    return DAPythonSyntax::builtins();
}

/**
 * @brief 当前是否处于可用的 Python 环境
 */
bool DAPyPythonCompletion::isPythonReady()
{
    return DAPyInterpreter::isPythonInitialized();
}

/**
 * @brief 取指定对象的成员列表（首次调用查 Python，之后命中缓存）
 *
 * objectKey 取值：pd / np / DataFrame / Series / Series.str / Series.dt / Index。
 * 未知 key 返回空列表；Python 未初始化或导入失败时返回空列表（不抛异常）。
 * @param objectKey 对象标识
 * @return 公共成员名列表
 */
QStringList DAPyPythonCompletion::membersOf(const QString& objectKey)
{
    // 只在 UI 线程调用；缓存避免每次击键都跨 Python 取 dir()
    static QHash< QString, QStringList > s_cache;
    const auto cached = s_cache.constFind(objectKey);
    if (cached != s_cache.constEnd()) {
        return cached.value();
    }
    if (!isPythonReady()) {
        return {};
    }
    QStringList members;
    try {
        DAPyGILGuard gil;
        pybind11::object obj;
        if (objectKey == QLatin1String("pd")) {
            obj = pybind11::module_::import("pandas");
        } else if (objectKey == QLatin1String("np")) {
            obj = pybind11::module_::import("numpy");
        } else if (objectKey == QLatin1String("DataFrame")) {
            obj = pybind11::module_::import("pandas").attr("DataFrame");
        } else if (objectKey == QLatin1String("Series")) {
            obj = pybind11::module_::import("pandas").attr("Series");
        } else if (objectKey == QLatin1String("Series.str")) {
            obj = pybind11::module_::import("pandas").attr("Series").attr("str");
        } else if (objectKey == QLatin1String("Series.dt")) {
            obj = pybind11::module_::import("pandas").attr("Series").attr("dt");
        } else if (objectKey == QLatin1String("Index")) {
            obj = pybind11::module_::import("pandas").attr("Index");
        }
        if (!obj) {
            return {};
        }
        members = pythonDir(obj);
    } catch (const std::exception& e) {
        // 开发诊断：补全数据源不可用不应打断编辑
        qWarning() << "DAPyPythonCompletion::membersOf failed:" << objectKey << e.what();
        return {};
    }
    s_cache.insert(objectKey, members);
    return members;
}
}  // namespace DA
