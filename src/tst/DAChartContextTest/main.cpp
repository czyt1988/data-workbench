// DAChartContextTest 的测试实现
//
// T6 WP4：把「图元树目标抽象 + 通用回退图元面板 + 当前子图导出」的能力固化为 chart 域回归测试。
// 覆盖点：
// 1. DAChartManageWidget::setFigures / setCurrentFigure / figures() / 清空重建（不依赖 APP）；
// 2. 未注册 rtti 的图元：工厂回退 DAChartGenericItemSettingPanel，setPlotItem 安全；
// 3. DAChartCommonItemsSettingWidget::updateUI 在已注册/未注册 rtti 之间切换面板（不再停留上一个）；
// 4. DAFigureWidget::exportChartToFile 的 PNG/SVG/PDF 与 copyChartToClipboard；
// 5. 空参数/空路径导出返回 false 且不崩溃。

#include <QtTest/QtTest>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QPixmap>
#include <QScopedPointer>
#include <QTemporaryDir>

// DAGui
#include "Chart/DAChartManageWidget.h"
#include "Chart/Setting/DAChartCommonItemsSettingWidget.h"
#include "Chart/Setting/DAChartGenericItemSettingPanel.h"
#include "Chart/Setting/DAChartItemSettingPanel.h"
#include "Chart/Setting/DAChartItemSettingPanelFactory.h"
#include "Common/Settings/DAPropertyPanelContainerWidget.h"
#include "Chart/Setting3D/DAChart3DGenericItemSettingPanel.h"
#include "Chart/Setting3D/DAChart3DItemSettingPanel.h"
#include "Chart/Setting3D/DAChart3DItemSettingPanelFactory.h"
#include "Chart/Setting3D/DAChart3DWaterfallSettingPanel.h"
// DAFigure
#include "DAChartWidget.h"
#include "DAFigureWidget.h"
// qwt
#include "qwt_plot_item.h"
#include "qwt_plot_marker.h"
#include "qwt_text.h"
// qwt3d
#include "qwt3d_plotitem.h"
#include "qwt3d_waterfall.h"

namespace DA
{
/**
 * @brief 未注册 rtti 的自定义图元（模拟插件自定义图元）
 *
 * rtti 取 0x7F00（>= QwtPlotItem::Rtti_PlotUserItem 的保留区），且从不注册到
 * DAChartItemSettingPanelFactory，用于验证工厂/公共面板的回退路径。
 */
class TSTUnregisteredPlotItem : public QwtPlotItem
{
public:
    // 未注册的 RTTI 值
    static constexpr int RttValue = 0x7F00;

    explicit TSTUnregisteredPlotItem(const QString& title = QString()) : QwtPlotItem(title)
    {
    }

    int rtti() const override
    {
        return RttValue;
    }

    void draw(QPainter*, const QwtScaleMap&, const QwtScaleMap&, const QRectF&) const override
    {
    }
};

/**
 * @brief 未注册 rtti 的自定义 3D 图元（模拟插件自定义 3D 图元）
 *
 * rtti 取 0x7F01（不与 qwt3d 内置 Rtti_Plot3D* 冲突），且从不注册到
 * DAChart3DItemSettingPanelFactory，用于验证 3D 工厂/通用面板的回退路径。
 */
class TSTUnregistered3DPlotItem : public Qwt3DPlotItem
{
public:
    // 未注册的 RTTI 值
    static constexpr int RttValue = 0x7F01;

    int rtti() const override
    {
        return RttValue;
    }

    void draw() override
    {
    }

