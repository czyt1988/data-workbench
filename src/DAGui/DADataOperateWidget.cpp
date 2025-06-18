#include "DADataOperateWidget.h"
#include "ui_DADataOperateWidget.h"
#include <QDebug>
#include <QPointer>
// api
#include "DADataManager.h"
#include "DADataOperatePageWidget.h"
#include <QUndoStack>
#if DA_ENABLE_PYTHON
// widget
#include "DADataOperateOfDataFrameWidget.h"
// py
#include "DADataPyObject.h"
#endif
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

namespace DA
{

class DADataOperateWidget::PrivateData
{
public:
	DA_DECLARE_PUBLIC(DADataOperateWidget)
	PrivateData(DADataOperateWidget* p);

public:
	QMap< DA::DAData, QPointer< QWidget > > _dataToWidget;  ///< 记录数据对应的窗口
	DADataManager* _dataManager;
};

DADataOperateWidget::PrivateData::PrivateData(DADataOperateWidget* p) : q_ptr(p)
{
}

//===================================================
// DADataOperateWidget
//===================================================
DADataOperateWidget::DADataOperateWidget(DADataManager* mgr, QWidget* parent)
	: DAAbstractOperateWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADataOperateWidget)
{
	init();
	setDataManager(mgr);
}

DADataOperateWidget::DADataOperateWidget(QWidget* parent)
	: DAAbstractOperateWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DADataOperateWidget)
{
	init();
}

void DADataOperateWidget::init()
{
	ui->setupUi(this);
	connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DADataOperateWidget::onTabWidgetCurrentChanged);
	connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &DADataOperateWidget::onTabWidgetCloseRequested);
}

DADataOperateWidget::~DADataOperateWidget()
{
	delete ui;
}

void DADataOperateWidget::setDataManager(DADataManager* mgr)
{
	d_ptr->_dataManager = mgr;
	connect(mgr, &DADataManager::dataRemoved, this, &DADataOperateWidget::onDataRemoved);
	connect(mgr, &DADataManager::dataChanged, this, &DADataOperateWidget::onDataChanged);
}

DADataManager* DADataOperateWidget::getDataManger() const
{
    return d_ptr->_dataManager;
}

/**
 * @brief 当前显示的窗口
 * @return
 */
QWidget* DADataOperateWidget::currentWidget() const
{
    return ui->tabWidget->currentWidget();
}
/**
 * @brief 当前显示的DataFrame窗口，如果不是DataFrame窗口，返回nullptr
 * @return
 */
DADataOperateOfDataFrameWidget* DADataOperateWidget::getCurrentDataFrameWidget() const
{
#if DA_ENABLE_PYTHON
	return qobject_cast< DADataOperateOfDataFrameWidget* >(currentWidget());
#else
	return nullptr;
#endif
}

/**
 * @brief 获取当前表格操作选中的数据
 *
 * 如果用户打开一个表格，选中了其中一列，那么将返回那一列pd.Series作为数据，
 * 如果用户选中了多列，那么每列作为一个DAData，最后组成一个QList<DAData>返回,如果用户打开了表格，但没选择任何列，这个函数返回这个表作为Data（pd.DataFrame）
 *
 * 如果用户没有选择列，但选中了单元格，那么相当于选中了单元格对应的列
 *
 * 如果什么都没选中，那么返回一个空的list
 * @return
 */
QList< DAData > DADataOperateWidget::getCurrentSelectDatas() const
{
	QList< DAData > res;
#if DA_ENABLE_PYTHON
	DADataOperateOfDataFrameWidget* dfw = getCurrentDataFrameWidget();
	if (!dfw) {
		return res;
	}
	res = dfw->getSlectedSeries();
#else
	// TODO:目前还没有非python数据类型支持
#endif
	return res;
}

QUndoStack* DADataOperateWidget::getUndoStack()
{
#if DA_ENABLE_PYTHON
	DADataOperateOfDataFrameWidget* w = getCurrentDataFrameWidget();
	if (w) {
		return w->getUndoStack();
	}
#endif
	return nullptr;
}

#if DA_ENABLE_PYTHON
/**
 * @brief 获取当前选中的Dataframe,如果用户在选中了列，返回选中的列索引
 * @return
 */
std::pair< DAPyDataFrame, QList< int > > DADataOperateWidget::getCurrentSelectDataFrame() const
{
	std::pair< DAPyDataFrame, QList< int > > res;
	DADataOperateOfDataFrameWidget* dfw = getCurrentDataFrameWidget();
	if (!dfw) {
		return res;
	}
	res.first  = dfw->getDataframe();
	res.second = dfw->getSelectedDataframeCoumns();
	return res;
}
#endif
/**
 * @brief 显示数据，如果数据已经有，唤起对应的tab，如果没有，则创建一个sheet
 * @param d
 */
void DADataOperateWidget::showData(const DA::DAData& d)
{
	switch (d.getDataType()) {
	case DAAbstractData::TypePythonDataFrame:
		showDataframeData(d);
		break;
	default:
		break;
	}
}

