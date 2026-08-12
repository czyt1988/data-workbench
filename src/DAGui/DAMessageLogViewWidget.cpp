#include "DAMessageLogViewWidget.h"
#include "ui_DAMessageLogViewWidget.h"
#include <QDebug>
#include <QMenu>
#include <QItemSelectionModel>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include "Models/DAMessageLogsModel.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAMessageLogViewWidget
//===================================================
DAMessageLogViewWidget::DAMessageLogViewWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAMessageLogViewWidget), mMenu(nullptr)
{
    ui->setupUi(this);
    // 创建action
    mActionMessageLogShowInfo     = createAction("actionMessageLogShowInfo",
                                             ":/DAGui/MessageType/icon/messageType/messageTypeInfo.svg",
                                             true,
                                             true);
    mActionMessageLogShowWarning  = createAction("actionMessageLogShowWarning",
                                                ":/DAGui/MessageType/icon/messageType/messageTypeWarning.svg",
                                                true,
                                                true);
    mActionMessageLogShowCritical = createAction("actionMessageLogShowCritical",
                                                 ":/DAGui/MessageType/icon/messageType/messageTypeError.svg",
                                                 true,
                                                 true);
    mActionMessageLogClear        = createAction("actionMessageLogClear", ":/DAGui/icon/clear-message.svg");
    mActionCopySelectMessage      = createAction("actionCopySelectMessage", ":/DAGui/icon/copy.svg");
    // 构建菜单

    //
    mModel           = new DAMessageLogsModel(this);
    mSortFilterModel = new DAMessageLogsSortFilterProxyModel(this);
    mSortFilterModel->setSourceModel(mModel);
    ui->tableView->setModel(mSortFilterModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    QFontMetrics fm = ui->tableView->fontMetrics();
    // 高度为行高的1.2
    ui->tableView->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.2);

    //
    ui->toolButtonInfo->setDefaultAction(mActionMessageLogShowInfo);
    ui->toolButtonWarning->setDefaultAction(mActionMessageLogShowWarning);
    ui->toolButtonCritial->setDefaultAction(mActionMessageLogShowCritical);
    ui->toolButtonClear->setDefaultAction(mActionMessageLogClear);
    //
    connect(mActionMessageLogShowInfo, &QAction::triggered, this, &DAMessageLogViewWidget::setEnableShowInfoMsg);
    connect(mActionMessageLogShowWarning, &QAction::triggered, this, &DAMessageLogViewWidget::setEnableShowWarningMsg);
    connect(mActionMessageLogShowCritical, &QAction::triggered, this, &DAMessageLogViewWidget::setEnableShowCriticalMsg);
    connect(mActionMessageLogClear, &QAction::triggered, this, &DAMessageLogViewWidget::clearAll);
    connect(mActionCopySelectMessage, &QAction::triggered, this, &DAMessageLogViewWidget::copySelectionMessageToClipBoard);
    connect(this, &DAMessageLogViewWidget::customContextMenuRequested, this, &DAMessageLogViewWidget::onCustomContextMenuRequested);
    ui->tableView->setWordWrap(true);
    connect(ui->tableView, &QTableView::clicked, this, &DAMessageLogViewWidget::onTableViewItemClicked);
    setContextMenuPolicy(Qt::CustomContextMenu);
    DAMessageLogQueue* messageQueue = &DAMessageLogQueue::instance();
    connect(messageQueue, &DAMessageLogQueue::messageQueueAppended, this, &DAMessageLogViewWidget::onMessageAppended);

    retranslateUi();
}

DAMessageLogViewWidget::~DAMessageLogViewWidget()
{
    delete ui;
}

QAction* DAMessageLogViewWidget::createAction(const char* objname, const char* iconpath, bool checkable, bool checked)
{
    QAction* act = new QAction(this);
    act->setObjectName(QString::fromUtf8(objname));
    QIcon icon(iconpath);
    act->setIcon(icon);
    act->setCheckable(checkable);
    if (checkable) {
        act->setChecked(checked);
    }
    return act;
}

void DAMessageLogViewWidget::onCustomContextMenuRequested(const QPoint& pos)
{
    if (ui->tableView->underMouse()) {
        if (nullptr == mMenu) {
            buildMenu();
        }
        mMenu->exec(mapToGlobal(pos));
    }
}

void DAMessageLogViewWidget::onMessageAppended()
{
    if (isAutoScrollToButtom()) {
        ui->tableView->scrollToBottom();
    }
}

void DAMessageLogViewWidget::buildMenu()
{
    mMenu = new QMenu(this);
    mMenu->addAction(mActionCopySelectMessage);
    mMenu->addSeparator();
    mMenu->addAction(mActionMessageLogShowInfo);
    mMenu->addAction(mActionMessageLogShowWarning);
    mMenu->addAction(mActionMessageLogShowCritical);
    mMenu->addSeparator();
    mMenu->addAction(mActionMessageLogClear);
}

bool DAMessageLogViewWidget::isAutoScrollToButtom() const
{
    return mIsAutoScrollToButtom;
}