    ParallelEpiped hull() const override
    {
        return ParallelEpiped();
    }
};

/**
 * @brief T6 WP4 图元树目标抽象、通用回退图元面板与当前子图导出测试
 */
class TSTDAChartContextTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    // 注册内置图元面板，保证「已注册 rtti」用例可复现
    void initTestCase();
    // setFigures/setCurrentFigure/figures()/清空
    void testChartManageSetFigures();
    // setFigures 反复重建管理树不累积 figure 销毁监听连接（C-09）
    void testChartManageRepeatedSetFiguresDoesNotLeakConnections();
    // 未注册 rtti → 工厂回退通用面板
    void testUnregisteredRttiFallback();
    // 通用回退面板的属性修改写回图元（标题/Z值/可见性）
    void testGenericItemPanelPropertyWriteBack();
    // 3D 未注册 rtti → 3D 工厂回退通用面板；瀑布 rtti 走向专用面板，setPlot3DItem 安全（SF-08）
    void test3DUnregisteredRttiFallback();
    // updateUI 在已注册/未注册 rtti 之间切换 currentItemPanel
    void testCommonItemsSettingWidgetSwitchesPanel();
    // 导出 PNG/SVG/PDF 到文件
    void testExportChartToFile();
    // 复制当前子图到剪贴板
    void testCopyChartToClipboard();
    // 空 chart/空路径导出返回 false 且不崩溃
    void testExportWithEmptyArguments();
    // 覆盖保存失败时返回 false 且不破坏已存在的同名文件（C-13）
    void testExportOverwriteFailureKeepsExistingFile();

private:
    // 创建并显示指定尺寸的 figure，等待窗口暴露与布局完成
    static bool initFigure(DAFigureWidget* fig, int width = 800, int height = 600);
    // 处理事件并短暂等待，让布局与渲染状态稳定
    static void settle(DAFigureWidget* fig, int ms = 30);
};

//===================================================
// 辅助函数
//===================================================

bool TSTDAChartContextTest::initFigure(DAFigureWidget* fig, int width, int height)
{
    if (!fig) {
        return false;
    }
    fig->resize(width, height);
    fig->show();
    if (!QTest::qWaitForWindowExposed(fig)) {
        return false;
    }
    settle(fig);
    return true;
}

void TSTDAChartContextTest::settle(DAFigureWidget* fig, int ms)
{
    Q_UNUSED(fig);
    QApplication::processEvents();
    QTest::qWait(ms);
    QApplication::processEvents();
}

//===================================================
// 用例实现
//===================================================

/**
 * @brief 注册内置图元设置面板
 *
 * 生产环境由 DAChartSettingWidget 构造函数调用；测试不构造该窗口，这里显式注册，
 * 使「已注册 rtti 使用专属面板」的用例可复现。
 */
void TSTDAChartContextTest::initTestCase()
{
    DAChartItemSettingPanelFactory::instance().registerAllKnownPanels();
    QVERIFY2(DAChartItemSettingPanelFactory::instance().isRegistered(QwtPlotItem::Rtti_PlotMarker),
             "the built-in marker panel should be registered");
    QVERIFY2(!DAChartItemSettingPanelFactory::instance().isRegistered(TSTUnregisteredPlotItem::RttValue),
             "the custom rtti must stay unregistered in this test");
    // 3D 面板一并注册（生产环境由 DAChart3DSettingWidget 构造函数注册）
    DAChart3DItemSettingPanelFactory::instance().registerAllKnown3DPanels();
    QVERIFY2(DAChart3DItemSettingPanelFactory::instance().isRegistered(Rtti_Plot3DWaterfall),
             "the waterfall rtti should be registered to its dedicated panel");
    QVERIFY2(!DAChart3DItemSettingPanelFactory::instance().isRegistered(TSTUnregistered3DPlotItem::RttValue),
             "the custom 3D rtti must stay unregistered in this test");
}

/**
 * @brief setFigures 按给定列表重建管理树；setCurrentFigure 生效；清空后不崩
 */
