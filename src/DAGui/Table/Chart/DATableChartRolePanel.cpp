#include "Table/Chart/DATableChartRolePanel.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "DATableDataSource.h"
#include "DALogCategory.h"
// Qt
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>
#include <functional>

namespace DA
{

namespace
{
// 列下拉的特殊条目值（真正的列用数据列号 >= 0）
constexpr int kRowIndexItem = -2;  ///< 行号
constexpr int kNoneItem     = -3;  ///< （不使用）

enum class ColumnFilter
{
    Numeric,             ///< 只列数值列
    NumericOrDateTime,   ///< 数值或时间列
    Categorical,         ///< 只列分类列
    CategoryLike,        ///< 分类列 + 数值列（低基数数值可作为分类）
    Supported            ///< 所有可静态判定角色的列
};

bool acceptsRole(ColumnFilter filter, DATableColumnRole role)
{
    switch (filter) {
    case ColumnFilter::Numeric:
        return role == DATableColumnRole::Numeric;
    case ColumnFilter::NumericOrDateTime:
        return role == DATableColumnRole::Numeric || role == DATableColumnRole::DateTime;
    case ColumnFilter::Categorical:
        return role == DATableColumnRole::Categorical;
    case ColumnFilter::CategoryLike:
        return role == DATableColumnRole::Categorical || role == DATableColumnRole::Numeric;
    case ColumnFilter::Supported:
        return role != DATableColumnRole::Unsupported;
    }
    return false;
}

}  // namespace

// ============================================================
// DATableChartRolePanelPrivate
// ============================================================

class DATableChartRolePanel::PrivateData
{
    DA_DECLARE_PUBLIC(DATableChartRolePanel)
public:
    explicit PrivateData(DATableChartRolePanel* q) : q_ptr(q)
    {
    }

    // 列清单：优先整表列，缺省时退回当前请求的列
    const QList< DATableChartColumn >& availableColumns() const
    {
        return request.allColumns.isEmpty() ? request.columns : request.allColumns;
    }

    const DATableChartColumn* findColumn(int dataIndex) const
    {
        const QList< DATableChartColumn >& columns = availableColumns();
        for (const DATableChartColumn& column : columns) {
            if (column.dataIndex == dataIndex) {
                return &column;
            }
        }
        for (const DATableChartColumn& column : std::as_const(request.columns)) {
            if (column.dataIndex == dataIndex) {
                return &column;
            }
        }
        return nullptr;
    }

    // 数据列号 → request.columns 下标；不在选区时追加（设置区允许换整表任意列）
    int ensureColumnIndex(int dataIndex)
    {
        if (dataIndex < 0) {
            return -1;
        }
        for (int i = 0; i < request.columns.size(); ++i) {
            if (request.columns.at(i).dataIndex == dataIndex) {
                return i;
            }
        }
        DATableChartColumn column;
        if (const DATableChartColumn* found = findColumn(dataIndex)) {
            column = *found;
        } else {
            column.dataIndex = dataIndex;
        }
        request.columns.append(column);
        return request.columns.size() - 1;
    }

    // 编辑后统一刷新：先通知窗口重绘，再把角色行重建排到下一轮事件循环
    void afterEdit()
    {
        Q_EMIT q_func()->specEdited();
        QTimer::singleShot(0, q_func(), [ this ]() { q_func()->rebuildRows(); });
    }

