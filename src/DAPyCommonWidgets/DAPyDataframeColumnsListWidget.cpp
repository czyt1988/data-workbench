#include "DAPyDataframeColumnsListWidget.h"
#include "DAPyDTypeComboBox.h"
#include <QDebug>
namespace DA
{
DAPyDataframeColumnsListWidget::DAPyDataframeColumnsListWidget(QWidget* parent) : QListWidget(parent)
{
	setSelectionMode(QAbstractItemView::SingleSelection);
}

DAPyDataframeColumnsListWidget::~DAPyDataframeColumnsListWidget()
{
}

void DAPyDataframeColumnsListWidget::setDataframe(const DAPyDataFrame& df)
{
	mDataframe = df;
	updateColumnsInfo(df);
}

DAPyDataFrame DAPyDataframeColumnsListWidget::getDataFrame() const
{
	return mDataframe;
}

/**
 * @brief 获取当前选择的列名
 * @return
 */
QString DAPyDataframeColumnsListWidget::getSelectedColumn() const noexcept
{
	try {
		int c                 = currentRow();
		QList< QString > cols = mDataframe.columns();
		return cols[ c ];
	} catch (const std::exception& e) {
		qCritical() << tr("Exception in get selected column:%1").arg(e.what());  // cn:获取选中的列发生异常：%1
	}
	return QString();
}

/**
 * @brief 获取选中的series
 * @return
 */
DAPySeries DAPyDataframeColumnsListWidget::getCurrentSeries() const noexcept
{
	try {
		QString c    = getSelectedColumn();
		DAPySeries s = mDataframe[ c ];
		return s;
	} catch (const std::exception& e) {
		qCritical() << tr("Exception in get selected series:%1").arg(e.what());  // cn:获取选中的序列发生异常：%1
	}
	return DAPySeries();
}

/**
 * @brief 获取所有选中的series
 * @return
 */
QList< DAPySeries > DAPyDataframeColumnsListWidget::getAllSelectedSeries() const
{
	QList< DAPySeries > res;
	auto indexs = selectedIndexes();
	try {
		for (int i = 0; i < indexs.size(); ++i) {
			int dfIndex  = indexs[ i ].row();
			DAPySeries s = mDataframe[ dfIndex ];
			res.append(s);
		}
	} catch (const std::exception& e) {
		qCritical() << tr("Exception in get selected series:%1").arg(e.what());  // cn:获取选中的序列发生异常：%1
	}
	return res;
}

/**
 * @brief 获取选择的索引.
 *
 * @return
 */
QList< int > DAPyDataframeColumnsListWidget::getAllSelectedSeriesIndexs() const
{
	QList< int > res;
	auto indexs = selectedIndexes();
	for (int i = 0; i < indexs.size(); ++i) {
		int dfIndex = indexs[ i ].row();
		res.append(dfIndex);
	}
	return res;
}

/**
 * @brief 获取选中的索引名
 *
 * @return $RETURN
 */
QList< QString > DAPyDataframeColumnsListWidget::getAllSelectedSeriesNames() const
{
	QList< QString > res;
	auto selItems = selectedItems();
	for (int i = 0; i < selItems.size(); ++i) {
        auto t = selItems[i]->text();
		res.append(t);
	}
	return res;
}

/**
 * @brief 更新信息
 */
void DAPyDataframeColumnsListWidget::updateColumnsInfo()
{
	updateColumnsInfo(mDataframe);
}

/**
 * @brief 更新信息
 * @param df
 */
void DAPyDataframeColumnsListWidget::updateColumnsInfo(const DAPyDataFrame& df)
{
	clear();
	if (!df) {
		clear();
		return;
	}
	QList< QString > cols = df.columns();
	for (int i = 0; i < cols.size(); ++i) {
		QListWidgetItem* item = new QListWidgetItem(cols[ i ], this);
		item->setIcon(DAPyDTypeComboBox::getIconByDtypeChar(df[ cols[ i ] ].dtype().char_()));
	}
}

}  // end DA
