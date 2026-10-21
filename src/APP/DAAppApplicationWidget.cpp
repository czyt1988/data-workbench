#include "DAAppApplicationWidget.h"
#include "DAAppActions.h"
#include "DAAppCore.h"
#include "DAAppProject.h"
#include "DARecentFilesManager.h"
#include "SARibbonMainWindow.h"
#include <QAction>
#include <QDateTime>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>

namespace DA
{
/**
 * @brief 面板页索引
 */
enum DAAppApplicationWidgetPage
{
    PageOpen = 0,  ///< 打开工程页
    PageNew,       ///< 新建工程页
    PageImport,    ///< 导入数据页
    PageSaveAs,    ///< 另存为页
    PageCount
};

/**
 * @brief 面板样式表
 *
 * 颜色取自项目官方色板（DESIGN.md）：主蓝 #5280C1（选中边、主操作）、浅蓝 #BDCDE8（hover/选中底）、中灰 #727272（次要描边）
 */
static const char* kAppWidgetStyleSheet = R"(
QToolButton#navBtn {
    border: none;
    border-radius: 4px;
    padding: 8px 10px;
}
QToolButton#navBtn:hover {
    background: rgba(189, 205, 232, 0.55);
}
QToolButton#navBtn:checked {
    background: rgba(189, 205, 232, 0.9);
    border-left: 3px solid #5280C1;
}
QToolButton#primaryBtn {
    border: 1px solid #5280C1;
    border-radius: 4px;
    padding: 10px 18px;
    color: #5280C1;
}
QToolButton#primaryBtn:hover {
    background: rgba(82, 128, 193, 0.15);
}
QToolButton#secondaryBtn {
    border: 1px solid #727272;
    border-radius: 4px;
    padding: 6px 12px;
}
QToolButton#secondaryBtn:hover {
    background: rgba(189, 205, 232, 0.45);
}
QListWidget {
    border: none;
    background: transparent;
}
QListWidget::item {
    border-radius: 4px;
}
QListWidget::item:hover {
    background: rgba(189, 205, 232, 0.45);
}
)";

class DAAppApplicationWidget::PrivateData
{
    DA_DECLARE_PUBLIC(DAAppApplicationWidget)
public:
    explicit PrivateData(DAAppApplicationWidget* p);
    void buildUI();
    QWidget* buildNavWidget();
    QWidget* buildPageOpen();
    QWidget* buildPageNew();
    QWidget* buildPageImport();
    QWidget* buildPageSaveAs();
    QToolButton* makeNavButton(const QIcon& icon);
    QToolButton* makePrimaryButton(const QIcon& icon);
    QToolButton* makeSecondaryButton(const QIcon& icon);
    QWidget* makeRecentItemWidget(const QString& filePath);
    void makeTitle(QLabel* label);
    void refreshRecentProjects();
    void refreshSaveAsPage();
    void onRecentItemClicked(QListWidgetItem* item);
    void triggerActionAndHide(QAction* act);
    void exitApp();

    DAAppActions* actions { nullptr };
    QStackedWidget* stacked { nullptr };
    // 导航页签
    QToolButton* btnOpenPage { nullptr };
    QToolButton* btnNewPage { nullptr };
    QToolButton* btnImportPage { nullptr };
    QToolButton* btnSaveAsPage { nullptr };
    // 底部动作
    QToolButton* btnSave { nullptr };
    QToolButton* btnSetting { nullptr };
    QToolButton* btnAbout { nullptr };
    QToolButton* btnExit { nullptr };
    // 打开工程页
    QLabel* labelOpenTitle { nullptr };
    QToolButton* btnBrowseProject { nullptr };
    QLabel* labelRecentTitle { nullptr };
    QListWidget* recentList { nullptr };
    QLabel* recentEmptyHint { nullptr };
    QToolButton* btnAppendProject { nullptr };
    QToolButton* btnOpenMarkdown { nullptr };
    QToolButton* btnClearRecent { nullptr };
    // 新建工程页
    QLabel* labelNewTitle { nullptr };
    QLabel* labelNewDescription { nullptr };
    QToolButton* btnCreateProject { nullptr };
    // 导入数据页
    QLabel* labelImportTitle { nullptr };
    QLabel* labelImportDescription { nullptr };
    QToolButton* btnImportData { nullptr };
    // 另存为页
    QLabel* labelSaveAsTitle { nullptr };
    QLabel* labelCurrentProjectPath { nullptr };
    QToolButton* btnChooseSavePath { nullptr };
};

