#include "DAChartAddXYSeriesWidget.h"
#include <QMessageBox>
#include <QDialog>
#include <QHeaderView>
#include "ui_DAChartAddXYSeriesWidget.h"
// DA
#include "DADataManager.h"
#include "DAAutoincrementSeries.hpp"
#include "DALogCategory.h"
#include "Models/DAPySeriesTableModel.h"
#include "pandas/DAPyDataFrame.h"
#include "pandas/DAPySeries.h"
namespace DA
{

//===================================================
// DAChartAddXYSeriesWidget
//===================================================

DAChartAddXYSeriesWidget::DAChartAddXYSeriesWidget(QWidget* parent)
    : DAAbstractChartAddItemWidget(parent), ui(new Ui::DAChartAddXYSeriesWidget)
{
    ui->setupUi(this);
    DAPySeriesTableModel* model = new DAPySeriesTableModel(this);
    model->setHeaderLabel({ tr("x"),  // cn:x
                            tr("y")   // cn:y
                          });
    ui->tableViewXY->setModel(model);
    QFontMetrics fm = fontMetrics();
    ui->tableViewXY->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.1);
    ui->listViewX->setAcceptMode(DAPySeriesListView::AcceptOneSeries);
    // y 支持多选（1 个 x + 多个 y 一次绘制多条曲线）
    ui->listViewY->setAcceptMode(DAPySeriesListView::AcceptOneDataframeMultSeries);
    // x 只允许一个序列：拖入多个时仅保留第一个
    connect(ui->listViewX, &DAPySeriesListView::excessSeriesIgnored, this, [ this ](int) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("Only one series can be used as x, the extra series were ignored")  // cn:x只能添加一个序列，多余的序列已被忽略
        );
    });
    connect(ui->groupBoxXAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYSeriesWidget::onGroupBoxXAutoincrementClicked);
    connect(ui->groupBoxYAutoincrement, &QGroupBox::clicked, this, &DAChartAddXYSeriesWidget::onGroupBoxYAutoincrementClicked);
    connect(ui->listViewX, &DAPySeriesListView::seriesChanged, this, &DAChartAddXYSeriesWidget::onXSeriesChanged);
    connect(ui->listViewY, &DAPySeriesListView::seriesChanged, this, &DAChartAddXYSeriesWidget::onYSeriesChanged);
    connect(ui->toolButtonRemoveFromX, &QToolButton::clicked, this, &DAChartAddXYSeriesWidget::onButtonXRemoveClicked);
    connect(ui->toolButtonRemoveFromY, &QToolButton::clicked, this, &DAChartAddXYSeriesWidget::onButtonYRemoveClicked);
    connect(ui->toolButtonAddToX, &QToolButton::clicked, this, &DAChartAddXYSeriesWidget::onButtonXAddClicked);
    connect(ui->toolButtonAddToY, &QToolButton::clicked, this, &DAChartAddXYSeriesWidget::onButtonYAddClicked);
}

