#include "DAPyNodeWidget.h"
#include "DAPyParamTypeHelper.h"
#include "DAPyNodeProxy.h"
#include "DAPybind11InQt.h"
#include "DAPyGILGuard.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace DA
{

//===================================================
// DAPyNodeWidget::PrivateData
//===================================================

class DAPyNodeWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNodeWidget)

public:
    PrivateData(DAPyNodeWidget* p);
    ~PrivateData();

    // 清理所有控件
    void clearWidgets();

    // 创建参数控件
    QWidget* createParameterWidget(const QJsonObject& paramDef, const QString& paramName);

    // 从控件获取值
    QVariant getValueFromWidget(QWidget* widget) const;

    // 设置控件值
    void setValueToWidget(QWidget* widget, const QVariant& value);

public:
    DAPyNodeProxy* mNodeProxy { nullptr };              ///< Python节点代理
    QFormLayout* mFormLayout { nullptr };               ///< 表单布局
    QVBoxLayout* mMainLayout { nullptr };               ///< 主布局
    QHash<QString, QWidget*> mParamWidgets;             ///< 参数名到控件的映射
    QHash<QString, QJsonObject> mParamDefinitions;      ///< 参数定义缓存
    bool mIsModified { false };                         ///< 是否已修改
};

/**
 * @brief 构造函数
 * @param[in] p 父对象指针
 */
DAPyNodeWidget::PrivateData::PrivateData(DAPyNodeWidget* p) : q_ptr(p)
{
}

/**
 * @brief 析构函数
 */
DAPyNodeWidget::PrivateData::~PrivateData()
{
    clearWidgets();
}

/**
 * @brief 清理所有参数控件
 */
void DAPyNodeWidget::PrivateData::clearWidgets()
{
    mParamWidgets.clear();
    mParamDefinitions.clear();
}

/**
 * @brief 创建参数控件
 * @param[in] paramDef 参数定义JSON对象
 * @param[in] paramName 参数名称
 * @return 创建的控件指针
 */
QWidget* DAPyNodeWidget::PrivateData::createParameterWidget(const QJsonObject& paramDef, const QString& paramName)
{
    Q_UNUSED(paramName)

    QString typeName = paramDef.value("type").toString("str");
    QWidget* widget = DAPyParamTypeHelper::createWidgetForParamType(typeName, q_ptr);

    if (!widget) {
        // 未知类型，创建文本输入框作为回退
        widget = new QLineEdit(q_ptr);
    }

    // 设置默认值
    if (paramDef.contains("default")) {
        QJsonValue defaultValue = paramDef.value("default");
        QVariant variantValue;

        if (defaultValue.isString()) {
            variantValue = defaultValue.toString();
        } else if (defaultValue.isDouble()) {
            variantValue = defaultValue.toDouble();
        } else if (defaultValue.isBool()) {
            variantValue = defaultValue.toBool();
        } else if (defaultValue.isArray()) {
            QJsonArray arr = defaultValue.toArray();
            QStringList list;
            for (const auto& val : arr) {
                list.append(val.toString());
            }
            variantValue = list;
        }

        setValueToWidget(widget, variantValue);
    }

    // 设置占位文本（如果有描述）
    if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
        QString desc = paramDef.value("description").toString();
        if (!desc.isEmpty()) {
            edit->setPlaceholderText(desc);
        }
    }

    // 设置枚举值（如果是enum类型）
    if (typeName == "enum" || typeName == "enumeration") {
        if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
            QJsonArray enumValues = paramDef.value("enum_values").toArray();
            for (const auto& val : enumValues) {
                combo->addItem(val.toString());
            }
            // 如果有默认值，选中它
            if (paramDef.contains("default")) {
                QString defaultStr = paramDef.value("default").toString();
                int idx = combo->findText(defaultStr);
                if (idx >= 0) {
                    combo->setCurrentIndex(idx);
                }
            }
        }
    }

    // 设置数值范围
    if (typeName == "int" || typeName == "integer") {
        if (QSpinBox* spin = qobject_cast<QSpinBox*>(widget)) {
            int minVal = paramDef.value("min").toInt(-9999);
            int maxVal = paramDef.value("max").toInt(9999);
            spin->setRange(minVal, maxVal);
        }
    } else if (typeName == "float" || typeName == "double") {
        if (QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>(widget)) {
            double minVal = paramDef.value("min").toDouble(-9999.0);
            double maxVal = paramDef.value("max").toDouble(9999.0);
            int decimals = paramDef.value("decimals").toInt(2);
            spin->setRange(minVal, maxVal);
            spin->setDecimals(decimals);
        }
    }

    return widget;
}

/**
 * @brief 从控件获取值
 * @param[in] widget 控件指针
 * @return 控件当前值
 */