/**
 * @brief 私有数据构造
 * @param p 面板指针
 */
DAAppApplicationWidget::PrivateData::PrivateData(DAAppApplicationWidget* p) : q_ptr(p)
{
}

/**
 * @brief 把标签设置为页面标题样式（放大加粗）
 */
void DAAppApplicationWidget::PrivateData::makeTitle(QLabel* label)
{
    QFont f = label->font();
    f.setPointSizeF(f.pointSizeF() * 1.6);
    f.setBold(true);
    label->setFont(f);
}

/**
 * @brief 创建左栏导航按钮（页签与底部动作共用样式）
 */
QToolButton* DAAppApplicationWidget::PrivateData::makeNavButton(const QIcon& icon)
{
    QToolButton* btn = new QToolButton;
    btn->setObjectName("navBtn");
    btn->setIcon(icon);
    btn->setIconSize(QSize(20, 20));
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return btn;
}

/**
 * @brief 创建页面主操作按钮（主蓝描边）
 */
QToolButton* DAAppApplicationWidget::PrivateData::makePrimaryButton(const QIcon& icon)
{
    QToolButton* btn = new QToolButton;
    btn->setObjectName("primaryBtn");
    btn->setIcon(icon);
    btn->setIconSize(QSize(24, 24));
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return btn;
}

/**
 * @brief 创建页面次要操作按钮（中灰描边）
 */
QToolButton* DAAppApplicationWidget::PrivateData::makeSecondaryButton(const QIcon& icon)
{
    QToolButton* btn = new QToolButton;
    btn->setObjectName("secondaryBtn");
    btn->setIcon(icon);
    btn->setIconSize(QSize(18, 18));
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return btn;
}

/**
 * @brief 构建左栏导航：上部内容页签 + 下部立即执行的动作
 */
QWidget* DAAppApplicationWidget::PrivateData::buildNavWidget()
{
    QWidget* nav = new QWidget;
    nav->setFixedWidth(200);
    QVBoxLayout* lay = new QVBoxLayout(nav);
    lay->setContentsMargins(12, 28, 12, 16);
    lay->setSpacing(4);
    // 内容页签
    btnOpenPage   = makeNavButton(actions->actionOpen->icon());
    btnNewPage    = makeNavButton(actions->actionNewProject->icon());
    btnImportPage = makeNavButton(actions->actionAddData->icon());
    btnSaveAsPage = makeNavButton(actions->actionSaveAs->icon());
    btnOpenPage->setCheckable(true);
    btnNewPage->setCheckable(true);
    btnImportPage->setCheckable(true);
    btnSaveAsPage->setCheckable(true);
    btnOpenPage->setChecked(true);
    QButtonGroup* grp = new QButtonGroup(q_ptr);
    grp->addButton(btnOpenPage, PageOpen);
    grp->addButton(btnNewPage, PageNew);
    grp->addButton(btnImportPage, PageImport);
    grp->addButton(btnSaveAsPage, PageSaveAs);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(grp,
                     QOverload< int >::of(&QButtonGroup::buttonClicked),
                     q_ptr,
                     [ this ](int id) { stacked->setCurrentIndex(id); });
#else
    QObject::connect(grp, &QButtonGroup::idClicked, q_ptr, [ this ](int id) { stacked->setCurrentIndex(id); });
#endif
    lay->addWidget(btnOpenPage);
    lay->addWidget(btnNewPage);
    lay->addWidget(btnImportPage);
    lay->addWidget(btnSaveAsPage);
    lay->addStretch(1);
    // 分隔线
    QFrame* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    lay->addWidget(line);
    // 底部动作
    btnSave    = makeNavButton(actions->actionSave->icon());
    btnSetting = makeNavButton(actions->actionSetting->icon());
    btnAbout   = makeNavButton(actions->actionAbout->icon());
    btnExit    = makeNavButton(QIcon(":/app/bright/Icon/exit.svg"));
    QObject::connect(btnSave, &QToolButton::clicked, q_ptr, [ this ]() { triggerActionAndHide(actions->actionSave); });
    QObject::connect(btnSetting,
                     &QToolButton::clicked,
                     q_ptr,
                     [ this ]() { triggerActionAndHide(actions->actionSetting); });
    QObject::connect(btnAbout, &QToolButton::clicked, q_ptr, [ this ]() { triggerActionAndHide(actions->actionAbout); });
    QObject::connect(btnExit, &QToolButton::clicked, q_ptr, [ this ]() { exitApp(); });
    lay->addWidget(btnSave);
    lay->addWidget(btnSetting);
    lay->addWidget(btnAbout);
    lay->addWidget(btnExit);
    return nav;
}

