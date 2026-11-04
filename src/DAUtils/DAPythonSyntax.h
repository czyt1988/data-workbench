#ifndef DAPYTHONSYNTAX_H
#define DAPYTHONSYNTAX_H
#include <QString>
#include <QStringList>
#include "DAUtilsAPI.h"

namespace DA
{
/**
 * @brief Python 语言词法数据（关键字 / 内置名 / 标识符判定）
 *
 * 供语法高亮、自动补全、列名反引号判定共用，避免同一份词表在多处维护。
 * 纯静态工具类，不依赖 Python 解释器。
 */
class DAUTILS_API DAPythonSyntax
{
public:
    // Python 硬关键字（3.x）
    static const QStringList& keywords();
    // 内置函数与常用内置异常名
    static const QStringList& builtins();
    // 是否合法 Python 标识符（含 Unicode 字母）
    static bool isIdentifier(const QString& text);
    // 是否 Python 关键字
    static bool isKeyword(const QString& text);
    // 列名是否必须用反引号包裹（非法标识符 / 关键字 / 命中保留名）
    static bool needsBacktick(const QString& name, const QStringList& reservedNames = QStringList());
};
}  // namespace DA
#endif  // DAPYTHONSYNTAX_H
