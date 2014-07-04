#include "abstracteditmode.h"

class SelectMode : public AbstractSelectMode
{
	QPoint _rangeStart;
	bool _rangepick;
	bool _addpick;

	DEFINE_CLASS (SelectMode, AbstractSelectMode)

public:
	SelectMode (GLRenderer* renderer);

	virtual void mouseReleased (MouseEventData const& data) override;
	virtual EditModeType type() const override;
};