#include "tst_param_type_registry.h"
#include "DAParamTypeRegistry.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QFontComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

using namespace DA;

/**
 * @brief 辅助方法，构造参数描述符 JSON 对象
 *
 * @param[in] type 参数类型字符串
 * @param[in] description 参数描述（可选）
 * @param[in] defaultValue 默认值（可选）
 * @param[in] extensions 扩展字段（如 enum_values, min, max, decimals 等）
 * @return 构造好的 QJsonObject
 */
QJsonObject TestDAParamTypeRegistry::makeDescriptor(const QString& type,
                                                     const QString& description,
                                                     const QVariant& defaultValue,
                                                     const QJsonObject& extensions) const
{
    QJsonObject obj;
    obj["name"]      = "test_param";
    obj["type"]      = type;
    obj["description"] = description;
    if (defaultValue.isValid()) {
        obj["default"] = QJsonValue::fromVariant(defaultValue);
    }
    for (auto it = extensions.constBegin(); it != extensions.constEnd(); ++it) {
        obj[it.key()] = it.value();
    }
    return obj;
}

/**
 * @brief 测试初始化
 *
 * 确认 DAParamTypeRegistry 实例可以正常创建，且默认注册了 11 种类型。
 */
void TestDAParamTypeRegistry::initTestCase()
{
    DAParamTypeRegistry registry;
    QVERIFY(!registry.supportedTypes().isEmpty());
    QCOMPARE(registry.supportedTypes().size(), 11);
}

/**
 * @brief 验证 11 种内置类型的编辑器创建与控件类型
 *
 * 依次对 str/int/float/bool/enum/list/file/folder/color/font/code
 * 调用 createEditor()，检查返回指针非空，并验证控件类型正确。
 */
void TestDAParamTypeRegistry::test_registerAndCreate_all11Types()
{
    DAParamTypeRegistry registry;

    // str → QLineEdit
    {
        QJsonObject desc = makeDescriptor("str", "输入文本");
        QWidget* editor  = registry.createEditor("str", desc);
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QLineEdit*>(editor) != nullptr);
        delete editor;
    }

    // int → QSpinBox
    {
        QJsonObject desc = makeDescriptor("int", "整数输入");
        QWidget* editor  = registry.createEditor("int", desc);
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QSpinBox*>(editor) != nullptr);
        delete editor;
    }

    // float → QDoubleSpinBox
    {
        QJsonObject desc = makeDescriptor("float", "浮点输入");
        QWidget* editor  = registry.createEditor("float", desc);
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QDoubleSpinBox*>(editor) != nullptr);
        delete editor;
    }

    // bool → QCheckBox
    {
        QJsonObject desc = makeDescriptor("bool", "布尔开关");
        QWidget* editor  = registry.createEditor("bool", desc);
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QCheckBox*>(editor) != nullptr);
        delete editor;
    }

    // enum → QComboBox
    {
        QJsonArray enumVals;
        enumVals.append("optionA");
        enumVals.append("optionB");
        QJsonObject ext;
        ext["enum_values"] = enumVals;
        QJsonObject desc = makeDescriptor("enum", "枚举选择", QVariant(), ext);
        QWidget* editor  = registry.createEditor("enum", desc);
        QVERIFY(editor != nullptr);
        QComboBox* combo = qobject_cast<QComboBox*>(editor);
        QVERIFY(combo != nullptr);
        QCOMPARE(combo->count(), 2);
        delete editor;
    }

    // list → composite widget (QListWidget + buttons)
    {
        QJsonObject desc = makeDescriptor("list", "列表编辑");
        QWidget* editor  = registry.createEditor("list", desc);
        QVERIFY(editor != nullptr);
        QListWidget* lw = editor->findChild<QListWidget*>();
        QVERIFY(lw != nullptr);
        QList<QPushButton*> buttons = editor->findChildren<QPushButton*>();
        QVERIFY(buttons.size() >= 2);
        delete editor;
    }

    // file → DAFilePathEditWidget
    {
        QJsonObject desc = makeDescriptor("file", "文件路径");
        QWidget* editor  = registry.createEditor("file", desc);
        QVERIFY(editor != nullptr);
        QLineEdit* le = editor->findChild<QLineEdit*>();
        QVERIFY(le != nullptr);
        delete editor;
    }

    // folder → DAFilePathEditWidget (directory mode)
    {
        QJsonObject desc = makeDescriptor("folder", "文件夹路径");
        QWidget* editor  = registry.createEditor("folder", desc);
        QVERIFY(editor != nullptr);
        QLineEdit* le = editor->findChild<QLineEdit*>();
        QVERIFY(le != nullptr);
        delete editor;
    }

    // color → DAColorPickerButton
    {
        QJsonObject desc = makeDescriptor("color", "颜色选择");
        QWidget* editor  = registry.createEditor("color", desc);
        QVERIFY(editor != nullptr);
        QVERIFY(editor->inherits("QToolButton"));
        delete editor;
    }

    // font → DAFontEditPannelWidget
    {
        QJsonObject desc = makeDescriptor("font", "字体设置");
        QWidget* editor  = registry.createEditor("font", desc);
        QVERIFY(editor != nullptr);
        QVERIFY(editor->findChild<QFontComboBox*>() != nullptr);
        delete editor;
    }

    // code → QPlainTextEdit
    {
        QJsonObject desc = makeDescriptor("code", "代码编辑");
        QWidget* editor  = registry.createEditor("code", desc);
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QPlainTextEdit*>(editor) != nullptr);
        delete editor;
    }
}

