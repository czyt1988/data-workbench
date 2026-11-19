#include "Archive/DAChart3DItemArchive.h"
#include <QDebug>
#include <QDomDocument>
#include <QIODevice>
#include <exception>
#include <utility>
#include "Archive/DAAbstractArchive.h"
#include "DAChart3DItemsManager.h"
#include "DAChart3DWidget.h"
#include "DAChart3DSerialize.h"
#include "qwt3d_plotitem.h"

namespace DA
{
namespace
{
/// 索引文件名（chart3d-items.xml）
const QString& chart3DItemsIndexFileName()
{
    static const QString s_fileName = QStringLiteral("chart3d-items.xml");
    return s_fileName;
}

/// 拼出存档内的相对路径
QString makeArchivePath(const QString& prefix, const QString& fileName)
{
    if (prefix.isEmpty()) {
        return fileName;
    }
    if (prefix.endsWith(QLatin1Char('/'))) {
        return prefix + fileName;
    }
    return prefix + QLatin1Char('/') + fileName;
}

/// 把快照的 key + rtti 索引写成 XML 字节流
QByteArray makeSnapshotIndexXml(const QList< DAChart3DItemSnapshot >& snapshots)
{
    QDomDocument doc(QStringLiteral("chart3d-items"));
    QDomElement root = doc.createElement(QStringLiteral("items"));
    doc.appendChild(root);
    for (const DAChart3DItemSnapshot& snap : snapshots) {
        if (snap.key.isEmpty()) {
            qWarning() << "DAChart3DItemArchive::makeSnapshotIndexXml: snapshot key is empty, skipped";
            continue;
        }
        QDomElement itemEle = doc.createElement(QStringLiteral("item"));
        itemEle.setAttribute(QStringLiteral("key"), snap.key);
        itemEle.setAttribute(QStringLiteral("rtti"), snap.rtti);
        root.appendChild(itemEle);
    }
    return doc.toByteArray();
}

/// 解析索引 XML，得到 (key, rtti) 列表；空索引返回 true
bool parseIndexXml(const QByteArray& data, QList< QPair< QString, int > >* result, QString* errorMessage)
{
    if (!result) {
        return false;
    }
    QDomDocument doc;
    QString parseError;
    if (!doc.setContent(data, &parseError)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("cannot parse chart3d-items.xml: %1").arg(parseError);
        }
        return false;
    }
    const QDomElement root = doc.documentElement();
    const QDomNodeList childs = root.childNodes();
    for (int i = 0; i < childs.size(); ++i) {
        const QDomElement itemEle = childs.at(i).toElement();
        if (itemEle.isNull() || itemEle.tagName() != QStringLiteral("item")) {
            continue;
        }
        bool rttiOk    = false;
        const QString key = itemEle.attribute(QStringLiteral("key"));
        const int rtti    = itemEle.attribute(QStringLiteral("rtti")).toInt(&rttiOk);
        if (key.isEmpty() || !rttiOk) {
            continue;
        }
        result->append(qMakePair(key, rtti));
    }
    return true;
}

/// 按已读出的索引数据逐个加载图元；单个图元缺失/反序列化失败时跳过并记录首个错误
bool loadFromIndexData(const QByteArray& indexData,
                       DAAbstractArchive* archive,
                       DAChart3DItemsManager* itemsMgr,
                       const QString& prefix,
                       QString* errorMessage)
{
    QList< QPair< QString, int > > keyToRtti;
    if (!parseIndexXml(indexData, &keyToRtti, errorMessage)) {
        return false;
    }
    if (keyToRtti.isEmpty()) {
        return true;
    }
    DAChart3DItemSerialize serializer;
    for (const QPair< QString, int >& itemInfo : std::as_const(keyToRtti)) {
        const QString itemPath = makeArchivePath(prefix, itemInfo.first);
        const QByteArray byte  = archive->read(itemPath);
        if (byte.isEmpty()) {
            qWarning() << "DAChart3DItemArchive::load: cannot read" << itemPath << "from archive, skipped";
            if (errorMessage && errorMessage->isEmpty()) {
                *errorMessage = QStringLiteral("cannot read %1 from archive").arg(itemPath);
            }
            continue;
        }
        Qwt3DPlotItem* plotItem = serializer.serializeIn(byte);
        if (!plotItem) {
            qWarning() << "DAChart3DItemArchive::load: cannot deserialize" << itemPath << "to a 3D plot item, skipped";
            if (errorMessage && errorMessage->isEmpty()) {
                *errorMessage = QStringLiteral("cannot deserialize %1 to a 3D plot item").arg(itemPath);
            }
            continue;
        }
        itemsMgr->recordItem(plotItem, itemInfo.first);
    }
    return true;
}
}  // namespace

