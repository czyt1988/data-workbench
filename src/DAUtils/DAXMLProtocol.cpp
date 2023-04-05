#include "DAXMLProtocol.h"
#include <QHash>
#include <QVector>
#include <QPointF>
//#include <QXmlStreamWriter>
//#include <QXmlStreamReader>
#include <QDomDocument>
#include <QDomElement>
#include <QDomAttr>
#include "DAProperties.h"
//支持QVariant和文本的转换
#include "DAXMLFileInterface.h"

#define DEFAULT_GROUP_NAME "${sa-default-group}"
namespace DA
{
class DAXMLProtocolPrivate
{
    DA_IMPL_PUBLIC(DAXMLProtocol)
public:
    DAXMLProtocolPrivate(DAXMLProtocol* p);
    DAXMLProtocolPrivate(const DAXMLProtocolPrivate& other, DAXMLProtocol* p);
    ~DAXMLProtocolPrivate();
    //
    void copy(const DAXMLProtocolPrivate* other);
    bool fromByteArray(const QByteArray& data);
    bool fromString(const QString& str);
    bool isHasGroup(const QString& groupName) const;
    bool isHasKey(const QString& groupName, const QString& keyName) const;
    void readProp(QDomElement& propEle, DAProperties& prop);
    QString toString();

    //清空内容
    void clear();