void TSTDAChartContextTest::testChartManageSetFigures()
{
    DAChartManageWidget manager;
    QScopedPointer< DAFigureWidget > first(new DAFigureWidget());
    QScopedPointer< DAFigureWidget > second(new DAFigureWidget());
    QVERIFY(first->createChart());
    QVERIFY(second->createChart());

    // 传入 current：列表顺序与当前项都应生效
    manager.setFigures({ first.data(), second.data() }, second.data());
    QCOMPARE(manager.figures().size(), 2);
    QVERIFY(manager.figures().at(0) == first.data());
    QVERIFY(manager.figures().at(1) == second.data());
    QVERIFY2(manager.getCurrentFigure() == second.data(), "the explicit current figure should be applied");

    // setCurrentFigure 切到另一个已纳管的 figure
    manager.setCurrentFigure(first.data());
    QVERIFY2(manager.getCurrentFigure() == first.data(), "setCurrentFigure() should switch the current figure");

    // 空指针 / 不在列表内的 figure 视为空操作，不崩溃也不改变当前项
    manager.setCurrentFigure(nullptr);
    QVERIFY(manager.getCurrentFigure() == first.data());
    QScopedPointer< DAFigureWidget > other(new DAFigureWidget());
    manager.setCurrentFigure(other.data());
    QVERIFY2(manager.getCurrentFigure() == first.data(),
             "setCurrentFigure() with an unmanaged figure should be a no-op");

    // 不传 current 时取列表首项
    manager.setFigures({ first.data(), second.data() });
    QVERIFY2(manager.getCurrentFigure() == first.data(), "the first figure should become current when current is null");

    // 清空：列表为空，不崩溃
    manager.setFigures({}, nullptr);
    QVERIFY(manager.figures().isEmpty());
    QVERIFY2(manager.getCurrentFigure() == nullptr, "the current figure should be cleared with the managed list");
    // 清空后再次操作不崩溃
    manager.setCurrentFigure(first.data());
    QVERIFY(manager.figures().isEmpty());
    manager.expandCurrentTree();
    manager.collapseCurrentTree();
}

/**
 * @brief setFigures 反复重建管理树：figure 销毁监听连接不累积（C-09）
 *
 * 信号分析页每次刷新目标都会调 setFigures()；旧实现只在 figure 销毁时清理连接，
 * 同一 figure 会被重复连接。这里用连接计数把「重建后连接数 = figure 数」固化下来。
 */
void TSTDAChartContextTest::testChartManageRepeatedSetFiguresDoesNotLeakConnections()
{
    DAChartManageWidget manager;
    QScopedPointer< DAFigureWidget > first(new DAFigureWidget());
    QScopedPointer< DAFigureWidget > second(new DAFigureWidget());
    QVERIFY(first->createChart());
    QVERIFY(second->createChart());

    manager.setFigures({ first.data(), second.data() }, first.data());
    QCOMPARE(manager.managedFigureConnectionCount(), 2);
    // 同一列表重建两次：连接数不增长
    manager.setFigures({ first.data(), second.data() }, first.data());
    QCOMPARE(manager.figures().size(), 2);
    QVERIFY2(manager.managedFigureConnectionCount() == 2,
             "rebuilding the manage tree must not accumulate destroyed-signal connections");
    // 新列表包含重复项时按 figure 去重
    manager.setFigures({ first.data(), second.data(), first.data() }, second.data());
    QCOMPARE(manager.figures().size(), 2);
    QCOMPARE(manager.managedFigureConnectionCount(), 2);

    // figure 销毁：管理项被移除且连接记录随之清理
    first.reset();
    QCOMPARE(manager.figures().size(), 1);
    QCOMPARE(manager.figures().first(), second.data());
    QCOMPARE(manager.managedFigureConnectionCount(), 1);

    // 清空管理树：连接全部断开
    manager.setFigures({}, nullptr);
    QVERIFY(manager.figures().isEmpty());
    QCOMPARE(manager.managedFigureConnectionCount(), 0);
}

/**
 * @brief 未注册 rtti 的图元：工厂回退 DAChartGenericItemSettingPanel，setPlotItem 安全
 */
