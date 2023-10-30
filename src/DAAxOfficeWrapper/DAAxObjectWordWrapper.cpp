#include "DAAxObjectWordWrapper.h"
#include "DAAxObjectWordTableWrapper.h"
#include <QDir>

namespace DA
{
class DAAxObjectWordWrapper::PrivateData
{
    DA_DECLARE_PUBLIC(DAAxObjectWordWrapper)
public:
    PrivateData(DAAxObjectWordWrapper* p);
    //尝试打开app
    bool tryCreateComControl(QAxObject* obj);
    //初始化
    bool initialize();
    //安全清楚
    void safeClear();
    //判断是否初始化完成
    bool isInitialize() const;
    //是否存在文档
    bool isHaveDocument() const;
    //打开文件
    bool open(const QString& docfile, bool isvisible = false);
    //退出app
    void quit();
    //关闭文档
    void close(bool on);
    //保存
    void save();
    //另存
    bool saveAs(const QString& docfile);
    //选中标签
    QAxObject* selectMark(const QString& markName);
    //替换标签内容
    bool replaceMark(const QString& markName, const QVariant& replaceContent);
    //插入图片
    bool insertPictureAtMark(const QString& markName, const QString& picturePath);
    //插入表格
    DAAxObjectWordTableWrapper insertTableAtMark(const QString& markName, int rowCnt, int colCnt);

public:
    const QString cWordAppComControlName { "word.Application" };  ///< word的程序名
    const QString cWpsAppComControlName { "kwps.Application" };   ///< wps的程序名
    QAxObject* mWordApp { nullptr };                              ///< word.Application
    QAxObject* mDocument { nullptr };                             ///< 文档
};

DAAxObjectWordWrapper::PrivateData::PrivateData(DAAxObjectWordWrapper* p) : q_ptr(p)
{
    initialize();
}

bool DAAxObjectWordWrapper::PrivateData::tryCreateComControl(QAxObject* obj)
{
    if (!obj->setControl(cWordAppComControlName)) {
        //无法打开word，尝试用wps打开
        if (!obj->setControl(cWpsAppComControlName)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 初始化
 * @return
 */
bool DAAxObjectWordWrapper::PrivateData::initialize()
{
    mWordApp = new QAxObject(q_ptr);
    tryCreateComControl(mWordApp);  //打开word
    return !mWordApp->isNull();
}

/**
 * @brief 安全清除
 */
void DAAxObjectWordWrapper::PrivateData::safeClear()
{
    close(true);
    quit();
    if (mWordApp) {
        delete mWordApp;
        mWordApp = nullptr;
    }
    // mDocument无需删除，因为是mWordApp生成，有父子关系
    mDocument = nullptr;
}

/**
 * @brief 是否完成初始化
 * @return
 */
bool DAAxObjectWordWrapper::PrivateData::isInitialize() const
{
    return (mWordApp != nullptr) && !(mWordApp->isNull());
}

/**
 * @brief 是否存在文档
 * @return
 */
bool DAAxObjectWordWrapper::PrivateData::isHaveDocument() const
{
    return mDocument != nullptr;
}

/**
 * @brief 打开
 * @param docfile
 * @param isvisible
 * @return
 */
bool DAAxObjectWordWrapper::PrivateData::open(const QString& docfile, bool isvisible)
{
    if (!isInitialize()) {
        return false;
    }
    QAxObject* documents = mWordApp->querySubObject("Documents");  //获取所有的工作文档(返回一个指向QAxObject包含的COM对象)
    if (!documents) {
        return false;
    }
    documents->dynamicCall("Add(QString)", docfile);         //添加一个文档
    mDocument = mWordApp->querySubObject("ActiveDocument");  //获取当前激活的文档
    return (mDocument != nullptr);
}

void DAAxObjectWordWrapper::PrivateData::quit()
{
    if (isInitialize()) {
        //退出
        mWordApp->dynamicCall("Quit()");  //退出word
    }
    //重置状态
}

/**
 * @brief 关闭
 * @param on
 */
void DAAxObjectWordWrapper::PrivateData::close(bool on)
{
    if (isHaveDocument()) {
        mDocument->dynamicCall("Close(bool)", on);
    }
}

void DAAxObjectWordWrapper::PrivateData::save()
{
    if (isHaveDocument()) {
        mDocument->dynamicCall("Save()");
    }
}

bool DAAxObjectWordWrapper::PrivateData::saveAs(const QString& docfile)
{
    if (isHaveDocument()) {
        return mDocument->dynamicCall("SaveAs(const QString&)", docfile).toBool();
    }
    return false;
}

/**
 * @brief 选中标签，如果没有标签，返回nullptr
 * @param markName
 * @return
 */
QAxObject* DAAxObjectWordWrapper::PrivateData::selectMark(const QString& markName)
{
    if (!isHaveDocument()) {
        return nullptr;
    }
    QAxObject* bookmarkCode = mDocument->querySubObject("Bookmarks(QString)", markName);
    if (bookmarkCode) {
        bookmarkCode->dynamicCall("Select(void)");
        return bookmarkCode;
    }
    return nullptr;
}

/**
 * @brief 替换标签内容
 * @param markName 标签名
 * @param replaceContent 标签内容
 * @return
 */
bool DAAxObjectWordWrapper::PrivateData::replaceMark(const QString& markName, const QVariant& replaceContent)
{
    QAxObject* bookmarkCode = selectMark(markName);
    if (!bookmarkCode) {
        return false;
    }
    //选中标签，将字符textg插入到标签位置
    QAxObject* range = bookmarkCode->querySubObject("Range");
    range->setProperty("Text", replaceContent);
    delete range;
    return true;
}

/**
 * @brief 插入图片到标签处
 * @param markName
 * @param picturePath
 * @return
 */
bool DAAxObjectWordWrapper::PrivateData::insertPictureAtMark(const QString& markName, const QString& picturePath)
{
    QAxObject* bookmarkCode = selectMark(markName);
    if (!bookmarkCode) {
        return false;
    }
    QAxObject* Inlineshapes = mDocument->querySubObject("InlineShapes");
    Inlineshapes->dynamicCall("AddPicture(const QString&)", QDir::toNativeSeparators(picturePath));
    delete Inlineshapes;
    return true;
}

/**
 * @brief 插入表格
 * @param markName
 * @param rowCnt 行数
 * @param colCnt 列数
 * @return
 */
DAAxObjectWordTableWrapper DAAxObjectWordWrapper::PrivateData::insertTableAtMark(const QString& markName, int rowCnt, int colCnt)
{
    QAxObject* bookmarkCode = selectMark(markName);
    if (!bookmarkCode) {
        return DAAxObjectWordTableWrapper(nullptr);
    }
    QAxObject* selection = mWordApp->querySubObject("Selection");
    if (!selection) {
        return DAAxObjectWordTableWrapper(nullptr);
    }
    QAxObject* range = selection->querySubObject("Range");
    if (!range) {
        return DAAxObjectWordTableWrapper(nullptr);
    }
    QAxObject* tables = mDocument->querySubObject("Tables");
    if (!tables) {
        return DAAxObjectWordTableWrapper(nullptr);
    }
    QAxObject* table = tables->querySubObject("Add(QVariant,int,int)", range->asVariant(), rowCnt, colCnt);
    //    for (int i = 1; i <= 6; i++) {
    //        QString str        = QString("Borders(-%1)").arg(i);
    //        QAxObject* borders = table->querySubObject(str.toLocal8Bit().constData());
    //        borders->dynamicCall("SetLineStyle(int)", 1);
    //    }
    table->setProperty("Style", QString(u8"网格型"));
    table->dynamicCall("AutoFitBehavior(WdAutoFitBehavior)", 2);  //表格自动拉伸列 0固定  1根据内容调整  2 根据窗口调整
    return DAAxObjectWordTableWrapper(table);
}

//===================================================
// DAAxObjectWordWrapper
//===================================================

DAAxObjectWordWrapper::DAAxObjectWordWrapper(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

DAAxObjectWordWrapper::~DAAxObjectWordWrapper()
{
    d_ptr->safeClear();
}

/**
 * @brief 这是调用quit后，如果还要继续操作，可以调用此函数
 */
void DAAxObjectWordWrapper::reloadApp()
{
    d_ptr->initialize();
}

/**
 * @brief 打开
 * @param docfile
 * @param isvisible
 * @return
 */
bool DAAxObjectWordWrapper::open(const QString& docfile, bool isvisible)
{
    return d_ptr->open(docfile, isvisible);
}
/**
 * @brief 是否初始化完成
 * @return
 */
bool DAAxObjectWordWrapper::isInitialize() const
{
    return d_ptr->isInitialize();
}

/**
 * @brief 是否存在文档
 * @return
 */
bool DAAxObjectWordWrapper::isHaveDocument() const
{
    return d_ptr->isHaveDocument();
}

/**
 * @brief 退出，正常用户无需手动调用
 */
void DAAxObjectWordWrapper::quit()
{
    d_ptr->quit();
}

/**
 * @brief 关闭
 * @param on
 */
void DAAxObjectWordWrapper::close()
{
    d_ptr->close(true);
}

/**
 * @brief 保存
 */
void DAAxObjectWordWrapper::save()
{
    d_ptr->save();
}

/**
 * @brief 另存
 * @param docfile
 */
bool DAAxObjectWordWrapper::saveAs(const QString& docfile)
{
    return d_ptr->saveAs(docfile);
}

/**
 * @brief 替换标签内容
 * @param markName 标签名
 * @param replaceContent 标签内容
 * @return
 */
bool DAAxObjectWordWrapper::replaceMark(const QString& markName, const QVariant& replaceContent)
{
    return d_ptr->replaceMark(markName, replaceContent);
}

/**
 * @brief 在mark（书签）位置添加文本
 * @param markName 书签名
 * @param replaceContent 内容
 * @return
 */
bool DAAxObjectWordWrapper::insertTextAtMark(const QString& markName, const QString& replaceContent)
{
    return d_ptr->replaceMark(markName, replaceContent);
    //    QAxObject* rang = d_ptr->selectMark(markName);
    //    if (!rang) {
    //        return false;
    //    }

    //    rang->dynamicCall("InsertAfter(QString)", replaceContent);
    //    rang->dynamicCall("InsertParagraphAfter(void)");
    //    return true;
}

/**
 * @brief 在mark（书签）位置添加图片
 * @param markName 书签名
 * @param picturePath 图片路径
 * @return
 */
bool DAAxObjectWordWrapper::insertPictureAtMark(const QString& markName, const QString& picturePath)
{
    return d_ptr->insertPictureAtMark(markName, picturePath);
}

/**
 * @brief 在书签处插入表格
 * @param markName 书签名
 * @param rowCount 行数
 * @param colCount 列数
 * @return
 */
DAAxObjectWordTableWrapper DAAxObjectWordWrapper::insertTableAtMark(const QString& markName, int rowCount, int colCount)
{
    return d_ptr->insertTableAtMark(markName, rowCount, colCount);
}
}  // end DA
