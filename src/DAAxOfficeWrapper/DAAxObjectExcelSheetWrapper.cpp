#include "DAAxObjectExcelSheetWrapper.h"
#include <QDebug>
/**
    QAxObject *range = workSheet->querySubObject("Range(const Qvariant&)", QVariant(QString("A1:F1")));
    if(range == NULL)
        return false;

    // 设置自动适配宽度
    range->setProperty("VerticalAlignment", -4108);     // 水平居中
    range->setProperty("HorizontalAlignment", -4108);   // 垂直居中


    // 把E那一列，设定固定宽度30，并设置自动换行
    QAxObject *E = workSheet->querySubObject("Columns(const QString&)", "E");
    E->setProperty("ColumnWidth", 30);
    E->setProperty("WrapText", true);

    QAxObject *font = workSheet->querySubObject("Range(const QString&)", "A1:P1")->querySubObject("Font");//
获取单元格字体 font->setProperty("Bold",true);// 设置单元格字体加粗 font->setProperty("Size",13);// 设置单元格字体大小


    QAxObject* window = excel->querySubObject("ActiveWindow");
    window->setProperty("SplitRow", 1);
    window->setProperty("FreezePanes", true);



bool ExcelHandle::setAutoFit(int lines)
{
    if(workSheet == NULL)
        return false;
    QAxObject *range = workSheet->querySubObject("Range(const Qvariant&)", QVariant(QString("A1:F%1").arg(lines)));
    if(range == NULL)
        return false;

    QAxObject *cells = range->querySubObject("Columns");
    cells->dynamicCall("AutoFit");
    return true;
}

bool ExcelHandle::writeMulty(QVariant value, QString start, QString end)
{
if(workSheet == NULL)
return false;
QAxObject *range = workSheet->querySubObject("Range(const QString&)",QString("(" +start+ ": " +end+ ")"));
range->setProperty("Value", value);
delete range;
range = NULL;
return true;
}

设置背景色
bool ExcelHandle::setColor(QColor color, QString start, QString end)
{
    QAxObject *range = workSheet->querySubObject("Range(const Qvariant&)", QVariant("=(" +start+ ": " +end+ ")"));
    if(range == NULL)
        return false;
    QAxObject *cells = range->querySubObject("Columns");
    QAxObject *interior = cells->querySubObject("Interior");
    interior->setProperty("Color", color);
    delete interior;
    interior = NULL;
    delete cells;
    cells = NULL;
    return true;
}

//合并单元格
bool ExcelHandle::mergeCells(QString start, QString end, QString value)
{
    if(workSheet == NULL)
        return false;
    QAxObject *range = workSheet->querySubObject("Range(const Qvariant&)", QVariant("=(" +start+ ": " +end+ ")"));
    if(range == NULL)
        return false;

    range->setProperty("MergeCells", true); // 合并单元格
    range->setProperty("Value", value);
    return true;
}

//范围写入

int rows = 2000;
int cols = 3;

QVariantList mList;     // 准备写入的值
  // 写入Range的数据
  for (int i = 0; i < rows; ++i) {
    QVariantList tempVarRow;        // 每一行的QVariantList
    for (int j = 0; j < cols; ++j) {
      tempVarRow << QString("%1,%2").arg (i).arg (j);
      cout << i << j;
    }
    mList << QVariant(tempVarRow);  // 通过QVariant加入mList
  }

  cout << mList;

  Range = WorkSheet->querySubObject ("Range(QString)","A1");
  Range = Range->querySubObject("Resize(int,int)", rows,cols);

  Range->setProperty ("Value",mList);       // 可以设置属性的方法写入
 */

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

DAAxObjectExcelSheetWrapper::DAAxObjectExcelSheetWrapper(QAxObject* sheetObj) : mAxSheet(sheetObj)
{
}

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
        qDebug() << QString("can not get range(%1,%2,%3,%4)").arg(startRow).arg(startColumn).arg(endRow).arg(endColumn);
        return false;
    }
    QVariant varTable = tableToVaraint(table);
    //! Value2能兼容wps和office，
    if (!rangeObj->setProperty("Value2", varTable)) {
        qDebug()
            << QString("range(%1,%2,%3,%4) failed to set value property").arg(startRow).arg(startColumn).arg(endRow).arg(endColumn);
    }
    delete rangeObj;
    return true;
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
    if (var.canConvert(QMetaType::QVariantList)) {
        QVariantList varRows = var.toList();
        if (varRows.isEmpty()) {
            return res;
        }
        const int rowCount = varRows.size();
        for (int r = 0; r < rowCount; ++r) {
            QVariantList rowData;
            rowData = varRows[ r ].toList();
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
QString DAAxObjectExcelSheetWrapper::indexToAlphbat(int n)
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
QVariant DAAxObjectExcelSheetWrapper::tableToVaraint(const DATable< QVariant >& table)
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
    return (indexToAlphbat(startColumn) + QString::number(startRow) + ":" + indexToAlphbat(endColumn)
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
