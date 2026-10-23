#include "DAChartAddXYSeriesWidget.h"
#include <QMessageBox>
#include <QDialog>
#include "ui_DAChartAddXYSeriesWidget.h"
#include "DADataManager.h"
#include "Models/DADataManagerTreeModel.h"
#include "DALogCategory.h"
#include "DAChartSeriesPickerWidget.h"
#include <QHeaderView>
#include "Models/DAPySeriesTableModel.h"
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
 * @brief 根据配置获取数据
 * @return 如果没有符合条件，返回一个empty的vector
 */
QVector< QPointF > DAChartAddXYSeriesWidget::getSeries() const
{
    DAChartAddXYSeriesWidget* that = const_cast< DAChartAddXYSeriesWidget* >(this);
    QVector< QPointF > xy;
    that->getToVectorPointFFromUI(xy);
    return xy;
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
 * @brief 获取x自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYSeriesWidget::getXAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
    bool isOK   = false;
    double base = ui->lineEditXInitValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(
            this,
            tr("Warning"),                                                                                 // cn:警告
            tr("The initial value of x auto increment series must be a floating-point arithmetic number")  // cn:x自增序列的初始值必须为浮点数
        );
        return false;
    }
    double step = ui->lineEditXStepValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of x auto increment series "
                                "must be a floating-point arithmetic number")  // cn:x自增序列的步长必须为浮点数
        );
        return false;
    }
    v.setBaseValue(base);
    v.setStepValue(step);
    return true;
}

/**
 * @brief 获取y自增
 * @param v
 * @return 成功返回true
 * @note 注意此函数失败会有警告对话框
 */
