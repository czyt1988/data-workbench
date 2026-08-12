#include "DAChartAdd3DBarWidget.h"
#include "ui_DAChartAdd3DBarWidget.h"
#include "DADataManager.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
// qwt3d
#include "qwt3d_bar.h"
#include "qwt3d_types.h"
// DAPyBindQt — 涉及 QString 传给 Python 时必须 include
#include "DAPybind11QtCaster.hpp"

namespace DA
{

DAChartAdd3DBarWidget::DAChartAdd3DBarWidget(QWidget* parent)
    : DAAbstractChart3DAddItemWidget(parent), ui(new Ui::DAChartAdd3DBarWidget)
{
    ui->setupUi(this);
    ui->comboBoxMode->setCurrentIndex(Series1D);
    ui->widgetGridConfig->setVisible(false);
    ui->widgetSeriesConfig->setVisible(true);
    connect(ui->comboBoxMode, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DBarWidget::onComboBoxModeCurrentIndexChanged);
    connect(ui->comboBoxDataframe, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DBarWidget::onComboBoxDataframeCurrentIndexChanged);
    connect(ui->comboBoxSeries, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DBarWidget::onComboBoxSeriesCurrentIndexChanged);
}

DAChartAdd3DBarWidget::~DAChartAdd3DBarWidget()
{
    delete ui;
}

/**
 * @brief 设置datamanager
 */
void DAChartAdd3DBarWidget::setDataManager(DADataManager* dmgr)
{
    mDataMgr = dmgr;
    DAAbstractChart3DAddItemWidget::setDataManager(dmgr);
    refreshDataframeCombo();
}

/**
 * @brief 创建3D柱状图item
 */
Qwt3DPlotItem* DAChartAdd3DBarWidget::create3DPlotItem()
{
    if (getDataMode() == Series1D) {
        QVector< QwtPoint3D > points;
        if (!extractSeriesData(points)) {
            return nullptr;
        }
        Qwt3DBar* item = new Qwt3DBar();
        item->setSamples(points);
        item->setTitle(getNameHint());
        return item;
    } else {
        QVector< double > xCoords;
        QVector< double > yCoords;
        QVector< QVector< double > > zMatrix;
        if (!extractGridData(xCoords, yCoords, zMatrix)) {
            return nullptr;
        }
        int rows = zMatrix.size();
        int cols = (rows > 0) ? zMatrix[ 0 ].size() : 0;
        if (rows < 1 || cols < 1) {
            return nullptr;
        }
        // Qwt3DBar::setSamples 接受 double**（C 风格二维数组）
        double** dataPtr = new double*[ static_cast< std::size_t >(rows) ];
        for (int r = 0; r < rows; ++r) {
            dataPtr[ r ] = new double[ static_cast< std::size_t >(cols) ];
            for (int c = 0; c < cols; ++c) {
                dataPtr[ r ][ c ] = zMatrix[ r ][ c ];
            }
        }
        Qwt3DBar* item = new Qwt3DBar();
        // setSamples(z, columns, rows, minX, maxX, minY, maxY)
        item->setSamples(dataPtr,
                        cols,
                        rows,
                        xCoords.first(),
                        xCoords.last(),
                        yCoords.first(),
                        yCoords.last());
        for (int r = 0; r < rows; ++r) {
            delete[] dataPtr[ r ];
        }
        delete[] dataPtr;
        item->setTitle(getNameHint());
        return item;
    }
}

/**
 * @brief 获取当前数据模式
 */
DAChartAdd3DBarWidget::BarDataMode DAChartAdd3DBarWidget::getDataMode() const
{
    return static_cast< BarDataMode >(ui->comboBoxMode->currentIndex());
}

/**
 * @brief 获取推荐名称
 */
QString DAChartAdd3DBarWidget::getNameHint() const
{
    return tr("Bar3D");  // cn:3D柱状
}

void DAChartAdd3DBarWidget::onComboBoxModeCurrentIndexChanged(int index)
{
    bool isGrid = (index == Grid2D);
    ui->widgetGridConfig->setVisible(isGrid);
    ui->widgetSeriesConfig->setVisible(!isGrid);
}

void DAChartAdd3DBarWidget::onComboBoxDataframeCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
    refreshSeriesCombo();
}

