#ifndef DADIALOGTABLEDISPLAYFORMAT_H
#define DADIALOGTABLEDISPLAYFORMAT_H
#include "DAGuiAPI.h"
#include "DATableDisplayFormat.h"
#include <QDialog>
#include <QVariant>

class QStackedWidget;
class QListWidget;
class QLabel;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QRadioButton;

namespace DA
{
class DAPyDType;

/**
 * @brief 表格列显示格式设置对话框（类似 Excel“设置单元格格式 > 数字”）
 *
 * 左侧类别列表（按列 dtype 启用），右侧按类别切换选项（精度/日期 pattern/epoch 单位），
 * 顶部预览用样本原始值实时 formatValue。OK 返回配置好的 DATableDisplayFormat。
 */
class DAGUI_API DADialogTableDisplayFormat : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造
     * @param current 当前格式（用于初始化选项）
     * @param dtype 列 dtype（决定哪些类别可用）
     * @param sample 样本原始值（用于预览）
     * @param parent 父窗口
     */
    DADialogTableDisplayFormat(const DATableDisplayFormat& current,
                               const DAPyDType& dtype,
                               const QVariant& sample,
                               QWidget* parent = nullptr);
    ~DADialogTableDisplayFormat();

    // 根据当前 UI 状态构造格式（General 类别返回 invalid，表示清除格式）
    DATableDisplayFormat getResult() const;

private:
    void setupUi(const DAPyDType& dtype);
    void switchToCategory(DATableDisplayFormat::Category c);
    void updatePreview();
    DATableDisplayFormat buildFormat() const;

private:
    DATableDisplayFormat mCurrent;
    QVariant mSample;
    // UI 控件（代码构建）
    QStackedWidget* mStack { nullptr };
    QListWidget* mCategoryList { nullptr };
    QLabel* mPreviewLabel { nullptr };
    QSpinBox* mSpinPrecision { nullptr };          // Number/Scientific/Percent 共用
    QComboBox* mComboDateTimePreset { nullptr };   // DateTime
    QLineEdit* mEditDateTimePattern { nullptr };   // DateTime 自定义
    QRadioButton* mRadioEpochSeconds { nullptr }; // DatetimeAsNumber
    QRadioButton* mRadioEpochMillis { nullptr };  // DatetimeAsNumber
};
}  // end of namespace DA
#endif  // DADIALOGTABLEDISPLAYFORMAT_H