DAChartAddXYSeriesWidget::~DAChartAddXYSeriesWidget()
{
    delete ui;
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

void DAChartAddXYSeriesWidget::setDataManager(DADataManager* dmgr)
{
    DAAbstractChartAddItemWidget::setDataManager(dmgr);
    ui->listViewX->setDataManager(dmgr);
    ui->listViewY->setDataManager(dmgr);
}

/**
 * @brief 根据配置获取单条点列
 * @return 失败或没有符合条件时返回空数组
 * @note 本函数不弹窗、不写界面，失败原因请使用 extractSeries()
 */
QVector< QPointF > DAChartAddXYSeriesWidget::getSeries() const
{
    return extractSeries().points;
}

/**
 * @brief 提取单条 xy 序列
 * @return 点列与失败原因
 */
DAChartSeriesResult DAChartAddXYSeriesWidget::extractSeries() const
{
    DAChartXYSeriesSpec spec;
    DAChartSeriesDataError error;
    if (!buildSeriesSpec(spec, error)) {
        DAChartSeriesResult res;
        res.error = error;
        return res;
    }
    return extractXYSeries(spec);
}

/**
 * @brief 提取多条 xy 序列（1 个 x + 多个 y）
 * @return 点列、命名与失败原因
 */
DAChartMultiSeriesResult DAChartAddXYSeriesWidget::extractMultiSeries() const
{
    DAChartXYSeriesSpec spec;
    DAChartSeriesDataError error;
    if (!buildSeriesSpec(spec, error)) {
        DAChartMultiSeriesResult res;
        res.error = error;
        return res;
    }
    return extractMultiXYSeries(spec);
}

/**
 * @brief 从界面收集取数输入
 * @param spec 输出：取数输入
 * @param error 输出：自增数值解析失败时的失败原因
 * @return 输入完整且合法返回 true
 */
bool DAChartAddXYSeriesWidget::buildSeriesSpec(DAChartXYSeriesSpec& spec, DAChartSeriesDataError& error) const
{
    spec = DAChartXYSeriesSpec();
    spec.xAutoincrement = isXAutoincrement();
    spec.yAutoincrement = isYAutoincrement();
    if (spec.xAutoincrement && spec.yAutoincrement) {
        error.code = DAChartSeriesDataError::BothAutoincrement;
        return false;
    }
    const QPair< DAData, QString > x = getX();
    spec.xData                       = x.first;
    spec.xColumn                     = x.second;
    if (!spec.yAutoincrement) {
        spec.ySeries = getYList();
    }
    bool isOK = false;
    if (spec.xAutoincrement) {
        spec.xBase = ui->lineEditXInitValue->text().toDouble(&isOK);
        if (!isOK) {
            error.code = DAChartSeriesDataError::InvalidXInitialValue;
            return false;
        }
        spec.xStep = ui->lineEditXStepValue->text().toDouble(&isOK);
        if (!isOK) {
            error.code = DAChartSeriesDataError::InvalidXStepValue;
            return false;
        }
    }
    if (spec.yAutoincrement) {
        spec.yBase = ui->lineEditYInitValue->text().toDouble(&isOK);
        if (!isOK) {
            error.code = DAChartSeriesDataError::InvalidYInitialValue;
            return false;
        }
        spec.yStep = ui->lineEditYStepValue->text().toDouble(&isOK);
        if (!isOK) {
            error.code = DAChartSeriesDataError::InvalidYStepValue;
            return false;
        }
    }
    return true;
}

/**
 * @brief 把取数失败原因翻译成界面文案
 * @param error 失败原因
 * @return 已翻译的提示文本，没有错误时返回空串
 */
QString DAChartAddXYSeriesWidget::describeSeriesError(const DAChartSeriesDataError& error) const
{
    switch (error.code) {
    case DAChartSeriesDataError::NoError:
        return QString();
    case DAChartSeriesDataError::BothAutoincrement:
        return tr("x and y cannot be set to autoincrement at the same time");  // cn:x和y无法同时设置为自增
    case DAChartSeriesDataError::InvalidXInitialValue:
        return tr("The initial value of x auto increment series "
                  "must be a floating-point arithmetic number");  // cn:x自增序列的初始值必须为浮点数
    case DAChartSeriesDataError::InvalidXStepValue:
        return tr("The step value of x auto increment series "
                  "must be a floating-point arithmetic number");  // cn:x自增序列的步长必须为浮点数
    case DAChartSeriesDataError::InvalidYInitialValue:
        return tr("The initial value of y auto increment series "
                  "must be a floating-point arithmetic number");  // cn:y自增序列的初始值必须为浮点数
    case DAChartSeriesDataError::InvalidYStepValue:
        return tr("The step value of y auto increment series "
                  "must be a floating-point arithmetic number");  // cn:y自增序列的步长必须为浮点数
    case DAChartSeriesDataError::LengthMismatch:
        return tr("The length of y series '%1' (%2) does not match the length of x series "
                  "'%3' (%4). All y series must have the same length as x")
            .arg(error.seriesName)
            .arg(error.seriesSize)
            .arg(error.xSeriesName)
            .arg(error.xSize);  // cn:y序列'%1'的长度(%2)与x序列'%3'的长度(%4)不一致，所有y序列的长度必须与x一致
    case DAChartSeriesDataError::ConversionFailed:
        return tr("Exception occurred during extracting from "
                  "pandas.Series to double vector");  // cn:从 pandas.Series 提取为 double vector 过程中出现异常
    case DAChartSeriesDataError::NoXSeries:
    case DAChartSeriesDataError::NoYSeries:
    case DAChartSeriesDataError::SeriesNotConvertible:
        return tr("The None value cannot be converted to a series");  // cn:None值无法转换为序列
    }
    return QString();
}

/**
 * @brief 弹出取数失败警告
 * @param error 失败原因
 * @note 转换类失败同时写一条 daCritical 日志，便于排查具体异常
 */
void DAChartAddXYSeriesWidget::warnSeriesError(const DAChartSeriesDataError& error)
{
    if (DAChartSeriesDataError::ConversionFailed == error.code) {
        daCritical << tr("Exception occurred during extracting from "
                         "pandas.Series to double vector:%1")
                          .arg(error.detail);  // cn:从 pandas.Series 提取为 double vector 过程中出现异常：%1
    }
    QMessageBox::warning(this,
                         tr("Warning"),  // cn:警告
                         describeSeriesError(error));
}

/**
 * @brief 推荐的名字使用y值的参数名
 * @return
 */
QString DAChartAddXYSeriesWidget::getNameHint() const
{
    return getY().second;
}

/**
 * @brief 设置当前的x到list中
 * @param data
 * @param seriesName
 */
void DAChartAddXYSeriesWidget::setX(const DAData& dataframeData, const QString& seriesName)
{
    ui->listViewX->addSeries(dataframeData, seriesName);
}

/**
 * @brief 设置当前的y到list中
 * @param data
 * @param seriesName
 */
void DAChartAddXYSeriesWidget::setY(const DAData& dataframeData, const QString& seriesName)
{
    ui->listViewY->addSeries(dataframeData, seriesName);
}

/**
 * @brief x值是否使用自增序列
 * @param on
 */
void DAChartAddXYSeriesWidget::onGroupBoxXAutoincrementClicked(bool on)
{
    Q_UNUSED(on);
    updateTable();
}

/**
 * @brief y值是否使用自增序列
 * @param on
 */
void DAChartAddXYSeriesWidget::onGroupBoxYAutoincrementClicked(bool on)
{
    Q_UNUSED(on);
    updateTable();
}

void DAChartAddXYSeriesWidget::onXSeriesChanged()
{
    updateTable();
}

void DAChartAddXYSeriesWidget::onYSeriesChanged()
{
    updateTable();
}

void DAChartAddXYSeriesWidget::updateTable()
{
    ui->tableViewXY->clear();
    if (ui->groupBoxXAutoincrement->isChecked()) {
        double base, step;
        if (tryGetXSelfInc(base, step)) {
            ui->tableViewXY->setSeriesAt(0, DAAutoincrementSeries< double >(base, step));
        }
    } else {
        DAPySeries x = getXSeries();
        if (!x.isNone()) {
            ui->tableViewXY->setSeriesAt(0, x);
        }
    }
    // y 支持多列：第一列落到 1 号位，其余依次追加
    if (ui->groupBoxYAutoincrement->isChecked()) {
        double base, step;
        if (tryGetYSelfInc(base, step)) {
            ui->tableViewXY->setSeriesAt(1, DAAutoincrementSeries< double >(base, step));
        }
    } else {
        const QList< DAPySeries > ys = getYSeriesList();
        for (int i = 0; i < ys.size(); ++i) {
            if (!ys[ i ].isNone()) {
                ui->tableViewXY->setSeriesAt(1 + i, ys[ i ]);
            }
        }
    }
}

void DAChartAddXYSeriesWidget::onButtonXRemoveClicked()
{
    ui->listViewX->removeCurrentSelect();
}

void DAChartAddXYSeriesWidget::onButtonYRemoveClicked()
{
    ui->listViewY->removeCurrentSelect();
}

/**
 * @brief 点击 X 旁的添加按钮
 *
 * 隐藏父级绘图引导对话框，弹出"选择序列"窗口供用户拾取 X 序列
 */
void DAChartAddXYSeriesWidget::onButtonXAddClicked()
{
    DAChartSeriesPickerWidget* picker = ensurePicker(mPickerX, DAChartSeriesPickerWidget::RoleX);
    hideParentGuideDialog();
    picker->startPick();
}

/**
 * @brief 点击 Y 旁的添加按钮
 *
 * 隐藏父级绘图引导对话框，弹出"选择序列"窗口供用户拾取 Y 序列
 */
void DAChartAddXYSeriesWidget::onButtonYAddClicked()
{
    DAChartSeriesPickerWidget* picker = ensurePicker(mPickerY, DAChartSeriesPickerWidget::RoleY);
    hideParentGuideDialog();
    picker->startPick();
}

/**
 * @brief 懒创建"选择序列"窗口并连接信号
 * @param picker 成员指针引用
 * @param role 选择目标角色
 * @return picker 指针
 */
DAChartSeriesPickerWidget* DAChartAddXYSeriesWidget::ensurePicker(DAChartSeriesPickerWidget*& picker, DAChartSeriesPickerWidget::Role role)
{
    if (!picker) {
        picker = new DAChartSeriesPickerWidget(getDataManager(), role, this);
        connect(picker, &DAChartSeriesPickerWidget::seriesConfirmed, this, [ this, role ](const DAData& data, const QString& name) {
            if (role == DAChartSeriesPickerWidget::RoleX) {
                setX(data, name);
            } else {
                setY(data, name);
            }
            showParentGuideDialog();
        });
        connect(picker, &DAChartSeriesPickerWidget::canceled, this, &DAChartAddXYSeriesWidget::showParentGuideDialog);
    } else {
        picker->setExpression(QString());
    }
    return picker;
}

/**
 * @brief 隐藏父级绘图引导对话框（DADialogChartGuide）
 *
 * DAChartAddXYSeriesWidget 通常嵌入在 DADialogChartGuide 中，
 * 弹出"选择序列"窗口前需隐藏父对话框以解除模态阻塞，让用户能操作主窗口的 dock。
 */
void DAChartAddXYSeriesWidget::hideParentGuideDialog()
{
    if (auto* dlg = qobject_cast< QDialog* >(window())) {
        dlg->hide();
    }
}

/**
 * @brief 显示父级绘图引导对话框
 */
void DAChartAddXYSeriesWidget::showParentGuideDialog()
{
    if (auto* dlg = qobject_cast< QDialog* >(window())) {
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }
}

/**
 * @brief 尝试获取x值的自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYSeriesWidget::tryGetXSelfInc(double& base, double& step) const
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
 * @brief 尝试获取y值的自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYSeriesWidget::tryGetYSelfInc(double& base, double& step) const
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

QPair< DAData, QString > DAChartAddXYSeriesWidget::getY() const
{
    QList< QPair< DAData, QStringList > > datas = ui->listViewY->getSeries();
    return getFirstValue(datas);
}

QPair< DAData, QString > DAChartAddXYSeriesWidget::getX() const
{
    QList< QPair< DAData, QStringList > > datas = ui->listViewX->getSeries();
    return getFirstValue(datas);
}

DAPySeries DAChartAddXYSeriesWidget::getYSeries() const
{
    QPair< DAData, QString > y = getY();
    if (y.first.isNull()) {
        return DAPySeries();
    }
    DAPyDataFrame df = y.first.toDataFrame();
    if (df.isNone()) {
        return DAPySeries();
    }
    return df[ y.second ];
}

DAPySeries DAChartAddXYSeriesWidget::getXSeries() const
{
    QPair< DAData, QString > x = getX();
    if (x.first.isNull()) {
        return DAPySeries();
    }
    DAPyDataFrame df = x.first.toDataFrame();
    if (df.isNone()) {
        return DAPySeries();
    }
    return df[ x.second ];
}

QPair< DAData, QString > DAChartAddXYSeriesWidget::getFirstValue(const QList< QPair< DAData, QStringList > >& datas) const
{
    if (datas.empty()) {
        return qMakePair(DAData(), QString());
    }
    auto data = datas.first();
    if (data.second.empty()) {
        return qMakePair(DAData(), QString());
    }
    return qMakePair(data.first, data.second.first());
}

/**
 * @brief 移除一个已添加的 y 序列
 * @param dataframeData y 所属 dataframe
 * @param seriesName 列名
 */
void DAChartAddXYSeriesWidget::removeY(const DAData& dataframeData, const QString& seriesName)
{
    ui->listViewY->removeSeries(dataframeData, seriesName);
}

/**
 * @brief 获取全部已选的 y（保持添加顺序）
 * @return (dataframe, 列名) 列表
 */
QList< QPair< DAData, QString > > DAChartAddXYSeriesWidget::getYList() const
{
    QList< QPair< DAData, QString > > res;
    const QList< QPair< DAData, QStringList > > datas = ui->listViewY->getSeries();
    for (const auto& d : datas) {
        for (const QString& name : d.second) {
            res.append(qMakePair(d.first, name));
        }
    }
    return res;
}

/**
 * @brief 获取当前 xy 的来源信息
 * @return x 一条（自增时列名为空），y 多条（自增时列表为一个空列名项）
 */
DAChartXYItemInfo DAChartAddXYSeriesWidget::getXYInfo() const
{
    DAChartXYItemInfo info;
    info.x = getX();
    if (isYAutoincrement()) {
        info.y.append(qMakePair(DAData(), QString()));
    } else {
        info.y = getYList();
    }
    return info;
}

/**
 * @brief 获取列表中全部 y 对应的 DAPySeries（保持添加顺序，含无效项）
 */
QList< DAPySeries > DAChartAddXYSeriesWidget::getYSeriesList() const
{
    QList< DAPySeries > res;
    const QList< QPair< DAData, QString > > ys = getYList();
    res.reserve(ys.size());
    for (const auto& p : ys) {
        if (p.first.isNull()) {
            res.append(DAPySeries());
            continue;
        }
        DAPyDataFrame df = p.first.toDataFrame();
        if (df.isNone()) {
            res.append(DAPySeries());
            continue;
        }
        res.append(df[ p.second ]);
    }
    return res;
}

/**
 * @brief 坐标轴标题推荐
 *
 * x 轴：选了 x 序列用其列名，x 自增时为 "x"；
 * y 轴：单个 y 序列用其列名，多个 y 时统一为 "y"，y 自增时为 "y"
 */
bool DAChartAddXYSeriesWidget::getAxisTitleHint(QString& xTitle, QString& yTitle) const
{
    xTitle = QStringLiteral("x");
    yTitle = QStringLiteral("y");
    if (!isXAutoincrement()) {
        QPair< DAData, QString > x = getX();
        if (!x.second.isEmpty()) {
            xTitle = x.second;
        }
    }
    if (!isYAutoincrement()) {
        const QList< QPair< DAData, QString > > ys = getYList();
        if (ys.size() == 1 && !ys.first().second.isEmpty()) {
            yTitle = ys.first().second;
        }
    }
    return true;
}

}
