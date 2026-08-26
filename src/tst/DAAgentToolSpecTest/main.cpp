// DAAgentToolSpecTest/main.cpp
// 单元测试：DAAgentToolSpec → OpenAI function schema 的序列化等价性
//（DAAgentToolSpecJson.h 的 DA::toJson）。
//
// 背景：getToolSpec() 从手写 QJsonObject 迁移为结构化 DAAgentToolSpec 后，
// 序列化输出必须与旧手写 JSON 语义等价，否则会悄悄改变下发给 LLM 的工具
// schema（Python 侧 bind_tools 直接消费）。期望值按旧实现的 JSON 形态构造
//（分步赋值构造，避免 MSVC 对多层嵌套 QJsonObject 初始化列表的推导缺陷）。
// 归一化约定：required 数组为空时省略该键（旧实现有 3 个工具输出空数组，
// JSON Schema 语义不变）。

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonObject>

#include "DAAgentToolSpec.h"
#include "DAAgentToolSpecJson.h"

using DA::DAAgentToolParam;
using DA::DAAgentToolSpec;
using PType = DAAgentToolParam::Type;

class DAAgentToolSpecTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testFlatParams();            // 平参数（get_data_info 形态）
    void testArrayParam();            // 数组参数 + 单元素 items（create_chart.y 形态）
    void testUnionTypes();            // 联合 items / 联合主类型（add_annotation 形态）
    void testFreeFormObject();        // free-form object（run_code.args 形态）
    void testEnumAndDefault();        // enum / default 字段输出与省略
    void testEmptyRequiredOmitted();  // 无必填参数时省略 required 键
    void testEmptyParams();           // 无参数工具（list_data 形态）
};

// ---------------------------------------------------------------------------
// 平参数：旧 DAAgentToolDataInfo::getToolSpec 的 JSON 形态
// ---------------------------------------------------------------------------
void DAAgentToolSpecTest::testFlatParams()
{
    DAAgentToolSpec spec{QStringLiteral("get_data_info"),
                         QStringLiteral("Get dataset schema and a preview of the first N rows.")};
    spec.addParam({QStringLiteral("data_name"), QStringLiteral("Dataset name"), {PType::String}, true});
    spec.addParam({QStringLiteral("preview_rows"),
                   QStringLiteral("Number of preview rows, default 10"),
                   {PType::Integer}});

    QJsonObject dataNameProp;
    dataNameProp[QStringLiteral("type")]        = QStringLiteral("string");
    dataNameProp[QStringLiteral("description")] = QStringLiteral("Dataset name");
    QJsonObject previewRowsProp;
    previewRowsProp[QStringLiteral("type")]        = QStringLiteral("integer");
    previewRowsProp[QStringLiteral("description")] = QStringLiteral("Number of preview rows, default 10");
    QJsonObject properties;
    properties[QStringLiteral("data_name")]    = dataNameProp;
    properties[QStringLiteral("preview_rows")] = previewRowsProp;
    QJsonObject parameters;
    parameters[QStringLiteral("type")]       = QStringLiteral("object");
    parameters[QStringLiteral("properties")] = properties;
    parameters[QStringLiteral("required")]   = QJsonArray{QStringLiteral("data_name")};
    QJsonObject expected;
    expected[QStringLiteral("name")]        = QStringLiteral("get_data_info");
    expected[QStringLiteral("description")] = QStringLiteral("Get dataset schema and a preview of the first N rows.");
    expected[QStringLiteral("parameters")]  = parameters;

    QCOMPARE(DA::toJson(spec), expected);
}

