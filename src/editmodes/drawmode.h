#include "abstracteditmode.h"

class DrawMode : public AbstractDrawMode
{
	DEFINE_CLASS (DrawMode, AbstractDrawMode)
	bool _rectdraw;

public:
	DrawMode (GLRenderer* renderer);

	virtual void render (QPainter& painter) const override;
	virtual EditModeType type() const override;
	virtual bool preAddVertex (Vertex const& pos) override;
	virtual bool mouseReleased (MouseEventData const& data) override;
	virtual bool mouseMoved (QMouseEvent*) override;

private:
	void updateRectVerts();
};
