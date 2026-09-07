#include "DASqliteLazyTable.h"
#include <QDataStream>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

//===================================================
// DASqliteLazyTable
//===================================================

DASqliteLazyTable::DASqliteLazyTable()
{
}

void DASqliteLazyTable::setConnectionName(const QString& connName)
{
    mConnectionName = connName;
    invalidateCache();
}

QString DASqliteLazyTable::connectionName() const
{
    return mConnectionName;
}

void DASqliteLazyTable::setSelectSql(const QString& sql)
{
    mSelectSql = sql;
    invalidateCache();
}

QString DASqliteLazyTable::selectSql() const
{
    return mSelectSql;
}

QString DASqliteLazyTable::normalizedSelectSql() const
{
    QString sql = mSelectSql.trimmed();
    while (sql.endsWith(QLatin1Char(';'))) {
        sql.chop(1);
    }
    return sql;
}

QSqlDatabase DASqliteLazyTable::database() const
{
    if (mConnectionName.isEmpty() || !QSqlDatabase::contains(mConnectionName)) {
        return QSqlDatabase();
    }
    return QSqlDatabase::database(mConnectionName, false);
}

bool DASqliteLazyTable::isConnectionAvailable() const
{
    QSqlDatabase db = database();
    return db.isValid() && db.isOpen();
}

int DASqliteLazyTable::fetchCount() const
{
    return mFetchCount;
}

void DASqliteLazyTable::invalidateCache() const
{
    mSchemaValid   = false;
    mRowCountValid = false;
    mColumnNames.clear();
    mColumnMetaTypes.clear();
    mRowCountCache = 0;
}

DA::DAAbstractData::DataType DASqliteLazyTable::getDataType() const
{
    // 枚举无法表达数据库表类型，消费端应经tableSource()/typeIdentifier()分派
    return DA::DAAbstractData::TypeInnerData;
}

QVariant DASqliteLazyTable::toVariant(std::size_t dim1, std::size_t dim2) const
{
    if (!isConnectionAvailable()) {
        return QVariant();
    }
    QSqlQuery q(database());
    q.prepare(QStringLiteral("SELECT * FROM (%1) LIMIT 1 OFFSET %2").arg(normalizedSelectSql()).arg(dim1));
    if (!q.exec() || !q.next()) {
        qWarning() << "DASqliteLazyTable::toVariant query failed:" << q.lastError().text();
        return QVariant();
    }
    if (dim2 >= static_cast< std::size_t >(q.record().count())) {
        return QVariant();
    }
    return q.value(static_cast< int >(dim2));
}

bool DASqliteLazyTable::setValue(std::size_t, std::size_t, const QVariant&)
{
    // 只读数据源：写回应由数据库插件经事务/命令另行实现
    return false;
}

DA::DATableDataSource* DASqliteLazyTable::tableSource()
{
    return this;
}

const DA::DATableDataSource* DASqliteLazyTable::tableSource() const
{
    return this;
}

bool DASqliteLazyTable::isReferenceData() const
{
    return true;
}

bool DASqliteLazyTable::supportsUndoSnapshot() const
{
    return false;
}

QString DASqliteLazyTable::typeIdentifier() const
{
    return QStringLiteral("DATableDataTest.SqliteLazyTable");
}

void DASqliteLazyTable::write(QDataStream& out)
{
    // 引用payload：连接名+SELECT语句；真实插件还应保存连接配置（主机/库名等）
    out << mConnectionName << mSelectSql;
}

bool DASqliteLazyTable::read(QDataStream& in)
{
    in >> mConnectionName >> mSelectSql;
    invalidateCache();
    return in.status() == QDataStream::Ok;
}

std::size_t DASqliteLazyTable::tableRowCount() const
{
    if (mRowCountValid) {
        return mRowCountCache;
    }
    if (!isConnectionAvailable()) {
        return 0;
    }
    QSqlQuery q(database());
    // 子查询COUNT，兼容任意SELECT（含WHERE/JOIN）
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM (%1)").arg(normalizedSelectSql())) || !q.next()) {
        qWarning() << "DASqliteLazyTable::tableRowCount query failed:" << q.lastError().text();
        return 0;
    }
    mRowCountCache = q.value(0).toULongLong();
    mRowCountValid = true;
    return mRowCountCache;
}

void DASqliteLazyTable::ensureSchema() const
{
    if (mSchemaValid || !isConnectionAvailable()) {
        return;
    }
    QSqlQuery q(database());
    if (!q.exec(QStringLiteral("SELECT * FROM (%1) LIMIT 0").arg(normalizedSelectSql()))) {
        qWarning() << "DASqliteLazyTable::ensureSchema query failed:" << q.lastError().text();
        return;
    }
    QSqlRecord rec = q.record();
    mColumnNames.clear();
    mColumnMetaTypes.clear();
    for (int i = 0; i < rec.count(); ++i) {
        QSqlField f = rec.field(i);
        mColumnNames.append(f.name());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        mColumnMetaTypes.append(static_cast< int >(f.type()));  // Qt5: QVariant::Type与QMetaType id一致
#else
        mColumnMetaTypes.append(f.metaType().id());
#endif
    }
    mSchemaValid = true;
}

std::size_t DASqliteLazyTable::tableColumnCount() const
{
    ensureSchema();
    return static_cast< std::size_t >(mColumnNames.size());
}

QString DASqliteLazyTable::tableColumnName(std::size_t column) const
{
    ensureSchema();
    if (column >= static_cast< std::size_t >(mColumnNames.size())) {
        return QString();
    }
    return mColumnNames[ static_cast< int >(column) ];
}

int DASqliteLazyTable::tableColumnType(std::size_t column) const
{
    ensureSchema();
    if (column >= static_cast< std::size_t >(mColumnMetaTypes.size())) {
        return QMetaType::UnknownType;
    }
    return mColumnMetaTypes[ static_cast< int >(column) ];
}

DA::DATableDataBlock DASqliteLazyTable::fetchBlock(std::size_t startRow, std::size_t rowCount)
{
    if (startRow >= tableRowCount()) {
        // 完全越界：返回无效块（与DATableDataSource约定及pandas实现一致，
        // 避免OFFSET越界查询产出"有效空块"误导消费端）
        return DA::DATableDataBlock();
    }
    ++mFetchCount;
    if (!isConnectionAvailable() || rowCount == 0) {
        return DA::DATableDataBlock();
    }
    QSqlQuery q(database());
    q.prepare(QStringLiteral("SELECT * FROM (%1) LIMIT %2 OFFSET %3")
                  .arg(normalizedSelectSql())
                  .arg(rowCount)
                  .arg(startRow));
    if (!q.exec()) {
        qWarning() << "DASqliteLazyTable::fetchBlock query failed:" << q.lastError().text();
        return DA::DATableDataBlock();
    }
    ensureSchema();
    DA::DATableDataBlock block(startRow, static_cast< std::size_t >(mColumnNames.size()));
    while (q.next()) {
        QVariantList row;
        for (int c = 0; c < mColumnNames.size(); ++c) {
            row.append(q.value(c));
        }
        block.appendRow(row);
    }
    // 不提供rowHeaders，模型回退显示序号
    return block;
}

bool DASqliteLazyTable::isLazyLoaded() const
{
    return true;
}

bool DASqliteLazyTable::isTableEditable() const
{
    return false;
}
