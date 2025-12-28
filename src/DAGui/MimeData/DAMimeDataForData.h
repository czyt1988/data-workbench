#ifndef DAMIMEDATAFORDATA_H
#define DAMIMEDATAFORDATA_H
#include "DAGuiAPI.h"
#include <QMimeData>
#include "DAData.h"

namespace DA
{
class DADataManager;


/**
 * @brief 支持多个DAData下DataFrame的多个Series
 *
 * 这个主要适用于dataframe展开后的series树的多选拖曳
 */
class DAGUI_API DAMimeDataForData : public QMimeData
{
    Q_OBJECT
public:
    DAMimeDataForData();
    virtual bool hasFormat(const QString& mimeType) const override;
    virtual QStringList formats() const override;

public:
    // 添加series
    void appendDataSeries(const DAData& d, const QString& colName);
    const QList< QPair< DAData, QStringList > >& getDataSeries() const;
    QList< QPair< DAData, QStringList > >& getDataSeries();
    bool isHaveDataSeries() const;

    // 添加dataframe
    void appendDataframe(const DAData& d);
    const QList< DAData >& getDataframes() const;
    QList< DAData >& getDataframes();
    bool isHaveDataframe() const;

private:
    QList< QPair< DAData, QStringList > > m_dataSeriess;
    QList< DAData > m_dataframe;
};
}
#endif  // DAMIMEDATAFORDATA_H
