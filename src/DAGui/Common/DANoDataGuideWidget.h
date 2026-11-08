#ifndef DANODATAGUIDEWIDGET_H
#define DANODATAGUIDEWIDGET_H
#include <QWidget>
#include "DAGuiAPI.h"
class QLabel;
class QPushButton;
namespace DA
{
/**
 * @brief 无数据时的引导页（数据操作 / 绘图操作窗口共用）
 *
 * 软件刚打开、或关闭掉全部数据页/绘图页时，操作窗口显示本页：标题「尚未载入数据」、
 * 一句说明，以及「打开已有工程」「导入数据」两个入口。版式对齐信号分析页的空状态。
 *
 * 本页只发信号，具体打开工程/导入数据流程由宿主（APP 层）接线，避免 DAGui 反向依赖 APP。
 */
class DAGUI_API DANoDataGuideWidget : public QWidget
{
    Q_OBJECT
public:
    // 引导页所属的操作窗口（决定提示文案）
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
    // 重译界面文本
    void retranslateUi();

Q_SIGNALS:
    // 点击「打开已有工程」
    void openProjectRequested();
    // 点击「导入数据」
    void importDataRequested();

protected:
    void changeEvent(QEvent* e) override;

private:
    void buildUI();

private:
    Role mRole { DataOperateRole };
    QLabel* mTitleLabel { nullptr };
    QLabel* mHintLabel { nullptr };
    QPushButton* mOpenProjectButton { nullptr };
    QPushButton* mImportDataButton { nullptr };
};
}  // namespace DA
#endif  // DANODATAGUIDEWIDGET_H
