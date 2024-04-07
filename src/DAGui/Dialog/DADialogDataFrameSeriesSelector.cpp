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
            QOverload<int>::of(&QComboBox::activated),
            this,
            &DADialogDataFrameSeriesSelector::onCurrentDataframeComboboxActivated);
    connect(ui->listWidgetColumns,
            &DAPyDataframeColumnsListWidget::itemSelectionChanged,
            this,
            &DADialogDataFrameSeriesSelector::onDataframeColumnsListWidgetItemSelectionChanged);
}

DADialogDataFrameSeriesSelector::~DADialogDataFrameSeriesSelector()
{
    delete ui;
}

/**
 * @brief 获取选择的dataframe.
 *  
 * @return std::pair<dataframe,选中的series索引>
 */
std::pair<DAPyDataFrame, QList<int>> DA::DADialogDataFrameSeriesSelector::getCurrentDataFrameInfos() const
{
    std::pair<DAPyDataFrame, QList<int>> res;
    res.first = getCurrentDataFrame();
    res.second = ui->listWidgetColumns->getAllSelectedSeriesIndexs();
    return res;
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

void DADialogDataFrameSeriesSelector::onCurrentDataframeComboboxActivated(int i)
{
    // dataframe切换
    DAData data = ui->comboBoxDatas->getCurrentDAData();
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

void DADialogDataFrameSeriesSelector::onDataframeColumnsListWidgetItemSelectionChanged()
{
    //这时不能用getAllSelectedSeries，返回的不是选中的
    auto series = ui->listWidgetColumns->getAllSelectedSeries();
    d_ptr->mModule->setSeries(series);

    qDebug() << "set series:" << series.size() << " name:" << series.back().name();
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
