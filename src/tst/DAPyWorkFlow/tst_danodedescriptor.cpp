#include "tst_danodedescriptor.h"
#include "DANodeDescriptor.h"
#include "DAPortDescriptor.h"
#include "DAParameterDescriptor.h"
#include "DAPyNodeStyle.h"
#include "DAPyNodeStyleDefine.h"
#include "DAPyNodeFactory.h"
#include <QtTest/QtTest>

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

}  // namespace DA
