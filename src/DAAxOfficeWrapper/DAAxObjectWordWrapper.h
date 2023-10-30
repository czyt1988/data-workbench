#ifndef DAAXOBJECTWORDWRAPPER_H
#define DAAXOBJECTWORDWRAPPER_H
#include <QObject>
#include <QAxObject>
#include "DAAxOfficeWrapperGlobal.h"

namespace DA
{
class DAAxObjectWordTableWrapper;
/**
 * @brief word操作封装
 *
 *下面这个例子是用dot模板实现word的修改的案例，提前准备好一个word的模板，并定义好标签，通过@ref replaceMark , 以及@ref insertPictureAtMark 等函数即可实现模板的修改
 *
 * @code
 * QString templatePath;
 * ...
 * DAAxObjectWordWrapper word;
 * if (!word.isInitialize()) {
 *     return;
 * }
 * if (!word.open(templatePath)) {
 *     return;
 * }
 * word.replaceMark("da_create_year", QDate::currentDate().year());
 * word.replaceMark("da_create_month", QDate::currentDate().month());
 * word.insertPictureAtMark("da_picture_air_topology", mTopologyPicturePath);
 * @endcode
 *
 * 针对有表格的操作，可以通过@ref insertTableAtMark 函数插入一个表格，返回一个@sa
 *DAAxObjectWordTableWrapper 对象 ，通过操作@sa DAAxObjectWordTableWrapper 对象实现表格的插入
 *
 *@code
 * DAColumnTable< QString > table; // 这是一个内存表
 * ......
 * DAAxObjectWordTableWrapper tableAx = word->insertTableAtMark(mark,table.rowCount() + 1, table.columnCount());
 * if (!tableAx.isNull()) {
 *     //先写表头
 *     int cc             = table.columnCount();
 *     QStringList header = table.columnNames();
 *     for (int i = 0; i < cc; ++i) {
 *         tableAx.setCellText(0, i, header[ i ]);
 *     }
 *     //写表内容
 *     int rr = table.rowCount();
 *     for (int r = 0; r < rr; ++r) {
 *         for (int c = 0; c < cc; ++c) {
 *             tableAx.setCellText(r + 1, c, table.cell(r, c));
 *         }
 *     }
 * }
 *@endcode
 */
class DAAXOFFICEWRAPPER_API DAAxObjectWordWrapper : public QObject
{
    Q_OBJECT
    DA_DECLARE_PRIVATE(DAAxObjectWordWrapper)
public:
    DAAxObjectWordWrapper(QObject* par = nullptr);
    ~DAAxObjectWordWrapper();
    //重新加载app
    void reloadApp();
    //打开文件
    bool open(const QString& docfile, bool isvisible = false);
    //判断是否初始化完成
    bool isInitialize() const;
    //是否存在文档
    bool isHaveDocument() const;
    //退出app
    void quit();
    //关闭文档
    void close();
    //保存
    void save();
    //另存
    bool saveAs(const QString& docfile);
    //替换标签内容
    bool replaceMark(const QString& markName, const QVariant& replaceContent);
    //在mark（书签）位置添加文本，
    bool insertTextAtMark(const QString& markName, const QString& replaceContent);
    //在书签处插入图片
    bool insertPictureAtMark(const QString& markName, const QString& picturePath);
    //在书签处插入表格
    DAAxObjectWordTableWrapper insertTableAtMark(const QString& markName, int rowCount, int colCount);
};
}

#endif  // DAAXOBJECTWORDWRAPPER_H
