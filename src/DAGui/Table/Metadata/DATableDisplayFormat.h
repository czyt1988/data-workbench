#ifndef DATABLEDISPLAYFORMAT_H
#define DATABLEDISPLAYFORMAT_H
#include "DAGuiAPI.h"
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QDomDocument>
#include <QDomElement>

namespace DA
{
/**
 * @brief 表格列显示格式值对象
 *
 * 描述一列单元格如何把原始值渲染为显示文本（类似 Excel 的“设置单元格格式 > 数字”）。
 * 非 QObject，结构体形态，便于作为 QHash value 频繁拷贝。
 * 列级存储于 DATableStyleManager（与 bg/fg/font 的 DATableCellStyle 分离）。
 *
 * 有效标志 mValid 区分“未设置（继承默认显示）”与“显式设为某类别”。
 */
class DAGUI_API DATableDisplayFormat
{
public:
    enum Category
    {
        General,           // 常规：原始默认显示，也用作“清除格式”
        Number,            // 数值：保留 N 位小数
        Scientific,        // 科学计数法
        Percent,           // 百分比
        DateTime,          // 日期时间：套用 pattern
        DatetimeAsNumber,  // 时间显示为数字（epoch）
        Text               // 文本：强制按字符串显示
    };
    enum EpochUnit
    {
        EpochSeconds,
        EpochMilliseconds
    };

    DATableDisplayFormat() = default;
    explicit DATableDisplayFormat(Category c);

    // 类别
    Category category() const;
    void setCategory(Category c);
    // 精度（Number/Scientific/Percent 的小数位）
    int precision() const;
    void setPrecision(int p);
    // 日期时间 pattern（DateTime 用，Qt 日期格式串）
    QString pattern() const;
    void setPattern(const QString& p);
    // epoch 单位（DatetimeAsNumber 用）
    EpochUnit epochUnit() const;
    void setEpochUnit(EpochUnit u);

    // 是否设置了格式
    bool isValid() const;
    void setValid(bool v);

    // 把原始 QVariant 格式化为显示文本
    QString formatValue(const QVariant& raw) const;

    // 常用日期时间 pattern 预设
    static QStringList dateTimePresets();
    static QString defaultDateTimePattern();

    // 类别/epoch 单位 与字符串互转（英文，用于序列化与 combo 映射，非翻译文本）
    static QString categoryToString(Category c);
    static Category stringToCategory(const QString& s, bool* ok = nullptr);
    static QString epochUnitToString(EpochUnit u);
    static EpochUnit stringToEpochUnit(const QString& s, bool* ok = nullptr);

    bool operator==(const DATableDisplayFormat& other) const;
    bool operator!=(const DATableDisplayFormat& other) const;

    // 序列化：把属性作为属性写到元素 e 上（由 manager 包成 <col-format index=N>）
    void toXml(QDomDocument& doc, QDomElement& e) const;
    bool fromXml(const QDomElement& e);

private:
    Category mCategory { General };
    int mPrecision { 2 };
    QString mPattern;
    EpochUnit mEpochUnit { EpochMilliseconds };
    bool mValid { false };
};
}  // end of namespace DA

Q_DECLARE_METATYPE(DA::DATableDisplayFormat)

#endif  // DATABLEDISPLAYFORMAT_H
