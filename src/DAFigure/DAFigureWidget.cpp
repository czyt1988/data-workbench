#include "DAFigureWidget.h"
#include <QtWidgets/QApplication>
#include <QMessageBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QAction>
#include <QMimeData>
#include <QPaintEvent>
#include <QCoreApplication>
#include <QScopedPointer>
#include <QChildEvent>
#include <QCursor>
#include <QPainter>
#include <QVBoxLayout>
#include <QUndoStack>
#include <QDebug>
#include <QScopedPointer>
// chart
#include "DAChartUtil.h"
#include "DAChartWidget.h"
#include "DAChartSerialize.h"
#include "DAFigureContainer.h"
#include "DAFigureWidgetOverlayChartEditor.h"
namespace DA
{
const float c_figurewidget_default_x = 0.05f;
const float c_figurewidget_default_y = 0.05f;
const float c_figurewidget_default_w = 0.9f;
const float c_figurewidget_default_h = 0.9f;
//===================================================
// command
//===================================================

/**
 * @brief DAFigureWidget命令的基本体
 */
class DAFigureWidgetCommandBase : public QUndoCommand
{
public:
	DAFigureWidgetCommandBase(DAFigureWidget* fig, QUndoCommand* par = nullptr) : QUndoCommand(par), mFig(fig)
	{
	}
	DAFigureWidget* figure()
	{
		return mFig;
	}

public:
	DAFigureWidget* mFig;
};

/**
 * @brief 创建绘图
 */
class DAFigureWidgetCommandCreateChart : public DAFigureWidgetCommandBase
{
public:
	DAFigureWidgetCommandCreateChart(DAFigureWidget* fig,
									 float xPresent,
									 float yPresent,
									 float wPresent,
									 float hPresent,
									 QUndoCommand* par = nullptr)
		: DAFigureWidgetCommandBase(fig, par)
		, mChart(nullptr)
		, mXPresent(xPresent)
		, mYPresent(yPresent)
		, mWPresent(wPresent)
		, mHPresent(hPresent)
		, mNeedDelete(false)
	{
		setText(QObject::tr("create chart"));  // cn:创建绘图
	}
	~DAFigureWidgetCommandCreateChart()
	{
		if (mNeedDelete) {
			if (mChart) {
				mChart->deleteLater();
			}
		}
	}
	void redo() override
	{
		mNeedDelete = false;
		if (mChart) {
			figure()->addChart(mChart, mXPresent, mYPresent, mWPresent, mHPresent);
		} else {
			mChart = figure()->createChart(mXPresent, mYPresent, mWPresent, mHPresent);
			mChart->setXLabel("x");
			mChart->setYLabel("y");
		}
	}
	void undo() override
	{
		mNeedDelete = true;
		figure()->removeChart(mChart);
	}

public:
	DAChartWidget* mChart;
	float mXPresent;
	float mYPresent;
	float mWPresent;
	float mHPresent;
	bool mNeedDelete;
};

/**
 * @brief 设置绘图中窗体的尺寸
 */
class DAFigureWidgetCommandResizeWidget : public DAFigureWidgetCommandBase
{
public:
	DAFigureWidgetCommandResizeWidget(DAFigureWidget* fig,
									  QWidget* w,
									  const QRectF& oldPresent,
									  const QRectF& newPresent,
									  QUndoCommand* par = nullptr)
		: DAFigureWidgetCommandBase(fig, par), mWidget(w), mOldPresent(oldPresent), mNewPresent(newPresent)
	{
		setText(QObject::tr("set figure widget size"));  // cn:设置绘图中窗体的尺寸
	}
	void redo() override
	{
		figure()->setWidgetPosPercent(mWidget, mNewPresent);
	}
	void undo() override
	{
		figure()->setWidgetPosPercent(mWidget, mOldPresent);
	}

public:
	QWidget* mWidget;
	QRectF mOldPresent;
	QRectF mNewPresent;
};

/**
 * @brief 添加Item
 */
class DAFigureWidgetCommandAttachItem : public DAFigureWidgetCommandBase
{
public:
	/**
	 * @brief 添加Item
	 * @param fig figure
	 * @param chart 对应的DAChartWidget指针
	 * @param item 对应的QwtPlotItem
	 * @param skipFirst 第一次跳过item->attach(chart);操作，后续的redo不会再跳过
	 * @param par
	 */
	DAFigureWidgetCommandAttachItem(DAFigureWidget* fig,
									DAChartWidget* chart,
									QwtPlotItem* item,
									bool skipFirst    = true,
									QUndoCommand* par = nullptr)
		: DAFigureWidgetCommandBase(fig, par), mChart(chart), mItem(item), mSkipFirst(skipFirst), mNeedDelete(false)
	{
		setText(QObject::tr("add item in chart"));  // cn:设置绘图中窗体的尺寸
	}
	~DAFigureWidgetCommandAttachItem()
	{
		if (mNeedDelete) {
			if (mItem) {
				delete mItem;
			}
		}
	}
	void redo() override
	{
		if (mSkipFirst) {
			mSkipFirst = false;
		} else {
			mItem->attach(mChart);
			mNeedDelete = false;
		}
	}
	void undo() override
	{
		mItem->detach();
		mNeedDelete = true;
	}

public:
	DAChartWidget* mChart;
	QwtPlotItem* mItem;
	bool mSkipFirst;
	bool mNeedDelete;
};
//===================================================
// DAFigureWidgetPrivate
//===================================================

class DAFigureWidget::PrivateData
{
	DA_DECLARE_PUBLIC(DAFigureWidget)
public:
	DAChartWidget* mCurrentChart { nullptr };
	DAFigureWidgetOverlayChartEditor* mChartEditorOverlay { nullptr };  ///< 编辑模式
	QBrush mBackgroundBrush;                                            ///< 背景
	QUndoStack mUndoStack;                                              ///<
	QScopedPointer< DAChartFactory > mFactory;                          ///< 绘图创建的工厂
	DAColorTheme mColorTheme;  ///< 主题，注意，这里不要用DAColorTheme mColorTheme { DAColorTheme::ColorTheme_Archambault }这样的初始化，会被当作std::initializer_list< QColor >捕获
public:
	PrivateData(DAFigureWidget* p) : q_ptr(p), mColorTheme(DAColorTheme::ColorTheme_Archambault)
	{
		mFactory.reset(new DAChartFactory());
	}

