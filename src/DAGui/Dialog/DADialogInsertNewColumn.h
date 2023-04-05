#ifndef DADIALOGINSERTNEWCOLUMN_H
#define DADIALOGINSERTNEWCOLUMN_H

#include <QDialog>
#include "DAGuiAPI.h"
#include "numpy/DAPyDType.h"
namespace Ui
{
class DADialogInsertNewColumn;
}
namespace DA
{
class DAGUI_API DADialogInsertNewColumn : public QDialog
{
    Q_OBJECT

public:
    explicit DADialogInsertNewColumn(QWidget* parent = nullptr);
    ~DADialogInsertNewColumn();
    DAPyDType getDType() const;
    //是否是填充模式
    bool isFullValueMode() const;
    //是否是范围模式
    bool isRangeMode() const;
    //获取列名
    QString getName() const;
    //获取默认值
    QVariant getDefaultValue() const;
    //获取start值
    QVariant getStartValue() const;
    //获取stop值
    QVariant getStopValue() const;

private:
    //从字符串获取值
    QVariant fromString(const QString& str, const DAPyDType& dt) const;
private slots:
    //当前类型变化
    void onCurrentDtypeChanged(const DAPyDType& dt);

private:
    Ui::DADialogInsertNewColumn* ui;
};
}  // end of namespace DA
#endif  // DADIALOGINSERTNEWCOLUMN_H