void TSTDAChartContextTest::testUnregisteredRttiFallback()
{
    // 图元先于 figure 声明：figure 析构时图元仍存活，qwt 能安全 detach
    QScopedPointer< TSTUnregisteredPlotItem > item(new TSTUnregisteredPlotItem(QStringLiteral("custom item")));
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    settle(fig.data());

    // 构造并挂载一个未注册 rtti 的自定义图元
    item->setZ(12.5);
    item->attach(chart);
    QCOMPARE(item->rtti(), TSTUnregisteredPlotItem::RttValue);
    QVERIFY2(!DAChartItemSettingPanelFactory::instance().isRegistered(item->rtti()),
             "the custom rtti must not be registered");

    // 工厂必须返回非空的通用回退面板（而不是 nullptr / 上一个 item 的面板）
    QScopedPointer< DAChartItemSettingPanel > panel(
        DAChartItemSettingPanelFactory::instance().createPanel(item->rtti()));
    QVERIFY2(panel.data() != nullptr, "createPanel() must not return null for an unregistered rtti");
    DAChartGenericItemSettingPanel* generic = dynamic_cast< DAChartGenericItemSettingPanel* >(panel.data());
    QVERIFY2(generic != nullptr, "an unregistered rtti must fall back to DAChartGenericItemSettingPanel");

    // setPlotItem 不崩溃并能正确持有图元
    panel->setPlotItem(item.data());
    QVERIFY(panel->getPlotItem() == item.data());
    QVERIFY(panel->isHaveItem());

    // 已注册 rtti 仍走向专属面板（回退不吞掉正常注册）
    QScopedPointer< DAChartItemSettingPanel > markerPanel(
        DAChartItemSettingPanelFactory::instance().createPanel(QwtPlotItem::Rtti_PlotMarker));
    QVERIFY(markerPanel.data() != nullptr);
    QVERIFY2(dynamic_cast< DAChartGenericItemSettingPanel* >(markerPanel.data()) == nullptr,
             "a registered rtti should not use the generic fallback panel");

    item->detach();
}

/**
 * @brief 通用回退面板的属性修改写回图元（C-11）
 *
 * 面板此前只断言了 setPlotItem 持有，属性写回分支（title/legend/visible/z）没有断言。
 * 这里通过属性面板改值并触发 onPropertyValueChanged，验证图元确实被更新。
 */
void TSTDAChartContextTest::testGenericItemPanelPropertyWriteBack()
{
    QScopedPointer< TSTUnregisteredPlotItem > item(new TSTUnregisteredPlotItem(QStringLiteral("origin")));
    QScopedPointer< DAChartGenericItemSettingPanel > panel(new DAChartGenericItemSettingPanel());
    panel->setPlotItem(item.data());
    QVERIFY(panel->getPlotItem() == item.data());

    auto applyProperty = [ &panel ](int propertyId) {
        return QMetaObject::invokeMethod(panel.data(), "onPropertyValueChanged", Q_ARG(int, propertyId));
    };

    // 标题写回
    panel->propertyPanel()->setStringValue(DAChartGenericItemSettingPanel::PropTitle, QStringLiteral("renamed"));
    QVERIFY(applyProperty(DAChartGenericItemSettingPanel::PropTitle));
    QCOMPARE(item->title().text(), QStringLiteral("renamed"));

    // Z 值写回
    panel->propertyPanel()->setDoubleValue(DAChartGenericItemSettingPanel::PropZ, 7.5);
    QVERIFY(applyProperty(DAChartGenericItemSettingPanel::PropZ));
    QCOMPARE(item->z(), 7.5);

    // 可见性写回
    panel->propertyPanel()->setBoolValue(DAChartGenericItemSettingPanel::PropVisible, false);
    QVERIFY(applyProperty(DAChartGenericItemSettingPanel::PropVisible));
    QCOMPARE(item->isVisible(), false);

    // 图例属性写回
    panel->propertyPanel()->setBoolValue(DAChartGenericItemSettingPanel::PropLegend, true);
    QVERIFY(applyProperty(DAChartGenericItemSettingPanel::PropLegend));
    QVERIFY(item->testItemAttribute(QwtPlotItem::Legend));
}

