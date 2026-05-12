#ifndef DAPARAMTYPEREGISTRY_H
#define DAPARAMTYPEREGISTRY_H

#include "DAGuiAPI.h"
#include <QString>
#include <QStringList>
#include "DAParameterDescriptor.h"
#include <functional>
#include <map>

class QWidget;

namespace DA
{

/**
 * @brief 参数类型注册系统，映射 Python 描述符类型字符串到 Qt 编辑器控件
 *
 * 提供可扩展的类型→编辑器注册机制。默认注册 11 种内置类型：
 * str→QLineEdit, int→QSpinBox, float→QDoubleSpinBox, bool→QCheckBox,
 * enum→QComboBox, list→QListWidget+buttons, file→DAFilePathEditWidget,
 * folder→DAFilePathEditWidget(dir mode), color→DAColorPickerButton,
 * font→DAFontEditPannelWidget, code→QPlainTextEdit。
 *
 * 通过 registerType() 可注册自定义类型或覆盖已有类型的编辑器创建逻辑。
 * 每个类型对应一个 ParamEditorCreator 函数，接收参数描述符 JSON 和父控件，
 * 返回创建好的编辑器 QWidget。
 *
 * @code
 * DAParamTypeRegistry registry;
 * registry.registerDefaults();
 * QWidget* editor = registry.createEditor("int", descriptorJson);
 * @endcode
 *
 * @see DAPyParamTypeHelper, ParameterDescriptor
 */
class DAGUI_API DAParamTypeRegistry
{
    DA_DECLARE_PRIVATE(DAParamTypeRegistry)
public:
    /**
     * @brief 编辑器创建函数类型
     *
     * 接收参数描述符 JSON 对象和父控件指针，返回创建并配置好的编辑器 QWidget。
     * 描述符 JSON 包含 name、type、description、default 等字段，
     * 以及 C++ 端扩展字段如 enum_values、min、max、decimals 等。
     */
    using ParamEditorCreator = std::function< QWidget*(const DAParameterDescriptor& paramDescriptor, QWidget* parent) >;

    // 默认构造函数，自动注册所有内置类型
    DAParamTypeRegistry();

    // 析构函数
    ~DAParamTypeRegistry();

    // 注册自定义类型的编辑器创建函数（覆盖已有类型时直接替换）
    void registerType(const QString& typeStr, ParamEditorCreator creator);

    // 重新注册所有内置默认类型（覆盖任何自定义注册）
    void registerDefaults();

    // 根据类型字符串和参数描述符创建对应的编辑器控件
    QWidget* createEditor(const QString& typeStr, const DAParameterDescriptor& paramDescriptor, QWidget* parent = nullptr) const;

    // 获取所有已注册类型的名称列表
    QStringList supportedTypes() const;

    // 检查指定类型是否已注册
    bool isRegistered(const QString& typeStr) const;
};

}  // namespace DA

#endif  // DAPARAMTYPEREGISTRY_H
