#ifndef DAPYPARAMTYPEHELPER_H
#define DAPYPARAMTYPEHELPER_H
#include "DAPyWorkFlowAPI.h"
#include <QString>
#include <QMetaType>

class QWidget;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QListWidget;

namespace DA
{

/**
 * @brief 参数类型到Qt控件类型的映射工具类
 *
 * 提供Python参数类型与Qt控件类型之间的映射关系，
 * 用于动态生成节点参数配置界面。
 *
 * 支持的类型映射：
 * - 'str' → QLineEdit
 * - 'int' → QSpinBox
 * - 'float' → QDoubleSpinBox
 * - 'bool' → QCheckBox
 * - 'enum' → QComboBox
 * - 'list' → QListWidget
 *
 * @see DAPyNodeWidget DAPyNodeConfigDialog
 */
class DAPYWORKFLOW_API DAPyParamTypeHelper
{
public:
    /**
     * @brief 参数控件类型枚举
     */
    enum WidgetType {
        UnknownWidget,      ///< 未知控件类型
        LineEditWidget,     ///< QLineEdit - 字符串输入
        SpinBoxWidget,      ///< QSpinBox - 整数输入
        DoubleSpinBoxWidget,///< QDoubleSpinBox - 浮点数输入
        CheckBoxWidget,     ///< QCheckBox - 布尔值
        ComboBoxWidget,     ///< QComboBox - 枚举选择
        ListWidget          ///< QListWidget - 列表编辑
    };

    // 根据Python类型名获取对应的Qt控件类型
    static WidgetType getWidgetTypeForParamType(const QString& pythonTypeName);

    // 根据Python类型名创建对应的Qt控件实例
    static QWidget* createWidgetForParamType(const QString& pythonTypeName, QWidget* parent = nullptr);

    // 从控件读取值并转换为Python兼容的字符串表示
    static QVariant getValueFromWidget(QWidget* widget);

    // 设置控件的值
    static void setValueToWidget(QWidget* widget, const QVariant& value);

    // 获取控件类型的显示名称
    static QString getWidgetTypeDisplayName(WidgetType type);

    // 判断控件类型是否支持默认值设置
    static bool supportsDefaultValue(WidgetType type);
};

} // namespace DA

#endif // DAPYPARAMTYPEHELPER_H
