#ifndef DACOMMANDWITHTEMPLATEDATA_H
#define DACOMMANDWITHTEMPLATEDATA_H
#include <QDir>
#include "DAGuiAPI.h"
#include "DACommandWithRedoCount.h"
#include "DAData.h"
namespace DA
{
/**
 * @brief 此命令实现了临时文件接口，需要保存临时文件的继承此类
 * TODO:这个类的名字需要修改为DACommandDataframeWithTemplateData
 */
class DAGUI_API DACommandWithTemplateData : public DACommandWithRedoCount
{
public:
	/**
	 * @brief 构造函数执行会自动把原始的dataframe保存到临时目录中
	 * @param df
	 * @param par
	 * @param saveOnConstruct 是否构造时就保存副本
	 */
	DACommandWithTemplateData(const DAPyDataFrame& df, QUndoCommand* par = nullptr, bool saveOnConstruct = true);
	~DACommandWithTemplateData();

	// 获取临时文件的名字
	QString getTemplateFileName() const;
	// 临时路径
	QDir templateDir() const;
	// 获取临时文件的完整l路径
	QString getTemplateFilePath() const;
	// 保存
	bool save();
	// 从文件加载回来
	bool load();
	// 存放dataframe
	DAPyDataFrame& dataframe();
	const DAPyDataFrame& dataframe() const;
	/**
	 * @brief 获取Dataframe保存的临时文件路径
	 * @return
	 */
	static QString getDataframeTempPath();

protected:
	DAPyDataFrame mDataframe;

private:
	static QString s_temp_dataframe;
};
}  // end of namespace DA
#endif  // DACOMMANDWITHTEMPLATEDATA_H
