#include "DAMimeDataForData.h"
#include "DAMimeDataFormats.h"
#include "DADataManager.h"

namespace DA
{


//===============================================================
// DAMimeDataForMultDataSeries
//===============================================================

DAMimeDataForData::DAMimeDataForData() : QMimeData()
{
}

bool DAMimeDataForData::hasFormat(const QString& mimeType) const
{
    if (mimeType == DAMIMEDATA_FORMAT_DADATAS) {
        return true;
    }
    return false;
}

QStringList DAMimeDataForData::formats() const
{
    return { DAMIMEDATA_FORMAT_DADATAS };
}

void DAMimeDataForData::appendDataSeries(const DAData& d, const QString& colName)
{
    for (auto& p : m_dataSeriess) {
        if (p.first == d) {
            if (!p.second.contains(colName)) {
                p.second.append(colName);
                return;
            }
        }
    }
    m_dataSeriess.append(qMakePair(d, QStringList() << colName));
}

const QList< QPair< DAData, QStringList > >& DAMimeDataForData::getDataSeries() const
{
    return m_dataSeriess;
}

QList< QPair< DAData, QStringList > >& DAMimeDataForData::getDataSeries()
{
    return m_dataSeriess;
}

bool DAMimeDataForData::isHaveDataSeries() const
{
    return (!m_dataSeriess.isEmpty());
}

void DAMimeDataForData::appendDataframe(const DAData& d)
{
    m_dataframe.append(d);
}

const QList< DAData >& DAMimeDataForData::getDataframes() const
{
    return m_dataframe;
}

QList< DAData >& DAMimeDataForData::getDataframes()
{
    return m_dataframe;
}

bool DAMimeDataForData::isHaveDataframe() const
{
    return (!m_dataframe.isEmpty());
}

}
