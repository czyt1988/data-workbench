#include "DAPyDataframeColumnsListWidget.h"
#include "DAPyDTypeComboBox.h"
#include <QDebug>
namespace DA
{
DAPyDataframeColumnsListWidget::DAPyDataframeColumnsListWidget(QWidget* parent) : QListWidget(parent)
{
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
DAPySeries DAPyDataframeColumnsListWidget::getSelectedSeries() const noexcept
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
        item->setIcon(DAPyDTypeComboBox::getIconByDtypeChar(df[ cols[ i ] ].dtype().char_()));
    }
}
}
