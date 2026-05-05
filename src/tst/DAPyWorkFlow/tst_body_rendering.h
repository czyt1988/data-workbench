#pragma once

#include <QObject>

namespace DA
{

/**
 * @brief 节点体渲染 TDD 测试
 *
 * 验证 DAPyNodeGraphicsItem 的椭圆/圆角矩形渲染、
 * 名称位置布局、图标布局、状态装饰裁剪、椭圆碰撞形状及圆角半径配置。
 * 全部为纯 C++ 像素/几何测试，无 Python 依赖。
 */
class TestBodyRendering : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // 椭圆体渲染：中心像素有填充色，角落像素无填充色
    void testEllipseBodyRendering();

    // 圆角矩形向后兼容：RoundedRect 渲染与旧版一致
    void testRoundedRectBackCompat();

    // 名称位于下方：boundingRect 高度 > bodyRect 高度
    void testNameBelowBoundingRect();

    // 名称位于内部：boundingRect 不向下扩展
    void testNameInsideNoExpansion();

    // 图标在文本上方渲染
    void testIconAboveLayout();

    // 图标在文本左侧渲染（向后兼容）
    void testIconLeftBackCompat();

    // 状态装饰裁剪到椭圆：角落不填充状态色
    void testStateDecorationClippedToEllipse();

    // 椭圆碰撞形状：中心命中，角落不命中
    void testEllipseShapeHitTest();

    // 圆角半径可配置：cornerRadius=10 渲染更大的圆角
    void testCornerRadiusConfigurable();
};

}  // namespace DA