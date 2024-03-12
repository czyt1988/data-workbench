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
    ~PrivateData();
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
    //创建excel
    bool create(const QString& filename, bool visible = false, bool displayAlerts = false);
    //保存
    bool save();
    //保存
    bool saveAs(const QString& filename);
    //关闭
    void close();
    //释放
    void release();
    //加载所有的sheet,返回获取到的sheet的数量
    int loadAllSheet();
    //获取所有sheets的名称
    QStringList getSheetsName() const;
    //获取sheet的所有数据
    DATable< QVariant > getSheetAllData(int sheetIndex = 0);
    //获取sheet
    QAxObject* getSheet(int sheetIndex) const;

public:
    //获取第n个sheet
    static QAxObject* getWorkSheet(QAxObject* workSheetsObj, int indexOneBase);
    //获取sheets数量
    static int getWorkSheetsCount(QAxObject* workSheetsObj);
    //获取worksheet下所有的sheets
    static QList< QAxObject* > getAllSheet(QAxObject* workSheetsObj);
    //获取sheet的名字
    static QString getSheetName(QAxObject* sheetObj);
    //获取整个sheet的所有数据
    QVariant getSheetAllData(QAxObject* sheetObj);

public:
    const QString cAppComControlName { "Excel.Application" };  ///< excel的程序名
    const QString cAppComControlName2 { "kwps.Application" };  ///< excel的程序名
    std::unique_ptr< QAxObject > mAxApp;                       ///< Excel.Application
    QAxObject* mAxWorkbooks { nullptr };                       ///< Workbooks,父对象是mApp
    QAxObject* mAxWorkbook { nullptr };                        ///< Workbook,父对象是mAxWorkbooks/mApp
    QAxObject* mAxWorkSheets { nullptr };                      ///< WorkSheets,
    QList< QAxObject* > mAxSheets;                             ///< 存放sheets
};

DAAxObjectExcelWrapper::PrivateData::PrivateData(DAAxObjectExcelWrapper* p) : q_ptr(p)
{
    initialize();
}

DAAxObjectExcelWrapper::PrivateData::~PrivateData()
{
    release();
}

bool DAAxObjectExcelWrapper::PrivateData::initialize(bool visible, bool displayAlerts)
{
    mAxApp = std::make_unique< QAxObject >(q_ptr);
    if (!tryCreateComControl(mAxApp.get())) {
        return false;
    }
    setWindowVisible(visible);
    setDisplayAlerts(displayAlerts);
    mAxWorkbooks = mAxApp->querySubObject("Workbooks");
    return true;
}

bool DAAxObjectExcelWrapper::PrivateData::isInitialize() const
{
    return checkAxObjectValid(mAxApp.get());
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
    return checkAxObjectValid(mAxWorkbook);
}

bool DAAxObjectExcelWrapper::PrivateData::isHaveWorkSheets() const
{
    return checkAxObjectValid(mAxWorkSheets);
}

bool DAAxObjectExcelWrapper::PrivateData::tryCreateComControl(QAxObject* obj)
{
    bool res = obj->setControl(cAppComControlName);
    if (!res) {
        res = obj->setControl(cAppComControlName2);
    }
    return res;
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
    if (!QFile::exists(nativeFileName)) {
        qWarning() << QObject::tr("file \"%1\" does not exist").arg(nativeFileName);  // cn:文件“%1”不存在
        return false;
    }
    //文件存在
    mAxWorkbooks = mAxWorkbooks->querySubObject("Open(const QString &)", nativeFileName);
    mAxWorkbook  = mAxApp->querySubObject("ActiveWorkBook");
    if (!isHaveWorkbook()) {
        qWarning() << QObject::tr("can not get excel workbook");  // cn:无法获取excel的workbook
        return false;
    }
    mAxWorkSheets = mAxWorkbook->querySubObject("Sheets");  //获得所有工作表对象
    //加载WorkSheets下的所有sheet
    loadAllSheet();
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
        qWarning() << QObject::tr("unable initialize %1").arg(cAppComControlName);  // cn:无法初始化%1
        return false;
    }
    //文件不存在则创建
    mAxWorkbooks->dynamicCall("Add");
    mAxWorkbook = mAxApp->querySubObject("ActiveWorkBook");
    if (!isHaveWorkbook()) {
        qWarning() << QObject::tr("can not get excel workbook");  // cn:无法获取excel的workbook
        return false;
    }
    saveAs(filename);
    mAxWorkSheets = mAxWorkbook->querySubObject("Sheets");  //获得所有工作表对象
    //加载WorkSheets下的所有sheet
    loadAllSheet();
    return true;
}

bool DAAxObjectExcelWrapper::PrivateData::save()
{
    if (!isHaveWorkbook()) {
        return false;
    }
    mAxWorkbook->dynamicCall("Save()");
    return true;
}

