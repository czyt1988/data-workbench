#ifndef DASPLASHSCREEN_H
#define DASPLASHSCREEN_H
#include "DAGuiAPI.h"
#include <QSplashScreen>
namespace DA
{

/**
 * @brief 自定义启动画面
 *
 * 支持自定义背景图片、底部加载状态文字显示、版本信息显示。
 * 若未指定背景图片，将使用内置默认背景（通过QPainter程序化绘制）。
 *
 * 使用示例：
 * @code
 * DASplashScreen splash;
 * splash.show();
 * splash.showMessage("正在初始化核心组件...");
 * // ... 执行初始化 ...
 * splash.showMessage("启动完成");
 * splash.finish(&mainWindow);
 * @endcode
 */
class DAGUI_API DASplashScreen : public QSplashScreen
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DASplashScreen)
public:
    DASplashScreen();
    explicit DASplashScreen(const QPixmap& pixmap);
    ~DASplashScreen();

    // 设置背景图片
    void setBackgroundPixmap(const QPixmap& pixmap);

    // 从文件路径加载背景图片
    bool loadBackgroundPixmap(const QString& filePath);

    // 设置消息文字颜色
    void setMessageColor(const QColor& color);
    QColor messageColor() const;

    // 设置版本文字
    void setVersionText(const QString& text);
    QString versionText() const;

    // 设置标题文字
    void setTitleText(const QString& text);
    QString titleText() const;

    // 显示加载消息（便捷方法，始终居底部显示）
    void showMessage(const QString& message);

    // 检测是否有调试器附加（调试时不置顶启动画面）
    static bool isDebuggerPresent();

    // 默认启动画面尺寸
    static QSize defaultSize();

    // 生成默认背景图片
    static QPixmap generateDefaultPixmap(const QSize& size = defaultSize());

protected:
    void drawContents(QPainter* painter) override;
};

}  // end namespace DA
#endif  // DASPLASHSCREEN_H
