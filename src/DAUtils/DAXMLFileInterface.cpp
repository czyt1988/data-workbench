#include "DAXMLFileInterface.h"
#include <QDomDocument>
#include <QDomElement>
// QVariant cast use
#include <math.h>
#include <QBitArray>
#include <QBitmap>
#include <QBrush>
#include <QByteArray>
#include <QCursor>
#include <QDataStream>
#include <QDate>
#include <QEasingCurve>
#include <QFont>
#include <QIcon>
#include <QImage>
#include <QKeySequence>
#include <QLocale>
#include <QMatrix4x4>
#include <QPalette>
#include <QPen>
#include <QPolygon>
#include <QQuaternion>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QStringList>
#include <QTextFormat>
#include <QTextLength>
#include <QTransform>
#include <QUrl>
#include <QUuid>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QPointer>
namespace DA
{
DAXMLFileInterface::DAXMLFileInterface()
{
}

DAXMLFileInterface::~DAXMLFileInterface()
{
}
/**
 * @brief 生成一个QRect标签
 * @param p
 * @param tagName 标签名
 * @param doc
 * @return
 */
QDomElement DAXMLFileInterface::makeElement(const QRect& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement ele = doc->createElement(tagName);
    ele.setAttribute("class", "QRect");
    ele.setAttribute("x", QString::number(v.x()));
    ele.setAttribute("y", QString::number(v.y()));
    ele.setAttribute("w", QString::number(v.width()));
    ele.setAttribute("h", QString::number(v.height()));
    return ele;
}
/**
 * @brief 加载QRect
 * @param p
 * @param ele
 * @return 此函数不会返回false
 */
bool DAXMLFileInterface::loadElement(QRect& p, const QDomElement* ele)
{
    int v;
    if (getStringIntValue(ele->attribute("x"), v)) {
        p.setX(v);
    }
    if (getStringIntValue(ele->attribute("y"), v)) {
        p.setY(v);
    }
    if (getStringIntValue(ele->attribute("w"), v)) {
        p.setWidth(v);
    }
    if (getStringIntValue(ele->attribute("h"), v)) {
        p.setHeight(v);
    }
    return true;
}
/**
 * @brief 生成一个QRectF标签
 * @param p
 * @param tagName 标签名
 * @param doc
 * @return
 */
QDomElement DAXMLFileInterface::makeElement(const QRectF& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement ele = doc->createElement(tagName);
    ele.setAttribute("class", "QRectF");
    ele.setAttribute("x", QString::number(v.x()));
    ele.setAttribute("y", QString::number(v.y()));
    ele.setAttribute("w", QString::number(v.width()));
    ele.setAttribute("h", QString::number(v.height()));
    return ele;
}
/**
 * @brief 加载QRectF
 * @param p
 * @param ele
 * @return 此函数不会返回false
 */
bool DAXMLFileInterface::loadElement(QRectF& p, const QDomElement* ele)
{
    qreal v;
    if (getStringRealValue(ele->attribute("x"), v)) {
        p.setX(v);
    }
    if (getStringRealValue(ele->attribute("y"), v)) {
        p.setY(v);
    }
    if (getStringRealValue(ele->attribute("w"), v)) {
        p.setWidth(v);
    }
    if (getStringRealValue(ele->attribute("h"), v)) {
        p.setHeight(v);
    }
    return true;
}

/**
 * @brief 生成一个QPoint标签
 * @param p
 * @param tagName 标签名
 * @param doc
 * @return
 */
QDomElement DAXMLFileInterface::makeElement(const QPoint& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement ele = doc->createElement(tagName);
    ele.setAttribute("class", "QPoint");
    ele.setAttribute("x", QString::number(v.x()));
    ele.setAttribute("y", QString::number(v.y()));
    return ele;
}

/**
 * @brief 加载QPoint
 * @param p
 * @param ele
 * @return 此函数不会返回false
 */
bool DAXMLFileInterface::loadElement(QPoint& p, const QDomElement* ele)
{
    int v;
    if (getStringIntValue(ele->attribute("x"), v)) {
        p.setX(v);
    }
    if (getStringIntValue(ele->attribute("y"), v)) {
        p.setY(v);
    }
    return true;
}
/**
 * @brief 生成一个QPointF标签
 * @param p
 * @param tagName 标签名
 * @param doc
 * @return
 */
QDomElement DAXMLFileInterface::makeElement(const QPointF& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement ele = doc->createElement(tagName);
    ele.setAttribute("class", "QPointF");
    ele.setAttribute("x", QString::number(v.x()));
    ele.setAttribute("y", QString::number(v.y()));
    return ele;
}

/**
 * @brief 加载QPointF
 * @param p
 * @param ele
 * @return 此函数不会返回false
 */
bool DAXMLFileInterface::loadElement(QPointF& p, const QDomElement* ele)
{
    qreal v;
    if (getStringRealValue(ele->attribute("x"), v)) {
        p.setX(v);
    }
    if (getStringRealValue(ele->attribute("y"), v)) {
        p.setY(v);
    }
    return true;
}
/**
 * @brief 生成一个标准画笔标签
 * @param p
 * @param tagName 标签名
 * @param doc
 * @return
 */
QDomElement DAXMLFileInterface::makeElement(const QPen& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement ele = doc->createElement(tagName);
    ele.setAttribute("class", "QPen");
    ele.setAttribute("color", v.color().name());
    ele.setAttribute("width", v.widthF());
    ele.setAttribute("style", enumToString(v.style()));
    return ele;
}

/**
 * @brief 加载画笔
 * @param p
 * @param ele
 * @return 此函数不会返回false
 */
bool DAXMLFileInterface::loadElement(QPen& p, const QDomElement* ele)
{
    QColor c;
    c.setNamedColor(ele->attribute("color"));
    if (c.isValid()) {
        p.setColor(c);
    }
    qreal w;
    if (getStringRealValue(ele->attribute("width"), w)) {
        p.setWidthF(w);
    }
    p.setStyle(stringToEnum(ele->attribute("style"), Qt::SolidLine));
    return true;
}
/**
 * @brief 生成一个标准QBrush标签
 * @param v
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXMLFileInterface::makeElement(const QBrush& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement ele  = doc->createElement(tagName);
    Qt::BrushStyle s = v.style();
    ele.setAttribute("class", "QBrush");
    ele.setAttribute("style", enumToString(s));
    switch (s) {
    case Qt::NoBrush:
        break;
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
    case Qt::DiagCrossPattern:
        ele.setAttribute("color", v.color().name());
        break;
        // TODO:
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
    case Qt::TexturePattern:
    default:
        break;
    }
    return ele;
}

bool DAXMLFileInterface::loadElement(QBrush& p, const QDomElement* ele)
{
    if (!ele->hasAttribute("style")) {
        return false;
    }
    Qt::BrushStyle s = stringToEnum(ele->attribute("style"), Qt::SolidPattern);
    p.setStyle(s);
    switch (s) {
    case Qt::NoBrush:
        return true;
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
        QColor c;
        c.setNamedColor(ele->attribute("color"));
        if (c.isValid()) {
            p.setColor(c);
        }
    } break;
        // TODO:
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
    case Qt::TexturePattern:
    default:
        break;
    }
    return true;
}

QDomElement DAXMLFileInterface::makeElement(const QFont& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement fontEle = doc->createElement(tagName);
    fontEle.setAttribute("class", "QFont");
    fontEle.setAttribute("bold", v.bold());
    fontEle.setAttribute("italic", v.italic());
    fontEle.setAttribute("pointSizeF", v.pointSizeF());
    // QFont::Weight的枚举值在qt5和qt6不一致，为了避免直接传值，都需要转换为QFont::Weight
#if QT_VERSION_MAJOR >= 6
    fontEle.setAttribute("weight", enumToString(v.weight()));
#else
    fontEle.setAttribute("weight", enumToString(static_cast< QFont::Weight >(v.weight())));
#endif
    fontEle.setAttribute("family", v.family());
    return fontEle;
}

bool DAXMLFileInterface::loadElement(QFont& p, const QDomElement* ele)
{
    p.setBold(ele->attribute("bold").toInt());
    p.setItalic(ele->attribute("italic").toInt());
    p.setPointSizeF(ele->attribute("pointSizeF").toDouble());
    p.setWeight(stringToEnum(ele->attribute("weight"), QFont::Normal));
    p.setFamily(ele->attribute("family"));
    return true;
}

/**
 * @brief 创建一个QVariant的标签
 *
 * 针对QStringList，QVariantList，QVariantMap，QVariantHashd
 * @param v
 * @param tagName
 * @param doc
 * @return
 */
QDomElement DAXMLFileInterface::makeElement(const QVariant& v, const QString& tagName, QDomDocument* doc)
{
    QDomElement varEle = doc->createElement(tagName);
    varEle.setAttribute("class", "QVariant");
    QString vartype = v.typeName();
    varEle.setAttribute("type", vartype);
#if QT_VERSION_MAJOR >= 6
    int tid = v.typeId();
#else
    int tid = v.type();
#endif
    // 特殊对待
    switch (tid) {
    case QMetaType::QStringList: {
        QStringList vl = v.toStringList();
        for (const QString& i : vl) {
            QDomElement li = doc->createElement("li");
            QDomText t     = doc->createTextNode(i);
            li.appendChild(t);
            varEle.appendChild(li);
        }
    } break;
    case QMetaType::QVariantList: {
        QVariantList vl = v.toList();
        for (const QVariant& i : vl) {
            QDomElement li = makeElement(i, "li", doc);
            varEle.appendChild(li);
        }
    } break;

    case QMetaType::QVariantMap: {
        QVariantMap vl = v.toMap();
        for (auto i = vl.begin(); i != vl.end(); ++i) {
            QDomElement m = makeElement(i.value(), "map", doc);
            m.setAttribute("key", i.key());
            varEle.appendChild(m);
        }
    } break;
    case QMetaType::QVariantHash: {
        QVariantHash vl = v.toHash();
        for (auto i = vl.begin(); i != vl.end(); ++i) {
            QDomElement m = makeElement(i.value(), "map", doc);
            m.setAttribute("key", i.key());
            varEle.appendChild(m);
        }
    } break;
    default: {  // 其他类型通过字符串转换
        QDomText t = doc->createTextNode(DA::variantToString(v));
        varEle.appendChild(t);
    } break;
    }

    return varEle;
}

/**
 * @brief 加载
 * @param p
 * @param ele
 * @return
 */
bool DAXMLFileInterface::loadElement(QVariant& p, const QDomElement* ele)
{
    bool ok      = false;
    QString type = ele->attribute("type");
    if (0 == type.compare("QStringList", Qt::CaseInsensitive)) {
        QStringList res;
        QDomNodeList liNodes = ele->childNodes();
        for (int i = 0; i < liNodes.size(); ++i) {
            QDomElement liEle = liNodes.at(i).toElement();
            if (liEle.tagName() == "li") {
                res.append(liEle.text());
            }
        }
        p  = QVariant(res);
        ok = true;
    } else if (0 == type.compare("QVariantList", Qt::CaseInsensitive)) {
        QVariantList res;
        QDomNodeList liNodes = ele->childNodes();
        for (int i = 0; i < liNodes.size(); ++i) {
            QDomElement liEle = liNodes.at(i).toElement();
            if (liEle.tagName() == "li") {
                QVariant v;
                if (loadElement(v, &liEle)) {
                    res.append(v);
                }
            }
        }
        p  = QVariant(res);
        ok = true;
    } else if (0 == type.compare("QVariantMap", Qt::CaseInsensitive)) {
        QVariantMap res;
        QDomNodeList liNodes = ele->childNodes();
        for (int i = 0; i < liNodes.size(); ++i) {
            QDomElement liEle = liNodes.at(i).toElement();
            QString key       = liEle.attribute("key");
            if (liEle.tagName() == "map") {
                QVariant v;
                if (loadElement(v, &liEle)) {
                    res[ key ] = v;
                }
            }
        }
        p  = QVariant(res);
        ok = true;
    } else if (0 == type.compare("QVariantHash", Qt::CaseInsensitive)) {
        QVariantHash res;
        QDomNodeList liNodes = ele->childNodes();
        for (int i = 0; i < liNodes.size(); ++i) {
            QDomElement liEle = liNodes.at(i).toElement();
            QString key       = liEle.attribute("key");
            if (liEle.tagName() == "map") {
                QVariant v;
                if (loadElement(v, &liEle)) {
                    res[ key ] = v;
                }
            }
        }
        p  = QVariant(res);
        ok = true;
    } else {
        p  = stringToVariant(ele->text(), type);
        ok = true;
    }
    return ok;
}

/**
 * @brief 获取int值
 * @param valuestring
 * @param v
 * @return
 */
bool getStringIntValue(const QString& valuestring, int& v)
{
    bool ok = false;
    v       = valuestring.toInt(&ok);
    return ok;
}

/**
   @brief 获取unsigned int值
   @param valuestring
   @param v
   @return
 */
bool getStringUIntValue(const QString& valuestring, unsigned int& v)
{
    bool ok = false;
    v       = valuestring.toUInt(&ok);
    return ok;
}

/**
   @brief 获取unsigned ll值
   @param valuestring
   @param v
   @return
 */
bool getStringULongLongValue(const QString& valuestring, unsigned long long& v)
{
    bool ok = false;
    v       = valuestring.toULongLong(&ok);
    return ok;
}

/**
 * @brief 获取qreal值
 * @param valuestring
 * @param v
 * @return
 */
bool getStringRealValue(const QString& valuestring, qreal& v)
{
    bool ok = false;
    v       = valuestring.toDouble(&ok);
    return ok;
}

/**
 * @brief 获取为bool类型
 * @param valuestring
 * @return
 */
bool getStringBoolValue(const QString& valuestring)
{
    return valuestring.toInt();
}

QString enumToString(Qt::PenStyle e)
{
    switch (e) {
    case Qt::NoPen:
        return "NoPen";
    case Qt::SolidLine:
        return "SolidLine";
    case Qt::DashLine:
        return "DashLine";
    case Qt::DotLine:
        return "DotLine";
    case Qt::DashDotLine:
        return "DashDotLine";
    case Qt::DashDotDotLine:
        return "DashDotDotLine";
    default:
        break;
    }
    return "SolidLine";
}

Qt::PenStyle stringToEnum(const QString& s, Qt::PenStyle defaultEnum)
{
    if (0 == s.compare("NoPen", Qt::CaseInsensitive)) {
        return Qt::NoPen;
    } else if (0 == s.compare("SolidLine", Qt::CaseInsensitive)) {
        return Qt::SolidLine;
    } else if (0 == s.compare("DashLine", Qt::CaseInsensitive)) {
        return Qt::DashLine;
    } else if (0 == s.compare("DotLine", Qt::CaseInsensitive)) {
        return Qt::DotLine;
    } else if (0 == s.compare("DashDotLine", Qt::CaseInsensitive)) {
        return Qt::DashDotLine;
    } else if (0 == s.compare("DashDotDotLine", Qt::CaseInsensitive)) {
        return Qt::DashDotDotLine;
    }
    return defaultEnum;
}

QString enumToString(Qt::AspectRatioMode e)
{
    switch (e) {
    case Qt::IgnoreAspectRatio:
        return "IgnoreAspectRatio";
    case Qt::KeepAspectRatio:
        return "KeepAspectRatio";
    case Qt::KeepAspectRatioByExpanding:
        return "KeepAspectRatioByExpanding";
    default:
        break;
    }
    return "IgnoreAspectRatio";
}

Qt::AspectRatioMode stringToEnum(const QString& s, Qt::AspectRatioMode defaultEnum)
{
    if (0 == s.compare("IgnoreAspectRatio", Qt::CaseInsensitive)) {
        return Qt::IgnoreAspectRatio;
    } else if (0 == s.compare("KeepAspectRatio", Qt::CaseInsensitive)) {
        return Qt::KeepAspectRatio;
    } else if (0 == s.compare("KeepAspectRatioByExpanding", Qt::CaseInsensitive)) {
        return Qt::KeepAspectRatioByExpanding;
    }
    return defaultEnum;
}

QString enumToString(Qt::TransformationMode e)
{
    switch (e) {
    case Qt::FastTransformation:
        return "FastTransformation";
    case Qt::SmoothTransformation:
        return "SmoothTransformation";
    default:
        break;
    }
    return "FastTransformation";
}

Qt::TransformationMode stringToEnum(const QString& s, Qt::TransformationMode defaultEnum)
{
    if (0 == s.compare("FastTransformation", Qt::CaseInsensitive)) {
        return Qt::FastTransformation;
    } else if (0 == s.compare("SmoothTransformation", Qt::CaseInsensitive)) {
        return Qt::SmoothTransformation;
    }
    return defaultEnum;
}

QString enumToString(Qt::Alignment e)
{
    switch (e) {
    case Qt::AlignLeft:
        return "AlignLeft";
    case Qt::AlignRight:
        return "AlignRight";
    case Qt::AlignHCenter:
        return "AlignHCenter";
    case Qt::AlignJustify:
        return "AlignJustify";
    case Qt::AlignAbsolute:
        return "AlignAbsolute";
    case Qt::AlignHorizontal_Mask:
        return "AlignHorizontal_Mask";
    case Qt::AlignTop:
        return "AlignTop";
    case Qt::AlignBottom:
        return "AlignBottom";
    case Qt::AlignVCenter:
        return "AlignVCenter";
    case Qt::AlignBaseline:
        return "AlignBaseline";
    case Qt::AlignVertical_Mask:
        return "AlignVertical_Mask";
    case Qt::AlignCenter:
        return "AlignCenter";
    default:
        break;
    }
    return "AlignCenter";
}

Qt::Alignment stringToEnum(const QString& s, Qt::Alignment defaultEnum)
{
    if (0 == s.compare("AlignCenter", Qt::CaseInsensitive)) {
        return Qt::AlignCenter;
    } else if (0 == s.compare("AlignHCenter", Qt::CaseInsensitive)) {
        return Qt::AlignHCenter;
    } else if (0 == s.compare("AlignLeft", Qt::CaseInsensitive)) {
        return Qt::AlignLeft;
    } else if (0 == s.compare("AlignRight", Qt::CaseInsensitive)) {
        return Qt::AlignRight;
    } else if (0 == s.compare("AlignVCenter", Qt::CaseInsensitive)) {
        return Qt::AlignVCenter;
    } else if (0 == s.compare("AlignTop", Qt::CaseInsensitive)) {
        return Qt::AlignTop;
    } else if (0 == s.compare("AlignBottom", Qt::CaseInsensitive)) {
        return Qt::AlignBottom;
    } else if (0 == s.compare("AlignAbsolute", Qt::CaseInsensitive)) {
        return Qt::AlignAbsolute;
    } else if (0 == s.compare("AlignHorizontal_Mask", Qt::CaseInsensitive)) {
        return Qt::AlignHorizontal_Mask;
    } else if (0 == s.compare("AlignBaseline", Qt::CaseInsensitive)) {
        return Qt::AlignBaseline;
    } else if (0 == s.compare("AlignVertical_Mask", Qt::CaseInsensitive)) {
        return Qt::AlignVertical_Mask;
    }
    return defaultEnum;
}

QString enumToString(Qt::BrushStyle e)
{
    switch (e) {
    case Qt::NoBrush:
        return "NoBrush";
    case Qt::SolidPattern:
        return "SolidPattern";
    case Qt::Dense1Pattern:
        return "Dense1Pattern";
    case Qt::Dense2Pattern:
        return "Dense2Pattern";
    case Qt::Dense3Pattern:
        return "Dense3Pattern";
    case Qt::Dense4Pattern:
        return "Dense4Pattern";
    case Qt::Dense5Pattern:
        return "Dense5Pattern";
    case Qt::Dense6Pattern:
        return "Dense6Pattern";
    case Qt::Dense7Pattern:
        return "Dense7Pattern";
    case Qt::HorPattern:
        return "HorPattern";
    case Qt::VerPattern:
        return "VerPattern";
    case Qt::CrossPattern:
        return "CrossPattern";
    case Qt::BDiagPattern:
        return "BDiagPattern";
    case Qt::FDiagPattern:
        return "FDiagPattern";
    case Qt::DiagCrossPattern:
        return "DiagCrossPattern";
    case Qt::LinearGradientPattern:
        return "LinearGradientPattern";
    case Qt::RadialGradientPattern:
        return "RadialGradientPattern";
    case Qt::ConicalGradientPattern:
        return "ConicalGradientPattern";
    case Qt::TexturePattern:
        return "TexturePattern";
    default:
        break;
    }
    return "SolidPattern";
}

Qt::BrushStyle stringToEnum(const QString& s, Qt::BrushStyle defaultEnum)
{
    if (0 == s.compare("NoBrush", Qt::CaseInsensitive)) {
        return Qt::NoBrush;
    } else if (0 == s.compare("SolidPattern", Qt::CaseInsensitive)) {
        return Qt::SolidPattern;
    } else if (0 == s.compare("Dense1Pattern", Qt::CaseInsensitive)) {
        return Qt::Dense1Pattern;
    } else if (0 == s.compare("Dense2Pattern", Qt::CaseInsensitive)) {
        return Qt::Dense2Pattern;
    } else if (0 == s.compare("Dense3Pattern", Qt::CaseInsensitive)) {
        return Qt::Dense3Pattern;
    } else if (0 == s.compare("Dense4Pattern", Qt::CaseInsensitive)) {
        return Qt::Dense4Pattern;
    } else if (0 == s.compare("Dense5Pattern", Qt::CaseInsensitive)) {
        return Qt::Dense5Pattern;
    } else if (0 == s.compare("Dense6Pattern", Qt::CaseInsensitive)) {
        return Qt::Dense6Pattern;
    } else if (0 == s.compare("Dense7Pattern", Qt::CaseInsensitive)) {
        return Qt::Dense7Pattern;
    } else if (0 == s.compare("HorPattern", Qt::CaseInsensitive)) {
        return Qt::HorPattern;
    } else if (0 == s.compare("VerPattern", Qt::CaseInsensitive)) {
        return Qt::VerPattern;
    } else if (0 == s.compare("CrossPattern", Qt::CaseInsensitive)) {
        return Qt::CrossPattern;
    } else if (0 == s.compare("BDiagPattern", Qt::CaseInsensitive)) {
        return Qt::BDiagPattern;
    } else if (0 == s.compare("FDiagPattern", Qt::CaseInsensitive)) {
        return Qt::FDiagPattern;
    } else if (0 == s.compare("DiagCrossPattern", Qt::CaseInsensitive)) {
        return Qt::DiagCrossPattern;
    } else if (0 == s.compare("LinearGradientPattern", Qt::CaseInsensitive)) {
        return Qt::LinearGradientPattern;
    } else if (0 == s.compare("RadialGradientPattern", Qt::CaseInsensitive)) {
        return Qt::RadialGradientPattern;
    } else if (0 == s.compare("ConicalGradientPattern", Qt::CaseInsensitive)) {
        return Qt::ConicalGradientPattern;
    } else if (0 == s.compare("TexturePattern", Qt::CaseInsensitive)) {
        return Qt::TexturePattern;
    }
    return defaultEnum;
}

QString enumToString(QFont::Weight e)
{
    switch (e) {
    case QFont::Thin:
        return "Thin";
    case QFont::ExtraLight:
        return "ExtraLight";
    case QFont::Light:
        return "Light";
    case QFont::Normal:
        return "Normal";
    case QFont::Medium:
        return "Medium";
    case QFont::DemiBold:
        return "DemiBold";
    case QFont::Bold:
        return "Bold";
    case QFont::ExtraBold:
        return "ExtraBold";
    case QFont::Black:
        return "Black";
    default:
        break;
    }
    return "Normal";
}

QFont::Weight stringToEnum(const QString& s, QFont::Weight defaultEnum)
{
    if (0 == s.compare("Thin", Qt::CaseInsensitive)) {
        return QFont::Thin;
    } else if (0 == s.compare("ExtraLight", Qt::CaseInsensitive)) {
        return QFont::ExtraLight;
    } else if (0 == s.compare("Light", Qt::CaseInsensitive)) {
        return QFont::Light;
    } else if (0 == s.compare("Normal", Qt::CaseInsensitive)) {
        return QFont::Normal;
    } else if (0 == s.compare("Medium", Qt::CaseInsensitive)) {
        return QFont::Medium;
    } else if (0 == s.compare("DemiBold", Qt::CaseInsensitive)) {
        return QFont::DemiBold;
    } else if (0 == s.compare("Bold", Qt::CaseInsensitive)) {
        return QFont::Bold;
    } else if (0 == s.compare("ExtraBold", Qt::CaseInsensitive)) {
        return QFont::ExtraBold;
    } else if (0 == s.compare("Black", Qt::CaseInsensitive)) {
        return QFont::Black;
    }
    return QFont::Normal;
}

//===================================================

}  // end of DA