/**
 * @brief 构建打开工程页：最近工程列表 + 浏览入口
 */
QWidget* DAAppApplicationWidget::PrivateData::buildPageOpen()
{
    QWidget* page = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(36, 28, 36, 20);
    lay->setSpacing(10);
    labelOpenTitle = new QLabel(page);
    makeTitle(labelOpenTitle);
    lay->addWidget(labelOpenTitle);
    // 浏览入口
    btnBrowseProject = makePrimaryButton(actions->actionOpen->icon());
    QObject::connect(btnBrowseProject, &QToolButton::clicked, q_ptr, [ this ]() {
        triggerActionAndHide(actions->actionOpen);
    });
    lay->addWidget(btnBrowseProject);
    lay->addSpacing(14);
    // 最近列表
    labelRecentTitle = new QLabel(page);
    lay->addWidget(labelRecentTitle);
    recentList = new QListWidget(page);
    recentList->setMouseTracking(true);
    QObject::connect(recentList, &QListWidget::itemClicked, q_ptr, [ this ](QListWidgetItem* item) {
        onRecentItemClicked(item);
    });
    lay->addWidget(recentList, 1);
    recentEmptyHint = new QLabel(page);
    lay->addWidget(recentEmptyHint);
    // 次要入口行
    QHBoxLayout* secLay = new QHBoxLayout;
    secLay->setSpacing(8);
    btnAppendProject = makeSecondaryButton(actions->actionAppendProject->icon());
    QObject::connect(btnAppendProject, &QToolButton::clicked, q_ptr, [ this ]() {
        triggerActionAndHide(actions->actionAppendProject);
    });
    btnOpenMarkdown = makeSecondaryButton(actions->actionOpenMarkdown->icon());
    QObject::connect(btnOpenMarkdown, &QToolButton::clicked, q_ptr, [ this ]() {
        triggerActionAndHide(actions->actionOpenMarkdown);
    });
    btnClearRecent = makeSecondaryButton(QIcon());
    QObject::connect(btnClearRecent, &QToolButton::clicked, q_ptr, [ this ]() {
        actions->recentFilesManager->clear();
    });
    secLay->addWidget(btnAppendProject);
    secLay->addWidget(btnOpenMarkdown);
    secLay->addStretch(1);
    secLay->addWidget(btnClearRecent);
    lay->addLayout(secLay);
    return page;
}

/**
 * @brief 构建新建工程页：说明 + 创建入口
 */
