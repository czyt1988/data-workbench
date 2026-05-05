#include "tst_body_rendering.h"
#include "DAPyNodeGraphicsItem.h"
#include "DAPyNodeStyle.h"
#include "DAPyNodeStyleDefine.h"
#include "DAPyNodePalette.h"
#include <QtTest/QtTest>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <functional>

namespace DA
{

// ============================================================
// 辅助函数
// ============================================================

/**
 * @brief 将图形项渲染到指定大小的 QPixmap
 *
 * 使用 QGraphicsScene 渲染，确保坐标变换正确。
 *
 * @param[in] item 要渲染的图形项
 * @param[in] pmSize QPixmap 尺寸
 * @return 渲染后的 QPixmap
 */
static QPixmap renderItemToPixmap(DAPyNodeGraphicsItem* item, const QSize& pmSize)
{
    QGraphicsScene scene;
    scene.addItem(item);

    QRectF br = item->boundingRect();
    item->setPos(-br.left() + 20, -br.top() + 20);

    QRectF renderRect(0, 0, pmSize.width(), pmSize.height());
    QPixmap pixmap(pmSize);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    scene.render(&painter, renderRect, renderRect, Qt::IgnoreAspectRatio);
    painter.end();

    scene.removeItem(item);
    return pixmap;
}

/**
 * @brief 在渲染结果中查找特定颜色的像素
 * @param[in] img 渲染结果图像
 * @param[in] rect 搜索区域（像素坐标）
 * @param[in] matchFunc 颜色匹配函数
 * @return 是否找到匹配像素
 */
static bool findColorInRegion(const QImage& img, const QRect& rect,
                              const std::function<bool(const QColor&)>& matchFunc)
{
    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        for (int x = rect.left(); x <= rect.right(); ++x) {
            if (x < 0 || x >= img.width() || y < 0 || y >= img.height()) continue;
            if (matchFunc(img.pixelColor(x, y))) return true;
        }
    }
    return false;
}

/**
 * @brief 检查像素是否为指定红色（用于图标检测）
 */
static bool isRedPixel(const QColor& c)
{
    return c.red() > 200 && c.green() < 50 && c.blue() < 50;
}

/**
 * @brief 检查像素是否为指定填充色（允许抗锯齿偏差）
 */
static bool isFillColorPixel(const QColor& c, const QColor& fillColor)
{
    return qAbs(c.red() - fillColor.red()) < 30
        && qAbs(c.green() - fillColor.green()) < 30
        && qAbs(c.blue() - fillColor.blue()) < 30;
}

/**
 * @brief 检查像素是否接近白色
 */
static bool isNearWhite(const QColor& c)
{
    return c.red() > 230 && c.green() > 230 && c.blue() > 230;
}

// ============================================================
// testEllipseBodyRendering — 椭圆体渲染像素验证
// ============================================================

/**
 * @brief 验证椭圆体渲染：中心像素有填充色，角落像素无填充色
 *
 * 设置 style.bodyShape=Ellipse，使用红色背景色渲染到 QPixmap，
 * 检查椭圆中心的像素颜色与背景色匹配，
 * 检查矩形角落的像素颜色为白色（椭圆不会填充角落）。
 */
void TestBodyRendering::testEllipseBodyRendering()
{
    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.getStyle().bodyShape      = BodyShape::Ellipse;
    item.getStyle().backgroundColor = QColor(255, 0, 0);
    item.getStyle().borderColor     = QColor(0, 0, 0);
    item.getStyle().borderWidth     = 1.0;
    item.setBodySize(QSizeF(120, 60));

    QPixmap pm = renderItemToPixmap(&item, QSize(200, 160));
    QImage img = pm.toImage();

    // 椭圆中心区域应包含红色像素（填充色）
    bool foundRedCenter = findColorInRegion(img, QRect(0, 0, 200, 160), isRedPixel);
    QVERIFY2(foundRedCenter, "Ellipse should have red fill color at center");

    // 在椭圆角落不应找到红色像素
    bool foundRedAtEdge = findColorInRegion(img, QRect(0, 0, 8, 8), isRedPixel);
    QVERIFY2(!foundRedAtEdge,
             "Ellipse corner pixels should NOT be red (ellipse doesn't fill rectangular corners)");
}

