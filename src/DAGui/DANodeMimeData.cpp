#include "DANodeMimeData.h"

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DANodeMimeData
//===================================================
DANodeMimeData::DANodeMimeData(const DAPyNodeMetaData& md) : DAMimeData()
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

DAPyNodeMetaData DANodeMimeData::getNodeMetaData() const
{
    return (m_nodeMetaData);
}

void DANodeMimeData::setNodeMetaData(const DAPyNodeMetaData& md)
{
    m_nodeMetaData = md;
}

QString DANodeMimeData::formatString()
{
    return "DA.NodeMetaData";
}

}
