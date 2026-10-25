#include <QTextEdit>
#include <QToolButton>
#include <QtTest/QtTest>

#include "DAColorPickerButton.h"
#include "DAFontEditPannelWidget.h"
#include "DARichTextEditWidget.h"

using namespace DA;

namespace
{
bool hasToolButtonWithToolTip(QWidget* root, const QString& tip)
{
    const QList< QToolButton* > buttons = root->findChildren< QToolButton* >();
    for (QToolButton* b : buttons) {
        if (b->toolTip() == tip) {
            return true;
        }
    }
    return false;
}

bool hasColorButtonWithToolTip(QWidget* root, const QString& tip)
{
    const QList< DAColorPickerButton* > buttons = root->findChildren< DAColorPickerButton* >();
    for (DAColorPickerButton* b : buttons) {
        if (b->toolTip() == tip) {
            return true;
        }
    }
    return false;
}
}  // namespace

/**
 * @brief DARichTextEditWidget 的模块测试（T2.2）
 *
 * 只覆盖组件本身：HTML 往返、格式控件存在性、setHtml 不触发 textChanged、
 * 以及格式按钮确实作用到编辑区。popup 的定位/关闭语义按方案走图表标注手工冒烟。
 */
class DARichTextEditTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testHtmlRoundTrip();
    void testFormatWidgetsExist();
    void testSetHtmlDoesNotEmit();
    void testFormattingApplies();
};

void DARichTextEditTest::testHtmlRoundTrip()
{
    DARichTextEditWidget w;
    w.setHtml(QStringLiteral("<p><b>Bold</b> and <i>italic</i></p>"));
    QCOMPARE(w.toPlainText().trimmed(), QStringLiteral("Bold and italic"));

    const QString first = w.toHtml();
    QVERIFY(!first.isEmpty());
    QVERIFY(first.contains(QStringLiteral("Bold")));
    QVERIFY(first.contains(QStringLiteral("italic")));

    // 把导出的 HTML 再灌回去，输出应稳定（允许 Qt 首轮补全样式，第二次起必须一致）
    w.setHtml(first);
    const QString second = w.toHtml();
    w.setHtml(second);
    const QString third = w.toHtml();
    QCOMPARE(third, second);

    w.setHtml(QString());
    QVERIFY(w.toPlainText().isEmpty());
}

void DARichTextEditTest::testFormatWidgetsExist()
{
    DARichTextEditWidget w;
    QVERIFY(w.textEdit() != nullptr);
    QVERIFY(w.findChild< DAFontEditPannelWidget* >() != nullptr);
    // 文字背景色按钮（字体面板内部另有一个颜色按钮，按 tooltip 区分）
    QVERIFY(hasColorButtonWithToolTip(&w, QStringLiteral("Text Background Color")));
    QVERIFY(hasToolButtonWithToolTip(&w, QStringLiteral("Strikethrough")));
    QVERIFY(hasToolButtonWithToolTip(&w, QStringLiteral("Superscript")));
    QVERIFY(hasToolButtonWithToolTip(&w, QStringLiteral("Subscript")));
    QVERIFY(hasToolButtonWithToolTip(&w, QStringLiteral("Clear character format of selected text")));
}

void DARichTextEditTest::testSetHtmlDoesNotEmit()
{
    DARichTextEditWidget w;
    QSignalSpy spy(&w, &DARichTextEditWidget::textChanged);
    w.setHtml(QStringLiteral("<b>loaded</b>"));
    QCOMPARE(spy.count(), 0);
    // 用户编辑才发信号
    w.textEdit()->insertPlainText(QStringLiteral(" more"));
    QCOMPARE(spy.count(), 1);
}

void DARichTextEditTest::testFormattingApplies()
{
    DARichTextEditWidget w;
    w.setHtml(QStringLiteral("format me"));
    auto* fontPanel = w.findChild< DAFontEditPannelWidget* >();
    QVERIFY(fontPanel);
    // 字体面板颜色 -> 选中文本
    fontPanel->setCurrentFontColor(Qt::red);
    QCOMPARE(w.textEdit()->textCursor().charFormat().foreground().color(), QColor(Qt::red));

    // 删除线按钮 -> 选中文本
    const QList< QToolButton* > buttons = w.findChildren< QToolButton* >();
    for (QToolButton* b : buttons) {
        if (b->toolTip() == QStringLiteral("Strikethrough")) {
            b->click();
            break;
        }
    }
    QVERIFY(w.textEdit()->textCursor().charFormat().fontStrikeOut());
}

QTEST_MAIN(DARichTextEditTest)
#include "main.moc"
