#include "DAChartAddOHLCSeriesWidget.h"
#include "ui_DAChartAddOHLCSeriesWidget.h"
#include <QMessageBox>
#include "DADataManager.h"
#include "Models/DADataManagerTreeModel.h"
#include "DALogCategory.h"
#include "DAChartSeriesSelectWidget.h"
#include "qwt_samples.h"
#include "qwt_plot_tradingcurve.h"
#if DA_ENABLE_PYTHON
#include "Models/DAPySeriesTableModel.h"
#endif
namespace DA
{

class DAChartAddOHLCSeriesWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartAddOHLCSeriesWidget)
public:
    PrivateData(DAChartAddOHLCSeriesWidget* p);

public:
    DADataManager* _dataMgr { nullptr };
};

DAChartAddOHLCSeriesWidget::PrivateData::PrivateData(DAChartAddOHLCSeriesWidget* p) : q_ptr(p)
{
}
//===================================================
// DAChartAddOHLCSeriesWidget
//===================================================

DAChartAddOHLCSeriesWidget::DAChartAddOHLCSeriesWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), DA_PIMPL_CONSTRUCT, ui(new Ui::DAChartAddOHLCSeriesWidget)
{
    ui->setupUi(this);
#if DA_ENABLE_PYTHON
    DAPySeriesTableModel* m = new DAPySeriesTableModel(this);
    m->setHeaderLabel({ tr("Time"),   // cn:时间
                        tr("Open"),   // cn:开盘
                        tr("High"),   // cn:最高
                        tr("Low"),    // cn:最低
                        tr("Close")   // cn:收盘
                      });
    ui->tableViewOHLC->setModel(m);
#endif
    QFontMetrics fm = fontMetrics();
    ui->tableViewOHLC->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    ui->selectWidgetT->setRoleLabel(tr("Time"));  // cn:时间
    ui->selectWidgetO->setRoleLabel(tr("Open"));  // cn:开盘
    ui->selectWidgetH->setRoleLabel(tr("High"));  // cn:最高
    ui->selectWidgetL->setRoleLabel(tr("Low"));  // cn:最低
    ui->selectWidgetC->setRoleLabel(tr("Close"));  // cn:收盘
    connect(this, &DAChartAddOHLCSeriesWidget::dataManagerChanged, this, &DAChartAddOHLCSeriesWidget::onDataManagerChanged);
    connect(ui->selectWidgetT, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddOHLCSeriesWidget::onTSeriesChanged);
    connect(ui->selectWidgetO, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddOHLCSeriesWidget::onOSeriesChanged);
    connect(ui->selectWidgetH, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddOHLCSeriesWidget::onHSeriesChanged);
    connect(ui->selectWidgetL, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddOHLCSeriesWidget::onLSeriesChanged);
    connect(ui->selectWidgetC, &DAChartSeriesSelectWidget::seriesChanged, this, &DAChartAddOHLCSeriesWidget::onCSeriesChanged);
    connect(ui->groupBoxTAutoincrement, &QGroupBox::clicked, this, &DAChartAddOHLCSeriesWidget::onGroupBoxTAutoincrementClicked);
}

DAChartAddOHLCSeriesWidget::~DAChartAddOHLCSeriesWidget()
{
    delete ui;
}

/**
 * @brief 判断x是否是自增
 * @return
 */
bool DAChartAddOHLCSeriesWidget::isTAutoincrement() const
{
    return ui->groupBoxTAutoincrement->isChecked();
}

/**
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QVector< QwtOHLCSample > DAChartAddOHLCSeriesWidget::getSeries() const
{
    DAChartAddOHLCSeriesWidget* that = const_cast< DAChartAddOHLCSeriesWidget* >(this);
    QVector< QwtOHLCSample > ohlc;
    that->getToVectorPointFFromUI(ohlc);
    return ohlc;
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onTSeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetT->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewOHLC->setSeriesAt(0, series);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxOCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onOSeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetO->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewOHLC->setSeriesAt(1, series);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxHCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onHSeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetH->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewOHLC->setSeriesAt(2, series);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxLCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onLSeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetL->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewOHLC->setSeriesAt(3, series);
#endif
}

/**
 * @brief DAChartAddOHLCSeriesWidget::onComboBoxCCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddOHLCSeriesWidget::onCSeriesChanged()
{
#if DA_ENABLE_PYTHON
    QPair< DAData, QString > sel = ui->selectWidgetC->getCurrentSeries();
    DAPySeries series;
    if (!sel.first.isNull()) {
        DAPyDataFrame df = sel.first.toDataFrame();
        if (!df.isNone()) {
            series = df[ sel.second ];
        }
    }
    ui->tableViewOHLC->setSeriesAt(4, series);
#endif
}

/**
 * @brief x值是否使用自增序列
 * @param on
 */
void DAChartAddOHLCSeriesWidget::onGroupBoxTAutoincrementClicked(bool on)
{
#if DA_ENABLE_PYTHON
    if (on) {
        double base, step;
        if (tryGetTSelfInc(base, step)) {
            ui->tableViewOHLC->setSeriesAt(0, DAAutoincrementSeries< double >(base, step));
        }
    } else {
        // 取消要读取回原来的设置
        onTSeriesChanged();
    }
    ui->selectWidgetT->setEnabled(!on);
#endif
}

