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
    window.setWindowTitle(QObject::tr("DAPropertyPanelWidget Property Panel Demo"));  //cn:DAPropertyPanelWidget 属性面板演示

    // 属性面板
    DA::DAPropertyPanelWidget* panel = new DA::DAPropertyPanelWidget(&window);

    // 信号转发验证
    QObject::connect(panel, &DA::DAPropertyPanelWidget::propertyValueChanged,
                     [](int id) { qDebug() << "propertyValueChanged, id =" << id; });

    // === 根级别属性（Inline 布局）===
    panel->addIntProperty(QObject::tr("Integer Property"), 42, 0, 100);  //cn:整数属性
    panel->addDoubleProperty(QObject::tr("Double Property"), 3.14, 0.0, 10.0, 2);  //cn:浮点属性
    panel->addBoolProperty(QObject::tr("Boolean Property"), true);  //cn:布尔属性
    panel->addStringProperty(QObject::tr("String Property"), "Hello World");  //cn:字符串属性
    panel->addEnumProperty(QObject::tr("Enum Property"),  //cn:枚举属性
                           QStringList{ QObject::tr("Option A"), QObject::tr("Option B"), QObject::tr("Option C"), QObject::tr("Option D") },  //cn:选项A,选项B,选项C,选项D
                           QList< int >{ 10, 20, 30, 40 },
                           0);
    panel->addColorProperty(QObject::tr("Color Property"), QColor(255, 0, 0));  //cn:颜色属性
    panel->addAlignmentProperty(QObject::tr("Alignment Property"), Qt::AlignCenter);  //cn:对齐属性
    panel->addAlignmentPositionProperty(QObject::tr("Alignment Position Property"), Qt::AlignRight | Qt::AlignVCenter);  //cn:对齐位置属性
    panel->addFilePathProperty(QObject::tr("File Path"), "All Files (*.*)");  //cn:文件路径

    // 分隔线
    panel->addSeparator();

    // === 折叠分组：字体与画笔（Below 布局）===
    panel->addCollapsibleGroup(QObject::tr("Font & Brush"));  //cn:字体与画笔
    QFont font("Microsoft YaHei", 10);
    font.setBold(true);
    panel->addFontProperty(QObject::tr("Font Property"), font);  //cn:字体属性
    panel->addBrushProperty(QObject::tr("Brush Property"), QBrush(QColor(0, 128, 255)));  //cn:画刷属性
    panel->addPenProperty(QObject::tr("Pen Property"), QPen(Qt::black, 2, Qt::DashLine));  //cn:画笔属性
    panel->endGroup();

    // === 折叠分组：更多属性 ===
    panel->addCollapsibleGroup(QObject::tr("More Properties"));  //cn:更多属性
    panel->addIntProperty(QObject::tr("Line Width"), 1, 1, 20);  //cn:线宽
    panel->addDoubleProperty(QObject::tr("Opacity"), 0.8, 0.0, 1.0, 2);  //cn:透明度
    panel->addEnumProperty(QObject::tr("Line Style"),  //cn:线型
                           QStringList{ QObject::tr("Solid"), QObject::tr("Dash"), QObject::tr("Dot"), QObject::tr("Dash Dot") },  //cn:实线,虚线,点线,点划线
                           QList< int >{ 0, 1, 2, 3 },
                           0);
    panel->addColorProperty(QObject::tr("Background Color"), QColor(240, 240, 240));  //cn:背景色
    panel->addStringProperty(QObject::tr("Remark"), "");  //cn:备注
    panel->addBoolProperty(QObject::tr("Enable"), true);  //cn:启用
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
