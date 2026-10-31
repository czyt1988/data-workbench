#include <QtTest/QtTest>
#include <QApplication>
#include <QBuffer>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QTemporaryDir>

// 被测对象
#include "DATableExporter.h"
// 依赖的值类型与存储
#include "DATableMetadata.h"
#include "DATableStyleManager.h"
#include "DATableConditionalFormatStore.h"
#include "DATableConditionalFormatRule.h"
#include "DATableNoteStore.h"
#include "DATableCellStyle.h"
#include "DATableDisplayFormat.h"
// 数据层
#include "DAData.h"
#include "DAAbstractData.h"
#include "DATableDataSource.h"
#include "DATableDataBlock.h"

#include <memory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

//===================================================
// 内存表格数据源：惰性源形态（getDataType = TypeInnerData，同 DATableDataTest
// 的 DATestLazyTable 先例），可控行列数与取值，fetchBlock 计数用于验证分块取数次数
//===================================================
class TestMemoryTable : public DA::DAAbstractData, public DA::DATableDataSource
{
public:
    TestMemoryTable(std::size_t rows, std::size_t cols) : mRows(rows), mCols(cols)
    {
    }

    DA::DAAbstractData::DataType getDataType() const override
    {
        return DA::DAAbstractData::TypeInnerData;
    }
    QVariant toVariant(std::size_t dim1, std::size_t dim2) const override
    {
        return makeCell(dim1, dim2);
    }
    bool setValue(std::size_t, std::size_t, const QVariant&) override
    {
        return false;  // 只读
    }
    DA::DATableDataSource* tableSource() override
    {
        return this;
    }
    const DA::DATableDataSource* tableSource() const override
    {
        return this;
    }

public:  // DATableDataSource
    std::size_t tableRowCount() const override
    {
        return mRows;
    }
    std::size_t tableColumnCount() const override
    {
        return mCols;
    }
    QString tableColumnName(std::size_t column) const override
    {
        if (column >= mCols) {
            return QString();
        }
        // 第 1 列刻意带竖线与换行，钉住转义行为
        if (column == 1) {
            return QStringLiteral("b|c\nd");
        }
        return QStringLiteral("col%1").arg(column);
    }
    DA::DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override
    {
        ++mFetchCount;
        if (startRow >= mRows) {
            return DA::DATableDataBlock();
        }
        const std::size_t n = std::min(rowCount, mRows - startRow);
        DA::DATableDataBlock block(startRow, mCols);
        for (std::size_t r = 0; r < n; ++r) {
            QVariantList row;
            row.reserve(static_cast< int >(mCols));
            for (std::size_t c = 0; c < mCols; ++c) {
                row.append(makeCell(startRow + r, c));
            }
            block.appendRow(row);
        }
        return block;
    }
    bool isLazyLoaded() const override
    {
        return true;
    }
    bool isTableEditable() const override
    {
        return false;
    }

    // 单元格取值：col0 = 行号（整数），col1 = "a|b\nc"（转义钉子），其余 = 浮点
    QVariant makeCell(std::size_t r, std::size_t c) const
    {
        if (c == 0) {
            return QVariant(static_cast< qlonglong >(r));
        }
        if (c == 1) {
            return QStringLiteral("a|b\nc");
        }
        return QVariant(static_cast< double >(r) / 4.0);
    }

public:
    std::size_t mRows;
    std::size_t mCols;
    mutable int mFetchCount = 0;
};

//===================================================
// DATableExporterTest
//===================================================
class DATableExporterTest : public QObject
{
    Q_OBJECT
private slots:
    void testInvalidData();
    void testTabText();
    void testMarkdown();
    void testHtmlBasic();
    void testHtmlWithStyles();
    void testDisplayFormat();
    void testBlockFetchAndProgress();
    void testCancel();
    void testWriteToFile();
    void testLargeCopyThreshold();
    void testNoteTitle();
    void testColumnSubset();
    void testColumnSubsetWithStyles();
};

/**
 * 非表格型数据（null DAData / series）不可导出
 */