    //解析
    bool parser(const QString& str);

public:
    int _classID;
    int _funID;
    DAPropertiesGroup _propGroup;  ///< 存放属性信息
    QString _errorMsg;             ///< 错误信息
};

DAXMLProtocolPrivate::DAXMLProtocolPrivate(DAXMLProtocol* p) : q_ptr(p), _classID(-1), _funID(-1)
{
}

DAXMLProtocolPrivate::DAXMLProtocolPrivate(const DAXMLProtocolPrivate& other, DAXMLProtocol* p)
{
    copy(&other);
    q_ptr = p;
}

DAXMLProtocolPrivate::~DAXMLProtocolPrivate()
{
}

void DAXMLProtocolPrivate::copy(const DAXMLProtocolPrivate* other)
{
    this->_classID   = other->_classID;
    this->_funID     = other->_funID;
    this->_propGroup = other->_propGroup;
    this->_errorMsg  = other->_errorMsg;
    // this->q_ptr = other->q_ptr; //这个绝对不能有
}

bool DAXMLProtocolPrivate::fromByteArray(const QByteArray& data)
{
    clear();
    return (parser(QString(data)));
}

bool DAXMLProtocolPrivate::fromString(const QString& str)
{
    clear();
    return (parser(str));
}

bool DAXMLProtocolPrivate::parser(const QString& str)
{
    QDomDocument doc;

    if (!doc.setContent(str, &_errorMsg)) {
        return (false);
    }
    QDomElement rootele = doc.documentElement();

    if (rootele.isNull()) {
        _errorMsg = QObject::tr("DA xml protocol's root element error");  // DA xml协议的根节点异常
        return (false);
    }
    if (rootele.tagName() != "da") {
        _errorMsg = QObject::tr("root element name error,require \"da\" but get %1").arg(rootele.tagName());  // DA xml协议根节点要求为"da"标签，但解析到的为%1
        return (false);
    }
    DAPropertiesGroup properties;
    int classID;
    int funID;

    //获取类号和功能号
    classID = rootele.attribute("cid", "0").toInt();
    funID   = rootele.attribute("fid", "0").toInt();
    // 获取values节点,如果有多个values，会忽略后面的values
    QDomElement propsEle = rootele.firstChildElement("props");
    if (propsEle.isNull()) {
        //这是一个空的内容包
        _errorMsg = QObject::tr("DA xml protocol loss <props> tag");  // DA xml协议缺失<props>标签
        return (false);
    }
    QDomNodeList propNodeList = propsEle.childNodes();
    auto size                 = propNodeList.size();

    for (auto i = 0; i < size; ++i) {
        QDomElement propEle = propNodeList.at(i).toElement();
        if (propEle.isNull() || (propEle.tagName() != "prop")) {
            continue;
        }
        DAProperties prop;
        QString group = propEle.attribute("group", QString());
        if (group.isEmpty()) {
            //如果没有name属性，赋予默认值
            group = DAXMLProtocol::defaultGroupName();
            if (properties.hasGroup(group)) {
                //如果已经有默认分组，就把原来解析的默认分组取出
                prop = properties.getProperties(group);
            }
        }
        //获取property下的item
        readProp(propEle, prop);
        properties[ group ] = prop;
    }
    this->_funID     = funID;
    this->_classID   = classID;
    this->_propGroup = properties;
    return (true);
}

void DAXMLProtocolPrivate::readProp(QDomElement& propEle, DAProperties& prop)
{
    // 获取group下的item信息
    QDomNodeList itemNodes = propEle.childNodes();
    auto itemsize          = itemNodes.size();

    for (auto i = 0; i < itemsize; ++i) {
        QDomElement ie = itemNodes.at(i).toElement();
        if (ie.isNull() || (ie.tagName() != "item")) {
            continue;
        }
        QString key          = ie.attribute("key");
        QDomElement valueEle = ie.firstChildElement("value");
        QVariant var;
        if (DAXMLFileInterface::loadElement(var, &valueEle)) {
            prop[ key ] = var;
        }
    }
}

bool DAXMLProtocolPrivate::isHasGroup(const QString& groupName) const
{
    return (this->_propGroup.contains(groupName));
}

bool DAXMLProtocolPrivate::isHasKey(const QString& groupName, const QString& keyName) const
{
    auto i = this->_propGroup.find(groupName);

    if (i != this->_propGroup.end()) {
        return (i.value().contains(keyName));
    }
    return (false);
}

QString DAXMLProtocolPrivate::toString()
{
    QDomDocument doc("da");
    QDomElement root = doc.createElement("da");
    root.setAttribute("fid", QString::number(this->_funID));
    root.setAttribute("cid", QString::number(this->_classID));
    //写入props
    QDomElement propsEle = doc.createElement("props");

    //写入prop
    for (auto i = _propGroup.begin(); i != _propGroup.end(); ++i) {
        QDomElement propEle = doc.createElement("prop");
        propEle.setAttribute("group", i.key());
        for (auto j = i.value().begin(); j != i.value().end(); ++j) {
            QDomElement itemEle = doc.createElement("item");
            itemEle.setAttribute("key", j.key());
            QDomElement valueEle = DAXMLFileInterface::makeElement(j.value(), "value", &doc);
            itemEle.appendChild(valueEle);
            propEle.appendChild(itemEle);
        }
        propsEle.appendChild(propEle);
    }
    root.appendChild(propsEle);
    doc.appendChild(root);
    return (doc.toString());
}

void DAXMLProtocolPrivate::clear()
{
    _funID   = -1;
    _classID = -1;
    _propGroup.clear();
    _errorMsg = "";
}

//===================================================
// DAXMLProtocol
//===================================================

DAXMLProtocol::DAXMLProtocol() : DAAbstractProtocol(), d_ptr(new DAXMLProtocolPrivate(this))
{
}

DAXMLProtocol::DAXMLProtocol(const DAXMLProtocol& other) : DAAbstractProtocol()
{
    (*this) = other;
}

DAXMLProtocol::DAXMLProtocol(DAXMLProtocol&& other) : DAAbstractProtocol()
{
    this->d_ptr.reset(other.d_ptr.take());
    d_ptr->q_ptr = this;  //这个尤为关键
}

DAXMLProtocol& DAXMLProtocol::operator=(const DAXMLProtocol& other)
{
    // 1.防止自身赋值
    if (this == (&other)) {
        return (*this);
    }
    this->d_ptr.reset(new DAXMLProtocolPrivate(*(other.d_ptr.data()), this));
    // this->d_ptr->q_ptr = this;//这个尤为关键
    return (*this);
}

DAXMLProtocol::~DAXMLProtocol()
{
}

void DAXMLProtocol::setFunctionID(int funid)
{
    d_ptr->_funID = funid;
}

int DAXMLProtocol::getFunctionID() const
{
    return (d_ptr->_funID);
}

void DAXMLProtocol::setClassID(int classid)
{
    d_ptr->_classID = classid;
}

int DAXMLProtocol::getClassID() const
{
    return (d_ptr->_classID);
}

void DAXMLProtocol::setValue(const QString& groupName, const QString& keyName, const QVariant& var)
{
    d_ptr->_propGroup.setProperty(groupName, keyName, var);
}

void DAXMLProtocol::setValue(const QString& keyName, const QVariant& var)
{
    d_ptr->_propGroup.setProperty(DAXMLProtocol::defaultGroupName(), keyName, var);
}

QStringList DAXMLProtocol::getGroupNames() const
{
    return (d_ptr->_propGroup.keys());
}

QStringList DAXMLProtocol::getKeyNames(const QString& groupName) const
{
    if (!(d_ptr->_propGroup.hasGroup(groupName))) {
        return (QStringList());
    }
    return (d_ptr->_propGroup[ groupName ].keys());
}

/**
 * @brief 从默认分组中获取key值
 * @return
 */
QStringList DAXMLProtocol::getKeyNames() const
{
    return (getKeyNames(DAXMLProtocol::defaultGroupName()));
}

bool DAXMLProtocol::fromString(const QString& str)
{
    return (d_ptr->fromString(str));
}

QString DAXMLProtocol::toString() const
{
    return (d_ptr->toString());
}

bool DAXMLProtocol::fromByteArray(const QByteArray& data)
{
    return (d_ptr->fromByteArray(data));
}

QByteArray DAXMLProtocol::toByteArray() const
{
    return (toString().toUtf8());
}

QString DAXMLProtocol::defaultGroupName()
{
    return (DEFAULT_GROUP_NAME);
}

bool DAXMLProtocol::isHasGroup(const QString& groupName) const
{
    return (d_ptr->isHasGroup(groupName));
}

bool DAXMLProtocol::isHasKey(const QString& groupName, const QString& keyName) const
{
    return (d_ptr->isHasKey(groupName, keyName));
}

QVariant DAXMLProtocol::getValue(const QString& groupName, const QString& keyName, const QVariant& defaultVal) const
{
    return (d_ptr->_propGroup.getProperty(groupName, keyName, defaultVal));
}

QVariant DAXMLProtocol::getDefaultGroupValue(const QString& keyName, const QVariant& defaultVal) const
{
    return (getValue(defaultGroupName(), keyName, defaultVal));
}

/**
 * @brief 转换为SAPropertiesGroup
 * @return
 */
DAPropertiesGroup DAXMLProtocol::toPropGroup() const
{
    return d_ptr->_propGroup;
}

/**
 * @brief 从SAPropertiesGroup转换为xml协议
 * @param props
 */
void DAXMLProtocol::fromPropGroup(const DAPropertiesGroup& props)
{
    d_ptr->_propGroup = props;
}

/**
 * @brief 获取错误信息
 * @return 在@sa setProtocolData 返回false时调用
 */
QString DAXMLProtocol::getErrorString() const
{
    return (d_ptr->_errorMsg);
}

DAXMLProtocolPtr makeXMLProtocolPtr()
{
    return (std::make_shared< DAXMLProtocol >());
}
}  // end DA
