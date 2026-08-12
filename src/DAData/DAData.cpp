// qt
#include <QFileDialog>
#include <QFileInfo>

#include "DAData.h"
#include "DADataManager.h"
#if DA_ENABLE_PYTHON
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
#include "DADataPySeries.h"
#endif
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAData
//===================================================

/**
 * @brief 默认构造函数
 */
DAData::DAData() : mDataMgr(nullptr)
{
}

/**
 * @brief 通过智能指针构造
 * @param d 抽象数据的智能指针
 */
DAData::DAData(const DAAbstractData::Pointer& d) : mDataMgr(nullptr)
{
    mData = d;
}

/**
 * @brief 拷贝构造函数
 * @param d 另一个DAData
 */
DAData::DAData(const DAData& d)
{
    mData    = d.mData;
    mDataMgr = d.mDataMgr;
}

/**
 * @brief 移动构造函数
 * @param d 另一个DAData
 */
DAData::DAData(DAData&& d) noexcept : mData(std::move(d.mData)), mDataMgr(d.mDataMgr)
{
    d.mDataMgr = nullptr;
}

#if DA_ENABLE_PYTHON
/**
 * @brief 通过DAPyDataFrame构造
 * @param d dataframe
 */
DAData::DAData(const DAPyDataFrame& d) : mDataMgr(nullptr)
{
    mData = std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPyDataFrame >(d));
}

/**
 * @brief 通过DAPySeries构造
 * @param d series
 */
DAData::DAData(const DAPySeries& d) : mDataMgr(nullptr)
{
    mData = std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPySeries >(d));
}

/**
 * @brief 直接从 Python 对象构造，不允许隐式转换
 * @param obj python对象
 */
DAData::DAData(pybind11::object obj) : mDataMgr(nullptr)
{
    if (DAPyDataFrame::isDataFrame(obj)) {
        mData = std::make_shared< DADataPyDataFrame >(DAPyDataFrame(obj));
    } else if (DAPySeries::isSeries(obj)) {
        mData = std::make_shared< DADataPySeries >(DAPySeries(obj));
    } else {
        throw std::invalid_argument("Unsupported Python object");
    }
}
#endif

/**
 * @brief 析构函数
 */
DAData::~DAData()
{
}
/**
 * @brief 注意这里的等于不是指相等而是变量相等，类似is
 * @param d
 * @return
 */
bool DAData::operator==(const DAData& d) const
{
    return mData == d.mData;
}

/**
 * @brief 注意这里的等于不是指相等而是变量相等，类似is
 * @param d
 * @return
 */
bool DAData::operator!=(const DAData& d) const
{
    return mData != d.mData;
}

/**
 * @brief 小于比较运算符，基于原始指针比较
 * @param d 另一个DAData
 * @return true表示this小于d
 */
bool DAData::operator<(const DAData& d) const
{
    return rawPointer() < d.rawPointer();
}

/**
 * @brief 赋值运算符
 * @param d 另一个DAData
 * @return 自身引用
 */
DAData& DAData::operator=(const DAData& d)
{
    mData    = d.mData;
    mDataMgr = d.mDataMgr;
    return *this;
}

/**
 * @brief 移动赋值运算符
 * @param d 另一个DAData
 * @return 自身引用
 */
DAData& DAData::operator=(DAData&& d) noexcept
{
    if (this != &d) {
        mData      = std::move(d.mData);
        mDataMgr   = d.mDataMgr;
        d.mDataMgr = nullptr;
    }
    return *this;
}

#if DA_ENABLE_PYTHON
/**
 * @brief 通过DAPyDataFrame赋值
 * @param d dataframe
 * @return 自身引用
 */
DAData& DAData::operator=(const DAPyDataFrame& d)
{
    std::shared_ptr< DAAbstractData > p =
        std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPyDataFrame >(d));
    mData = p;
    return *this;
}

/**
 * @brief 通过DAPySeries赋值
 * @param d series
 * @return 自身引用
 */
DAData& DAData::operator=(const DAPySeries& d)
{
    std::shared_ptr< DAAbstractData > p =
        std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPySeries >(d));
    mData = p;
    return *this;
}
#endif

/**
 * @brief bool转换运算符，判断内部数据是否非空
 * @return 内部数据非空返回true
 */
DAData::operator bool() const
{
    return mData != nullptr;
}

/**
 * @brief 是否为空
 * @return
 */
