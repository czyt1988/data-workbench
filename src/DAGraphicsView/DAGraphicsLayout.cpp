#include "DAGraphicsLayout.h"
namespace DA
{
class DAGraphicsLayout::PrivateData
{
    DA_DECLARE_PUBLIC(DAGraphicsLayout)
public:
    PrivateData(DAGraphicsLayout* p);

public:
    QString mName;
    QList< QGraphicsItem* > mItems;
};

DAGraphicsLayout::PrivateData::PrivateData(DAGraphicsLayout* p) : q_ptr(p)
{
}
//----------------------------------------------------
//
//----------------------------------------------------

DAGraphicsLayout::DAGraphicsLayout(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

DAGraphicsLayout::~DAGraphicsLayout()
{
}

QString DAGraphicsLayout::getName() const
{
    return d_ptr->mName;
}

void DAGraphicsLayout::setName(const QString v)
{
    d_ptr->mName = v;
}

void DAGraphicsLayout::addItem(QGraphicsItem* v)
{
    d_ptr->mItems.append(v);
}

}
