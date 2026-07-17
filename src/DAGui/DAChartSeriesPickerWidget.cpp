#include "DAChartSeriesPickerWidget.h"
#include "ui_DAChartSeriesPickerWidget.h"
// Qt
#include <QApplication>
#include <QCloseEvent>
#include <QLineEdit>
#include <QToolButton>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
// ADS
#include "DockWidget.h"
// DA
#include "DADataManager.h"
#include "DADataManageWidget.h"
#include "DADataOperateWidget.h"
#include "DADataOperateOfDataFrameWidget.h"
#include "DALogCategory.h"
#if DA_ENABLE_PYTHON
#include "pandas/DAPyDataFrame.h"
#endif

namespace DA
{
//===================================================
// DAChartSeriesPickerWidget
//===================================================

DAChartSeriesPickerWidget::DAChartSeriesPickerWidget(DADataManager* mgr, Role role, QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::WindowStaysOnTopHint), ui(new Ui::DAChartSeriesPickerWidget), mDataMgr(mgr), mRole(role), mPickActive(false), mConfirmed(false)
{
    ui->setupUi(this);
    retranslateUi();
    connect(ui->toolButtonReturn, &QToolButton::clicked, this, &DAChartSeriesPickerWidget::onButtonReturnClicked);
}

DAChartSeriesPickerWidget::~DAChartSeriesPickerWidget()
{
    disconnectDataWidgets();
    delete ui;
}

/**
 * @brief 获取表达式文本
 * @return
 */
QString DAChartSeriesPickerWidget::getExpression() const
{
    return ui->lineEditExpr->text();
}

/**
 * @brief 设置表达式文本
 * @param expr
 */
void DAChartSeriesPickerWidget::setExpression(const QString& expr)
{
    ui->lineEditExpr->setText(expr);
}

/**
 * @brief 解析表达式为 DAData 和 series 名
 *
 * 支持 data['col'] 和 data["col"] 两种形式，使用 DADataManager::findData 查找数据，
 * 再校验列名是否存在于 dataframe。
 * @param outData 输出解析到的 DAData
 * @param outSeriesName 输出 series 名
 * @return 解析成功返回 true
 */
bool DAChartSeriesPickerWidget::resolveExpression(DAData& outData, QString& outSeriesName) const
{
    const QString expr = ui->lineEditExpr->text().trimmed();
    if (expr.isEmpty()) {
        return false;
    }
    static const QRegularExpression reSingle("^(.+)\\['(.+)'\\]$");
    static const QRegularExpression reDouble("^(.+)\\[\"(.+)\"\\]$");
    QString dataName, colName;
    QRegularExpressionMatch m = reSingle.match(expr);
    if (!m.hasMatch()) {
        m = reDouble.match(expr);
    }
    if (!m.hasMatch()) {
        return false;
    }
    dataName = m.captured(1).trimmed();
    colName  = m.captured(2);
    if (!mDataMgr) {
        return false;
    }
    DAData data = mDataMgr->findData(dataName);
    if (data.isNull() || !data.isDataFrame()) {
        return false;
    }
#if DA_ENABLE_PYTHON
    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        return false;
    }
    QList< QString > cols = df.columns();
    if (!cols.contains(colName)) {
        return false;
    }
#else
    return false;
#endif
    outData        = data;
    outSeriesName  = colName;
    return true;
}

DAChartSeriesPickerWidget::Role DAChartSeriesPickerWidget::getRole() const
{
    return mRole;
}

/**
 * @brief 开始选择：raise 数据窗口，连接点击信号
 */
void DAChartSeriesPickerWidget::startPick()
{
    mPickActive = true;
    mConfirmed  = false;
    ui->lineEditExpr->clear();
    connectDataWidgets();
    // raise 数据管理窗口和数据操作窗口
    if (auto* dmw = findDataManageWidget()) {
        raiseDockWidget(dmw);
    }
    if (auto* dow = findDataOperateWidget()) {
        raiseDockWidget(dow);
    }
    show();
    raise();
    activateWindow();
    ui->lineEditExpr->setFocus();
}

/**
 * @brief 结束选择：断开连接，隐藏窗口
 */
void DAChartSeriesPickerWidget::finishPick()
{
    mPickActive = false;
    disconnectDataWidgets();
    hide();
}

void DAChartSeriesPickerWidget::closeEvent(QCloseEvent* event)
{
    if (mPickActive && !mConfirmed) {
        mPickActive = false;
        disconnectDataWidgets();
        Q_EMIT canceled();
    }
    QWidget::closeEvent(event);
}

void DAChartSeriesPickerWidget::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        retranslateUi();
        break;
    default:
        break;
    }
}

/**
 * @brief 数据管理树单击 series 时填充表达式
 * @param data series 所属 dataframe
 * @param seriesName series 名
 */
void DAChartSeriesPickerWidget::onDataManageSeriesClicked(const DAData& data, const QString& seriesName)
{
    ui->lineEditExpr->setText(buildExpression(data, seriesName));
}

/**
 * @brief 表格表头点击时填充表达式
 * @param logicalIndex 列逻辑索引
 */