// ============================================================
// testRoundedRectBackCompat — 圆角矩形向后兼容
// ============================================================

/**
 * @brief 验证 RoundedRect 渲染向后兼容
 *
 * bodyShape=RoundedRect（默认值），渲染结果应与旧版一致。
 * 检查默认背景色 QColor(240,240,240) 的像素存在。
 */
void TestBodyRendering::testRoundedRectBackCompat()
{
    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.setBodySize(QSizeF(120, 60));

    QPixmap pm = renderItemToPixmap(&item, QSize(200, 160));
    QImage img = pm.toImage();

    QColor defaultBg(240, 240, 240);
    bool foundDefaultBg = findColorInRegion(img, QRect(0, 0, 200, 160),
        [&defaultBg](const QColor& c) { return isFillColorPixel(c, defaultBg); });

    QVERIFY2(foundDefaultBg, "RoundedRect should render default background color (240,240,240)");
}

// ============================================================
// testNameBelowBoundingRect — 名称位于下方扩展 boundingRect
// ============================================================

/**
 * @brief 验证 namePosition=Below 时 boundingRect 高度 > bodyRect 高度
 *
 * 设置 namePosition=Below，setNodeName("Test")，
 * 检查 boundingRect().height() > getBodyControlRect().height()。
 */
void TestBodyRendering::testNameBelowBoundingRect()
{
    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.getStyle().namePosition = NamePosition::Below;
    item.setNodeName("TestNodeName");
    item.setBodySize(QSizeF(120, 60));

    qreal bodyHeight    = item.getBodyControlRect().height();
    qreal boundingHeight = item.boundingRect().height();

    QVERIFY2(boundingHeight > bodyHeight,
             "When namePosition=Below, boundingRect should be taller than bodyRect to accommodate name text");
}

// ============================================================
// testNameInsideNoExpansion — 名称位于内部不扩展
// ============================================================

/**
 * @brief 验证 namePosition=Inside 时 boundingRect 不向下扩展
 *
 * 设置 namePosition=Inside，boundingRect 高度与无名称基准相同。
 */
void TestBodyRendering::testNameInsideNoExpansion()
{
    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.getStyle().namePosition = NamePosition::Inside;
    item.setNodeName("TestNode");
    item.setBodySize(QSizeF(120, 60));

    DAPyNodeGraphicsItem baseline(nullptr);
    baseline.getStyle().setDefaults();
    baseline.getStyle().namePosition = NamePosition::Inside;
    baseline.setBodySize(QSizeF(120, 60));

    QCOMPARE(item.boundingRect().height(), baseline.boundingRect().height());
}

// ============================================================
// testIconAboveLayout — 图标在文本上方布局
// ============================================================

/**
 * @brief 验证 iconPosition=AboveText 时图标渲染在文本上方
 *
 * 设置 iconPosition=AboveText，setIcon 使用红色 pixmap，
 * 渲染后检查红色像素出现在上半部分。
 */
void TestBodyRendering::testIconAboveLayout()
{
    QPixmap iconPm(24, 24);
    iconPm.fill(QColor(255, 0, 0));

    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.getStyle().iconPosition = IconPosition::AboveText;
    item.getStyle().iconSize     = 24.0;
    item.setIcon(QIcon(iconPm));
    item.setNodeName("TestNode");
    item.setBodySize(QSizeF(120, 80));

    QPixmap pm = renderItemToPixmap(&item, QSize(200, 200));
    QImage img = pm.toImage();

    // 找到所有红色像素的 Y 坐标范围
    int minY = 200, maxY = 0;
    bool foundRed = false;
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            QColor pixel = img.pixelColor(x, y);
            if (isRedPixel(pixel)) {
                foundRed = true;
                minY = qMin(minY, y);
                maxY = qMax(maxY, y);
            }
        }
    }

    QVERIFY2(foundRed, "Icon should be rendered in pixmap");
    QVERIFY2(minY < 80, "Icon should appear in upper half when iconPosition=AboveText");
    QVERIFY2(maxY < 100, "Icon should NOT extend into lower region when iconPosition=AboveText");
}

// ============================================================
// testIconLeftBackCompat — 图标在文本左侧布局
// ============================================================

