#ifndef DASIGNALANALYSISPAGE_H
#define DASIGNALANALYSISPAGE_H
#include <QWidget>

class QLabel;
class QTabBar;
class QStackedWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;

namespace DA
{
/**
 * @brief 信号分析主页面（中央停靠区页面，与绘图/表格/工作流并列）
 *
 * 结构（方案 D11）：页面内每个信号一个标签页。M2 骨架阶段只提供空状态页；
 * 信号视图容器（QTabBar + QStackedWidget）在此预置，供后续里程碑填充。
 *
 * 空状态不是「页面不存在」：页面始终在主工作区，只有内容区随信号数量切换
 * 空状态页 / 信号视图页。
 */
class DASignalAnalysisPage : public QWidget
{
    Q_OBJECT
public:
    explicit DASignalAnalysisPage(QWidget* par = nullptr);
    ~DASignalAnalysisPage() override;

    // 信号视图容器（后续里程碑使用）
    QTabBar* signalTabBar() const;
    QStackedWidget* signalStack() const;

    // 空状态下点击导入按钮时，由外部（UI 层）接管实际导入流程
    Q_SIGNALS:
    void requestImportAudio();      // 请求导入音频文件
    void requestImportDataFrame();  // 请求从数据帧导入
    void requestImportVideo();      // 请求导入视频（抽取音频轨）

public:
    // 重译界面文本（插件的 retranslate 与页面自身的 LanguageChange 都调用）
    void retranslateUi();

protected:
    void changeEvent(QEvent* e) override;

private:
    void buildUI();
    void updateEmptyState();

private:
    QStackedWidget* mRootStack { nullptr };   ///< 空状态页 / 信号视图页
    QWidget* mEmptyPage { nullptr };          ///< 空状态页
    QWidget* mContentPage { nullptr };        ///< 信号视图页
    QTabBar* mTabBar { nullptr };             ///< 信号标签栏
    QStackedWidget* mSignalStack { nullptr }; ///< 每个信号一个视图
    QLabel* mEmptyTitle { nullptr };
    QLabel* mEmptyHint { nullptr };
    QPushButton* mBtnImportAudio { nullptr };
    QPushButton* mBtnImportDataFrame { nullptr };
    QPushButton* mBtnImportVideo { nullptr };
};
}  // namespace DA

#endif  // DASIGNALANALYSISPAGE_H