	void setupUI()
	{
		q_ptr->setBackgroundColor(QColor(255, 255, 255));
		q_ptr->setWindowIcon(QIcon(":/DAFigure/icon/figure.svg"));
	}

	void retranslateUi()
	{
		q_ptr->setWindowTitle(QApplication::translate("DAFigureWidget", "Figure", 0));
	}
};

//===================================================
// DAFigureWidget
//===================================================
DAFigureWidget::DAFigureWidget(QWidget* parent) : DAFigureContainer(parent), DA_PIMPL_CONSTRUCT
{
	d_ptr->setupUI();
	setFocusPolicy(Qt::ClickFocus);
	static int s_figure_count = 0;

	++s_figure_count;
	setWindowTitle(QString("figure-%1").arg(s_figure_count));
	setMinimumWidth(100);
	setMinimumHeight(50);
}

DAFigureWidget::~DAFigureWidget()
{
	// qDebug() << "SAFigureWindow destroy";
}

DAChartFactory* DAFigureWidget::getChartFactory() const
{
    return d_ptr->mFactory.get();
}

/**
 * @brief 设置ChartFactory
 * @param fac
 */
void DAFigureWidget::setupChartFactory(DAChartFactory* fac)
{
    d_ptr->mFactory.reset(fac);
}

/**
 * @brief 添加一个chart
 *
 * 默认的位置占比为0.05f, 0.05f, 0.9f, 0.9f
 * @return  返回2D绘图的指针
 */
DAChartWidget* DAFigureWidget::createChart()
{
    return (createChart(c_figurewidget_default_x, c_figurewidget_default_y, c_figurewidget_default_w, c_figurewidget_default_h));
}

/**
 * @brief 添加一个chart，指定位置占比
 * @param xPresent
 * @param yPresent
 * @param wPresent
 * @param hPresent
 * @return
 */
DAChartWidget* DAFigureWidget::createChart(float xPresent, float yPresent, float wPresent, float hPresent)
{
	DAChartWidget* chart = d_ptr->mFactory->createChart(this);
	addChart(chart, xPresent, yPresent, wPresent, hPresent);
	// 不加这句话，有时候不显示出来
	chart->show();
	// 对于有Overlay，需要把Overlay提升到最前面，否则会被覆盖
	if (d_ptr->mChartEditorOverlay) {
		d_ptr->mChartEditorOverlay->raise();  // 同时提升最前
	}
	return chart;
}

/**
 * @brief 移除chart，但不会delete
 * @param chart
 */
void DAFigureWidget::removeChart(DAChartWidget* chart)
{
    removeWidget(chart);
}

/**
 * @brief 支持redo/undo的createchart
 * @return
 */
DAChartWidget* DAFigureWidget::createChart_()
{
    return createChart_(c_figurewidget_default_x, c_figurewidget_default_y, c_figurewidget_default_w, c_figurewidget_default_h);
}

/**
 * @brief 支持redo/undo的createchart
 * @param xPresent
 * @param yPresent
 * @param wPresent
 * @param hPresent
 * @return
 */
DAChartWidget* DAFigureWidget::createChart_(float xPresent, float yPresent, float wPresent, float hPresent)
{
	DAFigureWidgetCommandCreateChart* cmd = new DAFigureWidgetCommandCreateChart(this, xPresent, yPresent, wPresent, hPresent);
	d_ptr->mUndoStack.push(cmd);
	// 必须先push再获取chart
	return cmd->mChart;
}

/**
 * @brief 添加一个chart，指定位置占比
 * @param chart 绘图
 * @param xPresent
 * @param yPresent
 * @param wPresent
 * @param hPresent
 * @return
 */
void DAFigureWidget::addChart(DAChartWidget* chart, float xPresent, float yPresent, float wPresent, float hPresent)
{
	addWidget(chart, xPresent, yPresent, wPresent, hPresent);
	d_ptr->mCurrentChart = chart;
	emit chartAdded(chart);
	setFocusProxy(chart);
}

/**
 * @brief 获取所有的绘图
 * @return
 */
QList< DAChartWidget* > DAFigureWidget::getCharts() const
{
	QList< DAChartWidget* > res;
	QList< QWidget* > widgets = getWidgetList();

	for (QWidget* w : qAsConst(widgets)) {
		DAChartWidget* chart = qobject_cast< DAChartWidget* >(w);
		if (chart) {
			res.append(chart);
		}
	}
	return (res);
}

/**
 * @brief 获取所有的绘图,绘图有序
 * @return
 */
QList< DAChartWidget* > DAFigureWidget::getChartsOrdered() const
{
	QList< DAChartWidget* > res;
	QList< QWidget* > widgets = getOrderedWidgetList();

	for (QWidget* w : qAsConst(widgets)) {
		DAChartWidget* chart = qobject_cast< DAChartWidget* >(w);
		if (chart) {
			res.append(chart);
		}
	}
	return (res);
}

/**
 * @brief 当前的图表的指针
 * @return 当没有2d绘图时返回nullptr
 */
DAChartWidget* DAFigureWidget::getCurrentChart() const
{
    return (d_ptr->mCurrentChart);
}

/**
 * @brief like matlab/matplotlib api gca
 * @return
 */
DAChartWidget* DAFigureWidget::gca() const
{
    return getCurrentChart();
}

/**
 * @brief 清空同时删除
 */
void DAFigureWidget::clearAllCharts()
{
	QList< DAChartWidget* > charts = getCharts();

	while (!charts.isEmpty()) {
		DAChartWidget* p = charts.takeLast();
		emit chartWillRemove(p);
		p->hide();
		p->deleteLater();
	}
}

/**
 * @brief 设置figure背景
 * @param brush
 */
void DAFigureWidget::setBackgroundColor(const QBrush& brush)
{
	d_ptr->mBackgroundBrush = brush;
	repaint();
}

/**
 * @brief 设置figure背景
 * @param clr
 */
void DAFigureWidget::setBackgroundColor(const QColor& clr)
{
	d_ptr->mBackgroundBrush.setStyle(Qt::SolidPattern);
	d_ptr->mBackgroundBrush.setColor(clr);
	repaint();
}

/**
 * @brief 获取背景颜色
 * @return
 */
const QBrush& DAFigureWidget::getBackgroundColor() const
{
    return (d_ptr->mBackgroundBrush);
}

/**
 * @brief 设置当前的chart
 * @param p 如果p和当前的currentChart一样，不做任何动作
 * @return 如果成功设置返回true，如果当前窗口已经是p，则返回true，但不会发射currentWidgetChanged信号
 * @sa currentWidgetChanged
 */
bool DAFigureWidget::setCurrentChart(DAChartWidget* p)
{
	if (p == d_ptr->mCurrentChart) {
		return (true);
	}
	if (!isWidgetInContainer(p)) {
		return (false);
	}
	d_ptr->mCurrentChart = p;
	// setFocusProxy(p);
	// 如果在进行子窗口编辑模式，此时需要重新设置编辑
	if (isEnableSubChartEditor()) {
		// 避免信号重复触发，虽然不影响
		QSignalBlocker bl(d_ptr->mChartEditorOverlay);
		Q_UNUSED(bl);
		d_ptr->mChartEditorOverlay->setActiveWidget(p);
	}
	emit currentChartChanged(p);

	return (true);
}

/**
 * @brief 获取当前的chart，如果没有current chart，或figure不存在chart，
 * 则创建一个新chart，此函数不返回nullptr
 * @return
 */
DAChartWidget* DAFigureWidget::currentChart()
{
	DAChartWidget* w = getCurrentChart();
	if (w) {
		return w;
	}
	// 到这里说明没有chart
	QList< DAChartWidget* > cs = getCharts();
	if (!cs.empty()) {
		return cs.first();
	}
	return createChart();
}

/**
 * @brief 通过item查找对应的SAChart2D，如果没有返回nullptr
 * @param item
 * @return 如果没有返回nullptr
 */
DAChartWidget* DAFigureWidget::findChartFromItem(QwtPlotItem* item) const
{
	QList< DAChartWidget* > charts = getCharts();

	for (DAChartWidget* w : qAsConst(charts)) {
		QwtPlotItemList items = w->itemList();
		if (items.contains(item)) {
			return (w);
		}
	}
	return (nullptr);
}

///
/// \brief 是否开始子窗口编辑模式
/// \param enable
/// \param ptr 通过此参数可以指定自定义的编辑器，若为nullptr，将使用默认的编辑器，此指针的管理权将移交SAFigureWindow
///
void DAFigureWidget::enableSubChartEditor(bool enable)
{
	if (enable) {
		if (nullptr == d_ptr->mChartEditorOverlay) {
			d_ptr->mChartEditorOverlay = new DAFigureWidgetOverlayChartEditor(this);
			connect(d_ptr->mChartEditorOverlay,
					&DAFigureWidgetOverlayChartEditor::widgetGeometryChanged,
					this,
					&DAFigureWidget::onWidgetGeometryChanged);
			connect(d_ptr->mChartEditorOverlay,
					&DAFigureWidgetOverlayChartEditor::activeWidgetChanged,
					this,
					&DAFigureWidget::onOverlayActiveWidgetChanged);
			d_ptr->mChartEditorOverlay->show();
			d_ptr->mChartEditorOverlay->raise();  // 同时提升最前
		} else {
			if (d_ptr->mChartEditorOverlay->isHidden()) {
				d_ptr->mChartEditorOverlay->show();
				d_ptr->mChartEditorOverlay->raise();  // 同时提升最前
			}
		}
	} else {
		if (d_ptr->mChartEditorOverlay) {
			delete d_ptr->mChartEditorOverlay;
			d_ptr->mChartEditorOverlay = nullptr;
		}
	}
}

///
/// \brief 获取子窗口编辑器指针，若没有此编辑器，返回nullptr
///
/// 此指针的管理权在SAFigureWindow上，不要在外部对此指针进行释放
/// \return
///
DAFigureWidgetOverlayChartEditor* DAFigureWidget::getSubChartEditor() const
{
    return (d_ptr->mChartEditorOverlay);
}

/**
 * @brief SAFigureWindow::isSubWindowEditingMode
 * @return
 */
bool DAFigureWidget::isEnableSubChartEditor() const
{
	if (d_ptr->mChartEditorOverlay) {
		return (d_ptr->mChartEditorOverlay->isVisible());
	}
	return (false);
}

/**
 * @brief 获取图表的数量
 * @return
 */
int DAFigureWidget::getChartCount() const
{
	int c = 0;

	const QList< QWidget* > widgets = getWidgetList();
	for (QWidget* w : widgets) {
		if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(w)) {
			++c;
		}
	}
	return (c);
}

