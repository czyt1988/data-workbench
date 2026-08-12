#include "DAAxObjectExcelWrapper.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include "DALogCategory.h"
#include "qt_windows.h"
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
    ~PrivateData();
    // 初始化，加载app
    bool initialize(bool visible = false, bool displayAlerts = false);
    // 判断是否初始化完成
    bool isInitialize() const;
    // 尝试初始化，如果已经初始化无动作，如果没有初始化就初始化
    bool tryInitialize(bool visible = false, bool displayAlerts = false);
    // 判断QAxObject*是否是有效的
    bool checkAxObjectValid(QAxObject* ax) const;
    // 判断是否存在Workbooks
    bool isHaveWorkbooks() const;
    // 判断是否存在Workbook
    bool isHaveWorkbook() const;
    // 判断是否存在WorkSheets
    bool isHaveWorkSheets() const;
    // 尝试打开app
    bool tryCreateComControl(QAxObject* obj);
    // 窗体是否显示
    void setWindowVisible(bool on);
    // 是否显示警告
    void setDisplayAlerts(bool on);
    // 打开excel
    bool open(const QString& filename, bool visible = false, bool displayAlerts = false);
    // 创建excel
    bool create(const QString& filename, bool visible = false, bool displayAlerts = false);
    // 保存
    bool save();
    // 保存
    bool saveAs(const QString& filename);
    // 关闭
    void close();
    // 释放
    void release();
    // 激活一个excel 工作簿
    void activeWorkBook();
    // 获取所有sheets的名称
    QStringList getSheetsName();
    // 获取sheet的数量
    int getSheetsCount() const;
    // 添加sheet
    QAxObject* addSheet(const QString& name);
    // 获取sheet,注意此函数是0base
    QAxObject* getSheet(int sheetIndex);
    QAxObject* getSheet(const QString& name);
    QAxObject* getFirstSheet();
    QAxObject* getLastSheet();
    // 设置当前sheet
    bool setCurrentSheet(int index);
    bool setCurrentSheet(const QString& name);
    //
    int indexOfSheetName(const QString& name);
    // 获取当前sheet的obj
    QAxObject* getActiveSheet();

public:
    // 获取第n个sheet
    static QAxObject* getWorkSheet(QAxObject* workSheetsObj, int indexOneBase);
    // 获取sheets数量
    static int getWorkSheetsCount(QAxObject* workSheetsObj);
    // 获取worksheet下所有的sheets
    static QList< QAxObject* > getAllSheet(QAxObject* workSheetsObj);
    // 获取sheet的名字
    static QString getSheetName(QAxObject* sheetObj);
    // 获取sheet的名字
    static int getSheetIndex(QAxObject* sheetObj);
    // 获取整个sheet的所有数据
    QVariant getSheetAllData(QAxObject* sheetObj);

public:
    /**
     *@brief 几种类名
     * - excel的程序名:Excel.Application
     * - wps的程序名:ket.Application
     * - wps表格的程序名:kwps.Application
     * - wps表格的程序名2:ET.Application
     */
    const QStringList cappClassNames = { "Excel.Application", "ket.Application", "kwps.Application", "ET.Application" };
    std::unique_ptr< QAxObject > mAxApp;   ///< Excel.Application
    QAxObject* mAxWorkbooks { nullptr };   ///< Workbooks,父对象是mApp
    QAxObject* mAxWorkbook { nullptr };    ///< Workbook,父对象是mAxWorkbooks/mApp
    QAxObject* mAxWorkSheets { nullptr };  ///< WorkSheets,
};

/**
 * @brief 构造函数
 * @param p 父对象
 */
DAAxObjectExcelWrapper::PrivateData::PrivateData(DAAxObjectExcelWrapper* p) : q_ptr(p)
{
    initialize();
}

/**
 * @brief 析构函数
 */
DAAxObjectExcelWrapper::PrivateData::~PrivateData()
{
    release();
}

