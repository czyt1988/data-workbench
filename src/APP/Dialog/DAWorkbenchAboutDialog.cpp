#include "DAWorkbenchAboutDialog.h"
#include "ui_DAWorkbenchAboutDialog.h"
#include <QIcon>
#include <QPalette>
#include <QStringList>
#include <QVector>
#include "SARibbonGlobal.h"
#include "spdlog/version.h"
#include "qwt_global.h"
#include "DAGlobals.h"
// 通过此宏引入pybind11，避免qt中slot关键字冲突
#include "DAPybind11InQt.h"

namespace
{
/// 主题色，与图标/UI设计规范的主蓝保持一致
const QColor s_brandBlue(0x52, 0x80, 0xC1);

/// 第三方库信息
struct DALibraryInfo
{
    QString name;     ///< 库名称
    QString usage;    ///< 用途
    QString license;  ///< 协议
    QString version;  ///< 版本号，未知时留空
};

/**
 * @brief 生成超链接
 */
QString daAnchor(const QString& url, const QString& text)
{
    return (QStringLiteral("<a href=\"%1\" style=\"color:%2;text-decoration:none;\">%3</a>")
                .arg(url, s_brandBlue.name(), text));
}

/**
 * @brief 生成章节标题
 */
QString daSectionTitle(const QString& text)
{
    return (QStringLiteral(
                "<p style=\"margin-top:16px;margin-bottom:8px;\"><span style=\"font-size:14px;font-weight:bold;color:%1;\">%2</span></p>")
                .arg(s_brandBlue.name(), text));
}

/**
 * @brief 把库信息列表渲染为表格，表头列数由headers决定（3列无版本，4列带版本）
 */
QString daLibraryTable(const QVector< DALibraryInfo >& libs, const QStringList& headers, const QColor& zebraColor)
{
    const bool showVersion = headers.size() > 3;
    QString html;
    html += QStringLiteral("<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\" border=\"0\">");
    // 表头
    html += QStringLiteral("<tr>");
    html += QStringLiteral("<td width=\"170\" style=\"background-color:%1;color:#FFFFFF;\"><b>%2</b></td>")
                .arg(s_brandBlue.name(), headers.value(0));
    html += QStringLiteral("<td style=\"background-color:%1;color:#FFFFFF;\"><b>%2</b></td>")
                .arg(s_brandBlue.name(), headers.value(1));
    html += QStringLiteral("<td width=\"150\" style=\"background-color:%1;color:#FFFFFF;\"><b>%2</b></td>")
                .arg(s_brandBlue.name(), headers.value(2));
    if (showVersion) {
        html += QStringLiteral("<td width=\"90\" style=\"background-color:%1;color:#FFFFFF;\"><b>%2</b></td>")
                    .arg(s_brandBlue.name(), headers.value(3));
    }
    html += QStringLiteral("</tr>");
    // 数据行，隔行换色
    for (int i = 0; i < libs.size(); ++i) {
        const DALibraryInfo& lib = libs.at(i);
        QString cellStyle;
        if (i % 2 == 1) {
            cellStyle = QStringLiteral("background-color:%1;").arg(zebraColor.name());
        }
        html += QStringLiteral("<tr>");
        html += QStringLiteral("<td style=\"%1\"><b>%2</b></td>").arg(cellStyle, lib.name.toHtmlEscaped());
        html += QStringLiteral("<td style=\"%1\">%2</td>").arg(cellStyle, lib.usage);
        html += QStringLiteral("<td style=\"%1\">%2</td>").arg(cellStyle, lib.license);
        if (showVersion) {
            html += QStringLiteral("<td style=\"%1\">%2</td>")
                        .arg(cellStyle, lib.version.isEmpty() ? QStringLiteral("-") : lib.version);
        }
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");
    return html;
}
}  // namespace

namespace DA
{
/**
 * @brief 构造函数
 * @param parent 父窗口
 */
DAWorkbenchAboutDialog::DAWorkbenchAboutDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::DAWorkbenchAboutDialog)
{
    ui->setupUi(this);
    setupUiStyle();
    makeAboutInfo();
}

DAWorkbenchAboutDialog::~DAWorkbenchAboutDialog()
{
    delete ui;
}

/**
 * @brief 设置标题区、正文区的显示样式
 */
void DAWorkbenchAboutDialog::setupUiStyle()
{
    // 应用logo
    ui->labelLogo->setPixmap(QIcon(QStringLiteral(":/app/bright/Icon/icon.svg")).pixmap(64, 64));
    // 标题放大加粗，使用主题色
    QFont titleFont = ui->labelTitle->font();
    titleFont.setPointSize(titleFont.pointSize() + 7);
    titleFont.setBold(true);
    ui->labelTitle->setFont(titleFont);
    ui->labelTitle->setStyleSheet(QStringLiteral("color: %1;").arg(s_brandBlue.name()));
    // 版本号和简介使用次级文字色
    const QColor dimColor = palette().color(QPalette::Disabled, QPalette::WindowText);
    ui->labelVersion->setStyleSheet(QStringLiteral("color: %1;").arg(dimColor.name()));
    ui->labelDescription->setStyleSheet(QStringLiteral("color: %1;").arg(dimColor.name()));
    // 标题区底部增加分隔线
    ui->frameHeader->setStyleSheet(
        QStringLiteral("#frameHeader { border-bottom: 1px solid %1; }").arg(palette().color(QPalette::Mid).name()));
    // 正文区内边距
    ui->textBrowser->document()->setDocumentMargin(20);
}

/**
 * @brief 生成关于信息，包含软件简介、C++第三方库和Python依赖清单
 */
void DAWorkbenchAboutDialog::makeAboutInfo()
{
    ui->labelVersion->setText(tr("Version %1.%2.%3")  // cn:版本 %1.%2.%3
                                  .arg(DA_VERSION_MAJOR)
                                  .arg(DA_VERSION_MINOR)
                                  .arg(DA_VERSION_PATCH));
    ui->labelDescription->setText(tr("AI Agent driven data analysis workbench"));  // cn:AI Agent 驱动的数据分析工作平台
    ui->labelCopyright->setText(QStringLiteral("Copyright (C) 2024-2026 czyt1988"));

    QString html;
    html += makeOverviewSection();
    html += makeCppLibrarySection();
    html += makePythonLibrarySection();
    html += QStringLiteral("<p style=\"margin-top:16px;color:%1;\">%2</p>")
                .arg(dimTextColor().name(), tr("The third-party libraries listed above retain their original licenses, "
                                               "please refer to the corresponding projects for details."));  // cn:以上第三方库保留其原有协议，详见对应项目说明
    ui->textBrowser->setHtml(html);
}

/**
 * @brief 次级文字颜色
 */
QColor DAWorkbenchAboutDialog::dimTextColor() const
{
    return palette().color(QPalette::Disabled, QPalette::WindowText);
}

/**
 * @brief 生成软件简介章节
 */
QString DAWorkbenchAboutDialog::makeOverviewSection() const
{
    QString html;
    html += QStringLiteral("<p style=\"margin-top:0;\">");
    html += tr("DAWorkbench is an AI Agent driven data analysis workbench built on C++17/Qt, "
               "featuring a directed-graph workflow engine, embedded Python (pandas/numpy) data processing, "
               "interactive publication-grade charting, and a plugin architecture supporting both C++ and Python "
               "extensions.");
    // cn:DAWorkbench是一款AI Agent驱动的数据分析工作平台，基于C++17/Qt构建，内置有向图工作流引擎、内嵌Python（pandas/numpy）数据处理、可交互的论文级图表绘制，并支持C++与Python插件扩展。
    html += QStringLiteral("</p>");
    html += QStringLiteral("<p>%1</p>")
                .arg(tr("This software is open source under the LGPL v3.0 license."));  // cn:本软件以LGPL v3.0协议开源。
    html += QStringLiteral("<p style=\"margin-bottom:0;\">");
    html += tr("Project homepage: %1")  // cn:项目主页：%1
        .arg(daAnchor("https://github.com/czyt1988/data-workbench", "github.com/czyt1988/data-workbench"));
    html += QStringLiteral("<br>");
    html += tr("Documentation: %1")  // cn:项目文档：%1
        .arg(daAnchor("https://czyt1988.github.io/data-workbench", "czyt1988.github.io/data-workbench"));
    html += QStringLiteral("<br>");
    html += tr("Contact email: %1").arg(daAnchor("mailto:czy.t@163.com", "czy.t@163.com"));  // cn:联系邮箱：%1
    html += QStringLiteral("</p>");
    return html;
}

/**
 * @brief 生成C++第三方库章节
 */
QString DAWorkbenchAboutDialog::makeCppLibrarySection() const
{
    QString html = daSectionTitle(tr("C++ Third-Party Libraries"));  // cn:C++第三方库
    QVector< DALibraryInfo > libs;
    libs.append({ QStringLiteral("Qt"),
                  tr("Application and UI framework"),  // cn:应用与界面框架
                  QStringLiteral("LGPL v3"),
                  QString::fromLatin1(qVersion()) });
    libs.append({ QStringLiteral("SARibbon"),
                  tr("Ribbon style main window framework"),  // cn:Ribbon风格主窗口框架
                  QStringLiteral("MIT"),
                  QStringLiteral("%1.%2.%3")
                      .arg(SA_RIBBON_BAR_VERSION_MAJ)
                      .arg(SA_RIBBON_BAR_VERSION_MIN)
                      .arg(SA_RIBBON_BAR_VERSION_PAT) });
    libs.append({ QStringLiteral("Qt-Advanced-Docking-System"),
                  tr("Advanced docking system"),  // cn:停靠窗口系统
                  QStringLiteral("LGPL v2.1"),
                  QStringLiteral("4.4.1") });
    libs.append({ QStringLiteral("Qwt"),
                  tr("Plotting engine (maintained fork with QwtFigure extensions)"),  // cn:绘图引擎（带QwtFigure扩展的维护分支）
                  QStringLiteral("Qwt License (LGPL v2.1 with exceptions)"),
                  QStringLiteral(QWT_VERSION_STR) });
    libs.append({ QStringLiteral("spdlog"),
                  tr("Logging library"),  // cn:日志库
                  QStringLiteral("MIT"),
                  QStringLiteral("%1.%2.%3").arg(SPDLOG_VER_MAJOR).arg(SPDLOG_VER_MINOR).arg(SPDLOG_VER_PATCH) });
    libs.append({ QStringLiteral("pybind11"),
                  tr("Seamless C++/Python interoperability"),  // cn:C++/Python互操作
                  QStringLiteral("BSD-3-Clause"),
                  QStringLiteral("%1.%2.%3")
                      .arg(PYBIND11_VERSION_MAJOR)
                      .arg(PYBIND11_VERSION_MINOR)
                      .arg(PYBIND11_VERSION_MICRO) });
    libs.append({ QStringLiteral("QuaZip"),
                  tr("ZIP archive reading/writing"),  // cn:ZIP压缩文件读写
                  QStringLiteral("LGPL v2.1"),
                  QStringLiteral("1.7.1") });
    libs.append({ QStringLiteral("zlib"),
                  tr("Data compression"),  // cn:数据压缩
                  QStringLiteral("zlib License"),
                  QStringLiteral("1.3.1") });
    libs.append({ QStringLiteral("tessil/ordered-map"),
                  tr("Ordered hash map"),  // cn:有序哈希表
                  QStringLiteral("MIT"),
                  QString() });
    libs.append({ QStringLiteral("liteCtk"),
                  tr("Lightweight CTK widget set"),  // cn:轻量CTK控件集
                  QStringLiteral("Apache-2.0"),
                  QString() });
    libs.append({ QStringLiteral("DAWidgets"),
                  tr("General purpose widgets"),  // cn:通用控件库
                  QStringLiteral("MIT"),
                  QString() });
    QStringList headers;
    headers << tr("Name")     // cn:名称
            << tr("Usage")    // cn:用途
            << tr("License")  // cn:协议
            << tr("Version"); // cn:版本
    html += daLibraryTable(libs, headers, zebraColor());
    return html;
}

/**
 * @brief 生成Python依赖章节
 */
QString DAWorkbenchAboutDialog::makePythonLibrarySection() const
{
    QString html = daSectionTitle(tr("Python Dependencies"));  // cn:Python依赖
    html += QStringLiteral("<p style=\"margin-top:0;\">%1</p>")
                .arg(tr("Embedded Python interpreter: %1")  // cn:内嵌Python解释器：%1
                         .arg(QString::fromLocal8Bit(Py_GetVersion())));
    QVector< DALibraryInfo > libs;
    libs.append({ QStringLiteral("pandas"), tr("Core data analysis"), QStringLiteral("BSD-3-Clause"), QString() });  // cn:数据分析核心
    libs.append({ QStringLiteral("numpy"), tr("Numerical computing"), QStringLiteral("BSD-3-Clause"), QString() });  // cn:数值计算
    libs.append({ QStringLiteral("scipy"), tr("Scientific computing"), QStringLiteral("BSD-3-Clause"), QString() });  // cn:科学计算
    libs.append({ QStringLiteral("openpyxl"), tr("Excel file reading/writing"), QStringLiteral("MIT"), QString() });  // cn:Excel文件读写
    libs.append({ QStringLiteral("chardet"), tr("Text encoding detection"), QStringLiteral("LGPL v2.1"), QString() });  // cn:文本编码探测
    libs.append({ QStringLiteral("pyarrow"), tr("Arrow/Parquet data format"), QStringLiteral("Apache-2.0"), QString() });  // cn:Arrow/Parquet数据格式
    libs.append({ QStringLiteral("matplotlib"), tr("Plotting foundation"), QStringLiteral("matplotlib License"), QString() });  // cn:绘图基础
    libs.append({ QStringLiteral("seaborn"), tr("Statistical plotting"), QStringLiteral("BSD-3-Clause"), QString() });  // cn:统计绘图
    libs.append({ QStringLiteral("PyWavelets"), tr("Wavelet analysis"), QStringLiteral("MIT"), QString() });  // cn:小波分析
    libs.append({ QStringLiteral("langgraph"), tr("AI Agent orchestration framework"), QStringLiteral("MIT"), QString() });  // cn:AI Agent编排框架
    libs.append({ QStringLiteral("langchain-openai"), tr("LLM service integration"), QStringLiteral("MIT"), QString() });  // cn:LLM服务接入
    libs.append({ QStringLiteral("langgraph-cli"), tr("LangGraph command line tools"), QStringLiteral("MIT"), QString() });  // cn:LangGraph命令行工具
    libs.append({ QStringLiteral("pydantic"), tr("Data model validation"), QStringLiteral("MIT"), QString() });  // cn:数据模型校验
    libs.append({ QStringLiteral("tiktoken"), tr("Token counting"), QStringLiteral("MIT"), QString() });  // cn:token计数
    libs.append({ QStringLiteral("loguru"), tr("Python logging"), QStringLiteral("MIT"), QString() });  // cn:日志输出
    libs.append({ QStringLiteral("typing_extensions"), tr("Type hint backports"), QStringLiteral("PSF License"), QString() });  // cn:类型标注兼容
    QStringList headers;
    headers << tr("Name")     // cn:名称
            << tr("Usage")    // cn:用途
            << tr("License"); // cn:协议
    html += daLibraryTable(libs, headers, zebraColor());
    return html;
}

/**
 * @brief 表格隔行背景色
 */
QColor DAWorkbenchAboutDialog::zebraColor() const
{
    return palette().color(QPalette::Base).darker(104);
}
}  // namespace DA
