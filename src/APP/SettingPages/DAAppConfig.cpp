#include "DAAppConfig.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QObject>
#include "SARibbonBar.h"
#include "AppMainWindow.h"
#include "DAAppCore.h"
#include "DAAppUI.h"
#include "DAMessageQueueProxy.h"
namespace DA
{

DAAppConfig::DAAppConfig()
{
    mConfigFilePath = QApplication::applicationDirPath() + "/config.xml";
    mConfigFilePath = QDir::toNativeSeparators(mConfigFilePath);
    insert(DA_CONFIG_KEY_RIBBON_STYLE, int(SARibbonBar::WpsLiteStyleTwoRow));
    insert(DA_CONFIG_KEY_SHOW_LOG_NUM, 5000);  // 5000条日志
}

DAAppConfig::~DAAppConfig()
{
}

void DAAppConfig::setCore(DAAppCore* core)
{
    mCore       = core;
    mUI         = core->getAppUi();
    mMainWindow = qobject_cast< AppMainWindow* >(mUI->getMainWindow());
}

bool DAAppConfig::loadConfig(bool noFileCreateNewOne)
{
    QFile xmlConfigFile(mConfigFilePath);
    if (!xmlConfigFile.exists()) {
        //没有配置文件
        if (noFileCreateNewOne) {
            return saveConfig();
        }
        return false;
    }
    if (!xmlConfigFile.open(QIODevice::ReadWrite)) {
        //有配置文件，但打开失败
        qWarning().noquote() << QObject::tr("can not open config file \"%1\",because %2")  // cn:无法打开配置文件\"%1\",原因是%2
                                        .arg(mConfigFilePath, xmlConfigFile.errorString());
        return false;
    }

    //文件存在了才加载文件
    QString err;
    QDomDocument doc;
    if (!doc.setContent(&xmlConfigFile, &err)) {
        qCritical().noquote() << QObject::tr("can not load config file \"%1\",because %2")  // cn:无法加载配置文件\"%1\",原因是%2
                                         .arg(mConfigFilePath, err);
        return false;
    }
    QDomElement configsEle = doc.firstChildElement("configs");
    if (configsEle.isNull()) {
        qWarning().noquote() << QObject::tr("config file(%1) loss <configs> tag").arg(mConfigFilePath);  // cn:配置文件(%1)缺失<configs>标签
        return false;
    }
    //加载所有管理的配置
    if (!loadFromXml(&configsEle)) {
        return false;
    }

    return true;
}

bool DAAppConfig::saveConfig()
{
    QFile xmlConfigFile(mConfigFilePath);
    if (!xmlConfigFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        //有配置文件，但打开失败
        qCritical() << QObject::tr("can not open config file \"%1\",because %2")  // cn:无法打开配置文件\"%1\",原因是%2
                               .arg(mConfigFilePath, xmlConfigFile.errorString());
        return false;
    }
    QDomDocument doc;
    QDomElement configsEle = doc.createElement("configs");
    doc.appendChild(configsEle);
    //加载所有管理的配置
    if (!saveToXml(&doc, &configsEle)) {
        return false;
    }
    QTextStream s(&xmlConfigFile);
    doc.save(s, 2);
    xmlConfigFile.close();
    return true;
}

/**
 * @brief 保存到xml
 * @param doc
 * @param parentElement
 * @return
 */
bool DAAppConfig::saveToXml(QDomDocument* doc, QDomElement* parentElement) const
{
    auto i = begin();
    for (; i != end(); ++i) {
        QDomElement propEle = doc->createElement("prop");
        propEle.setAttribute("key", i.key());
        QDomElement valueEle = makeElement(i.value(), "value", doc);
        propEle.appendChild(valueEle);
        parentElement->appendChild(propEle);
    }
    parentElement->setAttribute("name", cConfigName);
    return true;
}

/**
 * @brief 从xml加载
 * @param parentElement
 * @return
 */
bool DAAppConfig::loadFromXml(const QDomElement* parentElement)
{
    QDomNodeList ns = parentElement->childNodes();
    for (int i = 0; i < ns.size(); ++i) {
        QDomElement propEle = ns.at(i).toElement();
        if (propEle.isNull()) {
            continue;
        }
        if (propEle.tagName() != "prop") {
            continue;
        }
        QString k            = propEle.attribute("key");
        QDomElement valueEle = propEle.firstChildElement("value");
        if (valueEle.isNull()) {
            continue;
        }
        QVariant v;
        if (!loadElement(v, &valueEle)) {
            continue;
        }
        insert(k, v);
    }
    return true;
}

bool DAAppConfig::apply()
{
    SARibbonBar* bar = mMainWindow->ribbonBar();
    if (bar) {
        SARibbonBar::RibbonStyle ribbonStyle = static_cast< SARibbonBar::RibbonStyle >(value(DA_CONFIG_KEY_RIBBON_STYLE).toInt());
        bar->setRibbonStyle(ribbonStyle);
    }
    bool isOK  = false;
    int logNum = value(DA_CONFIG_KEY_SHOW_LOG_NUM).toInt(&isOK);
    if (isOK && logNum > 10 && logNum < 999999) {
        DAMessageQueueProxy::setGlobalQueueCapacity(logNum);
    }
    return true;
}

}
