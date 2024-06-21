#include "DAStandardNodeInputOutput.h"
#include "DAStandardNodeInputOutputGraphicsItem.h"

namespace DA
{

DAStandardNodeInputOutput::DAStandardNodeInputOutput() : DAAbstractNode()
{
	metaData().setNodePrototype("DA.StandardInputOutputNode");
	metaData().setGroup(u8"fucntion");
	metaData().setNodeName(u8"Standard IO");
}

DAStandardNodeInputOutput::~DAStandardNodeInputOutput()
{
}

void DAStandardNodeInputOutput::setDisplayName(const QString& name)
{
	setProperty("_da.display-name", name);
	auto gi = graphicsItem();
	if (gi) {
		if (auto basegi = dynamic_cast< DAStandardNodeInputOutputGraphicsItem* >(gi)) {
			basegi->nodeDisplayNameChanged(name);
		}
	}
}

QString DAStandardNodeInputOutput::getDisplayName() const
{
	return getProperty("_da.display-name").toString();
}

DA::DAAbstractNodeGraphicsItem* DAStandardNodeInputOutput::createGraphicsItem()
{
	DAStandardNodeInputOutputGraphicsItem* item = new DAStandardNodeInputOutputGraphicsItem(this);
	return item;
}
}
