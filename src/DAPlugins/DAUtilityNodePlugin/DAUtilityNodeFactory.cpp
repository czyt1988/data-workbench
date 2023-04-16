#include "DAUtilityNodeFactory.h"
#include "DAUtilityNodeAppExecute.h"
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QApplication>
#include <QDebug>
#ifndef REGISTE_CLASS
#define REGISTE_CLASS(className)                                                                                       \
    do {                                                                                                               \
        FpCreate fp;                                                                                                   \
        auto _m = [](FpCreate f) -> DANodeMetaData {                                                                   \
            DAAbstractNode::SharedPointer t = f();                                                                     \
            DANodeMetaData meta             = t->metaData();                                                           \
            return (meta);                                                                                             \
        };                                                                                                             \
        fp = []() -> DAAbstractNode::SharedPointer { return DAAbstractNode::SharedPointer(new className()); };         \
        mPrototypeTpfp[ _m(fp) ] = fp;                                                                                 \
    } while (0)
#endif
namespace DA
{

//===================================================
// DAUtilityNodeFactory
//===================================================
DAUtilityNodeFactory::DAUtilityNodeFactory(DACoreInterface* c, QObject* p) : DAAbstractNodeFactory(p), mCore(c)
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
    FpCreate fp = mPrototypeTpfp.value(meta, nullptr);
    if (nullptr == fp) {
        return (nullptr);
    }
    return fp();
}

QStringList DAUtilityNodeFactory::getPrototypes() const
{
    QStringList res;

    res.reserve(mPrototypeTpfp.size());
    for (auto i = mPrototypeTpfp.begin(); i != mPrototypeTpfp.end(); ++i) {
        res.append(i.key().getNodePrototype());
    }
    return (res);
}

QList< DANodeMetaData > DAUtilityNodeFactory::getNodesMetaData() const
{
    return (mPrototypeTpfp.keys());
}

DAAbstractNodeWidget* DAUtilityNodeFactory::createNodeSettingWidget(const DAAbstractNode::SharedPointer& p)
{
    // TODO
    return nullptr;
}

DACoreInterface* DAUtilityNodeFactory::core() const
{
    return mCore;
}

void DAUtilityNodeFactory::createMetaData()
{
    REGISTE_CLASS(DAUtilityNodeAppExecute);
}
}
