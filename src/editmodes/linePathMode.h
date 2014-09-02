#pragma once
#include "abstractEditMode.h"

class LinePathMode : public AbstractDrawMode
{
	DEFINE_CLASS (LinePathMode, AbstractDrawMode)

public:
	LinePathMode (GLRenderer* renderer);

	void render (QPainter& painter) const override;
	EditModeType type() const override { return EditModeType::LinePath; }
	bool mouseReleased (MouseEventData const& data) override;
	bool preAddVertex (Vertex const& pos) override;
	bool keyReleased (QKeyEvent*) override;
	void endDraw();
};

