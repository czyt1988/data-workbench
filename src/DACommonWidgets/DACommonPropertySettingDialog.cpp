#include "DACommonPropertySettingDialog.h"
#include "ui_DACommonPropertySettingDialog.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
// Qt Property Browser
#include "qtpropertymanager.h"
#include "qttreepropertybrowser.h"
#include "qtvariantproperty.h"


namespace DA
{
class DACommonPropertySettingDialog::PrivateData
{
    DA_DECLARE_PUBLIC(DACommonPropertySettingDialog)
public:
    PrivateData(DACommonPropertySettingDialog* p);

    /**
     * @brief 初始化属性浏览器界面
     * @param propertyBrowser 属性浏览器控件指针
     */
    void setupUi(QtTreePropertyBrowser* propertyBrowser);

    /**
     * @brief 创建属性
     * @param propConfig 属性配置对象
     * @param parentGroup 父分组属性，如果为 nullptr 则创建顶层属性
     * @note 根据 type 字段决定创建分组属性还是值属性
     */
    void createProperty(const QJsonObject& propConfig, QtProperty* parentGroup = nullptr);

    /**
     * @brief 创建分组属性
     * @param propConfig 分组属性配置
     * @return 创建的 QtProperty 指针
     */
    QtProperty* createGroupProperty(const QJsonObject& propConfig);

    /**
     * @brief 创建值属性
     * @param propConfig 值属性配置
     * @return 创建的 QtVariantProperty 指针，失败返回 nullptr
     * @note 支持的类型：string, int, double, bool, color, font, enum, file, folder, stringlist
     */
    QtVariantProperty* createVariantProperty(const QJsonObject& propConfig);

    /**
     * @brief 清理所有属性
     * @note 删除所有属性并清空映射关系
     */
    void clearProperties();

    /**
     * @brief 验证属性配置
     * @param propConfig 属性配置对象
     * @return 如果配置有效返回 true，否则返回 false
     */
    bool validatePropertyConfig(const QJsonObject& propConfig);

    /**
     * @brief 获取支持的属性类型列表
     * @return 支持的类型字符串列表
     */
    QStringList getSupportedTypes() const;

    /**
     * @brief 获取枚举属性的显示文本列表
     * @param propConfig 属性配置对象
     * @return 显示文本列表
     * @note 如果配置了 enum_descriptions，则使用 enum_descriptions，
     *       否则使用 enum_items 作为显示文本
     */
    QStringList getEnumDisplayTexts(const QJsonObject& propConfig) const;

public:
    QtVariantPropertyManager* m_variantManager { nullptr };  ///< 变体属性管理器
    QtGroupPropertyManager* m_groupManager { nullptr };      ///< 分组属性管理器
    QtTreePropertyBrowser* m_propertyBrowser { nullptr };    ///< 属性浏览器控件
    QtVariantEditorFactory* m_variantFactory { nullptr };    ///< 变体编辑器工厂

