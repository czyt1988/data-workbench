#ifndef DAMODELFETCHDIALOG_H
#define DAMODELFETCHDIALOG_H
#include <QDialog>
#include <QStringList>

class QListWidget;
class QPushButton;

namespace DA
{
/**
 * @brief 模型获取结果对话框
 *
 * 「获取可用模型」GET /models 成功后弹出，列表项为可勾选 checkbox（默认全选），
 * 提供「全选」「全不选」按钮。OK 返回勾选的模型 id 列表。
 */
class DAModelFetchDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造
     * @param modelIds 获取到的模型 id 列表
     * @param parent 父窗口
     */
    explicit DAModelFetchDialog(const QStringList& modelIds, QWidget* parent = nullptr);

    /// 返回勾选的模型 id 列表
    QStringList getSelectedModelIds() const;

private Q_SLOTS:
    void onSelectAll();
    void onDeselectAll();

private:
    void buildUi();

    QListWidget* mList { nullptr };
};
}  // namespace DA

#endif  // DAMODELFETCHDIALOG_H
