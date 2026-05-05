#ifndef DAABSTRACTNODESETTINGWIDGET_H
#define DAABSTRACTNODESETTINGWIDGET_H

#include "DAGuiAPI.h"
#include "DAPyNodeProxy.h"
#include "DAGlobals.h"
#include <QWidget>
#include <QPointer>
#include <QJsonObject>

namespace DA
{
/**
 * @brief 节点设置窗口的抽象基类
 *
 * 封装了 DAPyNodeProxy 指针管理、描述符缓存等通用功能，
 * 子类只需实现 updateUI() 完成模型到界面的同步。
 *
 * @see DAPyNodeProxy DANodeSettingWidget
 */
class DAGUI_API DAAbstractNodeSettingWidget : public QWidget
{
    Q_OBJECT

    DA_DECLARE_PRIVATE(DAAbstractNodeSettingWidget)

public:
    explicit DAAbstractNodeSettingWidget(QWidget* parent = nullptr);
    ~DAAbstractNodeSettingWidget();

    // 设置/获取节点代理
    void setNodeProxy(DAPyNodeProxy* proxy);
    DAPyNodeProxy* getNodeProxy() const;

    // 获取缓存的描述符（若未设置或为空则返回空 QJsonObject）
    QJsonObject getDescriptor() const;

    // 从描述符中提取 parameters 数组
    QJsonArray getParameters() const;

    // 子类必须实现的界面同步方法
    virtual void updateUI() = 0;
};

}  // end namespace DA

#endif  // DAABSTRACTNODESETTINGWIDGET_H