QVariant DAPyNodeWidget::PrivateData::getValueFromWidget(QWidget* widget) const
{
    return DAPyParamTypeHelper::getValueFromWidget(widget);
}

/**
 * @brief 设置控件值
 * @param[in] widget 控件指针
 * @param[in] value 要设置的值
 */
void DAPyNodeWidget::PrivateData::setValueToWidget(QWidget* widget, const QVariant& value)
{
    DAPyParamTypeHelper::setValueToWidget(widget, value);
}

//===================================================
// DAPyNodeWidget
//===================================================

/**
 * @brief 构造函数
 * @param[in] parent 父控件
 */
DAPyNodeWidget::DAPyNodeWidget(QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->mMainLayout = new QVBoxLayout(this);
    d_ptr->mFormLayout = new QFormLayout();
    d_ptr->mMainLayout->addLayout(d_ptr->mFormLayout);
    d_ptr->mMainLayout->addStretch();

    setWindowTitle(tr("节点参数配置"));
}

/**
 * @brief 构造函数（带节点代理）
 * @param[in] proxy Python节点代理
 * @param[in] parent 父控件
 */
DAPyNodeWidget::DAPyNodeWidget(DAPyNodeProxy* proxy, QWidget* parent)
    : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    d_ptr->mMainLayout = new QVBoxLayout(this);
    d_ptr->mFormLayout = new QFormLayout();
    d_ptr->mMainLayout->addLayout(d_ptr->mFormLayout);
    d_ptr->mMainLayout->addStretch();

    setNodeProxy(proxy);
    setWindowTitle(tr("节点参数配置"));
}

/**
 * @brief 析构函数
 */
DAPyNodeWidget::~DAPyNodeWidget()
{
}

/**
 * @brief 设置节点代理
 * @param[in] proxy Python节点代理指针
 */
void DAPyNodeWidget::setNodeProxy(DAPyNodeProxy* proxy)
{
    d_ptr->mNodeProxy = proxy;
    if (proxy) {
        // 从代理获取描述符并构建控件
        QJsonObject descriptor = proxy->getDescriptor();
        QJsonArray params = descriptor.value("parameters").toArray();
        if (!params.isEmpty()) {
            pybind11::dict paramDict;
            for (const auto& val : params) {
                QJsonObject paramObj = val.toObject();
                QString name = paramObj.value("name").toString();
                if (!name.isEmpty()) {
                    // 转换为pybind11::dict
                    pybind11::dict paramDef;
                    for (auto it = paramObj.begin(); it != paramObj.end(); ++it) {
                        QString key = it.key();
                        QJsonValue jsonVal = it.value();
                        if (jsonVal.isString()) {
                            paramDef[key.toStdString().c_str()] = pybind11::str(jsonVal.toString().toStdString());
                        } else if (jsonVal.isBool()) {
                            paramDef[key.toStdString().c_str()] = pybind11::bool_(jsonVal.toBool());
                        } else if (jsonVal.isDouble()) {
                            paramDef[key.toStdString().c_str()] = pybind11::float_(jsonVal.toDouble());
                        } else if (jsonVal.isArray()) {
                            pybind11::list pyList;
                            QJsonArray arr = jsonVal.toArray();
                            for (const auto& item : arr) {
                                pyList.append(pybind11::str(item.toString().toStdString()));
                            }
                            paramDef[key.toStdString().c_str()] = pyList;
                        }
                    }
                    paramDict[name.toStdString().c_str()] = paramDef;
                }
            }
            buildWidgetsFromDescriptor(paramDict);
        }
    }
}

/**
 * @brief 获取节点代理
 * @return Python节点代理指针
 */
DAPyNodeProxy* DAPyNodeWidget::getNodeProxy() const
{
    return d_ptr->mNodeProxy;
}

/**
 * @brief 根据参数描述符动态构建控件
 *
 * 解析Python字典格式的参数定义，为每个参数创建对应的Qt控件。
 * 参数定义格式：
 * {
 *   "param_name": {
 *     "type": "str|int|float|bool|enum|list",
 *     "default": <value>,
 *     "description": "...",
 *     "enum_values": [...],  // 仅enum类型
 *     "min": <value>,        // 仅int/float类型
 *     "max": <value>,        // 仅int/float类型
 *     "decimals": <int>      // 仅float类型
 *   }
 * }
 *
 * @param[in] paramDict Python字典格式的参数定义
 */