// ---------------------------------------------------------------------------
// 数组参数：旧 DAAgentToolCreateChart 的 y 参数形态
// ---------------------------------------------------------------------------
void DAAgentToolSpecTest::testArrayParam()
{
    DAAgentToolSpec spec{QStringLiteral("create_chart"), QStringLiteral("Create a chart.")};
    DAAgentToolParam y{QStringLiteral("y"), QStringLiteral("Y-axis column name(s)."), {PType::Array}, true};
    y.itemTypes = {PType::String};
    spec.addParam(y);

    QJsonObject items;
    items[QStringLiteral("type")] = QStringLiteral("string");
    QJsonObject yProp;
    yProp[QStringLiteral("type")]        = QStringLiteral("array");
    yProp[QStringLiteral("description")] = QStringLiteral("Y-axis column name(s).");
    yProp[QStringLiteral("items")]       = items;
    QJsonObject properties;
    properties[QStringLiteral("y")] = yProp;
    QJsonObject parameters;
    parameters[QStringLiteral("type")]       = QStringLiteral("object");
    parameters[QStringLiteral("properties")] = properties;
    parameters[QStringLiteral("required")]   = QJsonArray{QStringLiteral("y")};
    QJsonObject expected;
    expected[QStringLiteral("name")]        = QStringLiteral("create_chart");
    expected[QStringLiteral("description")] = QStringLiteral("Create a chart.");
    expected[QStringLiteral("parameters")]  = parameters;

    QCOMPARE(DA::toJson(spec), expected);
}

// ---------------------------------------------------------------------------
// 联合类型：旧 DAAgentToolAddAnnotation 的 position（联合 items）
// 与 start_x（联合主类型）形态
// ---------------------------------------------------------------------------
void DAAgentToolSpecTest::testUnionTypes()
{
    DAAgentToolSpec spec{QStringLiteral("add_annotation"), QStringLiteral("Add an annotation.")};
    DAAgentToolParam position{QStringLiteral("position"), QStringLiteral("Position [x, y]."), {PType::Array}};
    position.itemTypes = {PType::Number, PType::String};
    spec.addParam(position);
    spec.addParam({QStringLiteral("start_x"), QStringLiteral("Region start x."), {PType::Number, PType::String}});

    QJsonObject items;
    items[QStringLiteral("type")] = QJsonArray{QStringLiteral("number"), QStringLiteral("string")};
    QJsonObject positionProp;
    positionProp[QStringLiteral("type")]        = QStringLiteral("array");
    positionProp[QStringLiteral("description")] = QStringLiteral("Position [x, y].");
    positionProp[QStringLiteral("items")]       = items;
    QJsonObject startXProp;
    startXProp[QStringLiteral("type")]        = QJsonArray{QStringLiteral("number"), QStringLiteral("string")};
    startXProp[QStringLiteral("description")] = QStringLiteral("Region start x.");
    QJsonObject properties;
    properties[QStringLiteral("position")] = positionProp;
    properties[QStringLiteral("start_x")]  = startXProp;
    QJsonObject parameters;
    parameters[QStringLiteral("type")]       = QStringLiteral("object");
    parameters[QStringLiteral("properties")] = properties;
    QJsonObject expected;
    expected[QStringLiteral("name")]        = QStringLiteral("add_annotation");
    expected[QStringLiteral("description")] = QStringLiteral("Add an annotation.");
    expected[QStringLiteral("parameters")]  = parameters;

    QCOMPARE(DA::toJson(spec), expected);
}

