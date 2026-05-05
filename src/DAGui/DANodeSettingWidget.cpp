#include "DANodeSettingWidget.h"
#include "DAPropertyPanelContainerWidget.h"
#include "DAPyNodeProxy.h"
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QLineEdit>
//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DANodeSettingWidget
//===================================================

/**
 * @brief 构造函数
 *
 * 创建DAPropertyPanelContainerWidget作为主布局，构建属性面板并连接3-hop信号链。
 * @param parent 父控件
 */
DANodeSettingWidget::DANodeSettingWidget(QWidget* parent) : QWidget(parent), mPanel(nullptr), _nodePtr(nullptr)
{
    // 创建DAPropertyPanelContainerWidget并设为自身主布局
    mPanel              = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    // 连接3-hop信号链
    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged, this, &DANodeSettingWidget::onPanelPropertyValueChanged);
    connect(this, &DANodeSettingWidget::propertyValueChanged, this, &DANodeSettingWidget::onPropertyValueChanged);

    buildPropertyPanel();
}

/**
 * @brief 析构函数
 */
DANodeSettingWidget::~DANodeSettingWidget()
{
}

/**
 * @brief 设置节点代理
 *
 * 设置当前关联的DAPyNodeProxy，并刷新面板数据。
 * @param[in] p 节点代理指针
 */
void DANodeSettingWidget::setNode(DAPyNodeProxy* p)
{
    _nodePtr = p;
    updateData();
}

/**
 * @brief 获取当前节点代理
 * @return 当前关联的DAPyNodeProxy指针
 */
DAPyNodeProxy* DANodeSettingWidget::getNode() const
{
    return _nodePtr;
}

/**
 * @brief 获取属性面板指针
 * @return DAPropertyPanelContainerWidget指针
 */
DAPropertyPanelContainerWidget* DANodeSettingWidget::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 刷新面板数据
 *
 * 从DAPyNodeProxy读取节点元数据写入面板属性。
 * 使用QSignalBlocker防止刷新时触发属性变更信号。
 */
void DANodeSettingWidget::updateData()
{
    QSignalBlocker blocker(mPanel);
    DAPyNodeProxy* n = getNode();
    if (n) {
        mPanel->setStringValue(PID_Prototype, n->getQualifiedName());
        mPanel->setStringValue(PID_Group, n->getNodeGroup());
        mPanel->setStringValue(PID_Name, n->getNodeName());
        // 设置限定名和组为只读
        DAPropertyItemWidget* prototypeItem = mPanel->getPropertyItem(PID_Prototype);
        if (prototypeItem) {
            QLineEdit* editor = qobject_cast< QLineEdit* >(prototypeItem->editorWidget());
            if (editor) {
                editor->setReadOnly(true);
            }
        }
        DAPropertyItemWidget* groupItem = mPanel->getPropertyItem(PID_Group);
        if (groupItem) {
            QLineEdit* editor = qobject_cast< QLineEdit* >(groupItem->editorWidget());
            if (editor) {
                editor->setReadOnly(true);
            }
        }
    } else {
        mPanel->setStringValue(PID_Prototype, QString());
        mPanel->setStringValue(PID_Group, QString());
        mPanel->setStringValue(PID_Name, QString());
    }
}

/**
 * @brief 构建属性面板
 *
 * 添加节点元数据属性：
 * - 限定名（只读字符串）
 * - 节点组（只读字符串）
 * - 节点名称（可编辑字符串）
 */
void DANodeSettingWidget::buildPropertyPanel()
{
    auto panel = propertyPanel();

    panel->addGroupLabel(tr("Meta Data"));
    panel->addStringProperty(PID_Prototype, tr("Prototype"));
    panel->addStringProperty(PID_Group, tr("Group"));
    panel->addStringProperty(PID_Name, tr("Name"));
}

/**
 * @brief 转发面板属性值变化信号
 *
 * 第1-hop：将DAPropertyPanelContainerWidget::propertyValueChanged转发为本类的propertyValueChanged信号。
 * @param propertyId 属性ID
 */
void DANodeSettingWidget::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 属性值变化处理
 *
 * 第2-hop：根据属性ID将面板值写回到DAPyNodeProxy。
 * 仅PID_Name为可编辑属性，写回后重新读取节点名称确保同步。
 * @param propertyId 属性ID
 */
void DANodeSettingWidget::onPropertyValueChanged(int propertyId)
{
    DAPyNodeProxy* p = getNode();
    if (!p) {
        return;
    }

    switch (propertyId) {
    case PID_Name: {
        p->setNodeName(mPanel->getStringValue(PID_Name));
        // 设置完成后重新读取节点名称，确保同步
        QSignalBlocker blocker(mPanel);
        mPanel->setStringValue(PID_Name, p->getNodeName());
        break;
    }
    default:
        break;
    }
}