#ifndef DAABSTRACTPROTOCOL_H
#define DAABSTRACTPROTOCOL_H
#include <QVariant>
#include "DAUtilsAPI.h"
namespace DA
{

/**
 * @brief da协议解析基类
 * @see DAAbstractProtocolMaker
 */
class DAUTILS_API DAAbstractProtocol
{
public:
    DAAbstractProtocol();
    virtual ~DAAbstractProtocol();

public:
    virtual void setFunctionID(int funid) = 0;              // 设置协议功能号
    virtual int getFunctionID() const = 0;                  // 获取协议功能号
    virtual void setClassID(int classid) = 0;               // 设置协议类号
    virtual int getClassID() const = 0;                     // 获取协议类号
    virtual bool fromByteArray(const QByteArray& data) = 0; // 设置协议的内容
    virtual bool fromString(const QString& str) = 0;        // 从字符串转换到协议
    // 设置键值，斜杠是分组和键值的分割符，因此键值不应该存在斜杠
    virtual void setValue(const QString& groupName, const QString& keyName, const QVariant& var) = 0;
    virtual void setValue(const QString& keyName, const QVariant& var)                           = 0;
    virtual QStringList getGroupNames() const = 0;                                                // 获取所有目录关键字，包含默认分组
    virtual QStringList getKeyNames(const QString& groupName) const = 0;                          // 获取目录下对应的所有关键字
    virtual QString toString() const = 0;                                                         // 转换为文本，utf8编码
    virtual QByteArray toByteArray() const = 0;                                                   // 转换为bytearray
    virtual bool isHasGroup(const QString& groupName) const = 0;                                  // 检测是否存在分组
    virtual bool isHasKey(const QString& groupName, const QString& keyName) const = 0;            // 检查在分组名下是否存在对应的键值
    // 获取键值对应的内容，如果键值不存在返回默认值
    virtual QVariant getValue(const QString& groupName, const QString& keyName, const QVariant& defaultVal = QVariant()) const = 0;
    // 从默认分组获取键值对应的内容，如果键值不存在返回默认值
    virtual QVariant getDefaultGroupValue(const QString& keyName, const QVariant& defaultVal = QVariant()) const = 0;
};
}  // end DA
#endif  // SAABSTRACTPROTOCOLPARSE_H
