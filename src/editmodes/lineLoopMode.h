#pragma once
#include "abstractEditMode.h"

class LineLoopMode : public AbstractDrawMode
{
	DEFINE_CLASS (LineLoopMode, AbstractDrawMode)

public:
	LineLoopMode (GLRenderer* renderer);

	void render (QPainter& painter) const override;
	EditModeType type() const override { return EditModeType::LineLoop; }
	bool mouseReleased (MouseEventData const& data) override;
	bool preAddVertex (Vertex const& pos) override;
	bool keyReleased (QKeyEvent*) override;
	void endDraw();
};

