#ifndef DAAXOBJECTEXCELSHEETWRAPPER_H
#define DAAXOBJECTEXCELSHEETWRAPPER_H
#include "DAAxOfficeWrapperGlobal.h"
#include <QAxObject>
#include <QVariant>
#include "DATable.h"
namespace DA
{

/**
 * @brief 针对excelSheet的操作封装，不对QAxObject的生命周期进行管理，确保此类不要单独保留，否则sheet析构后会引起异常
 */
class DAAXOFFICEWRAPPER_API DAAxObjectExcelSheetWrapper
{
public:
    DAAxObjectExcelSheetWrapper(QAxObject* sheetObj = nullptr);
    ~DAAxObjectExcelSheetWrapper();
    // 判断是否有效
    bool isNull() const;
    // 快速写，默认从A1开始写
    bool writeTable(const DATable< QVariant >& table, int startRow = 1, int startColumn = 1);
    // 批量读写
    DATable< QVariant > readTable();
    // 获取sheet的所有数据
    QVariant getAllData();
    // 获取内部管理的对象
    QAxObject* object() const;
    // 自动删除管理
    bool isAutoDelete() const;
    void setAutoDelete(bool v);
    // 获取范围
    QAxObject* range(int startRow, int startColumn, int endRow, int endColumn);
    // 激活
    void setActive();
    // 获取sheet的名字
    QString getName();
    void setName(const QString& n);
    // 获取sheet的索引
    int getIndex();

public:
    // 把序号转换为A~Z表示的二十六个字母
    static QString indexToAlphbat(int n);
    // table转换为QVariant
    static QVariant tableToVaraint(const DATable< QVariant >& table);

private:
    // 生成一个范围值，如makeRange(1,1,27,27)=A1:AA27
    QString makeRangeString(int startRow, int startColumn, int endRow, int endColumn);

private:
    QAxObject* mAxSheet { nullptr };
    bool mAutoDelete { false };  ///< 是否自动删除，自动删除将再析构执行delete,默认为false
};

}
#endif  // DAAXOBJECTEXCELSHEETWRAPPER_H