QWidget* DAAppApplicationWidget::PrivateData::buildPageNew()
{
    QWidget* page = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(36, 28, 36, 20);
    lay->setSpacing(10);
    labelNewTitle = new QLabel(page);
    makeTitle(labelNewTitle);
    lay->addWidget(labelNewTitle);
    labelNewDescription = new QLabel(page);
    labelNewDescription->setWordWrap(true);
    lay->addWidget(labelNewDescription);
    lay->addSpacing(8);
    btnCreateProject = makePrimaryButton(actions->actionNewProject->icon());
    QObject::connect(btnCreateProject, &QToolButton::clicked, q_ptr, [ this ]() {
        triggerActionAndHide(actions->actionNewProject);
    });
    lay->addWidget(btnCreateProject);
    lay->addStretch(1);
    return page;
}

/**
 * @brief 构建导入数据页：格式说明 + 导入入口
 */
QWidget* DAAppApplicationWidget::PrivateData::buildPageImport()
{
    QWidget* page = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(36, 28, 36, 20);
    lay->setSpacing(10);
    labelImportTitle = new QLabel(page);
    makeTitle(labelImportTitle);
    lay->addWidget(labelImportTitle);
    labelImportDescription = new QLabel(page);
    labelImportDescription->setWordWrap(true);
    lay->addWidget(labelImportDescription);
    lay->addSpacing(8);
    btnImportData = makePrimaryButton(actions->actionAddData->icon());
    QObject::connect(btnImportData, &QToolButton::clicked, q_ptr, [ this ]() {
        triggerActionAndHide(actions->actionAddData);
    });
    lay->addWidget(btnImportData);
    lay->addStretch(1);
    return page;
}

/**
 * @brief 构建另存为页：当前工程信息 + 另存入口
 */
QWidget* DAAppApplicationWidget::PrivateData::buildPageSaveAs()
{
    QWidget* page = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(36, 28, 36, 20);
    lay->setSpacing(10);
    labelSaveAsTitle = new QLabel(page);
    makeTitle(labelSaveAsTitle);
    lay->addWidget(labelSaveAsTitle);
    labelCurrentProjectPath = new QLabel(page);
    labelCurrentProjectPath->setWordWrap(true);
    lay->addWidget(labelCurrentProjectPath);
    lay->addSpacing(8);
    btnChooseSavePath = makePrimaryButton(actions->actionSaveAs->icon());
    QObject::connect(btnChooseSavePath, &QToolButton::clicked, q_ptr, [ this ]() {
        triggerActionAndHide(actions->actionSaveAs);
    });
    lay->addWidget(btnChooseSavePath);
    lay->addStretch(1);
    return page;
}

/**
 * @brief 构建面板整体布局并应用样式
 */
void DAAppApplicationWidget::PrivateData::buildUI()
{
    q_ptr->setAutoFillBackground(true);
    q_ptr->setStyleSheet(QString::fromUtf8(kAppWidgetStyleSheet));
    QHBoxLayout* mainLay = new QHBoxLayout(q_ptr);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);
    mainLay->addWidget(buildNavWidget());
    stacked = new QStackedWidget(q_ptr);
    stacked->addWidget(buildPageOpen());
    stacked->addWidget(buildPageNew());
    stacked->addWidget(buildPageImport());
    stacked->addWidget(buildPageSaveAs());
    stacked->setCurrentIndex(PageOpen);
    mainLay->addWidget(stacked, 1);
}

/**
 * @brief 创建最近工程列表的单个条目控件
 *
 * 失效路径的条目文字置灰并加删除线；控件对鼠标事件透明，点击由列表接管
 * @param filePath 工程文件路径
 * @return 条目控件（由 QListWidget 接管所有权）
 */
