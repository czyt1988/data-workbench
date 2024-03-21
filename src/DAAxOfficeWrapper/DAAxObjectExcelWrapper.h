#ifndef DAAXOBJECTEXCELWRAPPER_H
#define DAAXOBJECTEXCELWRAPPER_H
#include <QObject>
#include <QAxObject>
#include <QVariant>
#include <optional>
#include "DAAxOfficeWrapperGlobal.h"
#include "DAAxObjectExcelSheetWrapper.h"
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
    // 判断是否有效
    bool isValid() const;
    // 窗体是否显示
    void setWindowVisible(bool on);
    // 是否显示警告
    void setDisplayAlerts(bool on);
    // 打开excel
    bool open(const QString& filename, bool visible = false, bool displayAlerts = false);
    // 创建excel
    bool create(const QString& filename, bool visible = false, bool displayAlerts = false);
    // 保存
    bool save();
    // 保存
    bool saveAs(const QString& filename);
    // 获取所有sheets的名称
    QStringList getSheetsName() const;
    // 获取sheet的数量
    int getSheetsCount() const;
    // 设置当前sheet
    bool setCurrentSheet(int index);
    bool setCurrentSheet(const QString& name);
    // 通过sheet名字查找sheet索引
    int indexOfSheetName(const QString& name) const;
    // 获取当前sheet的名字
    QString getActiveSheetName() const;
    // 读取所有数据(0-base)
    DATable< QVariant > readTable(int sheetIndex) const;
    DATable< QVariant > readCurrentTable() const;
    // 获取当前的sheet(0-base)
    DAAxObjectExcelSheetWrapper getActiveSheet() const;
    DAAxObjectExcelSheetWrapper getSheet(int sheetIndex) const;
    DAAxObjectExcelSheetWrapper getFirstSheet() const;
    DAAxObjectExcelSheetWrapper getLastSheet() const;
    // 关闭
    void close();
    // 添加一个sheet
    DAAxObjectExcelSheetWrapper addSheet(const QString& name);

public:
    // 读取excel第sheetIndex的sheet的数据
    static DATable< QVariant > readExcelSheet(const QString& filename, int sheetIndex = 0, QString* errString = nullptr);
    // 把table写入excel中
    static bool writeExcel(const QString& filename,
                           const QString& sheetName,
                           const DATable< QVariant >& table,
                           bool appendLast    = false,
                           QString* errString = nullptr);
};
}

#endif  // DAAXOBJECTEXCELWRAPPER_H
