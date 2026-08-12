#include "DAPyNodeParameter.h"
#include "DAPyBindQt/DAPyGILGuard.h"
#include "DAPybind11QtCaster.hpp"
#include <QDebug>

namespace DA
{

/**
 * @brief 获取参数名称
 * @return 参数名称，无效时返回空字符串
 */
QString DAPyNodeParameter::name() const
{
    if (isNone()) {
        return {};
    }
    try {
        return attr("name").cast< QString >();
    } catch (const std::exception&) {
        return {};
    }
}

/**
 * @brief 获取参数类型标签
 *
 * 调用Python Parameter的get_type_label()方法获取类型字符串。
 *
 * @return 类型标签字符串（如"str"/"int"/"float"/"bool"/"enum"等）
 */
QString DAPyNodeParameter::typeLabel() const
{
    if (isNone()) {
        return {};
    }
    try {
        return attr("get_type_label")().cast< QString >();
    } catch (const std::exception&) {
        return {};
    }
}

/**
 * @brief 获取参数描述
 * @return 参数描述文本
 */
QString DAPyNodeParameter::description() const
{
    if (isNone()) {
        return {};
    }
    try {
        return attr("description").cast< QString >();
    } catch (const std::exception&) {
        return {};
    }
}

/**
 * @brief 获取参数默认值
 * @return 默认值QVariant，无默认值时返回无效QVariant
 */
QVariant DAPyNodeParameter::defaultValue() const
{
    if (isNone()) {
        return {};
    }
    try {
        pybind11::object defVal = attr("default");
        if (defVal.is_none()) {
            return {};
        }
        // bool必须在int之前检查
        if (pybind11::isinstance< pybind11::bool_ >(defVal)) {
            return QVariant(defVal.cast< bool >());
        } else if (pybind11::isinstance< pybind11::int_ >(defVal)) {
            return QVariant(defVal.cast< int >());
        } else if (pybind11::isinstance< pybind11::float_ >(defVal)) {
            return QVariant(defVal.cast< double >());
        } else if (pybind11::isinstance< pybind11::str >(defVal)) {
            return QVariant(defVal.cast< QString >());
        }
        return defVal.cast< QVariant >();
    } catch (const std::exception&) {
        return {};
    }
}

/**
 * @brief 判断是否有默认值
 * @return true表示Parameter声明了默认值
 */
bool DAPyNodeParameter::hasDefaultValue() const
{
    if (isNone()) {
        return false;
    }
    try {
        return !attr("default").is_none();
    } catch (...) {
        return false;
    }
}

/**
 * @brief 获取扩展属性
 *
 * 读取Python Parameter的_extra_kwargs属性，转换为QVariantHash。
 *
 * @return 扩展属性键值对
 */
QVariantHash DAPyNodeParameter::properties() const
{
    if (isNone()) {
        return {};
    }
    try {
        pybind11::object extraKwargs = attr("_extra_kwargs");
        if (extraKwargs.is_none() || !pybind11::isinstance< pybind11::dict >(extraKwargs)) {
            return {};
        }
        return extraKwargs.cast< QVariantHash >();
    } catch (const std::exception&) {
        return {};
    }
}

/**
 * @brief 判断是否包含指定扩展属性
 * @param[in] propName 属性名
 * @return true表示包含
 */
bool DAPyNodeParameter::hasProperty(const QString& propName) const
{
    if (isNone()) {
        return false;
    }
    try {
        pybind11::dict extraKwargs = attr("_extra_kwargs");
        if (extraKwargs.is_none()) {
            return false;
        }
        return extraKwargs.contains(pybind11::cast(propName));
    } catch (const std::exception&) {
        return false;
    }
    return false;
}

/**
 * @brief 输出 DAPyNodeParameter 信息到 QDebug
 * @param[in] dbg QDebug 对象
 * @param[in] param 参数代理
 * @return QDebug 对象
 */
QDebug operator<<(QDebug dbg, const DAPyNodeParameter& param)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "DAPyNodeParameter(name=" << param.name() << ", type=" << param.typeLabel() << ")";
    return dbg;
}

}  // namespace DA
