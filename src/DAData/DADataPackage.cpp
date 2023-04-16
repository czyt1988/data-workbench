#include "DADataPackage.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DADataPackage
//===================================================
DADataPackage::DADataPackage()
{
}

DADataPackage::~DADataPackage()
{
}

DAAbstractData::DataType DADataPackage::getDataType() const
{
    return DAAbstractData::TypeDataPackage;
}

/**
 * @brief 转换为value
 * @return
 */
QVariant DADataPackage::toVariant() const
{
    QVariantList var;
    for (const DAAbstractData::Pointer& p : qAsConst(mChildren)) {
        var.append(p->toVariant());
    }
    return var;
}

/**
 * @brief v作为package的名称进行设置
 * @param v
 * @return
 */
bool DADataPackage::setValue(const QVariant& v)
{
    QVariantList var = v.toList();
    if (var.isEmpty()) {
        return true;
    }
    for (const QVariant& v : qAsConst(var)) {
        //
        // TODO
    }
    return true;
}

/**
 * @brief 获取数据个数
 * @return
 */
int DADataPackage::getChildCount() const
{
    return mChildren.size();
}

/**
 * @brief 插入一个数据
 * @param p
 */
void DADataPackage::append(const DAAbstractData::Pointer& p)
{
    mChildren.append(p);
}

/**
 * @brief 索引数据
 * @param index
 * @return
 */
DAAbstractData::Pointer& DADataPackage::at(int index)
{
    return mChildren[ index ];
}

/**
 * @brief 索引数据
 * @param index
 * @return
 */
const DAAbstractData::Pointer& DADataPackage::at(int index) const
{
    return mChildren[ index ];
}

/**
 * @brief 索引数据
 * @param index
 * @return
 */
DAAbstractData::Pointer& DADataPackage::operator[](int index)
{
    return mChildren[ index ];
}

/**
 * @brief 索引数据
 * @param index
 * @return
 */
const DAAbstractData::Pointer& DADataPackage::operator[](int index) const
{
    return mChildren[ index ];
}

/**
 * @brief DADataPackage::isEmpty
 * @return
 */
bool DADataPackage::isEmpty() const
{
    return mChildren.isEmpty();
}
