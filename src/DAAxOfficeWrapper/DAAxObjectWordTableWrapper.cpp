#include "DAAxObjectWordTableWrapper.h"

namespace DA
{

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
    mAxTableObject->dynamicCall("AutoFitBehavior(WdAutoFitBehavior)", static_cast< int >(v));
}

/**
 * @brief 获取行数
 * @return
 */
int DAAxObjectWordTableWrapper::rowCount() const
{
    QAxObject* rows = mAxTableObject->querySubObject("Rows");
    if (nullptr == rows) {
        return 0;
    }
    return rows->dynamicCall("Count").toInt();
}

/**
 * @brief 获取列数
 * @return
 */
int DAAxObjectWordTableWrapper::columnCount() const
{
    QAxObject* columns = mAxTableObject->querySubObject("Columns");
    if (nullptr == columns) {
        return 0;
    }
    return columns->dynamicCall("Count").toInt();
}

/**
 * @brief 获取单元格,注意，row，col从0开始算
 * @param row 0base
 * @param col 0base
 * @return
 */
QAxObject* DAAxObjectWordTableWrapper::cell(int row, int col)
{
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
    return c->querySubObject("Range");
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
}
}
