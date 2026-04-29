#include "DataAnalysisNodeFactory.h"
//
#include <QMainWindow>
#include <QDir>
#include "DACoreInterface.h"
#include "DADockingAreaInterface.h"
#include "DAUIInterface.h"
#include "DAProjectInterface.h"

//! 注册节点在构造函数中执行REGISTE_CLASS宏，即可把节点类注册
//! REGISTE_CLASS(XXXXNode)
//! 本工厂通过 discoverNodes 自动发现 Python @NodeDef 节点（entry_points + 目录扫描）


DataAnalysisNodeFactory::DataAnalysisNodeFactory() : DA::DAPyNodeFactory()
{
   // 自动发现 DADataAnalysisPy 中的 Python 工作流节点
   // 1. 通过 entry_points 发现（setup.py 中注册的 "data_workbench.plugin" 组）
   discoverNodes(QStringList(), true);

   // 2. 额外扫描插件 PyScripts 目录（作为 entry_points 的补充）
   QString pluginPyPath = QDir::currentPath() + "/plugins/DataAnalysis/PyScripts/DADataAnalysisPy";
   if (QDir(pluginPyPath).exists()) {
       discoverNodes(QStringList() << pluginPyPath, false);
   }
}

DataAnalysisNodeFactory::~DataAnalysisNodeFactory()
{
}

void DataAnalysisNodeFactory::setCore(DA::DACoreInterface* c)
{
    mCore = c;
}

/**
 * @brief 工厂的唯一id
 */
QString DataAnalysisNodeFactory::factoryPrototypes() const
{
    return "DA.DataAnalysis";
}

/**
 * @brief 工厂名称
 */
QString DataAnalysisNodeFactory::factoryName() const
{
    return u8"DA DataAnalysis Factory";
}

/**
 * @brief 工厂描述
 */
QString DataAnalysisNodeFactory::factoryDescribe() const
{
    return u8"This is the fundamental data analysis plugin node factory";
}

void DataAnalysisNodeFactory::nodeAddedToWorkflow(DA::DAPyNodeProxy* proxy)
{
}

void DataAnalysisNodeFactory::nodeStartRemove(DA::DAPyNodeProxy* proxy)
{
}

void DataAnalysisNodeFactory::saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const
{
}

void DataAnalysisNodeFactory::loadExternInfoFromXml(const QDomElement* factoryExternElement)
{
}

QMainWindow* DataAnalysisNodeFactory::getMainWindow() const
{
    return mCore->getUiInterface()->getMainWindow();
}
