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
    DAData _data;
};
}
#endif  // DAMIMEDATAFORDATA_H