void DATableExporterTest::testInvalidData()
{
    DA::DAData d;
    DA::DATableExporter exp(d);
    QString why;
    QVERIFY(!exp.isValid(&why));
    QVERIFY(!why.isEmpty());
    QVERIFY(exp.rowCount() == 0);
    QVERIFY(exp.cellCount() == 0);
    QString out;
    QVERIFY(!exp.toString(DA::DATableExporter::Format::TabText, &out));
}

/**
 * Tab 文本：表头 + 行内 Tab + 行间换行 + 末行换行（与表格选区复制同口径，
 * 值内换行不转义——同 copySelectionToClipboard 的行为）
 */
void DATableExporterTest::testTabText()
{
    auto t = std::make_shared< TestMemoryTable >(3, 3);
    DA::DAData d(t);
    DA::DATableExporter exp(d);
    QVERIFY(exp.isValid());
    QCOMPARE(exp.rowCount(), qint64(3));
    QCOMPARE(exp.columnCount(), qint64(3));
    QCOMPARE(exp.cellCount(), qint64(9));

    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::TabText, &out));
    // col1 单元格值 "a|b\nc" 原样保留（含内嵌换行）：末行换行 + 每行 1 个内嵌换行
    // → 总换行数 = 表头 1 + 数据行 3×(1+1) = 8；直接比对完整内容
    QCOMPARE(out, QStringLiteral("col0\tb|c\nd\tcol2\n"
                                 "0\ta|b\nc\t0\n"
                                 "1\ta|b\nc\t0.25\n"
                                 "2\ta|b\nc\t0.5\n"));
}

/**
 * Markdown：表头/分隔行/数据行结构；竖线转义为 \|、换行转 <br>
 */
void DATableExporterTest::testMarkdown()
{
    auto t = std::make_shared< TestMemoryTable >(2, 3);
    DA::DAData d(t);
    DA::DATableExporter exp(d);
    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::Markdown, &out));
    const QStringList lines = out.split(QLatin1Char('\n'));
    QCOMPARE(lines.size(), 5);  // 表头 + 分隔行 + 2 数据行 + 末尾空串
    QCOMPARE(lines[ 0 ], QStringLiteral("| col0 | b\\|c<br>d | col2 |"));
    QCOMPARE(lines[ 1 ], QStringLiteral("| --- | --- | --- |"));
    QCOMPARE(lines[ 2 ], QStringLiteral("| 0 | a\\|b<br>c | 0 |"));
    QCOMPARE(lines[ 3 ], QStringLiteral("| 1 | a\\|b<br>c | 0.25 |"));
}

/**
 * HTML 基础形态：文档骨架 + 表头 + 数据行 + 值转义
 */
void DATableExporterTest::testHtmlBasic()
{
    auto t = std::make_shared< TestMemoryTable >(2, 3);
    DA::DAData d(t);
    DA::DATableExporter exp(d);
    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::Html, &out));
    QVERIFY(out.startsWith(QStringLiteral("<!DOCTYPE html>")));
    QVERIFY(out.contains(QStringLiteral("<meta charset=\"utf-8\">")));
    QVERIFY(out.contains(QStringLiteral("<table class=\"da-table\">")));
    QVERIFY(out.contains(QStringLiteral("</tbody>\n</table>\n</body>\n</html>")));
    // 表头转义：竖线原样、换行转 <br> 不是必须的，但 < > & 必须转义；
    // 这里列名只有竖线与换行，检查 td 行数即可（2 行 × 3 td）
    QVERIFY(out.count(QStringLiteral("<tr>")) >= 3);  // thead 1 + tbody 2
    QVERIFY(out.contains(QStringLiteral(">0</td>")));
    QVERIFY(out.contains(QStringLiteral(">a|b\nc</td>")));  // 竖线在 HTML 无需转义，原样
    QVERIFY(out.contains(QStringLiteral(">0.25</td>")));
}

/**
 * HTML 样式渲染：静态样式（列/行/单元格三层）与条件格式（单元格规则命中）→ CSS
 */
