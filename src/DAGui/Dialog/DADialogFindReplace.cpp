#include "DADialogFindReplace.h"
#include "ui_DADialogFindReplace.h"
#include <QTimer>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QKeyEvent>
#include <QCloseEvent>

namespace DA
{

/**
 * @brief 去抖间隔（毫秒）
 *
 * 硬约束①（§4.2）：输入去抖 ≥300ms，否则输入 "abc" 会触发 3 次全表扫描。取 350ms。
 * @return 毫秒
 */
int DADialogFindReplace::debounceIntervalMs()
{
    return 350;
}

/**
 * @brief 构造非模态浮动查找/替换条
 *
 * Qt::Tool + 父窗口 → 跟随主窗口、非模态、不进任务栏。替换行默认隐藏，
 * activate(true) 时展开（Ctrl+H 语义）。范围下拉填「全表 / 当前选区」两项。
 * @param parent 父窗口（一般为数据页 widget）
 */
DADialogFindReplace::DADialogFindReplace(QWidget* parent) : QDialog(parent), ui(new Ui::DADialogFindReplace)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::Tool);
    setModal(false);
    // 范围下拉：0=全表，1=当前选区
    ui->comboBoxScope->addItem(tr("Entire table"));   // cn:全表
    ui->comboBoxScope->addItem(tr("Current selection"));  // cn:当前选区
    ui->comboBoxScope->setCurrentIndex(0);
    ui->widgetReplace->setVisible(false);
    ui->labelCount->clear();

    // 去抖：输入/选项变化后 singleShot + restart，到期才发 searchRequested（硬约束①）
    mDebounceTimer = new QTimer(this);
    mDebounceTimer->setSingleShot(true);
    mDebounceTimer->setInterval(debounceIntervalMs());
    connect(mDebounceTimer, &QTimer::timeout, this, [ this ]() {
        Q_EMIT searchRequested();
    });
    connect(ui->lineEditFind, &QLineEdit::textChanged, this, &DADialogFindReplace::onInputChanged);
    connect(ui->checkBoxCase, &QCheckBox::toggled, this, &DADialogFindReplace::onInputChanged);
    connect(ui->checkBoxRegex, &QCheckBox::toggled, this, &DADialogFindReplace::onInputChanged);
    connect(ui->checkBoxWholeCell, &QCheckBox::toggled, this, &DADialogFindReplace::onInputChanged);
    connect(ui->comboBoxScope, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DADialogFindReplace::onInputChanged);
    // 回车 = 下一个（无命中时由页面先执行查找再导航）
    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, [ this ]() {
        Q_EMIT nextRequested();
    });
    connect(ui->pushButtonNext, &QPushButton::clicked, this, [ this ]() {
        Q_EMIT nextRequested();
    });
    connect(ui->pushButtonPrevious, &QPushButton::clicked, this, [ this ]() {
        Q_EMIT previousRequested();
    });
    connect(ui->pushButtonReplace, &QPushButton::clicked, this, [ this ]() {
        Q_EMIT replaceRequested();
    });
    connect(ui->pushButtonReplaceAll, &QPushButton::clicked, this, [ this ]() {
        Q_EMIT replaceAllRequested();
    });
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &DADialogFindReplace::close);
}

DADialogFindReplace::~DADialogFindReplace()
{
    delete ui;
}

/**
 * @brief 输入/选项变化：重启去抖定时器（singleShot + restart，硬约束①）
 */
void DADialogFindReplace::onInputChanged()
{
    mDebounceTimer->start();  // 重复调用即 restart
}

/**
 * @brief 立即触发一次查找（跳过去抖：显式动作如重新打开时）
 */
void DADialogFindReplace::requestSearchNow()
{
    mDebounceTimer->stop();
    Q_EMIT searchRequested();
}

/**
 * @brief 重启去抖定时（外部输入变化用：选区范围模式下选区变化触发的延迟重扫）
 */
void DADialogFindReplace::restartDebounce()
{
    mDebounceTimer->start();
}

/**
 * @brief 唤出并聚焦输入框
 * @param expandReplace true 展开替换行（Ctrl+H），false 仅查找行（Ctrl+F）
 */
void DADialogFindReplace::activate(bool expandReplace)
{
    ui->widgetReplace->setVisible(expandReplace);
    show();
    raise();
    activateWindow();
    ui->lineEditFind->setFocus();
    ui->lineEditFind->selectAll();
}

/**
 * @brief 设置查找关键字
 *
 * 右键「查找/替换」入口预填当前单元格内容用；textChanged 连去抖定时器，
 * 设值后自动走一次延迟扫描（与手输同链路）。
 * @param text 关键字
 */
void DADialogFindReplace::setFindText(const QString& text)
{
    if (ui->lineEditFind->text() != text) {
        ui->lineEditFind->setText(text);
    }
}

/**
 * @brief 查找输入框当前文本
 * @return 文本
 */
QString DADialogFindReplace::findText() const
{
    return ui->lineEditFind->text();
}

/**
 * @brief 当前参数快照
 * @return 查找/替换选项
 */
DADialogFindReplace::FindOptions DADialogFindReplace::options() const
{
    FindOptions opt;
    opt.keyword        = ui->lineEditFind->text();
    opt.replaceText    = ui->lineEditReplace->text();
    opt.caseSensitive  = ui->checkBoxCase->isChecked();
    opt.useRegex       = ui->checkBoxRegex->isChecked();
    opt.wholeCell      = ui->checkBoxWholeCell->isChecked();
    opt.selectionOnly  = (ui->comboBoxScope->currentIndex() == 1);
    return opt;
}

/**
 * @brief 更新计数显示
 * @param current 当前命中序号（1 起，<=0 表示无当前项）
 * @param shown 已高亮条数
 * @param totalHits 真实命中总数（truncated 时 > shown）
 * @param truncated 是否因上限截断（硬约束③提示）
 */
void DADialogFindReplace::setHitCount(int current, int shown, int totalHits, bool truncated)
{
    if (shown <= 0) {
        ui->labelCount->setText(tr("No matches"));  // cn:无匹配
        return;
    }
    const int cur = qMax(1, current);
    if (truncated) {
        ui->labelCount->setText(tr("%1 of %2 (truncated from %3)")  // cn:第 %1 条 / 共 %2 条（命中过多，已从 %3 条截断）
                                    .arg(cur)
                                    .arg(shown)
                                    .arg(totalHits));
    } else {
        ui->labelCount->setText(tr("%1 of %2")  // cn:第 %1 条 / 共 %2 条
                                    .arg(cur)
                                    .arg(shown));
    }
}

/**
 * @brief 扫描失败提示（如无效正则）
 * @param failed true 显示错误
 */
void DADialogFindReplace::setSearchFailed(bool failed)
{
    if (failed) {
        ui->labelCount->setText(tr("Invalid pattern"));  // cn:无效的匹配模式
    }
}

/**
 * @brief Esc 关闭（并让页面清查找高亮，经 closeEvent → closed）
 * @param event 按键事件
 */
void DADialogFindReplace::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

/**
 * @brief 关闭事件：发 closed（页面据此清 Source::Find 高亮，Esc/关闭按钮/程序关闭统一出口）
 * @param event 关闭事件
 */
void DADialogFindReplace::closeEvent(QCloseEvent* event)
{
    mDebounceTimer->stop();
    Q_EMIT closed();
    QDialog::closeEvent(event);
}

}  // end of namespace DA