bool DAAxObjectExcelWrapper::PrivateData::saveAs(const QString& filename)
{
    if (!isHaveWorkbook()) {
        return false;
    }
    mAxWorkbook->dynamicCall("SaveAs(const QString &)", QDir::toNativeSeparators(filename));
    return true;
}

void DAAxObjectExcelWrapper::PrivateData::close()
{
    if (isHaveWorkbook()) {
        qDebug() << "close Workbook";
        mAxWorkbook->dynamicCall("Close()");
    }
    //    if (isHaveWorkbooks()) {
    //        qDebug() << "close Workbooks";
    //        mAxWorkbooks->dynamicCall("Close()");
    //    }
}

void DAAxObjectExcelWrapper::PrivateData::release()
{
    close();
    //理论所有的AxObject的父类都是mAxApp，因此务必保证mAxApp被删除
    if (isInitialize()) {
        qDebug() << "quit excel";
        mAxApp->dynamicCall("Quit()");
    }
    mAxApp.reset(nullptr);
    mAxApp        = nullptr;
    mAxWorkbooks  = nullptr;
    mAxWorkbook   = nullptr;
    mAxWorkSheets = nullptr;
}

/**
 * @brief 加载所有的sheet,返回获取到的sheet的数量
 */
int DAAxObjectExcelWrapper::PrivateData::loadAllSheet()
{
    if (!isHaveWorkSheets()) {
        return 0;
    }
    QList< QAxObject* > res = getAllSheet(mAxWorkSheets);
    if (res.empty()) {
        return 0;
    }
    //删除mAxSheets
    for (QAxObject* ax : qAsConst(mAxSheets)) {
        if (ax && !ax->isNull()) {
            delete ax;
        }
    }
    mAxSheets.clear();
    mAxSheets = res;
    return res.size();
}

/**
 * @brief 获取所有sheet的名称
 * @return
 */
QStringList DAAxObjectExcelWrapper::PrivateData::getSheetsName() const
{
    QStringList res;
    for (QAxObject* ax : mAxSheets) {
        res.append(getSheetName(ax));
    }
    return res;
}

/**
 * @brief 获取整个sheet的所有数据
 * @param sheetIndex 0base index
 * @return
 */
DATable< QVariant > DAAxObjectExcelWrapper::PrivateData::getSheetAllData(int sheetIndex)
{
    DATable< QVariant > res;
    QAxObject* axSheet = getSheet(sheetIndex);
    if (nullptr == axSheet) {
        return res;
    }
    QVariant var         = getSheetAllData(axSheet);
    QVariantList varRows = var.toList();
    if (varRows.isEmpty()) {
        return res;
    }
    const int rowCount = varRows.size();
    for (int r = 0; r < rowCount; ++r) {
        QVariantList rowData;
        rowData = varRows[ r ].toList();
        for (int c = 0; c < rowData.size(); ++c) {
            res[ { r, c } ] = rowData[ c ];
        }
    }
    return res;
}

/**
 * @brief 获取sheet
 * @param sheetIndex
 * @return
 */
QAxObject* DAAxObjectExcelWrapper::PrivateData::getSheet(int sheetIndex) const
{
    if (sheetIndex < mAxSheets.size()) {
        return mAxSheets.at(sheetIndex);
    }
    return nullptr;
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
 * @brief 获取整个sheet的所有数据
 * @param sheetObj
 * @return
 */
QVariant DAAxObjectExcelWrapper::PrivateData::getSheetAllData(QAxObject* sheetObj)
{
    QVariant var;

    QAxObject* usedRange = sheetObj->querySubObject("UsedRange");
    if (nullptr == usedRange || usedRange->isNull()) {
        return var;
    }
    var = usedRange->dynamicCall("Value");
    delete usedRange;

    return var;
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
        QAxObject* ax = workSheetsObj->querySubObject("Item(int)", cnt);
        if (ax && !(ax->isNull())) {
            axSheets.append(ax);
        }
    }
    return axSheets;
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

bool DAAxObjectExcelWrapper::save()
{
    return d_ptr->save();
}

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
 * @brief 读取所有数据
 * @param sheetIndex
 * @return
 */
DATable< QVariant > DAAxObjectExcelWrapper::readAllData(int sheetIndex) const
{
    return d_ptr->getSheetAllData(sheetIndex);
}

/**
 * @brief 关闭
 */
void DAAxObjectExcelWrapper::close()
{
    d_ptr->close();
}

DATable< QVariant > DAAxObjectExcelWrapper::readAllData(const QString& filename, int sheetIndex, QString* errString)
{
    DATable< QVariant > res;
    DAAxObjectExcelWrapper excel;
    if (!excel.isValid()) {
        if (errString) {
            *errString = QObject::tr("The local computer does not have Excel or WPS installed");  //当前计算机中没有安装excel或者wps
        }
        return res;
    }
    if (!excel.open(filename)) {
        return res;
    }
    res = excel.readAllData(sheetIndex);
    return res;
}

}  // end DA