    QComboBox* createColumnCombo(QWidget* parent,
                                 ColumnFilter filter,
                                 int currentDataIndex,
                                 bool includeRowIndex,
                                 bool includeNone)
    {
        QComboBox* combo = new QComboBox(parent);
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        updating = true;
        if (includeRowIndex) {
            combo->addItem(DATableChartRolePanel::tr("Row number"), kRowIndexItem);  // cn:行号
        }
        if (includeNone) {
            combo->addItem(DATableChartRolePanel::tr("(none)"), kNoneItem);  // cn:（不使用）
        }
        const QList< DATableChartColumn >& columns = availableColumns();
        for (const DATableChartColumn& column : columns) {
            if (!acceptsRole(filter, column.role)) {
                continue;
            }
            combo->addItem(DATableChartRolePanel::columnDisplayName(column), column.dataIndex);
        }
        // 当前值不在候选里（例如手动换过列）时补一项，避免下拉显示错位
        int comboIndex = -1;
        const int wanted = (currentDataIndex < 0) ? (includeRowIndex ? kRowIndexItem : kNoneItem) : currentDataIndex;
        comboIndex       = combo->findData(wanted);
        if (comboIndex < 0 && currentDataIndex >= 0) {
            const DATableChartColumn* column = findColumn(currentDataIndex);
            combo->addItem(column ? DATableChartRolePanel::columnDisplayName(*column)
                                  : QString::number(currentDataIndex),
                           currentDataIndex);
            comboIndex = combo->count() - 1;
        }
        if (comboIndex >= 0) {
            combo->setCurrentIndex(comboIndex);
        }
        updating = false;
        return combo;
    }

    QWidget* createRow(QWidget* parent)
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        return row;
    }

    QLabel* createRoleLabel(QWidget* parent, const QString& text)
    {
        QLabel* label = new QLabel(text, parent);
        label->setMinimumWidth(64);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return label;
    }

    // 单列角色行：标签 + 列下拉
    void addSingleColumnRow(const QString& label,
                            ColumnFilter filter,
                            int currentDataIndex,
                            const std::function< void(int) >& apply,
                            bool includeRowIndex = false)
    {
        QWidget* row        = createRow(content);
        QHBoxLayout* layout = qobject_cast< QHBoxLayout* >(row->layout());
        layout->addWidget(createRoleLabel(row, label));
        QComboBox* combo = createColumnCombo(row, filter, currentDataIndex, includeRowIndex, false);
        layout->addWidget(combo, 1);
        connect(combo,
                QOverload< int >::of(&QComboBox::currentIndexChanged),
                q_func(),
                [ this, combo, apply ](int) {
                    if (updating) {
                        return;
                    }
                    apply(combo->currentData().toInt());
                    afterEdit();
                });
        rowsLayout->addWidget(row);
    }

    // 多值角色行：每行一个列下拉 + 删除按钮，末尾一个「添加」按钮
    void addMultiColumnRows(const QString& label,
                            const QList< int >& currentDataIndexes,
                            const std::function< void(const QList< int >&) >& apply,
                            bool allowEmpty)
    {
        QList< int > values = currentDataIndexes;
        for (int position = 0; position < values.size(); ++position) {
            QWidget* row        = createRow(content);
            QHBoxLayout* layout = qobject_cast< QHBoxLayout* >(row->layout());
            layout->addWidget(position == 0 ? createRoleLabel(row, label) : createRoleLabel(row, QString()));
            QComboBox* combo = createColumnCombo(row, ColumnFilter::Numeric, values.at(position), false, false);
            layout->addWidget(combo, 1);
            connect(combo,
                    QOverload< int >::of(&QComboBox::currentIndexChanged),
                    q_func(),
                    [ this, combo, values, position, apply ](int) {
                        if (updating) {
                            return;
                        }
                        QList< int > next = values;
                        next[ position ]  = combo->currentData().toInt();
                        apply(next);
                        afterEdit();
                    });
            QToolButton* remove = new QToolButton(row);
            remove->setText(QStringLiteral("\u2212"));
            remove->setToolTip(DATableChartRolePanel::tr("Remove this series"));  // cn:移除此序列
            remove->setAutoRaise(true);
            const bool canRemove = (values.size() > 1) || allowEmpty;
            remove->setEnabled(canRemove);
            connect(remove, &QToolButton::clicked, q_func(), [ this, values, position, apply ]() {
                QList< int > next = values;
                if (position >= 0 && position < next.size()) {
                    next.removeAt(position);
                }
                apply(next);
                afterEdit();
            });
            layout->addWidget(remove);
            rowsLayout->addWidget(row);
        }
        QWidget* addRow        = createRow(content);
        QHBoxLayout* addLayout = qobject_cast< QHBoxLayout* >(addRow->layout());
        addLayout->addWidget(createRoleLabel(addRow, QString()));
        QPushButton* addButton = new QPushButton(DATableChartRolePanel::tr("Add series"), addRow);  // cn:添加序列
        addButton->setAutoDefault(false);
        addLayout->addWidget(addButton, 0, Qt::AlignLeft);
        addLayout->addStretch(1);
        connect(addButton, &QPushButton::clicked, q_func(), [ this, values, apply ]() {
            QList< int > next = values;
            // 选一个尚未使用的数值列；都用了就重复第一个
            int candidate = -1;
            const QList< DATableChartColumn >& columns = availableColumns();
            for (const DATableChartColumn& column : columns) {
                if (column.role != DATableColumnRole::Numeric) {
                    continue;
                }
                bool used = false;
                for (int dataIndex : std::as_const(next)) {
                    if (dataIndex == column.dataIndex) {
                        used = true;
                        break;
                    }
                }
                if (!used) {
                    candidate = column.dataIndex;
                    break;
                }
            }
            if (candidate < 0) {
                if (next.isEmpty()) {
                    return;  // 没有可用数值列，不追加空行
                }
                candidate = next.first();
            }
            next.append(candidate);
            apply(next);
            afterEdit();
        });
        rowsLayout->addWidget(addRow);
    }