/**
 * @brief 初始化，加载Excel app
 * @param visible 是否显示窗口
 * @param displayAlerts 是否显示警告
 * @return 初始化成功返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::initialize(bool visible, bool displayAlerts)
{
    HRESULT r = OleInitialize(0);
    if (r != S_OK && r != S_FALSE) {
        qWarning().noquote() << QString("cannot initialize OLE, error code: %1").arg((int)r);
    }
    mAxApp = std::make_unique< QAxObject >(q_ptr);
    if (!tryCreateComControl(mAxApp.get())) {
        mAxApp.reset();
        return false;
    }
    setWindowVisible(visible);
    setDisplayAlerts(displayAlerts);
    mAxWorkbooks = mAxApp->querySubObject("Workbooks");
    if (!mAxWorkbooks) {
        qWarning().noquote() << "cannot query subobject 'Workbooks'";
        return false;
    }
    return true;
}

/**
 * @brief 判断是否初始化完成
 * @return 已初始化返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::isInitialize() const
{
    return checkAxObjectValid(mAxApp.get());
}

/**
 * @brief 尝试初始化，如果已经初始化无动作，如果没有初始化就初始化
 * @param visible 是否显示窗口
 * @param displayAlerts 是否显示警告
 * @return 初始化成功或已初始化返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::tryInitialize(bool visible, bool displayAlerts)
{
    if (!isInitialize()) {
        if (!initialize(visible, displayAlerts)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 判断QAxObject*是否是有效的
 * @param ax 待检查的对象
 * @return 有效返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::checkAxObjectValid(QAxObject* ax) const
{
    return (ax != nullptr) && !(ax->isNull());
}

/**
 * @brief 判断是否存在Workbooks
 * @return 存在返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::isHaveWorkbooks() const
{
    return checkAxObjectValid(mAxWorkbooks);
}

/**
 * @brief 判断是否存在Workbook
 * @return 存在返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::isHaveWorkbook() const
{
    return checkAxObjectValid(mAxWorkbook);
}

/**
 * @brief 判断是否存在WorkSheets
 * @return 存在返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::isHaveWorkSheets() const
{
    return checkAxObjectValid(mAxWorkSheets);
}

/**
 * @brief 尝试打开app，依次尝试Excel和WPS
 * @param obj QAxObject对象
 * @return 成功打开返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::tryCreateComControl(QAxObject* obj)
{
    bool res = false;
    for (const QString& clasName : cappClassNames) {
        res = obj->setControl(clasName);
        if (res) {
            break;
        }
    }
    return res;
}

/**
 * @brief 窗体是否显示
 * @param on true表示显示
 */
void DAAxObjectExcelWrapper::PrivateData::setWindowVisible(bool on)
{
    if (!isInitialize()) {
        return;
    }
    mAxApp->dynamicCall("SetVisible(bool)", on);  // false不显示窗体
}

/**
 * @brief 是否显示警告
 * @param on true表示显示警告
 */
void DAAxObjectExcelWrapper::PrivateData::setDisplayAlerts(bool on)
{
    if (!isInitialize()) {
        return;
    }
    mAxApp->setProperty("DisplayAlerts", on);  // 不显示警告。
}

/**
 * @brief 打开excel文件
 * @param filename excel文件名
 * @param visible 是否可见
 * @param displayAlerts 是否显示警告
 * @return 成功打开返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::open(const QString& filename, bool visible, bool displayAlerts)
{
    if (!tryInitialize(visible, displayAlerts)) {
        qWarning() << QString("failed to initialize %1").arg(cappClassNames.join(","));
        return false;
    }
    QString nativeFileName = QDir::toNativeSeparators(filename);
    if (!QFile::exists(nativeFileName)) {
        daWarning << QCoreApplication::translate("DAAxObjectExcelWrapper", "File \"%1\" does not exist").arg(nativeFileName);  //cn:文件 "%1" 不存在
        return false;
    }
    // 文件存在
    QAxObject* workbook = mAxWorkbooks->querySubObject("Open(const QString &)", nativeFileName);
    if (qaxobject_is_null(workbook)) {
        daWarning << QCoreApplication::translate("DAAxObjectExcelWrapper", "Failed to open Excel file \"%1\"").arg(nativeFileName);  //cn:无法打开 Excel 文件 "%1"
        return false;
    }
    mAxWorkbook = mAxApp->querySubObject("ActiveWorkBook");
    if (!isHaveWorkbook()) {
        qWarning() << "cannot get Excel workbook";
        return false;
    }
    mAxWorkSheets = mAxWorkbook->querySubObject("Sheets");  // 获得所有工作表对象
    return true;
}

/**
 * @brief 创建excel
 * @param filename
 * @param visible
 * @param displayAlerts
 * @return
 */
