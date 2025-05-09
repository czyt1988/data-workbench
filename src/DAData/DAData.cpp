#include "DAData.h"
#include "DADataManager.h"
#if DA_ENABLE_PYTHON
#include "DAPyScripts.h"
#include "DADataPyObject.h"
#include "DADataPyDataFrame.h"
#include "DADataPySeries.h"
#include "DAPyScriptsDataFrame.h"
#endif
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

namespace DA
{

//===================================================
// DAData
//===================================================
DAData::DAData() : mDataMgr(nullptr)
{
}

DAData::DAData(const DAAbstractData::Pointer& d) : mDataMgr(nullptr)
{
	mData = d;
}

DAData::DAData(const DAData& d)
{
	mData    = d.mData;
	mDataMgr = d.mDataMgr;
}

DAData::DAData(DAData&& d)
{
	mData    = std::move(d.mData);
	mDataMgr = std::move(d.mDataMgr);
}

#if DA_ENABLE_PYTHON
DAData::DAData(const DAPyDataFrame& d) : mDataMgr(nullptr)
{
	mData = std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPyDataFrame >(d));
}

DAData::DAData(const DAPySeries& d) : mDataMgr(nullptr)
{
	mData = std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPySeries >(d));
}
#endif

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

bool DAData::operator<(const DAData& d) const
{
    return rawPointer() < d.rawPointer();
}

DAData& DAData::operator=(const DAData& d)
{
	mData    = d.mData;
	mDataMgr = d.mDataMgr;
	return *this;
}

#if DA_ENABLE_PYTHON
DAData& DAData::operator=(const DAPyDataFrame& d)
{
	std::shared_ptr< DAAbstractData > p =
		std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPyDataFrame >(d));
	mData = p;
	return *this;
}

DAData& DAData::operator=(const DAPySeries& d)
{
	std::shared_ptr< DAAbstractData > p = std::static_pointer_cast< DAAbstractData >(std::make_shared< DADataPySeries >(d));
	mData = p;
	return *this;
}
#endif

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

DAAbstractData::DataType DAData::getDataType() const
{
	if (!mData) {
		return DAAbstractData::TypeNone;
	}
	return mData->getDataType();
}

QVariant DAData::value() const
{
	if (!mData) {
		return QVariant();
	}
	return mData->toVariant();
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
	if (!mData) {
		return false;
	}
	bool r = mData->setValue(v);
	if (r) {
		if (mDataMgr) {
			mDataMgr->callDataChangedSignal(*this, DADataManager::ChangeValue);
		}
	}
	return r;
}

QString DAData::getName() const
{
	if (!mData) {
		return QString();
	}
	return mData->getName();
}

void DAData::setName(const QString& n)
{
	if (!mData) {
		return;
	}
	mData->setName(n);
	if (mDataMgr) {
		mDataMgr->callDataChangedSignal(*this, DADataManager::ChangeName);
	}
}

QString DAData::getDescribe() const
{
	if (!mData) {
		return QString();
	}
	return mData->getDescribe();
}

void DAData::setDescribe(const QString& d)
{
	if (!mData) {
		return;
	}
	mData->setDescribe(d);
	if (mDataMgr) {
		mDataMgr->callDataChangedSignal(*this, DADataManager::ChangeDescribe);
	}
}

DAAbstractData* DAData::rawPointer()
{
	return mData.get();
}

const DAAbstractData* DAData::rawPointer() const
{
	return mData.get();
}

DAData::Pointer DAData::getPointer()
{
	return mData;
}

const DAData::Pointer DAData::getPointer() const
{
	return mData;
}

DAData::IdType DAData::id() const
{
	return mData->id();
}

bool DAData::isDataFrame() const
{
	if (!mData) {
		return false;
	}
	return (mData->getDataType() == DAAbstractData::TypePythonDataFrame);
}

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
#endif

/**
 * @brief 转换为datapackage,如果不是DADataPackage，返回nullptr
 * @return
 */
DADataPackage::Pointer DAData::toDataPackage() const
{
	if (isDataPackage()) {
		DADataPackage::Pointer d = std::static_pointer_cast< DADataPackage >(mData);
		return d;
	}
	return nullptr;
}

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
#if DA_ENABLE_PYTHON
	case DAAbstractData::TypePythonDataFrame: {
		DAPyScriptsDataFrame& pydf = DAPyScripts::getInstance().getDataFrame();
		if (!pydf.to_pickle(data.toDataFrame(), filePath)) {
			return false;
		}
	} break;
#endif
	default:
		break;
	}
	return true;
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

uint qHash(const DAData& key, uint seed)
{
    return ::qHash(key.rawPointer(), seed);
}

DA_AUTO_REGISTER_META_TYPE(DA::DAData)
}  // namespace DA
