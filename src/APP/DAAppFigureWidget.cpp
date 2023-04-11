#include "DAAppFigureWidget.h"
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDebug>
#include "MimeData/DAMimeDataForData.h"
#include "MimeData/DAMimeDataFormats.h"
#include "Dialog/DADialogDataframePlot.h"
namespace DA
{
class DAAppFigureWidgetPrivate
{
public:
    DA_IMPL_PUBLIC(DAAppFigureWidget)
    DAAppFigureWidgetPrivate(DAAppFigureWidget* p);
    //获取dlg指针，如果为nullptr，则创建
    DADialogDataframePlot* getDlgDataframeToPointVector();

public:
    bool _isStartDrag { false };
    DADialogDataframePlot* _dlgDataframePlot { nullptr };
    DADataManager* _dataManager { nullptr };
};
DAAppFigureWidgetPrivate::DAAppFigureWidgetPrivate(DAAppFigureWidget* p) : q_ptr(p)
{
}

DADialogDataframePlot* DAAppFigureWidgetPrivate::getDlgDataframeToPointVector()
{
    if (!_dlgDataframePlot) {
        _dlgDataframePlot = new DADialogDataframePlot(q_ptr);
    }
    return _dlgDataframePlot;
}
//==============================================================
// DAAppFigureWidget
//==============================================================

DAAppFigureWidget::DAAppFigureWidget(QWidget* parent)
    : DAFigureWidget(parent), d_ptr(new DAAppFigureWidgetPrivate(this))
{

    setAcceptDrops(true);
}

DAAppFigureWidget::~DAAppFigureWidget()
{
}

void DAAppFigureWidget::setDataManager(DADataManager* mgr)
{
    d_ptr->_dataManager = mgr;
}

/**
 * @brief 拖曳进入
 *
 * 此事件需要accept,否则move事件不会触发
 * @param e
 */
void DAAppFigureWidget::dragEnterEvent(QDragEnterEvent* e)
{
    if (!e) {
        return;
    }
    qDebug() << "DAAppFigureWidget::dragEnterEvent";
    if (e->source() == this || nullptr == e->source()) {
        return;
    }
    const QMimeData* mimeData = e->mimeData();
    if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATA)) {
        e->acceptProposedAction();
    } else {
        qDebug() << "DAAppFigureWidget::dragEnterEvent get unknow format:" << mimeData->formats();
    }
}

void DAAppFigureWidget::dragMoveEvent(QDragMoveEvent* e)
{
    if (!e) {
        return;
    }
    if (e->source() == this || nullptr == e->source()) {
        return;
    }
    const QMimeData* mimeData = e->mimeData();
    if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATA)) {
        //数据
        QWidget* w = getWidgetUnderPos(e->pos());
        if (nullptr == w) {
            e->setDropAction(Qt::IgnoreAction);
            return;
        }
        if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(w)) {
            Q_UNUSED(chart);
            e->setDropAction(Qt::CopyAction);
            e->accept();
            return;
        }
    }
    e->setDropAction(Qt::IgnoreAction);
}

void DAAppFigureWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
    if (!e) {
        return;
    }
    d_ptr->_isStartDrag = false;
    e->accept();
}

void DAAppFigureWidget::dropEvent(QDropEvent* e)
{
    if (!e) {
        return;
    }
    if (e->source() == this || nullptr == e->source()) {
        return;
    }
    const QMimeData* mimeData = e->mimeData();
    if (mimeData->hasFormat(DAMIMEDATA_FORMAT_DADATA)) {
        //数据
        const DAMimeDataForData* datamime = qobject_cast< const DAMimeDataForData* >(mimeData);
        if (nullptr == datamime) {
            return;
        }
        QWidget* w = getWidgetUnderPos(e->pos());
        if (nullptr == w) {
            return;
        }
        if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(w)) {
            qDebug() << "dropEvent";
            DADialogDataframePlot* dlg = d_ptr->getDlgDataframeToPointVector();
            dlg->setDataManager(d_ptr->_dataManager);
            dlg->setCurrentData(datamime->getDAData());
            if (QDialog::Accepted != dlg->exec()) {
                d_ptr->_isStartDrag = false;
                e->ignore();
                return;
            }
            //获取数据
            QVector< QPointF > p;
            if (!dlg->getToVectorPointF(p) || p.empty()) {
                e->ignore();
                return;
            }
            addCurve_(p);
            e->acceptProposedAction();
        }
    }
    d_ptr->_isStartDrag = false;
}

}