bool DAAxObjectExcelWrapper::PrivateData::create(const QString& filename, bool visible, bool displayAlerts)
{
    if (!tryInitialize(visible, displayAlerts)) {
        qWarning() << QString("failed to initialize %1").arg(cappClassNames.join(","));
        return false;
    }
    // 文件不存在则创建
    mAxWorkbooks->dynamicCall("Add");
    mAxWorkbook = mAxApp->querySubObject("ActiveWorkBook");
    if (!isHaveWorkbook()) {
        qWarning() << "cannot get Excel workbook";
        return false;
    }
    saveAs(filename);
    mAxWorkSheets = mAxWorkbook->querySubObject("Sheets");  // 获得所有工作表对象
    return true;
}

/**
 * @brief 保存当前工作簿
 * @return 成功返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::save()
{
    if (!isHaveWorkbook()) {
        return false;
    }
    mAxWorkbook->dynamicCall("Save()");
    return true;
}

/**
 * @brief 另存为
 * @param filename 文件名
 * @return 成功返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::saveAs(const QString& filename)
{
    if (!isHaveWorkbook()) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qWarning() << "have not work book";
#endif
        return false;
    }
    mAxWorkbook->dynamicCall("SaveAs(const QString &)", QDir::toNativeSeparators(filename));
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
    qDebug() << QString("Workbook SaveAs(%1)").arg(filename);
#endif
    return true;
}

/**
 * @brief 关闭工作簿
 */
void DAAxObjectExcelWrapper::PrivateData::close()
{
    if (isHaveWorkbook()) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug() << "close Workbook";
#endif
        mAxWorkbook->dynamicCall("Close()");
    }
    //    if (isHaveWorkbooks()) {
    //        qDebug() << "close Workbooks";
    //        mAxWorkbooks->dynamicCall("Close()");
    //    }
    mAxWorkbook   = nullptr;
    mAxWorkSheets = nullptr;
}

/**
 * @brief 释放所有资源
 */
void DAAxObjectExcelWrapper::PrivateData::release()
{
    close();
    // 理论所有的AxObject的父类都是mAxApp，因此务必保证mAxApp被删除
    if (isInitialize()) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug() << "quit excel";
#endif
        mAxApp->dynamicCall("Quit()");
    }
    mAxApp.reset();
    mAxWorkbooks  = nullptr;
    mAxWorkbook   = nullptr;
    mAxWorkSheets = nullptr;
    OleUninitialize();
}

/**
 * @brief 激活一个excel工作簿
 */
void DAAxObjectExcelWrapper::PrivateData::activeWorkBook()
{
    if (qaxobject_is_null(mAxWorkbooks)) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug() << "workbooks is null";
#endif
        return;
    }
    // 文件不存在则创建
    mAxWorkbooks->querySubObject("Add");
    mAxWorkbook = mAxApp->querySubObject("ActiveWorkBook");
    if (qaxobject_is_null(mAxWorkbook)) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug() << "cannot get ActiveWorkBook";
#endif
        return;
    }
    mAxWorkSheets = mAxWorkbook->querySubObject("Sheets");  // 获得所有工作表对象
}

/**
 * @brief 获取所有sheet的名称
 * @return
 */
QStringList DAAxObjectExcelWrapper::PrivateData::getSheetsName()
{
    QStringList res;
    int c = getSheetsCount();
    for (int i = 0; i < c; ++i) {
        DAAxObjectExcelSheetWrapper sheet(getSheet(i));
        sheet.setAutoDelete(true);
        res.append(sheet.getName());
    }
    return res;
}

/**
 * @brief sheet数量
 * @return
 */
int DAAxObjectExcelWrapper::PrivateData::getSheetsCount() const
{
    if (qaxobject_is_null(mAxWorkSheets)) {
        return 0;
    }
    return mAxWorkSheets->property("Count").toInt();  // 获取工作表的数量
}

