#include "DAChartItemSettingPanel.h"
#include "DAPropertyPanelContainerWidget.h"
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>

namespace DA
{

/**
 * @brief 构造函数
 * @param parent 父控件
 */
DAChartItemSettingPanel::DAChartItemSettingPanel(QWidget* parent)
    : DAAbstractChartItemSettingWidget(parent)
    , mPanel(nullptr)
{
    // 创建DAPropertyPanelContainerWidget并设为自身主布局
    mPanel = new DAPropertyPanelContainerWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mPanel);
    setLayout(layout);

    // 连接propertyValueChanged信号
    connect(mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged, this, &DAChartItemSettingPanel::onPanelPropertyValueChanged);

    // 注意：不在此调用buildPropertyPanel()，由子类构造函数末尾自行调用
}

/**
 * @brief 析构函数
 */
DAChartItemSettingPanel::~DAChartItemSettingPanel()
{
}

/**
 * @brief 获取通用属性面板指针
 * @return DAPropertyPanelContainerWidget指针
 */
DAPropertyPanelContainerWidget* DAChartItemSettingPanel::propertyPanel() const
{
    return mPanel;
}

/**
 * @brief 转发属性值变化信号
 * @param propertyId 属性ID
 */
void DAChartItemSettingPanel::onPanelPropertyValueChanged(int propertyId)
{
    emit propertyValueChanged(propertyId);
}

/**
 * @brief 添加曲线样式属性
 *
 * 创建QComboBox，填充QwtPlotCurve::CurveStyle枚举项：
 * Lines(1), Sticks(2), Steps(3), Dots(4), NoCurve(5)
 * itemData中存储对应的枚举int值
 *
 * @param id 属性ID
 * @param name 属性名
 */
void DAChartItemSettingPanel::addCurveStyleProperty(int id, const QString& name)
{
    QComboBox* combo = new QComboBox(this);
    combo->addItem(tr("Lines"), static_cast< int >(QwtPlotCurve::Lines));
    combo->addItem(tr("Sticks"), static_cast< int >(QwtPlotCurve::Sticks));
    combo->addItem(tr("Steps"), static_cast< int >(QwtPlotCurve::Steps));
    combo->addItem(tr("Dots"), static_cast< int >(QwtPlotCurve::Dots));
    combo->addItem(tr("No Curve"), static_cast< int >(QwtPlotCurve::NoCurve));

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged),
            mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged);

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加方向属性
 *
 * 创建包含Horizontal/Vertical两个QRadioButton的QWidget，
 * 使用QButtonGroup管理单选按钮行为
 *
 * @param id 属性ID
 * @param name 属性名
 */
void DAChartItemSettingPanel::addOrientationProperty(int id, const QString& name)
{
    QWidget* container = new QWidget(this);
    QHBoxLayout* hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(8);

    QRadioButton* rbH = new QRadioButton(tr("Horizontal"), container);
    QRadioButton* rbV = new QRadioButton(tr("Vertical"), container);

    QButtonGroup* group = new QButtonGroup(container);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    group->addButton(rbH, Qt::Horizontal);
    group->addButton(rbV, Qt::Vertical);
#else
    group->setId(rbH, Qt::Horizontal);
    group->setId(rbV, Qt::Vertical);
    group->addButton(rbH);
    group->addButton(rbV);
#endif
    rbH->setChecked(true);

    hLayout->addWidget(rbH);
    hLayout->addWidget(rbV);
    hLayout->addStretch();

    mButtonGroupMap[id] = group;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(group, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });
#else
    connect(group, &QButtonGroup::idClicked, this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });
#endif

    mPanel->addProperty(id, name, container);
}

/**
 * @brief 添加坐标轴属性
 *
 * 创建QComboBox，根据isYAxis填充对应轴的枚举值
 * 当isYAxis=true时：YLeft(0), YRight(1)
 * 当isYAxis=false时：XBottom(2), XTop(3)
 *
 * @param id 属性ID
 * @param name 属性名
 * @param isYAxis 是否为Y轴（默认true）
 */
