#include "DAChartAddXYSeriesWidget.h"
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
    DAPySeriesTableModule* _model { nullptr };
    bool _xNeedInsert0 { false };
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
    d_ptr->_model = new DAPySeriesTableModule(this);
    d_ptr->_model->setHeaderLabel({ tr("x"), tr("y") });
    ui->tableViewXY->setModel(d_ptr->_model);
    QFontMetrics fm = fontMetrics();
    ui->tableViewXY->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    connect(ui->comboBoxX, &DADataManagerComboBox::currentDataframeSeriesChanged, this, &DAChartAddXYSeriesWidget::onComboBoxXCurrentDataframeSeriesChanged);
    connect(ui->comboBoxY, &DADataManagerComboBox::currentDataframeSeriesChanged, this, &DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged);
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
 * @brief 创建item
 * @return
 */
QwtPlotItem* DAChartAddXYSeriesWidget::createPlotItem()
{
    QVector< QPointF > xy;
    if (!getToVectorPointF(xy)) {
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
    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        return;
    }
    DAPySeries s = df[ seriesName ];
    if (s.isNone()) {
        return;
    }
    if (d_ptr->_xNeedInsert0) {
        d_ptr->_xNeedInsert0 = false;
        d_ptr->_model->insertSeries(0, s);
    } else {
        d_ptr->_model->setSeriesAt(0, s);
    }
}

/**
 * @brief DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged
 * @param data
 * @param seriesName
 */
void DAChartAddXYSeriesWidget::onComboBoxYCurrentDataframeSeriesChanged(const DAData& data, const QString& seriesName)
{
    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        return;
    }
    DAPySeries s = df[ seriesName ];
    if (s.isNone()) {
        return;
    }
    if (0 == d_ptr->_model->getSeriesCount()) {
        d_ptr->_xNeedInsert0 = true;  //说明y先显示，这时x要插入0，而不是替换
    }
    d_ptr->_model->setSeriesAt(1, s);
}

bool DAChartAddXYSeriesWidget::getToVectorPointF(QVector< QPointF >& res)
{
    DAData xd = ui->comboBoxX->getCurrentDAData();
    DAData yd = ui->comboBoxY->getCurrentDAData();
    if (!xd.isSeries() || !yd.isSeries()) {
        return false;
    }
    DAPySeries x = xd.toSeries();
    DAPySeries y = yd.toSeries();
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
        x.castTo< double >(std::back_inserter(vx));
        y.castTo< double >(std::back_inserter(vy));
        res.resize(s);
        for (int i = 0; i < s; ++i) {
            res[ i ].setX(vx[ i ]);
            res[ i ].setY(vy[ i ]);
        }
    } catch (const std::exception& e) {
        qCritical() << tr("Exception occurred during extracting from pandas.Series to double vector:%1").arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
        return false;
    }
    return true;
}

}
