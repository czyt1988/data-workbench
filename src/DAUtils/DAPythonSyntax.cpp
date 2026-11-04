#include "DAPythonSyntax.h"
#include <QRegularExpression>

namespace DA
{
/**
 * @brief Python 硬关键字（3.x）
 */
const QStringList& DAPythonSyntax::keywords()
{
    static const QStringList s_keywords = {
        QStringLiteral("False"),  QStringLiteral("None"),    QStringLiteral("True"),     QStringLiteral("and"),
        QStringLiteral("as"),     QStringLiteral("assert"),  QStringLiteral("async"),    QStringLiteral("await"),
        QStringLiteral("break"),  QStringLiteral("class"),   QStringLiteral("continue"), QStringLiteral("def"),
        QStringLiteral("del"),    QStringLiteral("elif"),    QStringLiteral("else"),     QStringLiteral("except"),
        QStringLiteral("finally"), QStringLiteral("for"),    QStringLiteral("from"),     QStringLiteral("global"),
        QStringLiteral("if"),     QStringLiteral("import"),  QStringLiteral("in"),       QStringLiteral("is"),
        QStringLiteral("lambda"), QStringLiteral("nonlocal"), QStringLiteral("not"),     QStringLiteral("or"),
        QStringLiteral("pass"),   QStringLiteral("raise"),   QStringLiteral("return"),   QStringLiteral("try"),
        QStringLiteral("while"),  QStringLiteral("with"),    QStringLiteral("yield")
    };
    return s_keywords;
}

/**
 * @brief 内置函数与常用内置异常名
 */
const QStringList& DAPythonSyntax::builtins()
{
    static const QStringList s_builtins = {
        QStringLiteral("abs"),         QStringLiteral("aiter"),      QStringLiteral("all"),       QStringLiteral("anext"),
        QStringLiteral("any"),         QStringLiteral("ascii"),      QStringLiteral("bin"),       QStringLiteral("bool"),
        QStringLiteral("breakpoint"),  QStringLiteral("bytearray"),  QStringLiteral("bytes"),     QStringLiteral("callable"),
        QStringLiteral("chr"),         QStringLiteral("classmethod"), QStringLiteral("compile"),  QStringLiteral("complex"),
        QStringLiteral("delattr"),     QStringLiteral("dict"),       QStringLiteral("dir"),       QStringLiteral("divmod"),
        QStringLiteral("enumerate"),   QStringLiteral("eval"),       QStringLiteral("exec"),      QStringLiteral("filter"),
        QStringLiteral("float"),       QStringLiteral("format"),     QStringLiteral("frozenset"), QStringLiteral("getattr"),
        QStringLiteral("globals"),     QStringLiteral("hasattr"),    QStringLiteral("hash"),      QStringLiteral("help"),
        QStringLiteral("hex"),         QStringLiteral("id"),         QStringLiteral("input"),     QStringLiteral("int"),
        QStringLiteral("isinstance"),  QStringLiteral("issubclass"), QStringLiteral("iter"),      QStringLiteral("len"),
        QStringLiteral("list"),        QStringLiteral("locals"),     QStringLiteral("map"),       QStringLiteral("max"),
        QStringLiteral("memoryview"),  QStringLiteral("min"),        QStringLiteral("next"),      QStringLiteral("object"),
        QStringLiteral("oct"),         QStringLiteral("open"),       QStringLiteral("ord"),       QStringLiteral("pow"),
        QStringLiteral("print"),       QStringLiteral("property"),   QStringLiteral("range"),     QStringLiteral("repr"),
        QStringLiteral("reversed"),    QStringLiteral("round"),      QStringLiteral("set"),       QStringLiteral("setattr"),
        QStringLiteral("slice"),       QStringLiteral("sorted"),     QStringLiteral("staticmethod"), QStringLiteral("str"),
        QStringLiteral("sum"),         QStringLiteral("super"),      QStringLiteral("tuple"),     QStringLiteral("type"),
        QStringLiteral("vars"),        QStringLiteral("zip"),        QStringLiteral("Exception"), QStringLiteral("ValueError"),
        QStringLiteral("TypeError"),   QStringLiteral("KeyError"),   QStringLiteral("IndexError"), QStringLiteral("RuntimeError"),
        QStringLiteral("StopIteration"), QStringLiteral("AttributeError"), QStringLiteral("ImportError")
    };
    return s_builtins;
}

/**
 * @brief 是否合法 Python 标识符（含 Unicode 字母）
 * @param text 待判定文本
 * @return 合法返回 true；空串/含空格/以数字开头返回 false
 */
bool DAPythonSyntax::isIdentifier(const QString& text)
{
    if (text.isEmpty()) {
        return false;
    }
    static const QRegularExpression rx(QStringLiteral("^[\\p{L}_][\\p{L}\\p{N}_]*$"),
                                       QRegularExpression::UseUnicodePropertiesOption);
    return rx.match(text).hasMatch();
}

/**
 * @brief 是否 Python 关键字
 */
bool DAPythonSyntax::isKeyword(const QString& text)
{
    return keywords().contains(text);
}

/**
 * @brief 列名是否必须用反引号包裹
 * @param name 列名
 * @param reservedNames 额外保留名（如命名空间里的 df/pd/np/result）
 * @return 非法标识符 / 关键字 / 命中保留名返回 true
 */
bool DAPythonSyntax::needsBacktick(const QString& name, const QStringList& reservedNames)
{
    return (!isIdentifier(name)) || isKeyword(name) || reservedNames.contains(name);
}
}  // namespace DA