/**
 * @brief 3D 未注册 rtti 回退通用面板；瀑布 rtti 走向专用面板，setPlot3DItem 安全
 *
 * 面板构造与 updateUI 只读写 Qwt3DPlotItem 基类属性（title/visible/z），不需要 GL 环境；
 * 图元不挂任何 plot 也可安全设 target（setPlot3D(nullptr) 有空指针防御）。
 */
void TSTDAChartContextTest::test3DUnregisteredRttiFallback()
{
    // Rtti_Plot3DWaterfall：已注册到瀑布专用面板（不再是通用回退）
    QScopedPointer< DAChart3DItemSettingPanel > waterfallPanel(
        DAChart3DItemSettingPanelFactory::instance().createPanel(Rtti_Plot3DWaterfall));
    QVERIFY2(waterfallPanel.data() != nullptr, "createPanel(Rtti_Plot3DWaterfall) must not return null");
    QVERIFY2(dynamic_cast< DAChart3DWaterfallSettingPanel* >(waterfallPanel.data()) != nullptr,
             "Rtti_Plot3DWaterfall should use the dedicated waterfall setting panel");
    QVERIFY2(dynamic_cast< DAChart3DGenericItemSettingPanel* >(waterfallPanel.data()) == nullptr,
             "the waterfall rtti must not fall back to the generic 3D panel");

    // 未注册 rtti：工厂一律回退通用面板（不再返回 nullptr，设置页不再停留上一个图元）
    QScopedPointer< DAChart3DItemSettingPanel > unregisteredPanel(
        DAChart3DItemSettingPanelFactory::instance().createPanel(TSTUnregistered3DPlotItem::RttValue));
    QVERIFY2(unregisteredPanel.data() != nullptr, "createPanel() must not return null for an unregistered 3D rtti");
    QVERIFY2(dynamic_cast< DAChart3DGenericItemSettingPanel* >(unregisteredPanel.data()) != nullptr,
             "an unregistered 3D rtti must fall back to DAChart3DGenericItemSettingPanel");

    // setPlot3DItem 不崩溃并能正确持有图元（图元未挂 plot，getPlot3D 为空是预期）
    Qwt3DWaterfall waterfall;
    waterfall.setTitle(QStringLiteral("waterfall-item"));
    unregisteredPanel->setPlot3DItem(&waterfall);
    QVERIFY2(unregisteredPanel->getPlot3DItem() == &waterfall, "the panel should hold the waterfall item");
    QVERIFY(unregisteredPanel->isHaveItem());

    TSTUnregistered3DPlotItem custom;
    waterfallPanel->setPlot3DItem(&custom);
    QVERIFY2(waterfallPanel->getPlot3DItem() == &custom, "the waterfall panel should hold the custom 3D item");
    QVERIFY(waterfallPanel->isHaveItem());

    // 已注册 rtti 仍走向专属面板（回退不吞掉正常注册）
    QScopedPointer< DAChart3DItemSettingPanel > surfacePanel(
        DAChart3DItemSettingPanelFactory::instance().createPanel(Rtti_Plot3DSurface));
    QVERIFY(surfacePanel.data() != nullptr);
    QVERIFY2(dynamic_cast< DAChart3DGenericItemSettingPanel* >(surfacePanel.data()) == nullptr,
             "a registered 3D rtti should not use the generic fallback panel");
}

/**
 * @brief updateUI 从未注册 rtti 切回已注册 rtti 时，currentItemPanel 跟着切换
 */