void DAChartItemSettingPanel::addAxisProperty(int id, const QString& name, bool isYAxis)
{
    QComboBox* combo = new QComboBox(this);
    if (isYAxis) {
        combo->addItem(tr("Y Left"), static_cast< int >(QwtAxis::YLeft));
        combo->addItem(tr("Y Right"), static_cast< int >(QwtAxis::YRight));
    } else {
        combo->addItem(tr("X Bottom"), static_cast< int >(QwtAxis::XBottom));
        combo->addItem(tr("X Top"), static_cast< int >(QwtAxis::XTop));
    }

    connect(combo, QOverload< int >::of(&QComboBox::currentIndexChanged),
            mPanel, &DAPropertyPanelContainerWidget::propertyValueChanged);

    mPanel->addProperty(id, name, combo);
}

/**
 * @brief 添加标记属性
 *
 * 创建DAChartSymbolEditWidget编辑器，使用BelowLayout布局
 * 转发所有符号变更信号到propertyPanel的propertyValueChanged
 *
 * @param id 属性ID
 * @param name 属性名
 */
void DAChartItemSettingPanel::addSymbolProperty(int id, const QString& name)
{
    DAChartSymbolEditWidget* symbolEdit = new DAChartSymbolEditWidget(this);

    connect(symbolEdit, &DAChartSymbolEditWidget::symbolStyleChanged,
            this, [this, id](QwtSymbol::Style) { emit propertyValueChanged(id); });
    connect(symbolEdit, &DAChartSymbolEditWidget::symbolSizeChanged,
            this, [this, id](int) { emit propertyValueChanged(id); });
    connect(symbolEdit, &DAChartSymbolEditWidget::symbolColorChanged,
            this, [this, id](const QColor&) { emit propertyValueChanged(id); });
    connect(symbolEdit, &DAChartSymbolEditWidget::symbolOutlinePenChanged,
            this, [this, id](const QPen&) { emit propertyValueChanged(id); });

    mPanel->addProperty(id, name, symbolEdit, DAPropertyItemWidget::BelowLayout);
}

/**
 * @brief 添加刻度样式属性
 *
 * 创建包含Normal/DateTime两个QRadioButton的QWidget，
 * 使用QButtonGroup管理单选按钮行为
 *
 * @param id 属性ID
 * @param name 属性名
 */
void DAChartItemSettingPanel::addScaleStyleProperty(int id, const QString& name)
{
    QWidget* container = new QWidget(this);
    QHBoxLayout* hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(8);

    QRadioButton* rbNormal = new QRadioButton(tr("Normal"), container);
    QRadioButton* rbDateTime = new QRadioButton(tr("DateTime"), container);

    QButtonGroup* group = new QButtonGroup(container);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    group->addButton(rbNormal, 0);
    group->addButton(rbDateTime, 1);
#else
    group->setId(rbNormal, 0);
    group->setId(rbDateTime, 1);
    group->addButton(rbNormal);
    group->addButton(rbDateTime);
#endif
    rbNormal->setChecked(true);

    hLayout->addWidget(rbNormal);
    hLayout->addWidget(rbDateTime);
    hLayout->addStretch();

    mScaleStyleGroupMap[id] = group;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(group, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });
#else
    connect(group, &QButtonGroup::idClicked, this, [this, id](int) {
        onPanelPropertyValueChanged(id);
    });
#endif

    mPanel->addProperty(id, name, container);
}

/**
 * @brief 获取曲线样式值
 *
 * 通过ComboBox的currentData获取QwtPlotCurve::CurveStyle枚举值
 *
 * @param id 属性ID
 * @return 曲线样式枚举值
 */
QwtPlotCurve::CurveStyle DAChartItemSettingPanel::getCurveStyleValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return QwtPlotCurve::Lines;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return QwtPlotCurve::Lines;
    }
    return static_cast< QwtPlotCurve::CurveStyle >(combo->currentData().toInt());
}

/**
 * @brief 设置曲线样式值
 *
 * @param id 属性ID
 * @param style 曲线样式枚举值
 */
void DAChartItemSettingPanel::setCurveStyleValue(int id, QwtPlotCurve::CurveStyle style)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(style));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

/**
 * @brief 获取方向值
 *
 * 通过QButtonGroup的checkedId获取方向值
 * 返回Qt::Horizontal或Qt::Vertical
 *
 * @param id 属性ID
 * @return 方向值
 */
