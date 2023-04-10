#include "DAAppFigureWidget.h"
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDebug>
#include "DAMimeDataForData.h"
#include "DAMimeDataFormats.h"
#include "DADialogDataframeToPointVector.h"
namespace DA
{
class DAAppFigureWidgetPrivate
{
public:
    DA_IMPL_PUBLIC(DAAppFigureWidget)
    DAAppFigureWidgetPrivate(DAAppFigureWidget* p);
    //获取dlg指针，如果为nullptr，则创建
    DADialogDataframeToPointVector* getDlgDataframeToPointVector();

public:
    bool _isStartDrag { false };
    DADialogDataframeToPointVector* _dlgDataframeToPointVector { nullptr };
};
DAAppFigureWidgetPrivate::DAAppFigureWidgetPrivate(DAAppFigureWidget* p) : q_ptr(p)
{
}

DADialogDataframeToPointVector* DAAppFigureWidgetPrivate::getDlgDataframeToPointVector()
{
    if (!_dlgDataframeToPointVector) {
        _dlgDataframeToPointVector = new DADialogDataframeToPointVector(q_ptr);
    }
    return _dlgDataframeToPointVector;
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

void DAAppFigureWidget::dragEnterEvent(QDragEnterEvent* e)
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
            e->ignore();
            return;
        }
        if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(w)) {
            qDebug() << "dragEnterEvent setDropAction(Qt::CopyAction)";
            e->setDropAction(Qt::CopyAction);
            d_ptr->_isStartDrag = true;
            e->accept();
        }
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
            return;
        }
        if (DAChartWidget* chart = qobject_cast< DAChartWidget* >(w)) {
            qDebug() << "dragMoveEvent setDropAction(Qt::CopyAction)";
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }
    }
}

void DAAppFigureWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
    if (!e) {
        return;
    }
    d_ptr->_isStartDrag = false;
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
            DADialogDataframeToPointVector* dlg = d_ptr->getDlgDataframeToPointVector();
            if (QDialog::Accepted != dlg->exec()) {
                d_ptr->_isStartDrag = false;
                e->accept();
                return;
            }
            e->accept();
        }
    }
    d_ptr->_isStartDrag = false;
}

}