void DATableExporterTest::testHtmlWithStyles()
{
    auto t = std::make_shared< TestMemoryTable >(2, 3);
    DA::DAData d(t);
    DA::DATableMetadata meta;
    DA::DATableStyleManager* sm = meta.styleManager();

    // 列级背景（col0 全列黄底）
    DA::DATableCellStyle colStyle;
    colStyle.setBackground(QBrush(QColor(255, 255, 0)));
    sm->setColumnStyle(0, colStyle);
    // 单元格级（1,2 红底白字，覆盖列层之外的第二处颜色源）
    DA::DATableCellStyle cellStyle;
    cellStyle.setBackground(QBrush(QColor(255, 0, 0)));
    cellStyle.setForeground(QColor(255, 255, 255));
    sm->setCellStyle(1, 2, cellStyle);

    // 条件格式：col2 > 0.1 → 绿底（行 1 的 0.25 命中；无统计依赖的 CellRule）
    DA::DATableConditionalFormatRule rule;
    rule.type      = DA::DATableConditionalFormatRule::Type::CellRule;
    rule.condition = DA::DATableConditionalFormatRule::Condition::Greater;
    rule.value1    = 0.1;
    DA::DATableCellStyle cfStyle;
    cfStyle.setBackground(QBrush(QColor(0, 200, 0)));
    rule.style = cfStyle;
    rule.enabled = true;
    meta.conditionalFormat()->setColumnRules(2, { rule });

    DA::DATableExporter exp(d, &meta);
    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::Html, &out));
    // 列级样式：两行 col0 的 td 都带黄底
    QCOMPARE(out.count(QStringLiteral("background-color:#ffff00")), 2);
    // 单元格样式：(1,2) 红底白字（红底与条件格式的绿底叠加：单元格先设、条件格式后覆盖 → 绿底）
    // 先验证白字存在
    QVERIFY(out.contains(QStringLiteral("color:#ffffff")));
    // 条件格式命中：行 1 col2 = 0.25 > 0.1 → 绿底；行 0 col2 = 0 不命中
    QCOMPARE(out.count(QStringLiteral("background-color:#00c800")), 1);
}

/**
 * 列显示格式：数值列保留 1 位小数（值→文本与表格 DisplayRole 同口径）
 */
void DATableExporterTest::testDisplayFormat()
{
    auto t = std::make_shared< TestMemoryTable >(2, 3);
    DA::DAData d(t);
    DA::DATableMetadata meta;
    DA::DATableDisplayFormat fmt(DA::DATableDisplayFormat::Category::Number);
    fmt.setPrecision(1);
    fmt.setValid(true);
    meta.styleManager()->setColumnFormat(2, fmt);

    DA::DATableExporter exp(d, &meta);
    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::TabText, &out));
    // col2: 0 → 0.0, 0.25 → 0.3（四舍五入到 1 位）
    QVERIFY(out.contains(QStringLiteral("\t0.0\n")));
    QVERIFY(out.contains(QStringLiteral("\t0.3\n")));
}

/**
 * 分块取数：超过单块行数的表按 c_blockRows 分块，fetch 次数 = 块数；进度回调单调递增
 */
void DATableExporterTest::testBlockFetchAndProgress()
{
    const int rows = DA::DATableExporter::c_blockRows * 2 + 10;  // 3 块（256+256+10）
    auto t         = std::make_shared< TestMemoryTable >(rows, 2);
    DA::DAData d(t);
    DA::DATableExporter exp(d);
    QString out;
    qint64 lastDone      = -1;
    int progressCalls    = 0;
    const qint64 totalIn = exp.rowCount();
    QVERIFY(exp.toString(DA::DATableExporter::Format::TabText, &out, nullptr,
                         [ & ](qint64 done, qint64 total) {
                             ++progressCalls;
                             QVERIFY(done > lastDone);
                             QVERIFY(done <= total);
                             QCOMPARE(total, totalIn);
                             lastDone = done;
                         }));
    QCOMPARE(t->mFetchCount, 3);
    QCOMPARE(progressCalls, 3);
    QCOMPARE(lastDone, totalIn);
    // 换行数 = 表头行尾 1 + 表头名内嵌 1（col1 名 "b|c\nd"）+ 数据行各 2
    //（行尾 1 + col1 值 "a|b\nc" 内嵌 1）
    QCOMPARE(out.count(QLatin1Char('\n')), 2 + rows * 2);
}

