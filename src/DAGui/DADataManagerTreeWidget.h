#ifndef DADATAMANAGERTREEWIDGET_H
#define DADATAMANAGERTREEWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
namespace Ui
{
class DADataManagerTreeWidget;
}
namespace DA
{
class DADataManager;

/**
 * @brief 用于展示数据管理器的数据内容的树形窗口
 */
class DADataManagerTreeWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DADataManagerTreeWidget)
public:
    explicit DADataManagerTreeWidget(QWidget* parent = nullptr);
    ~DADataManagerTreeWidget();
    // 设置数据管理器
    void setDataManager(DADataManager* dataMgr);
    DADataManager* getDataManager() const;

protected:
    void changeEvent(QEvent* e);
private Q_SLOTS:
    void onToolButtonExpandClicked();
    void onToolButtonCollapseClicked();
    void onComboBoxEditTextChanged(const QString& text);

private:
    Ui::DADataManagerTreeWidget* ui;
};
}
#endif  // DADATAMANAGERTREEWIDGET_H
