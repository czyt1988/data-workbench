#include "DADialogDataframeToPointVector.h"
#include "ui_DADialogDataframeToPointVector.h"
#include "DADataManager.h"
#include <vector>
namespace DA
{

DADialogDataframeToPointVector::DADialogDataframeToPointVector(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataframeToPointVector), _dataMgr(nullptr)
{
    ui->setupUi(this);
    connect(ui->comboBoxDataFrame, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogDataframeToPointVector::onComboBoxCurrentIndexChanged);
}

DADialogDataframeToPointVector::~DADialogDataframeToPointVector()
{
    delete ui;
}

/**
 * @brief 设置datamanager,会把combox填入所有的dataframe
 * @param dmgr
 */
void DADialogDataframeToPointVector::setDataManager(DADataManager* dmgr)
{
    _dataMgr = dmgr;
    resetDataframeCombobox();
    updateDataframeColumnList();
}

/**
 * @brief 设置当前的数据
 * @param d
 */
void DADialogDataframeToPointVector::setCurrentData(const DAData& d)
{
    _currentData = d;
    updateData();
    updateDataframeColumnList();
}

DAData DADialogDataframeToPointVector::getCurrentData() const
{
    return _currentData;
}

/**
 * @brief 获取为vector pointf
 * @return 成功返回true
 */
bool DADialogDataframeToPointVector::getToVectorPointF(QVector< QPointF >& res)
{
    if (!_currentData.isDataFrame() || _currentData.isNull()) {
        return false;
    }
    DAPySeries x = ui->listWidgetX->getSelectedSeries();
    DAPySeries y = ui->listWidgetY->getSelectedSeries();
    if (x.isNone() || y.isNone()) {
        return false;
    }
    std::size_t s = std::min(x.size(), y.size());
    if (0 == s) {
        return true;
    }
    try {
        std::vector< double > vx, vy;
        vx.reserve(x.size());
        vy.reserve(y.size());
        x.castTo< double >(vx.begin());
        y.castTo< double >(vy.begin());
        res.resize(s);
        for (int i = 0; i < s; ++i) {
            res[ i ].setX(vx[ i ]);
            res[ i ].setY(vy[ i ]);
        }
    } catch (const std::exception& e) {
        qCritical() << tr("Exception occurred during extracting from pandas.Series to double vector");  // cn:从pandas.Series提取为double vector过程中出现异常
        return false;
    }
    return true;
}

void DADialogDataframeToPointVector::updateData()
{
    updateDataframeComboboxSelect();
    updateDataframeColumnList();
}

/**
 * @brief 刷新dataframe combobox
 */
void DADialogDataframeToPointVector::resetDataframeCombobox()
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
    ui->comboBoxDataFrame->setCurrentIndex(-1);  //不选中
}

/**
 * @brief 更新combobox的选中状态，但不会触发currentIndexChanged信号
 */
void DADialogDataframeToPointVector::updateDataframeComboboxSelect()
{
    if (nullptr == _dataMgr) {
        return;
    }
    int index = _dataMgr->getDataIndex(_currentData);
    if (index < 0) {
        return;
    }
    QSignalBlocker b(ui->comboBoxDataFrame);
    Q_UNUSED(b);
    ui->comboBoxDataFrame->setCurrentIndex(index);
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

void DADialogDataframeToPointVector::onComboBoxCurrentIndexChanged(int i)
{
    if (nullptr == _dataMgr) {
        return;
    }
    DAData d = _dataMgr->getData(i);
    setCurrentData(d);
    updateDataframeColumnList();
}

}