/**
 * @brief 获取默认的绘图颜色
 *
 * 每次调用figure的绘图相关函数，会调用getDefaultColor获取默认颜色为曲线填充，
 * 默认会根据颜色主题来变换颜色，也可以继承此函数，让figure每次给出自定义的颜色
 * @return
 */
QColor DAFigureWidget::getDefaultColor() const
{
    return (d_ptr->mColorTheme)++;
}

void DAFigureWidget::setFigureColorTheme(const DAColorTheme& th)
{
    d_ptr->mColorTheme = th;
}

DAColorTheme DAFigureWidget::getFigureColorTheme() const
{
    return d_ptr->mColorTheme;
}

const DAColorTheme& DAFigureWidget::figureColorTheme() const
{
    return d_ptr->mColorTheme;
}

DAColorTheme& DAFigureWidget::figureColorTheme()
{
    return d_ptr->mColorTheme;
}

/**
 * @brief 支持redo/undo的添加item
 *
 * 等同addItem_(gca(),item)
 * @param item
 * @return 如果没有加入成功，返回false
 */
bool DAFigureWidget::addItem_(QwtPlotItem* item)
{
	if (DAChartWidget* chart = gca()) {
		addItem_(chart, item);
		return true;
	}
	return false;
}

/**
 * @brief 支持redo/undo的添加item
 * @param chart
 * @param item
 */
