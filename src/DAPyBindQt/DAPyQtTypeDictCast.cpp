#include "DAPyQtTypeDictCast.h"
#include "DAPybind11QtCaster.hpp"

namespace DA
{

/**
 * @brief 将 QFont 转换为 pybind11::dict
 *
 * 从 QFont 提取 family/pointSize/bold/italic，构建 Python 字典。
 * 字典键为字符串，值类型与 Python 端约定一致。
 *
 * @param[in] font 要转换的 QFont 对象
 * @return 包含 family/size/bold/italic 四个键的 pybind11::dict
 */
pybind11::dict DAPyQtTypeDictCast::qFontToPyDict(const QFont& font)
{
    pybind11::dict result;
    result[ pybind11::str("family") ] = DA::PY::toPyObject(font.family());
    int pointSize = font.pointSize();
    result[ pybind11::str("size") ]   = pybind11::int_(pointSize > 0 ? pointSize : 9);
    result[ pybind11::str("bold") ]   = pybind11::bool_(font.bold());
    result[ pybind11::str("italic") ] = pybind11::bool_(font.italic());
    return result;
}

/**
 * @brief 将 pybind11::dict 转换为 QFont
 *
 * 使用 dict.get(key, default) 提取 family/size/bold/italic 字段，
 * 缺失字段使用默认值，忽略字典中的其他键。
 *
 * @param[in] dict 包含字体字段的 pybind11::dict
 * @return 构建好的 QFont 对象
 */
QFont DAPyQtTypeDictCast::pyDictToQFont(const pybind11::dict& dict)
{
    QFont font;
    if (dict.empty()) {
        return font;
    }

    pybind11::object get = dict.attr("get");

    // family
    pybind11::object familyObj = get("family", pybind11::str(""));
    font.setFamily(DA::PY::fromPyString(familyObj));

    // size
    pybind11::object sizeObj = get("size", pybind11::int_(9));
    int pointSize = 9;
    try {
        pointSize = sizeObj.cast< int >();
    } catch (const std::exception&) {
        // 转换失败保留默认值 9
    }
    font.setPointSize(pointSize > 0 ? pointSize : 9);

    // bold
    pybind11::object boldObj = get("bold", pybind11::bool_(false));
    try {
        font.setBold(boldObj.cast< bool >());
    } catch (const std::exception&) {
        font.setBold(false);
    }

    // italic
    pybind11::object italicObj = get("italic", pybind11::bool_(false));
    try {
        font.setItalic(italicObj.cast< bool >());
    } catch (const std::exception&) {
        font.setItalic(false);
    }

    return font;
}

}  // namespace DA
