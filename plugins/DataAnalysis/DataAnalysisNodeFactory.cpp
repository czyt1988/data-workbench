#include "DataAnalysisNodeFactory.h"
//
#include <QMainWindow>
#include "DACoreInterface.h"
#include "DADockingAreaInterface.h"
#include "DAUIInterface.h"
#include "DAProjectInterface.h"

//! 注册节点在构造函数中执行REGISTE_CLASS宏，即可把节点类注册
//! REGISTE_CLASS(XXXXNode)


DataAnalysisNodeFactory::DataAnalysisNodeFactory() : DA::DAPyNodeFactory()
{
   //注册节点创建的函数指针
   // REGISTE_CLASS(MyNode1);
   // REGISTE_CLASS(MyNode2);
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
