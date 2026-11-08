#ifndef DANODATAGUIDEWIDGET_H
#define DANODATAGUIDEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
class QLabel;
class QPushButton;
namespace DA
{
/**
 * @brief 操作窗口无内容时的引导页（数据操作 / 绘图操作窗口共用）
 *
 * 引导内容随「数据管理器里是否已有数据」变化：
 * - 没有数据：标题「尚未载入数据」，提供「打开已有工程」「导入数据」两个入口（两个窗口一致）；
 * - 已有数据、数据操作窗口：提示「双击数据管理区的数据来打开数据表」；
 * - 已有数据、绘图操作窗口：提供「添加绘图」入口。
 *
 * 本页只发信号，打开工程/导入数据/添加绘图都由宿主（APP 层）接线，避免 DAGui 反向依赖 APP。
 */
class DAGUI_API DANoDataGuideWidget : public QWidget
{
    Q_OBJECT
public:
    // 引导页所属的操作窗口（决定提示文案与可用入口）
    enum Role
    {
        DataOperateRole,  ///< 数据操作窗口
        ChartOperateRole  ///< 绘图操作窗口
    };
public:
    explicit DANoDataGuideWidget(Role role, QWidget* par = nullptr);
    ~DANoDataGuideWidget() override;

    // 所属窗口角色
    Role role() const;
    // 数据管理器里是否已有数据
    bool isDataAvailable() const;
    // 设置数据管理器是否已有数据（驱动引导内容切换）
    void setDataAvailable(bool available);
    // 重译界面文本
    void retranslateUi();

Q_SIGNALS:
    // 点击「打开已有工程」
    void openProjectRequested();
    // 点击「导入数据」
    void importDataRequested();
    // 已有数据但还没有绘图时点击「添加绘图」
    void addFigureRequested();

protected:
    void changeEvent(QEvent* e) override;

private:
    void buildUI();
    // 按「角色 + 数据是否可用」刷新标题、说明与按钮显隐
    void updateContent();

private:
    Role mRole { DataOperateRole };
    bool mDataAvailable { false };
    QLabel* mTitleLabel { nullptr };
    QLabel* mHintLabel { nullptr };
    QPushButton* mOpenProjectButton { nullptr };
    QPushButton* mImportDataButton { nullptr };
    QPushButton* mAddFigureButton { nullptr };
};
}  // namespace DA
#endif  // DANODATAGUIDEWIDGET_H