void TSTDAChartContextTest::testCommonItemsSettingWidgetSwitchesPanel()
{
    // 图元先于 figure 声明：figure 析构时图元仍存活，qwt 能安全 detach
    QScopedPointer< QwtPlotMarker > marker(new QwtPlotMarker());
    QScopedPointer< TSTUnregisteredPlotItem > custom(new TSTUnregisteredPlotItem(QStringLiteral("custom item")));
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data()));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    settle(fig.data());

    marker->attach(chart);
    custom->attach(chart);

    DAChartCommonItemsSettingWidget panelWidget;

    // 先给已注册 rtti：应使用专属面板
    panelWidget.updateUI(marker.data());
    DAChartItemSettingPanel* markerItemPanel = panelWidget.currentItemPanel();
    QVERIFY2(markerItemPanel != nullptr, "updateUI() should display a panel for a registered rtti");
    QVERIFY2(dynamic_cast< DAChartGenericItemSettingPanel* >(markerItemPanel) == nullptr,
             "a registered rtti should display its dedicated panel");
    QVERIFY(markerItemPanel->getPlotItem() == marker.data());

    // 再给未注册 rtti：必须切换到通用面板，而不是停留上一个面板
    panelWidget.updateUI(custom.data());
    DAChartItemSettingPanel* customItemPanel = panelWidget.currentItemPanel();
    QVERIFY2(customItemPanel != nullptr, "updateUI() should display the generic panel for an unregistered rtti");
    QVERIFY2(customItemPanel != markerItemPanel, "the panel must switch away from the previous item panel");
    QVERIFY2(dynamic_cast< DAChartGenericItemSettingPanel* >(customItemPanel) != nullptr,
             "an unregistered rtti must switch to DAChartGenericItemSettingPanel");
    QVERIFY(customItemPanel->getPlotItem() == custom.data());

    // 再切回已注册 rtti：回到专属面板（面板按 rtti 缓存复用）
    panelWidget.updateUI(marker.data());
    QVERIFY2(panelWidget.currentItemPanel() == markerItemPanel,
             "switching back to the registered rtti should restore its cached panel");

    marker->detach();
    custom->detach();
}

/**
 * @brief 导出当前 2D 子图到 PNG/SVG/PDF 文件
 */
