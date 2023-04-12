#include "DAChartAddXYSeriesWidget.h"
#include "ui_DAChartAddXYSeriesWidget.h"
#include "DADataManager.h"
namespace DA
{
DAChartAddXYSeriesWidget::DAChartAddXYSeriesWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartAddXYSeriesWidget)
{
    ui->setupUi(this);
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

bool DAChartAddXYSeriesWidget::getToVectorPointF(QVector< QPointF >& res)
{
    return false;
}
}
