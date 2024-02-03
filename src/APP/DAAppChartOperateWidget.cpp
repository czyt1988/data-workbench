#include "DAAppChartOperateWidget.h"
#include "Dialog/DADialogChartGuide.h"
#include "DAAppFigureWidget.h"
#include "DAWaitCursorScoped.h"
#include "DAChartUtil.h"
#include "DAEvenFilterDragPlotWithGuide.h"
namespace DA
{

DAAppChartOperateWidget::DAAppChartOperateWidget(QWidget* parent) : DAChartOperateWidget(parent)
{
    mFigEventFilter = new DAEvenFilterDragPlotWithGuide(this);
    mFigEventFilter->setChartOptWidget(this);
}

DAAppChartOperateWidget::~DAAppChartOperateWidget()
{
}

/**
 * @brief 设置dmg
 * @param mgr
 */
void DAAppChartOperateWidget::setDataManager(DADataManager* mgr)
{
    mDataMgr = mgr;
}

/**
 * @brief 创建figure
 * @return
 */
DAFigureWidget* DAAppChartOperateWidget::createFigure()
{
    DAFigureWidget* fig = DAChartOperateWidget::createFigure();
    if (DAAppFigureWidget* appFig = qobject_cast< DAAppFigureWidget* >(fig)) {
        appFig->installEventFilter(mFigEventFilter);
    }
    return fig;
}

/**
 * @brief 生成绘图引导窗口
 * @param data
 * @return
 */
QwtPlotItem* DAAppChartOperateWidget::plotWithGuideDialog(const DAData& data)
{
    if (nullptr == mChartGuideDlg) {
        mChartGuideDlg = new DADialogChartGuide(this);
        mChartGuideDlg->setDataManager(mDataMgr);
    }
    if (data) {
        mChartGuideDlg->setCurrentData(data);
    } else {
        mChartGuideDlg->updateData();
    }
    if (QDialog::Accepted != mChartGuideDlg->exec()) {
        return nullptr;
    }
    DAWaitCursorScoped wait;
    Q_UNUSED(wait);
    QwtPlotItem* item = mChartGuideDlg->createPlotItem();
    if (nullptr == item) {
        return nullptr;
    }
    QColor clr = mColorTheme.current();
    if (DAChartUtil::setPlotItemColor(item, clr)) {
        // 成功设置颜色，就把主题颜色下移一个
        mColorTheme.moveToNext();
    }
    qDebug() << "color:" << clr.name() << "  |  ColorTheme = " << mColorTheme;
    return item;
}

}  // end DA
