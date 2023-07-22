#include "DAWorkFlowOperateWidget.h"
// qt
#include <QImage>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QUndoStack>
// moc
#include "ui_DAWorkFlowOperateWidget.h"
// workflow
#include "DAWorkFlowGraphicsView.h"
#include "DAWorkFlowGraphicsScene.h"
#include "DAGraphicsResizeablePixmapItem.h"
//
#include "DAWorkFlowEditWidget.h"
#include "DACommandsForWorkFlowNodeGraphics.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAWorkFlowOperateWidget
//===================================================
DAWorkFlowOperateWidget::DAWorkFlowOperateWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAWorkFlowOperateWidget), _isShowGrid(true), _defaultTextColor(Qt::black)
{
    _isDestorying = false;
    ui->setupUi(this);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DAWorkFlowOperateWidget::onTabWidgetCurrentChanged);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &DAWorkFlowOperateWidget::onTabWidgetTabCloseRequested);
}

DAWorkFlowOperateWidget::~DAWorkFlowOperateWidget()
{
    qDebug() << "DAWorkFlowOperateWidget begin delete ui";
    _isDestorying = true;
    delete ui;
    qDebug() << "DAWorkFlowOperateWidget end delete ui";
}

/**
 * @brief 创建工作流，创建完后通过getWorkflow获取
 *
 * 如果对DAWorkFlow如果有继承，那么重载此函数创建自己的workflow就行
 *
 * 此函数会在@ref appendWorkflow 中调用
 * @return
 */
DAWorkFlow* DAWorkFlowOperateWidget::createWorkflow()
{
    return (new DAWorkFlow());
}

/**
 * @brief 添加一个工作流编辑窗口
 *
 * 此函数发射信号workflowCreated（先），也会触发currentWorkFlowWidgetChanged（后）
 * @param wfe
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::appendWorkflow(const QString& name)
{
    DAWorkFlowEditWidget* wfe = new DAWorkFlowEditWidget(ui->tabWidget);
    DAWorkFlow* wf            = createWorkflow();
    wf->setParent(wfe);
    wfe->setWorkFlow(wf);
    //把undo添加进去
    wfe->setEnableShowGrid(_isShowGrid);
    wfe->setDefaultTextColor(_defaultTextColor);
    wfe->setDefaultTextFont(_defaultFont);
    DAWorkFlowGraphicsScene* scene = wfe->getWorkFlowGraphicsScene();
    connect(wfe, &DAWorkFlowEditWidget::selectNodeItemChanged, this, &DAWorkFlowOperateWidget::selectNodeItemChanged);
    connect(wfe, &DAWorkFlowEditWidget::mouseActionFinished, this, &DAWorkFlowOperateWidget::mouseActionFinished);
    connect(scene, &DAWorkFlowGraphicsScene::selectionChanged, this, &DAWorkFlowOperateWidget::onSelectionChanged);
    connect(wfe, &DAWorkFlowEditWidget::startExecute, this, [ this, wfe ]() { emit workflowStartExecute(wfe); });
    connect(wfe, &DAWorkFlowEditWidget::nodeExecuteFinished, this, [ this, wfe ](DAAbstractNode::SharedPointer n, bool state) {
        emit nodeExecuteFinished(wfe, n, state);
    });
    connect(wfe, &DAWorkFlowEditWidget::finished, this, [ this, wfe ](bool s) { emit workflowFinished(wfe, s); });
    ui->tabWidget->addTab(wfe, name);
    //把名字保存到DAWorkFlowEditWidget中，在DAProject保存的时候会用到
    wfe->setWindowTitle(name);
    emit workflowCreated(wfe);
    ui->tabWidget->setCurrentIndex(ui->tabWidget->indexOf(wfe));
    return wfe;
}

/**
 * @brief 获取当前工作流的索引
 * @return
 */
int DAWorkFlowOperateWidget::getCurrentWorkflowIndex() const
{
    return ui->tabWidget->currentIndex();
}

/**
 * @brief 设置当前的工作流
 * @param index
 */
void DAWorkFlowOperateWidget::setCurrentWorkflow(int index)
{
    ui->tabWidget->setCurrentIndex(index);
}

