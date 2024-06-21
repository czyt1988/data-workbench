#ifndef DASTANDARDNODEINPUTOUTPUT_H
#define DASTANDARDNODEINPUTOUTPUT_H
#include "DAAbstractNode.h"
namespace DA
{
class DAAbstractNodeFactory;
/**
 * @brief 标准输入输出节点
 */
class DAWORKFLOW_API DAStandardNodeInputOutput : public DAAbstractNode
{
public:
	DAStandardNodeInputOutput();
	~DAStandardNodeInputOutput();
	// 设置显示名字
	void setDisplayName(const QString& name);
	QString getDisplayName() const;
	//
	virtual DA::DAAbstractNodeGraphicsItem* createGraphicsItem() override;
};
}
#endif  // DASTANDARDNODEINPUTOUTPUT_H
