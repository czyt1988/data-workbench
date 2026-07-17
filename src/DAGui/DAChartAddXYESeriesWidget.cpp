#include "DAChartAddXYESeriesWidget.h"
#include "ui_DAChartAddXYESeriesWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "DALogCategory.h"
#include "DAChartSeriesSelectWidget.h"
#if DA_ENABLE_PYTHON
#include "Models/DAPySeriesTableModel.h"
#endif
namespace DA
{

//===================================================
// DAChartAddXYESeriesWidget
//===================================================

DAChartAddXYESeriesWidget::DAChartAddXYESeriesWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddXYESeriesWidget)
{
    ui->setupUi(this);
#if DA_ENABLE_PYTHON
    DAPySeriesTableModel* m = new DAPySeriesTableModel(this);
    m->setHeaderLabel({ tr("x"),      // cn:x
                        tr("y"),      // cn:y
                        tr("error")   // cn:误差
                      });
    ui->tableViewXYE->setModel(m);
#endif
    QFontMetrics fm = fontMetrics();
    ui->tableViewXYE->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    ui->selectWidgetX->setRoleLabel(tr("X"));  // cn:X
    ui->selectWidgetY->setRoleLabel(tr("Y"));  // cn:Y
    ui->selectWidgetYE->setRoleLabel(tr("Error"));  // cn:误差
    connect(this, &DAChartAddXYESeriesWidget::dataManagerChanged, this, &DAChartAddXYESeriesWidget::onDataManagerChanged);
    connect(ui->selectWidgetX, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddXYESeriesWidget::onXSeriesChanged);
    connect(ui->selectWidgetY, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddXYESeriesWidget::onYSeriesChanged);
    connect(ui->selectWidgetYE, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddXYESeriesWidget::onYESeriesChanged);
    connect(ui->groupBoxXAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYESeriesWidget::onGroupBoxXAutoincrementClicked);
    connect(ui->groupBoxYAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYESeriesWidget::onGroupBoxYAutoincrementClicked);
}

DAChartAddXYESeriesWidget::~DAChartAddXYESeriesWidget()
{
    delete ui;
}

/**
 * @brief 判断x是否是自增
 * @return
 */
bool DAChartAddXYESeriesWidget::isXAutoincrement() const
{
    return ui->groupBoxXAutoincrement->isChecked();
}

/**
 * @brief 判断y是否是自增
 * @return
 */
bool DAChartAddXYESeriesWidget::isYAutoincrement() const
{
    return ui->groupBoxYAutoincrement->isChecked();
}

/**
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QVector< QwtIntervalSample > DAChartAddXYESeriesWidget::getSeries() const
{
    DAChartAddXYESeriesWidget* that = const_cast< DAChartAddXYESeriesWidget* >(this);
    QVector< QwtIntervalSample > xye;
    that->getToVectorPointFFromUI(xye);
    return xye;
}

/**
 * @brief DAChartAddXYESeriesWidget::onComboBoxXCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYESeriesWidget::onXSeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetX->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewXYE->setSeriesAt(0, series);
#endif
}

/**
 * @brief DAChartAddXYESeriesWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYESeriesWidget::onYSeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetY->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewXYE->setSeriesAt(1, series);
#endif
}

/**
 * @brief DAChartAddXYESeriesWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYESeriesWidget::onYESeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetYE->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewXYE->setSeriesAt(2, series);
#endif
}

/**
 * @brief x值是否使用自增序列
 * @param on
 */
void DAChartAddXYESeriesWidget::onGroupBoxXAutoincrementClicked(bool on)
{
#if DA_ENABLE_PYTHON
    if (on) {
        double base, step;
        if (tryGetXSelfInc(base, step)) {
            ui->tableViewXYE->setSeriesAt(0, DAAutoincrementSeries< double >(base, step));
        }
    } else {
        // 取消要读取回原来的设置
        onXSeriesChanged();
    }
    ui->selectWidgetX->setEnabled(!on);
#endif
}

