#include "Archive/DAChartItemArchive.h"
#include <QDebug>
#include <QDomDocument>
#include <QIODevice>
#include <exception>
#include <utility>
#include "Archive/DAAbstractArchive.h"
#include "Chart/DAChartItemsManager.h"
#include "DAChartSerialize.h"
#include "qwt_plot_item.h"

namespace DA
{
namespace
{
/// 索引文件名（chart-items.xml）
const QString& chartItemsIndexFileName()
{
    static const QString s_fileName = QStringLiteral("chart-items.xml");
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

/// 把图元 key + rtti 索引写成 XML 字节流
QByteArray makeIndexXml(const DAChartItemsManager* itemsMgr)
{
    QDomDocument doc(QStringLiteral("chart-items"));
    QDomElement root = doc.createElement(QStringLiteral("items"));
    doc.appendChild(root);
    const QList< QwtPlotItem* > items = itemsMgr->items();
    for (QwtPlotItem* item : items) {
        const QString key = itemsMgr->itemToKey(item);
        if (key.isEmpty()) {
            qWarning() << "DAChartItemArchive::makeIndexXml: plot item cannot find key";
            continue;
        }
        QDomElement itemEle = doc.createElement(QStringLiteral("item"));
        itemEle.setAttribute(QStringLiteral("key"), key);
        itemEle.setAttribute(QStringLiteral("rtti"), item->rtti());
        root.appendChild(itemEle);
    }
    return doc.toByteArray();
}

/// 把快照的 key + rtti 索引写成 XML 字节流（与 makeIndexXml 对同一批图元输出完全一致）
QByteArray makeSnapshotIndexXml(const QList< DAChartItemSnapshot >& snapshots)
{
    QDomDocument doc(QStringLiteral("chart-items"));
    QDomElement root = doc.createElement(QStringLiteral("items"));
    doc.appendChild(root);
    for (const DAChartItemSnapshot& snap : snapshots) {
        if (snap.key.isEmpty()) {
            qWarning() << "DAChartItemArchive::makeSnapshotIndexXml: snapshot key is empty, skipped";
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
            *errorMessage = QStringLiteral("cannot parse chart-items.xml: %1").arg(parseError);
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
                       DAChartItemsManager* itemsMgr,
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
    DAChartItemSerialize serializer;
    for (const QPair< QString, int >& itemInfo : std::as_const(keyToRtti)) {
        const QString itemPath = makeArchivePath(prefix, itemInfo.first);
        const QByteArray byte  = archive->read(itemPath);
        if (byte.isEmpty()) {
            qWarning() << "DAChartItemArchive::load: cannot read" << itemPath << "from archive, skipped";
            if (errorMessage && errorMessage->isEmpty()) {
                *errorMessage = QStringLiteral("cannot read %1 from archive").arg(itemPath);
            }
            continue;
        }
        QwtPlotItem* plotItem = serializer.serializeIn(byte);
        if (!plotItem) {
            qWarning() << "DAChartItemArchive::load: cannot deserialize" << itemPath << "to a plot item, skipped";
            if (errorMessage && errorMessage->isEmpty()) {
                *errorMessage = QStringLiteral("cannot deserialize %1 to a plot item").arg(itemPath);
            }
            continue;
        }
        itemsMgr->recordItem(plotItem, itemInfo.first);
    }
    itemsMgr->updateKeyID();
    return true;
}
}  // namespace

/**
 * @brief 把图元数据写入存档
 *
 * 先写 chart-items.xml 索引（key + rtti），再为每个图元写一个 <prefix>/<key> 二进制文件。
 * 图元的序列化走生产路径 @ref DAChartItemSerialize。
 * @param archive 存档对象（zip/文件等），由调用方保证已在正确的线程/打开状态
 * @param itemsMgr 图元管理者，保存结束后其 key 映射可一并释放
 * @param prefix 存档内相对目录，例如 "chart-data"、"signal-analysis/chart-data"
 * @param errorMessage 可选，返回失败/部分失败原因（英文诊断，经 qWarning 记录）
 * @return 索引写入成功返回 true；单个图元写入失败/序列化异常只记录错误并继续（与既有存档行为一致）
 */
bool DAChartItemArchive::save(DAAbstractArchive* archive,
                              DAChartItemsManager* itemsMgr,
                              const QString& prefix,
                              QString* errorMessage)
{
    if (!archive || !itemsMgr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("null archive or items manager");
        }
        return false;
    }
    //! 1.先把key和rtti信息保存到xml
    const QByteArray indexData = makeIndexXml(itemsMgr);
    // QDomDocument::toByteArray 理论上不会返回空；这里保留防御，避免写入0字节索引
    if (indexData.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("failed to generate chart-items.xml");
        }
        return false;
    }
    const QString indexPath = makeArchivePath(prefix, chartItemsIndexFileName());
    if (!archive->write(indexPath, indexData)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("failed to write %1").arg(indexPath);
        }
        return false;
    }
    //! 2.写每个图元的数据
    // 保持既有行为：单个图元写失败只记录错误并继续，整体仍返回 true（避免一个坏图元中断整个工程保存）
    DAChartItemSerialize serializer;
    const QList< QwtPlotItem* > items = itemsMgr->items();
    for (QwtPlotItem* item : items) {
        const QString key = itemsMgr->itemToKey(item);
        if (key.isEmpty()) {
            qWarning() << "DAChartItemArchive::save: plot item cannot find key, skipped";
            continue;
        }
        const QString itemPath = makeArchivePath(prefix, key);
        try {
            const QByteArray itemData = serializer.serializeOut(item);
            if (!archive->write(itemPath, itemData)) {
                qWarning() << "DAChartItemArchive::save: failed to write" << itemPath << ", skipped";
                if (errorMessage) {
                    *errorMessage = QStringLiteral("failed to write %1").arg(itemPath);
                }
            }
        } catch (const std::exception& e) {
            qWarning() << "DAChartItemArchive::save: serialize" << itemPath << "failed:" << e.what();
            if (errorMessage) {
                *errorMessage = QStringLiteral("failed to serialize %1: %2").arg(itemPath, QString::fromUtf8(e.what()));
            }
        }
    }
    return true;
}

/**
 * @brief 从存档读回图元数据
 *
 * 先读 chart-items.xml 索引，再逐个读 <prefix>/<key> 反序列化并登记到 itemsMgr。
 * 索引缺失（旧工程/空绘图）视为成功且不加载任何图元；索引存在但读不到内容或 XML 损坏返回 false。
 * 存档未打开（@ref DAAbstractArchive::isOpen 返回 false）直接返回 false，避免把「读不到」误判成旧工程。
 * 单个图元数据缺失或反序列化失败时跳过该图元（与既有行为一致，不中断整体加载），并写入 errorMessage。
 * @note 无法区分「缺失」与「存在但为空」的存档（未覆盖 contains 的存档）仍按「读不到即缺失」处理。
 * @param archive 存档对象
 * @param itemsMgr 输出：读出的图元登记到该管理者（谁持有图元所有权由读出的调用方决定）
 * @param prefix 存档内相对目录
 * @param errorMessage 可选，返回失败/跳过原因
 * @return 索引解析成功返回 true
 */
bool DAChartItemArchive::load(DAAbstractArchive* archive,
                              DAChartItemsManager* itemsMgr,
                              const QString& prefix,
                              QString* errorMessage)
{
    if (!archive || !itemsMgr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("null archive or items manager");
        }
        return false;
    }
    // 未打开的存档（zip 未 open/create）读不到任何内容，会被下面的「读不到即缺失」
    // 语义误判成旧工程/空绘图而静默返回 true；这里必须前置拒绝。
    if (!archive->isOpen()) {
        qWarning() << "DAChartItemArchive::load: archive is not open, prefix" << prefix;
        if (errorMessage) {
            *errorMessage = QStringLiteral("archive is not open");
        }
        return false;
    }
    const QString indexPath = makeArchivePath(prefix, chartItemsIndexFileName());
    // 区分「旧工程没有索引（=空绘图，合法）」和「索引存在但读空/损坏」：
    // 先问存档是否包含索引（zip 等可精确回答）；不支持的存档退回「读取为空即视为缺失」。
    if (archive->contains(indexPath)) {
        const QByteArray indexData = archive->read(indexPath);
        if (indexData.isEmpty()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("chart item index %1 is empty").arg(indexPath);
            }
            return false;
        }
        return loadFromIndexData(indexData, archive, itemsMgr, prefix, errorMessage);
    }
    const QByteArray indexData = archive->read(indexPath);
    if (indexData.isEmpty()) {
        // 存档无法判断存在性且读不到内容：按旧工程/空绘图处理
        return true;
    }
    return loadFromIndexData(indexData, archive, itemsMgr, prefix, errorMessage);
}