bool DAData::isNull() const
{
    return mData == nullptr;
}

/**
 * @brief 获取数据类型
 * @return 数据类型
 */
DAAbstractData::DataType DAData::getDataType() const
{
    if (!mData) {
        return DAAbstractData::TypeNone;
    }
    return mData->getDataType();
}

/**
 * @brief 获取变量值
 * @param dim1 第一维索引
 * @param dim2 第二维索引
 * @return 对应位置的值
 */
QVariant DAData::value(size_t dim1, std::size_t dim2) const
{
    if (!mData) {
        return QVariant();
    }
    return mData->toVariant(dim1, dim2);
}
/**
 * @brief 设置值
 *
 * 通过此函数设置的值，如果这个变量被变量管理器管理，变量管理器会发射一个DataChanged信号
 * @param v
 * @return
 */
bool DAData::setValue(std::size_t dim1, size_t dim2, const QVariant& v)
{
    if (!mData) {
        return false;
    }
    bool r = mData->setValue(dim1, dim2, v);
    if (r) {
        if (mDataMgr) {
            mDataMgr->notifyDataChangedSignal(*this, DADataManager::ChangeValue);
        }
    }
    return r;
}

/**
 * @brief 获取变量名
 * @return 变量名
 */
QString DAData::getName() const
{
    if (!mData) {
        return QString();
    }
    return mData->getName();
}

/**
 * @brief 设置变量名
 * @param n 变量名
 */
void DAData::setName(const QString& n)
{
    if (!mData) {
        return;
    }
    mData->setName(n);
    if (mDataMgr) {
        mDataMgr->notifyDataChangedSignal(*this, DADataManager::ChangeName);
    }
}

/**
 * @brief 获取变量描述
 * @return 变量描述
 */
QString DAData::getDescribe() const
{
    if (!mData) {
        return QString();
    }
    return mData->getDescribe();
}

/**
 * @brief 设置变量描述
 * @param d 变量描述
 */
void DAData::setDescribe(const QString& d)
{
    if (!mData) {
        return;
    }
    mData->setDescribe(d);
    if (mDataMgr) {
        mDataMgr->notifyDataChangedSignal(*this, DADataManager::ChangeDescribe);
    }
}

/**
 * @brief 返回原始指针
 * @return 抽象数据的裸指针
 */
DAAbstractData* DAData::rawPointer()
{
    return mData.get();
}

/**
 * @brief 返回原始指针（const版本）
 * @return 抽象数据的const裸指针
 */
const DAAbstractData* DAData::rawPointer() const
{
    return mData.get();
}

/**
 * @brief 返回智能指针
 * @return 抽象数据的智能指针
 */
DAData::Pointer DAData::getPointer()
{
    return mData;
}

/**
 * @brief 返回智能指针（const版本）
 * @return 抽象数据的智能指针
 */
DAData::Pointer DAData::getPointer() const
{
    return mData;
}

/**
 * @brief 获取id
 * @return 唯一id，如果内部数据为空返回0
 */
DAData::IdType DAData::id() const
{
    if (!mData) {
        return 0;
    }
    return mData->id();
}

/**
 * @brief 判断是否为dataframe
 * @return 是dataframe返回true
 */
bool DAData::isDataFrame() const
{
    if (!mData) {
        return false;
    }
    return (mData->getDataType() == DAAbstractData::TypePythonDataFrame);
}

/**
 * @brief 判断是否为series
 * @return 是series返回true
 */
bool DAData::isSeries() const
{
    if (!mData) {
        return false;
    }
    return (mData->getDataType() == DAAbstractData::TypePythonSeries);
}

/**
 * @brief 是否为datapackage
 * @return
 */
bool DAData::isDataPackage() const
{
    if (!mData) {
        return false;
    }
    return (mData->getDataType() == DAAbstractData::TypeDataPackage);
}

#if DA_ENABLE_PYTHON
/**
 * @brief 转换为DAPyDataFrame
 * @return 如果内部维护的不是DAPyDataFrame，返回一个默认构造的DAPyDataFrame(isNone=true)
 */
DAPyDataFrame DAData::toDataFrame() const
{
    if (isDataFrame()) {
        DADataPyDataFrame* df = static_cast< DADataPyDataFrame* >(mData.get());
        return df->dataframe();
    }
    return DAPyDataFrame();
}