    QHash< QtProperty*, QString > m_propertyNameMap;   ///< 属性对象到名称的映射
    QHash< QString, QtProperty* > m_namePropertyMap;   ///< 名称到属性对象的映射
    QHash< QString, QVariant > m_defaultValues;        ///< 默认值存储
    QHash< QtProperty*, QStringList > m_enumValueMap;  ///< 枚举属性到实际值的映射
    QHash< QString, QVariant > m_propertyRawValues;    ///< 属性原始值存储（用于枚举）
};


DACommonPropertySettingDialog::PrivateData::PrivateData(DACommonPropertySettingDialog* p) : q_ptr(p)
{
}

void DACommonPropertySettingDialog::PrivateData::setupUi(QtTreePropertyBrowser* propertyBrowser)
{
    m_propertyBrowser = propertyBrowser;
    m_variantManager  = new QtVariantPropertyManager(propertyBrowser);
    m_groupManager    = new QtGroupPropertyManager(propertyBrowser);
    m_variantFactory  = new QtVariantEditorFactory(propertyBrowser);

    propertyBrowser->setFactoryForManager(m_variantManager, m_variantFactory);
    propertyBrowser->setAlternatingRowColors(true);
    propertyBrowser->setPropertiesWithoutValueMarked(true);
    propertyBrowser->setRootIsDecorated(true);
}

bool DACommonPropertySettingDialog::PrivateData::validatePropertyConfig(const QJsonObject& propConfig)
{
    if (!propConfig.contains("name")) {
        qDebug() << "Property config missing 'name' field";
        return false;
    }

    if (!propConfig.contains("type")) {
        qDebug() << "Property config missing 'type' field for property:" << propConfig[ "name" ].toString();
        return false;
    }

    QString type               = propConfig[ "type" ].toString();
    QStringList supportedTypes = getSupportedTypes();

    if (!supportedTypes.contains(type)) {
        qDebug() << "Unsupported property type:" << type << "for property:" << propConfig[ "name" ].toString();
        return false;
    }

    // 验证特定类型需要的字段
    if (type == "int" || type == "double") {
        if (propConfig.contains("min") && propConfig.contains("max")) {
            double minVal = propConfig[ "min" ].toDouble();
            double maxVal = propConfig[ "max" ].toDouble();
            if (minVal > maxVal) {
                qDebug() << "Invalid range: min > max for property:" << propConfig[ "name" ].toString();
                return false;
            }

            if (propConfig.contains("value")) {
                double value = propConfig[ "value" ].toDouble();
                if (value < minVal || value > maxVal) {
                    qDebug() << "Default value out of range for property:" << propConfig[ "name" ].toString();
                    return false;
                }
            }
        }
    }

    // 验证枚举类型
    if (type == "enum") {
        if (!propConfig.contains("enum_items")) {
            qDebug() << "Enum property missing 'enum_items' field:" << propConfig[ "name" ].toString();
            return false;
        }

        QJsonArray enumItems = propConfig[ "enum_items" ].toArray();
        if (enumItems.isEmpty()) {
            qDebug() << "Enum items array is empty for property:" << propConfig[ "name" ].toString();
            return false;
        }

        // 验证 enum_descriptions 如果存在
        if (propConfig.contains("enum_descriptions")) {
            QJsonArray enumDescriptions = propConfig[ "enum_descriptions" ].toArray();
            if (enumDescriptions.size() != enumItems.size()) {
                qDebug() << "enum_descriptions size does not match enum_items size for property:"
                         << propConfig[ "name" ].toString();
                return false;
            }
        }
    }

    return true;
}

QStringList DACommonPropertySettingDialog::PrivateData::getSupportedTypes() const
{
    return { "group", "string", "int", "double", "bool", "color", "font", "enum", "file", "folder", "stringlist" };
}

QStringList DACommonPropertySettingDialog::PrivateData::getEnumDisplayTexts(const QJsonObject& propConfig) const
{
    QStringList displayTexts;

    if (propConfig.contains("enum_descriptions")) {
        QJsonArray descriptions = propConfig[ "enum_descriptions" ].toArray();
        for (const QJsonValue& desc : descriptions) {
            displayTexts.append(desc.toString());
        }
    } else {
        // 如果没有描述，使用枚举值作为显示文本
        QJsonArray enumItems = propConfig[ "enum_items" ].toArray();
        for (const QJsonValue& item : enumItems) {
            displayTexts.append(item.toString());
        }
    }

    return displayTexts;
}

void DACommonPropertySettingDialog::PrivateData::clearProperties()
{
    QList< QtProperty* > properties = m_propertyBrowser->properties();
    for (QtProperty* prop : properties) {
        m_propertyBrowser->removeProperty(prop);
        delete prop;
    }
    m_propertyNameMap.clear();
    m_namePropertyMap.clear();
    m_defaultValues.clear();
    m_enumValueMap.clear();
    m_propertyRawValues.clear();
}

void DACommonPropertySettingDialog::PrivateData::createProperty(const QJsonObject& propConfig, QtProperty* parentGroup)
{
    // 验证配置
    if (!validatePropertyConfig(propConfig)) {
        return;
    }

    QString type        = propConfig[ "type" ].toString();
    QString name        = propConfig[ "name" ].toString();
    QString displayName = propConfig.contains("display_name") ? propConfig[ "display_name" ].toString() : name;

    QtProperty* property = nullptr;

    if (type == "group") {
        property = createGroupProperty(propConfig);
    } else {
        property = createVariantProperty(propConfig);
    }

    if (!property) {
        return;
    }

    property->setPropertyName(displayName);

    // 设置描述
    if (propConfig.contains("description")) {
        property->setToolTip(propConfig[ "description" ].toString());
        property->setStatusTip(propConfig[ "description" ].toString());
    }

    // 设置是否只读
    if (propConfig.contains("read_only") && propConfig[ "read_only" ].toBool()) {
        property->setEnabled(false);
    }

    // 添加到浏览器
    if (parentGroup) {
        parentGroup->addSubProperty(property);
    } else {
        m_propertyBrowser->addProperty(property);
    }

    // 保存映射关系
    if (type != "group") {
        m_propertyNameMap[ property ] = name;
        m_namePropertyMap[ name ]     = property;

        // 保存默认值
        if (propConfig.contains("value")) {
            QVariant defaultValue;
            if (type == "enum") {
                // 处理枚举类型的默认值
                QString enumValue    = propConfig[ "value" ].toString();
                QJsonArray enumItems = propConfig[ "enum_items" ].toArray();

                // 存储实际值列表
                QStringList actualValues;
                for (const QJsonValue& item : enumItems) {
                    actualValues.append(item.toString());
                }
                m_enumValueMap[ property ] = actualValues;

                // 找到默认值在列表中的索引
                int index = actualValues.indexOf(enumValue);
                if (index < 0) {
                    index = 0;
                    qDebug() << "Default value not found in enum items, using first item for property:" << name;
                }

                defaultValue                = index;
                m_propertyRawValues[ name ] = actualValues[ index ];
            } else {
                // 对于变体属性，使用属性管理器获取值
                QtVariantProperty* variantProp = dynamic_cast< QtVariantProperty* >(property);
                if (variantProp) {
                    defaultValue = m_variantManager->value(property);
                } else {
                    // 如果没有值，使用默认值
                    if (type == "string") {
                        defaultValue = propConfig[ "value" ].toString("");
                    } else if (type == "int") {
                        defaultValue = propConfig[ "value" ].toInt(0);
                    } else if (type == "double") {
                        defaultValue = propConfig[ "value" ].toDouble(0.0);
                    } else if (type == "bool") {
                        defaultValue = propConfig[ "value" ].toBool(false);
                    } else if (type == "color") {
                        defaultValue = QColor(propConfig[ "value" ].toString("#000000"));
                    } else if (type == "font") {
                        QFont font;
                        font.fromString(propConfig[ "value" ].toString("Arial,12,-1,5,50,0,0,0,0,0"));
                        defaultValue = font;
                    } else if (type == "file" || type == "folder") {
                        defaultValue = propConfig[ "value" ].toString("");
                    } else if (type == "stringlist") {
                        QJsonArray array = propConfig[ "value" ].toArray();
                        QStringList stringList;
                        for (const QJsonValue& item : array) {
                            stringList.append(item.toString());
                        }
                        defaultValue = stringList;
                    }
                }
            }
            m_defaultValues[ name ] = defaultValue;
        }
    }
}

QtProperty* DACommonPropertySettingDialog::PrivateData::createGroupProperty(const QJsonObject& propConfig)
{
    QtProperty* group = m_groupManager->addProperty(propConfig[ "name" ].toString());

    if (propConfig.contains("properties")) {
        QJsonArray subProps = propConfig[ "properties" ].toArray();
        for (const QJsonValue& subPropValue : subProps) {
            QJsonObject subPropConfig = subPropValue.toObject();
            createProperty(subPropConfig, group);
        }
    }

    return group;
}

QtVariantProperty* DACommonPropertySettingDialog::PrivateData::createVariantProperty(const QJsonObject& propConfig)
{
    QString type = propConfig[ "type" ].toString();
    QString name = propConfig[ "name" ].toString();
    QVariant value;

    int variantType = QMetaType::UnknownType;

    // 根据类型设置variantType和value
    if (type == "string") {
        variantType = QMetaType::QString;
        value       = propConfig[ "value" ].toString("");
    } else if (type == "int") {
        variantType = QMetaType::Int;
        value       = propConfig[ "value" ].toInt(0);
    } else if (type == "double") {
        variantType = QMetaType::Double;
        value       = propConfig[ "value" ].toDouble(0.0);
    } else if (type == "bool") {
        variantType = QMetaType::Bool;
        value       = propConfig[ "value" ].toBool(false);
    } else if (type == "color") {
        variantType      =QMetaType::QColor;
        QString colorStr = propConfig[ "value" ].toString("#000000");
        value            = QColor(colorStr);
    } else if (type == "font") {
        variantType     = QMetaType::QFont;
        QString fontStr = propConfig[ "value" ].toString("Arial,12,-1,5,50,0,0,0,0,0");
        QFont font;
        font.fromString(fontStr);
        value = font;
    } else if (type == "enum") {
        variantType = QtVariantPropertyManager::enumTypeId();

        // 获取实际值和显示文本
        QJsonArray enumItems = propConfig[ "enum_items" ].toArray();
        QStringList actualValues;
        for (const QJsonValue& item : enumItems) {
            actualValues.append(item.toString());
        }

        QStringList displayNames = getEnumDisplayTexts(propConfig);
        QString enumValue        = propConfig[ "value" ].toString();

        int currentIndex = actualValues.indexOf(enumValue);
        if (currentIndex < 0) {
            currentIndex = 0;
            qDebug() << "Default value not found in enum items, using first item for property:" << name;
        }

        value = currentIndex;

        QtVariantProperty* property = m_variantManager->addProperty(variantType, name);
        if (property) {
            property->setAttribute("enumNames", displayNames);
            property->setValue(value);

            // 设置其他属性
            if (propConfig.contains("min")) {
                property->setAttribute("minimum", propConfig[ "min" ].toInt());
            }
            if (propConfig.contains("max")) {
                property->setAttribute("maximum", propConfig[ "max" ].toInt());
            }

            return property;
        }
    } else if (type == "file") {
        variantType = QMetaType::QString;
        value       = propConfig[ "value" ].toString("");
    } else if (type == "folder") {
        variantType = QMetaType::QString;
        value       = propConfig[ "value" ].toString("");
    } else if (type == "stringlist") {
        variantType      = QMetaType::QStringList;
        QJsonArray array = propConfig[ "value" ].toArray();
        QStringList stringList;
        for (const QJsonValue& item : array) {
            stringList.append(item.toString());
        }
        value = stringList;
    }

    // 创建属性
    QtVariantProperty* property = m_variantManager->addProperty(variantType, name);
    if (!property) {
        qDebug() << "Failed to create property:" << name << "type:" << type;
        return nullptr;
    }

    property->setValue(value);

    // 设置属性约束
    if (type == "int" || type == "double") {
        if (propConfig.contains("min")) {
            property->setAttribute("minimum", propConfig[ "min" ].toDouble());
        }
        if (propConfig.contains("max")) {
            property->setAttribute("maximum", propConfig[ "max" ].toDouble());
        }
        if (propConfig.contains("decimals")) {
            property->setAttribute("decimals", propConfig[ "decimals" ].toInt());
        }
        if (propConfig.contains("singleStep")) {
            property->setAttribute("singleStep", propConfig[ "singleStep" ].toDouble());
        }
    }

    return property;
}

//===============================================================
// DACommonPropertySettingDialog 实现
//===============================================================

DACommonPropertySettingDialog::DACommonPropertySettingDialog(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DACommonPropertySettingDialog)
{
    ui->setupUi(this);
    d_ptr->setupUi(ui->propertyBrowser);
    connect(d_ptr->m_variantManager,
            &QtVariantPropertyManager::valueChanged,
            this,
            &DACommonPropertySettingDialog::onPropertyValueChanged);
}

DACommonPropertySettingDialog::~DACommonPropertySettingDialog()
{
	// 先断开所有信号连接
	if (d_ptr->m_variantManager) {
		disconnect(d_ptr->m_variantManager, nullptr, this, nullptr);
	}

	// 清理属性管理器
	if (d_ptr->m_variantManager) {
		// 清理所有属性
		d_ptr->m_variantManager->clear();
	}
	// 清理属性浏览器
	if (ui->propertyBrowser) {
		// 阻塞浏览器信号
		ui->propertyBrowser->blockSignals(true);

		// 移除所有属性
		QList< QtProperty* > properties = ui->propertyBrowser->properties();
		for (QtProperty* prop : properties) {
			ui->propertyBrowser->removeProperty(prop);
		}
	}
    delete ui;
}

/**
 * @brief 从 JSON 字符串加载配置
 * @param jsonStr JSON 格式的配置字符串
 * @return 如果加载成功返回 true，否则返回 false
 * @note 如果 JSON 格式错误或解析失败，会输出警告信息并返回 false
 * @see loadFromJsonObject()
 * @code{.cpp}
 * QString json = R"({
 *     "window_title": "测试",
 *     "properties": [{
 *         "name": "param1",
 *         "type": "string",
 *         "value": "test"
 *     }]
 * })";
 * dialog.loadFromJson(json);
 * @endcode
 */
bool DACommonPropertySettingDialog::loadFromJson(const QString& jsonStr)
{
    if (jsonStr.isEmpty()) {
        qDebug() << tr("JSON string is empty");
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << tr("Config json parse error:%1").arg(parseError.errorString());
        return false;
    }

    return loadFromJsonObject(doc.object());
}

/**
 * @brief 从 QJsonObject 加载配置
 * @param jsonObj JSON 配置对象
 * @return 如果加载成功返回 true，否则返回 false
 * @note 此方法会清空现有的所有属性并重新创建
 * @see loadFromJson()
 */
bool DACommonPropertySettingDialog::loadFromJsonObject(const QJsonObject& jsonObj)
{
    if (jsonObj.isEmpty()) {
        qDebug() << tr("JSON object is empty");
        return false;
    }

    DA_D(d);

    // 清空现有属性
    d->clearProperties();

    // 设置窗口标题
    if (jsonObj.contains("window_title")) {
        setWindowTitle(jsonObj[ "window_title" ].toString());
    }

    // 创建属性
    if (jsonObj.contains("properties")) {
        QJsonArray propertiesArray = jsonObj[ "properties" ].toArray();
        if (propertiesArray.isEmpty()) {
            qDebug() << tr("Properties array is empty");
            return false;
        }

        for (const QJsonValue& propValue : propertiesArray) {
            QJsonObject propConfig = propValue.toObject();
            d->createProperty(propConfig);
        }
    } else {
        qDebug() << tr("Missing 'properties' field in JSON config");
        return false;
    }

    return true;
}

/**
 * @brief 获取所有属性的当前值
 * @return 包含所有属性名称和值的 JSON 对象
 * @note 枚举类型返回字符串值，颜色返回十六进制字符串，字体返回字体字符串
 * @see getValue()
 * @code{.cpp}
 * QJsonObject values = dialog.getCurrentValues();
 * QString jsonStr = QJsonDocument(values).toJson();
 * // 可以传递给 Python 端
 * @endcode
 */
QJsonObject DACommonPropertySettingDialog::getCurrentValues() const
{
    DA_DC(d);
    QJsonObject result;

    for (auto it = d->m_namePropertyMap.constBegin(); it != d->m_namePropertyMap.constEnd(); ++it) {
        QString propertyName = it.key();
        QtProperty* property = it.value();

        QVariant value = d->m_variantManager->value(property);

        // 处理枚举类型
        if (d->m_variantManager->propertyType(property) == QtVariantPropertyManager::enumTypeId()) {
            // 从映射中获取实际值
            QStringList actualValues = d->m_enumValueMap.value(property);
            int enumIndex            = value.toInt();
            if (enumIndex >= 0 && enumIndex < actualValues.size()) {
                result[ propertyName ] = actualValues[ enumIndex ];
            } else {
                qDebug() << "Invalid enum index for property:" << propertyName;
                result[ propertyName ] = "";
            }
        } else {
            // 处理其他类型
            int type = value.typeId();
            if (type == QMetaType::QColor) {
                result[ propertyName ] = value.value< QColor >().name();
            } else if (type == QMetaType::QFont) {
                result[ propertyName ] = value.value< QFont >().toString();
            } else if (type == QMetaType::QStringList) {
                QJsonArray array;
                QStringList list = value.toStringList();
                for (const QString& item : list) {
                    array.append(item);
                }
                result[ propertyName ] = array;
            } else {
                result[ propertyName ] = QJsonValue::fromVariant(value);
            }
        }
    }

    return result;
}

/**
 * @brief 获取特定属性的当前值
 * @param propertyName 属性名称（JSON 配置中的 name 字段）
 * @return 属性的 QVariant 值，如果属性不存在返回无效的 QVariant
 * @see getCurrentValues()
 * @code{.cpp}
 * QVariant value = dialog.getValue("threshold");
 * if (value.isValid()) {
 *     double threshold = value.toDouble();
 * }
 * @endcode
 */
QVariant DACommonPropertySettingDialog::getValue(const QString& propertyName) const
{
    DA_DC(d);
    QtProperty* property = d->m_namePropertyMap.value(propertyName);
    if (!property) {
        qDebug() << tr("Property not found:") << propertyName;
        return QVariant();
    }

    QVariant value = d->m_variantManager->value(property);

    // 处理枚举类型
    if (d->m_variantManager->propertyType(property) == QtVariantPropertyManager::enumTypeId()) {
        QStringList actualValues = d->m_enumValueMap.value(property);
        int enumIndex            = value.toInt();
        if (enumIndex >= 0 && enumIndex < actualValues.size()) {
            return actualValues[ enumIndex ];
        } else {
            return QVariant();
        }
    }

    return value;
}

/**
 * @brief 设置特定属性的值
 * @param propertyName 属性名称
 * @param value 要设置的值
 * @return 如果设置成功返回 true，否则返回 false
 * @note 可以在对话框显示前预置某些属性的值
 */
bool DACommonPropertySettingDialog::setValue(const QString& propertyName, const QVariant& value)
{
    DA_D(d);
    QtProperty* property = d->m_namePropertyMap.value(propertyName);
    if (!property) {
        qDebug() << tr("Property not found:") << propertyName;
        return false;
    }

    // 验证值是否有效
    if (!value.isValid()) {
        qDebug() << tr("Invalid value for property:") << propertyName;
        return false;
    }

    // 处理枚举类型
    if (d->m_variantManager->propertyType(property) == QtVariantPropertyManager::enumTypeId()) {
        QStringList actualValues = d->m_enumValueMap.value(property);
        QString actualValue      = value.toString();
        int index                = actualValues.indexOf(actualValue);
        if (index < 0) {
            qDebug() << tr("Value not found in enum items for property:") << propertyName;
            return false;
        }
        d->m_variantManager->setValue(property, index);
        return true;
    }

    // 检查类型是否匹配
    QVariant currentValue = d->m_variantManager->value(property);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (currentValue.type() != value.type()
        && !(currentValue.canConvert(value.type()) || value.canConvert(currentValue.type())))
#else
    if (currentValue.metaType() != value.metaType()
        && !(currentValue.canConvert(value.metaType()) || value.canConvert(currentValue.metaType())))
#endif
    {
        qDebug() << tr("Type mismatch for property:") << propertyName << "expected:" << currentValue.typeName()
                 << "got:" << value.typeName();
        return false;
    }

    d->m_variantManager->setValue(property, value);
    return true;
}

/**
 * @brief 重置所有属性为默认值
 * @note 默认值来自 JSON 配置中的 value 字段
 */
void DACommonPropertySettingDialog::resetToDefaults()
{
    DA_D(d);
    for (auto it = d->m_defaultValues.constBegin(); it != d->m_defaultValues.constEnd(); ++it) {
        QString propertyName = it.key();
        QtProperty* property = d->m_namePropertyMap.value(propertyName);
        if (property) {
            d->m_variantManager->setValue(property, it.value());

            // 更新枚举类型的原始值存储
            if (d->m_variantManager->propertyType(property) == QtVariantPropertyManager::enumTypeId()) {
                QStringList actualValues = d->m_enumValueMap.value(property);
                int index                = it.value().toInt();
                if (index >= 0 && index < actualValues.size()) {
                    d->m_propertyRawValues[ propertyName ] = actualValues[ index ];
                }
            }
        }
    }
}

/**
 * @brief 静态方法：显示设置对话框并获取结果
 * @param jsonConfig JSON 配置字符串
 * @param parent 父窗口指针，默认为 nullptr
 * @param defaultTitle 默认窗口标题，如果 JSON 中没有指定则使用此标题
 * @return 包含用户设置结果的 JSON 对象，如果用户取消则为空对象
 * @note 这是一个方便的静态方法，适合简单场景使用
 * @code{.cpp}
 * QJsonObject result = DACommonPropertySettingDialog::showSettingsDialog(
 *     jsonConfig, this, "参数设置");
 * if (!result.isEmpty()) {
 *     // 用户点击了确定
 * }
 * @endcode
 */
QJsonObject DACommonPropertySettingDialog::showSettingsDialog(const QString& jsonConfig, QWidget* parent)
{
    DACommonPropertySettingDialog dialog(parent);
    if (!dialog.loadFromJson(jsonConfig)) {
        qDebug() << tr("Failed to load JSON config for settings dialog");
        return QJsonObject();
    }

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.getCurrentValues();
    }

    return QJsonObject();
}

void DACommonPropertySettingDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

/**
 * @brief 处理属性值变更的槽函数
 * @param property QtProperty 对象
 * @param value 新的属性值
 */
void DACommonPropertySettingDialog::onPropertyValueChanged(QtProperty* property, const QVariant& value)
{
    DA_D(d);
    QString propertyName = d->m_propertyNameMap.value(property);
    if (!propertyName.isEmpty()) {
        // 处理枚举类型
        if (d->m_variantManager->propertyType(property) == QtVariantPropertyManager::enumTypeId()) {
            QStringList actualValues = d->m_enumValueMap.value(property);
            int index                = value.toInt();
            if (index >= 0 && index < actualValues.size()) {
                d->m_propertyRawValues[ propertyName ] = actualValues[ index ];
                Q_EMIT propertyValueChanged(propertyName, actualValues[ index ]);
            }
        } else {
            Q_EMIT propertyValueChanged(propertyName, value);
        }
    }
}

}  // end namespace DA
