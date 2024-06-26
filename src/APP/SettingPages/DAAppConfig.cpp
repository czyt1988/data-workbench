﻿#include "DAAppConfig.h"
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
#include "DAAbstractSettingPage.h"
namespace DA
{

DAAppConfig::DAAppConfig()
{
    mVersion = QVersionNumber(0,0,2);
    mConfigFilePath = getAbsoluteConfigFilePath();
    // 先设置默认参数，这些默认参数后续如果配置文件中有会被替换掉
    insert(DA_CONFIG_KEY_RIBBON_STYLE, static_cast< int >(SARibbonBar::RibbonStyleCompactTwoRow));
    insert(DA_CONFIG_KEY_SHOW_LOG_NUM, 5000);             // 5000条日志
    insert(DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE, false);  // 程序在退出时是否保存ui的状态
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
        // 没有配置文件
        if (noFileCreateNewOne) {
            return saveConfig();
        }
        return false;
    }
    if (!xmlConfigFile.open(QIODevice::ReadWrite)) {
        // 有配置文件，但打开失败
        qWarning().noquote() << QObject::tr("can not open config file \"%1\",because %2")  // cn:无法打开配置文件\"%1\",原因是%2
                                    .arg(mConfigFilePath, xmlConfigFile.errorString());
        return false;
    }

    // 文件存在了才加载文件
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
    QVersionNumber version=mVersion;
    QString ver = configsEle.attribute("ver");
    if(ver.isEmpty()){
        version = QVersionNumber();
    }
    // 加载所有管理的配置
    if (!loadFromXml(&configsEle,version)) {
        return false;
    }

    return true;
}

bool DAAppConfig::saveConfig()
{
    QFile xmlConfigFile(mConfigFilePath);
    // 一定要带上QIODevice::Truncate
    if (!xmlConfigFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        // 有配置文件，但打开失败
        qCritical() << QObject::tr("can not open config file \"%1\",because %2")  // cn:无法打开配置文件\"%1\",原因是%2
                           .arg(mConfigFilePath, xmlConfigFile.errorString());
        return false;
    }
    QDomDocument doc;
    QDomElement configsEle = doc.createElement("configs");
    configsEle.setAttribute("ver",mVersion.toString());
    doc.appendChild(configsEle);
    // 加载所有管理的配置
    if (!saveToXml(&doc, &configsEle,mVersion)) {
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
bool DAAppConfig::saveToXml(QDomDocument* doc, QDomElement* parentElement,const QVersionNumber& ver) const
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
bool DAAppConfig::loadFromXml(const QDomElement* parentElement,const QVersionNumber& ver)
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

/**
 * @brief 获取配置文件名字
 * @return
 */
QString DAAppConfig::getConfigFileName()
{
    return "dawork-config.xml";
}

/**
 * @brief 获取配置文件的绝对路径
 * @return
 */
QString DAAppConfig::getAbsoluteConfigFilePath()
{
    return QDir::toNativeSeparators(DAAbstractSettingPage::getConfigFileSavePath() + QDir::separator()
                                    + getConfigFileName());
}

/**
 * @brief 应用配置
 * @return
 */
bool DAAppConfig::apply()
{
    qDebug() << "apply setting";
    SARibbonBar* bar = mMainWindow->ribbonBar();
    if (bar) {
        SARibbonBar::RibbonStyles ribbonStyle = static_cast< SARibbonBar::RibbonStyles >(
            value(DA_CONFIG_KEY_RIBBON_STYLE).toInt());
        bar->setRibbonStyle(ribbonStyle);
    }
    bool isOK  = false;
    int logNum = value(DA_CONFIG_KEY_SHOW_LOG_NUM).toInt(&isOK);
    if (isOK && logNum > 10 && logNum < 999999) {
        DAMessageQueueProxy::setGlobalQueueCapacity(logNum);
    }
    bool isSaveUIState = value(DA_CONFIG_KEY_SAVE_UI_STATE_ON_CLOSE).toBool();
    if (mMainWindow) {
        mMainWindow->setSaveUIStateOnClose(isSaveUIState);
    }
    return true;
}

}  // end da
