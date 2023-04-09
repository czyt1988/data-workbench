#include "DADialogDataframeToPointVector.h"
#include "ui_DADialogDataframeToPointVector.h"
#include "DADataManager.h"
namespace DA
{

DADialogDataframeToPointVector::DADialogDataframeToPointVector(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataframeToPointVector), _dataMgr(nullptr)
{
    ui->setupUi(this);
}

DADialogDataframeToPointVector::~DADialogDataframeToPointVector()
{
    delete ui;
}

void DADialogDataframeToPointVector::setDataManager(DADataManager* dmgr)
{
    _dataMgr = dmgr;
}

void DADialogDataframeToPointVector::setCurrentData(const DAData& d)
{
    _currentData = d;
    updateData();
}

DAData DADialogDataframeToPointVector::getCurrentData() const
{
    return _currentData;
}

void DADialogDataframeToPointVector::updateData()
{
    updateDataframeCombobox();
    updateDataframeColumnList();
}

/**
 * @brief 刷新dataframe combobox
 */
void DADialogDataframeToPointVector::updateDataframeCombobox()
{
    if (nullptr == _dataMgr) {
        return;
    }
    ui->comboBoxDataFrame->clear();
    int c = _dataMgr->getDataCount();
    for (int i = 0; i < c; ++i) {
        DAData d = _dataMgr->getData(i);
        if (d.isNull() || !d.isDataFrame()) {
            continue;
        }
        DAPyDataFrame df = d.toDataFrame();
        if (df.isNone()) {
            continue;
        }
        // id作为data
        ui->comboBoxDataFrame->addItem(d.getName(), d.id());
    }
}

/**
 * @brief 刷新x，y两个列选择listwidget
 */
void DADialogDataframeToPointVector::updateDataframeColumnList()
{
    ui->listWidgetX->clear();
    ui->listWidgetY->clear();
    if (_currentData.isNull() || !_currentData.isDataFrame()) {
        return;
    }
    DAPyDataFrame df = _currentData.toDataFrame();
    if (df.isNone()) {
        return;
    }
    ui->listWidgetX->setDataframe(df);
    ui->listWidgetY->setDataframe(df);
}

}
