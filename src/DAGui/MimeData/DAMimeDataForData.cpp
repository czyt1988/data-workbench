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
    return QStringList() << DAMIMEDATA_FORMAT_DADATA;
}

void DAMimeDataForData::setDAData(const DAData& d)
{
    _data = d;
}

DAData DAMimeDataForData::getDAData() const
{
    return _data;
}

}
