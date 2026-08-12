#include "DAAxObjectWordTableWrapper.h"

namespace DA
{

/**
 * @brief 构造函数
 * @param p 表格的QAxObject指针
 */
DAAxObjectWordTableWrapper::DAAxObjectWordTableWrapper(QAxObject* p) : mAxTableObject(p)
{
}

/**
 * @brief 判断是否为空
 * @return
 */
bool DAAxObjectWordTableWrapper::isNull() const
{
    return mAxTableObject == nullptr;
}

/**
 * @brief 获取object
 * @return
 */
QAxObject* DAAxObjectWordTableWrapper::axObject() const
{
    return mAxTableObject;
}

/**
 * @brief 设置表格自动拉伸模式
 * @param v
 */
void DAAxObjectWordTableWrapper::setAutoFitBehavior(DAAxObjectWordTableWrapper::AutoFitBehavior v)
{
    if (isNull()) {
        return;
    }
    mAxTableObject->dynamicCall("AutoFitBehavior(WdAutoFitBehavior)", static_cast< int >(v));
}

/**
 * @brief 获取行数
 * @return
 */
int DAAxObjectWordTableWrapper::rowCount() const
{
    if (isNull()) {
        return 0;
    }
    QAxObject* rows = mAxTableObject->querySubObject("Rows");
    if (nullptr == rows) {
        return 0;
    }
    int cnt = rows->dynamicCall("Count").toInt();
    delete rows;
    return cnt;
}

/**
 * @brief 获取列数
 * @return
 */
int DAAxObjectWordTableWrapper::columnCount() const
{
    if (isNull()) {
        return 0;
    }
    QAxObject* columns = mAxTableObject->querySubObject("Columns");
    if (nullptr == columns) {
        return 0;
    }
    int cnt = columns->dynamicCall("Count").toInt();
    delete columns;
    return cnt;
}

/**
 * @brief 获取单元格,注意，row，col从0开始算
 * @param row 0base
 * @param col 0base
 * @return
 */
QAxObject* DAAxObjectWordTableWrapper::cell(int row, int col)
{
    if (isNull()) {
        return nullptr;
    }
    return mAxTableObject->querySubObject("Cell(int,int)", row + 1, col + 1);
}

/**
 * @brief 选中一个cell的内容,注意，row，col从0开始算
 * @param row 0base
 * @param col 0base
 * @return 如果没有返回nullptr
 */
QAxObject* DAAxObjectWordTableWrapper::selectCellRange(int row, int col)
{
    QAxObject* c = cell(row, col);
    if (nullptr == c) {
        return nullptr;
    }
    QAxObject* range = c->querySubObject("Range");
    delete c;
    return range;
}

/**
 * @brief 设置文本，注意，row，col从0开始算
 * @param row 0base
 * @param col 0base
 * @param text 文本
 */
void DAAxObjectWordTableWrapper::setCellText(int row, int col, const QString& text)
{
    QAxObject* rang = selectCellRange(row, col);
    if (nullptr == rang) {
        return;
    }
    rang->dynamicCall("SetText(QString)", text);
    delete rang;
}
}
