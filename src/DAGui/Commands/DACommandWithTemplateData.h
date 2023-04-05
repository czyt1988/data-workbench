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
 */
class DAGUI_API DACommandWithTemplateData : public DACommandWithRedoCount
{
public:
    DACommandWithTemplateData(const DAPyDataFrame& df, QUndoCommand* par = nullptr);
    ~DACommandWithTemplateData();
    //获取临时文件路径
    static QString getTemplateDirPath();
    //确保临时路径存在
    static bool ensureTemplateDirExists();
    //获取临时文件的名字
    virtual QString getTemplateFileName() const;
    //临时路径
    QDir& templateDir();
    //获取临时文件的完整l路径
    QString getTemplateFilePath() const;
    //从文件加载回来
    bool load();
    //存放dataframe
    DAPyDataFrame& dataframe();
    const DAPyDataFrame& dataframe() const;

protected:
    DAPyDataFrame m_dataframe;
    QDir m_tempDir;  ///< 临时文件夹的路径
};
}  // end of namespace DA
#endif  // DACOMMANDWITHTEMPLATEDATA_H
