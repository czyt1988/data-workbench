#include "DACommandWithTemplateData.h"
#include <QApplication>
#include <QDir>
#include "DAPyScripts.h"
namespace DA
{

//===================================================
// DACommandWithTemplateData
//===================================================
DACommandWithTemplateData::DACommandWithTemplateData(const DAPyDataFrame& df, QUndoCommand* par)
    : DACommandWithRedoCount(par), m_dataframe(df)
{
    m_tempDir.setPath(getTemplateDirPath());
    DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
    py.to_pickle(m_dataframe, getTemplateFilePath());
}

DACommandWithTemplateData::~DACommandWithTemplateData()
{
    m_tempDir.remove(getTemplateFileName());  //析构时删除临时文件
}

/**
 * @brief 获取临时路径
 * @return
 */
QString DACommandWithTemplateData::getTemplateDirPath()
{
    QString appPath = QApplication::applicationDirPath();
    return appPath + QDir::separator() + "tempData";
}

/**
 * @brief 确保临时路径存在
 * @return
 */
bool DACommandWithTemplateData::ensureTemplateDirExists()
{
    QDir tmp(getTemplateDirPath());
    return tmp.mkpath(getTemplateDirPath());
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
QDir& DACommandWithTemplateData::templateDir()
{
    return m_tempDir;
}

/**
 * @brief 获取临时文件的完整路径
 * @return
 */
QString DACommandWithTemplateData::getTemplateFilePath() const
{
    return m_tempDir.absoluteFilePath(getTemplateFileName());
}

/**
 * @brief 从文件加载回来
 * @return
 */
bool DACommandWithTemplateData::load()
{
    DAPyScriptsDataFrame& py = DAPyScripts::getInstance().getDataFrame();
    return py.from_pickle(m_dataframe, getTemplateFilePath());
}

DAPyDataFrame& DACommandWithTemplateData::dataframe()
{
    return m_dataframe;
}

const DAPyDataFrame& DACommandWithTemplateData::dataframe() const
{
    return m_dataframe;
}

}
