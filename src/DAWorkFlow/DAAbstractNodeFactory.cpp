#include "DAAbstractNodeFactory.h"
#include <QPointer>
#include <DAWorkFlow.h>

namespace DA
{

/**
 * @brief The DAAbstractNodeFactory::PrivateData class
 */
class DAAbstractNodeFactory::PrivateData
{
	DA_DECLARE_PUBLIC(DAAbstractNodeFactory)
public:
	PrivateData(DAAbstractNodeFactory* p);

public:
	QPointer< DAWorkFlow > mWorkflow;
};

//////////////////////////////////////////////
/// DAAbstractNodeFactoryPrivate
//////////////////////////////////////////////

DAAbstractNodeFactory::PrivateData::PrivateData(DAAbstractNodeFactory* p) : q_ptr(p)
{
}

//////////////////////////////////////////////
/// DAAbstractNodeFactory
//////////////////////////////////////////////

DAAbstractNodeFactory::DAAbstractNodeFactory() : QObject(nullptr), DA_PIMPL_CONSTRUCT
{
}

DAAbstractNodeFactory::~DAAbstractNodeFactory()
{
	qDebug() << "factory destroy";
}

void DAAbstractNodeFactory::registWorkflow(DAWorkFlow* wf)
{
	d_ptr->mWorkflow = wf;
}

DAWorkFlow* DAAbstractNodeFactory::getWorkFlow() const
{
	return d_ptr->mWorkflow.data();
}

DAAbstractNodeFactory::SharedPointer DAAbstractNodeFactory::pointer()
{
	return (shared_from_this());
}

void DAAbstractNodeFactory::initializNode(const DAAbstractNode::SharedPointer& node)
{
	node->registFactory(shared_from_this());
}

void DAAbstractNodeFactory::nodeAddedToWorkflow(DAAbstractNode::SharedPointer node)
{
	Q_UNUSED(node);
}

void DAAbstractNodeFactory::nodeStartRemove(DAAbstractNode::SharedPointer node)
{
    Q_UNUSED(node);
}

void DAAbstractNodeFactory::nodeLinkSucceed(DAAbstractNode::SharedPointer outNode,
                                            const QString& outKey,
                                            DAAbstractNode::SharedPointer inNode,
                                            const QString& inkey)
{
	Q_UNUSED(outNode);
	Q_UNUSED(outKey);
	Q_UNUSED(inNode);
	Q_UNUSED(inkey);
}

void DAAbstractNodeFactory::nodeLinkDetached(DAAbstractNode::SharedPointer outNode,
                                             const QString& outKey,
                                             DAAbstractNode::SharedPointer inNode,
                                             const QString& inkey)
{
	Q_UNUSED(outNode);
	Q_UNUSED(outKey);
	Q_UNUSED(inNode);
	Q_UNUSED(inkey);
}

void DAAbstractNodeFactory::saveExternInfoToXml(QDomDocument* doc, QDomElement* factoryExternElement) const
{
	Q_UNUSED(doc);
	Q_UNUSED(factoryExternElement);
}

void DAAbstractNodeFactory::loadExternInfoFromXml(const QDomElement* factoryExternElement)
{
	Q_UNUSED(factoryExternElement);
}

void DAAbstractNodeFactory::uiInitialization(DANodeGraphicsScene* scene)
{
	Q_UNUSED(scene);
}

void DAAbstractNodeFactory::workflowReady()
{
}

}  // end DA
