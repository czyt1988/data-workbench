#ifndef DADATAMANAGERTREEWIDGET_H
#define DADATAMANAGERTREEWIDGET_H

#include <QWidget>
#include "DAGuiAPI.h"
#include "Models/DADataManagerTreeModel.h"
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

    // 设置是否展开DataFrame到Series
    void setExpandDataframeToSeries(bool on);
    bool isExpandDataframeToSeries() const;

    // 设置列样式
    void setColumnStyle(DADataManagerTreeModel::ColumnStyle style);
    DADataManagerTreeModel::ColumnStyle getColumnStyle() const;

protected:
    void changeEvent(QEvent* e);
private Q_SLOTS:
    void onToolButtonExpandClicked();
    void onToolButtonCollapseClicked();
    void onComboBoxEditTextChanged(const QString& text);
    void updateCompleterModel();

private:
    // 初始化补全模型
    void initCompleter();
    // 更新过滤
    void updateFilter(const QString& text);
    // 收集所有数据项名称用于补全
    QStringList collectAllDataNames() const;
    Ui::DADataManagerTreeWidget* ui;
};
}
#endif  // DADATAMANAGERTREEWIDGET_H
