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
        vheader->setSectionResizeMode(QHeaderView::Interactive);
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
    // 设置数据时，按照行数设置垂直表头的宽度
    if (auto vh = verticalHeader()) {
        auto shape      = d.shape();
        QFontMetrics fm = vh->fontMetrics();
        int w           = fm.horizontalAdvance(QString(" %1 ").arg(shape.first));
        w               = w > 15 ? w : 15;
        vh->setFixedWidth(w);
        // qDebug() << "setFixWidth by" << QString("%1").arg(shape.first) << ",w=" << w;
    }
}

DAPyDataFrame DAPyDataFrameTableView::getDataframe() const
{
	if (mDataframeModel) {
		return mDataframeModel->dataFrame();
	}
	return DAPyDataFrame();
}

void DAPyDataFrameTableView::onVerticalScrollBarValueChanged(int v)
{
	if (!mDataframeModel) {
		return;
	}
	static QElapsedTimer s_elasped;
	if (s_elasped.elapsed() < 50) {
		return;  // 50ms内的重复变化只处理一次,防止抖动
	}
	s_elasped.start();

	// 计算滚动比例
    QScrollBar* vsc     = verticalScrollBar();
    const int totalRows = mDataframeModel->getActualDataframeRowCount();
    const int maxScroll = qMax(0, totalRows);

	const double ratio    = static_cast< double >(v) / (vsc->maximum() - vsc->minimum());
	const int targetStart = qMin(static_cast< int >(ratio * maxScroll), maxScroll);
    qDebug() << QString("onVerticalScrollBarValueChanged(%1):totalRows=%2,mCurrentShowRows=%3,maxScroll=%4,ratio=%5,"
                        "targetStart=%6")
                    .arg(v)
                    .arg(totalRows)
                    .arg(mCurrentShowRows)
                    .arg(maxScroll)
                    .arg(ratio)
                    .arg(targetStart);
	mDataframeModel->setCacheWindowStartRow(targetStart);
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

void DAPyDataFrameTableView::resizeEvent(QResizeEvent* event)
{
    // 刷新当前能显示多少行
    return QTableView::resizeEvent(event);
}

}
