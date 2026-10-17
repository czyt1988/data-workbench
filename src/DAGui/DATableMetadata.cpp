#include "DATableMetadata.h"
#include "DATableStyleManager.h"
#include "DATableViewStateStore.h"

namespace DA
{

/**
 * @brief 构造函数：创建两个 v1 子存储
 * @param parent 父对象（一般为注册表）
 */
DATableMetadata::DATableMetadata(QObject* parent) : QObject(parent)
{
    mStyleManager = new DATableStyleManager(this);
    mViewState    = new DATableViewStateStore(this);
}

DATableMetadata::~DATableMetadata()
{
    // 子存储 parent = this，由 Qt 自动销毁
}

/**
 * @brief 样式管理器
 * @return 非空
 */
DATableStyleManager* DATableMetadata::styleManager() const
{
    return mStyleManager;
}

/**
 * @brief 视图状态子存储
 * @return 非空
 */
DATableViewStateStore* DATableMetadata::viewState() const
{
    return mViewState;
}

/**
 * @brief 聚合序列化
 *
 * 样式段（column-style/row-style/cell/col-format）与 <view-state> 段均作为 tableEle 的子节点。
 * @param doc QDomDocument
 * @param tableEle table 元素
 */
void DATableMetadata::toXml(QDomDocument& doc, QDomElement& tableEle) const
{
    if (mStyleManager) {
        mStyleManager->toXml(doc, tableEle);
    }
    if (mViewState) {
        mViewState->toXml(doc, tableEle);
    }
}

/**
 * @brief 聚合反序列化
 *
 * 样式段由 DATableStyleManager::fromXml 解析（忽略未知标签，含 <view-state>）；
 * 本函数再单独取出 <view-state> 子元素交给视图状态子存储。旧 table-styles.xml 无该段，
 * viewState 保持为空。
 * @param tableEle table 元素
 * @return 始终返回 true
 */
bool DATableMetadata::fromXml(const QDomElement& tableEle)
{
    if (mStyleManager) {
        mStyleManager->fromXml(tableEle);
    }
    if (mViewState) {
        QDomElement vs = tableEle.firstChildElement(QStringLiteral("view-state"));
        if (!vs.isNull()) {
            mViewState->fromXml(vs);
        }
    }
    return true;
}

/**
 * @brief 全部子存储均为空
 * @return 样式与视图状态都空返回 true
 */
bool DATableMetadata::isEmpty() const
{
    const bool styleEmpty = !mStyleManager || mStyleManager->isEmpty();
    const bool viewEmpty  = !mViewState || mViewState->isEmpty();
    return styleEmpty && viewEmpty;
}

}  // end of namespace DA
