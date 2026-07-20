#ifndef DADIALOGDATAFRAMECOLUMNDESCRIBE_H
#define DADIALOGDATAFRAMECOLUMNDESCRIBE_H
#include <QDialog>
#include <QPair>
#include <QList>
#include <QString>
#include "DAGuiAPI.h"

namespace Ui
{
class DADialogDataframeColumnDescribe;
}

namespace DA
{
/**
 * @brief 显示列统计信息的对话框
 *
 * 通过 pandas Series.describe() 获取列的统计信息（count、mean、std、min、25%、50%、75%、max 等），
 * 以表格形式展示。
 */
class DAGUI_API DADialogDataframeColumnDescribe : public QDialog
{
    Q_OBJECT
public:
    explicit DADialogDataframeColumnDescribe(QWidget* parent = nullptr);
    ~DADialogDataframeColumnDescribe();
    // 设置列名
    void setColumnName(const QString& name);
    // 设置列的数据类型
    void setColumnDType(const QString& dtype);
    // 设置统计信息，每对为 (统计项名, 值字符串)
    void setStatistics(const QList<QPair<QString, QString>>& stats);
private:
    Ui::DADialogDataframeColumnDescribe* ui;
};
}  // end of namespace DA
#endif  // DADIALOGDATAFRAMECOLUMNDESCRIBE_H
