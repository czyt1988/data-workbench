#ifndef DADIALOGFILLSERIES_H
#define DADIALOGFILLSERIES_H
#include <QDialog>
#include <QString>
#include "DAGuiAPI.h"

class QRadioButton;
class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;

namespace DA
{
/**
 * @brief 序列填充对话框（005 T1.24，设计稿 §4.8）
 *
 * 两种序列：数值等差（起始值 / 步长 / 终止值）与日期步进（起始日期 / 步长 / 单位
 * 天·周·月·年），均由 Python 求值（da_fill_series，pandas date_range/DateOffset 语义），
 * 空值/dtype 判定不在 C++ 做。对话框只收集参数并**实时预览前几个值**（预览与写入
 * 共用同一算子，红线5）；实际写入的个数由页面 widget 按每列选中行数决定，对话框不接触 df。
 *
 * 起始值/步长/终止值留空即用算子默认（linear: 0/1；date: 今天/1）。日期模式下
 * 起始值需可被 pd.Timestamp 解析（如 2020-01-01），无法解析时预览显示错误、OK 置灰。
 */
class DAGUI_API DADialogFillSeries : public QDialog
{
    Q_OBJECT
public:
    // previewCount 为预览生成的值个数（一般取选区首列的行数，<=0 时用 5）
    explicit DADialogFillSeries(int previewCount = 5, QWidget* parent = nullptr);
    ~DADialogFillSeries();

    // 序列种类："linear" 数值等差 / "date" 日期步进
    QString kind() const;
    // 起始值文本（空串=默认）
    QString startText() const;
    // 步长文本（空串=默认）
    QString stepText() const;
    // 终止值文本（仅 linear，空串=不限）
    QString stopText() const;
    // 日期单位："D"/"W"/"M"/"Y"（仅 date）
    QString dateUnit() const;
    // 当前参数是否可生成有效序列（预览无错）
    bool isSeriesValid() const;

private Q_SLOTS:
    void onKindChanged();
    void updatePreview();

private:
    void setupUI();
    void retranslateUi();

    int mPreviewCount;
    QRadioButton* mRadioLinear { nullptr };
    QRadioButton* mRadioDate { nullptr };
    QLineEdit* mEditStart { nullptr };
    QLineEdit* mEditStep { nullptr };
    QLineEdit* mEditStop { nullptr };
    QComboBox* mComboDateUnit { nullptr };
    QLabel* mLabelStopTitle { nullptr };
    QLabel* mLabelPreview { nullptr };
    QPushButton* mButtonOk { nullptr };
    bool mSeriesValid { false };
};
}  // end of namespace DA
#endif  // DADIALOGFILLSERIES_H