/**
 * @brief 索引文件名
 * @return 固定返回 "chart-items.xml"
 */
QString DAChartItemArchive::itemsIndexFileName()
{
    return chartItemsIndexFileName();
}

/**
 * @brief 把管理者里全部图元序列化为快照
 *
 * 必须在 GUI 线程调用（内部走生产路径 @ref DAChartItemSerialize::serializeOut，触碰 QwtPlotItem*）。
 * 单个图元 key 为空或序列化抛异常时跳过并 qWarning，与 @ref save 的容错口径一致；
 * itemsMgr 为空时返回空列表。
 * @param itemsMgr 图元管理者（只读，不修改其登记）
 * @return 快照列表；调用方把它交给后台任务的 @ref saveSnapshots 写存档
 */
QList< DAChartItemSnapshot > DAChartItemArchive::makeSnapshots(const DAChartItemsManager* itemsMgr)
{
    QList< DAChartItemSnapshot > snapshots;
    if (!itemsMgr) {
        return snapshots;
    }
    DAChartItemSerialize serializer;
    const QList< QwtPlotItem* > items = itemsMgr->items();
    for (QwtPlotItem* item : items) {
        const QString key = itemsMgr->itemToKey(item);
        if (key.isEmpty()) {
            qWarning() << "DAChartItemArchive::makeSnapshots: plot item cannot find key, skipped";
            continue;
        }
        DAChartItemSnapshot snap;
        snap.key  = key;
        snap.rtti = item->rtti();
        try {
            snap.data = serializer.serializeOut(item);
        } catch (const std::exception& e) {
            qWarning() << "DAChartItemArchive::makeSnapshots: serialize" << key << "failed:" << e.what();
            continue;
        }
        snapshots.append(snap);
    }
    return snapshots;
}

