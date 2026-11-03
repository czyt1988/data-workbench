#ifndef DADIALOGFINDREPLACE_H
#define DADIALOGFINDREPLACE_H
#include <QDialog>
#include "DAGuiAPI.h"

class QTimer;
class QLineEdit;
class QLabel;
class QCheckBox;
class QComboBox;
class QWidget;

namespace Ui
{
class DADialogFindReplace;
}
namespace DA
{
/**
 * @brief 非模态浮动查找/替换条（002 T1.4，R17）
 *
 * Ctrl+F 唤出并聚焦输入框，Ctrl+H 展开替换行，Esc 关闭（页面据 closed 清查找高亮）。
 * 行为对齐 Excel：输入即查找（去抖后发 searchRequested）、上一个/下一个导航、
 * 替换/全部替换、选项行（区分大小写/正则/整格匹配/范围=全表或当前选区）。
 *
 * 三条硬约束的分工（§4.2）：
 * - ①输入去抖在本类（QTimer singleShot 350ms restart，≥300ms 要求）；
 * - ②请求序号丢弃过期结果在页面扫描侧（DADataOperateOfDataFrameWidget::runFindScan）；
 * - ③命中上限截断在算子侧（da_search_data max_hits），本类只负责截断提示文案。
 *
 * 选项/范围变化同样经去抖后触发重扫；回车=下一个（无命中时页面先执行查找）。
 * 计数标签显示「当前第 n 条 / 共 m 条」，截断时附真实总数，无效正则显示错误提示。
 */
class DAGUI_API DADialogFindReplace : public QDialog
{
    Q_OBJECT
public:
    // 查找/替换参数快照（页面按此执行扫描与替换）
    struct FindOptions
    {
        QString keyword;                 ///< 查找关键字
        QString replaceText;             ///< 替换为
        bool caseSensitive { false };    ///< 区分大小写
        bool useRegex { false };         ///< 正则表达式
        bool wholeCell { false };        ///< 整格匹配
        bool selectionOnly { false };    ///< 范围=当前选区（false=全表）
    };
    // 去抖间隔（毫秒，硬约束①：≥300ms）
    static int debounceIntervalMs();

    explicit DADialogFindReplace(QWidget* parent = nullptr);
    ~DADialogFindReplace();

    // 唤出并聚焦输入框；expandReplace=true 展开替换行（Ctrl+H 语义）
    void activate(bool expandReplace);
    // 设置查找关键字（右键入口预填当前单元格内容用；经 textChanged 走去抖扫描）
    void setFindText(const QString& text);
    // 查找输入框当前文本（测试与外部查询用）
    QString findText() const;
    // 当前参数快照
    FindOptions options() const;
    // 更新计数显示：current=当前命中序号（1 起，<=0 表示无当前项），shown=已高亮条数，
    // totalHits=真实命中总数（truncated=true 时 > shown）
    void setHitCount(int current, int shown, int totalHits, bool truncated);
    // 扫描失败提示（如无效正则）；false 恢复正常计数显示
    void setSearchFailed(bool failed);
    // 立即触发一次查找（跳过去抖：回车/按钮等显式动作）
    void requestSearchNow();
    // 重启去抖定时（外部输入变化用：如选区范围模式下选区变化触发的延迟重扫）
    void restartDebounce();

Q_SIGNALS:
    // 去抖到期或显式触发的查找请求
    void searchRequested();
    void nextRequested();
    void previousRequested();
    void replaceRequested();      ///< 替换当前命中
    void replaceAllRequested();   ///< 全部替换
    void closed();                ///< 关闭（页面据此清查找高亮）

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
    void onInputChanged();

private:
    Ui::DADialogFindReplace* ui;
    QTimer* mDebounceTimer { nullptr };  ///< 输入去抖（singleShot + restart，硬约束①）
};
}  // end of namespace DA
#endif  // DADIALOGFINDREPLACE_H