void DAChartAdd3DBarWidget::onComboBoxSeriesCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
}

/**
 * @brief 从单列提取1D series数据
 * x坐标自增，y坐标固定为0，z为列值
 */
bool DAChartAdd3DBarWidget::extractSeriesData(QVector< QwtPoint3D >& points) const
{
    DAData dfData = getCurrentDataframe();
    if (!dfData.isDataFrame()) {
        return false;
    }
    DAPyDataFrame df = dfData.toDataFrame();
    QString colName = ui->comboBoxSeries->currentText();
    if (colName.isEmpty()) {
        return false;
    }
    // df[colName] 传 QString 给 Python，需 DAPybind11QtCaster.hpp
    DAPySeries series = df[ colName ];
    int n = static_cast< int >(series.size());
    if (n < 1) {
        return false;
    }
    points.resize(n);
    for (int i = 0; i < n; ++i) {
        // DAPySeries::operator[] 返回 pybind11::object，需用 value(i) 获取 QVariant
        points[ i ] = QwtPoint3D(static_cast< double >(i), 0.0, series.value(static_cast< std::size_t >(i)).toDouble());
    }
    return true;
}

/**
 * @brief 从DataFrame提取2D grid数据
 * 与DAChartAdd3DSurfaceWidget::extractGridData实现一致
 */
bool DAChartAdd3DBarWidget::extractGridData(QVector< double >& xCoords,
                                             QVector< double >& yCoords,
                                             QVector< QVector< double > >& zMatrix) const
{
    DAData dfData = getCurrentDataframe();
    if (!dfData.isDataFrame()) {
        return false;
    }
    DAPyDataFrame df = dfData.toDataFrame();
    auto shape = df.shape();
    int rows = static_cast< int >(shape.first);
    int cols = static_cast< int >(shape.second);
    if (rows < 1 || cols < 1) {
        return false;
    }
    xCoords.resize(cols);
    for (int c = 0; c < cols; ++c) {
        xCoords[ c ] = static_cast< double >(c);
    }
    yCoords.resize(rows);
    for (int r = 0; r < rows; ++r) {
        yCoords[ r ] = static_cast< double >(r);
    }
    zMatrix.resize(rows);
    for (int r = 0; r < rows; ++r) {
        zMatrix[ r ].resize(cols);
        for (int c = 0; c < cols; ++c) {
            zMatrix[ r ][ c ] = df.iat(static_cast< std::size_t >(r), static_cast< std::size_t >(c)).toDouble();
        }
    }
    return true;
}

/**
 * @brief 刷新DataFrame下拉框
 */
void DAChartAdd3DBarWidget::refreshDataframeCombo()
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
    refreshSeriesCombo();
}

/**
 * @brief 刷新列下拉框（1D series模式）
 */
void DAChartAdd3DBarWidget::refreshSeriesCombo()
{
    DAData dfData = getCurrentDataframe();
    if (!dfData.isDataFrame()) {
        return;
    }
    DAPyDataFrame df = dfData.toDataFrame();
    QStringList cols = df.columns();
    ui->comboBoxSeries->clear();
    ui->comboBoxSeries->addItems(cols);
    if (cols.size() >= 1) {
        ui->comboBoxSeries->setCurrentIndex(0);
    }
}

/**
 * @brief 获取当前选中的DataFrame
 */
DAData DAChartAdd3DBarWidget::getCurrentDataframe() const
{
    int idx = ui->comboBoxDataframe->currentIndex();
    if (idx < 0) {
        return DAData();
    }
    return ui->comboBoxDataframe->itemData(idx).value< DAData >();
}

}  // end DA
