#ifndef DAPYNODEPARAMETER_H
#define DAPYNODEPARAMETER_H
#include "DAPyWorkFlowAPI.h"
#include "DAPyObjectWrapper.h"
#include <QString>
#include <QVariant>
#include <QVariantHash>

namespace DA
{

/**
 * @brief Python Parameter描述符的C++代理类
 *
 * 继承DAPyObjectWrapper，包装Python Parameter实例（类型声明描述符）。
 * Parameter是类级别的全局定义，不包含实例值。
 * 实例值的读写通过DAPyNode::getParameterValue()/setParameterValue()完成。
 *
 * @code
 * QList<DAPyNodeParameter> params = node.getParameters();
 * for (const auto& p : params) {
 *     qDebug() << p.name() << p.typeLabel() << p.defaultValue();
 *     QVariant val = node.getParameterValue(p.name());  // 通过DAPyNode读值
 * }
 * @endcode
 *
 * @see DAPyNode DAPyObjectWrapper
 */
class DAPYWORKFLOW_API DAPyNodeParameter : public DAPyObjectWrapper
{
public:
    using DAPyObjectWrapper::DAPyObjectWrapper;

    // 参数名称
    QString name() const;

    // 参数类型标签（调用Python get_type_label()方法）
    QString typeLabel() const;

    // 参数描述
    QString description() const;

    // 默认值
    QVariant defaultValue() const;

    // 是否有默认值
    bool hasDefaultValue() const;

    // 扩展属性（_extra_kwargs → QVariantHash）
    QVariantHash properties() const;

    // 是否包含指定扩展属性
    bool hasProperty(const QString& propName) const;
};

// QDebug输出
DAPYWORKFLOW_API QDebug operator<<(QDebug dbg, const DAPyNodeParameter& param);

}  // namespace DA

Q_DECLARE_METATYPE(DA::DAPyNodeParameter)

#endif  // DAPYNODEPARAMETER_H
