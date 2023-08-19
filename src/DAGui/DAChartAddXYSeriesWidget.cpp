#include "DAChartAddXYSeriesWidget.h"
#include <QMessageBox>
#include "ui_DAChartAddXYSeriesWidget.h"
#include "DADataManager.h"
#include "Models/DADataManagerTreeModel.h"
#include "Models/DAPySeriesTableModule.h"
#include "qwt_plot_curve.h"
namespace DA
{

class DAChartAddXYSeriesWidgetPrivate
{
    DA_IMPL_PUBLIC(DAChartAddXYSeriesWidget)
public:
    DAChartAddXYSeriesWidgetPrivate(DAChartAddXYSeriesWidget* p);

public:
    DAPySeriesTableModule* _model{ nullptr };
};

DAChartAddXYSeriesWidgetPrivate::DAChartAddXYSeriesWidgetPrivate(DAChartAddXYSeriesWidget* p) : q_ptr(p)
{
}
//===================================================
// DAChartAddXYSeriesWidget
//===================================================

DAChartAddXYSeriesWidget::DAChartAddXYSeriesWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddXYSeriesWidget), d_ptr(new DAChartAddXYSeriesWidgetPrivate(this))
{
    ui->setupUi(this);
    ui->lineEditTitle->setText(tr("curve"));
    d_ptr->_model = new DAPySeriesTableModule(this);
    d_ptr->_model->setHeaderLabel({ tr("x"), tr("y") });
    ui->tableViewXY->setModel(d_ptr->_model);
    QFontMetrics fm = fontMetrics();
    ui->tableViewXY->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    connect(ui->comboBoxX, &DADataManagerComboBox::currentDataframeSeriesChanged, this, &DAChartAddXYSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged);
    connect(ui->comboBoxY, &DADataManagerComboBox::currentDataframeSeriesChanged, this, &DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged);
    connect(ui->groupBoxXAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYSeriesWidget::onGroupBoxXAutoincrementClicked);
    connect(ui->groupBoxYAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYSeriesWidget::onGroupBoxYAutoincrementClicked);
}

DAChartAddXYSeriesWidget::~DAChartAddXYSeriesWidget()
{
    delete ui;
}

void DAChartAddXYSeriesWidget::setDataManager(DADataManager* dmgr)
{
    ui->comboBoxX->setDataManager(dmgr);
    ui->comboBoxY->setDataManager(dmgr);
}

DADataManager* DAChartAddXYSeriesWidget::getDataManager() const
{
    return ui->comboBoxX->getDataManager();
}

/**
 * @brief 判断x是否是自增
 * @return
 */
bool DAChartAddXYSeriesWidget::isXAutoincrement() const
{
    return ui->groupBoxXAutoincrement->isChecked();
}

/**
 * @brief 判断y是否是自增
 * @return
 */
bool DAChartAddXYSeriesWidget::isYAutoincrement() const
{
    return ui->groupBoxYAutoincrement->isChecked();
}

/**
 * @brief 创建item
 * @return
 */
QwtPlotItem* DAChartAddXYSeriesWidget::createPlotItem()
{
    QVector< QPointF > xy;
    if (!getToVectorPointFFromUI(xy)) {
        return nullptr;
    }
    QwtPlotCurve* cur = new QwtPlotCurve(ui->lineEditTitle->text());
    cur->setSamples(xy);
    return cur;
}

/**
 * @brief DAChartAddXYSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
{
    DAPySeries series;
    DAPyDataFrame df = data.toDataFrame();
    if (!df.isNone()) {
        series = df[ seriesName ];
    }
    d_ptr->_model->setSeriesAt(0, series);
}

/**
 * @brief DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
{
    DAPySeries series;
    DAPyDataFrame df = data.toDataFrame();
    if (!df.isNone()) {
        series = df[ seriesName ];
    }
    d_ptr->_model->setSeriesAt(1, series);
}

/**
 * @brief x值是否使用自增序列
 * @param on
 */
void DAChartAddXYSeriesWidget::onGroupBoxXAutoincrementClicked(bool on)
{
    if (on) {
        double base, step;
        if (tryGetXSelfInc(base, step)) {
            d_ptr->_model->setSeriesAt(0, DAAutoincrementSeries< double >(base, step));
        }
    } else {
        //取消要读取回原来的设置
        DAPySeries series;
        DAData data = ui->comboBoxX->getCurrentDAData();
        if (data) {
            series = data.toSeries();
        }
        d_ptr->_model->setSeriesAt(0, series);
    }
    ui->comboBoxX->setEnabled(!on);
}

