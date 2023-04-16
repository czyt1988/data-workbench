#include "DAUtilityNodePlugin.h"
#include <QObject>
#include <QFont>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QApplication>
#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QAction>
// SARibbon
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPannel.h"
// DA - interface
#include "DACoreInterface.h"
#include "DAUIInterface.h"
#include "DADockingAreaInterface.h"
#include "DARibbonAreaInterface.h"
// DA
#include "DAUtilityNodeFactory.h"
#include "DAProjectInterface.h"
#include "DATranslatorManeger.h"
#include "DAWorkFlowNodeListWidget.h"
#include "DANodeTreeWidget.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAChartOperateWidget.h"
#include "DAChartManageWidget.h"
#include "DADataManageWidget.h"
#include "DADataOperateWidget.h"
#include "DAMessageLogViewWidget.h"
#include "DANodeTreeWidget.h"

#define ROLE_IID (Qt::UserRole + 1)
#define ROLE_FIX_PROJECT (Qt::UserRole + 2)

namespace DA
{

//===================================================
//
//===================================================

/**
 * @brief
 *
 * @note 继承的QObject()不能传入父对象，否则@sa plugin_destory 在删除对象时会重复删除
 * @param c
 */
DAUtilityNodePlugin::DAUtilityNodePlugin(QObject* par) : QObject(par), DAAbstractNodePlugin()
{
}

DAUtilityNodePlugin::~DAUtilityNodePlugin()
{
}

bool DAUtilityNodePlugin::initialize()
{
    init();
    return true;
}
void DAUtilityNodePlugin::init()
{
    DATranslatorManeger mgr({ "daUtilsPlugin" });
    mgr.installAllTranslator();
}
QString DAUtilityNodePlugin::getIID() const
{
    return (IID_DAUTILITYNODEPLUGIN);
}

QString DAUtilityNodePlugin::getName() const
{
    return QObject::tr("Utility Nodes");
}

QString DAUtilityNodePlugin::getVersion() const
{
    return ("version 0.1.1");
}

QString DAUtilityNodePlugin::getDescription() const
{
    return QObject::tr("Data Work Flow Utility Nodes\n"
                       "Author:czy");
}

DAAbstractNodeFactory* DAUtilityNodePlugin::createNodeFactory()
{
    return (new DAUtilityNodeFactory(core()));
}

void DAUtilityNodePlugin::retranslate()
{
}

void DAUtilityNodePlugin::destoryNodeFactory(DAAbstractNodeFactory* p)
{
    if (p) {
        p->deleteLater();
    }
}

}  // end DA
