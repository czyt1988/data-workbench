#include <QtTest/QtTest>
#include <QApplication>
#include <QHeaderView>
#include <QScrollBar>
#include <QSignalSpy>
#include <QUndoStack>
#include <algorithm>

#include "DAAbstractData.h"
#include "DAData.h"
#include "DATableDataBlock.h"
#include "DATableDataSource.h"
#include "DACacheWindowTableView.h"
#include "DAVirtualScrollBar.h"
#include "Models/DADataTableModel.h"

namespace
{
//===================================================
// 合成惰性数据源：千万行×5列，cell 按行列号实时生成，不驻留内存
// 行数可改，用于验证「数据变少后窗口自愈」
//===================================================
class DANavTestSource : public DA::DAAbstractData, public DA::DATableDataSource
{
public:
    static constexpr int c_totalRows = 10000000;
    static constexpr int c_totalCols = 5;

    int mRows               = c_totalRows;
    mutable int mFetchCount = 0;

public:  // DAAbstractData
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
    bool isReferenceData() const override
    {
        return true;
    }
    bool supportsUndoSnapshot() const override
    {
        return false;
    }
    QString typeIdentifier() const override
    {
        return QStringLiteral("DATableNavigationTest.LazyTable");
    }
    void write(QDataStream& out) override
    {
        out << mRows;
    }
    bool read(QDataStream& in) override
    {
        in >> mRows;
        return in.status() == QDataStream::Ok;
    }

public:  // DATableDataSource
    std::size_t tableRowCount() const override
    {
        return static_cast< std::size_t >(mRows);
    }
    std::size_t tableColumnCount() const override
    {
        return c_totalCols;
    }
    QString tableColumnName(std::size_t column) const override
    {
        return QStringLiteral("col%1").arg(column);
    }
    int tableColumnType(std::size_t column) const override
    {
        Q_UNUSED(column);
        return QMetaType::QString;
    }
    DA::DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override
    {
        ++mFetchCount;
        const std::size_t total = static_cast< std::size_t >(mRows);
        if (startRow >= total) {
            return DA::DATableDataBlock();
        }
        const std::size_t n = std::min(rowCount, total - startRow);
        DA::DATableDataBlock block(startRow, c_totalCols);
        QVariantList headers;
        headers.reserve(static_cast< int >(n));
        for (std::size_t r = 0; r < n; ++r) {
            QVariantList row;
            row.reserve(c_totalCols);
            for (std::size_t c = 0; c < c_totalCols; ++c) {
                row.append(makeCell(startRow + r, c));
            }
            block.appendRow(row);
            headers.append(static_cast< qlonglong >(startRow + r));
        }
        block.setRowHeaders(headers);
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

public:
    static QVariant makeCell(std::size_t row, std::size_t col)
    {
        return QVariant(QStringLiteral("r%1c%2").arg(row).arg(col));
    }
};

//===================================================
// 测试夹具参数：窗口缩到 500 行，强制频繁触发窗口重定心
//===================================================
constexpr int c_windowRows = 500;
constexpr int c_viewWidth  = 800;
constexpr int c_viewHeight = 420;

struct NavFixture {
    DANavTestSource* source          = nullptr;
    DA::DADataTableModel* model      = nullptr;
    DA::DACacheWindowTableView* view = nullptr;
    QUndoStack undoStack;
};
}  // namespace

/**
 * @brief 滑动窗表格导航协议测试
 *
 * 覆盖：窗口不变量、方向键逐行遍历不跳行、翻页精确一屏、滚轮步进、
 * Home/End 家族、精确定位、整表坐标滚动条拖动、窗口平移后视图状态保持、
 * 绝对行号口径、结构变化自愈。
 */
class DATableNavigationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();
    // 窗口不变量 T = W + v
    void testWindowInvariant();
    // 方向键跨窗口逐行遍历，不跳行
    void testArrowKeyTraversal();
    // 翻页键精确一屏，光标跟随
    void testPageKey();
    // 滚轮一格三行
    void testWheelStep();
    // Home/End/Ctrl+Home/Ctrl+End
    void testHomeEndKeys();
    // showActualRow/placeViewportAt 精确定位
    void testPlaceViewportExact();
    // 整表坐标滚动条：位置换算与拖动定位
    void testVirtualScrollBarSeek();
    // 窗口平移后当前项仍指向同一绝对行
    void testCurrentRowSurvivesWindowShift();
    // notify* 行号口径为绝对行
    void testNotifyUsesAbsoluteRow();
    // 数据变少后窗口自愈
    void testShrinkNormalizesWindow();

private:
    NavFixture m_fx;

private:
    void buildFixture(int windowRows = c_windowRows);
    void destroyFixture();
    int cursorActualRow() const;
    void sendWheel(int angleDeltaY);
    static void sendMouse(QWidget* w, QEvent::Type type, const QPoint& pos, Qt::MouseButton button, Qt::MouseButtons buttons);
};

