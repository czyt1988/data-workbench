#ifndef DAPYNODEWIDGET_H
#define DAPYNODEWIDGET_H
#include "DAGuiAPI.h"
#include "DAPybind11InQt.h"
#include <QWidget>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>

class QFormLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QListWidget;

namespace DA
{

class DAPyNodeProxy;

/**
 * @brief Python节点参数配置动态UI组件
 *
 * 独立QWidget，根据DANodeDescriptor.parameters动态生成表单控件。
 * 不继承DAAbstractNodeWidget，直接使用QWidget基类和PIMPL模式。
 * 支持多种参数类型：str、int、float、bool、enum、list。
 * 采用表单布局（Form Layout），标签在左，编辑器在右。
 *
 * @code
 * // 创建参数配置组件
 * auto widget = new DA::DAPyNodeWidget(proxy, parent);
 * widget->buildWidgetsFromDescriptor(paramDict);
 *
 * // 获取当前参数值
 * auto values = widget->getParameterValuesAsDict();
 * @endcode
 *
 * @see DAPyParamTypeHelper DAPyNodeConfigDialog
 */
class DAGUI_API DAPyNodeWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyNodeWidget)

public:
    // 构造函数
    explicit DAPyNodeWidget(QWidget* parent = nullptr);

    // 构造函数（带节点代理）
    explicit DAPyNodeWidget(DAPyNodeProxy* proxy, QWidget* parent = nullptr);

    ~DAPyNodeWidget();

    // 设置节点代理
    void setNodeProxy(DAPyNodeProxy* proxy);

    // 获取节点代理
    DAPyNodeProxy* getNodeProxy() const;

    // 根据参数描述符动态构建控件
    void buildWidgetsFromDescriptor(const pybind11::dict& paramDict);

    // 获取当前参数值（Python字典格式）
    pybind11::dict getParameterValuesAsDict() const;

    // 从Python字典设置参数值
    void setParameterValuesFromDict(const pybind11::dict& valuesDict);

    // 清空所有参数控件
    void clearParameterWidgets();

    // 检查是否有任何参数
    bool hasParameters() const;

    // 获取参数数量
    int getParameterCount() const;

Q_SIGNALS:
    /**
     * @brief 参数值发生变化时发出的信号
     * @param paramName 参数名称
     * @param value 新的参数值
     */
    void parameterValueChanged(const QString& paramName, const QVariant& value);

    /**
     * @brief 参数配置被修改（与默认值不同）
     */
    void parametersModified();

protected:
    // 处理参数值变化
    void onParameterValueChanged();

private:
    // 创建列表类型控件的辅助UI（带+/-按钮）
    QWidget* createListWidgetWithButtons(const QJsonObject& paramDef);
};

} // namespace DA

#endif // DAPYNODEWIDGET_H