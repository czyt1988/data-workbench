#ifndef DAFORMEDITORREGISTRY_H
#define DAFORMEDITORREGISTRY_H

#include "DACommonWidgetsAPI.h"
#include "DAFormSpec.h"  // from DAUtils
#include <QHash>
#include <QString>
#include <QVariant>
#include <functional>
#include <QWidget>

namespace DA
{
/**
 * @brief 通用表单编辑器注册表
 *
 * 将 DAFormFieldDef 的类型字符串映射到一组编辑器适配器（创建/读取/写入/信号连接），
 * 是 DAPropertyFormWidget 渲染表单的核心依赖。默认注册 11 种内置类型：
 * str, int, float, bool, enum, list, file, folder, color, font, code。
 *
 * 每个适配器由四个 std::function 组成：
 * - EditorCreator：根据字段定义创建并配置好初始状态的编辑器控件
 * - EditorReader：从编辑器控件读取当前值
 * - EditorWriter：将值写入编辑器控件（程序化赋值，不应触发用户编辑语义）
 * - SignalConnector：将编辑器原生的值变化信号连接到指定槽
 *
 * 适配器设计使得渲染层与具体控件解耦，业务代码可通过 registerType() 注册自定义类型
 * 或覆盖已有类型的编辑器实现。
 *
 * @code
 * DAFormEditorRegistry registry;
 * registry.registerDefaults();
 * QWidget* editor = registry.createEditor(field, parent);
 * @endcode
 *
 * @see DAPropertyFormWidget, DAFormFieldDef
 */
class DACOMMONWIDGETS_API DAFormEditorRegistry
{
    DA_DECLARE_PRIVATE(DAFormEditorRegistry)
public:
    /// 编辑器创建函数：根据字段定义和父控件返回配置好的编辑器
    using EditorCreator = std::function< QWidget*(const DAFormFieldDef&, QWidget*) >;
    /// 编辑器读取函数：从编辑器控件返回当前值
    using EditorReader = std::function< QVariant(QWidget*) >;
    /// 编辑器写入函数：将值写入编辑器控件
    using EditorWriter = std::function< void(QWidget*, const QVariant&) >;
    /// 信号连接函数：将编辑器原生值变化信号连接到 context 持有的 slot
    using SignalConnector = std::function< void(QWidget*, QObject*, const std::function< void() >&) >;

    /**
     * @brief 编辑器适配器
     *
     * 聚合四个回调，描述一种类型完整的编辑器交互行为。
     */
    struct EditorAdapter
    {
        EditorCreator create;              ///< 创建编辑器
        EditorReader read;                 ///< 读取编辑器值
        EditorWriter write;                ///< 写入编辑器值
        SignalConnector connectValueChanged;  ///< 连接值变化信号
    };

    // 默认构造函数（不自动注册内置类型）
    DAFormEditorRegistry();
    // 析构函数
    ~DAFormEditorRegistry();

    // 注册全部 11 种内置默认类型（覆盖已有同名类型）
    void registerDefaults();

    // 注册或覆盖指定类型的编辑器适配器
    void registerType(const QString& type, const EditorAdapter& adapter);

    // 检查指定类型是否已注册
    bool hasType(const QString& type) const;

    // 根据字段定义创建编辑器控件，未注册类型返回 nullptr
    QWidget* createEditor(const DAFormFieldDef& field, QWidget* parent) const;

    // 从编辑器控件读取当前值，未注册类型返回无效 QVariant
    QVariant readValue(const DAFormFieldDef& field, QWidget* editor) const;

    // 将值写入编辑器控件，未注册类型不做任何操作
    void writeValue(const DAFormFieldDef& field, QWidget* editor, const QVariant& value) const;

    // 将编辑器原生值变化信号连接到 context 持有的 slot，未注册类型不做任何操作
    void connectValueChanged(const DAFormFieldDef& field, QWidget* editor, QObject* context, const std::function< void() >& slot) const;
};
}  // namespace DA

#endif  // DAFORMEDITORREGISTRY_H
