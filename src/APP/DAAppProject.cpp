#include "DAAppProject.h"
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
#include "DAWorkFlowOperateWidget.h"
#include "DAXmlHelper.h"
namespace DA
{

////////////////////////////////////////////////////

DAAppProject::DAAppProject(DACoreInterface* c, QObject* p) : DAProjectInterface(c, p)
{
}

DAAppProject::~DAAppProject()
{
}

/**
 * @brief 清除工程
 */
void DAAppProject::clear()
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
QString DAAppProject::getProjectFileSuffix()
{
    return "asproj";
}

}  // end DA
