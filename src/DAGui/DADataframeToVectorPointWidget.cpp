#include "DADataframeToVectorPointWidget.h"
#include "ui_DADataframeToVectorPointWidget.h"
#include <iterator>
#include <vector>
#include <QHeaderView>
#include "Models/DAPySeriesTableModel.h"
#include "DALogCategory.h"
namespace DA
{
DADataframeToVectorPointWidget::DADataframeToVectorPointWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DADataframeToVectorPointWidget)
{
    ui->setupUi(this);
    mModel = new DAPySeriesTableModel(this);
    mModel->setHeaderLabel({ tr("x"), tr("y") });  // cn:x,y
    ui->tableViewXY->setModel(mModel);
    QFontMetrics fm = fontMetrics();
    ui->tableViewXY->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    connect(ui->listWidgetX,
            &DAPyDataframeColumnsListWidget::currentTextChanged,
            this,
            &DADataframeToVectorPointWidget::onListWidgetXCurrentTextChanged);
    connect(ui->listWidgetY,
            &DAPyDataframeColumnsListWidget::currentTextChanged,
            this,
            &DADataframeToVectorPointWidget::onListWidgetYCurrentTextChanged);
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
    mCurrentData = d;
    updateDataframeColumnList();
}

DAData DADataframeToVectorPointWidget::getCurrentData() const
{
    return mCurrentData;
}

bool DADataframeToVectorPointWidget::getToVectorPointF(QVector< QPointF >& res)
{
    if (!mCurrentData.isDataFrame() || mCurrentData.isNull()) {
        return false;
    }
    DAPySeries x = ui->listWidgetX->getCurrentSeries();
    DAPySeries y = ui->listWidgetY->getCurrentSeries();
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
        res.resize(static_cast< int >(s));
        for (int i = 0; i < s; ++i) {
            res[ i ].setX(vx[ i ]);
            res[ i ].setY(vy[ i ]);
        }
    } catch (const std::exception& e) {
        daCritical << tr("Exception occurred during extraction from pandas.Series to double vector: %1")
                          .arg(e.what());  // cn:从pandas.Series提取为double vector过程中出现异常:%1
        return false;
    }
    return true;
}

void DADataframeToVectorPointWidget::updateDataframeColumnList()
{
    ui->listWidgetX->clear();
    ui->listWidgetY->clear();
    mModel->clearData();
    if (mCurrentData.isNull() || !mCurrentData.isDataFrame()) {
        return;
    }
    DAPyDataFrame df = mCurrentData.toDataFrame();
    if (df.isNone()) {
        return;
    }
    ui->listWidgetX->setDataframe(df);
    ui->listWidgetY->setDataframe(df);
}

void DADataframeToVectorPointWidget::onListWidgetXCurrentTextChanged(const QString& n)
{
    DAPyDataFrame df = mCurrentData.toDataFrame();
    if (df.isNone()) {
        return;
    }
    DAPySeries s = df[ n ];
    if (s.isNone()) {
        return;
    }
    mModel->setSeriesAt(0, s);
}

void DADataframeToVectorPointWidget::onListWidgetYCurrentTextChanged(const QString& n)
{
    DAPyDataFrame df = mCurrentData.toDataFrame();
    if (df.isNone()) {
        return;
    }
    DAPySeries s = df[ n ];
    if (s.isNone()) {
        return;
    }
    mModel->setSeriesAt(1, s);
}
}
