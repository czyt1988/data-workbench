#include "DASplashScreen.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QLinearGradient>
#include <QFont>
#include <QFileInfo>
#include <QDebug>
#include "DAConfigs.h"

namespace DA
{

//===================================================
// DASplashScreen::PrivateData
//===================================================

class DASplashScreen::PrivateData
{
    DA_DECLARE_PUBLIC(DASplashScreen)
public:
    PrivateData(DASplashScreen* p);

    QColor m_messageColor { Qt::white };
    QString m_currentMessage;
    QString m_versionText;
    QString m_titleText { QStringLiteral("DA WorkBench") };
};

DASplashScreen::PrivateData::PrivateData(DASplashScreen* p) : q_ptr(p)
{
    m_versionText = QString("V %1").arg(DA_VERSION);
}

//===================================================
// DASplashScreen
//===================================================

/**
 * @brief 构造函数，使用默认背景
 *
 * 创建一个使用程序化绘制的默认背景的启动画面
 */
DASplashScreen::DASplashScreen() : QSplashScreen(generateDefaultPixmap()), DA_PIMPL_CONSTRUCT
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

/**
 * @brief 构造函数，使用自定义背景图片
 *
 * @param[in] pixmap 自定义背景图片
 */
DASplashScreen::DASplashScreen(const QPixmap& pixmap)
    : QSplashScreen(pixmap.isNull() ? generateDefaultPixmap() : pixmap), DA_PIMPL_CONSTRUCT
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

DASplashScreen::~DASplashScreen()
{
}

/**
 * @brief 设置背景图片
 *
 * @param[in] pixmap 新的背景图片，若为空则使用默认背景
 */
void DASplashScreen::setBackgroundPixmap(const QPixmap& pixmap)
{
    if (pixmap.isNull()) {
        setPixmap(generateDefaultPixmap());
    } else {
        setPixmap(pixmap);
    }
}

/**
 * @brief 从文件路径加载背景图片
 *
 * @param[in] filePath 图片文件路径
 * @return 加载成功返回true，失败返回false
 */
bool DASplashScreen::loadBackgroundPixmap(const QString& filePath)
{
    if (!QFileInfo::exists(filePath)) {
        qWarning() << "DASplashScreen: background image file not found:" << filePath;
        return false;
    }
    QPixmap pix(filePath);
    if (pix.isNull()) {
        qWarning() << "DASplashScreen: failed to load background image:" << filePath;
        return false;
    }
    setPixmap(pix);
    return true;
}

/**
 * @brief 设置消息文字颜色
 *
 * @param[in] color 文字颜色
 */
void DASplashScreen::setMessageColor(const QColor& color)
{
    DA_D(d);
    d->m_messageColor = color;
}

/**
 * @brief 获取消息文字颜色
 *
 * @return 当前消息文字颜色
 */
QColor DASplashScreen::messageColor() const
{
    DA_DC(d);
    return d->m_messageColor;
}

/**
 * @brief 设置版本文字
 *
 * @param[in] text 版本文字，如 "V 0.0.2"
 */
void DASplashScreen::setVersionText(const QString& text)
{
    DA_D(d);
    d->m_versionText = text;
    repaint();
}

/**
 * @brief 获取版本文字
 *
 * @return 当前版本文字
 */
QString DASplashScreen::versionText() const
{
    DA_DC(d);
    return d->m_versionText;
}

/**
 * @brief 设置标题文字
 *
 * @param[in] text 标题文字
 */
void DASplashScreen::setTitleText(const QString& text)
{
    DA_D(d);
    d->m_titleText = text;
    repaint();
}

/**
 * @brief 获取标题文字
 *
 * @return 当前标题文字
 */
QString DASplashScreen::titleText() const
{
    DA_DC(d);
    return d->m_titleText;
}

/**
 * @brief 显示加载消息
 *
 * 在启动画面底部显示加载状态信息，同时处理事件循环以确保界面更新
 *
 * @param[in] message 要显示的消息文字
 */
void DASplashScreen::showMessage(const QString& message)
{
    DA_D(d);
    d->m_currentMessage = message;
    // 调用基类方法，位置设为底部居中，颜色为设定颜色
    QSplashScreen::showMessage(message, Qt::AlignBottom | Qt::AlignHCenter, d->m_messageColor);
    QApplication::processEvents();
}

/**
 * @brief 获取默认启动画面尺寸
 *
 * @return 默认尺寸 (700x420)
 */
QSize DASplashScreen::defaultSize()
{
    return QSize(700, 420);
}

/**
 * @brief 生成默认背景图片
 *
 * 使用QPainter程序化绘制一个现代风格的启动画面背景
 *
 * @param[in] size 图片尺寸
 * @return 生成的背景图片
 */
QPixmap DASplashScreen::generateDefaultPixmap(const QSize& size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    int w = size.width();
    int h = size.height();

    // 1. 绘制深蓝色渐变背景
    QLinearGradient bgGradient(0, 0, w, h);
    bgGradient.setColorAt(0.0, QColor(26, 35, 126));  // #1a237e
    bgGradient.setColorAt(0.5, QColor(13, 27, 62));   // #0d1b3e
    bgGradient.setColorAt(1.0, QColor(10, 15, 40));   // 深色
    painter.fillRect(0, 0, w, h, bgGradient);

    // 2. 绘制装饰性网格线（代表数据流/工作流连接）
    QPen gridPen(QColor(60, 80, 160, 30), 1);
    painter.setPen(gridPen);
    for (int x = 0; x < w; x += 40) {
        painter.drawLine(x, 0, x, h);
    }
    for (int y = 0; y < h; y += 40) {
        painter.drawLine(0, y, w, y);
    }

    // 3. 绘制装饰性节点圆点
    painter.setPen(Qt::NoPen);
    // 随机分布的节点点
    struct NodePoint
    {
        int x, y, r;
        int alpha;
    };
    const NodePoint nodes[] = { { 120, 80, 4, 60 },  { 280, 120, 3, 40 }, { 450, 60, 5, 50 },  { 580, 140, 3, 35 },
                                { 100, 200, 4, 45 }, { 350, 180, 6, 55 }, { 600, 220, 4, 40 }, { 180, 300, 3, 30 },
                                { 500, 280, 5, 50 }, { 650, 100, 3, 35 }, { 50, 350, 4, 25 },  { 400, 340, 3, 30 } };

    for (const auto& node : nodes) {
        painter.setBrush(QColor(100, 140, 230, node.alpha));
        painter.drawEllipse(QPoint(node.x, node.y), node.r, node.r);
    }

    // 4. 绘制节点之间的连接线
    QPen linkPen(QColor(80, 120, 200, 25), 1);
    painter.setPen(linkPen);
    painter.drawLine(120, 80, 280, 120);
    painter.drawLine(280, 120, 450, 60);
    painter.drawLine(450, 60, 580, 140);
    painter.drawLine(100, 200, 350, 180);
    painter.drawLine(350, 180, 600, 220);
    painter.drawLine(180, 300, 500, 280);

    // 5. 绘制底部半透明渐变条（用于显示消息文字）
    QLinearGradient bottomGradient(0, h - 60, 0, h);
    bottomGradient.setColorAt(0.0, QColor(0, 0, 0, 0));
    bottomGradient.setColorAt(1.0, QColor(0, 0, 0, 120));
    painter.setPen(Qt::NoPen);
    painter.setBrush(bottomGradient);
    painter.drawRect(0, h - 60, w, 60);

    // 6. 绘制顶部装饰线
    QLinearGradient topLine(0, 0, w, 0);
    topLine.setColorAt(0.0, QColor(64, 196, 255, 0));
    topLine.setColorAt(0.3, QColor(64, 196, 255, 200));
    topLine.setColorAt(0.7, QColor(64, 196, 255, 200));
    topLine.setColorAt(1.0, QColor(64, 196, 255, 0));
    painter.setPen(QPen(QBrush(topLine), 2));
    painter.drawLine(0, 0, w, 0);

    painter.end();
    return pixmap;
}

/**
 * @brief 重绘启动画面内容
 *
 * 在背景图片上绘制标题、版本号和加载状态消息
 *
 * @param[in] painter 绘图器指针
 */
void DASplashScreen::drawContents(QPainter* painter)
{
    DA_DC(d);
    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);