/**
 * @brief 把预序列化快照写入存档
 *
 * 在后台存档任务的 exec 内调用：只写字节，不触碰任何 QwtPlotItem*（图元可能已被 GUI
 * 线程删除/重建，见 BLK-01）。索引 XML 与 @ref save 的 makeIndexXml 输出完全同构
 * （key + rtti），加载端 @ref load 无需改动即可读回。
 * 单个快照写失败只记录错误并继续（与既有存档行为一致，不中断整体保存）。
 * @param archive 存档对象（zip/文件等），由调用方保证已在正确的线程/打开状态
 * @param snapshots GUI 线程经 @ref makeSnapshots 生成的快照
 * @param prefix 存档内相对目录，例如 "chart-data"、"signal-analysis/chart-data"
 * @param errorMessage 可选，返回失败/部分失败原因（英文诊断，经 qWarning 记录）
 * @return 索引写入成功返回 true
 */
bool DAChartItemArchive::saveSnapshots(DAAbstractArchive* archive,
                                       const QList< DAChartItemSnapshot >& snapshots,
                                       const QString& prefix,
                                       QString* errorMessage)
{
    if (!archive) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("null archive");
        }
        return false;
    }
    //! 1.先把key和rtti信息保存到xml（与 save() 的索引完全同构）
    const QByteArray indexData = makeSnapshotIndexXml(snapshots);
    // QDomDocument::toByteArray 理论上不会返回空；这里保留防御，避免写入0字节索引
    if (indexData.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("failed to generate chart-items.xml");
        }
        return false;
    }
    const QString indexPath = makeArchivePath(prefix, chartItemsIndexFileName());
    if (!archive->write(indexPath, indexData)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("failed to write %1").arg(indexPath);
        }
        return false;
    }
    //! 2.写每个快照的数据（纯字节写入，不依赖图元对象存活）
    for (const DAChartItemSnapshot& snap : snapshots) {
        if (snap.key.isEmpty()) {
            continue;  // 索引阶段已 qWarning
        }
        const QString itemPath = makeArchivePath(prefix, snap.key);
        if (!archive->write(itemPath, snap.data)) {
            qWarning() << "DAChartItemArchive::saveSnapshots: failed to write" << itemPath << ", skipped";
            if (errorMessage) {
                *errorMessage = QStringLiteral("failed to write %1").arg(itemPath);
            }
        }
    }
    return true;
}
}  // namespace DA
