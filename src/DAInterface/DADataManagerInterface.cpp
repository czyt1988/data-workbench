#include "DADataManagerInterface.h"
namespace DA
{
class DADataManagerInterface::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManagerInterface)
public:
    PrivateData(DADataManagerInterface* p);
    DADataManager* mDataMgr;
};
//==============================================================
// DADataManagerInterfacePrivate
//==============================================================
DADataManagerInterface::PrivateData::PrivateData(DADataManagerInterface* p) : q_ptr(p)
{
    mDataMgr = new DADataManager(p);
}

//==============================================================
// DADataManagerInterface
//==============================================================
DADataManagerInterface::DADataManagerInterface(DACoreInterface* c, QObject* par)
    : DABaseInterface(c, par), DA_PIMPL_CONSTRUCT
{
    connect(d_ptr->mDataMgr, &DADataManager::dataAdded, this, &DADataManagerInterface::dataAdded);
    connect(d_ptr->mDataMgr, &DADataManager::dataBeginRemove, this, &DADataManagerInterface::dataBeginRemove);
    connect(d_ptr->mDataMgr, &DADataManager::dataRemoved, this, &DADataManagerInterface::dataRemoved);
    connect(d_ptr->mDataMgr, &DADataManager::dataChanged, this, &DADataManagerInterface::dataChanged);
}

DADataManagerInterface::~DADataManagerInterface()
{
}
/**
 * @brief 获取数据管理的指针
 * @return
 */
DADataManager* DADataManagerInterface::dataManager() const
{
    return d_ptr->mDataMgr;
}
/**
 * @brief 添加数据
 *
 * @note 此函数会发生信号@sa dataAdded
 * @param d
 */
void DADataManagerInterface::addData(DAData& d)
{
    dataManager()->addData(d);
}

/**
 * @brief 带redo/undo的添加数据
 *
 * @note 此函数会发生信号@sa dataBeginAdd 和 @sa dataEndAdded
 * @param d
 */
void DADataManagerInterface::addData_(DAData& d)
{
    dataManager()->addData_(d);
}

/**
 * @brief 移除数据
 *
 * @note 此函数会发生信号@sa dataRemoved
 * @param d
 */
void DADataManagerInterface::removeData(DAData& d)
{
    dataManager()->removeData(d);
}

/**
 * @brief 带redo/undo的移除数据
 * @note 此函数会发生信号@sa dataBeginRemove 和 @sa dataEndRemoved
 * @param d
 */
void DADataManagerInterface::removeData_(DAData& d)
{
    dataManager()->removeData_(d);
}

/**
 * @brief 获取变量管理器管理的数据数量
 * @return
 */
int DADataManagerInterface::getDataCount() const
{
    return dataManager()->getDataCount();
}
/**
 * @brief 参数在变量管理器的索引
 *
 * 参数在变量管理器中有一个list来维护，这个索引就是链表的索引
 * @param d
 * @return
 */
int DADataManagerInterface::getDataIndex(const DAData& d) const
{
    return dataManager()->getDataIndex(d);
}
/**
 * @brief 根据索引获取对应的值
 * @param index
 * @return
 */
DAData DADataManagerInterface::getData(int index) const
{
    return dataManager()->getData(index);
}
/**
 * @brief 根据id获取数据
 * @param id
 * @return
 */
DAData DADataManagerInterface::getDataById(DAData::IdType id) const
{
    return dataManager()->getDataById(id);
}

/**
 * @brief 精确查找数据
 * @param name
 * @return
 */
DAData DADataManagerInterface::findData(const QString& name, Qt::CaseSensitivity cs) const
{
    return dataManager()->findData(name, cs);
}

/**
 * @brief 使用通配符或普通字符串查找匹配的数据
 * @param pattern 查找模式，可以是以下形式：
 *                - 通配符模式：使用*和?作为通配符，*匹配任意多个字符，?匹配单个字符
 *                - 普通字符串：不包含通配符时进行包含匹配
 *                - 特殊字符会被自动转义，确保安全匹配
 * @param cs 大小写敏感设置，默认为不区分大小写
 * @return 返回匹配的DAData列表，如果没有找到匹配项，返回空列表
 *
 * @details
 * 此函数为普通用户设计，提供了简单直观的查找方式。它会自动将通配符转换为正则表达式，
 * 并对普通字符串进行安全转义处理。对于简单查找，这是最推荐的使用方式。
 *
 * @note
 * - 通配符转换规则：* → .*（匹配任意多个字符），? → .（匹配单个字符）
 * - 特殊字符（如.、+、*等）在非通配符模式下会被自动转义
 * - 性能说明：对于大量数据，通配符查找会有一定性能开销，建议缓存结果
 *
 * @warning
 * - 如果pattern为空字符串，将返回空列表
 * - 复杂匹配建议使用正则表达式版本以获得更好的性能和控制力
 *
 * @code{.cpp}
 * // 示例1：查找所有内机数据（通配符模式）
 * DADataManager* mgr = getDataManager();
 * QList<DAData> indoorData = mgr->findDatas("*indoor*");
 * // 这将匹配：391117_indoor_1, 391117_indoor_2, indoor_sensor等
 *
 * // 示例2：查找特定编号的内机
 * QList<DAData> indoor1Data = mgr->findDatas("*indoor_1");
 * // 这将匹配：391117_indoor_1, system_indoor_1, indoor_1等
 *
 * // 示例3：查找模块数据（不区分大小写）
 * QList<DAData> moduleData = mgr->findDatas("*module*");
 * // 这将匹配：module_001, MODULE_A, system_module等
 *
 * // 示例4：查找系统数据（区分大小写）
 * QList<DAData> systemData = mgr->findDatas("System_*", Qt::CaseSensitive);
 * // 这将匹配：System_001，但不会匹配system_001
 *
 * // 示例5：使用问号通配符
 * QList<DAData> sensorData = mgr->findDatas("sensor_?");
 * // 这将匹配：sensor_1, sensor_A，但不会匹配sensor_12
 *
 * // 示例6：普通字符串查找（包含匹配）
 * QList<DAData> tempData = mgr->findDatas("temp");
 * // 这将匹配：temperature, temp_sensor, system_temp等
 *
 * // 示例7：查找精确名称（结合通配符）
 * QList<DAData> exactData = mgr->findDatas("391117_indoor_1");
 * // 这将只匹配完全相同的名称
 *
 * // 示例8：查找多个数据并处理结果
 * QList<DAData> results = mgr->findDatas("*indoor*");
 * for (const DAData& data : results) {
 *     if (data.isDataFrame()) {
 *         qDebug() << "找到内机数据：" << data.getName();
 *         // 进一步处理数据...
 *     }
 * }
 * @endcode
 *
 * @see findDatas(const QRegularExpression&)
 * @see findDataExact()
 */
