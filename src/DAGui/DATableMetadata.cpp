#include "DATableMetadata.h"
#include "DATableStyleManager.h"
#include "DATableViewStateStore.h"
#include "DATableConditionalFormatStore.h"
#include "DATableComputedColumnStore.h"
#include "DATableValidationStore.h"
#include "DATableNoteStore.h"
#include <QDomDocument>
#include <QDomElement>

namespace DA
{

/**
 * @brief 构造函数：创建各子存储
 * @param parent 父对象（一般为注册表）
 */
DATableMetadata::DATableMetadata(QObject* parent) : QObject(parent)
{
    mStyleManager      = new DATableStyleManager(this);
    mViewState         = new DATableViewStateStore(this);
    mConditionalFormat = new DATableConditionalFormatStore(this);
    mComputedColumns   = new DATableComputedColumnStore(this);
    mValidations       = new DATableValidationStore(this);
    mNotes             = new DATableNoteStore(this);
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
 * @brief 条件格式子存储
 * @return 非空
 */
DATableConditionalFormatStore* DATableMetadata::conditionalFormat() const
{
    return mConditionalFormat;
}

/**
 * @brief 计算列子存储（004 T1.23）
 * @return 非空
 */
DATableComputedColumnStore* DATableMetadata::computedColumns() const
{
    return mComputedColumns;
}

/**
 * @brief 数据校验规则子存储（005 T1.25）
 * @return 非空
 */
DATableValidationStore* DATableMetadata::validations() const
{
    return mValidations;
}

/**
 * @brief 单元格批注子存储（005 T1.26）
 * @return 非空
 */
DATableNoteStore* DATableMetadata::notes() const
{
    return mNotes;
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
    if (mConditionalFormat) {
        mConditionalFormat->toXml(doc, tableEle);
    }
    if (mComputedColumns) {
        mComputedColumns->toXml(doc, tableEle);
    }
    if (mValidations) {
        mValidations->toXml(doc, tableEle);
    }
    if (mNotes) {
        mNotes->toXml(doc, tableEle);
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
    if (mConditionalFormat) {
        QDomElement cf = tableEle.firstChildElement(QStringLiteral("conditional-format"));
        if (!cf.isNull()) {
            mConditionalFormat->fromXml(cf);
        }
    }
    if (mComputedColumns) {
        QDomElement cc = tableEle.firstChildElement(QStringLiteral("computed-column"));
        if (!cc.isNull()) {
            // 打开工程不重算（R7）：只恢复定义与状态，不触发求值
            mComputedColumns->fromXml(cc);
        }
    }
    if (mValidations) {
        QDomElement va = tableEle.firstChildElement(QStringLiteral("validation"));
        if (!va.isNull()) {
            mValidations->fromXml(va);
        }
    }
    if (mNotes) {
        QDomElement nt = tableEle.firstChildElement(QStringLiteral("note"));
        if (!nt.isNull()) {
            mNotes->fromXml(nt);
        }
    }
    return true;
}

/**
 * @brief 清空全部子存储（004 T1.27 就地形状变换用）
 *
 * 转置/melt 就地替换后行位置键（样式）与列名引用（列宽/计算列）全部失去意义，
 * 整束清除；调用方负责先做快照（snapshotToString）以便撤销还原。
 */
void DATableMetadata::clearAll()
{
    if (mStyleManager) {
        mStyleManager->clearAll();
    }
    if (mViewState) {
        mViewState->clear();
    }
    if (mConditionalFormat) {
        mConditionalFormat->clearAll();
    }
    if (mComputedColumns) {
        mComputedColumns->clear();
    }
    if (mValidations) {
        mValidations->clearAll();
    }
    if (mNotes) {
        mNotes->clearAll();
    }
}

/**
 * @brief 整束快照为 XML 文本（就地形状变换与元数据清理同一撤销单元用，004 T1.27）
 * @return <table> 元素的 XML 字符串；空束返回空串
 */
QString DATableMetadata::snapshotToString() const
{
    QDomDocument doc;
    QDomElement tableEle = doc.createElement(QStringLiteral("table"));
    doc.appendChild(tableEle);
    toXml(doc, tableEle);
    return doc.toString();
}

/**
 * @brief 从快照 XML 文本整束恢复
 *
 * 各子存储 fromXml 先自清空再读入，因此缺失的段等价清空——快照为空串时整束清空。
 * @param snapshot snapshotToString 的输出
 * @return 解析成功返回 true
 */
bool DATableMetadata::restoreFromString(const QString& snapshot)
{
    if (snapshot.isEmpty()) {
        clearAll();
        return true;
    }
    QDomDocument doc;
    if (!doc.setContent(snapshot)) {
        return false;
    }
    QDomElement tableEle = doc.documentElement();
    if (tableEle.isNull()) {
        return false;
    }
    // 先整束清空再读入：快照里没有的段（如无条件格式）也要清掉
    clearAll();
    return fromXml(tableEle);
}

/**
 * @brief 各类别条目数统计（就地替换确认框列出将被清除的类别与条目数，004 T1.27）
 * @return (类别显示名, 条目数) 列表，只含非空类别，顺序：样式/列视图/条件格式/计算列/校验/批注
 */
QList< QPair< QString, int > > DATableMetadata::entryCountsByCategory() const
{
    QList< QPair< QString, int > > res;
    if (mStyleManager && !mStyleManager->isEmpty()) {
        const int n = mStyleManager->styledCells().size() + mStyleManager->styledRows().size()
                      + mStyleManager->styledColumns().size() + mStyleManager->styledFormatColumns().size();
        res.append(qMakePair(tr("Cell styles"), n));  // cn:样式
    }
    if (mViewState && !mViewState->isEmpty()) {
        // 006：列视图设置（列宽 + 列序 + 隐藏 + 冻结计数一并计入，冻结算 1 条）
        const int n = mViewState->columnWidthEntries().size() + mViewState->columnOrder().size()
                      + mViewState->hiddenColumns().size() + (mViewState->freezeColumnCount() > 0 ? 1 : 0);
        res.append(qMakePair(tr("Column view settings"), n));  // cn:列视图设置
    }
    if (mConditionalFormat && !mConditionalFormat->isEmpty()) {
        res.append(qMakePair(tr("Conditional format rules"), mConditionalFormat->ruleCount()));  // cn:条件格式规则
    }
    if (mComputedColumns && !mComputedColumns->isEmpty()) {
        res.append(qMakePair(tr("Computed columns"), mComputedColumns->count()));  // cn:计算列
    }
    if (mValidations && !mValidations->isEmpty()) {
        res.append(qMakePair(tr("Validation rules"), mValidations->count()));  // cn:校验规则
    }
    if (mNotes && !mNotes->isEmpty()) {
        res.append(qMakePair(tr("Cell notes"), mNotes->count()));  // cn:批注
    }
    return res;
}

/**
 * @brief 全部子存储均为空
 * @return 样式与视图状态都空返回 true
 */
bool DATableMetadata::isEmpty() const
{
    const bool styleEmpty = !mStyleManager || mStyleManager->isEmpty();
    const bool viewEmpty  = !mViewState || mViewState->isEmpty();
    const bool cfEmpty    = !mConditionalFormat || mConditionalFormat->isEmpty();
    const bool ccEmpty    = !mComputedColumns || mComputedColumns->isEmpty();
    const bool vaEmpty    = !mValidations || mValidations->isEmpty();
    const bool noteEmpty  = !mNotes || mNotes->isEmpty();
    return styleEmpty && viewEmpty && cfEmpty && ccEmpty && vaEmpty && noteEmpty;
}

}  // end of namespace DA