/**
 * @brief 把三维图元数据写入存档
 *
 * 先写 chart3d-items.xml 索引（key + rtti），再为每个图元写一个 <prefix>/<key> 二进制文件。
 * 图元的序列化走生产路径 @ref DAChart3DItemSerialize。
 * @param archive 存档对象（zip/文件等），由调用方保证已在正确的线程/打开状态
 * @param itemsMgr 图元管理者（只读使用，不修改其登记）
 * @param prefix 存档内相对目录，例如 "chart3d-data"
 * @param errorMessage 可选，返回失败/部分失败原因（英文诊断，经 qWarning 记录）
 * @return 索引写入成功返回 true；单个图元写入失败/序列化异常只记录错误并继续（与二维 chart-data 一致）
 */
bool DAChart3DItemArchive::save(DAAbstractArchive* archive,
                                DAChart3DItemsManager* itemsMgr,
                                const QString& prefix,
                                QString* errorMessage)
{
    if (!archive || !itemsMgr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("null archive or 3D items manager");
        }
        return false;
    }
    // 索引 + 每图元字节都由 saveSnapshots 一次写完（与 save() 的输出完全同构）
    return saveSnapshots(archive, makeSnapshots(itemsMgr), prefix, errorMessage);
}

/**
 * @brief 从存档读回三维图元数据
 *
 * 先读 chart3d-items.xml 索引，再逐个读 <prefix>/<key> 反序列化并登记到 itemsMgr。
 * 索引缺失（旧工程/没有 3D 图元）视为成功且不加载任何图元；索引存在但读不到内容或 XML 损坏返回 false。
 * 存档未打开（@ref DAAbstractArchive::isOpen 返回 false）直接返回 false，避免把「读不到」误判成旧工程。
 * 单个图元数据缺失或反序列化失败时跳过该图元（与二维一致，不中断整体加载），并写入 errorMessage。
 * @param archive 存档对象
 * @param itemsMgr 输出：读出的图元登记到该管理者（谁持有图元所有权由读出的调用方决定）
 * @param prefix 存档内相对目录
 * @param errorMessage 可选，返回失败/跳过原因
 * @return 索引解析成功返回 true
 */
bool DAChart3DItemArchive::load(DAAbstractArchive* archive,
                                DAChart3DItemsManager* itemsMgr,
                                const QString& prefix,
                                QString* errorMessage)
{
    if (!archive || !itemsMgr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("null archive or 3D items manager");
        }
        return false;
    }
    // 未打开的存档读不到任何内容，会被「读不到即缺失」语义误判成旧工程而静默成功，这里前置拒绝。
    if (!archive->isOpen()) {
        qWarning() << "DAChart3DItemArchive::load: archive is not open, prefix" << prefix;
        if (errorMessage) {
            *errorMessage = QStringLiteral("archive is not open");
        }
        return false;
    }
    const QString indexPath = makeArchivePath(prefix, chart3DItemsIndexFileName());
    // 区分「旧工程没有索引（=没有 3D 图元，合法）」和「索引存在但读空/损坏」：
    // 先问存档是否包含索引（zip 等可精确回答）；不支持的存档退回「读取为空即视为缺失」。
    if (archive->contains(indexPath)) {
        const QByteArray indexData = archive->read(indexPath);
        if (indexData.isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("3D chart item index %1 is empty").arg(indexPath);
            }
            return false;
        }
        return loadFromIndexData(indexData, archive, itemsMgr, prefix, errorMessage);
    }
    const QByteArray indexData = archive->read(indexPath);
    if (indexData.isEmpty()) {
        // 存档无法判断存在性且读不到内容：按旧工程/没有 3D 图元处理
        return true;
    }
    return loadFromIndexData(indexData, archive, itemsMgr, prefix, errorMessage);
}

/**
 * @brief 索引文件名
 * @return 固定返回 "chart3d-items.xml"
 */
QString DAChart3DItemArchive::itemsIndexFileName()
{
    return chart3DItemsIndexFileName();
}

/**
 * @brief 把管理者里全部三维图元序列化为快照
 *
 * 必须在 GUI 线程调用（内部走生产路径 @ref DAChart3DItemSerialize::serializeOut，触碰图元对象）。
 * 单个图元 key 为空或序列化抛异常时跳过并 qWarning，与 @ref save 的容错口径一致；
 * itemsMgr 为空时返回空列表。
 * @param itemsMgr 图元管理者（只读，不修改其登记）
 * @return 快照列表；调用方把它交给后台任务的 @ref saveSnapshots 写存档
 */
QList< DAChart3DItemSnapshot > DAChart3DItemArchive::makeSnapshots(const DAChart3DItemsManager* itemsMgr)
{
    QList< DAChart3DItemSnapshot > snapshots;
    if (!itemsMgr) {
        return snapshots;
    }
    DAChart3DItemSerialize serializer;
    const QList< Qwt3DPlotItem* > items = itemsMgr->items();
    for (Qwt3DPlotItem* item : items) {
        const QString key = itemsMgr->itemToKey(item);
        if (key.isEmpty()) {
            qWarning() << "DAChart3DItemArchive::makeSnapshots: 3D plot item cannot find key, skipped";
            continue;
        }
        DAChart3DItemSnapshot snap;
        snap.key  = key;
        snap.rtti = item->rtti();
        try {
            snap.data = serializer.serializeOut(item);
        } catch (const std::exception& e) {
            qWarning() << "DAChart3DItemArchive::makeSnapshots: serialize" << key << "failed:" << e.what();
            continue;
        }
        snapshots.append(snap);
    }
    return snapshots;
}