void TSTDAChartContextTest::testExportChartToFile()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data(), 800, 600));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    settle(fig.data());
    QVERIFY2(chart->width() > 0 && chart->height() > 0,
             qPrintable(QStringLiteral("the chart should have a non-empty size, got %1x%2")
                            .arg(chart->width())
                            .arg(chart->height())));

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // PNG：文件存在、非空、能作为图片读回
    const QString pngPath = tmpDir.filePath(QStringLiteral("chart.png"));
    QVERIFY2(fig->exportChartToFile(chart, pngPath, DAFigureExportFormat::Png), "PNG export should succeed");
    const QFileInfo pngInfo(pngPath);
    QVERIFY2(pngInfo.exists(), "the PNG file should exist");
    QVERIFY2(pngInfo.size() > 0, "the PNG file should not be empty");
    QVERIFY2(!QImage(pngPath).isNull(), "the PNG file should be a readable image");

    // SVG：文件存在且包含根 <svg 标记
    const QString svgPath = tmpDir.filePath(QStringLiteral("chart.svg"));
    QVERIFY2(fig->exportChartToFile(chart, svgPath, DAFigureExportFormat::Svg), "SVG export should succeed");
    QFile svgFile(svgPath);
    QVERIFY2(svgFile.open(QIODevice::ReadOnly), "the SVG file should be readable");
    const QByteArray svgData = svgFile.readAll();
    svgFile.close();
    QVERIFY2(svgData.contains("<svg"), "the SVG content should contain the <svg element");

    // PDF：文件存在、非空、以 %PDF 开头
    const QString pdfPath = tmpDir.filePath(QStringLiteral("chart.pdf"));
    QVERIFY2(fig->exportChartToFile(chart, pdfPath, DAFigureExportFormat::Pdf), "PDF export should succeed");
    QFile pdfFile(pdfPath);
    QVERIFY2(pdfFile.open(QIODevice::ReadOnly), "the PDF file should be readable");
    const QByteArray pdfData = pdfFile.readAll();
    pdfFile.close();
    QVERIFY2(pdfData.startsWith("%PDF"), "the PDF content should start with the %PDF header");

    // 后缀 → 格式推断与文件对话框过滤器
    QCOMPARE(static_cast< int >(DAFigureWidget::exportFormatFromFilePath(QStringLiteral("a.svg"))),
             static_cast< int >(DAFigureExportFormat::Svg));
    QCOMPARE(static_cast< int >(DAFigureWidget::exportFormatFromFilePath(QStringLiteral("a.PDF"))),
             static_cast< int >(DAFigureExportFormat::Pdf));
    QCOMPARE(static_cast< int >(DAFigureWidget::exportFormatFromFilePath(QStringLiteral("a.unknown"))),
             static_cast< int >(DAFigureExportFormat::Png));
    const QString filter = DAFigureWidget::exportFileFilter();
    QVERIFY2(filter.contains(QStringLiteral("*.png")) && filter.contains(QStringLiteral("*.svg"))
                 && filter.contains(QStringLiteral("*.pdf")),
             "the export file filter should contain png/svg/pdf entries");

    // SF-05：以所选过滤器为权威修正后缀（选 SVG 过滤器保留 .png 时必须改写为 .svg）
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart.png"),
                                                     QStringLiteral("SVG Image (*.svg)")),
             QStringLiteral("chart.svg"));
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart.png"),
                                                     QStringLiteral("PDF Document (*.pdf)")),
             QStringLiteral("chart.pdf"));
    // 后缀与过滤器一致：原样返回（保留原路径大小写）
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart.PNG"),
                                                     QStringLiteral("PNG Image (*.png)")),
             QStringLiteral("chart.PNG"));
    // 无后缀/未知后缀：追加过滤器后缀
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart"),
                                                     QStringLiteral("SVG Image (*.svg)")),
             QStringLiteral("chart.svg"));
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart.txt"),
                                                     QStringLiteral("PDF Document (*.pdf)")),
             QStringLiteral("chart.txt.pdf"));
    // 过滤器解析不出唯一已知扩展名：保留旧默认行为（已知后缀原样，否则补 .png）
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart.svg"), QString()),
             QStringLiteral("chart.svg"));
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart"), QString()),
             QStringLiteral("chart.png"));
    QCOMPARE(DAFigureWidget::applyExportFilterSuffix(QStringLiteral("chart.png"),
                                                     QStringLiteral("Images (*.png *.svg)")),
             QStringLiteral("chart.png"));
}

/**
 * @brief 复制当前 2D 子图到剪贴板
 */
void TSTDAChartContextTest::testCopyChartToClipboard()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data(), 640, 480));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    settle(fig.data());
    QVERIFY2(chart->width() > 0 && chart->height() > 0, "the chart should have a non-empty size");

    QVERIFY2(fig->copyChartToClipboard(chart), "copying the chart to the clipboard should succeed");
    const QPixmap clipboardPixmap = QApplication::clipboard()->pixmap();
    QVERIFY2(!clipboardPixmap.isNull(), "the clipboard should contain a non-null pixmap");

    // 传 nullptr 时使用当前子图
    QVERIFY2(fig->copyChartToClipboard(), "copyChartToClipboard(nullptr) should use the current chart");
    QVERIFY(!QApplication::clipboard()->pixmap().isNull());
}

/**
 * @brief 空 chart/空路径导出返回 false，不崩溃
 */