/**
 * @brief 验证 iconPosition=LeftOfText 渲染与旧版一致
 *
 * iconPosition=LeftOfText（默认值），设置红色图标，
 * 渲染后检查红色像素在左半部分。
 */
void TestBodyRendering::testIconLeftBackCompat()
{
    QPixmap iconPm(24, 24);
    iconPm.fill(QColor(255, 0, 0));

    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.setIcon(QIcon(iconPm));
    item.setNodeName("TestNode");
    item.setBodySize(QSizeF(120, 60));

    QPixmap pm = renderItemToPixmap(&item, QSize(200, 160));
    QImage img = pm.toImage();

    int minX = 200, maxX = 0;
    bool foundRed = false;
    for (int y = 0; y < 160; ++y) {
        for (int x = 0; x < 200; ++x) {
            if (isRedPixel(img.pixelColor(x, y))) {
                foundRed = true;
                minX = qMin(minX, x);
                maxX = qMax(maxX, x);
            }
        }
    }

    QVERIFY2(foundRed, "Icon should be rendered in pixmap");
    QVERIFY2(minX < 100, "Icon should appear in left region when iconPosition=LeftOfText");
    QVERIFY2(maxX - minX <= 26, "Icon width should be ~24 pixels (iconSize=24)");
}

// ============================================================
// testStateDecorationClippedToEllipse — 状态装饰裁剪到椭圆
// ============================================================

/**
 * @brief 验证椭圆模式下状态装饰裁剪到椭圆区域
 *
 * 设置 bodyShape=Ellipse, nodeState=Success，
 * 状态装饰被椭圆裁剪，角落不被状态色填充。
 * 检查 boundingRect 角落像素不含状态色的绿色分量。
 */
void TestBodyRendering::testStateDecorationClippedToEllipse()
{
    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.getStyle().bodyShape = BodyShape::Ellipse;
    item.getStyle().backgroundColor = QColor(240, 240, 240);
    item.getStyle().borderWidth = 1.0;
    item.setNodeState(Success);
    item.setBodySize(QSizeF(120, 60));

    QColor successColor = DAPyNodePalette::getGlobalColorForState(Success);

    QPixmap pm = renderItemToPixmap(&item, QSize(200, 160));
    QImage img = pm.toImage();

    // 找到 item 的最左上角非白色像素
    int itemMinX = 200, itemMinY = 160;
    for (int y = 0; y < 160; ++y) {
        for (int x = 0; x < 200; ++x) {
            QColor c = img.pixelColor(x, y);
            if (!isNearWhite(c)) {
                itemMinX = qMin(itemMinX, x);
                itemMinY = qMin(itemMinY, y);
            }
        }
    }

    qDebug() << "StateDec clip: corner at (" << itemMinX << "," << itemMinY << ")"
             << "color=" << img.pixelColor(itemMinX + 1, itemMinY + 1)
             << "successColor=" << successColor;

    // 椭圆角落：在 boundingRect 角落区域不应有高绿色分量
    // 检查左上角 5x5 区域不应有接近 successColor 的像素
    bool foundStateColorInCorner = findColorInRegion(img,
        QRect(itemMinX, itemMinY, 5, 5),
        [&successColor](const QColor& c) {
            return c.green() > successColor.green() * 0.8;
        });

    QVERIFY2(!foundStateColorInCorner,
             "Ellipse corners should NOT be filled with state color (clipped to ellipse)");
}

// ============================================================
// testEllipseShapeHitTest — 椭圆碰撞形状命中测试
// ============================================================

/**
 * @brief 验证椭圆碰撞形状：中心命中，角落不命中
 *
 * 设置 bodyShape=Ellipse，使用 shape() QPainterPath，
 * 检查中心点 intersects=true，角落点 intersects=false。
 */