bool DAChartAddXYSeriesWidget::getYAutoIncFromUI(DAAutoincrementSeries< double >& v)
{
    bool isOK   = false;
    double base = ui->lineEditYInitValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The initial value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:y自增序列的初始值必须为浮点数
        );
        return false;
    }
    double step = ui->lineEditYStepValue->text().toDouble(&isOK);
    if (!isOK) {
        QMessageBox::warning(this,
                             tr("Warning"),  // cn:警告
                             tr("The step value of y auto increment series "
                                "must be a floating-point arithmetic number")  // cn:y自增序列的步长必须为浮点数
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
bool DAChartAddXYSeriesWidget::getToVectorPointFFromUI(QVector< QPointF >& res)
{
    bool isXAuto = ui->groupBoxXAutoincrement->isChecked();
    bool isYAuto = ui->groupBoxYAutoincrement->isChecked();
    if (isXAuto && isYAuto) {
        QMessageBox::warning(this,
                             tr("Warning"),                                                 // cn:警告
                             tr("x and y cannot be set to autoincrement at the same time")  // cn:x和y无法同时设置为自增
        );
        return false;
    }
    if (isXAuto) {  // 不存在同时，因此这个就是x自增
        DAAutoincrementSeries< double > xinc;
        if (!getXAutoIncFromUI(xinc)) {
            return false;
        }
        DAPySeries y = getYSeries();
        if (y.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        const int s = static_cast< int >(y.size());
        try {
            std::vector< double > vy;
            vy.reserve(y.size());
            y.castTo< double >(std::back_inserter(vy));
            res.resize(static_cast< int >(s));
            for (int i = 0; i < s; ++i) {
                res[ i ].setX(xinc[ i ]);
                res[ i ].setY(vy[ i ]);
            }
        } catch (const std::exception& e) {
            daCritical << tr("Exception occurred during extracting from "
                             "pandas.Series to double vector:%1")
                              .arg(e.what());  // cn:从 pandas.Series 提取为 double vector 过程中出现异常：%1
            QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从 pandas.Series 提取为 double vector 过程中出现异常

            return false;
        }
    } else if (isYAuto) {
        DAAutoincrementSeries< double > yinc;
        if (!getYAutoIncFromUI(yinc)) {
            return false;
        }
        DAPySeries x = getXSeries();
        if (x.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        std::size_t s = x.size();
        try {
            std::vector< double > vx;
            vx.reserve(x.size());
            x.castTo< double >(std::back_inserter(vx));
            res.resize(static_cast< int >(s));
            for (auto i = 0; i < s; ++i) {
                res[ i ].setX(vx[ i ]);
                res[ i ].setY(yinc[ i ]);
            }
        } catch (const std::exception& e) {
            daCritical << tr("Exception occurred during extracting from "
                             "pandas.Series to double vector:%1")
                              .arg(e.what());  // cn:从 pandas.Series 提取为 double vector 过程中出现异常：%1
            QMessageBox::warning(this,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从 pandas.Series 提取为 double vector 过程中出现异常

            return false;
        }
    } else {
        DAPySeries x = getXSeries();
        DAPySeries y = getYSeries();
        if (x.isNone() || y.isNone()) {
            QMessageBox::warning(this,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        std::size_t s = std::min(x.size(), y.size());
        if (0 == s) {
            return true;
        }
        try {
            std::vector< qreal > vx, vy;
            vx.reserve(x.size());
            vy.reserve(y.size());
            x.castTo< qreal >(std::back_inserter(vx));
            y.castTo< qreal >(std::back_inserter(vy));
            res.resize(static_cast< int >(s));
            for (auto i = 0; i < s; ++i) {
                res[ i ].setX(vx[ i ]);
                res[ i ].setY(vy[ i ]);
            }
        } catch (const std::exception& e) {
            daCritical << tr("Exception occurred during extracting from pandas.Series to double vector:%1")
                              .arg(e.what());  // cn:从 pandas.Series 提取为 double vector 过程中出现异常：%1
            return false;
        }
    }
    return true;
}

/**
 * @brief 尝试获取x值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYSeriesWidget::tryGetXSelfInc(double& base, double& step)
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
 * @brief 尝试获取y值得自增内容
 * @param base
 * @param step
 * @return
 */
bool DAChartAddXYSeriesWidget::tryGetYSelfInc(double& base, double& step)
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
 * @brief 批量获取多条 xy 序列（1 个 x + 多个 y）
 *
 * 每条 y 与 x 组成一组点列；x 为实际序列时要求所有 y 长度与 x 一致，
 * 不一致弹警告并返回 false。y 自增时长度取 x 长度。
 * @param res 输出：每组点列（与 y 一一对应）
 * @param names 输出：每条曲线的命名（y 列名，自增时为 "y"）
 * @return 成功返回 true；无数据返回 false（无警告框）
 */
bool DAChartAddXYSeriesWidget::getMultiSeries(QList< QVector< QPointF > >& res, QStringList& names) const
{
    res.clear();
    names.clear();
    const bool isXAuto = ui->groupBoxXAutoincrement->isChecked();
    const bool isYAuto = ui->groupBoxYAutoincrement->isChecked();
    DAChartAddXYSeriesWidget* that = const_cast< DAChartAddXYSeriesWidget* >(this);
    if (isXAuto && isYAuto) {
        QMessageBox::warning(that,
                             tr("Warning"),                                                 // cn:警告
                             tr("x and y cannot be set to autoincrement at the same time")  // cn:x和y无法同时设置为自增
        );
        return false;
    }
    const QList< QPair< DAData, QString > > ys = getYList();
    if (!isYAuto && ys.isEmpty()) {
        return false;
    }
    // x 数据
    DAAutoincrementSeries< double > xinc;
    std::vector< double > vx;
    std::size_t xSize = 0;
    if (isXAuto) {
        if (!that->getXAutoIncFromUI(xinc)) {
            return false;
        }
    } else {
        DAPySeries x = getXSeries();
        if (x.isNone()) {
            QMessageBox::warning(that,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        try {
            vx.reserve(x.size());
            x.castTo< double >(std::back_inserter(vx));
            xSize = vx.size();
        } catch (const std::exception& e) {
            daCritical << tr("Exception occurred during extracting from pandas.Series to double vector:%1")
                              .arg(e.what());  // cn:从 pandas.Series 提取为 double vector 过程中出现异常：%1
            QMessageBox::warning(that,
                                 tr("Warning"),  // cn:警告
                                 tr("Exception occurred during extracting from "
                                    "pandas.Series to double vector"));  // cn:从 pandas.Series 提取为 double vector 过程中出现异常
            return false;
        }
    }
    // y 自增：一条曲线，长度同 x
    if (isYAuto) {
        DAAutoincrementSeries< double > yinc;
        if (!that->getYAutoIncFromUI(yinc)) {
            return false;
        }
        const std::size_t s = xSize;  // x/y 无法同时自增，此处 x 一定非自增
        QVector< QPointF > xy(static_cast< int >(s));
        for (std::size_t i = 0; i < s; ++i) {
            xy[ static_cast< int >(i) ].setX(vx[ i ]);
            xy[ static_cast< int >(i) ].setY(yinc[ static_cast< int >(i) ]);
        }
        res.append(xy);
        names.append(QStringLiteral("y"));
        return true;
    }
    // 逐条 y 生成
    for (const auto& yp : ys) {
        if (yp.first.isNull()) {
            QMessageBox::warning(that,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        DAPyDataFrame df = yp.first.toDataFrame();
        if (df.isNone()) {
            QMessageBox::warning(that,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        DAPySeries y = df[ yp.second ];
        if (y.isNone()) {
            QMessageBox::warning(that,
                                 tr("Warning"),                                          // cn:警告
                                 tr("The None value cannot be converted to a series"));  // cn:None值无法转换为序列
            return false;
        }
        if (isXAuto) {
            // x 自增：长度按 y
            const int s = static_cast< int >(y.size());
            try {
                QVector< QPointF > xy(s);
                std::vector< double > vy;
                vy.reserve(y.size());
                y.castTo< double >(std::back_inserter(vy));
                for (int i = 0; i < s; ++i) {
                    xy[ i ].setX(xinc[ i ]);
                    xy[ i ].setY(vy[ static_cast< std::size_t >(i) ]);
                }
                res.append(xy);
                names.append(yp.second);
            } catch (const std::exception& e) {
                daCritical << tr("Exception occurred during extracting from pandas.Series to double vector:%1")
                                  .arg(e.what());  // cn:从 pandas.Series 提取为 double vector 过程中出现异常：%1
                QMessageBox::warning(that,
                                     tr("Warning"),  // cn:警告
                                     tr("Exception occurred during extracting from "
                                        "pandas.Series to double vector"));  // cn:从 pandas.Series 提取为 double vector 过程中出现异常
                return false;
            }
        } else {
            // x 为实际序列：y 长度必须与 x 一致
            if (y.size() != xSize) {
                QMessageBox::warning(that,
                                     tr("Warning"),  // cn:警告
                                     tr("The length of y series '%1' (%2) does not match the length of x series "
                                        "'%3' (%4). All y series must have the same length as x")
                                         .arg(yp.second)
                                         .arg(static_cast< qulonglong >(y.size()))
                                         .arg(getX().second)
                                         .arg(static_cast< qulonglong >(xSize))  // cn:y序列'%1'的长度(%2)与x序列'%3'的长度(%4)不一致，所有y序列的长度必须与x一致
                );
                return false;
            }
            try {
                QVector< QPointF > xy(static_cast< int >(xSize));
                std::vector< double > vy;
                vy.reserve(y.size());
                y.castTo< double >(std::back_inserter(vy));
                for (std::size_t i = 0; i < xSize; ++i) {
                    xy[ static_cast< int >(i) ].setX(vx[ i ]);
                    xy[ static_cast< int >(i) ].setY(vy[ i ]);
                }
                res.append(xy);
                names.append(yp.second);
            } catch (const std::exception& e) {
                daCritical << tr("Exception occurred during extracting from pandas.Series to double vector:%1")
                                  .arg(e.what());  // cn:从 pandas.Series 提取为 double vector 过程中出现异常：%1
                QMessageBox::warning(that,
                                     tr("Warning"),  // cn:警告
                                     tr("Exception occurred during extracting from "
                                        "pandas.Series to double vector"));  // cn:从 pandas.Series 提取为 double vector 过程中出现异常
                return false;
            }
        }
    }
    return true;
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

