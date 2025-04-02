#ifndef DAZIPARCHIVETHREADWRAPPER_H
#define DAZIPARCHIVETHREADWRAPPER_H
#include "DAGuiAPI.h"
#include <QObject>
#include <QString>
namespace DA
{
class DAAbstractArchiveTask;
/**
 * @brief DAZipArchive的多线程封装，此类内部维护着一个线程，封装了@sa DAZipArchive 的操作
 */
class DAGUI_API DAZipArchiveThreadWrapper : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAZipArchiveThreadWrapper)
public:
    DAZipArchiveThreadWrapper(QObject* par = nullptr);
    ~DAZipArchiveThreadWrapper();

public Q_SLOTS:
    void save(const QString& filePath);
    void load(const QString& filePath);
Q_SIGNALS:
    void beginSave(const QString& path);
    void beginLoad(const QString& path);
    void saved(bool success);
    void loaded(bool success);
};
}  // end DA
#endif  // DAZIPARCHIVETHREADWRAPPER_H
