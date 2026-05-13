#include "tst_danodestyle.h"
#include "DAPyNodeStyle.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyNodeStyleDefine.h"
#include <QtTest/QtTest>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{

// ============================================================
// testDefaultConstruction — 默认构造验证
// ============================================================

/**
 * @brief 验证 DANodeStyle 默认构造函数的所有字段值
 *
 * DANodeStyle构造函数调用setDefaults()，应设置：
 * - bodyShape == RoundedRect
 * - namePosition == Inside
 * - iconPosition == LeftOfText
 * - cornerRadius == 4.0
 * - inputPortSide == West (AspectDirection::West)
 * - outputPortSide == East (AspectDirection::East)
 */
void TestDANodeStyle::testDefaultConstruction()
{
    DANodeStyle style;

    QCOMPARE(style.bodyShape, BodyShape::RoundedRect);
    QCOMPARE(style.namePosition, NamePosition::Inside);
    QCOMPARE(style.iconPosition, IconPosition::LeftOfText);
    QCOMPARE(style.cornerRadius, 4.0);
    QCOMPARE(style.inputPortSide, PortSide::West);
    QCOMPARE(style.outputPortSide, PortSide::East);

    // 额外验证其他关键默认值
    QCOMPARE(style.backgroundColor, QColor(240, 240, 240));
    QCOMPARE(style.borderColor, QColor(180, 180, 180));
    QCOMPARE(style.borderWidth, 1.0);
    QCOMPARE(style.iconSize, 24.0);
    QCOMPARE(style.layoutStrategy, LinkPointLayoutStrategy::Auto);
    QCOMPARE(style.bodyIconType, BodyIconType::None);
    QVERIFY(style.bodyIconSource.isEmpty());
    QCOMPARE(style.bodyIconScale, 0.8);
}

// ============================================================
// testFieldAssignment — 字段赋值读写验证
// ============================================================

/**
 * @brief 验证 DANodeStyle 各字段的赋值与读回
 *
 * 构造DANodeStyle，逐字段修改并读回验证，确保公共字段可正确赋值和读取。
 */
void TestDANodeStyle::testFieldAssignment()
{
    DANodeStyle style;

    style.bodyShape = BodyShape::Ellipse;
    QCOMPARE(style.bodyShape, BodyShape::Ellipse);

    style.namePosition = NamePosition::Below;
    QCOMPARE(style.namePosition, NamePosition::Below);

    style.iconPosition = IconPosition::AboveText;
    QCOMPARE(style.iconPosition, IconPosition::AboveText);

    style.borderWidth = 3.5;
    QCOMPARE(style.borderWidth, 3.5);

    style.cornerRadius = 12.0;
    QCOMPARE(style.cornerRadius, 12.0);

    style.iconSize = 32.0;
    QCOMPARE(style.iconSize, 32.0);

    style.inputPortSide = PortSide::North;
    style.outputPortSide = PortSide::South;
    QCOMPARE(style.inputPortSide, PortSide::North);
    QCOMPARE(style.outputPortSide, PortSide::South);

    style.layoutStrategy = LinkPointLayoutStrategy::Manual;
    QCOMPARE(style.layoutStrategy, LinkPointLayoutStrategy::Manual);

    style.bodyIconType = BodyIconType::Svg;
    style.bodyIconSource = ":/icons/node.svg";
    style.bodyIconScale = 1.0;
    QCOMPARE(style.bodyIconType, BodyIconType::Svg);
    QCOMPARE(style.bodyIconSource, QString(":/icons/node.svg"));
    QCOMPARE(style.bodyIconScale, 1.0);
}

// ============================================================
// testColorPropertyRoundTrip — 颜色属性往返验证
// ============================================================

/**
 * @brief 验证 DANodeStyle 颜色字段的赋值与读回
 *
 * 设置 backgroundColor 和 borderColor 为不同颜色值，
 * 读回验证颜色值一致，包括 RGB 分量、透明度和有效性。
 */
void TestDANodeStyle::testColorPropertyRoundTrip()
{
    DANodeStyle style;

    style.backgroundColor = QColor(100, 200, 50);
    QCOMPARE(style.backgroundColor, QColor(100, 200, 50));
    QCOMPARE(style.backgroundColor.red(), 100);
    QCOMPARE(style.backgroundColor.green(), 200);
    QCOMPARE(style.backgroundColor.blue(), 50);
    QVERIFY(style.backgroundColor.isValid());

    style.borderColor = QColor(Qt::red);
    QCOMPARE(style.borderColor, QColor(Qt::red));
    QVERIFY(style.borderColor.isValid());

    style.backgroundColor = QColor(50, 100, 150, 128);
    QCOMPARE(style.backgroundColor.alpha(), 128);
    QCOMPARE(style.backgroundColor.red(), 50);
    QCOMPARE(style.backgroundColor.green(), 100);
    QCOMPARE(style.backgroundColor.blue(), 150);

    style.borderColor = QColor();
    QVERIFY(!style.borderColor.isValid());
}

