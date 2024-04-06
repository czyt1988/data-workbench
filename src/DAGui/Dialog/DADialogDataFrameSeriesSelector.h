#ifndef DADIALOGDATAFRAMESERIESSELECTOR_H
#define DADIALOGDATAFRAMESERIESSELECTOR_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "pandas/DAPyDataFrame.h"
#include "DADataManager.h"
namespace Ui
{
class DADialogDataFrameSeriesSelector;
}
namespace DA
{
class DAGUI_API DADialogDataFrameSeriesSelector : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADialogDataFrameSeriesSelector)
public:
    explicit DADialogDataFrameSeriesSelector(QWidget* parent = nullptr);
    ~DADialogDataFrameSeriesSelector();
    // 获取选中的dataframe以及选中的列
    std::pair< DAPyDataFrame, QList< int > > getCurrentSelectDataFrame() const;
    // 设置当前的df，如果此函数设置为None，将会加载datamanager的数据列表给用户
    void setCurrentDataFrame(const DAPyDataFrame& df);
    DAPyDataFrame getCurrentDataFrame() const;
    // 设置数据管理器
    void setDataManager(DADataManager* mgr);
    //
    void updateUI();
private slots:
    void onCurrentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
    void onDataframeColumnsListWidgetRowChanged(int i);
protected:
    void changeEvent(QEvent* e);

private:
    Ui::DADialogDataFrameSeriesSelector* ui;
};
}  // end DA
#endif  // DADIALOGDATAFRAMESERIESSELECTOR_H