// ---------------------------------------------------------------------------
// free-form object：旧 DAAgentToolRunCode 的 args 参数形态
// ---------------------------------------------------------------------------
void DAAgentToolSpecTest::testFreeFormObject()
{
    DAAgentToolSpec spec{QStringLiteral("run_code"), QStringLiteral("Execute code.")};
    spec.addParam({QStringLiteral("code"), QStringLiteral("Python code to execute"), {PType::String}, true});
    spec.addParam({QStringLiteral("args"),
                   QStringLiteral("Optional arguments injected as the 'args' dict in the namespace"),
                   {PType::Object}});

    QJsonObject codeProp;
    codeProp[QStringLiteral("type")]        = QStringLiteral("string");
    codeProp[QStringLiteral("description")] = QStringLiteral("Python code to execute");
    QJsonObject argsProp;
    argsProp[QStringLiteral("type")]        = QStringLiteral("object");
    argsProp[QStringLiteral("description")] = QStringLiteral("Optional arguments injected as the 'args' dict in the "
                                                             "namespace");
    QJsonObject properties;
    properties[QStringLiteral("code")] = codeProp;
    properties[QStringLiteral("args")] = argsProp;
    QJsonObject parameters;
    parameters[QStringLiteral("type")]       = QStringLiteral("object");
    parameters[QStringLiteral("properties")] = properties;
    parameters[QStringLiteral("required")]   = QJsonArray{QStringLiteral("code")};
    QJsonObject expected;
    expected[QStringLiteral("name")]        = QStringLiteral("run_code");
    expected[QStringLiteral("description")] = QStringLiteral("Execute code.");
    expected[QStringLiteral("parameters")]  = parameters;

    QCOMPARE(DA::toJson(spec), expected);
}

// ---------------------------------------------------------------------------
// enum / default：有值时输出，空/invalid 时省略
// ---------------------------------------------------------------------------
void DAAgentToolSpecTest::testEnumAndDefault()
{
    DAAgentToolSpec spec{QStringLiteral("demo_tool"), QStringLiteral("Demo.")};
    DAAgentToolParam style{QStringLiteral("style"), QStringLiteral("Chart style"), {PType::String}};
    style.enumValues   = QVariantList{QStringLiteral("dark"), QStringLiteral("light")};
    style.defaultValue = QStringLiteral("dark");
    spec.addParam(style);
    spec.addParam({QStringLiteral("plain"), QStringLiteral("No extras"), {PType::String}});

    const QJsonObject actual = DA::toJson(spec);
    const QJsonObject props  = actual[QStringLiteral("parameters")].toObject()[QStringLiteral("properties")].toObject();
    const QJsonObject styleObj = props[QStringLiteral("style")].toObject();
    QCOMPARE(styleObj[QStringLiteral("enum")].toArray(),
             (QJsonArray{QStringLiteral("dark"), QStringLiteral("light")}));
    QCOMPARE(styleObj[QStringLiteral("default")].toString(), QStringLiteral("dark"));
    // plain 参数不携带 enum / default 键
    QVERIFY(!props[QStringLiteral("plain")].toObject().contains(QStringLiteral("enum")));
    QVERIFY(!props[QStringLiteral("plain")].toObject().contains(QStringLiteral("default")));
}

// ---------------------------------------------------------------------------
// 归一化约定：无必填参数时省略 required 键
// ---------------------------------------------------------------------------
void DAAgentToolSpecTest::testEmptyRequiredOmitted()
{
    DAAgentToolSpec spec{QStringLiteral("set_chart_style"), QStringLiteral("Set styles.")};
    spec.addParam({QStringLiteral("title"), QStringLiteral("Chart title"), {PType::String}});

    const QJsonObject actual = DA::toJson(spec);
    QVERIFY(!actual[QStringLiteral("parameters")].toObject().contains(QStringLiteral("required")));
}

// ---------------------------------------------------------------------------
// 无参数工具：旧 DAAgentToolListData 形态（properties 为空 object）
// ---------------------------------------------------------------------------
void DAAgentToolSpecTest::testEmptyParams()
{
    DAAgentToolSpec spec{QStringLiteral("list_data"), QStringLiteral("List all datasets.")};

    QJsonObject parameters;
    parameters[QStringLiteral("type")]       = QStringLiteral("object");
    parameters[QStringLiteral("properties")] = QJsonObject();
    QJsonObject expected;
    expected[QStringLiteral("name")]        = QStringLiteral("list_data");
    expected[QStringLiteral("description")] = QStringLiteral("List all datasets.");
    expected[QStringLiteral("parameters")]  = parameters;

    QCOMPARE(DA::toJson(spec), expected);
}

QTEST_GUILESS_MAIN(DAAgentToolSpecTest)
#include "main.moc"
