#include "DAAxObjectExcelSheetWrapper.h"
#include <QDebug>

#ifndef NULL_AXOBJECT_CHECK_AND_RETURN
#define NULL_AXOBJECT_CHECK_AND_RETURN()                                                                               \
    do {                                                                                                               \
        if (isNull()) {                                                                                                \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)
#endif

#ifndef NULL_AXOBJECT_CHECK_WITH_RETURN
#define NULL_AXOBJECT_CHECK_WITH_RETURN(ret)                                                                           \
    do {                                                                                                               \
        if (isNull()) {                                                                                                \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)
#endif

namespace DA
{

/**
 * @brief 构造函数
 * @param sheetObj QAxObject指针
 */
DAAxObjectExcelSheetWrapper::DAAxObjectExcelSheetWrapper(QAxObject* sheetObj) : mAxSheet(sheetObj)
{
}

/**
 * @brief 析构函数，如果设置了自动删除则释放QAxObject
 */
DAAxObjectExcelSheetWrapper::~DAAxObjectExcelSheetWrapper()
{
    if (mAutoDelete) {
        if (!isNull()) {
            delete mAxSheet;
        }
    }
}

/**
 * @brief 判断是否为空
 * @return
 */
bool DAAxObjectExcelSheetWrapper::isNull() const
{
    if (mAxSheet == nullptr) {
        return true;
    }
    return mAxSheet->isNull();
}

/**
 * @brief 写表
 * @param table
 * @param startRow 开始的行，以1为开始
 * @param startColumn 开始的列，以1为开始
 * @return
 */
bool DAAxObjectExcelSheetWrapper::writeTable(const DATable< QVariant >& table, int startRow, int startColumn)
{
    // 获取表的范围
    int rowCnt          = table.rowCount();
    int colCnt          = table.columnCount();
    int endRow          = startRow + rowCnt - 1;
    int endColumn       = startColumn + colCnt - 1;
    QAxObject* rangeObj = range(startRow, startColumn, endRow, endColumn);
    if (qaxobject_is_null(rangeObj)) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug() << QString("can not get range(%1,%2,%3,%4)").arg(startRow).arg(startColumn).arg(endRow).arg(endColumn);
#endif
        return false;
    }
    QVariant varTable = tableToVariant(table);
    //! Value2能兼容wps和office，
    bool ok = rangeObj->setProperty("Value2", varTable);
    if (!ok) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug()
            << QString("range(%1,%2,%3,%4) failed to set value property").arg(startRow).arg(startColumn).arg(endRow).arg(endColumn);
#endif
    }
    delete rangeObj;
    return ok;
}

/**
 * @brief 获取sheet的所有数据,以表格形式返回
 * @return
 */
DATable< QVariant > DAAxObjectExcelSheetWrapper::readTable()
{
    DATable< QVariant > res;
    QVariant var = getAllData();
    if (var.isNull()) {
        return res;
    }
    // 只有一个单元格时，是不会转换为list的
    if (var.canConvert< QVariantList >()) {
        const QVariantList varRows = var.toList();
        if (varRows.isEmpty()) {
            return res;
        }
        const int rowCount = varRows.size();
        for (int r = 0; r < rowCount; ++r) {
            const QVariantList rowData = varRows[ r ].toList();
            for (int c = 0; c < rowData.size(); ++c) {
                res[ { r, c } ] = rowData[ c ];
            }
        }
        return res;
    }

    // 无法转换为list，说明是单一数据或空
    res[ { 0, 0 } ] = var;
    return res;
}
/**
 * @brief 获取所有数据
 * @return
 */
QVariant DAAxObjectExcelSheetWrapper::getAllData()
{
    QVariant var;
    NULL_AXOBJECT_CHECK_WITH_RETURN(var);
    QAxObject* usedRange = mAxSheet->querySubObject("UsedRange");
    if (qaxobject_is_null(usedRange)) {
        return var;
    }
    var = usedRange->dynamicCall("Value");
    delete usedRange;

    return var;
}

/**
 * @brief 获取内部管理的对象
 * @return
 */
QAxObject* DAAxObjectExcelSheetWrapper::object() const
{
    return mAxSheet;
}

/**
 * @brief 把序号转换为A~Z表示的二十六个字母
 * @param n
 * @return
 */
QString DAAxObjectExcelSheetWrapper::indexToAlphabet(int n)
{
    QString result = "";
    while (n > 0) {
        --n;
        char c = 'A' + n % 26;
        result = c + result;
        n /= 26;
    }
    return result;
}

/**
 * @brief table转换为variant
 * @param table
 * @return
 */
QVariant DAAxObjectExcelSheetWrapper::tableToVariant(const DATable< QVariant >& table)
{
    int row = table.rowCount();
    int col = table.columnCount();
    QList< QVariant > varll;
    for (int r = 0; r < row; ++r) {
        QList< QVariant > varRow;
        for (int c = 0; c < col; ++c) {
            varRow.append(table.cell(r, c));
        }
        varll.append(QVariant(varRow));  // 一定要显示转换，不能隐式转换
    }
    return QVariant(varll);  // 一定要显示转换，不能隐式转换
}

/**
 * @brief 生成一个excel范围值，如makeRange(1,1,27,27)=A1:AA27
 * @param startRow 开始的行，第一行为1
 * @param startColumn 开始的列，第一列为1
 * @param endRow 结束的行
 * @param endColumn 结束的列
 * @return
 */
QString DAAxObjectExcelSheetWrapper::makeRangeString(int startRow, int startColumn, int endRow, int endColumn)
{
    return (indexToAlphabet(startColumn) + QString::number(startRow) + ":" + indexToAlphabet(endColumn)
            + QString::number(endRow));
}

/**
 * @brief 自动删除管理
 * @return
 */
bool DAAxObjectExcelSheetWrapper::isAutoDelete() const
{
    return mAutoDelete;
}

/**
 * @brief 自动删除管理
 * @return
 */
void DAAxObjectExcelSheetWrapper::setAutoDelete(bool v)
{
    mAutoDelete = v;
}

/**
 * @brief 范围获取
 * @param startRow
 * @param startColumn
 * @param endRow
 * @param endColumn
 * @return
 */
QAxObject* DAAxObjectExcelSheetWrapper::range(int startRow, int startColumn, int endRow, int endColumn)
{
    NULL_AXOBJECT_CHECK_WITH_RETURN(nullptr);
    QString rangStr = makeRangeString(startRow, startColumn, endRow, endColumn);
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
    qDebug()
        << QString("range(%1,%2,%3,%4) , range string=%5").arg(startRow).arg(startColumn).arg(endRow).arg(endColumn).arg(rangStr);
#endif
    return (mAxSheet->querySubObject("Range(const QString&)", rangStr));
}

/**
 * @brief 激活
 */
void DAAxObjectExcelSheetWrapper::setActive()
{
    NULL_AXOBJECT_CHECK_AND_RETURN();
    mAxSheet->dynamicCall("Activate(void)");
}

/**
 * @brief 获取sheet的名字
 * @return
 */
QString DAAxObjectExcelSheetWrapper::getName()
{
    NULL_AXOBJECT_CHECK_WITH_RETURN(QString());
    return mAxSheet->property("Name").toString();
}
/**
 * @brief 设置sheet的名字
 * @param n
 */
void DAAxObjectExcelSheetWrapper::setName(const QString& n)
{
    NULL_AXOBJECT_CHECK_AND_RETURN();
    mAxSheet->setProperty("Name", n);
}

/**
 * @brief 获取sheet的索引
 * @return
 */
int DAAxObjectExcelSheetWrapper::getIndex()
{
    return mAxSheet->property("Index").toInt();
}

}
