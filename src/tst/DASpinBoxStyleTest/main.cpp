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
 * 窗口挂任意 QSS 后（SARibbon 主题必然挂），QStyleSheetStyle 会把 SpinBox 尺寸
 * 压成「按钮 + 4px 文本区」。本样式只补最小宽度，不改变平台原生的按钮布局
 * （Windows 11 的左右按钮点击区域比竖排更大）。
 */
class DASpinBoxStyleTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void valueAreaKeepsRoomUnderQss();
    void doubleValueAreaKeepsRoomUnderQss();
    void otherControlsDelegateToBase();
    void minimumWidthGrowsWithDecimals();
    void windows11ButtonsStayHorizontal();

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
    // 走控件真实样式（窗口有 QSS 时是 QStyleSheetStyle）
    return widget->style()->subControlRect(QStyle::CC_SpinBox, opt, sc, widget);
}

/**
 * @brief QSpinBox：QSS 触发的尺寸塌缩应被最小宽度抵消，数值区放得下范围两端
 */
void DASpinBoxStyleTest::valueAreaKeepsRoomUnderQss()
{
    QMainWindow window;
    // 任意 QSS 都会让子控件走 QStyleSheetStyle，这里模拟 SARibbon 主题
    window.setStyleSheet(QStringLiteral("QWidget { outline: none; }"));
    auto* spin = new QSpinBox(&window);
    spin->setRange(-9999, 9999);
    spin->setValue(1234);
    window.setCentralWidget(spin);
    window.resize(320, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QFontMetrics fm(spin->font());
    const QStyleOptionSpinBox opt = makeSpinOption(spin);
    const QRect edit                = subControlRect(spin, &opt, QStyle::SC_SpinBoxEditField);
    QVERIFY2(edit.width() >= fm.horizontalAdvance(QStringLiteral("-9999")),
             "value area must fit the widest value of the range");
}

/**
 * @brief QDoubleSpinBox：小数位、符号与后缀都要计入数值区宽度
 */
void DASpinBoxStyleTest::doubleValueAreaKeepsRoomUnderQss()
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

    const QFontMetrics fm(spin->font());
    const QStyleOptionSpinBox opt = makeSpinOption(spin);
    const QRect edit                = subControlRect(spin, &opt, QStyle::SC_SpinBoxEditField);
    QVERIFY2(edit.width() >= fm.horizontalAdvance(QStringLiteral("-99.00 px")),
             "value area must fit sign, decimals and suffix");
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
 * @brief Windows 11 真实场景：保留左右并排按钮，同时数值区放得下范围最大值
 */
void DASpinBoxStyleTest::windows11ButtonsStayHorizontal()
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
    QCOMPARE(up.top(), down.top());
    QVERIFY2(up.left() != down.left(), "windows11 native side-by-side buttons must be preserved");

    const QFontMetrics fm(spin->font());
    QVERIFY2(edit.width() >= fm.horizontalAdvance(QStringLiteral("9999")),
             "value area must fit the maximum value");
}

QTEST_MAIN(DASpinBoxStyleTest)
#include "main.moc"