/**
 * @brief 以 3D 子图为单位生成要落盘的快照
 *
 * 只序列化 @ref DAChart3DWidget::persistentItems（排除 transient 派生图元），key 取自该子图的
 * 图元管理者——与 XML `<items>` 记录的 key 同源，加载时才能按 key 挂回。
 * 必须在 GUI 线程调用。
 * @param chart3d 3D 子图，为空返回空列表
 * @return 快照列表
 */
QList< DAChart3DItemSnapshot > DAChart3DItemArchive::makeChart3DSnapshots(const DAChart3DWidget* chart3d)
{
    QList< DAChart3DItemSnapshot > snapshots;
    if (!chart3d) {
        return snapshots;
    }
    const DAChart3DItemsManager* itemsMgr = chart3d->items3DManager();
    DAChart3DItemSerialize serializer;
    const QList< Qwt3DPlotItem* > items = chart3d->persistentItems();
    for (Qwt3DPlotItem* item : items) {
        const QString key = itemsMgr ? itemsMgr->itemToKey(item) : QString();
        if (key.isEmpty()) {
            qWarning() << "DAChart3DItemArchive::makeChart3DSnapshots: 3D plot item has no key, skipped";
            continue;
        }
        DAChart3DItemSnapshot snap;
        snap.key  = key;
        snap.rtti = item->rtti();
        try {
            snap.data = serializer.serializeOut(item);
        } catch (const std::exception& e) {
            qWarning() << "DAChart3DItemArchive::makeChart3DSnapshots: serialize" << key << "failed:" << e.what();
            continue;
        }
        snapshots.append(snap);
    }
    return snapshots;
}

/**
 * @brief 把预序列化快照写入存档
 *
 * 在后台存档任务的 exec 内调用：只写字节，不触碰任何 Qwt3DPlotItem*（图元可能已被 GUI 线程
 * 删除/重建，见 BLK-01）。索引 XML 与 @ref save 的输出完全同构（key + rtti），加载端无需改动。
 * 单个快照写失败只记录错误并继续（与二维 chart-data 一致，不中断整体保存）。
 * @param archive 存档对象（zip/文件等），由调用方保证已在正确的线程/打开状态
 * @param snapshots GUI 线程经 @ref makeSnapshots 生成的快照
 * @param prefix 存档内相对目录，例如 "chart3d-data"
 * @param errorMessage 可选，返回失败/部分失败原因（英文诊断，经 qWarning 记录）
 * @return 索引写入成功返回 true
 */
bool DAChart3DItemArchive::saveSnapshots(DAAbstractArchive* archive,
                                         const QList< DAChart3DItemSnapshot >& snapshots,
                                         const QString& prefix,
                                         QString* errorMessage)
{
    if (!archive) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("null archive");
        }
        return false;
    }
    //! 1.先把key和rtti信息保存到xml
    const QByteArray indexData = makeSnapshotIndexXml(snapshots);
    if (indexData.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("failed to generate chart3d-items.xml");
        }
        return false;
    }
    const QString indexPath = makeArchivePath(prefix, chart3DItemsIndexFileName());
    if (!archive->write(indexPath, indexData)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("failed to write %1").arg(indexPath);
        }
        return false;
    }
    //! 2.写每个快照的数据（纯字节写入，不依赖图元对象存活）
    for (const DAChart3DItemSnapshot& snap : snapshots) {
        if (snap.key.isEmpty()) {
            continue;  // 索引阶段已 qWarning
        }
        const QString itemPath = makeArchivePath(prefix, snap.key);
        if (!archive->write(itemPath, snap.data)) {
            qWarning() << "DAChart3DItemArchive::saveSnapshots: failed to write" << itemPath << ", skipped";
            if (errorMessage) {
                *errorMessage = QStringLiteral("failed to write %1").arg(itemPath);
            }
        }
    }
    return true;
}

/**
 * @brief 删除没有被任何 3D 子图挂回去的图元
 *
 * 加载收尾用（figure XML 应用完之后）：索引里多出来的条目、坏索引导致的孤儿图元都还没有挂到
 * 任何 3D 子图上（plot() 为空），这里统一删除，避免每次加载泄漏一批图元。
 * @param itemsMgr 图元管理者（读出池）；为空返回 0
 * @return 删除的图元数量
 */
int DAChart3DItemArchive::deleteUnattachedItems(DAChart3DItemsManager* itemsMgr)
{
    if (!itemsMgr) {
        return 0;
    }
    int removed = 0;
    const QList< Qwt3DPlotItem* > items = itemsMgr->items();
    for (Qwt3DPlotItem* item : items) {
        if (!item || item->plot() != nullptr) {
            continue;
        }
        itemsMgr->unrecordItem(item);
        delete item;
        ++removed;
    }
    return removed;
}
}  // namespace DA