/**
 * @brief 添加sheet，会插入到最后
 * @param name sheet名称
 * @return 新建的sheet的QAxObject指针
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::addSheet(const QString& name)
{
    if (!isHaveWorkbook()) {
        activeWorkBook();
        // 首次激活workbook有个默认的sheet
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug() << QString("when addsheet no workbook ,activeWorkBook first");
#endif
        if (getSheetsCount() >= 1) {
            QAxObject* defaultSheet = getSheet(0);
            DAAxObjectExcelSheetWrapper s(defaultSheet);
            s.setName(name);
            return defaultSheet;
        }
    }
    if (qaxobject_is_null(mAxWorkSheets)) {
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
        qDebug() << "WorkSheets is NULL";
#endif
        return nullptr;
    }
    int count = getSheetsCount();
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
    qDebug() << QString("sheet count = %1").arg(count);
#endif
    //! mAxWorkSheets->dynamicCall("ADD()");会在最前面添加，并不是添加到最后
    //! mAxWorkSheets->dynamicCall("ADD()");
    //! QAxObject* newSheet = mAxWorkSheets->querySubObject("Item(int)", count + 1);
    QAxObject* lastSheet = mAxWorkSheets->querySubObject("Item(int)", count);
    if (qaxobject_is_null(lastSheet)) {
        return nullptr;
    }
    mAxWorkSheets->querySubObject("Add(QVariant)", lastSheet->asVariant());
    QAxObject* newSheet = mAxWorkSheets->querySubObject("Item(int)", count);
    if (qaxobject_is_null(newSheet)) {
        return nullptr;
    }
    lastSheet->dynamicCall("Move(QVariant)", newSheet->asVariant());
#if DAAXOFFICEWRAPPER_DEBUG_PRINT
    qDebug() << QString("WorkSheets count %1").arg(getSheetsCount());
#endif
    DAAxObjectExcelSheetWrapper s(newSheet);
    s.setName(name);
    return newSheet;
}

/**
 * @brief 获取sheet，注意此函数是0-base
 * @param sheetIndex sheet索引（0-base）
 * @return sheet的QAxObject指针
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::getSheet(int sheetIndex)
{
    if (!isHaveWorkbook()) {
        activeWorkBook();
    }
    if (qaxobject_is_null(mAxWorkSheets)) {
        return nullptr;
    }
    return mAxWorkSheets->querySubObject("Item(int)", sheetIndex + 1);
}

/**
 * @brief 获取sheet
 * @param name 0-base
 * @return
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::getSheet(const QString& name)
{
    if (!isHaveWorkbook()) {
        activeWorkBook();
    }
    if (qaxobject_is_null(mAxWorkSheets)) {
        return nullptr;
    }
    return mAxWorkSheets->querySubObject("Item(QString)", name);
}

/**
 * @brief 获取第一个sheet
 * @return sheet的QAxObject指针
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::getFirstSheet()
{
    return getSheet(0);
}

/**
 * @brief 获取最后的sheet
 * @return
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::getLastSheet()
{
    int i = getSheetsCount();
    return getSheet(i - 1);
}

/**
 * @brief 设置当前激活的sheet
 * @param index
 * @return
 */
bool DAAxObjectExcelWrapper::PrivateData::setCurrentSheet(int index)
{
    DAAxObjectExcelSheetWrapper sheet(getSheet(index));
    sheet.setAutoDelete(true);
    if (sheet.isNull()) {
        return false;
    }
    sheet.setActive();
    return true;
}

/**
 * @brief 设置当前激活的sheet
 * @param name sheet名称
 * @return 成功返回true
 */
bool DAAxObjectExcelWrapper::PrivateData::setCurrentSheet(const QString& name)
{
    DAAxObjectExcelSheetWrapper sheet(getSheet(name));
    sheet.setAutoDelete(true);
    if (sheet.isNull()) {
        return false;
    }
    sheet.setActive();
    return true;
}

/**
 * @brief 通过sheet名字查找sheet索引
 * @param name sheet名称
 * @return sheet索引，未找到返回-1
 */
int DAAxObjectExcelWrapper::PrivateData::indexOfSheetName(const QString& name)
{
    DAAxObjectExcelSheetWrapper sheet(getSheet(name));
    sheet.setAutoDelete(true);
    if (sheet.isNull()) {
        return -1;
    }
    return sheet.getIndex();
}

/**
 * @brief 获取当前激活的sheet
 * @return sheet的QAxObject指针
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::getActiveSheet()
{
    if (qaxobject_is_null(mAxWorkSheets)) {
        return nullptr;
    }
    return mAxWorkSheets->querySubObject("ActiveSheet");
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

/**
 * @brief 获取sheet的索引
 * @param sheetObj sheet对象
 * @return sheet索引
 */
int DAAxObjectExcelWrapper::PrivateData::getSheetIndex(QAxObject* sheetObj)
{
    return sheetObj->property("Index").toInt();
}

/**
 * @brief 获取整个sheet的所有数据
 * @param sheetObj
 * @return
 */
QVariant DAAxObjectExcelWrapper::PrivateData::getSheetAllData(QAxObject* sheetObj)
{
    DAAxObjectExcelSheetWrapper sheet(sheetObj);
    return sheet.getAllData();
}