/**
 * @brief 建表：千万行惰性源 + 500 行窗口
 * @param windowRows 窗口行数
 */
void DATableNavigationTest::buildFixture(int windowRows)
{
    m_fx.source = new DANavTestSource();
    DA::DAAbstractData::Pointer p(m_fx.source);
    DA::DAData d(p);
    m_fx.model = new DA::DADataTableModel(&m_fx.undoStack);
    m_fx.model->setCacheWindowSize(windowRows);
    m_fx.model->setData(d);
    m_fx.view = new DA::DACacheWindowTableView();
    m_fx.view->setModel(m_fx.model);
    m_fx.view->resize(c_viewWidth, c_viewHeight);
    m_fx.view->show();
    QApplication::processEvents();
}

void DATableNavigationTest::destroyFixture()
{
    delete m_fx.view;
    m_fx.view = nullptr;
    delete m_fx.model;
    m_fx.model  = nullptr;
    m_fx.source = nullptr;  // 由模型的引用计数释放
}

void DATableNavigationTest::init()
{
    buildFixture();
}

void DATableNavigationTest::cleanup()
{
    destroyFixture();
}

/**
 * @brief 当前光标所在的数据源绝对行
 * @return 绝对行号，无光标返回 -1
 */
int DATableNavigationTest::cursorActualRow() const
{
    const QModelIndex cur = m_fx.view->currentIndex();
    if (!cur.isValid()) {
        return -1;
    }
    return m_fx.view->toActualRow(cur.row());
}

/**
 * @brief 向视口发一次滚轮事件（负 angleDeltaY 表示向下滚）
 * @param angleDeltaY 滚轮角度增量
 */
void DATableNavigationTest::sendWheel(int angleDeltaY)
{
    QWidget* vp            = m_fx.view->viewport();
    const QPoint pos(20, 20);
    const QPoint globalPos = vp->mapToGlobal(pos);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QWheelEvent we(QPointF(pos),
                   QPointF(globalPos),
                   QPoint(),
                   QPoint(0, angleDeltaY),
                   Qt::NoButton,
                   Qt::NoModifier,
                   Qt::NoScrollPhase,
                   false);
#else
    QWheelEvent we(QPointF(pos), QPointF(globalPos), QPoint(), QPoint(0, angleDeltaY), 0, Qt::Vertical, Qt::NoButton, Qt::NoModifier);
#endif
    QApplication::sendEvent(vp, &we);
}

/**
 * @brief 发一次鼠标事件
 * @param w 目标部件
 * @param type 事件类型
 * @param pos 部件内坐标
 * @param button 按键
 * @param buttons 事件时的按键状态
 */
void DATableNavigationTest::sendMouse(QWidget* w,
                                      QEvent::Type type,
                                      const QPoint& pos,
                                      Qt::MouseButton button,
                                      Qt::MouseButtons buttons)
{
    const QPoint globalPos = w->mapToGlobal(pos);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMouseEvent e(type, QPointF(pos), QPointF(globalPos), button, buttons, Qt::NoModifier);
#else
    QMouseEvent e(type, QPointF(pos), QPointF(globalPos), button, buttons, Qt::NoModifier);
#endif
    QApplication::sendEvent(w, &e);
}

//===================================================
// 用例
//===================================================

/**
 * @brief 窗口不变量：视口首行 T = 窗口起始行 W + 滚动条 value v
 */
