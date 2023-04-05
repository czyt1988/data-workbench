#include "DAUtilityNodeFactory.h"
#include "DAUtilityNodeAppExecute.h"
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QApplication>
#include <QDebug>

QHash< QString, QPair< QString, QStringList > > DA::DAUtilityNodeFactory::s_settingMap = DA::DAUtilityNodeFactory::getSettingMap();
//===================================================
// using DA namespace -- 禁止在头文件using!!
//===================================================

using namespace DA;

//===================================================
//
//===================================================
#define REGISTE_CLASS(className)                                                                                       \
    do {                                                                                                               \
        FpCreate fp;                                                                                                   \
        auto _m = [](FpCreate f) -> DANodeMetaData {                                                                   \
            DAAbstractNode::SharedPointer t = f();                                                                     \
            DANodeMetaData meta             = t->metaData();                                                           \
            return (meta);                                                                                             \
        };                                                                                                             \
        fp = []() -> DAAbstractNode::SharedPointer { return DAAbstractNode::SharedPointer(new className()); };         \
        m_prototypeTpfp[ _m(fp) ] = fp;                                                                                \
    } while (0)

DAUtilityNodeFactory::DAUtilityNodeFactory(DACoreInterface* c, QObject* p) : DAAbstractNodeFactory(p), _core(c)
{
    createMetaData();
}

QString DAUtilityNodeFactory::factoryPrototypes() const
{
    return IID_DAUTILITYNODEPLUGIN;
}

QString DAUtilityNodeFactory::factoryName() const
{
    return QObject::tr("Utility");
}

QString DAUtilityNodeFactory::factoryDescribe() const
{
    return QObject::tr("Core nodes");
}

DAAbstractNode::SharedPointer DAUtilityNodeFactory::create(const DANodeMetaData& meta)
{
    FpCreate fp = m_prototypeTpfp.value(meta, nullptr);
    if (nullptr == fp) {
        return (nullptr);
    }
    return fp();
}

QStringList DAUtilityNodeFactory::getPrototypes() const
{
    QStringList res;

    res.reserve(m_prototypeTpfp.size());
    for (auto i = m_prototypeTpfp.begin(); i != m_prototypeTpfp.end(); ++i) {
        res.append(i.key().getNodePrototype());
    }
    return (res);
}

QList< DANodeMetaData > DAUtilityNodeFactory::getNodesMetaData() const
{
    return (m_prototypeTpfp.keys());
}

DAAbstractNodeWidget* DAUtilityNodeFactory::createNodeSettingWidget(const DAAbstractNode::SharedPointer& p)
{
    // TODO
    return nullptr;
}

QHash< QString, QPair< QString, QStringList > > DAUtilityNodeFactory::getSettingMap()
{
    QHash< QString, QPair< QString, QStringList > > res;
    QString settingpath = QString("%1/setting.xml").arg(QApplication::applicationDirPath());
    QFile file(settingpath);
    if (!file.open(QIODevice::ReadWrite)) {
        qCritical() << tr("can not load setting file");
        return res;
    }
    QString err;
    QDomDocument doc;
    if (!doc.setContent(&file, &err)) {
        qCritical() << tr("can not load setting file,error info is:%1").arg(err);
        return res;
    }
    QDomElement rootEle = doc.documentElement();
    QDomElement exeEle  = rootEle.firstChildElement("execute");
    if (exeEle.isNull()) {
        qCritical() << tr("setting file,loss execute tag");
        return res;
    }
    QDomNodeList ns = exeEle.childNodes();
    for (int i = 0; i < ns.size(); ++i) {
        QDomNode n = ns.at(i);
        if (n.nodeName() == "map") {
            QDomElement mapEle = n.toElement();
            QString key        = mapEle.attribute("key");
            QDomElement exeEle = mapEle.firstChildElement("exe");
            if (exeEle.isNull()) {
                continue;
            }
            QString value = exeEle.text();
            QStringList args;
            QDomElement argsEle = mapEle.firstChildElement("args");
            if (!argsEle.isNull()) {
                //有参数
                QDomNodeList nl = argsEle.childNodes();
                for (int i = 0; i < nl.size(); ++i) {
                    QDomElement aEle = nl.at(i).toElement();
                    if (aEle.isNull()) {
                        continue;
                    }
                    args.append(aEle.text());
                }
            }
            res[ key ] = qMakePair(value, args);
            qDebug() << "[" << key << "]-> exe=" << value << " args=" << args;
        }
    }
    return res;
}

DACoreInterface* DAUtilityNodeFactory::core() const
{
    return _core;
}

void DAUtilityNodeFactory::createMetaData()
{
    REGISTE_CLASS(DAUtilityNodeAppExecute);
}
