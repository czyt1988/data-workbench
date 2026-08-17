#ifndef DAPROVIDEREDITDIALOG_H
#define DAPROVIDEREDITDIALOG_H
#include <QDialog>
#include <QJsonObject>
#include <QNetworkAccessManager>

class QLineEdit;
class QTableWidget;
class QPushButton;
class QLabel;

namespace DA
{
/**
 * @brief 新增/修改 Provider 对话框
 *
 * 输入 Name / Base URL / API Key，并通过 models 表格管理模型（模型名 | 上下文大小 | 最大输出 token）。
 * 「获取可用模型」按 base_url GET /models 拉取，弹 DAModelFetchDialog 勾选添加；
 * 「+」新增模型弹 DAModelEditDialog；表格双击修改模型；「-」删除选中模型。
 * OK 返回 provider 对象 {name, base_url, api_key, models:[{id,context_window,max_output_tokens}]}。
 */
class DAProviderEditDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造
     * @param provider 初始 provider（修改模式预填，新增传空对象）
     * @param existingNames 已存在的 provider 名称列表（重名校验，修改时排除自身）
     * @param oldName 修改模式下的旧名称（排除重名校验），新增传空
     * @param parent 父窗口
     */
    DAProviderEditDialog(const QJsonObject& provider, const QStringList& existingNames,
                         const QString& oldName = QString(), QWidget* parent = nullptr);

    /// 获取结果 provider 对象
    QJsonObject getProvider() const;

private Q_SLOTS:
    void onFetchModels();
    void onAddModel();
    void onRemoveModel();
    void onModelDoubleClicked(int row, int column);
    void onSelectionChanged();
    void onOkClicked();

private:
    void buildUi();
    void loadProvider(const QJsonObject& provider);
    bool validate();
    QStringList collectModelIds(int excludeRow = -1) const;
    void appendModelRow(const QString& id, int ctxWin, int maxOut);

    QLineEdit* m_nameEdit { nullptr };
    QLineEdit* m_baseUrlEdit { nullptr };
    QLineEdit* m_apiKeyEdit { nullptr };
    QTableWidget* m_modelTable { nullptr };
    QPushButton* m_fetchBtn { nullptr };
    QPushButton* m_addModelBtn { nullptr };
    QPushButton* m_removeModelBtn { nullptr };
    QLabel* m_statusLabel { nullptr };
    QNetworkAccessManager* m_netMgr { nullptr };
    QStringList m_existingNames;
    QString m_oldName;
};
}  // namespace DA

#endif  // DAPROVIDEREDITDIALOG_H