QWidget* DAAppApplicationWidget::PrivateData::makeRecentItemWidget(const QString& filePath)
{
    QWidget* w = new QWidget;
    w->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    QHBoxLayout* hLay = new QHBoxLayout(w);
    hLay->setContentsMargins(8, 8, 8, 8);
    hLay->setSpacing(10);
    const QFileInfo fi(filePath);
    const bool exists = fi.exists();
    // 文件图标
    QLabel* iconLabel = new QLabel(w);
    iconLabel->setPixmap(actions->actionOpen->icon().pixmap(28, 28));
    hLay->addWidget(iconLabel);
    // 文件名 + 路径
    QVBoxLayout* vLay = new QVBoxLayout;
    vLay->setSpacing(2);
    QLabel* nameLabel = new QLabel(fi.fileName(), w);
    QFont nameFont = nameLabel->font();
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);
    QLabel* pathLabel = new QLabel(fi.absolutePath(), w);
    pathLabel->setStyleSheet("color:#727272;font-size:9pt;");
    vLay->addWidget(nameLabel);
    vLay->addWidget(pathLabel);
    hLay->addLayout(vLay, 1);
    // 最后修改时间
    QLabel* timeLabel = new QLabel(fi.lastModified().toString(Qt::SystemLocaleShortDate), w);
    timeLabel->setStyleSheet("color:#727272;font-size:9pt;");
    hLay->addWidget(timeLabel);
    if (!exists) {
        QFont strikeFont = nameLabel->font();
        strikeFont.setStrikeOut(true);
        nameLabel->setFont(strikeFont);
        nameLabel->setStyleSheet("color:#B2B2B2;");
        pathLabel->setStyleSheet("color:#B2B2B2;font-size:9pt;");
        timeLabel->setStyleSheet("color:#B2B2B2;font-size:9pt;");
    }
    return w;
}

/**
 * @brief 刷新最近工程列表
 */
void DAAppApplicationWidget::PrivateData::refreshRecentProjects()
{
    recentList->clear();
    const QStringList files = actions->recentFilesManager->getRecentFiles();
    const bool empty = files.isEmpty();
    recentList->setVisible(!empty);
    recentEmptyHint->setVisible(empty);
    for (const QString& f : files) {
        QListWidgetItem* item = new QListWidgetItem;
        item->setData(Qt::UserRole, f);
        item->setToolTip(f);
        QWidget* itemWidget = makeRecentItemWidget(f);
        item->setSizeHint(itemWidget->sizeHint());
        recentList->addItem(item);
        recentList->setItemWidget(item, itemWidget);
    }
}

/**
 * @brief 刷新另存为页的当前工程路径显示
 */
void DAAppApplicationWidget::PrivateData::refreshSaveAsPage()
{
    DAAppProject* proj = DA_APP_CORE.getAppProject();
    if (nullptr == proj) {
        labelCurrentProjectPath->clear();
        return;
    }
    const QString path = proj->getProjectFilePath();
    if (path.isEmpty()) {
        labelCurrentProjectPath->setText(DAAppApplicationWidget::tr("(untitled project)"));  // cn:(未命名工程)
    } else {
        labelCurrentProjectPath->setText(path);
    }
}

/**
 * @brief 最近列表条目点击：存在则打开工程并关闭面板，不存在则提示并从列表移除
 */
void DAAppApplicationWidget::PrivateData::onRecentItemClicked(QListWidgetItem* item)
{
    const QString path = item->data(Qt::UserRole).toString();
    if (actions->recentFilesManager->selectFile(path)) {
        q_ptr->hide();
    } else {
        QMessageBox::information(q_ptr,
                                 DAAppApplicationWidget::tr("Information"),  // cn:提示
                                 DAAppApplicationWidget::tr(
                                     "The file does not exist and has been removed from the recent list"));  // cn:文件不存在，已从最近列表中移除
    }
}

/**
 * @brief 先隐藏面板再触发动作，避免面板遮挡动作弹出的对话框
 * @param act 要触发的动作
 */
void DAAppApplicationWidget::PrivateData::triggerActionAndHide(QAction* act)
{
    q_ptr->hide();
    if (act) {
        act->trigger();
    }
}

/**
 * @brief 退出程序（关闭主窗口，复用其未保存提示流程）
 */
void DAAppApplicationWidget::PrivateData::exitApp()
{
    q_ptr->hide();
    if (QWidget* win = q_ptr->window()) {
        win->close();
    }
}

