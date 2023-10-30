#include "DAAxObjectExcelWrapper.h"
#include <QFile>
#include <QDir>
#include <QDebug>
namespace DA
{
/**
 * @brief The DAAxObjectExcelWrapper::PrivateData class
 */
class DAAxObjectExcelWrapper::PrivateData
{
    DA_DECLARE_PUBLIC(DAAxObjectExcelWrapper)
public:
    PrivateData(DAAxObjectExcelWrapper* p);
    //初始化，加载app
    bool initialize(bool visible = false, bool displayAlerts = false);
    //判断是否初始化完成
    bool isInitialize() const;
    //尝试初始化，如果已经初始化无动作，如果没有初始化就初始化
    bool tryInitialize(bool visible = false, bool displayAlerts = false);
    //判断QAxObject*是否是有效的
    bool checkAxObjectValid(QAxObject* ax) const;
    //判断是否存在Workbooks
    bool isHaveWorkbooks() const;
    //判断是否存在Workbook
    bool isHaveWorkbook() const;
    //判断是否存在WorkSheets
    bool isHaveWorkSheets() const;
    //尝试打开app
    bool tryCreateComControl(QAxObject* obj);
    //窗体是否显示
    void setWindowVisible(bool on);
    //是否显示警告
    void setDisplayAlerts(bool on);
    //打开excel
    bool open(const QString& filename, bool visible = false, bool displayAlerts = false);
    //
    void close();
    //释放
    void release();
    //获取第n个sheet
    static QAxObject* getWorkSheet(QAxObject* workSheetsObj, int indexOneBase);
    //获取sheets数量
    static int getWorkSheetsCount(QAxObject* workSheetsObj);
    //获取sheet的名字
    static QString getSheetName(QAxObject* sheetObj);

public:
    const QString cAppComControlName { "Excel.Application" };  ///< excel的程序名
    QAxObject* mAxApp { nullptr };                             ///< Excel.Application
    QAxObject* mAxWorkbooks { nullptr };                       ///< Workbooks,父对象是mApp
    QAxObject* mAxWorkbook { nullptr };                        ///< Workbook,父对象是mAxWorkbooks/mApp
    QAxObject* mAxWorkSheets { nullptr };                      ///< WorkSheets,
    QList< QAxObject* > mAxSheets;                             ///< 存放sheets
};

DAAxObjectExcelWrapper::PrivateData::PrivateData(DAAxObjectExcelWrapper* p) : q_ptr(p)
{
}

bool DAAxObjectExcelWrapper::PrivateData::initialize(bool visible, bool displayAlerts)
{
    mAxApp = new QAxObject(q_ptr);
    if (!tryCreateComControl(mAxApp)) {
        return false;
    }
    setWindowVisible(visible);
    setDisplayAlerts(displayAlerts);
    mAxWorkbooks = mAxApp->querySubObject("Workbooks");
    return true;
}

bool DAAxObjectExcelWrapper::PrivateData::isInitialize() const
{
    return checkAxObjectValid(mAxApp);
}

bool DAAxObjectExcelWrapper::PrivateData::tryInitialize(bool visible, bool displayAlerts)
{
    if (!isInitialize()) {
        if (!initialize(visible, displayAlerts)) {
            return false;
        }
    }
    return true;
}

bool DAAxObjectExcelWrapper::PrivateData::checkAxObjectValid(QAxObject* ax) const
{
    return (ax != nullptr) && !(ax->isNull());
}

bool DAAxObjectExcelWrapper::PrivateData::isHaveWorkbooks() const
{
    return checkAxObjectValid(mAxWorkbooks);
}

bool DAAxObjectExcelWrapper::PrivateData::isHaveWorkbook() const
{
    return checkAxObjectValid(mAxWorkbooks);
}

bool DAAxObjectExcelWrapper::PrivateData::isHaveWorkSheets() const
{
    return checkAxObjectValid(mAxWorkSheets);
}

bool DAAxObjectExcelWrapper::PrivateData::tryCreateComControl(QAxObject* obj)
{
    if (!obj->setControl(cAppComControlName)) {
        return false;
    }
    return true;
}

void DAAxObjectExcelWrapper::PrivateData::setWindowVisible(bool on)
{
    if (!isInitialize()) {
        return;
    }
    mAxApp->dynamicCall("SetVisible(bool)", on);  // false不显示窗体
}

void DAAxObjectExcelWrapper::PrivateData::setDisplayAlerts(bool on)
{
    if (!isInitialize()) {
        return;
    }
    mAxApp->setProperty("DisplayAlerts", on);  //不显示警告。
}

bool DAAxObjectExcelWrapper::PrivateData::open(const QString& filename, bool visible, bool displayAlerts)
{
    if (!tryInitialize(visible, displayAlerts)) {
        qWarning() << QObject::tr("unable initialize %1").arg(cAppComControlName);  // cn:无法初始化%1
        return false;
    }
    QString nativeFileName = QDir::toNativeSeparators(filename);
    if (QFile::exists(nativeFileName)) {
        //文件存在
        mAxWorkbooks = mAxWorkbooks->querySubObject("Open(const QString &)", nativeFileName);
        mAxWorkbook  = mAxApp->querySubObject("ActiveWorkBook");
    } else {
        //文件不存在则创建
        mAxWorkbooks->dynamicCall("Add");
        mAxWorkbook = mAxApp->querySubObject("ActiveWorkBook");
    }
    if (!isHaveWorkbook()) {
        qWarning() << QObject::tr("can not get excel workbook");  // cn:无法获取excel的workbook
        return false;
    }
    mAxWorkSheets = mAxWorkbook->querySubObject("Sheets");  //获得所有工作表对象
    return true;
}

void DAAxObjectExcelWrapper::PrivateData::close()
{
    if (isHaveWorkbook()) {
        mAxWorkbook->dynamicCall("Close()");
    }
    if (isHaveWorkbooks()) {
        mAxWorkbooks->dynamicCall("Close()");
    }
}

void DAAxObjectExcelWrapper::PrivateData::release()
{
    close();
    //理论所有的AxObject的父类都是mAxApp，因此务必保证mAxApp被删除
    if (isInitialize()) {
        mAxApp->dynamicCall("Quit()");
        delete mAxApp;
    } else {
        if (mAxApp) {
            //有可能初始化失败，但这个时候AxObject已经创建了
            delete mAxApp;
        }
    }

    mAxApp        = nullptr;
    mAxWorkbooks  = nullptr;
    mAxWorkbook   = nullptr;
    mAxWorkSheets = nullptr;
}

/**
 * @brief workSheets获取workSheet
 * @param workSheetsObj
 * @param indexOneBase
 * @return
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::getWorkSheet(QAxObject* workSheetsObj, int indexOneBase)
{
    return workSheetsObj->querySubObject("Item(int)", indexOneBase);
}

/**
 * @brief 获取sheets数量
 * @param workSheetsObj
 * @return
 */
int DAAxObjectExcelWrapper::PrivateData::getWorkSheetsCount(QAxObject* workSheetsObj)
{
    return workSheetsObj->property("Count").toInt();
}

/**
 * @brief 获取sheet的名字
 * @param sheetObj
 * @return
 */
QString DAAxObjectExcelWrapper::PrivateData::getSheetName(QAxObject* sheetObj)
{
    return sheetObj->property("Name").toString();
}

//----------------------------------------------------
// DAAxObjectExcelWrapper
//----------------------------------------------------

DAAxObjectExcelWrapper::DAAxObjectExcelWrapper(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

DAAxObjectExcelWrapper::~DAAxObjectExcelWrapper()
{
}

/**
 * @brief 是否显示窗口
 * @param on
 */
void DAAxObjectExcelWrapper::setWindowVisible(bool on)
{
    d_ptr->setWindowVisible(on);
}

/**
 * @brief 显示警告
 * @param on
 */
void DAAxObjectExcelWrapper::setDisplayAlerts(bool on)
{
    d_ptr->setDisplayAlerts(on);
}

bool DAAxObjectExcelWrapper::open(const QString& filename, bool visible, bool displayAlerts)
{
    return d_ptr->open(filename, visible, displayAlerts);
}

}
