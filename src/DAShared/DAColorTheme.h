#ifndef DACOLORTHEME_H
#define DACOLORTHEME_H
#include "DAIndexedVector.h"
#include <initializer_list>
#include <QColor>
#include <QDebug>
namespace DA
{

/**
 * @brief 颜色主题
 */
class DAColorTheme : public DAIndexedVector< QColor >
{
public:
	/**
	 * @brief 已有的主题
	 *
	 * 配色参考：https://github.com/BlakeRMills/MetBrewer
	 *
	 * 自动化生成代码见：src/PyScripts/create_color_theme.py
	 */
	enum ColorTheme
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
    DAColorTheme() : DAIndexedVector< QColor >()
    {
    }
    DAColorTheme(ColorTheme th) : DAIndexedVector< QColor >()
    {
        *this = DAColorTheme::create(th);
    }
    DAColorTheme(const std::initializer_list< QColor >& v) : DAIndexedVector< QColor >(v)
    {
    }
    ~DAColorTheme()
    {
    }

    /**
     * @brief 创建一个color theme
     * @param t
     * @return
     */
    static DAColorTheme create(ColorTheme t);
    /**
     * @brief 重载等于操作符，可以直接通过主题赋值
     * @param th
     * @return
     */
    DAColorTheme& operator=(const ColorTheme& th)
    {
        *this = DAColorTheme::create(th);
        return *this;
    }

private:
    ColorTheme mCurrentColorTheme { ColorTheme_UserDefine };  ///< 当前的主题
};

DAColorTheme DAColorTheme::create(DAColorTheme::ColorTheme t)
{
    switch (t) {
    case DAColorTheme::ColorTheme_Archambault:
        return DAColorTheme({ QColor("#ed968c"),
                              QColor("#88a0dc"),
                              QColor("#f9d14a"),
                              QColor("#e78429"),
                              QColor("#7c4b73"),
                              QColor("#ab3329"),
                              QColor("#381a61") });
    case DAColorTheme::ColorTheme_Cassatt1:
        return DAColorTheme({ QColor("#e3aba7"),
                              QColor("#8282aa"),
                              QColor("#b1615c"),
                              QColor("#c9c9dd"),
                              QColor("#9d9dc7"),
                              QColor("#d88782"),
                              QColor("#5a5a83"),
                              QColor("#edd7d9") });
    case DAColorTheme::ColorTheme_Cassatt2:
        return DAColorTheme({ QColor("#b695bc"),
                              QColor("#7fa074"),
                              QColor("#574571"),
                              QColor("#2c4b27"),
                              QColor("#dec5da"),
                              QColor("#c1d1aa"),
                              QColor("#2d223c"),
                              QColor("#0e2810"),
                              QColor("#90719f"),
                              QColor("#466c4b") });
    case DAColorTheme::ColorTheme_Demuth:
        return DAColorTheme({ QColor("#b64f32"),
                              QColor("#5d6174"),
                              QColor("#f7c267"),
                              QColor("#b9b9b8"),
                              QColor("#9b332b"),
                              QColor("#41485f"),
                              QColor("#d39a2d"),
                              QColor("#8b8b99"),
                              QColor("#591c19"),
                              QColor("#262d42") });
    case DAColorTheme::ColorTheme_Derain:
        return DAColorTheme({ QColor("#808fe1"),
                              QColor("#97c684"),
                              QColor("#5c66a8"),
                              QColor("#efc86e"),
                              QColor("#6f9969"),
                              QColor("#454a74"),
                              QColor("#aab5d5") });
    case DAColorTheme::ColorTheme_Egypt:
        return DAColorTheme({ QColor("#dd5129"), QColor("#0f7ba2"), QColor("#43b284"), QColor("#fab255") });
    case DAColorTheme::ColorTheme_Greek:
        return DAColorTheme(
            { QColor("#ed9b49"), QColor("#3c0d03"), QColor("#8d1c06"), QColor("#f5c34d"), QColor("#e67424") });
    case DAColorTheme::ColorTheme_Hiroshige:
        return DAColorTheme({ QColor("#72bcd5"),
                              QColor("#ef8a47"),
                              QColor("#ffd06f"),
                              QColor("#376795"),
                              QColor("#aadce0"),
                              QColor("#e76254"),
                              QColor("#ffe6b7"),
                              QColor("#1e466e"),
                              QColor("#f7aa58"),
                              QColor("#528fad") });
    case DAColorTheme::ColorTheme_Hokusai2:
        return DAColorTheme(
            { QColor("#2f70a1"), QColor("#72aeb6"), QColor("#0a3351"), QColor("#4692b0"), QColor("#abc9c8"), QColor("#134b73") });
    case DAColorTheme::ColorTheme_Hokusai3:
        return DAColorTheme(
            { QColor("#295384"), QColor("#95c36e"), QColor("#5a97c1"), QColor("#d8d97a"), QColor("#74c8c3"), QColor("#0a2e57") });
    case DAColorTheme::ColorTheme_Ingres:
        return DAColorTheme({ QColor("#7e5522"),
                              QColor("#d1b252"),
                              QColor("#18527e"),
                              QColor("#041d2c"),
                              QColor("#06314e"),
                              QColor("#2e77ab"),
                              QColor("#a97f2f"),
                              QColor("#472c0b") });
    case DAColorTheme::ColorTheme_Isfahan1:
        return DAColorTheme({ QColor("#178f92"),
                              QColor("#845d29"),
                              QColor("#1d1f54"),
                              QColor("#d8c29d"),
                              QColor("#4e3910"),
                              QColor("#4fb6ca"),
                              QColor("#175f5d") });
    case DAColorTheme::ColorTheme_Isfahan2:
        return DAColorTheme(
            { QColor("#4063a3"), QColor("#ddc000"), QColor("#79ad41"), QColor("#d7aca1"), QColor("#34b6c6") });
    case DAColorTheme::ColorTheme_Java:
        return DAColorTheme(
            { QColor("#663171"), QColor("#ea7428"), QColor("#0c7156"), QColor("#cf3a36"), QColor("#e2998a") });
    case DAColorTheme::ColorTheme_Johnson:
        return DAColorTheme(
            { QColor("#d04e00"), QColor("#0086a8"), QColor("#a00e00"), QColor("#f6c200"), QColor("#132b69") });
    case DAColorTheme::ColorTheme_Kandinsky:
        return DAColorTheme({ QColor("#3b7c70"), QColor("#ce9642"), QColor("#898e9f"), QColor("#3b3a3e") });
    case DAColorTheme::ColorTheme_Morgenstern:
        return DAColorTheme({ QColor("#a56457"),
                              QColor("#db8872"),
                              QColor("#ffb178"),
                              QColor("#dfbbc8"),
                              QColor("#b08ba5"),
                              QColor("#ffc680"),
                              QColor("#7c668c") });
    case DAColorTheme::ColorTheme_OKeeffe1:
        return DAColorTheme({ QColor("#da6c42"),
                              QColor("#447fdd"),
                              QColor("#f6f2ee"),
                              QColor("#ee956a"),
                              QColor("#7db0ea"),
                              QColor("#973d21"),
                              QColor("#225bb2"),
                              QColor("#6b200c"),
                              QColor("#133e7e"),
                              QColor("#fbc2a9"),
                              QColor("#bad6f9") });
    case DAColorTheme::ColorTheme_OKeeffe2:
        return DAColorTheme({ QColor("#f2c88f"),
                              QColor("#d37750"),
                              QColor("#92351e"),
                              QColor("#e69c6b"),
                              QColor("#b9563f"),
                              QColor("#ecb27d"),
                              QColor("#fbe3c2") });
    case DAColorTheme::ColorTheme_Pillement:
        return DAColorTheme(
            { QColor("#2b4655"), QColor("#738e8e"), QColor("#697852"), QColor("#a9845b"), QColor("#44636f"), QColor("#0f252f") });
    case DAColorTheme::ColorTheme_Tam:
        return DAColorTheme({ QColor("#ef8737"),
                              QColor("#bb292c"),
                              QColor("#ffd353"),
                              QColor("#62205f"),
                              QColor("#341648"),
                              QColor("#de4f33"),
                              QColor("#9f2d55"),
                              QColor("#ffb242") });
    case DAColorTheme::ColorTheme_Troy:
        return DAColorTheme({ QColor("#7ba0b4"),
                              QColor("#421401"),
                              QColor("#235070"),
                              QColor("#8b3a2b"),
                              QColor("#c27668"),
                              QColor("#0a2d46"),
                              QColor("#6c1d0e"),
                              QColor("#44728c") });
    case DAColorTheme::ColorTheme_VanGogh3:
        return DAColorTheme({ QColor("#9cc184"),
                              QColor("#1f5b25"),
                              QColor("#1e3d14"),
                              QColor("#669d62"),
                              QColor("#c2d6a4"),
                              QColor("#192813"),
                              QColor("#e7e5cc"),
                              QColor("#447243") });
    case DAColorTheme::ColorTheme_Veronese:
        return DAColorTheme({ QColor("#99610a"),
                              QColor("#6e948c"),
                              QColor("#2c6b67"),
                              QColor("#122c43"),
                              QColor("#67322e"),
                              QColor("#175449"),
                              QColor("#c38f16") });
    default:
        break;
    }
    return DAColorTheme();
}

// QDebug的打印支持
QDebug operator<<(QDebug debug, const DAColorTheme& th)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver);
    debug.nospace() << "[" << th.getCurrentIndex() << "] ";
    for (QColor c : th) {
        debug.nospace() << c.name() << ",";
    }
    return debug;
}

}  // end namespace DA

Q_DECLARE_METATYPE(DA::DAColorTheme)
#endif  // DACOLORTHEME_H
