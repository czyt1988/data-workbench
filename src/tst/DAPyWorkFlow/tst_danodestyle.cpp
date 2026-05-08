#include "tst_danodestyle.h"
#include "DAPyNodeStyle.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyNodeStyleDefine.h"
#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonDocument>
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
// testJsonRoundTrip — JSON序列化往返
// ============================================================

/**
 * @brief 验证 DANodeStyle JSON往返序列化
 *
 * 修改bodyShape=Ellipse, namePosition=Below后，
 * 通过DANodeStyleToJson→DANodeStyleFromJson往返，
 * 验证值与修改后一致。
 */
void TestDANodeStyle::testJsonRoundTrip()
{
    DANodeStyle style;
    style.bodyShape     = BodyShape::Ellipse;
    style.namePosition  = NamePosition::Below;
    style.cornerRadius  = 8.0;
    style.inputPortSide = PortSide::North;

    QJsonObject json     = DANodeStyleToJson(style);
    DANodeStyle restored = DANodeStyleFromJson(json);

    QCOMPARE(restored.bodyShape, BodyShape::Ellipse);
    QCOMPARE(restored.namePosition, NamePosition::Below);
    QCOMPARE(restored.cornerRadius, 8.0);
    QCOMPARE(restored.inputPortSide, PortSide::North);

    // 未修改的字段应保持默认值
    QCOMPARE(restored.iconPosition, IconPosition::LeftOfText);
    QCOMPARE(restored.outputPortSide, PortSide::East);
    QCOMPARE(restored.borderWidth, 1.0);
}

// ============================================================
// testJsonSparseStrategy — 稀疏策略验证
// ============================================================

/**
 * @brief 验证稀疏JSON策略：默认样式toJson应产生空JSON
 *
 * DANodeStyleToJson使用稀疏策略，仅写入与默认值不同的字段。
 * 默认构造的样式与默认实例完全相同，因此toJson应返回空QJsonObject。
 */
void TestDANodeStyle::testJsonSparseStrategy()
{
    DANodeStyle style;  // 全默认
    QJsonObject json = DANodeStyleToJson(style);

    QVERIFY(json.isEmpty());
}

// ============================================================
// testJsonSafeDefaults — 安全默认值验证
// ============================================================

/**
 * @brief 验证fromJson空QJsonObject时所有字段为默认值
 *
 * DANodeStyleFromJson对缺失字段使用默认值，
 * 空JSON对象应产生与默认构造完全相同的样式。
 */
void TestDANodeStyle::testJsonSafeDefaults()
{
    QJsonObject emptyJson;
    DANodeStyle style = DANodeStyleFromJson(emptyJson);

    DANodeStyle defaults;

    QCOMPARE(style.bodyShape, defaults.bodyShape);
    QCOMPARE(style.namePosition, defaults.namePosition);
    QCOMPARE(style.iconPosition, defaults.iconPosition);
    QCOMPARE(style.backgroundColor, defaults.backgroundColor);
    QCOMPARE(style.borderColor, defaults.borderColor);
    QCOMPARE(style.borderWidth, defaults.borderWidth);
    QCOMPARE(style.cornerRadius, defaults.cornerRadius);
    QCOMPARE(style.iconSize, defaults.iconSize);
    QCOMPARE(style.inputPortSide, defaults.inputPortSide);
    QCOMPARE(style.outputPortSide, defaults.outputPortSide);
    QCOMPARE(style.layoutStrategy, defaults.layoutStrategy);
    QCOMPARE(style.bodyIconType, defaults.bodyIconType);
    QCOMPARE(style.bodyIconSource, defaults.bodyIconSource);
    QCOMPARE(style.bodyIconScale, defaults.bodyIconScale);
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

// ============================================================
// testColorSerialization — QColor hex往返
// ============================================================

/**
 * @brief 验证QColor通过toJson/fromJson的hex格式往返
 *
 * QColor通过name()方法序列化为"#RRGGBB"格式，
 * 通过QColor(hexString)反序列化。验证颜色往返后RGB值一致。
 */
void TestDANodeStyle::testColorSerialization()
{
    // 构造含非默认颜色的样式
    DANodeStyle style;
    style.backgroundColor = QColor("#ff0000");  // 红色
    style.borderColor     = QColor("#00ff00");  // 绿色

    QJsonObject json = DANodeStyleToJson(style);

    // 验证JSON中颜色字段存在且为hex格式
    QVERIFY(json.contains("backgroundColor"));
    QCOMPARE(json.value("backgroundColor").toString(), QString("#ff0000"));

    QVERIFY(json.contains("borderColor"));
    QCOMPARE(json.value("borderColor").toString(), QString("#00ff00"));

    // 从Json往返
    DANodeStyle restored = DANodeStyleFromJson(json);
    QCOMPARE(restored.backgroundColor, QColor("#ff0000"));
    QCOMPARE(restored.borderColor, QColor("#00ff00"));

    // 验证RGB分量精确一致
    QCOMPARE(restored.backgroundColor.red(), 255);
    QCOMPARE(restored.backgroundColor.green(), 0);
    QCOMPARE(restored.backgroundColor.blue(), 0);

    QCOMPARE(restored.borderColor.red(), 0);
    QCOMPARE(restored.borderColor.green(), 255);
    QCOMPARE(restored.borderColor.blue(), 0);

    // 连接点样式颜色往返
    DAPyLinkPointStyle lpStyle;
    lpStyle.fillColor   = QColor("#0000ff");  // 蓝色
    lpStyle.borderColor = QColor("#ffff00");  // 黄色

    // 通过DANodeStyle的inputPortStyle验证
    DANodeStyle nodeStyle;
    nodeStyle.inputPortStyle = lpStyle;

    QJsonObject nodeJson = DANodeStyleToJson(nodeStyle);
    QVERIFY(nodeJson.contains("inputPortStyle"));

    DANodeStyle restoredNode = DANodeStyleFromJson(nodeJson);
    QCOMPARE(restoredNode.inputPortStyle.fillColor, QColor("#0000ff"));
    QCOMPARE(restoredNode.inputPortStyle.borderColor, QColor("#ffff00"));
}

}  // namespace DA
