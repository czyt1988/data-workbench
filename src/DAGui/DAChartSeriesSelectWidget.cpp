#include "DAChartSeriesSelectWidget.h"
#include "ui_DAChartSeriesSelectWidget.h"
// Qt
#include <QToolButton>
#include <QDialog>
// DA
#include "DADataManager.h"
#include "DAChartSeriesPickerWidget.h"
#include "DALogCategory.h"

namespace DA
{
//===================================================
// DAChartSeriesSelectWidget
//===================================================

DAChartSeriesSelectWidget::DAChartSeriesSelectWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAChartSeriesSelectWidget)
{
    ui->setupUi(this);
    ui->listView->setAcceptMode(DAPySeriesListView::AcceptOneSeries);
    connect(ui->toolButtonAdd, &QToolButton::clicked, this, &DAChartSeriesSelectWidget::onAddClicked);
    connect(ui->toolButtonRemove, &QToolButton::clicked, this, &DAChartSeriesSelectWidget::onRemoveClicked);
    connect(ui->listView, &DAPySeriesListView::seriesChanged, this, &DAChartSeriesSelectWidget::seriesChanged);
}

DAChartSeriesSelectWidget::~DAChartSeriesSelectWidget()
{
    delete ui;
}

void DAChartSeriesSelectWidget::setAcceptMode(DAPySeriesListView::AcceptMode mode)
{
    ui->listView->setAcceptMode(mode);
}

DAPySeriesListView::AcceptMode DAChartSeriesSelectWidget::getAcceptMode() const
{
    return ui->listView->getAcceptMode();
}

void DAChartSeriesSelectWidget::setDataManager(DADataManager* mgr)
{
    mDataMgr = mgr;
    ui->listView->setDataManager(mgr);
    if (mPicker) {
        // picker 内部通过 findChild 查找 DADataManageWidget，不直接持有 mgr，
        // 但若已懒创建，下次 startPick 时会自动重连。这里仅同步缓存。
    }
}

DADataManager* DAChartSeriesSelectWidget::getDataManager() const
{
    return mDataMgr;
}

void DAChartSeriesSelectWidget::setRoleLabel(const QString& label)
{
    mRoleLabel = label;
    if (mPicker) {
        mPicker->setRoleLabel(label);
    }
}

QString DAChartSeriesSelectWidget::getRoleLabel() const
{
    return mRoleLabel;
}

QPair< DAData, QString > DAChartSeriesSelectWidget::getCurrentSeries() const
{
    QList< QPair< DAData, QStringList > > datas = ui->listView->getSeries();
    if (datas.isEmpty() || datas.first().second.isEmpty()) {
        return qMakePair(DAData(), QString());
    }
    return qMakePair(datas.first().first, datas.first().second.first());
}

QList< QPair< DAData, QStringList > > DAChartSeriesSelectWidget::getSeries() const
{
    return ui->listView->getSeries();
}

void DAChartSeriesSelectWidget::addSeries(const DAData& dataframeData, const QString& name)
{
    ui->listView->addSeries(dataframeData, name);
}

void DAChartSeriesSelectWidget::removeCurrentSelect()
{
    ui->listView->removeCurrentSelect();
}

/**
 * @brief 点击添加按钮：隐藏父级绘图引导对话框，弹出 picker 浮动窗口
 */
void DAChartSeriesSelectWidget::onAddClicked()
{
    ensurePicker();
    hideParentGuideDialog();
    mPicker->startPick();
}

/**
 * @brief 点击移除按钮：移除 listView 当前选中项
 */
void DAChartSeriesSelectWidget::onRemoveClicked()
{
    ui->listView->removeCurrentSelect();
}

/**
 * @brief 懒创建 picker 浮动窗口并连接信号
 *
 * 首次调用时 new DAChartSeriesPickerWidget，后续复用。
 * picker 确认选择后通过 seriesConfirmed 信号回传 (data, seriesName)，
 * 调用 addSeries 注入 listView；canceled 时仅恢复父对话框显示。
 */
void DAChartSeriesSelectWidget::ensurePicker()
{
    if (!mPicker) {
        mPicker = new DAChartSeriesPickerWidget(mDataMgr, mRoleLabel + ":", this);
        connect(mPicker, &DAChartSeriesPickerWidget::seriesConfirmed, this,
                [ this ](const DAData& data, const QString& seriesName) {
                    addSeries(data, seriesName);
                    showParentGuideDialog();
                });
        connect(mPicker, &DAChartSeriesPickerWidget::canceled, this,
                &DAChartSeriesSelectWidget::showParentGuideDialog);
    } else {
        mPicker->setExpression(QString());
        mPicker->setRoleLabel(mRoleLabel + ":");
    }
}

/**
 * @brief 隐藏父级绘图引导对话框（DADialogChartGuide）
 *
 * 本组件通常嵌入在 DADialogChartGuide 中，弹出 picker 前需隐藏父对话框
 * 以解除模态阻塞，让用户能操作主窗口的数据管理 dock。
 */
void DAChartSeriesSelectWidget::hideParentGuideDialog()
{
    if (auto* dlg = qobject_cast< QDialog* >(window())) {
        dlg->hide();
    }
}

/**
 * @brief 显示父级绘图引导对话框
 */
void DAChartSeriesSelectWidget::showParentGuideDialog()
{
    if (auto* dlg = qobject_cast< QDialog* >(window())) {
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }
}

}  // namespace DA
