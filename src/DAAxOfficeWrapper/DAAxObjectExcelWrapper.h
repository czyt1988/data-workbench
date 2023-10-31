#ifndef DAAXOBJECTEXCELWRAPPER_H
#define DAAXOBJECTEXCELWRAPPER_H
#include <QObject>
#include <QAxObject>
#include <QVariant>
#include <optional>
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
    //判断是否有效
    bool isValid() const;
    //窗体是否显示
    void setWindowVisible(bool on);
    //是否显示警告
    void setDisplayAlerts(bool on);
    //打开excel
    bool open(const QString& filename, bool visible = false, bool displayAlerts = false);
    //创建excel
    bool create(const QString& filename, bool visible = false, bool displayAlerts = false);
    //保存
    bool save();
    //保存
    bool saveAs(const QString& filename);
    //获取所有sheets的名称
    QStringList getSheetsName() const;
    //读取所有数据
    DATable< QVariant > readAllData(int sheetIndex = 0) const;

public:
    //读取excel第sheetIndex的sheet的数据
    static DATable< QVariant > readAllData(const QString& filename, int sheetIndex = 0, QString* errString = nullptr);
};
}

#endif  // DAAXOBJECTEXCELWRAPPER_H
