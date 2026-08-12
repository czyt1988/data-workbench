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

/**
 * @brief PrivateData 构造函数
 * @param p 指向 DAGraphicsLayout 的指针
 */
DAGraphicsLayout::PrivateData::PrivateData(DAGraphicsLayout* p) : q_ptr(p)
{
}
//----------------------------------------------------
//
//----------------------------------------------------

/**
 * @brief DAGraphicsLayout 构造函数
 * @param par 父对象指针
 */
DAGraphicsLayout::DAGraphicsLayout(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief DAGraphicsLayout 析构函数
 */
DAGraphicsLayout::~DAGraphicsLayout()
{
}

/**
 * @brief 获取布局名称
 * @return 布局名称
 * @sa setName
 */
QString DAGraphicsLayout::getName() const
{
    return d_ptr->mName;
}

/**
 * @brief 设置布局名称
 * @param v 布局名称
 * @sa getName
 */
void DAGraphicsLayout::setName(const QString& v)
{
    d_ptr->mName = v;
}

/**
 * @brief 添加图形项到布局中
 * @param v 要添加的 QGraphicsItem 指针
 */
void DAGraphicsLayout::addItem(QGraphicsItem* v)
{
    d_ptr->mItems.append(v);
}

}