void DAPyNodeWidget::buildWidgetsFromDescriptor(const pybind11::dict& paramDict)
{
    // 清空现有控件
    clearParameterWidgets();

    DAPyGILGuard gil;
    if (!gil.isAcquired()) {
        qWarning() << "DAPyNodeWidget::buildWidgetsFromDescriptor: Failed to acquire GIL";
        return;
    }

    try {
        for (auto item : paramDict) {
            std::string nameKey = pybind11::str(item.first);
            QString paramName = QString::fromStdString(nameKey);
            pybind11::dict paramDef = pybind11::cast<pybind11::dict>(item.second);

            // 转换为QJsonObject
            QJsonObject paramObj;
            for (auto defItem : paramDef) {
                std::string key = pybind11::str(defItem.first);
                pybind11::object val = pybind11::cast<pybind11::object>(defItem.second);

                if (pybind11::isinstance<pybind11::str>(val)) {
                    paramObj[QString::fromStdString(key)] = QJsonValue(QString::fromStdString(pybind11::str(val)));
                } else if (pybind11::isinstance<pybind11::bool_>(val)) {
                    paramObj[QString::fromStdString(key)] = QJsonValue(pybind11::cast<bool>(val));
                } else if (pybind11::isinstance<pybind11::int_>(val)) {
                    paramObj[QString::fromStdString(key)] = QJsonValue(pybind11::cast<int>(val));
                } else if (pybind11::isinstance<pybind11::float_>(val)) {
                    paramObj[QString::fromStdString(key)] = QJsonValue(pybind11::cast<double>(val));
                } else if (pybind11::isinstance<pybind11::list>(val)) {
                    QJsonArray arr;
                    pybind11::list pyList = pybind11::cast<pybind11::list>(val);
                    for (auto listItem : pyList) {
                        arr.append(QJsonValue(QString::fromStdString(pybind11::str(listItem))));
                    }
                    paramObj[QString::fromStdString(key)] = arr;
                }
            }

            // 创建控件
            QWidget* widget = d_ptr->createParameterWidget(paramObj, paramName);
            if (widget) {
                // 连接值变化信号
                if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
                    connect(edit, &QLineEdit::textChanged, this, &DAPyNodeWidget::onParameterValueChanged);
                } else if (QSpinBox* spin = qobject_cast<QSpinBox*>(widget)) {
                    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &DAPyNodeWidget::onParameterValueChanged);
                } else if (QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>(widget)) {
                    connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DAPyNodeWidget::onParameterValueChanged);
                } else if (QCheckBox* check = qobject_cast<QCheckBox*>(widget)) {
                    connect(check, &QCheckBox::toggled, this, &DAPyNodeWidget::onParameterValueChanged);
                } else if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
                    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DAPyNodeWidget::onParameterValueChanged);
                }

                // 添加到表单布局
                QString labelText = paramName;
                QString desc = paramObj.value("description").toString();
                if (!desc.isEmpty()) {
                    labelText += QString(" (%1)").arg(desc);
                }
                d_ptr->mFormLayout->addRow(labelText + ":", widget);
                d_ptr->mParamWidgets[paramName] = widget;
                d_ptr->mParamDefinitions[paramName] = paramObj;
            }
        }
    } catch (const pybind11::error_already_set& e) {
        qWarning() << "DAPyNodeWidget::buildWidgetsFromDescriptor: Python error:" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeWidget::buildWidgetsFromDescriptor: Exception:" << e.what();
    }

    d_ptr->mIsModified = false;
}

/**
 * @brief 获取当前参数值（Python字典格式）
 * @return Python字典，键为参数名，值为当前控件值
 */
