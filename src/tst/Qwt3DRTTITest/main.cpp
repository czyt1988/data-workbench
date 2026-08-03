#include <QtTest/QtTest>
#include <QIODevice>
#include "qwt3d_types.h"
#include "qwt3d_plotitem.h"
#include "qwt3d_surface.h"
#include "qwt3d_bar.h"
#include "qwt3d_line3d.h"
#include "qwt3d_theme.h"
#include "qwt3d_serialize.h"

class Qwt3DRTTITest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    // RTTI 验证
    void testSurfaceRTTI();
    void testBarRTTI();
    void testLineRTTI();
    void testBaseRTTIviaSubclass();

    // 值类型序列化验证
    void testTripleSerialization();
    void testRGBASerialization();
    void testQwt3DThemeSerialization();
};

// ============================================================
// RTTI 验证
// ============================================================

void Qwt3DRTTITest::testSurfaceRTTI()
{
    // Qwt3DSurface 构造不需要 OpenGL 上下文（VBO/VAO 在 draw() 中延迟创建）
    Qwt3DSurface surface;
    QCOMPARE(surface.rtti(), Rtti_Plot3DSurface);  // 1001
}

void Qwt3DRTTITest::testBarRTTI()
{
    Qwt3DBar bar;
    QCOMPARE(bar.rtti(), Rtti_Plot3DBar);  // 1002
}

void Qwt3DRTTITest::testLineRTTI()
{
    Qwt3DLine line;
    QCOMPARE(line.rtti(), Rtti_Plot3DLine);  // 1003
}

void Qwt3DRTTITest::testBaseRTTIviaSubclass()
{
    // Qwt3DPlotItem 是抽象类（draw()=0, hull()=0），无法直接实例化
    // 通过子类对象调用基类 rtti() 应返回子类的值
    Qwt3DSurface surface;
    Qwt3DPlotItem* item = &surface;
    QCOMPARE(item->rtti(), Rtti_Plot3DSurface);
}

// ============================================================
// 值类型序列化验证
// ============================================================

void Qwt3DRTTITest::testTripleSerialization()
{
    Triple original(1.0, 2.0, 3.0);
    QByteArray data;
    {
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setVersion(qwt3d_datastream_version);
        out << original;
    }
    Triple restored;
    {
        QDataStream in(&data, QIODevice::ReadOnly);
        in.setVersion(qwt3d_datastream_version);
        in >> restored;
    }
    QCOMPARE(restored, original);
}

void Qwt3DRTTITest::testRGBASerialization()
{
    RGBA original(0.1, 0.2, 0.3, 0.9);
    QByteArray data;
    {
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setVersion(qwt3d_datastream_version);
        out << original;
    }
    RGBA restored;
    {
        QDataStream in(&data, QIODevice::ReadOnly);
        in.setVersion(qwt3d_datastream_version);
        in >> restored;
    }
    QCOMPARE(restored.r, original.r);
    QCOMPARE(restored.g, original.g);
    QCOMPARE(restored.b, original.b);
    QCOMPARE(restored.a, original.a);
}

void Qwt3DRTTITest::testQwt3DThemeSerialization()
{
    Qwt3DTheme theme = Qwt3DTheme::create(Qwt3DTheme::Dark);
    QByteArray themeData;
    {
        QDataStream out(&themeData, QIODevice::WriteOnly);
        out.setVersion(qwt3d_datastream_version);
        out << theme;
    }
    Qwt3DTheme restoredTheme;
    {
        QDataStream in(&themeData, QIODevice::ReadOnly);
        in.setVersion(qwt3d_datastream_version);
        in >> restoredTheme;
    }
    QCOMPARE(restoredTheme.backgroundColor().r, theme.backgroundColor().r);
    QCOMPARE(restoredTheme.meshColor().r, theme.meshColor().r);
    QCOMPARE(restoredTheme.lightingPreset(), theme.lightingPreset());
    QCOMPARE(restoredTheme.plotStyle(), theme.plotStyle());
    QCOMPARE(restoredTheme.shading(), theme.shading());
    QCOMPARE(restoredTheme.dataColorPreset(), theme.dataColorPreset());
    QCOMPARE(restoredTheme.shininess(), theme.shininess());
    QCOMPARE(restoredTheme.specularIntensity(), theme.specularIntensity());
}

// QtTest 的 main 函数通过 QTEST_MAIN 宏自动生成
QTEST_MAIN(Qwt3DRTTITest)
#include "main.moc"
