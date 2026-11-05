#ifndef DASIGNALCHARTPANEL_H
#define DASIGNALCHARTPANEL_H
#include <QWidget>
#include <QString>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace DA
{
class DAChartWidget;
class DAChartDataInterface;

/**
 * @brief 信号视图中的单图容器（方案 §3.2）
 *
 * 结构：标题行（标题 + 可选右侧工具控件）+ DAChartWidget。
 * 不使用 DAFigureWidget（滚动区 + 多轴容器，本页布局固定，不需要）。
 */
class DASignalChartPanel : public QWidget
{
    Q_OBJECT
public:
    explicit DASignalChartPanel(const QString& title, QWidget* parent = nullptr);
    ~DASignalChartPanel() override;

    // 图表控件
    DAChartWidget* chart() const;
    // 标题
    void setTitle(const QString& title);
    QString title() const;
    // 在标题行右侧插入工具控件（例如幅值模式下拉）
    void addTitleBarWidget(QWidget* w);
    // 底部工具区（例如时间滑块）
    void setBottomWidget(QWidget* w);

protected:
    void changeEvent(QEvent* e) override;

private:
    void retranslateUi();

private:
    QString mTitle;
    QLabel* mTitleLabel { nullptr };
    QHBoxLayout* mTitleLayout { nullptr };
    QVBoxLayout* mLayout { nullptr };
    DAChartWidget* mChart { nullptr };
};
}  // namespace DA

#endif  // DASIGNALCHARTPANEL_H
