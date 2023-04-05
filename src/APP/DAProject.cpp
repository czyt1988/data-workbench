#include "DAProject.h"
// Qt
#include <QBuffer>
#include <QDomDocument>
#include <QFile>
#include <QScopedPointer>
#include <QVariant>
#include <QPen>
#include <QElapsedTimer>
#include <QSet>
// DA
#include "DAWorkFlowGraphicsScene.h"
#include "DAAppPluginManager.h"
#include "DAGraphicsResizeablePixmapItem.h"
#include "DAStandardNodeLinkGraphicsItem.h"
#include "DAGraphicsItemFactory.h"
#include "DAAbstractNodeFactory.h"
#include "DAGraphicsItem.h"
#include "DAXMLFileInterface.h"
#include "DAStringUtil.h"
#include "DAWorkFlowOperateWidget.h"
#include "DAWorkFlowEditWidget.h"
#include "DAXmlHelper.h"
namespace DA
{

////////////////////////////////////////////////////

DAProject::DAProject(DACoreInterface* c, QObject* p) : DAProjectInterface(c, p)
{
}

DAProject::~DAProject()
{
}

/**
 * @brief 清除工程
 */
void DAProject::clear()
{
    DAWorkFlowOperateWidget* wfo = getWorkFlowOperateWidget();
    Q_CHECK_PTR(wfo);
    wfo->clear();
    DAProjectInterface::clear();
}

/**
 * @brief 工程文件的后缀
 * @return
 */
QString DAProject::getProjectFileSuffix()
{
    return "asproj";
}

}  // end DA
