#include <QtTest>
#include <cstdio>
#ifdef Q_OS_WIN
#    include <windows.h>
#else
#    include <unistd.h>
#endif
// Qt
#include <QAbstractItemView>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
// DAGui
#include "Common/CodeEditor/DAPythonCodeEdit.h"
// DAUtils
#include "DAPythonHighlighter.h"
#include "DAPythonSyntax.h"
// DAPyBindQt
#include "DAPyInterpreter.h"
#include "DAPyPythonCompletion.h"

/**
 * @brief 通用 Python 编辑能力测试
 *
 * 覆盖：词表/标识符判定、语法高亮（关键字/def 名/注释/字符串/数字/反引号/三引号跨行）、
 * 补全数据源（pandas/numpy 成员）、编辑器弹窗补全（成员补全、反引号列名补全）。
 */
class DAPythonCodeEditTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSyntaxHelpers();
    void testHighlighter();
    void testCompletionProvider();
    void testEditorCompletion();
};

void DAPythonCodeEditTest::initTestCase()
{
    // 与 APP main.cpp 一致：解析解释器路径并设置 Python Home，否则 pandas 无法导入
    QString pythonHomePath;
    const QList< QFileInfo > candidates = DA::DAPyInterpreter::wherePython();
    for (const QFileInfo& fi : candidates) {
        QDir homeDir = fi.absoluteDir();
        if (homeDir.exists(QStringLiteral("Lib/site-packages"))) {
            pythonHomePath = homeDir.absolutePath();
            break;
        }
    }
    if (pythonHomePath.isEmpty()) {
        const QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
        if (!pypath.isEmpty()) {
            pythonHomePath = QFileInfo(pypath).absolutePath();
        }
    }
    DA::DAPyInterpreter::initializePythonInterpreter(pythonHomePath);
}

void DAPythonCodeEditTest::testSyntaxHelpers()
{
    QVERIFY(DA::DAPythonSyntax::isIdentifier(QStringLiteral("A")));
    QVERIFY(DA::DAPythonSyntax::isIdentifier(QStringLiteral("col_1")));
    QVERIFY(!DA::DAPythonSyntax::isIdentifier(QStringLiteral("my col")));
    QVERIFY(!DA::DAPythonSyntax::isIdentifier(QStringLiteral("2024")));
    QVERIFY(DA::DAPythonSyntax::isKeyword(QStringLiteral("in")));
    QVERIFY(!DA::DAPythonSyntax::needsBacktick(QStringLiteral("A")));
    QVERIFY(DA::DAPythonSyntax::needsBacktick(QStringLiteral("my col")));
    QVERIFY(DA::DAPythonSyntax::needsBacktick(QStringLiteral("in")));
    QVERIFY(DA::DAPythonSyntax::needsBacktick(QStringLiteral("df"), { QStringLiteral("df") }));
    QVERIFY(DA::DAPythonSyntax::keywords().contains(QStringLiteral("def")));
    QVERIFY(DA::DAPythonSyntax::builtins().contains(QStringLiteral("len")));
}

void DAPythonCodeEditTest::testHighlighter()
{
    auto formatAt = [](const QTextBlock& block, int pos) -> QTextCharFormat {
        if (!block.isValid() || !block.layout()) {
            return {};
        }
        const QVector< QTextLayout::FormatRange > ranges = block.layout()->formats();
        for (const QTextLayout::FormatRange& r : ranges) {
            if (pos >= r.start && pos < r.start + r.length) {
                return r.format;
            }
        }
        return {};
    };
    QTextDocument doc;
    DA::DAPythonHighlighter highlighter(&doc);
    doc.setPlainText(QStringLiteral("def foo(x):\n    # note\n    s = \"text\"\n    return x + 1\n"));
    highlighter.rehighlight();

    QCOMPARE(formatAt(doc.findBlockByNumber(0), 0).foreground().color(), QColor(QStringLiteral("#5280C1")));  // def
    QCOMPARE(formatAt(doc.findBlockByNumber(0), 5).foreground().color(), QColor(QStringLiteral("#5D6D7E")));  // foo
    QCOMPARE(formatAt(doc.findBlockByNumber(1), 6).foreground().color(), QColor(QStringLiteral("#727272")));  // note
    QCOMPARE(formatAt(doc.findBlockByNumber(2), 10).foreground().color(), QColor(QStringLiteral("#669E8B")));  // text
    QCOMPARE(formatAt(doc.findBlockByNumber(3), 15).foreground().color(), QColor(QStringLiteral("#80559F")));  // 1

    // 反引号列名
    QTextDocument doc2;
    DA::DAPythonHighlighter highlighter2(&doc2);
    doc2.setPlainText(QStringLiteral("`my col` + A"));
    highlighter2.rehighlight();
    QCOMPARE(formatAt(doc2.firstBlock(), 1).foreground().color(), QColor(QStringLiteral("#3CAED5")));

    // 三引号字符串跨行
    doc2.setPlainText(QStringLiteral("s = \"\"\"line1\nline2\"\"\"\n"));
    highlighter2.rehighlight();
    QCOMPARE(formatAt(doc2.findBlockByNumber(1), 2).foreground().color(), QColor(QStringLiteral("#669E8B")));
}