/**
 * 取消：第二块开始前取消 → 返回 false、错误信息为「操作已取消」
 */
void DATableExporterTest::testCancel()
{
    const int rows = DA::DATableExporter::c_blockRows * 3;
    auto t         = std::make_shared< TestMemoryTable >(rows, 2);
    DA::DAData d(t);
    DA::DATableExporter exp(d);
    QString out;
    QString err;
    int calls = 0;
    const bool ok = exp.toString(DA::DATableExporter::Format::TabText, &out, &err, nullptr, [ & ]() {
        // 第 2 次询问（第 2 块边界）返回取消
        return ++calls >= 2;
    });
    QVERIFY(!ok);
    QVERIFY(!err.isEmpty());
    QVERIFY(t->mFetchCount >= 1);
}

/**
 * 增量写入：write 到 QBuffer 与 toString 结果一致（复制/导出共用渲染核心）
 */
void DATableExporterTest::testWriteToFile()
{
    auto t = std::make_shared< TestMemoryTable >(300, 3);  // 跨 2 块
    DA::DAData d(t);
    DA::DATableMetadata meta;
    DA::DATableCellStyle s;
    s.setBackground(QBrush(QColor(200, 220, 255)));
    meta.styleManager()->setColumnStyle(0, s);

    DA::DATableExporter exp(d, &meta);
    for (auto fmt : { DA::DATableExporter::Format::TabText, DA::DATableExporter::Format::Markdown, DA::DATableExporter::Format::Html }) {
        QString viaString;
        QVERIFY(exp.toString(fmt, &viaString));
        QByteArray ba;
        QBuffer buf(&ba);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QVERIFY(exp.write(fmt, &buf));
        buf.close();
        // toString 与 write 产物一致（同一渲染核心的直接验证）
        QCOMPARE(QString::fromUtf8(ba), viaString);
    }
    // 未打开的设备写入失败
    QBuffer closedBuf;
    QString err;
    QVERIFY(!exp.write(DA::DATableExporter::Format::Html, &closedBuf, &err));
    QVERIFY(!err.isEmpty());
}

/**
 * 复制大小阈值判定（isLargeForCopy）
 */
void DATableExporterTest::testLargeCopyThreshold()
{
    QVERIFY(!DA::DATableExporter::isLargeForCopy(0, 100));
    QVERIFY(!DA::DATableExporter::isLargeForCopy(100, 0));
    const qint64 lim = DA::DATableExporter::c_largeCopyCellLimit;
    QVERIFY(!DA::DATableExporter::isLargeForCopy(lim / 1000, 1000));
    QVERIFY(DA::DATableExporter::isLargeForCopy(lim / 1000 + 1, 1000));
}

/**
 * 批注：HTML 的 td 带 title 属性（悬停提示携带批注全文）
 */
void DATableExporterTest::testNoteTitle()
{
    auto t = std::make_shared< TestMemoryTable >(2, 2);
    DA::DAData d(t);
    DA::DATableMetadata meta;
    meta.notes()->setNote(1, 0, QStringLiteral("note <text>"));  // 尖括号验证转义
    DA::DATableExporter exp(d, &meta);
    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::Html, &out));
    QVERIFY(out.contains(QStringLiteral("title=\"note &lt;text&gt;\"")));
}

/**
 * 列子集：指定列（含乱序/重复/越界）只导出这些列、按传入顺序；
 * 空/全无效列表回退整表（与双参构造等价）
 */