    // 聚合口径行
    void addAggregationRow(const DATableChartAggregation& current)
    {
        QWidget* row        = createRow(content);
        QHBoxLayout* layout = qobject_cast< QHBoxLayout* >(row->layout());
        layout->addWidget(createRoleLabel(row, DATableChartRolePanel::tr("Aggregation")));  // cn:聚合
        QComboBox* combo = new QComboBox(row);
        updating         = true;
        const QList< DATableChartAggregation > all = DATableChartPlanner::allAggregations();
        for (DATableChartAggregation aggregation : all) {
            combo->addItem(DATableChartQuickPlotAggregationText(aggregation), static_cast< int >(aggregation));
        }
        combo->setCurrentIndex(combo->findData(static_cast< int >(current)));
        updating = false;
        layout->addWidget(combo, 1);
        connect(combo,
                QOverload< int >::of(&QComboBox::currentIndexChanged),
                q_func(),
                [ this, combo ](int) {
                    if (updating) {
                        return;
                    }
                    spec.aggregation = static_cast< DATableChartAggregation >(combo->currentData().toInt());
                    afterEdit();
                });
        rowsLayout->addWidget(row);
    }

    // 直方图分箱行
    void addHistogramBinsRow(int bins)
    {
        QWidget* row        = createRow(content);
        QHBoxLayout* layout = qobject_cast< QHBoxLayout* >(row->layout());
        layout->addWidget(createRoleLabel(row, DATableChartRolePanel::tr("Bins")));  // cn:分箱
        QSpinBox* spin = new QSpinBox(row);
        spin->setRange(1, 500);
        updating = true;
        spin->setValue(bins);
        updating = false;
        layout->addWidget(spin, 0, Qt::AlignLeft);
        layout->addStretch(1);
        connect(spin, QOverload< int >::of(&QSpinBox::valueChanged), q_func(), [ this, spin ](int) {
            if (updating) {
                return;
            }
            spec.histogramBins = spin->value();
            afterEdit();
        });
        rowsLayout->addWidget(row);
    }