void DAFigureWidget::addItem_(DAChartWidget* chart, QwtPlotItem* item)
{
	push(new DAFigureWidgetCommandAttachItem(this, chart, item, false));
}

/**
 * @brief 支持redo/undo的addCurve，等同于gca()->addCurve
 * @param xyDatas
 * @return 如果添加失败，返回一个nullptr
 */
QwtPlotCurve* DAFigureWidget::addCurve_(const QVector< QPointF >& xyDatas)
{
	if (DAChartWidget* chart = gca()) {
		QwtPlotCurve* item = chart->addCurve(xyDatas);
		DAChartUtil::setPlotItemColor(item, getDefaultColor());
		addItem_(chart, item);
		return item;
	}
	return nullptr;
}

/**
 * @brief 支持redo/undo的addScatter，等同于gca()->addCurve
 * @param xyDatas
 * @return 如果添加失败，返回一个nullptr
 */
QwtPlotCurve* DAFigureWidget::addScatter_(const QVector< QPointF >& xyDatas)
{
	if (DAChartWidget* chart = gca()) {
		QwtPlotCurve* item = chart->addScatter(xyDatas);
		DAChartUtil::setPlotItemColor(item, getDefaultColor());
		addItem_(chart, item);
		return item;
	}
	return nullptr;
}

