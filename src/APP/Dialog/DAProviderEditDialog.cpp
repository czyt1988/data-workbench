// DAProviderEditDialog.cpp
#include "DAProviderEditDialog.h"
#include "DAModelEditDialog.h"
#include "DAModelFetchDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QUrl>
#include <algorithm>

namespace DA
{

/**
 * @brief 构造
 * @param provider 初始 provider
 * @param existingNames 已存在的 provider 名称列表
 * @param oldName 修改模式下的旧名称
 * @param parent 父窗口
 */
DAProviderEditDialog::DAProviderEditDialog(const QJsonObject& provider, const QStringList& existingNames,
                                           const QString& oldName, QWidget* parent)
    : QDialog(parent)
    , m_existingNames(existingNames)
    , m_oldName(oldName)
{
    m_netMgr = new QNetworkAccessManager(this);
    buildUi();
    loadProvider(provider);
}

/** @brief 构建界面 */
void DAProviderEditDialog::buildUi()
{
    setWindowTitle(tr("Provider"));  // cn:供应商
    setMinimumSize(520, 460);
    QVBoxLayout* root = new QVBoxLayout(this);

    QFormLayout* form = new QFormLayout();
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("provider name, e.g. OpenAI"));  // cn:供应商名称，如 OpenAI
    m_baseUrlEdit = new QLineEdit(this);
    m_baseUrlEdit->setPlaceholderText(tr("https://api.openai.com/v1"));  // cn:https://api.openai.com/v1
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Name"), m_nameEdit);          // cn:名称
    form->addRow(tr("Base URL"), m_baseUrlEdit);   // cn:基础地址
    form->addRow(tr("API Key"), m_apiKeyEdit);     // cn:API 密钥
    root->addLayout(form);

    // models 标签 + 按钮 行
    QHBoxLayout* modelBtnRow = new QHBoxLayout();
    modelBtnRow->addWidget(new QLabel(tr("Models"), this));  // cn:模型
    m_fetchBtn = new QPushButton(tr("Fetch Available Models"), this);  // cn:获取可用模型
    m_addModelBtn = new QPushButton(tr("+ Add Model"), this);          // cn:+ 新增模型
    m_removeModelBtn = new QPushButton(tr("- Remove"), this);          // cn:- 删除
    m_removeModelBtn->setEnabled(false);
    modelBtnRow->addStretch();
    modelBtnRow->addWidget(m_fetchBtn);
    modelBtnRow->addWidget(m_addModelBtn);
    modelBtnRow->addWidget(m_removeModelBtn);
    root->addLayout(modelBtnRow);

    // 模型表格
    m_modelTable = new QTableWidget(0, 3, this);
    m_modelTable->setHorizontalHeaderLabels({
        tr("Model Name"),        // cn:模型名
        tr("Context Size"),      // cn:上下文大小
        tr("Max Output Tokens")  // cn:最大输出 token
    });
    m_modelTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_modelTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_modelTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_modelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_modelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_modelTable, 1);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #515151;"));
    root->addWidget(m_statusLabel);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bb->button(QDialogButtonBox::Ok)->setText(tr("OK"));        // cn:确定
    bb->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));  // cn:取消
    root->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &DAProviderEditDialog::onOkClicked);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_fetchBtn, &QPushButton::clicked, this, &DAProviderEditDialog::onFetchModels);
    connect(m_addModelBtn, &QPushButton::clicked, this, &DAProviderEditDialog::onAddModel);
    connect(m_removeModelBtn, &QPushButton::clicked, this, &DAProviderEditDialog::onRemoveModel);
    connect(m_modelTable, &QTableWidget::cellDoubleClicked, this, &DAProviderEditDialog::onModelDoubleClicked);
    connect(m_modelTable, &QTableWidget::itemSelectionChanged, this, &DAProviderEditDialog::onSelectionChanged);
}

