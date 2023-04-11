#include "DADialogDataframePlot.h"
#include "ui_DADialogDataframePlot.h"
#include "DADataManager.h"
#include <iterator>
#include <vector>
namespace DA
{

DADialogDataframePlot::DADialogDataframePlot(QWidget* parent)
    : QDialog(parent), ui(new Ui::DADialogDataframePlot), _dataMgr(nullptr)
{
    ui->setupUi(this);
    connect(ui->comboBoxDataFrame, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogDataframePlot::onComboBoxCurrentIndexChanged);
}

DADialogDataframePlot::~DADialogDataframePlot()
{
    delete ui;
}

/**
 * @brief 设置datamanager,会把combox填入所有的dataframe
 * @param dmgr
 */
void DADialogDataframePlot::setDataManager(DADataManager* dmgr)
{
    _dataMgr = dmgr;
    resetDataframeCombobox();
    updateDataframeColumnList();
}

/**
 * @brief 设置当前的数据
 * @param d
 */
void DADialogDataframePlot::setCurrentData(const DAData& d)
{
    _currentData = d;
    updateData();
    updateDataframeColumnList();
}

DAData DADialogDataframePlot::getCurrentData() const
{
    return _currentData;
}

/**
 * @brief 获取为vector pointf
 * @return 成功返回true
 */
bool DADialogDataframePlot::getToVectorPointF(QVector< QPointF >& res)
{
    if (!_currentData.isDataFrame() || _currentData.isNull()) {
        return false;
    }
    return ui->pageCurve->getToVectorPointF(res);
}

void DADialogDataframePlot::updateData()
{
    updateDataframeComboboxSelect();
    updateDataframeColumnList();
}

/**
 * @brief 刷新dataframe combobox
 */
void DADialogDataframePlot::resetDataframeCombobox()
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
void DADialogDataframePlot::updateDataframeComboboxSelect()
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
void DADialogDataframePlot::updateDataframeColumnList()
{
    ui->pageCurve->updateDataframeColumnList();
}

void DADialogDataframePlot::onComboBoxCurrentIndexChanged(int i)
{
    if (nullptr == _dataMgr || i < 0) {
        return;
    }
    DAData d = _dataMgr->getData(i);
    setCurrentData(d);
    updateDataframeColumnList();
}

}