/**
 * @brief 设置当前的页面
 * @param wf
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowWidget(DAWorkFlowEditWidget* wf)
{
    ui->tabWidget->setCurrentWidget(wf);
}

/**
 * @brief 获取当前选中的工作流窗口
 * @return
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::getCurrentWorkFlowWidget() const
{
    QWidget* w = ui->tabWidget->currentWidget();
    if (nullptr == w) {
        return nullptr;
    }
    return qobject_cast< DAWorkFlowEditWidget* >(w);
}

void DAWorkFlowOperateWidget::setCurrentWorkflowName(const QString& name)
{
    int i = getCurrentWorkflowIndex();
    renameWorkFlowWidget(i, name);
}

/**
 * @brief 获取scene
 * @return
 */
DAWorkFlowGraphicsScene* DAWorkFlowOperateWidget::getCurrentWorkFlowScene() const
{
    DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        return nullptr;
    }
    return w->getWorkFlowGraphicsScene();
}

/**
 * @brief 获取所有的工作流窗口
 * @return
 */
QList< DAWorkFlowGraphicsScene* > DAWorkFlowOperateWidget::getAllWorkFlowScene() const
{
    QList< DAWorkFlowGraphicsScene* > res;
    int c = ui->tabWidget->count();
    for (int i = 0; i < c; ++i) {
        DAWorkFlowEditWidget* we = qobject_cast< DAWorkFlowEditWidget* >(ui->tabWidget->widget(i));
        if (we) {
            DAWorkFlowGraphicsScene* sc = we->getWorkFlowGraphicsScene();
            if (sc) {
                res.append(sc);
            }
        }
    }
    return res;
}

/**
 * @brief 获取当前视图
 * @return
 */
DAWorkFlowGraphicsView* DAWorkFlowOperateWidget::getCurrentWorkFlowView() const
{
    DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        return nullptr;
    }
    return w->getWorkFlowGraphicsView();
}

/**
 * @brief 获取工作流窗口
 * @param index
 * @return 如果超出索引范围返回nullptr
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::getWorkFlowWidget(int index) const
{
    return qobject_cast< DAWorkFlowEditWidget* >(ui->tabWidget->widget(index));
}

/**
 * @brief 获取工作流窗口的名称
 * @param index
 * @return
 */
QString DAWorkFlowOperateWidget::getWorkFlowWidgetName(int index) const
{
    return ui->tabWidget->tabText(index);
}

/**
 * @brief 给工作流重命名
 * @param index
 * @param name
 */
void DAWorkFlowOperateWidget::renameWorkFlowWidget(int index, const QString& name)
{
    ui->tabWidget->setTabText(index, name);
}

/**
 * @brief 获取编辑窗口数量
 * @return
 */
int DAWorkFlowOperateWidget::count() const
{
    return ui->tabWidget->count();
}

/**
 * @brief 移除工作流
 * @param index
 */
void DAWorkFlowOperateWidget::removeWorkflow(int index)
{
    QWidget* w = ui->tabWidget->widget(index);
    if (nullptr == w) {
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::question(this,
                                                            tr("question"),  //疑问
                                                            tr("Confirm to delete workflow:%1").arg(getWorkFlowWidgetName(index))  //是否确认删除工作流:%1
    );
    if (btn != QMessageBox::Yes) {
        return;
    }
    ui->tabWidget->removeTab(index);
    w->hide();
    w->deleteLater();
}

/**
 * @brief 激活当前的回退功能
 */
void DAWorkFlowOperateWidget::setUndoStackActive()
{
    DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (w) {
        w->setUndoStackActive();
    }
}

/**
 * @brief 设置显示grid
 *
 * 此函数影响所有工作流编辑窗口
 * @param on
 */
void DAWorkFlowOperateWidget::setEnableShowGrid(bool on)
{
    _isShowGrid = on;
    const int c = count();
    for (int i = 0; i < c; ++i) {
        DAWorkFlowEditWidget* w = getWorkFlowWidget(i);
        if (w == nullptr) {
            continue;
        }
        w->setEnableShowGrid(_isShowGrid);
    }
}

/**
 * @brief 是否显示网格
 * @return
 */
bool DAWorkFlowOperateWidget::isEnableShowGrid() const
{
    return _isShowGrid;
}

/**
 * @brief 获取undostack
 * @return
 */
QUndoStack* DAWorkFlowOperateWidget::getUndoStack()
{
    DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (w) {
        return w->getUndoStack();
    }
    return nullptr;
}

void DAWorkFlowOperateWidget::addBackgroundPixmap(const QString& pixmapPath)
{
    DAWorkFlowGraphicsScene* s = getCurrentWorkFlowScene();
    if (nullptr == s) {
        return;
    }
    QImage img(pixmapPath);
    QPixmap px;
    px.convertFromImage(img);
    DAGraphicsResizeablePixmapItem* item = s->setBackgroundPixmap(px);
    item->setSelectable(true);
    item->setMoveable(true);
    // connect(item, &DAGraphicsResizeablePixmapItem::itemPosChange, this, &DAWorkFlowOperateWidget::onItemPosChange);
}

void DAWorkFlowOperateWidget::setBackgroundPixmapLock(bool on)
{
    DAWorkFlowGraphicsScene* s = getCurrentWorkFlowScene();
    if (nullptr == s) {
        return;
    }
    DAGraphicsResizeablePixmapItem* item = s->getBackgroundPixmapItem();
    if (nullptr == item) {
        return;
    }
    item->setSelectable(!on);
    item->setMoveable(!on);
}

void DAWorkFlowOperateWidget::setSelectTextColor(const QColor& color)
{
    DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectTextColor(color);
    }
}

