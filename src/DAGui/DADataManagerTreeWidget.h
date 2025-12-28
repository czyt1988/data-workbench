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

    // 获取当前选中的Data,如果选中的是series，返回的是series对应dataframe的DAData
    DAData getCurrentSelectData() const;

    // 当前是否选中了dataframe，选中了dataframe下面的series会返回false
    bool isSelectDataframe() const;

    // 当前是否选中了dataframe下面的series
    bool isSelectDataframeSeries() const;

    // 返回当前选中的series的名字
    QString getCurrentSelectSeriesName() const;

protected:
    void changeEvent(QEvent* e);
private Q_SLOTS:
    void onToolButtonExpandClicked();
    void onToolButtonCollapseClicked();
    void onComboBoxEditTextChanged(const QString& text);
    void updateCompleterModel();
    void applyFilter();

private:
    // 更新过滤
    void updateFilter(const QString& text);
    // 收集所有数据项名称用于补全
    QStringList collectAllDataNames() const;
    Ui::DADataManagerTreeWidget* ui;
};
}
#endif  // DADATAMANAGERTREEWIDGET_H
