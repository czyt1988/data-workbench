#include "DACommandWithTemporaryData.h"
#include <QApplication>
#include <QDir>
#include "DADir.h"
#include "DAPyScripts.h"
namespace DA
{
QString DACommandWithTemporaryData::s_temp_dataframe = DADir::createTempPath("dataframe");
//===================================================
// DACommandWithTemplateData
//===================================================
DACommandWithTemporaryData::DACommandWithTemporaryData(const DAPyDataFrame& df, QUndoCommand* par, bool saveOnConstruct)
    : DACommandWithRedoCount(par), mDataframe(df)
{
	if (saveOnConstruct) {
		save();
	}
}

DACommandWithTemporaryData::~DACommandWithTemporaryData()
{
}

/**
 * @brief 获取临时文件的路径
 * @return
 */
QString DACommandWithTemporaryData::getTemplateFileName() const
{
    return QString::number(quintptr(this));
}

/**
 * @brief 获得临时路径
 * @return
 */
QDir DACommandWithTemporaryData::templateDir() const
{
	QDir dir(getDataframeTempPath());
	return dir;
}

/**
 * @brief 获取临时文件的完整路径
 * @return
 */
QString DACommandWithTemporaryData::getTemplateFilePath() const
{
    return templateDir().absoluteFilePath(getTemplateFileName());
}

/**
 * @brief 把dataframe保存到临时文件中
 * @return
 */
bool DACommandWithTemporaryData::save()
{
	DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
    return py.to_pickle(mDataframe, getTemplateFilePath());
}

/**
 * @brief 从文件加载回来
 * @return
 */
bool DACommandWithTemporaryData::load()
{
	DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
	return py.from_pickle(mDataframe, getTemplateFilePath());
}

DAPyDataFrame& DACommandWithTemporaryData::dataframe()
{
	return mDataframe;
}

const DAPyDataFrame& DACommandWithTemporaryData::dataframe() const
{
	return mDataframe;
}

QString DACommandWithTemporaryData::getDataframeTempPath()
{
	return s_temp_dataframe;
}

}