void DAWorkFlowOperateWidget::setSelectShapeBackgroundBrush(const QBrush& b)
{
    DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectShapeBackgroundBrush(b);
    }
}

void DAWorkFlowOperateWidget::setSelectShapeBorderPen(const QPen& v)
{
    DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectShapeBorderPen(v);
    }
}

void DAWorkFlowOperateWidget::setSelectTextFont(const QFont& f)
{
    DAWorkFlowEditWidget* ww = getCurrentWorkFlowWidget();
    if (ww) {
        ww->setSelectTextItemFont(f);
    }
}

/**
 * @brief 设置当前工作流的网格显示与否
 * @param on
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowShowGrid(bool on)
{
    DAWorkFlowGraphicsScene* secen = getCurrentWorkFlowScene();
    if (nullptr == secen) {
        return;
    }
    secen->showGridLine(on);
    secen->update();
}

/**
 * @brief 创建一个新的工作流窗口
 * @note 此函数带有交互
 * @return
 */
DAWorkFlowEditWidget* DAWorkFlowOperateWidget::appendWorkflowWithDialog()
{
    bool ok      = false;
    QString text = QInputDialog::getText(this, tr("Title of new workflow"), tr("Title:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || text.isEmpty()) {
        return nullptr;
    }
    return appendWorkflow(text);
}

/**
 * @brief 设置当前工作流全部显示
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowWholeView()
{
    DAWorkFlowGraphicsView* view = getCurrentWorkFlowView();
    if (!view) {
        qWarning() << tr("Loss View");  // cn:缺少视图
        return;
    }
    view->setWholeView();
}

/**
 * @brief 放大
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowZoomIn()
{
    DAWorkFlowGraphicsView* view = getCurrentWorkFlowView();
    if (!view) {
        qWarning() << tr("Loss View");  // cn:缺少视图
        return;
    }
    view->zoomIn();
}

/**
 * @brief 缩小
 */
void DAWorkFlowOperateWidget::setCurrentWorkflowZoomOut()
{
    DAWorkFlowGraphicsView* view = getCurrentWorkFlowView();
    if (!view) {
        qWarning() << tr("Loss View");  // cn:缺少视图
        return;
    }
    view->zoomOut();
}

/**
 * @brief 运行工作流
 */
void DAWorkFlowOperateWidget::runCurrentWorkFlow()
{
    DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        qWarning() << tr("No active workflow detected");  //未检测到激活的工作流
        return;
    }
    DAWorkFlow* wf = w->getWorkflow();
    if (nullptr == wf) {
        qCritical() << tr("Unable to get workflow correctly");  //无法正确获取工作流
        return;
    }
    wf->exec();
}

/**
 * @brief 终止当前工作流
 */
void DAWorkFlowOperateWidget::terminateCurrentWorkFlow()
{
    DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        qWarning() << tr("No active workflow detected");  //未检测到激活的工作流
        return;
    }
    DAWorkFlow* wf = w->getWorkflow();
    if (nullptr == wf) {
        qCritical() << tr("Unable to get workflow correctly");  //无法正确获取工作流
        return;
    }
    wf->terminate();
}

/**
 * @brief 文本字体
 * @param c
 */
QFont DAWorkFlowOperateWidget::getDefaultTextFont() const
{
    return _defaultFont;
}
/**
 * @brief 设置文本字体
 * @param c
 */
void DAWorkFlowOperateWidget::setDefaultTextFont(const QFont& f)
{
    _defaultFont                             = f;
    QList< DAWorkFlowGraphicsScene* > secens = getAllWorkFlowScene();
    for (DAWorkFlowGraphicsScene* sc : qAsConst(secens)) {
        sc->setDefaultTextFont(f);
    }
}
/**
 * @brief 文本颜色
 * @param c
 */
