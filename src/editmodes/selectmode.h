#include "abstracteditmode.h"

class SelectMode : public AbstractSelectMode
{
	QPoint _rangeStart;
	bool _rangepick;
	bool _addpick;

	DEFINE_CLASS (SelectMode, AbstractSelectMode)

public:
	SelectMode (GLRenderer* renderer);

	virtual void render (QPainter& painter) const override;
	virtual bool mousePressed (QMouseEvent* ev);
	virtual bool mouseReleased (MouseEventData const& data) override;
	virtual bool mouseDoubleClicked (QMouseEvent* ev);
	virtual bool mouseMoved (QMouseEvent*) override;
	virtual EditModeType type() const override;
};