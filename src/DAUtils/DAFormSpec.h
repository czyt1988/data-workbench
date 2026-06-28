#ifndef DAFORMSPEC_H
#define DAFORMSPEC_H
#include "DAUtilsAPI.h"
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <memory>

/**
 * @file 此文件定义了表单规格的数据模型
 *
 * 包含表单选项、字段定义、分组定义、条目定义与表单规格等纯数据结构。
 * 这些结构由 DAFormSchemaIO 从 JSON 解析生成，由 DAFormRuleEvaluator 评估联动状态。
 */
namespace DA
{
// 前置声明，用于 DAFormGroupDef 与 DAFormItemDef 之间的递归引用
struct DAFormGroupDef;

/**
 * @brief 表单选项定义
 *
 * 描述枚举/下拉框等控件中的单个选项，包含值、显示标签与可选的描述文本。
 */
struct DAUTILS_API DAFormOption
{
    QVariant value;       // 选项值
    QString label;        // 显示文本
    QString description;  // 可选描述
};

/**
 * @brief 表单字段定义
 *
 * 描述单个表单字段的元数据：类型、标签、默认值、布局、联动规则等。
 * 数值与枚举相关扩展属性（min/max/step/decimals/filter 等）存放在 @ref attributes 中，
 * 而 @ref options 单独存储为 DAFormOption 列表。
 */
struct DAUTILS_API DAFormFieldDef
{
    QString name;                   ///< 字段名（唯一标识）
    QString type;                   ///< 字段类型
    QString label;                  ///< 显示标签
    QString description;            ///< 可选描述
    QVariant defaultValue;          ///< 默认值
    QString placeholder;            ///< 占位提示文本
    bool readOnly { false };        ///< 是否只读
    QString layout { "inline" };    ///< 布局方式
    int height { -1 };              ///< 固定高度，-1 表示不指定
    QVariantMap attributes;         ///< 额外属性（min/max/step/decimals/filter 等）
    QString visibleWhen;            ///< 可见性联动规则表达式
    QString enabledWhen;            ///< 启用性联动规则表达式
    QString requiredWhen;           ///< 必填性联动规则表达式
    QList< DAFormOption > options;  ///< 枚举选项列表
};

/**
 * @brief 表单项定义
 *
 * 表单中的一个条目，可以是字段或分组，通过 @ref kind 区分。
 * 当 kind == Group 时，@ref group 非空；当 kind == Field 时，使用 @ref field。
 * 这是带判别字段的联合体风格，下游渲染代码根据 kind 分支处理。
 */
struct DAUTILS_API DAFormItemDef
{
    /**
     * @brief 条目类型
     */
    enum Kind
    {
        Group,  ///< 分组
        Field   ///< 字段
    };
    Kind kind { Field };                      ///< 条目类型
    DAFormFieldDef field;                     ///< 字段定义（kind == Field 时有效）
    std::shared_ptr< DAFormGroupDef > group;  ///< 分组定义（kind == Group 时有效）
};

/**
 * @brief 表单分组定义
 *
 * 分组可嵌套：通过 @ref items 内的 DAFormItemDef 递归引用 DAFormGroupDef，从而构成树形结构。
 */
struct DAUTILS_API DAFormGroupDef
{
    QString name;                  ///< 分组名
    QString label;                 ///< 显示标签
    QString description;           ///< 可选描述
    QList< DAFormItemDef > items;  ///< 子条目列表
};

/**
 * @brief 表单规格定义
 *
 * 描述一个完整表单的顶层结构，包含版本号、标题与顶层条目列表。
 * 由 DAFormSchemaIO 解析自 JSON，由 DAFormRuleEvaluator 评估字段联动状态。
 */
struct DAUTILS_API DAFormSpec
{
    int version { 2 };             ///< schema 版本号
    QString title;                 ///< 表单标题
    QList< DAFormItemDef > items;  ///< 顶层条目列表
};

}  // namespace DA

#endif  // DAFORMSPEC_H