/**
 * @brief y值是否使用自增序列
 * @param on
 */
void DAChartAddXYESeriesWidget::onGroupBoxYAutoincrementClicked(bool on)
{
#if DA_ENABLE_PYTHON
    if (on) {
        double base, step;
        if (tryGetYSelfInc(base, step)) {
            ui->tableViewXYE->setSeriesAt(1, DAAutoincrementSeries< double >(base, step));
        }
    } else {
        // 取消要读取回原来的设置
        onYSeriesChanged();
    }
    ui->selectWidgetY->setEnabled(!on);
#endif
}

void DAChartAddXYESeriesWidget::onDataManagerChanged(DADataManager* dmgr)
{
    ui->selectWidgetX->setDataManager(dmgr);
    ui->selectWidgetY->setDataManager(dmgr);
    ui->selectWidgetYE->setDataManager(dmgr);
}

/**
 * @brief 获取x自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYESeriesWidget::getXAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
    bool isOK   = false;
    double base = ui->lineEditXInitValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(
            this,
            tr("Warning"),                                                                                 // cn:警告
            tr("The initial value of x auto increment series must be a floating-point arithmetic number")  // cn:x自增序列的初始值必须为浮点数
        );
        return false;
    }
    double step = ui->lineEditXStepValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of x auto increment series "
                                "must be a floating-point arithmetic number")  // cn:x自增序列的步长必须为浮点数
        );
        return false;
    }
    v.setBaseValue(base);
    v.setStepValue(step);
    return true;
}

/**
 * @brief 获取y自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYESeriesWidget::getYAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
    bool isOK   = false;
    double base = ui->lineEditYInitValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The initial value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:y自增序列的初始值必须为浮点数
        );
        return false;
    }
    double step = ui->lineEditYStepValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:y自增序列的步长必须为浮点数
        );
        return false;
    }
    v.setBaseValue(base);
    v.setStepValue(step);
    return true;
}

/**
 * @brief 获取点序列
 * @param res
 * @return
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYESeriesWidget::getToVectorPointFFromUI(QVector< QwtIntervalSample >& res)
{
    bool isXAuto = ui->groupBoxXAutoincrement->isChecked();
    bool isYAuto = ui->groupBoxYAutoincrement->isChecked();
    if (isXAuto && isYAuto) {
        QMessageBox::warning(this,
                             tr("Warning"),                                                 // cn:警告
                             tr("x and y cannot be set to autoincrement at the same time")  // cn:x和y无法同时设置为自增
        );
        return false;
    }
#if DA_ENABLE_PYTHON
    // 辅助 lambda：从 DAChartSeriesSelectWidget 提取 DAPySeries
    auto extractSeries = [](DAChartSeriesSelectWidget* w) -> DAPySeries {
        QPair< DAData, QString > sel = w->getCurrentSeries();
        if (sel.first.isNull()) {
            return DAPySeries();
        }
        DAPyDataFrame df = sel.first.toDataFrame();
        if (df.isNone()) {
            return DAPySeries();
        }
        return df[ sel.second ];
    };
    if (isXAuto) {  // 不存在同时，因此这个就是x自增
        DAAutoincrementSeries< double > xinc;
        if (!getXAutoIncFromUI(xinc)) {
            return false;
        }
        DAPySeries y = extractSeries(ui->selectWidgetY);
        DAPySeries e = extractSeries(ui->selectWidgetYE);
        if (y.isNone() || e.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                  // cn:警告
                                 tr("y - value/error value must be a series"));  // cn:y必须是序列
            return false;
        }
        std::size_t s = y.size();
        try {
            std::vector< double > yCenter;
            std::vector< double > yError;
            yCenter.reserve(y.size());
            yError.reserve(e.size());
            y.castTo< double >(std::back_inserter(yCenter));
            e.castTo< double >(std::back_inserter(yError));
            res.reserve(static_cast< int >(s));
            for (int i = 0; i < s; ++i) {
                double min = yCenter[ i ] - yError[ i ];  // 计算区间范围
                double max = yCenter[ i ] + yError[ i ];
                res.push_back(QwtIntervalSample(xinc[ i ], min, max));
            }
        } catch (const std::exception& e) {
            daCritical << tr("Exception occurred during extracting from "
                             "pandas.Series to double vector:%1")
                              .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
            QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从pandas.Series提取为double vector过程中出现异常

            return false;
        }
    } else if (isYAuto) {
        DAAutoincrementSeries< double > yinc;
        if (!getYAutoIncFromUI(yinc)) {
            return false;
        }
        DAPySeries x = extractSeries(ui->selectWidgetX);
        DAPySeries e = extractSeries(ui->selectWidgetYE);
        if (x.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
            );
            return false;
        }
        if (e.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        std::size_t s = x.size();
        try {
            std::vector< double > vx;
            std::vector< double > yError;
            vx.reserve(x.size());
            yError.reserve(e.size());
            x.castTo< double >(std::back_inserter(vx));
            e.castTo< double >(std::back_inserter(yError));
            res.reserve(static_cast< int >(s));
            for (auto i = 0; i < s; ++i) {
                double min = yinc[ i ] - yError[ i ];
                double max = yinc[ i ] + yError[ i ];
                res.push_back(QwtIntervalSample(vx[ i ], min, max));
            }
        } catch (const std::exception& e) {
            daCritical << tr("Exception occurred during extracting from "
                             "pandas.Series to double vector:%1")
                              .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
            QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从pandas.Series提取为double vector过程中出现异常

            return false;
        }
    } else {
        DAPySeries x = extractSeries(ui->selectWidgetX);
        DAPySeries y = extractSeries(ui->selectWidgetY);
        DAPySeries e = extractSeries(ui->selectWidgetYE);
        if (x.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
            );
            return false;
        }
        if (y.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("y must be a series"));  // cn:y必须是序列
            return false;
        }
        if (e.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                  // cn:警告
                                 tr("error must be a series"));  // cn:误差必须是序列
            return false;
        }
        std::size_t s = std::min(x.size(), y.size());
        if (0 == s) {
            return true;
        }
        try {
            std::vector< double > vx;
            std::vector< double > yCenter;
            std::vector< double > yError;
            vx.reserve(x.size());
            yCenter.reserve(y.size());
            yError.reserve(e.size());
            x.castTo< double >(std::back_inserter(vx));
            y.castTo< double >(std::back_inserter(yCenter));
            e.castTo< double >(std::back_inserter(yError));
            res.reserve(static_cast< int >(s));
            for (auto i = 0; i < s; ++i) {
                double min = yCenter[ i ] - yError[ i ];
                double max = yCenter[ i ] + yError[ i ];
                res.push_back(QwtIntervalSample(vx[ i ], min, max));
            }
        } catch (const std::exception& e) {
            daCritical << tr("Exception occurred during extracting from pandas.Series to double vector:%1")
                              .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
            return false;
        }
    }
#endif
    return true;
}

/**
 * @brief 尝试获取x值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYESeriesWidget::tryGetXSelfInc(double& base, double& step)
{
    bool isOK = false;
    double a  = ui->lineEditXInitValue->text().toDouble(&isOK);
    if (!isOK) {
        return false;
    }
    double b = ui->lineEditXStepValue->text().toDouble(&isOK);
    if (!isOK) {
        return false;
    }
    base = a;
    step = b;
    return true;
}

/**
 * @brief 尝试获取y值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYESeriesWidget::tryGetYSelfInc(double& base, double& step)
{
    bool isOK = false;
    double a  = ui->lineEditYInitValue->text().toDouble(&isOK);
    if (!isOK) {
        return false;
    }
    double b = ui->lineEditYStepValue->text().toDouble(&isOK);
    if (!isOK) {
        return false;
    }
    base = a;
    step = b;
    return true;
}
}
