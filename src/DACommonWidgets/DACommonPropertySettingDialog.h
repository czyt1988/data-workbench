#ifndef DACOMMONPROPERTYSETTINGDIALOG_H
#define DACOMMONPROPERTYSETTINGDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include "DACommonWidgetsAPI.h"

namespace Ui
{
class DACommonPropertySettingDialog;
}

class QtProperty;
namespace DA
{
/**
 * @class DACommonPropertySettingDialog
 * @brief 通用的属性设置对话框，支持通过 JSON 配置动态生成设置界面
 *
 * 该类使用 QtPropertyBrowser 框架，通过 JSON 配置动态生成属性编辑界面。
 * 支持从 Python 端生成 JSON 配置，C++ 端解析并显示设置窗口。
 *
 * ## JSON 配置格式说明
 *
 * JSON 配置是一个对象，包含以下主要字段：
 *
 * ### 1. 基本结构
 * @code{.json}
 * {
 *     "window_title": "窗口标题",
 *     "properties": [
 *         // 属性定义数组
 *     ]
 * }
 * @endcode
 *
 * ### 2. 属性类型说明
 *
 * #### 2.1 分组属性 (group)
 * 用于将相关属性组织在一起。
 * @code{.json}
 * {
 *     "name": "basic_settings",
 *     "type": "group",
 *     "display_name": "基本设置",
 *     "properties": [
 *         // 子属性定义
 *     ]
 * }
 * @endcode
 *
 * #### 2.2 字符串属性 (string)
 * @code{.json}
 * {
 *     "name": "str_param",
 *     "type": "string",
 *     "display_name": "字符串参数",
 *     "value": "默认值",
 *     "description": "参数说明",
 *     "read_only": false
 * }
 * @endcode
 *
 * #### 2.3 整数属性 (int)
 * @code{.json}
 * {
 *     "name": "int_param",
 *     "type": "int",
 *     "display_name": "整数参数",
 *     "value": 100,
 *     "min": 0,
 *     "max": 1000,
 *     "description": "整数参数说明",
 *     "singleStep": 10
 * }
 * @endcode
 *
 * #### 2.4 浮点数属性 (double)
 * @code{.json}
 * {
 *     "name": "float_param",
 *     "type": "double",
 *     "display_name": "浮点参数",
 *     "value": 0.5,
 *     "min": 0.0,
 *     "max": 1.0,
 *     "decimals": 3,
 *     "description": "浮点参数说明",
 *     "singleStep": 0.1
 * }
 * @endcode
 *
 * #### 2.5 布尔属性 (bool)
 * @code{.json}
 * {
 *     "name": "bool_param",
 *     "type": "bool",
 *     "display_name": "布尔参数",
 *     "value": true,
 *     "description": "布尔参数说明"
 * }
 * @endcode
 *
 * #### 2.6 枚举属性 (enum)
 * 支持两种配置方式：
 *
 * 方式一：简写方式（兼容旧版本）
 * @code{.json}
 * {
 *     "name": "algorithm",
 *     "type": "enum",
 *     "display_name": "算法选择",
 *     "value": "svm",
 *     "enum_items": ["svm", "random_forest", "neural_network"],
 *     "description": "选择算法类型"
 * }
 * @endcode
 *
 * 方式二：带描述的方式（推荐）
 * @code{.json}
 * {
 *     "name": "algorithm",
 *     "type": "enum",
 *     "display_name": "算法选择",
 *     "value": "svm",
 *     "enum_items": ["svm", "random_forest", "neural_network"],
 *     "enum_descriptions": ["支持向量机", "随机森林", "神经网络"],
 *     "description": "选择算法类型"
 * }
 * @endcode
 *
 * 如果没有提供 enum_descriptions，则显示 enum_items 的值。
 * 获取值时始终返回 enum_items 中的值。
 *
 * #### 2.7 颜色属性 (color)
 * @code{.json}
 * {
 *     "name": "text_color",
 *     "type": "color",
 *     "display_name": "文本颜色",
 *     "value": "#FF0000",
 *     "description": "选择文本颜色"
 * }
 * @endcode
 *
 * #### 2.8 字体属性 (font)
 * @code{.json}
 * {
 *     "name": "text_font",
 *     "type": "font",
 *     "display_name": "文本字体",
 *     "value": "Arial,12,-1,5,50,0,0,0,0,0",
 *     "description": "选择文本字体"
 * }
 * @endcode
 *
 * #### 2.9 文件选择属性 (file)
 * @code{.json}
 * {
 *     "name": "input_file",
 *     "type": "file",
 *     "display_name": "输入文件",
 *     "value": "",
 *     "filter": "文本文件 (*.txt);;所有文件 (*.*)",
 *     "description": "选择输入文件"
 * }
 * @endcode
 *
 * #### 2.10 文件夹选择属性 (folder)
 * @code{.json}
 * {
 *     "name": "output_dir",
 *     "type": "folder",
 *     "display_name": "输出目录",
 *     "value": "",
 *     "description": "选择输出目录"
 * }
 * @endcode
 *
 * #### 2.11 字符串列表属性 (stringlist)
 * @code{.json}
 * {
 *     "name": "string_list",
 *     "type": "stringlist",
 *     "display_name": "字符串列表",
 *     "value": ["item1", "item2", "item3"],
 *     "description": "字符串列表参数"
 * }
 * @endcode
 *
 * ## 使用示例
 *
 * ### C++ 端示例
 * @code{.cpp}
 * #include "DACommonPropertySettingDialog.h"
 *
 * // 1. 直接使用对话框
 * DA::DACommonPropertySettingDialog dialog;
 * if (dialog.loadFromJson(jsonString)) {
 *     if (dialog.exec() == QDialog::Accepted) {
 *         QJsonObject result = dialog.getCurrentValues();
 *         // 处理结果
 *     }
 * }
 *
 * // 2. 使用静态方法
 * QJsonObject result = DA::DACommonPropertySettingDialog::showSettingsDialog(
 *     jsonString, parentWidget);
 *
 * if (!result.isEmpty()) {
 *     // 处理结果
 * }
 * @endcode
 *
 * ### Python 端示例
 * @code{.python}
 * import json
 *
 * # 生成配置
 * config = {
 *     "window_title": "参数设置",
 *     "properties": [
 *         {
 *             "name": "basic",
 *             "type": "group",
 *             "display_name": "基本设置",
 *             "properties": [
 *                 {
 *                     "name": "threshold",
 *                     "type": "double",
 *                     "display_name": "阈值",
 *                     "value": 0.5,
 *                     "min": 0.0,
 *                     "max": 1.0,
 *                     "decimals": 2,
 *                     "description": "处理阈值"
 *                 },
 *                 {
 *                     "name": "enable_filter",
 *                     "type": "bool",
 *                     "display_name": "启用过滤",
 *                     "value": True,
 *                     "description": "是否启用过滤器"
 *                 }
 *             ]
 *         }
 *     ]
 * }
 *
 * # 转换为 JSON 字符串传递给 C++
 * json_string = json.dumps(config, ensure_ascii=False)
 * @endcode
 *
 * @note 确保在创建对话框前已经初始化了 QApplication
 * @see QtPropertyBrowser, QtVariantPropertyManager
 */
class DACOMMONWIDGETS_API DACommonPropertySettingDialog : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DACommonPropertySettingDialog)
public:
    explicit DACommonPropertySettingDialog(QWidget* parent = nullptr);
    ~DACommonPropertySettingDialog();

    // 从 JSON 字符串加载配置
    bool loadFromJson(const QString& jsonStr);

    // 从 QJsonObject 加载配置
    bool loadFromJsonObject(const QJsonObject& jsonObj);

    // 获取所有属性的当前值
    QJsonObject getCurrentValues() const;

    // 获取特定属性的当前值
    QVariant getValue(const QString& propertyName) const;

    // 设置特定属性的值
    bool setValue(const QString& propertyName, const QVariant& value);

    // 静态方法：显示设置对话框并获取结果
    static QJsonObject showSettingsDialog(const QString& jsonConfig, QWidget* parent = nullptr);

    // 重置所有属性为默认值
    void resetToDefaults();

protected:
    /**
     * @brief 处理界面语言变更事件
     * @param e 事件对象
     * @note 当应用程序语言变更时，会自动重新翻译界面
     */
    void changeEvent(QEvent* e) override;

Q_SIGNALS:
    /**
     * @brief 属性值变更信号
     * @param propertyName 属性名称
     * @param value 新的属性值
     * @note 当用户修改属性值时发出此信号
     * @code{.cpp}
     * connect(&dialog, &DACommonPropertySettingDialog::propertyValueChanged,
     *         this, [](const QString& name, const QVariant& value) {
     *     qDebug() << "属性" << name << "变为:" << value;
     * });
     * @endcode
     */
    void propertyValueChanged(const QString& propertyName, const QVariant& value);

private Q_SLOTS:

    void onPropertyValueChanged(QtProperty* property, const QVariant& value);

private:
    Ui::DACommonPropertySettingDialog* ui;
};
}  // end namespace DA
#endif  // DACOMMONPROPERTYSETTINGDIALOG_H