/**
 * @brief 未知类型调用 createEditor 返回 nullptr
 */
void TestDAParamTypeRegistry::test_unknownType_returnsNull()
{
    DAParamTypeRegistry registry;
    QJsonObject desc;
    desc["name"] = "unknown_param";
    desc["type"] = "unknown_type";

    QWidget* editor = registry.createEditor("unknown_type", desc);
    QVERIFY(editor == nullptr);

    editor = registry.createEditor("", desc);
    QVERIFY(editor == nullptr);
}

/**
 * @brief supportedTypes() 返回 11 种类型的完整列表
 */
void TestDAParamTypeRegistry::test_supportedTypes()
{
    DAParamTypeRegistry registry;
    QStringList types = registry.supportedTypes();

    QCOMPARE(types.size(), 11);

    QVERIFY(types.contains("str"));
    QVERIFY(types.contains("int"));
    QVERIFY(types.contains("float"));
    QVERIFY(types.contains("bool"));
    QVERIFY(types.contains("enum"));
    QVERIFY(types.contains("list"));
    QVERIFY(types.contains("file"));
    QVERIFY(types.contains("folder"));
    QVERIFY(types.contains("color"));
    QVERIFY(types.contains("font"));
    QVERIFY(types.contains("code"));
}

/**
 * @brief 各类型编辑器的默认值设置验证
 *
 * - str: placeholder 来自 description 字段
 * - int: 默认范围 -9999 到 9999，默认值 0
 * - float: 默认范围 -9999 到 9999，小数位 2，默认值 0.0
 * - bool: 默认 unchecked
 * - int/float: descriptor 中 min/max/decimals 覆盖默认值
 */
void TestDAParamTypeRegistry::test_defaultValues()
{
    DAParamTypeRegistry registry;

    // str → placeholder 来自 description
    {
        QJsonObject desc = makeDescriptor("str", "请输入名称");
        QLineEdit* edit  = qobject_cast<QLineEdit*>(registry.createEditor("str", desc));
        QVERIFY(edit != nullptr);
        QCOMPARE(edit->placeholderText(), QString("请输入名称"));
        delete edit;
    }

    // int → 默认范围与值
    {
        QJsonObject desc = makeDescriptor("int");
        QSpinBox* spin   = qobject_cast<QSpinBox*>(registry.createEditor("int", desc));
        QVERIFY(spin != nullptr);
        QCOMPARE(spin->minimum(), -9999);
        QCOMPARE(spin->maximum(), 9999);
        QCOMPARE(spin->value(), 0);
        delete spin;
    }

    // float → 默认范围、小数位与值
    {
        QJsonObject desc = makeDescriptor("float");
        QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(registry.createEditor("float", desc));
        QVERIFY(dspin != nullptr);
        QCOMPARE(dspin->minimum(), -9999.0);
        QCOMPARE(dspin->maximum(), 9999.0);
        QCOMPARE(dspin->decimals(), 2);
        QCOMPARE(dspin->value(), 0.0);
        delete dspin;
    }

    // bool → 默认 unchecked
    {
        QJsonObject desc = makeDescriptor("bool");
        QCheckBox* cb    = qobject_cast<QCheckBox*>(registry.createEditor("bool", desc));
        QVERIFY(cb != nullptr);
        QVERIFY(!cb->isChecked());
        delete cb;
    }

    // int → descriptor 中的 min/max 和 default
    {
        QJsonObject ext;
        ext["min"] = 0;
        ext["max"] = 100;
        QJsonObject desc = makeDescriptor("int", "", 50, ext);
        QSpinBox* spin   = qobject_cast<QSpinBox*>(registry.createEditor("int", desc));
        QVERIFY(spin != nullptr);
        QCOMPARE(spin->minimum(), 0);
        QCOMPARE(spin->maximum(), 100);
        QCOMPARE(spin->value(), 50);
        delete spin;
    }

    // float → descriptor 中的 min/max/decimals
    {
        QJsonObject ext;
        ext["min"]      = 0.0;
        ext["max"]      = 1.0;
        ext["decimals"] = 4;
        QJsonObject desc = makeDescriptor("float", "", 0.5, ext);
        QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(registry.createEditor("float", desc));
        QVERIFY(dspin != nullptr);
        QCOMPARE(dspin->minimum(), 0.0);
        QCOMPARE(dspin->maximum(), 1.0);
        QCOMPARE(dspin->decimals(), 4);
        delete dspin;
    }
}

