#include <QMouseEvent>
#include "../glRenderer.h"
#include "selectmode.h"

SelectMode::SelectMode (GLRenderer* renderer) :
	Super (renderer) {}

EditModeType SelectMode::type() const
{
	return EditModeType::Select;
}

bool SelectMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (not data.mouseMoved)
		_rangepick = false;

	if (not _rangepick)
		_addpick = (data.keymods & Qt::ControlModifier);

	if (not data.mouseMoved || _rangepick)
		renderer()->pick (data.ev->x(), data.ev->y());

	_rangepick = false;
}

bool SelectMode::mousePressed (QMouseEvent* ev)
{
	if (Super::mousePressed (ev))
		return true;

	if (ev->modifiers() & Qt::ControlModifier)
	{
		_rangepick = true;
		_rangeStart.setX (ev->x());
		_rangeStart.setY (ev->y());
		_addpick = (ev->modifiers() & Qt::AltModifier);
		return true;
	}

	return false;
}

bool SelectMode::mouseDoubleClicked (QMouseEvent* ev)
{
	if (Super::mouseDoubleClicked (ev))
		return true;

	if (ev->buttons() & Qt::LeftButton)
	{
		renderer()->document()->clearSelection();
		LDObjectPtr obj = renderer()->pickOneObject (ev->x(), ev->y());

		if (obj != null)
		{
			AddObjectDialog::staticDialog (obj->type(), obj);
			g_win->endAction();
			return true;
		}
	}

	return false;
}
