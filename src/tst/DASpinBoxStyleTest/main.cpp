#include <QtTest/QtTest>

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMainWindow>
#include <QSpinBox>
#include <QStyleFactory>
#include <QStyleOptionComboBox>
#include <QStyleOptionSpinBox>

#include "DASpinBoxStyle.h"

/**
 * @brief SpinBox 代理样式的回归测试
 *
 * 覆盖两个问题：
 * 1. windows11 平台样式把上下按钮画成左右并排；
 * 2. 窗口挂任意 QSS 后，QStyleSheetStyle 把 SpinBox 尺寸压成「按钮 + 4px 文本区」。
 */
class DASpinBoxStyleTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void spinButtonsAreStacked();
    void doubleSpinButtonsAreStacked();
    void valueAreaKeepsRoom();
    void otherControlsDelegateToBase();
    void minimumWidthGrowsWithDecimals();
    void windows11HorizontalButtonsAreRelayouted();

private:
    QStyleOptionSpinBox makeSpinOption(const QAbstractSpinBox* spin) const;
    static QRect subControlRect(const QWidget* widget,
                                const QStyleOptionComplex* opt,
                                QStyle::SubControl sc);

private:
    QStyle* mBaseStyle { nullptr };
};

/**
 * @brief 安装代理样式（不依赖当前平台样式，Fusion 在所有平台可用）
 */
void DASpinBoxStyleTest::initTestCase()
{
    mBaseStyle = QStyleFactory::create(QStringLiteral("Fusion"));
    QVERIFY(mBaseStyle);
    QApplication::setStyle(new DA::DASpinBoxStyle(mBaseStyle));
}

QStyleOptionSpinBox DASpinBoxStyleTest::makeSpinOption(const QAbstractSpinBox* spin) const
{
    QStyleOptionSpinBox opt;
    opt.initFrom(spin);
    opt.buttonSymbols = spin->buttonSymbols();
    opt.subControls = QStyle::SC_All;
    return opt;
}

QRect DASpinBoxStyleTest::subControlRect(const QWidget* widget,
                                         const QStyleOptionComplex* opt,
                                         QStyle::SubControl sc)
{
    // 走控件真实样式（窗口有 QSS 时是 QStyleSheetStyle，最终委托到代理样式）
    return widget->style()->subControlRect(QStyle::CC_SpinBox, opt, sc, widget);
}

/**
 * @brief 窗口挂 QSS 时，QSpinBox 的上下按钮应上下排列且数值区可见
 */