/**
 * @brief 推入一个命令
 * @param cmd
 */
void DAFigureWidget::push(QUndoCommand* cmd)
{
    d_ptr->mUndoStack.push(cmd);
}

/**
 * @brief 获取内部的undoStack
 * @return
 */
QUndoStack* DAFigureWidget::getUndoStack()
{
    return &(d_ptr->mUndoStack);
}

/**
 * @brief 返回当前光标下的widget
 * @return 如果当前没有返回nullptr
 */
QWidget* DAFigureWidget::getUnderCursorWidget() const
{
	QPoint p = mapFromGlobal(QCursor::pos());
	return (childAt(p));
}

/**
 * @brief 返回在当前光标下的2D图
 * @return 如果当前没有返回nullptr
 */
DAChartWidget* DAFigureWidget::getUnderCursorChart() const
{
	QWidget* w = getUnderCursorWidget();
	return qobject_cast< DAChartWidget* >(w);
}

/**
 * @brief SAFigureWindow::paintEvent
 * @param e
 */
void DAFigureWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);

	p.setBrush(d_ptr->mBackgroundBrush);
	p.fillRect(0, 0, width(), height(), d_ptr->mBackgroundBrush);
	DAFigureContainer::paintEvent(e);
}

/**
 * @brief DAFigureWidgetChartRubberbandEditOverlay导致的尺寸变化
 * @param w 子窗体
 * @param oldGeometry 旧尺寸
 * @param newGeometry 新尺寸
 */