/** @brief 从 provider 对象载入表单与模型表格 */
void DAProviderEditDialog::loadProvider(const QJsonObject& provider)
{
    m_nameEdit->setText(provider.value("name").toString());
    m_baseUrlEdit->setText(provider.value("base_url").toString());
    m_apiKeyEdit->setText(provider.value("api_key").toString());
    const QJsonArray models = provider.value("models").toArray();
    for (const QJsonValue& mv : models) {
        QJsonObject mo = mv.toObject();
        if (mo.isEmpty() && mv.isString()) {
            mo["id"] = mv.toString();
        }
        appendModelRow(mo.value("id").toString(),
                       mo.value("context_window").toInt(262144),
                       mo.value("max_output_tokens").toInt(8192));
    }
}

/** @brief 追加一行模型 */
void DAProviderEditDialog::appendModelRow(const QString& id, int ctxWin, int maxOut)
{
    int row = m_modelTable->rowCount();
    m_modelTable->insertRow(row);
    m_modelTable->setItem(row, 0, new QTableWidgetItem(id));
    m_modelTable->setItem(row, 1, new QTableWidgetItem(QString::number(ctxWin)));
    m_modelTable->setItem(row, 2, new QTableWidgetItem(QString::number(maxOut)));
}

/** @brief 收集现有模型 id（excludeRow 排除某行，用于修改时排除自身） */
QStringList DAProviderEditDialog::collectModelIds(int excludeRow) const
{
    QStringList ids;
    for (int r = 0; r < m_modelTable->rowCount(); ++r) {
        if (r == excludeRow) continue;
        if (QTableWidgetItem* it = m_modelTable->item(r, 0)) {
            ids.append(it->text());
        }
    }
    return ids;
}

