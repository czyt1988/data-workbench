#ifndef DADIALOGGOTOCELL_H
#define DADIALOGGOTOCELL_H
#include "DAGuiAPI.h"
#include <QDialog>
#include <QString>

class QSpinBox;
class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QDialogButtonBox;

namespace DA
{
/**
 * @brief 定位对话框（007 T1.7）
 *
 * 四类输入（行位置 / 索引标签 / 列名 / 首尾快捷项）+「保持隐藏列隐藏」勾选项。
 * 行列"没改 = 不改"：只填列保持当前行、只填行保持当前列；快捷项直接置 kind 并
 * accept。懒建复用：setup() 清上一次状态（不回显残留）。
 */
class DAGUI_API DADialogGotoCell : public QDialog
{
    Q_OBJECT
public:
    struct Request {
        enum class RowKind { None, Position, IndexLabel, FirstRow, LastRow };
        enum class ColumnKind { None, Name, FirstColumn, LastColumn };
        RowKind rowKind { RowKind::None };
        int rowPosition { 1 };      ///< 1 基（与屏幕序号一致）
        QString indexLabel;         ///< 索引标签原文
        ColumnKind columnKind { ColumnKind::None };
        QString columnName;         ///< 列名原文
        bool keepHidden { false };  ///< 「保持隐藏列隐藏」勾选项
    };

    explicit DADialogGotoCell(QWidget* parent = nullptr);
    ~DADialogGotoCell() override;

    // 列名列表重名保留重复项；勾选项可见性由 hasHiddenColumns 决定
    void setup(int totalRows, const QStringList& columnNames, int currentRowPosition,
               const QString& currentColumnName, bool hasHiddenColumns);
    // 用户请求（OK 后取；行列 kind 按"是否改动过初值"判定）
    Request request() const;

private:
    void setupUi();
    void applyQuickRow(Request::RowKind k);
    void applyQuickColumn(Request::ColumnKind k);

private:
    QSpinBox* mSpinRow { nullptr };
    QLabel* mLabelRowRange { nullptr };
    QLineEdit* mEditIndexLabel { nullptr };
    QComboBox* mComboColumn { nullptr };
    QCheckBox* mChkKeepHidden { nullptr };
    QDialogButtonBox* mButtonBox { nullptr };
    Request mPrepared;        ///< 快捷按钮写入的 kind（request() 优先保留）
    int mInitialRowPosition { 1 };
    QString mInitialColumnName;
};
}  // end of namespace DA
#endif  // DADIALOGGOTOCELL_H