pybind11::dict DAPyNodeWidget::getParameterValuesAsDict() const
{
    pybind11::dict result;

    DAPyGILGuard gil;
    if (!gil.isAcquired()) {
        qWarning() << "DAPyNodeWidget::getParameterValuesAsDict: Failed to acquire GIL";
        return result;
    }

    try {
        for (auto it = d_ptr->mParamWidgets.begin(); it != d_ptr->mParamWidgets.end(); ++it) {
            QString paramName = it.key();
            QWidget* widget = it.value();
            QVariant value = d_ptr->getValueFromWidget(widget);

            // 转换为Python对象
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            if (value.typeId() == QMetaType::QString) {
#else
            if (value.type() == QMetaType::QString) {
#endif
                result[pybind11::str(paramName.toStdString())] = pybind11::str(value.toString().toStdString());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            } else if (value.typeId() == QMetaType::Int) {
#else
            } else if (value.type() == QMetaType::Int) {
#endif
                result[pybind11::str(paramName.toStdString())] = pybind11::int_(value.toInt());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            } else if (value.typeId() == QMetaType::Double) {
#else
            } else if (value.type() == QMetaType::Double) {
#endif
                result[pybind11::str(paramName.toStdString())] = pybind11::float_(value.toDouble());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            } else if (value.typeId() == QMetaType::Bool) {
#else
            } else if (value.type() == QMetaType::Bool) {
#endif
                result[pybind11::str(paramName.toStdString())] = pybind11::bool_(value.toBool());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            } else if (value.typeId() == QMetaType::QStringList) {
#else
            } else if (value.type() == QMetaType::QStringList) {
#endif
                pybind11::list pyList;
                QStringList list = value.toStringList();
                for (const QString& item : list) {
                    pyList.append(pybind11::str(item.toStdString()));
                }
                result[pybind11::str(paramName.toStdString())] = pyList;
            }
        }
    } catch (const pybind11::error_already_set& e) {
        qWarning() << "DAPyNodeWidget::getParameterValuesAsDict: Python error:" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeWidget::getParameterValuesAsDict: Exception:" << e.what();
    }

    return result;
}

/**
 * @brief 从Python字典设置参数值
 * @param[in] valuesDict Python字典，键为参数名，值为要设置的值
 */
void DAPyNodeWidget::setParameterValuesFromDict(const pybind11::dict& valuesDict)
{
    DAPyGILGuard gil;
    if (!gil.isAcquired()) {
        qWarning() << "DAPyNodeWidget::setParameterValuesFromDict: Failed to acquire GIL";
        return;
    }

    try {
        for (auto item : valuesDict) {
            std::string nameKey = pybind11::str(item.first);
            QString paramName = QString::fromStdString(nameKey);

            if (d_ptr->mParamWidgets.contains(paramName)) {
                QWidget* widget = d_ptr->mParamWidgets[paramName];
                pybind11::object val = pybind11::cast<pybind11::object>(item.second);

                QVariant variantValue;
                if (pybind11::isinstance<pybind11::str>(val)) {
                    variantValue = QString::fromStdString(pybind11::str(val));
                } else if (pybind11::isinstance<pybind11::bool_>(val)) {
                    variantValue = pybind11::cast<bool>(val);
                } else if (pybind11::isinstance<pybind11::int_>(val)) {
                    variantValue = pybind11::cast<int>(val);
                } else if (pybind11::isinstance<pybind11::float_>(val)) {
                    variantValue = pybind11::cast<double>(val);
                } else if (pybind11::isinstance<pybind11::list>(val)) {
                    QStringList list;
                    pybind11::list pyList = pybind11::cast<pybind11::list>(val);
                    for (auto listItem : pyList) {
                        list.append(QString::fromStdString(pybind11::str(listItem)));
                    }
                    variantValue = list;
                }

                d_ptr->setValueToWidget(widget, variantValue);
            }
        }
    } catch (const pybind11::error_already_set& e) {
        qWarning() << "DAPyNodeWidget::setParameterValuesFromDict: Python error:" << e.what();
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeWidget::setParameterValuesFromDict: Exception:" << e.what();
    }
}

/**
 * @brief 清空所有参数控件
 */
void DAPyNodeWidget::clearParameterWidgets()
{
    // 从布局中移除所有行
    while (d_ptr->mFormLayout->rowCount() > 0) {
        d_ptr->mFormLayout->removeRow(0);
    }

    d_ptr->clearWidgets();
    d_ptr->mIsModified = false;
}

/**
 * @brief 检查是否有任何参数
 * @return 有参数返回true，否则返回false
 */
bool DAPyNodeWidget::hasParameters() const
{
    return !d_ptr->mParamWidgets.isEmpty();
}

/**
 * @brief 获取参数数量
 * @return 参数数量
 */
int DAPyNodeWidget::getParameterCount() const
{
    return d_ptr->mParamWidgets.size();
}

/**
 * @brief 处理参数值变化
 */
void DAPyNodeWidget::onParameterValueChanged()
{
    d_ptr->mIsModified = true;
    emit parametersModified();

    // 获取变化的参数名
    QObject* senderObj = sender();
    for (auto it = d_ptr->mParamWidgets.begin(); it != d_ptr->mParamWidgets.end(); ++it) {
        if (it.value() == senderObj || it.value()->findChild<QWidget*>(QString(), Qt::FindDirectChildrenOnly) == senderObj) {
            emit parameterValueChanged(it.key(), d_ptr->getValueFromWidget(it.value()));
            break;
        }
    }
}

/**
 * @brief 创建列表类型控件的辅助UI（带+/-按钮）
 * @param[in] paramDef 参数定义
 * @return 包含列表和按钮的复合控件
 */
QWidget* DAPyNodeWidget::createListWidgetWithButtons(const QJsonObject& paramDef)
{
    Q_UNUSED(paramDef)

    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    QListWidget* listWidget = new QListWidget(container);
    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(listWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("+", container);
    QPushButton* removeBtn = new QPushButton("-", container);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // 连接按钮信号
    connect(addBtn, &QPushButton::clicked, this, [listWidget]() {
        listWidget->addItem("");
        listWidget->editItem(listWidget->item(listWidget->count() - 1));
    });

    connect(removeBtn, &QPushButton::clicked, this, [listWidget]() {
        QList<QListWidgetItem*> items = listWidget->selectedItems();
        for (QListWidgetItem* item : items) {
            delete listWidget->takeItem(listWidget->row(item));
        }
    });

    return container;
}

} // namespace DA