    // 聚合口径显示名（面板内部用，与浮窗的静态方法保持同一文案）
    static QString DATableChartQuickPlotAggregationText(DATableChartAggregation aggregation)
    {
        switch (aggregation) {
        case DATableChartAggregation::Mean:
            return DATableChartRolePanel::tr("Mean");  // cn:平均值
        case DATableChartAggregation::Median:
            return DATableChartRolePanel::tr("Median");  // cn:中位数
        case DATableChartAggregation::Sum:
            return DATableChartRolePanel::tr("Sum");  // cn:求和
        case DATableChartAggregation::Count:
            return DATableChartRolePanel::tr("Count");  // cn:计数
        case DATableChartAggregation::Std:
            return DATableChartRolePanel::tr("Std");  // cn:标准差
        case DATableChartAggregation::Var:
            return DATableChartRolePanel::tr("Var");  // cn:方差
        case DATableChartAggregation::Min:
            return DATableChartRolePanel::tr("Min");  // cn:最小值
        case DATableChartAggregation::Max:
            return DATableChartRolePanel::tr("Max");  // cn:最大值
        }
        return QString();
    }

public:
    DATableChartRequest request;
    DATableChartSpec spec;
    QWidget* content { nullptr };
    QVBoxLayout* rowsLayout { nullptr };
    bool updating { false };
};

// ============================================================
// DATableChartRolePanel
// ============================================================

DATableChartRolePanel::DATableChartRolePanel(QWidget* parent) : QWidget(parent), DA_PIMPL_CONSTRUCT
{
    DA_D(d);
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 2, 4, 2);
    root->setSpacing(2);
    d->content     = new QWidget(this);
    d->rowsLayout  = new QVBoxLayout(d->content);
    d->rowsLayout->setContentsMargins(0, 0, 0, 0);
    d->rowsLayout->setSpacing(2);
    root->addWidget(d->content);
    root->addStretch(1);
}

DATableChartRolePanel::~DATableChartRolePanel()
{
}

/**
 * @brief 设置请求与规格（按规格图型重建角色行）
 */
void DATableChartRolePanel::setContext(const DATableChartRequest& request, const DATableChartSpec& spec)
{
    DA_D(d);
    d->request = request;
    d->spec    = spec;
    rebuildRows();
}

/**
 * @brief 当前请求（可能已追加手动选择的列）
 */
const DATableChartRequest& DATableChartRolePanel::request() const
{
    DA_DC(d);
    return d->request;
}

/**
 * @brief 当前规格
 */
const DATableChartSpec& DATableChartRolePanel::spec() const
{
    DA_DC(d);
    return d->spec;
}

/**
 * @brief 列的显示名（重名列追加序号）
 */
QString DATableChartRolePanel::columnDisplayName(const DATableChartColumn& column)
{
    if (column.occurrence <= 0) {
        return column.name;
    }
    return QStringLiteral("%1 (%2)").arg(column.name).arg(column.occurrence + 1);
}

/**
 * @brief 按当前规格图型重建角色行
 *
 * 角色表见实施方案 §3.2。所有改动都直接写回 d->spec / d->request，
 * 通过 specEdited 让浮窗即时重绘。
 */