QString DA::variantToString(const QVariant& var)
{

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    int tid = var.type();
#else
    int tid = var.typeId();
#endif
    switch (tid) {
    case QMetaType::UnknownType:
        return (QString());

    case QMetaType::QBitArray:
        return (converVariantToBase64String< QBitArray >(var));

    case QMetaType::QBitmap:
        return (converVariantToBase64String< QBitmap >(var));

    case QMetaType::Bool:
        return (var.toBool() ? "1" : "0");

    case QMetaType::QBrush:
        return (converVariantToBase64String< QBrush >(var));

    case QMetaType::QByteArray:
        return (converVariantToBase64String< QByteArray >(var));

    case QMetaType::QChar:
        return (var.toChar());

    case QMetaType::QColor: {
        QColor clr = var.value< QColor >();
        return (clr.name(QColor::HexArgb));
    }

    case QMetaType::QCursor:
        return (converVariantToBase64String< QCursor >(var));

    case QMetaType::QDate: {
        QDate d = var.toDate();
        return (d.toString(Qt::ISODate));
    }

    case QMetaType::QDateTime: {
        QDateTime d = var.toDateTime();
        return (d.toString(Qt::ISODate));
    }

    case QMetaType::Double: {
        double d = var.toDouble();
        return (doubleToString(d));  // 针对double，用非科学计数法会对小数丢失精度，因此采样g最合理，但小数点较多时需要适当处理
    }

    case QMetaType::QEasingCurve: {
        return (converVariantToBase64String< QEasingCurve >(var));
    }

    case QMetaType::QUuid: {
        return (var.toUuid().toString());
    }

    case QMetaType::QFont: {
        return (converVariantToBase64String< QFont >(var));
    }

    case QMetaType::QVariantHash: {
        return (converVariantToBase64String< QVariantHash >(var));
    }

    case QMetaType::QIcon: {
        return (converVariantToBase64String< QIcon >(var));
    }

    case QMetaType::QImage: {
        return (converVariantToBase64String< QImage >(var));
    }

    case QMetaType::Int: {
        return (QString::number(var.toInt()));
    }

    case QMetaType::QKeySequence: {
        QKeySequence d = var.value< QKeySequence >();
        return (d.toString(QKeySequence::NativeText));
    }

    case QMetaType::QLine: {
        QLine d = var.toLine();
        return (QString("%1;%2;%3;%4").arg(d.x1()).arg(d.y1()).arg(d.x2()).arg(d.y2()));
    }

    case QMetaType::QLineF: {
        QLineF d = var.toLineF();
        return (QString("%1;%2;%3;%4").arg(doubleToString(d.x1()), doubleToString(d.y1()), doubleToString(d.x2()), doubleToString(d.y2())));
    }

    case QMetaType::QVariantList: {
        return (converVariantToBase64String< QVariantList >(var));
    }

    case QMetaType::QLocale: {
        return (var.toLocale().name());
    }

    case QMetaType::LongLong: {
        return (QString::number(var.toLongLong()));
    }

    case QMetaType::QVariantMap: {
        return (converVariantToBase64String< QVariantMap >(var));
    }

    case QMetaType::QTransform: {
        QTransform d = var.value< QTransform >();
        return (QString("%1;%2;%3;%4;%5;%6;%7;%8;%9")
                        .arg(d.m11())
                        .arg(d.m12())
                        .arg(d.m13())
                        .arg(d.m21())
                        .arg(d.m22())
                        .arg(d.m23())
                        .arg(d.m31())
                        .arg(d.m32())
                        .arg(d.m33()));
    }

    case QMetaType::QMatrix4x4: {
        QMatrix4x4 d = var.value< QMatrix4x4 >();
        return (QString("%1;%2;%3;%4;%5;%6;%7;%8;%9;%10;%11;%12;%13;%14;%15;%16")
                        .arg(d(0, 0))
                        .arg(d(0, 1))
                        .arg(d(0, 2))
                        .arg(d(0, 3))
                        .arg(d(1, 0))
                        .arg(d(1, 1))
                        .arg(d(1, 2))
                        .arg(d(1, 3))
                        .arg(d(2, 0))
                        .arg(d(2, 1))
                        .arg(d(2, 2))
                        .arg(d(2, 3))
                        .arg(d(3, 0))
                        .arg(d(3, 1))
                        .arg(d(3, 2))
                        .arg(d(3, 3)));
    }

    case QMetaType::QPalette: {
        return (converVariantToBase64String< QPalette >(var));
    }

    case QMetaType::QPen: {
        return (converVariantToBase64String< QPen >(var));
    }

    case QMetaType::QPixmap: {
        return (converVariantToBase64String< QPixmap >(var));
    }

    case QMetaType::QPoint: {
        QPoint d = var.toPoint();
        return (QString("%1;%2").arg(d.x()).arg(d.y()));
    }

    case QMetaType::QPointF: {
        QPointF d = var.toPointF();
        return (QString("%1;%2").arg(doubleToString(d.x()), doubleToString(d.y())));
    }

    case QMetaType::QPolygon: {
        QPolygon d = var.value< QPolygon >();
        QString str;
        if (!d.isEmpty()) {
            str += QString("%1;%2").arg(d[ 0 ].x()).arg(d[ 0 ].y());
        }
        for (int i = 1; i < d.size(); ++i) {
            str += QString("|%1;%2").arg(d[ i ].x()).arg(d[ i ].y());
        }
        return (str);
    }

    case QMetaType::QPolygonF: {
        QPolygonF d = var.value< QPolygonF >();
        QString str;
        if (!d.isEmpty()) {
            ////用g,而不用f，f会导致小数位过多，并不适合协议传输，但针对double类型，默认是用f
            str += QString("%1;%2").arg(doubleToString(d[ 0 ].x()), doubleToString(d[ 0 ].y()));
        }
        // 用非科学计数法转换，避免精度的丢失
        for (int i = 1; i < d.size(); ++i) {
            str += QString("|%1;%2").arg(doubleToString(d[ i ].x()), doubleToString(d[ i ].y()));
        }
        return (str);
    }

    case QMetaType::QQuaternion: {
        return (converVariantToBase64String< QQuaternion >(var));
    }

    case QMetaType::QRect: {
        QRect d = var.toRect();
        return (QString("%1;%2;%3;%4").arg(d.x()).arg(d.y()).arg(d.width()).arg(d.height()));
    }

    case QMetaType::QRectF: {
        QRectF d = var.toRectF();
        return (QString("%1;%2;%3;%4").arg(doubleToString(d.x()), doubleToString(d.y()), doubleToString(d.width()), doubleToString(d.height())));
    }

    case QMetaType::QRegularExpression: {
        return (converVariantToBase64String< QRegularExpression >(var));
    }

    case QMetaType::QRegion: {
        return (converVariantToBase64String< QRegion >(var));
    }

    case QMetaType::QSize: {
        QSize d = var.toSize();
        return (QString("%1;%2").arg(d.width()).arg(d.height()));
    }

    case QMetaType::QSizeF: {
        QSizeF d = var.toSizeF();
        return (QString("%1;%2").arg(doubleToString(d.width()), doubleToString(d.height())));
    }

    case QMetaType::QSizePolicy: {
        return (converVariantToBase64String< QSizePolicy >(var));
    }

    case QMetaType::QString: {
        return (var.toString());
    }

    case QMetaType::QStringList: {
        return (converVariantToBase64String< QStringList >(var));
    }

    case QMetaType::QTextFormat: {
        return (converVariantToBase64String< QTextFormat >(var));
    }

    case QMetaType::QTextLength: {
        return (converVariantToBase64String< QTextLength >(var));
    }

    case QMetaType::QTime: {
        return (var.toTime().toString(Qt::ISODate));
    }

    case QMetaType::UInt: {
        return (QString::number(var.toUInt()));
    }

    case QMetaType::ULongLong: {
        return (QString::number(var.toULongLong()));
    }

    case QMetaType::QUrl: {
        return (var.toUrl().toString());
    }

    case QMetaType::QVector2D: {
        return (converVariantToBase64String< QVector2D >(var));
    }

    case QMetaType::QVector3D: {
        return (converVariantToBase64String< QVector3D >(var));
    }

    case QMetaType::QVector4D: {
        return (converVariantToBase64String< QVector4D >(var));
    }

    default:
        return (QString());
    }
    return (QString());
}

