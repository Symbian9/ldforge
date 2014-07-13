/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QMouseEvent>
#include "selectMode.h"
#include "../glRenderer.h"
#include "../addObjectDialog.h"
#include "../mainWindow.h"
#include "../glRenderer.h"

SelectMode::SelectMode (GLRenderer* renderer) :
	Super (renderer),
	_rangepick (false) {}

EditModeType SelectMode::type() const
{
	return EditModeType::Select;
}

void SelectMode::render (QPainter& painter) const
{
	// If we're range-picking, draw a rectangle encompassing the selection area.
	if (_rangepick)
	{
		int x0 = _rangeStart.x(),
			y0 = _rangeStart.y(),
			x1 = renderer()->mousePosition().x(),
			y1 = renderer()->mousePosition().y();

		QRect rect (x0, y0, x1 - x0, y1 - y0);
		QColor fillColor = (_addpick ? "#40FF00" : "#00CCFF");
		fillColor.setAlphaF (0.2f);
		painter.setPen (QPen (QColor (0, 0, 0, 208), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.setBrush (QBrush (fillColor));
		painter.drawRect (rect);
	}
}

bool SelectMode::mouseReleased (MouseEventData const& data)
{
	if (Super::mouseReleased (data))
		return true;

	if (data.releasedButtons & Qt::LeftButton)
	{
		if (not data.mouseMoved)
			_rangepick = false;

		if (not _rangepick)
			_addpick = (data.keymods & Qt::ControlModifier);

		if (not data.mouseMoved or _rangepick)
		{
			QRect area;
			int const mx = data.ev->x();
			int const my = data.ev->y();

			if (not _rangepick)
			{
				area = QRect (mx, my, 1, 1);
			}
			else
			{
				int const x = min (_rangeStart.x(), mx);
				int const y = min (_rangeStart.y(), my);
				int const width = abs (_rangeStart.x() - mx);
				int const height = abs (_rangeStart.y() - my);
				area = QRect (x, y, width, height);
			}

			renderer()->pick (area, _addpick);
		}

		_rangepick = false;
		return true;
	}

	return false;
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

bool SelectMode::mouseMoved (QMouseEvent*)
{
	return _rangepick;
}