void TSTDAChartContextTest::testExportWithEmptyArguments()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // 没有任何子图的 figure：为空参数与「无当前子图」提供现场
    QScopedPointer< DAFigureWidget > emptyFig(new DAFigureWidget());
    QVERIFY2(!emptyFig->exportChartToFile(nullptr, tmpDir.filePath(QStringLiteral("null.png")),
                                         DAFigureExportFormat::Png),
             "exporting a null chart must fail");
    QVERIFY2(!emptyFig->exportChartToFile(nullptr, QString(), DAFigureExportFormat::Png),
             "exporting a null chart with an empty path must fail");
    QVERIFY2(!emptyFig->copyChartToClipboard(), "copying without any chart must fail");

    // 有子图但路径为空：返回 false 且不写文件
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data(), 400, 300));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    settle(fig.data());
    QVERIFY2(!fig->exportChartToFile(chart, QString(), DAFigureExportFormat::Png),
             "exporting with an empty file path must fail");
    QVERIFY2(!fig->exportChartToFile(chart, QString(), DAFigureExportFormat::Svg),
             "exporting SVG with an empty file path must fail");
    QVERIFY2(!fig->exportChartToFile(chart, QString(), DAFigureExportFormat::Pdf),
             "exporting PDF with an empty file path must fail");
    QVERIFY(chart->width() > 0 && chart->height() > 0);
}

/**
 * @brief 覆盖保存：写下失败时返回 false 且旧文件原样保留；可写时成功替换（C-13）
 *
 * 旧实现只判 `painter.end() && QFileInfo::exists(filePath)`：目标文件已存在时，即使本次
 * 写入失败也会误报成功。新实现经 QSaveFile 写入，commit 成功才替换目标文件。
 */
void TSTDAChartContextTest::testExportOverwriteFailureKeepsExistingFile()
{
    QScopedPointer< DAFigureWidget > fig(new DAFigureWidget());
    QVERIFY(initFigure(fig.data(), 400, 300));
    DAChartWidget* chart = fig->createChart();
    QVERIFY(chart);
    settle(fig.data());

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QByteArray sentinel = QByteArrayLiteral("SENTINEL");

    struct ExportCase
    {
        const char* suffix;
        DAFigureExportFormat format;
    };
    const ExportCase cases[] = {
        { "png", DAFigureExportFormat::Png },
        { "svg", DAFigureExportFormat::Svg },
        { "pdf", DAFigureExportFormat::Pdf },
    };
    for (const ExportCase& c : cases) {
        const QString suffix = QString::fromLatin1(c.suffix);
        const QString path   = tmpDir.filePath(QStringLiteral("chart.") + suffix);
        {
            QFile seed(path);
            QVERIFY(seed.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QCOMPARE(seed.write(sentinel), static_cast< qint64 >(sentinel.size()));
        }
#if defined(Q_OS_WIN)
        // 用不带删除共享的读句柄占用目标文件：本次覆盖保存无法替换目标，必须返回 false。
        // （Windows 上 Qt 的读句柄不带 FILE_SHARE_DELETE，MoveFileEx/ReplaceFile 会失败）
        QFile lock(path);
        QVERIFY(lock.open(QIODevice::ReadOnly));
        const bool lockedOk = fig->exportChartToFile(chart, path, c.format);
        lock.close();
        QVERIFY2(!lockedOk, qPrintable(QStringLiteral("export must fail while the target cannot be replaced: %1").arg(suffix)));
        // 失败不破坏旧文件
        QFile preserved(path);
        QVERIFY(preserved.open(QIODevice::ReadOnly));
        QCOMPARE(preserved.readAll(), sentinel);
        preserved.close();
#endif
        // 目标可写时的覆盖保存：返回 true 且内容确实被替换
        QVERIFY2(fig->exportChartToFile(chart, path, c.format),
                 qPrintable(QStringLiteral("overwriting an existing file must succeed: %1").arg(suffix)));
        const QFileInfo exported(path);
        QVERIFY(exported.exists());
        QVERIFY2(exported.size() > sentinel.size(),
                 qPrintable(QStringLiteral("the exported file should be replaced with new content: %1").arg(suffix)));
    }
}

}  // namespace DA

//===================================================
// main
//===================================================
QTEST_MAIN(DA::TSTDAChartContextTest)

#include "main.moc"
