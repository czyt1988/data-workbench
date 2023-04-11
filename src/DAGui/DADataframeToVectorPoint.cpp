#include "DADataframeToVectorPoint.h"
#include "ui_DADataframeToVectorPoint.h"
#include <iterator>
#include <vector>
namespace DA
{
DADataframeToVectorPoint::DADataframeToVectorPoint(QWidget* parent)
    : QWidget(parent), ui(new Ui::DADataframeToVectorPoint)
{
    ui->setupUi(this);
}

DADataframeToVectorPoint::~DADataframeToVectorPoint()
{
    delete ui;
}

/**
 * @brief 设置当前的数据
 * @param d
 */
void DADataframeToVectorPoint::setCurrentData(const DAData& d)
{
    _currentData = d;
    updateDataframeColumnList();
}

DAData DADataframeToVectorPoint::getCurrentData() const
{
    return _currentData;
}

bool DADataframeToVectorPoint::getToVectorPointF(QVector< QPointF >& res)
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

void DADataframeToVectorPoint::updateDataframeColumnList()
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
}