/**
 * @brief 获取WorkSheets下所有的sheet
 * @param workSheetsObj
 * @return
 */
QList< QAxObject* > DAAxObjectExcelWrapper::PrivateData::getAllSheet(QAxObject* workSheetsObj)
{
    int cnt = getWorkSheetsCount(workSheetsObj);
    QList< QAxObject* > axSheets;
    for (int i = 1; i <= cnt; ++i) {
        QAxObject* ax = workSheetsObj->querySubObject("Item(int)", i);
        if (ax && !(ax->isNull())) {
            axSheets.append(ax);
        }
    }
    return axSheets;
}

//----------------------------------------------------
// DAAxObjectExcelWrapper
//----------------------------------------------------

/**
 * @brief 构造函数
 * @param par 父对象
 */
DAAxObjectExcelWrapper::DAAxObjectExcelWrapper(QObject* par) : QObject(par), DA_PIMPL_CONSTRUCT
{
}

/**
 * @brief 析构函数
 */
DAAxObjectExcelWrapper::~DAAxObjectExcelWrapper()
{
}

/**
 * @brief 判断是否有效
 * @return 有效返回true
 */
bool DAAxObjectExcelWrapper::isValid() const
{
    return d_ptr->isInitialize();
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

/**
 * @brief 打开excel
 * @param filename excel文件名
 * @param visible 是否可见
 * @param displayAlerts 是否显示警告
 * @return 成功返回true
 * @note 如果没有excel文件，此函数会返回false
 */
bool DAAxObjectExcelWrapper::open(const QString& filename, bool visible, bool displayAlerts)
{
    return d_ptr->open(filename, visible, displayAlerts);
}

/**
 * @brief 打开excel
 * @param filename excel文件名
 * @param visible 是否可见
 * @param displayAlerts 是否显示警告
 * @return 成功返回true
 */
bool DAAxObjectExcelWrapper::create(const QString& filename, bool visible, bool displayAlerts)
{
    return d_ptr->create(filename, visible, displayAlerts);
}

/**
 * @brief 保存
 * @return 成功返回true
 */
bool DAAxObjectExcelWrapper::save()
{
    return d_ptr->save();
}

/**
 * @brief 另存为
 * @param filename 文件名
 * @return 成功返回true
 */
bool DAAxObjectExcelWrapper::saveAs(const QString& filename)
{
    return d_ptr->saveAs(filename);
}

/**
 * @brief 获取所有的sheet名称
 * @return
 */
QStringList DAAxObjectExcelWrapper::getSheetsName() const
{
    return d_ptr->getSheetsName();
}

/**
 * @brief 获取sheet的数量
 * @return
 */
int DAAxObjectExcelWrapper::getSheetsCount() const
{
    return d_ptr->getSheetsCount();
}

/**
 *
 * @brief 设置当前sheet
 * @param index 0 base
 * @return
 */
bool DAAxObjectExcelWrapper::setCurrentSheet(int index)
{
    return d_ptr->setCurrentSheet(index);
}

/**
 * @brief 设置当前sheet
 * @param name
 * @return
 */
bool DAAxObjectExcelWrapper::setCurrentSheet(const QString& name)
{
    return d_ptr->setCurrentSheet(name);
}

/**
 * @brief 通过sheet名字查找sheet索引
 * @param name
 * @return
 */
int DAAxObjectExcelWrapper::indexOfSheetName(const QString& name) const
{
    return d_ptr->indexOfSheetName(name);
}

/**
 * @brief 当前激活sheet的名字
 * @return
 */
QString DAAxObjectExcelWrapper::getActiveSheetName() const
{
    DAAxObjectExcelSheetWrapper s = getActiveSheet();
    if (s.isNull()) {
        return QString();
    }
    return s.getName();
}

/**
 * @brief 读取所有数据
 * @param sheetIndex
 * @return
 */
DATable< QVariant > DAAxObjectExcelWrapper::readTable(int sheetIndex) const
{
    DATable< QVariant > res;
    DAAxObjectExcelSheetWrapper sheet = getSheet(sheetIndex);
    sheet.setAutoDelete(true);
    if (sheet.isNull()) {
        return res;
    }
    return sheet.readTable();
}

/**
 * @brief 读取当前sheet的所有数据
 * @return 表格数据
 */
DATable< QVariant > DAAxObjectExcelWrapper::readCurrentTable() const
{
    DATable< QVariant > res;
    DAAxObjectExcelSheetWrapper sheet = getActiveSheet();
    sheet.setAutoDelete(true);
    if (sheet.isNull()) {
        return res;
    }
    return sheet.readTable();
}

/**
 * @brief 获取当前的sheet
 * @return
 */
DAAxObjectExcelSheetWrapper DAAxObjectExcelWrapper::getActiveSheet() const
{
    return DAAxObjectExcelSheetWrapper(d_ptr->getActiveSheet());
}

/**
 * @brief 获取sheet
 * @param sheetIndex
 * @return
 */
DAAxObjectExcelSheetWrapper DAAxObjectExcelWrapper::getSheet(int sheetIndex) const
{
    return DAAxObjectExcelSheetWrapper(d_ptr->getSheet(sheetIndex));
}

/**
 * @brief 获取第一个sheet
 * @return sheet包装器
 */
DAAxObjectExcelSheetWrapper DAAxObjectExcelWrapper::getFirstSheet() const
{
    return DAAxObjectExcelSheetWrapper(d_ptr->getFirstSheet());
}

/**
 * @brief 获取最后一个sheet
 * @return sheet包装器
 */
DAAxObjectExcelSheetWrapper DAAxObjectExcelWrapper::getLastSheet() const
{
    return DAAxObjectExcelSheetWrapper(d_ptr->getLastSheet());
}

/**
 * @brief 关闭
 */
void DAAxObjectExcelWrapper::close()
{
    d_ptr->close();
}

/**
 * @brief 添加sheet
 *
 * 会插入到最后
 * @param name
 * @return
 */
DAAxObjectExcelSheetWrapper DAAxObjectExcelWrapper::addSheet(const QString& name)
{
    return DAAxObjectExcelSheetWrapper(d_ptr->addSheet(name));
}

/**
 * @brief 读取excel第sheetIndex的sheet的数据
 * @param filename excel文件名
 * @param sheetIndex sheet索引（0-base）
 * @param errString 错误信息输出
 * @return 表格数据
 */
DATable< QVariant > DAAxObjectExcelWrapper::readExcelSheet(const QString& filename, int sheetIndex, QString* errString)
{
    DATable< QVariant > res;
    DAAxObjectExcelWrapper excel;
    if (!excel.isValid()) {
        if (errString) {
            *errString = DAAxObjectExcelWrapper::tr("The local computer does not have Excel or WPS installed");  // cn:当前计算机中没有安装excel或者wps
        }
        return res;
    }
    if (!excel.open(filename)) {
        if (errString) {
            *errString = DAAxObjectExcelWrapper::tr("cannot open Excel");  // cn:无法打开Excel
        }
        return res;
    }
    res = excel.readTable(sheetIndex);
    return res;
}

/**
 * @brief 把table写入excel中
 * @param filename excel文件名
 * @param sheetName sheet名称
 * @param table 表格数据
 * @param appendLast 是否追加到最后
 * @param errString 错误信息输出
 * @return 成功返回true
 */
bool DAAxObjectExcelWrapper::writeExcel(const QString& filename,
                                        const QString& sheetName,
                                        const DATable< QVariant >& table,
                                        bool appendLast,
                                        QString* errString)
{
    DAAxObjectExcelWrapper excel;
    if (!excel.isValid()) {
        if (errString) {
            *errString = DAAxObjectExcelWrapper::tr("The local computer does not have Excel or WPS installed");  // cn:当前计算机中没有安装excel或者wps
        }
        return false;
    }
    DAAxObjectExcelSheetWrapper sheet;
    if (excel.getSheetsCount() <= 0) {
        sheet = excel.addSheet(sheetName);
    } else {
        if (appendLast) {
            sheet = excel.addSheet(sheetName);
        } else {
            sheet = excel.getFirstSheet();
            sheet.setName(sheetName);
        }
    }
    if (sheet.isNull()) {
        if (errString) {
            *errString = DAAxObjectExcelWrapper::tr("cannot get or create sheet");  // cn:无法获取或创建工作表
        }
        return false;
    }
    if (!sheet.writeTable(table)) {
        if (errString) {
            *errString = DAAxObjectExcelWrapper::tr("failed to write table to sheet");  // cn:写入表格数据到sheet失败
        }
        return false;
    }
    if (!excel.saveAs(filename)) {
        if (errString) {
            *errString = DAAxObjectExcelWrapper::tr("failed to save Excel file");  // cn:保存Excel文件失败
        }
        return false;
    }
    return true;
}

}  // end DA