/**
 * @brief 删除tab窗口，同时删除tab标签和上次tab对应的widget
 * @param w
 * @return 成功删除返回true
 */
bool DADataOperateWidget::removeTabWidget(QWidget* w)
{
	int ti = ui->tabWidget->indexOf(w);
	if (ti < 0) {
		qCritical() << tr("removing a widget that does not exist in tab");  // cn:正在移除一个不存在的窗口
		return false;
	}
	if (ti >= 0) {
		ui->tabWidget->removeTab(ti);
	}
	DADataOperatePageWidget* page = qobject_cast< DADataOperatePageWidget* >(w);
	if (page) {
		emit dataTableRemoving(page);
	}
	w->hide();
	w->deleteLater();
	// 移除_dataToWidget记录
	for (auto i = d_ptr->_dataToWidget.begin(); i != d_ptr->_dataToWidget.end();) {
		if (i.value() == w) {
			i = d_ptr->_dataToWidget.erase(i);
		} else {
			++i;
		}
	}
	return true;
}

/**
 * @brief 清除操作
 */
void DADataOperateWidget::clear()
{
	// 清空栈
	getUndoStack()->clear();
	// 窗口删除
	while (ui->tabWidget->count() != 0) {
		QWidget* tabWidget = ui->tabWidget->widget(0);
		ui->tabWidget->removeTab(0);
		// 删除窗口
		tabWidget->deleteLater();
	}
	// 数据清除
	getDataManger()->clear();
}

/**
 * @brief 数据删除过程触发的槽
 * @param d
 * @param index
 */
void DADataOperateWidget::onDataRemoved(const DA::DAData& d, int index)
{
	Q_UNUSED(index);
	auto ite = d_ptr->_dataToWidget.find(d);
	if (ite == d_ptr->_dataToWidget.end()) {
		return;
	}
	// 标记数据已经删除
	int ti          = ui->tabWidget->indexOf(ite.value());
	QString tabName = ui->tabWidget->tabText(ti);
	// 标记已删除
	tabName = tabName + tr("[deleted]");  // cn:[已删除]
	ui->tabWidget->setTabText(ti, tabName);
}

/**
 * @brief 变量信息改变
 * @param d
 * @param t
 */
void DADataOperateWidget::onDataChanged(const DA::DAData& d, DADataManager::ChangeType t)
{
	auto ite = d_ptr->_dataToWidget.find(d);
	if (ite == d_ptr->_dataToWidget.end()) {
		return;
	}
	int ti = ui->tabWidget->indexOf(ite.value());
	if (ti < 0) {
		return;
	}
	switch (t) {
	case DADataManager::ChangeName:
		ui->tabWidget->setTabText(ti, d.getName());
		break;
	case DADataManager::ChangeDescribe:
		ui->tabWidget->setTabToolTip(ti, d.getDescribe());
		break;
	default:
		break;
	}
}

/**
 * @brief tab标签切换
 * @param index
 */
void DADataOperateWidget::onTabWidgetCurrentChanged(int index)
{
	QWidget* w = ui->tabWidget->widget(index);
	if (!w) {
		return;
	}
#if DA_ENABLE_PYTHON
	if (DADataOperateOfDataFrameWidget* d = qobject_cast< DADataOperateOfDataFrameWidget* >(w)) {
		// 激活undostack
		d->activeUndoStack();
	}
#endif
	emit currentDataTableWidgetChanged(qobject_cast< DADataOperatePageWidget* >(w), index);
}

/**
 * @brief tab的关闭请求
 * @param index
 */
void DADataOperateWidget::onTabWidgetCloseRequested(int index)
{
	QWidget* w = ui->tabWidget->widget(index);
	if (!w) {
		return;
	}
	removeTabWidget(w);
}

void DADataOperateWidget::showDataframeData(const DA::DAData& d)
{
#if DA_ENABLE_PYTHON
	// 先查找是否已经存在对于窗口
	DADataOperateOfDataFrameWidget* w =
		qobject_cast< DADataOperateOfDataFrameWidget* >(d_ptr->_dataToWidget.value(d, nullptr).data());
	if (nullptr == w) {
		// 没有就创建
		w = new DADataOperateOfDataFrameWidget(d, ui->tabWidget);
		emit dataTableCreated(w);
		// 记录窗口
		d_ptr->_dataToWidget[ d ] = w;
	}
	// 在判断窗口是否已经存在于tabwidget
	int index = ui->tabWidget->indexOf(w);
	if (index < 0) {
		// 说明tab没有，添加进去
		index = ui->tabWidget->addTab(w, d.getName());
	} else {
		ui->tabWidget->setTabText(index, d.getName());  // 防止关闭tab后，窗口不销毁，且data进行了其他操作
	}
	ui->tabWidget->setTabToolTip(index, d.getDescribe());
	// 把当前的tabwidget唤起
	ui->tabWidget->setCurrentIndex(index);
#endif
}

}
