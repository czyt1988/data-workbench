#ifndef DAPYNODECONFIGDIALOG_H
#define DAPYNODECONFIGDIALOG_H
#include "DAGuiAPI.h"
#include "DAPybind11InQt.h"
#include <QDialog>

class QPushButton;
class QDialogButtonBox;

namespace DA
{

class DAPyNodeWidget;
class DAPyNodeProxy;

/**
 * @brief Python节点参数配置对话框
 *
 * 包装DAPyNodeWidget的QDialog，提供OK/Cancel/Apply按钮。
 * 双击节点时弹出此对话框进行参数配置。
 *
 * 按钮行为：
 * - OK: 应用参数到Python节点并关闭对话框
 * - Cancel: 放弃更改并关闭对话框
 * - Apply: 应用参数到Python节点但不关闭对话框
 *
 * @code
 * // 显示节点配置对话框
 * auto dialog = new DA::DAPyNodeConfigDialog(proxy, parent);
 * if (dialog->exec() == QDialog::Accepted) {
 *     // 参数已应用
 * }
 * @endcode
 *
 * @see DAPyNodeWidget DAPyNodeGraphicsItem
 */
class DAGUI_API DAPyNodeConfigDialog : public QDialog
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAPyNodeConfigDialog)

public:
    // 构造函数
    explicit DAPyNodeConfigDialog(QWidget* parent = nullptr);

    // 构造函数（带节点代理）
    explicit DAPyNodeConfigDialog(DAPyNodeProxy* proxy, QWidget* parent = nullptr);

    ~DAPyNodeConfigDialog();

    // 设置节点代理
    void setNodeProxy(DAPyNodeProxy* proxy);

    // 获取节点代理
    DAPyNodeProxy* getNodeProxy() const;

    // 获取参数配置组件
    DAPyNodeWidget* getNodeWidget() const;

    // 应用参数到Python节点
    bool applyParameters();

    // 获取当前参数值
    pybind11::dict getParameterValues() const;

protected Q_SLOTS:
    // OK按钮点击
    void onOkClicked();

    // Cancel按钮点击
    void onCancelClicked();

    // Apply按钮点击
    void onApplyClicked();

    // 参数修改状态变化
    void onParametersModified();

protected:
    // 初始化UI
    void initUI();

    // 初始化连接
    void initConnect();
};

} // namespace DA

#endif // DAPYNODECONFIGDIALOG_H