// ============================================================
// testPortStyleAssignment — 端口样式字段赋值验证
// ============================================================

/**
 * @brief 验证 DAPyLinkPointStyle 字段赋值与读回
 *
 * 修改 inputPortStyle 和 outputPortStyle 的 shape、fillColor、borderColor、borderWidth，
 * 读回验证值一致，并验证 isFillColorValid/isBorderColorValid 辅助方法。
 */
void TestDANodeStyle::testPortStyleAssignment()
{
    DANodeStyle style;

    style.inputPortStyle.shape      = PortShape::Circle;
    style.inputPortStyle.fillColor  = QColor(Qt::white);
    style.inputPortStyle.borderColor = QColor(Qt::black);
    style.inputPortStyle.borderWidth = 2.0;

    QCOMPARE(style.inputPortStyle.shape, PortShape::Circle);
    QCOMPARE(style.inputPortStyle.fillColor, QColor(Qt::white));
    QCOMPARE(style.inputPortStyle.borderColor, QColor(Qt::black));
    QCOMPARE(style.inputPortStyle.borderWidth, 2.0);
    QVERIFY(style.inputPortStyle.isFillColorValid());
    QVERIFY(style.inputPortStyle.isBorderColorValid());

    style.outputPortStyle.shape      = PortShape::Diamond;
    style.outputPortStyle.fillColor  = QColor(Qt::darkGray);
    style.outputPortStyle.borderColor = QColor(Qt::blue);
    style.outputPortStyle.borderWidth = 1.5;

    QCOMPARE(style.outputPortStyle.shape, PortShape::Diamond);
    QCOMPARE(style.outputPortStyle.fillColor, QColor(Qt::darkGray));
    QCOMPARE(style.outputPortStyle.borderColor, QColor(Qt::blue));
    QCOMPARE(style.outputPortStyle.borderWidth, 1.5);
    QVERIFY(style.outputPortStyle.isFillColorValid());
    QVERIFY(style.outputPortStyle.isBorderColorValid());
}

// ============================================================
// testSetDefaultsReset — setDefaults重置验证
// ============================================================

/**
 * @brief 验证 setDefaults() 能将已修改的字段重置回默认值
 *
 * 先修改多个字段，调用 setDefaults()，验证所有字段恢复默认。
 */
void TestDANodeStyle::testSetDefaultsReset()
{
    DANodeStyle style;

    style.bodyShape       = BodyShape::Ellipse;
    style.backgroundColor = QColor(Qt::red);
    style.borderWidth     = 5.0;
    style.cornerRadius    = 20.0;
    style.inputPortSide   = PortSide::North;
    style.outputPortSide  = PortSide::South;
    style.layoutStrategy  = LinkPointLayoutStrategy::Manual;
    style.bodyIconType    = BodyIconType::Svg;

    style.setDefaults();

    QCOMPARE(style.bodyShape, BodyShape::RoundedRect);
    QCOMPARE(style.backgroundColor, QColor(240, 240, 240));
    QCOMPARE(style.borderWidth, 1.0);
    QCOMPARE(style.cornerRadius, 4.0);
    QCOMPARE(style.inputPortSide, PortSide::West);
    QCOMPARE(style.outputPortSide, PortSide::East);
    QCOMPARE(style.layoutStrategy, LinkPointLayoutStrategy::Auto);
    QCOMPARE(style.bodyIconType, BodyIconType::None);
}

// ============================================================
// testXmlRoundTrip — XML持久化往返
// ============================================================

/**
 * @brief 验证通过DAPyNodeGraphicsItem的XML往返持久化
 *
 * 构造DAPyNodeGraphicsItem，修改style.bodyShape=Ellipse，
 * saveToXml→loadFromXml，验证style.bodyShape被正确保存和恢复。
 */
void TestDANodeStyle::testXmlRoundTrip()
{
    DAPyNodeGraphicsItem item(nullptr);
    item.nodeStyle().setDefaults();
    item.nodeStyle().bodyShape    = BodyShape::Ellipse;
    item.nodeStyle().cornerRadius = 10.0;
    item.nodeStyle().namePosition = NamePosition::Below;

    // 保存到XML
    QDomDocument doc("test");
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    // DAGraphicsResizeableItem::saveToXml需要一个itemElement作为容器
    // DAPyNodeGraphicsItem::saveToXml将pyNodeItem追加到parentElement
    QDomElement container = doc.createElement("item");
    root.appendChild(container);

    QVERIFY(item.saveToXml(&doc, &container, QVersionNumber(1, 0, 0)));

    // 从XML加载
    DAPyNodeGraphicsItem loadedItem(nullptr);
    QVERIFY(loadedItem.loadFromXml(&container, QVersionNumber(1, 0, 0)));

    QCOMPARE(loadedItem.nodeStyle().bodyShape, BodyShape::Ellipse);
    QCOMPARE(loadedItem.nodeStyle().cornerRadius, 10.0);
    QCOMPARE(loadedItem.nodeStyle().namePosition, NamePosition::Below);

    // 未修改字段应保持默认
    QCOMPARE(loadedItem.nodeStyle().iconPosition, IconPosition::LeftOfText);
    QCOMPARE(loadedItem.nodeStyle().inputPortSide, PortSide::West);
    QCOMPARE(loadedItem.nodeStyle().outputPortSide, PortSide::East);
}

