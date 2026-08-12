#ifndef DAPYQTTYPEDICTCAST_H
#define DAPYQTTYPEDICTCAST_H
#include "DAPyBindQtGlobal.h"
#include "DAPybind11InQt.h"
#include <QFont>

namespace DA
{

/**
 * @brief Qt 类型与 pybind11::dict 之间的直接转换工具类
 *
 * 提供 Qt 复合类型（如 QFont、QColor 等）与 Python 字典之间的直接双向转换，
 * 避免通过 QVariantMap 中转的开销。所有方法均为静态方法，无需创建实例。
 *
 * 当前支持的类型：
 * - QFont ↔ dict（字段：family/size/bold/italic）
 *
 * 后续如需添加其他 Qt 类型（QColor、QPen、QBrush 等）的字典转换，
 * 在本类中添加对应的静态方法即可。
 *
 * @code
 * // QFont 转 dict
 * QFont font("Microsoft YaHei", 9);
 * font.setBold(true);
 * pybind11::dict dict = DAPyQtTypeDictCast::qFontToPyDict(font);
 * // dict == {"family": "Microsoft YaHei", "size": 9, "bold": true, "italic": false}
 *
 * // dict 转 QFont
 * QFont f = DAPyQtTypeDictCast::pyDictToQFont(dict);
 * @endcode
 *
 * @see DAPyJsonCast
 */
class DAPYBINDQT_API DAPyQtTypeDictCast
{
    Q_DISABLE_COPY(DAPyQtTypeDictCast)
public:
    DAPyQtTypeDictCast() = delete;
    ~DAPyQtTypeDictCast() = delete;

    // 将 QFont 转换为 pybind11::dict
    static pybind11::dict qFontToPyDict(const QFont& font);

    // 将 pybind11::dict 转换为 QFont
    static QFont pyDictToQFont(const pybind11::dict& dict);
};

}  // namespace DA

#endif  // DAPYQTTYPEDICTCAST_H
