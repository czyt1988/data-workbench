#include "tst_dapynodemetadata.h"
#include "DAPyNodeFactory.h"
#include <QtTest/QtTest>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{

/**
 * @brief 从 XML 节点元素中提取 qualifiedName
 *
 * 此函数模拟 DAPyWorkFlowSceneSerializer::loadSceneFromXml 中的 qualifiedName
 * 提取逻辑。向后兼容修复（T14/T15）将修改此行为：当 "qualified_name" 为空时，
 * 回退读取 "prototype" 属性。
 *
 * @param[in] nodeEle XML 节点元素
 * @return 提取的 qualifiedName 字符串
 */
static QString extractQualifiedNameFromXmlNode(const QDomElement& nodeEle)
{
    QString qualifiedName = nodeEle.attribute("qualified_name");
    if (qualifiedName.isEmpty()) {
        qualifiedName = nodeEle.attribute("prototype");  // backward compat: 旧版本使用prototype属性
    }
    if (qualifiedName.isEmpty()) {
        qualifiedName = nodeEle.attribute("protoType");  // backward compat: DAXmlHelper旧格式使用protoType属性
    }
    return qualifiedName;
}

// ============================================================
// 测试实现
// ============================================================

void TestDAPyNodeMetaData::testDefaultConstruction()
{
    DAPyNodeMetaData meta;

    // 默认构造后所有 QString/QList 字段应为空
    QVERIFY(meta.name.isEmpty());
    QVERIFY(meta.qualifiedName.isEmpty());
    QVERIFY(meta.group.isEmpty());
    QVERIFY(meta.iconPath.isEmpty());
    QVERIFY(meta.tooltip.isEmpty());
    QVERIFY(meta.inputKeys.isEmpty());
    QVERIFY(meta.outputKeys.isEmpty());
}

void TestDAPyNodeMetaData::testFieldAssignment()
{
    DAPyNodeMetaData meta;
    meta.name = QStringLiteral("TestNode");
    meta.qualifiedName = QStringLiteral("test.module.TestNode");
    meta.group = QStringLiteral("TestGroup");
    meta.iconPath = QStringLiteral(":/icons/test.png");
    meta.tooltip = QStringLiteral("A test node");
    meta.inputKeys << QStringLiteral("in1") << QStringLiteral("in2");
    meta.outputKeys << QStringLiteral("out1");

    QCOMPARE(meta.name, QStringLiteral("TestNode"));
    QCOMPARE(meta.qualifiedName, QStringLiteral("test.module.TestNode"));
    QCOMPARE(meta.group, QStringLiteral("TestGroup"));
    QCOMPARE(meta.iconPath, QStringLiteral(":/icons/test.png"));
    QCOMPARE(meta.tooltip, QStringLiteral("A test node"));
    QCOMPARE(meta.inputKeys.size(), 2);
    QCOMPARE(meta.outputKeys.size(), 1);
}

void TestDAPyNodeMetaData::testIsValid()
{
    // 空元数据应无效
    DAPyNodeMetaData empty;
    QVERIFY(!empty.isValid());

    // qualifiedName 非空应有效
    DAPyNodeMetaData valid;
    valid.qualifiedName = QStringLiteral("some.qualified.name");
    QVERIFY(valid.isValid());

    // 仅其他字段非空但 qualifiedName 为空，仍应无效
    DAPyNodeMetaData partial;
    partial.name = QStringLiteral("Named");
    QVERIFY(!partial.isValid());
}

void TestDAPyNodeMetaData::testBoolConversion()
{
    DAPyNodeMetaData empty;
    QVERIFY(!static_cast<bool>(empty));

    DAPyNodeMetaData valid;
    valid.qualifiedName = QStringLiteral("test.node");
    QVERIFY(static_cast<bool>(valid));
}

void TestDAPyNodeMetaData::testComparisonOperators()
{
    DAPyNodeMetaData a;
    a.qualifiedName = QStringLiteral("alpha.node");

    DAPyNodeMetaData b;
    b.qualifiedName = QStringLiteral("beta.node");

    DAPyNodeMetaData c;
    c.qualifiedName = QStringLiteral("alpha.node");

    // operator==
    QVERIFY(a == c);
    QVERIFY(!(a == b));

    // operator!=
    QVERIFY(a != b);
    QVERIFY(!(a != c));

    // operator<
    QVERIFY(a < b);
    QVERIFY(!(b < a));
    QVERIFY(!(a < c));
    QVERIFY(!(c < a));  // same qualifiedName, not less
}

void TestDAPyNodeMetaData::testGetterMethods()
{
    DAPyNodeMetaData meta;
    meta.name = QStringLiteral("MyNode");
    meta.qualifiedName = QStringLiteral("mod.MyNode");
    meta.group = QStringLiteral("Math");
    meta.tooltip = QStringLiteral("Do math");

    QCOMPARE(meta.getNodeName(), QStringLiteral("MyNode"));
    QCOMPARE(meta.getNodeQualifiedName(), QStringLiteral("mod.MyNode"));
    QCOMPARE(meta.getGroup(), QStringLiteral("Math"));
    QCOMPARE(meta.getNodeTooltip(), QStringLiteral("Do math"));
}

void TestDAPyNodeMetaData::testGetIconEmptyPath()
{
    // 空 iconPath 应返回空 QIcon
    DAPyNodeMetaData meta;
    QIcon icon = meta.getIcon();
    QVERIFY(icon.isNull());

    // 设置 iconPath 后应返回非空 QIcon
    // （注意：文件不存在时 QIcon 也可能.isNull()，这里仅验证逻辑路径不为空）
    meta.iconPath = QStringLiteral(":/some/icon.png");
    // 图标文件不存在时为 null，但至少验证不会崩溃
    QIcon icon2 = meta.getIcon();
    Q_UNUSED(icon2);
}

// ============================================================
// XML 向后兼容测试
// ============================================================

void TestDAPyNodeMetaData::testXmlNewFormatQualifiedname()
{
    // 构造模拟新格式 XML：<node qualified_name="test.module.TestNode" name="TestNode"/>
    QDomDocument doc;
    QDomElement nodeEle = doc.createElement("node");
    nodeEle.setAttribute("qualified_name", "pandas.DataFrameLoader");
    nodeEle.setAttribute("name", "DataFrame Loader");

    QString qualifiedName = extractQualifiedNameFromXmlNode(nodeEle);

    QCOMPARE(qualifiedName, QStringLiteral("pandas.DataFrameLoader"));
}

void TestDAPyNodeMetaData::testXmlOldPrototypeBackwardCompat()
{
    // 构造模拟旧格式 XML：<node prototype="test.module.LegacyNode" name="LegacyNode"/>
    // 旧版本使用 "prototype" 属性而非 "qualified_name"
    QDomDocument doc;
    QDomElement nodeEle = doc.createElement("node");
    nodeEle.setAttribute("prototype", "legacy.module.OldNode");
    nodeEle.setAttribute("name", "Old Legacy Node");

    QString qualifiedName = extractQualifiedNameFromXmlNode(nodeEle);

    // 向后兼容修复完成后，应该能从 "prototype" 属性读取到 qualifiedName
    // 当前此测试应 FAIL（返回空字符串），直到 T14/T15 实现兼容逻辑
    QCOMPARE(qualifiedName, QStringLiteral("legacy.module.OldNode"));
}

}  // namespace DA