/**
 * @brief 转换为DAPySeries
 * @return 如果内部维护的不是DAPySeries，返回一个默认构造的DAPySeries(isNone=true)
 */
DAPySeries DAData::toSeries() const
{
    if (isSeries()) {
        DADataPySeries* ser = static_cast< DADataPySeries* >(mData.get());
        return ser->series();
    }
    return DAPySeries();
}

/**
 * @brief 转换为python对象，如果无法转换，将返回none
 * @return
 */
pybind11::object DAData::toPyObject() const
{
    switch (getDataType()) {
    case DAAbstractData::TypeNone:
        return pybind11::none();
    case DAAbstractData::TypePythonDataFrame: {
        DADataPyDataFrame* df = static_cast< DADataPyDataFrame* >(mData.get());
        return df->object().object();
    }
    case DAAbstractData::TypePythonSeries: {
        DADataPySeries* ser = static_cast< DADataPySeries* >(mData.get());
        return ser->object().object();
    }
    case DAAbstractData::TypePythonObject: {
        DADataPyObject* obj = static_cast< DADataPyObject* >(mData.get());
        return obj->object().object();
    }
    default:
        break;
    }
    return pybind11::none();
}

/**
 * @brief 设置python对象，此函数会替换掉数据管理的对象内容
 * @param obj python对象
 */
void DA::DAData::setPyObject(const pybind11::object& obj)
{
    switch (getDataType()) {
    case DAAbstractData::TypePythonDataFrame: {
        DADataPyDataFrame* df = static_cast< DADataPyDataFrame* >(mData.get());
        df->object()          = obj;

    } break;
    case DAAbstractData::TypePythonSeries: {
        DADataPySeries* ser = static_cast< DADataPySeries* >(mData.get());
        ser->object()       = obj;
    } break;
    case DAAbstractData::TypePythonObject: {
        DADataPyObject* pyobj = static_cast< DADataPyObject* >(mData.get());
        pyobj->object()       = obj;
    } break;
    default:
        break;
    }
}


#endif

/**
 * @brief 把数据写到文件
 * @param data
 * @param filePath
 * @return
 */
bool DAData::writeToFile(const DAData& data, const QString& filePath)
{
    if (data.isNull()) {
        return false;
    }
    switch (data.getDataType()) {
    case DAAbstractData::TypePythonDataFrame: {
        return data.toDataFrame().to_parquet(filePath);
    } break;
    default:
        break;
    }
    return false;
}

/**
 * @brief 数据类型转换为文字
 * @return 类型对应的文字
 */
QString DAData::typeToString() const
{
    if (!mData) {
        return DAAbstractData::typeToString(DAAbstractData::TypeNone);
    }
    return DAAbstractData::typeToString(mData->getDataType());
}

/**
 * @brief 获取数据对应的datamanager
 * @return
 */
DADataManager* DAData::getDataManager() const
{
    return mDataMgr;
}

/**
 * @brief 是否存在数据管理器
 * @return
 */
bool DAData::isHaveDataManager() const
{
    return (mDataMgr != nullptr);
}

/**
 * @brief 获取数据的尺寸
 * @return 返回行列数组成的pair
 */
std::pair< size_t, size_t > DAData::shape() const
{
    switch (getDataType()) {
    case DAAbstractData::TypePythonDataFrame: {
        DADataPyDataFrame* df = static_cast< DADataPyDataFrame* >(mData.get());
        return df->dataframe().shape();
    } break;
    case DAAbstractData::TypePythonSeries: {
        DADataPySeries* ser = static_cast< DADataPySeries* >(mData.get());
        return std::make_pair(ser->series().size(), 1);
    } break;
    default:
        break;
    }
    return std::make_pair(0, 0);
}


/**
 * @brief 设置变量管理器，在data添加如变量管理器后，data内部就会记录变量管理器的指针
 * @note 此函数是DADataManager调用
 * @param mgr
 */
void DAData::setDataManager(DADataManager* mgr)
{
    mDataMgr = mgr;
}

/**
 * @brief qHash函数，用于QHash/QMap的键计算
 * @param key DAData键
 * @param seed 种子
 * @return hash值
 */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
uint qHash(const DAData& key, uint seed)
#else
std::size_t qHash(const DAData& key, std::size_t seed)
#endif
{
    return ::qHash(key.rawPointer(), seed);
}

}  // namespace DA
DA_AUTO_REGISTER_META_TYPE(DA::DAData)
