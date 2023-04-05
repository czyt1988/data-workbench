#ifndef DANODESETTINGWIDGET_H
#define DANODESETTINGWIDGET_H
#include "DAGuiAPI.h"
#include <QWidget>
#include "DAAbstractNode.h"
namespace Ui
{
class DANodeSettingWidget;
}
namespace DA
{
/**
 * @brief 节点信息设置窗口
 */
class DAGUI_API DANodeSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DANodeSettingWidget(QWidget* parent = nullptr);
    ~DANodeSettingWidget();
    //设置节点
    void setNode(DAAbstractNode::SharedPointer p);
    DAAbstractNode::SharedPointer getNode() const;
    //刷新数据
    void updateData();
private slots:
    void onLineEditNameTextEdited(const QString& t);

private:
    Ui::DANodeSettingWidget* ui;
    DAAbstractNode::WeakPointer _nodePtr;
};
}  // end of namespace DA
#endif  // DANODESETTINGWIDGET_H