QVariant DA::stringToVariant(const QString& var, const QString& typeName)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    int type = QVariant::nameToType(typeName.toLocal8Bit().data());
#else
    int type = QMetaType::fromName(typeName.toLocal8Bit().data()).id();
#endif
    switch (type) {
    case QMetaType::UnknownType:
        return (QVariant());

    case QMetaType::QBitArray:
        return (converBase64StringToVariant< QBitArray >(var));

    case QMetaType::QBitmap:
        return (converBase64StringToVariant< QBitmap >(var));

    case QMetaType::Bool: {
        bool d = var.toInt();
        return (d);
    }

    case QMetaType::QBrush:
        return (converBase64StringToVariant< QBrush >(var));

    case QMetaType::QByteArray:
        return (converBase64StringToVariant< QByteArray >(var));

    case QMetaType::QChar: {
        if (var.size() <= 0) {
            return (QVariant());
        }
        return (QChar(var[ 0 ]));
    }

    case QMetaType::QColor: {
        QColor clr;
        clr.setNamedColor(var);
        return (clr);
    }

    case QMetaType::QCursor:
        return (converBase64StringToVariant< QCursor >(var));

    case QMetaType::QDate: {
        QDate d;
        d.fromString(var, Qt::ISODate);
        return (d);
    }

    case QMetaType::QDateTime: {
        QDateTime d;
        d.fromString(var, Qt::ISODate);
        return (d);
    }

    case QMetaType::Double: {
        double d = var.toDouble();
        return (d);
    }

    case QMetaType::QEasingCurve: {
        return (converBase64StringToVariant< QEasingCurve >(var));
    }

    case QMetaType::QUuid: {
        QUuid d(var);
        return (d);
    }

    case QMetaType::QFont: {
        return (converBase64StringToVariant< QFont >(var));
    }

    case QMetaType::QVariantHash: {
        return (converBase64StringToVariant< QVariantHash >(var));
    }

    case QMetaType::QIcon: {
        return (converBase64StringToVariant< QIcon >(var));
    }

    case QMetaType::QImage: {
        return (converBase64StringToVariant< QImage >(var));
    }

    case QMetaType::Int: {
        return (QString::number(var.toInt()));
    }

    case QMetaType::QKeySequence: {
        QKeySequence d(var, QKeySequence::NativeText);
        return (d);
    }

    case QMetaType::QLine: {
        QStringList list = var.split(';');
        if (list.size() != 4) {
            return (QVariant());
        }
        QLine d(list[ 0 ].toInt(), list[ 1 ].toInt(), list[ 2 ].toInt(), list[ 3 ].toInt());
        return (d);
    }

    case QMetaType::QLineF: {
        QStringList list = var.split(';');
        if (list.size() != 4) {
            return (QVariant());
        }
        QLineF d(list[ 0 ].toDouble(), list[ 1 ].toDouble(), list[ 2 ].toDouble(), list[ 3 ].toDouble());
        return (d);
    }

    case QMetaType::QVariantList: {
        return (converBase64StringToVariant< QVariantList >(var));
    }

    case QMetaType::QLocale: {
        return (QLocale(var));
    }

    case QMetaType::LongLong: {
        return (QString::number(var.toLongLong()));
    }

    case QMetaType::QVariantMap: {
        return (converBase64StringToVariant< QVariantMap >(var));
    }

    case QMetaType::QTransform: {
        QStringList list = var.split(';');
        if (list.size() != 9) {
            return (QVariant());
        }
        QTransform d(list[ 0 ].toDouble(),
                     list[ 1 ].toDouble(),
                     list[ 2 ].toDouble(),
                     list[ 3 ].toDouble(),
                     list[ 4 ].toDouble(),
                     list[ 5 ].toDouble(),
                     list[ 6 ].toDouble(),
                     list[ 7 ].toDouble(),
                     list[ 8 ].toDouble());
        return (d);
    }

    case QMetaType::QMatrix4x4: {
        QStringList list = var.split(';');
        if (list.size() != 16) {
            return (QVariant());
        }
        QMatrix4x4 d(list[ 0 ].toDouble(),
                     list[ 1 ].toDouble(),
                     list[ 2 ].toDouble(),
                     list[ 3 ].toDouble(),
                     list[ 4 ].toDouble(),
                     list[ 5 ].toDouble(),
                     list[ 6 ].toDouble(),
                     list[ 7 ].toDouble(),
                     list[ 8 ].toDouble(),
                     list[ 9 ].toDouble(),
                     list[ 10 ].toDouble(),
                     list[ 11 ].toDouble(),
                     list[ 12 ].toDouble(),
                     list[ 13 ].toDouble(),
                     list[ 14 ].toDouble(),
                     list[ 15 ].toDouble());
        return (d);
    }

    case QMetaType::QPalette: {
        return (converBase64StringToVariant< QPalette >(var));
    }

    case QMetaType::QPen: {
        return (converBase64StringToVariant< QPen >(var));
    }

    case QMetaType::QPixmap: {
        return (converBase64StringToVariant< QPixmap >(var));
    }

    case QMetaType::QPoint: {
        QStringList list = var.split(';');
        if (list.size() != 2) {
            return (QVariant());
        }
        QPoint d(list[ 0 ].toInt(), list[ 1 ].toInt());
        return (d);
    }

    case QMetaType::QPointF: {
        QStringList list = var.split(';');
        if (list.size() != 2) {
            return (QPointF());
        }
        QPointF d(list[ 0 ].toDouble(), list[ 1 ].toDouble());
        return (d);
    }

    case QMetaType::QPolygon: {
        QPolygon d;
        QStringList list = var.split('|');
        if (0 == list.size()) {
            list = var.split(';');
            if (list.size() == 2) {
                d << QPoint(list[ 0 ].toDouble(), list[ 1 ].toDouble());
            }
            return (d);
        }
        for (int i = 0; i < list.size(); ++i) {
            QStringList plist = list[ i ].split(';');
            if (2 == plist.size()) {
                d.append(QPoint(plist[ 0 ].toDouble(), plist[ 1 ].toDouble()));
            }
        }
        return (d);
    }

    case QMetaType::QPolygonF: {
        QStringList list = var.split('|');
        QPolygonF d;
        if (0 == list.size()) {
            list = var.split(';');
            if (list.size() == 2) {
                d << QPointF(list[ 0 ].toDouble(), list[ 1 ].toDouble());
            }
            return (d);
        }
        for (int i = 0; i < list.size(); ++i) {
            QStringList plist = list[ i ].split(';');
            if (2 == plist.size()) {
                d.append(QPointF(plist[ 0 ].toDouble(), plist[ 1 ].toDouble()));
            }
        }
        return (d);
    }

    case QMetaType::QQuaternion: {
        return (converBase64StringToVariant< QQuaternion >(var));
    }

    case QMetaType::QRect: {
        QStringList list = var.split(';');
        if (list.size() != 4) {
            return (QVariant());
        }
        QRect d(list[ 0 ].toInt(), list[ 1 ].toInt(), list[ 2 ].toInt(), list[ 3 ].toInt());
        return (d);
    }

    case QMetaType::QRectF: {
        QStringList list = var.split(';');
        if (list.size() != 4) {
            return (QVariant());
        }
        QRectF d(list[ 0 ].toDouble(), list[ 1 ].toDouble(), list[ 2 ].toDouble(), list[ 3 ].toDouble());
        return (d);
    }

    case QMetaType::QRegularExpression: {
        return (converBase64StringToVariant< QRegularExpression >(var));
    }

    case QMetaType::QRegion: {
        return (converBase64StringToVariant< QRegion >(var));
    }

    case QMetaType::QSize: {
        QStringList list = var.split(';');
        if (list.size() != 2) {
            return (QVariant());
        }
        QSize d(list[ 0 ].toInt(), list[ 1 ].toInt());
        return (d);
    }

    case QMetaType::QSizeF: {
        QStringList list = var.split(';');
        if (list.size() != 2) {
            return (QVariant());
        }
        QSizeF d(list[ 0 ].toDouble(), list[ 1 ].toDouble());
        return (d);
    }

    case QMetaType::QSizePolicy: {
        return (converBase64StringToVariant< QSizePolicy >(var));
    }

    case QMetaType::QString: {
        return (var);
    }

    case QMetaType::QStringList: {
        return (converBase64StringToVariant< QStringList >(var));
    }

    case QMetaType::QTextFormat: {
        return (converBase64StringToVariant< QTextFormat >(var));
    }

    case QMetaType::QTextLength: {
        return (converBase64StringToVariant< QTextLength >(var));
    }

    case QMetaType::QTime: {
        QTime t;
        t.fromString(var, Qt::ISODate);
        return (t);
    }

    case QMetaType::UInt: {
        return (var.toUInt());
    }

    case QMetaType::ULongLong: {
        return (var.toULongLong());
    }

    case QMetaType::QUrl: {
        return (QUrl(var));
    }

    case QMetaType::QVector2D: {
        return (converBase64StringToVariant< QVector2D >(var));
    }

    case QMetaType::QVector3D: {
        return (converBase64StringToVariant< QVector3D >(var));
    }

    case QMetaType::QVector4D: {
        return (converBase64StringToVariant< QVector4D >(var));
    }

    default:
        return (QVariant());
    }
    return (QVariant());
}

