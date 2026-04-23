#include "DAPyParamTypeHelper.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QDebug>

namespace DA
{

/**
 * @brief 根据Python类型名获取对应的Qt控件类型
 *
 * 将Python参数类型字符串映射到对应的WidgetType枚举值。
 * 支持的标准类型：str, int, float, bool, enum, list。
 *
 * @param[in] pythonTypeName Python类型名称（如"str", "int", "float"等）
 * @return 对应的WidgetType枚举值，未知类型返回UnknownWidget
 */
DAPyParamTypeHelper::WidgetType DAPyParamTypeHelper::getWidgetTypeForParamType(const QString& pythonTypeName)
{
    QString lowerType = pythonTypeName.toLower();

    if (lowerType == "str" || lowerType == "string") {
        return LineEditWidget;
    } else if (lowerType == "int" || lowerType == "integer") {
        return SpinBoxWidget;
    } else if (lowerType == "float" || lowerType == "double") {
        return DoubleSpinBoxWidget;
    } else if (lowerType == "bool" || lowerType == "boolean") {
        return CheckBoxWidget;
    } else if (lowerType == "enum" || lowerType == "enumeration") {
        return ComboBoxWidget;
    } else if (lowerType == "list" || lowerType == "array") {
        return ListWidget;
    }

    return UnknownWidget;
}

/**
 * @brief 根据Python类型名创建对应的Qt控件实例
 *
 * 为指定的Python参数类型创建合适的Qt控件，并设置默认属性：
 * - QSpinBox: 默认范围 -9999 到 9999
 * - QDoubleSpinBox: 默认范围 -9999.0 到 9999.0，小数位2位
 * - QComboBox: 可编辑属性设为false
 * - QListWidget: 启用多选
 *
 * @param[in] pythonTypeName Python类型名称
 * @param[in] parent 父控件指针
 * @return 创建的Qt控件指针，未知类型返回nullptr
 */
QWidget* DAPyParamTypeHelper::createWidgetForParamType(const QString& pythonTypeName, QWidget* parent)
{
    WidgetType widgetType = getWidgetTypeForParamType(pythonTypeName);

    switch (widgetType) {
    case LineEditWidget: {
        QLineEdit* edit = new QLineEdit(parent);
        return edit;
    }
    case SpinBoxWidget: {
        QSpinBox* spinBox = new QSpinBox(parent);
        spinBox->setRange(-9999, 9999);
        return spinBox;
    }
    case DoubleSpinBoxWidget: {
        QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox(parent);
        doubleSpinBox->setRange(-9999.0, 9999.0);
        doubleSpinBox->setDecimals(2);
        return doubleSpinBox;
    }
    case CheckBoxWidget: {
        QCheckBox* checkBox = new QCheckBox(parent);
        return checkBox;
    }
    case ComboBoxWidget: {
        QComboBox* comboBox = new QComboBox(parent);
        comboBox->setEditable(false);
        return comboBox;
    }
    case ListWidget: {
        QListWidget* listWidget = new QListWidget(parent);
        listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        return listWidget;
    }
    default:
        qWarning() << "DAPyParamTypeHelper: Unknown Python type" << pythonTypeName;
        return nullptr;
    }
}

/**
 * @brief 从控件读取值并转换为QVariant
 *
 * 根据控件类型读取当前值：
 * - QLineEdit: 返回QString
 * - QSpinBox: 返回int
 * - QDoubleSpinBox: 返回double
 * - QCheckBox: 返回bool
 * - QComboBox: 返回当前选中的QString
 * - QListWidget: 返回QStringList
 *
 * @param[in] widget 控件指针
 * @return 控件当前值的QVariant表示，无效控件返回无效QVariant
 */
QVariant DAPyParamTypeHelper::getValueFromWidget(QWidget* widget)
{
    if (!widget) {
        return QVariant();
    }

    if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
        return edit->text();
    } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
        return spinBox->value();
    } else if (QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        return doubleSpinBox->value();
    } else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
        return checkBox->isChecked();
    } else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
        return comboBox->currentText();
    } else if (QListWidget* listWidget = qobject_cast<QListWidget*>(widget)) {
        QStringList items;
        for (int i = 0; i < listWidget->count(); ++i) {
            items.append(listWidget->item(i)->text());
        }
        return items;
    }

    return QVariant();
}

/**
 * @brief 设置控件的值
 *
 * 根据控件类型设置值，支持类型转换：
 * - QLineEdit: 接受QString或任何可转换为字符串的类型
 * - QSpinBox: 接受int或double（截断）
 * - QDoubleSpinBox: 接受double或int
 * - QCheckBox: 接受bool
 * - QComboBox: 查找并选择匹配的文本
 * - QListWidget: 接受QStringList，清空并重新填充
 *
 * @param[in] widget 控件指针
 * @param[in] value 要设置的值
 */
void DAPyParamTypeHelper::setValueToWidget(QWidget* widget, const QVariant& value)
{
    if (!widget || !value.isValid()) {
        return;
    }

    if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
        edit->setText(value.toString());
    } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
        spinBox->setValue(value.toInt());
    } else if (QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        doubleSpinBox->setValue(value.toDouble());
    } else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
        checkBox->setChecked(value.toBool());
    } else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
        QString text = value.toString();
        int index = comboBox->findText(text);
        if (index >= 0) {
            comboBox->setCurrentIndex(index);
        }
    } else if (QListWidget* listWidget = qobject_cast<QListWidget*>(widget)) {
        listWidget->clear();
        QStringList items = value.toStringList();
        for (const QString& item : items) {
            listWidget->addItem(item);
        }
    }
}

/**
 * @brief 获取控件类型的显示名称
 *
 * @param[in] type WidgetType枚举值
 * @return 该类型的可读名称（中文）
 */
QString DAPyParamTypeHelper::getWidgetTypeDisplayName(WidgetType type)
{
    switch (type) {
    case LineEditWidget:
        return QString("文本输入");
    case SpinBoxWidget:
        return QString("整数输入");
    case DoubleSpinBoxWidget:
        return QString("浮点数输入");
    case CheckBoxWidget:
        return QString("布尔选择");
    case ComboBoxWidget:
        return QString("下拉选择");
    case ListWidget:
        return QString("列表编辑");
    default:
        return QString("未知类型");
    }
}

/**
 * @brief 判断控件类型是否支持默认值设置
 *
 * 所有标准控件类型都支持默认值设置。
 *
 * @param[in] type WidgetType枚举值
 * @return 支持默认值返回true
 */
bool DAPyParamTypeHelper::supportsDefaultValue(WidgetType type)
{
    return type != UnknownWidget;
}

} // namespace DA
