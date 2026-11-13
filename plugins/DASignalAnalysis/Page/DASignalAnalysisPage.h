#ifndef DASIGNALANALYSISPAGE_H
#define DASIGNALANALYSISPAGE_H
#include <QWidget>
#include <QList>
#include <QHash>
#include "Signal/DASignal.h"

class QLabel;
class QTabBar;
class QStackedWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
namespace DA
{
class DASignalAnalysisView;
}

namespace DA
{
/**
 * @brief 信号分析主页面（中央停靠区页面，与绘图/表格/工作流并列）
 *
 * 结构（方案 D11 修订版）：页面惰性创建（首次打开/导入/工程恢复时才建），
 * 页面内每个信号一个标签页；信号视图容器（QTabBar + QStackedWidget）
 * 与每个信号的分析视图（波形 + 频谱 + 时频）均由本页创建。
 *
 * 空状态只是本页内容区的分支：页面已创建但没有信号时显示导入引导，
 * 有信号时切换到信号视图页；页面本身由 DASignalAnalysisUI::ensurePageCreated 管理。
 */
class DASignalAnalysisPage : public QWidget
{
    Q_OBJECT
public:
    explicit DASignalAnalysisPage(QWidget* par = nullptr);
    ~DASignalAnalysisPage() override;

    // 信号视图容器（标签栏与每信号视图堆叠）
    QTabBar* signalTabBar() const;
    QStackedWidget* signalStack() const;

    // 添加一个信号：新增标签页与完整信号视图（视图登记在 mViews，供结果回填与查找）
    void addSignal(const DA::DASignalPtr& signal);
    // 清空全部信号标签页与视图（工程加载前调用，避免旧工程信号叠加）
    void clearAll();
    // 当前信号（无信号返回空指针）
    DA::DASignalPtr currentSignal() const;
    // 全部信号
    QList< DA::DASignalPtr > allSignals() const;
    // 信号数量
    int signalCount() const;
    // 某信号对应的视图（无对应视图返回 nullptr）
    DA::DASignalAnalysisView* viewForSignal(const DA::DASignal* sig) const;
    // 某信号在标签栏中的索引（未找到返回 -1）
    int indexOfSignal(const DA::DASignal* sig) const;
    // 用新信号替换指定索引的信号（重新指定来源用，评审 U16）：重建视图并通知外部清缓存
    bool replaceSignalAt(int index, const DA::DASignalPtr& signal);

    // 空状态下点击导入按钮时，由外部（UI 层）接管实际导入流程
    Q_SIGNALS:
    void requestImportAudio();      // 请求导入音频文件
    void requestImportDataFrame();  // 请求从数据帧导入
    void requestImportVideo();      // 请求导入视频（抽取音频轨）
    void currentSignalChanged();    // 当前信号标签页切换
    void signalRemoved(const DA::DASignalPtr& signal);  // 某信号标签页已关闭（外部清理相关缓存）

public:
    // 重译界面文本（插件的 retranslate 与页面自身的 LanguageChange 都调用）
    void retranslateUi();

protected:
    void changeEvent(QEvent* e) override;

private:
    void buildUI();
    void updateEmptyState();
    // 关闭指定索引的信号：移除标签页与视图并通知外部
    void removeSignalAt(int index);
    // 同名信号显示名去重：与现有信号重名时追加 " (2)"/" (3)" 序号（BLK-02）
    QString uniqueDisplayName(const QString& name, const DA::DASignal* except) const;

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
    QList< DA::DASignalPtr > mSignals;  ///< 已导入信号（与标签页索引一一对应）
    QHash< const DA::DASignal*, DA::DASignalAnalysisView* > mViews;  ///< 信号 → 视图
};
}  // namespace DA

#endif  // DASIGNALANALYSISPAGE_H
