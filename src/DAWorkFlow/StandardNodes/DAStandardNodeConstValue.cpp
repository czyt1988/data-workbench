#include "DAStandardNodeConstValue.h"
#include "DAStandardNodeConstValueGraphicsItem.h"
namespace DA
{

DAStandardNodeConstValue::DAStandardNodeConstValue() : DAAbstractNode()
{
	metaData().setNodePrototype("DA.ConstValue");
	metaData().setGroup(u8"common");
	metaData().setNodeName(u8"Const Value");
	addOutputKey("value");
}

DAStandardNodeConstValue::~DAStandardNodeConstValue()
{
}

bool DAStandardNodeConstValue::exec()
{
	return true;
}

void DAStandardNodeConstValue::setDisplayName(const QString& name)
{
	setProperty("display-name", name);
	auto gi = graphicsItem();
	if (gi) {
		if (auto basegi = dynamic_cast< DAStandardNodeConstValueGraphicsItem* >(gi)) {
			basegi->nodeDisplayNameChanged(name);
		}
	}
}

QString DAStandardNodeConstValue::getDisplayName() const
{
	return getProperty("display-name").toString();
}

void DAStandardNodeConstValue::setValue(const QVariant& v)
{
	mValue = v;
}

QVariant DAStandardNodeConstValue::getValue() const
{
	return mValue;
}

DAAbstractNodeGraphicsItem* DAStandardNodeConstValue::createGraphicsItem()
{
	auto item = new DAStandardNodeConstValueGraphicsItem(this);
	return item;
}

}
