#include "DACommandWithTemplateData.h"
#include <QApplication>
#include <QDir>
#include "DADir.h"
#include "DAPyScripts.h"
namespace DA
{
QString DACommandWithTemplateData::s_temp_dataframe = DADir::createTempPath("dataframe");
//===================================================
// DACommandWithTemplateData
//===================================================
DACommandWithTemplateData::DACommandWithTemplateData(const DAPyDataFrame& df, QUndoCommand* par, bool saveOnConstruct)
    : DACommandWithRedoCount(par), mDataframe(df)
{
	if (saveOnConstruct) {
		save();
	}
}

DACommandWithTemplateData::~DACommandWithTemplateData()
{
}

/**
 * @brief 获取临时文件的路径
 * @return
 */
QString DACommandWithTemplateData::getTemplateFileName() const
{
    return QString::number(quintptr(this));
}

/**
 * @brief 获得临时路径
 * @return
 */
QDir DACommandWithTemplateData::templateDir() const
{
	QDir dir(getDataframeTempPath());
	return dir;
}

/**
 * @brief 获取临时文件的完整路径
 * @return
 */
QString DACommandWithTemplateData::getTemplateFilePath() const
{
    return templateDir().absoluteFilePath(getTemplateFileName());
}

/**
 * @brief 把dataframe保存到临时文件中
 * @return
 */
bool DACommandWithTemplateData::save()
{
	DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
	return py.to_pickle(mDataframe, getTemplateFilePath());
}

/**
 * @brief 从文件加载回来
 * @return
 */
bool DACommandWithTemplateData::load()
{
	DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
	return py.from_pickle(mDataframe, getTemplateFilePath());
}

DAPyDataFrame& DACommandWithTemplateData::dataframe()
{
	return mDataframe;
}

const DAPyDataFrame& DACommandWithTemplateData::dataframe() const
{
	return mDataframe;
}

QString DACommandWithTemplateData::getDataframeTempPath()
{
	return s_temp_dataframe;
}

}