void DATableNavigationTest::testWindowInvariant()
{
    QCOMPARE(m_fx.view->getTopActualRow(), 0);
    QCOMPARE(m_fx.model->getCacheWindowStartRow(), 0);
    // 定位到很远的行
    m_fx.view->showActualRow(1234567);
    QCOMPARE(m_fx.view->getTopActualRow(), 1234567);
    QVERIFY(m_fx.model->getCacheWindowStartRow() > 0);
    QCOMPARE(m_fx.view->getTopActualRow(),
             m_fx.model->getCacheWindowStartRow() + m_fx.view->verticalScrollBar()->value());
    // 滚动条展示的是整表位置而不是窗口内位置
    DA::DAVirtualScrollBar* bar = m_fx.view->getVirtualScrollBar();
    QVERIFY(bar);
    QCOMPARE(bar->getDatasetFirstVisibleRow(), 1234567);
    QCOMPARE(bar->getDatasetTotalRows(), DANavTestSource::c_totalRows);
    QVERIFY(bar->getDatasetViewportRows() > 0);
    // 窗口起始行永远在合法范围内
    QVERIFY(m_fx.model->getCacheWindowStartRow() <= m_fx.model->maxCacheWindowStartRow());
}

/**
 * @brief 方向键逐行遍历：跨多个窗口也不跳行
 *
 * 这是原实现最核心的缺陷：每次经滚动条发生的滚动都被按「整表比例」放大，
 * 方向键一下会跳 totalRows/窗口行数 行。
 */
void DATableNavigationTest::testArrowKeyTraversal()
{
    m_fx.view->selectActualCell(0, 0);
    m_fx.view->setFocus();
    QCOMPARE(cursorActualRow(), 0);

    const int startW = m_fx.model->getCacheWindowStartRow();
    const int steps  = c_windowRows * 3;  // 走满 3 个窗口
    for (int i = 1; i <= steps; ++i) {
        QTest::keyClick(m_fx.view, Qt::Key_Down);
        QCOMPARE(cursorActualRow(), i);  // 一次一键正好一行
    }
    // 走过 3 个窗口，窗口必然搬动过
    QVERIFY(m_fx.model->getCacheWindowStartRow() != startW);
    QCOMPARE(m_fx.view->getTopActualRow(),
             m_fx.model->getCacheWindowStartRow() + m_fx.view->verticalScrollBar()->value());
    QVERIFY(m_fx.view->isActualRowVisible(cursorActualRow()));

    // 再逐行走回起点，同样不跳行
    for (int i = steps - 1; i >= 0; --i) {
        QTest::keyClick(m_fx.view, Qt::Key_Up);
        QCOMPARE(cursorActualRow(), i);
    }
    QCOMPARE(m_fx.model->getCacheWindowStartRow(), startW);
    QCOMPARE(m_fx.view->getTopActualRow(), 0);
}

/**
 * @brief 翻页键精确一屏，且光标跟随翻页
 */
void DATableNavigationTest::testPageKey()
{
    m_fx.view->selectActualCell(0, 0);
    const int vp = m_fx.view->getViewportRowCount();
    QVERIFY(vp > 1);

    const int pages = 40;  // 远超过一个窗口，必然触发窗口重定心
    for (int i = 1; i <= pages; ++i) {
        QTest::keyClick(m_fx.view, Qt::Key_PageDown);
        QCOMPARE(m_fx.view->getTopActualRow(), i * vp);
        QCOMPARE(cursorActualRow(), i * vp);
    }
    for (int i = pages - 1; i >= 0; --i) {
        QTest::keyClick(m_fx.view, Qt::Key_PageUp);
        QCOMPARE(m_fx.view->getTopActualRow(), i * vp);
        QCOMPARE(cursorActualRow(), i * vp);
    }
}

/**
 * @brief 滚轮一格 = 系统行数（默认 3 行），不是整表比例
 */
void DATableNavigationTest::testWheelStep()
{
    m_fx.view->selectActualCell(0, 0);
    QCOMPARE(m_fx.view->getTopActualRow(), 0);
    const int lines = qMax(1, QApplication::wheelScrollLines());
    sendWheel(-120);  // 向下滚一格
    QCOMPARE(m_fx.view->getTopActualRow(), lines);
    sendWheel(120);  // 向上滚一格
    QCOMPARE(m_fx.view->getTopActualRow(), 0);
}

/**
 * @brief Home/End 家族：Ctrl+Home/Ctrl+End 走整表语义，Home/End 只在当前行内移动
 */
