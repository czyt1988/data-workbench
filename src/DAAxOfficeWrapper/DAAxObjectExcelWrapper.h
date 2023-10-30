#ifndef DAAXOBJECTEXCELWRAPPER_H
#define DAAXOBJECTEXCELWRAPPER_H
#include <QObject>
#include <QAxObject>
#include "DAAxOfficeWrapperGlobal.h"
#include "DATable.h"
namespace DA
{
/**
 * @brief excel操作封装
 */
class DAAXOFFICEWRAPPER_API DAAxObjectExcelWrapper : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAxObjectExcelWrapper)
public:
    DAAxObjectExcelWrapper(QObject* par = nullptr);
    ~DAAxObjectExcelWrapper();
    //窗体是否显示
    void setWindowVisible(bool on);
    //是否显示警告
    void setDisplayAlerts(bool on);
    //打开excel
    bool open(const QString& filename, bool visible = false, bool displayAlerts = false);
    //读取所有
    DATable< QVariant > readAllData() const;
};
}

#endif  // DAAXOBJECTEXCELWRAPPER_H
