#include "tst_danodedescriptor.h"
#include "DANodeDescriptor.h"
#include "DAPortDescriptor.h"
#include "DAParameterDescriptor.h"
#include "DAPyNodeStyle.h"
#include "DAPyNodeStyleDefine.h"
#include "DAPyNodeFactory.h"
#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonArray>

namespace DA
{

// ============================================================
// testDefaultConstruction — 默认构造验证
// ============================================================

/**
 * @brief 验证 DANodeDescriptor 默认构造函数的所有字段值
 *
 * 默认构造应设置：
 * - name == 空字符串
 * - qualifiedName == 空字符串
 * - category == 空字符串
 * - icon == 空字符串
 * - inputs == 空 QVector
 * - outputs == 空 QVector
 * - parameters == 空 QVector
 * - renderTemplate == NodeStyleTemplate
 * - style == 默认 DANodeStyle
 */
void TestDANodeDescriptor::testDefaultConstruction()
{
    DANodeDescriptor desc;

    QVERIFY(desc.name.isEmpty());
    QVERIFY(desc.qualifiedName.isEmpty());
    QVERIFY(desc.category.isEmpty());
    QVERIFY(desc.icon.isEmpty());
    QVERIFY(desc.inputs.isEmpty());
    QVERIFY(desc.outputs.isEmpty());
    QVERIFY(desc.parameters.isEmpty());
    QCOMPARE(desc.renderTemplate, RenderTemplate::NodeStyleTemplate);

    // style 应为默认构造（字段由 setDefaults 初始化）
    QCOMPARE(desc.style.bodyShape, BodyShape::RoundedRect);
    QCOMPARE(desc.style.cornerRadius, 4.0);
}

// ============================================================
// testFieldAccess — 字段访问验证
// ============================================================

/**
 * @brief 验证设置字段后能正确读取各字段值
 */
void TestDANodeDescriptor::testFieldAccess()
{
    DANodeDescriptor desc;
    desc.name           = QStringLiteral("MyNode");
    desc.qualifiedName  = QStringLiteral("pkg.module.MyNode");
    desc.category       = QStringLiteral("数据分析");
    desc.icon           = QStringLiteral(":/icons/node.svg");
    desc.renderTemplate = RenderTemplate::WidgetTemplate;

    DAPortDescriptor inputPort(QStringLiteral("data_in"), QStringLiteral("DataFrame"));
    DAPortDescriptor outputPort(QStringLiteral("data_out"), QStringLiteral("Series"));
    desc.inputs.append(inputPort);
    desc.outputs.append(outputPort);

    DAParameterDescriptor param;
    param.name = QStringLiteral("threshold");
    param.type = QStringLiteral("float");
    desc.parameters.append(param);

    QCOMPARE(desc.name, QStringLiteral("MyNode"));
    QCOMPARE(desc.qualifiedName, QStringLiteral("pkg.module.MyNode"));
    QCOMPARE(desc.category, QStringLiteral("数据分析"));
    QCOMPARE(desc.icon, QStringLiteral(":/icons/node.svg"));
    QCOMPARE(desc.renderTemplate, RenderTemplate::WidgetTemplate);
    QCOMPARE(desc.inputs.size(), 1);
    QCOMPARE(desc.outputs.size(), 1);
    QCOMPARE(desc.parameters.size(), 1);
    QCOMPARE(desc.inputs[ 0 ].name, QStringLiteral("data_in"));
    QCOMPARE(desc.outputs[ 0 ].name, QStringLiteral("data_out"));
    QCOMPARE(desc.parameters[ 0 ].name, QStringLiteral("threshold"));
}

// ============================================================
// testIsValid — 有效性验证
// ============================================================

/**
 * @brief 验证 isValid() 的正确行为
 *
 * - 默认构造：qualifiedName 为空 → false
 * - 仅设置 name：qualifiedName 为空 → false
 * - 设置 qualifiedName：→ true
 */
void TestDANodeDescriptor::testIsValid()
{
    DANodeDescriptor desc;
    QVERIFY(!desc.isValid());

    desc.name = QStringLiteral("MyNode");
    QVERIFY(!desc.isValid());

    desc.qualifiedName = QStringLiteral("pkg.module.MyNode");
    QVERIFY(desc.isValid());

    // 清空 qualifiedName 后应再次无效
    desc.qualifiedName.clear();
    QVERIFY(!desc.isValid());
}

// ============================================================
// testToMetaData — toMetaData 转换验证
// ============================================================

/**
 * @brief 验证 DANodeDescriptor 转换为 DAPyNodeMetaData 的字段映射
 *
 * 映射关系：
 * - name → name
 * - qualifiedName → qualifiedName
 * - category → group
 * - icon → iconPath
 * - inputs 的 name 列表 → inputKeys
 * - outputs 的 name 列表 → outputKeys
 */
void TestDANodeDescriptor::testToMetaData()
{
    DANodeDescriptor desc;
    desc.name          = QStringLiteral("筛选节点");
    desc.qualifiedName = QStringLiteral("data_workbench.filter_node");
    desc.category      = QStringLiteral("数据清洗");
    desc.icon          = QStringLiteral(":/icons/filter.svg");

    DAPortDescriptor in1(QStringLiteral("raw_data"), QStringLiteral("DataFrame"));
    DAPortDescriptor in2(QStringLiteral("config"), QStringLiteral("dict"));
    DAPortDescriptor out1(QStringLiteral("filtered"), QStringLiteral("DataFrame"));
    desc.inputs.append(in1);
    desc.inputs.append(in2);
    desc.outputs.append(out1);

    DAPyNodeMetaData meta = desc.toMetaData();

    QCOMPARE(meta.name, QStringLiteral("筛选节点"));
    QCOMPARE(meta.qualifiedName, QStringLiteral("data_workbench.filter_node"));
    QCOMPARE(meta.group, QStringLiteral("数据清洗"));
    QCOMPARE(meta.iconPath, QStringLiteral(":/icons/filter.svg"));
    QCOMPARE(meta.inputKeys.size(), 2);
    QCOMPARE(meta.inputKeys[ 0 ], QStringLiteral("raw_data"));
    QCOMPARE(meta.inputKeys[ 1 ], QStringLiteral("config"));
    QCOMPARE(meta.outputKeys.size(), 1);
    QCOMPARE(meta.outputKeys[ 0 ], QStringLiteral("filtered"));

    // 验证 tooltip: name + " (" + qualifiedName + ")"
    QCOMPARE(meta.tooltip, QStringLiteral("筛选节点 (data_workbench.filter_node)"));
}

// ============================================================
// testJsonRoundTrip — JSON 序列化往返
// ============================================================

/**
 * @brief 验证 DANodeDescriptor JSON 往返序列化
 *
 * 创建带值描述符 → toJson → fromJson → 验证值与原始一致
 */
void TestDANodeDescriptor::testJsonRoundTrip()
{
    DANodeDescriptor orig;
    orig.name           = QStringLiteral("汇总节点");
    orig.qualifiedName  = QStringLiteral("data_workbench.summary_node");
    orig.category       = QStringLiteral("统计");
    orig.icon           = QStringLiteral(":/icons/summary.svg");
    orig.renderTemplate = RenderTemplate::WidgetTemplate;

    DAPortDescriptor inputPort(QStringLiteral("input"), QStringLiteral("DataFrame"));
    DAPortDescriptor outputPort(QStringLiteral("output"), QStringLiteral("DataFrame"));
    orig.inputs.append(inputPort);
    orig.outputs.append(outputPort);

    DAParameterDescriptor param;
    param.name = QStringLiteral("method");
    param.type = QStringLiteral("str");
    orig.parameters.append(param);

    // 不拷贝 style（使用默认值），验证 roundTrip 仍正确
    QJsonObject json           = orig.toJson();
    DANodeDescriptor roundTrip = DANodeDescriptor::fromJson(json);

    QCOMPARE(roundTrip.name, orig.name);
    QCOMPARE(roundTrip.qualifiedName, orig.qualifiedName);
    QCOMPARE(roundTrip.category, orig.category);
    QCOMPARE(roundTrip.icon, orig.icon);
    QCOMPARE(roundTrip.renderTemplate, orig.renderTemplate);
    QCOMPARE(roundTrip.inputs.size(), orig.inputs.size());
    QCOMPARE(roundTrip.inputs[ 0 ].name, orig.inputs[ 0 ].name);
    QCOMPARE(roundTrip.outputs.size(), orig.outputs.size());
    QCOMPARE(roundTrip.outputs[ 0 ].name, orig.outputs[ 0 ].name);
    QCOMPARE(roundTrip.parameters.size(), orig.parameters.size());
    QCOMPARE(roundTrip.parameters[ 0 ].name, orig.parameters[ 0 ].name);
}

// ============================================================
// testJsonKeysMatchOldFormat — JSON 键名验证
// ============================================================

/**
 * @brief 验证 JSON 键名与旧 Python dict 格式一致
 *
 * 旧 Python 键名（snake_case）：
 * - "name", "qualified_name", "category", "icon"
 * - "inputs", "outputs", "parameters"
 * - "render_template", "style"
 */
void TestDANodeDescriptor::testJsonKeysMatchOldFormat()
{
    DANodeDescriptor desc;
    desc.name           = QStringLiteral("测试节点");
    desc.qualifiedName  = QStringLiteral("pkg.test_node");
    desc.category       = QStringLiteral("测试分组");
    desc.icon           = QStringLiteral(":/test.svg");
    desc.renderTemplate = RenderTemplate::WidgetTemplate;

    DAPortDescriptor inPort(QStringLiteral("x"), QStringLiteral("int"));
    DAPortDescriptor outPort(QStringLiteral("y"), QStringLiteral("float"));
    desc.inputs.append(inPort);
    desc.outputs.append(outPort);

    DAParameterDescriptor param;
    param.name = QStringLiteral("threshold");
    param.type = QStringLiteral("float");
    desc.parameters.append(param);

    QJsonObject json = desc.toJson();

    // 验证核心键名存在且为 snake_case 格式
    QVERIFY(json.contains(QStringLiteral("name")));
    QVERIFY(json.contains(QStringLiteral("qualified_name")));
    QVERIFY(json.contains(QStringLiteral("category")));
    QVERIFY(json.contains(QStringLiteral("icon")));
    QVERIFY(json.contains(QStringLiteral("inputs")));
    QVERIFY(json.contains(QStringLiteral("outputs")));
    QVERIFY(json.contains(QStringLiteral("parameters")));
    QVERIFY(json.contains(QStringLiteral("render_template")));

    // 验证 render_template 的字符串值
    QCOMPARE(json.value(QStringLiteral("render_template")).toString(), QStringLiteral("widget"));

    // 验证 inputs 数组中端口对象的键名
    QJsonArray inputsArr = json.value(QStringLiteral("inputs")).toArray();
    QVERIFY(inputsArr.size() > 0);
    QJsonObject inputObj = inputsArr[ 0 ].toObject();
    QVERIFY(inputObj.contains(QStringLiteral("name")));
    QVERIFY(inputObj.contains(QStringLiteral("data_type")));

    // 验证 outputs 数组中端口对象的键名
    QJsonArray outputsArr = json.value(QStringLiteral("outputs")).toArray();
    QVERIFY(outputsArr.size() > 0);
    QJsonObject outputObj = outputsArr[ 0 ].toObject();
    QVERIFY(outputObj.contains(QStringLiteral("name")));
    QVERIFY(outputObj.contains(QStringLiteral("data_type")));

    // 验证 parameters 数组中参数对象的键名
    QJsonArray paramsArr = json.value(QStringLiteral("parameters")).toArray();
    QVERIFY(paramsArr.size() > 0);
    QJsonObject paramObj = paramsArr[ 0 ].toObject();
    QVERIFY(paramObj.contains(QStringLiteral("name")));
    QVERIFY(paramObj.contains(QStringLiteral("type")));
}

// ============================================================
// testToJsonSparseStrategy — 稀疏序列化策略验证
// ============================================================

/**
 * @brief 验证 toJson() 稀疏策略：省略空/默认值字段
 *
 * 默认构造的描述符 toJson() 应仅包含 name 和 qualified_name（均为空字符串），
 * 不包含空 icon、空数组（inputs/outputs/parameters）、
 * 默认 renderTemplate（NodeStyleTemplate）或默认 style。
 */
void TestDANodeDescriptor::testToJsonSparseStrategy()
{
    // 1) 全默认描述符：仅应有 name 和 qualified_name 键
    DANodeDescriptor defaultDesc;
    QJsonObject defaultJson = defaultDesc.toJson();

    QVERIFY(defaultJson.contains(QStringLiteral("name")));
    QVERIFY(defaultJson.contains(QStringLiteral("qualified_name")));
    QVERIFY(!defaultJson.contains(QStringLiteral("category")));         // 空 category → 省略
    QVERIFY(!defaultJson.contains(QStringLiteral("icon")));             // 空 icon → 省略
    QVERIFY(!defaultJson.contains(QStringLiteral("inputs")));           // 空 inputs → 省略
    QVERIFY(!defaultJson.contains(QStringLiteral("outputs")));          // 空 outputs → 省略
    QVERIFY(!defaultJson.contains(QStringLiteral("parameters")));       // 空 parameters → 省略
    QVERIFY(!defaultJson.contains(QStringLiteral("render_template")));  // 默认 NodeStyleTemplate → 省略
    QVERIFY(!defaultJson.contains(QStringLiteral("style")));            // 默认 style → 省略

    // 2) 仅 category 非空：应包含 category
    DANodeDescriptor descWithCategory;
    descWithCategory.name          = QStringLiteral("节点");
    descWithCategory.qualifiedName = QStringLiteral("pkg.node");
    descWithCategory.category      = QStringLiteral("数据");
    QJsonObject catJson            = descWithCategory.toJson();
    QVERIFY(catJson.contains(QStringLiteral("category")));
    QVERIFY(!catJson.contains(QStringLiteral("icon")));
    QVERIFY(!catJson.contains(QStringLiteral("inputs")));

    // 3) 仅 icon 非空：应包含 icon
    DANodeDescriptor descWithIcon;
    descWithIcon.name          = QStringLiteral("节点");
    descWithIcon.qualifiedName = QStringLiteral("pkg.node");
    descWithIcon.icon          = QStringLiteral(":/icon.svg");
    QJsonObject iconJson       = descWithIcon.toJson();
    QVERIFY(iconJson.contains(QStringLiteral("icon")));
    QVERIFY(!iconJson.contains(QStringLiteral("category")));

    // 4) WidgetTemplate → render_template 应写入
    DANodeDescriptor descWidget;
    descWidget.name           = QStringLiteral("节点");
    descWidget.qualifiedName  = QStringLiteral("pkg.node");
    descWidget.renderTemplate = RenderTemplate::WidgetTemplate;
    QJsonObject widgetJson    = descWidget.toJson();
    QVERIFY(widgetJson.contains(QStringLiteral("render_template")));
    QCOMPARE(widgetJson.value(QStringLiteral("render_template")).toString(), QStringLiteral("widget"));
}

// ============================================================
// testFromJsonFullFields — fromJson 完整字段验证
// ============================================================

/**
 * @brief 验证 fromJson() 在所有字段均存在时正确反序列化
 *
 * 构造包含完整字段的 JSON 对象，验证每个字段被正确还原。
 */
void TestDANodeDescriptor::testFromJsonFullFields()
{
    QJsonObject obj;
    obj[ QStringLiteral("name") ]           = QStringLiteral("完整节点");
    obj[ QStringLiteral("qualified_name") ] = QStringLiteral("data_workbench.full_node");
    obj[ QStringLiteral("category") ]       = QStringLiteral("完整分类");
    obj[ QStringLiteral("icon") ]           = QStringLiteral(":/icons/full.svg");

    // inputs 数组
    QJsonArray inputsArr;
    QJsonObject inputObj;
    inputObj[ QStringLiteral("name") ]      = QStringLiteral("data_in");
    inputObj[ QStringLiteral("data_type") ] = QStringLiteral("DataFrame");
    inputObj[ QStringLiteral("required") ]  = false;
    inputsArr.append(inputObj);
    obj[ QStringLiteral("inputs") ] = inputsArr;

    // outputs 数组
    QJsonArray outputsArr;
    QJsonObject outputObj;
    outputObj[ QStringLiteral("name") ]      = QStringLiteral("data_out");
    outputObj[ QStringLiteral("data_type") ] = QStringLiteral("Series");
    outputsArr.append(outputObj);
    obj[ QStringLiteral("outputs") ] = outputsArr;

    // parameters 数组
    QJsonArray paramsArr;
    QJsonObject paramObj;
    paramObj[ QStringLiteral("name") ]        = QStringLiteral("threshold");
    paramObj[ QStringLiteral("type") ]        = QStringLiteral("float");
    paramObj[ QStringLiteral("description") ] = QStringLiteral("筛选阈值");
    paramObj[ QStringLiteral("default") ]     = 0.5;
    paramsArr.append(paramObj);
    obj[ QStringLiteral("parameters") ] = paramsArr;

    obj[ QStringLiteral("render_template") ] = QStringLiteral("widget");

    // style 子对象
    QJsonObject styleObj;
    styleObj[ QStringLiteral("bodyShape") ]    = QStringLiteral("ellipse");
    styleObj[ QStringLiteral("cornerRadius") ] = 10.0;
    obj[ QStringLiteral("style") ]             = styleObj;

    DANodeDescriptor desc = DANodeDescriptor::fromJson(obj);

    QCOMPARE(desc.name, QStringLiteral("完整节点"));
    QCOMPARE(desc.qualifiedName, QStringLiteral("data_workbench.full_node"));
    QCOMPARE(desc.category, QStringLiteral("完整分类"));
    QCOMPARE(desc.icon, QStringLiteral(":/icons/full.svg"));
    QCOMPARE(desc.inputs.size(), 1);
    QCOMPARE(desc.inputs[ 0 ].name, QStringLiteral("data_in"));
    QCOMPARE(desc.inputs[ 0 ].dataType, QStringLiteral("DataFrame"));
    QCOMPARE(desc.inputs[ 0 ].required, false);
    QCOMPARE(desc.outputs.size(), 1);
    QCOMPARE(desc.outputs[ 0 ].name, QStringLiteral("data_out"));
    QCOMPARE(desc.outputs[ 0 ].dataType, QStringLiteral("Series"));
    QCOMPARE(desc.parameters.size(), 1);
    QCOMPARE(desc.parameters[ 0 ].name, QStringLiteral("threshold"));
    QCOMPARE(desc.parameters[ 0 ].type, QStringLiteral("float"));
    QCOMPARE(desc.parameters[ 0 ].description, QStringLiteral("筛选阈值"));
    QCOMPARE(desc.renderTemplate, RenderTemplate::WidgetTemplate);
    QCOMPARE(desc.style.bodyShape, BodyShape::Ellipse);
    QCOMPARE(desc.style.cornerRadius, 10.0);
}

// ============================================================
// testFromJsonMissingOptionalFields — fromJson 缺失可选字段验证
// ============================================================

/**
 * @brief 验证 fromJson() 在缺失可选字段时使用默认值回退
 *
 * 仅提供 name 和 qualified_name 的最小 JSON，
 * 缺失的 category、icon、inputs、outputs、parameters、
 * render_template、style 应使用默认值。
 */
void TestDANodeDescriptor::testFromJsonMissingOptionalFields()
{
    QJsonObject obj;
    obj[ QStringLiteral("name") ]           = QStringLiteral("最小节点");
    obj[ QStringLiteral("qualified_name") ] = QStringLiteral("pkg.min_node");

    DANodeDescriptor desc = DANodeDescriptor::fromJson(obj);

    QCOMPARE(desc.name, QStringLiteral("最小节点"));
    QCOMPARE(desc.qualifiedName, QStringLiteral("pkg.min_node"));

    // 缺失字段应为默认值
    QVERIFY(desc.category.isEmpty());
    QVERIFY(desc.icon.isEmpty());
    QVERIFY(desc.inputs.isEmpty());
    QVERIFY(desc.outputs.isEmpty());
    QVERIFY(desc.parameters.isEmpty());
    QCOMPARE(desc.renderTemplate, RenderTemplate::NodeStyleTemplate);
    QCOMPARE(desc.style.bodyShape, BodyShape::RoundedRect);  // 默认 DANodeStyle
    QCOMPARE(desc.style.cornerRadius, 4.0);

    // 验证向后兼容："rect" 和 "svg" 映射到 NodeStyleTemplate
    QJsonObject rectObj;
    rectObj[ QStringLiteral("name") ]            = QStringLiteral("旧节点");
    rectObj[ QStringLiteral("qualified_name") ]  = QStringLiteral("pkg.old_rect");
    rectObj[ QStringLiteral("render_template") ] = QStringLiteral("rect");
    DANodeDescriptor rectDesc                    = DANodeDescriptor::fromJson(rectObj);
    QCOMPARE(rectDesc.renderTemplate, RenderTemplate::NodeStyleTemplate);

    QJsonObject svgObj;
    svgObj[ QStringLiteral("name") ]            = QStringLiteral("旧节点2");
    svgObj[ QStringLiteral("qualified_name") ]  = QStringLiteral("pkg.old_svg");
    svgObj[ QStringLiteral("render_template") ] = QStringLiteral("svg");
    DANodeDescriptor svgDesc                    = DANodeDescriptor::fromJson(svgObj);
    QCOMPARE(svgDesc.renderTemplate, RenderTemplate::NodeStyleTemplate);
}

}  // namespace DA
