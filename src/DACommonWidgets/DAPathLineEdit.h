#ifndef DAPATHLINEEDIT_H
#define DAPATHLINEEDIT_H
#include "ctkPathLineEdit.h"
#include "DACommonWidgetsAPI.h"
namespace DA
{
/**
 * @brief 封装ctkPathLineEdit，便于进行文件选择
 */
class DACOMMONWIDGETS_API DAPathLineEdit : public ctkPathLineEdit
{
    Q_OBJECT
public:
    DAPathLineEdit(QWidget* p = nullptr);
    DAPathLineEdit(const QString& label, const QStringList& nameFilters, Filters filters = ctkPathLineEdit::AllEntries, QWidget* p = 0);
};
}  // namespace DA
#endif  // FCPATHLINEEDIT_H