void DAChartAddOHLCSeriesWidget::onDataManagerChanged(DADataManager* dmgr)
{
    ui->selectWidgetT->setDataManager(dmgr);
    ui->selectWidgetO->setDataManager(dmgr);
    ui->selectWidgetH->setDataManager(dmgr);
    ui->selectWidgetL->setDataManager(dmgr);
    ui->selectWidgetC->setDataManager(dmgr);
}

/**
 * @brief 获取x自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddOHLCSeriesWidget::getTAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
    bool isOK   = false;
    double base = ui->lineEditTInitValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(
            this,
            tr("Warning"),                                                                                 // cn:警告
            tr("The initial value of t auto increment series must be a floating-point arithmetic number")  // cn:t自增序列的初始值必须为浮点数
        );
        return false;
    }
    double step = ui->lineEditTStepValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of t auto increment series "
                                "must be a floating-point arithmetic number")  // cn:t自增序列的步长必须为浮点数
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
bool DAChartAddOHLCSeriesWidget::getToVectorPointFFromUI(QVector< QwtOHLCSample >& res)
{
    bool isTAuto = ui->groupBoxTAutoincrement->isChecked();
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
    if (isTAuto) {  // 不存在同时，因此这个就是x自增
        DAAutoincrementSeries< double > tinc;
        if (!getTAutoIncFromUI(tinc)) {
            return false;
        }
        DAPySeries o = extractSeries(ui->selectWidgetO);
        DAPySeries h = extractSeries(ui->selectWidgetH);
        DAPySeries l = extractSeries(ui->selectWidgetL);
        DAPySeries c = extractSeries(ui->selectWidgetC);
        if (o.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                       // cn:警告
                                 tr("open value must be a series"));  // cn:开盘值必须是序列
            return false;
        }
        if (h.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                       // cn:警告
                                 tr("high value must be a series"));  // cn:最高值必须是序列
            return false;
        }
        if (l.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                      // cn:警告
                                 tr("low value must be a series"));  // cn:最低值必须是序列
            return false;
        }
        if (c.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                        // cn:警告
                                 tr("close value must be a series"));  // cn:收盘值必须是序列
            return false;
        }
        std::size_t s = o.size();
        try {
            std::vector< double > vo;
            std::vector< double > vh;
            std::vector< double > vl;
            std::vector< double > vc;
            vo.reserve(o.size());
            vh.reserve(h.size());
            vl.reserve(l.size());
            vc.reserve(c.size());
            o.castTo< double >(std::back_inserter(vo));
            h.castTo< double >(std::back_inserter(vh));
            l.castTo< double >(std::back_inserter(vl));
            c.castTo< double >(std::back_inserter(vc));
            res.resize(static_cast< int >(s));
            for (int i = 0; i < s; ++i) {
                res[ i ] = QwtOHLCSample(tinc[ i ], vo[ i ], vh[ i ], vl[ i ], vc[ i ]);
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
        DAPySeries t = extractSeries(ui->selectWidgetT);
        DAPySeries o = extractSeries(ui->selectWidgetO);
        DAPySeries h = extractSeries(ui->selectWidgetH);
        DAPySeries l = extractSeries(ui->selectWidgetL);
        DAPySeries c = extractSeries(ui->selectWidgetC);
        if (t.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                     // cn:警告
                                 tr("time value must be a series")  // cn:时间必须是序列
            );
            return false;
        }
        if (o.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                       // cn:警告
                                 tr("open value must be a series"));  // cn:开盘值必须是序列
            return false;
        }
        if (h.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                       // cn:警告
                                 tr("high value must be a series"));  // cn:最高值必须是序列
            return false;
        }
        if (l.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                      // cn:警告
                                 tr("low value must be a series"));  // cn:最低值必须是序列
            return false;
        }
        if (c.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                        // cn:警告
                                 tr("close value must be a series"));  // cn:收盘值必须是序列
            return false;
        }

        std::size_t s = std::min(t.size(), o.size());
        if (0 == s) {
            return true;
        }
        try {
            std::vector< double > vt;
            std::vector< double > vo;
            std::vector< double > vh;
            std::vector< double > vl;
            std::vector< double > vc;
            vt.reserve(t.size());
            vo.reserve(o.size());
            vh.reserve(h.size());
            vl.reserve(l.size());
            vc.reserve(c.size());
            t.castTo< double >(std::back_inserter(vt));
            o.castTo< double >(std::back_inserter(vo));
            h.castTo< double >(std::back_inserter(vh));
            l.castTo< double >(std::back_inserter(vl));
            c.castTo< double >(std::back_inserter(vc));
            res.resize(static_cast< int >(s));
            for (int i = 0; i < s; ++i) {
                res[ i ] = QwtOHLCSample(vt[ i ], vo[ i ], vh[ i ], vl[ i ], vc[ i ]);
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
 * @brief 尝试获取t值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddOHLCSeriesWidget::tryGetTSelfInc(double& base, double& step)
{
    bool isOK = false;
    double a  = ui->lineEditTInitValue->text().toDouble(&isOK);
    if (!isOK) {
        return false;
    }
    double b = ui->lineEditTStepValue->text().toDouble(&isOK);
    if (!isOK) {
        return false;
    }
    base = a;
    step = b;
    return true;
}
}
