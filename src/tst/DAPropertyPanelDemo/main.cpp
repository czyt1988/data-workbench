/**
 * @brief DAPropertyPanelWidget 属性面板演示程序
 *
 * 展示全部 12 种属性类型的布局效果，用于验证 Inline/Below 两种布局模式下
 * 编辑器控件的水平拉伸行为是否正确。
 */

#include "DAPropertyPanelWidget.h"
#include <QApplication>
#include <QMainWindow>
#include <QScrollArea>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DAPropertyPanelDemo"));

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("DAPropertyPanelWidget 属性面板演示"));

    // 属性面板
    DA::DAPropertyPanelWidget* panel = new DA::DAPropertyPanelWidget(&window);

    // 信号转发验证
    QObject::connect(panel, &DA::DAPropertyPanelWidget::propertyValueChanged,
                     [](int id) { qDebug() << "propertyValueChanged, id =" << id; });

    // === 根级别属性（Inline 布局）===
    panel->addIntProperty("整数属性", 42, 0, 100);
    panel->addDoubleProperty("浮点属性", 3.14, 0.0, 10.0, 2);
    panel->addBoolProperty("布尔属性", true);
    panel->addStringProperty("字符串属性", "Hello World");
    panel->addEnumProperty("枚举属性",
                           QStringList{ "选项A", "选项B", "选项C", "选项D" },
                           QList< int >{ 10, 20, 30, 40 },
                           0);
    panel->addColorProperty("颜色属性", QColor(255, 0, 0));
    panel->addAlignmentProperty("对齐属性", Qt::AlignCenter);
    panel->addAlignmentPositionProperty("对齐位置属性", Qt::AlignRight | Qt::AlignVCenter);
    panel->addFilePathProperty("文件路径", "All Files (*.*)");

    // 分隔线
    panel->addSeparator();

    // === 折叠分组：字体与画笔（Below 布局）===
    panel->addCollapsibleGroup("字体与画笔");
    QFont font("Microsoft YaHei", 10);
    font.setBold(true);
    panel->addFontProperty("字体属性", font);
    panel->addBrushProperty("画刷属性", QBrush(QColor(0, 128, 255)));
    panel->addPenProperty("画笔属性", QPen(Qt::black, 2, Qt::DashLine));
    panel->endGroup();

    // === 折叠分组：更多属性 ===
    panel->addCollapsibleGroup("更多属性");
    panel->addIntProperty("线宽", 1, 1, 20);
    panel->addDoubleProperty("透明度", 0.8, 0.0, 1.0, 2);
    panel->addEnumProperty("线型",
                           QStringList{ "实线", "虚线", "点线", "点划线" },
                           QList< int >{ 0, 1, 2, 3 },
                           0);
    panel->addColorProperty("背景色", QColor(240, 240, 240));
    panel->addStringProperty("备注", "");
    panel->addBoolProperty("启用", true);
    panel->endGroup();

    // 末尾间距
    panel->addSpacer(16);

    // 滚动区域包裹
    QScrollArea* scroll = new QScrollArea(&window);
    scroll->setWidget(panel);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    window.setCentralWidget(scroll);

    window.resize(420, 800);
    window.show();

    return app.exec();
}