/**
 * @brief DAAppApplicationWidget 构造函数
 * @param parent 主窗口
 * @param actions 应用动作集，提供面板内各入口触发的动作
 * @details 面板配置为宽度占主窗口 2/3、全高贴左伸出，启用半透明遮罩（点击关闭），
 *          滑入动画使用库默认配置（开启，250 毫秒）
 */
DAAppApplicationWidget::DAAppApplicationWidget(SARibbonMainWindow* parent, DAAppActions* actions)
    : SARibbonApplicationWidget(parent), DA_PIMPL_CONSTRUCT
{
    setWidthRatio(2.0 / 3.0);
    setMaskWidgetEnabled(true);
    DA_D(d);
    d->actions = actions;
    d->buildUI();
    retranslateUi();
}

DAAppApplicationWidget::~DAAppApplicationWidget()
{
}

/**
 * @brief 语言变化时刷新全部界面文本
 */
void DAAppApplicationWidget::retranslateUi()
{
    DA_D(d);
    // 导航页签
    d->btnOpenPage->setText(tr("Open Project"));   // cn:打开工程
    d->btnNewPage->setText(tr("New Project"));     // cn:新建工程
    d->btnImportPage->setText(tr("Import Data"));  // cn:导入数据
    d->btnSaveAsPage->setText(tr("Save As"));      // cn:另存为
    // 底部动作
    d->btnSave->setText(tr("Save Project"));  // cn:保存工程
    d->btnSetting->setText(tr("Setting"));    // cn:设置
    d->btnAbout->setText(tr("About"));        // cn:关于
    d->btnExit->setText(tr("Exit"));          // cn:退出
    // 打开工程页
    d->labelOpenTitle->setText(tr("Open Project"));           // cn:打开工程
    d->btnBrowseProject->setText(tr("Browse..."));            // cn:浏览…
    d->labelRecentTitle->setText(tr("Recent Projects"));      // cn:最近打开的工程
    d->recentEmptyHint->setText(tr("No recent projects"));    // cn:暂无最近打开的工程
    d->btnAppendProject->setText(tr("Append To Project..."));  // cn:追加工程…
    d->btnOpenMarkdown->setText(tr("Open Markdown..."));      // cn:打开 Markdown…
    d->btnClearRecent->setText(tr("Clear Recent List"));      // cn:清空最近列表
    // 新建工程页
    d->labelNewTitle->setText(tr("New Project"));  // cn:新建工程
    d->labelNewDescription->setText(tr("Create a new empty untitled project. You will be asked to save the "
                                       "current project if it has unsaved changes."));  // cn:创建一个新的空白未命名工程；当前工程有未保存的更改时会先提示保存。
    d->btnCreateProject->setText(tr("Create Empty Project"));  // cn:创建空白工程
    // 导入数据页
    d->labelImportTitle->setText(tr("Import Data"));  // cn:导入数据
    d->labelImportDescription->setText(
        tr("Import data files into the project (txt, csv, xls, xlsx, parquet, pickle; multiple files can be "
           "selected for batch import)."));  // cn:导入数据文件到工程（txt、csv、xls、xlsx、parquet、pickle，可多选批量导入）。
    d->btnImportData->setText(tr("Browse Files..."));  // cn:浏览文件导入…
    // 另存为页
    d->labelSaveAsTitle->setText(tr("Save As"));  // cn:另存为
    d->btnChooseSavePath->setText(tr("Choose Save Location..."));  // cn:选择保存位置…
    d->refreshRecentProjects();
    d->refreshSaveAsPage();
}

/**
 * @brief 显示时刷新最近工程列表与另存为页信息
 */
void DAAppApplicationWidget::showEvent(QShowEvent* e)
{
    DA_D(d);
    d->refreshRecentProjects();
    d->refreshSaveAsPage();
    SARibbonApplicationWidget::showEvent(e);
}
}  // namespace DA