/** @brief 获取可用模型：GET {base_url}/models，成功后弹 DAModelFetchDialog 勾选添加 */
void DAProviderEditDialog::onFetchModels()
{
    QString baseUrl = m_baseUrlEdit->text().trimmed();
    QString apiKey = m_apiKeyEdit->text().trimmed();
    if (baseUrl.isEmpty()) {
        m_statusLabel->setStyleSheet("color: #CE6043;");
        m_statusLabel->setText(tr("Base URL is required to fetch models"));  // cn:获取模型需要先填写基础地址
        return;
    }
    while (baseUrl.endsWith('/')) baseUrl.chop(1);

    QNetworkRequest request(QUrl(baseUrl + "/models"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    }

    m_fetchBtn->setEnabled(false);
    m_statusLabel->setStyleSheet("color: #515151;");
    m_statusLabel->setText(tr("Fetching..."));  // cn:获取中...
    QNetworkReply* reply = m_netMgr->get(request);
    QTimer::singleShot(15000, reply, [reply]() {
        if (reply->isRunning()) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_fetchBtn->setEnabled(true);
        if (reply->error() != QNetworkReply::NoError) {
            m_statusLabel->setStyleSheet("color: #CE6043;");
            m_statusLabel->setText(tr("✗ Fetch failed: %1").arg(reply->errorString()));  // cn:✗ 获取失败: %1
            reply->deleteLater();
            return;
        }
        // OpenAI 兼容接口返回 {"data":[{"id":"..."},...]}
        QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
        QJsonArray data = resp.value("data").toArray();
        QStringList ids;
        for (const QJsonValue& v : data) {
            QString id = v.toObject().value("id").toString();
            if (!id.isEmpty()) ids.append(id);
        }
        reply->deleteLater();
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
        if (ids.isEmpty()) {
            m_statusLabel->setStyleSheet("color: #CE6043;");
            m_statusLabel->setText(tr("✗ No models returned"));  // cn:✗ 未返回任何模型
            return;
        }
        m_statusLabel->setText(tr("✓ %1 models fetched").arg(ids.size()));  // cn:✓ 获取到 %1 个模型
        // 弹出勾选对话框，默认全选
        DAModelFetchDialog dlg(ids, this);
        if (dlg.exec() == QDialog::Accepted) {
            QStringList selected = dlg.getSelectedModelIds();
            QStringList existing = collectModelIds();
            int added = 0;
            for (const QString& id : selected) {
                if (existing.contains(id)) continue;  // 去重
                appendModelRow(id, 262144, 8192);
                existing.append(id);
                ++added;
            }
            m_statusLabel->setStyleSheet("color: #669E8B;");
            m_statusLabel->setText(tr("Added %1 models").arg(added));  // cn:已添加 %1 个模型
        }
    });
}

/** @brief 新增模型：弹 DAModelEditDialog（空）追加行 */
void DAProviderEditDialog::onAddModel()
{
    QStringList existing = collectModelIds();
    DAModelEditDialog dlg(QString(), 262144, 8192, existing, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        appendModelRow(dlg.getModelId(), dlg.getContextWindow(), dlg.getMaxOutputTokens());
    }
}

/** @brief 删除选中模型行 */
void DAProviderEditDialog::onRemoveModel()
{
    int row = m_modelTable->currentRow();
    if (row >= 0) {
        m_modelTable->removeRow(row);
    }
}

/** @brief 表格选择变化：仅当有选中行时启用 Remove */
void DAProviderEditDialog::onSelectionChanged()
{
    m_removeModelBtn->setEnabled(m_modelTable->currentRow() >= 0);
}

/** @brief 表格双击：弹 DAModelEditDialog 修改该行 */
void DAProviderEditDialog::onModelDoubleClicked(int row, int /*column*/)
{
    if (row < 0 || row >= m_modelTable->rowCount()) return;
    QString id = m_modelTable->item(row, 0) ? m_modelTable->item(row, 0)->text() : QString();
    int ctxWin = m_modelTable->item(row, 1) ? m_modelTable->item(row, 1)->text().toInt() : 262144;
    int maxOut = m_modelTable->item(row, 2) ? m_modelTable->item(row, 2)->text().toInt() : 8192;
    QStringList existing = collectModelIds(row);  // 排除自身
    DAModelEditDialog dlg(id, ctxWin, maxOut, existing, id, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (m_modelTable->item(row, 0)) m_modelTable->item(row, 0)->setText(dlg.getModelId());
        if (m_modelTable->item(row, 1)) m_modelTable->item(row, 1)->setText(QString::number(dlg.getContextWindow()));
        if (m_modelTable->item(row, 2)) m_modelTable->item(row, 2)->setText(QString::number(dlg.getMaxOutputTokens()));
    }
}

/** @brief OK：校验通过才 accept */
void DAProviderEditDialog::onOkClicked()
{
    if (validate()) {
        accept();
    }
}

/** @brief 校验：名称非空且不重名，base_url 非空 */
bool DAProviderEditDialog::validate()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_nameEdit->setFocus();
        return false;
    }
    for (const QString& ex : std::as_const(m_existingNames)) {
        if (ex == name && ex != m_oldName) {
            m_statusLabel->setStyleSheet("color: #CE6043;");
            m_statusLabel->setText(tr("Provider name already exists"));  // cn:供应商名称已存在
            m_nameEdit->setFocus();
            return false;
        }
    }
    if (m_baseUrlEdit->text().trimmed().isEmpty()) {
        m_baseUrlEdit->setFocus();
        return false;
    }
    return true;
}

/** @brief 获取结果 provider 对象 */
QJsonObject DAProviderEditDialog::getProvider() const
{
    QJsonObject p;
    p["name"]     = m_nameEdit->text().trimmed();
    p["base_url"] = m_baseUrlEdit->text().trimmed();
    p["api_key"]  = m_apiKeyEdit->text();
    QJsonArray models;
    for (int r = 0; r < m_modelTable->rowCount(); ++r) {
        QJsonObject mo;
        mo["id"] = m_modelTable->item(r, 0) ? m_modelTable->item(r, 0)->text() : QString();
        mo["context_window"] = m_modelTable->item(r, 1) ? m_modelTable->item(r, 1)->text().toInt() : 262144;
        mo["max_output_tokens"] = m_modelTable->item(r, 2) ? m_modelTable->item(r, 2)->text().toInt() : 8192;
        models.append(mo);
    }
    p["models"] = models;
    return p;
}

}  // namespace DA
