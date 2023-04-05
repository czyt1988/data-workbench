#include "DANodeMimeData.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DANodeMimeData
//===================================================
DANodeMimeData::DANodeMimeData(const DANodeMetaData& md) : DAMimeData()
{
    setNodeMetaData(md);
}

QStringList DANodeMimeData::formats() const
{
    return (QStringList() << formatString());
}

bool DANodeMimeData::hasFormat(const QString& mimeType) const
{
    return (mimeType == formatString());
}

DANodeMetaData DANodeMimeData::getNodeMetaData() const
{
    return (m_nodeMetaData);
}

void DANodeMimeData::setNodeMetaData(const DANodeMetaData& md)
{
    m_nodeMetaData = md;
}

QString DANodeMimeData::formatString()
{
    return "DA.NodeMetaData";
}

}
