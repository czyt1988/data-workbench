#include "DADataframeToVectorPointWidget.h"
#include "ui_DADataframeToVectorPointWidget.h"
#include <iterator>
#include <vector>
#include <QHeaderView>
#include "Models/DAPySeriesTableModule.h"
namespace DA
{
DADataframeToVectorPointWidget::DADataframeToVectorPointWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DADataframeToVectorPointWidget)
{
    ui->setupUi(this);
    _model = new DAPySeriesTableModule(this);
    _model->setHeaderLabel({ tr("x"), tr("y") });
    ui->tableViewXY->setModel(_model);
    QFontMetrics fm = fontMetrics();
    ui->tableViewXY->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    connect(ui->listWidgetX, &DAPyDataframeColumnsListWidget::currentTextChanged, this, &DADataframeToVectorPointWidget::onListWidgetXCurrentTextChanged);
    connect(ui->listWidgetY, &DAPyDataframeColumnsListWidget::currentTextChanged, this, &DADataframeToVectorPointWidget::onListWidgetYCurrentTextChanged);
}

DADataframeToVectorPointWidget::~DADataframeToVectorPointWidget()
{
    delete ui;
}

/**
 * @brief 设置当前的数据
 * @param d
 */
void DADataframeToVectorPointWidget::setCurrentData(const DAData& d)
{
    _currentData = d;
    updateDataframeColumnList();
}

DAData DADataframeToVectorPointWidget::getCurrentData() const
{
    return _currentData;
}

bool DADataframeToVectorPointWidget::getToVectorPointF(QVector< QPointF >& res)
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

void DADataframeToVectorPointWidget::updateDataframeColumnList()
{
    ui->listWidgetX->clear();
    ui->listWidgetY->clear();
    _xNeedInsert0 = false;
    _model->clear();
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

void DADataframeToVectorPointWidget::onListWidgetXCurrentTextChanged(const QString& n)
{
    DAPyDataFrame df = _currentData.toDataFrame();
    if (df.isNone()) {
        return;
    }
    DAPySeries s = df[ n ];
    if (s.isNone()) {
        return;
    }
    if (_xNeedInsert0) {
        _xNeedInsert0 = false;
        _model->insertSeries(0, s);
    } else {
        _model->setSeriesAt(0, s);
    }
}

void DADataframeToVectorPointWidget::onListWidgetYCurrentTextChanged(const QString& n)
{
    DAPyDataFrame df = _currentData.toDataFrame();
    if (df.isNone()) {
        return;
    }
    DAPySeries s = df[ n ];
    if (s.isNone()) {
        return;
    }
    if (0 == _model->getSeriesCount()) {
        _xNeedInsert0 = true;  //说明y先显示，这时x要插入0，而不是替换
    }
    _model->setSeriesAt(1, s);
}
}