// ============================================================
// testXmlBackwardCompat — XML向后兼容
// ============================================================

/**
 * @brief 验证旧式XML（无style元素，renderTemplate="rect"）能正常加载
 *
 * 旧版保存的XML没有<style>子元素，renderTemplate属性为"rect"。
 * loadFromXml应能正常加载，style保持默认值。
 */
void TestDANodeStyle::testXmlBackwardCompat()
{
    // 手动构造旧式XML（模拟旧版保存的数据）
    // 必须包含基类 DAGraphicsItem 和 DAGraphicsResizeableItem 所需的子元素：
    // - <info> 子元素（flags, id, x, y 等）
    // - <resize-info> 子元素（width, height, enableResize 等）
    QDomDocument doc("test");
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    // 构造 item 容器 — 这是 loadFromXml 的入参
    // 基类会在 itemElement 下查找 <info> 和 <resize-info>
    QDomElement itemContainer = doc.createElement("item");
    root.appendChild(itemContainer);

    // <info> 子元素（DAGraphicsItem::loadFromXml 需要）
    QDomElement infoEle = doc.createElement("info");
    infoEle.setAttribute("flags", "0");
    infoEle.setAttribute("id", "1");
    infoEle.setAttribute("x", "0");
    infoEle.setAttribute("y", "0");
    infoEle.setAttribute("z", "0");
    infoEle.setAttribute("acceptDrops", "0");
    infoEle.setAttribute("enable", "1");
    infoEle.setAttribute("opacity", "1");
    infoEle.setAttribute("rotation", "0");
    infoEle.setAttribute("scale", "1");
    itemContainer.appendChild(infoEle);

    // <resize-info> 子元素（DAGraphicsResizeableItem::loadFromXml 需要）
    QDomElement resizeEle = doc.createElement("resize-info");
    resizeEle.setAttribute("enableResize", "1");
    resizeEle.setAttribute("width", "120");
    resizeEle.setAttribute("height", "80");
    resizeEle.setAttribute("maxWidth", "500");
    resizeEle.setAttribute("maxHeight", "500");
    resizeEle.setAttribute("minWidth", "60");
    resizeEle.setAttribute("minHeight", "40");
    itemContainer.appendChild(resizeEle);

    // 构造旧式 pyNodeItem（无 style 元素，renderTemplate="rect"）
    QDomElement pyNodeEle = doc.createElement("pyNodeItem");
    pyNodeEle.setAttribute("renderTemplate", "rect");
    pyNodeEle.setAttribute("nodeName", "OldNode");
    pyNodeEle.setAttribute("nodeState", "0");
    itemContainer.appendChild(pyNodeEle);

    // 加载
    DAPyNodeGraphicsItem loadedItem(nullptr);
    QVERIFY(loadedItem.loadFromXml(&itemContainer, QVersionNumber(1, 0, 0)));

    // renderTemplate应为RectTemplate（rect → NodeStyleTemplate 映射）
    QCOMPARE(loadedItem.getRenderTemplate(), DA::RenderTemplate::NodeStyleTemplate);

    // style应保持默认值（无style元素→mStyle未修改）
    DANodeStyle defaults;
    QCOMPARE(loadedItem.nodeStyle().bodyShape, defaults.bodyShape);
    QCOMPARE(loadedItem.nodeStyle().namePosition, defaults.namePosition);
    QCOMPARE(loadedItem.nodeStyle().cornerRadius, defaults.cornerRadius);
    QCOMPARE(loadedItem.nodeStyle().inputPortSide, defaults.inputPortSide);
    QCOMPARE(loadedItem.nodeStyle().outputPortSide, defaults.outputPortSide);
}

// ============================================================
// testLinkPointStyleDefaults — 连接点样式默认值
// ============================================================

/**
 * @brief 验证 DAPyLinkPointStyle 默认构造值
 *
 * DAPyLinkPointStyle默认构造：
 * - shape == PortShape::Rect
 * - fillColor.isValid() == false（无效颜色，使用运行时默认值）
 * - borderColor.isValid() == false（无效颜色，使用运行时默认值）
 * - borderWidth == 1.0
 */
void TestDANodeStyle::testLinkPointStyleDefaults()
{
    DAPyLinkPointStyle style;

    QCOMPARE(style.shape, PortShape::Rect);
    QVERIFY(!style.isFillColorValid());
    QVERIFY(!style.isBorderColorValid());
    QCOMPARE(style.borderWidth, 1.0);

    // 验证QColor::isValid()直接返回false
    QVERIFY(!style.fillColor.isValid());
    QVERIFY(!style.borderColor.isValid());
}

}  // namespace DA