/**
 * @brief y值是否使用自增序列
 * @param on
 */
void DAChartAddXYSeriesWidget::onGroupBoxYAutoincrementClicked(bool on)
{
    if (on) {
        double base, step;
        if (tryGetYSelfInc(base, step)) {
            d_ptr->_model->setSeriesAt(1, DAAutoincrementSeries< double >(base, step));
        }
    } else {
        //取消要读取回原来的设置
        DAPySeries series;
        DAData data = ui->comboBoxY->getCurrentDAData();
        if (data) {
            series = data.toSeries();
        }
        d_ptr->_model->setSeriesAt(1, series);
    }
    ui->comboBoxY->setEnabled(!on);
}

/**
 * @brief 获取x自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYSeriesWidget::getXAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
    bool isOK   = false;
    double base = ui->lineEditXInitValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::
                warning(this,
                        tr("Warning"),  // cn:警告
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
bool DAChartAddXYSeriesWidget::getYAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
    bool isOK   = false;
    double base = ui->lineEditYInitValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The initial value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:x自增序列的初始值必须为浮点数
        );
        return false;
    }
    double step = ui->lineEditYStepValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:x自增序列的步长必须为浮点数
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
bool DAChartAddXYSeriesWidget::getToVectorPointFFromUI(QVector< QPointF >& res)
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
    if (isXAuto) {  //不存在同时，因此这个就是x自增
        DAAutoincrementSeries< double > xinc;
        if (!getXAutoIncFromUI(xinc)) {
            return false;
        }
        DAData yd = ui->comboBoxY->getCurrentDAData();
        if (!yd.isSeries()) {
            QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("y must be a series"));  // cn:y必须是序列
            return false;
        }
        DAPySeries y = yd.toSeries();
        if (y.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        std::size_t s = y.size();
        try {
            std::vector< double > vy;
            vy.reserve(y.size());
            y.castTo< double >(std::back_inserter(vy));
            res.resize(s);
            for (std::size_t i = 0; i < s; ++i) {
                res[ i ].setX(xinc[ i ]);
                res[ i ].setY(vy[ i ]);
            }
        } catch (const std::exception& e) {
            qCritical() << tr("Exception occurred during extracting from "
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
        DAData xd = ui->comboBoxX->getCurrentDAData();
        if (!xd.isSeries()) {
            QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
            );
            return false;
        }
        DAPySeries x = xd.toSeries();
        if (x.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        std::size_t s = x.size();
        try {
            std::vector< double > vx;
            vx.reserve(x.size());
            x.castTo< double >(std::back_inserter(vx));
            res.resize(s);
            for (std::size_t i = 0; i < s; ++i) {
                res[ i ].setX(vx[ i ]);
                res[ i ].setY(yinc[ i ]);
            }
        } catch (const std::exception& e) {
            qCritical() << tr("Exception occurred during extracting from "
                              "pandas.Series to double vector:%1")
                                   .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
            QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从pandas.Series提取为double vector过程中出现异常

            return false;
        }
    } else {
        DAData xd = ui->comboBoxX->getCurrentDAData();
        DAData yd = ui->comboBoxY->getCurrentDAData();
        if (!xd.isSeries()) {
            QMessageBox::warning(this,
                                 tr("Warning"),            // cn:警告
                                 tr("x must be a series")  // cn:x必须是序列
            );
            return false;
        }
        if (!yd.isSeries()) {
            QMessageBox::warning(this,
                                 tr("Warning"),              // cn:警告
                                 tr("y must be a series"));  // cn:y必须是序列
            return false;
        }
        DAPySeries x = xd.toSeries();
        DAPySeries y = yd.toSeries();
        if (x.isNone() || y.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
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
            x.castTo< double >(std::back_inserter(vx));
            y.castTo< double >(std::back_inserter(vy));
            res.resize(s);
            for (std::size_t i = 0; i < s; ++i) {
                res[ i ].setX(vx[ i ]);
                res[ i ].setY(vy[ i ]);
            }
        } catch (const std::exception& e) {
            qCritical() << tr("Exception occurred during extracting from pandas.Series to double vector:%1").arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
            return false;
        }
    }
    return true;
}

/**
 * @brief 尝试获取x值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYSeriesWidget::tryGetXSelfInc(double& base, double& step)
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
bool DAChartAddXYSeriesWidget::tryGetYSelfInc(double& base, double& step)
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
