#include "DABrushStyleComboBox.h"
#include <memory>
#include <QPainter>
namespace DA
{

class DABrushStyleComboBox::PrivateData
{
    DA_DECLARE_PUBLIC(DABrushStyleComboBox)
public:
    PrivateData(DABrushStyleComboBox* p);
    bool mIsShowNoBrush;
    std::unique_ptr< QColor > mColor;
    std::unique_ptr< QGradient > mGradient;
    std::unique_ptr< QPixmap > mPixmap;
    Qt::BrushStyle mCurrentBrushStyle;  ///< 记录当前的style
    bool mStyleTextVisible { false };
};

DABrushStyleComboBox::PrivateData::PrivateData(DABrushStyleComboBox* p)
    : q_ptr(p), mIsShowNoBrush(true), mCurrentBrushStyle(Qt::NoBrush)
{
}

//===================================================
// DABrushStyleComboBox
//===================================================

DABrushStyleComboBox::DABrushStyleComboBox(QWidget* parent) : QComboBox(parent), DA_PIMPL_CONSTRUCT
{
    setBrushColor(Qt::black);
    connect(this, QOverload< int >::of(&QComboBox::currentIndexChanged), this, &DABrushStyleComboBox::onCurrentIndexChanged);
}

DABrushStyleComboBox::~DABrushStyleComboBox()
{
}

QString DABrushStyleComboBox::brushStyleToString(Qt::BrushStyle s)
{
    switch (s) {
    case Qt::NoBrush:
        return QObject::tr("No Brush");  // No brush pattern
    case Qt::SolidPattern:
        return QObject::tr("Solid Pattern");  // Uniform color
    case Qt::Dense1Pattern:
        return QObject::tr("Dense1 Pattern");  // Extremely dense brush pattern
    case Qt::Dense2Pattern:
        return QObject::tr("Dense2 Pattern");  // Very dense brush pattern
    case Qt::Dense3Pattern:
        return QObject::tr("Dense3 Pattern");  // Somewhat dense brush pattern
    case Qt::Dense4Pattern:
        return QObject::tr("Dense4 Pattern");  // Half dense brush pattern
    case Qt::Dense5Pattern:
        return QObject::tr("Dense5 Pattern");  // Somewhat sparse brush pattern
    case Qt::Dense6Pattern:
        return QObject::tr("Dense6 Pattern");  // Very sparse brush pattern
    case Qt::Dense7Pattern:
        return QObject::tr("Dense7 Pattern");  // Extremely sparse brush pattern
    case Qt::HorPattern:
        return QObject::tr("Hor Pattern");  // Horizontal lines
    case Qt::VerPattern:
        return QObject::tr("Ver Pattern");  // Vertical lines
    case Qt::CrossPattern:
        return QObject::tr("Cross Pattern");  // Crossing horizontal and vertical lines
    case Qt::BDiagPattern:
        return QObject::tr("Backward Diagonal Pattern");  // Backward diagonal lines
    case Qt::FDiagPattern:
        return QObject::tr("Forward Diagonal Pattern");  // Forward diagonal lines
    case Qt::DiagCrossPattern:
        return QObject::tr("Crossing Diagonal Pattern");  // Crossing diagonal lines
    case Qt::LinearGradientPattern:
        return QObject::tr("Linear Gradient Pattern");  // Linear gradient (set using a dedicated QBrush constructor).
    case Qt::ConicalGradientPattern:
        return QObject::tr("Conical Gradient Pattern");  // Conical gradient (set using a dedicated QBrush constructor).
    case Qt::RadialGradientPattern:
        return QObject::tr("Radial Gradient Pattern");  // Radial gradient (set using a dedicated QBrush constructor).
    case Qt::TexturePattern:
        return QObject::tr("Texture Pattern");  // Custom pattern (see QBrush::setTexture()).
    default:
        break;
    }
    return QString();
}

/**
 * @brief 设置显示无画刷
 * @param v
 */
void DABrushStyleComboBox::setShowNoBrushStyle(bool v)
{
    if (d_ptr->mIsShowNoBrush != v) {
        d_ptr->mIsShowNoBrush = v;
        rebuildItems();
    }
}

/**
 * @brief 显示无画刷
 * @return
 */
bool DABrushStyleComboBox::isShowNoBrushStyle() const
{
    return d_ptr->mIsShowNoBrush;
}

/**
 * @brief 判断是否是纯色填充
 * @return
 */
bool DABrushStyleComboBox::isColorBrush() const
{
    return d_ptr->mColor != nullptr;
}

/**
 * @brief ShowPaintStyle显示，ShowGradientStyle和ShowTextureStyle被去除
 * @param v
 */
void DABrushStyleComboBox::setBrushColor(const QColor& v)
{
    if (d_ptr->mColor) {
        *(d_ptr->mColor) = v;
    } else {
        d_ptr->mColor.reset(new QColor(v));
        d_ptr->mGradient.reset();
        d_ptr->mPixmap.reset();
        rebuildItems();
    }
}

/**
 * @brief DABrushStyleComboBox::getBrushColor
 * @return
 */
QColor DABrushStyleComboBox::getBrushColor() const
{
    if (d_ptr->mColor) {
        return *(d_ptr->mColor);
    }
    return QColor();
}

/**
 * @brief 重建所有item
 */
void DABrushStyleComboBox::rebuildItems()
{
    clear();
    if (isColorBrush()) {
        addItem(Qt::NoBrush);
        addItem(Qt::SolidPattern);
        addItem(Qt::Dense1Pattern);
        addItem(Qt::Dense2Pattern);
        addItem(Qt::Dense3Pattern);
        addItem(Qt::Dense4Pattern);
        addItem(Qt::Dense5Pattern);
        addItem(Qt::Dense6Pattern);
        addItem(Qt::Dense7Pattern);
        addItem(Qt::HorPattern);
        addItem(Qt::VerPattern);
        addItem(Qt::CrossPattern);
        addItem(Qt::BDiagPattern);
        addItem(Qt::FDiagPattern);
        addItem(Qt::DiagCrossPattern);
    }
}

QIcon DABrushStyleComboBox::generateBrushStyleIcon(Qt::BrushStyle s)
{
    QSize iss = iconSize();
    if (!iss.isValid()) {
        return QIcon();
    }
    static QIcon s_noBrush = QIcon(":/DACommonWidgets/icon/no-style.svg");
    switch (s) {
    case Qt::NoBrush:
        return s_noBrush;
    case Qt::SolidPattern:
    case Qt::Dense1Pattern:
    case Qt::Dense2Pattern:
    case Qt::Dense3Pattern:
    case Qt::Dense4Pattern:
    case Qt::Dense5Pattern:
    case Qt::Dense6Pattern:
    case Qt::Dense7Pattern:
    case Qt::HorPattern:
    case Qt::VerPattern:
    case Qt::CrossPattern:
    case Qt::BDiagPattern:
    case Qt::FDiagPattern:
    case Qt::DiagCrossPattern: {
        QPixmap pixmap(iss);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        QBrush b(getBrushColor(), s);
        painter.fillRect(QRect(QPoint(0, 0), iss), b);
        return QIcon(pixmap);
    }
    default:
        break;
    }
    return QIcon();
}

/**
 * @brief 更新icon
 */
void DABrushStyleComboBox::updateIcons()
{
    int c = count();

    for (int i = 0; i < c; ++i) {
        Qt::BrushStyle s = static_cast< Qt::BrushStyle >(itemData(i).toInt());
        if (Qt::NoBrush == s) {
            continue;
        }
        setItemIcon(i, generateBrushStyleIcon(s));
    }
}

void DABrushStyleComboBox::setStyleTextVisible(bool on)
{
    if (d_ptr->mStyleTextVisible != on) {
        // 要重新生成
        // 先获取已经设置的
        QList< Qt::BrushStyle > ps;
        int c = count();
        for (int i = 0; i < c; ++i) {
            ps.append(static_cast< Qt::BrushStyle >(itemData(i).toInt()));
        }
        clear();
        for (Qt::BrushStyle s : std::as_const(ps)) {
            addItem(s);
        }
    }
    d_ptr->mStyleTextVisible = on;
}

bool DABrushStyleComboBox::isStyleTextVisible() const
{
    return d_ptr->mStyleTextVisible;
}

/**
   @brief 获取当前的画刷样式
   @return
 */
Qt::BrushStyle DABrushStyleComboBox::getCurrentBrushStyle() const
{
    QVariant v = currentData();
    if (v.isValid()) {
        return static_cast< Qt::BrushStyle >(v.toInt());
    }
    return Qt::NoBrush;
}

/**
   @brief 获取画刷样式
   @param index
   @return
 */
Qt::BrushStyle DABrushStyleComboBox::getBrushStyle(int index) const
{
    QVariant v = itemData(index);
    if (v.isValid()) {
        return static_cast< Qt::BrushStyle >(v.toInt());
    }
    return Qt::NoBrush;
}

/**
 * @brief 设置当前选中的样式
 *
 * @note 此函数会发射信号@sa currentBrushStyleChanged
 * @note 如果此时index和style一致，不做任何处理
 * @param s
 */
void DABrushStyleComboBox::setCurrentBrushStyle(Qt::BrushStyle s)
{
    const int c = count();
    for (int i = 0; i < c; ++i) {
        Qt::BrushStyle ds = static_cast< Qt::BrushStyle >(itemData(i).toInt());
        if (ds == s) {
            if (currentIndex() != i) {
                setCurrentIndex(i);
            }
        }
    }
}

void DABrushStyleComboBox::onCurrentIndexChanged(int index)
{
    emit currentBrushStyleChanged(getBrushStyle(index));
}

void DABrushStyleComboBox::addItem(Qt::BrushStyle s)
{
    QComboBox::addItem(generateBrushStyleIcon(s), isStyleTextVisible() ? brushStyleToString(s) : "", static_cast< int >(s));
}

}
