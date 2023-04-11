#ifndef DADIALOGDATAFRAMEPLOT_H
#define DADIALOGDATAFRAMEPLOT_H
#include "DAGuiAPI.h"
#include <QDialog>
#include "DAData.h"
namespace Ui
{
class DADialogDataframePlot;
}

class QListWidgetItem;
namespace DA
{
class DADataManager;
/**
 * @brief 把dataframe抽取两列转换为两个double-vector
 * @code
 * DADialogDataframeToPointVector dlg;
 * dlg.setDataManager(xx);
 * dlg.setCurrentData(xxx);
 * if(QDialog::Accept == dlg.exec()){
 *
 * }
 * @endcode
 */
class DAGUI_API DADialogDataframePlot : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 绘图类型
     */
    enum ChartType
    {
        ChartCurve,      ///< 曲线
        ChartScatter,    ///< 散点
        UnknowChartType  ///< 未知
    };

public:
    explicit DADialogDataframePlot(QWidget* parent = nullptr);
    ~DADialogDataframePlot();
    //设置datamanager,会把combox填入所有的dataframe
    void setDataManager(DADataManager* dmgr);
    //设置datafram
    void setCurrentData(const DAData& d);
    DAData getCurrentData() const;
    //获取当前的绘图类型
    ChartType getCurrentChartType() const;

public:
    //获取为vector pointf 针对ChartCurve和ChartScatter
    bool getToVectorPointF(QVector< QPointF >& res);

protected:
    void updateData();
    //刷新dataframe combobox
    void resetDataframeCombobox();
    //更新combobox的选则状态
    void updateDataframeComboboxSelect();
    //刷新x，y两个列选择listwidget
    void updateCurrentPageData();
private slots:
    void onComboBoxCurrentIndexChanged(int i);
    //选择绘图类型改变
    void onListWidgetItemChanged(QListWidgetItem* item);

private:
    void init();

private:
    Ui::DADialogDataframePlot* ui;
    DADataManager* _dataMgr;
    DAData _currentData;
};
}  // end DA
#endif  // DADIALOGDATAFRAMEPLOT_H