void DATableNavigationTest::testHomeEndKeys()
{
    m_fx.view->selectActualCell(12345, 2);
    QCOMPARE(m_fx.view->getTopActualRow(), 12345);
    QTest::keyClick(m_fx.view, Qt::Key_Home, Qt::ControlModifier);
    QCOMPARE(m_fx.view->getTopActualRow(), 0);
    QCOMPARE(cursorActualRow(), 0);
    QCOMPARE(m_fx.view->currentIndex().column(), 0);

    // 无修饰键：当前行首列/末列，纵向位置不变
    m_fx.view->selectActualCell(20000, 3);
    QTest::keyClick(m_fx.view, Qt::Key_Home);
    QCOMPARE(cursorActualRow(), 20000);
    QCOMPARE(m_fx.view->currentIndex().column(), 0);
    QCOMPARE(m_fx.view->getTopActualRow(), 20000);
    QTest::keyClick(m_fx.view, Qt::Key_End);
    QCOMPARE(cursorActualRow(), 20000);
    QCOMPARE(m_fx.view->currentIndex().column(), m_fx.model->columnCount() - 1);
    QCOMPARE(m_fx.view->getTopActualRow(), 20000);

    // Ctrl+End：数据末行
    QTest::keyClick(m_fx.view, Qt::Key_End, Qt::ControlModifier);
    QCOMPARE(cursorActualRow(), DANavTestSource::c_totalRows - 1);
    QVERIFY(m_fx.view->isActualRowVisible(DANavTestSource::c_totalRows - 1));
}

/**
 * @brief 定位精确性：目标行落在视口首行（数据尾部只能贴最后一屏）
 */
void DATableNavigationTest::testPlaceViewportExact()
{
    const int vp    = m_fx.view->getViewportRowCount();
    const int total = DANavTestSource::c_totalRows;
    const QList< int > targets { 0, 1, 7, 12345, 5000000, 9999000, total - 1 };
    for (int t : targets) {
        m_fx.view->showActualRow(t);
        const int top = m_fx.view->getTopActualRow();
        if (t + vp <= total) {
            // 下面还有一整屏，目标就落在视口首行
            QCOMPARE(top, t);
        } else {
            // 数据尾部凑不满一屏：贴到最后一屏，但目标必须可见
            QVERIFY(m_fx.view->isActualRowVisible(t));
            QVERIFY(top <= t);
        }
        // 目标行确实能读到对应的数据（映射正确）
        const int localRow = m_fx.view->toLogicalRow(t);
        QVERIFY(localRow >= 0 && localRow < m_fx.model->rowCount());
        QCOMPARE(m_fx.model->data(m_fx.model->index(localRow, 0), Qt::DisplayRole),
                 DANavTestSource::makeCell(static_cast< std::size_t >(t), 0));
    }
}

/**
 * @brief 整表坐标滚动条：位置换算单调可逆、拖动到目标位置能定位过去
 */
void DATableNavigationTest::testVirtualScrollBarSeek()
{
    DA::DAVirtualScrollBar* bar = m_fx.view->getVirtualScrollBar();
    QVERIFY(bar);
    const int total = DANavTestSource::c_totalRows;
    const int vp    = m_fx.view->getViewportRowCount();

    // 位置换算单调：行号越大，位置越靠下
    bar->setDatasetPosition(0, total, vp);
    int lastPos = -1;
    for (int r = 0; r <= total - 1; r += total / 10) {
        const int pos = bar->positionForActualRow(r);
        QVERIFY(pos >= lastPos);
        lastPos = pos;
    }

    // 拖动滑块到目标位置：视图首行 = 该位置换算出来的行号
    const int targetRow  = total / 3;
    const int thumbTopY  = bar->positionForActualRow(bar->getDatasetFirstVisibleRow());
    const int targetTopY = bar->positionForActualRow(targetRow);
    const int grabOffset = 6;  // 抓在滑块上部，避免抓到边缘
    QVERIFY(bar->rect().contains(QPoint(bar->width() / 2, thumbTopY + grabOffset)));
    QVERIFY(bar->rect().contains(QPoint(bar->width() / 2, targetTopY + grabOffset)));

    sendMouse(bar, QEvent::MouseButtonPress, QPoint(bar->width() / 2, thumbTopY + grabOffset), Qt::LeftButton, Qt::LeftButton);
    QVERIFY(bar->isSeeking());
    sendMouse(bar, QEvent::MouseMove, QPoint(bar->width() / 2, targetTopY + grabOffset), Qt::NoButton, Qt::LeftButton);
    sendMouse(bar, QEvent::MouseButtonRelease, QPoint(bar->width() / 2, targetTopY + grabOffset), Qt::LeftButton, Qt::NoButton);
    QVERIFY(!bar->isSeeking());

    // 拖动落点 = 目标位置对应的行号（同一套换算，必然一致）
    const int expectRow = bar->actualRowForPosition(targetTopY);
    QCOMPARE(m_fx.view->getTopActualRow(), expectRow);
    QVERIFY(qAbs(expectRow - targetRow) < total / 20);  // 比例拖动精度受轨道像素限制
    // 拖动之后方向键依旧精确（缓冲区已经跟着搬过来）
    m_fx.view->selectActualCell(m_fx.view->getTopActualRow(), 0);
    const int before = cursorActualRow();
    QTest::keyClick(m_fx.view, Qt::Key_Down);
    QCOMPARE(cursorActualRow(), before + 1);
    QTest::keyClick(m_fx.view, Qt::Key_Up);
    QCOMPARE(cursorActualRow(), before);
}

