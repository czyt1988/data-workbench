#ifndef DACOLORTHEME_H
#define DACOLORTHEME_H
#include "DAIndexedVector.h"
#include <initializer_list>
#include <QColor>
namespace DA
{
/**
 * @brief 颜色主题
 */
class DAColorTheme
{
public:
    /**
     * @brief 已有的主题
     *
     * 配色参考：https://github.com/BlakeRMills/MetBrewer
     *
     * 自动化生成代码见：src/PyScripts/create_color_theme.py
     */
    enum Theme
    {
        ColorTheme_Archambault,
        ColorTheme_Cassatt1,
        ColorTheme_Cassatt2,
        ColorTheme_Demuth,
        ColorTheme_Derain,
        ColorTheme_Egypt,
        ColorTheme_Greek,
        ColorTheme_Hiroshige,
        ColorTheme_Hokusai2,
        ColorTheme_Hokusai3,
        ColorTheme_Ingres,
        ColorTheme_Isfahan1,
        ColorTheme_Isfahan2,
        ColorTheme_Java,
        ColorTheme_Johnson,
        ColorTheme_Kandinsky,
        ColorTheme_Morgenstern,
        ColorTheme_OKeeffe1,
        ColorTheme_OKeeffe2,
        ColorTheme_Pillement,
        ColorTheme_Tam,
        ColorTheme_Troy,
        ColorTheme_VanGogh3,
        ColorTheme_Veronese,
        ColorTheme_End,               ///< 结束
        ColorTheme_UserDefine = 2000  ///< 用户自定义
    };

public:
    DAColorTheme();
    DAColorTheme(const std::initializer_list< QColor >& v);
    ~DAColorTheme();
    //颜色个数
    int size() const;
    //是否为空
    bool isEmpty() const;
    //获取颜色
    QColor at(int index) const;
    //插入颜色
    void append(const QColor& c);
    //获取下一个主题颜色
    QColor next();
    //获取前一个主题颜色
    QColor previous();
    //获取颜色队列
    QVector< QColor >& colorVector();
    const QVector< QColor >& colorVector() const;
    //获取当前的索引
    int getCurrentIndex() const;
    //设置当前的索引
    void setCurrentIndex(int v);
    //创建一个color theme
    static DAColorTheme create(Theme t);

private:
    DAIndexedVector< QColor > mColorVector;
};

//创建一个color theme
DAColorTheme createColorTheme(DAColorTheme::Theme t);
}

#endif  // DACOLORTHEME_H