void DATableExporterTest::testColumnSubset()
{
    auto t = std::make_shared< TestMemoryTable >(3, 4);
    DA::DAData d(t);
    // 乱序 + 重复 + 越界：{2, 0, 2, 9} → 输出列 [col2, col0]
    DA::DATableExporter exp(d, nullptr, { 2, 0, 2, 9 });
    QVERIFY(exp.isValid());
    QCOMPARE(exp.columnCount(), qint64(2));
    QCOMPARE(exp.cellCount(), qint64(6));  // 3 行 × 2 列
    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::TabText, &out));
    QCOMPARE(out, QStringLiteral("col2\tcol0\n"
                                 "0\t0\n"
                                 "0.25\t1\n"
                                 "0.5\t2\n"));
    // Markdown 同一映射
    QVERIFY(exp.toString(DA::DATableExporter::Format::Markdown, &out));
    QVERIFY(out.startsWith(QStringLiteral("| col2 | col0 |")));
    // 空列表回退整表（与双参构造同结果）
    DA::DATableExporter whole(d, nullptr, {});
    QCOMPARE(whole.columnCount(), qint64(4));
    QString wholeOut, refOut;
    QVERIFY(whole.toString(DA::DATableExporter::Format::TabText, &wholeOut));
    DA::DATableExporter ref(d);
    QVERIFY(ref.toString(DA::DATableExporter::Format::TabText, &refOut));
    QCOMPARE(wholeOut, refOut);
}

/**
 * 列子集样式：样式/显示格式/批注按原始列号解析——子集只裁输出列，
 * 被选中列的样式与整表导出时完全一致
 */
void DATableExporterTest::testColumnSubsetWithStyles()
{
    auto t = std::make_shared< TestMemoryTable >(2, 4);
    DA::DAData d(t);
    DA::DATableMetadata meta;
    // col3 黄底（不在子集内）+ col0 红底（在子集内）
    DA::DATableCellStyle s0, s3;
    s0.setBackground(QBrush(QColor(255, 0, 0)));
    s3.setBackground(QBrush(QColor(255, 255, 0)));
    meta.styleManager()->setColumnStyle(0, s0);
    meta.styleManager()->setColumnStyle(3, s3);
    // col2 显示格式 1 位小数（子集内）
    DA::DATableDisplayFormat fmt(DA::DATableDisplayFormat::Category::Number);
    fmt.setPrecision(1);
    fmt.setValid(true);
    meta.styleManager()->setColumnFormat(2, fmt);
    // 批注 (1, 2)（子集内）
    meta.notes()->setNote(1, 2, QStringLiteral("note"));

    // 子集 {2, 0}：col3 的黄底不应出现，col0 红底与 col2 格式/批注应保留
    DA::DATableExporter exp(d, &meta, { 2, 0 });
    QString out;
    QVERIFY(exp.toString(DA::DATableExporter::Format::Html, &out));
    QVERIFY(out.contains(QStringLiteral("background-color:#ff0000")));  // col0 红底
    QCOMPARE(out.count(QStringLiteral("background-color:#ffff00")), 0);  // col3 未选
    QVERIFY(out.contains(QStringLiteral("title=\"note\"")));             // col2 批注
    // 列序：子集顺序 col2 在前
    QVERIFY(out.indexOf(QStringLiteral("<th>col2</th>")) < out.indexOf(QStringLiteral("<th>col0</th>")));
    // 显示格式：col2 的 0.25 → 0.3（Tab 文本验证）
    QString tabOut;
    QVERIFY(exp.toString(DA::DATableExporter::Format::TabText, &tabOut));
    QVERIFY(tabOut.contains(QStringLiteral("0.3\t")));
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DATableExporterTest tc;
    int ret = QTest::qExec(&tc, argc, argv);
    std::fflush(nullptr);
#ifdef Q_OS_WIN
    // Windows下_exit仍会走LdrShutdownProcess→DLL_PROCESS_DETACH触发各DLL静态析构，
    // 必须TerminateProcess才能完全跳过（与 DATableDataTest 同款处理）
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(ret));
#else
    ::_exit(ret);
#endif
    return ret;
}
#include "main.moc"