QColor DAWorkFlowOperateWidget::getDefaultTextColor() const
{
    return _defaultTextColor;
}
/**
 * @brief 设置默认的文本颜色
 * @param c
 */
void DAWorkFlowOperateWidget::setDefaultTextColor(const QColor& c)
{
    _defaultTextColor                        = c;
    QList< DAWorkFlowGraphicsScene* > secens = getAllWorkFlowScene();
    for (DAWorkFlowGraphicsScene* sc : secens) {
        sc->setDefaultTextColor(c);
    }
}

/**
 * @brief tab窗口发送了变化
 * @param index
 */
void DAWorkFlowOperateWidget::onTabWidgetCurrentChanged(int index)
{
    DAWorkFlowEditWidget* w = getWorkFlowWidget(index);
    if (nullptr == w) {
        return;
    }
    emit currentWorkFlowWidgetChanged(w);
}

/**
 * @brief 请求关闭
 * @param index
 */
void DAWorkFlowOperateWidget::onTabWidgetTabCloseRequested(int index)
{
    removeWorkflow(index);
}

/**
 * @brief 场景条目选择变化触发的槽
 */
void DAWorkFlowOperateWidget::onSelectionChanged()
{
    if (_isDestorying) {
        //! 很奇怪，DAWorkFlowGraphicsScene已经析构了，但此槽函数还是能调用，在DAWorkFlowOperateWidget
        //! 开始delete ui的时候，先析构DAWorkFlowGraphicsView，再析构DAWorkFlowGraphicsScene
        //! 然后就会调用此槽函数，这时导致错误，从qt原理上，在析构时应该会把槽函数都断开连接才合理
        return;
    }
    DAWorkFlowGraphicsScene* scene = getCurrentWorkFlowScene();
    if (nullptr == scene) {
        return;
    }
    QList< QGraphicsItem* > sits = scene->selectedItems();
    if (sits.isEmpty()) {
        return;
    }
    emit selectionItemChanged(sits.last());
}

QList< DAStandardGraphicsTextItem* > DAWorkFlowOperateWidget::getSelectTextItems()
{
    QList< DAStandardGraphicsTextItem* > res;
    DAWorkFlowGraphicsScene* secen = getCurrentWorkFlowScene();
    if (nullptr == secen) {
        return res;
    }
    QList< QGraphicsItem* > its = secen->selectedItems();
    if (its.size() == 0) {
        return res;
    }
    for (QGraphicsItem* item : qAsConst(its)) {
        if (DAStandardGraphicsTextItem* textItem = dynamic_cast< DAStandardGraphicsTextItem* >(item)) {
            res.append(textItem);
        }
    }
    return res;
}

/**
 * @brief 设置鼠标动作
 *
 * 一旦设置鼠标动作，鼠标点击后就会触发此动作，continuous来标记动作结束后继续保持还是还原为无动作
 * @param mf 鼠标动作
 * @param continuous 是否连续执行
 */
bool DAWorkFlowOperateWidget::setMouseActionFlag(DAWorkFlowGraphicsScene::MouseActionFlag mf, bool continous)
{
    DAWorkFlowEditWidget* w = getCurrentWorkFlowWidget();
    if (nullptr == w) {
        qWarning() << tr("No active workflow detected");  //未检测到激活的工作流
        return false;
    }
    w->setMouseActionFlag(mf, continous);
    return true;
}

void DAWorkFlowOperateWidget::clear()
{
    int count = ui->tabWidget->count();
    QList< DAWorkFlowEditWidget* > wfes;
    for (int i = 0; i < count; ++i) {
        DAWorkFlowEditWidget* wfe = getWorkFlowWidget(i);
        wfes.append(wfe);
    }
    //清空tab
    while (ui->tabWidget->count() > 0) {
        ui->tabWidget->removeTab(0);
    }
    //清空
    for (DAWorkFlowEditWidget* w : wfes) {
        w->hide();
        w->deleteLater();
    }
}

/**
 * @brief 获取所有工作流的名字
 * @return
 */
QList< QString > DAWorkFlowOperateWidget::getAllWorkflowNames() const
{
    QList< QString > names;
    int c = ui->tabWidget->count();
    for (int i = 0; i < c; ++i) {
        names.append(ui->tabWidget->tabText(i));
    }
    return names;
}

}