void DAChartSeriesPickerWidget::onDataOperateHeaderClicked(int logicalIndex)
{
    auto* dow = findDataOperateWidget();
    if (!dow) {
        return;
    }
    DADataOperateOfDataFrameWidget* dfw = dow->getCurrentDataFrameWidget();
    if (!dfw) {
        return;
    }
    DAData data = dfw->data();
    if (data.isNull() || !data.isDataFrame()) {
        return;
    }
#if DA_ENABLE_PYTHON
    DAPyDataFrame df = data.toDataFrame();
    if (df.isNone()) {
        return;
    }
    QList< QString > cols = df.columns();
    if (logicalIndex < 0 || logicalIndex >= cols.size()) {
        return;
    }
    QString colName = cols.at(logicalIndex);
    ui->lineEditExpr->setText(buildExpression(data, colName));
#endif
}

/**
 * @brief 点击"回到添加绘图"按钮
 *
 * 解析表达式，失败则警告，成功则发 seriesConfirmed 信号并隐藏窗口
 */
void DAChartSeriesPickerWidget::onButtonReturnClicked()
{
    DAData data;
    QString seriesName;
    if (!resolveExpression(data, seriesName)) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("Cannot resolve expression '%1', expected format: data['column']").arg(ui->lineEditExpr->text())  // cn:无法解析表达式'%1'，期望格式: data['列名']
        );
        return;
    }
    mConfirmed  = true;
    mPickActive = false;
    disconnectDataWidgets();
    Q_EMIT seriesConfirmed(data, seriesName);
    hide();
}

/**
 * @brief 连接数据管理/操作窗口的点击信号
 */
void DAChartSeriesPickerWidget::connectDataWidgets()
{
    disconnectDataWidgets();
    if (auto* dmw = findDataManageWidget()) {
        mConnTreeClicked = connect(dmw, &DADataManageWidget::dataSeriesClicked, this, &DAChartSeriesPickerWidget::onDataManageSeriesClicked);
    }
    if (auto* dow = findDataOperateWidget()) {
        mConnHeaderClicked = connect(dow, &DADataOperateWidget::currentDataFrameColumnHeaderClicked, this, &DAChartSeriesPickerWidget::onDataOperateHeaderClicked);
    }
}

/**
 * @brief 断开数据管理/操作窗口的点击信号连接
 */
void DAChartSeriesPickerWidget::disconnectDataWidgets()
{
    if (mConnTreeClicked) {
        disconnect(mConnTreeClicked);
        mConnTreeClicked = QMetaObject::Connection();
    }
    if (mConnHeaderClicked) {
        disconnect(mConnHeaderClicked);
        mConnHeaderClicked = QMetaObject::Connection();
    }
}

/**
 * @brief 查找主窗口中的 DADataManageWidget
 * @return 找不到返回 nullptr
 */
DADataManageWidget* DAChartSeriesPickerWidget::findDataManageWidget() const
{
    const auto tops = QApplication::topLevelWidgets();
    for (QWidget* top : tops) {
        if (auto* w = top->findChild< DADataManageWidget* >()) {
            return w;
        }
    }
    return nullptr;
}

/**
 * @brief 查找主窗口中的 DADataOperateWidget
 * @return 找不到返回 nullptr
 */
DADataOperateWidget* DAChartSeriesPickerWidget::findDataOperateWidget() const
{
    const auto tops = QApplication::topLevelWidgets();
    for (QWidget* top : tops) {
        if (auto* w = top->findChild< DADataOperateWidget* >()) {
            return w;
        }
    }
    return nullptr;
}

/**
 * @brief 构造 pandas 提取序列的表达式
 * @param data dataframe 数据
 * @param seriesName series 名
 * @return 形如 data['B'] 的表达式
 */
QString DAChartSeriesPickerWidget::buildExpression(const DAData& data, const QString& seriesName) const
{
    return QString("%1['%2']").arg(data.getName(), seriesName);
}

void DAChartSeriesPickerWidget::retranslateUi()
{
    setWindowTitle(tr("Select Series"));  // cn:选择序列
    ui->labelRole->setText(roleLabel());
    ui->toolButtonReturn->setToolTip(tr("Return to add chart"));  // cn:回到添加绘图
}

QString DAChartSeriesPickerWidget::roleLabel() const
{
    switch (mRole) {
    case RoleX:
        return tr("X:");  // cn:X:
    case RoleY:
        return tr("Y:");  // cn:Y:
    }
    return QString();
}

/**
 * @brief raise 一个 widget 所在的 dock widget
 *
 * 遍历父链查找 ads::CDockWidget，找到则 raise 该 dock（会激活其所在的 tab），
 * 同时对 widget 本身调用 activateWindow。
 * @param w 目标 widget（如 DADataManageWidget）
 */
void DAChartSeriesPickerWidget::raiseDockWidget(QWidget* w)
{
    if (!w) {
        return;
    }
    QWidget* p = w->parentWidget();
    while (p) {
        if (auto* dock = qobject_cast< ads::CDockWidget* >(p)) {
            dock->raise();
            break;
        }
        p = p->parentWidget();
    }
    w->activateWindow();
}

}  // namespace DA
