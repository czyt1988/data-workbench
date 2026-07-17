#ifndef DACHARTSERIESPICKERWIDGET_H
#define DACHARTSERIESPICKERWIDGET_H
#include "DAGuiAPI.h"
#include "DAData.h"
#include <QWidget>

class QLineEdit;
class QToolButton;
class QLabel;

namespace Ui
{
class DAChartSeriesPickerWidget;
}

namespace DA
{
class DADataManager;
class DADataManageWidget;
class DADataOperateWidget;

/**
 * @brief 数据序列选择浮动窗口
 *
 * 类似 Excel 绘图时的数据列选择窗口。在 DAChartAddXYSeriesWidget 中
 * 点击添加按钮后弹出，让用户通过点击数据管理树或表格表头来选择一个序列，
 * 以 pandas 提取序列的表达式形式显示（如 data['B']），用户可手动编辑。
 */
class DAGUI_API DAChartSeriesPickerWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 选择目标角色
     */
    enum Role
    {
        RoleX,  ///< 为 X 选择序列
        RoleY   ///< 为 Y 选择序列
    };

    explicit DAChartSeriesPickerWidget(DADataManager* mgr, Role role, QWidget* parent = nullptr);
    ~DAChartSeriesPickerWidget();
    // 获取表达式文本
    QString getExpression() const;
    // 设置表达式文本
    void setExpression(const QString& expr);
    // 解析表达式为 DAData 和 series 名
    bool resolveExpression(DAData& outData, QString& outSeriesName) const;
    // 获取选择目标角色
    Role getRole() const;

public Q_SLOTS:
    // 开始选择：raise 数据窗口，连接点击信号
    void startPick();
    // 结束选择：断开连接，隐藏窗口
    void finishPick();

Q_SIGNALS:
    // 用户确认选择（点"回到添加绘图"且表达式解析成功）
    void seriesConfirmed(const DA::DAData& data, const QString& seriesName);
    // 用户取消（关闭窗口未确认）
    void canceled();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private Q_SLOTS:
    void onDataManageSeriesClicked(const DAData& data, const QString& seriesName);
    void onDataOperateHeaderClicked(int logicalIndex);
    void onButtonReturnClicked();

private:
    void connectDataWidgets();
    void disconnectDataWidgets();
    DADataManageWidget* findDataManageWidget() const;
    DADataOperateWidget* findDataOperateWidget() const;
    QString buildExpression(const DAData& data, const QString& seriesName) const;
    void retranslateUi();
    QString roleLabel() const;
    static void raiseDockWidget(QWidget* w);

private:
    Ui::DAChartSeriesPickerWidget* ui;
    DADataManager* mDataMgr;
    Role mRole;
    bool mPickActive;
    bool mConfirmed;
    QMetaObject::Connection mConnTreeClicked;
    QMetaObject::Connection mConnHeaderClicked;
};
}  // namespace DA
#endif  // DACHARTSERIESPICKERWIDGET_H
