#ifndef DADATATABLEDATASQLITELAZYTABLE_H
#define DADATATABLEDATASQLITELAZYTABLE_H
#include <QString>
#include <QStringList>
#include <QList>
#include <QSqlDatabase>
#include "DAAbstractData.h"
#include "DATableDataSource.h"

/**
 * @brief SQLite惰性表数据源——数据库管理插件的数据层样板
 *
 * 引用式数据：工程持久化只保存连接名+SELECT语句，数据本体留在sqlite文件中；
 * tableRowCount走COUNT(*)子查询，fetchBlock走LIMIT/OFFSET分页查询，
 * 任何时刻都不把全表加载进内存。行数与schema带缓存，invalidateCache()失效。
 *
 * 演示了DATableDataSource三个关键约定的落地方式：
 * - SELECT语句必须含确定性排序（如ORDER BY主键），否则OFFSET分页结果不稳定
 * - isLazyLoaded()=true提示消费端缓存块数据、避免高频小块取数
 * - isTableEditable()=false时模型自动去掉编辑标志（只读浏览）
 */
class DASqliteLazyTable : public DA::DAAbstractData, public DA::DATableDataSource
{
public:
    DASqliteLazyTable();
    // 绑定数据库连接名（连接需已通过QSqlDatabase::addDatabase注册并打开）
    void setConnectionName(const QString& connName);
    QString connectionName() const;
    // 设置SELECT语句（应包含ORDER BY保证分页稳定），末尾分号会被去除
    void setSelectSql(const QString& sql);
    QString selectSql() const;
    // 连接是否可用
    bool isConnectionAvailable() const;
    // fetchBlock调用次数（测试验证取数次数用）
    int fetchCount() const;
    // 失效行数/schema缓存
    void invalidateCache() const;

public:  // DAAbstractData
    DA::DAAbstractData::DataType getDataType() const override;
    QVariant toVariant(std::size_t dim1, std::size_t dim2) const override;
    bool setValue(std::size_t dim1, std::size_t dim2, const QVariant& v) override;
    DA::DATableDataSource* tableSource() override;
    const DA::DATableDataSource* tableSource() const override;
    bool isReferenceData() const override;
    bool supportsUndoSnapshot() const override;
    QString typeIdentifier() const override;
    void write(QDataStream& out) override;
    bool read(QDataStream& in) override;

public:  // DATableDataSource
    std::size_t tableRowCount() const override;
    std::size_t tableColumnCount() const override;
    QString tableColumnName(std::size_t column) const override;
    int tableColumnType(std::size_t column) const override;
    DA::DATableDataBlock fetchBlock(std::size_t startRow, std::size_t rowCount) override;
    bool isLazyLoaded() const override;
    bool isTableEditable() const override;

private:
    QSqlDatabase database() const;
    // 规整后的SELECT（去末尾分号）
    QString normalizedSelectSql() const;
    void ensureSchema() const;

private:
    QString mConnectionName;
    QString mSelectSql;
    mutable QStringList mColumnNames;
    mutable QList< int > mColumnMetaTypes;
    mutable bool mSchemaValid { false };
    mutable std::size_t mRowCountCache { 0 };
    mutable bool mRowCountValid { false };
    mutable int mFetchCount { 0 };
};

#endif  // DADATATABLEDATASQLITELAZYTABLE_H
