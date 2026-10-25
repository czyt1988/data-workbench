#include "DAChartTextEditorPopup.h"
#include "DARichTextEditWidget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QHideEvent>
#include <QEvent>
#include <QTextEdit>
#include <QGuiApplication>
#include <QScreen>

namespace DA
{

class DAChartTextEditorPopup::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartTextEditorPopup)
public:
    PrivateData(DAChartTextEditorPopup* p);

    DARichTextEditWidget* mEditor { nullptr };
    QPushButton* mBtnOk { nullptr };
    QPushButton* mBtnCancel { nullptr };
    bool mUpdating { false };  ///< hideEvent 去重
};

DAChartTextEditorPopup::PrivateData::PrivateData(DAChartTextEditorPopup* p) : q_ptr(p)
{
}

//===================================================
// DAChartTextEditorPopup
//===================================================

/**
 * @brief 构造函数
 * @param parent 父窗口（弹窗独立于绘图窗口，通常传nullptr）
 */
DAChartTextEditorPopup::DAChartTextEditorPopup(QWidget* parent) : QWidget(parent, Qt::Popup), DA_PIMPL_CONSTRUCT
{
    buildUI();
}

/**
 * @brief 析构函数
 */
DAChartTextEditorPopup::~DAChartTextEditorPopup()
{
}

/**
 * @brief 构建界面与信号连接
 */
void DAChartTextEditorPopup::buildUI()
{
    DA_D(d);
    setAttribute(Qt::WA_DeleteOnClose, false);
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(6, 6, 6, 6);
    mainLay->setSpacing(4);

    // 内容控件（工具栏 + 编辑区）
    d->mEditor = new DARichTextEditWidget(this);
    mainLay->addWidget(d->mEditor, 1);

    // 确认/取消按钮（壳负责，内容控件不拥有）
    QHBoxLayout* btnLay = new QHBoxLayout();
    btnLay->addStretch(1);
    d->mBtnCancel = new QPushButton(tr("Cancel")  //cn:取消
                                     ,
                                     this);
    btnLay->addWidget(d->mBtnCancel);
    d->mBtnOk = new QPushButton(tr("OK")  //cn:确定
                                 ,
                                 this);
    d->mBtnOk->setDefault(true);
    btnLay->addWidget(d->mBtnOk);
    mainLay->addLayout(btnLay);

    connect(d->mEditor, &DARichTextEditWidget::textChanged, this, &DAChartTextEditorPopup::textChanged);
    connect(d->mBtnOk, &QPushButton::clicked, this, &DAChartTextEditorPopup::onAccept);
    connect(d->mBtnCancel, &QPushButton::clicked, this, &DAChartTextEditorPopup::onReject);
    // Esc拦截（Qt::Popup默认Esc行为无法区分确认/取消，这里统一转为取消）
    d->mEditor->textEdit()->installEventFilter(this);
    adjustSize();
}

/**
 * @brief 设置初始富文本内容
 * @param html 富文本HTML片段
 */
void DAChartTextEditorPopup::setHtml(const QString& html)
{
    d_ptr->mEditor->setHtml(html);
}

/**
 * @brief 获取富文本内容
 * @return body内层HTML片段
 */
QString DAChartTextEditorPopup::toHtml() const
{
    return d_ptr->mEditor->toHtml();
}

/**
 * @brief 获取纯文本内容
 * @return 纯文本
 */
QString DAChartTextEditorPopup::toPlainText() const
{
    return d_ptr->mEditor->toPlainText();
}

/**
 * @brief 在锚点附近弹出弹窗
 *
 * 候选方位优先级：左下 -> 右下 -> 左上 -> 右上，
 * 每个候选要求完整位于屏幕可用区域内，全部不满足时夹取到屏幕内
 * @param globalAnchor 锚点全局坐标
 */
void DAChartTextEditorPopup::popupAt(const QPoint& globalAnchor)
{
    adjustSize();
    const QSize sz = size();
    QScreen* screen = QGuiApplication::screenAt(globalAnchor);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        move(globalAnchor);
        execShow();
        return;
    }
    const QRect avail = screen->availableGeometry();
    const int margin  = 4;
    const QList< QPoint > candidates = {
        QPoint(globalAnchor.x() - sz.width() - margin, globalAnchor.y() + margin),   // 左下
        QPoint(globalAnchor.x() + margin, globalAnchor.y() + margin),                // 右下
        QPoint(globalAnchor.x() - sz.width() - margin, globalAnchor.y() - sz.height() - margin),  // 左上
        QPoint(globalAnchor.x() + margin, globalAnchor.y() - sz.height() - margin),  // 右上
    };
    for (const QPoint& c : candidates) {
        if (avail.contains(QRect(c, sz))) {
            move(c);
            execShow();
            return;
        }
    }
    // 全部越界，夹取到屏幕可用区域内
    int x = qBound(avail.left(), globalAnchor.x() - sz.width() - margin, avail.right() - sz.width());
    int y = qBound(avail.top(), globalAnchor.y() + margin, avail.bottom() - sz.height());
    move(x, y);
    execShow();
}

/**
 * @brief 显示弹窗并聚焦编辑区
 */
void DAChartTextEditorPopup::execShow()
{
    DA_D(d);
    show();
    raise();
    activateWindow();
    d->mEditor->textEdit()->setFocus(Qt::PopupFocusReason);
}

/**
 * @brief 拦截Esc转为取消
 * @param e 按键事件
 */
void DAChartTextEditorPopup::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        onReject();
        return;
    }
    QWidget::keyPressEvent(e);
}

/**
 * @brief 隐藏事件，判定关闭原因
 *
 * hideEvent是所有关闭路径的汇聚点：
 * - 点击外部导致的关闭不经过任何槽，状态仍为None，视为确认
 * @param e 隐藏事件
 */
void DAChartTextEditorPopup::hideEvent(QHideEvent* e)
{
    DA_D(d);
    QWidget::hideEvent(e);
    if (!d->mUpdating) {
        // 状态未被accept/reject设置，说明是点击外部关闭 -> 确认保留
        d->mUpdating = true;  // 防止重复发射
        Q_EMIT accepted();
    }
    d->mUpdating = false;
}

/**
 * @brief 事件过滤器，拦截编辑区的Esc
 * @param obj 监听对象
 * @param e 事件
 * @return 是否拦截
 */
bool DAChartTextEditorPopup::eventFilter(QObject* obj, QEvent* e)
{
    DA_D(d);
    if (obj == d->mEditor->textEdit() && e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast< QKeyEvent* >(e);
        if (ke->key() == Qt::Key_Escape) {
            onReject();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}

/**
 * @brief 确认编辑
 */
void DAChartTextEditorPopup::onAccept()
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    d->mUpdating = true;  // hideEvent不再重复发射
    Q_EMIT accepted();
    hide();
    d->mUpdating = false;
}

/**
 * @brief 取消编辑
 */
void DAChartTextEditorPopup::onReject()
{
    DA_D(d);
    if (d->mUpdating) {
        return;
    }
    d->mUpdating = true;
    Q_EMIT rejected();
    hide();
    d->mUpdating = false;
}

}  // namespace DA
