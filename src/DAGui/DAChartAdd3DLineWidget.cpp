#include "DAChartAdd3DLineWidget.h"
#include "ui_DAChartAdd3DLineWidget.h"
#include "DADataManager.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
// qwt3d
#include "qwt3d_line3d.h"
#include "qwt3d_types.h"
// DAPyBindQt — 涉及 QString 传给 Python 时必须 include
#include "DAPybind11QtCaster.hpp"

namespace DA
{

DAChartAdd3DLineWidget::DAChartAdd3DLineWidget(QWidget* parent)
    : DAAbstractChart3DAddItemWidget(parent), ui(new Ui::DAChartAdd3DLineWidget)
{
    ui->setupUi(this);
    connect(ui->comboBoxDataframe, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DLineWidget::onComboBoxDataframeCurrentIndexChanged);
}

DAChartAdd3DLineWidget::~DAChartAdd3DLineWidget()
{
    delete ui;
}

/**
 * @brief 设置datamanager
 */
void DAChartAdd3DLineWidget::setDataManager(DADataManager* dmgr)
{
    mDataMgr = dmgr;
    DAAbstractChart3DAddItemWidget::setDataManager(dmgr);
    refreshDataframeCombo();
}

/**
 * @brief 创建3D线图item
 */
Qwt3DPlotItem* DAChartAdd3DLineWidget::create3DPlotItem()
{
    QVector< QwtPoint3D > points;
    if (!extractLineData(points)) {
        return nullptr;
    }
    Qwt3DLine* item = new Qwt3DLine();
    item->setSamples(points);
    item->setTitle(getNameHint());
    return item;
}

/**
 * @brief 获取推荐名称
 */
QString DAChartAdd3DLineWidget::getNameHint() const
{
    return tr("Line3D");  // cn:3D线图
}

void DAChartAdd3DLineWidget::onComboBoxDataframeCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
    refreshColumnCombos();
}

/**
 * @brief 从三列提取3D轨迹数据
 */
bool DAChartAdd3DLineWidget::extractLineData(QVector< QwtPoint3D >& points) const
{
    DAData dfData = getCurrentDataframe();
    if (!dfData.isDataFrame()) {
        return false;
    }
    DAPyDataFrame df = dfData.toDataFrame();
    QString xCol = ui->comboBoxX->currentText();
    QString yCol = ui->comboBoxY->currentText();
    QString zCol = ui->comboBoxZ->currentText();
    if (xCol.isEmpty() || yCol.isEmpty() || zCol.isEmpty()) {
        return false;
    }
    // df[xCol] 传 QString 给 Python，需 DAPybind11QtCaster.hpp
    DAPySeries xs = df[ xCol ];
    DAPySeries ys = df[ yCol ];
    DAPySeries zs = df[ zCol ];
    int n = static_cast< int >(xs.size());
    if (n < 2) {
        return false;
    }
    points.resize(n);
    for (int i = 0; i < n; ++i) {
        // DAPySeries::operator[] 返回 pybind11::object，需用 value(i) 获取 QVariant
        points[ i ] = QwtPoint3D(xs.value(static_cast< std::size_t >(i)).toDouble(),
                                 ys.value(static_cast< std::size_t >(i)).toDouble(),
                                 zs.value(static_cast< std::size_t >(i)).toDouble());
    }
    return true;
}

/**
 * @brief 刷新DataFrame下拉框
 */
void DAChartAdd3DLineWidget::refreshDataframeCombo()
{
    ui->comboBoxDataframe->clear();
    if (mDataMgr == nullptr) {
        return;
    }
    const QList< DAData > datas = mDataMgr->getAllDatas();
    for (const DAData& d : std::as_const(datas)) {
        if (d.isDataFrame()) {
            ui->comboBoxDataframe->addItem(d.getName(), QVariant::fromValue(d));
        }
    }
    refreshColumnCombos();
}

/**
 * @brief 刷新列下拉框
 */
void DAChartAdd3DLineWidget::refreshColumnCombos()
{
    DAData dfData = getCurrentDataframe();
    if (!dfData.isDataFrame()) {
        return;
    }
    DAPyDataFrame df = dfData.toDataFrame();
    QStringList cols = df.columns();
    ui->comboBoxX->clear();
    ui->comboBoxY->clear();
    ui->comboBoxZ->clear();
    ui->comboBoxX->addItems(cols);
    ui->comboBoxY->addItems(cols);
    ui->comboBoxZ->addItems(cols);
    if (cols.size() >= 3) {
        ui->comboBoxX->setCurrentIndex(0);
        ui->comboBoxY->setCurrentIndex(1);
        ui->comboBoxZ->setCurrentIndex(2);
    }
}

/**
 * @brief 获取当前选中的DataFrame
 */
DAData DAChartAdd3DLineWidget::getCurrentDataframe() const
{
    int idx = ui->comboBoxDataframe->currentIndex();
    if (idx < 0) {
        return DAData();
    }
    return ui->comboBoxDataframe->itemData(idx).value< DAData >();
}

}  // end DA