void DAPythonCodeEditTest::testCompletionProvider()
{
    QVERIFY(DA::DAPyPythonCompletion::isPythonReady());
    QVERIFY(DA::DAPyPythonCompletion::keywords().contains(QStringLiteral("def")));
    QVERIFY(DA::DAPyPythonCompletion::builtins().contains(QStringLiteral("len")));
    const QStringList pandasMembers = DA::DAPyPythonCompletion::membersOf(QStringLiteral("pd"));
    QVERIFY(pandasMembers.contains(QStringLiteral("DataFrame")));
    QVERIFY(pandasMembers.contains(QStringLiteral("read_csv")));
    const QStringList numpyMembers = DA::DAPyPythonCompletion::membersOf(QStringLiteral("np"));
    QVERIFY(numpyMembers.contains(QStringLiteral("array")));
    const QStringList seriesStr = DA::DAPyPythonCompletion::membersOf(QStringLiteral("Series.str"));
    QVERIFY(seriesStr.contains(QStringLiteral("contains")));
}

void DAPythonCodeEditTest::testEditorCompletion()
{
    DA::DAPythonCodeEdit edit;
    edit.resize(600, 320);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    QList< DA::DAPythonCompletionEntry > entries;
    DA::DAPythonCompletionEntry col1;
    col1.text       = QStringLiteral("my col");
    col1.insertText = QStringLiteral("`my col`");
    DA::DAPythonCompletionEntry col2;
    col2.text       = QStringLiteral("amount");
    col2.insertText = QStringLiteral("amount");
    entries << col1 << col2;
    edit.setExtraEntries(entries);
    edit.setFocus();

    QAbstractItemView* popup = edit.completionPopup();
    QVERIFY(popup != nullptr);

    // 成员补全：输入 pd.D 自动弹出 DataFrame 等成员
    edit.setPlainText(QStringLiteral("pd."));
    QTextCursor cur = edit.textCursor();
    cur.movePosition(QTextCursor::End);
    edit.setTextCursor(cur);
    QTest::keyClicks(&edit, QStringLiteral("D"));
    QTest::qWait(80);
    QVERIFY(popup->isVisible());
    QVERIFY(popup->model()->rowCount() > 0);
    QTest::keyClick(&edit, Qt::Key_Return);
    QVERIFY(!popup->isVisible());
    QVERIFY(edit.toPlainText().startsWith(QStringLiteral("pd.D")));
    QVERIFY(edit.toPlainText().size() > 4);

    // 反引号列名补全：输入 `my 后 Ctrl+Space，选中 my col 自动补右反引号
    edit.setPlainText(QStringLiteral("`my"));
    cur = edit.textCursor();
    cur.movePosition(QTextCursor::End);
    edit.setTextCursor(cur);
    QTest::keyClick(&edit, Qt::Key_Space, Qt::ControlModifier);
    QTest::qWait(80);
    QVERIFY(popup->isVisible());
    QCOMPARE(popup->model()->rowCount(), 1);  // 只有 my col 匹配前缀 my
    QTest::keyClick(&edit, Qt::Key_Return);
    QCOMPARE(edit.toPlainText(), QStringLiteral("`my col`"));

    // 列名插入（菜单按钮走同一 API）
    edit.setPlainText(QStringLiteral("A + "));
    cur = edit.textCursor();
    cur.movePosition(QTextCursor::End);
    edit.setTextCursor(cur);
    edit.insertCodeAtCursor(QStringLiteral("`my col`"));
    QCOMPARE(edit.toPlainText(), QStringLiteral("A + `my col`"));
}

// 不用 QTEST_MAIN：嵌入式 Python 的静态对象在进程 teardown 阶段析构顺序不可控，
// 会在测试结果完整输出后引发访问冲突。qExec 返回时断言结果与 -o 文件已写完，
// 直接终止进程跳过 teardown，保证退出码可信（同 DATableDataTest 的模板）。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DAPythonCodeEditTest tc;
    int ret = QTest::qExec(&tc, argc, argv);
    std::fflush(nullptr);
#ifdef Q_OS_WIN
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(ret));
#else
    ::_exit(ret);
#endif
    return ret;
}
#include "main.moc"
