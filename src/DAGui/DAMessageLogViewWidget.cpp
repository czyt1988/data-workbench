#include "DAMessageLogViewWidget.h"
#include "ui_DAMessageLogViewWidget.h"
#include <QDebug>
#include <QMenu>
#include <QItemSelectionModel>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include "DAMessageLogsModel.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAMessageLogViewWidget
//===================================================
DAMessageLogViewWidget::DAMessageLogViewWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAMessageLogViewWidget), _menu(nullptr)
{
    ui->setupUi(this);
    //创建action
    _actionMessageLogShowInfo = createAction("actionMessageLogShowInfo", ":/messageType/icon/messageType/messageTypeInfo.svg", true, true);
    _actionMessageLogShowWarning  = createAction("actionMessageLogShowWarning",
                                                ":/messageType/icon/messageType/messageTypeWarning.svg",
                                                true,
                                                true);
    _actionMessageLogShowCritical = createAction("actionMessageLogShowCritical",
                                                 ":/messageType/icon/messageType/messageTypeError.svg",
                                                 true,
                                                 true);
    _actionMessageLogClear        = createAction("actionMessageLogClear", ":/icon/icon/clear-message.svg");
    _actionCopySelectMessage      = createAction("actionCopySelectMessage", ":/icon/icon/copy.svg");
    //构建菜单

    //
    _model           = new DAMessageLogsModel(this);
    _sortFilterModel = new DAMessageLogsSortFilterProxyModel(this);
    _sortFilterModel->setSourceModel(_model);
    ui->tableView->setModel(_sortFilterModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    QFontMetrics fm = ui->tableView->fontMetrics();
    //高度为行高的1.2
    ui->tableView->verticalHeader()->setDefaultSectionSize(fm.lineSpacing() * 1.2);

    //
    ui->toolButtonInfo->setDefaultAction(_actionMessageLogShowInfo);
    ui->toolButtonWarning->setDefaultAction(_actionMessageLogShowWarning);
    ui->toolButtonCritial->setDefaultAction(_actionMessageLogShowCritical);
    ui->toolButtonClear->setDefaultAction(_actionMessageLogClear);
    //
    connect(_actionMessageLogShowInfo, &QAction::triggered, this, &DAMessageLogViewWidget::setEnableShowInfoMsg);
    connect(_actionMessageLogShowWarning, &QAction::triggered, this, &DAMessageLogViewWidget::setEnableShowWarningMsg);
    connect(_actionMessageLogShowCritical, &QAction::triggered, this, &DAMessageLogViewWidget::setEnableShowCriticalMsg);
    connect(_actionMessageLogClear, &QAction::triggered, this, &DAMessageLogViewWidget::clearAll);
    connect(_actionCopySelectMessage, &QAction::triggered, this, &DAMessageLogViewWidget::copySelectionMessageToClipBoard);
    connect(this, &DAMessageLogViewWidget::customContextMenuRequested, this, &DAMessageLogViewWidget::onCustomContextMenuRequested);
    ui->tableView->setWordWrap(true);
    connect(ui->tableView, &QTableView::clicked, this, &DAMessageLogViewWidget::onTableViewItemClicked);
    DAMessageQueueProxy* messageQueue = &(_model->messageQueueProxy());
    connect(messageQueue, &DAMessageQueueProxy::messageQueueAppended, this, &DAMessageLogViewWidget::onMessageAppended);

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
        if (nullptr == _menu) {
            buildMenu();
        }
        _menu->exec(mapToGlobal(pos));
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
    _menu = new QMenu(this);
    _menu->addAction(_actionCopySelectMessage);
    _menu->addSeparator();
    _menu->addAction(_actionMessageLogShowInfo);
    _menu->addAction(_actionMessageLogShowWarning);
    _menu->addAction(_actionMessageLogShowCritical);
    _menu->addSeparator();
    _menu->addAction(_actionMessageLogClear);
    setContextMenuPolicy(Qt::CustomContextMenu);
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
    _sortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptDebugMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许DebugMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowDebugMsg() const
{
    return _sortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptDebugMsg);
}
/**
 * @brief 设置是否允许WarningMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowWarningMsg(bool on)
{
    _sortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptWarningMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许WarningMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowWarningMsg() const
{
    return _sortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptWarningMsg);
}
/**
 * @brief 设置是否允许CriticalMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowCriticalMsg(bool on)
{
    _sortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptCriticalMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许CriticalMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowCriticalMsg() const
{
    return _sortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptCriticalMsg);
}
/**
 * @brief 设置是否允许FatalMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowFatalMsg(bool on)
{
    _sortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptFatalMsg, on);
    ui->tableView->update();
}
/**
 * @brief 检测是否允许FatalMsg的显示
 * @return
 */
bool DAMessageLogViewWidget::isEnableShowFatalMsg() const
{
    return _sortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptFatalMsg);
}
/**
 * @brief 设置是否允许InfoMsg的显示
 * @param on
 */
void DAMessageLogViewWidget::setEnableShowInfoMsg(bool on)
{
    _sortFilterModel->setAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptInfoMsg, on);
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
    _model->clearAll();
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
    int cc = _sortFilterModel->columnCount();
    QSet< int > rowIndexs;
    QModelIndexList indexs = sm->selectedIndexes();
    QString text;
    for (const QModelIndex& i : qAsConst(indexs)) {
        if (!rowIndexs.contains(i.row())) {
            if (!rowIndexs.isEmpty()) {
                text += "\n";
            }
            rowIndexs.insert(i.row());
            QString line;
            for (int c = 0; c < cc; ++c) {
                line += _sortFilterModel->data(_sortFilterModel->index(i.row(), c)).toString();
                if (c != cc - 1) {
                    line += "\t";
                }
            }
            text += line;
        }
    }
    if (text.isEmpty()) {
        qDebug() << "copy nothing to clipboard";
        return;
    }
    QClipboard* appclip = QApplication::clipboard();
    if (appclip) {
        appclip->setText(text);
    }
    qDebug() << "copy to clipboard:" << text;
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
    _actionMessageLogShowInfo->setText(tr("Info"));
    _actionMessageLogShowInfo->setToolTip(tr("Show Info Message"));
    _actionMessageLogShowWarning->setText(tr("Warning"));
    _actionMessageLogShowWarning->setToolTip(tr("Show Warning Message"));
    _actionMessageLogShowCritical->setText(tr("Critical"));
    _actionMessageLogShowCritical->setToolTip(tr("Show Critical Message"));
    _actionMessageLogClear->setText(tr("Clear"));                  // cn:清空
    _actionMessageLogClear->setToolTip(tr("Clear All Messages"));  // cn:清空所有消息
    _actionCopySelectMessage->setText(tr("Copy"));                 // 复制
    _actionCopySelectMessage->setToolTip(tr("Copy Select Message"));
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
                //复制
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
        return _actionMessageLogShowInfo;
    case ActionWarning:
        return _actionMessageLogShowWarning;
    case ActionCritial:
        return _actionMessageLogShowCritical;
    case ActionClear:
        return _actionMessageLogClear;
    case ActionCopy:
        return _actionCopySelectMessage;
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
    return _sortFilterModel->testAcceptMessageTypeFlag(DAMessageLogsSortFilterProxyModel::AcceptInfoMsg);
}
