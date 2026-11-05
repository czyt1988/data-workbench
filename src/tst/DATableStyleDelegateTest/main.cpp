#include <QtTest/QtTest>
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QStandardItemModel>

#include <cstdio>

#include "Table/Metadata/DATableCellStyle.h"
#include "Table/Metadata/DATableStyleManager.h"
#include "Table/Views/DATableStyleItemDelegate.h"

namespace
{
// 离屏绘制画布宽度（足够宽，右上角留出没有文字的空像素供取样）
const int c_canvasWidth  = 200;
const int c_canvasHeight = 28;
// 取样点：右上角空像素（文字在左侧，批注角标未启用）
const QPoint c_samplePoint(c_canvasWidth - 6, 5);

/**
 * @brief 在白色画布上绘制一个单元格
 * @param delegate 样式委托
 * @param model 数据模型（取 index(0,0)）
 * @param background 画布初始填充色
 * @return 绘制结果
 */
QImage paintCell(DA::DATableStyleItemDelegate& delegate, QStandardItemModel& model, const QColor& background = Qt::white)
{
    QImage img(c_canvasWidth, c_canvasHeight, QImage::Format_ARGB32);
    img.fill(background);
    QPainter painter(&img);
    QStyleOptionViewItem opt;
    opt.rect               = QRect(0, 0, c_canvasWidth, c_canvasHeight);
    opt.state              = QStyle::State_Enabled;
    opt.font               = QApplication::font();
    opt.direction          = Qt::LeftToRight;
    opt.displayAlignment   = Qt::AlignLeft | Qt::AlignVCenter;
    opt.decorationPosition = QStyleOptionViewItem::Left;
    delegate.paint(&painter, opt, model.index(0, 0));
    painter.end();
    return img;
}
}  // namespace

//===================================================
// DATableStyleDelegateTest
//===================================================
class DATableStyleDelegateTest : public QObject
{
    Q_OBJECT
private slots:
    // 底色「无」：不填充 → 保留默认底色（回归：曾按黑色实心填充）
    void testNoneFillKeepsDefaultBackground();
    // 有效底色：正常填充
    void testSolidFillPaintsColor();
    // 单元格「无」覆盖列级底色 → 该格不填充
    void testNoneFillOverridesColumnFill();
};

/**
 * 底色「无」不参与填充，露出控件默认底色
 *
 * 回归背景：颜色菜单的「无」传入无效 QColor()，QBrush(QColor()) 在 Qt 里 style 仍是
 * SolidPattern，历史行为是黑色实心填充，浅色主题下整片变黑、文字看不见。
 */
void DATableStyleDelegateTest::testNoneFillKeepsDefaultBackground()
{
    DA::DATableStyleManager mgr;
    DA::DATableCellStyle noneStyle;
    noneStyle.setBackground(QBrush(QColor()));  // 「无」
    mgr.setCellStyle(0, 0, noneStyle);
    QVERIFY(!mgr.resolveCellStyle(0, 0).hasFill());

    DA::DATableStyleItemDelegate delegate(&mgr, nullptr);
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), 1234);

    const QImage img = paintCell(delegate, model, Qt::white);
    QCOMPARE(img.pixelColor(c_samplePoint), QColor(Qt::white));
}

/**
 * 有效底色照常填充（确认 hasFill 门禁没有误伤正常路径）
 */
void DATableStyleDelegateTest::testSolidFillPaintsColor()
{
    DA::DATableStyleManager mgr;
    DA::DATableCellStyle solidStyle;
    solidStyle.setBackground(QBrush(QColor(255, 255, 0)));
    mgr.setCellStyle(0, 0, solidStyle);

    DA::DATableStyleItemDelegate delegate(&mgr, nullptr);
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), 1234);

    const QImage img = paintCell(delegate, model, Qt::white);
    QCOMPARE(img.pixelColor(c_samplePoint), QColor(255, 255, 0));
}

/**
 * 单元格级「无」覆盖列级底色：显式不填充（层序 列→行→单元格）
 */
void DATableStyleDelegateTest::testNoneFillOverridesColumnFill()
{
    DA::DATableStyleManager mgr;
    DA::DATableCellStyle colStyle;
    colStyle.setBackground(QBrush(QColor(255, 255, 0)));
    mgr.setColumnStyle(0, colStyle);
    QVERIFY(mgr.resolveCellStyle(0, 0).hasFill());

    DA::DATableCellStyle noneStyle;
    noneStyle.setBackground(QBrush(QColor()));  // 「无」
    mgr.setCellStyle(0, 0, noneStyle);
    QVERIFY(!mgr.resolveCellStyle(0, 0).hasFill());

    DA::DATableStyleItemDelegate delegate(&mgr, nullptr);
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), 1234);

    const QImage img = paintCell(delegate, model, Qt::white);
    QCOMPARE(img.pixelColor(c_samplePoint), QColor(Qt::white));
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DATableStyleDelegateTest tc;
    const int rc = QTest::qExec(&tc, argc, argv);
    std::fflush(stdout);
    std::fflush(stderr);
    return rc;
}

#include "main.moc"
