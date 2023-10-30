#ifndef DAAXOBJECTWORDTABLEWRAPPER_H
#define DAAXOBJECTWORDTABLEWRAPPER_H
#include <QObject>
#include <QAxObject>
#include "DAAxOfficeWrapperGlobal.h"
namespace DA
{
/**
 * @brief 针对word的表格的封装
 */
class DAAXOFFICEWRAPPER_API DAAxObjectWordTableWrapper
{
public:
    /**
     * @brief 表格自动拉伸列 0固定  1根据内容调整  2 根据窗口调整
     */
    enum AutoFitBehavior
    {
        FitBehaviorFixed         = 0,  ///< 固定
        FitBehaviorAdjustContent = 1,  ///< 根据内容调整
        FitBehaviorAdjustWindow  = 2   ///< 根据窗口调整
    };

public:
    DAAxObjectWordTableWrapper(QAxObject* p);
    bool isNull() const;
    QAxObject* axObject() const;
    //设置表格自动拉伸模式
    void setAutoFitBehavior(AutoFitBehavior v);
    //获取行数
    int rowCount() const;
    //获取列数
    int columnCount() const;
    //获取单元格,注意，row，col从0开始算
    QAxObject* cell(int row, int col);
    //选中一个cell的内容,注意，row，col从0开始算
    QAxObject* selectCellRange(int row, int col);
    //设置文本，注意，row，col从0开始算
    void setCellText(int row, int col, const QString& text);

private:
    QAxObject* mAxTableObject { nullptr };
};
}

#endif  // DAAXOBJECTWORDTABLEWRAPPER_H
