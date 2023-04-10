#ifndef DADIALOGDATAFRAMETOPOINTVECTOR_H
#define DADIALOGDATAFRAMETOPOINTVECTOR_H
#include "DAGuiAPI.h"
#include <QDialog>
#include "DAData.h"
namespace Ui
{
class DADialogDataframeToPointVector;
}

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
class DAGUI_API DADialogDataframeToPointVector : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogDataframeToPointVector(QWidget* parent = nullptr);
    ~DADialogDataframeToPointVector();
    //设置datamanager,会把combox填入所有的dataframe
    void setDataManager(DADataManager* dmgr);
    //设置datafram
    void setCurrentData(const DAData& d);
    DAData getCurrentData() const;
    //获取为vector pointf
    bool getToVectorPointF(QVector< QPointF >& res);

protected:
    void updateData();
    //刷新dataframe combobox
    void resetDataframeCombobox();
    //更新combobox的选则状态
    void updateDataframeComboboxSelect();
    //刷新x，y两个列选择listwidget
    void updateDataframeColumnList();
private slots:
    void onComboBoxCurrentIndexChanged(int i);

private:
    Ui::DADialogDataframeToPointVector* ui;
    DADataManager* _dataMgr;
    DAData _currentData;
};
}  // end DA
#endif  // DADIALOGDATAFRAMETOPOINTVECTOR_H