void DASpinBoxStyleTest::spinButtonsAreStacked()
{
    QMainWindow window;
    // 任意 QSS 都会让子控件走 QStyleSheetStyle，这里模拟 SARibbon 主题
    window.setStyleSheet(QStringLiteral("QPushButton { color: red; }"));
    auto* spin = new QSpinBox(&window);
    spin->setRange(0, 9999);
    spin->setValue(42);
    window.setCentralWidget(spin);
    window.resize(320, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QStyleOptionSpinBox opt = makeSpinOption(spin);
    const QRect up                  = subControlRect(spin, &opt, QStyle::SC_SpinBoxUp);
    const QRect down                = subControlRect(spin, &opt, QStyle::SC_SpinBoxDown);
    const QRect edit                = subControlRect(spin, &opt, QStyle::SC_SpinBoxEditField);
    QVERIFY(!up.isEmpty());
    QVERIFY(!down.isEmpty());
    QCOMPARE(up.left(), down.left());
    QVERIFY2(up.bottom() < down.top(), "up/down buttons must be stacked vertically");
    QVERIFY2(edit.width() >= 40, "value area must keep room for the number");
}

/**
 * @brief QDoubleSpinBox 同样应竖排按钮并保留数值区
 */
void DASpinBoxStyleTest::doubleSpinButtonsAreStacked()
{
    QMainWindow window;
    window.setStyleSheet(QStringLiteral("QLabel { color: red; }"));
    auto* spin = new QDoubleSpinBox(&window);
    spin->setRange(-99.0, 99.0);
    spin->setDecimals(2);
    spin->setSuffix(QStringLiteral(" px"));
    spin->setValue(1.5);
    window.setCentralWidget(spin);
    window.resize(320, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QStyleOptionSpinBox opt = makeSpinOption(spin);
    const QRect up                  = subControlRect(spin, &opt, QStyle::SC_SpinBoxUp);
    const QRect down                = subControlRect(spin, &opt, QStyle::SC_SpinBoxDown);
    const QRect edit                = subControlRect(spin, &opt, QStyle::SC_SpinBoxEditField);
    QCOMPARE(up.left(), down.left());
    QVERIFY(up.bottom() < down.top());
    QVERIFY(edit.width() >= 40);
}

/**
 * @brief QSS 触发的尺寸塌缩应被最小宽度抵消，数值不贴边
 */
void DASpinBoxStyleTest::valueAreaKeepsRoom()
{
    QMainWindow window;
    window.setStyleSheet(QStringLiteral("QWidget { outline: none; }"));
    auto* spin = new QSpinBox(&window);
    spin->setRange(-99999, 99999);
    spin->setValue(12345);
    window.setCentralWidget(spin);
    window.resize(320, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QFontMetrics fm(spin->font());
    QVERIFY2(spin->minimumWidth() >= fm.horizontalAdvance(QStringLiteral("-99999")),
             "minimum width must fit the widest value");
    const QStyleOptionSpinBox opt = makeSpinOption(spin);
    const QRect edit                = subControlRect(spin, &opt, QStyle::SC_SpinBoxEditField);
    QVERIFY(edit.width() >= fm.horizontalAdvance(QStringLiteral("12345")));
}

/**
 * @brief 非 SpinBox 控件仍完全交给基础样式，不应被代理改变
 */
void DASpinBoxStyleTest::otherControlsDelegateToBase()
{
    QComboBox combo;
    combo.addItem(QStringLiteral("Knuckle"));
    combo.resize(120, 24);
    QStyleOptionComboBox opt;
    opt.initFrom(&combo);
    opt.subControls = QStyle::SC_All;

    const QRect baseRect =
        mBaseStyle->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxFrame, &combo);
    const QRect appRect =
        QApplication::style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxFrame, &combo);
    QCOMPARE(appRect, baseRect);
}

/**
 * @brief 小数位越多，最小宽度越大（数值样本包含小数部分）
 */
void DASpinBoxStyleTest::minimumWidthGrowsWithDecimals()
{
    QDoubleSpinBox noDecimals;
    noDecimals.setDecimals(0);
    noDecimals.ensurePolished();

    QDoubleSpinBox fourDecimals;
    fourDecimals.setDecimals(4);
    fourDecimals.ensurePolished();

    QVERIFY(fourDecimals.minimumWidth() > noDecimals.minimumWidth());
}

/**
 * @brief windows11 样式的左右并排按钮应被重排为竖排（真实问题场景）
 */
void DASpinBoxStyleTest::windows11HorizontalButtonsAreRelayouted()
{
    if (!QStyleFactory::keys().contains(QStringLiteral("windows11"), Qt::CaseInsensitive)) {
        QSKIP("windows11 style is not available on this platform");
    }
    QStyle* windows11 = QStyleFactory::create(QStringLiteral("windows11"));
    QVERIFY(windows11);
    QApplication::setStyle(new DA::DASpinBoxStyle(windows11));

    QMainWindow window;
    window.setStyleSheet(QStringLiteral("QLabel { color: red; }"));
    auto* spin = new QSpinBox(&window);
    spin->setRange(0, 9999);
    spin->setValue(7);
    window.setCentralWidget(spin);
    window.resize(320, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QStyleOptionSpinBox opt = makeSpinOption(spin);
    const QRect up                  = subControlRect(spin, &opt, QStyle::SC_SpinBoxUp);
    const QRect down                = subControlRect(spin, &opt, QStyle::SC_SpinBoxDown);
    const QRect edit                = subControlRect(spin, &opt, QStyle::SC_SpinBoxEditField);
    QCOMPARE(up.left(), down.left());
    QVERIFY2(up.bottom() < down.top(), "windows11 side-by-side buttons must become vertical");
    QVERIFY(edit.width() >= 40);
}

QTEST_MAIN(DASpinBoxStyleTest)
#include "main.moc"
