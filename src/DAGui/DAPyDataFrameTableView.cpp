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
DAPyDataFrameTableView::DAPyDataFrameTableView(QWidget* parent) : DACacheWindowTableView(parent)
{
	setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
	setAlternatingRowColors(false);
	if (QHeaderView* hheader = horizontalHeader()) {
		hheader->setSectionResizeMode(QHeaderView::Interactive);
	}
}

DAPyDataFrameTableView::~DAPyDataFrameTableView()
{
}

DAPyDataFrameTableModel* DAPyDataFrameTableView::getDataframeModel() const
{
    return qobject_cast< DAPyDataFrameTableModel* >(model());
}

void DAPyDataFrameTableView::setDataFrame(const DAPyDataFrame& d)
{
    DAPyDataFrameTableModel* m = getDataframeModel();
    if (!m) {
		qWarning() << tr("DataFrameTableView must set model first");  // cn:你需要先设置模型
		return;
	}
    m->setDataFrame(d);
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
    DAPyDataFrameTableModel* m = getDataframeModel();
    if (m) {
        return m->dataFrame();
	}
	return DAPyDataFrame();
}

}