void DAMessageLogViewWidget::setAutoScrollToButtom(bool isAutoScrollToButtom)
{
    mIsAutoScrollToButtom = isAutoScrollToButtom;
}
/**
 * @brief 设置是否允许DebugMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowDebugMsg(bool on)
{
    mSortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptDebugMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许DebugMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowDebugMsg() const
{
    return mSortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptDebugMsg);
}
/**
 * @brief 设置是否允许WarningMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowWarningMsg(bool on)
{
    mSortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptWarningMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许WarningMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowWarningMsg() const
{
    return mSortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptWarningMsg);
}
/**
 * @brief 设置是否允许CriticalMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowCriticalMsg(bool on)
{
    mSortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptCriticalMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许CriticalMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowCriticalMsg() const
{
    return mSortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptCriticalMsg);
}
/**
 * @brief 设置是否允许FatalMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowFatalMsg(bool on)
{
    mSortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptFatalMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许FatalMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowFatalMsg() const
{
    return mSortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptFatalMsg);
}
/**
 * @brief 设置是否允许InfoMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowInfoMsg(bool on)
{
    mSortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptInfoMsg, on);
    ui->tableView->update();
}

/**
 * @brief 点击后自动适应尺寸
 * @param index
 */
void DAMessageLogViewWidget::onTableViewItemClicked(const QModelIndex& index)
{
    QHeaderView* vh = ui->tableView->verticalHeader();
    if (vh && index.isValid()) {
        if (index.row() < vh->count()) {
            vh->setSectionResizeMode(index.row(), QHeaderView::ResizeToContents);
        }
    }
}

/**
 * @brief 清空所有消息
 */
void DAMessageLogViewWidget::clearAll()
{
    mModel->clearAll();
}

/**
 * @brief 把选中的文本复制到剪切板
 */
void DAMessageLogViewWidget::copySelectionMessageToClipBoard()
{
    QItemSelectionModel* sm = ui->tableView->selectionModel();
    if (nullptr == sm) {
        return;
    }
    int cc = mSortFilterModel->columnCount();
    QSet< int > rowIndexs;
    QModelIndexList indexs = sm->selectedIndexes();
    QString text;
    for (const QModelIndex& i : std::as_const(indexs)) {
        if (!rowIndexs.contains(i.row())) {
            if (!rowIndexs.isEmpty()) {
                text += "\n";
            }
            rowIndexs.insert(i.row());
            QString line;
            for (int c = 0; c < cc; ++c) {
                line += mSortFilterModel->data(mSortFilterModel->index(i.row(), c)).toString();
                if (c != cc - 1) {
                    line += "\t";
                }
            }
            text += line;
        }
    }
    if (text.isEmpty()) {
        return;
    }
    QClipboard* appclip = QApplication::clipboard();
    if (appclip) {
        appclip->setText(text);
    }
}

/**
 * @brief 选中所有
 */
void DAMessageLogViewWidget::selectAll()
{
    ui->tableView->selectAll();
}

/**
 * @brief 事件改变捕获
 * @param event
 */
void DAMessageLogViewWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

/**
 * @brief 设置文本
 */
void DAMessageLogViewWidget::retranslateUi()
{
    mActionMessageLogShowInfo->setText(tr("Info"));                          // cn:信息
    mActionMessageLogShowInfo->setToolTip(tr("Show Info Message"));          // cn:显示信息消息
    mActionMessageLogShowWarning->setText(tr("Warning"));                    // cn:警告
    mActionMessageLogShowWarning->setToolTip(tr("Show Warning Message"));    // cn:显示警告消息
    mActionMessageLogShowCritical->setText(tr("Critical"));                  // cn:严重
    mActionMessageLogShowCritical->setToolTip(tr("Show Critical Message"));  // cn:显示严重消息
    mActionMessageLogClear->setText(tr("Clear"));                            // cn:清空
    mActionMessageLogClear->setToolTip(tr("Clear All Messages"));            // cn:清空所有消息
    mActionCopySelectMessage->setText(tr("Copy"));                           // cn:复制
    mActionCopySelectMessage->setToolTip(tr("Copy Selected Message"));       // cn:复制选中消息
}

/**
 * @brief 处理快捷键
 * @param event
 */
void DAMessageLogViewWidget::keyPressEvent(QKeyEvent* event)
{
    if (event) {
        if (Qt::ControlModifier == event->modifiers()) {
            if (Qt::Key_C == event->key()) {
                // 复制
                copySelectionMessageToClipBoard();
                event->accept();
            } else if (Qt::Key_A == event->key()) {
                selectAll();
                event->accept();
            }
        }
    }
    QWidget::keyPressEvent(event);
}

/**
 * @brief 获取内部的action
 * @param ac
 * @return
 */
QAction* DAMessageLogViewWidget::getAction(DAMessageLogViewWidget::MessageLogActions ac) const
{
    switch (ac) {
    case ActionInfo:
        return mActionMessageLogShowInfo;
    case ActionWarning:
        return mActionMessageLogShowWarning;
    case ActionCritial:
        return mActionMessageLogShowCritical;
    case ActionClear:
        return mActionMessageLogClear;
    case ActionCopy:
        return mActionCopySelectMessage;
    default:
        break;
    }
    return nullptr;
}
/**
 * @brief 检测是否允许InfoMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowInfoMsg() const
{
    return mSortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptInfoMsg);
}
