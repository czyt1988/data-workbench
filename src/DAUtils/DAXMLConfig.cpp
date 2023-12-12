#include "DAXMLConfig.h"
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>
#include <QDomNode>
#include <QDebug>
#define CONFIG_FILE_NAME "saconfig.cfg"

#define CONFIG_ROOT_NAME "config"
#define CONFIG_GROUP_NAME "group"
#define CONFIG_CONTENT_PROP_NAME "name"
#define CONFIG_KEY_NAME "key"
#define CONFIG_KEY_PROP_NAME_NAME "name"
#define CONFIG_KEY_PROP_TYPE_NAME "type"
#define CONFIG_KEY_LIST_NAME "list"

namespace DA
{

class DAXMLConfig::PrivateData
{
    DA_DECLARE_PUBLIC(DAXMLConfig)
public:
    PrivateData(DAXMLConfig* par);
    PrivateData(DAXMLConfig* par, const QString& cfgPath);
    PrivateData(const PrivateData& other, DAXMLConfig* p);
    bool setCfgFile(const QString& cfgPath);
    bool save(const QString& saveFilePath);

public:
    QString mConfigFilePath;
    bool mIsDirty { false };
};

DAXMLConfig::PrivateData::PrivateData(DAXMLConfig* par) : q_ptr(par), mIsDirty(false)
{
}

DAXMLConfig::PrivateData::PrivateData(DAXMLConfig* par, const QString& cfgPath) : q_ptr(par), mIsDirty(false)
{
    setCfgFile(cfgPath);
}

DAXMLConfig::PrivateData::PrivateData(const PrivateData& other, DAXMLConfig* p)
{
    this->q_ptr           = p;
    this->mConfigFilePath = other.mConfigFilePath;
    this->mIsDirty        = other.mIsDirty;
}

bool DAXMLConfig::PrivateData::setCfgFile(const QString& cfgPath)
{
    mConfigFilePath = cfgPath;
    QFile file(cfgPath);

    if (!file.open(QIODevice::ReadWrite)) {
        return (false);
    }
    if (q_ptr->fromByteArray(file.readAll())) {
        return (true);
    }
    return (false);
}

bool DAXMLConfig::PrivateData::save(const QString& saveFilePath)
{
    QFile file(saveFilePath);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {  //覆盖务必加上QIODevice::Truncate
        return (false);
    }
    QTextStream txt(&file);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    txt.setCodec("UTF-8");
#else
    txt.setEncoding(QStringConverter::Utf8);
#endif
    txt << q_ptr->toString();
    txt.flush();
    file.close();
    mIsDirty = false;
    return (true);
}

//===================================================
// DAXMLConfig
//===================================================

DAXMLConfig::DAXMLConfig() : DAXMLProtocol(), DA_PIMPL_CONSTRUCT
{
}

DAXMLConfig::DAXMLConfig(const QString& filepath) : DAXMLProtocol(), d_ptr(new PrivateData(this, filepath))
{
}

DAXMLConfig::DAXMLConfig(const DAXMLConfig& other) : DAXMLProtocol(), d_ptr(new PrivateData(*(other.d_ptr), this))
{
}

DAXMLConfig::DAXMLConfig(DAXMLConfig&& other) : DAXMLProtocol()
{
    this->d_ptr  = std::move(other.d_ptr);
    d_ptr->q_ptr = this;  //这个尤为关键
}

DAXMLConfig& DAXMLConfig::operator=(const DAXMLConfig& other)
{
    if (this == (&other)) {
        return (*this);
    }
    DAXMLProtocol::operator      =(other);
    this->d_ptr->mConfigFilePath = other.d_func()->mConfigFilePath;
    this->d_ptr->mIsDirty        = other.d_func()->mIsDirty;
    return (*this);
}

DAXMLConfig::~DAXMLConfig()
{
}

/**
 * @brief 设置配置文件路径
 * @param filePath
 * @return
 */
bool DAXMLConfig::setFilePath(const QString& filePath)
{
    return (d_ptr->setCfgFile(filePath));
}

/**
 * @brief 获取配置文件路径
 * @return
 */
QString DAXMLConfig::getFilePath() const
{
    return (d_ptr->mConfigFilePath);
}

/**
 * @brief 设置内容，调用此函数会使dirty为true
 * @param groupName
 * @param keyName
 * @param var
 */
void DAXMLConfig::setValue(const QString& groupName, const QString& keyName, const QVariant& var)
{
    d_ptr->mIsDirty = true;
    DAXMLProtocol::setValue(groupName, keyName, var);
}

/**
 * @brief 设置内容，调用此函数会使dirty为true
 * @param keyName
 * @param var
 */
void DAXMLConfig::setValue(const QString& keyName, const QVariant& var)
{
    d_ptr->mIsDirty = true;
    DAXMLProtocol::setValue(keyName, var);
}

/**
 * @brief 判断是否有改变
 * @return
 */
bool DAXMLConfig::isDirty() const
{
    return (d_ptr->mIsDirty);
}

/**
 * @brief 保存修改
 * 如果有打开文件，会保存到已有文件路径，如果没有打开文件，此函数不做任何动作
 * @return 成功返回true
 */
bool DAXMLConfig::save()
{
    if (d_ptr->mConfigFilePath.isEmpty()) {
        return (false);
    }
    return (saveAs(d_ptr->mConfigFilePath));
}

/**
 * @brief 另存为
 * @param filePath 文件路径
 * @return 成功返回true
 */
bool DAXMLConfig::saveAs(const QString& filePath)
{
    return (d_ptr->save(filePath));
}

/**
 * @brief 保护的setProtocolData函数，此函数在此类不能使用，用setFilePath代替
 * @param data
 * @return
 */
bool DAXMLConfig::fromByteArray(const QByteArray& data)
{
    return (DAXMLProtocol::fromByteArray(data));
}

/**
 * @brief SAXMLConfigParser::splitNamePath
 * @param namePath
 * @param groupName
 * @param keyName
 */
void DAXMLConfig::splitNamePath(const QString& namePath, QString& groupName, QString& keyName)
{
    QStringList pl = namePath.split("/");

    if (1 == pl.size()) {
        groupName = "";
        keyName   = pl[ 0 ];
    } else if (2 == pl.size()) {
        groupName = pl[ 0 ];
        keyName   = pl[ 1 ];
    } else {
        groupName = "";
        keyName   = namePath;
    }
}
}  // end DA
