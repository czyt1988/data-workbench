#include "DADataManager.h"
#include <QList>
#include <QMap>
#include <QDebug>
#include <QUndoStack>
#include <QRegularExpressionMatch>
// DAUtils
#include "DAStringUtil.h"
//
#include "DACommandsDataManager.h"
namespace DA
{

class DADataManager::PrivateData
{
    DA_DECLARE_PUBLIC(DADataManager)
public:
    PrivateData(DADataManager* p);
    QList< DAData > _dataList;
    QMap< DAData::IdType, DAData > _dataMap;
    bool _dirtyFlag;               ///< 标记是否dirty
    QUndoStack _dataManagerStack;  ///< 数据管理的stack
};

//===================================================
// DADataManagerPrivate
//===================================================

DADataManager::PrivateData::PrivateData(DADataManager* p) : q_ptr(p), _dirtyFlag(false)
{
}

//===================================================
// DADataManager
//===================================================
DADataManager::DADataManager(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

DADataManager::~DADataManager()
{
}

/**
 * @brief 添加数据
 *
 * @note 此函数会发生信号@sa dataAdded
 * @param d
 */
void DADataManager::addData(DAData& d)
{
    if (d_ptr->_dataMap.contains(d.id())) {
        // 说明已经添加过
        qWarning() << tr("data:%1 have been added").arg(d.getName());
        if (d.getDataManager() != this) {
            // 说明这个data引用没有获取到datamanager
            d.setDataManager(this);
        }
        return;
    }
    setUniqueDataName(d);
    d.setDataManager(this);
    d_ptr->_dataList.push_back(d);
    d_ptr->_dataMap[ d.id() ] = d;
    setDirtyFlag(true);
    Q_EMIT dataAdded(d);
}
/**
 * @brief 带redo/undo的添加数据
 *
 * @note 此函数会发生信号@sa dataBeginAdd 和 @sa dataEndAdded
 * @param d
 */
void DADataManager::addData_(DAData& d)
{
    DACommandDataManagerAdd* cmd = new DACommandDataManagerAdd(d, this);
    d_ptr->_dataManagerStack.push(cmd);
}

/**
 * @brief 添加数据
 * @param d
 * @return 返回数据DAData
 */
DAData DADataManager::addData(const DAAbstractData::Pointer& d)
{
    DAData res(d);
    addData(res);
    return res;
}
/**
 * @brief 带redo/undo的添加数据
 * @param d
 * @return 返回数据DAData
 */
DAData DADataManager::addData_(const DAAbstractData::Pointer& d)
{
    DAData res(d);
    addData_(res);
    return res;
}
/**
 * @brief 批量添加数据
 *
 * 等同多次调用addData_，会发射多次信号
 * @param d
 */
void DADataManager::addDatas_(const QList< DAData >& datas)
{
    std::unique_ptr< QUndoCommand > cmdGroup(new QUndoCommand(tr("add datas")));
    for (const DAData& d : datas) {
        new DACommandDataManagerAdd(d, this, cmdGroup.get());
    }
    if (cmdGroup->childCount() > 0) {
        d_ptr->_dataManagerStack.push(cmdGroup.release());
    }
}

/**
 * @brief 移除数据
 *
 * @note 此函数会发生信号@sa dataRemoved
 * @param d
 */
void DADataManager::removeData(DAData& d)
{
    int index = d_ptr->_dataList.indexOf(d);
    Q_EMIT dataBeginRemove(d, index);
    doRemoveData(d);
    Q_EMIT dataRemoved(d, index);
}

/**
 * @brief 带redo/undo的移除数据
 * @note 此函数会发生信号@sa dataBeginRemove 和 @sa dataEndRemoved
 * @param d
 */
void DADataManager::removeData_(DAData& d)
{
    DACommandDataManagerRemove* cmd = new DACommandDataManagerRemove(d, this);
    d_ptr->_dataManagerStack.push(cmd);
}

/**
 * @brief 批量移除数据
 * @param datas
 */
void DADataManager::removeDatas_(const QList< DAData >& datas)
{
    std::unique_ptr< QUndoCommand > cmdGroup(new QUndoCommand(tr("remove datas")));
    for (const DAData& d : datas) {
        new DACommandDataManagerRemove(d, this, cmdGroup.get());
    }
    if (cmdGroup->childCount() > 0) {
        d_ptr->_dataManagerStack.push(cmdGroup.release());
    }
}

/**
 * @brief 获取变量管理器管理的数据数量
 * @return
 */
int DADataManager::getDataCount() const
{
    return d_ptr->_dataList.size();
}

/**
 * @brief 参数在变量管理器的索引
 *
 * 参数在变量管理器中有一个list来维护，这个索引就是链表的索引
 * @param d
 * @return 如果没有在管理器中找到data，返回-1
 */
int DADataManager::getDataIndex(const DAData& d) const
{
    return d_ptr->_dataList.indexOf(d);
}

/**
 * @brief 根据索引获取对应的值
 * @param index
 * @return
 */
DAData DADataManager::getData(int index) const
{
    return d_ptr->_dataList.at(index);
}

/**
 * @brief 精确查找数据
 * @param name
 * @return
 */
DAData DADataManager::findData(const QString& name, Qt::CaseSensitivity cs) const
{
    for (const DAData& data : std::as_const(d_ptr->_dataList)) {
        if (data.isNull()) {
            continue;
        }

        if (data.getName().compare(name, cs) == 0) {
            return data;
        }
    }
    return DAData();
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
 * @see findData
 */
QList< DAData > DADataManager::findDatas(const QString& pattern, Qt::CaseSensitivity cs) const
{
    // 将通配符模式转换为正则表达式
    QRegularExpression regex = wildcardToRegex(pattern, cs);
    return findDatasReg(regex);
}

QRegularExpression DADataManager::wildcardToRegex(const QString& pattern, Qt::CaseSensitivity cs)
{
    if (pattern.isEmpty()) {
        return QRegularExpression(".*", QRegularExpression::CaseInsensitiveOption);
    }

    QString regexPattern;
    bool hasWildcard = pattern.contains(QLatin1Char('*')) || pattern.contains(QLatin1Char('?'));

    if (hasWildcard) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
        // Qt 5.12+ 使用内置函数（更可靠）
        regexPattern = QRegularExpression::wildcardToRegularExpression(pattern);
#else
        // 低版本 Qt 手动实现通配符转换
        regexPattern = QRegularExpression::escape(pattern);
        regexPattern.replace(QLatin1Char('*'), QLatin1String(".*"));
        regexPattern.replace(QLatin1Char('?'), QLatin1String("."));
#endif
    } else {
        // 无通配符：转义特殊字符 + 前后拼接 .* 实现“包含匹配”
        regexPattern = QLatin1String(".*") + QRegularExpression::escape(pattern) + QLatin1String(".*");
    }

    // 构建正则选项
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (cs == Qt::CaseInsensitive) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    // 增加优化选项：减少回溯，提升匹配性能
    options |= QRegularExpression::DontCaptureOption;

    QRegularExpression regex(regexPattern, options);
    // 校验正则有效性（提前报错，避免运行时问题）
    if (!regex.isValid()) {
        qWarning() << "[DADataManager::wildcardToRegex] Invalid regex pattern:" << regexPattern
                   << "Error:" << regex.errorString();
        // 回退到匹配空（避免返回无效正则导致崩溃）
        return QRegularExpression();
    }

    return regex;
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
 * DADataManager* mgr = getDataManager();
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
QList< DAData > DADataManager::findDatasReg(const QRegularExpression& regex) const
{
    QList< DAData > result;

    if (!regex.isValid()) {
        qWarning() << "Invalid regular expression pattern:" << regex.pattern();
        return result;
    }

    for (const DAData& data : std::as_const(d_ptr->_dataList)) {
        if (data.isNull()) {
            continue;
        }

        QString dataName = data.getName();
        // 使用 match 并指定匹配范围（0 到末尾，即完全匹配）
        QRegularExpressionMatch match = regex.match(dataName, 0, QRegularExpression::NormalMatch);
        if (match.hasMatch()) {
            result.append(data);
        }
    }

    return result;
}

/**
 * @brief 获取所有数据
 * @return
 */
QList< DAData > DADataManager::getAllDatas() const
{
    return d_ptr->_dataList;
}

/**
 * @brief 根据id获取数据
 * @param id
 * @return
 */
DAData DADataManager::getDataById(DAData::IdType id) const
{
    return d_ptr->_dataMap.value(id, DAData());
}

/**
 * @brief 判断是否dirty，数据的改变和添加都会把此flag标记为true
 * @return
 */
bool DADataManager::isDirty() const
{
    return d_ptr->_dirtyFlag;
}

/**
 * @brief 设置脏标记
 *
 * 此函数在 判断 是否 需要保存时使用
 * @param on
 */
void DADataManager::setDirtyFlag(bool on)
{
    d_ptr->_dirtyFlag = on;
}

/**
 * @brief 获取undo stack
 * @return
 */
QUndoStack* DADataManager::getUndoStack() const
{
    return &(d_ptr->_dataManagerStack);
}

/**
 * @brief 触发DataChanged信号
 * @param d
 * @param t
 */
void DADataManager::notifyDataChangedSignal(const DAData& d, DADataManager::ChangeType t)
{
    setDirtyFlag(true);
    Q_EMIT dataChanged(d, t);
}

void DADataManager::setUniqueDataName(DAData& d) const
{
    QString n = d.getName();
    if (n.isEmpty()) {
        n = d.typeToString();
        d.setName(n);
    }
    QSet< QString > names = getDatasNameSet();
    // 构造一个唯一的名字
    n = DA::makeUniqueString(names, n);
    d.setName(n);
}
/**
 * @brief 把所有管理的变量的名字按照set返回
 * @return
 */
QSet< QString > DADataManager::getDatasNameSet() const
{
    QSet< QString > names;
    for (const DAData& d : std::as_const(d_ptr->_dataList)) {
        names.insert(d.getName());
    }
    return names;
}

/**
 * @brief 移除数据
 * @param d
 */
void DADataManager::doRemoveData(DAData& d)
{
    int index = d_ptr->_dataList.indexOf(d);
    d_ptr->_dataList.removeAt(index);
    d_ptr->_dataMap.remove(d.id());
    d.setDataManager(nullptr);
    setDirtyFlag(true);
}

/**
 * @brief 清除数据
 */
void DADataManager::clear()
{
    // 栈清空
    d_ptr->_dataManagerStack.clear();
    // 数据清空
    d_ptr->_dataList.clear();
    d_ptr->_dataMap.clear();
    setDirtyFlag(false);
    Q_EMIT datasCleared();
}

}
