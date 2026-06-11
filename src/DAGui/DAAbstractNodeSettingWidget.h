#ifndef DAABSTRACTNODESETTINGWIDGET_H
#define DAABSTRACTNODESETTINGWIDGET_H

#include "DAGuiAPI.h"
#include "DAPyNode.h"
#include "DAGlobals.h"
#include "NodeSetting/DAParamDef.h"
#include "DAPyWorkFlow/DAPyNodeFactory.h"
#include <QWidget>
#include <QJsonObject>
namespace DA
{
/**
 * @brief 节点设置窗口的抽象基类
 *
 * 封装了 DAPyNode 值持有、描述符缓存等通用功能，
 * 子类只需实现 updateUI() 完成模型到界面的同步。
 *
 * @see DAPyNode DANodeSettingWidget
 */
class DAGUI_API DAAbstractNodeSettingWidget : public QWidget
{
    Q_OBJECT

    DA_DECLARE_PRIVATE(DAAbstractNodeSettingWidget)

public:
    explicit DAAbstractNodeSettingWidget(QWidget* parent = nullptr);
    ~DAAbstractNodeSettingWidget();

    // 设置/获取节点代理
    virtual void setNode(const DAPyNode& proxy);
    const DAPyNode& getNode() const;
    DAPyNode& node();
    // 获取节点元数据
    const DAPyNodeMetaData& getMetaData() const;

    // 从元数据中提取参数定义列表
    const QVector< DAParamDef >& getParamDefs() const;

    // 子类必须实现的界面同步方法
    virtual void updateUI() = 0;
};

}  // end namespace DA

#endif  // DAABSTRACTNODESETTINGWIDGET_H
