#ifndef DASETTINGPAGEPYTHON_H
#define DASETTINGPAGEPYTHON_H
#include "DAAbstractSettingPage.h"
#include <QWidget>
#include "DAGuiAPI.h"

namespace Ui
{
class DASettingPagePython;
}

namespace DA
{
class DAAppConfig;

/**
 * @brief Python 环境设置页：解释器路径、额外模块搜索路径
 */
class DASettingPagePython : public DAAbstractSettingPage
{
    Q_OBJECT

public:
    explicit DASettingPagePython(QWidget* parent = nullptr);
    ~DASettingPagePython();
    virtual void apply() override;
    virtual QString getSettingPageTitle() const override;
    virtual QIcon getSettingPageIcon() const override;
    bool setAppConfig(DAAppConfig* p);
private slots:
    void onLineEditPythonPathTextChanged(const QString& text);
    void onToolButtonBrowseClicked();
    void onToolButtonAutoDetectClicked();
    void onToolButtonTestClicked();
    void onToolButtonAddPathClicked();
    void onToolButtonRemovePathClicked();

private:
    // 读取 python-config.json 中的解释器路径
    QString readPythonConfigJson() const;
    // 写入 python-config.json
    void writePythonConfigJson(const QString& interpreterPath);

private:
    Ui::DASettingPagePython* ui;
    DAAppConfig* mAppConfig { nullptr };
};
}
#endif  // DASETTINGPAGEPYTHON_H
