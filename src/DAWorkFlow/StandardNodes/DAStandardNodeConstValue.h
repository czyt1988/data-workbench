#ifndef DASTANDARDNODECONSTVALUE_H
#define DASTANDARDNODECONSTVALUE_H
#include "DAAbstractNode.h"
namespace DA
{
/**
 * @brief 常数节点
 */
class DAWORKFLOW_API DAStandardNodeConstValue : public DA::DAAbstractNode
{
public:
	DAStandardNodeConstValue();
	virtual ~DAStandardNodeConstValue();
	// 运行
	virtual bool exec() override;
	// 设置显示名字
	void setDisplayName(const QString& name);
	QString getDisplayName() const;
	// 参数
	void setValue(const QVariant& v);
	QVariant getValue() const;
	//
	virtual DAAbstractNodeGraphicsItem* createGraphicsItem() override;

private:
	QVariant mValue;
};
}
#endif  // DASTANDARDNODECONSTVALUE_H