void DATableChartRolePanel::rebuildRows()
{
    DA_D(d);
    d->updating = true;
    // 清空旧行：先移出布局，再延迟删除（本函数可能由下拉框的信号栈触发）
    while (QLayoutItem* item = d->rowsLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }

    const QList< int > numericValues = d->spec.valueColumns;

    switch (d->spec.type) {
    case DATableChartType::Line:
        d->addSingleColumnRow(
            tr("X axis"),  // cn:X 轴
            ColumnFilter::NumericOrDateTime,
            d->spec.xColumn >= 0 ? d->request.columns.value(d->spec.xColumn).dataIndex : -2,
            [ d ](int dataIndex) { d->spec.xColumn = d->ensureColumnIndex(dataIndex); },
            true);
        d->addSingleColumnRow(
            tr("Y"),  // cn:Y
            ColumnFilter::Numeric,
            numericValues.isEmpty() ? -1 : d->request.columns.value(numericValues.first()).dataIndex,
            [ d ](int dataIndex) {
                const int index = d->ensureColumnIndex(dataIndex);
                if (index >= 0) {
                    d->spec.valueColumns = { index };
                }
            });
        break;
    case DATableChartType::Scatter:
        d->addSingleColumnRow(
            tr("X"),  // cn:X
            ColumnFilter::NumericOrDateTime,
            d->spec.xColumn >= 0 ? d->request.columns.value(d->spec.xColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.xColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Y"),  // cn:Y
            ColumnFilter::Numeric,
            numericValues.isEmpty() ? -1 : d->request.columns.value(numericValues.first()).dataIndex,
            [ d ](int dataIndex) {
                const int index = d->ensureColumnIndex(dataIndex);
                if (index >= 0) {
                    d->spec.valueColumns = { index };
                }
            });
        break;
    case DATableChartType::MultiLine:
        d->addSingleColumnRow(
            tr("X axis"),  // cn:X 轴
            ColumnFilter::NumericOrDateTime,
            d->spec.xColumn >= 0 ? d->request.columns.value(d->spec.xColumn).dataIndex : -2,
            [ d ](int dataIndex) { d->spec.xColumn = d->ensureColumnIndex(dataIndex); },
            true);
        d->addMultiColumnRows(
            tr("Y"),  // cn:Y
            numericValues,
            [ d ](const QList< int >& dataIndexes) {
                d->spec.valueColumns.clear();
                for (int dataIndex : dataIndexes) {
                    d->spec.valueColumns.append(d->ensureColumnIndex(dataIndex));
                }
            },
            false);
        break;
    case DATableChartType::Histogram:
        d->addSingleColumnRow(
            tr("Value"),  // cn:值
            ColumnFilter::Numeric,
            numericValues.isEmpty() ? -1 : d->request.columns.value(numericValues.first()).dataIndex,
            [ d ](int dataIndex) {
                const int index = d->ensureColumnIndex(dataIndex);
                if (index >= 0) {
                    d->spec.valueColumns = { index };
                }
            });
        d->addHistogramBinsRow(d->spec.histogramBins);
        break;
    case DATableChartType::Boxplot: {
        d->addMultiColumnRows(
            tr("Values"),  // cn:数值
            numericValues,
            [ d ](const QList< int >& dataIndexes) {
                d->spec.valueColumns.clear();
                for (int dataIndex : dataIndexes) {
                    d->spec.valueColumns.append(d->ensureColumnIndex(dataIndex));
                }
            },
            false);
        QWidget* row        = d->createRow(d->content);
        QHBoxLayout* layout = qobject_cast< QHBoxLayout* >(row->layout());
        layout->addWidget(d->createRoleLabel(row, tr("Group")));  // cn:分组
        QComboBox* combo = d->createColumnCombo(
            row,
            ColumnFilter::Categorical,
            d->spec.categoryColumn >= 0 ? d->request.columns.value(d->spec.categoryColumn).dataIndex : -1,
            false,
            true);
        layout->addWidget(combo, 1);
        connect(combo,
                QOverload< int >::of(&QComboBox::currentIndexChanged),
                this,
                [ d, combo ](int) {
                    if (d->updating) {
                        return;
                    }
                    d->spec.categoryColumn = d->ensureColumnIndex(combo->currentData().toInt());
                    d->afterEdit();
                });
        d->rowsLayout->addWidget(row);
    } break;
    case DATableChartType::Bar: {
        d->addSingleColumnRow(
            tr("Category"),  // cn:分类
            ColumnFilter::CategoryLike,
            d->spec.categoryColumn >= 0 ? d->request.columns.value(d->spec.categoryColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.categoryColumn = d->ensureColumnIndex(dataIndex); });
        if (!d->spec.valueColumns.isEmpty()) {
            d->addSingleColumnRow(
                tr("Value"),  // cn:数值
                ColumnFilter::Numeric,
                d->request.columns.value(d->spec.valueColumns.first()).dataIndex,
                [ d ](int dataIndex) {
                    const int index = d->ensureColumnIndex(dataIndex);
                    if (index >= 0) {
                        d->spec.valueColumns = { index };
                    }
                });
            d->addAggregationRow(d->spec.aggregation);
        }
    } break;
    case DATableChartType::MultiBar:
        d->addSingleColumnRow(
            tr("Category"),  // cn:分类
            ColumnFilter::CategoryLike,
            d->spec.categoryColumn >= 0 ? d->request.columns.value(d->spec.categoryColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.categoryColumn = d->ensureColumnIndex(dataIndex); });
        d->addMultiColumnRows(
            tr("Values"),  // cn:数值
            numericValues,
            [ d ](const QList< int >& dataIndexes) {
                d->spec.valueColumns.clear();
                for (int dataIndex : dataIndexes) {
                    d->spec.valueColumns.append(d->ensureColumnIndex(dataIndex));
                }
            },
            false);
        d->addAggregationRow(d->spec.aggregation);
        break;
    case DATableChartType::Contour:
        d->addSingleColumnRow(
            tr("X"),  // cn:X
            ColumnFilter::Numeric,
            d->spec.xColumn >= 0 ? d->request.columns.value(d->spec.xColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.xColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Y"),  // cn:Y
            ColumnFilter::Numeric,
            d->spec.yColumn >= 0 ? d->request.columns.value(d->spec.yColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.yColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Value"),  // cn:值
            ColumnFilter::Numeric,
            d->spec.zColumn >= 0 ? d->request.columns.value(d->spec.zColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.zColumn = d->ensureColumnIndex(dataIndex); });
        break;
    case DATableChartType::ErrorBar:
        d->addSingleColumnRow(
            tr("X"),  // cn:X
            ColumnFilter::Numeric,
            d->spec.xColumn >= 0 ? d->request.columns.value(d->spec.xColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.xColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Y"),  // cn:Y
            ColumnFilter::Numeric,
            d->spec.yColumn >= 0 ? d->request.columns.value(d->spec.yColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.yColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Error"),  // cn:误差
            ColumnFilter::Numeric,
            d->spec.errorColumn >= 0 ? d->request.columns.value(d->spec.errorColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.errorColumn = d->ensureColumnIndex(dataIndex); });
        break;
    case DATableChartType::VectorField:
        d->addSingleColumnRow(
            tr("X"),  // cn:X
            ColumnFilter::Numeric,
            d->spec.xColumn >= 0 ? d->request.columns.value(d->spec.xColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.xColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Y"),  // cn:Y
            ColumnFilter::Numeric,
            d->spec.yColumn >= 0 ? d->request.columns.value(d->spec.yColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.yColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("U"),  // cn:U
            ColumnFilter::Numeric,
            d->spec.uColumn >= 0 ? d->request.columns.value(d->spec.uColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.uColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("V"),  // cn:V
            ColumnFilter::Numeric,
            d->spec.vColumn >= 0 ? d->request.columns.value(d->spec.vColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.vColumn = d->ensureColumnIndex(dataIndex); });
        break;
    case DATableChartType::Surface3D:
    case DATableChartType::Line3D:
        d->addSingleColumnRow(
            tr("X"),  // cn:X
            ColumnFilter::Numeric,
            d->spec.xColumn >= 0 ? d->request.columns.value(d->spec.xColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.xColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Y"),  // cn:Y
            ColumnFilter::Numeric,
            d->spec.yColumn >= 0 ? d->request.columns.value(d->spec.yColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.yColumn = d->ensureColumnIndex(dataIndex); });
        d->addSingleColumnRow(
            tr("Z"),  // cn:Z
            ColumnFilter::Numeric,
            d->spec.zColumn >= 0 ? d->request.columns.value(d->spec.zColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.zColumn = d->ensureColumnIndex(dataIndex); });
        break;
    case DATableChartType::None:
        break;
    }
    d->updating = false;
}

}  // namespace DA
