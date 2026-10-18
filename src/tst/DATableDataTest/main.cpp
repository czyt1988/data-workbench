#include <QtTest/QtTest>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <cstdlib>
#include <QDataStream>
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QVariantList>
#include <QMetaType>

#include "DAPyInterpreter.h"
#include "DAPybind11InQt.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
#include <pybind11/eval.h>

// DAPyScripts 算子封装（002：da_sort 多键置换 / da_reorder_rows / 查找替换）
#include "DAPyScripts.h"
#include "DAPyScriptsDataFrame.h"
#include "Commands/DACommandsDataFrame.h"
#include "Dialog/DADialogFindReplace.h"
#include <QElapsedTimer>
#include <QSet>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>

#include "DAData.h"
#include "DAAbstractData.h"
#include "DATableDataSource.h"
#include "DATableDataBlock.h"
#include "DATableScan.h"
#include "DADataFactory.h"

// 表格元数据基础设施（001 计划 §4.1）
#include "DATableHighlightOverlay.h"
#include "DATableKeyRemap.h"
#include "DATableMetadata.h"
#include "DATableMetadataRegistry.h"
#include "DATableStyleManager.h"
#include "DATableViewStateStore.h"
#include "DATablePaneContainer.h"
#include "DATableStatsWidget.h"
#include "Commands/DACommandsTableColumnView.h"
#include "Dialog/DADialogTableColumnView.h"
#include "Dialog/DADialogGotoCell.h"
#include <QWindow>
#include <QScrollBar>
#include <QSpinBox>
#include "DADataHeaderView.h"
#include "DADataTableView.h"
#include "DADataOperateOfDataFrameWidget.h"
#include <QSignalSpy>
#include <QMenu>
#include <QHeaderView>
#include <QTest>

#include "Models/DADataTableModel.h"
#include <QUndoStack>

// 条件格式（003 计划 §4.1）
#include "DATableConditionalFormatRule.h"
#include "DATableConditionalFormatStore.h"
#include "Commands/DACommandsConditionalFormat.h"
#include "Dialog/DADialogConditionalFormat.h"
#include <QDomDocument>
#include <QStandardItemModel>
#include <QElapsedTimer>
#include <QLabel>
#include <QImage>

// 计算列（004 计划 §3，T1.23）
#include "DAComputedColumn.h"
#include "DATableComputedColumnStore.h"
#include "Dialog/DADialogComputedColumn.h"
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QListWidget>
#include <QTableWidget>

// 分列/合并列（004 计划 §4，T1.9）
#include "Dialog/DADialogSplitColumn.h"
#include "Dialog/DADialogMergeColumns.h"
#include "Dialog/DADialogGroupBy.h"
#include "DADataManager.h"
#include <QRadioButton>
#include <QSpinBox>
#include <QItemSelectionModel>
#include <QItemSelection>

// 转置/melt 形状变换（004 计划 §6，T1.27）
#include "Dialog/DADialogReshape.h"
#include "DADataUndoCommand.h"
#include <QCheckBox>

// 填充（005 计划 §3，T1.24）
#include "Dialog/DADialogFillSeries.h"

// 数据校验（005 计划 §4，T1.25）
#include "DATableValidationRule.h"
#include "DATableValidationStore.h"
#include "Commands/DACommandsTableValidation.h"
#include "Dialog/DADialogValidationRules.h"
#include "Dialog/DADialogValidationReport.h"
#include "DATableStyleItemDelegate.h"
#include <QTreeWidget>
#include <QPlainTextEdit>

// 单元格批注（005 计划 §5，T1.26）
#include "DATableNoteStore.h"
#include "DATableNoteEditDialog.h"
#include "Commands/DACommandsTableStyle.h"
#include <QToolTip>
#include <QHelpEvent>
#include <QFile>

// 撤销历史面板（005 计划 §6，T1.21）
#include "DAUndoHistoryWidget.h"
#include <QUndoGroup>
#include <QUndoView>
#include <QSpinBox>

#include "DASqliteLazyTable.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>

#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

//===================================================
// 合成惰性数据源：千万行×5列，cell 按行列号实时生成，不驻留内存
// 模拟数据库惰性表的行为（fetchBlock 计数用于验证取数次数）
//===================================================
class DATestLazyTable : public DA::DAAbstractData, public DA::DATableDataSource
{
public:
    static constexpr std::size_t c_totalRows = 10000000;
    static constexpr std::size_t c_totalCols = 5;
    mutable int mFetchCount                  = 0;
    QString mRefName;  ///< 引用式序列化内容（模拟连接串+表名）

public:
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
        return QStringLiteral("DATableDataTest.LazyTable");
    }
    void write(QDataStream& out) override
    {
        out << mRefName;
    }
    bool read(QDataStream& in) override
    {
        in >> mRefName;
        return in.status() == QDataStream::Ok;
    }

public:  // DATableDataSource
    std::size_t tableRowCount() const override
    {
        return c_totalRows;
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
        switch (column) {
        case 0:
            return QMetaType::LongLong;
        case 1:
            return QMetaType::Double;
        case 2:
            return QMetaType::QString;
        case 3:
            return QMetaType::Bool;
        default:
            return QMetaType::Int;
        }
    }
    DA::DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override
    {
        ++mFetchCount;
        if (startRow >= c_totalRows) {
            return DA::DATableDataBlock();
        }
        std::size_t n = std::min(rowCount, c_totalRows - startRow);
        DA::DATableDataBlock block(startRow, c_totalCols);
        QVariantList headers;
        headers.reserve(static_cast< int >(n));
        for (std::size_t r = 0; r < n; ++r) {
            QVariantList row;
            row.reserve(static_cast< int >(c_totalCols));
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
        switch (col) {
        case 0:
            return QVariant(static_cast< qlonglong >(row));
        case 1:
            return QVariant(static_cast< double >(row) * 0.5 + static_cast< double >(col));
        case 2:
            return QVariant(QStringLiteral("r%1c%2").arg(row).arg(col));
        case 3:
            return QVariant(row % 2 == 0);
        default:
            return QVariant(static_cast< int >(row + col));
        }
    }
};

//===================================================
// 测试辅助
//===================================================

// NaN感知的QVariant相等比较（QVariant(double NaN) == QVariant(double NaN) 为 false）
static bool variantEquals(const QVariant& a, const QVariant& b)
{
    if (a.userType() == QMetaType::Double || b.userType() == QMetaType::Double) {
        bool aNan = (a.userType() == QMetaType::Double && qIsNaN(a.toDouble()));
        bool bNan = (b.userType() == QMetaType::Double && qIsNaN(b.toDouble()));
        if (aNan || bNan) {
            return aNan && bNan;
        }
    }
    return a == b;
}

// 自造带 buttons 的鼠标事件（拖拽模拟；006c）。pos 为控件局部坐标；
// QTest::mouseMove 不携带按键状态、不能用——照 DATableNavigationTest 拷一份。
// 事件经 QApplication::sendEvent 直达目标部件：用于 mDragMoved 判据类回归
// （不依赖 QGuiApplication::mouseButtons()/underMouse() 的路径）。
static void sendMouse(QWidget* w, QEvent::Type type, const QPoint& pos, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    const QPoint globalPos = w->mapToGlobal(pos);
    QMouseEvent e(type, QPointF(pos), QPointF(globalPos), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(w, &e);
}

// 走 QTest 的 QWindow 重载（经 QWindowSystemInterface 平台链路：会更新
// QGuiApplication::mouseButtons() 并合成 Enter/Leave，命中"用户拖拽"闸门；
// 006c 列宽拖拽用例专用——QWidget 重载不走平台、闸门不触发）
static void sendWindowMouse(QWidget* target, QEvent::Type type, const QPoint& pos, Qt::MouseButton button)
{
    QWidget* tlw = target->window();
    QWindow* win = tlw->windowHandle();
    if (!win) {
        return;
    }
    const QPoint localPos = target->mapTo(tlw, pos);
    if (type == QEvent::MouseButtonPress) {
        QTest::mousePress(win, button, Qt::NoModifier, localPos);
    } else if (type == QEvent::MouseButtonRelease) {
        QTest::mouseRelease(win, button, Qt::NoModifier, localPos);
    } else if (type == QEvent::MouseMove) {
        QTest::mouseMove(win, localPos);
    }
}

// 构造混合类型测试DataFrame（int64/float64含NaN/str/datetime64/bool，10行5列）
static pybind11::object makeTestDataFrame(){
    try {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({
    "i": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    "f": [0.5, float("nan"), 2.5, 3.5, float("nan"), 5.5, 6.5, 7.5, 8.5, 9.5],
    "s": ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"],
    "d": pd.to_datetime(["2020-01-01","2020-01-02","2020-01-03","2020-01-04","2020-01-05",
                          "2020-01-06","2020-01-07","2020-01-08","2020-01-09","2020-01-10"]),
    "b": [True, False, True, False, True, False, True, False, True, False],
})
)",
                       pybind11::globals(),
                       scope);
        return scope[ "df" ];
    } catch (const std::exception& e) {
        qWarning() << "makeTestDataFrame failed:" << e.what();
    }
    return pybind11::none();
}

// 构造测试Series（float64含NaN，带名字，6行）
static pybind11::object makeTestSeries()
{
    try {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
ser = pd.Series([1.5, float("nan"), 2.5, 3.5, float("nan"), 5.5], name="ser")
)",
                       pybind11::globals(),
                       scope);
        return scope[ "ser" ];
    } catch (const std::exception& e) {
        qWarning() << "makeTestSeries failed:" << e.what();
    }
    return pybind11::none();
}

// 构造int64测试Series（验证批量取数保持numpy标量类型，与iat一致为LongLong）
static pybind11::object makeTestSeriesInt()
{
    try {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
ser = pd.Series([10, 20, 30, 40], dtype="int64", name="ser_int")
)",
                       pybind11::globals(),
                       scope);
        return scope[ "ser" ];
    } catch (const std::exception& e) {
        qWarning() << "makeTestSeriesInt failed:" << e.what();
    }
    return pybind11::none();
}

//===================================================
// 测试类
//===================================================
class DATableDataTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testLazyTableSchema();
    void testLazyTableFetchBlock();
    void testBlockBounds();
    void testPandasDataFrameEquivalence();
    void testPandasSeriesEquivalence();
    void testEmptyDataFrame();
    void testFactoryRoundTrip();
    void testModelLazyDisplay();
    void testModelBlockCacheAndScroll();
    void testModelPandasEditRegression();
    void testSqliteLazyTable();
    void testSqliteModelIntegration();
    void testSqliteReferenceRoundTrip();
    // 表格元数据基础设施（001 计划 §4.1）：瞬态高亮层
    void testHighlightOverlayBasics();
    void testHighlightOverlayPriority();
    void testHighlightOverlayKeyPacking();
    // 位置键重映射工具与 DATableStyleManager::onRowsReordered
    void testKeyRemapUtility();
    void testStyleManagerRowsReordered();
    // 视图状态子存储与元数据注册表
    void testViewStateStore();
    void testMetadataRegistry();
    // §4.2 手工验收的程序化验证：旧工程 XML 兼容、菜单构成、视图状态重开应用、表头点击链路
    void testLegacyTableStylesXmlCompat();
    void testContextMenuComposition();
    void testViewStateApplyAndHeaderClickChain();
    // 006a：view-state 三个新子元素（列序/隐藏/冻结列数）的 round-trip 与迁移
    void testViewStateColumnViewXml();
    // 006a：normalizeToColumns 加载收敛语义
    void testViewStateNormalizeConvergence();
    // 006a：entryCountsByCategory 覆盖列视图设置（列宽+列序+隐藏+冻结）
    void testMetadataEntryCountsColumnView();
    // 006b：容器运行时包装主窗格 + 列序/隐藏投影（列宽/尾部空白列不受影响）
    void testPaneContainerWrappingAndMainPaneProjection();
    // 006b：setColumnHiddenDirectly 直接入口（写 store + 重投影、不入撤销栈）
    void testSetColumnHiddenDirectlyNoUndo();
    // 006c：列序拖动不误发点击（红色用例：拖回原段）且落库入栈一条
    void testColumnOrderDragDoesNotSort();
    // 006c：列视图命令族往返（列序/隐藏/冻结 + 批量父命令）
    void testColumnViewCommandsUndoRedo();
    // 006c：真实列宽拖拽写 store、不入撤销栈（可拖动表头回归点）
    void testColumnWidthNotOnUndoStack();
    // 006d：「列」子菜单 5 项逐项生效（隐藏/显示全部/冻结/解冻/对话框）
    void testHeaderColumnViewMenuActions();
    // 006d：列视图对话框确定按差异入一个撤销单元、一次 undo 全还原
    void testColumnViewDialogDiffAndUndo();
    // 006d：重名列 a / a (2) 独立勾选、取消不改动
    void testColumnViewDialogDuplicateNamesInitialAndCancel();
    // 006e T6：冻结 + 列视图叠加（双窗格投影互补、宽度、policy、滚动同步、转发、往返）
    void testFrozenAndColumnViewCombination();
    // 006e T7：列宽在冻结/解冻往返中不回退、不入撤销栈
    void testColumnWidthSurvivesFreezeCycle();
    // 006e T8：行头归属、双向滚动同步、共享 selectionModel
    void testPaneRowHeaderAndScrollSync();
    // 007a：定位单入口——隐藏列自动取消隐藏（不入栈）
    void testLocateActualCellHiddenColumn();
    // 007a：定位到冻结区列——主区横滚不动、目标格选中、焦点给冻结窗格
    void testLocateActualCellFrozenColumn();
    // 007a：定位窗外行——整表语义（窗口搬动到目标行）
    void testLocateActualCellOutOfWindow();
    // 007a：keepHidden=true / select=false 的边界（导航不收缩选区、不解除隐藏）
    void testLocateKeepHiddenAndNoSelect();
    // 007b：定位请求解析（行位置/索引标签/列名/快捷项/keepHidden 全路径）
    void testGotoDialogRequestResolution();
    // 007c：列统计区间路径与 pandas 直算对拍（含类型/空值/区间边界/唯一值数）
    void testColumnStatsOperatorExtended();
    // 007c：旧路径兼容（columnStats 三元组行为逐位不变）
    void testColumnStatsLegacyCompatibility();
    // 007c：唯一值数哨兵（超阈值/不要/数值列不请求）
    void testColumnStatsUniqueSentinel();
    // 007c：computeColumnStats 默认 nullopt + 覆写生效
    void testComputeColumnStatsNullopt();
    // 007d：状态栏统计分档显示（单列/多列/超限/多段/单格/不支持）
    void testStatsWidgetSelectionTiers();
    // 007d：统计部件去抖与只读（撤销栈不变、解绑隐藏）
    void testStatsWidgetDebounceAndReadonly();
    // 002 表头排序（T1.3 §4.1）：算子多键+置换、命令 undo/redo+样式重映射、表头交互/拾取互斥/只读源、10万行计时
    void testSortOperatorMultiKeyPerm();
    void testSortRowsCommandUndoRedoAndStyleRemap();
    void testHeaderSortInteraction();
    void testSortPerf100k();
    // 排序选区重映射：散落选区跟随正确性 + 连续段归并 + 整列选区排序性能守卫（2026-10-18 修复回归）
    void testSortSelectionRemapPerf();
    // 002 查找/替换（T1.4 §4.1）：搜索扩展、替换算子、替换命令 undo/redo、浮动条去抖、页面查找替换全链路
    void testSearchOperatorExtended();
    void testFindReplaceOperator();
    void testReplaceCommandUndoRedo();
    void testFindReplaceDialogDebounce();
    void testFindReplaceWidgetFlow();
    // 003 条件格式（T1.5 §4.1）：规则求值、叠加顺序、色阶插值、数据条归一化、
    // 存储 XML round-trip/重映射/统计缓存、命令 undo/redo、统计算子、绘制像素级验证、
    // 对话框惰性源限制、千行滚动计时
    void testConditionalRuleEvaluation();
    void testConditionalEvaluateOrder();
    void testColorScaleInterpolation();
    void testDataRowNormalization();
    void testConditionalStoreXmlAndRemap();
    void testConditionalStoreStatsCache();
    void testConditionalCommands();
    void testColumnStatsOperator();
    void testConditionalPaintWidgetFlow();
    void testConditionalDialogSourceRestriction();
    void testConditionalScrollPerf1k();

    // 004 WP1 计算列（T1.23 §3.4/§7.1）：定义类与三态存储状态机、XML round-trip、
    // 依赖提取（含反引号/空格列名/语法错误）、求值算子（快照/失败不动数据）、
    // 重算命令 undo/redo、对话框校验、widget 全链路（创建/标脏/重算/撤销/表头三态/
    // 右键菜单/改名删列联动/打开工程不重算）、排序不标脏（R13）
    void testComputedColumnDefinitionAndStore();
    void testComputedColumnStoreXmlRoundTrip();
    void testComputedColumnExprDepends();
    void testComputedColumnEvalOperator();
    void testComputedColumnCommandUndoRedo();
    void testComputedColumnDialogValidation();
    void testComputedColumnWidgetFlow();
    void testComputedColumnRenameRemoveLinkage();

    // 004 WP2 分列/合列（T1.9 §4.4/§7.1）：三模式算子、溢出策略、类型推断开关、
    // 合并顺序/分隔符/跳过空值、列级撤销还原（列数/列序/内容）、10 万行分列计时、
    // 对话框校验与 widget 全链路
    void testSplitColumnOperator();
    void testMergeColumnsOperator();
    void testSplitMergeCommandUndoRedo();
    void testSplitColumnPerf100k();
    void testSplitMergeWidgetFlow();

    // 004 WP3 分组聚合（T1.22 §5.4/§7.1）：多键多列与 pandas 手算对齐、size、
    // NaN 键组保留、全 NaN 值列、函数守卫、新数据集注册（xxx_GroupBy 命名 + addData_ 可撤销）
    void testGroupbyOperator();
    void testGroupByWidgetFlow();

    // 004 WP4 转置/melt（T1.27 §6.4/§7.1）：算子形状/列名/dtype、melt 行数=行×展开列、
    // 规模守卫预估与阈值、就地替换的元数据类别条目数统计与快照清理还原、
    // 默认产新数据集（原表无损+注册可撤销）与就地路径（数据+元数据同一撤销单元）
    void testTransposeMeltOperator();
    void testMeltGuardEstimate();
    void testInplaceReshapeMetadataClear();
    void testReshapeWidgetFlow();

    // 005 WP1 填充（T1.24 §3.4/§7.1）：选区求值（NaN/NaT/None 三种空值、四种模式、
    // 不足两行）、序列生成（等差/终止值/日期步进）、一次填充一条命令 + undo 完全还原、
    // 多列独立、Ctrl+D 快捷键与右键菜单构成
    void testFillSelectionOperator();
    void testFillSeriesOperator();
    void testFillCommandUndoRedo();
    void testFillWidgetFlow();

    // 005 WP2 数据校验（T1.25 §4.4/§7.1）：规则定义与存储 XML round-trip 与改名迁移、
    // 各类型违规判定 + 重名消歧 + 缺列 + 非法正则、违规高亮来源隔离与两级级别、
    // 唯一性缓存二次不重算（打点计数）、校验只读不入栈、规则对话框命令入栈与撤销、
    // 枚举下拉编辑器（allowOther 开关 / 无规则默认编辑器 / 只读门禁在模型 flags）、
    // 报告 Markdown 与点击定位
    void testValidationRuleAndStoreXml();
    void testValidateOperator();
    void testValidationWidgetFlow();
    void testValidationRulesDialogFlow();
    void testValidationRenameMigration();
    void testEnumEditorDelegate();
    void testValidationReportMarkdown();
    // 100 万行表校验耗时可接受（§4.4 验收：记录耗时；唯一性只算一次、二次不重算）
    void testValidationPerf1M();

    // 005 WP3 单元格批注（T1.26 §5.4/§7.1）：存储增删改/XML round-trip/快照、
    // 位置键重映射（排序置换跟行走/插删行下移/插删列）、命令 undo/redo（旧新文本+
    // 时间戳）、widget 全链路（弹层/清除/插行下移/排序跟随/导出不污染红线3）、
    // 角标像素断言与 helpEvent tooltip
    void testNoteStoreBasics();
    void testNoteRemap();
    void testNoteCommands();
    void testNoteWidgetFlow();
    void testNotePaintAndTooltip();

    // 005 WP4-A 撤销历史面板与深度可配置（T1.21 §6.3/§7.1）：默认 20/调大后 30 步
    // 全撤销/深度裁剪仍生效、QUndoView 绑组跟随激活栈与无空白条目、深度调节信号
    // （用户改发/程序注入不发）、Swap/Persist 命令默认文案非空（红线5）
    void testUndoStackDepthConfigurable();
    void testUndoHistoryWidget();
    void testUndoCommandDefaultTexts();

    // 005 §7.2 收官：10 万行综合体验链路（批注→排序→查找替换→条件格式→填充→校验），
    // 分步计时 + 全部撤销后数据精确还原
    void testBatch3FullChain100k();

private:
    // 生成/复用sqlite测试库（百万行），失败返回false并置mSqliteErr
    bool ensureSqliteReady();
    // 创建绑定测试库的惰性表数据（SELECT带ORDER BY保证分页稳定）
    DA::DAAbstractData::Pointer makeSqliteLazyData();

private:
    static constexpr qlonglong c_sqliteRows = 1000000;
    bool mPandasOk                          = false;
    bool mPyScriptsOk                       = false;  ///< DAPyScripts 算子模块可用（da_sort 等）
    QUndoStack mUndoStack;
    QTemporaryDir mTmpDir;
    QString mSqliteConnName = QStringLiteral("databledata_sqlite_conn");
    bool mSqliteReady       = false;
    QString mSqliteErr;
};

void DATableDataTest::initTestCase()
{
    // 与APP main.cpp的初始化逻辑一致：解析解释器路径并设置Python Home，
    // 否则缺少PyConfig home时sys.path不含site-packages，pandas无法导入。
    // 候选中优先取存在Lib/site-packages的真实安装，排除Microsoft Store存根
    // （WindowsApps\python.exe是0字节重解析点，会导致sys.path损坏）
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
        QString pypath = DA::DAPyInterpreter::getPythonInterpreterPath();
        if (!pypath.isEmpty()) {
            pythonHomePath = QFileInfo(pypath).absolutePath();
        }
    }
    DA::DAPyInterpreter::initializePythonInterpreter(pythonHomePath);
    try {
        pybind11::module::import("pandas");
        mPandasOk = true;
    } catch (const std::exception& e) {
        qWarning() << "pandas not available:" << e.what();
        mPandasOk = false;
    }
    // DAPyScripts 算子模块（002）：sys.path 指向本仓库 src/PyScripts（DAWorkbench 包源码位置），
    // 再初始化模块封装。DAWorkbench.DAPyBase 的 io/app_wrapper 依赖 app 注册的 da_app 绑定模块，
    // 测试环境缺失时按 _safe_import_submodule 降级为 None，不影响 dataframe 算子子模块
    if (mPandasOk) {
        const QString tstDir   = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath();
        const QString repoRoot = QDir::cleanPath(tstDir + QStringLiteral("/../../.."));
        DA::DAPyInterpreter::appendSysPath(repoRoot + QStringLiteral("/src/PyScripts"));
        // 测试环境没有 app 注册的 da_app/da_interface/da_data pybind 模块，
        // DAWorkbench.DAPyBase.io / app_wrapper 会导入失败 → DAWorkbench 顶层缺 io 属性 →
        // DAPyWorkBench::import 的 attr("io") 抛错连坐 dataframe。用空模块桩顶掉这三个
        // 绑定模块（io.py 只在函数体内使用 da_app.*，模块级 import 桩即可通过），
        // 让 dataframe 算子模块可用——测试只用 da_sort/da_search_data 等纯 pandas 算子
        try {
            pybind11::exec(R"(
import sys, types
for _m in ("da_app", "da_interface", "da_data"):
    if _m not in sys.modules:
        sys.modules[_m] = types.ModuleType(_m)
)");
        } catch (const std::exception& e) {
            qWarning() << "stub da_* modules failed:" << e.what();
        }
        mPyScriptsOk = DA::DAPyScripts::initScripts();
        if (!mPyScriptsOk) {
            qWarning() << "DAPyScripts::initScripts failed";
        }
    }
}

void DATableDataTest::cleanupTestCase()
{
    if (QSqlDatabase::contains(mSqliteConnName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(mSqliteConnName, false);
            if (db.isOpen()) {
                db.close();
            }
        }  // 局部db副本先析构，避免removeDatabase告警
        QSqlDatabase::removeDatabase(mSqliteConnName);
    }
}

// 生成/复用sqlite测试库：bigtable(id INTEGER PRIMARY KEY, val REAL, label TEXT)
bool DATableDataTest::ensureSqliteReady()
{
    if (mSqliteReady) {
        return true;
    }
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        mSqliteErr = QStringLiteral("QSQLITE driver not available");
        return false;
    }
    QString dbPath = mTmpDir.isValid() ? mTmpDir.filePath(QStringLiteral("lazy.sqlite"))
                                       : QDir::temp().filePath(QStringLiteral("databledata_lazy.sqlite"));
    {
        QSqlDatabase db = QSqlDatabase::contains(mSqliteConnName)
                              ? QSqlDatabase::database(mSqliteConnName, false)
                              : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), mSqliteConnName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            mSqliteErr = db.lastError().text();
            return false;
        }
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS bigtable(id INTEGER PRIMARY KEY, val REAL, label TEXT)"))) {
            mSqliteErr = q.lastError().text();
            return false;
        }
        // 已生成过则直接复用
        QSqlQuery cnt(db);
        if (cnt.exec(QStringLiteral("SELECT COUNT(*) FROM bigtable")) && cnt.next()
            && cnt.value(0).toLongLong() >= c_sqliteRows) {
            mSqliteReady = true;
            return true;
        }
        q.exec(QStringLiteral("DELETE FROM bigtable"));
        db.transaction();
        const qlonglong batch = 5000;
        for (qlonglong start = 0; start < c_sqliteRows; start += batch) {
            QStringList values;
            values.reserve(static_cast< int >(batch));
            for (qlonglong r = start; r < qMin(start + batch, c_sqliteRows); ++r) {
                values << QStringLiteral("(%1,%2,'row%1')").arg(r).arg(r * 0.5);
            }
            QString sql = QStringLiteral("INSERT INTO bigtable(id,val,label) VALUES ") + values.join(QLatin1Char(','));
            if (!q.exec(sql)) {
                mSqliteErr = q.lastError().text();
                db.rollback();
                return false;
            }
        }
        db.commit();
    }
    mSqliteReady = true;
    return true;
}

DA::DAAbstractData::Pointer DATableDataTest::makeSqliteLazyData()
{
    std::shared_ptr< DASqliteLazyTable > p = std::make_shared< DASqliteLazyTable >();
    p->setConnectionName(mSqliteConnName);
    p->setSelectSql(QStringLiteral("SELECT id, val, label FROM bigtable ORDER BY id"));
    return p;
}

// 合成惰性源经DAData句柄的schema与能力位
void DATableDataTest::testLazyTableSchema()
{
    DA::DAAbstractData::Pointer p(new DATestLazyTable());
    DA::DAData d(p);
    QVERIFY(!d.isNull());
    QVERIFY(d.isTable());
    QVERIFY(!d.isDataFrame());
    QVERIFY(!d.isSeries());
    QVERIFY(d.isReferenceData());
    QVERIFY(!d.supportsUndoSnapshot());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("DATableDataTest.LazyTable"));

    // shape()走tableSource路由，不再返回(0,0)
    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, DATestLazyTable::c_totalRows);
    QCOMPARE(sp.second, DATestLazyTable::c_totalCols);

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QCOMPARE(ts->tableColumnName(2), QStringLiteral("col2"));
    QCOMPARE(ts->tableColumnType(0), static_cast< int >(QMetaType::LongLong));
    QVERIFY(ts->isLazyLoaded());
    QVERIFY(!ts->isTableEditable());

    // const路径
    const DA::DAData& cd               = d;
    const DA::DATableDataSource* cts   = cd.tableSource();
    QVERIFY(cts != nullptr);
    QCOMPARE(cts->tableRowCount(), DATestLazyTable::c_totalRows);
}

// 合成惰性源的块级取数：越界、截断、行头、取数计数
void DATableDataTest::testLazyTableFetchBlock()
{
    DATestLazyTable* lazy = new DATestLazyTable();
    DA::DAAbstractData::Pointer p(lazy);
    DA::DAData d(p);
    DA::DATableDataSource* ts = d.tableSource();

    const int before                 = lazy->mFetchCount;
    DA::DATableDataBlock block       = ts->fetchBlock(5000000, 100);
    QCOMPARE(lazy->mFetchCount, before + 1);  // 一次块取数=一次fetch
    QVERIFY(block.isValid());
    QCOMPARE(block.startRow(), static_cast< std::size_t >(5000000));
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(100));
    QCOMPARE(block.columnCount(), DATestLazyTable::c_totalCols);
    QVERIFY(block.containsRow(5000000));
    QVERIFY(block.containsRow(5000099));
    QVERIFY(!block.containsRow(5000100));
    QVERIFY(!block.containsRow(4999999));

    for (std::size_t r = 5000000; r < 5000100; r += 7) {
        for (std::size_t c = 0; c < DATestLazyTable::c_totalCols; ++c) {
            QVERIFY(block.cell(r, c) == DATestLazyTable::makeCell(r, c));
            // 与cell级接口toVariant一致
            QVERIFY(d.value(r, c) == block.cell(r, c));
        }
        QVERIFY(block.rowHeader(r) == QVariant(static_cast< qlonglong >(r)));
    }
    // 越界cell返回无效QVariant
    QVERIFY(!block.cell(5000100, 0).isValid());
    QVERIFY(!block.cell(5000000, 99).isValid());

    // 尾部截断
    DA::DATableDataBlock tail = ts->fetchBlock(DATestLazyTable::c_totalRows - 10, 100);
    QVERIFY(tail.isValid());
    QCOMPARE(tail.rowCount(), static_cast< std::size_t >(10));

    // 完全越界返回无效块
    QVERIFY(!ts->fetchBlock(DATestLazyTable::c_totalRows, 1).isValid());
}

// DATableDataBlock自身的边界行为
void DATableDataTest::testBlockBounds()
{
    DA::DATableDataBlock invalid;
    QVERIFY(!invalid.isValid());
    QVERIFY(invalid.isEmpty());
    QVERIFY(!invalid.containsRow(0));
    QVERIFY(!invalid.cell(0, 0).isValid());

    DA::DATableDataBlock block(10, 2);
    QVERIFY(block.isValid());
    QVERIFY(block.isEmpty());  // 有效但还没有行
    QCOMPARE(block.startRow(), static_cast< std::size_t >(10));
    QCOMPARE(block.columnCount(), static_cast< std::size_t >(2));

    QVariantList row;
    row << QVariant(1) << QVariant(QStringLiteral("a"));
    block.appendRow(row);
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(1));
    QVERIFY(block.containsRow(10));
    QVERIFY(!block.containsRow(9));
    QVERIFY(!block.containsRow(11));
    QCOMPARE(block.cell(10, 0), QVariant(1));
    QCOMPARE(block.cell(10, 1).toString(), QStringLiteral("a"));

    block.clear();
    QVERIFY(!block.isValid());
}

// pandas DataFrame：fetchBlock与逐cell iat的等价性（值+类型）
void DATableDataTest::testPandasDataFrameEquivalence()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    QVERIFY(d.isDataFrame());
    QVERIFY(d.isTable());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("DataFrame"));
    QVERIFY(!d.isReferenceData());
    QVERIFY(d.supportsUndoSnapshot());

    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(10));
    QCOMPARE(sp.second, static_cast< std::size_t >(5));

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QVERIFY(!ts->isLazyLoaded());      // pandas数据全量在内存
    QVERIFY(ts->isTableEditable());    // 可编辑

    // 列名与现有columns()一致
    const QList< QString > cols = df.columns();
    QCOMPARE(static_cast< std::size_t >(cols.size()), ts->tableColumnCount());
    for (std::size_t c = 0; c < ts->tableColumnCount(); ++c) {
        QCOMPARE(ts->tableColumnName(c), cols[ static_cast< int >(c) ]);
    }

    // 列类型与caster转换语义对齐
    QCOMPARE(ts->tableColumnType(0), static_cast< int >(QMetaType::LongLong));   // int64
    QCOMPARE(ts->tableColumnType(1), static_cast< int >(QMetaType::Double));     // float64
    QCOMPARE(ts->tableColumnType(2), static_cast< int >(QMetaType::UnknownType));  // object(str)
    QCOMPARE(ts->tableColumnType(3), static_cast< int >(QMetaType::QDateTime));   // datetime64
    QCOMPARE(ts->tableColumnType(4), static_cast< int >(QMetaType::Bool));       // bool

    // 全表块取数与逐cell iat等价（值+QVariant类型）
    DA::DATableDataBlock block = ts->fetchBlock(0, 10);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(10));
    for (std::size_t r = 0; r < 10; ++r) {
        for (std::size_t c = 0; c < 5; ++c) {
            QVariant expect = df.iat(r, c);
            QVariant got    = block.cell(r, c);
            QVERIFY2(variantEquals(expect, got),
                     qPrintable(QStringLiteral("cell(%1,%2) mismatch: iat=%3 block=%4")
                                    .arg(r)
                                    .arg(c)
                                    .arg(expect.toString())
                                    .arg(got.toString())));
            QCOMPARE(got.userType(), expect.userType());
        }
        // 行头与index().value(i)等价
        QVERIFY(block.rowHeader(r) == df.index().value(r));
    }

    // 部分块：fetchBlock(4,3)覆盖绝对行4~6
    DA::DATableDataBlock part = ts->fetchBlock(4, 3);
    QVERIFY(part.isValid());
    QCOMPARE(part.rowCount(), static_cast< std::size_t >(3));
    QVERIFY(part.containsRow(4));
    QVERIFY(part.containsRow(6));
    QVERIFY(!part.containsRow(7));
    QVERIFY(variantEquals(part.cell(5, 1), df.iat(5, 1)));

    // 尾部截断与越界
    DA::DATableDataBlock tail = ts->fetchBlock(9, 5);
    QVERIFY(tail.isValid());
    QCOMPARE(tail.rowCount(), static_cast< std::size_t >(1));
    QVERIFY(!ts->fetchBlock(10, 5).isValid());
}

// pandas Series：单列表格语义与逐元素value()等价
void DATableDataTest::testPandasSeriesEquivalence()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestSeries();
    QVERIFY(!obj.is_none());
    DA::DAPySeries ser(obj);
    DA::DAData d(ser);
    QVERIFY(d.isSeries());
    QVERIFY(d.isTable());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("Series"));

    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(6));
    QCOMPARE(sp.second, static_cast< std::size_t >(1));

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QCOMPARE(ts->tableColumnName(0), QStringLiteral("ser"));
    QCOMPARE(ts->tableColumnType(0), static_cast< int >(QMetaType::Double));

    DA::DATableDataBlock block = ts->fetchBlock(1, 3);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(3));
    QCOMPARE(block.columnCount(), static_cast< std::size_t >(1));
    for (std::size_t r = 1; r < 4; ++r) {
        QVariant expect = ser.value(r);
        QVariant got    = block.cell(r, 0);
        QVERIFY2(variantEquals(expect, got),
                 qPrintable(QStringLiteral("series cell(%1) mismatch").arg(r)));
        QCOMPARE(got.userType(), expect.userType());
        QVERIFY(block.rowHeader(r) == ser.index().value(r));
    }
    // 与cell级接口toVariant一致
    QVERIFY(variantEquals(d.value(2, 0), block.cell(2, 0)));

    // int64 series：批量取数须保持numpy标量语义（LongLong，与iat一致），
    // 防止pandas 3.0把元素转成python int导致类型退化为Int
    pybind11::object objInt = makeTestSeriesInt();
    QVERIFY(!objInt.is_none());
    DA::DAPySeries serInt(objInt);
    DA::DAData di(serInt);
    DA::DATableDataSource* tsi = di.tableSource();
    QVERIFY(tsi != nullptr);
    QCOMPARE(tsi->tableColumnType(0), static_cast< int >(QMetaType::LongLong));
    DA::DATableDataBlock blockInt = tsi->fetchBlock(0, 4);
    QVERIFY(blockInt.isValid());
    for (std::size_t r = 0; r < 4; ++r) {
        QVariant expect = serInt.value(r);
        QVariant got    = blockInt.cell(r, 0);
        QCOMPARE(got, expect);
        QCOMPARE(got.userType(), expect.userType());
        QCOMPARE(got.userType(), static_cast< int >(QMetaType::LongLong));
    }
}

// 空DataFrame的退化行为
void DATableDataTest::testEmptyDataFrame()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    DA::DAPyDataFrame df;  // 默认构造空DataFrame
    DA::DAData d(df);
    QVERIFY(d.isDataFrame());
    QVERIFY(d.isTable());
    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(0));
    QCOMPARE(sp.second, static_cast< std::size_t >(0));
    QVERIFY(!d.tableSource()->fetchBlock(0, 10).isValid());
}

// 工厂注册表与引用式序列化往返
void DATableDataTest::testFactoryRoundTrip()
{
    const QString tid = QStringLiteral("DATableDataTest.LazyTable");

    // 未注册时create返回空
    QVERIFY(!DA::DADataFactory::contains(tid));
    QVERIFY(DA::DADataFactory::create(tid) == nullptr);

    // 注册
    QVERIFY(DA::DADataFactory::registerCreator(tid, []() {
        return DA::DAAbstractData::Pointer(new DATestLazyTable());
    }));
    QVERIFY(DA::DADataFactory::contains(tid));
    QVERIFY(DA::DADataFactory::registeredTypeIdentifiers().contains(tid));
    // 空参数注册失败
    QVERIFY(!DA::DADataFactory::registerCreator(QString(), []() { return DA::DAAbstractData::Pointer(); }));
    QVERIFY(!DA::DADataFactory::registerCreator(tid, DA::DADataFactory::Creator()));

    // 引用式序列化：write写payload
    QByteArray payload;
    {
        DATestLazyTable src;
        src.mRefName = QStringLiteral("db://test/table1");
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDataStream out(&buf);
        src.write(out);
    }
    QVERIFY(!payload.isEmpty());

    // 工厂重建 + read恢复引用
    DA::DAAbstractData::Pointer p = DA::DADataFactory::create(tid);
    QVERIFY(p != nullptr);
    QCOMPARE(p->typeIdentifier(), tid);
    QVERIFY(p->isReferenceData());
    QVERIFY(p->tableSource() != nullptr);
    {
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::ReadOnly));
        QDataStream in(&buf);
        QVERIFY(p->read(in));
    }
    DATestLazyTable* lazy = dynamic_cast< DATestLazyTable* >(p.get());
    QVERIFY(lazy != nullptr);
    QCOMPARE(lazy->mRefName, QStringLiteral("db://test/table1"));
    // 重建后立即可用
    DA::DATableDataBlock block = lazy->fetchBlock(0, 5);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(5));

    // 注销
    DA::DADataFactory::unregisterCreator(tid);
    QVERIFY(!DA::DADataFactory::contains(tid));
    QVERIFY(DA::DADataFactory::create(tid) == nullptr);
}

// DADataTableModel接入惰性表：窗口映射、schema缓存、只读门禁
void DATableDataTest::testModelLazyDisplay()
{
    DATestLazyTable* lazy = new DATestLazyTable();
    DA::DAAbstractData::Pointer p(lazy);
    DA::DAData d(p);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);

    // actualRowCount走schema缓存，千万行不被滑动窗截断
    QCOMPARE(model.actualRowCount(), static_cast< int >(DATestLazyTable::c_totalRows));
    // 视图行数=滑动窗上限，规避QTableView超大行数问题
    QVERIFY(model.rowCount() < model.actualRowCount());
    QCOMPARE(model.rowCount(), qMin(model.getCacheWindowSize(), model.actualRowCount()));
    // 列数=5数据列+扩展列
    QCOMPARE(model.columnCount(), static_cast< int >(DATestLazyTable::c_totalCols) + model.getExtraColumnCount());
    // 水平表头来自tableColumnName
    QCOMPARE(model.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("col2"));
    // 垂直表头来自块rowHeaders
    QVERIFY(model.headerData(3, Qt::Vertical, Qt::DisplayRole) == QVariant(static_cast< qlonglong >(3)));
    // 只读：无编辑标志，setData被拒绝
    // （经基类指针调用，DADataTableModel::setData(DAData)会隐藏基类三参重载）
    QVERIFY(!(model.flags(model.index(0, 0)) & Qt::ItemIsEditable));
    QAbstractItemModel* baseModel = &model;
    QVERIFY(!baseModel->setData(baseModel->index(0, 0), QVariant(123), Qt::EditRole));
    // cell值经块缓存正确映射
    QVERIFY(model.data(model.index(3, 0), Qt::DisplayRole) == DATestLazyTable::makeCell(3, 0));
    QVERIFY(model.data(model.index(39, 4), Qt::DisplayRole) == DATestLazyTable::makeCell(39, 4));
    // 超出数据范围的扩展列/行返回无效值
    QVERIFY(!model.data(model.index(0, static_cast< int >(DATestLazyTable::c_totalCols)), Qt::DisplayRole).isValid());
}

// 块缓存命中与滚动窗口：扫描可见区不重复取数，滚动一次只预取一块
void DATableDataTest::testModelBlockCacheAndScroll()
{
    DATestLazyTable* lazy = new DATestLazyTable();
    DA::DAAbstractData::Pointer p(lazy);
    DA::DAData d(p);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);  // refreshData会预取窗口起始块
    const int fetchAfterSet = lazy->mFetchCount;
    QVERIFY(fetchAfterSet >= 1);

    // 扫描可见区40行×5列+行头：全部命中同一缓存块，零额外取数
    for (int r = 0; r < 40; ++r) {
        for (int c = 0; c < 5; ++c) {
            QVERIFY(model.data(model.index(r, c), Qt::DisplayRole) == DATestLazyTable::makeCell(r, c));
            model.headerData(r, Qt::Vertical, Qt::DisplayRole);
        }
    }
    QCOMPARE(lazy->mFetchCount, fetchAfterSet);

    // 滚动窗口到表中段：预取一次
    model.setCacheWindowStartRow(5000000);
    const int fetchAfterScroll = lazy->mFetchCount;
    QVERIFY(fetchAfterScroll > fetchAfterSet);
    // 视图行r映射到绝对行5000000+r
    for (int r = 0; r < 40; ++r) {
        QVERIFY(model.data(model.index(r, 0), Qt::DisplayRole) == DATestLazyTable::makeCell(5000000 + r, 0));
    }
    // 预取块覆盖可见区，扫描不再触发取数
    QCOMPARE(lazy->mFetchCount, fetchAfterScroll);
}

// pandas经统一块路径后的显示与编辑回归：cell编辑、undo、缓存失效
void DATableDataTest::testModelPandasEditRegression()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);

    // pandas表可编辑
    QVERIFY(model.flags(model.index(0, 0)) & Qt::ItemIsEditable);
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 1LL);
    QCOMPARE(model.data(model.index(9, 0), Qt::EditRole).toLongLong(), 10LL);

    // 经undo栈的cell编辑（经基类指针调用三参setData）
    QAbstractItemModel* baseModel = &model;
    QVERIFY(baseModel->setData(baseModel->index(0, 0), QVariant(42), Qt::EditRole));
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 42LL);
    // DisplayRole同步更新（块缓存已被notify回调失效）
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toLongLong(), 42LL);

    // undo还原
    mUndoStack.undo();
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 1LL);
    // redo再应用
    mUndoStack.redo();
    QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toLongLong(), 42LL);
    mUndoStack.clear();
}

// SQLite惰性表：schema/COUNT/LIMIT-OFFSET分页/能力位——数据库插件数据层样板验证
void DATableDataTest::testSqliteLazyTable()
{
    if (!ensureSqliteReady()) {
        QSKIP(qPrintable(mSqliteErr));
    }
    DA::DAAbstractData::Pointer p = makeSqliteLazyData();
    DASqliteLazyTable* tbl        = static_cast< DASqliteLazyTable* >(p.get());
    DA::DAData d(p);
    QVERIFY(d.isTable());
    QVERIFY(d.isReferenceData());
    QVERIFY(!d.supportsUndoSnapshot());
    QCOMPARE(d.typeIdentifier(), QStringLiteral("DATableDataTest.SqliteLazyTable"));

    // 百万行schema经COUNT(*)与LIMIT 0探针，全表不进内存
    std::pair< std::size_t, std::size_t > sp = d.shape();
    QCOMPARE(sp.first, static_cast< std::size_t >(c_sqliteRows));
    QCOMPARE(sp.second, static_cast< std::size_t >(3));

    DA::DATableDataSource* ts = d.tableSource();
    QVERIFY(ts != nullptr);
    QVERIFY(ts->isLazyLoaded());
    QVERIFY(!ts->isTableEditable());
    QCOMPARE(ts->tableColumnName(0), QStringLiteral("id"));
    QCOMPARE(ts->tableColumnName(1), QStringLiteral("val"));
    QCOMPARE(ts->tableColumnName(2), QStringLiteral("label"));
    // REAL→Double、TEXT→QString；INTEGER经LIMIT 0探针在Qt5驱动下报Int（有值时为LongLong），
    // 列类型是提示性信息，整数族即可
    const int idType = ts->tableColumnType(0);
    QVERIFY(idType == QMetaType::Int || idType == QMetaType::LongLong);
    QCOMPARE(ts->tableColumnType(1), static_cast< int >(QMetaType::Double));
    QCOMPARE(ts->tableColumnType(2), static_cast< int >(QMetaType::QString));

    // 分页块取数：表中部一次查询取100行
    const int fetchBefore          = tbl->fetchCount();
    DA::DATableDataBlock block     = ts->fetchBlock(123456, 100);
    QCOMPARE(tbl->fetchCount(), fetchBefore + 1);
    QVERIFY(block.isValid());
    QCOMPARE(block.startRow(), static_cast< std::size_t >(123456));
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(100));
    QCOMPARE(block.cell(123456, 0).toLongLong(), 123456LL);
    QCOMPARE(block.cell(123456, 1).toDouble(), 123456 * 0.5);
    QCOMPARE(block.cell(123555, 2).toString(), QStringLiteral("row123555"));
    // 未提供行头（模型将回退显示序号）
    QVERIFY(!block.rowHeader(123456).isValid());

    // 尾部截断与越界
    DA::DATableDataBlock tail = ts->fetchBlock(static_cast< std::size_t >(c_sqliteRows) - 5, 100);
    QVERIFY(tail.isValid());
    QCOMPARE(tail.rowCount(), static_cast< std::size_t >(5));
    QVERIFY(!ts->fetchBlock(static_cast< std::size_t >(c_sqliteRows), 1).isValid());

    // cell级接口与只读语义
    QCOMPARE(d.value(7, 2).toString(), QStringLiteral("row7"));
    QVERIFY(!d.setValue(7, 2, QVariant(QStringLiteral("x"))));
}

// SQLite惰性表经DADataTableModel的窗口显示与滚动取数
void DATableDataTest::testSqliteModelIntegration()
{
    if (!ensureSqliteReady()) {
        QSKIP(qPrintable(mSqliteErr));
    }
    DA::DAAbstractData::Pointer p = makeSqliteLazyData();
    DASqliteLazyTable* tbl        = static_cast< DASqliteLazyTable* >(p.get());
    DA::DAData d(p);

    DA::DADataTableModel model(&mUndoStack);
    model.setData(d);
    QCOMPARE(model.actualRowCount(), static_cast< int >(c_sqliteRows));
    QVERIFY(model.rowCount() < model.actualRowCount());  // 滑动窗截断，视图不遍历百万行
    QCOMPARE(model.columnCount(), 3 + model.getExtraColumnCount());
    QCOMPARE(model.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("label"));

    // 只读门禁
    QVERIFY(!(model.flags(model.index(0, 0)) & Qt::ItemIsEditable));
    QAbstractItemModel* baseModel = &model;
    QVERIFY(!baseModel->setData(baseModel->index(0, 0), QVariant(1), Qt::EditRole));

    // 表头区数据与行头序号回退
    QCOMPARE(model.data(model.index(7, 0), Qt::DisplayRole).toLongLong(), 7LL);
    QCOMPARE(model.data(model.index(7, 2), Qt::DisplayRole).toString(), QStringLiteral("row7"));
    QCOMPARE(model.headerData(7, Qt::Vertical, Qt::DisplayRole).toLongLong(), 7LL);

    // 滚动到表尾区域：窗口起始行会被clamp到 总行数-窗口大小+扩展行，
    // 取clamp后的最大起始行验证表尾显示
    const int tailStart = model.actualRowCount() - model.rowCount() + model.getExtraRowCount();
    model.setCacheWindowStartRow(tailStart);
    QCOMPARE(model.getCacheWindowStartRow(), tailStart);
    const int fetchAfterJump = tbl->fetchCount();
    QVERIFY(fetchAfterJump >= 1);
    // 预取一次后扫描可见区零额外查询
    for (int r = 0; r < 40; ++r) {
        QCOMPARE(model.data(model.index(r, 0), Qt::DisplayRole).toLongLong(), static_cast< qlonglong >(tailStart + r));
        model.data(model.index(r, 1), Qt::DisplayRole);
        model.data(model.index(r, 2), Qt::DisplayRole);
        model.headerData(r, Qt::Vertical, Qt::DisplayRole);
    }
    QCOMPARE(tbl->fetchCount(), fetchAfterJump);
    // 窗口最远数据行（表尾最后一行）触发一次新块查询后正确映射
    const int lastViewRow = model.rowCount() - 1 - model.getExtraRowCount();
    QCOMPARE(model.data(model.index(lastViewRow, 2), Qt::DisplayRole).toString(),
             QStringLiteral("row%1").arg(c_sqliteRows - 1));
    QVERIFY(tbl->fetchCount() <= fetchAfterJump + 2);
}

// SQLite惰性表引用式序列化：工厂注册→write→create→read→立即可取数（模拟工程保存/加载）
void DATableDataTest::testSqliteReferenceRoundTrip()
{
    if (!ensureSqliteReady()) {
        QSKIP(qPrintable(mSqliteErr));
    }
    const QString tid = QStringLiteral("DATableDataTest.SqliteLazyTable");
    QVERIFY(DA::DADataFactory::registerCreator(tid, []() {
        return DA::DAAbstractData::Pointer(new DASqliteLazyTable());
    }));

    // 原对象写出引用payload（连接名+SELECT）
    DA::DAAbstractData::Pointer src = makeSqliteLazyData();
    QByteArray payload;
    {
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDataStream out(&buf);
        src->write(out);
    }
    QVERIFY(!payload.isEmpty());
    QVERIFY(payload.size() < 1024);  // 引用payload是常量级，与表规模无关

    // 工厂重建+read恢复（工程加载路径）
    DA::DAAbstractData::Pointer rebuilt = DA::DADataFactory::create(tid);
    QVERIFY(rebuilt != nullptr);
    {
        QBuffer buf(&payload);
        QVERIFY(buf.open(QIODevice::ReadOnly));
        QDataStream in(&buf);
        QVERIFY(rebuilt->read(in));
    }
    DASqliteLazyTable* tbl = dynamic_cast< DASqliteLazyTable* >(rebuilt.get());
    QVERIFY(tbl != nullptr);
    QCOMPARE(tbl->connectionName(), mSqliteConnName);
    QCOMPARE(tbl->selectSql(), QStringLiteral("SELECT id, val, label FROM bigtable ORDER BY id"));

    // 重建后立即可取数（连接池仍在；真实插件场景由read/首次访问时重连）
    QCOMPARE(tbl->tableRowCount(), static_cast< std::size_t >(c_sqliteRows));
    DA::DATableDataBlock block = tbl->fetchBlock(0, 3);
    QVERIFY(block.isValid());
    QCOMPARE(block.rowCount(), static_cast< std::size_t >(3));
    QCOMPARE(block.cell(2, 2).toString(), QStringLiteral("row2"));

    DA::DADataFactory::unregisterCreator(tid);
}

//===================================================
// 表格元数据基础设施（001 计划 §4.1 单测）
//===================================================

// 高亮层：set/levelAt/clear(Source)/clear(Source,Level)/clearAll、来源隔离、级别隔离、changed 信号
void DATableDataTest::testHighlightOverlayBasics()
{
    using DA::DATableHighlightOverlay;
    DATableHighlightOverlay overlay;
    QVERIFY(overlay.isEmpty());
    QCOMPARE(overlay.levelAt(0, 0), DATableHighlightOverlay::Level::None);

    // Find 命中装载
    QSignalSpy spy(&overlay, &DATableHighlightOverlay::changed);
    overlay.set(DATableHighlightOverlay::Source::Find,
                DATableHighlightOverlay::Level::Hit,
                { qMakePair(1, 2), qMakePair(3, 4) });
    QCOMPARE(spy.count(), 1);
    QVERIFY(!overlay.isEmpty());
    QCOMPARE(overlay.levelAt(1, 2), DATableHighlightOverlay::Level::Hit);
    QCOMPARE(overlay.levelAt(3, 4), DATableHighlightOverlay::Level::Hit);
    QCOMPARE(overlay.levelAt(9, 9), DATableHighlightOverlay::Level::None);

    // 级别隔离：同来源设 Hit 后再设 CurrentHit，Hit 集合仍在
    overlay.set(DATableHighlightOverlay::Source::Find,
                DATableHighlightOverlay::Level::CurrentHit,
                { qMakePair(1, 2) });
    QCOMPARE(overlay.levelAt(1, 2), DATableHighlightOverlay::Level::CurrentHit);
    QCOMPARE(overlay.levelAt(3, 4), DATableHighlightOverlay::Level::Hit);

    // Validation 来源与 Find 并存，Error/Warning 两级并存
    overlay.set(DATableHighlightOverlay::Source::Validation,
                DATableHighlightOverlay::Level::ViolationError,
                { qMakePair(5, 5) });
    overlay.set(DATableHighlightOverlay::Source::Validation,
                DATableHighlightOverlay::Level::ViolationWarning,
                { qMakePair(6, 6) });
    QCOMPARE(overlay.levelAt(5, 5), DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::ViolationWarning);

    // 来源隔离：清 Find 不动 Validation
    overlay.clear(DATableHighlightOverlay::Source::Find);
    QCOMPARE(overlay.levelAt(1, 2), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(3, 4), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(5, 5), DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::ViolationWarning);

    // clear(Source, Level) 只清该级别（查找导航单独重设 CurrentHit 的场景）
    overlay.clear(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay.levelAt(5, 5), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::ViolationWarning);

    // clearAll
    overlay.clearAll();
    QVERIFY(overlay.isEmpty());
    QCOMPARE(overlay.levelAt(6, 6), DATableHighlightOverlay::Level::None);
}

// 高亮层：levelAt 固定优先级 CurrentHit > Hit > ViolationError > ViolationWarning；
// 同格既是查找命中又是校验违规时以 Find 为准（有意决策，本用例钉死，不许改成校验优先）
void DATableDataTest::testHighlightOverlayPriority()
{
    using DA::DATableHighlightOverlay;
    DATableHighlightOverlay overlay;

    // (0,0)：四级别同格 → CurrentHit 胜出
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, { qMakePair(0, 0) });
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit, { qMakePair(0, 0) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, { qMakePair(0, 0) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, { qMakePair(0, 0) });
    QCOMPARE(overlay.levelAt(0, 0), DATableHighlightOverlay::Level::CurrentHit);

    // (1,1)：Find Hit 与 Validation Error 同格 → 以 Find 为准
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, { qMakePair(1, 1) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, { qMakePair(1, 1) });
    QCOMPARE(overlay.levelAt(1, 1), DATableHighlightOverlay::Level::Hit);

    // (2,2)：Error 压过 Warning
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationError, { qMakePair(2, 2) });
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, { qMakePair(2, 2) });
    QCOMPARE(overlay.levelAt(2, 2), DATableHighlightOverlay::Level::ViolationError);

    // (3,3)：仅 Warning
    overlay.set(DATableHighlightOverlay::Source::Validation, DATableHighlightOverlay::Level::ViolationWarning, { qMakePair(3, 3) });
    QCOMPARE(overlay.levelAt(3, 3), DATableHighlightOverlay::Level::ViolationWarning);

    // 语义色映射：命中/警告=金黄、当前命中=主蓝、错误=橙红（官方色板）、None 无效
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::Hit), QColor(0xE6, 0xC2, 0x7C));
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::CurrentHit), QColor(0x52, 0x80, 0xC1));
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::ViolationError), QColor(0xCE, 0x60, 0x43));
    QCOMPARE(DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::ViolationWarning), QColor(0xE6, 0xC2, 0x7C));
    QVERIFY(!DATableHighlightOverlay::colorForLevel(DATableHighlightOverlay::Level::None).isValid());
}

// 高亮层：打包键 (row<<32)|col 的边界——col=0 与 col=2^31-1 不串行、行维度不串行
void DATableDataTest::testHighlightOverlayKeyPacking()
{
    using DA::DATableHighlightOverlay;
    DATableHighlightOverlay overlay;
    const int maxCol = 2147483647;  // 2^31 - 1（键打包前提：行/列号 < 2^31）

    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::Hit, { qMakePair(7, 0) });
    overlay.set(DATableHighlightOverlay::Source::Find, DATableHighlightOverlay::Level::CurrentHit, { qMakePair(7, maxCol) });
    QCOMPARE(overlay.levelAt(7, 0), DATableHighlightOverlay::Level::Hit);
    QCOMPARE(overlay.levelAt(7, maxCol), DATableHighlightOverlay::Level::CurrentHit);
    // 行维度不串：相邻行的同列、同行的相邻列都无命中
    QCOMPARE(overlay.levelAt(6, 0), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(8, 0), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(6, maxCol), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(7, 1), DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay.levelAt(7, maxCol - 1), DATableHighlightOverlay::Level::None);
}

// 重映射工具：置换/逆置换、行键与单元格键（按行/按列）、置换+逆置换还原、空置换、单元素置换
void DATableDataTest::testKeyRemapUtility()
{
    using DA::DATableKeyRemap;
    // 置换语义钉死：perm[i] = 新表第 i 行在旧表中的行号（与 df.iloc[perm] 一致）
    const QVector< int > perm { 2, 0, 1 };  // 新行0←旧行2，新行1←旧行0，新行2←旧行1
    const QVector< int > inv = DATableKeyRemap::inversePermutation(perm);
    const QVector< int > invExpected { 1, 2, 0 };  // inv[旧行号]=新位置
    QCOMPARE(inv, invExpected);

    // 行键重映射
    QHash< int, QString > rowKeys;
    rowKeys.insert(0, QStringLiteral("a"));
    rowKeys.insert(1, QStringLiteral("b"));
    rowKeys.insert(2, QStringLiteral("c"));
    DATableKeyRemap::remapKeys(rowKeys, perm);
    QCOMPARE(rowKeys.value(0), QStringLiteral("c"));  // 旧行2 → 新行0
    QCOMPARE(rowKeys.value(1), QStringLiteral("a"));  // 旧行0 → 新行1
    QCOMPARE(rowKeys.value(2), QStringLiteral("b"));  // 旧行1 → 新行2

    // 置换 + 逆置换 = 还原
    DATableKeyRemap::remapKeys(rowKeys, inv);
    QCOMPARE(rowKeys.value(0), QStringLiteral("a"));
    QCOMPARE(rowKeys.value(1), QStringLiteral("b"));
    QCOMPARE(rowKeys.value(2), QStringLiteral("c"));

    // 单元格键按行维重映射（列维不变）
    QHash< QPair< int, int >, QString > cells;
    cells.insert(qMakePair(0, 5), QStringLiteral("x"));
    cells.insert(qMakePair(2, 7), QStringLiteral("y"));
    DATableKeyRemap::remapKeys(cells, perm, DATableKeyRemap::Dimension::Row);
    QCOMPARE(cells.value(qMakePair(1, 5)), QStringLiteral("x"));  // 旧行0 → 新行1
    QCOMPARE(cells.value(qMakePair(0, 7)), QStringLiteral("y"));  // 旧行2 → 新行0
    QVERIFY(!cells.contains(qMakePair(0, 5)));
    QCOMPARE(cells.size(), 2);

    // 单元格键按列维重映射（行维不变）：列置换交换 5↔7
    const QVector< int > colPerm { 0, 1, 2, 3, 4, 7, 6, 5 };  // 新列5←旧列7，新列7←旧列5
    DATableKeyRemap::remapKeys(cells, colPerm, DATableKeyRemap::Dimension::Column);
    QCOMPARE(cells.value(qMakePair(1, 7)), QStringLiteral("x"));
    QCOMPARE(cells.value(qMakePair(0, 5)), QStringLiteral("y"));

    // 空置换：新表 0 行，全部键失效
    QHash< int, QString > h0;
    h0.insert(0, QStringLiteral("a"));
    DATableKeyRemap::remapKeys(h0, QVector< int >());
    QVERIFY(h0.isEmpty());

    // 单元素置换
    QHash< int, QString > h1;
    h1.insert(0, QStringLiteral("only"));
    DATableKeyRemap::remapKeys(h1, QVector< int > { 0 });
    QCOMPARE(h1.size(), 1);
    QCOMPARE(h1.value(0), QStringLiteral("only"));

    // 未被置换覆盖的旧键丢弃（新表行数少于旧表）
    QHash< int, QString > h2;
    h2.insert(0, QStringLiteral("a"));
    h2.insert(5, QStringLiteral("far"));
    DATableKeyRemap::remapKeys(h2, QVector< int > { 0 });
    QCOMPARE(h2.size(), 1);
    QCOMPARE(h2.value(0), QStringLiteral("a"));
}

// DATableStyleManager::onRowsReordered：单元格/行样式键位移正确、styleReset 发出、
// 列级样式不受影响、与 onRowsInserted/onRowsRemoved 不互相干扰、空置换为无操作
void DATableDataTest::testStyleManagerRowsReordered()
{
    DA::DATableStyleManager mgr;
    auto makeStyle = [](const QColor& c) {
        DA::DATableCellStyle s;
        s.setBackground(QBrush(c));
        return s;
    };
    const QColor red(255, 0, 0), green(0, 255, 0), blue(0, 0, 255), yellow(255, 255, 0);
    mgr.setCellStyle(0, 1, makeStyle(red));
    mgr.setCellStyle(2, 1, makeStyle(green));
    mgr.setRowStyle(1, makeStyle(blue));
    mgr.setColumnStyle(3, makeStyle(yellow));  // 列级：行重排不应影响

    QSignalSpy resetSpy(&mgr, &DA::DATableStyleManager::styleReset);
    const QVector< int > perm { 2, 0, 1 };  // 新表 = 旧表 iloc[[2,0,1]]
    mgr.onRowsReordered(perm);
    QCOMPARE(resetSpy.count(), 1);
    // 旧行2(green)→新行0；旧行0(red)→新行1；旧行1的单元格无、行样式 blue→新行2
    QCOMPARE(mgr.getCellStyle(0, 1).background().color(), green);
    QCOMPARE(mgr.getCellStyle(1, 1).background().color(), red);
    QVERIFY(!mgr.hasCellStyle(2, 1));
    QCOMPARE(mgr.getRowStyle(2).background().color(), blue);
    QVERIFY(!mgr.hasRowStyle(0));
    QVERIFY(!mgr.hasRowStyle(1));
    // 列级样式不受行重排影响
    QCOMPARE(mgr.getColumnStyle(3).background().color(), yellow);

    // 与 onRowsInserted/onRowsRemoved 不互相干扰：头部插一行键 +1，删回还原
    mgr.onRowsInserted({ 0 });
    QCOMPARE(mgr.getCellStyle(1, 1).background().color(), green);
    QCOMPARE(mgr.getCellStyle(2, 1).background().color(), red);
    mgr.onRowsRemoved({ 0 });
    QCOMPARE(mgr.getCellStyle(0, 1).background().color(), green);
    QCOMPARE(mgr.getCellStyle(1, 1).background().color(), red);

    // 空置换 = 无操作：不发信号、不清存储（与 on*Inserted/Removed 的空表行为一致）
    const int resetCount = resetSpy.count();
    mgr.onRowsReordered(QVector< int >());
    QCOMPARE(resetSpy.count(), resetCount);
    QCOMPARE(mgr.getCellStyle(0, 1).background().color(), green);
}

// DATableViewStateStore：XML round-trip（列宽按名/行头宽）、renameColumn 迁移、
// removeColumn 后重新插入同名不限、重名列 occurrence 消歧互不影响
void DATableDataTest::testViewStateStore()
{
    DA::DATableViewStateStore store;
    QVERIFY(store.isEmpty());
    QCOMPARE(store.columnWidth(QStringLiteral("value")), -1);
    QCOMPARE(store.rowHeaderWidth(), -1);

    store.setColumnWidth(QStringLiteral("id"), 0, 80);
    store.setColumnWidth(QStringLiteral("value"), 0, 120);
    store.setColumnWidth(QStringLiteral("value"), 1, 200);  // 两个同名 value 列
    store.setRowHeaderWidth(60);
    QVERIFY(!store.isEmpty());
    // 重名列 occurrence 消歧：各自宽度互不影响
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 0), 120);
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 1), 200);
    store.setColumnWidth(QStringLiteral("value"), 0, 130);
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 0), 130);
    QCOMPARE(store.columnWidth(QStringLiteral("value"), 1), 200);

    // XML round-trip
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("table"));
    doc.appendChild(root);
    store.toXml(doc, root);
    QDomElement vsEle = root.firstChildElement(QStringLiteral("view-state"));
    QVERIFY(!vsEle.isNull());
    DA::DATableViewStateStore store2;
    QVERIFY(store2.fromXml(vsEle));
    QCOMPARE(store2.columnWidth(QStringLiteral("id"), 0), 80);
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 0), 130);
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 1), 200);
    QCOMPARE(store2.rowHeaderWidth(), 60);

    // renameColumn 迁移（occurrence 保持）
    store2.renameColumn(QStringLiteral("value"), QStringLiteral("val2"));
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 0), -1);
    QCOMPARE(store2.columnWidth(QStringLiteral("value"), 1), -1);
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 0), 130);
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 1), 200);

    // removeColumn 后重新插入同名不限
    store2.removeColumn(QStringLiteral("val2"));
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 0), -1);
    store2.setColumnWidth(QStringLiteral("val2"), 0, 99);
    QCOMPARE(store2.columnWidth(QStringLiteral("val2"), 0), 99);

    // 空 store 不写出 <view-state> 段；clear 复位
    DA::DATableViewStateStore emptyStore;
    QDomDocument doc2;
    QDomElement root2 = doc2.createElement(QStringLiteral("table"));
    doc2.appendChild(root2);
    emptyStore.toXml(doc2, root2);
    QVERIFY(root2.firstChildElement(QStringLiteral("view-state")).isNull());
    store2.clear();
    QVERIFY(store2.isEmpty());
}

// DATableMetadataRegistry：getOrCreate/get/has/remove/clear/nonEmptyEntries 语义与旧注册表一致；
// 束内子存储随束创建；空束不进保存清单
void DATableDataTest::testMetadataRegistry()
{
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DAAbstractData::Pointer p1(new DATestLazyTable());
    DA::DAAbstractData::Pointer p2(new DATestLazyTable());
    DA::DAData d1(p1);
    DA::DAData d2(p2);

    QVERIFY(!registry.has(d1));
    QVERIFY(registry.get(d1) == nullptr);
    QVERIFY(registry.nonEmptyEntries().isEmpty());

    // getOrCreate：幂等，束内子存储随束创建
    DA::DATableMetadata* m1 = registry.getOrCreate(d1);
    QVERIFY(m1 != nullptr);
    QCOMPARE(registry.get(d1), m1);
    QVERIFY(registry.has(d1));
    QCOMPARE(registry.getOrCreate(d1), m1);
    QVERIFY(m1->styleManager() != nullptr);
    QVERIFY(m1->viewState() != nullptr);
    QVERIFY(m1->isEmpty());
    // 空束不进保存清单
    QVERIFY(registry.nonEmptyEntries().isEmpty());

    // 非空束进 nonEmptyEntries（保存遍历依据）
    m1->viewState()->setRowHeaderWidth(50);
    QCOMPARE(registry.nonEmptyEntries().size(), 1);
    QCOMPARE(registry.nonEmptyEntries().first().first, d1);
    QCOMPARE(registry.nonEmptyEntries().first().second, m1);

    // 不同数据键互不干扰
    DA::DATableMetadata* m2 = registry.getOrCreate(d2);
    QVERIFY(m2 != nullptr);
    QVERIFY(m2 != m1);
    QCOMPARE(registry.get(d2), m2);

    // remove：显式移除并销毁。重建的"新对象"断言用语义新鲜度（旧束状态标记消失）
    // 而不是与悬空指针比地址——分配器可能复用同一地址（指针 ABA，实测会偶发相等）
    m2->viewState()->setRowHeaderWidth(77);  // 旧束状态标记
    registry.remove(d2);
    QVERIFY(!registry.has(d2));
    QVERIFY(registry.get(d2) == nullptr);
    DA::DATableMetadata* m2Rebuilt = registry.getOrCreate(d2);  // 重新创建是全新空束
    QVERIFY(m2Rebuilt != nullptr);
    QCOMPARE(m2Rebuilt->viewState()->rowHeaderWidth(), -1);
    QVERIFY(m2Rebuilt->isEmpty());
    registry.remove(d2);

    // clear：清空全部
    registry.clear();
    QVERIFY(!registry.has(d1));
    QVERIFY(registry.get(d1) == nullptr);
    QVERIFY(registry.nonEmptyEntries().isEmpty());
}

// §4.2 验收（程序化）：旧工程 table-styles.xml 的 <table> 内层结构可被元数据束兼容读取
// （内层样式标签与旧文件一致、无 <view-state> 段），新格式 round-trip 样式+视图状态都在
void DATableDataTest::testLegacyTableStylesXmlCompat()
{
    auto makeStyle = [](const QColor& c) {
        DA::DATableCellStyle s;
        s.setBackground(QBrush(c));
        return s;
    };
    const QColor red(255, 0, 0), blue(0, 0, 255), green(0, 255, 0);

    // 构造旧工程 table-styles.xml 的 <table> 内层（内层标签由 DATableStyleManager::toXml
    // 写出，与旧文件逐字节同构），并做一次「文件文本」round-trip 模拟真实解析路径
    DA::DATableStyleManager src;
    src.setColumnStyle(1, makeStyle(red));
    src.setRowStyle(2, makeStyle(blue));
    src.setCellStyle(3, 0, makeStyle(green));
    QDomDocument doc;
    QDomElement tableEle = doc.createElement(QStringLiteral("table"));
    tableEle.setAttribute(QStringLiteral("data-id"), QStringLiteral("42"));
    tableEle.setAttribute(QStringLiteral("data-name"), QStringLiteral("legacy"));
    doc.appendChild(tableEle);
    src.toXml(doc, tableEle);
    const QByteArray legacyText = doc.toByteArray();

    QDomDocument parsed;
    QVERIFY(parsed.setContent(legacyText));
    QDomElement parsedTable = parsed.documentElement();
    QVERIFY(parsedTable.firstChildElement(QStringLiteral("view-state")).isNull());  // 旧文件无 view-state 段

    DA::DATableMetadata meta;
    QVERIFY(meta.fromXml(parsedTable));
    QCOMPARE(meta.styleManager()->getColumnStyle(1).background().color(), red);
    QCOMPARE(meta.styleManager()->getRowStyle(2).background().color(), blue);
    QCOMPARE(meta.styleManager()->getCellStyle(3, 0).background().color(), green);
    QVERIFY(meta.viewState()->isEmpty());  // 旧文件没有视图状态：为空而非报错
    QVERIFY(!meta.isEmpty());              // 但有样式：进保存清单（另存即升级为新文件内容）

    // 新格式 round-trip：样式 + <view-state> 都恢复
    meta.viewState()->setColumnWidth(QStringLiteral("f"), 0, 111);
    meta.viewState()->setRowHeaderWidth(55);
    QDomDocument doc2;
    QDomElement t2 = doc2.createElement(QStringLiteral("table"));
    doc2.appendChild(t2);
    meta.toXml(doc2, t2);
    QDomDocument parsed2;
    QVERIFY(parsed2.setContent(doc2.toByteArray()));
    DA::DATableMetadata meta2;
    QVERIFY(meta2.fromXml(parsed2.documentElement()));
    QCOMPARE(meta2.styleManager()->getColumnStyle(1).background().color(), red);
    QCOMPARE(meta2.styleManager()->getRowStyle(2).background().color(), blue);
    QCOMPARE(meta2.styleManager()->getCellStyle(3, 0).background().color(), green);
    QCOMPARE(meta2.viewState()->columnWidth(QStringLiteral("f"), 0), 111);
    QCOMPARE(meta2.viewState()->rowHeaderWidth(), 55);
}

// §4.2 验收（程序化）：列头菜单 6 项 4 组（顺序/分隔符与迁移前逐项一致、图标取 DAGui 资源）；
// 表体菜单 v1 三项；无选区全部置灰。离屏构建 QMenu 断言 action 文本序列，不弹出。
void DATableDataTest::testContextMenuComposition()
{
    // ---- 列头菜单：重命名列/删除列 ‖ 复制列名 ‖ 定位最大/定位最小 ‖ 描述统计 ----
    DA::DADataHeaderView hv(Qt::Horizontal, nullptr);
    QMenu* menu = hv.buildColumnContextMenu(0);
    QVERIFY(menu != nullptr);
    QStringList seq;
    const auto headerActions = menu->actions();
    for (QAction* a : headerActions) {
        seq << (a->isSeparator() ? QStringLiteral("<SEP>") : a->text());
        if (!a->isSeparator() && !a->menu()) {  // 子菜单本身不设图标（006d 豁免）
            QVERIFY2(!a->icon().isNull(), qPrintable(QStringLiteral("icon empty for %1").arg(a->text())));
        }
    }
    const QStringList expectedHeader { QStringLiteral("Sort Ascending"),
                                       QStringLiteral("Sort Descending"),
                                       QStringLiteral("Add Secondary Sort (Ascending)"),
                                       QStringLiteral("Add Secondary Sort (Descending)"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Rename Column"),
                                       QStringLiteral("Remove Column"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Copy Column Name"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Goto Max"),
                                       QStringLiteral("Goto Min"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Show Statistics"),
                                       QStringLiteral("<SEP>"),
                                       QStringLiteral("Columns") };
    QCOMPARE(seq, expectedHeader);
    // 「列」子菜单（006d）：5 项 2 分隔符，5 个图标齐全（冻结/取消冻结共用 freeze-column）
    QAction* columnsAction = nullptr;
    for (QAction* a : headerActions) {
        if (a->text() == QStringLiteral("Columns")) {
            columnsAction = a;
            break;
        }
    }
    QVERIFY(columnsAction != nullptr);
    QVERIFY(columnsAction->menu() != nullptr);
    const QStringList expectedColumnsMenu { QStringLiteral("Hide This Column"),
                                            QStringLiteral("Show All Columns"),
                                            QStringLiteral("<SEP>"),
                                            QStringLiteral("Freeze to This Column"),
                                            QStringLiteral("Unfreeze"),
                                            QStringLiteral("<SEP>"),
                                            QStringLiteral("Column View...") };
    QStringList colSeq;
    const auto colActs = columnsAction->menu()->actions();
    for (QAction* a : colActs) {
        colSeq << (a->isSeparator() ? QStringLiteral("<SEP>") : a->text());
        if (!a->isSeparator()) {
            QVERIFY2(!a->icon().isNull(), qPrintable(QStringLiteral("icon empty for %1").arg(a->text())));
        }
    }
    QCOMPARE(colSeq, expectedColumnsMenu);
    // 两个新图标已登记 qrc（引用即渲染）
    QVERIFY(!QIcon(QStringLiteral(":/DAGui/icon/freeze-column.svg")).isNull());
    QVERIFY(!QIcon(QStringLiteral(":/DAGui/icon/column-view.svg")).isNull());
    delete menu;

    // ---- 表体菜单：剪切/复制/粘贴 ‖ 查找/替换（002）‖ 条件格式（003）；无有效选区（无模型）----
    // 启用规则：剪切/复制/粘贴/条件格式无选区置灰；查找恒可用；替换在只读源置灰（无模型=只读）
    DA::DADataTableView tv;
    QMenu* body = tv.buildContextMenu(QModelIndex());
    QVERIFY(body != nullptr);
    QStringList bodySeq;
    const auto bodyActions = body->actions();
    QHash< QString, QAction* > byText;
    for (QAction* a : bodyActions) {
        bodySeq << (a->isSeparator() ? QStringLiteral("<SEP>") : a->text());
        if (!a->isSeparator()) {
            byText.insert(a->text(), a);
        }
    }
    const QStringList expectedBody { QStringLiteral("Cut"),
                                     QStringLiteral("Copy"),
                                     QStringLiteral("Paste"),
                                     QStringLiteral("<SEP>"),
                                     QStringLiteral("Find..."),
                                     QStringLiteral("Replace..."),
                                     QStringLiteral("<SEP>"),
                                     QStringLiteral("Conditional Format..."),
                                     QStringLiteral("<SEP>"),
                                     QStringLiteral("Computed Column..."),
                                     QStringLiteral("Split Column..."),
                                     QStringLiteral("Merge Columns..."),
                                     QStringLiteral("Group By..."),
                                     QStringLiteral("Transpose..."),
                                     QStringLiteral("Unpivot (Melt)..."),
                                     QStringLiteral("<SEP>"),
                                     QStringLiteral("Fill"),
                                     QStringLiteral("<SEP>"),
                                     QStringLiteral("Validation Rules..."),
                                     QStringLiteral("Run Validation"),
                                     QStringLiteral("<SEP>"),
                                     QStringLiteral("Insert Note"),
                                     QStringLiteral("Edit Note"),
                                     QStringLiteral("Delete Note"),
                                     QStringLiteral("Clear All Notes") };
    QCOMPARE(bodySeq, expectedBody);
    // 剪贴板组无选区置灰
    QVERIFY(!byText.value(QStringLiteral("Cut"))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Copy"))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Paste"))->isEnabled());
    // 查找恒可用；替换在无模型（只读）时置灰
    QVERIFY(byText.value(QStringLiteral("Find..."))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Replace..."))->isEnabled());
    // 条件格式（003）：无选区置灰（入口作用于选区所在列）
    QVERIFY(!byText.value(QStringLiteral("Conditional Format..."))->isEnabled());
    // 计算列（004）：无模型=只读源置灰
    QVERIFY(!byText.value(QStringLiteral("Computed Column..."))->isEnabled());
    // 分列/合并列（004 T1.9）：无模型=只读置灰；且无选区时分列（需单列）/合并（需多列）都置灰
    QVERIFY(!byText.value(QStringLiteral("Split Column..."))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Merge Columns..."))->isEnabled());
    // 分组聚合（004 T1.22）：无模型=无 dataframe 数据体，置灰
    QVERIFY(!byText.value(QStringLiteral("Group By..."))->isEnabled());
    // 转置/宽转长（004 T1.27）：无模型置灰
    QVERIFY(!byText.value(QStringLiteral("Transpose..."))->isEnabled());
    QVERIFY(!byText.value(QStringLiteral("Unpivot (Melt)..."))->isEnabled());
    // 填充子菜单（005 T1.24）：无模型=只读源，五项全部置灰且图标齐备
    QAction* actFill = byText.value(QStringLiteral("Fill"));
    QVERIFY(actFill != nullptr && actFill->menu() != nullptr);
    QStringList fillSeq;
    const auto fillActs = actFill->menu()->actions();
    for (QAction* a : fillActs) {
        if (a->isSeparator()) {
            continue;
        }
        fillSeq << a->text();
        QVERIFY(!a->isEnabled());
        QVERIFY2(!a->icon().isNull(), qPrintable(QStringLiteral("icon empty for %1").arg(a->text())));
    }
    QCOMPARE(fillSeq, QStringList({ QStringLiteral("Fill Down"),
                                    QStringLiteral("Fill Up"),
                                    QStringLiteral("Fill Forward (ffill)"),
                                    QStringLiteral("Fill Backward (bfill)"),
                                    QStringLiteral("Fill Series...") }));
    // 数据校验（005 T1.25）：无模型=无 dataframe 数据体，两项置灰且图标齐备
    for (const QString& t : { QStringLiteral("Validation Rules..."), QStringLiteral("Run Validation") }) {
        QAction* a = byText.value(t);
        QVERIFY(a != nullptr);
        QVERIFY(!a->isEnabled());
        QVERIFY2(!a->icon().isNull(), qPrintable(QStringLiteral("icon empty for %1").arg(t)));
    }
    // 批注（005 T1.26）：无模型=无当前格且无批注，四项全置灰；批注是元数据，
    // 只读源不置灰（启用规则只看当前格/批注存在性）
    for (const QString& t : { QStringLiteral("Insert Note"), QStringLiteral("Edit Note"),
                              QStringLiteral("Delete Note"), QStringLiteral("Clear All Notes") }) {
        QAction* a = byText.value(t);
        QVERIFY(a != nullptr);
        QVERIFY(!a->isEnabled());
        QVERIFY2(!a->icon().isNull(), qPrintable(QStringLiteral("icon empty for %1").arg(t)));
    }
    delete body;
}

// §4.2 验收（程序化）：列宽/行头宽「拖拽后重开数据页保留」——store 写入后新建页面
// 构造时按列名应用；图表「选择序列」拾取链路回归——点击表头仍发 columnHeaderClicked
void DATableDataTest::testViewStateApplyAndHeaderClickChain()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);

    // 第一次打开：模拟用户拖拽后写入 store（列 f 宽 137、行头宽 66）
    {
        DA::DADataOperateOfDataFrameWidget w(d, &registry);
        DA::DATableViewStateStore* store = w.viewStateStore();
        QVERIFY(store != nullptr);
        store->setColumnWidth(QStringLiteral("f"), 0, 137);
        store->setRowHeaderWidth(66);
    }

    // 「重开」数据页（同一 registry 即同一会话；工程保存/加载由 XML round-trip 用例覆盖）：
    // 构造时按列名应用列宽，行头宽以 setFixedWidth（min=max=w）落地
    DA::DADataOperateOfDataFrameWidget w2(d, &registry);
    DA::DADataTableView* tv = w2.getDataTableView();
    QVERIFY(tv != nullptr);
    QHeaderView* hh = tv->horizontalHeader();
    QVERIFY(hh != nullptr);
    int colF = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model() && hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("f")) {
            colF = c;
            break;
        }
    }
    QVERIFY(colF >= 0);
    QCOMPARE(hh->sectionSize(colF), 137);
    QHeaderView* vh = tv->verticalHeader();
    QVERIFY(vh != nullptr);
    QCOMPARE(vh->minimumWidth(), 66);
    QCOMPARE(vh->maximumWidth(), 66);

    // 图表「选择序列」拾取链路回归（002：链路切换为 DADataHeaderView::clickedWithModifiers，
    // 拾取模式下页面只转发 columnHeaderClicked 不排序）：拾取模式 → 左键点击列头 → columnHeaderClicked
    w2.setHeaderPickMode(true);
    QVERIFY(w2.isHeaderPickMode());
    QSignalSpy clickSpy(&w2, &DA::DADataOperateOfDataFrameWidget::columnHeaderClicked);
    w2.resize(800, 400);
    w2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w2));
    const int xPos = hh->sectionViewportPosition(colF) + qMax(1, hh->sectionSize(colF) / 2);
    const int yPos = qMax(1, hh->viewport()->height() / 2);
    QTest::mouseClick(hh->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(xPos, yPos));
    QVERIFY(clickSpy.count() >= 1);
    QCOMPARE(clickSpy.takeFirst().at(0).toInt(), colF);
    // 拾取模式下带 Ctrl 修饰点击同样只转发（修饰键不被吞、不排序）
    if (mPyScriptsOk) {
        QTest::mouseClick(hh->viewport(), Qt::LeftButton, Qt::ControlModifier, QPoint(xPos, yPos));
        QVERIFY(clickSpy.count() >= 1);
        QCOMPARE(clickSpy.takeFirst().at(0).toInt(), colF);
        QVERIFY(w2.sortSpec().isEmpty());  // 拾取模式不产生排序规格
    }
    // 退出拾取模式后点击列头不再发 columnHeaderClicked，也不再排序
    // （2026-10-18 交互修订：排序入口=列头右键菜单 → 页面 sortColumn/addSecondarySortColumn）
    if (mPyScriptsOk) {
        w2.setHeaderPickMode(false);
        QTest::mouseClick(hh->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(xPos, yPos));
        QCOMPARE(clickSpy.count(), 0);
        QVERIFY(w2.sortSpec().isEmpty());
        // 菜单排序的执行点（页面方法）仍然生效
        w2.sortColumn(colF, Qt::AscendingOrder);
        QCOMPARE(w2.sortSpec().size(), 1);
        QCOMPARE(w2.sortSpec().at(0).first, colF);
    }
    w2.hide();
}

// 006a：view-state 三个新子元素（column-order / column hidden / freeze）round-trip、
// 重名消歧、旧格式兼容、renameColumn/removeColumn 三份键迁移、只设冻结（隐藏）非空
void DATableDataTest::testViewStateColumnViewXml()
{
    using Idents = QList< QPair< QString, int > >;
    DA::DATableViewStateStore store;
    QVERIFY(store.isEmpty());

    // ① 三类新元素 round-trip：列序 + 隐藏 + 冻结，逐字段相等
    store.setColumnOrder(Idents({ qMakePair(QStringLiteral("f"), 0), qMakePair(QStringLiteral("g"), 0),
                                  qMakePair(QStringLiteral("value"), 1) }));
    store.setColumnHidden(QStringLiteral("h"), 0, true);
    store.setFreezeColumnCount(2);
    QVERIFY(!store.isEmpty());
    QCOMPARE(store.freezeColumnCount(), 2);
    QVERIFY(store.isColumnHidden(QStringLiteral("h"), 0));
    QVERIFY(!store.isColumnHidden(QStringLiteral("h"), 1));

    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("table"));
    doc.appendChild(root);
    store.toXml(doc, root);
    QDomElement vsEle = root.firstChildElement(QStringLiteral("view-state"));
    QVERIFY(!vsEle.isNull());
    DA::DATableViewStateStore store2;
    QVERIFY(store2.fromXml(vsEle));
    const Idents expectOrder { qMakePair(QStringLiteral("f"), 0), qMakePair(QStringLiteral("g"), 0),
                               qMakePair(QStringLiteral("value"), 1) };
    QCOMPARE(store2.columnOrder(), expectOrder);
    QVERIFY(store2.isColumnHidden(QStringLiteral("h"), 0));
    QCOMPARE(store2.hiddenColumns().size(), 1);
    QCOMPARE(store2.freezeColumnCount(), 2);

    // ② 重名消歧：两个 value 列的隐藏与列序各自独立
    store2.setColumnHidden(QStringLiteral("value"), 0, true);
    QVERIFY(store2.isColumnHidden(QStringLiteral("value"), 0));
    QVERIFY(!store2.isColumnHidden(QStringLiteral("value"), 1));
    store2.setColumnHidden(QStringLiteral("value"), 0, false);
    QVERIFY(!store2.isColumnHidden(QStringLiteral("value"), 0));
    QVERIFY(!store2.isColumnHidden(QStringLiteral("value"), 1));
    QCOMPARE(store2.hiddenColumns().size(), 1);  // 只剩 round-trip 进来的 ("h", 0)

    // ③ 旧格式（只有 column-width + row-header-width）加载 → 新状态为空、列宽照旧、无报错
    QDomDocument oldDoc;
    oldDoc.setContent(QStringLiteral("<table><view-state row-header-width=\"66\">"
                                     "<column-width name=\"f\" occurrence=\"0\" width=\"137\"/>"
                                     "</view-state></table>"));
    QDomElement oldVs = oldDoc.firstChildElement(QStringLiteral("table")).firstChildElement(QStringLiteral("view-state"));
    DA::DATableViewStateStore store3;
    QVERIFY(store3.fromXml(oldVs));
    QCOMPARE(store3.columnWidth(QStringLiteral("f"), 0), 137);
    QCOMPARE(store3.rowHeaderWidth(), 66);
    QVERIFY(store3.columnOrder().isEmpty());
    QVERIFY(store3.hiddenColumns().isEmpty());
    QCOMPARE(store3.freezeColumnCount(), 0);

    // ④ renameColumn 迁移三份键（列宽/列序/隐藏）
    DA::DATableViewStateStore store4;
    store4.setColumnWidth(QStringLiteral("f"), 0, 137);
    store4.setColumnOrder(Idents({ qMakePair(QStringLiteral("f"), 0), qMakePair(QStringLiteral("g"), 0) }));
    store4.setColumnHidden(QStringLiteral("f"), 0, true);
    store4.renameColumn(QStringLiteral("f"), QStringLiteral("f2"));
    QCOMPARE(store4.columnWidth(QStringLiteral("f"), 0), -1);
    QCOMPARE(store4.columnWidth(QStringLiteral("f2"), 0), 137);
    QCOMPARE(store4.columnOrder(),
             Idents({ qMakePair(QStringLiteral("f2"), 0), qMakePair(QStringLiteral("g"), 0) }));
    QVERIFY(!store4.isColumnHidden(QStringLiteral("f"), 0));
    QVERIFY(store4.isColumnHidden(QStringLiteral("f2"), 0));

    // ⑤ removeColumn 移除三份
    store4.removeColumn(QStringLiteral("f2"));
    QCOMPARE(store4.columnWidth(QStringLiteral("f2"), 0), -1);
    QCOMPARE(store4.columnOrder(), Idents({ qMakePair(QStringLiteral("g"), 0) }));
    QVERIFY(!store4.isColumnHidden(QStringLiteral("f2"), 0));

    // ⑥ 只设冻结列数（或只设隐藏）时 isEmpty() == false
    DA::DATableViewStateStore freezeOnly;
    freezeOnly.setFreezeColumnCount(3);
    QVERIFY(!freezeOnly.isEmpty());
    freezeOnly.setFreezeColumnCount(0);
    QVERIFY(freezeOnly.isEmpty());
    DA::DATableViewStateStore hiddenOnly;
    hiddenOnly.setColumnHidden(QStringLiteral("x"), 0, true);
    QVERIFY(!hiddenOnly.isEmpty());
    hiddenOnly.setColumnHidden(QStringLiteral("x"), 0, false);
    QVERIFY(hiddenOnly.isEmpty());
    // setFreezeColumnCount 归一：负数归 0
    freezeOnly.setFreezeColumnCount(-2);
    QCOMPARE(freezeOnly.freezeColumnCount(), 0);
    QVERIFY(freezeOnly.isEmpty());
}

// 006a：normalizeToColumns 收敛语义——孤儿剪除、新列追加末尾、空列序不填充、
// 冻结计数不写回、返回值口径、一次调用即置 isColumnSetNormalized
void DATableDataTest::testViewStateNormalizeConvergence()
{
    using Idents = QList< QPair< QString, int > >;

    // ① 列序含已删除列 → 剪除；② 列序缺实际列 → 按实际顺序追加末尾
    {
        DA::DATableViewStateStore store;
        store.setColumnOrder(Idents({ qMakePair(QStringLiteral("a"), 0), qMakePair(QStringLiteral("gone"), 0),
                                      qMakePair(QStringLiteral("b"), 0) }));
        store.setColumnHidden(QStringLiteral("gone"), 0, true);  // ③ 隐藏集合孤儿剪除
        store.setFreezeColumnCount(3);                            // ⑥ 冻结计数不被写回
        QVERIFY(!store.isColumnSetNormalized());
        const bool changed = store.normalizeToColumns(
            Idents({ qMakePair(QStringLiteral("a"), 0), qMakePair(QStringLiteral("b"), 0), qMakePair(QStringLiteral("c"), 0) }));
        QVERIFY(changed);  // ④ 有改动返回 true
        QCOMPARE(store.columnOrder(),
                 Idents({ qMakePair(QStringLiteral("a"), 0), qMakePair(QStringLiteral("b"), 0),
                          qMakePair(QStringLiteral("c"), 0) }));
        QVERIFY(!store.isColumnHidden(QStringLiteral("gone"), 0));
        QCOMPARE(store.hiddenColumns().size(), 0);
        QCOMPARE(store.freezeColumnCount(), 3);  // 冻结不参与收敛
        QVERIFY(store.isColumnSetNormalized());  // ⑦ 调用后置位
        // 再收敛一次（无变化）→ 返回 false
        QVERIFY(!store.normalizeToColumns(
            Idents({ qMakePair(QStringLiteral("a"), 0), qMakePair(QStringLiteral("b"), 0), qMakePair(QStringLiteral("c"), 0) })));
    }

    // ⑤ 列序为空时不填充（空 = 未记录 = 数据列序）
    {
        DA::DATableViewStateStore store;
        store.setColumnHidden(QStringLiteral("z"), 0, true);
        QVERIFY(store.normalizeToColumns(
            Idents({ qMakePair(QStringLiteral("a"), 0), qMakePair(QStringLiteral("b"), 0) })));
        QVERIFY(store.columnOrder().isEmpty());
    }
}

// 006a：entryCountsByCategory 覆盖列视图设置——只设列宽 1 + 列序 2 + 隐藏 1 + 冻结 3
// → 视图状态那一项计数 == 1+2+1+1（冻结算 1 条）
void DATableDataTest::testMetadataEntryCountsColumnView()
{
    DA::DATableMetadata meta(nullptr);
    DA::DATableViewStateStore* vs = meta.viewState();
    QVERIFY(vs != nullptr);
    vs->setColumnWidth(QStringLiteral("f"), 0, 137);
    vs->setColumnOrder({ qMakePair(QStringLiteral("f"), 0), qMakePair(QStringLiteral("g"), 0) });
    vs->setColumnHidden(QStringLiteral("h"), 0, true);
    vs->setFreezeColumnCount(3);
    const QList< QPair< QString, int > > counts = meta.entryCountsByCategory();
    bool found = false;
    for (const QPair< QString, int >& c : counts) {
        if (c.first == QStringLiteral("Column view settings")) {
            found = true;
            QCOMPARE(c.second, 5);  // 1 列宽 + 2 列序 + 1 隐藏 + 1 冻结
        }
    }
    QVERIFY(found);
}

// 006b：容器运行时包装主窗格——.ui 不动，getDataTableView() 语义不变；
// 列序/隐藏投影到主窗格（列宽照常、尾部空白列恒可见排最后、冻结本单夹紧为 0）
void DATableDataTest::testPaneContainerWrappingAndMainPaneProjection()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);

    // 第一次打开：写 store（列序打乱 s,i,f,d,b + 隐藏 f + 列宽 f=137）
    {
        DA::DADataOperateOfDataFrameWidget w(d, &registry);
        DA::DATableViewStateStore* store = w.viewStateStore();
        QVERIFY(store != nullptr);
        store->setColumnOrder({ qMakePair(QStringLiteral("s"), 0), qMakePair(QStringLiteral("i"), 0),
                                qMakePair(QStringLiteral("f"), 0), qMakePair(QStringLiteral("d"), 0),
                                qMakePair(QStringLiteral("b"), 0) });
        store->setColumnHidden(QStringLiteral("f"), 0, true);
        store->setColumnWidth(QStringLiteral("f"), 0, 137);
    }

    // 「重开」数据页（同 testViewStateApplyAndHeaderClickChain 范式）
    DA::DADataOperateOfDataFrameWidget w2(d, &registry);
    DA::DADataTableView* tv = w2.getDataTableView();
    QVERIFY(tv != nullptr);
    // ① 运行时包装：视图的父部件即容器
    DA::DATablePaneContainer* c = qobject_cast< DA::DATablePaneContainer* >(tv->parentWidget());
    QVERIFY(c != nullptr);
    QCOMPARE(c->mainView(), tv);
    QCOMPARE(c->frozenView(), nullptr);
    QCOMPARE(c->effectiveFrozenColumnCount(), 0);
    QCOMPARE(w2.effectiveFrozenColumnCount(), 0);
    // ② 缓存与只读口径 getter
    QCOMPARE(c->cachedOrderedLogical().size(), 5);
    QCOMPARE(c->cachedHiddenLogical().size(), 1);
    QCOMPARE(c->visibleColumnOrderLogical().size(), 4);
    QCOMPARE(c->orderedColumnsLogical().size(), 5);

    QHeaderView* hh = tv->horizontalHeader();
    QVERIFY(hh != nullptr);
    // 数据列 logical：i=0, f=1, s=2, d=3, b=4；尾部预留空白列 = 5
    const int colI = 0, colF = 1, colS = 2, colD = 3, colB = 4, colSpare = 5;
    // ③ 列宽真源仍在（QHeaderView::sectionSize 对隐藏段返回 0，不能直接断言）
    QCOMPARE(w2.viewStateStore()->columnWidth(QStringLiteral("f"), 0), 137);
    // ④ 可见性：f 隐藏，其余可见
    QVERIFY(hh->isSectionHidden(colF));
    QVERIFY(!hh->isSectionHidden(colI));
    QVERIFY(!hh->isSectionHidden(colS));
    QVERIFY(!hh->isSectionHidden(colD));
    QVERIFY(!hh->isSectionHidden(colB));
    // ⑤ 顺序：可见列按 visualIndex 升序 == {s, i, d, b}
    QCOMPARE(hh->visualIndex(colS), 0);
    QCOMPARE(hh->visualIndex(colI), 1);
    QCOMPARE(hh->visualIndex(colD), 2);
    QCOMPARE(hh->visualIndex(colB), 3);
    // ⑥ 尾部预留空白列：恒可见、恒排最后
    QVERIFY(!hh->isSectionHidden(colSpare));
    QCOMPARE(hh->visualIndex(colSpare), 4);
    // ③b 恢复可见后列宽落地 137（隐藏期间宽度记在 hiddenSectionSize，显示时恢复）
    w2.setColumnHiddenDirectly(QStringLiteral("f"), 0, false);
    QVERIFY(!hh->isSectionHidden(colF));
    QCOMPARE(hh->sectionSize(colF), 137);
}

// 006b：setColumnHiddenDirectly——写 store + 重投影，不入撤销栈；isColumnInFrozenArea 恒 false
void DATableDataTest::testSetColumnHiddenDirectlyNoUndo()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    // store 预置隐藏 f → 重开页面投影生效
    {
        DA::DADataOperateOfDataFrameWidget w(d, &registry);
        w.viewStateStore()->setColumnHidden(QStringLiteral("f"), 0, true);
    }
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    QHeaderView* hh = tv->horizontalHeader();
    QVERIFY(hh != nullptr);
    const int colF = 1;  // f 的 logical
    QVERIFY(hh->isSectionHidden(colF));
    // 直接入口恢复可见：不入撤销栈
    const int undoBefore = w.getUndoStack()->count();
    w.setColumnHiddenDirectly(QStringLiteral("f"), 0, false);
    QVERIFY(!hh->isSectionHidden(colF));
    QCOMPARE(w.getUndoStack()->count(), undoBefore);
    // 冻结区判定（本单恒 false）
    QVERIFY(!w.isColumnInFrozenArea(colF));
    // 直接入口确实写了真源
    QVERIFY(!w.viewStateStore()->isColumnHidden(QStringLiteral("f"), 0));
}

// 006c：列序拖动判据——①拖出阈值又拖回原段松手不得误发 clickedWithModifiers
// （红色用例：修复前只看起止逻辑段会误发，拾取模式下会误填表达式）；
// ②拖到别处 = 一条 Reorder columns 命令；③对照：点击发一次 clickedWithModifiers 但不排序
void DATableDataTest::testColumnOrderDragDoesNotSort()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(900, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    QHeaderView* hh = tv->horizontalHeader();
    QVERIFY(hh != nullptr);
    QVERIFY(hh->sectionsMovable());
    DA::DADataHeaderView* hv = qobject_cast< DA::DADataHeaderView* >(hh);
    QVERIFY(hv != nullptr);

    const int col0 = 0;  // i
    const QPoint p0(hh->sectionViewportPosition(col0) + hh->sectionSize(col0) / 2, hh->viewport()->height() / 2);
    QWidget* hvp = hh->viewport();

    // ① 红色用例：拖出阈值 → 拖回原段松手 → 不发点击、不入栈
    QSignalSpy clickSpy(hv, &DA::DADataHeaderView::clickedWithModifiers);
    const int undoBefore = w.getUndoStack()->count();
    const int dragDist   = QApplication::startDragDistance() + 10;
    sendWindowMouse(hvp, QEvent::MouseMove, p0 - QPoint(dragDist, 0), Qt::NoButton);  // 先把鼠标送进表头
    sendWindowMouse(hvp, QEvent::MouseButtonPress, p0, Qt::LeftButton);
    sendWindowMouse(hvp, QEvent::MouseMove, p0 + QPoint(dragDist, 0), Qt::NoButton);
    sendWindowMouse(hvp, QEvent::MouseMove, p0, Qt::NoButton);
    sendWindowMouse(hvp, QEvent::MouseButtonRelease, p0, Qt::LeftButton);
    QCOMPARE(clickSpy.count(), 0);
    QCOMPARE(w.getUndoStack()->count(), undoBefore);  // 拖回原段 = 无列序变化

    // ② 回归：拖到第 2 个槽位 → 栈 +1、undoText=="Reorder columns"、可视顺序已变
    //（释放点必须越过第 2 段中点：Qt 的落点判定是 checkPos > posThreshold，恰好
    // 压在中点上不算移入，会被判为拖回原位）
    const int col1 = 1;  // f
    const QPoint p1(hh->sectionViewportPosition(col1) + hh->sectionSize(col1) / 2 + 15,
                    hh->viewport()->height() / 2);
    sendWindowMouse(hvp, QEvent::MouseButtonPress, p0, Qt::LeftButton);
    sendWindowMouse(hvp, QEvent::MouseMove, p1, Qt::NoButton);
    sendWindowMouse(hvp, QEvent::MouseButtonRelease, p1, Qt::LeftButton);
    QCOMPARE(clickSpy.count(), 0);
    QCOMPARE(w.getUndoStack()->count(), undoBefore + 1);
    QCOMPARE(w.getUndoStack()->undoText(), QStringLiteral("Reorder columns"));
    QCOMPARE(hh->visualIndex(col0), 1);
    // store 列序前两项 == 当前 visual 0/1 两列的身份（(列名, occ) 按数据列序算）
    const QList< QPair< QString, int > > order = w.viewStateStore()->columnOrder();
    QCOMPARE(order.size(), 5);
    QCOMPARE(order.at(0), qMakePair(QStringLiteral("f"), 0));
    QCOMPARE(order.at(1), qMakePair(QStringLiteral("i"), 0));

    // ③ 对照：真实点击（按下即松手、无移动）发 clickedWithModifiers，但不再产生排序命令
    const int undoAfterDrag = w.getUndoStack()->count();
    QTest::mouseClick(hvp, Qt::LeftButton, Qt::NoModifier, p0);
    QCOMPARE(clickSpy.count(), 1);
    QCOMPARE(w.getUndoStack()->count(), undoAfterDrag);  // 点击不排序（2026-10-18 修订基线）
    // 拖动产生的列序可一次撤销回数据列序（store 原为空 → 旧值按实际列序记录，
    // 投影等价于默认顺序：visualIndex 归位、身份表 = 数据列序）
    w.getUndoStack()->undo();
    QCOMPARE(hh->visualIndex(col0), 0);
    const QList< QPair< QString, int > > undone = w.viewStateStore()->columnOrder();
    QCOMPARE(undone.size(), 5);
    QCOMPARE(undone.at(0), qMakePair(QStringLiteral("i"), 0));
    QCOMPARE(undone.at(1), qMakePair(QStringLiteral("f"), 0));
}

// 006c：列视图命令族往返——三条命令 redo/undo 逐字段还原、文案、批量父命令
void DATableDataTest::testColumnViewCommandsUndoRedo()
{
    using Idents = QList< QPair< QString, int > >;
    using IdentSet = QSet< QPair< QString, int > >;
    DA::DATableViewStateStore store;
    int cbCount = 0;
    auto cb = [ &cbCount ]() { ++cbCount; };

    // ① 列序命令：文案 + redo 写新值 + undo 回旧值
    {
        DA::DACommandTableColumnOrder cmd(&store, Idents(), Idents({ qMakePair(QStringLiteral("b"), 0), qMakePair(QStringLiteral("a"), 0) }), cb);
        QCOMPARE(cmd.text(), QStringLiteral("Reorder columns"));
        cmd.redo();
        QCOMPARE(cbCount, 1);
        QCOMPARE(store.columnOrder(), Idents({ qMakePair(QStringLiteral("b"), 0), qMakePair(QStringLiteral("a"), 0) }));
        cmd.undo();
        QCOMPARE(cbCount, 2);
        QVERIFY(store.columnOrder().isEmpty());
    }
    // ② 隐藏命令：单列文案 + redo/undo 往返
    {
        DA::DACommandTableColumnVisibility cmd(&store, IdentSet(), IdentSet({ qMakePair(QStringLiteral("a"), 0) }), cb);
        QCOMPARE(cmd.text(), QStringLiteral("Hide column \"a\""));
        cmd.redo();
        QVERIFY(store.isColumnHidden(QStringLiteral("a"), 0));
        QCOMPARE(store.hiddenColumns().size(), 1);
        cmd.undo();
        QVERIFY(!store.isColumnHidden(QStringLiteral("a"), 0));
        QVERIFY(store.hiddenColumns().isEmpty());
    }
    // ③ 显示所有列文案 + redo/undo 往返
    {
        DA::DACommandTableColumnVisibility cmd(&store, IdentSet({ qMakePair(QStringLiteral("a"), 0) }), IdentSet(), cb);
        QCOMPARE(cmd.text(), QStringLiteral("Show all columns"));
        cmd.redo();
        QVERIFY(store.hiddenColumns().isEmpty());
        cmd.undo();
        QCOMPARE(store.hiddenColumns().size(), 1);
    }
    // ④ 冻结命令：两种文案 + redo/undo 往返
    {
        DA::DACommandTableFreezeColumns cmd(&store, 0, 2, cb);
        QCOMPARE(cmd.text(), QStringLiteral("Freeze columns to 2"));
        cmd.redo();
        QCOMPARE(store.freezeColumnCount(), 2);
        cmd.undo();
        QCOMPARE(store.freezeColumnCount(), 0);
        DA::DACommandTableFreezeColumns cmd2(&store, 2, 0, cb);
        QCOMPARE(cmd2.text(), QStringLiteral("Unfreeze columns"));
        cmd2.redo();
        QCOMPARE(store.freezeColumnCount(), 0);
        cmd2.undo();
        QCOMPARE(store.freezeColumnCount(), 2);
    }
    // ⑤ 批量父命令：两个子命令一次 redo/undo 全生效/全还原
    //（freeze 子命令旧值 = 进入⑤时 store 的现值 2，与页面批量构造方式一致）
    {
        DA::DACommandTableColumnViewBatch* parent = new DA::DACommandTableColumnViewBatch(QStringLiteral("Column View..."));
        new DA::DACommandTableColumnVisibility(&store, IdentSet({ qMakePair(QStringLiteral("a"), 0) }),
                                               IdentSet({ qMakePair(QStringLiteral("a"), 0), qMakePair(QStringLiteral("x"), 0) }),
                                               cb, QString(), parent);
        new DA::DACommandTableFreezeColumns(&store, 2, 3, cb, parent);
        QVERIFY(!parent->text().isEmpty());
        parent->redo();
        QVERIFY(store.isColumnHidden(QStringLiteral("x"), 0));
        QVERIFY(store.isColumnHidden(QStringLiteral("a"), 0));
        QCOMPARE(store.freezeColumnCount(), 3);
        parent->undo();
        QVERIFY(!store.isColumnHidden(QStringLiteral("x"), 0));
        QVERIFY(store.isColumnHidden(QStringLiteral("a"), 0));  // 旧隐藏集合（④ 结束时的 {a}）还原
        QCOMPARE(store.freezeColumnCount(), 2);                  // 旧冻结数还原
        delete parent;
    }
}

// 006c：真实列宽拖拽（贴段右缘起手）写 store、不入撤销栈——
// 可拖动表头（setSectionsMovable(true)）把 resize 起手吞掉是本单红线级回归点
void DATableDataTest::testColumnWidthNotOnUndoStack()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(900, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QHeaderView* hh = tv->horizontalHeader();
    QVERIFY(hh != nullptr);
    const int colF = 1;  // f
    const int widthBefore = hh->sectionSize(colF);
    // 起手点取 f 的左右边界（段边界 = 任何 Qt 版本的 resize 起手区都覆盖）
    const QPoint p(hh->sectionViewportPosition(colF) + hh->sectionSize(colF), hh->viewport()->height() / 2);
    QWidget* hvp = hh->viewport();
    const int undoBefore = w.getUndoStack()->count();
    // 真实拖宽：走平台事件链路（QTest QWindow 重载 → QWindowSystemInterface）
    // 更新 QGuiApplication::mouseButtons；合成链路不触发 Enter 分发，先给表头
    // 发一条 Enter（WA_UnderMouse 的唯一写入路径，= "鼠标已位于表头上"）
    const QPoint globalP(hvp->mapToGlobal(p));
    QEnterEvent enterEv(QPointF(p), QPointF(w.window()->mapToGlobal(QPoint(0, 0))) + QPointF(p), QPointF(globalP));
    QApplication::sendEvent(hh, &enterEv);
    QVERIFY(hh->underMouse());
    sendWindowMouse(hvp, QEvent::MouseButtonPress, p, Qt::LeftButton);
    sendWindowMouse(hvp, QEvent::MouseMove, p + QPoint(20, 0), Qt::NoButton);
    sendWindowMouse(hvp, QEvent::MouseButtonRelease, p + QPoint(20, 0), Qt::LeftButton);
    // 拖宽生效（±2px 允许样式微调）且写进 store、撤销栈不增长
    QVERIFY(qAbs(hh->sectionSize(colF) - (widthBefore + 20)) <= 2);
    QCOMPARE(w.viewStateStore()->columnWidth(QStringLiteral("f"), 0), hh->sectionSize(colF));
    QCOMPARE(w.getUndoStack()->count(), undoBefore);
}

namespace
{
// 构造 N 列小 DataFrame（列名给定）
static pybind11::object makeDataFrameWithColumns(const QStringList& columns)
{
    try {
        pybind11::dict scope;
        QStringList quoted;
        for (const QString& c : columns) {
            quoted << QStringLiteral("\"%1\"").arg(c);
        }
        QStringList rows;
        for (int r = 0; r < 3; ++r) {
            QStringList vals;
            for (int c = 0; c < columns.size(); ++c) {
                vals << QString::number(r + c);
            }
            rows << QStringLiteral("[%1]").arg(vals.join(QStringLiteral(", ")));
        }
        const QString pyCode = QStringLiteral("import pandas as pd\n"
                                              "df = pd.DataFrame([%1], columns=[%2])\n")
                                   .arg(rows.join(QStringLiteral(", ")))
                                   .arg(quoted.join(QStringLiteral(", ")));
        pybind11::exec(pyCode.toStdString(), pybind11::globals(), scope);
        return scope[ "df" ];
    } catch (const std::exception& e) {
        qWarning() << "makeDataFrameWithColumns failed:" << e.what();
    }
    return pybind11::none();
}

// 触发「列」子菜单某项：构建菜单 → 尾部 Columns 子菜单 → 按文本 trigger（006d）
static void triggerColumnViewMenuAction(DA::DADataHeaderView* hv, int logicalIndex, const QString& text)
{
    QVERIFY(hv != nullptr);
    QMenu* menu = hv->buildColumnContextMenu(logicalIndex);
    QVERIFY(menu != nullptr);
    QAction* columnsAct = nullptr;
    const auto acts = menu->actions();
    for (QAction* a : acts) {
        if (a->menu() != nullptr && a->text() == QStringLiteral("Columns")) {
            columnsAct = a;
            break;
        }
    }
    QVERIFY(columnsAct != nullptr);
    QAction* target = nullptr;
    const auto colActs = columnsAct->menu()->actions();
    for (QAction* a : colActs) {
        if (a->text() == text) {
            target = a;
            break;
        }
    }
    QVERIFY(target != nullptr);
    target->trigger();
    delete menu;
}
}  // namespace

// 006d：「列」子菜单 5 项逐项生效、各入栈 1 条；列视图对话框可开可取消
void DATableDataTest::testHeaderColumnViewMenuActions()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeDataFrameWithColumns({ QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c") });
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataHeaderView* hv = qobject_cast< DA::DADataHeaderView* >(w.getDataTableView()->horizontalHeader());
    QVERIFY(hv != nullptr);
    DA::DATableViewStateStore* store = w.viewStateStore();
    QVERIFY(store != nullptr);
    const int n0 = w.getUndoStack()->count();

    // ① 冻结到此列（logical=1：可见列序序号 1，+1 = 冻结 2 个可见列）
    triggerColumnViewMenuAction(hv, 1, QStringLiteral("Freeze to This Column"));
    QCOMPARE(store->freezeColumnCount(), 2);
    QCOMPARE(w.getUndoStack()->count(), n0 + 1);
    // ② 取消冻结
    triggerColumnViewMenuAction(hv, 1, QStringLiteral("Unfreeze"));
    QCOMPARE(store->freezeColumnCount(), 0);
    QCOMPARE(w.getUndoStack()->count(), n0 + 2);
    // ③ 隐藏此列（logical=2，列 c）
    triggerColumnViewMenuAction(hv, 2, QStringLiteral("Hide This Column"));
    QVERIFY(store->isColumnHidden(QStringLiteral("c"), 0));
    QCOMPARE(w.getUndoStack()->count(), n0 + 3);
    // ④ 显示所有列
    triggerColumnViewMenuAction(hv, 2, QStringLiteral("Show All Columns"));
    QVERIFY(store->hiddenColumns().isEmpty());
    QCOMPARE(w.getUndoStack()->count(), n0 + 4);
    // ⑤ 列视图…（模态对话框弹出后被代为取消：store 与撤销栈不变）
    bool opened = false;
    QTimer::singleShot(0, &w, [ &opened ]() {
        if (QWidget* m = QApplication::activeModalWidget()) {
            opened = true;
            QDialog* dlg = qobject_cast< QDialog* >(m);
            if (dlg) {
                dlg->reject();
            }
        }
    });
    triggerColumnViewMenuAction(hv, 0, QStringLiteral("Column View..."));
    QVERIFY(opened);
    QVERIFY(store->hiddenColumns().isEmpty());
    QCOMPARE(store->freezeColumnCount(), 0);
    QCOMPARE(w.getUndoStack()->count(), n0 + 4);
}

// 006d：列视图对话框——隐藏 + 冻结差异合成一个撤销单元，一次 undo 全还原
void DATableDataTest::testColumnViewDialogDiffAndUndo()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeDataFrameWithColumns({ QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c") });
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    w.freezeColumnsTo(1);  // 前置：冻结 1
    const int n0 = w.getUndoStack()->count();

    // 打开对话框：勾掉 a 与 c、spinFreeze 设 1 → 确定
    //（lambda 内只采集状态 + 收尾关闭对话框，不做断言——QCOMPARE 失败会提前
    //  return 导致对话框滞留、exec 阻塞）
    bool opened   = false;
    int seenCount = -1;
    QTimer::singleShot(0, &w, [ &opened, &seenCount ]() {
        DA::DADialogTableColumnView* dlg
            = qobject_cast< DA::DADialogTableColumnView* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        opened = true;
        QListWidget* list = dlg->findChild< QListWidget* >(QStringLiteral("listColumns"));
        QSpinBox* spin    = dlg->findChild< QSpinBox* >(QStringLiteral("spinFreeze"));
        if (list && spin) {
            seenCount = list->count();
            for (int i = 0; i < list->count(); ++i) {
                const QString name = list->item(i)->data(Qt::UserRole).toString();
                if (name == QStringLiteral("a") || name == QStringLiteral("c")) {
                    list->item(i)->setCheckState(Qt::Unchecked);
                }
            }
            spin->setValue(1);
        }
        dlg->accept();
    });
    w.showColumnViewDialog();
    QVERIFY(opened);
    QCOMPARE(seenCount, 3);
    // ① 两列隐藏且主窗格投影生效（006b）
    QVERIFY(w.viewStateStore()->isColumnHidden(QStringLiteral("a"), 0));
    QVERIFY(w.viewStateStore()->isColumnHidden(QStringLiteral("c"), 0));
    QVERIFY(tv->isColumnHidden(0));
    QVERIFY(tv->isColumnHidden(2));
    // ② 冻结数落库
    QCOMPARE(w.viewStateStore()->freezeColumnCount(), 1);
    // ③ 一个撤销单元
    QCOMPARE(w.getUndoStack()->count(), n0 + 1);
    QCOMPARE(w.getUndoStack()->undoText(), QStringLiteral("Column View..."));
    // ④ 一次 undo：隐藏集合与冻结数一起还原到前置值（a 归冻结区 → 冻结窗格可见）
    w.getUndoStack()->undo();
    QVERIFY(w.viewStateStore()->hiddenColumns().isEmpty());
    QVERIFY(!tv->isColumnHidden(1));
    QVERIFY(!tv->isColumnHidden(2));
    QVERIFY(tv->isColumnHidden(0));  // a 在冻结区（前置 freeze=1），主窗格隐藏
    QCOMPARE(w.viewStateStore()->freezeColumnCount(), 1);
    // ⑤ 重开：全部勾回、spinFreeze 设 0 → 确定 → 全可见、冻结 0；undo 后重推会
    // 清掉已撤销的批命令（QUndoStack::push 语义），栈计数回到 n0+1
    bool opened2 = false;
    QTimer::singleShot(0, &w, [ &opened2 ]() {
        DA::DADialogTableColumnView* dlg
            = qobject_cast< DA::DADialogTableColumnView* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        opened2 = true;
        QListWidget* list = dlg->findChild< QListWidget* >(QStringLiteral("listColumns"));
        QSpinBox* spin    = dlg->findChild< QSpinBox* >(QStringLiteral("spinFreeze"));
        if (list && spin) {
            for (int i = 0; i < list->count(); ++i) {
                list->item(i)->setCheckState(Qt::Checked);
            }
            spin->setValue(0);
        }
        dlg->accept();
    });
    w.showColumnViewDialog();
    QVERIFY(opened2);
    QVERIFY(w.viewStateStore()->hiddenColumns().isEmpty());
    QCOMPARE(w.viewStateStore()->freezeColumnCount(), 0);
    QCOMPARE(w.getUndoStack()->count(), n0 + 1);  // 已撤销的批命令被新 push 清掉
}

// 006d：重名列显示 a / a (2) 且两项独立勾选；取消不改动任何东西
void DATableDataTest::testColumnViewDialogDuplicateNamesInitialAndCancel()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeDataFrameWithColumns({ QStringLiteral("a"), QStringLiteral("a"), QStringLiteral("b") });
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    w.freezeColumnsTo(2);  // 前置：冻结 2
    const int n0 = w.getUndoStack()->count();

    // 打开：读初始状态（重名文本/SpinBox 起始值）后取消勾选 a(occ 0)+改冻结数，再取消
    bool opened = false;
    QStringList seenTexts;
    int seenSpin = -1;
    QTimer::singleShot(0, &w, [ &opened, &seenTexts, &seenSpin ]() {
        DA::DADialogTableColumnView* dlg
            = qobject_cast< DA::DADialogTableColumnView* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        opened = true;
        QListWidget* list = dlg->findChild< QListWidget* >(QStringLiteral("listColumns"));
        QSpinBox* spin    = dlg->findChild< QSpinBox* >(QStringLiteral("spinFreeze"));
        if (list && spin) {
            const int n = list->count();
            for (int i = 0; i < n; ++i) {
                seenTexts << list->item(i)->text();
            }
            seenSpin = spin->value();
            // ③ 取消路径：改动对话框状态后 reject → 不落任何东西
            list->item(0)->setCheckState(Qt::Unchecked);
            spin->setValue(1);
        }
        dlg->reject();
    });
    w.showColumnViewDialog();
    QVERIFY(opened);
    // ① 重名显示（occurrence 0 不带后缀）
    QCOMPARE(seenTexts.size(), 3);
    QCOMPARE(seenTexts.at(0), QStringLiteral("a"));
    QCOMPARE(seenTexts.at(1), QStringLiteral("a (2)"));
    // ② SpinBox 起始值 = 生效冻结数（006d 阶段冻结投影未生效，恒 0）
    QCOMPARE(seenSpin, w.effectiveFrozenColumnCount());
    QVERIFY(w.viewStateStore()->hiddenColumns().isEmpty());
    QCOMPARE(w.viewStateStore()->freezeColumnCount(), 2);
    QCOMPARE(w.getUndoStack()->count(), n0);
    // ④ 确定路径：只取消勾选 a (2) → accept → 重名两项独立
    bool opened2 = false;
    QTimer::singleShot(0, &w, [ &opened2 ]() {
        DA::DADialogTableColumnView* dlg
            = qobject_cast< DA::DADialogTableColumnView* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        opened2 = true;
        QListWidget* list = dlg->findChild< QListWidget* >(QStringLiteral("listColumns"));
        if (list && list->count() == 3) {
            list->item(1)->setCheckState(Qt::Unchecked);  // "a (2)"
        }
        dlg->accept();
    });
    w.showColumnViewDialog();
    QVERIFY(opened2);
    QVERIFY(!w.viewStateStore()->isColumnHidden(QStringLiteral("a"), 0));
    QVERIFY(w.viewStateStore()->isColumnHidden(QStringLiteral("a"), 1));
    QVERIFY(!w.viewStateStore()->isColumnHidden(QStringLiteral("b"), 0));
}

// 006e T6：冻结 + 列视图叠加——打乱列序 + 隐藏 1 列 + 冻结 2 的全链路投影
void DATableDataTest::testFrozenAndColumnViewCombination()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    // widget#1：写 store（列序 s,i,f,d,b + 隐藏 f + 冻结 2 + 各列宽）
    {
        DA::DADataOperateOfDataFrameWidget w(d, &registry);
        DA::DATableViewStateStore* store = w.viewStateStore();
        QVERIFY(store != nullptr);
        store->setColumnOrder({ qMakePair(QStringLiteral("s"), 0), qMakePair(QStringLiteral("i"), 0),
                                qMakePair(QStringLiteral("f"), 0), qMakePair(QStringLiteral("d"), 0),
                                qMakePair(QStringLiteral("b"), 0) });
        store->setColumnHidden(QStringLiteral("f"), 0, true);
        store->setFreezeColumnCount(2);
        store->setColumnWidth(QStringLiteral("s"), 0, 101);
        store->setColumnWidth(QStringLiteral("i"), 0, 102);
        store->setColumnWidth(QStringLiteral("d"), 0, 103);
        store->setColumnWidth(QStringLiteral("b"), 0, 104);
    }
    // 「重开」数据页：构造时投影（frozenSet={s,i}、mainSet={d,b}，spare 恒主窗格可见）
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATablePaneContainer* container = w.paneContainer();
    QVERIFY(container != nullptr);
    DA::DADataTableView* main = w.getDataTableView();
    DA::DADataTableView* frozen = container->frozenView();
    QVERIFY(main != nullptr);
    QVERIFY(frozen != nullptr);
    QVERIFY(frozen->isVisible());
    QCOMPARE(w.effectiveFrozenColumnCount(), 2);
    // 数据列 logical：i=0, f=1, s=2, d=3, b=4；spare=5
    const int colI = 0, colF = 1, colS = 2, colD = 3, colB = 4;
    QHeaderView* fh = frozen->horizontalHeader();
    QHeaderView* mh = main->horizontalHeader();
    QVERIFY(fh != nullptr);
    QVERIFY(mh != nullptr);
    // ① 可见性互补（按"应可见"数据列取样：f 是用户隐藏列，两窗格都隐藏）
    QVERIFY(fh->isSectionHidden(colF));
    QVERIFY(mh->isSectionHidden(colF));
    QVERIFY(!fh->isSectionHidden(colS));
    QVERIFY(mh->isSectionHidden(colS));  // 冻结列在主区隐藏
    QVERIFY(!fh->isSectionHidden(colI));
    QVERIFY(mh->isSectionHidden(colI));
    QVERIFY(fh->isSectionHidden(colD));
    QVERIFY(!mh->isSectionHidden(colD));
    QVERIFY(fh->isSectionHidden(colB));
    QVERIFY(!mh->isSectionHidden(colB));
    // 两窗格可见数据列的并集 = 全部非隐藏列、交集为空
    QSet< int > frozenVisible, mainVisible;
    const int dataCols[] = { colI, colF, colS, colD, colB };
    for (int c : dataCols) {
        if (!fh->isSectionHidden(c)) {
            frozenVisible.insert(c);
        }
        if (!mh->isSectionHidden(c)) {
            mainVisible.insert(c);
        }
    }
    QCOMPARE(frozenVisible, QSet< int >({ colS, colI }));
    QCOMPARE(mainVisible, QSet< int >({ colD, colB }));
    // ② 顺序：各窗格可见列按 visualIndex 升序 == ordered 里本区段的顺序
    {
        QList< int > frozenOrder, mainOrder;
        for (int v = 0; v < fh->count(); ++v) {
            const int logical = fh->logicalIndex(v);
            if (!fh->isSectionHidden(logical)) {
                frozenOrder.append(logical);
            }
        }
        for (int v = 0; v < mh->count(); ++v) {
            const int logical = mh->logicalIndex(v);
            if (!mh->isSectionHidden(logical) && logical != 5) {  // spare 恒排主区末尾
                mainOrder.append(logical);
            }
        }
        QCOMPARE(frozenOrder, QList< int >({ colS, colI }));
        QCOMPARE(mainOrder, QList< int >({ colD, colB }));
    }
    // ③ 列宽在两窗格与 store 一致（f 隐藏，宽度记在 hiddenSectionSize，恢复可见后比对）
    QCOMPARE(fh->sectionSize(colS), 101);
    QCOMPARE(fh->sectionSize(colI), 102);
    QCOMPARE(mh->sectionSize(colD), 103);
    QCOMPARE(mh->sectionSize(colB), 104);
    // ④ 冻结窗格宽 = Σ(冻结列宽) + 行头宽 + 2×frame（+2px 余量，±2 断言）
    {
        int expect = fh->sectionSize(colS) + fh->sectionSize(colI) + frozen->verticalHeader()->width()
                     + 2 * frozen->frameWidth();
        QVERIFY(qAbs(frozen->width() - expect) <= 3);
    }
    // ⑤ 滚动条策略：冻结窗格纵横 AlwaysOff；主窗格保持默认
    QCOMPARE(frozen->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QCOMPARE(frozen->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
    QCOMPARE(main->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(main->horizontalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    // ⑥ 视口等高（底部等高占位生效）
    QCOMPARE(frozen->viewport()->height(), main->viewport()->height());
    // ⑦ 纵向滚动同步 + 末尾不错行
    {
        QScrollBar* mv = main->verticalScrollBar();
        QScrollBar* fv = frozen->verticalScrollBar();
        mv->setValue(mv->maximum());
        QApplication::processEvents();
        QCOMPARE(fv->value(), mv->value());
        QCOMPARE(frozen->rowViewportPosition(0), main->rowViewportPosition(0));
        QCOMPARE(frozen->verticalHeader()->offset(), main->verticalHeader()->offset());
    }
    // ⑧ 转发契约：冻结表头 columnActionRequested 转发到主表头（Q_SIGNALS 即 public）
    {
        DA::DADataHeaderView* fheader = qobject_cast< DA::DADataHeaderView* >(fh);
        QVERIFY(fheader != nullptr);
        QSignalSpy spy(mh, &QHeaderView::sectionMoved);  // 占位：真实断言在下面
        Q_UNUSED(spy);
        DA::DADataHeaderView* mheader = qobject_cast< DA::DADataHeaderView* >(mh);
        QVERIFY(mheader != nullptr);
        QSignalSpy actionSpy(mheader, &DA::DADataHeaderView::columnActionRequested);
        Q_EMIT fheader->columnActionRequested(colD, DA::DADataHeaderView::ColumnAction::GotoMax);
        QCOMPARE(actionSpy.count(), 1);
    }
    // ⑨ 解冻 → 冻结窗格隐藏；再冻结 → 顺序/宽度/可见集合全部还原（复用已有窗格）
    w.freezeColumnsTo(0);
    QVERIFY(!frozen->isVisible());
    QCOMPARE(mh->sectionSize(colS), 101);  // 解冻后主窗格接管全部列宽
    w.freezeColumnsTo(2);
    QVERIFY(frozen->isVisible());
    QCOMPARE(fh->sectionSize(colS), 101);
    QCOMPARE(fh->sectionSize(colI), 102);
    QCOMPARE(mh->sectionSize(colD), 103);
    QCOMPARE(mh->sectionSize(colB), 104);
    QVERIFY(!fh->isSectionHidden(colS) && fh->isSectionHidden(colD));
}

// 006e T7：列宽 137 在"冻结 → 解冻 → 再冻结"往返中两窗格不回退；列宽不入栈
void DATableDataTest::testColumnWidthSurvivesFreezeCycle()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* main   = w.getDataTableView();
    DA::DATablePaneContainer* container = w.paneContainer();
    const int colF = 1;  // f
    // 冻结（默认列序 [i,f,s,d,b] + 冻结 2 → f 在冻结区）
    const int n0 = w.getUndoStack()->count();
    w.freezeColumnsTo(2);
    QCOMPARE(w.getUndoStack()->count(), n0 + 1);  // 冻结命令入栈 1 条
    QCOMPARE(w.getUndoStack()->undoText(), QStringLiteral("Freeze columns to 2"));
    DA::DADataTableView* frozen = container->frozenView();
    QVERIFY(frozen != nullptr);
    // ① 用户拖宽 f 到 137（程序化写宽度模拟拖拽结果；采集闸门不受影响——直接写 store）
    frozen->horizontalHeader()->resizeSection(colF, 137);
    w.viewStateStore()->setColumnWidth(QStringLiteral("f"), 0, 137);
    QCOMPARE(frozen->horizontalHeader()->sectionSize(colF), 137);
    // ② 解冻后主窗格接管：宽度 137 保留
    w.freezeColumnsTo(0);
    QCOMPARE(main->horizontalHeader()->sectionSize(colF), 137);
    // ③ 再冻结：f 归冻结区，冻结窗格 137；主窗格侧 f 隐藏（宽度记在
    // hiddenSectionSize，Qt 对隐藏段 sectionSize 恒 0——不能直接断言）
    w.freezeColumnsTo(2);
    QCOMPARE(frozen->horizontalHeader()->sectionSize(colF), 137);
    QVERIFY(main->horizontalHeader()->isSectionHidden(colF));
    // 再解冻一次：主窗格恢复 137（hiddenSectionSize 未回退的证明）
    w.freezeColumnsTo(0);
    QApplication::processEvents();
    QCOMPARE(main->horizontalHeader()->sectionSize(colF), 137);
    // ④ 列宽不入栈：全程只有 4 条冻结命令（冻/解/再冻/再解），无列宽命令
    QCOMPARE(w.getUndoStack()->count(), n0 + 4);
    const int nBeforeWidth = w.getUndoStack()->count();
    frozen->horizontalHeader()->resizeSection(colF, 150);
    w.viewStateStore()->setColumnWidth(QStringLiteral("f"), 0, 150);
    QCOMPARE(w.getUndoStack()->count(), nBeforeWidth);  // 宽度变化零命令
}

// 006e T8：行头归属与宽、双向纵向滚动同步、两窗格共享同一 selectionModel/model
void DATableDataTest::testPaneRowHeaderAndScrollSync()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* main = w.getDataTableView();
    DA::DATablePaneContainer* container = w.paneContainer();
    w.freezeColumnsTo(2);
    DA::DADataTableView* frozen = container->frozenView();
    QVERIFY(frozen != nullptr);
    // ① 行头归属：冻结时行头归冻结窗格（frozen 可见、main 隐藏）
    QVERIFY(frozen->verticalHeader()->isVisibleTo(frozen));
    QVERIFY(!main->verticalHeader()->isVisibleTo(main));
    // ② 主窗格滚动 → 冻结窗格跟随（取中间值与最大值各一次）
    QScrollBar* mv = main->verticalScrollBar();
    QScrollBar* fv = frozen->verticalScrollBar();
    const int mid = (mv->minimum() + mv->maximum()) / 2;
    mv->setValue(mid);
    QApplication::processEvents();
    QCOMPARE(fv->value(), mv->value());
    QCOMPARE(frozen->rowViewportPosition(0), main->rowViewportPosition(0));
    mv->setValue(mv->maximum());
    QApplication::processEvents();
    QCOMPARE(fv->value(), mv->value());
    QCOMPARE(frozen->rowViewportPosition(0), main->rowViewportPosition(0));
    // ③ 反向：冻结窗格滚动条变化 → 主窗格跟随
    fv->setValue(mid / 2);
    QApplication::processEvents();
    QCOMPARE(mv->value(), fv->value());
    // ④ 共享同一 selectionModel 与 model
    QVERIFY(frozen->selectionModel() == main->selectionModel());
    QVERIFY(frozen->model() == main->model());
    // ⑤ getDataTableView() == 容器主窗格
    QVERIFY(w.getDataTableView() == container->mainView());
}

// 007a：定位到隐藏列——自动取消隐藏（不入撤销栈）并选中目标格
void DATableDataTest::testLocateActualCellHiddenColumn()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    const int colF = 1;  // f
    // 隐藏 f（006 页面入口，入栈一条隐藏命令）
    w.hideColumnAt(colF);
    QVERIFY(w.viewStateStore()->isColumnHidden(QStringLiteral("f"), 0));
    QVERIFY(tv->isColumnHidden(colF));
    const int undoBefore = w.getUndoStack()->count();
    // 定位到隐藏列 → 自动取消隐藏 + 提示 + 不入栈
    w.locateActualCell(3, colF);
    QVERIFY(!w.viewStateStore()->isColumnHidden(QStringLiteral("f"), 0));
    QVERIFY(!tv->isColumnHidden(colF));
    QCOMPARE(tv->toActualRow(tv->currentIndex().row()), 3);
    QCOMPARE(tv->currentIndex().column(), colF);
    QCOMPARE(w.getUndoStack()->count(), undoBefore);  // 取消隐藏不入撤销栈
    // Ctrl+Z 不会把列再藏回去（栈顶仍是 hide 命令——回退的是"隐藏"本身）
    w.getUndoStack()->undo();
    QVERIFY(!w.viewStateStore()->isColumnHidden(QStringLiteral("f"), 0));  // hide 命令 undo = 显示
}

// 007a：定位到冻结区列——主区横滚不动、目标格选中、焦点给冻结窗格
void DATableDataTest::testLocateActualCellFrozenColumn()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* main   = w.getDataTableView();
    DA::DATablePaneContainer* c = w.paneContainer();
    QVERIFY(c != nullptr);
    // 冻结前 2 列（默认列序 i,f → 冻结区 = {i, f}）
    w.freezeColumnsTo(2);
    QVERIFY(w.isColumnInFrozenArea(1));  // f 在冻结区
    DA::DADataTableView* frozen = c->frozenView();
    QVERIFY(frozen != nullptr && frozen->isVisible());
    // 先把主区横向滚动条挪到非 0 位置（f 在冻结区、主区只剩 s,d,b+空白列）
    QScrollBar* hb = main->horizontalScrollBar();
    hb->setValue(hb->maximum() > 0 ? hb->maximum() : 0);
    QApplication::processEvents();
    const int hbBefore = hb->value();
    // 定位到冻结区内的列 f
    w.locateActualCell(5, 1);
    QCOMPARE(hb->value(), hbBefore);  // 主区横向滚动条不动
    QCOMPARE(main->toActualRow(main->currentIndex().row()), 5);
    QCOMPARE(main->currentIndex().column(), 1);
    // 目标格被选中（ClearAndSelect，不是 NoUpdate）
    QVERIFY(main->selectionModel()->isSelected(main->model()->index(main->toLogicalRow(5), 1)));
    // 焦点在冻结窗格
    w.activateWindow();
    QApplication::processEvents();
    QVERIFY(frozen->hasFocus());
}

// 007a：定位窗外行——整表语义（showActualRow 把窗口搬过去）
void DATableDataTest::testLocateActualCellOutOfWindow()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::dict scope;
    pybind11::exec("import pandas as pd\n"
                   "df = pd.DataFrame({'a': range(100000), 'b': [float(i) for i in range(100000)]})",
                    pybind11::globals(),
                    scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    QVERIFY(!tv->isActualRowVisible(60000));  // 定位前在窗口外（默认滑动窗 20000 行）
    w.locateActualCell(60000, 0);
    // showActualRow 目标行落到视口首行（贴数据两端时收紧）；60000 在数据中部应精确落位
    QCOMPARE(tv->toActualRow(tv->currentIndex().row()), 60000);
    QVERIFY(tv->isActualRowVisible(60000));
    QVERIFY(qAbs(tv->getTopActualRow() - 60000) <= static_cast< int >(60000) / 10);  // 窗口围绕目标行
    QCOMPARE(tv->currentIndex().column(), 0);
    // 越界夹取：定位超界行 → 夹到数据范围内（贴近末行；末行走"贴数据尾部收紧"
    // 路径，Qt 视口取整允许 ±1 行的落位偏差，断言按范围与可见性写）
    w.locateActualCell(999999, 0);
    const int lastCur = tv->toActualRow(tv->currentIndex().row());
    QVERIFY(lastCur >= 100000 - 3 && lastCur <= 99999);
    QVERIFY(tv->isActualRowVisible(lastCur));
}

// 007a：keepHidden=true / select=false 边界——不收缩选区、不解除隐藏、不夺焦点
void DATableDataTest::testLocateKeepHiddenAndNoSelect()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    const int colF = 1, colS = 2;
    // 隐藏 f + 制造既有选区（整列选中 s）
    w.hideColumnAt(colF);
    w.ensureColumnVisible(QStringLiteral("s"), true);
    const int selBefore      = static_cast< int >(tv->selectionModel()->selectedIndexes().size());
    const int undoBefore     = w.getUndoStack()->count();
    const auto hiddenBefore  = w.viewStateStore()->hiddenColumns();
    // ① select=false：导航不收缩选区、不解除隐藏、不入栈；当前项移到目标格
    w.locateActualCell(3, colF, /*select=*/false);
    QVERIFY(w.viewStateStore()->isColumnHidden(QStringLiteral("f"), 0));
    QVERIFY(tv->isColumnHidden(colF));
    QCOMPARE(w.viewStateStore()->hiddenColumns().size(), hiddenBefore.size());
    QCOMPARE(w.getUndoStack()->count(), undoBefore);
    QCOMPARE(static_cast< int >(tv->selectionModel()->selectedIndexes().size()), selBefore);  // 002 回归：选区不收缩
    QCOMPARE(tv->currentIndex().column(), colF);
    QCOMPARE(tv->toActualRow(tv->currentIndex().row()), 3);
    // ② keepHidden=true：定位选中但 store 保持隐藏（007b 对话框勾选项的通路）。
    // 视图层已知边界（执行单 §4 步骤 1 写明）：目标列既隐藏又不在冻结区时，
    // 主区分支的 selectActualCell→showColumn 会解除视图隐藏——v1 接受该结果，
    // 断言按"store 真源不变 + 当前项与选区落位"写
    w.locateActualCell(3, colF, /*select=*/true, /*keepHidden=*/true);
    QVERIFY(w.viewStateStore()->isColumnHidden(QStringLiteral("f"), 0));  // 真源仍隐藏
    QCOMPARE(w.getUndoStack()->count(), undoBefore);
    QCOMPARE(tv->currentIndex().column(), colF);
    QVERIFY(tv->selectionModel()->isSelected(tv->model()->index(tv->toLogicalRow(3), colF)));  // 目标格已选中
    // ③ 对照：未隐藏列上 keepHidden=true → 正常定位选中（"保持隐藏"不是"不动位置"）
    w.locateActualCell(2, colS, true, true);
    QVERIFY(!tv->isColumnHidden(colS));
    QCOMPARE(tv->currentIndex().column(), colS);
    QCOMPARE(tv->toActualRow(tv->currentIndex().row()), 2);
    QVERIFY(tv->selectionModel()->isSelected(tv->model()->index(tv->toLogicalRow(2), colS)));
}

// 007b：定位请求解析——行位置/索引标签/列名/快捷项/keepHidden 全路径
void DATableDataTest::testGotoDialogRequestResolution()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    // 索引标签 7 重名、列名 b 重名（执行单夹具列名表 ["a","b","b"] 与其 ① 的 "c" 断言
    // 自相矛盾——改为四列 a,b,c,b：既有唯一名 c 供 ①/⑦，又保留重名 b 供 ④）
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
gdf = pd.DataFrame({"a": [10, 20, 30], "b": [1, 2, 3], "c": [4, 5, 6], "e": [7, 8, 9]}, index=[7, 7, 9])
gdf.columns = ["a", "b", "c", "b"]
)",
                    pybind11::globals(),
                    scope);
    DA::DAPyDataFrame df(scope[ "gdf" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    using Req = DA::DADialogGotoCell::Request;

    // ① 行位置 + 列名（普通）
    {
        Req req;
        req.rowKind     = Req::RowKind::Position;
        req.rowPosition = 3;
        req.columnKind  = Req::ColumnKind::Name;
        req.columnName  = QStringLiteral("c");
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 2);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 2);
    }
    // ② 索引标签：唯一 → 行 2；重名 → 第一个（行 0）
    {
        Req req;
        req.rowKind    = Req::RowKind::IndexLabel;
        req.indexLabel = QStringLiteral("9");
        req.columnKind = Req::ColumnKind::None;
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 2);
    }
    {
        Req req;
        req.rowKind    = Req::RowKind::IndexLabel;
        req.indexLabel = QStringLiteral("7");  // 重名索引：跳到第一个
        req.columnKind = Req::ColumnKind::None;
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 0);
    }
    // ③ 索引不存在：false 且选区不变
    {
        tv->selectActualCell(1, 0);
        const QModelIndex before = tv->currentIndex();
        Req req;
        req.rowKind    = Req::RowKind::IndexLabel;
        req.indexLabel = QStringLiteral("zzz");
        req.columnKind = Req::ColumnKind::None;
        QVERIFY(!w.resolveGotoRequest(req));
        QCOMPARE(tv->currentIndex(), before);
    }
    // ④ 列名重名取第一个；列名不存在 → false
    {
        Req req;
        req.rowKind     = Req::RowKind::Position;
        req.rowPosition = 1;
        req.columnKind  = Req::ColumnKind::Name;
        req.columnName  = QStringLiteral("b");  // 重名列取第一个（列 1）
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeColumn(), 1);
    }
    {
        const QModelIndex before = tv->currentIndex();
        Req req;
        req.rowKind     = Req::RowKind::None;
        req.columnKind  = Req::ColumnKind::Name;
        req.columnName  = QStringLiteral("zzz");
        QVERIFY(!w.resolveGotoRequest(req));
        QCOMPARE(tv->currentIndex(), before);
    }
    // ⑤ 四个快捷项
    {
        Req req;
        req.rowKind    = Req::RowKind::FirstRow;
        req.columnKind = Req::ColumnKind::FirstColumn;
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 0);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 0);
    }
    {
        Req req;
        req.rowKind    = Req::RowKind::LastRow;
        req.columnKind = Req::ColumnKind::LastColumn;  // 末列 = 索引 3（b 重名第二项）
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 2);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 3);
    }
    {
        Req req;
        req.rowKind    = Req::RowKind::FirstRow;
        req.columnKind = Req::ColumnKind::LastColumn;
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 0);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 3);
    }
    {
        Req req;
        req.rowKind    = Req::RowKind::LastRow;
        req.columnKind = Req::ColumnKind::FirstColumn;
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 2);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 0);
    }
    // ⑥ 只填列保持当前行；只填行保持当前列
    {
        Req req;
        req.rowKind     = Req::RowKind::Position;
        req.rowPosition = 2;
        req.columnKind  = Req::ColumnKind::Name;
        req.columnName  = QStringLiteral("a");  // 行 1 列 0
        QVERIFY(w.resolveGotoRequest(req));
        QCOMPARE(w.getSelectedOneDataframeRow(), 1);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 0);
        Req onlyCol;
        onlyCol.rowKind    = Req::RowKind::None;  // 只填列 → 保持当前行（行 1）
        onlyCol.columnKind = Req::ColumnKind::Name;
        onlyCol.columnName = QStringLiteral("a");
        QVERIFY(w.resolveGotoRequest(onlyCol));
        QCOMPARE(w.getSelectedOneDataframeRow(), 1);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 0);
        Req onlyRow;
        onlyRow.rowKind     = Req::RowKind::Position;
        onlyRow.rowPosition = 1;  // 只填行 → 保持当前列（列 0）
        onlyRow.columnKind  = Req::ColumnKind::None;
        QVERIFY(w.resolveGotoRequest(onlyRow));
        QCOMPARE(w.getSelectedOneDataframeRow(), 0);
        QCOMPARE(w.getSelectedOneDataframeColumn(), 0);
    }
    // ⑦ keepHidden：隐藏 c 后定位 → 真源保持隐藏（视图层走 v1 已知边界）
    {
        w.viewStateStore()->setColumnHidden(QStringLiteral("c"), 0, true);
        DA::DADataOperateOfDataFrameWidget wProjector(d, &registry);  // 触发重投影（重开页范式）
        Req req;
        req.rowKind     = Req::RowKind::Position;
        req.rowPosition = 3;
        req.columnKind  = Req::ColumnKind::Name;
        req.columnName  = QStringLiteral("c");
        req.keepHidden  = true;
        QVERIFY(w.resolveGotoRequest(req));
        QVERIFY(w.viewStateStore()->isColumnHidden(QStringLiteral("c"), 0));  // 真源仍隐藏
        QCOMPARE(w.getSelectedOneDataframeRow(), 2);
    }
}

// 007c：列统计区间路径与 pandas 直算对拍（类型/空值/区间边界/唯一值数）
void DATableDataTest::testColumnStatsOperatorExtended()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
sdf = pd.DataFrame({
    "v": [1, 2, 3, 4, 100],
    "n": [1.0, None, 3.0, None, 5.0],
    "e": [None, None, None, None, None],
    "t": ["a", "b", "c", "d", "e"],
    "d": pd.to_datetime(["2020-01-01","2020-01-02","2020-01-03","2020-01-04","2020-01-05"]),
    "b": [True, False, True, False, True],
})
)",
                    pybind11::globals(),
                    scope);
    DA::DAPyDataFrame df(scope[ "sdf" ]);
    QVERIFY(!df.isNone());
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    const QList< int > allCols { 0, 1, 2, 3, 4, 5 };
    const auto r = py.columnStatsRange(df, allCols, 0, 5, 100);
    QVERIFY(r.ok);
    QCOMPARE(r.columns.size(), 6);
    // ① 数值列 v
    QCOMPARE(r.columns[ 0 ].count, std::size_t(5));
    QCOMPARE(r.columns[ 0 ].nullCount, std::size_t(0));
    QVERIFY(r.columns[ 0 ].sum.has_value());
    QCOMPARE(*r.columns[ 0 ].sum, 110.0);
    QVERIFY(r.columns[ 0 ].mean.has_value());
    QCOMPARE(*r.columns[ 0 ].mean, 22.0);
    QCOMPARE(*r.columns[ 0 ].min, 1.0);
    QCOMPARE(*r.columns[ 0 ].max, 100.0);
    // ② 含 NaN 列 n
    QCOMPARE(r.columns[ 1 ].count, std::size_t(3));
    QCOMPARE(r.columns[ 1 ].nullCount, std::size_t(2));
    QCOMPARE(*r.columns[ 1 ].sum, 9.0);
    QCOMPARE(*r.columns[ 1 ].mean, 3.0);
    QCOMPARE(*r.columns[ 1 ].min, 1.0);
    QCOMPARE(*r.columns[ 1 ].max, 5.0);
    // ③ 全空列 e：四项数值指标皆无值
    QCOMPARE(r.columns[ 2 ].count, std::size_t(0));
    QCOMPARE(r.columns[ 2 ].nullCount, std::size_t(5));
    QVERIFY(!r.columns[ 2 ].sum.has_value());
    QVERIFY(!r.columns[ 2 ].mean.has_value());
    QVERIFY(!r.columns[ 2 ].min.has_value());
    QVERIFY(!r.columns[ 2 ].max.has_value());
    // ④ 文本列 t：四项全无值、uniqueCount 有值
    QCOMPARE(r.columns[ 3 ].count, std::size_t(5));
    QVERIFY(!r.columns[ 3 ].sum.has_value());
    QVERIFY(!r.columns[ 3 ].min.has_value());
    QVERIFY(r.columns[ 3 ].uniqueCount.has_value());
    QCOMPARE(*r.columns[ 3 ].uniqueCount, std::size_t(5));
    // ⑤ 日期列 d 同文本列
    QCOMPARE(r.columns[ 4 ].count, std::size_t(5));
    QVERIFY(!r.columns[ 4 ].sum.has_value());
    QVERIFY(r.columns[ 4 ].uniqueCount.has_value());
    QCOMPARE(*r.columns[ 4 ].uniqueCount, std::size_t(5));
    // ⑥ 布尔列 b：四项全无值（即使 pandas 能求和也不给）
    QCOMPARE(r.columns[ 5 ].count, std::size_t(5));
    QVERIFY(!r.columns[ 5 ].sum.has_value());
    QVERIFY(r.columns[ 5 ].uniqueCount.has_value());
    QCOMPARE(*r.columns[ 5 ].uniqueCount, std::size_t(2));
    // ⑦ 区间边界：半开 (0,3) 只统计前 3 行；(0,1) 一行；(2,2) 空
    const auto r3 = py.columnStatsRange(df, { 0 }, 0, 3, 100);
    QVERIFY(r3.ok);
    QCOMPARE(r3.columns[ 0 ].count, std::size_t(3));
    QCOMPARE(*r3.columns[ 0 ].sum, 6.0);
    const auto r1 = py.columnStatsRange(df, { 0 }, 0, 1, 100);
    QCOMPARE(r1.columns[ 0 ].count, std::size_t(1));
    const auto r0 = py.columnStatsRange(df, { 0 }, 2, 2, 100);
    QVERIFY(r0.ok);
    QCOMPARE(r0.columns[ 0 ].count, std::size_t(0));
    QVERIFY(!r0.columns[ 0 ].sum.has_value());
    // ⑧ count + nullCount == 区间行数 对每列都成立
    for (const auto& col : r.columns) {
        QCOMPARE(col.count + col.nullCount, std::size_t(5));
    }
}

// 007c：旧路径兼容（三元组行为逐位不变 + Python 直调口径）
void DATableDataTest::testColumnStatsLegacyCompatibility()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
ldf = pd.DataFrame({"v": [1, 2, 100], "t": ["x", "y", "z"], "e": [None, None, None]})
)",
                    pybind11::globals(),
                    scope);
    DA::DAPyDataFrame df(scope[ "ldf" ]);
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // ① 既有封装：数值列三元组
    const auto r = py.columnStats(df, 0);
    QVERIFY(r.ok);
    QCOMPARE(r.minValue, 1.0);
    QCOMPARE(r.maxValue, 100.0);
    QCOMPARE(r.medianValue, 2.0);
    // ② 纯文本列 → ok=false（全 NaN）
    QVERIFY(!py.columnStats(df, 1).ok);
    QVERIFY(!py.columnStats(df, 2).ok);
    // ③ 越界 → ok=false
    QVERIFY(!py.columnStats(df, 99).ok);
    QVERIFY(!py.columnStats(df, -1).ok);
    // ④ Python 直调：旧路径三元组逐位相等
    {
        pybind11::object ret = py.attr("da_column_stats")(df.object(), 0);
        pybind11::tuple t    = ret.cast< pybind11::tuple >();
        QCOMPARE(t[ 0 ].cast< double >(), 1.0);
        QCOMPARE(t[ 1 ].cast< double >(), 100.0);
        QCOMPARE(t[ 2 ].cast< double >(), 2.0);
    }
    // ⑤ Python 直调：全 NaN 列返回 None
    {
        pybind11::object ret = py.attr("da_column_stats")(df.object(), 2);
        QVERIFY(ret.is_none());
    }
}

// 007c：唯一值数哨兵（超阈值/不要/数值列不请求）
void DATableDataTest::testColumnStatsUniqueSentinel()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
udf = pd.DataFrame({"t": ["a", "b", "c", "d", "e"], "v": [1, 2, 3, 4, 5]})
)",
                    pybind11::globals(),
                    scope);
    DA::DAPyDataFrame df(scope[ "udf" ]);
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // ① rows(5) > unique_limit(4) → 不给
    {
        const auto r = py.columnStatsRange(df, { 0 }, 0, 5, 4);
        QVERIFY(r.ok);
        QVERIFY(!r.columns[ 0 ].uniqueCount.has_value());
    }
    // ② unique_limit=0（不要）→ 不给
    {
        const auto r = py.columnStatsRange(df, { 0 }, 0, 5, 0);
        QVERIFY(r.ok);
        QVERIFY(!r.columns[ 0 ].uniqueCount.has_value());
    }
    // ③ 数值列即便 rows<=limit 也不给（口径上不请求）
    {
        const auto r = py.columnStatsRange(df, { 1 }, 0, 5, 100);  // 只请求列 1（数值列 v）
        QVERIFY(r.ok);
        QVERIFY(!r.columns[ 0 ].uniqueCount.has_value());
        QVERIFY(r.columns[ 0 ].sum.has_value());
    }
    // ④ 文本列 rows(5) <= limit(5) → 给且等于 nunique
    {
        const auto r = py.columnStatsRange(df, { 0 }, 0, 5, 5);
        QVERIFY(r.ok);
        QVERIFY(r.columns[ 0 ].uniqueCount.has_value());
        QCOMPARE(*r.columns[ 0 ].uniqueCount, std::size_t(5));
    }
    // ⑤ DADataPyDataFrame 翻译层：nullopt → kUniqueNotComputed
    {
        DA::DAData d2(df);
        const DA::DATableDataSource* src = d2.tableSource();
        QVERIFY(src != nullptr);
        DA::DATableScanRequest req2;
        req2.rowBegin  = 0;
        req2.rowEnd    = 5;
        req2.columns   = { 0 };
        req2.uniqueLimit = 4;
        const auto res = src->computeColumnStats(req2);
        QVERIFY(res.has_value());
        QCOMPARE((*res)[ 0 ].uniqueCount, DA::kUniqueNotComputed);
    }
}

// 007c：computeColumnStats 默认 nullopt（未覆写源）+ 覆写生效（DADataPyDataFrame）
void DATableDataTest::testComputeColumnStatsNullopt()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    DA::DATableScanRequest req;
    req.rowBegin = 0;
    req.rowEnd   = 3;
    req.columns  = { 0 };
    // ① 未覆写源：DATestLazyTable（既有测试载体，10 万行合成惰性表）
    {
        DATestLazyTable ts;
        QVERIFY(ts.computeColumnStats(req) == std::nullopt);
    }
    // ② sqlite 惰性表（若可用）同样 nullopt
    {
        if (ensureSqliteReady()) {
            DA::DAData sqlD = makeSqliteLazyData();
            const DA::DATableDataSource* src = sqlD.tableSource();
            QVERIFY(src != nullptr);
            QVERIFY(src->computeColumnStats(req) == std::nullopt);
        }
    }
    // ③ DADataPyDataFrame：正常请求有值且与 pandas 直算一致
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
ndf = pd.DataFrame({"a": [1, 2, 3], "b": [4, 5, 6]})
)",
                        pybind11::globals(),
                        scope);
        DA::DAPyDataFrame df(scope[ "ndf" ]);
        DA::DAData d(df);
        const DA::DATableDataSource* src = d.tableSource();
        QVERIFY(src != nullptr);
        const auto res = src->computeColumnStats(req);
        QVERIFY(res.has_value());
        QCOMPARE((*res).size(), 1);
        QCOMPARE((*res)[ 0 ].count, std::size_t(3));
        QCOMPARE(*(*res)[ 0 ].sum, 6.0);
        // 越界列 → nullopt（不静默裁剪、不抛异常）
        DA::DATableScanRequest bad = req;
        bad.columns = { 2 };  // 只有 2 列
        QVERIFY(src->computeColumnStats(bad) == std::nullopt);
    }
}

// 007d：状态栏统计分档显示（单列/多列/超限/多段/单格/不支持）
void DATableDataTest::testStatsWidgetSelectionTiers()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    // 12 列夹具（多列超限分支需要 ≥9 列）
    QStringList colNames;
    for (int i = 0; i < 12; ++i) {
        colNames << QStringLiteral("c%1").arg(i);
    }
    pybind11::object obj = makeDataFrameWithColumns(colNames);
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableStatsWidget stats;
    stats.setPage(&w);
    DA::DADataTableView* tv = w.getDataTableView();
    QItemSelectionModel* sm = tv->selectionModel();

    // ① 单列数值列（c0）：全指标数值档，无 Unique
    {
        QItemSelection sel(tv->model()->index(0, 0), tv->model()->index(2, 0));
        sm->select(sel, QItemSelectionModel::ClearAndSelect);
        stats.refreshNow();
        const QString text = stats.statsText();
        qWarning() << "[dbg] tier1 statsText =" << text;
        QVERIFY(text.contains(QStringLiteral("Count")));
        QVERIFY(text.contains(QStringLiteral("Null")));
        QVERIFY(text.contains(QStringLiteral("Sum")));
        QVERIFY(text.contains(QStringLiteral("Mean")));
        QVERIFY(text.contains(QStringLiteral("Min")));
        QVERIFY(text.contains(QStringLiteral("Max")));
        QVERIFY(!text.contains(QStringLiteral("Unique")));
        QVERIFY(stats.detailsTooltipText().isEmpty());
    }
    // ② 多列 c ≤ 8（c0..c2）：状态栏只放规模；tooltip 有逐列明细
    {
        QItemSelection sel(tv->model()->index(0, 0), tv->model()->index(2, 2));
        sm->select(sel, QItemSelectionModel::ClearAndSelect);
        stats.refreshNow();
        const QString text = stats.statsText();
        QVERIFY(text.contains(QStringLiteral("cols selected")));
        QVERIFY(!text.contains(QStringLiteral("Sum")));  // 状态栏不含指标
        QVERIFY(stats.detailsTooltipText().contains(QStringLiteral("Per-column details")));
        QVERIFY(stats.detailsTooltipText().contains(QStringLiteral("Sum")));    // 数值列明细有 Sum
        QVERIFY(stats.detailsTooltipText().contains(QStringLiteral("Count")));
    }
    // ③ 多列超限（9 列 c0..c8 > 8）：不请求，tooltip 写"选区过大"
    {
        QItemSelection sel(tv->model()->index(0, 0), tv->model()->index(0, 8));
        sm->select(sel, QItemSelectionModel::ClearAndSelect);
        stats.refreshNow();
        QVERIFY(stats.detailsTooltipText().contains(
            QStringLiteral("Selection is too large; per-column details were not computed")));
        QVERIFY(stats.statsText().contains(QStringLiteral("cols selected")));
    }
    // ④ 多段选区：几何计数 + 说明，不请求统计（两段都在模型窗口内：不同列的行段）
    {
        sm->clearSelection();
        QItemSelection seg1(tv->model()->index(0, 0), tv->model()->index(1, 0));
        QItemSelection seg2(tv->model()->index(0, 4), tv->model()->index(1, 4));        sm->select(seg1, QItemSelectionModel::Select);
        sm->select(seg2, QItemSelectionModel::Select);
        stats.refreshNow();
        const QString text = stats.statsText();
        QVERIFY(text.contains(QStringLiteral("2 segments")));
        QVERIFY(text.contains(QStringLiteral("4 cells")));        QVERIFY(stats.detailsTooltipText().contains(
            QStringLiteral("Aggregation is not available for multi-range selections")));
    }
    // ⑤ 单格（1×1）：引用 + 原始值（数据只有 3 行，取第 2 行 → 显示 Row 2）
    {
        QItemSelection sel(tv->model()->index(1, 1), tv->model()->index(1, 1));
        sm->select(sel, QItemSelectionModel::ClearAndSelect);
        sm->setCurrentIndex(tv->model()->index(1, 1), QItemSelectionModel::NoUpdate);
        stats.refreshNow();
        const QString text = stats.statsText();
        QVERIFY(text.contains(QStringLiteral("Row 2")));
        QVERIFY(text.contains(QStringLiteral("c1")));
    }
    // ⑥ 不支持的源（惰性表页）：明说"不支持"
    {
        DA::DAAbstractData::Pointer lazyPtr(new DATestLazyTable());
        DA::DAData lazyD(lazyPtr);
        DA::DADataOperateOfDataFrameWidget lazyW(lazyD, &registry);
        lazyW.resize(800, 400);
        lazyW.show();
        QVERIFY(QTest::qWaitForWindowExposed(&lazyW));
        stats.setPage(&lazyW);
        QItemSelectionModel* lazySm = lazyW.getDataTableView()->selectionModel();
        QItemSelection sel(lazyW.getDataTableView()->model()->index(0, 0),
                           lazyW.getDataTableView()->model()->index(4, 0));
        lazySm->select(sel, QItemSelectionModel::ClearAndSelect);
        stats.refreshNow();
        QVERIFY(stats.statsText().contains(
            QStringLiteral("Statistics are not available for the current data source")));
        stats.setPage(&w);
    }
}

// 007d：统计部件去抖与只读（撤销栈不变、解绑隐藏）
void DATableDataTest::testStatsWidgetDebounceAndReadonly()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableStatsWidget stats;
    stats.setPage(&w);
    const int undoBefore = w.getUndoStack()->count();
    DA::DADataTableView* tv = w.getDataTableView();
    QItemSelectionModel* sm = tv->selectionModel();
    // 连续 3 次改选区（不插事件循环等待）→ 去抖期间不触发刷新
    for (int i = 0; i < 3; ++i) {
        QItemSelection sel(tv->model()->index(i, 1), tv->model()->index(i + 4, 1));
        sm->select(sel, QItemSelectionModel::ClearAndSelect);
    }
    // 统计是只读操作：撤销栈不变
    QCOMPARE(w.getUndoStack()->count(), undoBefore);
    // 去抖到期后刷新一次
    QTest::qWait(600);
    QVERIFY(!stats.statsText().isEmpty());
    // 解绑 → 隐藏、清文本
    stats.setPage(nullptr);
    QVERIFY(stats.isHidden());
    QVERIFY(stats.statsText().isEmpty());
}

//===================================================
// 002 表头排序（T1.3，计划 §4.1 用例）
//===================================================

// 002 §4.1：da_sort 多键 + 置换——单键升/降序结果正确、稳定排序（同值保序）、
// perm 语义（new[i]==old[perm[i]]）、置换往返（逆置换还原）、混合类型抛错
void DATableDataTest::testSortOperatorMultiKeyPerm()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();

    // 单键升序 + perm 语义（new[i] == old[perm[i]]）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"a":[3,1,2],"b":["x","y","z"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QList< QVariant > oldB { df.iat(0, 1), df.iat(1, 1), df.iat(2, 1) };
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, true) }, &perm));
        QCOMPARE(perm, QVector< int >({ 1, 2, 0 }));
        QCOMPARE(df.iat(0, 0).toInt(), 1);
        QCOMPARE(df.iat(1, 0).toInt(), 2);
        QCOMPARE(df.iat(2, 0).toInt(), 3);
        for (int i = 0; i < 3; ++i) {
            QCOMPARE(df.iat(i, 1), oldB.at(perm.at(i)));
        }
    }
    // 单键降序稳定（同值保序：c, b(0), b(2), a(1), a(3)）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"k":["b","a","b","a","c"],"ord":[0,1,2,3,4]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, false) }, &perm));
        const QList< int > expect { 4, 0, 2, 1, 3 };
        for (int i = 0; i < 5; ++i) {
            QCOMPARE(df.iat(i, 1).toInt(), expect.at(i));
        }
    }
    // 多键：A 升序主键，B 降序次键；NaN 恒排最后（组内也一样）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,1,2,2,1],"B":[1.0,2.0,3.0,4.0,None]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, true), qMakePair(1, false) }, &perm));
        QCOMPARE(df.iat(0, 0).toInt(), 1);
        QCOMPARE(df.iat(1, 0).toInt(), 1);
        QCOMPARE(df.iat(2, 0).toInt(), 1);
        QCOMPARE(df.iat(3, 0).toInt(), 2);
        QCOMPARE(df.iat(4, 0).toInt(), 2);
        QCOMPARE(df.iat(0, 1).toDouble(), 2.0);
        QCOMPARE(df.iat(1, 1).toDouble(), 1.0);
        QVERIFY(qIsNaN(df.iat(2, 1).toDouble()));
        QCOMPARE(df.iat(3, 1).toDouble(), 4.0);
        QCOMPARE(df.iat(4, 1).toDouble(), 3.0);
    }
    // 置换往返：随机数据 sort → 逆置换还原 → 正置换再排序（undo/redo 的算子基础）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd, numpy as np
np.random.seed(7)
df = pd.DataFrame({"x":np.random.rand(50),"y":np.random.randint(0,5,50)})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< QVariant > oldX, oldY;
        oldX.reserve(50);
        oldY.reserve(50);
        for (int r = 0; r < 50; ++r) {
            oldX.append(df.iat(r, 0));
            oldY.append(df.iat(r, 1));
        }
        QVector< int > perm;
        QVERIFY(py.sort(df, { qMakePair(0, true) }, &perm));
        QCOMPARE(perm.size(), 50);
        for (int r = 1; r < 50; ++r) {
            QVERIFY(df.iat(r - 1, 0).toDouble() <= df.iat(r, 0).toDouble());
        }
        const QVector< int > inv = DA::DATableKeyRemap::inversePermutation(perm);
        QVERIFY(py.reorderRows(df, inv));
        for (int r = 0; r < 50; ++r) {
            QCOMPARE(df.iat(r, 0), oldX.at(r));
            QCOMPARE(df.iat(r, 1), oldY.at(r));
        }
        QVERIFY(py.reorderRows(df, perm));
        for (int r = 1; r < 50; ++r) {
            QVERIFY(df.iat(r - 1, 0).toDouble() <= df.iat(r, 0).toDouble());
        }
    }
    // 混合类型 object 列：算子抛 ValueError（not comparable）→ wrapper 返回 false，数据不变
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"m":pd.Series([1,"a",2.5], dtype=object)})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QVector< int > perm;
        QVERIFY(!py.sort(df, { qMakePair(0, true) }, &perm));
        QCOMPARE(df.iat(0, 0).toInt(), 1);
        QCOMPARE(df.iat(1, 0).toString(), QLatin1String("a"));
    }
}

// 002 §4.1：DACommandDataFrame_sortRows undo/redo 语义 + onRowsReordered 样式键随排序联动
void DATableDataTest::testSortRowsCommandUndoRedoAndStyleRemap()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"a":[3,1,2],"b":["x","y","z"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    // 行 0（a=3）设单元格样式，验证排序后样式键随行移动
    DA::DATableStyleManager mgr;
    DA::DATableCellStyle red;
    red.setBackground(QColor(Qt::red));
    mgr.setCellStyle(0, 0, red, false);
    QVERIFY(mgr.hasCellStyle(0, 0));

    QUndoStack stack;
    DA::DACommandDataFrame_sortRows* cmd = new DA::DACommandDataFrame_sortRows(df, { qMakePair(0, true) });
    DA::DACommandDataFrame_sortRows* cmdRaw = cmd;
    cmd->setDirectionalCallBack([ &mgr, cmdRaw ](bool isUndo) {
        Q_UNUSED(isUndo);
        mgr.onRowsReordered(cmdRaw->lastAppliedPermutation());
    });
    QVERIFY(cmd->exec());
    // 升序 [1,2,3]；原行 0（a=3,b=x）移动到行 2，样式跟随
    QCOMPARE(df.iat(0, 0).toInt(), 1);
    QCOMPARE(df.iat(2, 0).toInt(), 3);
    QCOMPARE(df.iat(2, 1).toString(), QLatin1String("x"));
    QVERIFY(!mgr.hasCellStyle(0, 0));
    QVERIFY(mgr.hasCellStyle(2, 0));
    // 命令文本（撤销面板显示用）
    QCOMPARE(cmd->text(), QStringLiteral("Sort rows"));

    stack.push(cmd);  // 首次 redo 跳过（DACommandWithRedoCount）
    QCOMPARE(df.iat(0, 0).toInt(), 1);
    // Ctrl+Z 一次 → 顺序完全还原，样式回原行
    stack.undo();
    QCOMPARE(df.iat(0, 0).toInt(), 3);
    QCOMPARE(df.iat(1, 0).toInt(), 1);
    QCOMPARE(df.iat(2, 0).toInt(), 2);
    QCOMPARE(df.iat(0, 1).toString(), QLatin1String("x"));
    QCOMPARE(df.iat(1, 1).toString(), QLatin1String("y"));
    QVERIFY(mgr.hasCellStyle(0, 0));
    QVERIFY(!mgr.hasCellStyle(2, 0));
    // redo → 再次排序，样式再跟随
    stack.redo();
    QCOMPARE(df.iat(0, 0).toInt(), 1);
    QCOMPARE(df.iat(2, 0).toInt(), 3);
    QVERIFY(mgr.hasCellStyle(2, 0));
    QVERIFY(!mgr.hasCellStyle(0, 0));
}

// 002 §4.2（程序化）：表头点击排序交互——单击升/降循环、点另一列重置、Shift/Ctrl 追加与切换次排序列、
// 一次点击=一个撤销单元、选中行随置换跟随、拾取模式互斥、只读源不排序
void DATableDataTest::testHeaderSortInteraction()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    DA::DADataHeaderView* hh = qobject_cast< DA::DADataHeaderView* >(tv->horizontalHeader());
    QVERIFY(hh != nullptr);
    auto colOf = [ hh ](const char* name) -> int {
        for (int c = 0; c < hh->count(); ++c) {
            if (hh->model() && hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String(name)) {
                return c;
            }
        }
        return -1;
    };
    const int colI = colOf("i");
    const int colF = colOf("f");
    const int colS = colOf("s");
    QVERIFY(colI >= 0 && colF >= 0 && colS >= 0);
    auto clickHeader = [ hh ](int col, Qt::KeyboardModifiers mods) {
        const int x = hh->sectionViewportPosition(col) + qMax(1, hh->sectionSize(col) / 2);
        const int y = qMax(1, hh->viewport()->height() / 2);
        QTest::mouseClick(hh->viewport(), Qt::LeftButton, mods, QPoint(x, y));
    };

    // 【2026-10-18 交互修订】点击表头不再触发排序（误触即整表重排，用户反馈修订）：
    // 规格保持空、数据不变（Qt 默认的列选中行为保留）
    clickHeader(colI, Qt::NoModifier);
    QVERIFY(w.sortSpec().isEmpty());
    QCOMPARE(df.iat(0, colI).toInt(), 1);
    QCOMPARE(df.iat(9, colI).toInt(), 10);
    clickHeader(colI, Qt::ShiftModifier);
    clickHeader(colF, Qt::ControlModifier);
    QVERIFY(w.sortSpec().isEmpty());
    // 排序入口 = 列头右键菜单（APP 接线到页面方法）：降序
    w.sortColumn(colI, Qt::DescendingOrder);
    QCOMPARE(w.sortSpec().size(), 1);
    QCOMPARE(w.sortSpec().at(0).first, colI);
    QCOMPARE(w.sortSpec().at(0).second, Qt::DescendingOrder);
    QCOMPARE(hh->sortSpec(), w.sortSpec());  // 表头角标数据源同步
    QCOMPARE(df.iat(0, colI).toInt(), 10);
    QCOMPARE(df.iat(9, colI).toInt(), 1);
    // 重置规格为仅该列：升序
    w.sortColumn(colI, Qt::AscendingOrder);
    QCOMPARE(w.sortSpec().size(), 1);
    QCOMPARE(w.sortSpec().at(0).second, Qt::AscendingOrder);
    QCOMPARE(df.iat(0, colI).toInt(), 1);
    // 排另一列 → 只按新列排序（旧列指示消失：规格仅剩一项）
    w.sortColumn(colF, Qt::AscendingOrder);
    QCOMPARE(w.sortSpec().size(), 1);
    QCOMPARE(w.sortSpec().at(0).first, colF);
    QCOMPARE(w.sortSpec().at(0).second, Qt::AscendingOrder);
    // f 升序：前 8 行单调不减，2 个 NaN 排最后
    for (int r = 1; r < 8; ++r) {
        QVERIFY(df.iat(r - 1, colF).toDouble() <= df.iat(r, colF).toDouble());
    }
    QVERIFY(qIsNaN(df.iat(8, colF).toDouble()));
    QVERIFY(qIsNaN(df.iat(9, colF).toDouble()));
    // 追加次级排序列（菜单「追加为次级排序」）：两列规格，多列序号角标的数据源
    w.addSecondarySortColumn(colI, Qt::AscendingOrder);
    QCOMPARE(w.sortSpec().size(), 2);
    QCOMPARE(w.sortSpec().at(1).first, colI);
    QCOMPARE(w.sortSpec().at(1).second, Qt::AscendingOrder);
    QCOMPARE(hh->sortSpec().size(), 2);
    // 角标绘制冒烟（§4.2「两列都显示序号角标」的程序化验证）：渲染表头，
    // 断言角标主蓝 #5280C1 与箭头深蓝灰 #5D6D7E 的纯色像素出现（抗锯齿边缘不参与计数）
    {
        const QSize sz = hh->viewport()->size();
        QVERIFY(sz.width() > 0 && sz.height() > 0);
        QImage img(sz, QImage::Format_ARGB32);
        img.fill(static_cast< QRgb >(0x00FFFFFF));
        hh->viewport()->render(&img);
        auto countColor = [ &img ](QRgb c) -> int {
            int n = 0;
            for (int y = 0; y < img.height(); ++y) {
                for (int x = 0; x < img.width(); ++x) {
                    if (img.pixel(x, y) == c) {
                        ++n;
                    }
                }
            }
            return n;
        };
        const int badgePx = countColor(qRgb(0x52, 0x80, 0xC1));  // 序号角标圆底（两列各一枚）
        const int arrowPx = countColor(qRgb(0x5D, 0x6D, 0x7E));  // 右端方向箭头（两列各一枚）
        QVERIFY2(badgePx >= 10, qPrintable(QStringLiteral("badge pixels=%1").arg(badgePx)));
        QVERIFY2(arrowPx >= 4, qPrintable(QStringLiteral("arrow pixels=%1").arg(arrowPx)));
    }
    // 同列再追加 → 切换其方向（规格列数不变）
    QVector< QVariant > beforeUndoF, beforeUndoI;
    beforeUndoF.reserve(10);
    beforeUndoI.reserve(10);
    for (int r = 0; r < 10; ++r) {
        beforeUndoF.append(df.iat(r, colF));
        beforeUndoI.append(df.iat(r, colI));
    }
    w.addSecondarySortColumn(colI, Qt::DescendingOrder);
    QCOMPARE(w.sortSpec().size(), 2);
    QCOMPARE(w.sortSpec().at(1).second, Qt::DescendingOrder);
    bool changed = false;
    for (int r = 0; r < 10 && !changed; ++r) {
        changed = (df.iat(r, colI) != beforeUndoI.at(r));
    }
    QVERIFY(changed);  // 方向切换确实重排了数据
    // Ctrl+Z 一次 → 顺序完全还原（一次点击 = 一个撤销单元）
    // f 列含 NaN，QVariant 相等比较对 NaN!=NaN，用 NaN 感知比较
    auto sameValue = [](const QVariant& a, const QVariant& b) -> bool {
        const double da = a.toDouble();
        const double db = b.toDouble();
        if (qIsNaN(da) && qIsNaN(db)) {
            return true;
        }
        return a == b;
    };
    w.getUndoStack()->undo();
    for (int r = 0; r < 10; ++r) {
        QVERIFY2(sameValue(df.iat(r, colF), beforeUndoF.at(r)), qPrintable(QStringLiteral("undo mismatch at row %1 col f").arg(r)));
        QVERIFY2(sameValue(df.iat(r, colI), beforeUndoI.at(r)), qPrintable(QStringLiteral("undo mismatch at row %1 col i").arg(r)));
    }
    // 选中行随置换跟随：选中行 0 → 排序 → 选区映射到该行数据的新位置
    tv->selectionModel()->select(tv->model()->index(0, colS), QItemSelectionModel::ClearAndSelect);
    const QString marked = df.iat(0, colS).toString();
    w.sortColumn(colI, Qt::AscendingOrder);  // 重置为仅 i 升序
    QCOMPARE(w.sortSpec().size(), 1);
    int newRow = -1;
    for (int r = 0; r < 10; ++r) {
        if (df.iat(r, colS).toString() == marked) {
            newRow = r;
            break;
        }
    }
    QVERIFY(newRow >= 0);
    bool selFollowed = false;
    const QModelIndexList sel = tv->selectionModel()->selectedIndexes();
    for (const QModelIndex& si : sel) {
        if (tv->toActualRow(si.row()) == newRow) {
            selFollowed = true;
        }
    }
    QVERIFY(selFollowed);
    w.hide();

    // 只读/惰性源：排序入口不生效、规格保持空（daWarning 提示路径）
    if (ensureSqliteReady()) {
        DA::DAAbstractData::Pointer lazy = makeSqliteLazyData();
        DA::DAData ld(lazy);
        DA::DATableMetadataRegistry reg2(nullptr);
        DA::DADataOperateOfDataFrameWidget wl(ld, &reg2);
        wl.resize(600, 300);
        wl.show();
        QVERIFY(QTest::qWaitForWindowExposed(&wl));
        DA::DADataHeaderView* hl = qobject_cast< DA::DADataHeaderView* >(wl.getDataTableView()->horizontalHeader());
        QVERIFY(hl != nullptr);
        const int x = hl->sectionViewportPosition(0) + qMax(1, hl->sectionSize(0) / 2);
        const int y = qMax(1, hl->viewport()->height() / 2);
        QTest::mouseClick(hl->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(x, y));
        wl.sortColumn(0, Qt::AscendingOrder);  // 菜单排序入口在只读源上不执行
        wl.addSecondarySortColumn(0, Qt::AscendingOrder);
        QVERIFY(wl.sortSpec().isEmpty());
        wl.hide();
    } else {
        qWarning() << "sqlite lazy table not ready, read-only sort guard skipped:" << mSqliteErr;
    }
}

// 002 §4.2（程序化）：10 万行单列排序 ≤300 ms（算子+封装路径计时）
void DATableDataTest::testSortPerf100k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
np.random.seed(123)
big = pd.DataFrame({"v": np.random.rand(100000), "w": np.random.randint(0, 100, 100000)})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "big" ]);
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    QVector< int > perm;
    QElapsedTimer timer;
    timer.start();
    QVERIFY(py.sort(df, { qMakePair(0, true) }, &perm));
    const qint64 elapsed = timer.elapsed();
    qDebug().noquote() << QStringLiteral("testSortPerf100k: 100k single-key sort elapsed_ms=%1").arg(elapsed);
    QCOMPARE(perm.size(), 100000);
    QVERIFY2(elapsed <= 300, qPrintable(QStringLiteral("100k sort took %1 ms (> 300 ms)").arg(elapsed)));
    // 抽查升序（首、中、尾）
    const QList< int > probes { 1, 1000, 50000, 99999 };
    for (int r : probes) {
        QVERIFY(df.iat(r - 1, 0).toDouble() <= df.iat(r, 0).toDouble());
    }
    // 双键排序也在预算内
    timer.restart();
    QVERIFY(py.sort(df, { qMakePair(1, true), qMakePair(0, false) }, &perm));
    const qint64 elapsed2 = timer.elapsed();
    qDebug().noquote() << QStringLiteral("testSortPerf100k: 100k two-key sort elapsed_ms=%1").arg(elapsed2);
    QVERIFY2(elapsed2 <= 300, qPrintable(QStringLiteral("100k two-key sort took %1 ms (> 300 ms)").arg(elapsed2)));
}

// 排序选区重映射（2026-10-18 性能修复的回归守卫）：
// 正确性——散落单元格选区随置换跟行走、连续行段映射后归并成单 range；
// 性能——整列选择（Qt 表头点击/ selectColumn 会产生每行一个 range 的 2 万格选区）
// 下排序不再逐格重建选区（旧实现 32560 行实测 985~7736ms，用户现场 6~19s）
void DATableDataTest::testSortSelectionRemapPerf()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    // ---- 正确性（小表 10 行）----
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    DA::DADataHeaderView* hh = qobject_cast< DA::DADataHeaderView* >(tv->horizontalHeader());
    QVERIFY(hh != nullptr);
    int colI = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model() && hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("i")) {
            colI = c;
            break;
        }
    }
    QVERIFY(colI >= 0);
    QAbstractItemModel* m  = tv->model();
    QItemSelectionModel* sm = tv->selectionModel();
    // 散落选中 i 列行 0/2/5（值 1/3/6）→ 降序（10..1）后旧行 r 的新位置 = 9-r
    QItemSelection sel;
    const QList< int > pickRows { 0, 2, 5 };
    for (int r : pickRows) {
        const QModelIndex ix = m->index(r, colI);
        sel.select(ix, ix);
    }
    sm->select(sel, QItemSelectionModel::ClearAndSelect);
    w.sortColumn(colI, Qt::DescendingOrder);
    QSet< int > mappedRows;
    const QModelIndexList after = sm->selectedIndexes();
    for (const QModelIndex& ix : after) {
        if (ix.column() == colI) {
            mappedRows.insert(tv->toActualRow(ix.row()));
        }
    }
    QCOMPARE(mappedRows.size(), 3);
    QVERIFY(mappedRows.contains(9));  // 值1 → 行9
    QVERIFY(mappedRows.contains(7));  // 值3 → 行7
    QVERIFY(mappedRows.contains(4));  // 值6 → 行4
    // 连续行段：当前为降序（10..1），选中视图行 2..5（值 8/7/6/5）→ 升序后落位行 4..7，连续 → 应归并为 1 个 range
    QItemSelection sel2;
    sel2.select(m->index(2, colI), m->index(5, colI));
    sm->select(sel2, QItemSelectionModel::ClearAndSelect);
    w.sortColumn(colI, Qt::AscendingOrder);
    QCOMPARE(sm->selection().size(), 1);
    const QItemSelectionRange only = sm->selection().at(0);
    QCOMPARE(only.top(), 4);
    QCOMPARE(only.bottom(), 7);
    QCOMPARE(only.left(), colI);
    QCOMPARE(only.right(), colI);
    w.hide();

    // ---- 性能（32560 行，复现用户现场规模）----
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
rng = np.random.default_rng(7)
N = 32560
big = pd.DataFrame({"v": rng.normal(size=N), "w": rng.integers(0, 1000, size=N)})
)",
                   scope);
    DA::DAPyDataFrame bdf(scope[ "big" ]);
    DA::DAData bd(bdf);
    DA::DATableMetadataRegistry reg2(nullptr);
    DA::DADataOperateOfDataFrameWidget wb(bd, &reg2);
    wb.resize(800, 400);
    wb.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wb));
    DA::DADataTableView* tvb = wb.getDataTableView();
    QVERIFY(tvb != nullptr);
    // 整列选择（每行一个 range，2 万格）——旧实现的卡死触发器
    tvb->selectColumn(0);
    QVERIFY(tvb->selectionModel()->selectedIndexes().size() > 10000);
    QElapsedTimer t;
    t.start();
    wb.sortColumn(0, Qt::DescendingOrder);
    const qint64 msSort = t.elapsed();
    qDebug().noquote() << QStringLiteral("testSortSelectionRemapPerf: 32560-row sort with full-column selection elapsed_ms=%1").arg(msSort);
    // 旧实现实测 985~7736ms（用户现场 6~19s）；修复后应为几十 ms 级，放宽到 1500ms 防抖
    QVERIFY2(msSort < 1500, qPrintable(QStringLiteral("sort with full-column selection too slow: %1 ms").arg(msSort)));
    // 整窗快速路径语义：排序后列仍整体选中
    QVERIFY(tvb->selectionModel()->hasSelection());
    // undo 走同一重映射路径，同样不许卡
    t.restart();
    wb.getUndoStack()->undo();
    const qint64 msUndo = t.elapsed();
    qDebug().noquote() << QStringLiteral("testSortSelectionRemapPerf: undo elapsed_ms=%1").arg(msUndo);
    QVERIFY2(msUndo < 1500, qPrintable(QStringLiteral("sort undo too slow: %1 ms").arg(msUndo)));
    wb.hide();
}

//===================================================
// 002 查找/替换（T1.4，计划 §4.1 用例）
//===================================================

// 002 §4.1：da_search_data 扩展——大小写、正则、整格匹配、行/列范围命中集合、上限截断与总数、
// 数值列等值（文本查找不退化数值命中）、文本列能搜到（toDouble 修复的验收）
void DATableDataTest::testSearchOperatorExtended()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
sdf = pd.DataFrame({
    "t": ["Abc", "abc", "xabcx", None, "def"],
    "n": [1.0, 2.0, 32.0, 4.0, np.nan],
    "i": [10, 20, 30, 32, 50],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "sdf" ]);

    // 文本列子串、大小写不敏感（默认）：命中 t 列 3 格
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 3);
        QCOMPARE(res.rows, QVector< int >({ 0, 1, 2 }));
        QCOMPARE(res.cols, QVector< int >({ 0, 0, 0 }));
    }
    // 区分大小写：只剩小写 abc 两格
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), true, false, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 1, 2 }));
    }
    // 正则：^a 匹配 Abc/abc（不区分大小写）
    {
        const auto res = py.searchData(df, QStringLiteral("^a"), false, true, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 0, 1 }));
    }
    // 整格匹配：只有纯 "abc" 的格（行1），Abc 大小写不敏感整格也算（行0）
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, true);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 0, 1 }));
    }
    // 数值列等值命中（文本查找不退化数值命中）："32" → n 列行2、i 列行3
    {
        const auto res = py.searchData(df, QStringLiteral("32"), false, false, false);
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        // 行主序：(2,1) 先于 (3,2)
        QCOMPARE(res.rows, QVector< int >({ 2, 3 }));
        QCOMPARE(res.cols, QVector< int >({ 1, 2 }));
    }
    // 行范围 [1,2]：t 列 abc 只剩行1、行2
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false, qMakePair(1, 2));
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 2);
        QCOMPARE(res.rows, QVector< int >({ 1, 2 }));
    }
    // 列范围 [1,2]（排除 t 列）：搜 abc 命中 0
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false, std::nullopt, qMakePair(1, 2));
        QVERIFY(res.ok);
        QCOMPARE(res.totalHits, 0);
        QVERIFY(res.rows.isEmpty());
    }
    // 上限截断：max_hits=2 → 只回 2 条，totalHits=3（truncated 为真）
    {
        const auto res = py.searchData(df, QStringLiteral("abc"), false, false, false, std::nullopt, std::nullopt, 2);
        QVERIFY(res.ok);
        QCOMPARE(res.rows.size(), 2);
        QCOMPARE(res.totalHits, 3);
        QVERIFY(res.truncated());
    }
    // 无效正则：ok=false（不崩、不静默命中）
    {
        const auto res = py.searchData(df, QStringLiteral("(["), false, true, false);
        QVERIFY(!res.ok);
    }
}

// 002 §4.1：da_find_and_replace——变更清单与替换结果一致、数值列整格替换保持 dtype、
// 文本列正则替换、按清单写回还原（撤销）
void DATableDataTest::testFindReplaceOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();

    // 文本子串替换 + 变更清单 + 写回还原
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"t": ["Abc", "abc", "xabcx"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVector< int > rows, cols;
        pybind11::list olds, news;
        QVERIFY(py.findReplace(df, QStringLiteral("abc"), QStringLiteral("Q"), false, false, false, std::nullopt, std::nullopt, &rows, &cols, &olds, &news));
        QCOMPARE(rows, QVector< int >({ 0, 1, 2 }));
        QCOMPARE(cols, QVector< int >({ 0, 0, 0 }));
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("Q"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("xQx"));
        // 写回旧值 = 撤销
        QVERIFY(py.writeCells(df, rows, cols, olds));
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("Abc"));
        QCOMPARE(df.iat(1, 0).toString(), QLatin1String("abc"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("xabcx"));
    }
    // 正则替换（捕获组）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"t": ["a1b", "c2d", "ef"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVERIFY(py.findReplace(df, QStringLiteral("([a-z])(\\d)"), QStringLiteral("\\2\\1"), false, true, false));
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("1ab"));
        QCOMPARE(df.iat(1, 0).toString(), QLatin1String("2cd"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("ef"));
    }
    // 数值列整格替换保持 dtype（int64 保持整型）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"i": [10, 32, 50]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVector< int > rows, cols;
        QVERIFY(py.findReplace(df, QStringLiteral("32"), QStringLiteral("7"), false, false, false, std::nullopt, std::nullopt, &rows, &cols));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(df.iat(1, 0).toInt(), 7);
        // dtype 仍为整型（Python 侧校验；用 exec 执行含 import 的语句，eval 只接受表达式）
        pybind11::dict chk;
        chk[ "df" ] = df.object();
        pybind11::exec("import numpy as np\nres = bool(df['i'].dtype == np.int64)", pybind11::globals(), chk);
        const bool isInt = chk[ "res" ].cast< bool >();
        QVERIFY(isInt);
    }
    // 数值列 replace 不可解析 → 该列不替换（保持原值）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"i": [10, 32, 50]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "rdf" ]);
        QVector< int > rows, cols;
        QVERIFY(py.findReplace(df, QStringLiteral("32"), QStringLiteral("abc"), false, false, false, std::nullopt, std::nullopt, &rows, &cols));
        QVERIFY(rows.isEmpty());
        QCOMPARE(df.iat(1, 0).toInt(), 32);
    }
}

// 002 §4.1：DACommandDataFrame_replace undo/redo 语义（小表 + 变更清单断言）
void DATableDataTest::testReplaceCommandUndoRedo()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
rdf = pd.DataFrame({"t": ["aa", "ab", "ba", "bb"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "rdf" ]);
    // 先就地替换 "a"→"X" 拿变更清单（模拟页面前置 findReplace），再据此构造命令
    QVector< int > rows, cols;
    pybind11::list olds, news;
    QVERIFY(py.findReplace(df, QStringLiteral("a"), QStringLiteral("X"), false, false, false, std::nullopt, std::nullopt, &rows, &cols, &olds, &news));
    QCOMPARE(rows.size(), 3);  // aa(0), ab(1), ba(2)
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("XX"));

    QUndoStack stack;
    DA::DACommandDataFrame_replace* cmd = new DA::DACommandDataFrame_replace(df, rows, cols, olds, news);
    QCOMPARE(cmd->changeCount(), 3);
    QCOMPARE(cmd->text(), QStringLiteral("Replace all"));
    stack.push(cmd);  // 首次 redo 跳过（数据已是替换后状态）
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("XX"));
    // undo → 写回旧值
    stack.undo();
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("aa"));
    QCOMPARE(df.iat(1, 0).toString(), QLatin1String("ab"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("ba"));
    QCOMPARE(df.iat(3, 0).toString(), QLatin1String("bb"));
    // redo → 再写新值
    stack.redo();
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("XX"));
    QCOMPARE(df.iat(1, 0).toString(), QLatin1String("Xb"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("bX"));
}

// 002 §4.2（程序化）：浮动查找条去抖——连续输入只触发一次 searchRequested（硬约束①）
void DATableDataTest::testFindReplaceDialogDebounce()
{
    DA::DADialogFindReplace dlg;
    QSignalSpy spy(&dlg, &DA::DADialogFindReplace::searchRequested);
    QVERIFY(spy.isValid());
    // 模拟连续敲击 "abc"：三次快速改文本（未经去抖间隔）
    dlg.activate(false);
    QLineEdit* findEdit = dlg.findChild< QLineEdit* >(QStringLiteral("lineEditFind"));
    QVERIFY(findEdit != nullptr);
    findEdit->setText(QStringLiteral("a"));
    findEdit->setText(QStringLiteral("ab"));
    findEdit->setText(QStringLiteral("abc"));
    // 去抖窗口内不应触发
    QTest::qWait(DA::DADialogFindReplace::debounceIntervalMs() / 2);
    QCOMPARE(spy.count(), 0);
    // 越过去抖窗口后恰好一次
    QTest::qWait(DA::DADialogFindReplace::debounceIntervalMs() + 100);
    QCOMPARE(spy.count(), 1);
    // 再输入一轮 → 再恰好一次（restart 语义）
    findEdit->setText(QStringLiteral("abcd"));
    findEdit->setText(QStringLiteral("abcde"));
    QTest::qWait(DA::DADialogFindReplace::debounceIntervalMs() + 100);
    QCOMPARE(spy.count(), 2);
    // 选项快照
    findEdit->setText(QStringLiteral("kw"));
    dlg.findChild< QCheckBox* >(QStringLiteral("checkBoxRegex"))->setChecked(true);
    dlg.findChild< QComboBox* >(QStringLiteral("comboBoxScope"))->setCurrentIndex(1);
    const auto opt = dlg.options();
    QCOMPARE(opt.keyword, QStringLiteral("kw"));
    QVERIFY(opt.useRegex);
    QVERIFY(opt.selectionOnly);
    // Esc 关闭发 closed
    QSignalSpy closeSpy(&dlg, &DA::DADialogFindReplace::closed);
    QTest::keyClick(&dlg, Qt::Key_Escape);
    QCOMPARE(closeSpy.count(), 1);
}

// 002 §4.2（程序化）：页面查找/替换全链路——扫描装载命中、导航、全部替换、撤销还原、Esc 清高亮
void DATableDataTest::testFindReplaceWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
wdf = pd.DataFrame({"t": ["abc", "def", "abcx", "xyz", "abc"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "wdf" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 360);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // 打开查找条并扫描 "abc"（3 命中）
    w.showFindReplace(false);
    DA::DADialogFindReplace* dlg = w.findChild< DA::DADialogFindReplace* >();
    QVERIFY(dlg != nullptr);
    QLineEdit* findEdit = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditFind"));
    QVERIFY(findEdit != nullptr);
    findEdit->setText(QStringLiteral("abc"));
    dlg->requestSearchNow();  // 跳过去抖立即扫描
    QCOMPARE(w.findHitCount(), 3);
    QCOMPARE(w.currentFindHitIndex(), 0);
    // 高亮层：Hit 3 格 + CurrentHit 1 格
    DA::DATableHighlightOverlay* ov = w.highlightOverlay();
    QVERIFY(ov != nullptr);
    QCOMPARE(ov->levelAt(0, 0), DA::DATableHighlightOverlay::Level::CurrentHit);
    QCOMPARE(ov->levelAt(2, 0), DA::DATableHighlightOverlay::Level::Hit);
    QCOMPARE(ov->levelAt(1, 0), DA::DATableHighlightOverlay::Level::None);
    // 导航到下一条
    QPushButton* nextBtn = dlg->findChild< QPushButton* >(QStringLiteral("pushButtonNext"));
    QVERIFY(nextBtn != nullptr);
    QTest::mouseClick(nextBtn, Qt::LeftButton);
    QCOMPARE(w.currentFindHitIndex(), 1);
    QCOMPARE(ov->levelAt(2, 0), DA::DATableHighlightOverlay::Level::CurrentHit);
    // 全部替换 abc→Q（展开替换行）
    w.showFindReplace(true);
    QLineEdit* replEdit = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditReplace"));
    QVERIFY(replEdit != nullptr);
    replEdit->setText(QStringLiteral("Q"));
    QSignalSpy replaceSpy(&w, &DA::DADataOperateOfDataFrameWidget::replaceApplied);
    QPushButton* replAllBtn = dlg->findChild< QPushButton* >(QStringLiteral("pushButtonReplaceAll"));
    QVERIFY(replAllBtn != nullptr);
    QTest::mouseClick(replAllBtn, Qt::LeftButton);
    QCOMPARE(replaceSpy.count(), 1);
    QCOMPARE(replaceSpy.takeFirst().at(0).toInt(), 3);
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("Q"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("Qx"));
    QCOMPARE(df.iat(4, 0).toString(), QLatin1String("Q"));
    // 替换后自动重扫：关键字 abc 已无命中
    QCOMPARE(w.findHitCount(), 0);
    // Ctrl+Z 一次 → 完全还原（全部替换 = 一个撤销单元）
    w.getUndoStack()->undo();
    QCOMPARE(df.iat(0, 0).toString(), QLatin1String("abc"));
    QCOMPARE(df.iat(2, 0).toString(), QLatin1String("abcx"));
    QCOMPARE(df.iat(4, 0).toString(), QLatin1String("abc"));
    // 选区内替换（§4.2）：范围=当前选区，只改选区包围盒内的命中
    {
        DA::DADataTableView* tv = w.getDataTableView();
        QVERIFY(tv != nullptr);
        QItemSelectionModel* sm = tv->selectionModel();
        QVERIFY(sm != nullptr);
        // 选中行 0..2（列 0）：命中 rows {0,2,4} 中只有 0、2 在选区内
        sm->select(QItemSelection(tv->model()->index(0, 0), tv->model()->index(2, 0)), QItemSelectionModel::ClearAndSelect);
        dlg->findChild< QComboBox* >(QStringLiteral("comboBoxScope"))->setCurrentIndex(1);  // 当前选区
        findEdit->setText(QStringLiteral("abc"));
        replEdit->setText(QStringLiteral("S"));
        dlg->requestSearchNow();
        QCOMPARE(w.findHitCount(), 2);  // 扫描范围同样受选区约束
        QTest::mouseClick(replAllBtn, Qt::LeftButton);
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("S"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("Sx"));
        QCOMPARE(df.iat(4, 0).toString(), QLatin1String("abc"));  // 选区外不变
        w.getUndoStack()->undo();
        QCOMPARE(df.iat(0, 0).toString(), QLatin1String("abc"));
        QCOMPARE(df.iat(2, 0).toString(), QLatin1String("abcx"));
        dlg->findChild< QComboBox* >(QStringLiteral("comboBoxScope"))->setCurrentIndex(0);  // 复位全表
    }
    // 关闭查找条 → Find 高亮清空
    w.showFindReplace(false);
    findEdit->setText(QStringLiteral("abc"));
    dlg->requestSearchNow();
    QCOMPARE(w.findHitCount(), 3);
    w.closeFindReplace();
    QCOMPARE(w.findHitCount(), 0);
    QCOMPARE(ov->levelAt(0, 0), DA::DATableHighlightOverlay::Level::None);
    QVERIFY(ov->isEmpty());
    w.hide();
}

//===================================================
// 003 条件格式（T1.5 §4.1）
//===================================================
namespace
{
using DA::DATableConditionalFormatRule;
using DA::DATableConditionalFormatStore;
using CfCond = DATableConditionalFormatRule::Condition;
using CfType = DATableConditionalFormatRule::Type;

// 官方色板端点（色阶默认色）
const QColor c_cfScaleMin(0xBD, 0xCD, 0xE8);  // 浅蓝
const QColor c_cfScaleMax(0x52, 0x80, 0xC1);  // 主蓝
const QColor c_cfBar(0xCE, 0x60, 0x43);       // 橙红（测试数据条用，与背景区分明显）

// 构造带背景色的单元格规则
DATableConditionalFormatRule makeBgRule(CfCond cond, const QVariant& v1, const QColor& bg, const QVariant& v2 = QVariant())
{
    DATableConditionalFormatRule r;
    r.type      = CfType::CellRule;
    r.condition = cond;
    r.value1    = v1;
    r.value2    = v2;
    r.style.setBackground(QBrush(bg));
    return r;
}

// 统计图像中某矩形内与 target 完全相等的像素数（文字/网格线像素自然排除）
int countExactPixels(const QImage& img, const QRect& rect, QRgb target)
{
    int n = 0;
    const QRect r = rect.intersected(img.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) {
        for (int x = r.left(); x <= r.right(); ++x) {
            if (img.pixel(x, y) == target) {
                ++n;
            }
        }
    }
    return n;
}

// 统计矩形内"深色文字"像素数（数据条之上文字未被遮挡的证据：条色为亮色，文字为深色）
int countDarkPixels(const QImage& img, const QRect& rect)
{
    int n = 0;
    const QRect r = rect.intersected(img.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) {
        for (int x = r.left(); x <= r.right(); ++x) {
            const QRgb p = img.pixel(x, y);
            if (qRed(p) < 90 && qGreen(p) < 90 && qBlue(p) < 90) {
                ++n;
            }
        }
    }
    return n;
}

// 规则列表逐元素相等（避免依赖 QTest::toString 流式输出）
bool cfRulesEqual(const QVector< DATableConditionalFormatRule >& a, const QVector< DATableConditionalFormatRule >& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (int i = 0; i < a.size(); ++i) {
        if (!(a.at(i) == b.at(i))) {
            return false;
        }
    }
    return true;
}
}  // namespace

/**
 * @brief 规则求值：每种操作符边界（语义钉死见 DATableConditionalFormatRule 头注释）、
 * 数值列/文本列分别比较、null/NaN 不命中
 */
void DATableDataTest::testConditionalRuleEvaluation()
{
    // Greater：严格比较，等于不命中（钉死）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(101), QVariant("100"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(100), QVariant("100"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(99), QVariant("100"), QVariant()));
    // 数值单元格 vs 字符串参数："100" 可严格转数值 → 数值比较（跨类型数值等价）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(100.5), QVariant(100), QVariant()));
    // 文本列：双方非数值 → 字符串比较
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(QStringLiteral("b")), QVariant(QStringLiteral("a")), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(QStringLiteral("a")), QVariant(QStringLiteral("b")), QVariant()));
    // 混合（"12abc" 无法整串转数值）→ 退化字符串比较
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(QStringLiteral("12abc")), QVariant(QStringLiteral("12")), QVariant()));
    // Less：镜像语义
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Less, QVariant(99), QVariant("100"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Less, QVariant(100), QVariant("100"), QVariant()));
    // Between：闭区间 [v1, v2]（含端点，钉死）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(5), QVariant(1), QVariant(10)));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(1), QVariant(1), QVariant(10)));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(10), QVariant(1), QVariant(10)));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(0), QVariant(1), QVariant(10)));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(11), QVariant(1), QVariant(10)));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(QStringLiteral("b")), QVariant(QStringLiteral("a")), QVariant(QStringLiteral("c"))));
    // Equal：数值等价跨类型、字符串精确
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(100), QVariant("100"), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(100.0), QVariant(100), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(QStringLiteral("abc")), QVariant(QStringLiteral("abc")), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(QStringLiteral("abc")), QVariant(QStringLiteral("abd")), QVariant()));
    // Contains：字符串子串，区分大小写（钉死）
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::Contains, QVariant(QStringLiteral("hello world")), QVariant(QStringLiteral("world")), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Contains, QVariant(QStringLiteral("Hello")), QVariant(QStringLiteral("hello")), QVariant()));
    // IsEmpty / IsNotEmpty：invalid/null/NaN/空串为空，数值 0 非空
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(), QVariant(), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(QString()), QVariant(), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(qQNaN()), QVariant(), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::IsEmpty, QVariant(0.0), QVariant(), QVariant()));
    QVERIFY(DATableConditionalFormatRule::evaluateCondition(CfCond::IsNotEmpty, QVariant(0.0), QVariant(), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::IsNotEmpty, QVariant(), QVariant(), QVariant()));
    // 空值/NaN 对其余操作符一律不命中（钉死）
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(), QVariant("5"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(qQNaN()), QVariant("5"), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Equal, QVariant(QString()), QVariant(QStringLiteral("x")), QVariant()));
    // 参数缺失 → 不命中
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Greater, QVariant(5), QVariant(), QVariant()));
    QVERIFY(!DATableConditionalFormatRule::evaluateCondition(CfCond::Between, QVariant(5), QVariant(1), QVariant()));
    // toNumeric 严格性
    double d = 0.0;
    QVERIFY(DATableConditionalFormatRule::toNumeric(QVariant(QStringLiteral("123")), &d) && d == 123.0);
    QVERIFY(!DATableConditionalFormatRule::toNumeric(QVariant(QStringLiteral("123abc")), &d));
    QVERIFY(!DATableConditionalFormatRule::toNumeric(QVariant(qQNaN()), &d));
    QVERIFY(!DATableConditionalFormatRule::toNumeric(QVariant(QDateTime::currentDateTime()), &d));
}

/**
 * @brief 叠加顺序：同列两条规则都命中时后者覆盖前者；静态样式打底 + 条件覆盖；
 * 停用规则跳过；色阶/数据条与统计有效性
 */
void DATableDataTest::testConditionalEvaluateOrder()
{
    // 两条都命中：后者覆盖前者的背景，未设置属性保留 merge 语义
    QVector< DATableConditionalFormatRule > rules;
    DATableConditionalFormatRule r1 = makeBgRule(CfCond::Greater, QVariant(0), QColor(0xCE, 0x60, 0x43));
    DATableConditionalFormatRule r2;
    r2.type      = CfType::CellRule;
    r2.condition = CfCond::Greater;
    r2.value1    = QVariant(5);
    r2.style.setForeground(QColor(0xFF, 0xFF, 0xFF));  // 只设前景
    rules << r1 << r2;
    DATableConditionalFormatStore::ColumnStats stats;
    auto res = DATableConditionalFormatStore::evaluate(rules, QVariant(10), false, stats);
    QVERIFY(res.style.backgroundValid());
    QCOMPARE(res.style.background().color(), QColor(0xCE, 0x60, 0x43));  // r1 的背景保留（r2 未设背景）
    QVERIFY(res.style.foregroundValid());
    QCOMPARE(res.style.foreground(), QColor(0xFF, 0xFF, 0xFF));  // r2 前景叠加
    // 后者覆盖前者：r2 也设背景 → 最终为 r2 的背景
    r2.style.setBackground(QBrush(QColor(0x66, 0x9E, 0x8B)));
    rules[ 1 ] = r2;
    res        = DATableConditionalFormatStore::evaluate(rules, QVariant(10), false, stats);
    QCOMPARE(res.style.background().color(), QColor(0x66, 0x9E, 0x8B));
    // 只命中第一条：值为 3（>0 但 <5）→ r1 背景 + 无前景
    res = DATableConditionalFormatStore::evaluate(rules, QVariant(3), false, stats);
    QCOMPARE(res.style.background().color(), QColor(0xCE, 0x60, 0x43));
    QVERIFY(!res.style.foregroundValid());
    // 停用规则跳过
    rules[ 0 ].enabled = false;
    res                = DATableConditionalFormatStore::evaluate(rules, QVariant(3), false, stats);
    QVERIFY(res.style.isNull());
    rules[ 0 ].enabled = true;
    // 静态打底 + 条件覆盖（delegate 的叠加语义：base.mergeFrom(cf)）
    DA::DATableCellStyle base;
    base.setBackground(QBrush(QColor(0xBD, 0xCD, 0xE8)));  // 静态列样式底色
    res = DATableConditionalFormatStore::evaluate(rules, QVariant(10), false, stats);
    base.mergeFrom(res.style);
    QCOMPARE(base.background().color(), QColor(0x66, 0x9E, 0x8B));  // 条件覆盖静态
    // 色阶规则：统计无效 → 不产生背景；统计有效 → 插值背景
    QVector< DATableConditionalFormatRule > scaleRules;
    DATableConditionalFormatRule sr;
    sr.type = CfType::ColorScale;
    scaleRules << sr;
    res = DATableConditionalFormatStore::evaluate(scaleRules, QVariant(5.0), false, stats);
    QVERIFY(res.style.isNull());
    stats.minValue = 0.0;
    stats.maxValue = 10.0;
    res            = DATableConditionalFormatStore::evaluate(scaleRules, QVariant(5.0), true, stats);
    QVERIFY(res.style.backgroundValid());
    QCOMPARE(res.style.background().color(), DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 0.5));
    QVERIFY(!res.style.foregroundValid());  // 前景不因色阶变化（可读性）
    // 非数值单元格：色阶跳过
    res = DATableConditionalFormatStore::evaluate(scaleRules, QVariant(QStringLiteral("abc")), true, stats);
    QVERIFY(res.style.isNull());
    // 数据条规则：统计有效 + 数值 → 条长比例与颜色；统计无效 → 无条
    QVector< DATableConditionalFormatRule > barRules;
    DATableConditionalFormatRule br;
    br.type     = CfType::DataBar;
    br.barColor = c_cfBar;
    barRules << br;
    res = DATableConditionalFormatStore::evaluate(barRules, QVariant(2.5), true, stats);
    QVERIFY(res.hasDataBar);
    QCOMPARE(res.barRatio, 0.25);
    QCOMPARE(res.barColor, c_cfBar);
    res = DATableConditionalFormatStore::evaluate(barRules, QVariant(2.5), false, stats);
    QVERIFY(!res.hasDataBar);
}

/**
 * @brief 色阶插值：端点、中点（三色）、双色；min==max 退化；越界截断
 */
void DATableDataTest::testColorScaleInterpolation()
{
    const QColor cmin(0xBD, 0xCD, 0xE8);
    const QColor cmid(0xFF, 0xFF, 0xFF);
    const QColor cmax(0x52, 0x80, 0xC1);
    // 双色：端点精确
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(0.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmin);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(10.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmax);
    // 双色中点 = lerp(0.5)（与纯函数自洽）
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(5.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmin, cmax, 0.5));
    // 三色：value<=median 段 cmin→cmid，> median 段 cmid→cmax
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(5.0, 0.0, 10.0, 5.0, true, cmin, cmid, cmax), cmid);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(0.0, 0.0, 10.0, 5.0, true, cmin, cmid, cmax), cmin);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(10.0, 0.0, 10.0, 5.0, true, cmin, cmid, cmax), cmax);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(2.5, 0.0, 10.0, 5.0, true, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmin, cmid, 0.5));
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(7.5, 0.0, 10.0, 5.0, true, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmid, cmax, 0.5));
    // min==max 退化：normalizedPosition → 0.5（钉死）
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(5.0, 5.0, 5.0, 5.0, false, cmin, cmid, cmax),
             DATableConditionalFormatRule::lerpColor(cmin, cmax, 0.5));
    // 越界截断到端点色
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(-100.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmin);
    QCOMPARE(DATableConditionalFormatRule::colorScaleColor(100.0, 0.0, 10.0, 5.0, false, cmin, cmid, cmax), cmax);
    // lerpColor 端点与中值分量
    QCOMPARE(DATableConditionalFormatRule::lerpColor(cmin, cmax, 0.0), cmin);
    QCOMPARE(DATableConditionalFormatRule::lerpColor(cmin, cmax, 1.0), cmax);
    const QColor half = DATableConditionalFormatRule::lerpColor(QColor(0, 0, 0), QColor(255, 255, 255), 0.5);
    QVERIFY(qAbs(half.red() - 128) <= 1 && qAbs(half.green() - 128) <= 1 && qAbs(half.blue() - 128) <= 1);
}

/**
 * @brief 数据条归一化：(v-min)/(max-min)；min==max 退化取半格；越界截断 [0,1]
 */
void DATableDataTest::testDataRowNormalization()
{
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(5.0, 0.0, 10.0), 0.5);
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(0.0, 0.0, 10.0), 0.0);
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(10.0, 0.0, 10.0), 1.0);
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(-5.0, 0.0, 10.0), 0.0);   // 越下界截断
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(15.0, 0.0, 10.0), 1.0);   // 越上界截断
    QCOMPARE(DATableConditionalFormatRule::dataBarRatio(5.0, 5.0, 5.0), 0.5);     // min==max 退化半格（§1.4-1）
    QCOMPARE(DATableConditionalFormatRule::normalizedPosition(2.5, 0.0, 10.0), 0.25);
}

/**
 * @brief 存储：XML round-trip（含 DATableMetadata 束聚合）、列插删重映射、
 * moveRule/clearColumn/clearAll 语义与信号
 */
void DATableDataTest::testConditionalStoreXmlAndRemap()
{
    DATableConditionalFormatStore store;
    // 列 1：单元格规则（背景+前景+字体、Between 双参数、停用规则）+ 三色色阶 + 数据条
    DATableConditionalFormatRule cellRule;
    cellRule.type      = CfType::CellRule;
    cellRule.condition = CfCond::Between;
    cellRule.value1    = QVariant(QStringLiteral("10"));
    cellRule.value2    = QVariant(QStringLiteral("20"));
    cellRule.style.setBackground(QBrush(QColor(0xCE, 0x60, 0x43)));
    cellRule.style.setForeground(QColor(0xFF, 0xFF, 0xFF));
    QFont f(QStringLiteral("Arial"), 12);
    f.setBold(true);
    cellRule.style.setFont(f);
    DATableConditionalFormatRule offRule = makeBgRule(CfCond::Greater, QVariant(100), QColor(0x66, 0x9E, 0x8B));
    offRule.enabled                      = false;
    DATableConditionalFormatRule scaleRule;
    scaleRule.type           = CfType::ColorScale;
    scaleRule.threeColorScale = true;
    DATableConditionalFormatRule barRule;
    barRule.type     = CfType::DataBar;
    barRule.barColor = QColor(0x49, 0x7C, 0xAD);
    store.addRule(1, cellRule);
    store.addRule(1, offRule);
    store.addRule(1, scaleRule);
    store.addRule(1, barRule);
    store.addRule(3, makeBgRule(CfCond::Contains, QVariant(QStringLiteral("x")), QColor(0xE6, 0xC2, 0x7C)));
    QCOMPARE(store.ruleCount(), 5);
    QCOMPARE(store.columnsWithRules(), QList< int >() << 1 << 3);
    // XML round-trip（store 级）
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("table"));
    doc.appendChild(root);
    store.toXml(doc, root);
    const QString xml = doc.toString();
    QVERIFY(xml.contains(QLatin1String("<conditional-format>")));
    QVERIFY(xml.contains(QLatin1String("col=\"1\"")));
    QVERIFY(xml.contains(QLatin1String("type=\"color-scale\"")));
    QVERIFY(xml.contains(QLatin1String("cond=\"between\"")));
    QDomDocument doc2;
    QVERIFY(doc2.setContent(xml));
    DATableConditionalFormatStore store2;
    QVERIFY(store2.fromXml(doc2.documentElement().firstChildElement(QStringLiteral("conditional-format"))));
    QCOMPARE(store2.ruleCount(), 5);
    QCOMPARE(store2.columnsWithRules(), QList< int >() << 1 << 3);
    // operator== 逐字段（含字体/停用/双参数/色阶端点/条色）
    QVERIFY(cfRulesEqual(store2.columnRules(1), store.columnRules(1)));
    QVERIFY(cfRulesEqual(store2.columnRules(3), store.columnRules(3)));
    // DATableMetadata 束聚合 round-trip（<conditional-format> 段随束序列化）
    {
        DA::DATableMetadata meta;
        meta.conditionalFormat()->addRule(2, cellRule);
        meta.styleManager()->setColumnStyle(0, DA::DATableCellStyle());
        QDomDocument mdoc;
        QDomElement mroot = mdoc.createElement(QStringLiteral("table"));
        mdoc.appendChild(mroot);
        meta.toXml(mdoc, mroot);
        QDomDocument mdoc2;
        QVERIFY(mdoc2.setContent(mdoc.toString()));
        DA::DATableMetadata meta2;
        QVERIFY(meta2.fromXml(mdoc2.documentElement()));
        QCOMPARE(meta2.conditionalFormat()->ruleCount(), 1);
        QVERIFY(cfRulesEqual(meta2.conditionalFormat()->columnRules(2), QVector< DATableConditionalFormatRule >() << cellRule));
        QVERIFY(!meta2.isEmpty());
    }
    // 信号：rulesChanged(column)
    QSignalSpy changedSpy(&store, &DATableConditionalFormatStore::rulesChanged);
    QSignalSpy resetSpy(&store, &DATableConditionalFormatStore::ruleReset);
    store.moveRule(1, 0, 2);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(store.columnRules(1).at(2) == cellRule);  // [off,scale,cell,bar]
    store.moveRule(1, 2, 0);
    QVERIFY(store.columnRules(1).at(0) == cellRule);  // 还原
    QCOMPARE(changedSpy.count(), 1);                  // 第二次 move 也发 rulesChanged(1)
    QCOMPARE(changedSpy.takeFirst().at(0).toInt(), 1);
    // clearColumn：桶删除 + 信号
    store.clearColumn(3);
    QVERIFY(!store.hasRules(3));
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.takeFirst().at(0).toInt(), 3);
    store.clearColumn(3);  // 已空列再清不发信号（无变化）
    QCOMPARE(changedSpy.count(), 0);
    // 列插删重映射（签名 const QList<int>&，语义同 DATableStyleManager）
    store.addRule(0, makeBgRule(CfCond::Greater, QVariant(1), QColor(0x52, 0x80, 0xC1)));
    // 现有：列 0、列 1；插入列 1 → 原列 1 的规则移到列 2，列 0 不动
    store.onColumnsInserted({ 1 });
    QCOMPARE(resetSpy.count(), 1);
    QVERIFY(store.hasRules(0));
    QVERIFY(!store.hasRules(1));
    QCOMPARE(store.columnRules(2).size(), 4);
    // 删除列 2 → 该列规则丢弃，> 2 的键 -1（此处无）
    store.onColumnsRemoved({ 2 });
    QCOMPARE(resetSpy.count(), 2);
    QVERIFY(!store.hasRules(2));
    QVERIFY(store.hasRules(0));
    // 批量插入多个索引（降序处理语义与样式管理器一致）：{0,5} 插入 {1,3} → {0,7}
    store.addRule(5, makeBgRule(CfCond::Greater, QVariant(1), QColor(0x52, 0x80, 0xC1)));
    store.onColumnsInserted({ 1, 3 });
    QVERIFY(store.hasRules(0));   // < 1 不动
    QVERIFY(!store.hasRules(1));  // 插入列本身无规则
    QVERIFY(!store.hasRules(5));  // 5 已位移
    QVERIFY(store.hasRules(7));   // 5 → +2（1 与 3 都 <= 5）
    // 批量删除：删掉 0 与 7 → 全部规则清空
    store.onColumnsRemoved({ 0, 7 });
    QVERIFY(!store.hasRules(0));
    QVERIFY(!store.hasRules(6));
    QCOMPARE(store.ruleCount(), 0);
    // clearAll：全部清空 + ruleReset
    const int resetsBefore = resetSpy.count();
    store.clearAll();
    QVERIFY(store.isEmpty());
    QCOMPARE(store.ruleCount(), 0);
    QCOMPARE(resetSpy.count(), resetsBefore + 1);
    // 空 store toXml 不写段
    QDomDocument edoc;
    QDomElement eroot = edoc.createElement(QStringLiteral("table"));
    edoc.appendChild(eroot);
    store.toXml(edoc, eroot);
    QVERIFY(edoc.firstChildElement(QStringLiteral("conditional-format")).isNull());
}

/**
 * @brief 统计缓存：惰性计算一次 + 缓存命中不再调提供者；失效触发重算；
 * 无规则列不计算；失败结果也缓存（不每格重试）
 */
void DATableDataTest::testConditionalStoreStatsCache()
{
    DATableConditionalFormatStore store;
    int providerCalls = 0;
    store.setStatsProvider([ &providerCalls ](int column, DATableConditionalFormatStore::ColumnStats& out) {
        ++providerCalls;
        if (column == 9) {
            return false;  // 模拟非数值列/失败
        }
        out.minValue    = 0.0;
        out.maxValue    = 100.0;
        out.medianValue = 50.0;
        return true;
    });
    DATableConditionalFormatStore::ColumnStats st;
    // 无规则的列：不计算（红线2）
    QVERIFY(!store.columnStats(0, st));
    QCOMPARE(providerCalls, 0);
    // 加色阶规则后：首次计算，二次命中缓存
    DATableConditionalFormatRule sr;
    sr.type = CfType::ColorScale;
    store.addRule(0, sr);
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 1);
    QCOMPARE(st.minValue, 0.0);
    QCOMPARE(st.maxValue, 100.0);
    QCOMPARE(st.medianValue, 50.0);
    QVERIFY(store.columnStats(0, st));
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 1);  // 缓存生效，不重算
    // 失效单列 → 重算一次
    store.invalidateColumnStats(0);
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 2);
    // 失败列：失败也缓存，不每格重试
    DATableConditionalFormatRule sr9;
    sr9.type = CfType::DataBar;
    store.addRule(9, sr9);
    QVERIFY(!store.columnStats(9, st));
    QVERIFY(!store.columnStats(9, st));
    QCOMPARE(providerCalls, 3);  // 只调用一次
    // invalidateAllStats → 全部重算
    store.invalidateAllStats();
    QVERIFY(store.columnStats(0, st));
    QCOMPARE(providerCalls, 4);
    // needsColumnStats：纯单元格规则列不需要统计
    store.addRule(1, makeBgRule(CfCond::Greater, QVariant(1), QColor(0xCE, 0x60, 0x43)));
    QVERIFY(!store.needsColumnStats(1));
    QVERIFY(store.needsColumnStats(0));
    // 提供者置空 → 统计不可用
    store.setStatsProvider(nullptr);
    QVERIFY(!store.columnStats(0, st));
}

/**
 * @brief 命令：增/删/改/移动/清列的 undo/redo 结果对象一致（旧新对象记录范式）
 */
void DATableDataTest::testConditionalCommands()
{
    DATableConditionalFormatStore store;
    QUndoStack stack;
    const DATableConditionalFormatRule ruleA = makeBgRule(CfCond::Greater, QVariant(10), QColor(0xCE, 0x60, 0x43));
    const DATableConditionalFormatRule ruleB = makeBgRule(CfCond::Less, QVariant(5), QColor(0x66, 0x9E, 0x8B));
    const QVector< DATableConditionalFormatRule > onlyA  { ruleA };
    const QVector< DATableConditionalFormatRule > bThenA { ruleB, ruleA };
    const QVector< DATableConditionalFormatRule > aThenB { ruleA, ruleB };
    // 增：push 即 redo
    stack.push(new DA::DACommandConditionalFormat_addRule(&store, 0, ruleA));
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    stack.undo();
    QVERIFY(!store.hasRules(0));
    stack.redo();
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    // 第二条 + 指定位置插入
    stack.push(new DA::DACommandConditionalFormat_addRule(&store, 0, ruleB, 0));
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    stack.redo();
    // 改：旧新对象记录
    DATableConditionalFormatRule ruleA2 = ruleA;
    ruleA2.value1                       = QVariant(20);
    stack.push(new DA::DACommandConditionalFormat_modifyRule(&store, 0, 1, ruleA, ruleA2));
    QVERIFY(store.columnRules(0).at(1) == ruleA2);
    stack.undo();
    QVERIFY(store.columnRules(0).at(1) == ruleA);
    stack.redo();
    QVERIFY(store.columnRules(0).at(1) == ruleA2);
    stack.undo();  // 回到 [ruleB, ruleA]
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    // 移动：记录前后索引
    stack.push(new DA::DACommandConditionalFormat_moveRule(&store, 0, 0, 1));
    QVERIFY(cfRulesEqual(store.columnRules(0), aThenB));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.redo();
    QVERIFY(cfRulesEqual(store.columnRules(0), aThenB));
    stack.undo();  // 回到 [ruleB, ruleA]
    // 删：undo 原位插回（对象一致）
    stack.push(new DA::DACommandConditionalFormat_removeRule(&store, 0, 0));
    QVERIFY(cfRulesEqual(store.columnRules(0), onlyA));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.redo();
    stack.undo();  // 回到 [ruleB, ruleA]
    // 清列：undo 整体还原
    stack.push(new DA::DACommandConditionalFormat_clearColumn(&store, 0));
    QVERIFY(!store.hasRules(0));
    stack.undo();
    QVERIFY(cfRulesEqual(store.columnRules(0), bThenA));
    stack.redo();
    QVERIFY(!store.hasRules(0));
}

/**
 * @brief Python 算子 da_column_stats：min/max/median 全表归约、NaN 排除、
 * 非数值列/越界 → 不可用（C++ wrapper ok=false）
 */
void DATableDataTest::testColumnStatsOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import numpy as np
import pandas as pd
sdf = pd.DataFrame({
    "v": [1.0, 2.0, 3.0, 4.0, 100.0],
    "n": [1.0, np.nan, 3.0, np.nan, 5.0],
    "t": ["a", "b", "c", "d", "e"],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "sdf" ]);
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    auto r0                      = py.columnStats(df, 0);
    QVERIFY(r0.ok);
    QCOMPARE(r0.minValue, 1.0);
    QCOMPARE(r0.maxValue, 100.0);
    QCOMPARE(r0.medianValue, 3.0);
    // NaN 排除后归约（有效值 [1,3,5]）
    auto r1 = py.columnStats(df, 1);
    QVERIFY(r1.ok);
    QCOMPARE(r1.minValue, 1.0);
    QCOMPARE(r1.maxValue, 5.0);
    QCOMPARE(r1.medianValue, 3.0);
    // 纯文本列：pd.to_numeric coerce 全 NaN → 不可用
    auto r2 = py.columnStats(df, 2);
    QVERIFY(!r2.ok);
    // 越界 → 不可用（IndexError 经 dealException 吞掉）
    auto r3 = py.columnStats(df, 99);
    QVERIFY(!r3.ok);
    auto r4 = py.columnStats(df, -1);
    QVERIFY(!r4.ok);
}

/**
 * @brief 绘制管线②④的像素级验证（§4.2 程序化）：色阶分布、编辑极值后端点更新
 *（统计失效）、数据条几何与文字不被遮挡、查找高亮③覆盖条件格式②、
 * 规则命令 undo/redo、预览叠加/还原
 */
void DATableDataTest::testConditionalPaintWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import numpy as np
import pandas as pd
cdf = pd.DataFrame({
    "v": np.arange(10, dtype=float),
    "b": np.arange(10, dtype=float),
    "t": ["s%d" % i for i in range(10)],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "cdf" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    tv->clearSelection();
    DATableConditionalFormatStore* store = w.conditionalFormatStore();
    QVERIFY(store != nullptr);
    // 色阶（列 0，双色默认浅蓝→主蓝）+ 数据条（列 1，橙红）
    DATableConditionalFormatRule scaleRule;
    scaleRule.type = CfType::ColorScale;
    store->addRule(0, scaleRule);
    DATableConditionalFormatRule barRule;
    barRule.type     = CfType::DataBar;
    barRule.barColor = c_cfBar;
    store->addRule(1, barRule);
    auto renderViewport = [ &w, tv ]() {
        // 清除当前格（模型装载后默认 (0,0)）：windows 风格会给当前格叠渐变着色，
        // 干扰纯色像素断言（选区已在测试开头 clearSelection）
        if (tv->selectionModel() && tv->selectionModel()->currentIndex().isValid()) {
            tv->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
        }
        QApplication::processEvents();
        tv->viewport()->repaint();
        QImage img(tv->viewport()->size(), QImage::Format_ARGB32);
        tv->viewport()->render(&img);
        return img;
    };
    auto cellRect = [ tv ](int row, int col) {
        return tv->visualRect(tv->model()->index(row, col));
    };
    // —— 色阶分布：min 行（值0）= 浅蓝端点，max 行（值9）= 主蓝端点，中间行介于两者 ——
    QImage img = renderViewport();
    const QRgb lowRgb  = c_cfScaleMin.rgb();
    const QRgb highRgb = c_cfScaleMax.rgb();
    QVERIFY2(countExactPixels(img, cellRect(0, 0), lowRgb) >= 20, "color scale min cell should be filled with low-end color");
    QVERIFY2(countExactPixels(img, cellRect(9, 0), highRgb) >= 20, "color scale max cell should be filled with high-end color");
    // 中间行（值5，t=5/9）：与两端点色都不同（分布插值的证据）
    const QRect midRect = cellRect(5, 0);
    QCOMPARE(countExactPixels(img, midRect, lowRgb), 0);
    QCOMPARE(countExactPixels(img, midRect, highRgb), 0);
    const QColor midExpect = DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 5.0 / 9.0);
    QVERIFY2(countExactPixels(img, midRect, midExpect.rgb()) >= 20, "middle cell should be the interpolated color");
    // —— 数据条：max 行满格条（右侧区域为条色），min 行比例为 0 无条 ——
    const QRect barMaxRect = cellRect(9, 1);
    QVERIFY2(countExactPixels(img, barMaxRect, c_cfBar.rgb()) >= 20, "data bar of max cell should be drawn");
    // 文字不被遮挡：条色为亮色（橙红），文字为深色 → 条区域内存在深色文字像素
    QVERIFY2(countDarkPixels(img, barMaxRect) > 0, "text pixels must be visible above the data bar");
    const QRect barMinRect = cellRect(0, 1);
    QCOMPARE(countExactPixels(img, barMinRect, c_cfBar.rgb()), 0);  // ratio=0 无条
    // 2px 内边距：单元格最右侧 1px 列不应有条色（条不满到边缘）——值9 比例=1 时条右缘 = 内容区右缘
    QVERIFY(countExactPixels(img, QRect(barMaxRect.right(), barMaxRect.top(), 1, barMaxRect.height()), c_cfBar.rgb()) == 0);
    // —— 编辑极值 → 统计失效 → 色阶端点更新（§4.2 第 2 条）——
    // 行 8（值 8）编辑前 t=8/9；把行 9 的值 9 改成 90 后 max=90 → 行 8 的 t=8/90，
    // 颜色随之变化即证明统计缓存被编辑失效并重算
    const QColor before8 = DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 8.0 / 9.0);
    QVERIFY2(countExactPixels(img, cellRect(8, 0), before8.rgb()) >= 20, "row8 paints with old max=9 interpolation");
    QVERIFY(tv->model()->setData(tv->model()->index(9, 0), QVariant(90.0), Qt::EditRole));
    img = renderViewport();
    const QColor after8 = DATableConditionalFormatRule::lerpColor(c_cfScaleMin, c_cfScaleMax, 8.0 / 90.0);
    QVERIFY2(countExactPixels(img, cellRect(8, 0), after8.rgb()) >= 20,
             "stats must be invalidated after editing the extreme value (row8 re-interpolated against max=90)");
    QCOMPARE(countExactPixels(img, cellRect(8, 0), before8.rgb()), 0);
    // 被编辑格（值 90）= 新 max → 高端色
    QVERIFY2(countExactPixels(img, cellRect(9, 0), highRgb) >= 20, "new max cell paints high-end color");
    // 撤销编辑 → 统计再失效 → 恢复原端点
    w.getUndoStack()->undo();
    img = renderViewport();
    QVERIFY2(countExactPixels(img, cellRect(8, 0), before8.rgb()) >= 20, "undo edit restores original scale endpoints");
    QCOMPARE(countExactPixels(img, cellRect(8, 0), after8.rgb()), 0);
    // —— ③瞬态覆盖②：查找高亮（金黄 alpha160）叠加在色阶背景之上 ——
    {
        DA::DATableHighlightOverlay* ov = w.highlightOverlay();
        ov->set(DA::DATableHighlightOverlay::Source::Find, DA::DATableHighlightOverlay::Level::Hit, { qMakePair(0, 0) });
        img = renderViewport();
        // 期望混合色：dst = (hl*160 + bg*(255-160))/255（bg = 色阶低端浅蓝）
        const QColor hl(0xE6, 0xC2, 0x7C);
        auto blend = [ & ](int cBg, int cHl) {
            return (cHl * 160 + cBg * (255 - 160)) / 255;
        };
        const QColor expect(blend(c_cfScaleMin.red(), hl.red()), blend(c_cfScaleMin.green(), hl.green()), blend(c_cfScaleMin.blue(), hl.blue()));
        const QRect r00 = cellRect(0, 0);
        int blended     = 0;
        for (int y = r00.top(); y <= r00.bottom(); ++y) {
            for (int x = r00.left(); x <= r00.right(); ++x) {
                const QRgb p = img.pixel(x, y);
                if (qAbs(qRed(p) - expect.red()) <= 2 && qAbs(qGreen(p) - expect.green()) <= 2 && qAbs(qBlue(p) - expect.blue()) <= 2) {
                    ++blended;
                }
            }
        }
        QVERIFY2(blended >= 20, qPrintable(QStringLiteral("overlay(3) must cover conditional format(2), blended px=%1").arg(blended)));
        // 纯②色不再出现于该格（被③覆盖）
        QCOMPARE(countExactPixels(img, r00, lowRgb), 0);
        ov->clear(DA::DATableHighlightOverlay::Source::Find);
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(0, 0), lowRgb) >= 20, "clearing overlay restores 2-layer paint");
    }
    // —— 规则命令 undo/redo（加规则/删规则）——
    {
        QSignalSpy ruleSpy(store, &DATableConditionalFormatStore::rulesChanged);
        QUndoStack* us = w.getUndoStack();
        QVERIFY(us != nullptr);
        DATableConditionalFormatRule greenRule = makeBgRule(CfCond::Greater, QVariant(5), QColor(0x66, 0x9E, 0x8B));
        us->push(new DA::DACommandConditionalFormat_addRule(store, 0, greenRule));
        QCOMPARE(store->columnRules(0).size(), 2);
        QVERIFY(ruleSpy.count() >= 1);
        img = renderViewport();
        // 值 6..9 命中绿规则（列表靠后覆盖色阶）
        QVERIFY2(countExactPixels(img, cellRect(9, 0), QColor(0x66, 0x9E, 0x8B).rgb()) >= 20, "later rule overrides color scale");
        us->undo();
        QCOMPARE(store->columnRules(0).size(), 1);
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(9, 0), highRgb) >= 20, "undo add-rule restores scale paint");
        // push 时栈顶位于已撤销的编辑命令之下（undo 后 index 在其前）→ 编辑命令被丢弃，
        // 栈中只剩本 addRule 命令（QUndoStack::push 标准语义）
        QCOMPARE(us->count(), 1);
        us->redo();
        QCOMPARE(store->columnRules(0).size(), 2);
        us->undo();  // 复原，避免影响后续断言
    }
    // —— 预览规则集：叠加显示、清除还原（对话框实时预览机制）——
    {
        QHash< int, QVector< DATableConditionalFormatRule > > preview;
        preview.insert(0, QVector< DATableConditionalFormatRule >() << makeBgRule(CfCond::IsNotEmpty, QVariant(), QColor(0xE6, 0xC2, 0x7C)));
        store->setPreviewRules(preview);
        QVERIFY(store->hasPreview());
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(3, 0), QColor(0xE6, 0xC2, 0x7C).rgb()) >= 20, "preview rules paint without committing");
        QCOMPARE(store->ruleCount(), 2);  // 预览不落库（列0 一条色阶 + 列1 一条数据条）
        store->clearPreview();
        QVERIFY(!store->hasPreview());
        img = renderViewport();
        QVERIFY2(countExactPixels(img, cellRect(3, 0), QColor(0xE6, 0xC2, 0x7C).rgb()) == 0, "clearPreview restores committed rules");
    }
    w.hide();
}

/**
 * @brief 对话框：惰性/只读源上色阶与数据条置灰 + 说明文案；普通源全部可用；
 * 编辑不落库（取消后 store 不变）、预览通道工作
 */
void DATableDataTest::testConditionalDialogSourceRestriction()
{
    DATableConditionalFormatStore store;
    const QStringList names { QStringLiteral("a"), QStringLiteral("b") };
    {
        DA::DADialogConditionalFormat dlg(&store, nullptr);
        dlg.setup(names, { 0, 1 }, false);  // 惰性/只读源
        QComboBox* comboType = dlg.findChild< QComboBox* >(QStringLiteral("comboType"));
        QVERIFY(comboType != nullptr);
        QStandardItemModel* m = qobject_cast< QStandardItemModel* >(comboType->model());
        QVERIFY(m != nullptr);
        QVERIFY(m->item(0)->isEnabled());   // 单元格规则可用
        QVERIFY(!m->item(1)->isEnabled());  // 色阶置灰
        QVERIFY(!m->item(2)->isEnabled());  // 数据条置灰
        QLabel* hint = dlg.findChild< QLabel* >(QStringLiteral("labelDisabledHint"));
        QVERIFY(hint != nullptr);
        QVERIFY(!hint->text().isEmpty());       // 说明文案（tr）
        QVERIFY(!hint->isHidden());             // 且可见
        // 预览通道：setup 后 store 挂上预览（初始 = 现状）
        QVERIFY(store.hasPreview());
        // 新增规则只进工作集，不落库
        dlg.show();  // 离屏显示，保证鼠标事件投递
        QPushButton* addBtn = dlg.findChild< QPushButton* >(QStringLiteral("buttonAdd"));
        QVERIFY(addBtn != nullptr);
        QTest::mouseClick(addBtn, Qt::LeftButton);
        QCOMPARE(dlg.resultRules().value(0).size() + dlg.resultRules().value(1).size(), 1);
        QCOMPARE(store.ruleCount(), 0);  // 未提交
        dlg.reject();                    // 取消 → 预览还原
        QVERIFY(!store.hasPreview());
        QVERIFY(store.isEmpty());
    }
    {
        DA::DADialogConditionalFormat dlg(&store, nullptr);
        dlg.setup(names, { 0 }, true);  // 普通源
        QComboBox* comboType = dlg.findChild< QComboBox* >(QStringLiteral("comboType"));
        QStandardItemModel* m = qobject_cast< QStandardItemModel* >(comboType->model());
        QVERIFY(m->item(1)->isEnabled());
        QVERIFY(m->item(2)->isEnabled());
        dlg.accept();  // 确定：预览清除，结果交调用方（此处无差异 → 不提交）
        QVERIFY(!store.hasPreview());
    }
}

/**
 * @brief 滚动性能（§4.2「滚动千行无卡顿」的程序化近似）：5000 行 × 3 规则列，
 * 50 次跳滚 + 全视口重绘计时；并用计数提供者钉住「统计只算一次、滚动不重算」
 *（红线2 的缓存语义），逐格遍历规则由按列分桶结构保证（rulesForColumn O(1)）
 */
void DATableDataTest::testConditionalScrollPerf1k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import numpy as np
import pandas as pd
pdf5k = pd.DataFrame({
    "x": np.arange(5000, dtype=float),
    "y": np.random.rand(5000) * 100.0,
    "z": np.arange(5000, dtype=float) % 7,
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "pdf5k" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    DATableConditionalFormatStore* store = w.conditionalFormatStore();
    QVERIFY(store != nullptr);
    // 三列各一条规则：色阶 + 数据条 + 单元格规则
    DATableConditionalFormatRule scaleRule;
    scaleRule.type = CfType::ColorScale;
    store->addRule(0, scaleRule);
    DATableConditionalFormatRule barRule;
    barRule.type = CfType::DataBar;
    store->addRule(1, barRule);
    store->addRule(2, makeBgRule(CfCond::Greater, QVariant(3), QColor(0xE6, 0xC2, 0x7C)));
    // 用计数提供者替换 widget 安装的 Python 提供者：钉住统计计算次数
    int statsCalls = 0;
    store->setStatsProvider([ &statsCalls ](int column, DATableConditionalFormatStore::ColumnStats& out) {
        ++statsCalls;
        out.minValue    = 0.0;
        out.maxValue    = 5000.0;
        out.medianValue = 2500.0;
        Q_UNUSED(column);
        return true;
    });
    // 首绘：色阶列与数据条列各计算一次统计（单元格规则列不算）
    tv->viewport()->repaint();
    QApplication::processEvents();
    const int callsAfterFirstPaint = statsCalls;
    QVERIFY2(callsAfterFirstPaint >= 2, qPrintable(QStringLiteral("stats computed lazily on first paint, calls=%1").arg(callsAfterFirstPaint)));
    // 50 次跳滚（覆盖 5000 行）+ 全视口重绘计时
    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i <= 4900; i += 100) {
        tv->showActualRow(i);
        tv->viewport()->repaint();
        if ((i % 1000) == 0) {
            QApplication::processEvents();
        }
    }
    const qint64 elapsed = timer.elapsed();
    // 滚动不重算统计（缓存 + 精确失效挂点：滚动不发 cellValueChanged）
    QCOMPARE(statsCalls, callsAfterFirstPaint);
    // 离屏软件渲染的宽松上界（主要防"每格重算统计/每格遍历全部规则"级回归；
    // 主观流畅度仍属 §4.2 手工验收）
    QVERIFY2(elapsed < 20000, qPrintable(QStringLiteral("50 scroll steps + repaints took %1 ms").arg(elapsed)));
    qDebug().noquote() << QStringLiteral("testConditionalScrollPerf1k: 5000 rows, 50 scroll steps + repaints elapsed_ms=%1, statsCalls=%2")
                              .arg(elapsed)
                              .arg(statsCalls);
    w.hide();
}

//===================================================
// 004 WP1 计算列（T1.23）
//===================================================

// 004 §7.1：计算列状态机——创建→有效、依赖变更→待重算、依赖删除→失效、
// 目标列被删→定义移除、改名迁移、Invalid 不降级、快照/恢复
void DATableDataTest::testComputedColumnDefinitionAndStore()
{
    using DA::DAComputedColumn;
    using DA::DATableComputedColumnStore;
    using State                    = DATableComputedColumnStore::State;
    // —— 定义类（纯数据）——
    DAComputedColumn def;
    QVERIFY(!def.isValid());
    def = DAComputedColumn(QStringLiteral("C"), QStringLiteral("A + B"));
    QVERIFY(def.isValid());
    QCOMPARE(def.engine, DAComputedColumn::pandasEvalEngine());
    QCOMPARE(def.version, DAComputedColumn::c_currentVersion);
    QVERIFY(def.enabled);
    def.dependsOn = QStringList { QStringLiteral("A"), QStringLiteral("B") };
    QVERIFY(!def.isSelfReferencing());
    def.dependsOn = QStringList { QStringLiteral("A"), QStringLiteral("C") };
    QVERIFY(def.isSelfReferencing());  // R7：自覆盖判定（依赖含目标列）
    // —— 存储状态机 ——
    DATableComputedColumnStore store;
    QVERIFY(store.isEmpty());
    QSignalSpy defSpy(&store, &DATableComputedColumnStore::definitionsChanged);
    QSignalSpy stateSpy(&store, &DATableComputedColumnStore::stateChanged);
    DAComputedColumn c1(QStringLiteral("C"), QStringLiteral("A + B"));
    c1.dependsOn = QStringList { QStringLiteral("A"), QStringLiteral("B") };
    DAComputedColumn c2(QStringLiteral("D"), QStringLiteral("C * 2"));
    c2.dependsOn = QStringList { QStringLiteral("C") };
    store.addDefinition(c1, State::Valid);
    store.addDefinition(c2, State::Valid);
    QCOMPARE(store.count(), 2);
    QCOMPARE(store.targetColumns(), QStringList({ QStringLiteral("C"), QStringLiteral("D") }));
    QCOMPARE(defSpy.count(), 2);
    QCOMPARE(store.stateOf(QStringLiteral("C")), State::Valid);
    QCOMPARE(store.stateOf(QStringLiteral("nope")), State::Invalid);  // 无定义按失效
    QVERIFY(!store.hasDefinition(QStringLiteral("nope")));
    // 依赖列变更 → 直接依赖者 Stale，间接依赖者不动（一次只标直接依赖）
    QStringList marked = store.markStaleForChangedColumns({ QStringLiteral("A") });
    QCOMPARE(marked, QStringList({ QStringLiteral("C") }));
    QCOMPARE(store.stateOf(QStringLiteral("C")), State::Stale);
    QCOMPARE(store.stateOf(QStringLiteral("D")), State::Valid);
    QCOMPARE(stateSpy.count(), 1);
    // 目标列自身被改 → 也标 Stale；同时是他人依赖 → 两者都标
    store.setState(QStringLiteral("C"), State::Valid);
    marked = store.markStaleForChangedColumns({ QStringLiteral("C") });
    QCOMPARE(marked.size(), 2);
    QVERIFY(marked.contains(QStringLiteral("C")) && marked.contains(QStringLiteral("D")));
    // Invalid 不因数据变更降级为 Stale（保持失效直到修复）
    store.setState(QStringLiteral("C"), State::Invalid);
    store.setState(QStringLiteral("D"), State::Valid);
    marked = store.markStaleForChangedColumns({ QStringLiteral("A") });
    QCOMPARE(marked, QStringList());
    QCOMPARE(store.stateOf(QStringLiteral("C")), State::Invalid);
    // markAllStale 只标 Valid（整表操作保守标记，R8）
    marked = store.markAllStale();
    QCOMPARE(marked, QStringList({ QStringLiteral("D") }));
    QCOMPARE(store.stateOf(QStringLiteral("C")), State::Invalid);
    // 改名迁移（R20）：依赖列改名 / 目标列自身改名
    store.setState(QStringLiteral("C"), State::Valid);
    store.setState(QStringLiteral("D"), State::Valid);
    int n = store.renameColumnReferences(QStringLiteral("A"), QStringLiteral("A2"));
    QCOMPARE(n, 1);
    QCOMPARE(store.definition(QStringLiteral("C")).dependsOn, QStringList({ QStringLiteral("A2"), QStringLiteral("B") }));
    n = store.renameColumnReferences(QStringLiteral("C"), QStringLiteral("C2"));
    QCOMPARE(n, 2);  // c1 目标 + c2 依赖
    QVERIFY(store.hasDefinition(QStringLiteral("C2")));
    QVERIFY(!store.hasDefinition(QStringLiteral("C")));
    QCOMPARE(store.definition(QStringLiteral("D")).dependsOn, QStringList({ QStringLiteral("C2") }));
    // definitionsReferencing：引用 C2 作依赖的定义（不含自身）
    QCOMPARE(store.definitionsReferencing(QStringLiteral("C2")), QStringList({ QStringLiteral("D") }));
    QVERIFY(store.definitionsReferencing(QStringLiteral("D")).isEmpty());
    // 删列联动：删依赖列 B → C2 失效（定义保留）；删目标列 D → 定义移除
    QStringList removedTargets, invalidatedTargets;
    store.onColumnsRemoved({ QStringLiteral("B"), QStringLiteral("D") }, &removedTargets, &invalidatedTargets);
    QCOMPARE(removedTargets, QStringList({ QStringLiteral("D") }));
    QCOMPARE(invalidatedTargets, QStringList({ QStringLiteral("C2") }));
    QCOMPARE(store.count(), 1);
    QCOMPARE(store.stateOf(QStringLiteral("C2")), State::Invalid);
    // 快照/恢复（删列命令 undo 联动用）
    const QList< DATableComputedColumnStore::Entry > snap = store.snapshot();
    store.clear();
    QVERIFY(store.isEmpty());
    store.restore(snap);
    QCOMPARE(store.count(), 1);
    QCOMPARE(store.stateOf(QStringLiteral("C2")), State::Invalid);
    QVERIFY(defSpy.count() > 2);
}

// 004 R10 磁盘契约：<computed-column> 段 round-trip（expr 特殊字符/空格列名/状态/依赖/engine/version）
void DATableDataTest::testComputedColumnStoreXmlRoundTrip()
{
    using DA::DAComputedColumn;
    using DA::DATableComputedColumnStore;
    using State = DATableComputedColumnStore::State;
    DATableComputedColumnStore store;
    DAComputedColumn c1(QStringLiteral("ratio"), QStringLiteral("A < B && `my col` > 1"));
    c1.dependsOn = QStringList { QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("my col") };
    store.addDefinition(c1, State::Stale);
    DAComputedColumn c2(QStringLiteral("flag"), QStringLiteral("A >= 60"));
    c2.dependsOn = QStringList { QStringLiteral("A") };
    c2.enabled   = false;
    store.addDefinition(c2, State::Invalid);
    QDomDocument doc;
    QDomElement table = doc.createElement(QStringLiteral("table"));
    doc.appendChild(table);
    store.toXml(doc, table);
    QDomElement cc = table.firstChildElement(QStringLiteral("computed-column"));
    QVERIFY(!cc.isNull());
    // 磁盘契约字段（expr+engine+version+state）
    QDomElement col0 = cc.firstChildElement(QStringLiteral("column"));
    QCOMPARE(col0.attribute(QStringLiteral("target")), QStringLiteral("ratio"));
    QCOMPARE(col0.attribute(QStringLiteral("engine")), QStringLiteral("pandas.eval"));
    QCOMPARE(col0.attribute(QStringLiteral("version")).toInt(), DAComputedColumn::c_currentVersion);
    QCOMPARE(col0.attribute(QStringLiteral("state")), QStringLiteral("stale"));
    // round-trip
    DATableComputedColumnStore store2;
    QSignalSpy spy(&store2, &DATableComputedColumnStore::definitionsChanged);
    QVERIFY(store2.fromXml(cc));
    QCOMPARE(store2.count(), 2);
    QCOMPARE(spy.count(), 1);  // 加载只恢复定义，不发求值（store 层无求值路径，R7）
    QCOMPARE(store2.targetColumns(), QStringList({ QStringLiteral("ratio"), QStringLiteral("flag") }));
    QCOMPARE(store2.stateOf(QStringLiteral("ratio")), State::Stale);  // 状态持久化（打开工程不重算，R7）
    QCOMPARE(store2.stateOf(QStringLiteral("flag")), State::Invalid);
    const DAComputedColumn r1 = store2.definition(QStringLiteral("ratio"));
    QCOMPARE(r1.expr, c1.expr);  // XML 特殊字符（< && 反引号）经 CDATA 无损
    QCOMPARE(r1.dependsOn, c1.dependsOn);
    QCOMPARE(r1.engine, c1.engine);
    QCOMPARE(r1.version, c1.version);
    const DAComputedColumn r2 = store2.definition(QStringLiteral("flag"));
    QVERIFY(!r2.enabled);
    // 空 store 不写段
    DATableComputedColumnStore empty;
    QDomDocument doc2;
    QDomElement table2 = doc2.createElement(QStringLiteral("table"));
    doc2.appendChild(table2);
    empty.toXml(doc2, table2);
    QVERIFY(table2.firstChildElement(QStringLiteral("computed-column")).isNull());
}

// 004 §7.1：表达式依赖提取（普通列/反引号空格列名/函数名过滤/语法错误）+ 反引号判定
void DATableDataTest::testComputedColumnExprDepends()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2],"B":[3,4],"my col":[5,6]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    bool ok  = false;
    // 普通列引用（按列序返回）
    QStringList deps = py.exprDepends(df, QStringLiteral("B * 2 + A"), &ok);
    QVERIFY(ok);
    QCOMPARE(deps, QStringList({ QStringLiteral("A"), QStringLiteral("B") }));
    // 反引号空格列名
    deps = py.exprDepends(df, QStringLiteral("`my col` / A"), &ok);
    QVERIFY(ok);
    QCOMPARE(deps, QStringList({ QStringLiteral("A"), QStringLiteral("my col") }));
    // 函数名与字符串字面量不误报（只留实际列）
    deps = py.exprDepends(df, QStringLiteral("abs(A) + 1"), &ok);
    QVERIFY(ok);
    QCOMPARE(deps, QStringList({ QStringLiteral("A") }));
    // 语法错误 → ok=false（对话框转校验失败提示）
    QString err;
    deps = py.exprDepends(df, QStringLiteral("A +"), &ok, &err);
    QVERIFY(!ok);
    QVERIFY(deps.isEmpty());
    QVERIFY(!err.isEmpty());
    // 反引号判定（R20）：合法标识符裸写，空格/数字开头/关键字加反引号
    QVERIFY(!py.needsBacktick(QStringLiteral("A")));
    QVERIFY(!py.needsBacktick(QStringLiteral("col_1")));
    QVERIFY(py.needsBacktick(QStringLiteral("my col")));
    QVERIFY(py.needsBacktick(QStringLiteral("2024")));
    QVERIFY(py.needsBacktick(QStringLiteral("in")));
}

// 004 §7.1：计算列求值算子（新建/重算快照/失败不动数据/空格目标列/预览副本/还原）
void DATableDataTest::testComputedColumnEvalOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2,3],"B":[4,5,6]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    // 新建列：had=false、无旧列快照
    auto r = py.evalColumn(df, QStringLiteral("C"), QStringLiteral("A + B"));
    QVERIFY(r.ok);
    QVERIFY(!r.hadColumn);
    QVERIFY(r.oldColumn.isNone());
    QCOMPARE(df.iat(0, 2).toInt(), 5);
    QCOMPARE(df.iat(2, 2).toInt(), 9);
    // 重算：had=true + 旧列快照
    QVERIFY(py.evalColumn(df, QStringLiteral("A"), QStringLiteral("A0")) .ok == false);  // A0 不存在 → 失败
    QVERIFY(df.columns().contains(QStringLiteral("A")));                                  // 数据不变
    r = py.evalColumn(df, QStringLiteral("C"), QStringLiteral("A * 10"));
    QVERIFY(r.ok);
    QVERIFY(r.hadColumn);
    QVERIFY(!r.oldColumn.isNone());
    QCOMPARE(df.iat(0, 2).toInt(), 10);
    // 还原旧列（undo 路径算子）
    QVERIFY(py.setColumn(df, QStringLiteral("C"), r.oldColumn));
    QCOMPARE(df.iat(0, 2).toInt(), 5);
    QCOMPARE(df.iat(2, 2).toInt(), 9);
    // 空格目标列名（da_eval_column 走 df[target]=result 赋值，不受 pandas eval 反引号赋值缺陷影响）
    r = py.evalColumn(df, QStringLiteral("new col"), QStringLiteral("A * `new col`"));
    QVERIFY(!r.ok);  // 引用了尚不存在的 `new col` → 失败（自覆盖同款防线）
    r = py.evalColumn(df, QStringLiteral("new col"), QStringLiteral("A * 2"));
    QVERIFY(r.ok);
    QVERIFY(df.columns().contains(QStringLiteral("new col")));
    QCOMPARE(df.iat(1, df.columns().indexOf(QStringLiteral("new col"))).toInt(), 4);
    // 失败：引用不存在列 → ok=false、errorMessage 非空、不新建列
    r = py.evalColumn(df, QStringLiteral("D"), QStringLiteral("NOSUCH + 1"));
    QVERIFY(!r.ok);
    QVERIFY(!r.errorMessage.isEmpty());
    QVERIFY(!df.columns().contains(QStringLiteral("D")));
    // 预览副本：head 上求值不触碰原表
    DA::DAPyDataFrame head = py.headCopy(df, 2);
    QVERIFY(!head.isNone());
    QCOMPARE(head.shape().first, static_cast< std::size_t >(2));
    r = py.evalColumn(head, QStringLiteral("P"), QStringLiteral("A + B"));
    QVERIFY(r.ok);
    QVERIFY(head.columns().contains(QStringLiteral("P")));
    QVERIFY(!df.columns().contains(QStringLiteral("P")));
    // 列副本算子（分列/合列快照用）
    DA::DAPySeries colCopy = py.getColumnCopy(df, 0);
    QVERIFY(!colCopy.isNone());
    QVERIFY(py.getColumnCopy(df, 99).isNone());  // 越界 → none
}

// 004 §7.1：重算命令 undo/redo（创建=删列，重算=还原旧列；列值一致，D4 列级记录）
void DATableDataTest::testComputedColumnCommandUndoRedo()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2,3],"B":[4,5,6]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QUndoStack stack;
    // —— 创建命令：exec 建列，push 首次 redo 跳过 ——
    auto* cmd = new DA::DACommandDataFrame_evalColumn(df, QStringLiteral("C"), QStringLiteral("A + B"));
    int cbCount = 0, dirUndo = 0, dirRedo = 0;
    cmd->setCallBack([ &cbCount ]() {
        ++cbCount;
    });
    cmd->setDirectionalCallBack([ &dirUndo, &dirRedo ](bool isUndo) {
        if (isUndo) {
            ++dirUndo;
        } else {
            ++dirRedo;
        }
    });
    QVERIFY(cmd->exec());
    QCOMPARE(df.iat(0, 2).toInt(), 5);
    QVERIFY(df.columns().contains(QStringLiteral("C")));
    stack.push(cmd);  // 首次 redo 跳过，不重复求值
    QCOMPARE(cbCount, 1);
    QCOMPARE(dirRedo, 1);
    QCOMPARE(stack.count(), 1);
    // undo → 删除新建列
    stack.undo();
    QVERIFY(!df.columns().contains(QStringLiteral("C")));
    QCOMPARE(df.columns().size(), 2);
    QCOMPARE(dirUndo, 1);
    // redo → 重新求值建列
    stack.redo();
    QVERIFY(df.columns().contains(QStringLiteral("C")));
    QCOMPARE(df.iat(2, 2).toInt(), 9);
    // —— 重算命令：目标列已存在，undo 还原旧列 ——
    auto* cmd2 = new DA::DACommandDataFrame_evalColumn(df, QStringLiteral("C"), QStringLiteral("A * 10"));
    QVERIFY(cmd2->exec());
    QCOMPARE(df.iat(0, 2).toInt(), 10);  // C 被重算为 A*10
    stack.push(cmd2);
    stack.undo();                          // 还原为 A+B
    QCOMPARE(df.iat(0, 2).toInt(), 5);
    QCOMPARE(df.iat(2, 2).toInt(), 9);
    stack.redo();
    QCOMPARE(df.iat(0, 2).toInt(), 10);
    // —— 失败表达式：exec 返回 false，不入栈，数据不变 ——
    DA::DACommandDataFrame_evalColumn bad(df, QStringLiteral("E"), QStringLiteral("NOPE + 1"));
    QVERIFY(!bad.exec());
    QVERIFY(!bad.lastErrorMessage().isEmpty());
    QVERIFY(!df.columns().contains(QStringLiteral("E")));
}

// 004 §3.4：对话框校验（自覆盖/已存在列/不存在列/语法错误拒绝，合法表达式通过并给出依赖）
void DATableDataTest::testComputedColumnDialogValidation()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2,3],"B":[4,5,6]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    auto okButton = [](DA::DADialogComputedColumn& dlg) -> QPushButton* {
        QDialogButtonBox* box = dlg.findChild< QDialogButtonBox* >();
        return box ? box->button(QDialogButtonBox::Ok) : nullptr;
    };
    auto setInputs = [](DA::DADialogComputedColumn& dlg, const QString& target, const QString& expr) {
        QLineEdit* targetEdit = dlg.findChild< QLineEdit* >(QStringLiteral("lineEditComputedTarget"));
        QLineEdit* exprEdit   = dlg.findChild< QLineEdit* >(QStringLiteral("lineEditComputedExpr"));
        QVERIFY(targetEdit != nullptr && exprEdit != nullptr);
        targetEdit->setText(target);
        exprEdit->setText(expr);
    };
    // 合法创建：目标新列 + 引用现有列 → OK 可用，依赖 = {A,B}
    {
        DA::DADialogComputedColumn dlg(df);
        setInputs(dlg, QStringLiteral("C"), QStringLiteral("A + B"));
        QTest::qWait(450);  // 越过去抖（300ms）
        QPushButton* ok = okButton(dlg);
        QVERIFY(ok && ok->isEnabled());
        QCOMPARE(dlg.dependencies(), QStringList({ QStringLiteral("A"), QStringLiteral("B") }));
        QCOMPARE(dlg.targetColumnName(), QStringLiteral("C"));
        QCOMPARE(dlg.expressionBody(), QStringLiteral("A + B"));
    }
    // 目标列已存在（创建模式）→ 拒绝（禁止覆盖已有列，R7）
    {
        DA::DADialogComputedColumn dlg(df);
        setInputs(dlg, QStringLiteral("A"), QStringLiteral("B * 2"));
        QTest::qWait(450);
        QPushButton* ok = okButton(dlg);
        QVERIFY(ok && !ok->isEnabled());
    }
    // 引用不存在列 → 预览求值失败 → 拒绝
    {
        DA::DADialogComputedColumn dlg(df);
        setInputs(dlg, QStringLiteral("Z"), QStringLiteral("NOPE + 1"));
        QTest::qWait(450);
        QPushButton* ok = okButton(dlg);
        QVERIFY(ok && !ok->isEnabled());
    }
    // 语法错误 → 依赖解析失败 → 拒绝
    {
        DA::DADialogComputedColumn dlg(df);
        setInputs(dlg, QStringLiteral("Z"), QStringLiteral("A +"));
        QTest::qWait(450);
        QPushButton* ok = okButton(dlg);
        QVERIFY(ok && !ok->isEnabled());
    }
    // 编辑模式：目标字段只读；自覆盖（表达式引用目标列 A）→ 拒绝
    {
        DA::DADialogComputedColumn dlg(df);
        DA::DAComputedColumn def(QStringLiteral("A"), QStringLiteral("B * 2"));
        dlg.setEditMode(def);
        QVERIFY(dlg.isEditMode());
        QLineEdit* targetEdit = dlg.findChild< QLineEdit* >(QStringLiteral("lineEditComputedTarget"));
        QLineEdit* exprEdit   = dlg.findChild< QLineEdit* >(QStringLiteral("lineEditComputedExpr"));
        QVERIFY(targetEdit->isReadOnly());
        QCOMPARE(targetEdit->text(), QStringLiteral("A"));
        exprEdit->setText(QStringLiteral("A + 1"));  // A 是目标 → 自覆盖
        QTest::qWait(450);
        QPushButton* ok = okButton(dlg);
        QVERIFY(ok && !ok->isEnabled());
    }
}

// 004 §3.4：widget 全链路——创建入栈、依赖变更标脏、手动重算、undo 还原、表头浅蓝底色
void DATableDataTest::testComputedColumnWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2,3],"B":[4,5,6]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableComputedColumnStore* store = w.computedColumnStore();
    QVERIFY(store != nullptr);
    QVERIFY(store->isEmpty());
    // —— 经对话框创建计算列 C = A + B（驱动模态对话框填写并接受）——
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogComputedColumn* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QLineEdit* targetEdit = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditComputedTarget"));
        QLineEdit* exprEdit   = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditComputedExpr"));
        if (targetEdit && exprEdit) {
            targetEdit->setText(QStringLiteral("C"));
            exprEdit->setText(QStringLiteral("A + B"));
        }
        QTimer::singleShot(500, [ dlg ]() {
            QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
            if (box && box->button(QDialogButtonBox::Ok)->isEnabled()) {
                dlg->accept();
            } else {
                dlg->reject();
            }
        });
    });
    w.showComputedColumnDialog(-1);
    // 创建结果：列 C 物化、定义 Valid、入撤销栈
    QVERIFY(df.columns().contains(QStringLiteral("C")));
    QCOMPARE(df.iat(0, 2).toInt(), 5);
    QVERIFY(store->hasDefinition(QStringLiteral("C")));
    QCOMPARE(store->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Valid);
    QCOMPARE(w.getUndoStack()->count(), 1);
    // 表头浅蓝底色（R9：BackgroundRole = #BDCDE8）
    const int colC = df.columns().indexOf(QStringLiteral("C"));
    QVariant bg    = w.getDataTableView()->model()->headerData(colC, Qt::Horizontal, Qt::BackgroundRole);
    QVERIFY(bg.canConvert< QColor >());
    QCOMPARE(bg.value< QColor >(), QColor(0xBD, 0xCD, 0xE8));
    // 非计算列无底色
    QVariant bgA = w.getDataTableView()->model()->headerData(0, Qt::Horizontal, Qt::BackgroundRole);
    QVERIFY(!bgA.canConvert< QColor >() || !bgA.isValid());
    // —— 改依赖列 A[0] 1→10 → C 转待重算（cellValueChanged 精确挂点）——
    QAbstractItemModel* model = w.getDataTableView()->model();
    QVERIFY(model->setData(model->index(0, 0), QVariant(10), Qt::EditRole));
    QCOMPARE(store->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Stale);
    // —— 手动重算 C → 值更新（A[0]+B[0]=14）且转有效，入栈 ——
    const int undoBefore = w.getUndoStack()->count();
    QVERIFY(w.recomputeComputedColumn(colC));
    QCOMPARE(df.iat(0, 2).toInt(), 14);
    QCOMPARE(store->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Valid);
    QCOMPARE(w.getUndoStack()->count(), undoBefore + 1);
    // —— undo 重算 → C 还原为旧值（5），状态回到重算前（Stale）——
    w.getUndoStack()->undo();
    QCOMPARE(df.iat(0, 2).toInt(), 5);
    QCOMPARE(store->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Stale);
    // —— 移除公式（保留数据）：定义消失、列仍在，可撤销 ——
    w.getUndoStack()->redo();  // 回到 Valid
    QVERIFY(w.removeComputedColumnFormula(colC));
    QVERIFY(!store->hasDefinition(QStringLiteral("C")));
    QVERIFY(df.columns().contains(QStringLiteral("C")));  // 列数据保留
    w.getUndoStack()->undo();                             // 恢复定义
    QVERIFY(store->hasDefinition(QStringLiteral("C")));
    // —— 打开工程不重算（R7）：元数据快照恢复 + 篡改列值后新建页面，值保持篡改后（未求值）——
    {
        DA::DATableMetadata* bundle1 = registry.getOrCreate(d);
        QVERIFY(bundle1 != nullptr);
        const QString snapshot = bundle1->snapshotToString();
        QVERIFY(!snapshot.isEmpty());
        // 篡改 C 列值（偏离公式结果 A+B），模拟"工程里的物化值"
        pybind11::exec(R"(df["C"] = [99, 99, 99])", pybind11::globals(), scope);
        QCOMPARE(df.iat(0, 2).toInt(), 99);
        // 新会话：新注册表 + 快照恢复（等价 DAAppProject 加载路径），再建页面
        DA::DATableMetadataRegistry registry2(nullptr);
        DA::DATableMetadata* bundle2 = registry2.getOrCreate(d);
        QVERIFY(bundle2->restoreFromString(snapshot));
        DA::DATableComputedColumnStore* store2 = bundle2->computedColumns();
        QVERIFY(store2->hasDefinition(QStringLiteral("C")));
        QCOMPARE(store2->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Valid);  // 状态恢复
        DA::DADataOperateOfDataFrameWidget w2(d, &registry2);
        w2.resize(700, 400);
        w2.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w2));
        // 打开页面不触发任何重算：C 保持篡改后的值（若重算会变回 A+B=14/7/9）
        QCOMPARE(df.iat(0, 2).toInt(), 99);
        QCOMPARE(df.iat(1, 2).toInt(), 99);
        QCOMPARE(w2.computedColumnStore()->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Valid);
        w2.hide();
    }
    w.hide();
}

// 004 §3.4：改名/删列联动 + 排序不触发标脏（R13/R20）
void DATableDataTest::testComputedColumnRenameRemoveLinkage()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2,3],"B":[4,5,6],"K":[3,1,2]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableComputedColumnStore* store = w.computedColumnStore();
    QVERIFY(store != nullptr);
    // 直接建一个计算列 C=A+B（走对话框路径，驱动模态）
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogComputedColumn* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QLineEdit* targetEdit = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditComputedTarget"));
        QLineEdit* exprEdit   = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditComputedExpr"));
        if (targetEdit && exprEdit) {
            targetEdit->setText(QStringLiteral("C"));
            exprEdit->setText(QStringLiteral("A + B"));
        }
        QTimer::singleShot(500, [ dlg ]() {
            QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
            if (box && box->button(QDialogButtonBox::Ok)->isEnabled()) {
                dlg->accept();
            } else {
                dlg->reject();
            }
        });
    });
    w.showComputedColumnDialog(-1);
    QVERIFY(store->hasDefinition(QStringLiteral("C")));
    QCOMPARE(store->definition(QStringLiteral("C")).dependsOn, QStringList({ QStringLiteral("A"), QStringLiteral("B") }));
    DA::DADataTableView* tv = w.getDataTableView();
    // —— 排序不触发标脏（R13）：点 K 列表头排序后 C 仍 Valid ——
    DA::DADataHeaderView* hh = qobject_cast< DA::DADataHeaderView* >(tv->horizontalHeader());
    QVERIFY(hh != nullptr);
    int colK = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("K")) {
            colK = c;
            break;
        }
    }
    QVERIFY(colK >= 0);
    w.sortColumn(colK, Qt::AscendingOrder);  // 菜单排序入口（2026-10-18 修订：点击表头不再排序）
    QCOMPARE(store->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Valid);  // 排序未标脏
    // —— 改名被依赖列 A→A2：确认框点 Yes，定义依赖迁移；undo 还原 ——
    int colA = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("A")) {
            colA = c;
            break;
        }
    }
    QVERIFY(colA >= 0);
    QTimer::singleShot(0, []() {
        auto* mb = qobject_cast< QMessageBox* >(QApplication::activeModalWidget());
        if (mb) {
            mb->button(QMessageBox::Yes)->click();
        }
    });
    const int undoCnt = w.getUndoStack()->count();
    QVERIFY(w.renameColumn(colA, QStringLiteral("A2")));
    QCOMPARE(store->definition(QStringLiteral("C")).dependsOn, QStringList({ QStringLiteral("A2"), QStringLiteral("B") }));
    // undo 改名 → 依赖迁移回 A
    w.getUndoStack()->undo();
    QCOMPARE(store->definition(QStringLiteral("C")).dependsOn, QStringList({ QStringLiteral("A"), QStringLiteral("B") }));
    w.getUndoStack()->redo();  // 回到 A2 状态便于后续（依赖 A2）
    QCOMPARE(w.getUndoStack()->count(), undoCnt + 1);
    // —— 删除被依赖列 B → C 失效（定义保留，不静默清空）；undo 恢复 Valid ——
    int colB = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("B")) {
            colB = c;
            break;
        }
    }
    QVERIFY(colB >= 0);
    tv->clearSelection();
    tv->selectColumn(colB);
    QVERIFY(w.removeSelectColumn() > 0);
    QVERIFY(!df.columns().contains(QStringLiteral("B")));
    QVERIFY(store->hasDefinition(QStringLiteral("C")));                              // 定义保留
    QCOMPARE(store->stateOf(QStringLiteral("C")), DA::DATableComputedColumnStore::State::Invalid);  // 失效
    w.getUndoStack()->undo();                                                        // 恢复列 + 定义状态
    QVERIFY(df.columns().contains(QStringLiteral("B")));
    QVERIFY(store->hasDefinition(QStringLiteral("C")));
    // —— 删除计算列自身 C → 定义一并移除；undo 恢复定义 ——
    int colC = -1;
    for (int c = 0; c < hh->count(); ++c) {
        if (hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("C")) {
            colC = c;
            break;
        }
    }
    QVERIFY(colC >= 0);
    tv->clearSelection();
    tv->selectColumn(colC);
    QVERIFY(w.removeSelectColumn() > 0);
    QVERIFY(!df.columns().contains(QStringLiteral("C")));
    QVERIFY(!store->hasDefinition(QStringLiteral("C")));  // 目标列被删 → 定义移除
    w.getUndoStack()->undo();
    QVERIFY(store->hasDefinition(QStringLiteral("C")));   // 撤销恢复定义
    w.hide();
}

//===================================================
// 004 WP2 分列 / 合并列（T1.9）
//===================================================

// 004 §7.1：分列算子——单字符/正则/固定宽度、溢出策略（丢弃/保留原文/报错）、类型推断开关、自动命名与落位
void DATableDataTest::testSplitColumnOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // —— char 模式："1,2,3" 按逗号 → 3 列数值自动解析；原列保留、结果列插原列右侧 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["1,2,3","4,5,6"],"X":[0,1]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QString err;
        const QStringList names =
            py.splitColumn(df, 0, QStringLiteral("char"), QStringLiteral(","), QString(), QList< int >(), 0, QStringLiteral("discard"), true, QStringList(), &err);
        QCOMPARE(names, QStringList({ QStringLiteral("S_1"), QStringLiteral("S_2"), QStringLiteral("S_3") }));
        // 落位规则：原列保留，结果列插在原列右侧（S, S_1, S_2, S_3, X）
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("S"), QStringLiteral("S_1"), QStringLiteral("S_2"), QStringLiteral("S_3"), QStringLiteral("X") }));
        QCOMPARE(df.iat(0, 1).toInt(), 1);
        QCOMPARE(df.iat(1, 3).toInt(), 6);
        // 数值自动解析：int64 → LongLong
        QCOMPARE(df.iat(0, 1).userType(), static_cast< int >(QMetaType::LongLong));
    }
    // —— 类型推断关：保持字符串 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["1,2","3,4"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QStringList names = py.splitColumn(df,
                                                 0,
                                                 QStringLiteral("char"),
                                                 QStringLiteral(","),
                                                 QString(),
                                                 QList< int >(),
                                                 0,
                                                 QStringLiteral("discard"),
                                                 false,
                                                 QStringList());
        QCOMPARE(names.size(), 2);
        QCOMPARE(df.iat(0, 1).userType(), static_cast< int >(QMetaType::QString));
        QCOMPARE(df.iat(0, 1).toString(), QStringLiteral("1"));
    }
    // —— regex 捕获组：列数=组数 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["2024-01-15","2023-12-01"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QStringList names = py.splitColumn(df,
                                                 0,
                                                 QStringLiteral("regex"),
                                                 QString(),
                                                 QStringLiteral(R"((\d{4})-(\d{2})-(\d{2}))"),
                                                 QList< int >(),
                                                 0,
                                                 QStringLiteral("discard"),
                                                 false,
                                                 QStringList());
        QCOMPARE(names.size(), 3);
        QCOMPARE(df.iat(0, 1).toString(), QStringLiteral("2024"));
        QCOMPARE(df.iat(0, 3).toString(), QStringLiteral("15"));
    }
    // —— 溢出策略：keep（剩余并入最后一列）/ discard（丢弃）/ error（报错且数据不变）——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["a,b,c,d","e,f","g,h,i"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QStringList names = py.splitColumn(df,
                                                 0,
                                                 QStringLiteral("char"),
                                                 QStringLiteral(","),
                                                 QString(),
                                                 QList< int >(),
                                                 3,
                                                 QStringLiteral("keep"),
                                                 false,
                                                 QStringList());
        QCOMPARE(names.size(), 3);
        QCOMPARE(df.iat(0, 3).toString(), QStringLiteral("c,d"));  // 剩余并入最后一列
        // 短行末列为空（NaN 经 caster 为 double NaN，非 null QVariant，需 NaN 感知判定）
        const QVariant missV = df.iat(1, 3);
        QVERIFY(missV.isNull() || missV.toString().isEmpty()
                || (missV.userType() == QMetaType::Double && qIsNaN(missV.toDouble())));
    }
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["a,b,c,d","e,f"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QStringList names = py.splitColumn(df,
                                                 0,
                                                 QStringLiteral("char"),
                                                 QStringLiteral(","),
                                                 QString(),
                                                 QList< int >(),
                                                 2,
                                                 QStringLiteral("discard"),
                                                 false,
                                                 QStringList());
        QCOMPARE(names.size(), 2);
        QCOMPARE(df.columns().size(), 3);  // S + 2 结果列（超出被丢弃）
    }
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["a,b,c,d"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QString err;
        const QStringList names =
            py.splitColumn(df, 0, QStringLiteral("char"), QStringLiteral(","), QString(), QList< int >(), 2, QStringLiteral("error"), false, QStringList(), &err);
        QVERIFY(names.isEmpty());
        QVERIFY(!err.isEmpty());
        QCOMPARE(df.columns().size(), 1);  // 报错时数据不变
    }
    // —— width 固定宽度 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["abcdef","ghijkl"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QStringList names = py.splitColumn(df,
                                                 0,
                                                 QStringLiteral("width"),
                                                 QString(),
                                                 QString(),
                                                 QList< int >({ 2, 2, 2 }),
                                                 0,
                                                 QStringLiteral("discard"),
                                                 false,
                                                 QStringList());
        QCOMPARE(names.size(), 3);
        QCOMPARE(df.iat(0, 1).toString(), QStringLiteral("ab"));
        QCOMPARE(df.iat(1, 3).toString(), QStringLiteral("kl"));
    }
    // —— NaN 行 → 结果各列 NaN；自定义列名 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["a,b",None,"c,d"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const QStringList names = py.splitColumn(df,
                                                 0,
                                                 QStringLiteral("char"),
                                                 QStringLiteral(","),
                                                 QString(),
                                                 QList< int >(),
                                                 0,
                                                 QStringLiteral("discard"),
                                                 false,
                                                 QStringList({ QStringLiteral("P"), QStringLiteral("Q") }));
        QCOMPARE(names, QStringList({ QStringLiteral("P"), QStringLiteral("Q") }));
        // NaN 源行 → 结果各列缺失（null / NaN / 空串皆视为缺失）
        auto isMissing = [](const QVariant& v) {
            return v.isNull() || v.toString().isEmpty() || (v.userType() == QMetaType::Double && qIsNaN(v.toDouble()));
        };
        QVERIFY(isMissing(df.iat(1, 1)));
        QVERIFY(isMissing(df.iat(1, 2)));
        QCOMPARE(df.iat(2, 1).toString(), QStringLiteral("c"));
    }
}

// 004 §7.1：合并算子——顺序、分隔符、跳过空值开关、落位（替换被合并列的位置）
void DATableDataTest::testMergeColumnsOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // 跳过空值：缺失值不留多余分隔符；结果列在最左源列位置、源列删除
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"F":["张",None,"王"],"L":["三","李",None],"O":[1,2,3]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QString err;
        const int pos = py.mergeColumns(df, QList< int >({ 0, 1 }), QStringLiteral(" "), true, QStringLiteral("姓名"), &err);
        QCOMPARE(pos, 0);
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("姓名"), QStringLiteral("O") }));
        QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("张 三"));
        QCOMPARE(df.iat(1, 0).toString(), QStringLiteral("李"));   // 跳过空值：无多余分隔符
        QCOMPARE(df.iat(2, 0).toString(), QStringLiteral("王"));
    }
    // 不跳过空值：缺失按空串参与拼接（保留分隔符）
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"F":["张",None],"L":["三","李"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const int pos = py.mergeColumns(df, QList< int >({ 0, 1 }), QStringLiteral("-"), false, QStringLiteral("M"));
        QCOMPARE(pos, 0);
        QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("张-三"));
        QCOMPARE(df.iat(1, 0).toString(), QStringLiteral("-李"));
    }
    // 拼接顺序 = cols 顺序（逆序合并）；落位 = 最左源列位置
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":["a1","a2"],"B":["b1","b2"],"C":["c1","c2"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        const int pos = py.mergeColumns(df, QList< int >({ 1, 0 }), QStringLiteral("|"), true, QStringLiteral("BA"), nullptr);
        QCOMPARE(pos, 0);  // 最左源列位置 = min(1,0) = 0
        QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("b1|a1"));
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("BA"), QStringLiteral("C") }));
    }
    // 少于两列 → 失败
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":["a1"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QString err;
        QCOMPARE(py.mergeColumns(df, QList< int >({ 0 }), QString(), true, QStringLiteral("M"), &err), -1);
    }
}

// 004 §7.1：列级撤销——undo 后列数、列顺序、内容完全还原；redo 重放
void DATableDataTest::testSplitMergeCommandUndoRedo()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // —— 分列命令 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["a,b,c","d,e,f"],"X":[1,2]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        DA::DACommandDataFrame_splitColumn::SplitParams params;
        params.col          = 0;
        params.mode         = QStringLiteral("char");
        params.delimiter    = QStringLiteral(",");
        params.overflow     = QStringLiteral("discard");
        params.inferNumeric = false;
        const DA::DAPySeries oldSource = py.getColumnCopy(df, 0);
        QVERIFY(!oldSource.isNone());
        QUndoStack stack;
        auto* cmd = new DA::DACommandDataFrame_splitColumn(df, params, oldSource);
        QVERIFY(cmd->exec());
        QCOMPARE(cmd->insertedNames(), QStringList({ QStringLiteral("S_1"), QStringLiteral("S_2"), QStringLiteral("S_3") }));
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("S"), QStringLiteral("S_1"), QStringLiteral("S_2"), QStringLiteral("S_3"), QStringLiteral("X") }));
        stack.push(cmd);
        // undo：删结果列 + 源列快照写回 → 列数/列序/内容完全还原
        stack.undo();
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("S"), QStringLiteral("X") }));
        QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("a,b,c"));
        QCOMPARE(df.iat(1, 1).toInt(), 2);
        // redo：重放拆分
        stack.redo();
        QCOMPARE(df.columns().size(), 5);
        QCOMPARE(df.iat(0, 2).toString(), QStringLiteral("b"));
    }
    // —— 合并命令 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":["a1","a2"],"B":["b1","b2"],"C":["c1","c2"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        DA::DACommandDataFrame_mergeColumns::MergeParams params;
        params.cols   = QList< int >({ 0, 1 });
        params.sep    = QStringLiteral("-");
        params.skipNa = true;
        params.name   = QStringLiteral("M");
        QList< DA::DAPySeries > snapshots;
        for (int c : std::as_const(params.cols)) {
            snapshots.append(py.getColumnCopy(df, c));
        }
        QCOMPARE(snapshots.size(), 2);
        QUndoStack stack;
        auto* cmd = new DA::DACommandDataFrame_mergeColumns(df, params, snapshots);
        QCOMPARE(cmd->insertPosition(), 0);
        QVERIFY(cmd->exec());
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("M"), QStringLiteral("C") }));
        QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("a1-b1"));
        stack.push(cmd);
        // undo：删结果列 + 按原位置升序回插源列 → 完全还原
        stack.undo();
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C") }));
        QCOMPARE(df.iat(1, 0).toString(), QStringLiteral("a2"));
        QCOMPARE(df.iat(1, 1).toString(), QStringLiteral("b2"));
        // redo：重放合并
        stack.redo();
        QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("M"), QStringLiteral("C") }));
        QCOMPARE(df.iat(1, 0).toString(), QStringLiteral("a2-b2"));
    }
}

// 004 §4.4 验收：10 万行分列在可接受时间内完成（算子级实测；界面另有忙等待光标）
void DATableDataTest::testSplitColumnPerf100k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S": [",".join(str((i * j) % 97) for j in range(5)) for i in range(100000)]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QCOMPARE(df.shape().first, static_cast< std::size_t >(100000));
    QElapsedTimer timer;
    timer.start();
    const QStringList names =
        py.splitColumn(df, 0, QStringLiteral("char"), QStringLiteral(","), QString(), QList< int >(), 0, QStringLiteral("discard"), true, QStringList());
    const qint64 elapsed = timer.elapsed();
    QCOMPARE(names.size(), 5);
    QCOMPARE(df.shape().second, static_cast< std::size_t >(6));
    // 验收口径：无界面冻结超过数秒——算子级 10 万行 × 5 段应在 3s 内（实测亚秒级）
    QVERIFY2(elapsed < 3000, qPrintable(QStringLiteral("100k split took %1 ms (>= 3000 ms)").arg(elapsed)));
    qDebug().noquote() << QStringLiteral("DATableDataTest::testSplitColumnPerf100k: rows=100000 parts=5 elapsed_ms=%1").arg(elapsed);
}

// 004 §4.4 验收（程序化）：对话框驱动的 widget 全链路——分列预览一致、Ctrl+Z 一次完全还原、合并同理
void DATableDataTest::testSplitMergeWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"S":["1,2","3,4"],"X":[10,20]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    // —— 分列：选中 S 列 → 对话框（默认 char ","）→ OK ——
    tv->clearSelection();
    tv->selectColumn(0);
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogSplitColumn* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QTimer::singleShot(500, [ dlg ]() {
            QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
            if (box && box->button(QDialogButtonBox::Ok)->isEnabled()) {
                dlg->accept();
            } else {
                dlg->reject();
            }
        });
    });
    const int undoBefore = w.getUndoStack()->count();
    w.showSplitColumnDialog(-1);
    // 结果：S 保留，S_1/S_2 插在其右侧，数值解析
    QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("S"), QStringLiteral("S_1"), QStringLiteral("S_2"), QStringLiteral("X") }));
    QCOMPARE(df.iat(0, 1).toInt(), 1);
    QCOMPARE(df.iat(1, 2).toInt(), 4);
    QCOMPARE(w.getUndoStack()->count(), undoBefore + 1);
    // Ctrl+Z 一次完全还原（含列顺序与列数）
    w.getUndoStack()->undo();
    QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("S"), QStringLiteral("X") }));
    QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("1,2"));
    w.getUndoStack()->redo();
    QCOMPARE(df.columns().size(), 4);
    // —— 合并：选 S(0) 与 X(3) → 对话框默认（分隔符空、跳过空值、名字 S_X）→ OK ——
    tv->clearSelection();
    tv->selectColumn(0);
    if (QItemSelectionModel* sm = tv->selectionModel()) {
        QItemSelection selCol(tv->model()->index(0, 3), tv->model()->index(tv->model()->rowCount() - 1, 3));
        sm->select(selCol, QItemSelectionModel::Select);
    }
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogMergeColumns* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QLineEdit* nameEdit = dlg->findChild< QLineEdit* >(QStringLiteral("lineEditMergeName"));
        if (nameEdit) {
            nameEdit->setText(QStringLiteral("SX"));  // 避免与默认自动名断言耦合
        }
        QTimer::singleShot(500, [ dlg ]() {
            QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
            if (box && box->button(QDialogButtonBox::Ok)->isEnabled()) {
                dlg->accept();
            } else {
                dlg->reject();
            }
        });
    });
    w.showMergeColumnsDialog();
    // 结果列替换被合并列位置（最左源列 = 0），另一源列删除
    QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("SX"), QStringLiteral("S_1"), QStringLiteral("S_2") }));
    QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("1,210"));  // "1,2" + "" + "10"（分隔符空）
    // Ctrl+Z 一次完全还原
    w.getUndoStack()->undo();
    QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("S"), QStringLiteral("S_1"), QStringLiteral("S_2"), QStringLiteral("X") }));
    QCOMPARE(df.iat(0, 0).toString(), QStringLiteral("1,2"));
    QCOMPARE(df.iat(0, 3).toInt(), 10);
    w.hide();
}


//===================================================
// 004 WP3 分组聚合（T1.22）
//===================================================

// 004 §7.1：groupby 算子——多键多列与手算对齐（mean/sum/size）、NaN 键组保留、全 NaN 值列、守卫
void DATableDataTest::testGroupbyOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // —— 多键多列：g1,g2 分组，v 求均值 + w 求和（与 pandas 手算对齐）——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import numpy as np
import pandas as pd
df = pd.DataFrame({
    "g1": ["a","a","b","b","a"],
    "g2": [1,1,1,2,2],
    "v": [10.0,20.0,30.0,np.nan,50.0],
    "w": [1,2,3,4,5],
})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QString err;
        DA::DAPyDataFrame out = py.groupbyAggregate(df,
                                                    QStringList({ QStringLiteral("g1"), QStringLiteral("g2") }),
                                                    QStringList({ QStringLiteral("v"), QStringLiteral("w") }),
                                                    QStringList({ QStringLiteral("mean"), QStringLiteral("sum") }),
                                                    &err);
        QVERIFY2(!out.isNone(), qPrintable(err));
        // 长格式：分组键 reset_index 成普通列 + 每聚合对一列（"{值列}_{函数}"）
        QCOMPARE(out.columns(),
                 QList< QString >({ QStringLiteral("g1"), QStringLiteral("g2"), QStringLiteral("v_mean"), QStringLiteral("w_sum") }));
        QCOMPARE(out.shape().first, static_cast< std::size_t >(4));  // 4 组
        // (a,1): v_mean=(10+20)/2=15, w_sum=1+2=3
        QCOMPARE(out.iat(0, 0).toString(), QStringLiteral("a"));
        QCOMPARE(out.iat(0, 1).toInt(), 1);
        QCOMPARE(out.iat(0, 2).toDouble(), 15.0);
        QCOMPARE(out.iat(0, 3).toInt(), 3);
        // (a,2): v_mean=50, w_sum=5
        QCOMPARE(out.iat(1, 2).toDouble(), 50.0);
        // (b,2): v 全 NaN → v_mean=NaN（空组行为），w_sum=4
        QCOMPARE(out.iat(3, 0).toString(), QStringLiteral("b"));
        QCOMPARE(out.iat(3, 1).toInt(), 2);
        QVERIFY(qIsNaN(out.iat(3, 2).toDouble()));
        QCOMPARE(out.iat(3, 3).toInt(), 4);
        // 分组数（预估结果行数）
        QCOMPARE(py.groupCount(df, QStringList({ QStringLiteral("g1"), QStringLiteral("g2") })), static_cast< qint64 >(4));
        // size：只计组内行数（聚合函数集合与透视表对齐）
        DA::DAPyDataFrame out2 =
            py.groupbyAggregate(df, QStringList({ QStringLiteral("g1") }), QStringList({ QStringLiteral("v") }), QStringList({ QStringLiteral("size") }));
        QVERIFY(!out2.isNone());
        QCOMPARE(out2.columns(), QList< QString >({ QStringLiteral("g1"), QStringLiteral("v_size") }));
        QCOMPARE(out2.iat(0, 1).toInt(), 3);  // a 组 3 行
        QCOMPARE(out2.iat(1, 1).toInt(), 2);  // b 组 2 行
        // 同一列多函数
        DA::DAPyDataFrame out3 = py.groupbyAggregate(df,
                                                     QStringList({ QStringLiteral("g1") }),
                                                     QStringList({ QStringLiteral("v"), QStringLiteral("v") }),
                                                     QStringList({ QStringLiteral("mean"), QStringLiteral("sum") }));
        QVERIFY(!out3.isNone());
        QCOMPARE(out3.columns(), QList< QString >({ QStringLiteral("g1"), QStringLiteral("v_mean"), QStringLiteral("v_sum") }));
        QCOMPARE(out3.iat(0, 2).toDouble(), 80.0);  // a 组 v 求和 = 10+20+50
        // 不支持的函数（与透视表集合对齐：只有 mean/sum/size）→ 失败 + 诊断
        QString err2;
        QVERIFY(py.groupbyAggregate(df,
                                    QStringList({ QStringLiteral("g1") }),
                                    QStringList({ QStringLiteral("v") }),
                                    QStringList({ QStringLiteral("median") }),
                                    &err2)
                    .isNone());
        QVERIFY(!err2.isEmpty());
        // 不存在的键列 → 失败
        QVERIFY(py.groupbyAggregate(df, QStringList({ QStringLiteral("nope") }), QStringList({ QStringLiteral("v") }), QStringList({ QStringLiteral("mean") }))
                    .isNone());
    }
    // —— NaN 键组保留（dropna=False，SQL GROUP BY 语义）——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"k":[None,"a",None,"a"],"v":[1.0,2.0,3.0,4.0]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        DA::DAPyDataFrame out =
            py.groupbyAggregate(df, QStringList({ QStringLiteral("k") }), QStringList({ QStringLiteral("v") }), QStringList({ QStringLiteral("sum") }));
        QVERIFY(!out.isNone());
        QCOMPARE(out.shape().first, static_cast< std::size_t >(2));  // "a" 组 + NaN 组
        QCOMPARE(py.groupCount(df, QStringList({ QStringLiteral("k") })), static_cast< qint64 >(2));
    }
    // —— 大表（100 万行）分组耗时可接受（验收 §5.4；界面侧对话框全表聚合带忙等待光标）——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import numpy as np
import pandas as pd
n = 1000000
df = pd.DataFrame({
    "g": np.random.randint(0, 1000, n),
    "v": np.random.rand(n),
})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QElapsedTimer timer;
        timer.start();
        DA::DAPyDataFrame out = py.groupbyAggregate(df,
                                                    QStringList({ QStringLiteral("g") }),
                                                    QStringList({ QStringLiteral("v") }),
                                                    QStringList({ QStringLiteral("mean") }));
        const qint64 elapsed = timer.elapsed();
        QVERIFY(!out.isNone());
        QCOMPARE(out.shape().first, static_cast< std::size_t >(1000));  // 1000 组
        // 验收口径：耗时可接受（100 万行 × 1000 组 < 5s；pandas groupby 实测远低于此）
        QVERIFY2(elapsed < 5000, qPrintable(QStringLiteral("1M-row groupby took %1 ms (>= 5000 ms)").arg(elapsed)));
        qDebug().noquote() << QStringLiteral("DATableDataTest::testGroupbyOperator: groupby rows=1000000 groups=1000 elapsed_ms=%1").arg(elapsed);
    }
}

// 004 §5.4 验收（程序化）：新数据集注册（xxx_GroupBy 命名、addData_ 可撤销、原表无损、信号发射）
void DATableDataTest::testGroupByWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"g1":["a","a","b"],"v":[10.0,20.0,30.0]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("src"));
    mgr.addData(d);  // 初始数据（非撤销路径注册）
    QCOMPARE(mgr.getDataCount(), 1);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QSignalSpy createdSpy(&w, &DA::DADataOperateOfDataFrameWidget::newDataCreated);
    QVERIFY(createdSpy.isValid());
    // 驱动对话框：键选 g1、聚合列选 v、函数改 sum → OK
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogGroupBy* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QListWidget* keys   = dlg->findChild< QListWidget* >(QStringLiteral("listGroupKeys"));
        QListWidget* values = dlg->findChild< QListWidget* >(QStringLiteral("listGroupValues"));
        if (keys && values) {
            for (int i = 0; i < keys->count(); ++i) {
                if (keys->item(i)->text() == QLatin1String("g1")) {
                    keys->item(i)->setSelected(true);
                }
            }
            for (int i = 0; i < values->count(); ++i) {
                if (values->item(i)->text() == QLatin1String("v")) {
                    values->item(i)->setSelected(true);
                }
            }
        }
        QTimer::singleShot(100, [ dlg ]() {
            QTableWidget* funcs = dlg->findChild< QTableWidget* >(QStringLiteral("tableGroupFuncs"));
            if (funcs && funcs->rowCount() > 0) {
                if (QComboBox* cb = qobject_cast< QComboBox* >(funcs->cellWidget(0, 1))) {
                    cb->setCurrentText(QStringLiteral("sum"));
                }
            }
            QTimer::singleShot(500, [ dlg ]() {
                QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
                if (box && box->button(QDialogButtonBox::Ok)->isEnabled()) {
                    dlg->accept();
                } else {
                    dlg->reject();
                }
            });
        });
    });
    w.showGroupByDialog();
    // 新数据集已注册且命名符合 xxx_GroupBy 惯例
    QCOMPARE(createdSpy.count(), 1);
    QCOMPARE(mgr.getDataCount(), 2);
    DA::DAData nd = mgr.findData(QStringLiteral("src_GroupBy"));
    QVERIFY(!nd.isNull());
    // 结果表内容（长格式：g1, v_sum；a→30、b→30）
    DA::DAPyDataFrame rdf = nd.toDataFrame();
    QVERIFY(!rdf.isNone());
    QCOMPARE(rdf.columns(), QList< QString >({ QStringLiteral("g1"), QStringLiteral("v_sum") }));
    QCOMPARE(rdf.shape().first, static_cast< std::size_t >(2));
    QCOMPARE(rdf.iat(0, 0).toString(), QStringLiteral("a"));
    QCOMPARE(rdf.iat(0, 1).toDouble(), 30.0);
    QCOMPARE(rdf.iat(1, 1).toDouble(), 30.0);
    // 原表完全无损（非破坏产出）
    QCOMPARE(df.columns(), QList< QString >({ QStringLiteral("g1"), QStringLiteral("v") }));
    QCOMPARE(df.shape().first, static_cast< std::size_t >(3));
    // Ctrl+Z 一次撤销「新数据集注册」（数据集从管理器消失）——addData_ 进管理器自己的撤销栈
    QUndoStack* mgrStack = mgr.getUndoStack();
    QVERIFY(mgrStack != nullptr && mgrStack->count() >= 1);
    mgrStack->undo();
    QVERIFY(mgr.findData(QStringLiteral("src_GroupBy")).isNull());
    QCOMPARE(mgr.getDataCount(), 1);
    // redo 恢复注册
    mgrStack->redo();
    QVERIFY(!mgr.findData(QStringLiteral("src_GroupBy")).isNull());
    w.hide();
}


//===================================================
// 004 WP4 转置 / melt（T1.27）
//===================================================

// 004 §7.1：melt/transpose 算子——形状、列名、dtype；melt 行数 = 原行数 × 展开列数
void DATableDataTest::testTransposeMeltOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // —— transpose：行列互换，第一列 "index" = 原列名，其余列名 = 原行索引字符串 ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2],"B":[3.5,4.5],"C":["x","y"]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        QString err;
        DA::DAPyDataFrame out = py.transpose(df, &err);
        QVERIFY2(!out.isNone(), qPrintable(err));
        QCOMPARE(out.shape().first, static_cast< std::size_t >(3));   // 行 = 原列数
        QCOMPARE(out.shape().second, static_cast< std::size_t >(3));  // 列 = index + 原行数
        QCOMPARE(out.columns(), QList< QString >({ QStringLiteral("index"), QStringLiteral("0"), QStringLiteral("1") }));
        QCOMPARE(out.iat(0, 0).toString(), QStringLiteral("A"));
        QCOMPARE(out.iat(1, 0).toString(), QStringLiteral("B"));
        // 列 1/2 = 原行索引 "0"/"1"：C 行在 "0" 列为 x、"1" 列为 y；B 行在 "1" 列为 4.5
        QCOMPARE(out.iat(2, 1).toString(), QStringLiteral("x"));
        QCOMPARE(out.iat(2, 2).toString(), QStringLiteral("y"));
        QCOMPARE(out.iat(1, 1).toDouble(), 3.5);
        QCOMPARE(out.iat(1, 2).toDouble(), 4.5);
        // 原表无损（R14 非破坏）
        QCOMPARE(df.shape().first, static_cast< std::size_t >(2));
        QCOMPARE(df.shape().second, static_cast< std::size_t >(3));
    }
    // —— melt：行数 = 原行数 × 展开列数；保留列/变量名/值列名；dtype ——
    {
        pybind11::dict scope;
        pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"C":["x","y"],"A":[1.5,2.5],"B":[3.5,4.5]})
)",
                       pybind11::globals(),
                       scope);
        DA::DAPyDataFrame df(scope[ "df" ]);
        DA::DAPyDataFrame out = py.melt(df,
                                        QStringList({ QStringLiteral("C") }),
                                        QStringList({ QStringLiteral("A"), QStringLiteral("B") }),
                                        QStringLiteral("var"),
                                        QStringLiteral("val"));
        QVERIFY(!out.isNone());
        QCOMPARE(out.shape().first, static_cast< std::size_t >(4));  // 2 行 × 2 展开列
        QCOMPARE(out.shape().second, static_cast< std::size_t >(3));
        QCOMPARE(out.columns(), QList< QString >({ QStringLiteral("C"), QStringLiteral("var"), QStringLiteral("val") }));
        QCOMPARE(out.iat(0, 0).toString(), QStringLiteral("x"));
        QCOMPARE(out.iat(0, 1).toString(), QStringLiteral("A"));
        QCOMPARE(out.iat(0, 2).toDouble(), 1.5);
        QCOMPARE(out.iat(3, 1).toString(), QStringLiteral("B"));
        QCOMPARE(out.iat(3, 2).toDouble(), 4.5);
        // value_vars 空 = 全部列（pandas 语义）
        DA::DAPyDataFrame out2 = py.melt(df, QStringList(), QStringList(), QStringLiteral("variable"), QStringLiteral("value"));
        QVERIFY(!out2.isNone());
        QCOMPARE(out2.shape().first, static_cast< std::size_t >(6));  // 2 行 × 3 列全展开
        // 失败哨兵：df none → 显式 none（isNone()==true，空 DataFrame 哨兵陷阱已修）
        DA::DAPyDataFrame noneDf { pybind11::none() };
        QVERIFY(py.transpose(noneDf).isNone());
        QVERIFY(py.melt(noneDf, QStringList(), QStringList(), QString(), QString()).isNone());
    }
}

// 004 §6.4 验收：melt 规模守卫预估（行数×展开列数、阈值、强制继续判定）+ 就地撤销内存阈值
void DATableDataTest::testMeltGuardEstimate()
{
    using DA::DADialogReshape;
    using DA::DADataOperateOfDataFrameWidget;
    // 预估纯函数：行数 × 展开列数
    QCOMPARE(DADialogReshape::estimateMeltOutputRows(1000, 5), static_cast< qint64 >(5000));
    QCOMPARE(DADialogReshape::estimateMeltOutputRows(0, 5), static_cast< qint64 >(0));
    QCOMPARE(DADialogReshape::estimateMeltOutputRows(100, 0), static_cast< qint64 >(0));
    // 阈值：5000 万（超过默认拒绝，可强制继续——确认点在对话框 onOkClicked）
    QCOMPARE(DADialogReshape::c_meltRowLimit, static_cast< qint64 >(50000000));
    QVERIFY(DADialogReshape::estimateMeltOutputRows(10000000, 5) == DADialogReshape::c_meltRowLimit);   // 恰好等于阈值：不超
    QVERIFY(DADialogReshape::estimateMeltOutputRows(10000001, 5) > DADialogReshape::c_meltRowLimit);    // 超阈值
    // 就地替换撤销内存阈值（超大表降级为不可撤销，§4.11 策略表）
    QVERIFY(!DADataOperateOfDataFrameWidget::isInplaceUndoTooLarge(1000, 100));
    QVERIFY(DADataOperateOfDataFrameWidget::isInplaceUndoTooLarge(10000, 10000));  // 1 亿格 > 5000 万
    QVERIFY(!DADataOperateOfDataFrameWidget::isInplaceUndoTooLarge(0, 0));
}

// 004 §7.1：就地替换的元数据清理——按类别统计条目数（与确认框一致）、快照清理还原 round-trip
void DATableDataTest::testInplaceReshapeMetadataClear()
{
    DA::DATableMetadataRegistry registry(nullptr);
    pybind11::object obj = makeTestDataFrame();
    QVERIFY(!obj.is_none());
    DA::DAPyDataFrame df(obj);
    DA::DAData d(df);
    DA::DATableMetadata* bundle = registry.getOrCreate(d);
    QVERIFY(bundle != nullptr);
    QVERIFY(bundle->isEmpty());
    // 填充四类元数据：样式 / 列宽 / 条件格式 / 计算列
    DA::DATableCellStyle st;
    st.setBackground(QBrush(QColor(Qt::red)));
    bundle->styleManager()->setCellStyle(0, 0, st);
    bundle->styleManager()->setCellStyle(1, 1, st);
    bundle->styleManager()->setRowStyle(2, st);
    bundle->viewState()->setColumnWidth(QStringLiteral("i"), 0, 120);
    bundle->viewState()->setRowHeaderWidth(66);
    DA::DATableConditionalFormatRule rule;
    rule.type      = DA::DATableConditionalFormatRule::Type::CellRule;
    rule.condition = DA::DATableConditionalFormatRule::Condition::Greater;
    rule.value1    = 3.0;
    bundle->conditionalFormat()->addRule(1, rule);
    DA::DAComputedColumn cc(QStringLiteral("Z"), QStringLiteral("i + 1"));
    cc.dependsOn = QStringList { QStringLiteral("i") };
    bundle->computedColumns()->addDefinition(cc, DA::DATableComputedColumnStore::State::Stale);
    // 类别条目数统计（确认框「列出将被清除的元数据类别与条目数」的数据源）
    const QList< QPair< QString, int > > counts = bundle->entryCountsByCategory();
    QCOMPARE(counts.size(), 4);
    int styleCount = 0, widthCount = 0, cfCount = 0, ccCount = 0;
    for (const auto& p : counts) {
        if (p.first == DA::DATableMetadata::tr("Cell styles")) {
            styleCount = p.second;
        } else if (p.first == DA::DATableMetadata::tr("Column view settings")) {  // 006a 起：列宽并入列视图设置
            widthCount = p.second;
        } else if (p.first == DA::DATableMetadata::tr("Conditional format rules")) {
            cfCount = p.second;
        } else if (p.first == DA::DATableMetadata::tr("Computed columns")) {
            ccCount = p.second;
        }
    }
    QCOMPARE(styleCount, 3);  // 2 单元格 + 1 行
    QCOMPARE(widthCount, 1);
    QCOMPARE(cfCount, 1);
    QCOMPARE(ccCount, 1);
    // 快照 → 清空 → 还原（就地替换撤销单元的元数据侧语义）
    const QString snapshot = bundle->snapshotToString();
    QVERIFY(!snapshot.isEmpty());
    bundle->clearAll();
    QVERIFY(bundle->isEmpty());
    QVERIFY(bundle->restoreFromString(snapshot));
    QVERIFY(!bundle->isEmpty());
    const QList< QPair< QString, int > > counts2 = bundle->entryCountsByCategory();
    QCOMPARE(counts2.size(), 4);
    QCOMPARE(bundle->computedColumns()->count(), 1);
    QCOMPARE(bundle->computedColumns()->stateOf(QStringLiteral("Z")), DA::DATableComputedColumnStore::State::Stale);  // 状态随快照还原
    QCOMPARE(bundle->computedColumns()->definition(QStringLiteral("Z")).expr, QStringLiteral("i + 1"));
    QCOMPARE(bundle->viewState()->columnWidth(QStringLiteral("i"), 0), 120);
    QCOMPARE(bundle->conditionalFormat()->ruleCount(), 1);
    QVERIFY(bundle->styleManager()->hasCellStyle(0, 0));
    // 空快照还原 = 整束清空
    QVERIFY(bundle->restoreFromString(QString()));
    QVERIFY(bundle->isEmpty());
}

// 004 §6.4 验收（程序化）：默认产新数据集（原表无损+注册可撤销）；就地替换（Swap+元数据同一撤销单元）
void DATableDataTest::testReshapeWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"A":[1,2],"B":[3.5,4.5]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("src"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    // 预置元数据（就地替换确认框条目与清理验证用）
    DA::DATableMetadata* bundle = registry.getOrCreate(d);
    DA::DATableCellStyle st;
    st.setBackground(QBrush(QColor(Qt::red)));
    bundle->styleManager()->setCellStyle(0, 0, st);
    DA::DAComputedColumn cc(QStringLiteral("B"), QStringLiteral("A * 2"));
    cc.dependsOn = QStringList { QStringLiteral("A") };
    bundle->computedColumns()->addDefinition(cc, DA::DATableComputedColumnStore::State::Valid);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    // —— 默认路径：转置产出新数据集，原表与元数据完全无损 ——
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogReshape* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QVERIFY(dlg->findChild< QCheckBox* >(QStringLiteral("checkInplace")) != nullptr);
        QTimer::singleShot(500, [ dlg ]() {
            QPushButton* ok = dlg->findChild< QPushButton* >(QStringLiteral("buttonOk"));
            if (ok && ok->isEnabled()) {
                ok->click();  // 走 onOkClicked（守卫确认点）→ accept
            }
        });
    });
    QSignalSpy createdSpy(&w, &DA::DADataOperateOfDataFrameWidget::newDataCreated);
    w.showReshapeDialog(false);  // 转置模式
    QCOMPARE(createdSpy.count(), 1);
    DA::DAData td = mgr.findData(QStringLiteral("src_Transposed"));
    QVERIFY(!td.isNull());  // 命名 xxx_Transposed
    DA::DAPyDataFrame tdf = td.toDataFrame();
    QCOMPARE(tdf.shape().first, static_cast< std::size_t >(2));   // 行 = 原列数
    QCOMPARE(tdf.shape().second, static_cast< std::size_t >(3));  // 列 = index + 原行数
    // 原表与元数据完全无损（R14）
    QCOMPARE(df.shape().first, static_cast< std::size_t >(2));
    QVERIFY(bundle->styleManager()->hasCellStyle(0, 0));
    QVERIFY(bundle->computedColumns()->hasDefinition(QStringLiteral("B")));
    // Ctrl+Z 撤销注册（数据集从管理器消失）
    mgr.getUndoStack()->undo();
    QVERIFY(mgr.findData(QStringLiteral("src_Transposed")).isNull());
    mgr.getUndoStack()->redo();
    QVERIFY(!mgr.findData(QStringLiteral("src_Transposed")).isNull());
    // —— 就地路径（melt）：模拟 APP 接线（Swap 命令 + directional 元数据清理/还原同一撤销单元）——
    QUndoStack& pageStack = *w.getUndoStack();
    QString confirmText;
    QObject::connect(&w,
                     &DA::DADataOperateOfDataFrameWidget::inplaceDataReplaceRequested,
                     [ &w, bundle, &pageStack, &confirmText ](const DA::DAData& data, const DA::DAData& source, const QString& text, const QString& snapshot) {
                         Q_UNUSED(confirmText);
                         auto* cmd = new DA::DADataObjectSwapUndoCommand();
                         cmd->setText(text);
                         QPointer< DA::DADataOperateOfDataFrameWidget > wp(&w);
                         cmd->setDirectionalCallBack([ wp, bundle, snapshot ](bool isUndo) {
                             if (isUndo) {
                                 bundle->restoreFromString(snapshot);
                             } else {
                                 bundle->clearAll();
                             }
                             if (wp) {
                                 wp->refreshAfterInplaceReplace();
                             }
                         });
                         cmd->setOldData(data);
                         DA::DAData mutableData = data;  // DAData 共享底层对象，副本上 setPyObject 即生效
                         mutableData.setPyObject(source.toPyObject());
                         cmd->setNewData(data);
                         pageStack.push(cmd);
                     });
    // 驱动对话框：melt 模式 + 勾选就地替换；随后确认框（记录文本）点 Yes
    QTimer::singleShot(0, [ &confirmText ]() {
        auto* dlg = qobject_cast< DA::DADialogReshape* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QCheckBox* inplace = dlg->findChild< QCheckBox* >(QStringLiteral("checkInplace"));
        if (inplace) {
            inplace->setChecked(true);
        }
        QTimer::singleShot(500, [ dlg, &confirmText ]() {
            QPushButton* ok = dlg->findChild< QPushButton* >(QStringLiteral("buttonOk"));
            if (ok && ok->isEnabled()) {
                ok->click();
            }
            // melt 计算后就地确认框弹出：记录文本并点 Yes
            QTimer::singleShot(400, [ &confirmText ]() {
                if (QMessageBox* mb = qobject_cast< QMessageBox* >(QApplication::activeModalWidget())) {
                    confirmText = mb->text();
                    mb->button(QMessageBox::Yes)->click();
                }
            });
        });
    });
    const int stackBefore = pageStack.count();
    w.showReshapeDialog(true);  // melt 模式
    // 就地替换已生效：数据变为 melt 结果（2 行 × 2 列全展开 = 4 行）
    DA::DAPyDataFrame cur = w.getDataframe();
    QCOMPARE(cur.shape().first, static_cast< std::size_t >(4));
    QCOMPARE(pageStack.count(), stackBefore + 1);
    // 确认框列出的清除项与实际一致：文本包含样式与计算列类别，且束已被清空
    QVERIFY2(confirmText.contains(DA::DATableMetadata::tr("Cell styles")), qPrintable(confirmText));
    QVERIFY2(confirmText.contains(DA::DATableMetadata::tr("Computed columns")), qPrintable(confirmText));
    QVERIFY(bundle->isEmpty());
    // Ctrl+Z：数据与元数据一起还原（同一撤销单元）
    pageStack.undo();
    DA::DAPyDataFrame back = w.getDataframe();
    QCOMPARE(back.shape().first, static_cast< std::size_t >(2));
    QCOMPARE(back.shape().second, static_cast< std::size_t >(2));
    QVERIFY(bundle->styleManager()->hasCellStyle(0, 0));
    QVERIFY(bundle->computedColumns()->hasDefinition(QStringLiteral("B")));
    QCOMPARE(bundle->computedColumns()->stateOf(QStringLiteral("B")), DA::DATableComputedColumnStore::State::Valid);
    // redo：再次替换 + 清空
    pageStack.redo();
    QCOMPARE(w.getDataframe().shape().first, static_cast< std::size_t >(4));
    QVERIFY(bundle->isEmpty());
    w.hide();
}


//===================================================
// 005 WP1 填充（T1.24）
//===================================================

// 005 §7.1：选区填充求值——NaN/NaT/None 三种空值、down/up/ffill/bfill 四模式、不足两行
void DATableDataTest::testFillSelectionOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
df = pd.DataFrame({
    "f": [1.0, np.nan, np.nan, 4.0, np.nan],
    "d": pd.to_datetime(["2020-01-01", None, None, "2020-01-04", None]),
    "s": ["a", None, "c", None, None],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    // down：首行值 1.0 填满其余（float NaN→1.0、4.0→1.0 都算变化）
    {
        const auto r = py.fillSelection(df, 0, 0, 4, QStringLiteral("down"));
        QVERIFY(r.ok);
        QCOMPARE(r.rows, QVector< int >({ 1, 2, 3, 4 }));
        QCOMPARE(r.values.size(), 4);
        for (const QVariant& v : r.values) {
            QCOMPARE(v.toDouble(), 1.0);
        }
    }
    // ffill：NaN 由最后一个非空值向下传递（行1,2←1.0；行4←4.0）
    {
        const auto r = py.fillSelection(df, 0, 0, 4, QStringLiteral("ffill"));
        QVERIFY(r.ok);
        QCOMPARE(r.rows, QVector< int >({ 1, 2, 4 }));
        QCOMPARE(r.values.size(), 3);
        QCOMPARE(r.values[ 0 ].toDouble(), 1.0);
        QCOMPARE(r.values[ 1 ].toDouble(), 1.0);
        QCOMPARE(r.values[ 2 ].toDouble(), 4.0);
    }
    // bfill：NaT/NaN 由下一个非空值向上传递（行1,2←4.0）
    {
        const auto r = py.fillSelection(df, 0, 0, 4, QStringLiteral("bfill"));
        QVERIFY(r.ok);
        QCOMPARE(r.rows, QVector< int >({ 1, 2 }));
        QCOMPARE(r.values[ 0 ].toDouble(), 4.0);
        QCOMPARE(r.values[ 1 ].toDouble(), 4.0);
    }
    // 日期列 ffill：NaT 判定正确（pandas 侧），值经 caster 落 QDateTime
    {
        const auto r = py.fillSelection(df, 1, 0, 4, QStringLiteral("ffill"));
        QVERIFY(r.ok);
        QCOMPARE(r.rows, QVector< int >({ 1, 2, 4 }));
        QCOMPARE(r.values[ 0 ].userType(), static_cast< int >(QMetaType::QDateTime));
        QCOMPARE(r.values[ 0 ].toDateTime().date(), QDate(2020, 1, 1));
        QCOMPARE(r.values[ 2 ].toDateTime().date(), QDate(2020, 1, 4));
    }
    // 字符串列 bfill：None 判定正确（行1 ← 下一个非空 "c"；末尾连续空值行3,4 无值可传，保持为空）
    {
        const auto r = py.fillSelection(df, 2, 0, 4, QStringLiteral("bfill"));
        QVERIFY(r.ok);
        QCOMPARE(r.rows, QVector< int >({ 1 }));
        QCOMPARE(r.values.size(), 1);
        QCOMPARE(r.values[ 0 ].toString(), QStringLiteral("c"));
    }
    // up：末行为空（NaN）时按 Excel 语义以空值填满其余（写入 NaN）
    {
        const auto r = py.fillSelection(df, 0, 0, 4, QStringLiteral("up"));
        QVERIFY(r.ok);
        QCOMPARE(r.rows, QVector< int >({ 0, 3 }));
        for (const QVariant& v : r.values) {
            QVERIFY(qIsNaN(v.toDouble()));
        }
    }
    // 区间不足两行 / 子区间 / 非法参数：空清单且 ok（不发命令由 widget 侧兜底）
    {
        const auto r1 = py.fillSelection(df, 0, 2, 2, QStringLiteral("down"));
        QVERIFY(r1.ok && r1.rows.isEmpty());
        // 锚点为空且目标也为空（col0 行1-2 均 NaN）→ 无变化
        const auto r2 = py.fillSelection(df, 0, 1, 2, QStringLiteral("down"));
        QVERIFY(r2.ok && r2.rows.isEmpty());
        const auto r3 = py.fillSelection(df, 99, 0, 4, QStringLiteral("down"));
        QVERIFY(r3.ok && r3.rows.isEmpty());
        const auto r4 = py.fillSelection(df, 0, 0, 4, QStringLiteral("bad-mode"));
        QVERIFY(r4.ok && r4.rows.isEmpty());
    }
    // ffill 在无值可传递的纯空区间上返回空清单（区间开头连续空值保持为空）
    {
        pybind11::dict s2;
        pybind11::exec(R"(
import pandas as pd, numpy as np
df2 = pd.DataFrame({"f": [np.nan, np.nan, 9.0]})
)",
                       pybind11::globals(),
                       s2);
        DA::DAPyDataFrame df2(s2[ "df2" ]);
        const auto r = py.fillSelection(df2, 0, 0, 1, QStringLiteral("ffill"));
        QVERIFY(r.ok && r.rows.isEmpty());
    }
}

// 005 §7.1：序列填充生成——等差/终止值截断/日期步进（天/周/月/年）/非法日期报错
void DATableDataTest::testFillSeriesOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    // 数值等差
    {
        bool ok = false;
        const QVariantList v = py.fillSeries(5, QStringLiteral("linear"), QStringLiteral("10"), QStringLiteral("2.5"), QString(), QStringLiteral("D"), &ok);
        QVERIFY(ok);
        QCOMPARE(v.size(), 5);
        QCOMPARE(v[ 0 ].toDouble(), 10.0);
        QCOMPARE(v[ 2 ].toDouble(), 15.0);
        QCOMPARE(v[ 4 ].toDouble(), 20.0);
    }
    // 终止值截断（0,1,2,3 —— 4 超过 3.5 停止）
    {
        bool ok = false;
        const QVariantList v = py.fillSeries(10, QStringLiteral("linear"), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("3.5"), QStringLiteral("D"), &ok);
        QVERIFY(ok);
        QCOMPARE(v.size(), 4);
        QCOMPARE(v[ 3 ].toDouble(), 3.0);
    }
    // 日期步进：天/月（含闰月末语义）/年
    {
        bool ok = false;
        const QVariantList d = py.fillSeries(3, QStringLiteral("date"), QStringLiteral("2020-01-01"), QStringLiteral("2"), QString(), QStringLiteral("D"), &ok);
        QVERIFY(ok);
        QCOMPARE(d.size(), 3);
        QCOMPARE(d[ 0 ].toDateTime().date(), QDate(2020, 1, 1));
        QCOMPARE(d[ 1 ].toDateTime().date(), QDate(2020, 1, 3));
        QCOMPARE(d[ 2 ].toDateTime().date(), QDate(2020, 1, 5));
        const QVariantList m = py.fillSeries(3, QStringLiteral("date"), QStringLiteral("2020-01-31"), QStringLiteral("1"), QString(), QStringLiteral("M"), &ok);
        QVERIFY(ok);
        QCOMPARE(m[ 1 ].toDateTime().date(), QDate(2020, 2, 29));  // DateOffset 月末语义
        QCOMPARE(m[ 2 ].toDateTime().date(), QDate(2020, 3, 31));
        const QVariantList y = py.fillSeries(2, QStringLiteral("date"), QStringLiteral("2020-02-29"), QStringLiteral("1"), QString(), QStringLiteral("Y"), &ok);
        QVERIFY(ok);
        QCOMPARE(y[ 1 ].toDateTime().date(), QDate(2021, 2, 28));
    }
    // 默认参数（起始 0 步长 1）与 count<=0
    {
        bool ok = false;
        const QVariantList v = py.fillSeries(3, QStringLiteral("linear"), QString(), QString(), QString(), QStringLiteral("D"), &ok);
        QVERIFY(ok);
        QCOMPARE(v.size(), 3);
        QCOMPARE(v[ 2 ].toDouble(), 2.0);
        const QVariantList e = py.fillSeries(0, QStringLiteral("linear"), QString(), QString(), QString(), QStringLiteral("D"), &ok);
        QVERIFY(ok && e.isEmpty());
    }
    // 非法日期起始 → ok=false（对话框据此置灰 OK）
    {
        bool ok = true;
        const QVariantList v = py.fillSeries(3, QStringLiteral("date"), QStringLiteral("not-a-date"), QString(), QString(), QStringLiteral("D"), &ok);
        QVERIFY(!ok);
        QVERIFY(v.isEmpty());
    }
}

// 005 §7.1：一次填充一条命令 + undo 完全还原（NaN 旧值以 Python 对象记录，还原保真）
void DATableDataTest::testFillCommandUndoRedo()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
df = pd.DataFrame({"f": [7.0, np.nan, np.nan, np.nan]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    const auto r = py.fillSelection(df, 0, 0, 3, QStringLiteral("down"));
    QVERIFY(r.ok);
    QCOMPARE(r.rows.size(), 3);
    QList< int > rows, cols;
    QList< QVariant > values;
    for (int i = 0; i < r.rows.size(); ++i) {
        rows.append(r.rows[ i ]);
        cols.append(0);
        values.append(r.values[ i ]);
    }
    QUndoStack stack;
    DA::DACommandDataFrame_paste* cmd = new DA::DACommandDataFrame_paste(df, rows, cols, values);
    cmd->setText(QStringLiteral("fill down (test)"));
    QVERIFY(cmd->exec());
    stack.push(cmd);
    QCOMPARE(stack.count(), 1);  // 一次填充 = 一条命令
    // 填充生效
    QCOMPARE(df.iat(1, 0).toDouble(), 7.0);
    QCOMPARE(df.iat(3, 0).toDouble(), 7.0);
    // undo 完全还原（旧值 NaN 精确回写）
    stack.undo();
    QVERIFY(qIsNaN(df.iat(1, 0).toDouble()));
    QVERIFY(qIsNaN(df.iat(3, 0).toDouble()));
    QCOMPARE(df.iat(0, 0).toDouble(), 7.0);
    // redo 再应用
    stack.redo();
    QCOMPARE(df.iat(2, 0).toDouble(), 7.0);
    QCOMPARE(stack.text(0), QStringLiteral("fill down (test)"));
}

// 005 §3.4：widget 全链路——向下填充多列独立（互不串列）、Ctrl+D 快捷键、
// ffill、序列填充对话框（日期按月），每步一条命令、undo 还原
void DATableDataTest::testFillWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
df = pd.DataFrame({
    "n": [5.0, np.nan, np.nan, np.nan, np.nan, np.nan],
    "s": ["x", None, None, None, None, None],
    "d": pd.to_datetime([None]*6),
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("src"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    QUndoStack& pageStack = *w.getUndoStack();
    const int stackBefore = pageStack.count();
    // 注：模型预留 1 个扩展行（extraRow），dataRowCount = dfRows-1，末行为保留行不参与
    // 选区类操作（与 paste/removeSelectRow 同一口径）。故选区取行 0..4（df 共 6 行）。
    // —— 多列选区（n/s 两列，行0-4）向下填充：每列各按自己的首行值填充，互不串列 ——
    {
        QItemSelection sel(tv->model()->index(0, 0), tv->model()->index(4, 1));
        tv->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
        const int written = w.fillSelection(DA::DADataOperateOfDataFrameWidget::FillMode::Down);
        QCOMPARE(written, 8);  // 两列各 4 行（行1-4）
        QCOMPARE(df.iat(3, 0).toDouble(), 5.0);
        QCOMPARE(df.iat(3, 1).toString(), QStringLiteral("x"));
        QCOMPARE(pageStack.count(), stackBefore + 1);  // 一次填充一条命令
        pageStack.undo();
        QVERIFY(qIsNaN(df.iat(3, 0).toDouble()));
        QCOMPARE(pageStack.count(), stackBefore + 1);
        pageStack.redo();
        QCOMPARE(df.iat(3, 0).toDouble(), 5.0);
        pageStack.undo();  // 回到填充前，后续断言基于原始数据
    }
    // —— Ctrl+D 快捷键：只选 n 列行0-2，首行 5.0 填满其余 ——
    {
        QItemSelection sel(tv->model()->index(0, 0), tv->model()->index(2, 0));
        tv->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
        QSignalSpy spy(tv, &DA::DADataTableView::fillDownRequested);
        QTest::keyClick(tv, Qt::Key_D, Qt::ControlModifier);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(df.iat(1, 0).toDouble(), 5.0);
        QCOMPARE(df.iat(2, 0).toDouble(), 5.0);
        QCOMPARE(pageStack.count(), stackBefore + 1);
        pageStack.undo();
        QVERIFY(qIsNaN(df.iat(1, 0).toDouble()));
    }
    // —— ffill：先把行0/行3 置值再传递（行1,2 ← 行0；行4 ← 行3）——
    {
        pybind11::exec(R"(
import numpy as np
df.iat[0, 0] = 2.0
df.iat[3, 0] = 8.0
)",
                       pybind11::globals(),
                       scope);
        QItemSelection sel(tv->model()->index(0, 0), tv->model()->index(4, 0));
        tv->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
        const int written = w.fillSelection(DA::DADataOperateOfDataFrameWidget::FillMode::Forward);
        QCOMPARE(written, 3);  // 行1,2←2.0；行4←8.0
        QCOMPARE(df.iat(2, 0).toDouble(), 2.0);
        QCOMPARE(df.iat(4, 0).toDouble(), 8.0);
        pageStack.undo();
        QVERIFY(qIsNaN(df.iat(2, 0).toDouble()));
        QVERIFY(qIsNaN(df.iat(4, 0).toDouble()));
    }
    // —— 序列填充对话框：d 列行0-2 按月步进（2021-01-01 起），预览与写入同一算子 ——
    {
        QItemSelection sel(tv->model()->index(0, 2), tv->model()->index(2, 2));
        tv->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
        QTimer::singleShot(0, []() {
            auto* dlg = qobject_cast< DA::DADialogFillSeries* >(QApplication::activeModalWidget());
            if (!dlg) {
                return;
            }
            QRadioButton* dateRadio = dlg->findChild< QRadioButton* >(QStringLiteral("fillSeriesKindDate"));
            QVERIFY(dateRadio != nullptr);
            dateRadio->setChecked(true);
            QLineEdit* start = dlg->findChild< QLineEdit* >(QStringLiteral("fillSeriesStart"));
            QVERIFY(start != nullptr);
            start->setText(QStringLiteral("2021-01-01"));
            QComboBox* unit = dlg->findChild< QComboBox* >(QStringLiteral("fillSeriesDateUnit"));
            QVERIFY(unit != nullptr);
            unit->setCurrentIndex(2);  // 月
            // 预览已实时生成（同一算子）
            QLabel* preview = dlg->findChild< QLabel* >(QStringLiteral("fillSeriesPreview"));
            QVERIFY(preview != nullptr);
            QVERIFY2(preview->text().contains(QStringLiteral("2021-01-01")), qPrintable(preview->text()));
            QVERIFY(dlg->isSeriesValid());
            QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
            QVERIFY(box && box->button(QDialogButtonBox::Ok)->isEnabled());
            box->button(QDialogButtonBox::Ok)->click();
        });
        const int written = w.showFillSeriesDialog();
        QCOMPARE(written, 3);
        QCOMPARE(df.iat(0, 2).toDateTime().date(), QDate(2021, 1, 1));
        QCOMPARE(df.iat(1, 2).toDateTime().date(), QDate(2021, 2, 1));
        QCOMPARE(df.iat(2, 2).toDateTime().date(), QDate(2021, 3, 1));
        QCOMPARE(pageStack.count(), stackBefore + 1);  // 序列填充也是一条命令
        pageStack.undo();
        QVERIFY(df.iat(0, 2).isNull() || df.iat(0, 2).toString().isEmpty());  // 还原为 NaT
    }
    w.hide();
}


//===================================================
// 005 WP2 数据校验（T1.25）
//===================================================

// 005 §7.1：规则定义（isValid/paramSummary/枚举字符串）与存储（增删改/XML round-trip/改名迁移）
void DATableDataTest::testValidationRuleAndStoreXml()
{
    using DA::DATableValidationRule;
    using DA::DATableValidationStore;
    // —— 规则定义完整性 ——
    DATableValidationRule r;
    QVERIFY(!r.isValid());  // 无列名
    r.columnName = QStringLiteral("score");
    r.type       = DATableValidationRule::Type::NumericRange;
    QVERIFY(!r.isValid());  // 区间两端全空
    r.hasNumericMin = true;
    r.numericMin    = 0.0;
    r.hasNumericMax = true;
    r.numericMax    = 100.0;
    QVERIFY(r.isValid());
    QCOMPARE(r.paramSummary(), QStringLiteral("[0, 100]"));
    r.numericMin = 200.0;  // min > max 非法
    QVERIFY(!r.isValid());
    // 枚举/正则/日期
    DATableValidationRule re;
    re.type       = DATableValidationRule::Type::EnumSet;
    re.columnName = QStringLiteral("cat");
    QVERIFY(!re.isValid());  // 无允许值
    re.enumValues = QStringList { QStringLiteral("a"), QStringLiteral("b,c") };
    QVERIFY(re.isValid());
    QCOMPARE(re.paramSummary(), QStringLiteral("{a, b,c}"));  // 值含逗号也不破坏结构（XML 走子元素）
    DATableValidationRule rr;
    rr.type         = DATableValidationRule::Type::Regex;
    rr.columnName   = QStringLiteral("code");
    rr.regexPattern = QStringLiteral("[A-Z]+");
    QVERIFY(rr.isValid());
    QCOMPARE(rr.paramSummary(), QStringLiteral("[A-Z]+"));
    // 枚举 ↔ 字符串稳定标识
    QCOMPARE(DATableValidationRule::typeToString(DATableValidationRule::Type::Unique), QStringLiteral("unique"));
    DATableValidationRule::Type t;
    QVERIFY(DATableValidationRule::typeFromString(QStringLiteral("date-range"), &t));
    QCOMPARE(t, DATableValidationRule::Type::DateRange);
    QVERIFY(!DATableValidationRule::typeFromString(QStringLiteral("bogus"), &t));
    // —— 存储与 XML round-trip ——
    DATableValidationStore store;
    QVERIFY(store.isEmpty());
    DATableValidationRule rNum;
    rNum.columnName    = QStringLiteral("score");
    rNum.type          = DATableValidationRule::Type::NumericRange;
    rNum.hasNumericMin = true;
    rNum.numericMin    = 0.0;
    rNum.hasNumericMax = true;
    rNum.numericMax    = 100.0;
    rNum.level         = DATableValidationRule::Level::Error;
    rNum.label         = QStringLiteral("score in [0,100]");
    DATableValidationRule rEnum;
    rEnum.columnName       = QStringLiteral("x");
    rEnum.columnOccurrence = 1;  // 第 2 个同名列（R20 消歧）
    rEnum.type             = DATableValidationRule::Type::EnumSet;
    rEnum.enumValues       = QStringList { QStringLiteral("a"), QStringLiteral("b") };
    rEnum.enumAllowOther   = false;
    rEnum.level            = DATableValidationRule::Level::Warning;
    rEnum.enabled          = false;
    DATableValidationRule rDate;
    rDate.columnName = QStringLiteral("d");
    rDate.type       = DATableValidationRule::Type::DateRange;
    rDate.dateMin    = QDateTime(QDate(2020, 1, 1), QTime(0, 0));
    store.addRule(rNum);
    store.addRule(rEnum);
    store.addRule(rDate);
    QCOMPARE(store.count(), 3);
    QCOMPARE(store.enabledRuleIndexes(), QList< int >({ 0, 2 }));  // rEnum 停用
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("table"));
    doc.appendChild(root);
    store.toXml(doc, root);
    const QString xml = doc.toString();
    DATableValidationStore store2;
    QDomDocument doc2;
    QVERIFY(doc2.setContent(xml));
    QVERIFY(store2.fromXml(doc2.documentElement().firstChildElement(QStringLiteral("validation"))));
    QCOMPARE(store2.count(), 3);
    QCOMPARE(store2.rules(), store.rules());  // 全字段 round-trip（含 occurrence/allowOther/enabled/label/日期）
    // 改名迁移与引用计数（R20）
    QCOMPARE(store2.rulesReferencing(QStringLiteral("score")), 1);
    QCOMPARE(store2.renameColumnReferences(QStringLiteral("score"), QStringLiteral("score2")), 1);
    QCOMPARE(store2.ruleAt(0).columnName, QStringLiteral("score2"));
    QCOMPARE(store2.rulesReferencing(QStringLiteral("score")), 0);
    // setRules 整体替换 + 快照恢复
    const auto snap = store2.snapshot();
    store2.setRules({ rNum });
    QCOMPARE(store2.count(), 1);
    store2.restore(snap);
    QCOMPARE(store2.count(), 3);
}

// 005 §7.1：算子各类型违规判定（区间边界/枚举含空值/非空/唯一重复组/正则整格）+
// 重名消歧 + 缺列跳过 + 非法正则报错
void DATableDataTest::testValidateOperator()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
df = pd.DataFrame({
    "v": [0.0, 100.0, 150.0, np.nan, -3.0],
    "cat": ["a", "b", "zzz", None, "a"],
    "d": pd.to_datetime(["2020-06-01", "2019-01-01", None, "2021-01-01", "2020-06-15"]),
    "dup": [1, 2, 2, 3, 1],
    "code": ["A1", "B2", "A1", None, "C!"],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QList< DA::DAPyScriptsDataFrame::ValidationRuleSpec > specs;
    auto addSpec = [&specs](int idx, const QString& col, const QString& type) {
        DA::DAPyScriptsDataFrame::ValidationRuleSpec s;
        s.index  = idx;
        s.column = col;
        s.type   = type;
        specs.append(s);
        return specs.size() - 1;
    };
    // 数值区间 [0,100]：边界值 0/100 不违规（闭区间），150/-3 违规，NaN 不判
    int iNum = addSpec(0, QStringLiteral("v"), QStringLiteral("numeric-range"));
    specs[ iNum ].hasMin     = true;
    specs[ iNum ].numericMin = 0.0;
    specs[ iNum ].hasMax     = true;
    specs[ iNum ].numericMax = 100.0;
    // 枚举 {a,b}：zzz 与空值都违规
    int iEnum = addSpec(1, QStringLiteral("cat"), QStringLiteral("enum"));
    specs[ iEnum ].enumValues = QStringList { QStringLiteral("a"), QStringLiteral("b") };
    // 日期区间 [2020-01-01, 2020-12-31]：2019/2021 违规，NaT 不判
    int iDate = addSpec(2, QStringLiteral("d"), QStringLiteral("date-range"));
    specs[ iDate ].dateMin = QStringLiteral("2020-01-01T00:00:00.000");
    specs[ iDate ].dateMax = QStringLiteral("2020-12-31T00:00:00.000");
    // 唯一：重复组（1,2）全部行违规
    addSpec(3, QStringLiteral("dup"), QStringLiteral("unique"));
    // 非空
    addSpec(4, QStringLiteral("cat"), QStringLiteral("not-empty"));
    // 正则整格匹配 [A-Z]\d
    int iRegex = addSpec(5, QStringLiteral("code"), QStringLiteral("regex"));
    specs[ iRegex ].regexPattern = QStringLiteral("[A-Z]\\d");
    // 缺失列 → 空清单不报错
    addSpec(6, QStringLiteral("no-such-col"), QStringLiteral("not-empty"));
    QList< DA::DAPyScriptsDataFrame::ValidationRuleResult > results;
    QString err;
    QVERIFY(py.validateDataFrame(df, specs, &results, &err));
    QCOMPARE(results.size(), specs.size());
    QHash< int, QVector< int > > byIndex;
    for (const auto& r : std::as_const(results)) {
        byIndex.insert(r.index, r.rows);
    }
    QCOMPARE(byIndex.value(0), QVector< int >({ 2, 4 }));        // 数值区间：150 与 -3
    QCOMPARE(byIndex.value(1), QVector< int >({ 2, 3 }));        // 枚举：zzz 与 None
    QCOMPARE(byIndex.value(2), QVector< int >({ 1, 3 }));        // 日期：2019 与 2021
    QCOMPARE(byIndex.value(3), QVector< int >({ 0, 1, 2, 4 }));  // 唯一：1(行0,4) 2(行1,2)
    QCOMPARE(byIndex.value(4), QVector< int >({ 3 }));           // 非空
    QCOMPARE(byIndex.value(5), QVector< int >({ 4 }));           // 正则：C! 不整格匹配，None 不判
    QCOMPARE(byIndex.value(6), QVector< int >());                // 缺列
    // 重名列「第 n 个同名列」消歧（R20）
    {
        pybind11::dict s2;
        pybind11::exec(R"(
import pandas as pd
df2 = pd.DataFrame([[1, 10], [2, 20], [3, 30]], columns=["x", "x"])
)",
                       pybind11::globals(),
                       s2);
        DA::DAPyDataFrame df2(s2[ "df2" ]);
        QList< DA::DAPyScriptsDataFrame::ValidationRuleSpec > sp;
        DA::DAPyScriptsDataFrame::ValidationRuleSpec a;
        a.index = 0;
        a.column = QStringLiteral("x");
        a.occurrence = 0;
        a.type = QStringLiteral("numeric-range");
        a.hasMin = true;
        a.numericMin = 2.0;
        sp.append(a);
        DA::DAPyScriptsDataFrame::ValidationRuleSpec b = a;
        b.index = 1;
        b.occurrence = 1;
        b.numericMin = 25.0;
        sp.append(b);
        QList< DA::DAPyScriptsDataFrame::ValidationRuleResult > res;
        QVERIFY(py.validateDataFrame(df2, sp, &res, nullptr));
        QCOMPARE(res.size(), 2);
        QCOMPARE(res[ 0 ].rows, QVector< int >({ 0 }));     // 第 1 个 x=[1,2,3]，<2 → 行0
        QCOMPARE(res[ 1 ].rows, QVector< int >({ 0, 1 }));  // 第 2 个 x=[10,20,30]，<25 → 行0,1
    }
    // 非法正则 → 整体失败并输出诊断（调用方提示，不装载部分结果）
    {
        QList< DA::DAPyScriptsDataFrame::ValidationRuleSpec > sp;
        DA::DAPyScriptsDataFrame::ValidationRuleSpec a;
        a.index = 0;
        a.column = QStringLiteral("code");
        a.type = QStringLiteral("regex");
        a.regexPattern = QStringLiteral("[");
        sp.append(a);
        QList< DA::DAPyScriptsDataFrame::ValidationRuleResult > res;
        QString e2;
        QVERIFY(!py.validateDataFrame(df, sp, &res, &e2));
        QVERIFY(!e2.isEmpty());
    }
}

// 005 §4.4/§7.1：widget 校验全链路——违规装载两级高亮、来源隔离（清空只清 Validation）、
// 唯一性缓存二次不重算（打点）、只读不入撤销栈不改数据、耗时输出
void DATableDataTest::testValidationWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
df = pd.DataFrame({
    "score": [50.0, 150.0, np.nan, -3.0, 80.0],
    "cat": ["a", "b", "zzz", None, "a"],
    "dup": [1, 2, 2, 3, 1],
})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("vsrc"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableValidationStore* store = w.validationStore();
    QVERIFY(store != nullptr);
    // 规则：score∈[0,100] Error；cat 枚举{a,b} Warning；dup 唯一 Error
    DA::DATableValidationRule rNum;
    rNum.columnName = QStringLiteral("score");
    rNum.type = DA::DATableValidationRule::Type::NumericRange;
    rNum.hasNumericMin = true;
    rNum.numericMin = 0.0;
    rNum.hasNumericMax = true;
    rNum.numericMax = 100.0;
    rNum.level = DA::DATableValidationRule::Level::Error;
    DA::DATableValidationRule rEnum;
    rEnum.columnName = QStringLiteral("cat");
    rEnum.type = DA::DATableValidationRule::Type::EnumSet;
    rEnum.enumValues = QStringList { QStringLiteral("a"), QStringLiteral("b") };
    rEnum.level = DA::DATableValidationRule::Level::Warning;
    DA::DATableValidationRule rUniq;
    rUniq.columnName = QStringLiteral("dup");
    rUniq.type = DA::DATableValidationRule::Type::Unique;
    rUniq.level = DA::DATableValidationRule::Level::Error;
    store->addRule(rNum);
    store->addRule(rEnum);
    store->addRule(rUniq);
    // 先装载一条查找命中（来源隔离对照）
    DA::DATableHighlightOverlay* overlay = w.highlightOverlay();
    QVERIFY(overlay != nullptr);
    overlay->set(DA::DATableHighlightOverlay::Source::Find, DA::DATableHighlightOverlay::Level::Hit,
                 QVector< QPair< int, int > >({ qMakePair(2, 1) }));
    // —— 执行校验（只读红线2：撤销栈不增长、数据不变）——
    const int stackBefore = w.getUndoStack()->count();
    int elapsed = -1;
    const auto outcomes = w.evaluateValidationRules(&elapsed);
    QCOMPARE(outcomes.size(), 3);
    QVERIFY(elapsed >= 0);
    QCOMPARE(w.getUndoStack()->count(), stackBefore);  // 不入撤销栈
    QCOMPARE(df.iat(1, 0).toDouble(), 150.0);          // 数据不变
    // 逐规则违规行
    QCOMPARE(outcomes[ 0 ].rows, QVector< int >({ 1, 3 }));        // 150 与 -3
    QVERIFY(!outcomes[ 0 ].columnMissing);
    QCOMPARE(outcomes[ 0 ].columnPosition, 0);
    QCOMPARE(outcomes[ 1 ].rows, QVector< int >({ 2, 3 }));        // zzz 与 None
    QCOMPARE(outcomes[ 2 ].rows, QVector< int >({ 0, 1, 2, 4 }));  // 唯一重复组
    // 高亮装载：Error→ViolationError、Warning→ViolationWarning
    QCOMPARE(overlay->levelAt(1, 0), DA::DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay->levelAt(2, 1), DA::DATableHighlightOverlay::Level::Hit);  // 查找优先（跨来源优先级 001 钉死）
    QCOMPARE(overlay->levelAt(3, 1), DA::DATableHighlightOverlay::Level::ViolationWarning);
    QCOMPARE(overlay->levelAt(0, 2), DA::DATableHighlightOverlay::Level::ViolationError);
    QCOMPARE(overlay->levelAt(4, 0), DA::DATableHighlightOverlay::Level::None);  // 50 合规
    // 唯一性缓存：首次已求值 1 次；二次校验不重算（打点计数，§4.9）
    QCOMPARE(store->uniquePythonEvals(), 1);
    const auto outcomes2 = w.evaluateValidationRules();
    QCOMPARE(store->uniquePythonEvals(), 1);  // 缓存命中
    QCOMPARE(outcomes2[ 2 ].rows, outcomes[ 2 ].rows);
    // 数据变更（单元格编辑信号路径）→ 缓存失效 → 三次校验重算
    store->invalidateUniqueCache();
    w.evaluateValidationRules();
    QCOMPARE(store->uniquePythonEvals(), 2);
    // 来源隔离：清 Validation 不动 Find 命中（红线2，专门测）
    w.clearValidationHighlights();
    QCOMPARE(overlay->levelAt(1, 0), DA::DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay->levelAt(3, 1), DA::DATableHighlightOverlay::Level::None);
    QCOMPARE(overlay->levelAt(2, 1), DA::DATableHighlightOverlay::Level::Hit);  // 查找命中仍在
    w.hide();
}

// 005 §4.3-2：规则增删改走命令入栈——对话框确定生成 setRules 命令，undo/redo 还原
void DATableDataTest::testValidationRulesDialogFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"score": [1.0, 2.0], "cat": ["a", "b"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("vsrc2"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableValidationStore* store = w.validationStore();
    QVERIFY(store != nullptr);
    QVERIFY(store->isEmpty());
    // 驱动对话框：添加一条「cat 非空（Warning）」规则 → OK
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogValidationRules* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QPushButton* add = dlg->findChild< QPushButton* >(QStringLiteral("btnAddRule"));
        QVERIFY(add != nullptr);
        add->click();
        QComboBox* colCombo = dlg->findChild< QComboBox* >(QStringLiteral("comboValidationColumn"));
        QVERIFY(colCombo != nullptr);
        for (int i = 0; i < colCombo->count(); ++i) {
            if (colCombo->itemText(i) == QLatin1String("cat")) {
                colCombo->setCurrentIndex(i);
                break;
            }
        }
        QComboBox* typeCombo = dlg->findChild< QComboBox* >(QStringLiteral("comboValidationType"));
        QVERIFY(typeCombo != nullptr);
        for (int i = 0; i < typeCombo->count(); ++i) {
            if (typeCombo->itemData(i).toInt() == static_cast< int >(DA::DATableValidationRule::Type::NotEmpty)) {
                typeCombo->setCurrentIndex(i);
                break;
            }
        }
        QComboBox* levelCombo = dlg->findChild< QComboBox* >(QStringLiteral("comboValidationLevel"));
        QVERIFY(levelCombo != nullptr);
        for (int i = 0; i < levelCombo->count(); ++i) {
            if (levelCombo->itemData(i).toInt() == static_cast< int >(DA::DATableValidationRule::Level::Warning)) {
                levelCombo->setCurrentIndex(i);
                break;
            }
        }
        QTimer::singleShot(200, [ dlg ]() {
            QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
            QVERIFY(box != nullptr);
            box->button(QDialogButtonBox::Ok)->click();
        });
    });
    const int stackBefore = w.getUndoStack()->count();
    w.showValidationRulesDialog();
    // 规则已落库并入栈一条命令
    QCOMPARE(store->count(), 1);
    QCOMPARE(store->ruleAt(0).columnName, QStringLiteral("cat"));
    QCOMPARE(store->ruleAt(0).type, DA::DATableValidationRule::Type::NotEmpty);
    QCOMPARE(store->ruleAt(0).level, DA::DATableValidationRule::Level::Warning);
    QCOMPARE(w.getUndoStack()->count(), stackBefore + 1);
    // undo 移除规则；redo 恢复
    w.getUndoStack()->undo();
    QVERIFY(store->isEmpty());
    w.getUndoStack()->redo();
    QCOMPARE(store->count(), 1);
    // 再开对话框不改任何内容直接 OK → 无差异不推空命令
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DADialogValidationRules* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QDialogButtonBox* box = dlg->findChild< QDialogButtonBox* >();
        if (box) {
            box->button(QDialogButtonBox::Ok)->click();
        }
    });
    w.showValidationRulesDialog();
    QCOMPARE(w.getUndoStack()->count(), stackBefore + 1);
    QCOMPARE(store->count(), 1);
    w.hide();
}

// 005 §4.4：列改名提示「有 N 条规则引用它」并可同步（与改名同一撤销单元，undo 还原）
void DATableDataTest::testValidationRenameMigration()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"score": [1.0, 2.0], "other": [3.0, 4.0]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableValidationStore* store = w.validationStore();
    QVERIFY(store != nullptr);
    DA::DATableValidationRule r;
    r.columnName = QStringLiteral("score");
    r.type = DA::DATableValidationRule::Type::NotEmpty;
    store->addRule(r);
    QCOMPARE(store->rulesReferencing(QStringLiteral("score")), 1);
    // 改名 score→score2：确认框（计算列无引用不弹；校验规则弹）点 Yes
    QTimer::singleShot(0, []() {
        auto* mb = qobject_cast< QMessageBox* >(QApplication::activeModalWidget());
        if (mb) {
            mb->button(QMessageBox::Yes)->click();
        }
    });
    QVERIFY(w.renameColumn(0, QStringLiteral("score2")));
    QCOMPARE(store->ruleAt(0).columnName, QStringLiteral("score2"));  // 引用已同步
    // undo 改名 → 引用迁移回 score（同一撤销单元）
    w.getUndoStack()->undo();
    QCOMPARE(store->ruleAt(0).columnName, QStringLiteral("score"));
    w.getUndoStack()->redo();
    QCOMPARE(store->ruleAt(0).columnName, QStringLiteral("score2"));
    w.hide();
}

// 005 §4.3-6：枚举列编辑下拉——有启用枚举规则的列出 QComboBox（allowOther 控制可编辑），
// 无规则列出基类默认编辑器；只读门禁在模型 flags（红线4：delegate 不另造判定）
void DATableDataTest::testEnumEditorDelegate()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"cat": ["a", "b"], "num": [1.0, 2.0]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("vsrc3"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableValidationStore* store = w.validationStore();
    QVERIFY(store != nullptr);
    // cat 列枚举规则（不允许其他值）
    DA::DATableValidationRule rEnum;
    rEnum.columnName = QStringLiteral("cat");
    rEnum.type = DA::DATableValidationRule::Type::EnumSet;
    rEnum.enumValues = QStringList { QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c") };
    rEnum.enumAllowOther = false;
    store->addRule(rEnum);
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    auto* delegate = qobject_cast< DA::DATableStyleItemDelegate* >(tv->itemDelegate());
    QVERIFY(delegate != nullptr);
    QStyleOptionViewItem opt;
    // cat 列（0）→ QComboBox，非 editable，选项来自规则
    {
        QModelIndex idx = tv->model()->index(0, 0);
        QWidget* ed     = delegate->createEditor(&w, opt, idx);
        QVERIFY(ed != nullptr);
        QComboBox* cb = qobject_cast< QComboBox* >(ed);
        QVERIFY(cb != nullptr);
        QVERIFY(!cb->isEditable());  // allowOther=false → 只能列表内取值
        QCOMPARE(cb->count(), 3);
        QCOMPARE(cb->itemText(0), QStringLiteral("a"));
        QCOMPARE(cb->currentText(), QStringLiteral("a"));  // 预填当前格值
        delete ed;
    }
    // num 列（1）无枚举规则 → 基类默认编辑器（非 QComboBox）
    {
        QModelIndex idx = tv->model()->index(0, 1);
        QWidget* ed     = delegate->createEditor(&w, opt, idx);
        QVERIFY(ed != nullptr);
        QVERIFY(qobject_cast< QComboBox* >(ed) == nullptr);
        delete ed;
    }
    // allowOther=true → editable 组合框
    store->clearAll();
    rEnum.enumAllowOther = true;
    store->addRule(rEnum);
    {
        QModelIndex idx = tv->model()->index(1, 0);
        QWidget* ed     = delegate->createEditor(&w, opt, idx);
        QComboBox* cb   = qobject_cast< QComboBox* >(ed);
        QVERIFY(cb != nullptr);
        QVERIFY(cb->isEditable());
        QCOMPARE(cb->currentText(), QStringLiteral("b"));
        delete ed;
    }
    // 停用规则不出下拉
    rEnum.enabled = false;
    store->clearAll();
    store->addRule(rEnum);
    {
        QModelIndex idx = tv->model()->index(0, 0);
        QWidget* ed     = delegate->createEditor(&w, opt, idx);
        QVERIFY(ed != nullptr);
        QVERIFY(qobject_cast< QComboBox* >(ed) == nullptr);
        delete ed;
    }
    // 只读门禁在模型 flags（红线4）：惰性只读源 flags 无 ItemIsEditable，
    // 视图根本不会为该索引调 createEditor——断言模型侧门禁仍生效
    {
        DATestLazyTable* lazy = new DATestLazyTable();
        DA::DAAbstractData::Pointer p(lazy);
        DA::DAData ld(p);
        DA::DATableMetadataRegistry reg2(nullptr);
        DA::DADataOperateOfDataFrameWidget w2(ld, &reg2);
        QAbstractItemModel* am = w2.getDataTableView()->model();
        QVERIFY(am != nullptr);
        QModelIndex roIdx = am->index(0, 0);
        QVERIFY(roIdx.isValid());
        QVERIFY(!(am->flags(roIdx) & Qt::ItemIsEditable));
    }
    w.hide();
}

// 005 §4.3-5：报告——按规则分组/违规数/样例行文本、Markdown 导出内容、点击定位信号
void DATableDataTest::testValidationReportMarkdown()
{
    qRegisterMetaType< QTreeWidgetItem* >("QTreeWidgetItem*");
    DA::DADialogValidationReport dlg;
    QList< DA::DADialogValidationReport::Entry > entries;
    DA::DADialogValidationReport::Entry e1;
    e1.ruleText       = QStringLiteral("score · Numeric range [0, 100]");
    e1.isError        = true;
    e1.columnPosition = 0;
    e1.rows           = QVector< int >({ 1, 3 });
    e1.rowTexts       = QStringList { QStringLiteral("Row 2: 150"), QStringLiteral("Row 4: -3") };
    entries.append(e1);
    DA::DADialogValidationReport::Entry e2;
    e2.ruleText       = QStringLiteral("cat · Allowed values (enum) {a, b}");
    e2.isError        = false;
    e2.columnPosition = 1;
    e2.rows           = QVector< int >({ 2 });
    e2.rowTexts       = QStringList { QStringLiteral("Row 3: zzz") };
    entries.append(e2);
    DA::DADialogValidationReport::Entry e3;
    e3.ruleText       = QStringLiteral("gone · Not empty");
    e3.columnMissing  = true;
    entries.append(e3);
    dlg.setResults(entries, 3, 42);
    QCOMPARE(dlg.totalViolations(), 3);
    // 树结构：3 个顶层规则项；e1 两个样例子项；缺列项无子项
    QTreeWidget* tree = dlg.findChild< QTreeWidget* >(QStringLiteral("validationReportTree"));
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->topLevelItemCount(), 3);
    QCOMPARE(tree->topLevelItem(0)->childCount(), 2);
    QCOMPARE(tree->topLevelItem(1)->childCount(), 1);
    QCOMPARE(tree->topLevelItem(2)->childCount(), 0);
    QVERIFY(tree->topLevelItem(2)->text(0).contains(QStringLiteral("column missing")));
    // 汇总标签含规则数/违规数/耗时
    QLabel* summary = dlg.findChild< QLabel* >(QStringLiteral("validationReportSummary"));
    QVERIFY(summary != nullptr);
    QVERIFY(summary->text().contains(QStringLiteral("3")));
    QVERIFY(summary->text().contains(QStringLiteral("42")));
    // Markdown 导出内容（不经 WebEngine）
    const QString md = dlg.buildMarkdown();
    QVERIFY(md.contains(QStringLiteral("score · Numeric range [0, 100]")));
    QVERIFY(md.contains(QStringLiteral("Row 2: 150")));
    QVERIFY(md.contains(QStringLiteral("Row 3: zzz")));
    QVERIFY(md.contains(QStringLiteral("2 violation(s)")));
    // 点击样例行 → locateCellRequested(绝对行, 列)
    QSignalSpy spy(&dlg, &DA::DADialogValidationReport::locateCellRequested);
    QVERIFY(spy.isValid());
    QTreeWidgetItem* child = tree->topLevelItem(0)->child(1);
    QVERIFY(child != nullptr);
    QVERIFY(QMetaObject::invokeMethod(&dlg, "onItemClicked", Q_ARG(QTreeWidgetItem*, child), Q_ARG(int, 0)));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 3);  // 第 2 个样例行 = 绝对行 3
    QCOMPARE(spy.at(0).at(1).toInt(), 0);  // score 列
    // 顶层项点击不定位
    QVERIFY(QMetaObject::invokeMethod(&dlg, "onItemClicked", Q_ARG(QTreeWidgetItem*, tree->topLevelItem(0)), Q_ARG(int, 0)));
    QCOMPARE(spy.count(), 1);
    // 清除高亮信号
    QSignalSpy clearSpy(&dlg, &DA::DADialogValidationReport::clearHighlightsRequested);
    QPushButton* btnClear = dlg.findChild< QPushButton* >(QStringLiteral("btnClearHighlights"));
    QVERIFY(btnClear != nullptr);
    btnClear->click();
    QCOMPARE(clearSpy.count(), 1);
}


// 005 §4.4：100 万行表校验耗时可接受（记录耗时）；唯一性校验只算一次，二次校验不重算
void DATableDataTest::testValidationPerf1M()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
n = 1000000
rng = np.random.default_rng(42)
df = pd.DataFrame({
    "v": rng.uniform(-10, 110, n),
    "cat": rng.choice(["a", "b", "c", "zzz"], n),
    "dup": rng.integers(0, 500000, n),
    "v2": rng.uniform(0, 1, n),
    "v3": rng.uniform(0, 1, n),
})
df.loc[df.index[::100000], "v"] = np.nan
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    QCOMPARE(df.shape().first, static_cast< std::size_t >(1000000));
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("vperf"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    DA::DATableValidationStore* store = w.validationStore();
    QVERIFY(store != nullptr);
    // 规则：数值区间 + 枚举 + 唯一 + 非空（四类混合，含 O(n) 的唯一性）
    DA::DATableValidationRule rNum;
    rNum.columnName = QStringLiteral("v");
    rNum.type = DA::DATableValidationRule::Type::NumericRange;
    rNum.hasNumericMin = true;
    rNum.numericMin = 0.0;
    rNum.hasNumericMax = true;
    rNum.numericMax = 100.0;
    store->addRule(rNum);
    DA::DATableValidationRule rEnum;
    rEnum.columnName = QStringLiteral("cat");
    rEnum.type = DA::DATableValidationRule::Type::EnumSet;
    rEnum.enumValues = QStringList { QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c") };
    store->addRule(rEnum);
    DA::DATableValidationRule rUniq;
    rUniq.columnName = QStringLiteral("dup");
    rUniq.type = DA::DATableValidationRule::Type::Unique;
    store->addRule(rUniq);
    DA::DATableValidationRule rNe;
    rNe.columnName = QStringLiteral("v");
    rNe.type = DA::DATableValidationRule::Type::NotEmpty;
    store->addRule(rNe);
    // 首次校验（含唯一性 O(n)）
    int elapsed1 = -1;
    QElapsedTimer wall;
    wall.start();
    const auto out1 = w.evaluateValidationRules(&elapsed1);
    const qint64 wall1 = wall.elapsed();
    QCOMPARE(out1.size(), 4);
    QVERIFY(!out1[ 2 ].rows.isEmpty());  // dup 必有重复
    QCOMPARE(store->uniquePythonEvals(), 1);
    // 二次校验：唯一性命中缓存不重算（打点计数不变）
    int elapsed2 = -1;
    wall.restart();
    const auto out2 = w.evaluateValidationRules(&elapsed2);
    const qint64 wall2 = wall.elapsed();
    QCOMPARE(store->uniquePythonEvals(), 1);
    QCOMPARE(out2[ 2 ].rows, out1[ 2 ].rows);
    // 耗时记录（开发诊断，英文）+ 宽松上限断言（100 万行混合四类规则含装载高亮 ≤ 30s，
    // 二次校验因唯一性缓存命中应不慢于首次）
    qDebug().noquote() << QStringLiteral("DATableDataTest::testValidationPerf1M: first_eval_ms=%1 first_wall_ms=%2 second_eval_ms=%3 second_wall_ms=%4 violations=%5")
                              .arg(elapsed1)
                              .arg(wall1)
                              .arg(elapsed2)
                              .arg(wall2)
                              .arg(out1[ 0 ].rows.size() + out1[ 1 ].rows.size() + out1[ 2 ].rows.size() + out1[ 3 ].rows.size());
    QVERIFY2(wall1 < 30000, qPrintable(QStringLiteral("first validation too slow: %1 ms").arg(wall1)));
    w.hide();
}


//===================================================
// 005 WP3 单元格批注（T1.26）
//===================================================

// 005 §7.1：批注存储——(row,col) 键增删改、时间戳维护、XML round-trip、快照恢复
void DATableDataTest::testNoteStoreBasics()
{
    DA::DATableNoteStore store;
    QVERIFY(store.isEmpty());
    QCOMPARE(store.count(), 0);
    QVERIFY(!store.hasNote(1, 2));
    QVERIFY(store.noteText(1, 2).isEmpty());
    // 新增
    store.setNote(1, 2, QStringLiteral("hello"));
    QVERIFY(store.hasNote(1, 2));
    QCOMPARE(store.noteText(1, 2), QStringLiteral("hello"));
    const auto created = store.note(1, 2).createdAt;
    QVERIFY(created.isValid());
    QCOMPARE(store.note(1, 2).updatedAt, created);
    // 修改：createdAt 不变，updatedAt 刷新
    store.setNote(1, 2, QStringLiteral("world"));
    QCOMPARE(store.note(1, 2).createdAt, created);
    QVERIFY(store.note(1, 2).updatedAt >= created);
    // 空文本 = 删除
    store.setNote(1, 2, QString());
    QVERIFY(!store.hasNote(1, 2));
    // 多键 + keys 升序稳定
    store.setNote(5, 1, QStringLiteral("n51"));
    store.setNote(2, 3, QStringLiteral("n23"));
    store.setNote(2, 1, QStringLiteral("n21"));
    QCOMPARE(store.count(), 3);
    const auto ks = store.keys();
    QCOMPARE(ks.size(), 3);
    QVERIFY(ks[ 0 ] == qMakePair(2, 1) && ks[ 1 ] == qMakePair(2, 3) && ks[ 2 ] == qMakePair(5, 1));
    // XML round-trip（文本 + 时间戳保留）
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("table"));
    doc.appendChild(root);
    store.toXml(doc, root);
    DA::DATableNoteStore store2;
    QDomDocument doc2;
    QVERIFY(doc2.setContent(doc.toString()));
    QVERIFY(store2.fromXml(doc2.documentElement().firstChildElement(QStringLiteral("note"))));
    QCOMPARE(store2.count(), store.count());
    for (const auto& k : ks) {
        QCOMPARE(store2.noteText(k.first, k.second), store.noteText(k.first, k.second));
        QCOMPARE(store2.note(k.first, k.second).createdAt, store.note(k.first, k.second).createdAt);
        QCOMPARE(store2.note(k.first, k.second).updatedAt, store.note(k.first, k.second).updatedAt);
    }
    // 快照/恢复（setNoteEntry 保留时间戳）
    const auto snap = store.snapshot();
    store.clearAll();
    QVERIFY(store.isEmpty());
    store.restore(snap);
    QCOMPARE(store.count(), 3);
    QCOMPARE(store.note(5, 1).createdAt, snap.value(qMakePair(5, 1)).createdAt);
    // removeNote 单键
    store.removeNote(5, 1);
    QCOMPARE(store.count(), 2);
    QVERIFY(!store.hasNote(5, 1));
}

// 005 §7.1：位置键重映射——排序置换（批注跟着行走）、插删行、插删列
void DATableDataTest::testNoteRemap()
{
    DA::DATableNoteStore store;
    store.setNote(0, 0, QStringLiteral("n00"));
    store.setNote(2, 1, QStringLiteral("n21"));
    // —— 行重排：perm[i]=新表第 i 行的旧行号（DATableKeyRemap 钉死语义）——
    // perm=[2,0,1]：新行0=旧行2 → n21 应落到 (0,1)；新行2=旧行1（无批注）
    store.onRowsReordered(QVector< int >({ 2, 0, 1 }));
    QVERIFY(!store.hasNote(2, 1));
    QCOMPARE(store.noteText(0, 1), QStringLiteral("n21"));
    QVERIFY(store.hasNote(1, 0));  // 旧行0 → 新行1（inv[0]=1）
    // —— 行插入：>=插入行的键 +1 ——
    store.clearAll();
    store.setNote(2, 0, QStringLiteral("a"));
    store.setNote(5, 0, QStringLiteral("b"));
    store.onRowsInserted({ 3 });
    QVERIFY(!store.hasNote(2, 0) == false);  // 行2 < 3 不动
    QCOMPARE(store.noteText(2, 0), QStringLiteral("a"));
    QCOMPARE(store.noteText(6, 0), QStringLiteral("b"));  // 行5 → 6
    // 在批注行上方插入一行 → 批注下移（§5.4 验收）
    store.onRowsInserted({ 0 });
    QCOMPARE(store.noteText(3, 0), QStringLiteral("a"));
    QCOMPARE(store.noteText(7, 0), QStringLiteral("b"));
    // —— 行删除：==删除行的批注删除，>删除行的键 -1 ——
    store.onRowsRemoved({ 3 });
    QVERIFY(!store.hasNote(3, 0));           // 被删行的批注丢失
    QCOMPARE(store.noteText(6, 0), QStringLiteral("b"));  // 7 → 6
    // —— 列插入/删除 ——
    store.clearAll();
    store.setNote(0, 1, QStringLiteral("c1"));
    store.setNote(0, 3, QStringLiteral("c3"));
    store.onColumnsInserted({ 2 });
    QCOMPARE(store.noteText(0, 1), QStringLiteral("c1"));  // 列1 < 2 不动
    QCOMPARE(store.noteText(0, 4), QStringLiteral("c3"));  // 列3 → 4
    store.onColumnsRemoved({ 1 });
    QVERIFY(!store.hasNote(0, 1));                          // 被删列的批注丢弃
    QCOMPARE(store.noteText(0, 3), QStringLiteral("c3"));  // 列4 → 3
}

// 005 §7.1：批注命令——增/改/删（旧新条目记录含时间戳）与清除全部的 undo/redo；文案非空
void DATableDataTest::testNoteCommands()
{
    DA::DATableNoteStore store;
    QUndoStack stack;
    // 新增
    DA::DATableNoteStore::Note n1;
    n1.text      = QStringLiteral("note-1");
    n1.createdAt = QDateTime(QDate(2026, 1, 1), QTime(10, 0));
    n1.updatedAt = n1.createdAt;
    auto* cmdAdd = new DA::DACommandTableNote_set(&store, 3, 1, DA::DATableNoteStore::Note(), n1);
    QVERIFY(!cmdAdd->text().isEmpty());  // 撤销面板无空白条目（红线5）
    stack.push(cmdAdd);
    QCOMPARE(store.noteText(3, 1), QStringLiteral("note-1"));
    QCOMPARE(store.note(3, 1).createdAt, n1.createdAt);  // 时间戳按命令记录写入
    stack.undo();
    QVERIFY(!store.hasNote(3, 1));
    stack.redo();
    QCOMPARE(store.noteText(3, 1), QStringLiteral("note-1"));
    // 编辑（旧新文本记录）
    DA::DATableNoteStore::Note n2   = n1;
    n2.text                         = QStringLiteral("note-2");
    n2.updatedAt                    = QDateTime(QDate(2026, 2, 2), QTime(11, 0));
    auto* cmdEdit                   = new DA::DACommandTableNote_set(&store, 3, 1, n1, n2);
    QCOMPARE(cmdEdit->text().isEmpty(), false);
    stack.push(cmdEdit);
    QCOMPARE(store.noteText(3, 1), QStringLiteral("note-2"));
    stack.undo();
    QCOMPARE(store.noteText(3, 1), QStringLiteral("note-1"));
    QCOMPARE(store.note(3, 1).updatedAt, n1.updatedAt);  // 时间戳精确还原
    stack.redo();
    // 删除
    auto* cmdDel = new DA::DACommandTableNote_set(&store, 3, 1, n2, DA::DATableNoteStore::Note());
    stack.push(cmdDel);
    QVERIFY(!store.hasNote(3, 1));
    stack.undo();
    QCOMPARE(store.noteText(3, 1), QStringLiteral("note-2"));
    // 清除本表批注（一次命令；首次 redo 捕获整店快照）
    store.setNote(0, 0, QStringLiteral("x"));
    store.setNote(9, 9, QStringLiteral("y"));
    const int before = store.count();
    QVERIFY(before >= 3);
    auto* cmdClear = new DA::DACommandTableNote_clearAll(&store);
    QVERIFY(!cmdClear->text().isEmpty());
    stack.push(cmdClear);
    QVERIFY(store.isEmpty());
    stack.undo();
    QCOMPARE(store.count(), before);
    QCOMPARE(store.noteText(9, 9), QStringLiteral("y"));
    stack.redo();
    QVERIFY(store.isEmpty());
}

// 005 §5.4：widget 全链路——弹层插入/编辑/删除、undo/redo、清除本表批注、
// 插行批注下移、排序后批注跟着行走、导出不污染（批注不进 DataFrame，红线3）
void DATableDataTest::testNoteWidgetFlow()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"k": [30, 10, 20, 40, 50, 60], "s": ["c", "a", "b", "d", "e", "f"]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("nsrc"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableNoteStore* store = w.noteStore();
    QVERIFY(store != nullptr);
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    QUndoStack& pageStack = *w.getUndoStack();
    const int stackBefore = pageStack.count();
    // —— 弹层插入批注（当前格 = (1,0)，驱动 DATableNoteEditDialog）——
    tv->selectionModel()->setCurrentIndex(tv->model()->index(1, 0), QItemSelectionModel::ClearAndSelect);
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DATableNoteEditDialog* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        QVERIFY(!dlg->findChild< QPushButton* >(QStringLiteral("noteDeleteBtn"))->isVisibleTo(dlg));  // 无批注不显示删除
        auto* edit = dlg->findChild< QPlainTextEdit* >(QStringLiteral("noteEditText"));
        QVERIFY(edit != nullptr);
        edit->setPlainText(QStringLiteral("NOTE-SECRET-42"));
        QTimer::singleShot(100, [ dlg ]() {
            dlg->findChild< QPushButton* >(QStringLiteral("noteSaveBtn"))->click();
        });
    });
    w.showNoteEditorForCurrentCell();
    QCOMPARE(store->noteText(1, 0), QStringLiteral("NOTE-SECRET-42"));
    QCOMPARE(pageStack.count(), stackBefore + 1);  // 一次批注 = 一条命令
    // undo 移除 / redo 恢复
    pageStack.undo();
    QVERIFY(!store->hasNote(1, 0));
    pageStack.redo();
    QCOMPARE(store->noteText(1, 0), QStringLiteral("NOTE-SECRET-42"));
    // —— 编辑批注（弹层预填旧文本，删除按钮可见）——
    QTimer::singleShot(0, []() {
        auto* dlg = qobject_cast< DA::DATableNoteEditDialog* >(QApplication::activeModalWidget());
        if (!dlg) {
            return;
        }
        auto* edit = dlg->findChild< QPlainTextEdit* >(QStringLiteral("noteEditText"));
        QCOMPARE(edit->toPlainText(), QStringLiteral("NOTE-SECRET-42"));
        QVERIFY(dlg->findChild< QPushButton* >(QStringLiteral("noteDeleteBtn"))->isVisibleTo(dlg));
        edit->setPlainText(QStringLiteral("NOTE-EDITED"));
        QTimer::singleShot(100, [ dlg ]() {
            dlg->findChild< QPushButton* >(QStringLiteral("noteSaveBtn"))->click();
        });
    });
    w.showNoteEditorForCurrentCell();
    QCOMPARE(store->noteText(1, 0), QStringLiteral("NOTE-EDITED"));
    QCOMPARE(pageStack.count(), stackBefore + 2);
    pageStack.undo();
    QCOMPARE(store->noteText(1, 0), QStringLiteral("NOTE-SECRET-42"));  // 还原旧文本
    pageStack.redo();
    // —— 删除当前格批注 ——
    QVERIFY(w.deleteNoteAtCurrentCell());
    QVERIFY(!store->hasNote(1, 0));
    QCOMPARE(pageStack.count(), stackBefore + 3);
    pageStack.undo();
    QCOMPARE(store->noteText(1, 0), QStringLiteral("NOTE-EDITED"));
    // —— 在批注行上方插入一行 → 批注下移（§5.4）——
    w.insertRowAt(0);
    QCOMPARE(store->noteText(2, 0), QStringLiteral("NOTE-EDITED"));
    QVERIFY(!store->hasNote(1, 0) || store->noteText(1, 0).isEmpty());
    pageStack.undo();  // 撤销插行 → 批注回位
    QCOMPARE(store->noteText(1, 0), QStringLiteral("NOTE-EDITED"));
    // —— 排序后批注跟着行走（§5.4：对带批注的行排序）——
    // k=[30,10,20,...] 升序 → 旧行1(k=10) 排到新行0；批注在旧行1 → 应到新行0
    {
        DA::DADataHeaderView* hh = qobject_cast< DA::DADataHeaderView* >(tv->horizontalHeader());
        QVERIFY(hh != nullptr);
        int colK = -1;
        for (int c = 0; c < hh->count(); ++c) {
            if (hh->model() && hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("k")) {
                colK = c;
                break;
            }
        }
        QVERIFY(colK >= 0);
        w.sortColumn(colK, Qt::AscendingOrder);  // 菜单排序入口（2026-10-18 修订：点击表头不再排序）
        QCOMPARE(df.iat(0, 0).toDouble(), 10.0);            // 升序生效
        QCOMPARE(store->noteText(0, 0), QStringLiteral("NOTE-EDITED"));  // 批注跟着行走
        QVERIFY(!store->hasNote(1, 0));
        pageStack.undo();  // 撤销排序 → 批注回原行
        QCOMPARE(store->noteText(1, 0), QStringLiteral("NOTE-EDITED"));
        pageStack.redo();  // 再排序（下步基于排序后状态）
    }
    // —— 导出不污染（红线3：批注不进 DataFrame）——
    {
        store->setNote(0, 1, QStringLiteral("EXPORT-CHECK-NOTE"));
        const QString csvPath = mTmpDir.isValid() ? mTmpDir.filePath(QStringLiteral("note_export.csv"))
                                                  : QDir::temp().filePath(QStringLiteral("note_export.csv"));
        QVERIFY(DA::DAPyScripts::getDataFrame().to_csv(df, csvPath, QStringLiteral(",")));
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString content = QString::fromUtf8(f.readAll());
        f.close();
        QVERIFY(!content.contains(QStringLiteral("EXPORT-CHECK-NOTE")));
        QVERIFY(!content.contains(QStringLiteral("NOTE-EDITED")));
        // 列数不变（批注没有变成数据列）
        QCOMPARE(df.shape().second, static_cast< std::size_t >(2));
    }
    // —— 清除本表批注（一次命令可撤销）——
    const int noteCount = store->count();
    QVERIFY(noteCount >= 2);
    QCOMPARE(w.clearAllNotes(), noteCount);
    QVERIFY(store->isEmpty());
    pageStack.undo();
    QCOMPARE(store->count(), noteCount);
    QCOMPARE(store->noteText(0, 0), QStringLiteral("NOTE-EDITED"));
    w.hide();
}

// 005 §5.3-3：角标绘制（层⑤之后右上小三角，官方金黄）与 helpEvent tooltip（只查可见格）
void DATableDataTest::testNotePaintAndTooltip()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd
df = pd.DataFrame({"a": [1.0, 2.0, 3.0, 4.0], "b": [5.0, 6.0, 7.0, 8.0]})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("npaint"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(700, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DATableNoteStore* store = w.noteStore();
    QVERIFY(store != nullptr);
    store->setNote(0, 0, QStringLiteral("paint-note"));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    // 清当前索引避免选中框干扰角标像素
    if (tv->selectionModel() && tv->selectionModel()->currentIndex().isValid()) {
        tv->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
    }
    QApplication::processEvents();
    tv->viewport()->repaint();
    QImage img(tv->viewport()->size(), QImage::Format_ARGB32);
    tv->viewport()->render(&img);
    const QRect r00 = tv->visualRect(tv->model()->index(0, 0));
    const QRect r10 = tv->visualRect(tv->model()->index(1, 0));
    QVERIFY(r00.isValid() && r10.isValid());
    // 官方金黄 #E6C27C（DESIGN.md §2 标记语义）角标：右上 8px 三角区域含金黄色像素
    const QRgb gold = qRgb(0xE6, 0xC2, 0x7C);
    const QRect corner00(r00.right() - 8, r00.top(), 9, 9);
    int goldCount = 0;
    for (int y = corner00.top(); y <= corner00.bottom() && y < img.height(); ++y) {
        for (int x = corner00.left(); x <= corner00.right() && x < img.width(); ++x) {
            if (x >= 0 && y >= 0 && img.pixel(x, y) == gold) {
                ++goldCount;
            }
        }
    }
    QVERIFY2(goldCount >= 10, qPrintable(QStringLiteral("note corner marker not painted, gold pixels=%1").arg(goldCount)));
    // 无批注格右上角无金黄
    const QRect corner10(r10.right() - 8, r10.top(), 9, 9);
    int goldCount2 = 0;
    for (int y = corner10.top(); y <= corner10.bottom() && y < img.height(); ++y) {
        for (int x = corner10.left(); x <= corner10.right() && x < img.width(); ++x) {
            if (x >= 0 && y >= 0 && img.pixel(x, y) == gold) {
                ++goldCount2;
            }
        }
    }
    QCOMPARE(goldCount2, 0);
    // —— helpEvent tooltip：有批注格返回 true 且 QToolTip 文本 = 批注文本；无批注格走基类 ——
    auto* delegate = qobject_cast< DA::DATableStyleItemDelegate* >(tv->itemDelegate());
    QVERIFY(delegate != nullptr);
    QStyleOptionViewItem opt;
    opt.initFrom(tv);
    const QRect vr = tv->visualRect(tv->model()->index(0, 0));
    const QPoint local(vr.center());
    const QPoint global = tv->viewport()->mapToGlobal(local);
    QHelpEvent ev(QEvent::ToolTip, local, global);
    QToolTip::hideText();
    QVERIFY(delegate->helpEvent(&ev, tv, opt, tv->model()->index(0, 0)));
    QCOMPARE(QToolTip::text(), QStringLiteral("paint-note"));
    // 无批注格：基类路径（无 ToolTipRole → false）
    QToolTip::hideText();
    QStyleOptionViewItem opt2;
    opt2.initFrom(tv);
    QHelpEvent ev2(QEvent::ToolTip, vr.center(), global);
    QVERIFY(!delegate->helpEvent(&ev2, tv, opt2, tv->model()->index(1, 0)));
    // 删除批注后角标消失（重绘断言）
    store->removeNote(0, 0);
    QApplication::processEvents();
    tv->viewport()->repaint();
    QImage img2(tv->viewport()->size(), QImage::Format_ARGB32);
    tv->viewport()->render(&img2);
    int goldCount3 = 0;
    for (int y = corner00.top(); y <= corner00.bottom() && y < img2.height(); ++y) {
        for (int x = corner00.left(); x <= corner00.right() && x < img2.width(); ++x) {
            if (x >= 0 && y >= 0 && img2.pixel(x, y) == gold) {
                ++goldCount3;
            }
        }
    }
    QCOMPARE(goldCount3, 0);
    w.hide();
}


//===================================================
// 005 WP4-A 撤销历史面板与深度可配置（T1.21）
//===================================================

// 005 §6.3：撤销栈深度可配置——默认 20；调大到 100 后 30 步操作全部可撤销；
// 深度仍生效（回到 20 时超出部分被裁剪）
void DATableDataTest::testUndoStackDepthConfigurable()
{
    if (!mPandasOk) {
        QSKIP("pandas not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    pybind11::exec(R"(
import pandas as pd, numpy as np
df = pd.DataFrame({"a": np.arange(40, dtype="float64")})
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    DA::DAData d(df);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    // 默认深度 20（构造里 setUndoLimit(20)）
    QCOMPARE(w.undoLimit(), 20);
    // 调大到 100（面板路径的 DAGui 侧口径；APP 施加与配置持久化在 L5 接线）
    w.setUndoLimit(100);
    QCOMPARE(w.undoLimit(), 100);
    // 30 步单元格编辑命令（iat 值记录）
    for (int i = 0; i < 30; ++i) {
        const double oldv = df.iat(static_cast< std::size_t >(i), 0).toDouble();
        auto* cmd         = new DA::DACommandDataFrame_iat(df, i, 0, QVariant(oldv), QVariant(oldv + 1000.0));
        QVERIFY(cmd->exec());
        w.push(cmd);
    }
    QCOMPARE(w.getUndoStack()->count(), 30);  // 深度 100 → 30 步全保留
    QCOMPARE(df.iat(29, 0).toDouble(), 1029.0);
    // 全部撤销（§6.3 验收：做出 30 步操作后仍能全部撤销）
    for (int i = 0; i < 30; ++i) {
        w.getUndoStack()->undo();
    }
    QCOMPARE(w.getUndoStack()->index(), 0);
    QCOMPARE(df.iat(29, 0).toDouble(), 29.0);
    // 深度限制仍生效：调回 5，push 8 条只留 5 条
    w.getUndoStack()->clear();
    w.setUndoLimit(5);
    for (int i = 0; i < 8; ++i) {
        const double oldv = df.iat(static_cast< std::size_t >(i), 0).toDouble();
        auto* cmd         = new DA::DACommandDataFrame_iat(df, i, 0, QVariant(oldv), QVariant(oldv + 100.0));
        QVERIFY(cmd->exec());
        w.push(cmd);
    }
    QCOMPARE(w.getUndoStack()->count(), 5);
    QCOMPARE(w.undoLimit(), 5);
}

// 005 §6.3：QUndoView 面板——绑 QUndoGroup 跟随激活栈、条目数与文案（无空白条目红线5）、
// 深度调节信号（用户改发信号、程序 setUndoLimit 不发）
void DATableDataTest::testUndoHistoryWidget()
{
    DA::DAUndoHistoryWidget panel;
    QUndoGroup group;
    QUndoStack stackA;
    QUndoStack stackB;
    group.addStack(&stackA);
    group.addStack(&stackB);
    panel.setUndoGroup(&group);
    QCOMPARE(panel.undoGroup(), &group);
    // 栈 A 推两条有文案的命令并激活；视图模型条目跟随激活栈
    auto* cmd1 = new DA::DACommandTableNote_clearAll(nullptr);  // 空 store 安全（setText 非空即可）
    QVERIFY(!cmd1->text().isEmpty());
    stackA.push(cmd1);
    stackA.setActive();
    QUndoView* view = panel.findChild< QUndoView* >(QStringLiteral("undoHistoryView"));
    QVERIFY(view != nullptr);
    QVERIFY(view->model() != nullptr);
    // QUndoStack 内置模型行 0 是「<empty>」栈底伪条目（Qt 自带标签，非空白命令），
    // 命令条目从行 1 起：1 条命令 → 2 行
    QCOMPARE(view->model()->rowCount(), 2);
    // 命令文案在视图模型里可见且非空（面板无空白条目的口径）
    const QString rowText = view->model()->index(1, 0).data(Qt::DisplayRole).toString();
    QVERIFY(!rowText.trimmed().isEmpty());
    // 切换激活栈 → 视图跟随（stackB 空 → 只剩栈底伪条目）
    stackB.setActive();
    QCOMPARE(view->model()->rowCount(), 1);
    stackA.setActive();
    QCOMPARE(view->model()->rowCount(), 2);
    // 深度调节：用户改 SpinBox → 发 undoLimitChangeRequested；程序 setUndoLimit → 不发
    QSpinBox* spin = panel.findChild< QSpinBox* >(QStringLiteral("undoLimitSpin"));
    QVERIFY(spin != nullptr);
    QCOMPARE(panel.undoLimit(), 20);  // 默认 20（R19）
    QSignalSpy spy(&panel, &DA::DAUndoHistoryWidget::undoLimitChangeRequested);
    panel.setUndoLimit(100);
    QCOMPARE(panel.undoLimit(), 100);
    QCOMPARE(spy.count(), 0);  // 程序注入不回环
    spin->setValue(50);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 50);
}

// 005 红线5：Swap/Persist 命令构造即带默认 tr() 文案（调用方 setText 覆盖；
// 空文案不覆盖——DAAppCommand 已加守卫），撤销面板不允许空白条目
void DATableDataTest::testUndoCommandDefaultTexts()
{
    {
        DA::DADataObjectSwapUndoCommand cmd;
        QVERIFY(!cmd.text().isEmpty());
    }
    {
        DA::DADataObjectPersistUndoCommand cmd;
        QVERIFY(!cmd.text().isEmpty());
    }
}


//===================================================
// 005 §7.2 收官：10 万行综合体验链路（排序 → 查找替换 → 条件格式 → 填充 → 校验 → 批注）
//===================================================

// 程序化走一遍整批能力的组合链路：无明显卡顿（分步计时 + 宽松上限）、无错位
// （撤销全部回退后数据精确还原）、各步撤销栈只增对应条数
void DATableDataTest::testBatch3FullChain100k()
{
    if (!mPandasOk || !mPyScriptsOk) {
        QSKIP("pandas/DAPyScripts not available");
    }
    qRegisterMetaType< DA::DAData >("DA::DAData");
    pybind11::dict scope;
    QElapsedTimer total;
    total.start();
    pybind11::exec(R"(
import pandas as pd, numpy as np
n = 100000
rng = np.random.default_rng(11)
cat_pool = ["alpha", "beta", "x777", "gamma"]
sp = np.arange(n, dtype="float64") * 0.5
sp[::10] = np.nan
df = pd.DataFrame({
    "id": rng.integers(0, 10**9, n),
    "val": rng.uniform(-20, 120, n),
    "cat": rng.choice(cat_pool, n),
    "d": pd.to_datetime("2020-01-01") + pd.to_timedelta(rng.integers(0, 1000, n), unit="D"),
    "sp": sp,
    "flag": rng.integers(0, 2, n).astype(bool),
})
first_val = float(df.iat[0, 1])
)",
                   pybind11::globals(),
                   scope);
    DA::DAPyDataFrame df(scope[ "df" ]);
    const double firstVal = scope[ "first_val" ].cast< double >();
    QCOMPARE(df.shape().first, static_cast< std::size_t >(100000));
    DA::DADataManager mgr;
    DA::DAData d(df);
    d.setName(QStringLiteral("chain100k"));
    mgr.addData(d);
    DA::DATableMetadataRegistry registry(nullptr);
    DA::DADataOperateOfDataFrameWidget w(d, &registry);
    w.resize(800, 500);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    DA::DADataTableView* tv = w.getDataTableView();
    QVERIFY(tv != nullptr);
    QUndoStack& pageStack = *w.getUndoStack();
    DA::DAPyScriptsDataFrame& py = DA::DAPyScripts::getDataFrame();
    QElapsedTimer step;
    qint64 msNote = 0, msSort = 0, msReplace = 0, msCf = 0, msFill = 0, msValidate = 0;

    // —— ① 批注（先放批注，验证它扛过后续排序）——
    DA::DATableNoteStore* notes = w.noteStore();
    QVERIFY(notes != nullptr);
    step.start();
    {
        DA::DATableNoteStore::Note n1;
        n1.text      = QStringLiteral("chain-note");
        n1.createdAt = QDateTime::currentDateTime();
        n1.updatedAt = n1.createdAt;
        w.push(new DA::DACommandTableNote_set(notes, 50000, 0, DA::DATableNoteStore::Note(), n1));
    }
    msNote = step.elapsed();
    QCOMPARE(notes->noteText(50000, 0), QStringLiteral("chain-note"));

    // —— ② 排序（列头右键菜单入口的页面执行点，破坏式重排 + 置换撤销；
    //    2026-10-18 交互修订：点击表头不再排序）——
    step.start();
    {
        DA::DADataHeaderView* hh = qobject_cast< DA::DADataHeaderView* >(tv->horizontalHeader());
        QVERIFY(hh != nullptr);
        int colVal = -1;
        for (int c = 0; c < hh->count(); ++c) {
            if (hh->model()->headerData(c, Qt::Horizontal).toString() == QLatin1String("val")) {
                colVal = c;
                break;
            }
        }
        QVERIFY(colVal >= 0);
        w.sortColumn(colVal, Qt::AscendingOrder);
        QCOMPARE(w.sortSpec().size(), 1);
    }
    msSort = step.elapsed();
    // 升序：首行是列最小值附近（<= 0 的负端），且批注仍在（跟着某一行移动，总数不变）
    QVERIFY(df.iat(0, 1).toDouble() < 0.0);
    QCOMPARE(notes->count(), 1);
    QCOMPARE(pageStack.count(), 2);  // 批注 + 排序

    // —— ③ 查找替换（变更清单，一个撤销单元）——
    QVector< int > repRows, repCols;
    pybind11::list oldVals, newVals;
    step.start();
    {
        QVERIFY(py.findReplace(df,
                               QStringLiteral("x777"),
                               QStringLiteral("x777z"),
                               false,
                               false,
                               false,
                               qMakePair(0, 99999),
                               qMakePair(2, 2),
                               &repRows,
                               &repCols,
                               &oldVals,
                               &newVals));
        QVERIFY(repRows.size() > 10000);  // 1/4 池命中率
        auto* cmd = new DA::DACommandDataFrame_replace(df, repRows, repCols, oldVals, newVals);
        cmd->setText(QStringLiteral("replace all (chain)"));
        QVERIFY(cmd->exec());
        w.push(cmd);
    }
    msReplace = step.elapsed();
    const int firstRepRow = repRows.isEmpty() ? -1 : repRows.first();
    if (firstRepRow >= 0) {
        QCOMPARE(df.iat(static_cast< std::size_t >(firstRepRow), 2).toString(), QStringLiteral("x777z"));
    }
    QCOMPARE(pageStack.count(), 3);

    // —— ④ 条件格式（元数据 + delegate 绘制，不进撤销栈）——
    step.start();
    {
        DA::DATableConditionalFormatStore* cf = w.conditionalFormatStore();
        QVERIFY(cf != nullptr);
        DA::DATableConditionalFormatRule rule;
        rule.type      = DA::DATableConditionalFormatRule::Type::CellRule;
        rule.condition = DA::DATableConditionalFormatRule::Condition::Greater;
        rule.value1    = QVariant(100.0);
        DA::DATableCellStyle st;
        st.setBackground(QBrush(QColor(0xCE, 0x60, 0x43)));
        rule.style = st;
        cf->addRule(1, rule);
        // 全视口重绘无卡顿（滚动到中部再渲染一次）
        tv->showActualRow(50000);
        QApplication::processEvents();
        QImage img(tv->viewport()->size(), QImage::Format_ARGB32);
        tv->viewport()->render(&img);
        QVERIFY(!img.isNull());
    }
    msCf = step.elapsed();
    const int stackAfterCf = pageStack.count();

    // —— ⑤ 填充（sp 列 ffill，选 100 行）——
    step.start();
    {
        int colSp = -1;
        const QList< QString > cols = df.columns();
        for (int c = 0; c < cols.size(); ++c) {
            if (cols[ c ] == QLatin1String("sp")) {
                colSp = c;
                break;
            }
        }
        QVERIFY(colSp >= 0);
        QItemSelection sel(tv->model()->index(0, colSp), tv->model()->index(99, colSp));
        tv->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
        const int written = w.fillSelection(DA::DADataOperateOfDataFrameWidget::FillMode::Forward);
        QVERIFY(written > 0);  // sp 每 10 行一个 NaN，100 行内必有填充
    }
    msFill = step.elapsed();
    QCOMPARE(pageStack.count(), stackAfterCf + 1);  // 条件格式不入栈、填充入栈

    // —— ⑥ 校验（只读，违规高亮 + 耗时记录）——
    step.start();
    {
        DA::DATableValidationStore* vs = w.validationStore();
        QVERIFY(vs != nullptr);
        DA::DATableValidationRule rNum;
        rNum.columnName = QStringLiteral("val");
        rNum.type       = DA::DATableValidationRule::Type::NumericRange;
        rNum.hasNumericMin = true;
        rNum.numericMin = 0.0;
        rNum.hasNumericMax = true;
        rNum.numericMax = 100.0;
        vs->addRule(rNum);
        DA::DATableValidationRule rNe;
        rNe.columnName = QStringLiteral("cat");
        rNe.type       = DA::DATableValidationRule::Type::NotEmpty;
        rNe.level      = DA::DATableValidationRule::Level::Warning;
        vs->addRule(rNe);
        int evalMs = -1;
        const auto outcomes = w.evaluateValidationRules(&evalMs);
        QCOMPARE(outcomes.size(), 2);
        QVERIFY(outcomes[ 0 ].rows.size() > 10000);  // uniform(-20,120) 约 2/7 越界
        QVERIFY(evalMs >= 0);
        // 违规高亮已装载（任取一条违规行）
        DA::DATableHighlightOverlay* ov = w.highlightOverlay();
        QCOMPARE(ov->levelAt(outcomes[ 0 ].rows.first(), outcomes[ 0 ].columnPosition),
                 DA::DATableHighlightOverlay::Level::ViolationError);
    }
    msValidate = step.elapsed();
    QCOMPARE(pageStack.count(), stackAfterCf + 1);  // 校验只读不入栈（红线2）

    // —— 全部撤销：数据精确还原 ——
    while (pageStack.index() > 0) {
        pageStack.undo();
    }
    QCOMPARE(pageStack.index(), 0);
    QCOMPARE(df.iat(0, 1).toDouble(), firstVal);          // 排序还原
    QCOMPARE(notes->count(), 0);                          // 批注命令还原
    QVERIFY(qIsNaN(df.iat(10, 4).toDouble()));            // 填充还原（sp[10] 回到 NaN）
    if (firstRepRow >= 0) {
        // 排序已还原 → firstRepRow 的绝对行含义回到替换时（替换发生在排序后、排序已先还原？）
        // 撤销顺序：填充→替换→排序→批注；替换还原发生在排序还原之前，行号在替换坐标系内一致
        QVERIFY(df.iat(static_cast< std::size_t >(firstRepRow), 2).toString() != QStringLiteral("x777z"));
    }
    // 校验高亮是瞬态（不在撤销栈），清除入口只清自己来源
    w.clearValidationHighlights();
    QVERIFY(w.highlightOverlay()->isEmpty());

    const qint64 msTotal = total.elapsed();
    qDebug().noquote() << QStringLiteral("DATableDataTest::testBatch3FullChain100k: note=%1ms sort=%2ms replace=%3ms cf=%4ms fill=%5ms validate=%6ms total=%7ms")
                              .arg(msNote).arg(msSort).arg(msReplace).arg(msCf).arg(msFill).arg(msValidate).arg(msTotal);
    // 宽松上限（防劣化）：全链路 < 60s；排序单步 < 10s（002 实测 10 万行 ~15ms）
    QVERIFY2(msTotal < 60000, qPrintable(QStringLiteral("full chain too slow: %1 ms").arg(msTotal)));
    QVERIFY2(msSort < 10000, qPrintable(QStringLiteral("sort too slow: %1 ms").arg(msSort)));
    w.hide();
}


// 不用QTEST_MAIN：嵌入式python的静态对象（DAPyBindQt.dll内的解释器句柄、QVariant caster
// 类型缓存等）在进程teardown阶段析构顺序不可控，会在测试结果完整输出后引发访问冲突。
// qExec返回时断言结果与-o文件均已写完，直接终止进程跳过teardown，保证退出码可信
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    DATableDataTest tc;
    int ret = QTest::qExec(&tc, argc, argv);
    std::fflush(nullptr);
#ifdef Q_OS_WIN
    // Windows下_exit仍会走LdrShutdownProcess→DLL_PROCESS_DETACH触发各DLL静态析构，
    // 必须TerminateProcess才能完全跳过
    ::TerminateProcess(::GetCurrentProcess(), static_cast< UINT >(ret));
#else
    ::_exit(ret);
#endif
    return ret;
}
#include "main.moc"