Qt::Orientation DAChartItemSettingPanel::getOrientationValue(int id) const
{
    auto it = mButtonGroupMap.constFind(id);
    if (it == mButtonGroupMap.constEnd()) {
        return Qt::Horizontal;
    }
    QButtonGroup* group = it.value();
    int checkedId = group->checkedId();
    return static_cast< Qt::Orientation >(checkedId);
}

/**
 * @brief 设置方向值
 *
 * @param id 属性ID
 * @param orientation 方向值（Qt::Horizontal或Qt::Vertical）
 */
void DAChartItemSettingPanel::setOrientationValue(int id, Qt::Orientation orientation)
{
    auto it = mButtonGroupMap.find(id);
    if (it == mButtonGroupMap.end()) {
        return;
    }
    QButtonGroup* group = it.value();
    // 通过blockSignals防止触发信号
    bool wasBlocked = group->blockSignals(true);
    if (orientation == Qt::Horizontal) {
        // Qt5/Qt6兼容查找button
        auto buttons = group->buttons();
        for (auto btn : buttons) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            if (group->id(btn) == Qt::Horizontal) {
#else
            if (group->id(btn) == Qt::Horizontal) {
#endif
                btn->setChecked(true);
                break;
            }
        }
    } else {
        auto buttons = group->buttons();
        for (auto btn : buttons) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            if (group->id(btn) == Qt::Vertical) {
#else
            if (group->id(btn) == Qt::Vertical) {
#endif
                btn->setChecked(true);
                break;
            }
        }
    }
    group->blockSignals(wasBlocked);
}

/**
 * @brief 获取坐标轴值
 *
 * 通过ComboBox的currentData获取QwtAxis::Position枚举值
 *
 * @param id 属性ID
 * @return 坐标轴枚举值
 */
QwtAxis::Position DAChartItemSettingPanel::getAxisValue(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return QwtAxis::YLeft;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return QwtAxis::YLeft;
    }
    return static_cast< QwtAxis::Position >(combo->currentData().toInt());
}

/**
 * @brief 设置坐标轴值
 *
 * @param id 属性ID
 * @param axisId 坐标轴枚举值
 */
void DAChartItemSettingPanel::setAxisValue(int id, QwtAxis::Position axisId)
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return;
    }
    QComboBox* combo = qobject_cast< QComboBox* >(item->editorWidget());
    if (nullptr == combo) {
        return;
    }
    QSignalBlocker blocker(combo);
    int index = combo->findData(static_cast< int >(axisId));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

/**
 * @brief 获取标记编辑器Widget
 *
 * @param id 属性ID
 * @return DAChartSymbolEditWidget指针，失败返回nullptr
 */
DAChartSymbolEditWidget* DAChartItemSettingPanel::getSymbolWidget(int id) const
{
    DAPropertyItemWidget* item = mPanel->getPropertyItem(id);
    if (nullptr == item) {
        return nullptr;
    }
    return qobject_cast< DAChartSymbolEditWidget* >(item->editorWidget());
}

/**
 * @brief 获取刻度样式值
 *
 * 通过QButtonGroup的checkedId获取刻度样式值
 * 返回0(Normal)或1(DateTime)
 *
 * @param id 属性ID
 * @return 刻度样式值
 */
int DAChartItemSettingPanel::getScaleStyleValue(int id) const
{
    auto it = mScaleStyleGroupMap.constFind(id);
    if (it == mScaleStyleGroupMap.constEnd()) {
        return 0;  // 默认Normal
    }
    QButtonGroup* group = it.value();
    return group->checkedId();
}

/**
 * @brief 设置刻度样式值
 *
 * @param id 属性ID
 * @param scaleStyle 刻度样式值（0=Normal, 1=DateTime）
 */
void DAChartItemSettingPanel::setScaleStyleValue(int id, int scaleStyle)
{
    auto it = mScaleStyleGroupMap.find(id);
    if (it == mScaleStyleGroupMap.end()) {
        return;
    }
    QButtonGroup* group = it.value();
    bool wasBlocked = group->blockSignals(true);
    auto buttons = group->buttons();
    for (auto btn : buttons) {
        if (group->id(btn) == scaleStyle) {
            btn->setChecked(true);
            break;
        }
    }
    group->blockSignals(wasBlocked);
}

}  // end namespace DA
