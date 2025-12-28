#include "DAMimeDataForData.h"
#include "DAMimeDataFormats.h"
#include "DADataManager.h"

namespace DA
{

DAMimeDataForData::DAMimeDataForData() : QMimeData()
{
}

bool DAMimeDataForData::hasFormat(const QString& mimeType) const
{
    if (mimeType == DAMIMEDATA_FORMAT_DADATA) {
        return true;
    }
    return false;
}

QStringList DAMimeDataForData::formats() const
{
    return { DAMIMEDATA_FORMAT_DADATA };
}

void DAMimeDataForData::setDAData(const DAData& d)
{
    m_data = d;
}

DAData DAMimeDataForData::getDAData() const
{
    return m_data;
}


//===============================================================
// DAMimeDataForDataSeries
//===============================================================

DAMimeDataForDataSeries::DAMimeDataForDataSeries() : QMimeData()
{
}

bool DAMimeDataForDataSeries::hasFormat(const QString& mimeType) const
{
    if (mimeType == DAMIMEDATA_FORMAT_DADATA_DATAFRAME_SERIES) {
        return true;
    }
    return false;
}

QStringList DAMimeDataForDataSeries::formats() const
{
    return { DAMIMEDATA_FORMAT_DADATA_DATAFRAME_SERIES };
}

void DAMimeDataForDataSeries::setDAData(const DAData& d)
{
    m_data = d;
}

DAData DAMimeDataForDataSeries::getDAData() const
{
    return m_data;
}

void DAMimeDataForDataSeries::setSeriesName(const QString& name)
{
    m_seriesName = name;
}

QString DAMimeDataForDataSeries::getSeriesName() const
{
    return m_seriesName;
}

//===============================================================
// DAMimeDataForMultDataSeries
//===============================================================

DAMimeDataForMultDataSeries::DAMimeDataForMultDataSeries() : QMimeData()
{
}

bool DAMimeDataForMultDataSeries::hasFormat(const QString& mimeType) const
{
    if (mimeType == DAMIMEDATA_FORMAT_MULT_DADATAS_SERIES) {
        return true;
    }
    return false;
}

QStringList DAMimeDataForMultDataSeries::formats() const
{
    return { DAMIMEDATA_FORMAT_MULT_DADATAS_SERIES };
}

void DAMimeDataForMultDataSeries::appendDataSeries(const DAData& d, const QString& colName)
{
    for (auto& p : m_datas) {
        if (p.first == d) {
            if (!p.second.contains(colName)) {
                p.second.append(colName);
                return;
            }
        }
    }
    m_datas.append(qMakePair(d, QStringList() << colName));
}

const QList< QPair< DAData, QStringList > >& DAMimeDataForMultDataSeries::getDADatas() const
{
    return m_datas;
}

QList< QPair< DAData, QStringList > >& DAMimeDataForMultDataSeries::getDADatas()
{
    return m_datas;
}

bool DAMimeDataForMultDataSeries::isEmpty() const
{
    return m_datas.isEmpty();
}

}
