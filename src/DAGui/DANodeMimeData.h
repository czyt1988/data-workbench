#ifndef FCNODEMIMEDATA_H
#define FCNODEMIMEDATA_H
#include <QtCore/qglobal.h>
#include "DAGlobals.h"
#include "DAMimeData.h"
#include "DANodeMetaData.h"
#include "DAGuiAPI.h"
namespace DA
{
/**
 * @brief 节点移动的mime data
 */
class DAGUI_API DANodeMimeData : public DAMimeData
{
    Q_OBJECT
public:
    DANodeMimeData(const DANodeMetaData& md);
    //节点格式 - MIME_STANDARD_NODE
    virtual QStringList formats() const;
    virtual bool hasFormat(const QString& mimeType) const;

    //设置/获取节点名
    DANodeMetaData getNodeMetaData() const;
    void setNodeMetaData(const DANodeMetaData& md);

    //格式字符串
    static QString formatString();

private:
    DANodeMetaData m_nodeMetaData;
};
}  // namespace DA
#endif  // GNODEMIMEDATA_H