    QRect cr = contentsRect();
    int w    = cr.width();
    int h    = cr.height();

    // 1. 绘制标题
    if (!d->m_titleText.isEmpty()) {
        QFont titleFont = painter->font();
        titleFont.setPixelSize(36);
        titleFont.setBold(true);
        titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        painter->setFont(titleFont);
        painter->setPen(QColor(255, 255, 255, 230));

        QRect titleRect(0, h / 3 - 30, w, 50);
        painter->drawText(titleRect, Qt::AlignCenter, d->m_titleText);
    }

    // 2. 绘制版本号
    if (!d->m_versionText.isEmpty()) {
        QFont verFont = painter->font();
        verFont.setPixelSize(14);
        verFont.setBold(false);
        painter->setFont(verFont);
        painter->setPen(QColor(180, 200, 240, 180));

        QRect verRect(0, h / 3 + 25, w, 24);
        painter->drawText(verRect, Qt::AlignCenter, d->m_versionText);
    }

    // 3. 绘制底部加载消息
    if (!d->m_currentMessage.isEmpty()) {
        QFont msgFont = painter->font();
        msgFont.setPixelSize(13);
        msgFont.setBold(false);
        painter->setFont(msgFont);
        painter->setPen(d->m_messageColor);

        QRect msgRect(20, h - 35, w - 40, 25);
        painter->drawText(msgRect, Qt::AlignHCenter | Qt::AlignVCenter, d->m_currentMessage);
    }

    painter->restore();
}

}  // end namespace DA
