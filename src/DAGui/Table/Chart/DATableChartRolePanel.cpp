#include "Table/Chart/DATableChartRolePanel.h"
#include "Table/Chart/DATableChartPlanner.h"
#include "Common/Settings/DAPropertyPanelWidget.h"
#include "DATableDataSource.h"
#include "DALogCategory.h"
// Qt
#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
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
    Numeric,            ///< 只列数值列
    NumericOrDateTime,  ///< 数值或时间列
    Categorical,        ///< 只列分类列
    CategoryLike,       ///< 分类列 + 数值列（低基数数值可作为分类）
    Supported           ///< 所有可静态判定角色的列
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

// 属性面板中下拉/微调框统一横向填满（与 DAPropertyPanelWidget::addEnumProperty 一致）
void makeEditorFillWidth(QWidget* editor)
{
    if (editor) {
        editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
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

    // 数据列号 → request.columns 下标；不在选区时追加（设置页允许换整表任意列）
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

    // 往属性面板追加一行角色行（label + editor），返回属性项 ID
    int addRow(const QString& label, QWidget* editor)
    {
        if (!rowPanel || !editor) {
            delete editor;
            return -1;
        }
        const int id = rowPanel->addProperty(label, editor);
        if (id < 0) {
            delete editor;
            return -1;
        }
        rowIds.append(id);
        return id;
    }

    // 编辑后统一刷新：先通知窗口重绘，再把角色行重建排到下一轮事件循环
    void afterEdit()
    {
        Q_EMIT q_func()->specEdited();
        QTimer::singleShot(0, q_func(), [ this ]() { q_func()->rebuildRows(); });
    }

    // 列下拉：候选列按角色过滤；当前值不在候选里时补一项，避免显示错位
    QComboBox* createColumnCombo(ColumnFilter filter, int currentDataIndex, bool includeRowIndex, bool includeNone)
    {
        QComboBox* combo = new QComboBox();
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        makeEditorFillWidth(combo);
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
        int comboIndex   = -1;
        const int wanted = (currentDataIndex < 0) ? (includeRowIndex ? kRowIndexItem : kNoneItem) : currentDataIndex;
        comboIndex       = combo->findData(wanted);
        if (comboIndex < 0 && currentDataIndex >= 0) {
            const DATableChartColumn* column = findColumn(currentDataIndex);
            combo->addItem(column ? DATableChartRolePanel::columnDisplayName(*column) : QString::number(currentDataIndex),
                           currentDataIndex);
            comboIndex = combo->count() - 1;
        }
        if (comboIndex >= 0) {
            combo->setCurrentIndex(comboIndex);
        }
        updating = false;
        return combo;
    }

    // 单列角色行：属性名 + 列下拉
    void addSingleColumnRow(const QString& label,
                            ColumnFilter filter,
                            int currentDataIndex,
                            const std::function< void(int) >& apply,
                            bool includeRowIndex = false)
    {
        QComboBox* combo = createColumnCombo(filter, currentDataIndex, includeRowIndex, false);
        addRow(label, combo);
        QObject::connect(combo,
                         QOverload< int >::of(&QComboBox::currentIndexChanged),
                         q_func(),
                         [ this, combo, apply ](int) {
                             if (updating) {
                                 return;
                             }
                             apply(combo->currentData().toInt());
                             afterEdit();
                         });
    }

    // 多值角色行：每行一个列下拉 + 删除按钮，末尾一个「添加序列」按钮行
    void addMultiColumnRows(const QString& label,
                            const QList< int >& currentDataIndexes,
                            const std::function< void(const QList< int >&) >& apply,
                            bool allowEmpty)
    {
        const QList< int > values = currentDataIndexes;
        for (int position = 0; position < values.size(); ++position) {
            QWidget* row   = new QWidget();
            QHBoxLayout* lay = new QHBoxLayout(row);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(4);
            QComboBox* combo = createColumnCombo(ColumnFilter::Numeric, values.at(position), false, false);
            lay->addWidget(combo, 1);
            QToolButton* remove = new QToolButton(row);
            remove->setText(QStringLiteral("\u2212"));
            remove->setToolTip(DATableChartRolePanel::tr("Remove this series"));  // cn:移除此序列
            remove->setAutoRaise(true);
            remove->setEnabled((values.size() > 1) || allowEmpty);
            lay->addWidget(remove);
            makeEditorFillWidth(row);
            addRow(position == 0 ? label : QString(), row);
            QObject::connect(combo,
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
            QObject::connect(remove, &QToolButton::clicked, q_func(), [ this, values, position, apply ]() {
                QList< int > next = values;
                if (position >= 0 && position < next.size()) {
                    next.removeAt(position);
                }
                apply(next);
                afterEdit();
            });
        }
        QPushButton* addButton = new QPushButton(DATableChartRolePanel::tr("Add series"));  // cn:添加序列
        addButton->setAutoDefault(false);
        addButton->setIcon(QIcon(QStringLiteral(":/DAGui/icon/right-add.svg")));
        addButton->setIconSize(QSize(12, 12));
        addRow(QString(), addButton);
        QObject::connect(addButton, &QPushButton::clicked, q_func(), [ this, values, apply ]() {
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
    }

    // 聚合口径行
    void addAggregationRow(const DATableChartAggregation& current)
    {
        QComboBox* combo = new QComboBox();
        makeEditorFillWidth(combo);
        updating = true;
        const QList< DATableChartAggregation > all = DATableChartPlanner::allAggregations();
        for (DATableChartAggregation aggregation : all) {
            combo->addItem(aggregationText(aggregation), static_cast< int >(aggregation));
        }
        combo->setCurrentIndex(combo->findData(static_cast< int >(current)));
        updating = false;
        addRow(DATableChartRolePanel::tr("Aggregation"), combo);  // cn:聚合
        QObject::connect(combo,
                         QOverload< int >::of(&QComboBox::currentIndexChanged),
                         q_func(),
                         [ this, combo ](int) {
                             if (updating) {
                                 return;
                             }
                             spec.aggregation = static_cast< DATableChartAggregation >(combo->currentData().toInt());
                             afterEdit();
                         });
    }

    // 直方图分箱行
    void addHistogramBinsRow(int bins)
    {
        QSpinBox* spin = new QSpinBox();
        spin->setRange(1, 500);
        makeEditorFillWidth(spin);
        updating = true;
        spin->setValue(bins);
        updating = false;
        addRow(DATableChartRolePanel::tr("Bins"), spin);  // cn:分箱
        QObject::connect(spin, QOverload< int >::of(&QSpinBox::valueChanged), q_func(), [ this, spin ](int) {
            if (updating) {
                return;
            }
            spec.histogramBins = spin->value();
            afterEdit();
        });
    }

    // 聚合口径显示名（面板内部用，与浮窗的静态方法保持同一文案）
    static QString aggregationText(DATableChartAggregation aggregation)
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
    DAPropertyPanelWidget* rowPanel { nullptr };
    QList< int > rowIds;  ///< 当前角色行在属性面板里的 ID（重建时逐个移除）
    bool updating { false };
};

// ============================================================
// DATableChartRolePanel
// ============================================================

DATableChartRolePanel::DATableChartRolePanel(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
}

DATableChartRolePanel::~DATableChartRolePanel()
{
}

/**
 * @brief 设置承载角色行的属性面板
 *
 * 宿主设置页在「数据角色」分组创建后调用；切换面板时先清掉旧面板上的行。
 */
void DATableChartRolePanel::setRowPanel(DAPropertyPanelWidget* panel)
{
    DA_D(d);
    if (d->rowPanel == panel) {
        return;
    }
    clearRows();
    d->rowPanel = panel;
    rebuildRows();
}

/**
 * @brief 当前承载面板
 */
DAPropertyPanelWidget* DATableChartRolePanel::rowPanel() const
{
    DA_DC(d);
    return d->rowPanel;
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
 * @brief 移除当前全部角色行
 *
 * removeProperty 走 deleteLater，可在下拉框自己的信号栈上安全调用。
 */
void DATableChartRolePanel::clearRows()
{
    DA_D(d);
    if (d->rowPanel) {
        for (int id : std::as_const(d->rowIds)) {
            d->rowPanel->removeProperty(id);
        }
    }
    d->rowIds.clear();
}

/**
 * @brief 按当前规格图型重建角色行
 *
 * 角色表见实施方案 §3.2。所有改动都直接写回 d->spec / d->request，
 * 通过 specEdited 让绘图区即时重绘。行本身写进通用属性面板（DATableChartRolePanel 只做编排），
 * 外观/行高/属性名宽度与图表设置页一致。
 */
void DATableChartRolePanel::rebuildRows()
{
    DA_D(d);
    clearRows();
    if (!d->rowPanel) {
        return;
    }
    d->updating = true;

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
        d->addSingleColumnRow(
            tr("Group"),  // cn:分组
            ColumnFilter::Categorical,
            d->spec.categoryColumn >= 0 ? d->request.columns.value(d->spec.categoryColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.categoryColumn = d->ensureColumnIndex(dataIndex); });
    } break;
    case DATableChartType::Bar: {
        d->addSingleColumnRow(
            tr("Category"),  // cn:分类
            ColumnFilter::CategoryLike,
            d->spec.categoryColumn >= 0 ? d->request.columns.value(d->spec.categoryColumn).dataIndex : -1,
            [ d ](int dataIndex) { d->spec.categoryColumn = d->ensureColumnIndex(dataIndex); });
        if (!d->spec.valueColumns.isEmpty()) {
            d->addSingleColumnRow(
                tr("Value"),  // cn:值
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