QList< DAData > DADataManagerInterface::findDatas(const QString& pattern, Qt::CaseSensitivity cs) const
{
    return dataManager()->findDatas(pattern, cs);
}

/**
 * @brief 使用正则表达式查找匹配的数据
 * @param regex 正则表达式对象，用于复杂模式匹配
 * @return 返回匹配的DAData列表，如果没有找到匹配项，返回空列表
 *
 * @details
 * 此函数为高级用户设计，提供完整的正则表达式匹配能力。适合需要复杂匹配模式、
 * 精确控制匹配规则或需要更高性能的场景。正则表达式提供了强大的模式匹配功能，
 * 可以处理通配符无法表达的复杂逻辑。
 *
 * @note
 * - 传入的QRegularExpression必须是有效的，无效的正则表达式会返回空列表并输出警告
 * - 此函数性能通常比通配符版本更好，因为避免了模式转换开销
 * - 对于简单匹配，建议使用通配符版本以获得更好的可读性
 *
 * @warning
 * - 复杂的正则表达式可能影响性能，特别是当数据量很大时
 * - 确保正则表达式的正确性，错误的模式可能导致意外结果
 * - 建议对复杂的正则表达式进行预编译以获得更好的性能
 *
 * @code{.cpp}
 * // 示例1：查找所有内机数据（正则表达式版本）
 * DADataManagerInterface* mgr = ...;
 * QRegularExpression regex1(".*indoor.*", QRegularExpression::CaseInsensitiveOption);
 * QList<DAData> indoorData = mgr->findDatas(regex1);
 *
 * // 示例2：查找特定范围内的编号
 * QRegularExpression regex2(".*indoor_[1-3]");  // 匹配indoor_1, indoor_2, indoor_3
 * QList<DAData> specificIndoorData = mgr->findDatas(regex2);
 *
 * // 示例3：精确匹配特定格式
 * QRegularExpression regex3("^391117_indoor_\\d+$");  // 匹配391117_indoor_后跟数字
 * QList<DAData> exactFormatData = mgr->findDatas(regex3);
 *
 * // 示例4：使用预编译的正则表达式提高性能
 * static QRegularExpression precompiledRegex(".*module.*",
 *                                            QRegularExpression::CaseInsensitiveOption);
 * precompiledRegex.optimize();  // 优化性能
 * QList<DAData> moduleData = mgr->findDatas(precompiledRegex);
 *
 * // 示例5：复杂匹配 - 查找温度或湿度传感器
 * QRegularExpression complexRegex(".*(temp|humidity).*",
 *                                 QRegularExpression::CaseInsensitiveOption);
 * QList<DAData> sensorData = mgr->findDatas(complexRegex);
 *
 * // 示例6：匹配数字编号的内机（更精确）
 * QRegularExpression numberedRegex(".*indoor_\\d{2,3}");  // 匹配2-3位数字编号
 * QList<DAData> numberedIndoorData = mgr->findDatas(numberedRegex);
 *
 * // 示例7：排除特定模式
 * QRegularExpression excludeRegex(".*(?<!test)_indoor.*");  // 匹配不以test结尾的前缀
 * QList<DAData> excludeTestData = mgr->findDatas(excludeRegex);
 *
 * // 示例8：处理正则表达式错误
 * QRegularExpression invalidRegex("[invalid");  // 无效的正则表达式
 * if (invalidRegex.isValid()) {
 *     QList<DAData> data = mgr->findDatas(invalidRegex);
 * } else {
 *     qWarning() << "正则表达式无效：" << invalidRegex.errorString();
 * }
 *
 * // 示例9：批量处理多个正则表达式
 * QList<QRegularExpression> regexList = {
 *     QRegularExpression(".*indoor.*"),
 *     QRegularExpression(".*module.*"),
 *     QRegularExpression(".*system.*")
 * };
 *
 * for (const QRegularExpression& regex : regexList) {
 *     QList<DAData> matches = mgr->findDatas(regex);
 *     qDebug() << "模式" << regex.pattern() << "找到" << matches.size() << "个匹配";
 * }
 * @endcode
 *
 * @see findDatas(const QString&, Qt::CaseSensitivity)
 */
QList< DAData > DADataManagerInterface::findDatasReg(const QRegularExpression& regex) const
{
    return dataManager()->findDatasReg(regex);
}

/**
 * @brief 获取所有数据
 * @return
 */
QList< DAData > DADataManagerInterface::getAllDatas() const
{
    return dataManager()->getAllDatas();
}

/**
 * @brief 获取undo stack
 * @return
 */
QUndoStack* DADataManagerInterface::getUndoStack() const
{
    return dataManager()->getUndoStack();
}
}
