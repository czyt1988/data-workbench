#include "DANodeParameterFormAdapter.h"
#include <QPair>
#include <QStringList>
#include <QVariant>
#include <QVariantHash>

namespace DA
{

/**
 * @brief 将 Python 端类型别名归一化为内部标准类型字符串
 *
 * 镜像原 DAParamDef::normalizeTypeName 的映射规则（大小写不敏感）：
 * - "string"  → "str"
 * - "boolean" → "bool"
 * - "double"  → "float"
 * - "integer" → "int"
 * 其他类型字符串原样返回（仅做 trim + toLower）。空字符串返回空字符串。
 *
 * @param[in] rawType 原始类型字符串
 * @return 归一化后的类型字符串
 */
static QString normalizeTypeName(const QString& rawType)
{
    QString t = rawType.trimmed().toLower();
    if (t.isEmpty()) {
        return QString();
    }
    if (t == QStringLiteral("string")) {
        return QStringLiteral("str");
    }
    if (t == QStringLiteral("boolean")) {
        return QStringLiteral("bool");
    }
    if (t == QStringLiteral("double")) {
        return QStringLiteral("float");
    }
    if (t == QStringLiteral("integer")) {
        return QStringLiteral("int");
    }
    return t;
}

/**
 * @brief 将 enum 扩展属性转换为 DAFormOption 列表
 *
 * 兼容两种存储格式：
 * - QList<QPair<QString,int>>：取 first 作为 value 与 label，丢弃 int
 *   （DAFormOption 无 int 字段，旧枚举编辑器读取 currentText，行为保持一致）
 * - QStringList：每个字符串同时作为 value 与 label
 *
 * @param[in] enumVar enum 扩展属性的 QVariant
 * @param[out] options 输出的 DAFormOption 列表
 */
static void convertEnumToOptions(const QVariant& enumVar, QList< DAFormOption >& options)
{
    // 优先匹配 QList<QPair<QString,int>>（更具体），再回退 QStringList
    if (enumVar.canConvert< QList< QPair< QString, int > > >()) {
        QList< QPair< QString, int > > pairList = enumVar.value< QList< QPair< QString, int > > >();
        for (const QPair< QString, int >& item : std::as_const(pairList)) {
            DAFormOption opt;
            opt.value       = item.first;
            opt.label       = item.first;
            opt.description = QString();
            options.append(opt);
        }
    } else if (enumVar.canConvert< QStringList >()) {
        QStringList sl = enumVar.value< QStringList >();
        for (const QString& s : std::as_const(sl)) {
            DAFormOption opt;
            opt.value       = s;
            opt.label       = s;
            opt.description = QString();
            options.append(opt);
        }
    }
}

/**
 * @brief 将 DAPyNodeParameter 列表转换为 DAFormSpec
 *
 * 每个 DAPyNodeParameter 映射为一个 DAFormFieldDef，并包装为顶层 DAFormItemDef（kind=Field）。
 * 不使用分组——节点参数面板历史上无分组需求。
 * 扩展属性（enum/min/max/step/decimals/filter/layout/height）分发到字段的不同成员：
 * - enum → field.options（int 丢弃，仅保留字符串）
 * - min/max/step/decimals/filter → field.attributes
 * - layout → field.layout（默认 "inline"）
 * - height → field.height（默认 -1）
 * description 同时写入 field.description 与 field.placeholder，保持旧编辑器以描述为占位提示的行为。
 *
 * @param[in] parameters 节点参数代理列表
 * @param[in] title 表单标题，非空时写入 spec.title
 * @return 构建好的 DAFormSpec
 */
DAFormSpec DANodeParameterFormAdapter::toFormSpec(const QList< DAPyNodeParameter >& parameters, const QString& title)
{
    DAFormSpec spec;
    spec.version = 2;
    if (!title.isEmpty()) {
        spec.title = title;
    }

    for (const DAPyNodeParameter& param : std::as_const(parameters)) {
        DAFormFieldDef field;
        field.name = param.name();
        field.type = normalizeTypeName(param.typeLabel());
        // 旧面板以参数名作为显示标签
        field.label        = param.name();
        field.description  = param.description();
        field.placeholder  = param.description();
        field.defaultValue = param.defaultValue();

        const QVariantHash props = param.properties();

        // enum → options（兼容 QStringList 与 QList<QPair<QString,int>>）
        if (props.contains(QStringLiteral("enum"))) {
            convertEnumToOptions(props.value(QStringLiteral("enum")), field.options);
        }
        // 数值/路径扩展属性 → attributes
        if (props.contains(QStringLiteral("min"))) {
            field.attributes[ QStringLiteral("min") ] = props.value(QStringLiteral("min"));
        }
        if (props.contains(QStringLiteral("max"))) {
            field.attributes[ QStringLiteral("max") ] = props.value(QStringLiteral("max"));
        }
        if (props.contains(QStringLiteral("step"))) {
            field.attributes[ QStringLiteral("step") ] = props.value(QStringLiteral("step"));
        }
        if (props.contains(QStringLiteral("decimals"))) {
            field.attributes[ QStringLiteral("decimals") ] = props.value(QStringLiteral("decimals"));
        }
        if (props.contains(QStringLiteral("filter"))) {
            field.attributes[ QStringLiteral("filter") ] = props.value(QStringLiteral("filter"));
        }
        // layout → field.layout（默认 "inline"）
        if (props.contains(QStringLiteral("layout"))) {
            QString lay = props.value(QStringLiteral("layout")).toString().toLower();
            field.layout = lay.isEmpty() ? QStringLiteral("inline") : lay;
        }
        // height → field.height（默认 -1）
        if (props.contains(QStringLiteral("height"))) {
            bool ok    = false;
            int h      = props.value(QStringLiteral("height")).toInt(&ok);
            field.height = ok ? h : -1;
        }

        DAFormItemDef item;
        item.kind  = DAFormItemDef::Field;
        item.field = std::move(field);
        spec.items.append(std::move(item));
    }

    return spec;
}

}  // namespace DA
