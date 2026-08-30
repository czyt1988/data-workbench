#include "DAChartTextMarkerEditor.h"
#include "DAChartTextMarker.h"
#include "DAChartTextEditorPopup.h"
#include "da_qt5qt6_compat.hpp"

#include <QTimer>
#include <QColor>
#include <QCursor>
#include <QDebug>
#include "qwt_plot.h"
#include "qwt_text.h"

namespace DA
{
class DAChartTextMarkerEditor::PrivateData
{
    DA_DECLARE_PUBLIC(DAChartTextMarkerEditor)
public:
    PrivateData(DAChartTextMarkerEditor* p);
    ~PrivateData();

public:
    // DAChartTextMarker 非 QObject，不能用 QPointer，生命周期由本类独占管理
    DAChartTextMarker* mMarker { nullptr };          ///< 临时文本标注
    QPointer< DAChartTextEditorPopup > mPopup { nullptr };  ///< 富文本编辑弹窗
    QPoint mGlobalPressPos;                          ///< 按下位置（全局坐标），用于弹窗定位
    bool mFinished { false };                        ///< 防止重复结束编辑
};

DAChartTextMarkerEditor::PrivateData::PrivateData(DAChartTextMarkerEditor* p) : q_ptr(p)
{
}

DAChartTextMarkerEditor::PrivateData::~PrivateData()
{
    if (mPopup) {
        mPopup->deleteLater();
    }
    if (mMarker) {
        mMarker->detach();
        delete mMarker;
        mMarker = nullptr;
    }
}

/**
 * @brief 构造函数
 * @param parent 关联的QwtPlot
 */
DAChartTextMarkerEditor::DAChartTextMarkerEditor(QwtPlot* parent) : DAAbstractChartEditor(parent), DA_PIMPL_CONSTRUCT
{
    // 文本添加模式使用文本编辑光标，编辑结束随canvas事件过滤器卸载恢复默认
    if (QwtPlot* p = plot()) {
        if (QWidget* canvas = p->canvas()) {
            canvas->setCursor(Qt::IBeamCursor);
        }
    }
}

/**
 * @brief 析构函数，恢复canvas默认光标
 */
DAChartTextMarkerEditor::~DAChartTextMarkerEditor()
{
    if (QwtPlot* p = plot()) {
        if (QWidget* canvas = p->canvas()) {
            canvas->setCursor(Qt::ArrowCursor);
        }
    }
}

/**
 * @brief 运行时类型标识
 * @return RTTI值
 */
int DAChartTextMarkerEditor::rtti() const
{
    return DAAbstractChartEditor::RTTITextEditor;
}

/**
 * @brief 交出临时图元的所有权
 * @return 文本标注图元，调用后内部指针置空
 */
QwtPlotItem* DAChartTextMarkerEditor::takeItem()
{
    DA_D(d);
    QwtPlotItem* item = d->mMarker;
    d->mMarker        = nullptr;
    return item;
}

/**
 * @brief 取消编辑（Esc），清理临时标注
 * @return 取消成功返回true
 */
bool DAChartTextMarkerEditor::cancel()
{
    releaseMarker();
    finishEdit(true);
    return true;
}

/**
 * @brief 鼠标按下：在点击位置创建文本标注
 *
 * 此时创建带占位文本的标注，让用户立即看到落点位置；
 * 弹窗延迟到释放后再弹出，避免与overlay的grabMouse冲突
 * @param e 鼠标事件
 * @return 是否处理事件
 */
bool DAChartTextMarkerEditor::mousePressEvent(const QMouseEvent* e)
{
    DA_D(d);
    if (d->mFinished || d->mMarker) {
        return true;
    }
    QwtPlot* gca = plot();
    if (!gca) {
        return false;
    }
    Q_EMIT beginEdit();
    const QPoint canvasPos = compat::eventPos(e);
    d->mGlobalPressPos     = QCursor::pos();
    // canvas坐标转数据坐标
    QPointF pos = invTransform(canvasPos);

    DAChartTextMarker* marker = new DAChartTextMarker(QObject::tr("Text Marker")  //cn:文本标注
    );
    marker->setAnchorPosition(pos);
    // 占位文本，灰色提示用户输入
    QwtText placeholder(QObject::tr("Text")  //cn:文本
    );
    placeholder.setColor(QColor(160, 160, 160));
    placeholder.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
    marker->setLabel(placeholder);
    marker->attach(gca);
    d->mMarker = marker;
    gca->replot();
    return true;
}

/**
 * @brief 鼠标释放：延迟弹出富文本编辑弹窗
 *
 * overlay 在 mousePress 转发后会 grabMouse，在 release 时才 releaseMouse，
 * 若此处直接弹 Qt::Popup 会产生双 grab 冲突，因此用 singleShot(0) 延迟到事件循环下一轮
 * @param e 鼠标事件
 * @return 是否处理事件
 */
bool DAChartTextMarkerEditor::mouseReleaseEvent(const QMouseEvent* e)
{
    DA_D(d);
    Q_UNUSED(e);
    if (d->mFinished || !d->mMarker || d->mPopup) {
        return true;
    }
    QTimer::singleShot(0, this, [ this, d ]() {
        if (d->mFinished || !d->mMarker || d->mPopup) {
            return;
        }
        DAChartTextEditorPopup* popup = new DAChartTextEditorPopup();
        d->mPopup                     = popup;
        // 先连接信号再设置内容，避免初始 setHtml 触发同步
        connect(popup, &DAChartTextEditorPopup::textChanged, this, &DAChartTextMarkerEditor::onPopupTextChagned);
        connect(popup, &DAChartTextEditorPopup::accepted, this, &DAChartTextMarkerEditor::onPopupAccepted);
        connect(popup, &DAChartTextEditorPopup::rejected, this, &DAChartTextMarkerEditor::onPopupRejected);
        popup->setHtml(QString());
        popup->popupAt(d->mGlobalPressPos);
    });
    return true;
}

/**
 * @brief 弹窗文本变化，实时同步到标注
 * @param html 富文本HTML片段
 */
void DAChartTextMarkerEditor::onPopupTextChagned(const QString& html)
{
    DA_D(d);
    if (!d->mMarker) {
        return;
    }
    d->mMarker->setHtmlText(html);
    if (QwtPlot* p = d->mMarker->plot()) {
        p->replot();
    }
}

/**
 * @brief 弹窗确认：保留标注，结束编辑
 */
void DAChartTextMarkerEditor::onPopupAccepted()
{
    DA_D(d);
    if (d->mFinished) {
        return;
    }
    if (d->mPopup && d->mPopup->toPlainText().trimmed().isEmpty()) {
        // 空文本视为取消
        releaseMarker();
        finishEdit(true);
        return;
    }
    finishEdit(false);
}

/**
 * @brief 弹窗取消：删除标注，结束编辑
 */
void DAChartTextMarkerEditor::onPopupRejected()
{
    DA_D(d);
    if (d->mFinished) {
        return;
    }
    releaseMarker();
    finishEdit(true);
}

/**
 * @brief 结束编辑
 *
 * 统一收尾：关闭弹窗、发射 finishedEdit
 * @param isCancel 是否取消
 */
void DAChartTextMarkerEditor::finishEdit(bool isCancel)
{
    DA_D(d);
    if (d->mFinished) {
        return;
    }
    d->mFinished = true;
    if (d->mPopup) {
        d->mPopup->deleteLater();
        d->mPopup = nullptr;
    }
    Q_EMIT finishedEdit(isCancel);
}

/**
 * @brief 清理临时标注
 */
void DAChartTextMarkerEditor::releaseMarker()
{
    DA_D(d);
    if (d->mMarker) {
        d->mMarker->detach();
        delete d->mMarker;
        d->mMarker = nullptr;
        if (QwtPlot* p = plot()) {
            p->replot();
        }
    }
}

}  // namespace DA
