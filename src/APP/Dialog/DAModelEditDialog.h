#ifndef DAMODELEDITDIALOG_H
#define DAMODELEDITDIALOG_H
#include <QDialog>

class QLineEdit;
class QSpinBox;

namespace DA
{
/**
 * @brief 模型编辑对话框（新增/修改单个模型）
 *
 * 三个字段：模型 id（必填）、上下文大小（默认 256K）、最大输出 token（默认 8192）。
 * 用于 DAProviderEditDialog 的 add（空）与表格双击修改（预填）。
 */
class DAModelEditDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造
     * @param id 初始模型 id（新增传空）
     * @param contextWindow 初始上下文窗口
     * @param maxOutputTokens 初始最大输出 token
     * @param existingIds 已存在的模型 id 列表（用于重名校验，修改时排除自身）
     * @param oldId 修改模式下自身的旧 id（排除重名校验用），新增传空
     * @param parent 父窗口
     */
    DAModelEditDialog(const QString& id, int contextWindow, int maxOutputTokens,
                      const QStringList& existingIds, const QString& oldId = QString(),
                      QWidget* parent = nullptr);

    QString getModelId() const;
    int getContextWindow() const;
    int getMaxOutputTokens() const;

private Q_SLOTS:
    void onOkClicked();

private:
    void buildUi();
    bool validate();

    QLineEdit* m_idEdit { nullptr };
    QSpinBox* m_ctxSpin { nullptr };
    QSpinBox* m_maxOutSpin { nullptr };
    QStringList m_existingIds;
    QString m_oldId;
};
}  // namespace DA

#endif  // DAMODELEDITDIALOG_H
