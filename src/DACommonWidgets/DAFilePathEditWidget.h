#ifndef DAFILEPATHEDITWIDGET_H
#define DAFILEPATHEDITWIDGET_H

#include <QWidget>
#include "DACommonWidgetsAPI.h"
namespace Ui
{
class DAFilePathEditWidget;
}

namespace DA
{

/**
 * @brief 打开文件编辑窗口，可通过此窗口选中一个文件路径
 *
 * 形如下面的选择按钮
 * --------------  ----
 * |C:/src      | |    |
 * --------------  ----
 *
 */
class DACOMMONWIDGETS_API DAFilePathEditWidget : public QWidget
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAFilePathEditWidget)
public:
    explicit DAFilePathEditWidget(QWidget* parent = nullptr);
    ~DAFilePathEditWidget();
    // 设置过滤
    void setNameFilter(const QString& filter);
    void setNameFilters(const QStringList& filters);
    // 获取路径
    QString getFilePath() const;
    void setFilePath(const QString& v);
signals:
    /**
     * @brief 选中路径后会发射此信号
     * @param p 路径
     */
    void selectedPath(const QString& p);
private slots:
    void onToolButtonOpenClicked();
    void onLineEditEditingFinished();

protected:
    void changeEvent(QEvent* e);

private:
    Ui::DAFilePathEditWidget* ui;
};
}

#endif  // DAFILEPATHEDITWIDGET_H
