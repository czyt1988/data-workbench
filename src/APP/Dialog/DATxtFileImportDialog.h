#ifndef DATXTFILEIMPORTDIALOG_H
#define DATXTFILEIMPORTDIALOG_H

#include <QDialog>
#include "DATextReadWriter.h"
namespace Ui
{
class DATxtFileImportDialog;
}

namespace DA
{

class DAPyDataFrameTableModule;

class DATxtFileImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DATxtFileImportDialog(QWidget* parent = nullptr);
    ~DATxtFileImportDialog();
    // 获取路径
    QString getTextFilePath() const;
    void setTextFilePath(const QString& p);
    // 获取设置内容
    QVariantMap getSetting();
public slots:
    // 刷新
    void refresh();
private slots:
    // 文件路径选择选择了路径
    void onFilePathEditSelectedPath(const QString& p);
    // 读取文本回调
    void onTextReadComplete(const QString& txt, bool isReadAll);
    // 文本读取完成回调
    void onTextReadFinished(int code);
    // 跳过脚注数值变化槽函数，这个是为了和max——row互斥
    void onSpinBoxSkipFooterValueChanged(int v);

protected:
    void changeEvent(QEvent* e);
    // 读取文本
    void readTextFile(const QString& p);

private:
    Ui::DATxtFileImportDialog* ui;
#if DA_ENABLE_PYTHON
    DAPyDataFrameTableModule* mModule { nullptr };
#endif
};
}

#endif  // DATXTFILEIMPORTDIALOG_H