/**
 * @brief enum 类型从 descriptor 读取 enum_values 和 default
 *
 * 验证 QComboBox 填充了 enum_values 的条目，
 * 并且 default 值被正确设置为当前选中项。
 */
void TestDAParamTypeRegistry::test_enumValues()
{
    DAParamTypeRegistry registry;

    // 基础 enum_values 测试
    {
        QJsonArray enumVals;
        enumVals.append("red");
        enumVals.append("green");
        enumVals.append("blue");
        QJsonObject ext;
        ext["enum_values"] = enumVals;
        QJsonObject desc   = makeDescriptor("enum", "颜色选择", QVariant(), ext);

        QComboBox* combo = qobject_cast<QComboBox*>(registry.createEditor("enum", desc));
        QVERIFY(combo != nullptr);
        QCOMPARE(combo->count(), 3);
        QCOMPARE(combo->itemText(0), QString("red"));
        QCOMPARE(combo->itemText(1), QString("green"));
        QCOMPARE(combo->itemText(2), QString("blue"));
        QCOMPARE(combo->currentIndex(), 0);
        delete combo;
    }

    // enum_values + default 指定选中项
    {
        QJsonArray enumVals;
        enumVals.append("small");
        enumVals.append("medium");
        enumVals.append("large");
        QJsonObject ext;
        ext["enum_values"] = enumVals;
        QJsonObject desc   = makeDescriptor("enum", "尺寸选择", QString("medium"), ext);

        QComboBox* combo = qobject_cast<QComboBox*>(registry.createEditor("enum", desc));
        QVERIFY(combo != nullptr);
        QCOMPARE(combo->count(), 3);
        QCOMPARE(combo->currentIndex(), 1);
        QCOMPARE(combo->currentText(), QString("medium"));
        delete combo;
    }

    // 空 enum_values → 空的 QComboBox
    {
        QJsonArray emptyEnum;
        QJsonObject ext;
        ext["enum_values"] = emptyEnum;
        QJsonObject desc   = makeDescriptor("enum", "空枚举", QVariant(), ext);

        QComboBox* combo = qobject_cast<QComboBox*>(registry.createEditor("enum", desc));
        QVERIFY(combo != nullptr);
        QCOMPARE(combo->count(), 0);
        delete combo;
    }
}

/**
 * @brief 重新注册同一类型会覆盖旧的 creator
 *
 * 先注册自定义 creator，验证生效后调用 registerDefaults() 恢复默认行为。
 * 反向验证：先 registerDefaults()，再自定义注册覆盖。
 */
void TestDAParamTypeRegistry::test_reRegistration_overwrites()
{
    DAParamTypeRegistry registry;

    // 场景 1：自定义注册 → registerDefaults 覆盖
    {
        registry.registerType("str", [](const QJsonObject&, QWidget* parent) -> QWidget* {
            return new QPlainTextEdit(parent);
        });

        QWidget* editor = registry.createEditor("str", QJsonObject());
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QPlainTextEdit*>(editor) != nullptr);
        delete editor;

        registry.registerDefaults();

        editor = registry.createEditor("str", QJsonObject());
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QLineEdit*>(editor) != nullptr);
        delete editor;
    }

    // 场景 2：registerDefaults → 自定义注册覆盖
    {
        registry.registerDefaults();

        registry.registerType("int", [](const QJsonObject&, QWidget* parent) -> QWidget* {
            return new QCheckBox("custom int", parent);
        });

        QWidget* editor = registry.createEditor("int", QJsonObject());
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QCheckBox*>(editor) != nullptr);
        delete editor;
    }
}