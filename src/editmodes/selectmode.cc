#include <QMouseEvent>
#include "../glRenderer.h"
#include "selectmode.h"

SelectMode::SelectMode (GLRenderer* renderer) :
	Super (renderer) {}

EditModeType SelectMode::type() const
{
	return EditModeType::Select;
}

void SelectMode::mouseReleased (MouseEventData const& data)
{
	if (not data.mouseMoved)
		_rangepick = false;

	if (not _rangepick)
		_addpick = (data.keymods & Qt::ControlModifier);

	if (not data.mouseMoved || _rangepick)
		renderer()->pick (data.ev->x(), data.ev->y());
}

void SelectMode::mousePressed (MouseEventData const& data)
{
	if (data.ev->modifiers() & Qt::ControlModifier)
	{
		_rangepick = true;
		_rangeStart.setX (data.ev->x());
		_rangeStart.setY (data.ev->y());
		_addpick = (data.keymods & Qt::AltModifier);
		data.ev->accept();
	}
}