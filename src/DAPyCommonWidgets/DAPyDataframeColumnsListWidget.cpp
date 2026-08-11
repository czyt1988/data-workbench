#include "DAPyDataframeColumnsListWidget.h"
#include "DAPyDTypeComboBox.h"
#include <QDebug>
#include "DALogCategory.h"
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
    int c = currentRow();
    if (c < 0) {
        return QString();
    }
    try {
        return mDataframe.columnName(static_cast< std::size_t >(c));
    } catch (const std::exception& e) {
        qCritical() << "Exception in getting selected column:" << e.what();
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
        qCritical() << "Exception in getting selected series:" << e.what();
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
    auto indexes = selectedIndexes();
    try {
        for (int i = 0; i < indexes.size(); ++i) {
            int dfIndex  = indexes[ i ].row();
            DAPySeries s = mDataframe[ dfIndex ];
            res.append(s);
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception in getting selected series:" << e.what();
        res.clear();
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
    auto indexes = selectedIndexes();
    for (int i = 0; i < indexes.size(); ++i) {
        int dfIndex = indexes[ i ].row();
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
        auto t = selItems[ i ]->text();
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
        return;
    }
    QList< QString > cols = df.columns();
    for (int i = 0; i < cols.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(cols[ i ], this);
        DAPyDType dt          = df.dtypeObject(static_cast< std::size_t >(i));
        item->setIcon(DAPyDTypeComboBox::getIconByDType(dt));
    }
}

}  // end DA