QString DA::doubleToString(const double a)
{
    char f        = 'g';
    int precision = 6;

    if (a > 1e6) {
        // 当数据非常大时，精度需要根据大小进行调整
        int tmp = a / 1e6;
        while (tmp / 10 != 0)  // && precision < 16 precision不大于16
        {
            tmp /= 10;
            ++precision;
        }
        // 精度还可以扩充，继续去处理小数位
        // 取出double的小数位
        double decimal = a - floor(a);
        // 把小数转换为最大可处理的整形以便处理
        tmp = decimal * pow(10, 9);
        // 把整形的小位的0去除
        while (tmp)  // precision不大于16
        {
            if (0 != tmp % 10) {
                break;
            }
            tmp /= 10;
        }
        // 把整形剩余位取出，作为精度
        while (tmp / 10 != 0)  // && precision < 16 precision不大于16
        {
            tmp /= 10;
            ++precision;
        }
    } else if ((a < 1e-6) && !qFuzzyCompare(a + 1, 1)) {
        // 当数据非常小时
        // 先把数据上调
        int i     = 0;
        double ta = a;
        while (ta < 1 && i < 308) {
            ta *= 10;
            ++i;  // 防止ta == 0
        }
        precision = 16;
        int tmp   = ta * 1e16;
        // 把整形的小位的0去除
        while (tmp && precision > 6)  // precision不小于6
        {
            if (0 == tmp % 10) {
                --precision;
            }
            tmp /= 10;
        }
    }
    return (QString::number(a, f, precision));
}
