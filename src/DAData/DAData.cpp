#include "DAData.h"
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
#include "DADataManager.h"
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAData
//===================================================
DAData::DAData() : _dmgr(nullptr)
{
}

DAData::DAData(const DAAbstractData::Pointer& d) : _dmgr(nullptr)
{
    _data = d;
}

DAData::DAData(const DAData& d)
{
    _data = d._data;
    _dmgr = d._dmgr;
}

DAData::DAData(DAData&& d)
{
    _data = std::move(d._data);
    _dmgr = std::move(d._dmgr);
}

DAData::DAData(const DAPyDataFrame& d) : _dmgr(nullptr)
{
    _data = std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPyDataFrame >(d));
}

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
    return _data == d._data;
}

bool DAData::operator<(const DAData& d) const
{
    return rawPointer() < d.rawPointer();
}

DAData& DAData::operator=(const DAData& d)
{
    _data = d._data;
    _dmgr = d._dmgr;
    return *this;
}

DAData& DAData::operator=(const DAPyDataFrame& d)
{
    std::shared_ptr< DAAbstractData > p = std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPyDataFrame >(d));
    _data                               = p;
    return *this;
}

DAData::operator bool() const
{
    return _data != nullptr;
}

/**
 * @brief 是否为空
 * @return
 */
bool DAData::isNull() const
{
    return _data == nullptr;
}

DAAbstractData::DataType DAData::getDataType() const
{
    if (!_data) {
        return DAAbstractData::TypeNone;
    }
    return _data->getDataType();
}

QVariant DAData::value() const
{
    if (!_data) {
        return QVariant();
    }
    return _data->toVariant();
}
/**
 * @brief 设置值
 *
 * 通过此函数设置的值，如果这个变量被变量管理器管理，变量管理器会发射一个DataChanged信号
 * @param v
 * @return
 */
bool DAData::setValue(const QVariant& v) const
{
    if (!_data) {
        return false;
    }
    bool r = _data->setValue(v);
    if (r) {
        if (_dmgr) {
            _dmgr->callDataChangedSignal(*this, DADataManager::ChangeValue);
        }
    }
    return r;
}

QString DAData::getName() const
{
    if (!_data) {
        return QString();
    }
    return _data->getName();
}

void DAData::setName(const QString& n)
{
    if (!_data) {
        return;
    }
    _data->setName(n);
    if (_dmgr) {
        _dmgr->callDataChangedSignal(*this, DADataManager::ChangeName);
    }
}

QString DAData::getDescribe() const
{
    if (!_data) {
        return QString();
    }
    return _data->getDescribe();
}

void DAData::setDescribe(const QString& d)
{
    if (!_data) {
        return;
    }
    _data->setDescribe(d);
    if (_dmgr) {
        _dmgr->callDataChangedSignal(*this, DADataManager::ChangeDescribe);
    }
}

DAAbstractData* DAData::rawPointer()
{
    return _data.get();
}

const DAAbstractData* DAData::rawPointer() const
{
    return _data.get();
}

DAData::Pointer DAData::getPointer()
{
    return _data;
}

const DAData::Pointer DAData::getPointer() const
{
    return _data;
}

DAData::IdType DAData::id() const
{
    return _data->id();
}

bool DAData::isDataFrame() const
{
    if (!_data) {
        return false;
    }
    return (_data->getDataType() == DAAbstractData::TypePythonDataFrame);
}

/**
 * @brief 是否为datapackage
 * @return
 */
bool DAData::isDataPackage() const
{
    if (!_data) {
        return false;
    }
    return (_data->getDataType() == DAAbstractData::TypeDataPackage);
}

/**
 * @brief 转换为DAPyDataFrame
 * @return 如果内部维护的不是DAPyDataFrame，返回一个默认构造的DAPyDataFrame
 */
DAPyDataFrame DAData::toDataFrame() const
{
    if (isDataFrame()) {
        DADataPyDataFrame* df = static_cast< DADataPyDataFrame* >(_data.get());
        return df->dataframe();
    }
    return DAPyDataFrame();
}

/**
 * @brief 转换为datapackage,如果不是DADataPackage，返回nullptr
 * @return
 */
DADataPackage::Pointer DAData::toDataPackage() const
{
    if (isDataPackage()) {
        DADataPackage::Pointer d = std::static_pointer_cast< DADataPackage >(_data);
        return d;
    }
    return nullptr;
}

QString DAData::typeToString() const
{
    if (!_data) {
        return DAAbstractData::typeToString(DAAbstractData::TypeNone);
    }
    return DAAbstractData::typeToString(_data->getDataType());
}

/**
 * @brief 获取数据对应的datamanager
 * @return
 */
DADataManager* DAData::getDataManager() const
{
    return _dmgr;
}

/**
 * @brief 设置变量管理器，在data添加如变量管理器后，data内部就会记录变量管理器的指针
 * @note 此函数是DADataManager调用
 * @param mgr
 */
void DAData::setDataManager(DADataManager* mgr)
{
    _dmgr = mgr;
}

namespace DA
{
uint qHash(const DAData& key, uint seed)
{
    return ::qHash(key.rawPointer(), seed);
}
}  // namespace DA
