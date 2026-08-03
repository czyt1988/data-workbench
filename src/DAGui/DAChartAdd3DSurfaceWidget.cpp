#include "DAChartAdd3DSurfaceWidget.h"
#include "ui_DAChartAdd3DSurfaceWidget.h"
#include "DADataManager.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
// qwt3d
#include "qwt3d_surface.h"
// DAPyBindQt — 涉及 QString 传给 Python 时必须 include
#include "DAPybind11QtCaster.hpp"
// daWarning 用户消息
#include "DALogCategory.h"
// std::min_element / std::max_element
#include <algorithm>
// std::isnan
#include <cmath>
#include <vector>

namespace DA
{

DAChartAdd3DSurfaceWidget::DAChartAdd3DSurfaceWidget(QWidget* parent)
    : DAAbstractChart3DAddItemWidget(parent), ui(new Ui::DAChartAdd3DSurfaceWidget)
{
    ui->setupUi(this);
    // 默认规则网格模式
    ui->comboBoxMode->setCurrentIndex(GridMode);
    // 散点模式的列选择默认隐藏
    ui->widgetScatterConfig->setVisible(false);
    ui->widgetGridConfig->setVisible(true);
    connect(ui->comboBoxMode, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DSurfaceWidget::onComboBoxModeCurrentIndexChanged);
    connect(ui->comboBoxDataframe, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DSurfaceWidget::onComboBoxDataframeCurrentIndexChanged);
    connect(ui->comboBoxX, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DSurfaceWidget::onComboBoxXCurrentIndexChanged);
    connect(ui->comboBoxY, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DSurfaceWidget::onComboBoxYCurrentIndexChanged);
    connect(ui->comboBoxZ, QOverload< int >::of(&QComboBox::currentIndexChanged),
            this, &DAChartAdd3DSurfaceWidget::onComboBoxZCurrentIndexChanged);
}

DAChartAdd3DSurfaceWidget::~DAChartAdd3DSurfaceWidget()
{
    delete ui;
}

/**
 * @brief 设置datamanager，刷新DataFrame下拉框
 * @param dmgr
 */
void DAChartAdd3DSurfaceWidget::setDataManager(DADataManager* dmgr)
{
    mDataMgr = dmgr;
    DAAbstractChart3DAddItemWidget::setDataManager(dmgr);
    refreshDataframeCombo();
}

/**
 * @brief 创建3D曲面item
 * @return 如果数据无效返回nullptr
 */
Qwt3DPlotItem* DAChartAdd3DSurfaceWidget::create3DPlotItem()
{
    QVector< double > xCoords;
    QVector< double > yCoords;
    QVector< QVector< double > > zMatrix;
    if (getDataMode() == GridMode) {
        if (!extractGridData(xCoords, yCoords, zMatrix)) {
            return nullptr;
        }
    } else {
        if (!extractScatterDataAndInterpolate(xCoords, yCoords, zMatrix)) {
            return nullptr;
        }
    }
    int rows = zMatrix.size();
    int cols = (rows > 0) ? zMatrix[ 0 ].size() : 0;
    if (rows < 2 || cols < 2) {
        return nullptr;
    }
    // Qwt3DSurface::loadFromData 接受 double**（C 风格二维数组），需要从
    // QVector<QVector<double>> 转换为 double** 临时数组
    double** dataPtr = new double*[ static_cast< std::size_t >(rows) ];
    for (int r = 0; r < rows; ++r) {
        dataPtr[ r ] = new double[ static_cast< std::size_t >(cols) ];
        for (int c = 0; c < cols; ++c) {
            dataPtr[ r ][ c ] = zMatrix[ r ][ c ];
        }
    }
    Qwt3DSurface* item = new Qwt3DSurface();
    // loadFromData(data, columns, rows, minx, maxx, miny, maxy)
    // columns 对应 x 方向（xCoords），rows 对应 y 方向（yCoords）
    item->loadFromData(dataPtr,
                       static_cast< unsigned int >(cols),
                       static_cast< unsigned int >(rows),
                       xCoords.first(),
                       xCoords.last(),
                       yCoords.first(),
                       yCoords.last());
    // Qwt3DSurface 内部拷贝数据，释放临时数组
    for (int r = 0; r < rows; ++r) {
        delete[] dataPtr[ r ];
    }
    delete[] dataPtr;
    item->setTitle(getNameHint());
    return item;
}

/**
 * @brief 获取当前数据模式
 */
DAChartAdd3DSurfaceWidget::SurfaceDataMode DAChartAdd3DSurfaceWidget::getDataMode() const
{
    return static_cast< SurfaceDataMode >(ui->comboBoxMode->currentIndex());
}

/**
 * @brief 获取推荐名称
 */
QString DAChartAdd3DSurfaceWidget::getNameHint() const
{
    return tr("Surface3D");  // cn:3D曲面
}

/**
 * @brief 模式切换槽函数
 */
void DAChartAdd3DSurfaceWidget::onComboBoxModeCurrentIndexChanged(int index)
{
    bool isScatter = (index == ScatterMode);
    ui->widgetScatterConfig->setVisible(isScatter);
    ui->widgetGridConfig->setVisible(!isScatter);
}

/**
 * @brief DataFrame下拉框改变
 */
void DAChartAdd3DSurfaceWidget::onComboBoxDataframeCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
    refreshColumnCombos();
}

void DAChartAdd3DSurfaceWidget::onComboBoxXCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
}

void DAChartAdd3DSurfaceWidget::onComboBoxYCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
}

void DAChartAdd3DSurfaceWidget::onComboBoxZCurrentIndexChanged(int index)
{
    Q_UNUSED(index)
}

/**
 * @brief 从DataFrame提取二维矩阵数据（规则网格模式）
 * 行列索引作为x/y坐标，值作为z高度
 */