void DAFigureWidget::onWidgetGeometryChanged(QWidget* w, const QRect& oldGeometry, const QRect& newGeometry)
{
	Q_UNUSED(oldGeometry);
	QRectF oldPresent = getWidgetPosPercent(w);
	QRectF newPresent = calcPercentByGeometryRect(newGeometry);

	DAFigureWidgetCommandResizeWidget* cmd = new DAFigureWidgetCommandResizeWidget(this, w, oldPresent, newPresent);
	push(cmd);
	// 由于设置geo会有一定误差，因此，这里需要更新一下overlay
	if (d_ptr->mChartEditorOverlay) {
		d_ptr->mChartEditorOverlay->updateOverlay();
	}
}

/**
 * @brief DAFigureOverlayChartEditor的激活窗口变化
 *
 * 此槽函数用于改变当前的chart
 * @param oldActive
 * @param newActive
 */
void DAFigureWidget::onOverlayActiveWidgetChanged(QWidget* oldActive, QWidget* newActive)
{
	Q_UNUSED(oldActive);
	DAChartWidget* c = qobject_cast< DAChartWidget* >(newActive);
	if (c) {
		setCurrentChart(c);
	}
}

QDataStream& operator<<(QDataStream& out, const DAFigureWidget* p)
{
	const uint32_t magicStart = 0x1314abc;

	out << magicStart << p->saveGeometry();
	QList< DAChartWidget* > charts = p->getCharts();
	QList< QRectF > pos;

	for (int i = 0; i < charts.size(); ++i) {
		pos.append(p->getWidgetPosPercent(charts[ i ]));
	}
	out << pos;
	for (int i = 0; i < charts.size(); ++i) {
		out << charts[ i ];
	}
	return (out);
}

QDataStream& operator>>(QDataStream& in, DAFigureWidget* p)
{
	const uint32_t magicStart = 0x1314abc;
	int tmp;

	in >> tmp;
	if (tmp != magicStart) {
		throw DABadSerializeExpection(
			QObject::tr("DAFigureWidget get invalid magic strat code"));  // cn: DAFigureWidget的文件头异常
		return (in);
	}
	QByteArray geometryData, stateData;

	in >> geometryData;
	p->restoreGeometry(geometryData);
	QList< QRectF > pos;

	in >> pos;
	try {
		for (int i = 0; i < pos.size(); ++i) {
			const QRectF& r = pos[ i ];
			auto chart      = p->createChart(r.x(), r.y(), r.width(), r.height());
			std::unique_ptr< DAChartWidget > chart_guard(chart);
			in >> chart;
			chart->show();
			chart_guard.release();
		}
	} catch (const DABadSerializeExpection& exp) {
		throw exp;
	}
	return (in);
}
}
