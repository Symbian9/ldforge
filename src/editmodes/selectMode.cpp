/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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
#include "../canvas.h"
#include "../mainwindow.h"
#include "../ldDocument.h"

SelectMode::SelectMode (Canvas* canvas) :
    Super (canvas),
	m_rangepick (false) {}

EditModeType SelectMode::type() const
{
	return EditModeType::Select;
}

void SelectMode::render (QPainter& painter) const
{
	// If we're range-picking, draw a rectangle encompassing the selection area.
	if (m_rangepick)
	{
		int x0 = m_rangeStart.x(),
			y0 = m_rangeStart.y(),
			x1 = renderer()->mousePosition().x(),
			y1 = renderer()->mousePosition().y();

		QRect rect (x0, y0, x1 - x0, y1 - y0);
		QColor fillColor = (m_addpick ? "#40FF00" : "#00CCFF");
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
			m_rangepick = false;

		if (not m_rangepick)
			m_addpick = (data.keymods & Qt::ControlModifier);

		if (not data.mouseMoved or m_rangepick)
		{
			QRect area;
			int const mx = data.ev->x();
			int const my = data.ev->y();

			if (not m_rangepick)
			{
				area = QRect (mx, my, 1, 1);
			}
			else
			{
				int const x = qMin (m_rangeStart.x(), mx);
				int const y = qMin (m_rangeStart.y(), my);
				int const width = qAbs (m_rangeStart.x() - mx);
				int const height = qAbs (m_rangeStart.y() - my);
				area = QRect (x, y, width, height);
			}

			renderer()->pick (area, m_addpick);
		}

		m_rangepick = false;
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
		m_rangepick = true;
		m_rangeStart.setX (ev->x());
		m_rangeStart.setY (ev->y());
		m_addpick = (ev->modifiers() & Qt::AltModifier);
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
		currentDocument()->clearSelection();
		LDObject* obj = renderer()->pickOneObject (ev->x(), ev->y());

		if (obj)
		{
			// TODO:
			m_window->endAction();
			return true;
		}
	}

	return false;
}

bool SelectMode::mouseMoved (QMouseEvent*)
{
	return m_rangepick;
}