bool DAChartAdd3DSurfaceWidget::extractGridData(QVector< double >& xCoords,
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
    if (rows < 2 || cols < 2) {
        return false;
    }
    // x坐标为列索引（0, 1, ..., cols-1），y坐标为行索引（0, 1, ..., rows-1）
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
 * @brief 从三列散点数据插值为规则网格
 * 调用scipy.interpolate.griddata进行插值
 */
bool DAChartAdd3DSurfaceWidget::extractScatterDataAndInterpolate(
    QVector< double >& xCoords,
    QVector< double >& yCoords,
    QVector< QVector< double > >& zMatrix) const
{
    // 获取选中的三列数据
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
    // 提取三列数据（df[xCol] 传 QString 给 Python，需 DAPybind11QtCaster.hpp）
    DAPySeries xs = df[ xCol ];
    DAPySeries ys = df[ yCol ];
    DAPySeries zs = df[ zCol ];
    int n = static_cast< int >(xs.size());
    if (n < 3) {
        return false;
    }
    // DAPySeries 无 min()/max() 成员，用 toQVectorDouble 转换后 std::min/max_element
    QVector< double > xVec = toQVectorDouble(xs);
    QVector< double > yVec = toQVectorDouble(ys);
    double xMin = *std::min_element(xVec.begin(), xVec.end());
    double xMax = *std::max_element(xVec.begin(), xVec.end());
    double yMin = *std::min_element(yVec.begin(), yVec.end());
    double yMax = *std::max_element(yVec.begin(), yVec.end());
    int gridSize = 30;  // 默认30x30网格
    xCoords.resize(gridSize);
    yCoords.resize(gridSize);
    double xStep = (xMax - xMin) / (gridSize - 1);
    double yStep = (yMax - yMin) / (gridSize - 1);
    for (int i = 0; i < gridSize; ++i) {
        xCoords[ i ] = xMin + i * xStep;
        yCoords[ i ] = yMin + i * yStep;
    }
    // 检查 scipy 是否可用，避免静默失败
    try {
        pybind11::module_::import("scipy");
    } catch (const pybind11::error_already_set&) {
        daWarning << tr("scipy is not available, cannot perform scatter interpolation. "  // cn:scipy不可用，无法进行散点插值
                           "Please install scipy: pip install scipy");
        return false;
    }
    // 使用Python调用scipy.interpolate.griddata
    // 注意必须包含DAPybind11QtCaster.hpp
    pybind11::module_ scipyInterp = pybind11::module_::import("scipy.interpolate");
    pybind11::object griddata = scipyInterp.attr("griddata");
    // 构建points数组 (n, 2)
    std::vector< pybind11::ssize_t > ptShape = { static_cast< pybind11::ssize_t >(n),
                                                  static_cast< pybind11::ssize_t >(2) };
    pybind11::array_t< double > points(ptShape);
    auto pointsBuf = points.mutable_unchecked< 2 >();
    for (int i = 0; i < n; ++i) {
        pointsBuf(i, 0) = xVec[ i ];
        pointsBuf(i, 1) = yVec[ i ];
    }
    // 构建values数组 (n,)
    pybind11::array_t< double > values(static_cast< pybind11::ssize_t >(n));
    auto valuesBuf = values.mutable_unchecked< 1 >();
    for (int i = 0; i < n; ++i) {
        valuesBuf(i) = zs.value(static_cast< std::size_t >(i)).toDouble();
    }
    // 构建网格坐标 (gridSize*gridSize, 2)
    std::vector< pybind11::ssize_t > gridShape = { static_cast< pybind11::ssize_t >(gridSize * gridSize),
                                                    static_cast< pybind11::ssize_t >(2) };
    pybind11::array_t< double > grid(gridShape);
    auto gridBuf = grid.mutable_unchecked< 2 >();
    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            gridBuf(i * gridSize + j, 0) = xCoords[ i ];
            gridBuf(i * gridSize + j, 1) = yCoords[ j ];
        }
    }
    // 调用griddata
    pybind11::object resultObj = griddata(points, values, grid, "cubic");
    pybind11::array_t< double > result = resultObj.cast< pybind11::array_t< double > >();
    auto resultBuf = result.unchecked< 1 >();
    // 填充zMatrix (rows=gridSize, cols=gridSize)
    // griddata 对凸包外的点返回 NaN，需替换为 0
    zMatrix.resize(gridSize);
    for (int i = 0; i < gridSize; ++i) {
        zMatrix[ i ].resize(gridSize);
        for (int j = 0; j < gridSize; ++j) {
            double v = resultBuf(i * gridSize + j);
            if (std::isnan(v)) {
                v = 0.0;  // 凸包外的点用0填充
            }
            zMatrix[ i ][ j ] = v;
        }
    }
    return true;
}

/**
 * @brief 刷新DataFrame下拉框
 */
void DAChartAdd3DSurfaceWidget::refreshDataframeCombo()
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
 * @brief 刷新列下拉框（散点模式）
 */
void DAChartAdd3DSurfaceWidget::refreshColumnCombos()
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
    // 默认选前三列
    if (cols.size() >= 3) {
        ui->comboBoxX->setCurrentIndex(0);
        ui->comboBoxY->setCurrentIndex(1);
        ui->comboBoxZ->setCurrentIndex(2);
    }
}

/**
 * @brief 获取当前选中的DataFrame
 */
DAData DAChartAdd3DSurfaceWidget::getCurrentDataframe() const
{
    int idx = ui->comboBoxDataframe->currentIndex();
    if (idx < 0) {
        return DAData();
    }
    return ui->comboBoxDataframe->itemData(idx).value< DAData >();
}

}  // end DA