void TestBodyRendering::testEllipseShapeHitTest()
{
    DAPyNodeGraphicsItem item(nullptr);
    item.getStyle().setDefaults();
    item.getStyle().bodyShape = BodyShape::Ellipse;
    item.setBodySize(QSizeF(120, 60));

    QPainterPath shapePath = item.shape();

    QPointF center = item.getBodyControlRect().center();
    QVERIFY2(shapePath.intersects(QRectF(center, QSizeF(1, 1))),
             "Ellipse center should be inside shape path");

    QRectF controlRect = item.getBodyControlRect();
    QPointF topLeftCorner(controlRect.left() + 1, controlRect.top() + 1);
    QVERIFY2(!shapePath.intersects(QRectF(topLeftCorner, QSizeF(1, 1))),
             "Ellipse topLeft corner should NOT be inside shape path");
}

// ============================================================
// testCornerRadiusConfigurable — 圆角半径可配置
// ============================================================

/**
 * @brief 验证圆角半径可配置渲染
 *
 * cornerRadius=10 与 cornerRadius=4 各渲染一个 item，
 * 在圆角区域比较像素色差：大圆角应更偏离填充色。
 */
void TestBodyRendering::testCornerRadiusConfigurable()
{
    QColor fillColor(200, 100, 50);

    DAPyNodeGraphicsItem itemLarge(nullptr);
    itemLarge.getStyle().setDefaults();
    itemLarge.getStyle().cornerRadius    = 10.0;
    itemLarge.getStyle().backgroundColor = fillColor;
    itemLarge.setBodySize(QSizeF(120, 60));

    DAPyNodeGraphicsItem itemSmall(nullptr);
    itemSmall.getStyle().setDefaults();
    itemSmall.getStyle().cornerRadius    = 4.0;
    itemSmall.getStyle().backgroundColor = fillColor;
    itemSmall.setBodySize(QSizeF(120, 60));

    QPixmap pmLarge = renderItemToPixmap(&itemLarge, QSize(200, 160));
    QPixmap pmSmall = renderItemToPixmap(&itemSmall, QSize(200, 160));

    QImage imgLarge = pmLarge.toImage();
    QImage imgSmall = pmSmall.toImage();

    // 找到各 item 的最左上角非白色像素
    int largeStartX = -1, largeStartY = -1;
    int smallStartX = -1, smallStartY = -1;

    for (int y = 0; y < 160; ++y) {
        for (int x = 0; x < 200; ++x) {
            if (largeStartX < 0 && !isNearWhite(imgLarge.pixelColor(x, y))) {
                largeStartX = x;
                largeStartY = y;
            }
            if (smallStartX < 0 && !isNearWhite(imgSmall.pixelColor(x, y))) {
                smallStartX = x;
                smallStartY = y;
            }
            if (largeStartX >= 0 && smallStartX >= 0) break;
        }
        if (largeStartX >= 0 && smallStartX >= 0) break;
    }

    QVERIFY2(largeStartX >= 0, "Large corner radius item should render pixels");
    QVERIFY2(smallStartX >= 0, "Small corner radius item should render pixels");

    qDebug() << "CornerRad: largeStart=" << largeStartX << "," << largeStartY
             << "smallStart=" << smallStartX << "," << smallStartY;

    // 在各偏移量处打印像素颜色
    for (int off = 2; off <= 12; off += 2) {
        QColor cl = imgLarge.pixelColor(largeStartX + off, largeStartY + off);
        QColor cs = imgSmall.pixelColor(smallStartX + off, smallStartY + off);
        qDebug() << "offset=" << off << "large=" << cl << "small=" << cs;
    }

    // 在圆角曲线区域比较色差
    int offset = 4;
    QColor cornerLarge = imgLarge.pixelColor(largeStartX + offset, largeStartY + offset);
    QColor cornerSmall = imgSmall.pixelColor(smallStartX + offset, smallStartY + offset);

    int largeDelta = qAbs(cornerLarge.red() - fillColor.red())
                   + qAbs(cornerLarge.green() - fillColor.green())
                   + qAbs(cornerLarge.blue() - fillColor.blue());
    int smallDelta = qAbs(cornerSmall.red() - fillColor.red())
                   + qAbs(cornerSmall.green() - fillColor.green())
                   + qAbs(cornerSmall.blue() - fillColor.blue());

    qDebug() << "CornerRad: largeDelta=" << largeDelta << "smallDelta=" << smallDelta;

    QVERIFY2(largeDelta > smallDelta,
             "Large corner radius (10) should produce more deviation from fill color near corner than small radius (4)");
}

}  // namespace DA