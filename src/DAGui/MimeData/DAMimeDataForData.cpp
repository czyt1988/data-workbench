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
    for (auto& p : mDataSeriess) {
        if (p.first == d) {
            if (!p.second.contains(colName)) {
                p.second.append(colName);
                return;
            }
        }
    }
    mDataSeriess.append(qMakePair(d, QStringList() << colName));
}

const QList< QPair< DAData, QStringList > >& DAMimeDataForData::getDataSeries() const
{
    return mDataSeriess;
}

QList< QPair< DAData, QStringList > >& DAMimeDataForData::getDataSeries()
{
    return mDataSeriess;
}

bool DAMimeDataForData::isHaveDataSeries() const
{
    return (!mDataSeriess.isEmpty());
}

void DAMimeDataForData::appendDataframe(const DAData& d)
{
    mDataframe.append(d);
}

const QList< DAData >& DAMimeDataForData::getDataframes() const
{
    return mDataframe;
}

QList< DAData >& DAMimeDataForData::getDataframes()
{
    return mDataframe;
}

bool DAMimeDataForData::isHaveDataframe() const
{
    return (!mDataframe.isEmpty());
}

}