/**
 * @brief 窗口平移（缓冲区搬家）后，当前项与选区仍指向同一绝对行
 */
void DATableNavigationTest::testCurrentRowSurvivesWindowShift()
{
    m_fx.view->selectActualCell(1000, 1);
    QCOMPARE(cursorActualRow(), 1000);
    const int w0 = m_fx.model->getCacheWindowStartRow();

    // 只挪视口、不动光标：向上滚一点就会触及缓冲区边缘，触发窗口平移
    m_fx.view->moveViewport(-100);
    QVERIFY(m_fx.model->getCacheWindowStartRow() != w0);
    QCOMPARE(cursorActualRow(), 1000);  // 绝对行没变

    // 再向下滚一个大跨度，同样要保持
    m_fx.view->moveViewport(300);
    QVERIFY(m_fx.model->getCacheWindowStartRow() != 0);
    QCOMPARE(cursorActualRow(), 1000);

    // 选区也还在同一绝对行上
    const int localRow = m_fx.view->toLogicalRow(1000);
    QVERIFY(localRow >= 0 && localRow < m_fx.model->rowCount());
    QVERIFY(m_fx.view->selectionModel()->isSelected(m_fx.model->index(localRow, 1)));
}

/**
 * @brief notify* 用数据源绝对行：窗口滑动后仍然命中正确的行
 */
void DATableNavigationTest::testNotifyUsesAbsoluteRow()
{
    m_fx.view->showActualRow(5000000);
    const int localRow = 3;
    const int absRow   = m_fx.view->toActualRow(localRow);
    QCOMPARE(absRow, m_fx.model->getCacheWindowStartRow() + localRow);

    QSignalSpy spy(m_fx.model, &QAbstractItemModel::dataChanged);
    m_fx.model->notifyDataChanged(absRow, 0);
    QCOMPARE(spy.count(), 1);
    const QModelIndex topLeft = spy.takeFirst().at(0).value< QModelIndex >();
    QCOMPARE(topLeft.row(), localRow);

    // 完全落在窗口外的绝对行不会误伤窗口内的行
    m_fx.model->notifyDataChanged(0, 0);
    QCOMPARE(spy.count(), 0);
}

/**
 * @brief 数据变少后窗口自愈：滚动范围与视口首行都落到新数据范围内
 */
void DATableNavigationTest::testShrinkNormalizesWindow()
{
    m_fx.view->showActualRow(9999000);
    QVERIFY(m_fx.model->getCacheWindowStartRow() > 0);

    m_fx.source->mRows = 100000;
    m_fx.model->notifyRowsRemoved({ 0 });  // 结构变化 → 视图自愈
    QApplication::processEvents();

    QCOMPARE(m_fx.model->actualRowCount(), 100000);
    QVERIFY(m_fx.model->getCacheWindowStartRow() <= m_fx.model->maxCacheWindowStartRow());
    QVERIFY(m_fx.view->getTopActualRow() <= 100000 - 1);
    QVERIFY(m_fx.view->getTopActualRow() >= 100000 - m_fx.view->getViewportRowCount());
    // 自愈之后键盘导航依旧精确
    m_fx.view->selectActualCell(99999, 0);
    QVERIFY(m_fx.view->isActualRowVisible(99999));  // 末行附近只能贴到最后一屏
    QCOMPARE(cursorActualRow(), 99999);
    QTest::keyClick(m_fx.view, Qt::Key_Up);
    QCOMPARE(cursorActualRow(), 99998);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    DATableNavigationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "main.moc"
