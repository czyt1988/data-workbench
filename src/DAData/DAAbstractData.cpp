#include "DAAbstractData.h"
#include <QObject>
#include <QDateTime>
#include "DAUniqueIDGenerater.h"
#include "DADataEnumStringUtils.h"
#include "DATableDataSource.h"
namespace DA
{

class DAAbstractData::PrivateData
{
    DA_DECLARE_PUBLIC(DAAbstractData)
public:
    PrivateData(DAAbstractData* p);
    QString mName;      ///< 名称
    QString mDescribe;  ///< 描述
    Pointer mParent;    ///< 记录父级节点
    IdType mID;         ///< id
};

//===================================================
// DAAbstractData::PrivateData
//===================================================

DAAbstractData::PrivateData::PrivateData(DAAbstractData* p) : q_ptr(p)
{
}

//===================================================
// DAAbstractData
//===================================================

/**
 * @brief 构造函数，自动生成唯一id
 */
DAAbstractData::DAAbstractData() : DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mID = generateID();
}

/**
 * @brief 虚析构函数
 */
DAAbstractData::~DAAbstractData()
{
}

/**
 * @brief 获取名字
 * @return 名称字符串
 */
QString DAAbstractData::getName() const
{
    DA_DC(d);
    return d->mName;
}

/**
 * @brief 设置名字
 * @param n 名称
 */
void DAAbstractData::setName(const QString& n)
{
    DA_D(d);
    d->mName = n;
}

/**
 * @brief 获取描述
 * @return 描述字符串
 */
QString DAAbstractData::getDescribe() const
{
    DA_DC(d);
    return d->mDescribe;
}

/**
 * @brief 设置描述
 * @param d 描述
 */
void DAAbstractData::setDescribe(const QString& d)
{
    d_ptr->mDescribe = d;
}

/**
 * @brief 获取父数据，默认为nullptr，除了一些特殊的数据如DADataPackage
 * @return 父数据的智能指针
 */
DAAbstractData::Pointer DAAbstractData::getParent() const
{
    DA_DC(d);
    return d->mParent;
}

/**
 * @brief 设置父级数据
 * @param p 父级数据的智能指针
 */
void DAAbstractData::setParent(Pointer& p)
{
    DA_D(d);
    d->mParent = p;
}

/**
 * @brief 获取表格数据源接口
 *
 * 表格型数据（dataframe、series、数据库惰性表等）重写此函数返回其
 * DATableDataSource 接口指针，消费端据此走统一的 schema/块级取数路径
 * @return 非表格型数据返回nullptr
 */
DATableDataSource* DAAbstractData::tableSource()
{
    return nullptr;
}

/**
 * @brief 获取表格数据源接口（const版本）
 * @return 非表格型数据返回nullptr
 */
const DATableDataSource* DAAbstractData::tableSource() const
{
    return nullptr;
}

/**
 * @brief 是否为引用式数据
 *
 * 引用式数据（如数据库惰性表）在工程持久化时只保存引用（连接、查询、schema等，
 * 经 @ref write 序列化），不保存数据本体
 * @return 默认返回false（数据本体随工程保存）
 */
bool DAAbstractData::isReferenceData() const
{
    return false;
}

/**
 * @brief 是否支持整表快照式undo
 *
 * 快照式undo（如 DADataObjectPersistUndoCommand 的整对象pickle）要求数据可全量物化，
 * 惰性/引用式数据应返回false，消费端据此禁用依赖整表快照的命令
 * @return 默认返回true
 */
bool DAAbstractData::supportsUndoSnapshot() const
{
    return true;
}

/**
 * @brief 类型唯一标识字符串
 *
 * 默认返回DataType枚举文本（与工程文件既有的type属性兼容）；
 * 枚举无法表达的扩展类型（如插件的数据库表）应重写此函数返回全局唯一字符串，
 * 并在 DADataFactory 注册同名创建函数
 * @return 类型标识
 */
QString DAAbstractData::typeIdentifier() const
{
    return enumToString(getDataType());
}

/**
 * @brief 把数据写入数据流
 * @param out 数据流
 */
void DAAbstractData::write(QDataStream& out)
{
    Q_UNUSED(out);
}

/**
 * @brief 从数据流读取数据
 * @param in 数据流
 * @return 读取成功返回true，否则返回false
 */
bool DAAbstractData::read(QDataStream& in)
{
    Q_UNUSED(in);
    return false;
}

/**
 * @brief 获取id
 * @return 唯一id
 */
DAAbstractData::IdType DAAbstractData::id() const
{
    DA_DC(d);
    return d->mID;
}

/**
 * @brief 设置id
 * @param d id值
 */
void DAAbstractData::setID(DAAbstractData::IdType d)
{
    d_ptr->mID = d;
}

/**
 * @brief 把数据类型转换为文字
 * @param d 数据类型
 * @return 类型对应的文字
 */
QString DAAbstractData::typeToString(DAAbstractData::DataType d)
{
    switch (d) {
    case TypeNone:
        return QObject::tr("none");  // cn:无
    case TypeDataPackage:
        return QObject::tr("package");  // cn:数据包
    case TypePythonObject:
        return QObject::tr("object");  // cn:对象
    case TypePythonDataFrame:
        return QObject::tr("dataframe");  // cn:数据框
    case TypePythonSeries:
        return QObject::tr("series");  // cn:序列
    case TypeInnerData:
        return QObject::tr("raw");  // cn:原始数据
    default:
        break;
    }
    return QString();
}

/**
 * @brief 生成一个唯一id
 * @return 唯一id
 */
DAAbstractData::IdType DAAbstractData::generateID()
{
    return DAUniqueIDGenerater::id_uint64();
}

}  // end of DA
