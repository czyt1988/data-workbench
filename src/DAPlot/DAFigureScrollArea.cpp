#include "DAFigureScrollArea.h"
#include "QImFigureWidget.h"
#include <QUndoStack>
#include <QUuid>
namespace DA
{
class DAFigureScrollArea::PrivateData
{
    DA_DECLARE_PUBLIC(DAFigureScrollArea)
public:
    PrivateData(DAFigureScrollArea* p);
    QString id;
    QUndoStack m_undoStack;  ///<
    QImFigureWidget* figure { nullptr };
};

DAFigureScrollArea::PrivateData::PrivateData(DAFigureScrollArea* p) : q_ptr(p)
{
    id = QUuid::createUuid().toString();
}

//----------------------------------------------------
// DAFigureScrollArea
//----------------------------------------------------

DAFigureScrollArea::DAFigureScrollArea(QWidget* parent) : QScrollArea(parent), DA_PIMPL_CONSTRUCT
{
}

DAFigureScrollArea::~DAFigureScrollArea()
{
}

QImFigureWidget* DAFigureScrollArea::figure() const
{
}

QString DAFigureScrollArea::getFigureId() const
{
    return d_ptr->id;
}

void DAFigureScrollArea::setFigureId(const QString& id)
{
    d_ptr->id = id;
}

void DAFigureScrollArea::init()
{
    DA_D(d);
    setWindowIcon(QIcon(":/DAPlot/icon/icon/figure.svg"));
    d->figure = new QIM::QImFigureWidget();
}

}
