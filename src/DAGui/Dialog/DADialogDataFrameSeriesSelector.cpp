#include "DADialogDataFrameSeriesSelector.h"
#include "ui_DADialogDataFrameSeriesSelector.h"
#include "Models/DAPySeriesTableModule.h"
namespace DA
{
class DADialogDataFrameSeriesSelector::PrivateData
{
    DA_DECLARE_PUBLIC(DADialogDataFrameSeriesSelector)
public:
    PrivateData(DADialogDataFrameSeriesSelector* p);

public:
    DAPyDataFrame mDataframe;
    DAPySeriesTableModule* mModule { nullptr };
};

DADialogDataFrameSeriesSelector::PrivateData::PrivateData(DADialogDataFrameSeriesSelector* p) : q_ptr(p)
{
    mModule = new DAPySeriesTableModule(p);
}

//===============================================================
// DADialogDataFrameSeriesSelector
//===============================================================

DADialogDataFrameSeriesSelector::DADialogDataFrameSeriesSelector(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataFrameSeriesSelector), DA_PIMPL_CONSTRUCT
{
    ui->setupUi(this);
    ui->tableView->setModel(d_ptr->mModule);
    ui->comboBoxDatas->setShowSeriesUnderDataframe(false);
    connect(ui->comboBoxDatas,
            &DADataManagerComboBox::currentDataframeSeriesChanged,
            this,
            &DADialogDataFrameSeriesSelector::onCurrentDataframeSeriesChanged);
    connect(ui->listWidgetColumns,
            &DAPyDataframeColumnsListWidget::currentRowChanged,
            this,
            &DADialogDataFrameSeriesSelector::onDataframeColumnsListWidgetRowChanged);
}

DADialogDataFrameSeriesSelector::~DADialogDataFrameSeriesSelector()
{
    delete ui;
}

void DADialogDataFrameSeriesSelector::setCurrentDataFrame(const DAPyDataFrame& df)
{
    d_ptr->mDataframe = df;
    updateUI();
}

DAPyDataFrame DADialogDataFrameSeriesSelector::getCurrentDataFrame() const
{
    return d_ptr->mDataframe;
}

void DADialogDataFrameSeriesSelector::setDataManager(DADataManager* mgr)
{
    ui->comboBoxDatas->setDataManager(mgr);
}

void DADialogDataFrameSeriesSelector::updateUI()
{
    ui->listWidgetColumns->setDataframe(d_ptr->mDataframe);
}

void DADialogDataFrameSeriesSelector::onCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
{
    // dataframe切换
    if (!data.isDataFrame()) {
        return;
    }
    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        return;
    }
    d_ptr->mDataframe = df;
    updateUI();
}

void DADialogDataFrameSeriesSelector::onDataframeColumnsListWidgetRowChanged(int i)
{
    auto series = ui->listWidgetColumns->getAllSelectedSeries();
    d_ptr->mModule->setSeries(series);
}

void DADialogDataFrameSeriesSelector::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

}
