#include "DAPyNodeConfigDialog.h"
#include "DAPyNodeWidget.h"
#include "DAPyNodeProxy.h"
#include "DAPybind11InQt.h"
#include "DAPyGILGuard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

namespace DA
{

class DAPyNodeConfigDialog::PrivateData
{
    DA_DECLARE_PUBLIC(DAPyNodeConfigDialog)
public:
    PrivateData(DAPyNodeConfigDialog* p);

public:
    DAPyNodeWidget* mNodeWidget { nullptr };        ///< 参数配置组件
    DAPyNodeProxy* mNodeProxy { nullptr };          ///< 节点代理
    QDialogButtonBox* mButtonBox { nullptr };       ///< 按钮组
    QPushButton* mOkButton { nullptr };             ///< OK按钮
    QPushButton* mCancelButton { nullptr };         ///< Cancel按钮
    QPushButton* mApplyButton { nullptr };          ///< Apply按钮
    bool mIsModified { false };                     ///< 是否已修改
};

DAPyNodeConfigDialog::PrivateData::PrivateData(DAPyNodeConfigDialog* p) : q_ptr(p)
{
}

/**
 * @brief 构造函数
 * @param[in] parent 父控件
 */
DAPyNodeConfigDialog::DAPyNodeConfigDialog(QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT
{
    initUI();
    initConnect();
}

/**
 * @brief 构造函数（带节点代理）
 * @param[in] proxy Python节点代理
 * @param[in] parent 父控件
 */
DAPyNodeConfigDialog::DAPyNodeConfigDialog(DAPyNodeProxy* proxy, QWidget* parent)
    : QDialog(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    d->mNodeProxy = proxy;
    initUI();
    initConnect();
    setNodeProxy(proxy);
}

/**
 * @brief 析构函数
 */
DAPyNodeConfigDialog::~DAPyNodeConfigDialog()
{
}

/**
 * @brief 初始化UI
 */
void DAPyNodeConfigDialog::initUI()
{
    DA_D(d);
    setWindowTitle(tr("节点参数配置"));
    setMinimumSize(400, 300);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 节点名称标签
    QLabel* titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; margin: 10px;");
    if (d->mNodeProxy) {
        titleLabel->setText(d->mNodeProxy->getNodeName());
    } else {
        titleLabel->setText(tr("节点配置"));
    }
    mainLayout->addWidget(titleLabel);

    // 创建参数配置组件
    d->mNodeWidget = new DAPyNodeWidget(d->mNodeProxy, this);
    mainLayout->addWidget(d->mNodeWidget, 1);

    // 创建按钮组
    d->mButtonBox = new QDialogButtonBox(this);
    d->mOkButton = d->mButtonBox->addButton(QDialogButtonBox::Ok);
    d->mCancelButton = d->mButtonBox->addButton(QDialogButtonBox::Cancel);
    d->mApplyButton = d->mButtonBox->addButton(QDialogButtonBox::Apply);

    d->mOkButton->setText(tr("确定"));
    d->mCancelButton->setText(tr("取消"));
    d->mApplyButton->setText(tr("应用"));

    // 初始状态下Apply按钮禁用（无修改时）
    d->mApplyButton->setEnabled(false);

    mainLayout->addWidget(d->mButtonBox);
}

/**
 * @brief 初始化信号连接
 */
void DAPyNodeConfigDialog::initConnect()
{
    DA_D(d);
    // 按钮连接
    connect(d->mOkButton, &QPushButton::clicked, this, &DAPyNodeConfigDialog::onOkClicked);
    connect(d->mCancelButton, &QPushButton::clicked, this, &DAPyNodeConfigDialog::onCancelClicked);
    connect(d->mApplyButton, &QPushButton::clicked, this, &DAPyNodeConfigDialog::onApplyClicked);

    // 参数修改信号
    if (d->mNodeWidget) {
        connect(d->mNodeWidget, &DAPyNodeWidget::parametersModified, this, &DAPyNodeConfigDialog::onParametersModified);
    }
}

/**
 * @brief 设置节点代理
 * @param[in] proxy Python节点代理
 */
void DAPyNodeConfigDialog::setNodeProxy(DAPyNodeProxy* proxy)
{
    DA_D(d);
    d->mNodeProxy = proxy;
    if (d->mNodeWidget) {
        d->mNodeWidget->setNodeProxy(proxy);
    }

    // 更新窗口标题
    if (proxy) {
        setWindowTitle(tr("节点参数配置 - %1").arg(proxy->getNodeName()));
    } else {
        setWindowTitle(tr("节点参数配置"));
    }

    d->mIsModified = false;
    if (d->mApplyButton) {
        d->mApplyButton->setEnabled(false);
    }
}

/**
 * @brief 获取节点代理
 * @return Python节点代理指针
 */
DAPyNodeProxy* DAPyNodeConfigDialog::getNodeProxy() const
{
    DA_DC(d);
    return d->mNodeProxy;
}

/**
 * @brief 获取参数配置组件
 * @return DAPyNodeWidget指针
 */
DAPyNodeWidget* DAPyNodeConfigDialog::getNodeWidget() const
{
    DA_DC(d);
    return d->mNodeWidget;
}

/**
 * @brief 应用参数到Python节点
 *
 * 通过pybind11调用Python节点的set_parameter方法，
 * 使用DAPyGILGuard确保GIL正确获取和释放。
 *
 * @return 应用成功返回true，失败返回false
 */
bool DAPyNodeConfigDialog::applyParameters()
{
    DA_D(d);
    if (!d->mNodeProxy || !d->mNodeWidget) {
        qWarning() << "DAPyNodeConfigDialog::applyParameters: Node proxy or widget is null";
        return false;
    }

    pybind11::dict paramValues = d->mNodeWidget->getParameterValuesAsDict();
    if (paramValues.empty()) {
        // 无参数，视为成功
        d->mIsModified = false;
        d->mApplyButton->setEnabled(false);
        return true;
    }

    // 获取Python节点引用
    pybind11::object pyNode = d->mNodeProxy->getPyNodeRef();
    if (!pyNode) {
        qWarning() << "DAPyNodeConfigDialog::applyParameters: Python node reference is invalid";
        return false;
    }

    DAPyGILGuard gil;
    if (!gil.isAcquired()) {
        qWarning() << "DAPyNodeConfigDialog::applyParameters: Failed to acquire GIL";
        return false;
    }

    try {
        // 检查Python节点是否有set_config方法
        if (pybind11::hasattr(pyNode, "set_config")) {
            // 调用set_config方法
            pyNode.attr("set_config")(paramValues);
        } else if (pybind11::hasattr(pyNode, "set_parameter")) {
            // 逐个设置参数
            for (auto item : paramValues) {
                std::string key = pybind11::str(item.first);
                pybind11::object value = pybind11::cast<pybind11::object>(item.second);
                pyNode.attr("set_parameter")(key, value);
            }
        } else {
            qWarning() << "DAPyNodeConfigDialog::applyParameters: Python node has no set_config or set_parameter method";
            return false;
        }

        d->mIsModified = false;
        d->mApplyButton->setEnabled(false);
        return true;

    } catch (const pybind11::error_already_set& e) {
        qWarning() << "DAPyNodeConfigDialog::applyParameters: Python error:" << e.what();
        return false;
    } catch (const std::exception& e) {
        qWarning() << "DAPyNodeConfigDialog::applyParameters: Exception:" << e.what();
        return false;
    }
}

/**
 * @brief 获取当前参数值
 * @return Python字典格式的参数值
 */
pybind11::dict DAPyNodeConfigDialog::getParameterValues() const
{
    DA_DC(d);
    if (d->mNodeWidget) {
        return d->mNodeWidget->getParameterValuesAsDict();
    }
    return pybind11::dict();
}

/**
 * @brief OK按钮点击处理
 *
 * 应用参数到Python节点并关闭对话框。
 */
void DAPyNodeConfigDialog::onOkClicked()
{
    DA_D(d);
    if (d->mIsModified) {
        if (!applyParameters()) {
            // 应用失败，询问是否继续关闭
            return;
        }
    }
    accept();
}

/**
 * @brief Cancel按钮点击处理
 *
 * 放弃更改并关闭对话框。
 */
void DAPyNodeConfigDialog::onCancelClicked()
{
    reject();
}

/**
 * @brief Apply按钮点击处理
 *
 * 应用参数到Python节点但不关闭对话框。
 */
void DAPyNodeConfigDialog::onApplyClicked()
{
    applyParameters();
}

/**
 * @brief 参数修改状态变化处理
 */
void DAPyNodeConfigDialog::onParametersModified()
{
    DA_D(d);
    d->mIsModified = true;
    if (d->mApplyButton) {
        d->mApplyButton->setEnabled(true);
    }
}

} // namespace DA