#ifndef DAABSTRACTWIDGETOVERLAY_H
#define DAABSTRACTWIDGETOVERLAY_H
#include "DAUtilsAPI.h"
#include <QWidget>
class QPainter;
class QRegion;

namespace DA
{

/**
 * @brief 参照QwtWidgetOverlay的Widget Overlay
 *
 * 遮罩层的主要用例是为了避免对下层窗口进行繁重的重绘操作。
 * 与绘图画布结合使用时，覆盖层可以避免重新绘制，因为画布的内容可以从其后备存储中恢复。
 */
class DAUTILS_API DAAbstractWidgetOverlay : public QWidget
{
	DA_DECLARE_PRIVATE(DAAbstractWidgetOverlay)
public:
	/*!
		   \brief 遮罩模式

	   当使用遮罩时，下层窗口仅会为覆盖层的遮罩区域接收绘制事件。
	   否则，Qt 会触发完整的重绘。在性能较低的硬件（如嵌入式系统）上，
	   或者在远程桌面上使用光栅绘制引擎时，位块传输是一个明显的操作，需要避免。

		是否以及如何使用遮罩取决于遮罩计算的复杂度和遮罩可以排除多少像素。

	   默认设置为 MaskHint。

		\sa setMaskMode(), maskMode()
	  */
	enum MaskMode
	{
		//! 不使用遮罩。
		NoMask,

		/*!
		   \brief 使用 maskHint() 作为遮罩

			   在许多情况下，快速近似就足够了，没有必要构建更详细的遮罩
			   （例如文本的边界矩形）。
			 */
		MaskHint,

		/*!
		   \brief 通过检查 alpha 值来计算遮罩

			   有时无法给出快速近似，需要通过绘制覆盖层并测试结果来计算遮罩。

			   当有一个有效的 maskHint() 时，只会检查该近似范围内的像素。
			 */
		AlphaMask
	};

	/*!
	   \brief 渲染模式

		为了计算 alpha 遮罩，覆盖层已经被绘制到一个临时的 QImage 上。
		可以避免两次渲染覆盖层，而是复制这个缓冲区来绘制覆盖层。

	   在使用光栅绘制引擎（QWS、Windows）的图形系统上，这通常只意味着复制一些内存。
	   在 X11 上，这会生成一个昂贵的操作来构建一个像素图，对于简单的覆盖层可能不推荐。

		\note 当 maskMode() != AlphaMask 时，渲染模式无效。
	  */
	enum RenderMode
	{
		//! 使用光栅绘制引擎时复制缓冲区。（默认）
		AutoRenderMode,

		//! 总是复制缓冲区
		CopyAlphaMask,

		//! 从不复制缓冲区
		DrawOverlay
	};

public:
	explicit DAAbstractWidgetOverlay(QWidget* parent);
	~DAAbstractWidgetOverlay();

	void setMaskMode(MaskMode mode);
	MaskMode getMaskMode() const;

	void setRenderMode(RenderMode mode);
	RenderMode getRenderMode() const;

	virtual bool eventFilter(QObject* object, QEvent* event) override;

public:
	/**
	 * @brief 矩形的遮罩区域计算
	 * @param r 矩形
	 * @param penWidth 画笔宽度
	 * @return
	 */
	static QRegion maskRegion(const QRect& r, int penWidth);
	/**
	 * @brief 针对水平线或竖直线的遮罩区域计算
	 * @param VOrHLine 水平线或竖直线
	 * @param penWidth 画笔的宽度
	 * @return
	 */
	static QRegion maskRegionVOrHLine(const QLine& VOrHLine, int penWidth);

protected:
	/**
	 * @brief 绘制遮罩层的主要业务实现函数
	 *
	 * @param painter
	 */
	virtual void drawOverlay(QPainter* painter) const = 0;

	/**
	 * @brief 计算遮罩区的近似值
	 *
	 *- MaskHint
	 *  遮罩建议区域。
	 *
	 *- AlphaMask
	 *  用于加速从非透明像素计算掩码的算法。
	 *
	 *- NoMask
	 *  不使用遮罩。
	 *
	 * 默认实现返回一个无效区域，表示没有建议区域。
	 *
	 * @return 遮罩的建议区域
	 */
	virtual QRegion maskHint() const;

	virtual void paintEvent(QPaintEvent* event) override;
	virtual void resizeEvent(QResizeEvent* event) override;
public Q_SLOTS:
	void updateOverlay();

private:
	void updateMask();
	void draw(QPainter* painter) const;
};
}  // end ns da
#endif  // DAWIDGETOVERLAY_H
