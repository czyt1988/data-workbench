#ifndef DAMIMEDATAFORDATA_H
#define DAMIMEDATAFORDATA_H
#include "DAGuiAPI.h"
#include <QMimeData>
#include "DAData.h"

namespace DA
{
class DADataManager;

/**
 * @brief DAData的mimeData
 */
class DAGUI_API DAMimeDataForData : public QMimeData
{
    Q_OBJECT
public:
    DAMimeDataForData();
    virtual bool hasFormat(const QString& mimeType) const override;
    virtual QStringList formats() const override;

public:
    void setDAData(const DAData& d);
    DAData getDAData() const;

private:
    DAData m_data;
};


/**
 * @brief DAData下DataFrame的Series名字
 *
 * 这个主要适用于dataframe展开后的series
 */
class DAGUI_API DAMimeDataForDataSeries : public QMimeData
{
    Q_OBJECT
public:
    DAMimeDataForDataSeries();
    virtual bool hasFormat(const QString& mimeType) const override;
    virtual QStringList formats() const override;

public:
    void setDAData(const DAData& d);
    DAData getDAData() const;
    // Series的名字
    void setSeriesName(const QString& name);
    QString getSeriesName() const;

private:
    DAData m_data;
    QString m_seriesName;
};

/**
 * @brief 支持多个DAData下DataFrame的多个Series
 *
 * 这个主要适用于dataframe展开后的series树的多选拖曳
 */
class DAGUI_API DAMimeDataForMultDataSeries : public QMimeData
{
    Q_OBJECT
public:
    DAMimeDataForMultDataSeries();
    virtual bool hasFormat(const QString& mimeType) const override;
    virtual QStringList formats() const override;

public:
    void appendDataSeries(const DAData& d, const QString& colName);
    const QList< QPair< DAData, QStringList > >& getDADatas() const;
    QList< QPair< DAData, QStringList > >& getDADatas();
    bool isEmpty() const;

private:
    QList< QPair< DAData, QStringList > > m_datas;
};
}
#endif  // DAMIMEDATAFORDATA_H
