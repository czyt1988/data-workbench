#ifndef DADIALOGDATAFRAMETOPOINTVECTOR_H
#define DADIALOGDATAFRAMETOPOINTVECTOR_H

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
 */
class DADialogDataframeToPointVector : public QDialog
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
    //

protected:
    void updateData();
    //刷新dataframe combobox
    void updateDataframeCombobox();
    //刷新x，y两个列选择listwidget
    void updateDataframeColumnList();

private:
    Ui::DADialogDataframeToPointVector* ui;
    DADataManager* _dataMgr;
    DAData _currentData;
};
}  // end DA
#endif  // DADIALOGDATAFRAMETOPOINTVECTOR_H
