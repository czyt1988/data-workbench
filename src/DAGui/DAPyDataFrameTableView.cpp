#include "DAPyDataFrameTableView.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QDebug>
#include <QElapsedTimer>
#include <QResizeEvent>
#include <QtMath>
#include <QSignalBlocker>
#include "Models/DAPyDataFrameTableModel.h"
namespace DA
{
DAPyDataFrameTableView::DAPyDataFrameTableView(QWidget* parent) : QTableView(parent)
{
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
	setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
	setAlternatingRowColors(false);
	QFontMetrics fm = fontMetrics();
	if (QHeaderView* vheader = verticalHeader()) {
		vheader->setDefaultSectionSize(fm.lineSpacing() * 1.2);
		vheader->setSectionsClickable(false);
		vheader->setSectionResizeMode(QHeaderView::Fixed);
	}
	if (QHeaderView* hheader = horizontalHeader()) {
		hheader->setSectionResizeMode(QHeaderView::Interactive);
	}
	if (QScrollBar* vsc = verticalScrollBar()) {
		connect(vsc, &QScrollBar::valueChanged, this, &DAPyDataFrameTableView::onVerticalScrollBarValueChanged);
    }
}

DAPyDataFrameTableView::~DAPyDataFrameTableView()
{
}

void DAPyDataFrameTableView::setDataframeModel(DAPyDataFrameTableModel* dataframeModle)
{
    // 断开旧模型连接
    if (mDataframeModel) {
        disconnect(mDataframeModel, &DAPyDataFrameTableModel::currentPageChanged, this, &DAPyDataFrameTableView::onPageChanged);
    }
	setModel(dataframeModle);
	mDataframeModel = dataframeModle;
    // 连接新模型信号
    if (mDataframeModel) {
        connect(mDataframeModel, &DAPyDataFrameTableModel::currentPageChanged, this, &DAPyDataFrameTableView::onPageChanged);
        updateScrollbarParameters();
    }
}

DAPyDataFrameTableModel* DAPyDataFrameTableView::getDataframeModel() const
{
	return mDataframeModel;
}

void DAPyDataFrameTableView::setDataFrame(const DAPyDataFrame& d)
{
	if (!mDataframeModel) {
		qWarning() << tr("DataFrameTableView must set model first");  // cn:你需要先设置模型
		return;
	}

	mDataframeModel->setDataFrame(d);
    updateScrollbarParameters();
}

DAPyDataFrame DAPyDataFrameTableView::getDataframe() const
{
	if (mDataframeModel) {
		return mDataframeModel->dataFrame();
	}
    return DAPyDataFrame();
}

void DAPyDataFrameTableView::updateScrollbarParameters()
{
    if (!mDataframeModel) {
        return;
    }

    QScrollBar* vsc = verticalScrollBar();
    if (!vsc) {
        return;
    }

    const int visibleRows = calcVisibleRowCount();
    const int totalRows   = mDataframeModel->getActualDataframeRowCount();
    const int maxPage     = qMax(0, (totalRows / mDataframeModel->getPageSize()) - 1);

    vsc->setRange(0, maxPage * mDataframeModel->getPageSize());  // 按页设置范围
    vsc->setPageStep(mDataframeModel->getPageSize());            // 每页步长
    vsc->setSingleStep(visibleRows);                             // 单步移动行数
}

void DAPyDataFrameTableView::resizeEvent(QResizeEvent* event)
{
    QTableView::resizeEvent(event);
    if (mDataframeModel) {
        updateScrollbarParameters();
    }
}

void DAPyDataFrameTableView::onVerticalScrollBarValueChanged(int v)
{
    static QElapsedTimer s_elasped;
    if (s_elasped.elapsed() < 50) {
        return;  // 50ms内的重复变化只处理一次,防止抖动
    }
    s_elasped.start();

    const int pageSize = mDataframeModel->getPageSize();
    const int newPage  = v / pageSize;

    if (newPage != mDataframeModel->getCurrentPage()) {
        QSignalBlocker bl(mDataframeModel);
        mDataframeModel->setCurrentPage(newPage);
        // 保持滚动偏移量（例如拖动到页面中间时换页）
        int offset = v % pageSize;
        verticalScrollBar()->setValue(newPage * pageSize + offset);
    }
}

void DAPyDataFrameTableView::onPageChanged(int page)
{
    // 数据变化时保持滚动位置
    QScrollBar* vsc = verticalScrollBar();
    if (!vsc) {
        return;
    }
    vsc->setValue(page * mDataframeModel->getPageSize());
}

int DAPyDataFrameTableView::calcVisibleRowCount() const
{
    QScrollBar* vsc      = verticalScrollBar();
    QHeaderView* vheader = verticalHeader();
    if (!vsc || !vheader) {
        return 0;
    }
    int baseRowHeight         = vheader->defaultSectionSize();
    const int actualRowHeight = vheader->sectionSize(0);
    baseRowHeight             = actualRowHeight > 0 ? actualRowHeight : baseRowHeight;

    const int viewportHeight = viewport()->height();
    return qMax(1, viewportHeight / baseRowHeight);
}

}
