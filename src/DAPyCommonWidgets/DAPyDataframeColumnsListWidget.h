#ifndef DAPYDATAFRAMECOLUMNSLISTWIDGET_H
#define DAPYDATAFRAMECOLUMNSLISTWIDGET_H
#include <QListWidget>
#include "DAPyCommonWidgetsAPI.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
namespace DA
{

/**
 * @brief 列举dataframe所有的columns
 */
class DAPYCOMMONWIDGETS_API DAPyDataframeColumnsListWidget : public QListWidget
{
    Q_OBJECT
public:
    DAPyDataframeColumnsListWidget(QWidget* parent = nullptr);
    ~DAPyDataframeColumnsListWidget();
public slots:
    //设置当前的dtype
    void setDataframe(const DAPyDataFrame& df);
    DAPyDataFrame getDataFrame() const;
    //更新列信息
    void updateColumnsInfo();
    //获取当前选择的列名
    QString getSelectedColumn() const noexcept;
    //获取选中的series
    DAPySeries getSelectedSeries() const noexcept;

private:
    void updateColumnsInfo(const DAPyDataFrame& df);

protected:
    DAPyDataFrame _dataframe;
};
}

#endif  // DAPYDATAFRAMECOLUMNSLISTWIDGET_H
